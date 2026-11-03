"""Builds the client patch archive from edits.json and installs it into a client.

    python tools/clientpatch/build.py build [--dbc DIR] [--client DIR] [--out DIR]
    python tools/clientpatch/build.py install [--client DIR] [--out DIR]
    python tools/clientpatch/build.py uninstall [--client DIR]
    python tools/clientpatch/build.py check [--out DIR]
    python tools/clientpatch/build.py sql [--dbc DIR]      write the server SQL for spell clones (`server_sql`)

The client patches each file only from update archives of the chain that holds it: "base" (Data/) or the locale
(Data/<locale>/). So the build writes one update archive per chain that has edits, each tagged with its chain.

Two kinds of edit. A `dbc` edit changes one cell of a table from the extracted server game data (`--dbc`, else
MOP_DBC_DIR, else <repo>/gamedata/dbc), which is the client's own latest data. A `text` edit replaces one exact
line in a client file (interface Lua and XML), read from the client's own archive chain with Blizzard's delta
patches applied (`--client`, else MOP_CLIENT_DIR, else the mop-clean folder next to the repo). A `spell` edit
clones a client spell to a new id with its own name, tooltip and icon (see spells.py); the server gets the same
spell from the generated `server_sql`. A `row` edit copies a row of a table (WDBC, or WDB2 without an id index
such as the spell visual tables) to a new id and sets named columns on the copy; rows are added in manifest order,
so later ones can point at earlier ones. Each edit names what it expects to replace, so a change in the source fails
the build instead of patching the wrong thing.
"""
import argparse
import json
import os
import shutil
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(HERE))

import client  # noqa: E402
import mpq  # noqa: E402
import spells  # noqa: E402
from dbcfile import Dbc, to_cell  # noqa: E402
from tools.dbc.schemas import get_schema  # noqa: E402

MANIFEST = HERE / 'edits.json'


def load_manifest(path: Path = MANIFEST) -> dict:
    return json.loads(path.read_text(encoding='utf-8'))


def patch_dbc(raw: bytes, schema, edits) -> bytes:
    magic, count, fields, record_size, _ = struct.unpack_from('<4sIIII', raw)
    if magic != b'WDBC':
        raise ValueError(schema.filename + ': only WDBC tables are supported')
    if record_size != fields * 4 or fields != len(schema.fields):
        raise ValueError(schema.filename + ': schema does not match the file layout')
    data = bytearray(raw)
    rows = {}
    for row in range(count):
        offset = 20 + row * record_size
        rows[struct.unpack_from('<I', data, offset + schema.key_index * 4)[0]] = offset
    for edit in edits:
        if edit['id'] not in rows:
            raise KeyError('%s: no row %d' % (schema.filename, edit['id']))
        column = schema.field_names.index(edit['field'])
        offset = rows[edit['id']] + column * 4
        current = struct.unpack_from('<I', data, offset)[0]
        if current != edit['from']:
            raise ValueError('%s row %d %s is %d, expected %d' % (schema.filename, edit['id'], edit['field'],
                                                                  current, edit['from']))
        struct.pack_into('<I', data, offset, edit['to'])
    return bytes(data)


def patch_text(raw: bytes, path: str, edits) -> bytes:
    text = raw.decode('utf-8')
    for edit in edits:
        count = text.count(edit['from'])
        if count != 1:
            raise ValueError('%s: expected "%s" once, found it %d times' % (path, edit['from'], count))
        text = text.replace(edit['from'], edit['to'])
    return text.encode('utf-8')


# A Blizzard addon's .toc.sig is a 276-byte signed hash of the addon's files; an edited file makes it fail and
# the client refuses the addon as CORRUPT. A signature of any other size counts as absent, so the addon loads
# unsigned instead.
UNSIGNED_SIG = b'unsigned by tools/clientpatch\n'


def unsigned_sig_path(path: str):
    parts = path.split('\\')
    if len(parts) > 3 and parts[0].lower() == 'interface' and parts[1].lower() == 'addons' \
            and parts[2].lower().startswith('blizzard_'):
        return '\\'.join(parts[:3] + [parts[2] + '.toc.sig'])
    return None


def archive_names(manifest: dict) -> dict:
    """Chain -> archive path under Data/, for both chains, so install and uninstall can clear stale ones."""
    build, locale = manifest['build'], manifest['locale']
    return {'base': 'wow-update-base-%d.MPQ' % build, locale: '%s\\wow-update-%s-%d.MPQ' % (locale, locale, build)}


def group_edits(manifest: dict, kinds) -> dict:
    by_file = {}
    for edit in manifest['edits']:
        kind = edit.get('type', 'dbc')
        if kind in kinds:
            by_file.setdefault((kind, edit.get('file', '')), []).append(edit)
    return by_file


def table_path(dbc_dir: Path, filename: str, locale: str) -> Path:
    """The extracted game data keeps DB2 files in a locale folder."""
    path = dbc_dir / filename
    return path if path.exists() else dbc_dir / locale / filename


def add_rows(raw: bytes, schema, edits) -> bytes:
    table = Dbc(raw, schema.filename)
    kinds = {f.name: f.kind for f in schema.fields}
    for edit in edits:
        source = table.find(0, edit['from'])
        if not source:
            raise KeyError('%s: no row %d to copy' % (schema.filename, edit['from']))
        row = list(source[0])
        row[0] = edit['id']
        for field, value in edit.get('set', {}).items():
            if field not in kinds:
                raise KeyError('%s: no column %s' % (schema.filename, field))
            row[schema.field_names.index(field)] = to_cell(value, kinds[field])
        table.append(row)
    return table.to_bytes()


def build_tables(manifest: dict, dbc_dir: Path):
    """Table file name -> patched bytes, and the rows each spell clone added (for the server SQL)."""
    raws = {}
    for (_, filename), edits in group_edits(manifest, ('dbc',)).items():
        schema = get_schema(filename)
        if not schema:
            raise KeyError('no schema for ' + filename)
        raws[filename] = patch_dbc((dbc_dir / filename).read_bytes(), schema, edits)
    for (_, filename), edits in group_edits(manifest, ('row',)).items():
        schema = get_schema(filename)
        if not schema:
            raise KeyError('no schema for ' + filename)
        raw = raws.get(filename) or table_path(dbc_dir, filename, manifest['locale']).read_bytes()
        raws[filename] = add_rows(raw, schema, edits)
    clones = [edit for edit in manifest['edits'] if edit.get('type') == 'spell']
    clone_rows = []
    if clones:
        tables = {}
        names = ['Spell', 'SpellMisc', 'SpellEffect']
        if any('skill_line' in edit for edit in clones):
            names += ['SkillLineAbility', 'SkillLineAbilitySortedSpell']
        if any('cooldown' in edit for edit in clones):
            names.append('SpellCooldowns')
        for name in names:
            filename = name + '.dbc'
            tables[name] = Dbc(raws.get(filename) or (dbc_dir / filename).read_bytes(), filename)
        clone_rows = [spells.clone(edit, tables) for edit in clones]
        for name, table in tables.items():
            raws[name + '.dbc'] = table.to_bytes()
    return raws, clone_rows


def build_archives(manifest: dict, dbc_dir: Path, client_dir: Path) -> dict:
    """Chain -> archive bytes, for each chain that has edits."""
    names = archive_names(manifest)
    # Read the client's own archives, never ours, so rebuilding is repeatable.
    skip = tuple(Path(name.replace('\\', '/')).name for name in names.values())
    locale = manifest['locale']

    def chain(path):
        return client.chain_of(str(client_dir), path, locale, skip)

    files = {}
    for (kind, filename), edits in sorted(group_edits(manifest, ('text',)).items()):
        current = client.read_current(str(client_dir), filename, skip=skip)
        files[filename] = patch_text(current, filename, edits)
        sig = unsigned_sig_path(filename)
        if sig:
            files[sig] = UNSIGNED_SIG
    tables, _ = build_tables(manifest, dbc_dir)
    for filename, raw in tables.items():
        files['DBFilesClient\\' + filename] = raw

    by_chain = {}
    for path, data in files.items():
        by_chain.setdefault(chain(path), {})[path] = data
    return {name: mpq.build(chain_files, name) for name, chain_files in by_chain.items()}


def default_client() -> Path:
    if os.environ.get('MOP_CLIENT_DIR'):
        return Path(os.environ['MOP_CLIENT_DIR'])
    # The client sits next to the main checkout; agent worktrees are two levels further down.
    for base in (ROOT, *ROOT.parents):
        if (base / 'mop-clean' / 'Data').is_dir():
            return base / 'mop-clean'
    return ROOT / 'mop-clean'


def default_dbc() -> Path:
    if os.environ.get('MOP_DBC_DIR'):
        return Path(os.environ['MOP_DBC_DIR'])
    # Game data is shared next to the main checkout, like the client.
    for base in (ROOT, *ROOT.parents):
        if (base / 'gamedata' / 'dbc').is_dir():
            return base / 'gamedata' / 'dbc'
    return ROOT / 'gamedata' / 'dbc'


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['build', 'install', 'uninstall', 'check', 'sql'])
    parser.add_argument('--dbc', type=Path, default=default_dbc())
    parser.add_argument('--out', type=Path, default=ROOT / 'build/clientpatch')
    parser.add_argument('--client', type=Path, default=default_client())
    args = parser.parse_args(argv)

    manifest = load_manifest()
    names = archive_names(manifest)

    def out_path(chain):
        return args.out / Path(names[chain].replace('\\', '/')).name

    if args.command == 'build':
        args.out.mkdir(parents=True, exist_ok=True)
        archives = build_archives(manifest, args.dbc, args.client)
        for chain in names:
            path = out_path(chain)
            if chain in archives:
                path.write_bytes(archives[chain])
                print('built %s (%s chain, %d bytes)' % (path, chain, path.stat().st_size))
            elif path.exists():
                path.unlink()
        print('%d edits' % len(manifest['edits']))
    elif args.command == 'sql':
        clones = [edit for edit in manifest['edits'] if edit.get('type') == 'spell']
        _, rows = build_tables(manifest, args.dbc)
        target = ROOT / manifest['server_sql']
        target.write_text(spells.server_sql(clones, rows), encoding='utf-8')
        print('wrote %s (%d spells)' % (target, len(clones)))
    elif args.command == 'check':
        for chain in names:
            if out_path(chain).exists():
                print('%s: %s' % (out_path(chain).name, ', '.join(mpq.listfile(out_path(chain).read_bytes()))))
    elif args.command in ('install', 'uninstall'):
        if not args.client or not (args.client / 'Data').is_dir():
            parser.error('--client must be a WoW folder with a Data directory')
        for chain in names:
            target = args.client / 'Data' / names[chain].replace('\\', '/')
            if args.command == 'install' and out_path(chain).exists():
                shutil.copyfile(out_path(chain), target)
                print('installed ' + str(target))
            elif target.exists():
                target.unlink()
                print('removed ' + str(target))
    return 0


if __name__ == '__main__':
    sys.exit(main())

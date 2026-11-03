"""Builds the client patch archive from edits.json and installs it into a client.

    python tools/clientpatch/build.py build [--dbc DIR] [--out DIR]
    python tools/clientpatch/build.py install --client DIR [--out DIR]
    python tools/clientpatch/build.py uninstall --client DIR
    python tools/clientpatch/build.py check [--out DIR]

Two kinds of edit. A `dbc` edit changes one cell of a table from the extracted server game data (`--dbc`, else
MOP_DBC_DIR, else <repo>/gamedata/dbc), which is the client's own latest data. A `text` edit replaces one exact
line in a client file (interface Lua and XML), read from the client's own archive chain with Blizzard's delta
patches applied (`--client`, else MOP_CLIENT_DIR, else the mop-clean folder next to the repo). Each edit names
what it expects to replace, so a change in the source fails the build instead of patching the wrong thing.
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


def build_archive(manifest: dict, dbc_dir: Path, client_dir: Path) -> bytes:
    by_file = {}
    for edit in manifest['edits']:
        by_file.setdefault((edit.get('type', 'dbc'), edit['file']), []).append(edit)
    files = {}
    for (kind, filename), edits in sorted(by_file.items()):
        if kind == 'text':
            # Read the client's current copy, never our own archive, so rebuilding is repeatable.
            current = client.read_current(str(client_dir), filename, skip=(manifest['archive'],))
            files[filename] = patch_text(current, filename, edits)
            continue
        schema = get_schema(filename)
        if not schema:
            raise KeyError('no schema for ' + filename)
        files['DBFilesClient\\' + filename] = patch_dbc((dbc_dir / filename).read_bytes(), schema, edits)
    return mpq.build(files)


def default_client() -> Path:
    if os.environ.get('MOP_CLIENT_DIR'):
        return Path(os.environ['MOP_CLIENT_DIR'])
    # The client sits next to the main checkout; agent worktrees are two levels further down.
    for base in (ROOT, *ROOT.parents):
        if (base / 'mop-clean' / 'Data').is_dir():
            return base / 'mop-clean'
    return ROOT / 'mop-clean'


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['build', 'install', 'uninstall', 'check'])
    parser.add_argument('--dbc', type=Path, default=Path(os.environ.get('MOP_DBC_DIR', ROOT / 'gamedata/dbc')))
    parser.add_argument('--out', type=Path, default=ROOT / 'build/clientpatch')
    parser.add_argument('--client', type=Path, default=default_client())
    args = parser.parse_args(argv)

    manifest = load_manifest()
    archive_name = manifest['archive']
    built = args.out / archive_name

    if args.command == 'build':
        args.out.mkdir(parents=True, exist_ok=True)
        built.write_bytes(build_archive(manifest, args.dbc, args.client))
        print('built %s (%d bytes, %d edits)' % (built, built.stat().st_size, len(manifest['edits'])))
    elif args.command == 'check':
        archive = built.read_bytes()
        print('%s: %s' % (built.name, ', '.join(mpq.listfile(archive))))
    elif args.command in ('install', 'uninstall'):
        if not args.client or not (args.client / 'Data').is_dir():
            parser.error('--client must be a WoW folder with a Data directory')
        target = args.client / 'Data' / archive_name
        if args.command == 'install':
            shutil.copyfile(built, target)
            print('installed ' + str(target))
        elif target.exists():
            target.unlink()
            print('removed ' + str(target))
    return 0


if __name__ == '__main__':
    sys.exit(main())

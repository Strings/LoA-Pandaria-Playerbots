"""Logical capture into an isolated MySQL instance, cold-file restore, migration checks."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import queue
from pathlib import Path
import re
import shutil
import socket
import subprocess
import threading
import time
import uuid

REPO = Path(__file__).resolve().parents[2]
IDENT = re.compile(r'^[a-z][a-z0-9_]{0,63}$')
MARKER = 'ultimatemop-save-safety-v1'


def identifier(value):
    if not isinstance(value, str) or not IDENT.fullmatch(value):
        raise ValueError('invalid database/table identifier: ' + str(value))
    return value


def owned_path(value):
    path = Path(value).resolve()
    if path == REPO or REPO not in path.parents:
        raise ValueError('writable path must be inside this worktree: ' + str(path))
    return path


def run(args, *, env=None, input=None, stdout=subprocess.PIPE):
    result = subprocess.run([str(a) for a in args], input=input, stdout=stdout,
                            stderr=subprocess.PIPE, env=env)
    if result.returncode:
        raise RuntimeError(Path(args[0]).name + ' failed: ' + result.stderr.decode(errors='replace')[-2000:])
    return result.stdout


class Mysql:
    def __init__(self, config, *, isolated=False):
        self.bin = Path(config['mysql_bin'])
        self.host = '127.0.0.1' if isolated else config.get('host', '127.0.0.1')
        self.port = int(config['capture_port'] if isolated else config.get('port', 3306))
        self.user = 'root' if isolated else config['user']
        self.lock_timeout = int(config.get('source_lock_timeout_seconds', 30))
        self.env = os.environ.copy()
        self.env.pop('MYSQL_PWD', None)
        if not isolated:
            self.env['MYSQL_PWD'] = os.environ[config.get('password_env', 'MOP_DB_PASSWORD')]

    def args(self, program):
        suffix = '.exe' if os.name == 'nt' else ''
        return [self.bin / (program + suffix), '--no-defaults', '--protocol=TCP',
                '-h', self.host, '-P', str(self.port), '-u', self.user]

    def sql(self, sql, database=None):
        args = self.args('mysql') + ['--batch', '--skip-column-names', '--binary-mode']
        if database:
            args += [identifier(database)]
        return run(args, env=self.env, input=sql.encode() if isinstance(sql, str) else sql)

    def dump(self, databases, destination, *, tables=None, include_databases=False):
        args = self.args('mysqldump') + ['--single-transaction', '--quick', '--no-tablespaces',
                '--set-gtid-purged=OFF', '--skip-comments', '--skip-dump-date',
                '--skip-add-locks', '--skip-lock-tables', '--hex-blob', '--order-by-primary',
                '--triggers', '--routines', '--events', '--skip-add-drop-table']
        if include_databases:
            args += ['--databases']
        args += [identifier(d) for d in databases]
        if tables is not None:
            args += [identifier(t) for t in tables]
        # This fork still has MyISAM tables: one transaction alone is insufficient.
        names = []
        for database in databases:
            for line in self.sql("SHOW FULL TABLES WHERE Table_type='BASE TABLE';", database).decode().splitlines():
                table = line.split('\t')[0]
                if tables is None or table in tables:
                    names.append('`' + identifier(database) + '`.`' + identifier(table) + '` READ')
        if not names:
            raise ValueError('no base tables selected for capture')
        holder = subprocess.Popen([str(a) for a in self.args('mysql') + ['--batch', '--skip-column-names', '--unbuffered']],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=self.env)
        ready = queue.Queue()
        threading.Thread(target=lambda: ready.put(holder.stdout.readline()), daemon=True).start()
        try:
            holder.stdin.write(('SET SESSION lock_wait_timeout=' + str(self.lock_timeout) + '; LOCK TABLES ' + ','.join(names) + "; SELECT 'SAVE_LOCKED';\n").encode())
            holder.stdin.flush()
            if ready.get(timeout=self.lock_timeout + 5).strip() != b'SAVE_LOCKED':
                raise RuntimeError('could not acquire source table read locks')
            with destination.open('wb') as stream:
                run(args, env=self.env, stdout=stream)
        finally:
            holder.terminate()  # This invocation's client only; disconnect releases all read locks.
            holder.wait(timeout=30)


def hash_file(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''):
            digest.update(block)
    return digest.hexdigest()


def files_manifest(root):
    result = {}
    for path in sorted(root.rglob('*')):
        if path.is_symlink() or (path.exists() and root.resolve() not in path.resolve().parents):
            raise ValueError('links are not supported in snapshot data')
        if path.is_file():
            result[path.relative_to(root).as_posix()] = hash_file(path)
    return result


def isolated_start(config, datadir, initialize=False):
    client = Mysql(config, isolated=True)
    if not 1024 <= client.port <= 65535 or client.port == int(config.get('port', 3306)):
        raise ValueError('capture_port must be an unused, separate unprivileged port')
    with socket.socket() as probe:
        if probe.connect_ex(('127.0.0.1', client.port)) == 0:
            raise ValueError('capture_port is occupied; no existing instance will be touched')
    executable = client.bin / ('mysqld.exe' if os.name == 'nt' else 'mysqld')
    args = [executable, '--no-defaults', '--basedir=' + str(client.bin.parent), '--datadir=' + str(datadir)]
    if initialize:
        run(args + ['--initialize-insecure'], env=client.env)
    log = (datadir.parent / 'mysqld.log').open('ab')
    process = subprocess.Popen([str(a) for a in args + ['--console', '--bind-address=127.0.0.1',
                               '--port=' + str(client.port), '--mysqlx=0', '--skip-log-bin',
                               '--innodb-buffer-pool-size=64M']], stdout=log, stderr=log, env=client.env)
    log.close()
    deadline = time.monotonic() + float(config.get('start_timeout_seconds', 120))
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError('isolated mysqld exited; see ' + str(datadir.parent / 'mysqld.log'))
        try:
            actual = Path(client.sql('SELECT @@datadir;').decode().strip()).resolve()
            if actual != datadir.resolve():
                raise ValueError('connected instance has the wrong datadir')
            return client, process
        except RuntimeError:
            time.sleep(.25)
    process.terminate()
    process.wait(timeout=30)
    raise RuntimeError('isolated mysqld startup timed out')


def isolated_stop(client, process):
    # The process must still be ours before using its port, never stop a shared service.
    if process.poll() is None:
        run(client.args('mysqladmin') + ['shutdown'], env=client.env)
        process.wait(timeout=120)


def snapshot(config):
    if int(config.get('retention', 7)) < 1:
        raise ValueError('retention must be positive')
    root = owned_path(config['snapshot_root'])
    root.mkdir(parents=True, exist_ok=True)
    name = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8]
    staging = root / ('.pending-' + name)
    staging.mkdir()
    data = staging / 'data'
    data.mkdir()
    source = Mysql(config)
    databases = [identifier(d) for d in config['databases']]
    dump = staging / 'capture.sql'
    source.dump(databases, dump, include_databases=True)
    client, process = isolated_start(config, data, initialize=True)
    try:
        with dump.open('rb') as stream:
            result = subprocess.run([str(a) for a in client.args('mysql') + ['--binary-mode']],
                                    stdin=stream, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=client.env)
        if result.returncode:
            raise RuntimeError('isolated import failed: ' + result.stderr.decode(errors='replace')[-2000:])
        counts = {}
        for database in databases:
            tables = client.sql('SHOW TABLES;', database).decode().splitlines()
            counts[database] = {t: int(client.sql('SELECT COUNT(*) FROM `' + identifier(t) + '`;', database)) for t in tables}
    finally:
        isolated_stop(client, process)
    dump.unlink()  # Only the temporary dump created by this invocation.
    manifest = {'format': MARKER, 'databases': databases, 'row_counts': counts,
                'created_utc': name, 'mysql_version': source.sql('SELECT VERSION();').decode().strip(),
                'files': files_manifest(data)}
    (staging / 'manifest.json').write_text(json.dumps(manifest, indent=2))
    destination = root / name
    staging.rename(destination)
    rotate(root, int(config.get('retention', 7)))
    return destination


def rotate(root, keep):
    if keep < 1:
        raise ValueError('retention must be positive')
    candidates = []
    for path in root.iterdir():
        if path.is_dir() and not path.name.startswith('.') and (path / 'manifest.json').is_file():
            meta = json.loads((path / 'manifest.json').read_text())
            if meta.get('format') == MARKER:
                candidates.append(path)
    for path in sorted(candidates, reverse=True)[keep:]:
        checked = owned_path(path)
        if checked.parent != root.resolve() or checked != path.absolute():
            raise ValueError('unsafe snapshot rotation path')
        shutil.rmtree(checked)  # Only this tool's marked snapshot directories.


def restore(snapshot_path, destination):
    source = Path(snapshot_path).resolve()
    meta = json.loads((source / 'manifest.json').read_text())
    if meta.get('format') != MARKER or files_manifest(source / 'data') != meta['files']:
        raise ValueError('snapshot integrity check failed')
    target = owned_path(destination)
    if target.exists():
        raise ValueError('restore destination must not exist; existing saves are never overwritten')
    shutil.copytree(source / 'data', target)
    if files_manifest(target) != meta['files']:
        raise ValueError('restored file hashes differ')
    return target


def validate_migration(sql, allowed):
    allowed = {identifier(t) for t in allowed}
    # Mask strings/comments before scanning identifiers and statement boundaries.
    pattern = r"'(?:\\.|''|[^'\\])*'|\"(?:\\.|\"\"|[^\"\\])*\"|--[^\n]*|\#[^\n]*|/\*[\s\S]*?\*/"
    if '/*!' in sql or '\\' in re.sub(pattern, '', sql):
        raise ValueError('executable comments/client commands are forbidden')
    masked = re.sub(pattern, "''", sql)
    # SOURCE is also a valid quoted column name; client directives are unquoted.
    client_code = re.sub(r'`(?:``|[^`])*`', 'identifier', masked)
    if re.search(r'(?:^|;)\s*SOURCE\b', client_code, re.I | re.M):
        raise ValueError('SQL client SOURCE commands are forbidden')
    masked = masked.replace('`', '')
    masked = masked.replace("''", ' ')
    if re.search(r'\b(USE|DATABASE|TRIGGER|PROCEDURE|FUNCTION|OUTFILE|INFILE|GRANT|REVOKE|RENAME)\b', masked, re.I):
        raise ValueError('migration must stay within explicitly allowed custom tables')
    if re.search(r'[a-z_]\w*\s*\.', masked, re.I):
        raise ValueError('qualified database names are forbidden')
    for statement in masked.split(';'):
        if not statement.strip():
            continue
        match = re.match(r'\s*(?:CREATE\s+TABLE\s+IF\s+NOT\s+EXISTS|ALTER\s+TABLE|INSERT\s+INTO|UPDATE|DELETE\s+FROM)\s+([a-z_]\w*)\b', statement, re.I)
        if not match or match[1].lower() not in allowed:
            raise ValueError('unsupported statement or table outside allowlist: ' + statement.strip()[:100])
        # No nested writes or multi-table update/delete escaping the target allowlist.
        if re.search(r'\b(JOIN|USING|CALL|PREPARE|EXECUTE|SELECT|REPLACE|DROP|TRUNCATE)\b', statement, re.I):
            raise ValueError('unsupported migration construct')
        if re.match(r'\s*UPDATE\b', statement, re.I) and ',' in statement[match.end():].split('SET', 1)[0].split('set', 1)[0]:
            raise ValueError('multi-table updates are forbidden')
    return sql


def migration_check(config, source, migrations, tables, seed=None):
    source = identifier(source)
    for path in migrations:
        validate_migration(path.read_text(encoding='utf-8-sig'), tables)
    if seed:
        validate_migration(seed.read_text(encoding='utf-8-sig'), tables)
    client = Mysql(config)
    scratch = identifier(source[:30] + '_check_' + uuid.uuid4().hex[:12])
    work = owned_path(config.get('check_root', REPO / 'build/migration-checks')) / scratch
    work.mkdir(parents=True)
    original = work / 'source.sql'
    client.dump([source], original)
    client.sql('CREATE DATABASE `' + scratch + '`;')
    try:
        with original.open('rb') as stream:
            result = subprocess.run([str(a) for a in client.args('mysql') + ['--binary-mode', scratch]],
                                    stdin=stream, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=client.env)
        if result.returncode:
            raise RuntimeError('save clone failed: ' + result.stderr.decode(errors='replace')[-2000:])
        core = [t for t in client.sql('SHOW TABLES;', scratch).decode().splitlines() if t not in tables]
        before = work / 'core-before.sql'
        client.dump([scratch], before, tables=core)
        for path in migrations:
            client.sql(path.read_bytes(), scratch)
        if seed:
            client.sql(seed.read_bytes(), scratch)
        first = work / 'first.sql'
        client.dump([scratch], first)
        for path in migrations:
            client.sql(path.read_bytes(), scratch)
        second, after = work / 'second.sql', work / 'core-after.sql'
        client.dump([scratch], second)
        client.dump([scratch], after, tables=core)
        if hash_file(first) != hash_file(second):
            raise ValueError('migration is not idempotent or changed saved rows on reapplication')
        if hash_file(before) != hash_file(after):
            raise ValueError('migration modified existing core tables')
        report = {'source': source, 'scratch': scratch, 'result': 'pass', 'core_tables_checked': len(core),
                  'save_sha256': hash_file(original), 'migrations': {str(p): hash_file(p) for p in migrations},
                  'idempotent_sha256': hash_file(second), 'seed': str(seed) if seed else None}
        (work / 'report.json').write_text(json.dumps(report, indent=2))
    finally:
        # A unique database created by this invocation, never an existing database.
        client.sql('DROP DATABASE `' + scratch + '`;')
    return work / 'report.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    sub = parser.add_subparsers(dest='action', required=True)
    sub.add_parser('snapshot')
    sub.add_parser('nightly')
    restore_parser = sub.add_parser('restore')
    restore_parser.add_argument('snapshot', type=Path)
    restore_parser.add_argument('destination', type=Path)
    check = sub.add_parser('check')
    check.add_argument('--source', required=True)
    check.add_argument('--table', action='append', required=True)
    check.add_argument('--seed', type=Path)
    check.add_argument('migration', type=Path, nargs='+')
    args = parser.parse_args()
    config = json.loads(args.config.read_text())
    if args.action == 'restore':
        print(restore(args.snapshot, args.destination))
    elif args.action == 'check':
        print(migration_check(config, args.source, args.migration, args.table, args.seed))
    else:
        while True:
            print(snapshot(config), flush=True)
            if args.action == 'snapshot':
                break
            time.sleep(float(config.get('snapshot_interval_seconds', 86400)))


if __name__ == '__main__':
    main()

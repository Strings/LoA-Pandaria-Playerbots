"""Checkpoint-based private fixtures; persistent bots; cold-file baseline restore."""
import argparse
from contextlib import contextmanager
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import socket
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
CATALOG = ROOT / 'sql/fixtures/characters.json'
SQL = ROOT / 'sql/fixtures/2026_11_04_codex_00_checkpoint_fixtures.sql'
PORTS = {'codex': (3744, 8105, 7898), 'claude': (3734, 8095, 7888), 'gemini': (3754, 8115, 7908)}


def load_catalog(path=CATALOG):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    rows = data['fixtures']
    if data.get('version') != 1 or sorted(row['level'] for row in rows) != [1, 30, 60, 85, 90]:
        raise ValueError('catalog must contain each of the five fixture levels exactly once')
    for key in ('id', 'name', 'checkpoint'):
        if len({row[key] for row in rows}) != len(rows):
            raise ValueError('duplicate fixture ' + key)
    for row in rows:
        if not re.fullmatch(r'[A-Za-z]{2,12}', row['name']) or not re.fullmatch(r'fixture-(1|30|60|85|90)', row['checkpoint']):
            raise ValueError('invalid fixture name/checkpoint')
        if row['checkpoint'] != 'fixture-' + str(row['level']) or row['race'] != 2 or row['class'] != 1:
            raise ValueError('shipped fixture checkpoint/identity mismatch')
    if set(data['limits']) != {'query_seconds', 'probe_seconds', 'client_seconds', 'logout_seconds', 'poll_seconds'} or any(not isinstance(v, (int, float)) or v <= 0 for v in data['limits'].values()):
        raise ValueError('fixture timing limits must be positive')
    return data


def owned_path(root, path):
    path = Path(path).resolve()
    path.relative_to(root.resolve() / 'build')
    if path == root.resolve() / 'build':
        raise ValueError('choose a directory below build')
    return path


class Environment:
    def __init__(self, runtime, mysql_bin=None, root=ROOT):
        self.root = root.resolve()
        self.runtime = owned_path(self.root, runtime)
        self.meta = json.loads((self.runtime / 'environment.json').read_text())
        self.agent = self.meta['agent']
        if self.agent not in PORTS or self.root.name != self.agent:
            raise ValueError('use your matching named agent worktree')
        self.databases = [name + '_' + self.agent for name in ('auth', 'characters', 'world', 'playerbots')]
        if self.meta.get('profile') != 'test' or self.meta.get('load') not in ('none', 'local') or self.meta.get('databases') != self.databases:
            raise ValueError('fixtures require matching private test databases, none/local load')
        if tuple(self.meta.get(key) for key in ('auth_port', 'world_port', 'soap_port')) != PORTS[self.agent]:
            raise ValueError('runtime ports do not match agent ownership')
        config = (self.runtime / 'worldserver.conf').read_text(encoding='utf-8-sig')
        connection = re.findall(r'^WorldDatabaseInfo\s*=\s*"([^"]+)"', config, re.M)
        if len(connection) != 1:
            raise ValueError('expected one database connection')
        self.host, self.port, self.user, self.password, database = connection[0].split(';')
        if self.host not in ('127.0.0.1', 'localhost') or database != self.databases[2]:
            raise ValueError('world config does not match private local metadata')
        for kind, expected in zip(('Login', 'Character', 'World', 'Playerbots'), self.databases):
            # Playerbots connection is configured in its own file on this fork.
            if kind == 'Playerbots':
                continue
            values = re.findall(r'^' + kind + r'DatabaseInfo\s*=\s*"([^"]+)"', config, re.M)
            if len(values) != 1 or values[0].split(';') != [self.host, self.port, self.user, self.password, expected]:
                raise ValueError('database connections disagree with private metadata')
        binary = mysql_bin or os.environ.get('MOP_MYSQL_BIN') or shutil.which('mysql')
        if not binary:
            raise ValueError('set MOP_MYSQL_BIN or --mysql-bin to the installed client directory')
        self.bin = Path(binary)
        if self.bin.is_file():
            self.bin = self.bin.parent
        self.catalog = load_catalog()
        self.limits = self.catalog['limits']

    def query(self, sql, database=None):
        database = database or self.databases[1]
        if database not in self.databases:
            raise ValueError('query must use a database owned by this agent')
        suffix = '.exe' if os.name == 'nt' else ''
        args = [str(self.bin / ('mysql' + suffix)), '--no-defaults', '--protocol=TCP', '--host=' + self.host,
                '--port=' + self.port, '--user=' + self.user, '--batch', '--raw', '--skip-column-names', database]
        result = subprocess.run(args, input=sql, text=True, encoding='utf-8', capture_output=True,
                                env=dict(os.environ, MYSQL_PWD=self.password), timeout=self.limits['query_seconds'])
        if result.returncode:
            raise RuntimeError('private fixture database operation failed; SQL/credentials omitted')
        return [line.split('\t') for line in result.stdout.splitlines()]

    def running(self):
        try:
            with socket.create_connection(('127.0.0.1', self.meta['world_port']), timeout=self.limits['probe_seconds']):
                return True
        except ConnectionRefusedError:
            return False
        except OSError as error:
            raise RuntimeError('world state is uncertain') from error

    @contextmanager
    def locked(self):
        path = self.root / 'build/checkpoint-fixtures.lock'
        with path.open('x') as stream:
            stream.write(str(os.getpid()))
        try:
            yield
        finally:
            path.unlink()

    def client(self, account, arguments, guid=None):
        env = dict(os.environ, MOP_HOST='127.0.0.1', MOP_AUTH_PORT=str(self.meta['auth_port']),
                   MOP_WORLD_PORT=str(self.meta['world_port']), MOP_ACCOUNT=account['username'],
                   MOP_PASSWORD=account['password'], HEADLESS_LOGOUT='1', PYTHONIOENCODING='utf-8')
        if guid is not None:
            env['MOP_CHAR_GUID'] = str(guid)
        else:
            env.pop('MOP_CHAR_GUID', None)
        result = subprocess.run([sys.executable, str(self.root / 'contrib/headless-client/headless.py')] + arguments,
                                env=env, capture_output=True, text=True, encoding='utf-8', errors='replace',
                                timeout=self.limits['client_seconds'])
        if result.returncode:
            raise RuntimeError('headless fixture operation failed; session packets/credentials omitted')
        return result.stdout

    def offline(self, guid):
        deadline = time.monotonic() + self.limits['logout_seconds']
        while time.monotonic() < deadline:
            if self.query('SELECT online FROM characters WHERE guid=' + str(guid) + ';') == [['0']]:
                return
            time.sleep(self.limits['poll_seconds'])
        raise ValueError('fixture is still online')


def account_for(env, create=False):
    path = env.runtime / 'checkpoint-fixture-account.json'
    account = json.loads(path.read_text()) if path.exists() else {
        'username': 'CHECKPOINT_' + env.agent.upper(), 'password': secrets.token_hex(12).upper()}
    if account['username'] != 'CHECKPOINT_' + env.agent.upper() or not re.fullmatch(r'[A-F0-9]{24}', account['password']):
        raise ValueError('credential file is not this tool\'s dedicated fixture identity')
    digest = hashlib.sha1((account['username'] + ':' + account['password']).encode('ascii')).hexdigest().upper()
    rows = env.query("SELECT id,sha_pass_hash FROM account WHERE username='" + account['username'] + "';", env.databases[0])
    if not rows:
        if not create:
            raise ValueError('provision the fixture account first')
        # Save the newly generated password before mutation so an interrupted bootstrap can resume.
        path.write_text(json.dumps(account, indent=2), encoding='utf-8')
        env.query("INSERT INTO account(username,sha_pass_hash,expansion) VALUES ('" + account['username'] + "','" + digest + "',4);", env.databases[0])
        rows = env.query("SELECT id,sha_pass_hash FROM account WHERE username='" + account['username'] + "';", env.databases[0])
    if len(rows) != 1 or rows[0][1] != digest:
        raise ValueError('existing fixture account differs; never replace its credentials')
    account['id'] = int(rows[0][0])
    owned_names = {row['name'].lower() for row in env.catalog['fixtures']}
    names = env.query('SELECT name FROM characters WHERE account=' + str(account['id']) + ';')
    if any(row[0].lower() not in owned_names for row in names):
        raise ValueError('dedicated fixture account contains an unmanaged character')
    if create:
        # Checkpoint apply is self-only in headless mode; only this isolated fixture account gets admin commands.
        env.query('INSERT INTO account_access(id,gmlevel,RealmID) VALUES (' + str(account['id']) + ',3,-1) ON DUPLICATE KEY UPDATE gmlevel=3;', env.databases[0])
        path.write_text(json.dumps(account, indent=2), encoding='utf-8')
    return account


def character(env, account, fixture):
    rows = env.query("SELECT guid,account,race,class,level,online FROM characters WHERE name='" + fixture['name'] + "' AND deleteDate IS NULL;")
    if not rows:
        return None
    row = rows[0]
    if len(rows) != 1 or [int(v) for v in row[1:4]] != [account['id'], fixture['race'], fixture['class']]:
        raise ValueError('fixture name belongs to another account/race/class')
    if row[5] != '0':
        raise ValueError('fixture is online; refusing to change it')
    return int(row[0])


def state(env, account, fixture, guid):
    row = env.query('SELECT level,race,class FROM characters WHERE guid=' + str(guid) + ';')[0]
    gear = env.query('SELECT c.slot,i.itemEntry,i.guid,t.RequiredLevel FROM character_inventory c JOIN item_instance i ON i.guid=c.item '
                     'JOIN `' + env.databases[2] + '`.item_template t ON t.entry=i.itemEntry WHERE c.guid=' + str(guid) + ' AND c.bag=0 AND c.slot<19 ORDER BY c.slot;')
    if [int(v) for v in row] != [fixture['level'], fixture['race'], fixture['class']]:
        raise ValueError('fixture level/identity differs: ' + fixture['id'])
    if len(gear) < fixture['minimum_equipped'] or not any(int(g[0]) == 15 for g in gear) or any(int(g[3]) > fixture['level'] for g in gear):
        raise ValueError('fixture equipment is incomplete or exceeds its level: ' + fixture['id'])
    return {'guid': guid, 'name': fixture['name'], 'level': fixture['level'], 'checkpoint': fixture['checkpoint'],
            'race': fixture['race'], 'class': fixture['class'], 'equipment': [[int(v) for v in g] for g in gear]}


def verify(env, account=None):
    account = account or account_for(env)
    saved = json.loads((env.runtime / 'fixture-manifest.json').read_text())
    if saved.get('version') != 1 or saved.get('agent') != env.agent or saved.get('account_id') != account['id']:
        raise ValueError('manifest ownership/version differs')
    for fixture in env.catalog['fixtures']:
        guid = character(env, account, fixture)
        if guid is None or state(env, account, fixture, guid) != saved['fixtures'].get(fixture['id']):
            raise ValueError('fixture state differs; restore baseline or explicitly provision --reset')
    return saved


def provision(env, reset=False):
    if not env.running():
        raise ValueError('start your private world before checkpoint provisioning')
    if env.meta['load'] != 'none':
        raise ValueError('provision fixtures in test/none after bot generation')
    account = account_for(env, create=True)
    previous_path = env.runtime / 'fixture-manifest.json'
    previous = json.loads(previous_path.read_text()) if previous_path.exists() else None
    if previous is not None and not reset:
        return verify(env, account)
    # Preflight the entire roster before any checkpoint can change a character.
    for fixture in env.catalog['fixtures']:
        character(env, account, fixture)
    # These rows only add/replace fixture-* checkpoint names; no character/item SQL is replayed.
    env.query(SQL.read_text(encoding='utf-8'), env.databases[2])
    manifest = {'version': 1, 'agent': env.agent, 'account_id': account['id'], 'account': account['username'], 'fixtures': {}}
    for fixture in env.catalog['fixtures']:
        guid = character(env, account, fixture)
        if guid is None:
            env.client(account, ['--cmd', 'create', '--create-name', fixture['name'], '--create-race', str(fixture['race']), '--create-class', str(fixture['class'])])
            deadline = time.monotonic() + env.limits['query_seconds']
            guid = character(env, account, fixture)
            while guid is None and time.monotonic() < deadline:
                time.sleep(env.limits['poll_seconds'])
                guid = character(env, account, fixture)
            if guid is None:
                raise RuntimeError('fixture creation did not persist')
        steps = ['say .revive', 'say .checkpoint reload', 'say .checkpoint apply ' + fixture['checkpoint'],
                 'say .gm off', 'say .save']
        args = ['--cmd', 'play', '--hold', '1']
        for step in steps:
            args += ['--step', step]
        output = env.client(account, args, guid)
        if not re.search(r'Checkpoint ' + re.escape(fixture['checkpoint']) + r' applied \(0 of \d+ steps failed\)', output):
            raise RuntimeError('checkpoint application did not succeed: ' + fixture['checkpoint'])
        env.offline(guid)
        manifest['fixtures'][fixture['id']] = state(env, account, fixture, guid)
    previous_path.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    return verify(env, account)


def safety_module():
    spec = importlib.util.spec_from_file_location('fixture_save_safety', ROOT / 'tools/backup/save_safety.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def snapshot(env, config_path, bot_count):
    if env.running():
        raise ValueError('stop your private world cleanly before baseline capture')
    manifest = verify(env)
    config = json.loads(owned_path(env.root, config_path).read_text())
    if config.get('user') != env.user or config.get('databases') != env.databases or config.get('host') not in ('127.0.0.1', 'localhost') or int(config.get('port', 0)) != int(env.port):
        raise ValueError('snapshot configuration must match all four private databases')
    owned_path(env.root, config['snapshot_root'])
    owned_path(env.root, config['check_root'])
    if int(config['capture_port']) in [int(env.port)] + [p for ports in PORTS.values() for p in ports] + [3724, 8085, 7878]:
        raise ValueError('capture port collides with a database/game server')
    if bot_count < 20 or bot_count > 50:
        raise ValueError('select a local pool of 20..50 pre-generated bots')
    text = (env.runtime / 'playerbots.conf').read_text(encoding='utf-8-sig')
    matches = re.findall(r'^AiPlayerbot.RandomBotAccountPrefix\s*=\s*"([A-Za-z0-9_]+)"', text, re.M)
    if len(matches) != 1:
        raise ValueError('expected one explicit bot account prefix')
    prefix = matches[0]
    bots = env.query('SELECT c.guid,c.name,c.account,c.race,c.class,c.level,c.online FROM characters c JOIN `' + env.databases[0] + "`.account a ON a.id=c.account WHERE LEFT(a.username," + str(len(prefix)) + ")='" + prefix + "' ORDER BY c.guid LIMIT " + str(bot_count) + ';')
    if len(bots) != bot_count or any(row[6] != '0' for row in bots):
        raise ValueError('generate the local bot pool once and log it out before snapshot')
    # Reuse T-037 capture/restore and its hash/version safeguards, without duplicating InnoDB logic.
    os.environ[config['password_env']] = env.password
    result = safety_module().snapshot(config)
    (result / 'fixture-baseline.json').write_text(json.dumps({'version': 1, 'agent': env.agent,
        'fixtures': manifest, 'bot_prefix': prefix, 'bots': bots}, indent=2), encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--mysql-bin', type=Path)
    sub = parser.add_subparsers(dest='command', required=True)
    apply = sub.add_parser('provision')
    apply.add_argument('--reset', action='store_true')
    sub.add_parser('verify')
    capture = sub.add_parser('snapshot')
    capture.add_argument('--config', type=Path, required=True)
    capture.add_argument('--bot-count', type=int, default=20)
    restore = sub.add_parser('restore')
    restore.add_argument('snapshot', type=Path)
    restore.add_argument('destination', type=Path)
    args = parser.parse_args()
    env = Environment(args.runtime, args.mysql_bin)
    with env.locked():
        if args.command == 'provision':
            result = provision(env, args.reset)
            print('Verified', len(result['fixtures']), 'checkpoint fixtures')
        elif args.command == 'verify':
            print('Verified', len(verify(env)['fixtures']), 'unchanged checkpoint fixtures')
        elif args.command == 'snapshot':
            print(snapshot(env, args.config, args.bot_count))
        else:
            source = owned_path(env.root, args.snapshot)
            baseline = json.loads((source / 'fixture-baseline.json').read_text())
            saved = json.loads((source / 'manifest.json').read_text())
            if baseline.get('agent') != env.agent or saved.get('databases') != env.databases:
                raise ValueError('snapshot belongs to another agent')
            print(safety_module().restore(source, owned_path(env.root, args.destination)))


if __name__ == '__main__':
    main()

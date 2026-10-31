"""Generate per-agent runtimes and safely import missing development databases."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PORTS = {'codex': (3744, 8105, 7898), 'claude': (3734, 8095, 7888)}
DATABASES = ('auth', 'characters', 'world', 'playerbots')


def override(text, values):
    for key, value in values.items():
        if '\n' in str(value) or '\r' in str(value):
            raise ValueError('config values must be single lines')
        pattern = re.compile(r'^\s*' + re.escape(key) + r'\s*=.*$', re.M)
        if len(pattern.findall(text)) > 1:
            raise ValueError('duplicate config key: ' + key)
        replacement = key + ' = ' + str(value)
        text = pattern.sub(lambda _: replacement, text) if pattern.search(text) else text.rstrip() + '\n' + replacement + '\n'
    return text


def local_commands(names, policy):
    names = [n.strip() for n in names if n.strip()]
    if len(set(names)) != len(names) or not policy['minimum'] <= len(names) <= policy['maximum']:
        raise ValueError('local roster must contain 20..50 distinct existing random bots')
    if any(not re.fullmatch('[A-Za-z]+', n) for n in names):
        raise ValueError('invalid character name in local roster')
    anchor = ' '.join(str(policy[k]) for k in ('map', 'x', 'y', 'z', 'duration_seconds'))
    return [{'bot': n, 'login': 'botscenario ' + n + ' login',
             'scenario': 'botscenario ' + n + ' wander ' + anchor + ' free ground'} for n in names]


def mysql(args, sql=None, input_file=None):
    suffix = '.exe' if os.name == 'nt' else ''
    command = [str(Path(args.mysql_bin) / ('mysql' + suffix)), '--no-defaults', '--protocol=TCP',
               '-h', args.db_host, '-P', str(args.db_port), '-u', args.db_user, '--batch', '--skip-column-names', '--binary-mode']
    env = os.environ.copy()
    env['MYSQL_PWD'] = os.environ[args.db_password_env]
    result = subprocess.run(command, input=sql.encode() if sql else None,
                            stdin=input_file, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    if result.returncode:
        raise RuntimeError('MySQL operation failed: ' + result.stderr.decode(errors='replace')[-1000:])
    return result.stdout.decode().strip()


def initialize(args):
    dumps = dict(pair.split('=', 1) for pair in args.dump)
    if set(dumps) != set(DATABASES):
        raise ValueError('--initialize requires --dump auth=PATH, characters=PATH, world=PATH, playerbots=PATH')
    # Preflight every input before creating any database. Never overwrite an existing table.
    for value in dumps.values():
        if not Path(value).is_file():
            raise ValueError('dump file missing: ' + value)
    for kind in DATABASES:
        name = kind + '_' + args.agent
        count = int(mysql(args, "SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA='" + name + "';"))
        if count:
            print(name + ': already populated, left unchanged')
            continue
        mysql(args, 'CREATE DATABASE IF NOT EXISTS `' + name + '`;')
        # Dump files must be database-local (standard release dump), never USE/CREATE DATABASE.
        path = Path(dumps[kind])
        with path.open('r', encoding='utf-8-sig', errors='replace') as stream:
            for line in stream:
                if re.match(r'^\s*(USE\b|CREATE\s+DATABASE\b|DROP\s+DATABASE\b)', line, re.I):
                    raise ValueError('dump must not select/create/drop a database: ' + str(path))
        suffix = '.exe' if os.name == 'nt' else ''
        command = [str(Path(args.mysql_bin) / ('mysql' + suffix)), '--no-defaults', '--protocol=TCP', '-h', args.db_host,
                   '-P', str(args.db_port), '-u', args.db_user, '--binary-mode', name]
        env = os.environ.copy()
        env['MYSQL_PWD'] = os.environ[args.db_password_env]
        with path.open('rb') as stream:
            result = subprocess.run(command, stdin=stream, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        if result.returncode:
            raise RuntimeError('import failed for ' + name + ': ' + result.stderr.decode(errors='replace')[-1000:])


def generate(args):
    target = Path(args.output).resolve()
    if ROOT not in target.parents:
        raise ValueError('output must be inside this worktree')
    for value in (args.db_host, args.db_user, os.environ[args.db_password_env]):
        if any(c in value for c in ';"\r\n'):
            raise ValueError('database connection values cannot contain semicolon/quote/newline')
    profiles = json.loads(Path(args.profiles).read_text())
    # Optional system overlays land on independent feature branches (e.g. Eluna).
    for overlay in sorted(Path(args.profiles).parent.glob('profile.d/*.json')):
        additions = json.loads(overlay.read_text())
        for profile in ('dev', 'test'):
            for area in ('world', 'bots'):
                profiles[profile][area].update(additions.get(profile, {}).get(area, {}))
    auth_port, world_port, soap_port = PORTS[args.agent]
    connection = lambda kind: '"' + ';'.join((args.db_host, str(args.db_port), args.db_user,
                                 os.environ[args.db_password_env], kind + '_' + args.agent)) + '"'
    common = {'LoginDatabaseInfo': connection('auth'), 'LogsDir': '"Logs"', 'BindIP': '"127.0.0.1"',
              'Updates.EnableDatabases': '0'}
    world = dict(common, WorldDatabaseInfo=connection('world'), CharacterDatabaseInfo=connection('characters'),
                 PlayerbotsDatabaseInfo=connection('playerbots'))
    world.update({'WorldServerPort': world_port, 'SOAP.Port': soap_port, 'SOAP.Enabled': 1,
                  'SOAP.IP': '"127.0.0.1"', 'Ra.Enable': 0, 'Console.Enable': 0,
                  'DataDir': '"' + str(Path(args.gamedata).resolve()).replace('\\', '/') + '"'})
    world.update(profiles[args.profile]['world'])
    world = {key: str(value).replace('{repo}', ROOT.as_posix()) for key, value in world.items()}
    bots = dict(profiles[args.profile]['bots'], **profiles['loads'][args.load])
    if args.load == 'scale' and not args.performance:
        raise ValueError('scale profile requires --performance')
    if args.load == 'local':
        if not args.roster:
            raise ValueError('local profile requires --roster (20..50 existing random bot names)')
        commands = local_commands(Path(args.roster).read_text().splitlines(), profiles['local'])
    target.mkdir(parents=True, exist_ok=True)
    (target / 'Logs').mkdir(exist_ok=True)
    templates = {'worldserver.conf': (ROOT / 'src/server/worldserver/worldserver.conf.dist', world),
                 'authserver.conf': (ROOT / 'src/server/authserver/authserver.conf.dist', dict(common, RealmServerPort=auth_port)),
                 'playerbots.conf': (ROOT / 'modules/mod_playerbots/config/playerbots.conf.dist', bots)}
    for name, (template, values) in templates.items():
        (target / name).write_text(override(template.read_text(encoding='utf-8-sig'), values), encoding='utf-8')
    if args.binaries:
        binary_source = Path(args.binaries).resolve()
        for name in ('authserver.exe', 'worldserver.exe', 'libmysql.dll', 'libcrypto-3-x64.dll', 'libssl-3-x64.dll', 'legacy.dll'):
            path = binary_source / name
            if not path.is_file():
                raise ValueError('required runtime file missing: ' + str(path))
            shutil.copy2(path, target / name)
    launcher = "param([ValidateSet('auth','world')][string]$Server='world')\n$runtime=Split-Path -Parent $MyInvocation.MyCommand.Path\nStart-Process -FilePath (Join-Path $runtime ($Server+'server.exe')) -ArgumentList @('-c', ('\"'+(Join-Path $runtime ($Server+'server.conf'))+'\"')) -WorkingDirectory $runtime -WindowStyle Hidden\n"
    (target / 'start.ps1').write_text(launcher)
    metadata = {'agent': args.agent, 'profile': args.profile, 'load': args.load, 'auth_port': auth_port,
                'world_port': world_port, 'soap_port': soap_port, 'databases': [d + '_' + args.agent for d in DATABASES]}
    (target / 'environment.json').write_text(json.dumps(metadata, indent=2))
    if args.load == 'local':
        (target / 'local-roster.json').write_text(json.dumps(commands, indent=2))
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--agent', choices=PORTS, default='codex')
    parser.add_argument('--output', default='build/devenv')
    parser.add_argument('--gamedata', required=True)
    parser.add_argument('--binaries', help='Existing build output; all six runtime files must be present')
    parser.add_argument('--mysql-bin', default=os.environ.get('MOP_MYSQL_BIN', ''))
    parser.add_argument('--db-host', default='127.0.0.1')
    parser.add_argument('--db-port', type=int, default=3306)
    parser.add_argument('--db-user', default='ultimatemop')
    parser.add_argument('--db-password-env', default='MOP_DB_PASSWORD')
    parser.add_argument('--profile', choices=('dev', 'test'), default='test')
    parser.add_argument('--load', choices=('none', 'local', 'scale'), default='none')
    parser.add_argument('--profiles', default=str(Path(__file__).with_name('profiles.json')))
    parser.add_argument('--roster')
    parser.add_argument('--performance', action='store_true')
    parser.add_argument('--initialize', action='store_true')
    parser.add_argument('--dump', action='append', default=[])
    parser.add_argument('--configure-db', action='store_true', help='Set only this agent realmlist and optionally create a missing GM')
    parser.add_argument('--gm-user-env', default='MOP_GM_USER')
    parser.add_argument('--gm-password-env', default='MOP_GM_PASSWORD')
    args = parser.parse_args()
    if args.initialize:
        initialize(args)
    if args.configure_db:
        name = 'auth_' + args.agent
        mysql(args, 'UPDATE `' + name + '`.realmlist SET port=' + str(PORTS[args.agent][1]) + ' WHERE id=1;')
        gm = os.environ.get(args.gm_user_env)
        if gm:
            if not re.fullmatch('[A-Za-z0-9_]+', gm):
                raise ValueError('GM account must be alphanumeric')
            password = os.environ[args.gm_password_env]
            digest = hashlib.sha1((gm.upper() + ':' + password.upper()).encode()).hexdigest().upper()
            mysql(args, "INSERT IGNORE INTO `" + name + "`.account (username,sha_pass_hash) VALUES ('" + gm.upper() + "','" + digest + "'); INSERT IGNORE INTO `" + name + "`.account_access (id,gmlevel,RealmID) SELECT id,3,-1 FROM `" + name + "`.account WHERE username='" + gm.upper() + "';")
    print(generate(args))


if __name__ == '__main__':
    main()

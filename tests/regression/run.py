"""Run assertion-driven scenarios against a generated private test runtime."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def events(output):
    result = []
    for line in output.splitlines():
        if not line.startswith('EVT '):
            continue
        parts = shlex.split(line)
        if len(parts) >= 3:
            result.append(dict(kind=parts[2], milliseconds=int(parts[1]),
                               **dict(token.split('=', 1) for token in parts[3:] if '=' in token)))
    return result


def assertions(output, checks):
    if not checks:
        raise ValueError('scenario must contain at least one assertion')
    parsed = events(output)
    failures = []
    for check in checks:
        kind = check['type']
        if kind == 'contains':
            ok = check['text'] in output
        elif kind == 'not_contains':
            ok = check['text'] not in output
        elif kind == 'regex':
            ok = re.search(check['pattern'], output, re.M) is not None
        elif kind == 'event':
            count = sum(event['kind'] == check['kind'] and all(str(event.get(k)) == str(v)
                        for k, v in check.get('fields', {}).items()) for event in parsed)
            ok = check.get('minimum', 1) <= count <= check.get('maximum', sys.maxsize)
        else:
            raise ValueError('unknown assertion type: ' + kind)
        if not ok:
            failures.append(check)
    return failures


def redact(output, environment):
    for key in ('MOP_PASSWORD', 'MOP_SOAP_PASSWORD'):
        if environment.get(key):
            output = output.replace(environment[key], '[redacted]')
    output = re.sub(r'K=[a-fA-F0-9.]+', 'K=[redacted]', output)
    return re.sub(r'(?m)^.*sent CMSG_AUTH_SESSION.*$', '[authentication packet redacted]', output)


def validate(case):
    if not re.fullmatch(r'[A-Za-z0-9_-]+', case['name']):
        raise ValueError('scenario name must be filename-safe')
    args = case.get('args', [])
    if not isinstance(args, list) or not all(isinstance(arg, str) for arg in args):
        raise ValueError('args must be a string array')
    forbidden = ('--password', '--soap-pass', '--soap-user', '--account', '--host', '--auth-port', '--world-port',
                 '--soap-host', '--soap-port', '--server-log')
    if any(arg.startswith('--') and any(flag.startswith(arg.split('=', 1)[0]) for flag in forbidden) for arg in args):
        raise ValueError('scenario cannot override identity, credentials or test-runtime endpoints')
    if not case.get('assertions') or float(case.get('timeout_seconds', 0)) <= 0:
        raise ValueError('scenario needs assertions and positive timeout_seconds')


def execute(case, runtime, destination, environment):
    validate(case)
    command = [sys.executable, str(ROOT / 'contrib/headless-client/headless.py')] + case.get('args', [])
    for step in case.get('steps', []):
        command += ['--step', step]
    start = time.monotonic()
    try:
        process = subprocess.run(command, cwd=ROOT, env=environment, text=True, encoding='utf-8',
                                 errors='replace', capture_output=True, timeout=float(case['timeout_seconds']))
        output = process.stdout + '\n' + process.stderr
        failures = assertions(output, case['assertions'])
        if process.returncode:
            failures.append({'exit_code': process.returncode})
    except subprocess.TimeoutExpired as exc:
        output = (exc.stdout or b'') + b'\n' + (exc.stderr or b'')
        if isinstance(output, bytes):
            output = output.decode('utf-8', errors='replace')
        failures = [{'timeout_seconds': case['timeout_seconds']}]
    output = redact(output, environment)
    (destination / (case['name'] + '.log')).write_text(output, encoding='utf-8')
    return {'name': case['name'], 'passed': not failures, 'failures': failures,
            'seconds': time.monotonic() - start}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--suite', type=Path, default=Path(__file__).with_name('scenarios'))
    parser.add_argument('--output', type=Path, default=ROOT / 'build/regression')
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    metadata = json.loads((runtime / 'environment.json').read_text())
    if metadata.get('profile') != 'test' or metadata.get('load') not in ('none', 'local'):
        raise ValueError('regression requires a generated test profile with none/local bot load')
    env = os.environ.copy()
    for key in ('MOP_ACCOUNT', 'MOP_PASSWORD'):
        if not env.get(key):
            raise ValueError('set ' + key + ' to dedicated fixture credentials')
    env.update(MOP_HOST='127.0.0.1', MOP_AUTH_PORT=str(metadata['auth_port']),
               MOP_WORLD_PORT=str(metadata['world_port']), MOP_SOAP_HOST='127.0.0.1',
               MOP_SOAP_PORT=str(metadata['soap_port']), HEADLESS_LOGOUT='1',
               MOP_SERVER_LOG=str(runtime / 'Logs/Server.log'), PYTHONIOENCODING='utf-8')
    destination = args.output.resolve()
    if ROOT not in destination.parents:
        raise ValueError('reports must stay within this worktree')
    destination /= datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8]
    destination.mkdir(parents=True)
    paths = sorted(args.suite.glob('*.json')) if args.suite.is_dir() else [args.suite]
    if not paths:
        raise ValueError('suite contains no scenarios')
    cases = [json.loads(path.read_text()) for path in paths]
    if len({case['name'] for case in cases}) != len(cases):
        raise ValueError('scenario names must be unique')
    for case in cases:
        validate(case)
    results = [execute(case, runtime, destination, env) for case in cases]
    (destination / 'results.json').write_text(json.dumps(results, indent=2))
    suite = ET.Element('testsuite', name='UltimateMoP', tests=str(len(results)),
                       failures=str(sum(not r['passed'] for r in results)))
    for result in results:
        element = ET.SubElement(suite, 'testcase', name=result['name'], time=str(result['seconds']))
        if not result['passed']:
            ET.SubElement(element, 'failure').text = json.dumps(result['failures'])
    ET.ElementTree(suite).write(destination / 'junit.xml', encoding='utf-8', xml_declaration=True)
    print(str(destination))
    for result in results:
        print(('PASS ' if result['passed'] else 'FAIL ') + result['name'])
    return 0 if all(result['passed'] for result in results) else 1


if __name__ == '__main__':
    sys.exit(main())

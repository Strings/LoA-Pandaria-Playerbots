"""Dependency-free MCP stdio adapter for Trinity SOAP and probe logs (Python 3.9+)."""
import base64
import http.client
import json
import math
import os
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
from xml.sax.saxutils import escape


def command(text):
    if not isinstance(text, str) or not text.strip() or any(c in text for c in '\r\n\0'):
        raise ValueError('command must be one nonempty line')
    user, password = os.environ.get('MOP_SOAP_USER'), os.environ.get('MOP_SOAP_PASSWORD')
    if not user or password is None:
        raise ValueError('set MOP_SOAP_USER and MOP_SOAP_PASSWORD')
    body = ('<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" xmlns:t="urn:TC">'
            '<s:Body><t:executeCommand><command>' + escape(text.lstrip('.')) +
            '</command></t:executeCommand></s:Body></s:Envelope>').encode('utf-8')
    auth = base64.b64encode((user + ':' + password).encode()).decode()
    conn = http.client.HTTPConnection(os.environ.get('MOP_SOAP_HOST', '127.0.0.1'),
                                     int(os.environ.get('MOP_SOAP_PORT', '7898')),
                                     timeout=float(os.environ.get('MOP_SOAP_TIMEOUT', '30')))
    try:
        conn.request('POST', '/', body, {'Content-Type': 'text/xml; charset=utf-8',
                     'SOAPAction': '"urn:TC#executeCommand"', 'Authorization': 'Basic ' + auth})
        response = conn.getresponse()
        payload = response.read(2 * 1024 * 1024 + 1)
        if len(payload) > 2 * 1024 * 1024:
            raise ValueError('SOAP response exceeded 2 MiB')
        if response.status != 200:
            raise ValueError('SOAP HTTP ' + str(response.status) + '; check credentials and command syntax')
        root = ET.fromstring(payload)
        if any(e.tag.rsplit('}', 1)[-1] == 'Fault' for e in root.iter()):
            raise ValueError('SOAP fault: ' + ' '.join(root.itertext()))
        return '\n'.join(e.text or '' for e in root.iter() if e.tag.rsplit('}', 1)[-1] == 'result')
    finally:
        conn.close()


def tail():
    path = os.environ.get('MOP_SERVER_LOG')
    if not path:
        raise ValueError('set MOP_SERVER_LOG to the probe/server log')
    limit = int(os.environ.get('MOP_LOG_SCAN_BYTES', '1048576'))
    if not 1024 <= limit <= 16777216:
        raise ValueError('MOP_LOG_SCAN_BYTES must be 1024..16777216')
    with Path(path).open('rb') as stream:
        size = stream.seek(0, 2)
        start = max(0, size - limit)
        stream.seek(start)
        if start:
            stream.readline()  # Discard the partial first record.
        return stream.read(limit).decode('utf-8', errors='replace').splitlines()


PROBE = re.compile(r'\[PROBE\] (\w+) (.*)')


def probes(lines):
    latest = {}
    for line in lines:
        match = PROBE.search(line)
        if match:
            fields = dict(re.findall(r'(\w+)=([^\s]+)', match[2]))
            latest[match[1].lower()] = {'name': match[1], 'fields': fields, 'raw': line}
    return latest


def schema(properties=None, required=None):
    return {'type': 'object', 'properties': properties or {}, 'required': required or [],
            'additionalProperties': False}


TOOLS = [
    ('server_info', 'Run server info over SOAP.', schema()),
    ('gm_command', 'Execute one GM command with the configured account privileges.',
     schema({'command': {'type': 'string'}}, ['command'])),
    ('botscenario', 'Run botscenario <bot> <scenario and arguments>. This changes server state.',
     schema({'bot': {'type': 'string'}, 'scenario': {'type': 'string'}}, ['bot', 'scenario'])),
    ('log_tail', 'Read a bounded tail of the configured log.',
     schema({'lines': {'type': 'integer', 'minimum': 1, 'maximum': 1000}})),
    ('bot_state', 'Latest recorded PROBE fields, not a live query. Requires botprobe logging.',
     schema({'bot': {'type': 'string'}}, ['bot'])),
    ('bots_near', 'Recorded probe positions within a 3D radius. May include master probes; not an online census.',
     schema({k: {'type': 'integer' if k == 'map' else 'number'}
             for k in ('map', 'x', 'y', 'z', 'radius')}, ['map', 'x', 'y', 'z', 'radius']))
]


def call(name, args):
    spec = next((s for n, _, s in TOOLS if n == name), None)
    if spec is None:
        raise ValueError('unknown tool: ' + str(name))
    if not isinstance(args, dict) or set(args) - set(spec['properties']) or set(spec['required']) - set(args):
        raise ValueError('invalid tool arguments')
    for key, value in args.items():
        kind = spec['properties'][key]['type']
        if (kind == 'string' and not isinstance(value, str) or
                kind == 'integer' and (type(value) is not int) or
                kind == 'number' and (type(value) not in (int, float) or not math.isfinite(value))):
            raise ValueError('invalid argument: ' + key)
    if name == 'server_info':
        return command('server info')
    if name == 'gm_command':
        return command(args['command'])
    if name == 'botscenario':
        if not re.fullmatch(r'[A-Za-z]+', args['bot']):
            raise ValueError('bot must be a character name')
        return command('botscenario ' + args['bot'] + ' ' + args['scenario'])
    lines = tail()
    if name == 'log_tail':
        count = args.get('lines', 100)
        if not 1 <= count <= 1000:
            raise ValueError('lines must be 1..1000')
        return '\n'.join(lines[-count:])
    latest = probes(lines)
    if name == 'bot_state':
        return {'observation': latest.get(args['bot'].lower()), 'historical': True,
                'note': 'Missing means no probe in the bounded log window; it does not mean offline.'}
    if args['radius'] < 0 or args['map'] < 0:
        raise ValueError('map and radius must be nonnegative')
    found = []
    for observation in latest.values():
        f = observation['fields']
        try:
            distance = math.sqrt(sum((float(f[k]) - args[k]) ** 2 for k in ('x', 'y', 'z')))
            if int(f['map']) == args['map'] and math.isfinite(distance) and distance <= args['radius']:
                found.append(dict(observation, distance=distance))
        except (KeyError, ValueError):
            continue
    return {'observations': sorted(found, key=lambda o: o['distance']), 'historical': True,
            'note': 'Only recorded probes, possibly including masters. Not an exhaustive live bot list.'}


def dispatch(request):
    if not isinstance(request, dict) or request.get('jsonrpc') != '2.0' or not isinstance(request.get('method'), str):
        return {'jsonrpc': '2.0', 'id': None, 'error': {'code': -32600, 'message': 'Invalid Request'}}
    if 'id' not in request:
        return None
    reply = {'jsonrpc': '2.0', 'id': request['id']}
    method, params = request['method'], request.get('params', {})
    if method == 'initialize':
        result = {'protocolVersion': '2024-11-05', 'capabilities': {'tools': {}},
                  'serverInfo': {'name': 'ultimatemop-live-server', 'version': '0.1.0'}}
    elif method == 'ping':
        result = {}
    elif method == 'tools/list':
        result = {'tools': [{'name': n, 'description': d, 'inputSchema': s} for n, d, s in TOOLS]}
    elif method == 'tools/call':
        try:
            value = call(params.get('name'), params.get('arguments', {}))
            result = {'content': [{'type': 'text', 'text': value if isinstance(value, str) else json.dumps(value)}]}
        except (ValueError, OSError, ET.ParseError, http.client.HTTPException, TypeError, AttributeError) as exc:
            result = {'isError': True, 'content': [{'type': 'text', 'text': str(exc)}]}
    else:
        reply['error'] = {'code': -32601, 'message': 'Method not found'}
        return reply
    reply['result'] = result
    return reply


def main():
    for line in sys.stdin:
        try:
            response = dispatch(json.loads(line))
        except (ValueError, TypeError):
            response = {'jsonrpc': '2.0', 'id': None, 'error': {'code': -32700, 'message': 'Parse error'}}
        if response is not None:
            print(json.dumps(response, ensure_ascii=True), flush=True)


if __name__ == '__main__':
    main()

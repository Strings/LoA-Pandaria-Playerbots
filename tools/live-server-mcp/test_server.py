import contextlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

import server


class Tests(unittest.TestCase):
    def test_stdio_protocol(self):
        requests = [dict(jsonrpc='2.0', id=1, method='initialize', params={}),
                    dict(jsonrpc='2.0', method='notifications/initialized'),
                    dict(jsonrpc='2.0', id=2, method='tools/list'),
                    dict(jsonrpc='2.0', id=3, method='missing')]
        run = subprocess.run([sys.executable, str(Path(server.__file__))],
                             input='\n'.join(map(json.dumps, requests)) + '\n{bad}\n',
                             text=True, capture_output=True, check=True)
        replies = list(map(json.loads, run.stdout.splitlines()))
        self.assertEqual(len(replies), 4)
        self.assertEqual(replies[0]['result']['protocolVersion'], '2024-11-05')
        self.assertEqual(len(replies[1]['result']['tools']), 6)
        self.assertEqual(replies[2]['error']['code'], -32601)
        self.assertEqual(replies[3]['error']['code'], -32700)

    def test_latest_position_and_invalid_arguments(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / 'probe.log'
            log.write_text('[PROBE] Alice t=1 map=0 x=1 y=2 z=3\n'
                           '[PROBE] Alice t=2 map=1 x=1 y=2 z=3\n'
                           '[PROBE] Bob t=1 map=0 x=1 y=2 z=3\n')
            with patch.dict(os.environ, {'MOP_SERVER_LOG': str(log)}):
                result = server.call('bots_near', dict(map=0, x=1, y=2, z=3, radius=0))
                self.assertEqual([o['name'] for o in result['observations']], ['Bob'])
                self.assertIsNone(server.call('bot_state', {'bot': 'Missing'})['observation'])
                for args in ({'lines': 0}, {'lines': True}, {'file': 'secret'}):
                    with self.assertRaises(ValueError):
                        server.call('log_tail', args)

    def test_soap_request_and_fault(self):
        captured = []

        class Handler(BaseHTTPRequestHandler):
            def do_POST(self):
                captured.append(self.rfile.read(int(self.headers['Content-Length'])))
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b'<Envelope><Body><result>World &amp; bots</result></Body></Envelope>')

            def log_message(self, *args):
                pass

        http = HTTPServer(('127.0.0.1', 0), Handler)
        worker = threading.Thread(target=http.serve_forever, daemon=True)
        worker.start()
        try:
            with patch.dict(os.environ, {'MOP_SOAP_HOST': '127.0.0.1', 'MOP_SOAP_PORT': str(http.server_port),
                                        'MOP_SOAP_USER': 'test', 'MOP_SOAP_PASSWORD': 'test'}):
                self.assertEqual(server.command('say <hello>'), 'World & bots')
                self.assertIn(b'&lt;hello&gt;', captured[0])
                with self.assertRaises(ValueError):
                    server.command('server info\nserver shutdown 1')
        finally:
            http.shutdown()
            http.server_close()
            worker.join()


if __name__ == '__main__':
    unittest.main()

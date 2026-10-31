"""Explicitly log in a local roster and anchor each bot's wander scenario in one zone."""
import argparse
import base64
import http.client
import json
import os
from pathlib import Path
import time
import xml.etree.ElementTree as ET
from xml.sax.saxutils import escape


def soap(port, text, timeout):
    credentials = os.environ['MOP_SOAP_USER'] + ':' + os.environ['MOP_SOAP_PASSWORD']
    connection = http.client.HTTPConnection(os.environ.get('MOP_SOAP_HOST', '127.0.0.1'), port, timeout=timeout)
    body = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" xmlns:t="urn:TC"><s:Body><t:executeCommand><command>' + escape(text) + '</command></t:executeCommand></s:Body></s:Envelope>'
    try:
        connection.request('POST', '/', body.encode(), {'Content-Type': 'text/xml',
                           'SOAPAction': '"urn:TC#executeCommand"',
                           'Authorization': 'Basic ' + base64.b64encode(credentials.encode()).decode()})
        response = connection.getresponse()
        payload = response.read()
        if response.status != 200:
            raise RuntimeError('SOAP command failed with HTTP ' + str(response.status))
        root = ET.fromstring(payload)
        if any(e.tag.rsplit('}', 1)[-1] == 'Fault' for e in root.iter()):
            raise RuntimeError('SOAP command returned a fault')
        return payload
    finally:
        connection.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runtime', type=Path)
    parser.add_argument('--profiles', type=Path, default=Path(__file__).with_name('profiles.json'))
    args = parser.parse_args()
    metadata = json.loads((args.runtime / 'environment.json').read_text())
    if metadata['load'] != 'local':
        raise ValueError('runtime must use the local profile')
    policy = json.loads(args.profiles.read_text())['local']
    for bot in json.loads((args.runtime / 'local-roster.json').read_text()):
        soap(metadata['soap_port'], bot['login'], policy['soap_timeout_seconds'])
        deadline = time.monotonic() + policy['login_timeout_seconds']
        while True:
            try:
                soap(metadata['soap_port'], bot['scenario'], policy['soap_timeout_seconds'])
                print(bot['bot'] + ': local scenario started', flush=True)
                break
            except RuntimeError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(policy['retry_seconds'])


if __name__ == '__main__':
    main()

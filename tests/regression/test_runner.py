import importlib.util
from pathlib import Path
import unittest
import subprocess
import tempfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('regression', Path(__file__).with_name('run.py'))
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


class Tests(unittest.TestCase):
    def test_event_fields_and_negative_assertions(self):
        text = 'EVT 12 chat msg="hello world" type=SYSTEM\n'
        self.assertEqual(r.assertions(text, [{'type': 'event', 'kind': 'chat', 'fields': {'msg': 'hello world'}, 'maximum': 1}]), [])
        self.assertEqual(len(r.assertions(text, [{'type': 'event', 'kind': 'missing'}])), 1)
        self.assertEqual(len(r.assertions(text, [{'type': 'not_contains', 'text': 'hello'}])), 1)
        with self.assertRaises(ValueError):
            r.assertions(text, [])

    def test_timeout_reports_text_bytes_and_mixed_output(self):
        case = {'name': 'timeout', 'args': [], 'assertions': [{'type': 'contains', 'text': 'unused'}],
                'timeout_seconds': 1}
        for stdout, stderr in [('secret', 'error'), (b'secret', b'error'), ('secret', b'error'),
                               (b'secret', 'error'), (None, None)]:
            with self.subTest(stdout=stdout, stderr=stderr), tempfile.TemporaryDirectory() as directory:
                destination = Path(directory)
                expired = subprocess.TimeoutExpired(['headless'], 1, output=stdout, stderr=stderr)
                with patch.object(r.subprocess, 'run', side_effect=expired):
                    result = r.execute(case, destination, destination, {'MOP_PASSWORD': 'secret'})
                self.assertFalse(result['passed'])
                self.assertEqual(result['failures'], [{'timeout_seconds': 1}])
                output = (destination / 'timeout.log').read_text(encoding='utf-8')
                self.assertNotIn('secret', output)
                if stderr:
                    self.assertIn('error', output)

    def test_credentials_are_redacted_and_overrides_rejected(self):
        output = r.redact('secret K=abcd...\nsent CMSG_AUTH_SESSION packet=123\n', {'MOP_PASSWORD': 'secret'})
        self.assertNotIn('secret', output)
        self.assertNotIn('abcd', output)
        self.assertNotIn('packet=123', output)
        with self.assertRaises(ValueError):
            r.validate({'name': 'bad', 'args': ['--world-port=8085'], 'assertions': [{}], 'timeout_seconds': 5})


if __name__ == '__main__':
    unittest.main()

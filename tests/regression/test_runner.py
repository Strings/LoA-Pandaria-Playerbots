import importlib.util
from pathlib import Path
import unittest

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

    def test_credentials_are_redacted_and_overrides_rejected(self):
        output = r.redact('secret K=abcd...\nsent CMSG_AUTH_SESSION packet=123\n', {'MOP_PASSWORD': 'secret'})
        self.assertNotIn('secret', output)
        self.assertNotIn('abcd', output)
        self.assertNotIn('packet=123', output)
        with self.assertRaises(ValueError):
            r.validate({'name': 'bad', 'args': ['--world-port=8085'], 'assertions': [{}], 'timeout_seconds': 5})


if __name__ == '__main__':
    unittest.main()

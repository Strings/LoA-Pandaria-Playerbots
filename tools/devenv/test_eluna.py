import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('enable_eluna', Path(__file__).with_name('enable_eluna.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class Tests(unittest.TestCase):
    def test_existing_config_is_enabled_idempotently(self):
        root = m.ROOT / 'build/test-temporary'
        root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=root) as name:
            runtime = Path(name)
            config = runtime / 'worldserver.conf'
            config.write_text('Other = 7\nEluna.Enabled = 0\nEluna.TraceBack = false\nEluna.ScriptPath = "lua_scripts"\n')
            m.enable(runtime)
            first = config.read_bytes()
            m.enable(runtime)
            self.assertEqual(first, config.read_bytes())
            self.assertIn(b'Other = 7', first)
            self.assertIn(b'Eluna.Enabled = 1', first)


if __name__ == '__main__':
    unittest.main()

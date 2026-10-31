import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('solo', Path(__file__).with_name('apply_solo.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class Tests(unittest.TestCase):
    def test_settings_exist_and_effective_defaults_match(self):
        import re
        policy = json.loads(Path(__file__).with_name('profile.d').joinpath('019-solo.json').read_text())
        dist = (m.ROOT / 'src/server/worldserver/worldserver.conf.dist').read_text(encoding='utf-8-sig')
        loader = (m.ROOT / 'src/server/game/World/World.cpp').read_text(encoding='utf-8-sig')
        loader += (m.ROOT / 'src/server/game/World/SoloConfig.cpp').read_text()
        for key, value in policy['dev']['world'].items():
            self.assertIn('"' + key + '"', loader)
            self.assertEqual(re.search(r'^' + re.escape(key) + r'\s*=\s*(.+)$', dist, re.M)[1].strip(), value)
        self.assertEqual(policy['dev'], policy['test'])

    def test_existing_config_is_changed_idempotently(self):
        root = m.ROOT / 'build/test-temporary'
        root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=root) as name:
            target = Path(name) / 'worldserver.conf'
            original = (m.ROOT / 'src/server/worldserver/worldserver.conf.dist').read_text(encoding='utf-8-sig')
            # An existing generated runtime predates the two new core settings.
            original = '\n'.join(line for line in original.splitlines() if not line.startswith(('Currency.WeeklyCaps.Enable =', 'SkillChance.Fishing.Always =')))
            target.write_text(original, encoding='utf-8')
            m.apply(name)
            first = target.read_bytes()
            self.assertIn(b'Currency.WeeklyCaps.Enable = 0', first)
            self.assertIn(b'SkillChance.Fishing.Always = 1', first)
            m.apply(name)
            self.assertEqual(first, target.read_bytes())


if __name__ == '__main__':
    unittest.main()

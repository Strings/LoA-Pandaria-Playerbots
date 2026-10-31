import json
from pathlib import Path
import unittest
import importlib.util

spec = importlib.util.spec_from_file_location('devenv_setup', Path(__file__).with_name('setup.py'))
s = importlib.util.module_from_spec(spec)
spec.loader.exec_module(s)


class Tests(unittest.TestCase):
    def test_override_preserves_comments_and_rejects_duplicates(self):
        text = '# Key = old\nKey = old\nOther = stay\n'
        self.assertEqual(s.override(text, {'Key': 'new'}), '# Key = old\nKey = new\nOther = stay\n')
        with self.assertRaises(ValueError):
            s.override('Key=1\nKey=2\n', {'Key': 3})

    def test_roster_is_bounded_and_anchored(self):
        policy = json.loads(Path(__file__).with_name('profiles.json').read_text())['local']
        names = ['Bot' + chr(65 + i) for i in range(20)]
        commands = s.local_commands(names, policy)
        self.assertEqual(len(commands), 20)
        self.assertTrue(all(' wander 0 -8833.0 628.0 95.0 300 ' in c['scenario'] for c in commands))
        with self.assertRaises(ValueError):
            s.local_commands(names[:19], policy)
        with self.assertRaises(ValueError):
            s.local_commands(names + [names[0]], policy)

    def test_profile_keys_exist_in_the_fork(self):
        profiles = json.loads(Path(__file__).with_name('profiles.json').read_text())
        world = (s.ROOT / 'src/server/worldserver/worldserver.conf.dist').read_text(encoding='utf-8-sig')
        bots = (s.ROOT / 'modules/mod_playerbots/config/playerbots.conf.dist').read_text()
        for profile in ('dev', 'test'):
            for key in profiles[profile]['world']:
                self.assertTrue(key.startswith('Logger.') or key in world, key)
            for key in profiles[profile]['bots']:
                self.assertIn(key, bots)
        for load in profiles['loads'].values():
            for key in load:
                self.assertIn(key, bots)


if __name__ == '__main__':
    unittest.main()

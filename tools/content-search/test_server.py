"""Unit and gamedata integration tests for tools/content-search/ (T-030)."""
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

THIS_DIR = Path(__file__).resolve().parent
ROOT = THIS_DIR.parents[1]
for p in (str(THIS_DIR), str(ROOT)):
    if p not in sys.path:
        sys.path.insert(0, p)

import server


class ContentSearchTests(unittest.TestCase):
    def setUp(self):
        env_dir = os.environ.get('MOP_DBC_DIR')
        self.dbc_dir = Path(env_dir) if env_dir else Path(__file__).resolve().parents[2] / 'gamedata' / 'dbc'
        if not self.dbc_dir.is_dir():
            self.skipTest(f'{self.dbc_dir} not present')

    def test_stdio_protocol_and_validation(self):
        requests = [
            {'jsonrpc': '2.0', 'id': 1, 'method': 'initialize', 'params': {}},
            {'jsonrpc': '2.0', 'method': 'notifications/initialized'},
            {'jsonrpc': '2.0', 'id': 2, 'method': 'tools/list'},
            {
                'jsonrpc': '2.0',
                'id': 3,
                'method': 'tools/call',
                'params': {'name': 'search_spells', 'arguments': {'spell_id': 133}},
            },
            {'jsonrpc': '2.0', 'id': 4, 'method': 'unknown_method'},
        ]
        run = subprocess.run(
            [sys.executable, str(Path(server.__file__))],
            input='\n'.join(map(json.dumps, requests)) + '\n{bad}\n',
            text=True,
            capture_output=True,
            check=True,
        )
        replies = list(map(json.loads, run.stdout.splitlines()))
        self.assertEqual(len(replies), 5)
        self.assertEqual(replies[0]['result']['protocolVersion'], '2024-11-05')
        self.assertEqual(len(replies[1]['result']['tools']), 5)
        spell_payload = json.loads(replies[2]['result']['content'][0]['text'])
        self.assertEqual(spell_payload['count'], 1)
        self.assertEqual(spell_payload['results'][0]['name'], 'Fireball')
        self.assertEqual(replies[3]['error']['code'], -32601)
        self.assertEqual(replies[4]['error']['code'], -32700)

        # Reject invalid arguments (bool as int, out-of-range limit, unknown key)
        for bad_args in (
            {'spell_id': True},
            {'limit': 0},
            {'limit': 500},
            {'unknown_field': 'x'},
        ):
            with self.assertRaises(ValueError):
                server.call('search_spells', bad_args)

    def test_search_spells_and_visuals(self):
        # 1. Spell 16614 (Heartseed placeholder proc spell: Lightning Strike)
        res = server.call('search_spells', {'spell_id': 16614})
        self.assertEqual(res['count'], 1)
        spell = res['results'][0]
        self.assertEqual(spell['name'], 'Lightning Strike')
        self.assertIn(4229, spell['spell_visual_ids'])
        self.assertTrue(any('LightningBolt_Impact_Chest' in m for m in spell['visual_models']))
        self.assertTrue(len(spell['effects']) >= 1)

        # 2. Weapon glow enchants for Heartseed (Jade Spirit 4442, Mongoose 2673)
        glow_res = server.call('search_visuals', {'kind': 'weapon_glow', 'query': 'Jade Spirit'})
        self.assertGreaterEqual(glow_res['count'], 1)
        jade = next(r for r in glow_res['results'] if r['enchant_id'] == 4442)
        self.assertEqual(jade['item_visual_id'], 238)
        self.assertIn('SPELLS\\JADESPIRIT_HIGH.MDX', jade['unique_models'])

        # 3. Spell visuals by model query
        vis_res = server.call('search_visuals', {'kind': 'spell_visual', 'visual_id': 67})
        self.assertEqual(vis_res['count'], 1)
        v67 = vis_res['results'][0]
        self.assertIn('cast', v67['kits'])
        self.assertTrue(any(s['name'] == 'Fireball' for s in v67['sample_spells']))

        # 4. Spell icons
        icon_res = server.call('search_visuals', {'kind': 'icon', 'query': 'FlameBolt', 'limit': 5})
        self.assertGreaterEqual(icon_res['count'], 1)
        self.assertTrue(any('FlameBolt' in r['icon_path'] for r in icon_res['results']))

    def test_search_items_creatures_and_display_ids(self):
        # 1. Search items (Shadowfang 1482)
        item_res = server.call('search_items', {'item_id': 1482})
        self.assertEqual(item_res['count'], 1)
        sf = item_res['results'][0]
        self.assertEqual(sf['name'], 'Shadowfang')
        self.assertEqual(sf['item_class'], 2)
        self.assertGreater(sf['display_id'], 0)

        # 2. Search creatures from Creature.db2 (Infernal 89)
        with patch.dict(os.environ, {'MOP_WORLD_DB': ''}, clear=False):
            server._ENGINE_CACHE.clear()
            c_res = server.call('search_creatures', {'creature_id': 89})
            self.assertEqual(c_res['count'], 1)
            inf = c_res['results'][0]
            self.assertEqual(inf['name'], 'Infernal')
            self.assertIn(169, inf['display_ids'])
            self.assertTrue(len(inf['displays']) >= 1)

        # 3. Search display IDs across creature, item, and gameobject
        cd_res = server.call('search_display_ids', {'kind': 'creature', 'display_id': 169})
        self.assertEqual(cd_res['count'], 1)
        self.assertEqual(cd_res['results'][0]['display_id'], 169)

        id_res = server.call('search_display_ids', {'kind': 'item', 'display_id': sf['display_id']})
        self.assertEqual(id_res['count'], 1)
        self.assertTrue(any(i['id'] == 1482 for i in id_res['results'][0]['sample_items']))

        gd_res = server.call('search_display_ids', {'kind': 'gameobject', 'query': 'Chest02', 'limit': 5})
        self.assertGreaterEqual(gd_res['count'], 1)
        self.assertTrue(any('Chest02' in r['model_path'] for r in gd_res['results']))


if __name__ == '__main__':
    unittest.main()

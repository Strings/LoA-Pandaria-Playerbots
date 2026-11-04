"""Boundary and state-contract tests for private checkpoint fixtures."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

spec = importlib.util.spec_from_file_location('fixtures', Path(__file__).with_name('fixtures.py'))
f = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f)


class Tests(unittest.TestCase):
    def test_catalog_rejects_duplicate_level_and_name(self):
        base = f.load_catalog()
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory) / 'catalog.json'
            for mutate in (lambda d: d['fixtures'][1].update(level=1),
                           lambda d: d['fixtures'][1].update(name=d['fixtures'][0]['name']),
                           lambda d: d['limits'].update(query_seconds=0)):
                data = copy.deepcopy(base)
                mutate(data)
                p.write_text(json.dumps(data))
                with self.assertRaises(ValueError):
                    f.load_catalog(p)

    def test_paths_cannot_escape_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'codex'
            root.mkdir()
            for target in (root, root / 'build', root / 'build/../../elsewhere'):
                with self.assertRaises(ValueError):
                    f.owned_path(root, target)
            self.assertEqual(f.owned_path(root, (root / 'build/fixtures').resolve()), (root / 'build/fixtures').resolve())

    def test_metadata_cannot_point_at_main_or_another_agent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'codex'
            runtime = root / 'build/test'
            runtime.mkdir(parents=True)
            meta = {'agent': 'codex', 'profile': 'test', 'load': 'none',
                    'databases': ['auth_codex', 'characters_codex', 'world_codex', 'playerbots_codex'],
                    'auth_port': 3744, 'world_port': 8105, 'soap_port': 7898}
            for changes in ({'world_port': 8085}, {'databases': ['auth', 'characters', 'world', 'playerbots']},
                            {'agent': 'claude'}, {'profile': 'dev'}, {'load': 'scale'}):
                (runtime / 'environment.json').write_text(json.dumps(dict(meta, **changes)))
                with self.assertRaises(ValueError):
                    f.Environment(runtime, root=root)

    def test_world_config_cannot_hide_a_foreign_bot_database(self):
        with tempfile.TemporaryDirectory() as directory:
            root = (Path(directory) / 'codex').resolve()
            runtime = root / 'build/test'
            runtime.mkdir(parents=True)
            metadata = {'agent': 'codex', 'profile': 'test', 'load': 'none',
                        'databases': ['auth_codex', 'characters_codex', 'world_codex', 'playerbots_codex'],
                        'auth_port': 3744, 'world_port': 8105, 'soap_port': 7898}
            (runtime / 'environment.json').write_text(json.dumps(metadata))
            config = '\n'.join(kind + 'DatabaseInfo = "127.0.0.1;3306;test;secret;' + database + '"'
                               for kind, database in [('Login', 'auth_codex'), ('Character', 'characters_codex'),
                               ('World', 'world_codex'), ('Playerbots', 'playerbots_claude')])
            (runtime / 'worldserver.conf').write_text(config)
            with self.assertRaises(ValueError):
                f.Environment(runtime, root=root)

    def test_character_identity_or_online_state_must_match(self):
        fixture = f.load_catalog()['fixtures'][0]
        env = SimpleNamespace(query=Mock())
        for row in (['9', '999', '2', '1', '1', '0'], ['9', '7', '1', '1', '1', '0'],
                    ['9', '7', '2', '8', '1', '0'], ['9', '7', '2', '1', '1', '1']):
            env.query.return_value = [row]
            with self.assertRaises(ValueError):
                f.character(env, {'id': 7}, fixture)

    def test_equipment_above_fixture_level_is_rejected(self):
        fixture = f.load_catalog()['fixtures'][0]
        env = SimpleNamespace(databases=['auth_codex', 'characters_codex', 'world_codex', 'playerbots_codex'],
                              query=Mock(side_effect=[[['1', '2', '1']],
                              [['3', '38', '1', '0'], ['6', '39', '2', '0'], ['15', '12282', '3', '60']]]))
        with self.assertRaises(ValueError):
            f.state(env, {'id': 7}, fixture, 9)

    def test_repeated_provision_verifies_without_regearing(self):
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            (runtime / 'fixture-manifest.json').write_text('{}')
            env = SimpleNamespace(runtime=runtime, running=Mock(return_value=True), meta={'load': 'none'}, query=Mock())
            saved = {'fixtures': {'one': {'guid': 7, 'equipment': [[15, 12282, 99, 0]]}}}
            with patch.object(f, 'account_for', return_value={'id': 7}), patch.object(f, 'verify', return_value=saved):
                self.assertEqual(f.provision(env), saved)
            env.query.assert_not_called()

    def test_full_roster_is_preflighted_before_checkpoint_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            env = SimpleNamespace(runtime=Path(directory), running=Mock(return_value=True), meta={'load': 'none'},
                                  catalog=f.load_catalog(), query=Mock(), client=Mock())
            with patch.object(f, 'account_for', return_value={'id': 7}), \
                 patch.object(f, 'character', side_effect=[None, ValueError('foreign character')]):
                with self.assertRaises(ValueError):
                    f.provision(env)
            env.query.assert_not_called()
            env.client.assert_not_called()

    def test_manifest_detects_changed_equipment(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = f.load_catalog()['fixtures'][0]
            saved = {'version': 1, 'agent': 'codex', 'account_id': 7,
                     'fixtures': {fixture['id']: {'equipment': [[15, 12282, 99, 0]]}}}
            runtime = Path(directory)
            (runtime / 'fixture-manifest.json').write_text(json.dumps(saved))
            env = SimpleNamespace(runtime=runtime, agent='codex', catalog=f.load_catalog())
            with patch.object(f, 'character', return_value=9), patch.object(f, 'state', return_value={'equipment': [[15, 12282, 100, 0]]}):
                with self.assertRaises(ValueError):
                    f.verify(env, {'id': 7})

    def test_closed_world_and_uncertain_timeout_are_distinct(self):
        env = SimpleNamespace(meta={'world_port': 8105}, limits={'probe_seconds': 5})
        with patch.object(f.socket, 'create_connection', side_effect=ConnectionRefusedError):
            self.assertFalse(f.Environment.running(env))
        with patch.object(f.socket, 'create_connection', side_effect=f.socket.timeout):
            with self.assertRaises(RuntimeError):
                f.Environment.running(env)

    def test_live_snapshot_is_rejected_before_capture(self):
        env = SimpleNamespace(running=Mock(return_value=True))
        with patch.object(f, 'safety_module') as module:
            with self.assertRaises(ValueError):
                f.snapshot(env, Path('unused'), 20)
            module.assert_not_called()


if __name__ == '__main__':
    unittest.main()

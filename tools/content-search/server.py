"""Dependency-free MCP stdio server and CLI adapter for MoP 5.4.8 content search (T-030)."""
import argparse
import json
import os
from pathlib import Path
import sys
from typing import Any, Dict, Optional, Tuple

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.dbc import DbcDirectory
from search_engine import ContentSearchEngine

_ENGINE_CACHE: Dict[Tuple[str, str, str], ContentSearchEngine] = {}


def get_engine() -> ContentSearchEngine:
    default_dbc = str(Path(__file__).resolve().parents[2] / 'gamedata' / 'dbc')
    dbc_dir = os.environ.get('MOP_DBC_DIR', default_dbc).strip()
    locale = os.environ.get('MOP_DBC_LOCALE', 'enUS').strip() or 'enUS'
    world_db = os.environ.get('MOP_WORLD_DB', '').strip()
    key = (str(Path(dbc_dir).resolve()), locale, world_db)
    engine = _ENGINE_CACHE.get(key)
    if engine is None:
        engine = ContentSearchEngine(dbc_dir=dbc_dir, locale=locale, world_db=world_db)
        _ENGINE_CACHE[key] = engine
    return engine


def schema(properties: Optional[Dict[str, Any]] = None, required: Optional[list] = None) -> Dict[str, Any]:
    return {
        'type': 'object',
        'properties': properties or {},
        'required': required or [],
        'additionalProperties': False,
    }


TOOLS = [
    (
        'search_spells',
        'Search spells in Spell.dbc, SpellMisc.dbc, SpellEffect.dbc, SpellIcon.dbc, and SpellVisual.db2.',
        schema({
            'query': {'type': 'string', 'description': 'Case-insensitive text search across spell name, rank, description, tooltip, and icon.'},
            'spell_id': {'type': 'integer', 'minimum': 1},
            'effect': {'type': 'integer', 'minimum': 0, 'description': 'Filter by SpellEffect.effect ID (e.g. 6 = APPLY_AURA).'},
            'aura': {'type': 'integer', 'minimum': 0, 'description': 'Filter by SpellEffect.apply_aura_name ID (e.g. 42 = PROC_TRIGGER_SPELL).'},
            'school_mask': {'type': 'integer', 'minimum': 1, 'description': 'Bitmask filter on SpellMisc.school_mask.'},
            'icon': {'type': 'string', 'description': 'Substring filter on SpellIcon path or exact icon ID.'},
            'visual_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter spells using a specific SpellVisual.db2 ID.'},
            'has_visual': {'type': 'boolean', 'description': 'If true, only return spells with resolved visual effect models.'},
            'limit': {'type': 'integer', 'minimum': 1, 'maximum': 100},
        }),
    ),
    (
        'search_visuals',
        'Search spell visuals (SpellVisual.db2, SpellVisualKit.db2, SpellVisualEffectName.db2), weapon glow enchants (SpellItemEnchantment.dbc, ItemVisuals.dbc, ItemVisualEffects.dbc), and spell icons (SpellIcon.dbc).',
        schema({
            'query': {'type': 'string', 'description': 'Case-insensitive search across visual .mdx/.mdl paths, enchant descriptions, sample spell names, or icon paths.'},
            'kind': {'type': 'string', 'enum': ['all', 'spell_visual', 'visual_effect', 'weapon_glow', 'icon']},
            'visual_id': {'type': 'integer', 'minimum': 1, 'description': 'Exact SpellVisual.db2 ID.'},
            'kit_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter SpellVisuals containing this SpellVisualKit.db2 ID.'},
            'effect_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter by SpellVisualEffectName.db2 ID.'},
            'enchant_id': {'type': 'integer', 'minimum': 1, 'description': 'Exact SpellItemEnchantment.dbc ID.'},
            'item_visual_id': {'type': 'integer', 'minimum': 1, 'description': 'Exact ItemVisuals.dbc ID (SpellItemEnchantment.aura_id).'},
            'limit': {'type': 'integer', 'minimum': 1, 'maximum': 100},
        }),
    ),
    (
        'search_items',
        'Search items across Item.db2, Item-sparse.db2, and ItemDisplayInfo.dbc.',
        schema({
            'query': {'type': 'string', 'description': 'Case-insensitive text search across item name, description, icon, and model names.'},
            'item_id': {'type': 'integer', 'minimum': 1},
            'display_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter by ItemDisplayInfo.dbc ID.'},
            'item_class': {'type': 'integer', 'minimum': 0, 'description': 'Filter by Item.db2 class (e.g. 2 = Weapon, 4 = Armor).'},
            'item_subclass': {'type': 'integer', 'minimum': 0},
            'inventory_type': {'type': 'integer', 'minimum': 0, 'description': 'Filter by inventory type (e.g. 13 = One-Hand, 17 = Two-Hand, 21 = Main Hand).'},
            'quality': {'type': 'integer', 'minimum': 0, 'maximum': 7},
            'min_ilvl': {'type': 'integer', 'minimum': 0},
            'max_ilvl': {'type': 'integer', 'minimum': 0},
            'min_req_level': {'type': 'integer', 'minimum': 0},
            'max_req_level': {'type': 'integer', 'minimum': 0},
            'has_item_visual': {'type': 'boolean'},
            'limit': {'type': 'integer', 'minimum': 1, 'maximum': 100},
        }),
    ),
    (
        'search_creatures',
        'Search creatures across Creature.db2 (and world.creature_template when MOP_WORLD_DB is configured), joined with CreatureDisplayInfo.dbc and CreatureModelData.dbc.',
        schema({
            'query': {'type': 'string', 'description': 'Case-insensitive text search across creature name, subname, .m2 model path, and skin names.'},
            'creature_id': {'type': 'integer', 'minimum': 1},
            'display_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter creatures using this CreatureDisplayInfo.dbc ID.'},
            'creature_type': {'type': 'integer', 'minimum': 0},
            'family': {'type': 'integer', 'minimum': 0},
            'min_level': {'type': 'integer', 'minimum': 0},
            'max_level': {'type': 'integer', 'minimum': 0},
            'rank': {'type': 'integer', 'minimum': 0},
            'limit': {'type': 'integer', 'minimum': 1, 'maximum': 100},
        }),
    ),
    (
        'search_display_ids',
        'Search display IDs across CreatureDisplayInfo.dbc (+ CreatureModelData.dbc), ItemDisplayInfo.dbc (+ ItemVisuals.dbc), and GameObjectDisplayInfo.dbc.',
        schema({
            'query': {'type': 'string', 'description': 'Case-insensitive text search across model paths (.m2/.mdx/.wmo), skin textures, icons, and linked entity names.'},
            'display_id': {'type': 'integer', 'minimum': 1},
            'kind': {'type': 'string', 'enum': ['all', 'creature', 'item', 'gameobject']},
            'model_id': {'type': 'integer', 'minimum': 1, 'description': 'Filter creature displays by CreatureModelData.dbc ID.'},
            'has_item_visual': {'type': 'boolean', 'description': 'Filter item displays with a non-zero ItemVisuals.dbc ID.'},
            'limit': {'type': 'integer', 'minimum': 1, 'maximum': 100},
        }),
    ),
]


def _validate_args(name: str, args: Any) -> Dict[str, Any]:
    spec = next((s for n, _, s in TOOLS if n == name), None)
    if spec is None:
        raise ValueError('unknown tool: ' + str(name))
    if not isinstance(args, dict) or set(args) - set(spec['properties']) or set(spec['required']) - set(args):
        raise ValueError('invalid tool arguments')
    for key, value in args.items():
        prop = spec['properties'][key]
        kind = prop['type']
        if kind == 'string':
            if not isinstance(value, str):
                raise ValueError('invalid argument: ' + key)
            if 'enum' in prop and value not in prop['enum']:
                raise ValueError(f"invalid argument {key}: must be one of {prop['enum']}")
        elif kind == 'integer':
            if type(value) is not int:
                raise ValueError('invalid argument: ' + key)
            if 'minimum' in prop and value < prop['minimum']:
                raise ValueError(f"invalid argument {key}: must be >= {prop['minimum']}")
            if 'maximum' in prop and value > prop['maximum']:
                raise ValueError(f"invalid argument {key}: must be <= {prop['maximum']}")
        elif kind == 'boolean':
            if type(value) is not bool:
                raise ValueError('invalid argument: ' + key)
    return args


def call(name: str, args: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    validated = _validate_args(name, args or {})
    engine = get_engine()
    if name == 'search_spells':
        return engine.search_spells(**validated)
    if name == 'search_visuals':
        return engine.search_visuals(**validated)
    if name == 'search_items':
        return engine.search_items(**validated)
    if name == 'search_creatures':
        return engine.search_creatures(**validated)
    if name == 'search_display_ids':
        return engine.search_display_ids(**validated)
    raise ValueError('unknown tool: ' + str(name))


def dispatch(request: Any) -> Optional[Dict[str, Any]]:
    if not isinstance(request, dict) or request.get('jsonrpc') != '2.0' or not isinstance(request.get('method'), str):
        return {'jsonrpc': '2.0', 'id': None, 'error': {'code': -32600, 'message': 'Invalid Request'}}
    if 'id' not in request:
        return None
    reply: Dict[str, Any] = {'jsonrpc': '2.0', 'id': request['id']}
    method, params = request['method'], request.get('params', {})
    if method == 'initialize':
        result: Any = {
            'protocolVersion': '2024-11-05',
            'capabilities': {'tools': {}},
            'serverInfo': {'name': 'ultimatemop-content-search', 'version': '0.1.0'},
        }
    elif method == 'ping':
        result = {}
    elif method == 'tools/list':
        result = {'tools': [{'name': n, 'description': d, 'inputSchema': s} for n, d, s in TOOLS]}
    elif method == 'tools/call':
        if not isinstance(params, dict):
            reply['error'] = {'code': -32602, 'message': 'Invalid params'}
            return reply
        try:
            value = call(params.get('name', ''), params.get('arguments', {}))
            text = value if isinstance(value, str) else json.dumps(value)
            result = {'content': [{'type': 'text', 'text': text}]}
        except (ValueError, OSError, KeyError, IndexError, TypeError) as exc:
            result = {'isError': True, 'content': [{'type': 'text', 'text': str(exc)}]}
    else:
        reply['error'] = {'code': -32601, 'message': 'Method not found'}
        return reply
    reply['result'] = result
    return reply


def run_cli(argv: list) -> int:
    parser = argparse.ArgumentParser(description='UltimateMoP Content Search CLI (T-030)')
    parser.add_argument('--cli', required=True, choices=[n for n, _, _ in TOOLS], help='Tool name to execute')
    parser.add_argument('--query', default=None, help='Text query')
    parser.add_argument('--kind', default=None, help='Kind filter for search_visuals or search_display_ids')
    parser.add_argument('--id', type=int, default=None, help='Primary ID filter (spell_id, visual_id, item_id, creature_id, display_id)')
    parser.add_argument('--enchant-id', type=int, default=None, help='Enchant ID for search_visuals')
    parser.add_argument('--display-id', type=int, default=None, help='Display ID filter')
    parser.add_argument('--limit', type=int, default=20, help='Maximum results (1..100)')
    ns = parser.parse_args(argv)

    tool_args: Dict[str, Any] = {'limit': ns.limit}
    if ns.query is not None:
        tool_args['query'] = ns.query
    if ns.kind is not None:
        tool_args['kind'] = ns.kind
    if ns.enchant_id is not None:
        tool_args['enchant_id'] = ns.enchant_id
    if ns.display_id is not None:
        tool_args['display_id'] = ns.display_id
    if ns.id is not None:
        id_Field_map = {
            'search_spells': 'spell_id',
            'search_visuals': 'visual_id',
            'search_items': 'item_id',
            'search_creatures': 'creature_id',
            'search_display_ids': 'display_id',
        }
        tool_args[id_Field_map[ns.cli]] = ns.id

    out = call(ns.cli, tool_args)
    print(json.dumps(out, indent=2))
    return 0


def main(argv: Optional[list] = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if args and '--cli' in args:
        return run_cli(args)
    for line in sys.stdin:
        try:
            response = dispatch(json.loads(line))
        except (ValueError, TypeError):
            response = {'jsonrpc': '2.0', 'id': None, 'error': {'code': -32700, 'message': 'Parse error'}}
        if response is not None:
            print(json.dumps(response, ensure_ascii=True), flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())

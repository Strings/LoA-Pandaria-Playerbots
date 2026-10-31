"""Content search engine over MoP 5.4.8 DBC/DB2 stores and optional world database."""
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Any, Dict, Iterable, List, Optional, Sequence, Set, Tuple, Union

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.dbc import DbcDirectory

IDENT_RE = re.compile(r'^[A-Za-z0-9_]+$')
UINT32_NONE = 0xFFFFFFFF

SPELL_VISUAL_KIT_STAGES = (
    ('precast', 1),
    ('cast', 2),
    ('impact', 3),
    ('state', 4),
    ('state_done', 5),
    ('channel', 6),
    ('caster_impact', 14),
    ('target_impact', 15),
    ('missile_targeting', 22),
    ('instant_area', 23),
    ('impact_area', 24),
    ('persistent_area', 25),
    ('alternative', 29),
)

KIT_EFFECT_SLOTS = (
    ('head', 4),
    ('chest', 5),
    ('base', 6),
    ('left_hand', 7),
    ('right_hand', 8),
    ('breath', 9),
    ('left_weapon', 10),
    ('right_weapon', 11),
    ('special_1', 12),
    ('special_2', 13),
    ('special_3', 14),
    ('world', 15),
)


def _tokenize_query(query: Optional[str]) -> List[str]:
    if not query or not query.strip():
        return []
    return [tok for tok in query.strip().lower().split() if tok]


def _score_match(tokens: Sequence[str], primary: str, *secondary: str) -> int:
    """Return >0 match score if all tokens appear across primary/secondary strings, else 0."""
    if not tokens:
        return 1
    prim_low = (primary or '').lower()
    sec_low = ' '.join(s for s in secondary if s).lower()
    combined = prim_low + ' ' + sec_low
    for tok in tokens:
        if tok not in combined:
            return 0
    score = 10
    full_q = ' '.join(tokens)
    if prim_low == full_q:
        score += 100
    elif prim_low.startswith(full_q):
        score += 60
    elif full_q in prim_low:
        score += 40
    elif all(tok in prim_low for tok in tokens):
        score += 25
    return score


class ContentSearchEngine:
    """Lazy-loading content search engine over DBC/DB2 stores and optional world MySQL DB."""

    def __init__(self, dbc_dir: Optional[Union[str, Path]] = None, locale: str = 'enUS',
                 world_db: Optional[str] = None) -> None:
        self.dbc = DbcDirectory(dbc_dir=dbc_dir, locale=locale)
        self.world_db = world_db if world_db is not None else os.environ.get('MOP_WORLD_DB', '').strip()

        # Lazy caches built on first use
        self._spell_effects_by_spell: Optional[Dict[int, List[Tuple[Any, ...]]]] = None
        self._kit_attaches_by_kit: Optional[Dict[int, List[int]]] = None
        self._missiles_by_set: Optional[Dict[int, List[Tuple[int, int]]]] = None
        self._spells_by_visual: Optional[Dict[int, List[Tuple[int, str]]]] = None
        self._item_visual_models: Optional[Dict[int, List[str]]] = None
        self._items_by_display: Optional[Dict[int, List[Tuple[int, str]]]] = None
        self._creatures_by_display: Optional[Dict[int, List[Tuple[int, str]]]] = None
        self._go_by_display: Optional[Dict[int, List[Tuple[int, str]]]] = None

    # -----------------------------------------------------------------------
    # Internal Lazy Indexes
    # -----------------------------------------------------------------------

    def _get_spell_effects_map(self) -> Dict[int, List[Tuple[Any, ...]]]:
        if self._spell_effects_by_spell is None:
            mapping: Dict[int, List[Tuple[Any, ...]]] = {}
            eff_table = self.dbc.load('SpellEffect.dbc')
            for row in eff_table.iter_raw_tuples():
                # 1=difficulty, 27=spell_id
                if row[1] != 0:
                    continue
                mapping.setdefault(row[27], []).append(row)
            for rows in mapping.values():
                rows.sort(key=lambda r: r[28])
            self._spell_effects_by_spell = mapping
        return self._spell_effects_by_spell

    def _get_kit_attaches_map(self) -> Dict[int, List[int]]:
        if self._kit_attaches_by_kit is None:
            mapping: Dict[int, List[int]] = {}
            table = self.dbc.load('SpellVisualKitModelAttach.db2')
            for row in table.iter_raw_tuples():
                eff_id = row[2]
                if eff_id and eff_id != UINT32_NONE:
                    mapping.setdefault(row[1], []).append(eff_id)
            self._kit_attaches_by_kit = mapping
        return self._kit_attaches_by_kit

    def _get_missiles_by_set(self) -> Dict[int, List[Tuple[int, int]]]:
        if self._missiles_by_set is None:
            mapping: Dict[int, List[Tuple[int, int]]] = {}
            table = self.dbc.load('SpellVisualMissile.db2')
            for row in table.iter_raw_tuples():
                # 1=missile_set_id, 2=spell_visual_effect_name_id, 4=impact_kit
                mapping.setdefault(row[1], []).append((row[2], row[4]))
            self._missiles_by_set = mapping
        return self._missiles_by_set

    def _get_spells_by_visual(self) -> Dict[int, List[Tuple[int, str]]]:
        if self._spells_by_visual is None:
            spells = self.dbc.load('Spell.dbc')
            misc = self.dbc.load('SpellMisc.dbc')
            get_s = spells.get_string
            mapping: Dict[int, List[Tuple[int, str]]] = {}
            for srow in spells.iter_raw_tuples():
                sid = srow[0]
                misc_id = srow[24]
                if not misc_id:
                    continue
                mrec = misc.get(misc_id)
                if mrec is None:
                    continue
                v1 = mrec.get_uint(21)
                v2 = mrec.get_uint(22)
                if v1 or v2:
                    sname = get_s(srow[1])
                    if v1 and len(mapping.setdefault(v1, [])) < 10:
                        mapping[v1].append((sid, sname))
                    if v2 and v2 != v1 and len(mapping.setdefault(v2, [])) < 10:
                        mapping[v2].append((sid, sname))
            self._spells_by_visual = mapping
        return self._spells_by_visual

    def _get_item_visual_models_map(self) -> Dict[int, List[str]]:
        if self._item_visual_models is None:
            ivis = self.dbc.load('ItemVisuals.dbc')
            iveff = self.dbc.load('ItemVisualEffects.dbc')
            eff_map: Dict[int, str] = {}
            for erow in iveff.iter_raw_tuples():
                eff_map[erow[0]] = iveff.get_string(erow[1])
            mapping: Dict[int, List[str]] = {}
            for vrow in ivis.iter_raw_tuples():
                models = [eff_map.get(slot_id, '') for slot_id in vrow[1:6] if slot_id and slot_id in eff_map]
                mapping[vrow[0]] = [m for m in models if m]
            self._item_visual_models = mapping
        return self._item_visual_models

    def _get_items_by_display_map(self) -> Dict[int, List[Tuple[int, str]]]:
        if self._items_by_display is None:
            items = self.dbc.load('Item.db2')
            sparse = self.dbc.load('Item-sparse.db2')
            mapping: Dict[int, List[Tuple[int, str]]] = {}
            for row in items.iter_raw_tuples():
                item_id, disp_id = row[0], row[5]
                if not disp_id:
                    continue
                bucket = mapping.setdefault(disp_id, [])
                if len(bucket) < 8:
                    srec = sparse.get(item_id)
                    name = srec.get_string(100) if srec is not None else ''
                    bucket.append((item_id, name))
            self._items_by_display = mapping
        return self._items_by_display

    def _get_creatures_by_display_map(self) -> Dict[int, List[Tuple[int, str]]]:
        if self._creatures_by_display is None:
            creatures = self.dbc.load('Creature.db2')
            mapping: Dict[int, List[Tuple[int, str]]] = {}
            for row in creatures.iter_raw_tuples():
                cid = row[0]
                cname = creatures.get_string(row[13])
                for d_id in row[5:9]:
                    if d_id and len(mapping.setdefault(d_id, [])) < 8:
                        mapping[d_id].append((cid, cname))
            self._creatures_by_display = mapping
        return self._creatures_by_display

    def _get_go_by_display_map(self) -> Dict[int, List[Tuple[int, str]]]:
        if self._go_by_display is None:
            gos = self.dbc.load('GameObjects.db2')
            mapping: Dict[int, List[Tuple[int, str]]] = {}
            for row in gos.iter_raw_tuples():
                gid, disp_id = row[0], row[2]
                if disp_id and len(mapping.setdefault(disp_id, [])) < 8:
                    mapping[disp_id].append((gid, gos.get_string(row[16])))
            self._go_by_display = mapping
        return self._go_by_display

    def _resolve_kit_models(self, kit_id: int) -> List[Dict[str, Any]]:
        if not kit_id or kit_id == UINT32_NONE:
            return []
        svk = self.dbc.load('SpellVisualKit.db2')
        sve = self.dbc.load('SpellVisualEffectName.db2')
        krec = svk.get(kit_id)
        if krec is None:
            return []
        kvals = krec._ensure_values()
        effects: List[Dict[str, Any]] = []
        seen_eff_ids: Set[int] = set()
        for slot_name, col_idx in KIT_EFFECT_SLOTS:
            eff_id = kvals[col_idx]
            if eff_id and eff_id != UINT32_NONE and eff_id in sve:
                seen_eff_ids.add(eff_id)
                effects.append({
                    'slot': slot_name,
                    'effect_id': eff_id,
                    'file_path': sve[eff_id].file_path,
                })
        for eff_id in self._get_kit_attaches_map().get(kit_id, ()):
            if eff_id not in seen_eff_ids and eff_id in sve:
                seen_eff_ids.add(eff_id)
                effects.append({
                    'slot': 'model_attach',
                    'effect_id': eff_id,
                    'file_path': sve[eff_id].file_path,
                })
        return effects

    def _resolve_spell_visual_summary(self, visual_id: int) -> Optional[Dict[str, Any]]:
        if not visual_id or visual_id == UINT32_NONE:
            return None
        sv = self.dbc.load('SpellVisual.db2')
        sve = self.dbc.load('SpellVisualEffectName.db2')
        vrec = sv.get(visual_id)
        if vrec is None:
            return None
        vvals = vrec._ensure_values()
        kits: Dict[str, Any] = {}
        all_models: List[str] = []
        seen_models: Set[str] = set()

        for stage_name, col_idx in SPELL_VISUAL_KIT_STAGES:
            kit_id = vvals[col_idx]
            if kit_id and kit_id != UINT32_NONE:
                effs = self._resolve_kit_models(kit_id)
                if effs:
                    kits[stage_name] = {'kit_id': kit_id, 'effects': effs}
                    for e in effs:
                        fp = e['file_path']
                        if fp and fp not in seen_models:
                            seen_models.add(fp)
                            all_models.append(fp)

        missile_models: List[str] = []
        has_missile = vvals[7]
        missile_model_id = vvals[8]
        if has_missile and missile_model_id and missile_model_id != UINT32_NONE:
            if missile_model_id in sve:
                mp = sve[missile_model_id].file_path
                if mp:
                    missile_models.append(mp)
                    if mp not in seen_models:
                        seen_models.add(mp)
                        all_models.append(mp)
            for eff_id, imp_kit in self._get_missiles_by_set().get(missile_model_id, ()):
                if eff_id in sve:
                    mp = sve[eff_id].file_path
                    if mp and mp not in missile_models:
                        missile_models.append(mp)
                    if mp and mp not in seen_models:
                        seen_models.add(mp)
                        all_models.append(mp)

        sample_spells = [
            {'id': sid, 'name': sname}
            for sid, sname in self._get_spells_by_visual().get(visual_id, ())
        ]
        return {
            'kind': 'spell_visual',
            'visual_id': visual_id,
            'kits': kits,
            'missile_models': missile_models,
            'all_models': all_models,
            'sample_spells': sample_spells,
        }

    # -----------------------------------------------------------------------
    # 1. Search Spells
    # -----------------------------------------------------------------------

    def search_spells(
        self,
        query: Optional[str] = None,
        spell_id: Optional[int] = None,
        effect: Optional[int] = None,
        aura: Optional[int] = None,
        school_mask: Optional[int] = None,
        icon: Optional[str] = None,
        visual_id: Optional[int] = None,
        has_visual: Optional[bool] = None,
        limit: int = 20,
    ) -> Dict[str, Any]:
        limit = max(1, min(int(limit), 100))
        tokens = _tokenize_query(query)
        icon_filter = (icon or '').strip().lower()

        spells = self.dbc.load('Spell.dbc')
        misc_table = self.dbc.load('SpellMisc.dbc')
        icons_table = self.dbc.load('SpellIcon.dbc')
        cooldowns_table = self.dbc.load('SpellCooldowns.dbc')
        levels_table = self.dbc.load('SpellLevels.dbc')
        durations_table = self.dbc.load('SpellDuration.dbc')
        cast_times_table = self.dbc.load('SpellCastTimes.dbc')
        effects_by_spell = self._get_spell_effects_map() if (effect is not None or aura is not None or spell_id is not None or tokens) else None

        get_str = spells.get_string
        scored: List[Tuple[int, int, Tuple[Any, ...]]] = []

        if spell_id is not None:
            rec = spells.get(int(spell_id))
            candidate_rows = [rec._table._row_struct.unpack_from(rec._table._raw, rec._row_offset)] if rec is not None else []
        else:
            candidate_rows = spells.iter_raw_tuples()

        for row in candidate_rows:
            sid = row[0]
            misc_id = row[24]
            mrec = misc_table.get(misc_id) if misc_id else None
            vis1 = mrec.get_uint(21) if mrec is not None else 0
            vis2 = mrec.get_uint(22) if mrec is not None else 0
            s_mask = mrec.get_uint(25) if mrec is not None else 0
            icon_id = mrec.get_uint(23) if mrec is not None else 0

            if school_mask is not None and (s_mask & int(school_mask)) == 0:
                continue
            if visual_id is not None and int(visual_id) not in (vis1, vis2):
                continue
            if has_visual is True and not (vis1 or vis2):
                continue
            if has_visual is False and (vis1 or vis2):
                continue

            if effect is not None or aura is not None:
                eff_rows = self._get_spell_effects_map().get(sid, ())
                if effect is not None and not any(er[2] == int(effect) for er in eff_rows):
                    continue
                if aura is not None and not any(er[4] == int(aura) for er in eff_rows):
                    continue

            icon_path = icons_table[icon_id].icon_path if (icon_id and icon_id in icons_table) else ''
            if icon_filter:
                if icon_filter.isdigit():
                    if icon_id != int(icon_filter):
                        continue
                elif icon_filter not in icon_path.lower():
                    continue

            name = get_str(row[1])
            rank = get_str(row[2])
            desc = get_str(row[3])
            tooltip = get_str(row[4])

            if tokens:
                score = _score_match(tokens, name, rank, desc, tooltip, icon_path)
                if score == 0:
                    continue
            else:
                score = 1

            if has_visual is True and (vis1 or vis2):
                v_summary = self._resolve_spell_visual_summary(vis1 or vis2)
                if not v_summary or not v_summary['all_models']:
                    continue

            scored.append((score, -sid, row))
            if not tokens and len(scored) >= limit:
                break

        scored.sort(reverse=True)
        results: List[Dict[str, Any]] = []
        eff_map = self._get_spell_effects_map()

        for _, _, row in scored[:limit]:
            sid = row[0]
            misc_id = row[24]
            mrec = misc_table.get(misc_id) if misc_id else None
            vis1 = mrec.get_uint(21) if mrec is not None else 0
            vis2 = mrec.get_uint(22) if mrec is not None else 0
            icon_id = mrec.get_uint(23) if mrec is not None else 0
            icon_path = icons_table[icon_id].icon_path if (icon_id and icon_id in icons_table) else ''

            cast_idx = mrec.get_uint(17) if mrec is not None else 0
            dur_idx = mrec.get_uint(18) if mrec is not None else 0
            cast_time_ms = cast_times_table[cast_idx].cast_time if (cast_idx and cast_idx in cast_times_table) else 0
            duration_ms = durations_table[dur_idx].duration if (dur_idx and dur_idx in durations_table) else 0

            cd_id = row[15]
            cd_rec = cooldowns_table.get(cd_id) if cd_id else None
            lvl_id = row[18]
            lvl_rec = levels_table.get(lvl_id) if lvl_id else None

            visual_models: List[str] = []
            for vid in (vis1, vis2):
                if vid:
                    vs = self._resolve_spell_visual_summary(vid)
                    if vs:
                        for m in vs['all_models']:
                            if m not in visual_models:
                                visual_models.append(m)

            eff_list = []
            for er in eff_map.get(sid, ()):
                eff_list.append({
                    'index': er[28],
                    'effect': er[2],
                    'aura': er[4],
                    'base_points': er[6],
                    'die_sides': er[10],
                    'amplitude_ms': er[5],
                    'misc_value': er[13],
                    'misc_value_b': er[14],
                    'trigger_spell': er[23],
                })

            results.append({
                'id': sid,
                'name': get_str(row[1]),
                'rank': get_str(row[2]),
                'description': get_str(row[3]),
                'tooltip': get_str(row[4]),
                'school_mask': mrec.get_uint(25) if mrec is not None else 0,
                'icon_id': icon_id,
                'icon_path': icon_path,
                'spell_visual_ids': [v for v in (vis1, vis2) if v],
                'visual_models': visual_models,
                'cast_time_ms': cast_time_ms,
                'duration_ms': duration_ms,
                'recovery_time_ms': cd_rec.recovery_time if cd_rec is not None else 0,
                'category_recovery_time_ms': cd_rec.category_recovery_time if cd_rec is not None else 0,
                'base_level': lvl_rec.base_level if lvl_rec is not None else 0,
                'max_level': lvl_rec.max_level if lvl_rec is not None else 0,
                'spell_level': lvl_rec.spell_level if lvl_rec is not None else 0,
                'effects': eff_list,
            })

        return {'count': len(results), 'results': results}

    # -----------------------------------------------------------------------
    # 2. Search Visuals (SpellVisual, VisualEffect, WeaponGlow, Icon)
    # -----------------------------------------------------------------------

    def search_visuals(
        self,
        query: Optional[str] = None,
        kind: str = 'all',
        visual_id: Optional[int] = None,
        kit_id: Optional[int] = None,
        effect_id: Optional[int] = None,
        enchant_id: Optional[int] = None,
        item_visual_id: Optional[int] = None,
        limit: int = 20,
    ) -> Dict[str, Any]:
        limit = max(1, min(int(limit), 100))
        kind_norm = (kind or 'all').strip().lower()
        if kind_norm not in ('all', 'spell_visual', 'visual_effect', 'weapon_glow', 'icon'):
            raise ValueError(f"Unsupported visual kind {kind!r}; expected all, spell_visual, visual_effect, weapon_glow, or icon")
        tokens = _tokenize_query(query)
        scored: List[Tuple[int, int, Dict[str, Any]]] = []

        # A) Weapon glow enchants (SpellItemEnchantment -> ItemVisuals -> ItemVisualEffects)
        if kind_norm in ('all', 'weapon_glow') and visual_id is None and kit_id is None and effect_id is None:
            ench_table = self.dbc.load('SpellItemEnchantment.dbc')
            iv_models = self._get_item_visual_models_map()
            for row in ench_table.iter_raw_tuples():
                eid = row[0]
                aura_id = row[12]
                if enchant_id is not None and eid != int(enchant_id):
                    continue
                if item_visual_id is not None and aura_id != int(item_visual_id):
                    continue
                if enchant_id is None and item_visual_id is None and not aura_id:
                    continue
                desc = ench_table.get_string(row[11])
                models = iv_models.get(aura_id, [])
                unique_models = list(dict.fromkeys(models))
                score = _score_match(tokens, desc, ' '.join(unique_models), str(eid), str(aura_id))
                if score > 0:
                    scored.append((score, -eid, {
                        'kind': 'weapon_glow',
                        'enchant_id': eid,
                        'description': desc,
                        'item_visual_id': aura_id,
                        'slot_models': models,
                        'unique_models': unique_models,
                    }))

        # B) Visual Effect models (SpellVisualEffectName.db2)
        if kind_norm in ('all', 'visual_effect') and visual_id is None and kit_id is None and enchant_id is None and item_visual_id is None:
            sve = self.dbc.load('SpellVisualEffectName.db2')
            for row in sve.iter_raw_tuples():
                eff_id_val = row[0]
                if effect_id is not None and eff_id_val != int(effect_id):
                    continue
                fp = sve.get_string(row[1])
                score = _score_match(tokens, fp, str(eff_id_val))
                if score > 0:
                    rec = sve[eff_id_val]
                    scored.append((score, -eff_id_val, {
                        'kind': 'visual_effect',
                        'effect_id': eff_id_val,
                        'file_path': fp,
                        'scale': round(rec.scale, 4),
                        'area_effect_size': round(rec.area_effect_size, 4),
                    }))

        # C) Full SpellVisual records (SpellVisual.db2 -> Kits -> Effects + Sample Spells)
        if kind_norm in ('all', 'spell_visual') and enchant_id is None and item_visual_id is None:
            sv = self.dbc.load('SpellVisual.db2')
            candidate_vids: Iterable[int]
            if visual_id is not None:
                candidate_vids = [int(visual_id)] if int(visual_id) in sv else []
            else:
                candidate_vids = (r[0] for r in sv.iter_raw_tuples())

            for vid in candidate_vids:
                vrec = sv.get(vid)
                if vrec is None:
                    continue
                vvals = vrec._ensure_values()
                if kit_id is not None and int(kit_id) not in vvals[1:]:
                    continue
                summary = self._resolve_spell_visual_summary(vid)
                if summary is None:
                    continue
                if effect_id is not None:
                    target_eff = int(effect_id)
                    has_eff = any(
                        e['effect_id'] == target_eff
                        for kinfo in summary['kits'].values()
                        for e in kinfo['effects']
                    )
                    if not has_eff:
                        continue
                if not summary['all_models'] and visual_id is None and kit_id is None:
                    continue
                spell_names = ' '.join(s['name'] for s in summary['sample_spells'])
                models_text = ' '.join(summary['all_models'])
                score = _score_match(tokens, models_text, spell_names, str(vid))
                if score > 0:
                    scored.append((score, -vid, summary))
                    if not tokens and len(scored) >= limit * 2:
                        break

        # D) Spell Icons (SpellIcon.dbc)
        if kind_norm in ('all', 'icon') and visual_id is None and kit_id is None and effect_id is None and enchant_id is None and item_visual_id is None:
            icons = self.dbc.load('SpellIcon.dbc')
            for row in icons.iter_raw_tuples():
                iid = row[0]
                ipath = icons.get_string(row[1])
                score = _score_match(tokens, ipath, str(iid))
                if score > 0:
                    scored.append((score, -iid, {
                        'kind': 'icon',
                        'icon_id': iid,
                        'icon_path': ipath,
                    }))

        scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
        results = [item[2] for item in scored[:limit]]
        return {'count': len(results), 'results': results}

    # -----------------------------------------------------------------------
    # 3. Search Items
    # -----------------------------------------------------------------------

    def search_items(
        self,
        query: Optional[str] = None,
        item_id: Optional[int] = None,
        display_id: Optional[int] = None,
        item_class: Optional[int] = None,
        item_subclass: Optional[int] = None,
        inventory_type: Optional[int] = None,
        quality: Optional[int] = None,
        min_ilvl: Optional[int] = None,
        max_ilvl: Optional[int] = None,
        min_req_level: Optional[int] = None,
        max_req_level: Optional[int] = None,
        has_item_visual: Optional[bool] = None,
        limit: int = 20,
    ) -> Dict[str, Any]:
        limit = max(1, min(int(limit), 100))
        tokens = _tokenize_query(query)

        items_db2 = self.dbc.load('Item.db2')
        sparse_db2 = self.dbc.load('Item-sparse.db2')
        disp_dbc = self.dbc.load('ItemDisplayInfo.dbc')
        iv_models = self._get_item_visual_models_map()

        scored: List[Tuple[int, int, Dict[str, Any]]] = []

        if item_id is not None:
            irec = items_db2.get(int(item_id))
            candidate_rows = [irec._table._row_struct.unpack_from(irec._table._raw, irec._row_offset)] if irec is not None else []
        else:
            candidate_rows = items_db2.iter_raw_tuples()

        for irow in candidate_rows:
            iid, iclass, isubclass, _, material, disp_id, inv_type, sheath = irow
            if display_id is not None and disp_id != int(display_id):
                continue
            if item_class is not None and iclass != int(item_class):
                continue
            if item_subclass is not None and isubclass != int(item_subclass):
                continue
            if inventory_type is not None and inv_type != int(inventory_type):
                continue

            srec = sparse_db2.get(iid)
            if srec is None:
                continue
            qual = srec.get_uint(1)
            ilvl = srec.get_uint(13)
            req_lvl = srec.get_int(14)

            if quality is not None and qual != int(quality):
                continue
            if min_ilvl is not None and ilvl < int(min_ilvl):
                continue
            if max_ilvl is not None and ilvl > int(max_ilvl):
                continue
            if min_req_level is not None and req_lvl < int(min_req_level):
                continue
            if max_req_level is not None and req_lvl > int(max_req_level):
                continue

            drec = disp_dbc.get(disp_id) if disp_id else None
            item_vis_id = drec.get_uint(24) if drec is not None else 0
            if has_item_visual is True and not item_vis_id:
                continue
            if has_item_visual is False and item_vis_id:
                continue

            name = srec.get_string(100)
            desc = srec.get_string(104)
            icon_name = drec.get_string(5) if drec is not None else ''
            left_model = drec.get_string(1) if drec is not None else ''
            right_model = drec.get_string(2) if drec is not None else ''

            if tokens:
                score = _score_match(tokens, name, desc, icon_name, left_model, right_model)
                if score == 0:
                    continue
            else:
                score = 1

            spell_ids = [srec.get_int(69 + idx) for idx in range(5)]
            spell_ids = [s for s in spell_ids if s > 0]

            scored.append((score, ilvl, {
                'id': iid,
                'name': name,
                'description': desc,
                'quality': qual,
                'item_level': ilvl,
                'required_level': req_lvl,
                'item_class': iclass,
                'item_subclass': isubclass,
                'inventory_type': inv_type,
                'material': material,
                'sheath': sheath,
                'bonding': srec.get_uint(99),
                'display_id': disp_id,
                'inventory_icon': icon_name,
                'models': [m for m in (left_model, right_model) if m],
                'model_textures': [
                    t for t in (
                        drec.get_string(3) if drec is not None else '',
                        drec.get_string(4) if drec is not None else '',
                    ) if t
                ],
                'spell_visual_id': drec.get_uint(11) if drec is not None else 0,
                'item_visual_id': item_vis_id,
                'item_visual_models': iv_models.get(item_vis_id, []) if item_vis_id else [],
                'spell_ids': spell_ids,
            }))
            if not tokens and len(scored) >= limit:
                break

        scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
        results = [item[2] for item in scored[:limit]]
        return {'count': len(results), 'results': results}

    # -----------------------------------------------------------------------
    # 4. Search Creatures (Creature.db2 + CreatureDisplayInfo/ModelData + optional world DB)
    # -----------------------------------------------------------------------

    def _resolve_creature_displays(self, display_ids: Iterable[int]) -> List[Dict[str, Any]]:
        cdi = self.dbc.load('CreatureDisplayInfo.dbc')
        cmd = self.dbc.load('CreatureModelData.dbc')
        resolved: List[Dict[str, Any]] = []
        for did in display_ids:
            if not did:
                continue
            drec = cdi.get(did)
            if drec is None:
                continue
            mid = drec.get_uint(1)
            mrec = cmd.get(mid) if mid else None
            skins = [drec.get_string(i) for i in (6, 7, 8) if drec.get_string(i)]
            resolved.append({
                'display_id': did,
                'model_id': mid,
                'model_path': mrec.get_string(2) if mrec is not None else '',
                'display_scale': round(drec.get_float(4), 3),
                'model_scale': round(mrec.get_float(4), 3) if mrec is not None else 1.0,
                'skins': skins,
            })
        return resolved

    def _query_world_creatures(
        self,
        tokens: Sequence[str],
        creature_id: Optional[int],
        display_id: Optional[int],
        creature_type: Optional[int],
        family: Optional[int],
        min_level: Optional[int],
        max_level: Optional[int],
        rank: Optional[int],
        limit: int,
    ) -> List[Dict[str, Any]]:
        if not self.world_db or not IDENT_RE.match(self.world_db):
            return []
        password = os.environ.get('MOP_DB_PASSWORD')
        if password is None:
            return []
        mysql_bin = os.environ.get('MOP_MYSQL_BIN', '')
        exe = str(Path(mysql_bin) / ('mysql.exe' if os.name == 'nt' else 'mysql')) if mysql_bin else 'mysql'
        if mysql_bin and not Path(exe).exists():
            return []

        where_clauses: List[str] = ['1=1']
        if creature_id is not None:
            where_clauses.append(f"ct.`entry` = {int(creature_id)}")
        if display_id is not None:
            did = int(display_id)
            where_clauses.append(
                f"EXISTS (SELECT 1 FROM `{self.world_db}`.`creature_template_model` m "
                f"WHERE m.`CreatureID` = ct.`entry` AND m.`CreatureDisplayID` = {did})"
            )
        if creature_type is not None:
            where_clauses.append(f"ct.`type` = {int(creature_type)}")
        if family is not None:
            where_clauses.append(f"ct.`family` = {int(family)}")
        if min_level is not None:
            where_clauses.append(f"ct.`maxlevel` >= {int(min_level)}")
        if max_level is not None:
            where_clauses.append(f"ct.`minlevel` <= {int(max_level)}")
        if rank is not None:
            where_clauses.append(f"ct.`rank` = {int(rank)}")
        for tok in tokens:
            safe_tok = re.sub(r"[^A-Za-z0-9_'\- ]", '', tok)
            if safe_tok:
                where_clauses.append(f"(ct.`name` LIKE '%{safe_tok}%' OR ct.`subname` LIKE '%{safe_tok}%')")

        sql = (
            f"SELECT ct.`entry`, ct.`name`, IFNULL(ct.`subname`, ''), ct.`minlevel`, ct.`maxlevel`, "
            f"ct.`rank`, ct.`type`, ct.`family`, ct.`faction`, ct.`npcflag`, ct.`scale`, "
            f"IFNULL((SELECT GROUP_CONCAT(ctm.`CreatureDisplayID` ORDER BY ctm.`Idx` SEPARATOR ',') "
            f"FROM `{self.world_db}`.`creature_template_model` ctm WHERE ctm.`CreatureID` = ct.`entry`), '') "
            f"FROM `{self.world_db}`.`creature_template` ct "
            f"WHERE {' AND '.join(where_clauses)} LIMIT {int(limit)};"
        )
        env = dict(os.environ)
        env['MYSQL_PWD'] = password
        cmd = [
            exe, '--protocol=TCP',
            f"--host={os.environ.get('MOP_DB_HOST', '127.0.0.1')}",
            f"--port={os.environ.get('MOP_DB_PORT', '3306')}",
            f"--user={os.environ.get('MOP_DB_USER', 'ultimatemop')}",
            '--batch', '--raw', '--skip-column-names', '-e', sql,
        ]
        try:
            res = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        except OSError:
            return []
        if res.returncode != 0:
            return []

        fam_dbc = self.dbc.load('CreatureFamily.dbc')
        type_dbc = self.dbc.load('CreatureType.dbc')
        rows: List[Dict[str, Any]] = []
        for line in res.stdout.decode('utf-8', errors='replace').splitlines():
            parts = line.split('\t')
            if len(parts) < 12:
                continue
            entry = int(parts[0])
            ctype = int(parts[6])
            cfam = int(parts[7])
            dids = [int(x) for x in parts[11].split(',') if x.strip().isdigit() and int(x) > 0]
            rows.append({
                'id': entry,
                'name': parts[1],
                'subname': parts[2],
                'min_level': int(parts[3]),
                'max_level': int(parts[4]),
                'rank': int(parts[5]),
                'creature_type': ctype,
                'creature_type_name': type_dbc[ctype].name if (ctype and ctype in type_dbc) else '',
                'family': cfam,
                'family_name': fam_dbc[cfam].name if (cfam and cfam in fam_dbc) else '',
                'faction': int(parts[8]),
                'npcflag': int(parts[9]),
                'scale': float(parts[10]),
                'display_ids': dids,
                'displays': self._resolve_creature_displays(dids),
                'source': f'{self.world_db}.creature_template',
            })
        return rows

    def search_creatures(
        self,
        query: Optional[str] = None,
        creature_id: Optional[int] = None,
        display_id: Optional[int] = None,
        creature_type: Optional[int] = None,
        family: Optional[int] = None,
        min_level: Optional[int] = None,
        max_level: Optional[int] = None,
        rank: Optional[int] = None,
        limit: int = 20,
    ) -> Dict[str, Any]:
        limit = max(1, min(int(limit), 100))
        tokens = _tokenize_query(query)

        # 1. Try world_db creature_template if configured
        db_results = self._query_world_creatures(
            tokens=tokens,
            creature_id=creature_id,
            display_id=display_id,
            creature_type=creature_type,
            family=family,
            min_level=min_level,
            max_level=max_level,
            rank=rank,
            limit=limit,
        )
        seen_ids = {r['id'] for r in db_results}

        # 2. Search Creature.db2 joined with CreatureDisplayInfo.dbc and CreatureModelData.dbc
        creatures_db2 = self.dbc.load('Creature.db2')
        fam_dbc = self.dbc.load('CreatureFamily.dbc')
        type_dbc = self.dbc.load('CreatureType.dbc')
        scored: List[Tuple[int, int, Dict[str, Any]]] = []

        for row in creatures_db2.iter_raw_tuples():
            cid = row[0]
            if cid in seen_ids:
                continue
            if creature_id is not None and cid != int(creature_id):
                continue
            ctype = row[1]
            cfam = row[17]
            crank = row[16]
            dids = [d for d in row[5:9] if d > 0]
            if display_id is not None and int(display_id) not in dids:
                continue
            if creature_type is not None and ctype != int(creature_type):
                continue
            if family is not None and cfam != int(family):
                continue
            if rank is not None and crank != int(rank):
                continue

            name = creatures_db2.get_string(row[13])
            subname = creatures_db2.get_string(row[15])
            displays = self._resolve_creature_displays(dids)
            model_paths = ' '.join(d['model_path'] for d in displays)
            skin_names = ' '.join(' '.join(d['skins']) for d in displays)

            if tokens:
                score = _score_match(tokens, name, subname, model_paths, skin_names)
                if score == 0:
                    continue
            else:
                score = 1

            scored.append((score, -cid, {
                'id': cid,
                'name': name,
                'subname': subname,
                'rank': crank,
                'creature_type': ctype,
                'creature_type_name': type_dbc[ctype].name if (ctype and ctype in type_dbc) else '',
                'family': cfam,
                'family_name': fam_dbc[cfam].name if (cfam and cfam in fam_dbc) else '',
                'display_ids': dids,
                'displays': displays,
                'source': 'Creature.db2',
            }))

        scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
        combined = db_results + [item[2] for item in scored[:max(0, limit - len(db_results))]]
        return {'count': len(combined), 'results': combined[:limit]}

    # -----------------------------------------------------------------------
    # 5. Search Display IDs (CreatureDisplayInfo, ItemDisplayInfo, GameObjectDisplayInfo)
    # -----------------------------------------------------------------------

    def search_display_ids(
        self,
        query: Optional[str] = None,
        display_id: Optional[int] = None,
        kind: str = 'all',
        model_id: Optional[int] = None,
        has_item_visual: Optional[bool] = None,
        limit: int = 20,
    ) -> Dict[str, Any]:
        limit = max(1, min(int(limit), 100))
        kind_norm = (kind or 'all').strip().lower()
        if kind_norm not in ('all', 'creature', 'item', 'gameobject'):
            raise ValueError(f"Unsupported display ID kind {kind!r}; expected all, creature, item, or gameobject")
        tokens = _tokenize_query(query)
        scored: List[Tuple[int, int, Dict[str, Any]]] = []

        # A) Creature Display IDs (CreatureDisplayInfo.dbc + CreatureModelData.dbc)
        if kind_norm in ('all', 'creature') and has_item_visual is None:
            cdi = self.dbc.load('CreatureDisplayInfo.dbc')
            cmd = self.dbc.load('CreatureModelData.dbc')
            creatures_by_disp = self._get_creatures_by_display_map()

            if display_id is not None:
                rec = cdi.get(int(display_id))
                c_rows = [rec._table._row_struct.unpack_from(rec._table._raw, rec._row_offset)] if rec is not None else []
            else:
                c_rows = cdi.iter_raw_tuples()

            for row in c_rows:
                did = row[0]
                mid = row[1]
                if model_id is not None and mid != int(model_id):
                    continue
                mrec = cmd.get(mid) if mid else None
                mpath = mrec.get_string(2) if mrec is not None else ''
                s1 = cdi.get_string(row[6])
                s2 = cdi.get_string(row[7])
                s3 = cdi.get_string(row[8])
                portrait = cdi.get_string(row[9])
                linked = creatures_by_disp.get(did, ())
                linked_names = ' '.join(cname for _, cname in linked)

                score = _score_match(tokens, mpath, s1, s2, s3, portrait, linked_names, str(did))
                if score > 0:
                    drec = cdi[did]
                    scored.append((score, -did, {
                        'kind': 'creature',
                        'display_id': did,
                        'model_id': mid,
                        'model_path': mpath,
                        'display_scale': round(drec.model_scale, 3),
                        'model_scale': round(mrec.get_float(4), 3) if mrec is not None else 1.0,
                        'collision_height': round(mrec.get_float(15), 3) if mrec is not None else 0.0,
                        'mount_height': round(mrec.get_float(16), 3) if mrec is not None else 0.0,
                        'skins': [s for s in (s1, s2, s3) if s],
                        'portrait_texture': portrait,
                        'extended_display_info_id': row[3],
                        'state_spell_visual_kit_id': row[19],
                        'sample_creatures': [{'id': cid, 'name': cname} for cid, cname in linked],
                    }))
                    if not tokens and len(scored) >= limit:
                        break

        # B) Item Display IDs (ItemDisplayInfo.dbc)
        if kind_norm in ('all', 'item') and model_id is None:
            idi = self.dbc.load('ItemDisplayInfo.dbc')
            iv_models = self._get_item_visual_models_map()
            items_by_disp = self._get_items_by_display_map()

            if display_id is not None:
                rec = idi.get(int(display_id))
                i_rows = [rec._table._row_struct.unpack_from(rec._table._raw, rec._row_offset)] if rec is not None else []
            else:
                i_rows = idi.iter_raw_tuples()

            for row in i_rows:
                did = row[0]
                item_vis_id = row[24]
                if has_item_visual is True and not item_vis_id:
                    continue
                if has_item_visual is False and item_vis_id:
                    continue
                icon = idi.get_string(row[5])
                lmodel = idi.get_string(row[1])
                rmodel = idi.get_string(row[2])
                ltex = idi.get_string(row[3])
                rtex = idi.get_string(row[4])
                linked = items_by_disp.get(did, ())
                linked_names = ' '.join(iname for _, iname in linked)

                score = _score_match(tokens, icon, lmodel, rmodel, ltex, rtex, linked_names, str(did))
                if score > 0:
                    textures = [idi.get_string(row[col]) for col in range(15, 24) if idi.get_string(row[col])]
                    scored.append((score, -did, {
                        'kind': 'item',
                        'display_id': did,
                        'inventory_icon': icon,
                        'models': [m for m in (lmodel, rmodel) if m],
                        'model_textures': [t for t in (ltex, rtex) if t],
                        'textures': textures,
                        'spell_visual_id': row[11],
                        'item_visual_id': item_vis_id,
                        'item_visual_models': iv_models.get(item_vis_id, []) if item_vis_id else [],
                        'particle_color_id': row[25],
                        'sample_items': [{'id': iid, 'name': iname} for iid, iname in linked],
                    }))
                    if not tokens and len(scored) >= limit * 2:
                        break

        # C) GameObject Display IDs (GameObjectDisplayInfo.dbc)
        if kind_norm in ('all', 'gameobject') and model_id is None and has_item_visual is None:
            godi = self.dbc.load('GameObjectDisplayInfo.dbc')
            go_by_disp = self._get_go_by_display_map()

            if display_id is not None:
                rec = godi.get(int(display_id))
                g_rows = [rec._table._row_struct.unpack_from(rec._table._raw, rec._row_offset)] if rec is not None else []
            else:
                g_rows = godi.iter_raw_tuples()

            for row in g_rows:
                did = row[0]
                mpath = godi.get_string(row[1])
                linked = go_by_disp.get(did, ())
                linked_names = ' '.join(gname for _, gname in linked)
                score = _score_match(tokens, mpath, linked_names, str(did))
                if score > 0:
                    grec = godi[did]
                    scored.append((score, -did, {
                        'kind': 'gameobject',
                        'display_id': did,
                        'model_path': mpath,
                        'bounding_box': {
                            'min_x': round(grec.min_x, 3),
                            'min_y': round(grec.min_y, 3),
                            'min_z': round(grec.min_z, 3),
                            'max_x': round(grec.max_x, 3),
                            'max_y': round(grec.max_y, 3),
                            'max_z': round(grec.max_z, 3),
                        },
                        'object_effect_package_id': row[18],
                        'sample_gameobjects': [{'id': gid, 'name': gname} for gid, gname in linked],
                    }))
                    if not tokens and len(scored) >= limit * 3:
                        break

        scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
        results = [item[2] for item in scored[:limit]]
        return {'count': len(results), 'results': results}

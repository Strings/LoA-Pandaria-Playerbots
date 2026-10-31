"""Named schemas and TrinityCore format strings for MoP 5.4.8 (build 18414) DBC and DB2 files."""
from dataclasses import dataclass
from typing import Dict, Optional, Sequence, Tuple


@dataclass(frozen=True)
class FieldSpec:
    name: str
    kind: str  # 'uint32', 'int32', 'float', 'string', 'uint8'


@dataclass(frozen=True)
class TableSchema:
    name: str
    filename: str
    fields: Tuple[FieldSpec, ...]
    key_field: str = 'id'
    trinity_fmt: str = ''

    @property
    def field_names(self) -> Tuple[str, ...]:
        return tuple(f.name for f in self.fields)

    @property
    def key_index(self) -> int:
        for idx, f in enumerate(self.fields):
            if f.name == self.key_field:
                return idx
        return 0


def _fields(*specs: Tuple[str, str]) -> Tuple[FieldSpec, ...]:
    return tuple(FieldSpec(name=n, kind=k) for n, k in specs)


# ---------------------------------------------------------------------------
# Spell & Visual Schemas
# ---------------------------------------------------------------------------

SPELL_SCHEMA = TableSchema(
    name='Spell',
    filename='Spell.dbc',
    trinity_fmt='nssxxixxiiiiiiiiiiiiiiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('name', 'string'),
        ('rank', 'string'),
        ('description', 'string'),
        ('tooltip', 'string'),
        ('rune_cost_id', 'uint32'),
        ('spell_missile_id', 'uint32'),
        ('description_variable_id', 'uint32'),
        ('spell_difficulty_id', 'uint32'),
        ('spell_scaling_id', 'uint32'),
        ('spell_aura_options_id', 'uint32'),
        ('spell_aura_restrictions_id', 'uint32'),
        ('spell_casting_requirements_id', 'uint32'),
        ('spell_categories_id', 'uint32'),
        ('spell_class_options_id', 'uint32'),
        ('spell_cooldowns_id', 'uint32'),
        ('spell_equipped_items_id', 'uint32'),
        ('spell_interrupts_id', 'uint32'),
        ('spell_levels_id', 'uint32'),
        ('spell_reagents_id', 'uint32'),
        ('spell_shapeshift_id', 'uint32'),
        ('spell_target_restrictions_id', 'uint32'),
        ('spell_totems_id', 'uint32'),
        ('research_project_id', 'uint32'),
        ('spell_misc_id', 'uint32'),
    ),
)

SPELL_MISC_SCHEMA = TableSchema(
    name='SpellMisc',
    filename='SpellMisc.dbc',
    trinity_fmt='nxxiiiiiiiiiiiiiiiiifiiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('spell_id', 'uint32'),
        ('difficulty', 'uint32'),
        ('attributes', 'uint32'),
        ('attributes_ex', 'uint32'),
        ('attributes_ex2', 'uint32'),
        ('attributes_ex3', 'uint32'),
        ('attributes_ex4', 'uint32'),
        ('attributes_ex5', 'uint32'),
        ('attributes_ex6', 'uint32'),
        ('attributes_ex7', 'uint32'),
        ('attributes_ex8', 'uint32'),
        ('attributes_ex9', 'uint32'),
        ('attributes_ex10', 'uint32'),
        ('attributes_ex11', 'uint32'),
        ('attributes_ex12', 'uint32'),
        ('attributes_ex13', 'uint32'),
        ('casting_time_index', 'uint32'),
        ('duration_index', 'uint32'),
        ('range_index', 'uint32'),
        ('speed', 'float'),
        ('spell_visual_1', 'uint32'),
        ('spell_visual_2', 'uint32'),
        ('spell_icon_id', 'uint32'),
        ('active_icon_id', 'uint32'),
        ('school_mask', 'uint32'),
    ),
)

SPELL_EFFECT_SCHEMA = TableSchema(
    name='SpellEffect',
    filename='SpellEffect.dbc',
    trinity_fmt='niifiiiffiiiiiifiifiiiiixiiiix',
    fields=_fields(
        ('id', 'uint32'),
        ('difficulty', 'uint32'),
        ('effect', 'uint32'),
        ('value_multiplier', 'float'),
        ('apply_aura_name', 'uint32'),
        ('aura_tick_count', 'uint32'),
        ('base_points', 'int32'),
        ('bonus_multiplier', 'float'),
        ('damage_multiplier', 'float'),
        ('chain_target', 'uint32'),
        ('die_sides', 'int32'),
        ('item_type', 'uint32'),
        ('mechanic', 'uint32'),
        ('misc_value', 'int32'),
        ('misc_value_b', 'int32'),
        ('points_per_combo_point', 'float'),
        ('radius_index', 'uint32'),
        ('radius_max_index', 'uint32'),
        ('real_points_per_level', 'float'),
        ('spell_class_mask_1', 'uint32'),
        ('spell_class_mask_2', 'uint32'),
        ('spell_class_mask_3', 'uint32'),
        ('spell_class_mask_4', 'uint32'),
        ('trigger_spell', 'uint32'),
        ('pos_facing', 'float'),
        ('implicit_target_a', 'uint32'),
        ('implicit_target_b', 'uint32'),
        ('spell_id', 'uint32'),
        ('effect_index', 'uint32'),
        ('unk0', 'uint32'),
    ),
)

SPELL_ICON_SCHEMA = TableSchema(
    name='SpellIcon',
    filename='SpellIcon.dbc',
    trinity_fmt='ns',
    fields=_fields(
        ('id', 'uint32'),
        ('icon_path', 'string'),
    ),
)

SPELL_COOLDOWNS_SCHEMA = TableSchema(
    name='SpellCooldowns',
    filename='SpellCooldowns.dbc',
    trinity_fmt='dxxiii',
    fields=_fields(
        ('id', 'uint32'),
        ('spell_id', 'uint32'),
        ('difficulty', 'uint32'),
        ('category_recovery_time', 'uint32'),
        ('recovery_time', 'uint32'),
        ('start_recovery_time', 'uint32'),
    ),
)

SPELL_LEVELS_SCHEMA = TableSchema(
    name='SpellLevels',
    filename='SpellLevels.dbc',
    trinity_fmt='dxxiii',
    fields=_fields(
        ('id', 'uint32'),
        ('spell_id', 'uint32'),
        ('difficulty', 'uint32'),
        ('base_level', 'uint32'),
        ('max_level', 'uint32'),
        ('spell_level', 'uint32'),
    ),
)

SPELL_DURATION_SCHEMA = TableSchema(
    name='SpellDuration',
    filename='SpellDuration.dbc',
    trinity_fmt='niii',
    fields=_fields(
        ('id', 'uint32'),
        ('duration', 'int32'),
        ('duration_per_level', 'int32'),
        ('max_duration', 'int32'),
    ),
)

SPELL_RANGE_SCHEMA = TableSchema(
    name='SpellRange',
    filename='SpellRange.dbc',
    trinity_fmt='nffffiss',
    fields=_fields(
        ('id', 'uint32'),
        ('min_range_hostile', 'float'),
        ('min_range_friend', 'float'),
        ('max_range_hostile', 'float'),
        ('max_range_friend', 'float'),
        ('flags', 'uint32'),
        ('name', 'string'),
        ('short_name', 'string'),
    ),
)

SPELL_CAST_TIMES_SCHEMA = TableSchema(
    name='SpellCastTimes',
    filename='SpellCastTimes.dbc',
    trinity_fmt='nixx',
    fields=_fields(
        ('id', 'uint32'),
        ('cast_time', 'int32'),
        ('cast_time_per_level', 'float'),
        ('min_cast_time', 'int32'),
    ),
)

SPELL_CATEGORIES_SCHEMA = TableSchema(
    name='SpellCategories',
    filename='SpellCategories.dbc',
    trinity_fmt='niiiiiiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('spell_id', 'uint32'),
        ('difficulty', 'uint32'),
        ('category', 'uint32'),
        ('dmg_class', 'uint32'),
        ('dispel', 'uint32'),
        ('mechanic', 'uint32'),
        ('prevention_type', 'uint32'),
        ('start_recovery_category', 'uint32'),
        ('charges_category', 'uint32'),
    ),
)

SPELL_ITEM_ENCHANTMENT_SCHEMA = TableSchema(
    name='SpellItemEnchantment',
    filename='SpellItemEnchantment.dbc',
    trinity_fmt='niiiiiiiiiisiiiiiiiixixfff',
    fields=_fields(
        ('id', 'uint32'),
        ('charges', 'uint32'),
        ('type_1', 'uint32'),
        ('type_2', 'uint32'),
        ('type_3', 'uint32'),
        ('amount_1', 'int32'),
        ('amount_2', 'int32'),
        ('amount_3', 'int32'),
        ('spell_id_1', 'uint32'),
        ('spell_id_2', 'uint32'),
        ('spell_id_3', 'uint32'),
        ('description', 'string'),
        ('aura_id', 'uint32'),
        ('flags', 'uint32'),
        ('gem_id', 'uint32'),
        ('enchantment_condition', 'uint32'),
        ('required_skill', 'uint32'),
        ('required_skill_value', 'uint32'),
        ('required_level', 'uint32'),
        ('max_level', 'uint32'),
        ('unk20', 'int32'),
        ('scaling_level', 'int32'),
        ('unk22', 'int32'),
        ('scaling_points_1', 'float'),
        ('scaling_points_2', 'float'),
        ('scaling_points_3', 'float'),
    ),
)

ITEM_VISUALS_SCHEMA = TableSchema(
    name='ItemVisuals',
    filename='ItemVisuals.dbc',
    trinity_fmt='niiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('slot_1', 'uint32'),
        ('slot_2', 'uint32'),
        ('slot_3', 'uint32'),
        ('slot_4', 'uint32'),
        ('slot_5', 'uint32'),
    ),
)

ITEM_VISUAL_EFFECTS_SCHEMA = TableSchema(
    name='ItemVisualEffects',
    filename='ItemVisualEffects.dbc',
    trinity_fmt='ns',
    fields=_fields(
        ('id', 'uint32'),
        ('model', 'string'),
    ),
)

SPELL_VISUAL_SCHEMA = TableSchema(
    name='SpellVisual',
    filename='SpellVisual.db2',
    trinity_fmt='n' + 'i' * 29,
    fields=_fields(
        ('id', 'uint32'),
        ('precast_kit', 'uint32'),
        ('cast_kit', 'uint32'),
        ('impact_kit', 'uint32'),
        ('state_kit', 'uint32'),
        ('state_done_kit', 'uint32'),
        ('channel_kit', 'uint32'),
        ('has_missile', 'uint32'),
        ('missile_model', 'uint32'),
        ('missile_path_type', 'uint32'),
        ('missile_destination_attachment', 'uint32'),
        ('missile_sound', 'uint32'),
        ('anim_event_sound_id', 'uint32'),
        ('flags', 'uint32'),
        ('caster_impact_kit', 'uint32'),
        ('target_impact_kit', 'uint32'),
        ('missile_attachment', 'uint32'),
        ('missile_follow_ground_height', 'uint32'),
        ('missile_follow_ground_drop_speed', 'uint32'),
        ('missile_follow_ground_approach', 'uint32'),
        ('missile_follow_ground_flags', 'uint32'),
        ('missile_motion_id', 'uint32'),
        ('missile_targeting_kit', 'uint32'),
        ('instant_area_kit', 'uint32'),
        ('impact_area_kit', 'uint32'),
        ('persistent_area_kit', 'uint32'),
        ('missile_cast_offset_x', 'float'),
        ('missile_cast_offset_y', 'float'),
        ('missile_cast_offset_z', 'float'),
        ('alternative_visual_kit', 'uint32'),
    ),
)

SPELL_VISUAL_KIT_SCHEMA = TableSchema(
    name='SpellVisualKit',
    filename='SpellVisualKit.db2',
    trinity_fmt='n' + 'i' * 21 + 'f' + 'i' * 16,
    fields=_fields(
        ('id', 'uint32'),
        ('start_anim_id', 'int32'),
        ('anim_id', 'int32'),
        ('anim_kit_id', 'uint32'),
        ('head_effect', 'uint32'),
        ('chest_effect', 'uint32'),
        ('base_effect', 'uint32'),
        ('left_hand_effect', 'uint32'),
        ('right_hand_effect', 'uint32'),
        ('breath_effect', 'uint32'),
        ('left_weapon_effect', 'uint32'),
        ('right_weapon_effect', 'uint32'),
        ('special_effect_1', 'uint32'),
        ('special_effect_2', 'uint32'),
        ('special_effect_3', 'uint32'),
        ('world_effect', 'uint32'),
        ('sound_id', 'uint32'),
        ('shake_id', 'uint32'),
        ('char_proc_1', 'int32'),
        ('char_proc_2', 'int32'),
        ('char_proc_3', 'int32'),
        ('char_proc_4', 'int32'),
        ('char_param_zero_1', 'float'),
        ('char_param_zero_2', 'float'),
        ('char_param_zero_3', 'float'),
        ('char_param_zero_4', 'float'),
        ('char_param_one_1', 'float'),
        ('char_param_one_2', 'float'),
        ('char_param_one_3', 'float'),
        ('char_param_one_4', 'float'),
        ('char_param_two_1', 'float'),
        ('char_param_two_2', 'float'),
        ('char_param_two_3', 'float'),
        ('char_param_two_4', 'float'),
        ('char_param_three_1', 'float'),
        ('char_param_three_2', 'float'),
        ('char_param_three_3', 'float'),
        ('char_param_three_4', 'float'),
        ('flags', 'uint32'),
    ),
)

SPELL_VISUAL_EFFECT_NAME_SCHEMA = TableSchema(
    name='SpellVisualEffectName',
    filename='SpellVisualEffectName.db2',
    trinity_fmt='nsffffifi',
    fields=_fields(
        ('id', 'uint32'),
        ('file_path', 'string'),
        ('area_effect_size', 'float'),
        ('scale', 'float'),
        ('min_allowed_scale', 'float'),
        ('max_allowed_scale', 'float'),
        ('unk1', 'uint32'),
        ('unk2', 'float'),
        ('unk3', 'uint32'),
    ),
)

SPELL_VISUAL_KIT_MODEL_ATTACH_SCHEMA = TableSchema(
    name='SpellVisualKitModelAttach',
    filename='SpellVisualKitModelAttach.db2',
    trinity_fmt='n' + 'i' * 14,
    fields=_fields(
        ('id', 'uint32'),
        ('parent_spell_visual_kit_id', 'uint32'),
        ('spell_visual_effect_name_id', 'uint32'),
        ('attachment_id', 'int32'),
        ('offset_x', 'float'),
        ('offset_y', 'float'),
        ('offset_z', 'float'),
        ('yaw', 'float'),
        ('pitch', 'float'),
        ('roll', 'float'),
        ('scale', 'float'),
        ('start_anim_id', 'int32'),
        ('anim_id', 'int32'),
        ('end_anim_id', 'int32'),
        ('flags', 'uint32'),
    ),
)

SPELL_VISUAL_MISSILE_SCHEMA = TableSchema(
    name='SpellVisualMissile',
    filename='SpellVisualMissile.db2',
    trinity_fmt='n' + 'i' * 16,
    fields=_fields(
        ('id', 'uint32'),
        ('missile_set_id', 'uint32'),
        ('spell_visual_effect_name_id', 'uint32'),
        ('sound_entries_id', 'uint32'),
        ('impact_kit', 'uint32'),
        ('attachment', 'int32'),
        ('destination_attachment', 'int32'),
        ('cast_offset_x', 'uint32'),
        ('cast_offset_y', 'uint32'),
        ('cast_offset_z', 'uint32'),
        ('impact_offset_x', 'uint32'),
        ('impact_offset_y', 'uint32'),
        ('impact_offset_z', 'uint32'),
        ('follow_ground_height', 'uint32'),
        ('follow_ground_drop_speed', 'uint32'),
        ('follow_ground_approach', 'uint32'),
        ('flags', 'uint32'),
    ),
)

# ---------------------------------------------------------------------------
# Item & Display Schemas
# ---------------------------------------------------------------------------

ITEM_SCHEMA = TableSchema(
    name='Item',
    filename='Item.db2',
    trinity_fmt='niiiiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('item_class', 'uint32'),
        ('item_subclass', 'uint32'),
        ('sound_override_subclass', 'int32'),
        ('material', 'int32'),
        ('display_id', 'uint32'),
        ('inventory_type', 'uint32'),
        ('sheath', 'uint32'),
    ),
)

_ITEM_SPARSE_SPECS = [
    ('id', 'uint32'),
    ('quality', 'uint32'),
    ('flags', 'uint32'),
    ('flags2', 'uint32'),
    ('flags3', 'uint32'),
    ('unk430_1', 'float'),
    ('unk430_2', 'float'),
    ('buy_count', 'uint32'),
    ('buy_price', 'uint32'),
    ('sell_price', 'uint32'),
    ('inventory_type', 'uint32'),
    ('allowable_class', 'int32'),
    ('allowable_race', 'int32'),
    ('item_level', 'uint32'),
    ('required_level', 'int32'),
    ('required_skill', 'uint32'),
    ('required_skill_rank', 'uint32'),
    ('required_spell', 'uint32'),
    ('required_honor_rank', 'uint32'),
    ('required_city_rank', 'uint32'),
    ('required_reputation_faction', 'uint32'),
    ('required_reputation_rank', 'uint32'),
    ('max_count', 'int32'),
    ('stackable', 'int32'),
    ('container_slots', 'uint32'),
]
for _i in range(1, 11):
    _ITEM_SPARSE_SPECS.append((f'stat_type_{_i}', 'int32'))
for _i in range(1, 11):
    _ITEM_SPARSE_SPECS.append((f'stat_value_{_i}', 'int32'))
for _i in range(1, 11):
    _ITEM_SPARSE_SPECS.append((f'stat_allocation_{_i}', 'int32'))
for _i in range(1, 11):
    _ITEM_SPARSE_SPECS.append((f'stat_socket_cost_multiplier_{_i}', 'float'))
_ITEM_SPARSE_SPECS.extend([
    ('scaling_stat_distribution', 'uint32'),
    ('damage_type', 'uint32'),
    ('delay', 'uint32'),
    ('ranged_mod_range', 'float'),
])
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_id_{_i}', 'int32'))
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_trigger_{_i}', 'int32'))
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_charges_{_i}', 'int32'))
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_cooldown_{_i}', 'int32'))
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_category_{_i}', 'int32'))
for _i in range(1, 6):
    _ITEM_SPARSE_SPECS.append((f'spell_category_cooldown_{_i}', 'int32'))
_ITEM_SPARSE_SPECS.extend([
    ('bonding', 'uint32'),
    ('name', 'string'),
    ('name2', 'string'),
    ('name3', 'string'),
    ('name4', 'string'),
    ('description', 'string'),
    ('page_text', 'uint32'),
    ('language_id', 'uint32'),
    ('page_material', 'uint32'),
    ('start_quest', 'uint32'),
    ('lock_id', 'uint32'),
    ('material', 'int32'),
    ('sheath', 'uint32'),
    ('random_property', 'uint32'),
    ('random_suffix', 'uint32'),
    ('item_set', 'uint32'),
    ('area', 'uint32'),
    ('map', 'uint32'),
    ('bag_family', 'uint32'),
    ('totem_category', 'uint32'),
    ('socket_color_1', 'uint32'),
    ('socket_color_2', 'uint32'),
    ('socket_color_3', 'uint32'),
    ('socket_content_1', 'uint32'),
    ('socket_content_2', 'uint32'),
    ('socket_content_3', 'uint32'),
    ('socket_bonus', 'int32'),
    ('gem_properties', 'uint32'),
    ('armor_damage_modifier', 'float'),
    ('duration', 'uint32'),
    ('item_limit_category', 'uint32'),
    ('holiday_id', 'uint32'),
    ('stat_scaling_factor', 'float'),
    ('currency_substitution_id', 'int32'),
    ('currency_substitution_count', 'int32'),
])

ITEM_SPARSE_SCHEMA = TableSchema(
    name='ItemSparse',
    filename='Item-sparse.db2',
    trinity_fmt='niiiiffiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiisssssiiiiiiiiiiiiiiiiiiiiiifiiifii',
    fields=_fields(*_ITEM_SPARSE_SPECS),
)

ITEM_DISPLAY_INFO_SCHEMA = TableSchema(
    name='ItemDisplayInfo',
    filename='ItemDisplayInfo.dbc',
    trinity_fmt='nssssssiiiiiiiiisssssssssii',
    fields=_fields(
        ('id', 'uint32'),
        ('left_model', 'string'),
        ('right_model', 'string'),
        ('left_model_texture', 'string'),
        ('right_model_texture', 'string'),
        ('inventory_icon', 'string'),
        ('inventory_icon_2', 'string'),
        ('geoset_group_1', 'uint32'),
        ('geoset_group_2', 'uint32'),
        ('geoset_group_3', 'uint32'),
        ('flags', 'uint32'),
        ('spell_visual_id', 'uint32'),
        ('group_sound_index', 'uint32'),
        ('helmet_geoset_vis_1', 'uint32'),
        ('helmet_geoset_vis_2', 'uint32'),
        ('texture_1', 'string'),
        ('texture_2', 'string'),
        ('texture_3', 'string'),
        ('texture_4', 'string'),
        ('texture_5', 'string'),
        ('texture_6', 'string'),
        ('texture_7', 'string'),
        ('texture_8', 'string'),
        ('texture_9', 'string'),
        ('item_visual_id', 'uint32'),
        ('particle_color_id', 'uint32'),
    ),
)

# ---------------------------------------------------------------------------
# Creature & GameObject Display Schemas
# ---------------------------------------------------------------------------

CREATURE_DB2_SCHEMA = TableSchema(
    name='Creature',
    filename='Creature.db2',
    trinity_fmt='niiiiiiiiiffffsisii',
    fields=_fields(
        ('id', 'uint32'),
        ('creature_type', 'uint32'),
        ('item_entry_1', 'uint32'),
        ('item_entry_2', 'uint32'),
        ('item_entry_3', 'uint32'),
        ('display_id_1', 'uint32'),
        ('display_id_2', 'uint32'),
        ('display_id_3', 'uint32'),
        ('display_id_4', 'uint32'),
        ('display_scale_1', 'float'),
        ('display_scale_2', 'float'),
        ('display_scale_3', 'float'),
        ('display_scale_4', 'float'),
        ('name', 'string'),
        ('unk_14', 'uint32'),
        ('subname', 'string'),
        ('rank', 'uint32'),
        ('creature_family', 'uint32'),
    ),
)

CREATURE_DISPLAY_INFO_SCHEMA = TableSchema(
    name='CreatureDisplayInfo',
    filename='CreatureDisplayInfo.dbc',
    trinity_fmt='niiifissssiiiiiiiiii',
    fields=_fields(
        ('id', 'uint32'),
        ('model_id', 'uint32'),
        ('sound_id', 'uint32'),
        ('extended_display_info_id', 'uint32'),
        ('model_scale', 'float'),
        ('model_alpha', 'uint32'),
        ('texture_variation_1', 'string'),
        ('texture_variation_2', 'string'),
        ('texture_variation_3', 'string'),
        ('portrait_texture_name', 'string'),
        ('size_class', 'int32'),
        ('blood_id', 'uint32'),
        ('npc_sound_id', 'uint32'),
        ('particle_color_id', 'uint32'),
        ('creature_geoset_data', 'uint32'),
        ('object_effect_package_id', 'uint32'),
        ('anim_replacement_set_id', 'uint32'),
        ('flags', 'uint32'),
        ('gender', 'int32'),
        ('state_spell_visual_kit_id', 'uint32'),
    ),
)

CREATURE_MODEL_DATA_SCHEMA = TableSchema(
    name='CreatureModelData',
    filename='CreatureModelData.dbc',
    trinity_fmt='nisxfxxxxxxxxxxffxxxxxxxxxxxxxxxxx',
    fields=_fields(
        ('id', 'uint32'),
        ('flags', 'uint32'),
        ('model_path', 'string'),
        ('size_class', 'uint32'),
        ('model_scale', 'float'),
        ('blood_id', 'uint32'),
        ('footprint_texture_id', 'uint32'),
        ('footprint_texture_length', 'float'),
        ('footprint_texture_width', 'float'),
        ('footprint_particle_scale', 'float'),
        ('foley_material_id', 'uint32'),
        ('footstep_shake_size', 'uint32'),
        ('death_thud_shake_size', 'uint32'),
        ('sound_id', 'uint32'),
        ('collision_width', 'float'),
        ('collision_height', 'float'),
        ('mount_height', 'float'),
        ('geo_box_min_x', 'float'),
        ('geo_box_min_y', 'float'),
        ('geo_box_min_z', 'float'),
        ('geo_box_max_x', 'float'),
        ('geo_box_max_y', 'float'),
        ('geo_box_max_z', 'float'),
        ('world_effect_scale', 'float'),
        ('attached_effect_scale', 'float'),
        ('missile_collision_radius', 'float'),
        ('missile_collision_push', 'float'),
        ('missile_collision_raise', 'float'),
        ('override_loot_effect_scale', 'float'),
        ('override_name_scale', 'float'),
        ('override_selection_radius', 'float'),
        ('tamed_pet_base_scale', 'float'),
        ('creature_geoset_data_id', 'uint32'),
        ('hover_height', 'float'),
    ),
)

CREATURE_FAMILY_SCHEMA = TableSchema(
    name='CreatureFamily',
    filename='CreatureFamily.dbc',
    trinity_fmt='nfifiiiiixss',
    fields=_fields(
        ('id', 'uint32'),
        ('min_scale', 'float'),
        ('min_scale_level', 'uint32'),
        ('max_scale', 'float'),
        ('max_scale_level', 'uint32'),
        ('skill_line_1', 'uint32'),
        ('skill_line_2', 'uint32'),
        ('pet_food_mask', 'uint32'),
        ('pet_talent_type', 'int32'),
        ('category_enum_id', 'int32'),
        ('name', 'string'),
        ('icon_file', 'string'),
    ),
)

CREATURE_TYPE_SCHEMA = TableSchema(
    name='CreatureType',
    filename='CreatureType.dbc',
    trinity_fmt='nsi',
    fields=_fields(
        ('id', 'uint32'),
        ('name', 'string'),
        ('flags', 'uint32'),
    ),
)

GAMEOBJECT_DISPLAY_INFO_SCHEMA = TableSchema(
    name='GameObjectDisplayInfo',
    filename='GameObjectDisplayInfo.dbc',
    trinity_fmt='nsiiiiiiiiiffffffiii',
    fields=_fields(
        ('id', 'uint32'),
        ('model_path', 'string'),
        ('sound_1', 'uint32'),
        ('sound_2', 'uint32'),
        ('sound_3', 'uint32'),
        ('sound_4', 'uint32'),
        ('sound_5', 'uint32'),
        ('sound_6', 'uint32'),
        ('sound_7', 'uint32'),
        ('sound_8', 'uint32'),
        ('sound_9', 'uint32'),
        ('sound_10', 'uint32'),
        ('min_x', 'float'),
        ('min_y', 'float'),
        ('min_z', 'float'),
        ('max_x', 'float'),
        ('max_y', 'float'),
        ('max_z', 'float'),
        ('object_effect_package_id', 'uint32'),
        ('override_loot_effect_scale', 'float'),
        ('override_name_scale', 'float'),
    ),
)

GAMEOBJECTS_DB2_SCHEMA = TableSchema(
    name='GameObjects',
    filename='GameObjects.db2',
    trinity_fmt='niiffffffffiiiiis',
    fields=_fields(
        ('id', 'uint32'),
        ('map_id', 'uint32'),
        ('display_id', 'uint32'),
        ('pos_x', 'float'),
        ('pos_y', 'float'),
        ('pos_z', 'float'),
        ('rot_x', 'float'),
        ('rot_y', 'float'),
        ('rot_z', 'float'),
        ('rot_w', 'float'),
        ('scale', 'float'),
        ('go_type', 'uint32'),
        ('data_1', 'uint32'),
        ('data_2', 'uint32'),
        ('data_3', 'uint32'),
        ('data_4', 'uint32'),
        ('name', 'string'),
    ),
)


SCHEMAS_BY_NAME: Dict[str, TableSchema] = {}
SCHEMAS_BY_FILENAME: Dict[str, TableSchema] = {}

for _schema in (
    SPELL_SCHEMA,
    SPELL_MISC_SCHEMA,
    SPELL_EFFECT_SCHEMA,
    SPELL_ICON_SCHEMA,
    SPELL_COOLDOWNS_SCHEMA,
    SPELL_LEVELS_SCHEMA,
    SPELL_DURATION_SCHEMA,
    SPELL_RANGE_SCHEMA,
    SPELL_CAST_TIMES_SCHEMA,
    SPELL_CATEGORIES_SCHEMA,
    SPELL_ITEM_ENCHANTMENT_SCHEMA,
    ITEM_VISUALS_SCHEMA,
    ITEM_VISUAL_EFFECTS_SCHEMA,
    SPELL_VISUAL_SCHEMA,
    SPELL_VISUAL_KIT_SCHEMA,
    SPELL_VISUAL_EFFECT_NAME_SCHEMA,
    SPELL_VISUAL_KIT_MODEL_ATTACH_SCHEMA,
    SPELL_VISUAL_MISSILE_SCHEMA,
    ITEM_SCHEMA,
    ITEM_SPARSE_SCHEMA,
    ITEM_DISPLAY_INFO_SCHEMA,
    CREATURE_DB2_SCHEMA,
    CREATURE_DISPLAY_INFO_SCHEMA,
    CREATURE_MODEL_DATA_SCHEMA,
    CREATURE_FAMILY_SCHEMA,
    CREATURE_TYPE_SCHEMA,
    GAMEOBJECT_DISPLAY_INFO_SCHEMA,
    GAMEOBJECTS_DB2_SCHEMA,
):
    SCHEMAS_BY_NAME[_schema.name.lower()] = _schema
    SCHEMAS_BY_FILENAME[_schema.filename.lower()] = _schema
    SCHEMAS_BY_NAME[_schema.filename.rsplit('.', 1)[0].lower()] = _schema


def get_schema(name_or_filename: str) -> Optional[TableSchema]:
    key = name_or_filename.strip().lower()
    return SCHEMAS_BY_FILENAME.get(key) or SCHEMAS_BY_NAME.get(key)

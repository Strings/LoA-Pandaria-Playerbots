-- T-083 part B: Growth XP and stat points (DESIGN.md Progression, "Three tracks, no rank cap"). Each Growth level
-- is one point the player spends on a stat below; a point is a percentage of that stat. The account rank and its
-- cap are gone, so the twink checkpoints' rank steps go too. Numbers are placeholders until playtests;
-- .reload heartseed picks up the stat table. Idempotent: the stat table holds only these rows, the rest are scoped.

CREATE TABLE IF NOT EXISTS `heartseed_growth_stat` (
  `id` TINYINT UNSIGNED NOT NULL COMMENT 'Saved in character_heartseed_growth_point.stat_id: append only',
  `name` VARCHAR(50) NOT NULL DEFAULT '',
  `main_stat` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = strength, agility or intellect by the character''s specialization (spells 310011-310013)',
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Passive aura whose effect 0 is points * pct_per_point; unused for main_stat',
  `pct_per_point` INT NOT NULL DEFAULT 0 COMMENT 'Percent of the stat per point',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Growth stat points (T-083)';

DELETE FROM `heartseed_growth_stat`;
INSERT INTO `heartseed_growth_stat` (`id`, `name`, `main_stat`, `spell_id`, `pct_per_point`, `comment`) VALUES
(1, 'Stamina',         0, 310010, 1, '+1% stamina per point'),
(2, 'Main stat',       1,      0, 1, '+1% strength, agility or intellect per point, following the specialization'),
(3, 'Critical strike', 0, 310014, 1, '+1% critical strike chance per point'),
(4, 'Haste',           0, 310015, 1, '+1% attack and casting speed per point');

-- Passive auras like the trait auras (Attributes 0x40 passive, infinite duration). Effect 6 = apply aura:
-- 137 MOD_TOTAL_STAT_PERCENTAGE with the stat in MiscValue (0 strength, 1 agility, 2 stamina, 3 intellect),
-- 290 MOD_CRIT_PCT, 193 MOD_HASTE (melee, ranged and casting speed).
DELETE FROM `spell_dbc` WHERE `Id` BETWEEN 310010 AND 310015;
INSERT INTO `spell_dbc` (`Id`, `SpellAuraOptionsId`, `SpellCastingRequirementsId`, `SpellCategoriesId`, `SpellClassOptionsId`, `SpellEquippedItemsId`, `SpellLevelsId`, `SpellTargetRestrictionsId`, `SpellInterruptsId`, `SpellMiscId`, `Comment`) VALUES
(310010, 0, 0, 0, 0, 0, 0, 0, 0, 310010, 'Heartseed: Growth stamina'),
(310011, 0, 0, 0, 0, 0, 0, 0, 0, 310011, 'Heartseed: Growth strength'),
(310012, 0, 0, 0, 0, 0, 0, 0, 0, 310012, 'Heartseed: Growth agility'),
(310013, 0, 0, 0, 0, 0, 0, 0, 0, 310013, 'Heartseed: Growth intellect'),
(310014, 0, 0, 0, 0, 0, 0, 0, 0, 310014, 'Heartseed: Growth critical strike'),
(310015, 0, 0, 0, 0, 0, 0, 0, 0, 310015, 'Heartseed: Growth haste');
DELETE FROM `spellmisc_dbc` WHERE `Id` BETWEEN 310010 AND 310015;
INSERT INTO `spellmisc_dbc` (`Id`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `AttributesEx8`, `AttributesEx9`, `AttributesEx10`, `AttributesEx11`, `AttributesEx12`, `AttributesEx13`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `Comment`) VALUES
(310010, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth stamina'),
(310011, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth strength'),
(310012, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth agility'),
(310013, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth intellect'),
(310014, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth critical strike'),
(310015, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Growth haste');
DELETE FROM `spelleffect_dbc` WHERE `Id` BETWEEN 310010 AND 310015;
INSERT INTO `spelleffect_dbc` (`Id`, `Effect`, `EffectValueMultiplier`, `EffectApplyAuraName`, `EffectAplitude`, `EffectBasePoints`, `EffectBonusMultiplier`, `EffectDamageMultiplier`, `EffectChainTarget`, `EffectDieSides`, `EffectMechanic`, `EffectMiscValue`, `EffectMiscValueB`, `EffectRadiusIndex`, `EffectRadiusIndexMax`, `EffectRealPointsPerLevel`, `EffectSpellClassMaskA`, `EffectSpellClassMaskB`, `EffectSpellClassMaskC`, `EffectSpellClassMaskD`, `EffectTriggerSpell`, `EffectImplicitTargetA`, `EffectImplicitTargetB`, `EffectSpellId`, `EffectIndex`, `EffectAttributes`) VALUES
(310010, 6, 0, 137, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310010, 0, 0),
(310011, 6, 0, 137, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310011, 0, 0),
(310012, 6, 0, 137, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310012, 0, 0),
(310013, 6, 0, 137, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310013, 0, 0),
(310014, 6, 0, 290, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310014, 0, 0),
(310015, 6, 0, 193, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310015, 0, 0);

-- No account rank to raise any more; heartseed_growth steps now set the character's Growth level.
DELETE FROM `gmtools_checkpoint_step` WHERE `type` = 'heartseed_rank';

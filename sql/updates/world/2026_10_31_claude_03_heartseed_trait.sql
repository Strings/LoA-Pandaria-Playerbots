-- T-021: Heartseed traits as data rows, plus the server-side passive auras they apply.
-- Idempotent: the trait table is created if missing; spell and trait rows are replaced by id.
--
-- Spells 310000-310999 are the Heartseed spell block (docs/ID_RANGES.md). They live only in the
-- spell_dbc tables, so they never reach the client: ids from 203751 up are flagged server-side only and
-- passive auras are not sent. The server reads these tables at startup only.
--
-- The traits below are placeholders with simple effects, to be replaced by the real trait design.

CREATE TABLE IF NOT EXISTS `heartseed_trait` (
  `id` INT UNSIGNED NOT NULL,
  `path` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 = every Heartseed, 1 = Xuen, 2 = Chi-Ji, 3 = Niuzao, 4 = Yu''lon',
  `kind` TINYINT UNSIGNED NOT NULL COMMENT '1 = minor (fills in by rank on a path with a major), 2 = major (chosen)',
  `tier` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Majors: tier 0-5, opening at levels 10, 25, 40, 55, 70, 85',
  `min_rank` TINYINT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Minors: Heartseed rank that fills it in',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'Passive aura applied while active; effect 0 gets the amount',
  `base_amount` INT NOT NULL DEFAULT '0' COMMENT 'Effect 0 amount at item level 0',
  `amount_per_ilvl` FLOAT NOT NULL DEFAULT '0' COMMENT 'Added per item level of the main-hand weapon',
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `description` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed minor and major traits';

-- Passive auras: Effect 6 = apply aura, target 1 = caster, Attributes 64 = passive, DurationIndex 21 = infinite.
DELETE FROM `spell_dbc` WHERE `Id` BETWEEN 310000 AND 310008;
INSERT INTO `spell_dbc` (`Id`, `SpellAuraOptionsId`, `SpellCastingRequirementsId`, `SpellCategoriesId`, `SpellClassOptionsId`, `SpellEquippedItemsId`, `SpellLevelsId`, `SpellTargetRestrictionsId`, `SpellInterruptsId`, `SpellMiscId`, `Comment`) VALUES
(310000, 0, 0, 0, 0, 0, 0, 0, 0, 310000, 'Heartseed: Xuen minor (placeholder)'),
(310001, 0, 0, 0, 0, 0, 0, 0, 0, 310001, 'Heartseed: Xuen major (placeholder)'),
(310002, 0, 0, 0, 0, 0, 0, 0, 0, 310002, 'Heartseed: Chi-Ji minor (placeholder)'),
(310003, 0, 0, 0, 0, 0, 0, 0, 0, 310003, 'Heartseed: Chi-Ji major (placeholder)'),
(310004, 0, 0, 0, 0, 0, 0, 0, 0, 310004, 'Heartseed: Niuzao minor (placeholder)'),
(310005, 0, 0, 0, 0, 0, 0, 0, 0, 310005, 'Heartseed: Niuzao major (placeholder)'),
(310006, 0, 0, 0, 0, 0, 0, 0, 0, 310006, 'Heartseed: Yu''lon minor (placeholder)'),
(310007, 0, 0, 0, 0, 0, 0, 0, 0, 310007, 'Heartseed: Yu''lon major (placeholder)'),
(310008, 0, 0, 0, 0, 0, 0, 0, 0, 310008, 'Heartseed: core passive, bonus weapon damage');

DELETE FROM `spellmisc_dbc` WHERE `Id` BETWEEN 310000 AND 310008;
INSERT INTO `spellmisc_dbc` (`Id`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `AttributesEx8`, `AttributesEx9`, `AttributesEx10`, `AttributesEx11`, `AttributesEx12`, `AttributesEx13`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `Comment`) VALUES
(310000, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Xuen minor'),
(310001, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Xuen major'),
(310002, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Chi-Ji minor'),
(310003, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Chi-Ji major'),
(310004, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Niuzao minor'),
(310005, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Niuzao major'),
(310006, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Yu''lon minor'),
(310007, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: Yu''lon major'),
(310008, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: core passive');

-- Effect row ids reuse the spell ids (the effect id space is separate and free there).
-- Auras: 31 run speed %, 290 crit %, 118 healing received %, 136 healing done %, 101 armor % (misc 1 = physical),
-- 87 damage taken % (misc 127 = all schools), 110 mana regen % (misc 0 = mana), 317 spell power %,
-- 13 flat damage done (misc 1 = physical; adds to weapon damage).
DELETE FROM `spelleffect_dbc` WHERE `Id` BETWEEN 310000 AND 310008;
INSERT INTO `spelleffect_dbc` (`Id`, `Effect`, `EffectValueMultiplier`, `EffectApplyAuraName`, `EffectAplitude`, `EffectBasePoints`, `EffectBonusMultiplier`, `EffectDamageMultiplier`, `EffectChainTarget`, `EffectDieSides`, `EffectMechanic`, `EffectMiscValue`, `EffectMiscValueB`, `EffectRadiusIndex`, `EffectRadiusIndexMax`, `EffectRealPointsPerLevel`, `EffectSpellClassMaskA`, `EffectSpellClassMaskB`, `EffectSpellClassMaskC`, `EffectSpellClassMaskD`, `EffectTriggerSpell`, `EffectImplicitTargetA`, `EffectImplicitTargetB`, `EffectSpellId`, `EffectIndex`, `EffectAttributes`) VALUES
(310000, 6, 0,  31, 0,  3, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310000, 0, 0),
(310001, 6, 0, 290, 0,  1, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310001, 0, 0),
(310002, 6, 0, 118, 0,  3, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310002, 0, 0),
(310003, 6, 0, 136, 0,  2, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310003, 0, 0),
(310004, 6, 0, 101, 0,  5, 0, 0, 0, 0, 0,   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310004, 0, 0),
(310005, 6, 0,  87, 0, -2, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310005, 0, 0),
(310006, 6, 0, 110, 0,  5, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310006, 0, 0),
(310007, 6, 0, 317, 0,  2, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310007, 0, 0),
(310008, 6, 0,  13, 0,  0, 0, 0, 0, 0, 0,   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310008, 0, 0);

-- Placeholder traits: one minor and one level 10 major per Celestial path. Amounts grow with the main-hand
-- weapon's item level, so a level 90 weapon (item level ~450-500) adds about 4-5 points to each major.
DELETE FROM `heartseed_trait` WHERE `id` BETWEEN 1 AND 8;
INSERT INTO `heartseed_trait` (`id`, `path`, `kind`, `tier`, `min_rank`, `spell_id`, `base_amount`, `amount_per_ilvl`, `name`, `description`) VALUES
(1, 1, 1, 0, 1, 310000,  3,  0,     'Tiger''s Stride',   'Placeholder. 3% run speed.'),
(2, 1, 2, 0, 1, 310001,  1,  0.01,  'Claws of Xuen',     'Placeholder. Critical strike chance, growing with weapon item level.'),
(3, 2, 1, 0, 1, 310002,  3,  0,     'Crane''s Grace',    'Placeholder. 3% more healing received.'),
(4, 2, 2, 0, 1, 310003,  2,  0.01,  'Wings of Hope',     'Placeholder. Healing done, growing with weapon item level.'),
(5, 3, 1, 0, 1, 310004,  5,  0,     'Ox Hide',           'Placeholder. 5% more armor.'),
(6, 3, 2, 0, 1, 310005, -2, -0.01,  'Bulwark of Niuzao', 'Placeholder. Less damage taken, growing with weapon item level.'),
(7, 4, 1, 0, 1, 310006,  5,  0,     'Serpent''s Patience', 'Placeholder. 5% mana regeneration.'),
(8, 4, 2, 0, 1, 310007,  2,  0.01,  'Wisdom of Yu''lon', 'Placeholder. Spell power, growing with weapon item level.');

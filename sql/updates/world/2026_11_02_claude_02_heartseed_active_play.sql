-- T-083: Heartseed active play (DESIGN.md "Active play, not big cooldowns"). Each Celestial path gets a meter that
-- normal play builds and the Gift spends; Gift cooldowns drop to 15-30 s. Chi-Ji's and Niuzao's Gifts stop reusing
-- monk abilities (Chi Torpedo, Leg Sweep): Crane's Plume is Spirit of Chi-Ji (148009, legendary cloak proc) and
-- Niuzao's Retort is Ox Charge (116098, creature spell). Numbers are placeholders until playtests;
-- .reload heartseed picks up changes. Idempotent: the meter table holds only these rows, the rest are scoped.

CREATE TABLE IF NOT EXISTS `heartseed_meter` (
  `path` TINYINT UNSIGNED NOT NULL COMMENT 'Heartseed::Path: 1 Xuen, 2 Chi-Ji, 3 Niuzao, 4 Yu''lon',
  `source` TINYINT UNSIGNED NOT NULL COMMENT 'Heartseed::MeterSource: 1 crit, 2 healing, 3 big hit taken, 4 spell cast',
  `max_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = no stacks, the Gift is lit instead',
  `threshold_pct` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'healing: % of max health per stack; big hit: % of max health',
  `lit_duration` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms the Gift stays castable after a big hit; 0 = never gated',
  `decay` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms after the last build before stacks drop; 0 = never',
  `build_cooldown` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms between two builds (and two crit cuts)',
  `spend_bonus_pct` INT NOT NULL DEFAULT 0 COMMENT 'Gift power per stack spent, %',
  `crit_cooldown_cut` INT NOT NULL DEFAULT 0 COMMENT 'ms each crit cuts from the Gift cooldown',
  `window` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms of damage taken the Gift remembers (Retort)',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`path`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed path meters (T-083)';

DELETE FROM `heartseed_meter`;
INSERT INTO `heartseed_meter` (`path`, `source`, `max_stacks`, `threshold_pct`, `lit_duration`, `decay`, `build_cooldown`, `spend_bonus_pct`, `crit_cooldown_cut`, `window`, `comment`) VALUES
(1, 1, 5, 0,    0, 15000, 500, 10,  250,    0, 'Xuen: crits build Thunder; each Thunder keeps the tiger 10% longer; crits cut the cooldown'),
(2, 2, 5, 5,    0, 20000,   0, 20,    0,    0, 'Chi-Ji: every 5% of max health healed (done or received) is one Hope; Plume heals 20% more per Hope'),
(3, 3, 0, 8, 6000,     0,   0,  0,    0, 5000, 'Niuzao: a hit of 8% of max health lights Retort for 6 s; Retort answers the last 5 s of damage'),
(4, 4, 5, 0,    0, 15000,   0, 20, 1000,    0, 'Yu''lon: spell casts build Coil; Breath deals 20% more per Coil; crits cut the cooldown 1 s');

-- Meter passive: a dummy aura (Effect 6, aura 4) on the caster, passive and infinite like the trait auras.
DELETE FROM `spell_dbc` WHERE `Id` = 310009;
INSERT INTO `spell_dbc` (`Id`, `SpellAuraOptionsId`, `SpellCastingRequirementsId`, `SpellCategoriesId`, `SpellClassOptionsId`, `SpellEquippedItemsId`, `SpellLevelsId`, `SpellTargetRestrictionsId`, `SpellInterruptsId`, `SpellMiscId`, `Comment`) VALUES
(310009, 0, 0, 0, 0, 0, 0, 0, 0, 310009, 'Heartseed: meter passive, crit procs');
DELETE FROM `spellmisc_dbc` WHERE `Id` = 310009;
INSERT INTO `spellmisc_dbc` (`Id`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `AttributesEx8`, `AttributesEx9`, `AttributesEx10`, `AttributesEx11`, `AttributesEx12`, `AttributesEx13`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `Comment`) VALUES
(310009, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 21, 1, 1, 'Heartseed: meter passive');
DELETE FROM `spelleffect_dbc` WHERE `Id` = 310009;
INSERT INTO `spelleffect_dbc` (`Id`, `Effect`, `EffectValueMultiplier`, `EffectApplyAuraName`, `EffectAplitude`, `EffectBasePoints`, `EffectBonusMultiplier`, `EffectDamageMultiplier`, `EffectChainTarget`, `EffectDieSides`, `EffectMechanic`, `EffectMiscValue`, `EffectMiscValueB`, `EffectRadiusIndex`, `EffectRadiusIndexMax`, `EffectRealPointsPerLevel`, `EffectSpellClassMaskA`, `EffectSpellClassMaskB`, `EffectSpellClassMaskC`, `EffectSpellClassMaskD`, `EffectTriggerSpell`, `EffectImplicitTargetA`, `EffectImplicitTargetB`, `EffectSpellId`, `EffectIndex`, `EffectAttributes`) VALUES
(310009, 6, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 310009, 0, 0);

-- Procs on every critical damage or healing hit the character deals: melee and ranged auto attacks and spells,
-- harmful and helpful spells, periodic ticks (procFlags 0x55554), critical hits only (procEx 2).
DELETE FROM `spell_proc_event` WHERE `entry` = 310009;
INSERT INTO `spell_proc_event` (`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `SpellFamilyMask3`, `procFlags`, `procEx`, `phase`, `ppmRate`, `CustomChance`, `Cooldown`) VALUES
(310009, 0, 0, 0, 0, 0, 0, 349524, 2, 'PROC_PHASE_HIT', 0, 100, 0);

-- Gifts: base_amount is now a percentage (heartseed_gift_rider and HeartseedMgr.h TraitInfo say of what).
UPDATE `heartseed_gift` SET `spell_id` = 148009, `cooldown` = 15000, `duration` = 0, `base_amount` = 8, `amount_per_ilvl` = 0,
    `comment` = 'Crane''s Plume: Spirit of Chi-Ji (legendary cloak proc); heal % of the caster''s max health' WHERE `trait_id` = 4;
UPDATE `heartseed_gift` SET `spell_id` = 116098, `cooldown` = 20000, `duration` = 0, `base_amount` = 50, `amount_per_ilvl` = 0,
    `comment` = 'Niuzao''s Retort: Ox Charge (creature spell); % of the damage taken in the window, per enemy' WHERE `trait_id` = 6;
UPDATE `heartseed_gift` SET `cooldown` = 30000, `base_amount` = 30, `amount_per_ilvl` = 0,
    `comment` = 'Serpent''s Breath: Celestial Breath (creature spell); tick = % of attack or spell power' WHERE `trait_id` = 8;

UPDATE `heartseed_trait` SET `description` = 'Xuen''s Gift: a spectral white tiger fights at your side. Your critical hits build Thunder, and each Thunder keeps the tiger longer.' WHERE `id` = 2;
UPDATE `heartseed_trait` SET `name` = 'Crane''s Plume', `description` = 'Chi-Ji''s Gift: a burst of the crane''s fire heals you and the most wounded allies near you. Healing you do or receive builds Hope; each Hope makes the Plume stronger.' WHERE `id` = 4;
UPDATE `heartseed_trait` SET `name` = 'Niuzao''s Retort', `description` = 'Niuzao''s Gift: after a heavy blow, strike back. A shockwave returns the damage you just took to every enemy around you and shields you.' WHERE `id` = 6;
UPDATE `heartseed_trait` SET `description` = 'Yu''lon''s Gift: breathe jade fire in a cone. Each spell you cast coils the serpent; the Breath burns hotter for each coil.' WHERE `id` = 8;

-- Riders for the new Gifts (kind: Heartseed::GiftRiderKind). Stage 1 is Sprout, so Retort's shield comes with it.
DELETE FROM `heartseed_gift_rider` WHERE `trait_id` IN (4, 6);
INSERT INTO `heartseed_gift_rider` (`trait_id`, `stage`, `kind`, `amount`, `duration`, `spell_id`, `comment`) VALUES
(4, 2, 1,   30, 4000,  2983, 'Sapling: a burst of speed (Sprint as helper)'),
(4, 3, 2,    2,    0,     0, 'Bloom: heals 2 more allies'),
(4, 4, 3,    0,    0,     0, 'Full Bloom: a killing blow resets it'),
(6, 1, 11,  30, 8000, 11426, 'Sprout: shield for 30% of the damage answered (Ice Barrier as helper)'),
(6, 2, 5,  -20, 8000, 51340, 'Sapling: enemies hit deal 20% less damage (Curse of Impotence as helper)'),
(6, 3, 7,   50,    0,     0, 'Bloom: a 50% wider shockwave'),
(6, 4, 10, 2000,   0,     0, 'Full Bloom: each enemy hit cuts the cooldown 2 s');

DELETE FROM `spell_script_names` WHERE `spell_id` IN (115008, 121828) AND `ScriptName` = 'spell_heartseed_gift';
DELETE FROM `spell_script_names` WHERE `spell_id` = 119381 AND `ScriptName` IN ('spell_heartseed_gift', 'spell_heartseed_ox_stomp');
DELETE FROM `spell_script_names` WHERE `spell_id` = 148009 AND `ScriptName` IN ('spell_heartseed_gift', 'spell_heartseed_crane_plume');
DELETE FROM `spell_script_names` WHERE `spell_id` = 116098 AND `ScriptName` IN ('spell_heartseed_gift', 'spell_heartseed_niuzao_retort');
DELETE FROM `spell_script_names` WHERE `spell_id` = 310009 AND `ScriptName` = 'spell_heartseed_meter';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(148009, 'spell_heartseed_gift'),
(148009, 'spell_heartseed_crane_plume'),
(116098, 'spell_heartseed_gift'),
(116098, 'spell_heartseed_niuzao_retort'),
(310009, 'spell_heartseed_meter');

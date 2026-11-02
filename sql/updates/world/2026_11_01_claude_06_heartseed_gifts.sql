-- T-080: Celestial Gifts. The level 10 major is an active ability, one per Celestial (DESIGN.md, What it grants).
-- Idempotent: the gift table is created if missing; trait, gift and script rows are replaced by id.
--
-- The four tier-0 majors keep their ids (2, 4, 6, 8), so characters that already chose one now hold that path's
-- Gift with no character data change. Their old stat auras move to new minors (9-12), which fill in on the path
-- once its major is chosen.
--
-- Gifts reuse client spells, because the 5.4.8 client only casts spells it knows; the scripts below adjust them
-- server-side. Three are monk talents: a monk who took the talent keeps it when the Gift changes.

CREATE TABLE IF NOT EXISTS `heartseed_gift` (
  `trait_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_trait major that teaches the Gift',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'Client spell taught while the major is active',
  `cooldown` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Gift cooldown in ms; applied when longer than the spell''s own',
  `base_amount` INT NOT NULL DEFAULT '0' COMMENT 'Gift power at item level 0 (damage per hit for scaled Gifts)',
  `amount_per_ilvl` FLOAT NOT NULL DEFAULT '0' COMMENT 'Added per item level of the main-hand weapon',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`trait_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Celestial Gifts: active spells taught by majors';

UPDATE `heartseed_trait` SET `spell_id` = 0, `base_amount` = 0, `amount_per_ilvl` = 0, `name` = 'Call of Xuen',
    `description` = 'Xuen''s Gift: a spectral white tiger fights at your side, crackling with lightning.' WHERE `id` = 2;
UPDATE `heartseed_trait` SET `spell_id` = 0, `base_amount` = 0, `amount_per_ilvl` = 0, `name` = 'Crane Rush',
    `description` = 'Chi-Ji''s Gift: rush forward on the crane''s wings, healing allies and striking enemies in your path.' WHERE `id` = 4;
UPDATE `heartseed_trait` SET `spell_id` = 0, `base_amount` = 0, `amount_per_ilvl` = 0, `name` = 'Ox Stomp',
    `description` = 'Niuzao''s Gift: stomp the ground and knock down every enemy around you.' WHERE `id` = 6;
UPDATE `heartseed_trait` SET `spell_id` = 0, `base_amount` = 0, `amount_per_ilvl` = 0, `name` = 'Serpent''s Breath',
    `description` = 'Yu''lon''s Gift: breathe jade fire in a cone, burning every enemy before you.' WHERE `id` = 8;

DELETE FROM `heartseed_trait` WHERE `id` BETWEEN 9 AND 12;
INSERT INTO `heartseed_trait` (`id`, `path`, `kind`, `tier`, `min_rank`, `spell_id`, `base_amount`, `amount_per_ilvl`, `name`, `description`) VALUES
(9,  1, 1, 0, 1, 310001,  1,  0.01, 'Claws of Xuen',     'Placeholder. Critical strike chance, growing with weapon item level.'),
(10, 2, 1, 0, 1, 310003,  2,  0.01, 'Wings of Hope',     'Placeholder. Healing done, growing with weapon item level.'),
(11, 3, 1, 0, 1, 310005, -2, -0.01, 'Bulwark of Niuzao', 'Placeholder. Less damage taken, growing with weapon item level.'),
(12, 4, 1, 0, 1, 310007,  2,  0.01, 'Wisdom of Yu''lon',  'Placeholder. Spell power, growing with weapon item level.');

-- Cooldowns: Invoke Xuen keeps its own 3 minutes; the others get 60 to 90 seconds. Serpent's Breath ticks three
-- times over 1.5 s; its tick damage replaces the creature spell's (spell_heartseed_celestial_breath).
DELETE FROM `heartseed_gift` WHERE `trait_id` IN (2, 4, 6, 8);
INSERT INTO `heartseed_gift` (`trait_id`, `spell_id`, `cooldown`, `base_amount`, `amount_per_ilvl`, `comment`) VALUES
(2, 123904,      0,  0, 0, 'Call of Xuen: Invoke Xuen, the White Tiger (monk talent)'),
(4, 115008,  60000,  0, 0, 'Crane Rush: Chi Torpedo (monk talent)'),
(6, 119381,  90000,  0, 0, 'Ox Stomp: Leg Sweep (monk talent)'),
(8, 117960,  60000, 30, 4, 'Serpent''s Breath: Celestial Breath (creature spell), damage per tick');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_heartseed_gift', 'spell_heartseed_celestial_breath');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(123904, 'spell_heartseed_gift'),
(115008, 'spell_heartseed_gift'),
(119381, 'spell_heartseed_gift'),
(117960, 'spell_heartseed_gift'),
(124947, 'spell_heartseed_celestial_breath');

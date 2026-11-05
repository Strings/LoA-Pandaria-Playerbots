-- T-089: the shared affix module (rares now; the corruption dial and Brawler mutators later) and its eight affixes.
CREATE TABLE IF NOT EXISTS `affix_template` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(32) NOT NULL,
  `kind` TINYINT UNSIGNED NOT NULL COMMENT '1 Molten, 2 Frozen, 3 Vampiric, 4 Shielded, 5 Teleporting, 6 Juggernaut, 7 Summoner, 8 Sha-touched',
  `aura_spell` INT UNSIGNED NOT NULL COMMENT 'visible marker aura, 318001+',
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'client spell the mechanic casts',
  `period_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount_pct` FLOAT NOT NULL DEFAULT 0 COMMENT 'share of health: the victim''s for damage, its own for shields, heals, Juggernaut health and adds',
  `summon_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  `summon_count` INT UNSIGNED NOT NULL DEFAULT 0,
  `xp_bonus_pct` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'added to the kill''s share of a level',
  `loot_rolls` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'extra rolls of the creature''s loot table',
  `weight` INT UNSIGNED NOT NULL DEFAULT 0,
  `hosts` INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1 rare, 2 corruption, 4 Brawler',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `affix_template` WHERE `id` BETWEEN 1 AND 8;
INSERT INTO `affix_template` (`id`, `name`, `kind`, `aura_spell`, `spell_id`, `period_ms`, `amount_pct`, `summon_entry`, `summon_count`, `xp_bonus_pct`, `loot_rolls`, `weight`, `hosts`) VALUES
(1, 'Molten',      1, 318001, 11969,  6000,   6, 0,     0, 20, 1, 100, 1),
(2, 'Frozen',      2, 318002, 12748, 12000,   4, 0,     0, 20, 1, 100, 1),
(3, 'Vampiric',    3, 318003,     0,     0,  40, 0,     0, 15, 1, 100, 1),
(4, 'Shielded',    4, 318004,    17, 15000,  12, 0,     0, 15, 1, 100, 1),
(5, 'Teleporting', 5, 318005,     0, 10000,   0, 0,     0, 15, 1,  80, 1),
(6, 'Juggernaut',  6, 318006,     0,     0, 150, 0,     0, 30, 2,  60, 1),
(7, 'Summoner',    7, 318007,     0, 20000,  15, 64247, 2, 25, 1,  70, 1),
(8, 'Sha-touched', 8, 318008, 13704, 18000,   0, 0,     0, 25, 1,  70, 1);

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_affix_marker';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(318001, 'spell_affix_marker'), (318002, 'spell_affix_marker'), (318003, 'spell_affix_marker'), (318004, 'spell_affix_marker'),
(318005, 'spell_affix_marker'), (318006, 'spell_affix_marker'), (318007, 'spell_affix_marker'), (318008, 'spell_affix_marker');

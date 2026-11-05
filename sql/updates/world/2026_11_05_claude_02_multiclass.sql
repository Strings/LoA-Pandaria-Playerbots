-- T-094: multi-classing. Second classes, their abilities (spell clones 320000+, tools/clientpatch/edits.json) and
-- the interaction rules between a second class and its base class's specs. Idempotent. See
-- src/server/scripts/Custom/Multiclass/README.md for the columns.

CREATE TABLE IF NOT EXISTS `multiclass_class` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(40) NOT NULL,
  `base_class_mask` INT UNSIGNED NOT NULL COMMENT '1 << (class id - 1) for each base class that can take it',
  `max_level` TINYINT UNSIGNED NOT NULL DEFAULT 30,
  `description` VARCHAR(200) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `multiclass_ability` (
  `class_id` INT UNSIGNED NOT NULL,
  `level` TINYINT UNSIGNED NOT NULL COMMENT 'second-class level that teaches it',
  `spell_id` INT UNSIGNED NOT NULL,
  `kind` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 active (learned), 1 passive (rules; listed only)',
  `power_pct` INT NOT NULL DEFAULT 0 COMMENT 'damage per hit, % of the Gift power; 0 keeps the spell''s own',
  `cooldown` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms',
  `power_type` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Powers: 0 mana, 1 rage, 3 energy',
  `cost_pct` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '% of the character''s maximum of that power',
  `name` VARCHAR(40) NOT NULL,
  `description` VARCHAR(160) NOT NULL DEFAULT '',
  PRIMARY KEY (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `multiclass_rule` (
  `id` INT UNSIGNED NOT NULL,
  `class_id` INT UNSIGNED NOT NULL,
  `base_spec` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ChrSpecialization id, 0 any',
  `min_level` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'second-class level',
  `trigger` TINYINT UNSIGNED NOT NULL COMMENT '1 cast of trigger_spell, 2 hit of second-class ability trigger_spell, 3 killing blow on a creature (trigger_spell 0)',
  `trigger_spell` INT UNSIGNED NOT NULL DEFAULT 0,
  `condition` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 none, 1 target has condition_spell from you, 2 you have condition_spell, 3 target attacking someone else',
  `condition_spell` INT UNSIGNED NOT NULL DEFAULT 0,
  `effect` TINYINT UNSIGNED NOT NULL COMMENT '1 cast effect_spell on target, 2 on yourself, 3 energize: effect_spell is the power type, amount the power',
  `effect_spell` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount` INT NOT NULL DEFAULT 0,
  `cooldown` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'ms between firings',
  `comment` VARCHAR(160) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `multiclass_class` WHERE `id` IN (1, 2);
INSERT INTO `multiclass_class` (`id`, `name`, `base_class_mask`, `max_level`, `description`) VALUES
(1, 'Barbarian',   1,   30, 'A warrior of the old warbands: thrown axes, war cries and a frenzy that feeds on your own strikes.'),
(2, 'Necromancer', 256, 30, 'A warlock who bends death itself: raises the fallen, hurls bone and makes corpses burst.');

DELETE FROM `multiclass_ability` WHERE `class_id` IN (1, 2);
INSERT INTO `multiclass_ability` (`class_id`, `level`, `spell_id`, `kind`, `power_pct`, `cooldown`, `power_type`, `cost_pct`, `name`, `description`) VALUES
(1,  1, 320000, 0, 120,  6000, 1, 10, 'Hurl Axe',         'Hurl an axe at an enemy. Arms: roots a Hamstrung target. Fury: a second axe while Enraged. Protection: taunts.'),
(1,  5, 320001, 0,   0, 60000, 1,  0, 'War Cry',          'A war cry that raises all damage you deal for a short time.'),
(1, 10, 320002, 1,   0,     0, 1,  0, 'Frenzy',           'Passive: Mortal Strike, Bloodthirst and Shield Slam send you into a frenzy of attack speed.'),
(2,  1, 320003, 0,   0, 60000, 0, 10, 'Raise Skeleton',   'Raise skeletal warriors to fight for you. Affliction: an enemy dying with your Corruption raises them.'),
(2,  5, 320004, 0, 110,     0, 0,  3, 'Bone Spear',       'Hurl a spear of bone. Demonology: grants Demonic Fury.'),
(2, 10, 320005, 0, 200, 10000, 0,  5, 'Corpse Explosion', 'Make a nearby corpse burst, hurting enemies near it. Destruction: enemies hit catch your Immolate.');

DELETE FROM `multiclass_rule` WHERE `id` BETWEEN 1 AND 99;
INSERT INTO `multiclass_rule` (`id`, `class_id`, `base_spec`, `min_level`, `trigger`, `trigger_spell`, `condition`, `condition_spell`, `effect`, `effect_spell`, `amount`, `cooldown`, `comment`) VALUES
(1, 1,  71,  1, 2, 320000, 1,   1715, 1,   6533,  0,     0, 'Arms: Hurl Axe roots (Net) a target you have Hamstrung'),
(2, 1,  72,  1, 2, 320000, 2,  12880, 1, 320000,  0,     0, 'Fury: a second Hurl Axe while Enraged'),
(3, 1,  73,  1, 2, 320000, 3,      0, 1,    355,  0,     0, 'Protection: Hurl Axe taunts a target attacking someone else'),
(4, 1,   0, 10, 1,  12294, 0,      0, 2, 320002,  0,     0, 'Frenzy from Mortal Strike'),
(5, 1,   0, 10, 1,  23881, 0,      0, 2, 320002,  0,     0, 'Frenzy from Bloodthirst'),
(6, 1,   0, 10, 1,  23922, 0,      0, 2, 320002,  0,     0, 'Frenzy from Shield Slam'),
(7, 2, 265,  1, 3,      0, 1, 146739, 2, 320003,  0, 20000, 'Affliction: an enemy dying with your Corruption raises skeletons'),
(8, 2, 266,  5, 2, 320004, 0,      0, 3,     15, 60,     0, 'Demonology: Bone Spear grants 60 Demonic Fury'),
(9, 2, 267, 10, 2, 320005, 0,      0, 1,    348,  0,     0, 'Destruction: enemies hit by Corpse Explosion catch your Immolate');

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_multiclass_ability', 'spell_multiclass_corpse_explosion');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(320000, 'spell_multiclass_ability'),
(320001, 'spell_multiclass_ability'),
(320003, 'spell_multiclass_ability'),
(320004, 'spell_multiclass_ability'),
(320005, 'spell_multiclass_ability'),
(320005, 'spell_multiclass_corpse_explosion');

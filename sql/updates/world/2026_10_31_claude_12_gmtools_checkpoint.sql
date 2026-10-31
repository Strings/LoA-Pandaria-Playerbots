-- T-048: GM test suite. Checkpoint templates (ordered typed steps, applied with .checkpoint apply) and kits.
-- Ranks follow the cap (level + 5 per awakening) with the first dungeon at 15, Pandaria at 85 and the first
-- world boss at 90. Legendary levels are placeholders until the pacing pass. Idempotent.

CREATE TABLE IF NOT EXISTS `gmtools_checkpoint` (
    `name`        VARCHAR(32) NOT NULL,
    `category`    VARCHAR(32) NOT NULL DEFAULT '',
    `sort_order`  INT UNSIGNED NOT NULL DEFAULT 0,
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: checkpoint templates (T-048)';

CREATE TABLE IF NOT EXISTS `gmtools_checkpoint_step` (
    `checkpoint` VARCHAR(32) NOT NULL,
    `step`       TINYINT UNSIGNED NOT NULL COMMENT 'applied in this order',
    `type`       VARCHAR(32) NOT NULL COMMENT 'step type registered by a system: level, gear, kit, heartseed_rank, ...',
    `arg1`       INT NOT NULL DEFAULT 0,
    `arg2`       INT NOT NULL DEFAULT 0,
    `text`       VARCHAR(255) NOT NULL DEFAULT '',
    `comment`    VARCHAR(255) NOT NULL DEFAULT '',
    PRIMARY KEY (`checkpoint`, `step`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: checkpoint steps (T-048)';

CREATE TABLE IF NOT EXISTS `gmtools_kit` (
    `name`        VARCHAR(32) NOT NULL,
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    `min_level`   TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `max_level`   TINYINT UNSIGNED NOT NULL DEFAULT 90,
    PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: item kits (T-048)';

CREATE TABLE IF NOT EXISTS `gmtools_kit_item` (
    `kit`        VARCHAR(32) NOT NULL,
    `item`       INT UNSIGNED NOT NULL,
    `count`      INT UNSIGNED NOT NULL DEFAULT 1,
    `class_mask` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 = every class',
    `equip`      TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = equip it, else bags',
    PRIMARY KEY (`kit`, `item`, `class_mask`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: kit items (T-048)';

DELETE FROM `gmtools_checkpoint_step` WHERE `checkpoint` IN ('hs-10', 'hs-25', 'hs-40', 'hs-55', 'legendary-1', 'hs-70', 'legendary-2', 'hs-85', 'max-90');
DELETE FROM `gmtools_checkpoint` WHERE `name` IN ('hs-10', 'hs-25', 'hs-40', 'hs-55', 'legendary-1', 'hs-70', 'legendary-2', 'hs-85', 'max-90');
INSERT INTO `gmtools_checkpoint` (`name`, `category`, `sort_order`, `description`) VALUES
('hs-10', 'heartseed', 10, 'Level 10, the first major choice waiting. No awakenings.'),
('hs-25', 'heartseed', 25, 'Level 25 major tier; first dungeon done.'),
('hs-40', 'heartseed', 40, 'Level 40 major tier.'),
('hs-55', 'heartseed', 55, 'Level 55 major tier.'),
('legendary-1', 'legendary', 60, 'First legacy legendary unlock (placeholder level 60 until the pacing pass).'),
('hs-70', 'heartseed', 70, 'Level 70 major tier.'),
('legendary-2', 'legendary', 80, 'Second legacy legendary unlock (placeholder level 80 until the pacing pass).'),
('hs-85', 'heartseed', 85, 'Level 85, ready to sail for Pandaria (its awakening still ahead).'),
('max-90', 'heartseed', 90, 'Level 90 with every awakening, epic gear, at the faction shrine.');
INSERT INTO `gmtools_checkpoint_step` (`checkpoint`, `step`, `type`, `arg1`, `arg2`, `text`, `comment`) VALUES
('hs-10', 1, 'heartseed', 0, 0, '', ''),
('hs-10', 2, 'level', 10, 0, '', ''),
('hs-10', 3, 'class_spells', 0, 0, '', ''),
('hs-10', 4, 'kit', 0, 0, 'bags', ''),
('hs-10', 5, 'gear', 2, 0, '', ''),
('hs-10', 6, 'heartseed_rank', 10, 0, '', 'about the cap at this level'),
('hs-10', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-10', 8, 'heartseed_flag', 0, 0, 'first_dungeon', ''),
('hs-10', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-10', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-10', 11, 'heartseed_major', 10, 0, '', 'this tier asks again'),
('hs-10', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('hs-25', 1, 'heartseed', 0, 0, '', ''),
('hs-25', 2, 'level', 25, 0, '', ''),
('hs-25', 3, 'class_spells', 0, 0, '', ''),
('hs-25', 4, 'kit', 0, 0, 'bags', ''),
('hs-25', 5, 'gear', 2, 0, '', ''),
('hs-25', 6, 'heartseed_rank', 30, 0, '', 'about the cap at this level'),
('hs-25', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-25', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('hs-25', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-25', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-25', 11, 'heartseed_major', 25, 0, '', 'this tier asks again'),
('hs-25', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('hs-40', 1, 'heartseed', 0, 0, '', ''),
('hs-40', 2, 'level', 40, 0, '', ''),
('hs-40', 3, 'class_spells', 0, 0, '', ''),
('hs-40', 4, 'kit', 0, 0, 'bags', ''),
('hs-40', 5, 'gear', 2, 0, '', ''),
('hs-40', 6, 'heartseed_rank', 45, 0, '', 'about the cap at this level'),
('hs-40', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-40', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('hs-40', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-40', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-40', 11, 'heartseed_major', 40, 0, '', 'this tier asks again'),
('hs-40', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('hs-55', 1, 'heartseed', 0, 0, '', ''),
('hs-55', 2, 'level', 55, 0, '', ''),
('hs-55', 3, 'class_spells', 0, 0, '', ''),
('hs-55', 4, 'kit', 0, 0, 'bags', ''),
('hs-55', 5, 'gear', 2, 0, '', ''),
('hs-55', 6, 'heartseed_rank', 60, 0, '', 'about the cap at this level'),
('hs-55', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-55', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('hs-55', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-55', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-55', 11, 'heartseed_major', 55, 0, '', 'this tier asks again'),
('hs-55', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('legendary-1', 1, 'heartseed', 0, 0, '', ''),
('legendary-1', 2, 'level', 60, 0, '', ''),
('legendary-1', 3, 'class_spells', 0, 0, '', ''),
('legendary-1', 4, 'kit', 0, 0, 'bags', ''),
('legendary-1', 5, 'gear', 3, 0, '', ''),
('legendary-1', 6, 'heartseed_rank', 65, 0, '', 'about the cap at this level'),
('legendary-1', 7, 'heartseed_growth', 0, 0, '', ''),
('legendary-1', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('legendary-1', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('legendary-1', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('legendary-1', 11, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('hs-70', 1, 'heartseed', 0, 0, '', ''),
('hs-70', 2, 'level', 70, 0, '', ''),
('hs-70', 3, 'class_spells', 0, 0, '', ''),
('hs-70', 4, 'kit', 0, 0, 'bags', ''),
('hs-70', 5, 'gear', 3, 0, '', ''),
('hs-70', 6, 'heartseed_rank', 75, 0, '', 'about the cap at this level'),
('hs-70', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-70', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('hs-70', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-70', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-70', 11, 'heartseed_major', 70, 0, '', 'this tier asks again'),
('hs-70', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('legendary-2', 1, 'heartseed', 0, 0, '', ''),
('legendary-2', 2, 'level', 80, 0, '', ''),
('legendary-2', 3, 'class_spells', 0, 0, '', ''),
('legendary-2', 4, 'kit', 0, 0, 'bags', ''),
('legendary-2', 5, 'gear', 3, 0, '', ''),
('legendary-2', 6, 'heartseed_rank', 85, 0, '', 'about the cap at this level'),
('legendary-2', 7, 'heartseed_growth', 0, 0, '', ''),
('legendary-2', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('legendary-2', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('legendary-2', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('legendary-2', 11, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('hs-85', 1, 'heartseed', 0, 0, '', ''),
('hs-85', 2, 'level', 85, 0, '', ''),
('hs-85', 3, 'class_spells', 0, 0, '', ''),
('hs-85', 4, 'kit', 0, 0, 'bags', ''),
('hs-85', 5, 'gear', 3, 0, '', ''),
('hs-85', 6, 'heartseed_rank', 90, 0, '', 'about the cap at this level'),
('hs-85', 7, 'heartseed_growth', 0, 0, '', ''),
('hs-85', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('hs-85', 9, 'heartseed_flag', 0, 0, 'reached_pandaria', ''),
('hs-85', 10, 'heartseed_flag', 0, 0, 'first_world_boss', ''),
('hs-85', 11, 'heartseed_major', 85, 0, '', 'this tier asks again'),
('hs-85', 12, 'tele', 0, 0, 'Stormwind|Orgrimmar', ''),
('max-90', 1, 'heartseed', 0, 0, '', ''),
('max-90', 2, 'level', 90, 0, '', ''),
('max-90', 3, 'class_spells', 0, 0, '', ''),
('max-90', 4, 'kit', 0, 0, 'bags', ''),
('max-90', 5, 'gear', 4, 0, '', ''),
('max-90', 6, 'heartseed_rank', 105, 0, '', 'about the cap at this level'),
('max-90', 7, 'heartseed_growth', 0, 0, '', ''),
('max-90', 8, 'heartseed_flag', 0, 1, 'first_dungeon', ''),
('max-90', 9, 'heartseed_flag', 0, 1, 'reached_pandaria', ''),
('max-90', 10, 'heartseed_flag', 0, 1, 'first_world_boss', ''),
('max-90', 11, 'tele', 0, 0, 'ShrineofSevenStars|ShrineofTwoMoons', '');

-- Twink kits per bracket come with T-045's gear list.
DELETE FROM `gmtools_kit_item` WHERE `kit` = 'bags';
DELETE FROM `gmtools_kit` WHERE `name` = 'bags';
INSERT INTO `gmtools_kit` (`name`, `description`, `min_level`, `max_level`) VALUES
('bags', 'Four Netherweave Bags in empty bag slots.', 1, 90);
INSERT INTO `gmtools_kit_item` (`kit`, `item`, `count`, `class_mask`, `equip`) VALUES
('bags', 21841, 4, 0, 1);

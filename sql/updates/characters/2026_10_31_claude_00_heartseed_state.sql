-- T-020: Heartseed state. A character has a Heartseed when it has a heartseed_state row; the granting
-- quest (T-025) creates it. Idempotent: creates missing tables only, never drops player data.

CREATE TABLE IF NOT EXISTS `heartseed_state` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `rank` TINYINT UNSIGNED NOT NULL DEFAULT '1',
  `growth` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Growth toward the next rank',
  `granted_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the Heartseed was granted',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed rank and Growth per character';

-- One saved path loadout per specialization: the major trait chosen at each major tier.
CREATE TABLE IF NOT EXISTS `heartseed_loadout` (
  `guid` INT UNSIGNED NOT NULL,
  `spec_id` SMALLINT UNSIGNED NOT NULL COMMENT 'ChrSpecialization.dbc ID',
  `tier` TINYINT UNSIGNED NOT NULL COMMENT 'Major tier index: 0 = level 10, 1 = 25, 2 = 40, 3 = 55, 4 = 70, 5 = 85',
  `trait_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_trait.id (world, T-021)',
  PRIMARY KEY (`guid`, `spec_id`, `tier`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed major choices per specialization';

CREATE TABLE IF NOT EXISTS `heartseed_essence` (
  `guid` INT UNSIGNED NOT NULL,
  `essence_id` INT UNSIGNED NOT NULL COMMENT 'Essence type (T-022)',
  `count` INT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Times collected',
  `active_slot` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 = collected but inactive, 1-3 = essence slot',
  PRIMARY KEY (`guid`, `essence_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed essences collected and slotted';

-- Story beats (first dungeon, reached Pandaria, first world boss, ...). Rows rather than a bitmask so beats
-- can be added as data and keep the time they happened for the chronicle.
CREATE TABLE IF NOT EXISTS `heartseed_story_flag` (
  `guid` INT UNSIGNED NOT NULL,
  `flag_id` SMALLINT UNSIGNED NOT NULL COMMENT 'HeartseedStoryFlag in Custom/Heartseed/HeartseedDefines.h',
  `set_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the beat happened',
  PRIMARY KEY (`guid`, `flag_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed story beats reached';

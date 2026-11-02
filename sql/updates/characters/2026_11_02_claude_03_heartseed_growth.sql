-- T-083 part B: Growth XP and stat points belong to each character (DESIGN.md, Balance stays per character). The
-- account_heartseed rank and growth columns are no longer read or written; they keep their defaults.

CREATE TABLE IF NOT EXISTS `character_heartseed_growth` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `level` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Growth level: one stat point each',
  `xp` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Growth XP toward the next level',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Growth XP per character (T-083)';

CREATE TABLE IF NOT EXISTS `character_heartseed_growth_point` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `stat_id` TINYINT UNSIGNED NOT NULL COMMENT 'world heartseed_growth_stat.id',
  `points` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `stat_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed stat points spent per character (T-083)';

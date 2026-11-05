-- T-094: a character's second classes (multi-classing). Idempotent.
CREATE TABLE IF NOT EXISTS `character_multiclass` (
  `guid` INT UNSIGNED NOT NULL,
  `class_id` INT UNSIGNED NOT NULL,
  `level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `xp` INT UNSIGNED NOT NULL DEFAULT 0,
  `active` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `class_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

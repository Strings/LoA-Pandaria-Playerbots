-- T-046: per-character return anchor; native character/bind tables are not altered.
CREATE TABLE IF NOT EXISTS `character_legacy_trip` (
  `guid` INT UNSIGNED NOT NULL,
  `dungeon` INT UNSIGNED NOT NULL,
  `destination` INT UNSIGNED NOT NULL,
  `return_map` INT UNSIGNED NOT NULL,
  `return_x` FLOAT NOT NULL,
  `return_y` FLOAT NOT NULL,
  `return_z` FLOAT NOT NULL,
  `return_o` FLOAT NOT NULL,
  `difficulty` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

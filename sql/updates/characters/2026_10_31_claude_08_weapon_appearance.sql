-- T-022: weapon appearances a character has unlocked by feeding a weapon to the Heartseed or deleting it. The
-- appearance collection (transmog from consumed and deleted weapons, regardless of proficiency) reads this
-- later; T-022 only records. Idempotent: creates the table if missing, never drops player data.

CREATE TABLE IF NOT EXISTS `character_weapon_appearance` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `item_entry` INT UNSIGNED NOT NULL COMMENT 'item_template.entry the appearance came from',
  `display_id` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'ItemDisplayInfo id at unlock time',
  `source` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '1 = fed to the Heartseed, 2 = deleted',
  `unlock_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time of the first unlock',
  PRIMARY KEY (`guid`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Weapon appearances unlocked by consuming or deleting weapons';

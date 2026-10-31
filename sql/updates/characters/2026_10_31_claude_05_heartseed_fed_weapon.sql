-- T-022: every weapon a Heartseed has fed on, by item entry. Consumed weapons become appearances later
-- (DESIGN.md, "Appearance collection"), so the record starts with the first weapon fed.
-- Idempotent: creates the table if missing, never drops player data.

CREATE TABLE IF NOT EXISTS `heartseed_fed_weapon` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `item_entry` INT UNSIGNED NOT NULL COMMENT 'item_template.entry of the weapon fed',
  `count` INT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Times a weapon of this entry was fed',
  `first_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the first one was fed',
  PRIMARY KEY (`guid`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Weapons each Heartseed has consumed';

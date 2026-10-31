-- T-049: one Heartseed per account (DESIGN.md, One seed per account). Rank, Growth, the essence collection and
-- the weapons fed move to account tables; loadouts, essence slots and story flags stay per character in the
-- heartseed_* tables. Idempotent: creates missing tables and copies pre-T-049 per-character rows into accounts
-- that have none yet, so running it again never overwrites newer account state. heartseed_state and
-- heartseed_fed_weapon are no longer written and are kept, not dropped.

CREATE TABLE IF NOT EXISTS `account_heartseed` (
  `account` INT UNSIGNED NOT NULL COMMENT 'auth account.id',
  `rank` TINYINT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Shared rank; each character uses it up to its own cap',
  `growth` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Growth toward the next rank',
  `granted_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the Heartseed was granted',
  PRIMARY KEY (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed rank and Growth per account';

CREATE TABLE IF NOT EXISTS `account_heartseed_essence` (
  `account` INT UNSIGNED NOT NULL,
  `essence_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_essence_template.id (world)',
  `count` INT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Times collected',
  PRIMARY KEY (`account`, `essence_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed essences the account has collected';

CREATE TABLE IF NOT EXISTS `account_heartseed_fed_weapon` (
  `account` INT UNSIGNED NOT NULL,
  `item_entry` INT UNSIGNED NOT NULL COMMENT 'item_template.entry of the weapon fed',
  `count` INT UNSIGNED NOT NULL DEFAULT '1' COMMENT 'Times a weapon of this entry was fed',
  `first_time` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Unix time the first one was fed',
  PRIMARY KEY (`account`, `item_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Weapons the account''s Heartseed has consumed';

-- The account keeps its furthest character's seed: highest rank, then most Growth, then the oldest grant.
INSERT IGNORE INTO `account_heartseed` (`account`, `rank`, `growth`, `granted_time`)
SELECT c.`account`, s.`rank`, s.`growth`, s.`granted_time`
FROM `heartseed_state` s JOIN `characters` c ON c.`guid` = s.`guid`
ORDER BY s.`rank` DESC, s.`growth` DESC, s.`granted_time` ASC;

-- Collections add up across the account's characters.
INSERT IGNORE INTO `account_heartseed_essence` (`account`, `essence_id`, `count`)
SELECT c.`account`, e.`essence_id`, LEAST(SUM(e.`count`), 4294967295)
FROM `heartseed_essence` e JOIN `characters` c ON c.`guid` = e.`guid`
GROUP BY c.`account`, e.`essence_id`;

INSERT IGNORE INTO `account_heartseed_fed_weapon` (`account`, `item_entry`, `count`, `first_time`)
SELECT c.`account`, f.`item_entry`, LEAST(SUM(f.`count`), 4294967295), MIN(f.`first_time`)
FROM `heartseed_fed_weapon` f JOIN `characters` c ON c.`guid` = f.`guid`
GROUP BY c.`account`, f.`item_entry`;

-- heartseed_essence now holds only each character's slotted essences.
DELETE FROM `heartseed_essence` WHERE `active_slot` = 0;

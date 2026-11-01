-- T-018: detached item storage (gear pages now; reagent bank T-043 and twink templates later). A stored item keeps
-- its item_instance row and has a row here instead of in character_inventory. Idempotent: creates the table only.

CREATE TABLE IF NOT EXISTS `detached_item` (
  `item_guid` INT UNSIGNED NOT NULL COMMENT 'item_instance.guid',
  `scope` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 = character, 1 = account',
  `owner` INT UNSIGNED NOT NULL COMMENT 'characters.guid or account.id, by scope',
  `kind` TINYINT UNSIGNED NOT NULL COMMENT '1 = gear page, 2 = reagent bank, 3 = twink template',
  `container` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Gear page: character_loadout.id',
  `slot` INT UNSIGNED NOT NULL COMMENT 'Gear page: equipment slot 0-18, 100+ for spares',
  PRIMARY KEY (`item_guid`),
  UNIQUE KEY `idx_place` (`scope`, `owner`, `kind`, `container`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Items held outside the inventory (detached storage)';

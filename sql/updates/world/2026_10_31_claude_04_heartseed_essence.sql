-- T-022: Heartseed essence types and where they come from when a weapon is fed.
-- Idempotent: tables are created if missing; the placeholder rows are replaced by id.
--
-- An essence is a minor trait by the fed weapon's origin (DESIGN.md, "Leveling"). While it sits in an open
-- essence slot (1 at level 10, 2 at 40, 3 at 70) it rolls a proc on damage the character deals, dealt like
-- the core proc: proc_spell_id only names the damage and sets its school in the combat log.

CREATE TABLE IF NOT EXISTS `heartseed_essence_template` (
  `id` INT UNSIGNED NOT NULL COMMENT 'Saved in characters heartseed_essence.essence_id; 0 is reserved',
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `proc_spell_id` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Spell shown for the proc damage; 0 = no proc',
  `proc_chance` FLOAT NOT NULL DEFAULT '0' COMMENT 'Percent per damaging hit',
  `weapon_damage_pct` FLOAT NOT NULL DEFAULT '0' COMMENT 'Proc damage as a share of the average weapon damage per swing',
  `description` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed essence types';

-- A row matches a fed weapon by item entry, or, with item_entry 0, by a text found anywhere in its name
-- (case-insensitive). Several matching rows for one essence give one roll at the best chance.
CREATE TABLE IF NOT EXISTS `heartseed_essence_source` (
  `essence_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_essence_template.id',
  `item_entry` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'item_template.entry, or 0 to match by name_pattern',
  `name_pattern` VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'Matched in the item name when item_entry is 0',
  `chance` FLOAT NOT NULL DEFAULT '0' COMMENT 'Percent per weapon fed',
  PRIMARY KEY (`essence_id`, `item_entry`, `name_pattern`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Which fed weapons can leave which Heartseed essence';

-- The two MVP essences. Numbers are placeholders until pacing is tuned in v0.5.
DELETE FROM `heartseed_essence_template` WHERE `id` IN (1, 2);
INSERT INTO `heartseed_essence_template` (`id`, `name`, `proc_spell_id`, `proc_chance`, `weapon_damage_pct`, `description`) VALUES
(1, 'Sha-touched Essence', 9613,  8, 30, 'Placeholder. Hits sometimes lash the target with shadow (Shadow Bolt in the combat log).'),
(2, 'Mogu Essence',        26415, 8, 30, 'Placeholder. Hits sometimes call down lightning (Shock in the combat log).');

DELETE FROM `heartseed_essence_source` WHERE `essence_id` IN (1, 2);
INSERT INTO `heartseed_essence_source` (`essence_id`, `item_entry`, `name_pattern`, `chance`) VALUES
(1, 0, 'sha-',  25),
(2, 0, 'mogu',  25);

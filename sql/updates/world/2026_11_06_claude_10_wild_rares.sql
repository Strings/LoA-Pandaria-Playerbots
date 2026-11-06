-- T-100: wild rares. Idempotent.
-- Creature entries that never turn wild (reload with `.reload rare_wild_denylist`).
CREATE TABLE IF NOT EXISTS `rare_wild_denylist` (
  `entry` INT UNSIGNED NOT NULL COMMENT 'creature_template.entry',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Wild rares (T-100): entries that never turn wild';

-- Every current affix may roll on wild rares too (host bit 8); a designer clears the bit to keep one off them.
UPDATE `affix_template` SET `hosts` = `hosts` | 8 WHERE `id` BETWEEN 1 AND 8;

-- The Wild Rare marker aura (client-patched clone 318100) frees the zone slot and minimap star on death.
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_wild_rare_marker';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (318100, 'spell_wild_rare_marker');

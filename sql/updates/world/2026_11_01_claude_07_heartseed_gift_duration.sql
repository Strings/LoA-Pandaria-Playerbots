-- T-080: Gift summon duration, and Call of Xuen tuned into the rotation (20 s cooldown, 10 s tiger).
-- Idempotent: `heartseed_gift` holds only these rows and is fully defined here, so it is rebuilt with the
-- `duration` column (MySQL 8 has no ADD COLUMN IF NOT EXISTS).
--
-- cooldown and duration now replace the client spell's own (0 keeps it), so a Gift can be shorter than the spell
-- it borrows: Invoke Xuen is 3 minutes and 45 seconds in the client data.

DROP TABLE IF EXISTS `heartseed_gift`;
CREATE TABLE `heartseed_gift` (
  `trait_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_trait major that teaches the Gift',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'Client spell taught while the major is active',
  `cooldown` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Gift cooldown in ms, replacing the spell''s own; 0 keeps it',
  `duration` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Summon lifetime in ms for summoning Gifts, replacing the spell''s; 0 keeps it',
  `base_amount` INT NOT NULL DEFAULT '0' COMMENT 'Gift power at item level 0 (damage per hit for scaled Gifts)',
  `amount_per_ilvl` FLOAT NOT NULL DEFAULT '0' COMMENT 'Added per item level of the main-hand weapon',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`trait_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Celestial Gifts: active spells taught by majors';

INSERT INTO `heartseed_gift` (`trait_id`, `spell_id`, `cooldown`, `duration`, `base_amount`, `amount_per_ilvl`, `comment`) VALUES
(2, 123904, 20000, 10000,  0, 0, 'Call of Xuen: Invoke Xuen, the White Tiger (monk talent); part of the rotation'),
(4, 115008, 60000,     0,  0, 0, 'Crane Rush: Chi Torpedo (monk talent)'),
(6, 119381, 90000,     0,  0, 0, 'Ox Stomp: Leg Sweep (monk talent)'),
(8, 117960, 60000,     0, 30, 4, 'Serpent''s Breath: Celestial Breath (creature spell), damage per tick');

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_heartseed_xuen_lightning';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(123996, 'spell_heartseed_xuen_lightning');

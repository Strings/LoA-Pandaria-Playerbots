-- T-086: characters who hold a Celestial Gift as the old client spell get its clone instead, and their action bar
-- buttons follow. The spell row comes first because the server drops action buttons for spells a character does
-- not know. The Heartseed's login sync removes the old spell (monks keep a talent they took) and drops a clone the
-- character's current major doesn't grant.
INSERT IGNORE INTO `character_spell` (`guid`, `spell`, `active`, `disabled`)
SELECT `guid`, 310100, 1, 0 FROM `character_spell` WHERE `spell` = 123904;
INSERT IGNORE INTO `character_spell` (`guid`, `spell`, `active`, `disabled`)
SELECT `guid`, 310101, 1, 0 FROM `character_spell` WHERE `spell` = 148009;
INSERT IGNORE INTO `character_spell` (`guid`, `spell`, `active`, `disabled`)
SELECT `guid`, 310102, 1, 0 FROM `character_spell` WHERE `spell` = 116098;
INSERT IGNORE INTO `character_spell` (`guid`, `spell`, `active`, `disabled`)
SELECT `guid`, 310103, 1, 0 FROM `character_spell` WHERE `spell` = 117960;

-- Monks keep a button for a talent they took; everyone else's moves to the clone.
UPDATE `character_action` a JOIN `characters` c ON c.`guid` = a.`guid`
SET a.`action` = 310100 WHERE a.`type` = 0 AND a.`action` = 123904 AND c.`class` <> 10;
UPDATE `character_action` SET `action` = 310101 WHERE `type` = 0 AND `action` = 148009;
UPDATE `character_action` SET `action` = 310102 WHERE `type` = 0 AND `action` = 116098;
UPDATE `character_action` SET `action` = 310103 WHERE `type` = 0 AND `action` = 117960;

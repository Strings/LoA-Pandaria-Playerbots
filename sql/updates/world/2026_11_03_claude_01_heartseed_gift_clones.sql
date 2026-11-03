-- T-086: the Celestial Gifts and their rider auras use their own spells (clones from
-- 2026_11_03_claude_00_heartseed_gift_spells.sql and the client patch), so they show their own names, tooltips and
-- icons, and the client spells they came from (monk talents, Berserking, Sprint, Ice Barrier) stay untouched.
UPDATE `heartseed_gift` SET `spell_id` = 310100 WHERE `trait_id` = 2 AND `spell_id` = 123904;
UPDATE `heartseed_gift` SET `spell_id` = 310101 WHERE `trait_id` = 4 AND `spell_id` = 148009;
UPDATE `heartseed_gift` SET `spell_id` = 310102 WHERE `trait_id` = 6 AND `spell_id` = 116098;
UPDATE `heartseed_gift` SET `spell_id` = 310103 WHERE `trait_id` = 8 AND `spell_id` = 117960;

UPDATE `heartseed_gift_rider` SET `spell_id` = 310110 WHERE `spell_id` = 26297;
UPDATE `heartseed_gift_rider` SET `spell_id` = 310111 WHERE `spell_id` = 2983;
UPDATE `heartseed_gift_rider` SET `spell_id` = 310112 WHERE `spell_id` = 11426;
UPDATE `heartseed_gift_rider` SET `spell_id` = 310113 WHERE `spell_id` = 51340;
UPDATE `heartseed_gift_rider` SET `spell_id` = 310114 WHERE `spell_id` = 118540;

-- The Gift scripts move to the clones; the class scripts the Gifts relied on are bound to the clones as well.
DELETE FROM `spell_script_names` WHERE `ScriptName` LIKE 'spell_heartseed_%' AND `spell_id` IN (123904, 148009, 116098, 117960);
DELETE FROM `spell_script_names` WHERE `spell_id` IN (310100, 310101, 310102, 310103);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(310100, 'spell_heartseed_gift'),
(310100, 'spell_monk_invoke_xuen'),
(310101, 'spell_heartseed_gift'),
(310101, 'spell_heartseed_crane_plume'),
(310101, 'spell_item_spirit_of_chi_ji_heal'),
(310102, 'spell_heartseed_gift'),
(310102, 'spell_heartseed_niuzao_retort'),
(310103, 'spell_heartseed_gift');

-- T-097: Durotar treasure items (199000-199006). Low-level trinkets carrying scaled-down effects from
-- later expansions, plus two epics. Each row copies a stock item of the same shape (trinkets from
-- Dog Whistle 3456, the dagger from Talon of Vultros 4454), then gets its own name, stats and spell.
-- The spells (319000-319006) are clones in tools/clientpatch/edits.json and ship in
-- 2026_11_03_claude_00_heartseed_gift_spells.sql. Spell category 0 keeps the trinkets off a shared cooldown.

DELETE FROM `item_template` WHERE `entry` BETWEEN 199000 AND 199006;
DELETE FROM `item_template_addon` WHERE `Id` BETWEEN 199000 AND 199006;

DROP TEMPORARY TABLE IF EXISTS `tmp_t097_item`;
CREATE TEMPORARY TABLE `tmp_t097_item` LIKE `item_template`;

-- Trinkets: six copies of Dog Whistle, 199000-199005.
INSERT INTO `tmp_t097_item` SELECT * FROM `item_template` WHERE `entry` = 3456;
UPDATE `tmp_t097_item` SET `entry` = 199000;
UPDATE `tmp_t097_item` SET
    `bonding` = 1, `maxcount` = 1, `BuyPrice` = 0, `SellPrice` = 250, `ScalingStatDistribution` = 0,
    `Quality` = 3, `ItemLevel` = 16, `RequiredLevel` = 8,
    `stat_type1` = 0, `stat_value1` = 0, `stat_type2` = 0, `stat_value2` = 0, `stat_type3` = 0, `stat_value3` = 0,
    `spelltrigger_1` = 0, `spellcharges_1` = 0, `spellcategory_1` = 0, `spellcategorycooldown_1` = -1;

INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
UPDATE `tmp_t097_item` SET `entry` = 199001; INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
UPDATE `tmp_t097_item` SET `entry` = 199002; INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
UPDATE `tmp_t097_item` SET `entry` = 199003; INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
UPDATE `tmp_t097_item` SET `entry` = 199004; INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
UPDATE `tmp_t097_item` SET `entry` = 199005; INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;

-- The dagger: a copy of Talon of Vultros.
DELETE FROM `tmp_t097_item`;
INSERT INTO `tmp_t097_item` SELECT * FROM `item_template` WHERE `entry` = 4454;
UPDATE `tmp_t097_item` SET `entry` = 199006, `bonding` = 1, `maxcount` = 1, `BuyPrice` = 0, `SellPrice` = 900,
    `ScalingStatDistribution` = 0;
INSERT INTO `item_template` SELECT * FROM `tmp_t097_item`;
DROP TEMPORARY TABLE `tmp_t097_item`;

-- Zalazane's Hex Doll: Echo Isles. Hex, the shaman staple from Wrath onward, at trinket scale.
UPDATE `item_template` SET `name` = 'Zalazane''s Hex Doll', `displayid` = 6358, `ItemLevel` = 18, `RequiredLevel` = 12,
    `stat_type1` = 5, `stat_value1` = 2, `stat_type2` = 7, `stat_value2` = 2,
    `spellid_1` = 319000, `spellcooldown_1` = 120000,
    `description` = 'The stitches still twitch when trolls walk past.'
WHERE `entry` = 199000;

-- Windrider's Plume: Drygulch Ravine. Slow fall from a harpy's feather.
UPDATE `item_template` SET `name` = 'Windrider''s Plume', `displayid` = 7266,
    `stat_type1` = 3, `stat_value1` = 2,
    `spellid_1` = 319001, `spellcooldown_1` = 60000,
    `description` = 'Lighter than it has any right to be.'
WHERE `entry` = 199001;

-- Tidewalker's Charm: Bladefist Bay. Water walking.
UPDATE `item_template` SET `name` = 'Tidewalker''s Charm', `displayid` = 12310,
    `stat_type1` = 6, `stat_value1` = 2, `stat_type2` = 7, `stat_value2` = 2,
    `spellid_1` = 319002, `spellcooldown_1` = 120000,
    `description` = 'The sea gives back what it takes. Sometimes.'
WHERE `entry` = 199002;

-- Thornhide Talisman: Razormane quilboar. A scaled-down Thorns.
UPDATE `item_template` SET `name` = 'Thornhide Talisman', `displayid` = 33849, `RequiredLevel` = 10,
    `stat_type1` = 7, `stat_value1` = 3,
    `spellid_1` = 319003, `spellcooldown_1` = 90000,
    `description` = 'Charged by a Razormane mystic. Still prickly.'
WHERE `entry` = 199003;

-- Burning Blade Brand: Razorwind Canyon. An instant shadowflame strike.
UPDATE `item_template` SET `name` = 'Burning Blade Brand', `displayid` = 6484, `RequiredLevel` = 10,
    `stat_type1` = 4, `stat_value1` = 2, `stat_type2` = 7, `stat_value2` = 2,
    `spellid_1` = 319004, `spellcooldown_1` = 90000,
    `description` = 'Warm to the touch, and it never cools.'
WHERE `entry` = 199004;

-- Ember of the Dormant Flame: epic trinket. A fire nova at level 10.
UPDATE `item_template` SET `name` = 'Ember of the Dormant Flame', `displayid` = 12736, `Quality` = 4,
    `ItemLevel` = 25, `RequiredLevel` = 10, `SellPrice` = 1200,
    `stat_type1` = 7, `stat_value1` = 5, `stat_type2` = 32, `stat_value2` = 3,
    `spellid_1` = 319005, `spellcooldown_1` = 120000,
    `description` = 'It is not dead. It is waiting.'
WHERE `entry` = 199005;

-- Felweaver's Fang: epic dagger with a chance on hit to bite for fel shadow (2.5 procs a minute).
UPDATE `item_template` SET `name` = 'Felweaver''s Fang', `displayid` = 25613, `Quality` = 4,
    `ItemLevel` = 25, `RequiredLevel` = 12,
    `stat_type1` = 3, `stat_value1` = 4, `stat_type2` = 7, `stat_value2` = 3, `stat_type3` = 0, `stat_value3` = 0,
    `spellid_1` = 319006, `spelltrigger_1` = 2, `spellcharges_1` = 0, `spellcooldown_1` = -1,
    `spellcategory_1` = 0, `spellcategorycooldown_1` = -1,
    `description` = 'Shadow Council work. The fel in it is hungry.'
WHERE `entry` = 199006;

INSERT INTO `item_template_addon` (`Id`, `FlagsCu`, `FoodType`, `MinMoneyLoot`, `MaxMoneyLoot`, `SpellPPMChance`)
VALUES (199006, 0, 0, 0, 0, 2.5);

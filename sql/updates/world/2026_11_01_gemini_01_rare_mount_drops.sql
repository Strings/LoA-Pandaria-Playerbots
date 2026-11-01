-- T-064: Add missing rare mount loot rows for MoP world bosses at 10% baseline
-- Follow-up to T-012 audit: Sha of Anger, Galleon, and Oondasta
-- Sha of Anger (60491): Reins of the Heavenly Onyx Cloud Serpent (87771)
-- Galleon (62346): Son of Galleon's Saddle (89783)
-- Oondasta (69161): Reins of the Cobalt Primordial Direhorn (94228), and link lootid 69161

START TRANSACTION;

-- Ensure Oondasta (69161) uses creature_loot_template entry 69161
UPDATE creature_template SET lootid = 69161 WHERE entry = 69161;

-- Sha of Anger (60491) -> Reins of the Heavenly Onyx Cloud Serpent (87771)
DELETE FROM creature_loot_template WHERE entry = 60491 AND item = 87771;
INSERT INTO creature_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(60491, 87771, 10, 0, 0, 1, 1);

-- Galleon (62346) -> Son of Galleon's Saddle (89783)
DELETE FROM creature_loot_template WHERE entry = 62346 AND item = 89783;
INSERT INTO creature_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(62346, 89783, 10, 0, 0, 1, 1);

-- Oondasta (69161) -> Reins of the Cobalt Primordial Direhorn (94228)
DELETE FROM creature_loot_template WHERE entry = 69161 AND item = 94228;
INSERT INTO creature_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(69161, 94228, 10, 0, 0, 1, 1);

COMMIT;

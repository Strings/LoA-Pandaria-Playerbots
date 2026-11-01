-- T-072: MoP World Boss Loot Audit
-- Audit checked all MoP world bosses and Timeless Isle rare elites.
-- Nalak (69099), Celestials (71952-71955), Ordos (72057), and all 31 Timeless Isle rares
-- already possess complete loot templates in creature_loot_template.
-- Sha of Anger (60491), Galleon (62346), and Oondasta (69161) have only their mount drops
-- (added in PR 72 / T-064). No existing reference_loot_template tables exist for their
-- gear in world_gemini. Per T-072 instructions, item lists are not invented; their missing
-- non-mount loot is audited and documented in docs/agents/world-boss-loot.md.
--
-- This update idempotently ensures lootid references are intact for audited world bosses.

START TRANSACTION;

UPDATE `creature_template` SET `lootid` = 60491 WHERE `entry` = 60491;
UPDATE `creature_template` SET `lootid` = 62346 WHERE `entry` = 62346;
UPDATE `creature_template` SET `lootid` = 69161 WHERE `entry` = 69161;

COMMIT;

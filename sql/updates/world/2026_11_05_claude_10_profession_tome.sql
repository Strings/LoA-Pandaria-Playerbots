-- T-096: profession tome. The Gusting Grimoire companion (creature 69208, journal species 1174, summon
-- spell 136484) becomes a trainer for every primary and secondary profession. Its trainer list is the
-- union of every stock trainer's rows for those skills, so it teaches each rank and recipe some trainer
-- teaches, at the stock cost and requirements. The script (npc_profession_tome) shows one profession at a
-- time; buying validates against this full list. Idempotent: the list is rebuilt from the stock rows.
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 | 16, `trainer_type` = 2, `ScriptName` = 'npc_profession_tome'
WHERE `entry` = 69208;

DELETE FROM `npc_trainer` WHERE `entry` = 69208;
INSERT INTO `npc_trainer` (`entry`, `spell`, `spellcost`, `reqskill`, `reqskillvalue`, `reqlevel`)
SELECT 69208, `t`.`spell`, MIN(`t`.`spellcost`), MAX(`t`.`reqskill`), MIN(`t`.`reqskillvalue`), MIN(`t`.`reqlevel`)
FROM `npc_trainer` `t`
WHERE `t`.`spell` > 0 AND `t`.`entry` <> 69208
  AND `t`.`reqskill` IN (129, 164, 165, 171, 182, 185, 186, 197, 202, 333, 356, 393, 755, 773, 794)
GROUP BY `t`.`spell`;


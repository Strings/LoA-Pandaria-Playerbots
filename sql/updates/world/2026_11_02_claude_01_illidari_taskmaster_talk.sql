-- Illidari Taskmaster (17058) said its aggro line through the action invoker. When a Fleeing Dreghood Warrior
-- (19477) started the fight, SmartAI made the warrior speak group 1, which it does not have, flooding DBErrors.log
-- with "Could not find TextGroup 1". The Taskmaster speaks the line itself; the talk target is still the invoker.
-- Idempotent: scoped UPDATE.

UPDATE `smart_scripts` SET `target_type` = 1 WHERE `entryorguid` = 17058 AND `source_type` = 0 AND `id` = 2 AND `action_type` = 1;

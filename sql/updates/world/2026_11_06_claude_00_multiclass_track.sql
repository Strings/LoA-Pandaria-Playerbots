-- T-098: second classes open at character level 10 and their 30-level track runs to 90, so the first unlocks
-- come closer together: second-class levels 1, 3 and 5 (were 1, 5 and 10). Scoped, re-runnable.
UPDATE `multiclass_ability` SET `level` = 3 WHERE `class_id` = 1 AND `spell_id` = 320001;
UPDATE `multiclass_ability` SET `level` = 5 WHERE `class_id` = 1 AND `spell_id` = 320002;
UPDATE `multiclass_ability` SET `level` = 3 WHERE `class_id` = 2 AND `spell_id` = 320004;
UPDATE `multiclass_ability` SET `level` = 5 WHERE `class_id` = 2 AND `spell_id` = 320005;

UPDATE `multiclass_rule` SET `min_level` = 5 WHERE `id` IN (4, 5, 6) AND `class_id` = 1;
UPDATE `multiclass_rule` SET `min_level` = 3 WHERE `id` = 8 AND `class_id` = 2;
UPDATE `multiclass_rule` SET `min_level` = 5 WHERE `id` = 9 AND `class_id` = 2;

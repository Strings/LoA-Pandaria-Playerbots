-- Playtest fix: every checkpoint teaches riding for its level and the race's mounts (step 7, free in all of them).
-- Idempotent.
DELETE FROM `gmtools_checkpoint_step` WHERE `step` = 7 AND `type` = 'riding';
INSERT INTO `gmtools_checkpoint_step` (`checkpoint`, `step`, `type`, `arg1`, `arg2`, `text`, `comment`)
SELECT `name`, 7, 'riding', 0, 0, '', 'riding for the level and racial mounts'
FROM `gmtools_checkpoint`
WHERE `name` NOT IN (SELECT `checkpoint` FROM (SELECT `checkpoint` FROM `gmtools_checkpoint_step` WHERE `step` = 7) AS `taken`);

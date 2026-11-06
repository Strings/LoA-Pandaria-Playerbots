-- T-104: weapon-based second-class abilities. multiclass_ability gains a share of another spell's bonus damage
-- (bonus_spell / bonus_effect / bonus_pct) and a power gain per cast (power_gain). Hurl Axe: 150% weapon damage
-- (spell data, tools/clientpatch/edits.json) plus 80% of Mortal Strike's bonus (12294 effect 1), 20 Rage, 6 s
-- cooldown, no cost, no longer Gift-power damage. Idempotent: columns are added only when missing.
SET @has_col := (SELECT COUNT(*) FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'multiclass_ability' AND COLUMN_NAME = 'bonus_spell');
SET @ddl := IF(@has_col = 0,
    'ALTER TABLE `multiclass_ability` ADD COLUMN `bonus_spell` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''adds bonus_pct % of this spell''''s effect bonus_effect value (at the caster''''s level) to each hit'' AFTER `cost_pct`, ADD COLUMN `bonus_effect` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `bonus_spell`, ADD COLUMN `bonus_pct` INT NOT NULL DEFAULT 0 AFTER `bonus_effect`, ADD COLUMN `power_gain` INT NOT NULL DEFAULT 0 COMMENT ''power_type gained per cast, raw units (rage in tenths: 200 = 20 Rage)'' AFTER `bonus_pct`',
    'DO 0');
PREPARE add_cols FROM @ddl;
EXECUTE add_cols;
DEALLOCATE PREPARE add_cols;

UPDATE `multiclass_ability`
SET `power_pct` = 0, `cooldown` = 6000, `power_type` = 1, `cost_pct` = 0,
    `bonus_spell` = 12294, `bonus_effect` = 1, `bonus_pct` = 80, `power_gain` = 200,
    `description` = 'Hurl an axe: 150% weapon damage plus 80% of Mortal Strike''s bonus, 20 Rage, 1 s cast on the move.'
WHERE `spell_id` = 320000;

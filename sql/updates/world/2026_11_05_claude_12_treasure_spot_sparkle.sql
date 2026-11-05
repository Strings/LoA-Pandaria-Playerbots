-- T-097: treasures hidden in plain sight. treasure_spot.sparkle = 0 keeps a treasure from sparkling, so only the
-- cursor turning to a hand over it gives it away. Idempotent: adds the column only when it is missing.
SET @has_sparkle := (SELECT COUNT(*) FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'treasure_spot' AND COLUMN_NAME = 'sparkle');
SET @ddl := IF(@has_sparkle = 0,
    'ALTER TABLE `treasure_spot` ADD COLUMN `sparkle` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT ''0 = no sparkle, hidden in plain sight'' AFTER `road_z`',
    'DO 0');
PREPARE add_sparkle FROM @ddl;
EXECUTE add_sparkle;
DEALLOCATE PREPARE add_sparkle;

-- T-093: treasures each character has opened. A treasure is a gameobject spawn whose template uses the go_treasure
-- script; `spawn_id` is that spawn's world.gameobject guid. A row hides the treasure from this character only.
CREATE TABLE IF NOT EXISTS `character_treasure` (
    `guid`      INT UNSIGNED NOT NULL COMMENT 'characters.guid',
    `spawn_id`  INT UNSIGNED NOT NULL COMMENT 'world.gameobject.guid of the treasure',
    `looted_at` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'unix time',
    PRIMARY KEY (`guid`, `spawn_id`),
    KEY `idx_spawn` (`spawn_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='T-093 per-character treasures';

-- T-093: metadata for hand-placed treasures. The treasure itself is a gameobject spawn whose template uses the
-- go_treasure script; this row names the spot and stores the road point the validator walks from.
CREATE TABLE IF NOT EXISTS `treasure_spot` (
    `spawn_guid` INT UNSIGNED NOT NULL COMMENT 'gameobject.guid',
    `label`      VARCHAR(100) NOT NULL DEFAULT '' COMMENT 'shown by .treasure commands and on opening',
    `landmark`   VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'why the spot is there',
    `road_x`     FLOAT NOT NULL DEFAULT 0 COMMENT 'access point a player walks from (0,0,0 = none)',
    `road_y`     FLOAT NOT NULL DEFAULT 0,
    `road_z`     FLOAT NOT NULL DEFAULT 0,
    PRIMARY KEY (`spawn_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='T-093 treasure spots';

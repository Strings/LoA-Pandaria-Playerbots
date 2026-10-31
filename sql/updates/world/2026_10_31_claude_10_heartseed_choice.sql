-- T-023: the Heartseed's major-choice window and the Celestial shrines where a chosen major can be changed.
-- IDs from the Heartseed block (docs/ID_RANGES.md). Idempotent.

CREATE TABLE IF NOT EXISTS `heartseed_shrine` (
    `id`         INT UNSIGNED NOT NULL,
    `map`        INT UNSIGNED NOT NULL,
    `position_x` FLOAT NOT NULL,
    `position_y` FLOAT NOT NULL,
    `position_z` FLOAT NOT NULL,
    `radius`     FLOAT NOT NULL COMMENT 'yards; within this distance the character is at the shrine',
    `name`       VARCHAR(100) NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed: Celestial shrines where chosen majors can be changed (T-023)';

-- The four Celestial temples and the Celestial Court, at their Celestial.
DELETE FROM `heartseed_shrine` WHERE `id` BETWEEN 1 AND 5;
INSERT INTO `heartseed_shrine` (`id`, `map`, `position_x`, `position_y`, `position_z`, `radius`, `name`) VALUES
(1, 870, 3837.5, 527.9, 639.7, 80, 'Temple of the White Tiger'),
(2, 870, -1710.9, 1070.9, 29.5, 80, 'Temple of the Red Crane'),
(3, 870, 2159.6, 4852.3, 182.0, 80, 'Niuzao Temple'),
(4, 870, 961.4, -2463.3, 180.6, 80, 'Temple of the Jade Serpent'),
(5, 870, -650.0, -5015.0, -6.2, 100, 'Celestial Court');

DELETE FROM `npc_text` WHERE `ID` IN (90002, 90003);
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `BroadcastTextID0`, `lang0`, `Probability0`) VALUES
(90002, 'Your Heartseed hums against your palm. Four voices answer it, faint as wind through bamboo: the Tiger, the Crane, the Ox and the Serpent.$B$BEach time you grow, one of them can be heard a little more clearly.', '', 0, 0, 1),
(90003, 'Which voice will your Heartseed follow?', '', 0, 0, 1);

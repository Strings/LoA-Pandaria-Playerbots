-- T-039: GM test scenarios for .scenario load. A scenario is an ordered list of GM command lines run as the
-- calling GM. Placeholders: {player}, {target} (selected unit), {map}, {zone}, {x}, {y}, {z}, {o} (the GM's
-- position) and {arg1}.. (extra words after the name). A step may not start another scenario. Idempotent.

CREATE TABLE IF NOT EXISTS `gmtools_scenario` (
    `name`        VARCHAR(32) NOT NULL,
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    `usage`       VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'arguments after the name, shown by .scenario explain',
    PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: test scenarios (T-039)';

CREATE TABLE IF NOT EXISTS `gmtools_scenario_step` (
    `scenario` VARCHAR(32) NOT NULL,
    `step`     TINYINT UNSIGNED NOT NULL COMMENT 'run in this order',
    `command`  VARCHAR(255) NOT NULL COMMENT 'GM command line with placeholders',
    `comment`  VARCHAR(255) NOT NULL DEFAULT '',
    PRIMARY KEY (`scenario`, `step`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='GM tools: test scenario steps (T-039)';

DELETE FROM `gmtools_scenario` WHERE `name` IN ('bot-wander-here', 'bot-fight-here', 'clock-next-day');
INSERT INTO `gmtools_scenario` (`name`, `description`, `usage`) VALUES
    ('bot-wander-here', 'Selected bot wanders around your position for a while, traced for .bot why', '<seconds>'),
    ('bot-fight-here', 'Selected bot fights a creature spawned at your position, traced for .bot why', '<creature entry> <seconds>'),
    ('clock-next-day', 'Jump the custom clock a day ahead and show it', '');

DELETE FROM `gmtools_scenario_step` WHERE `scenario` IN ('bot-wander-here', 'bot-fight-here', 'clock-next-day');
INSERT INTO `gmtools_scenario_step` (`scenario`, `step`, `command`, `comment`) VALUES
    ('bot-wander-here', 1, '.botscenario {target} wander {map} {x} {y} {z} {arg1} random', 'arg1: seconds'),
    ('bot-wander-here', 2, '.bot why {target}', 'turns the decision trace on'),
    ('bot-fight-here', 1, '.botscenario {target} attack {map} {x} {y} {z} {arg1} {arg2} spawn', 'arg1: creature entry, arg2: seconds'),
    ('bot-fight-here', 2, '.bot why {target}', 'turns the decision trace on'),
    ('clock-next-day', 1, '.clock jump 1d', ''),
    ('clock-next-day', 2, '.clock info', '');

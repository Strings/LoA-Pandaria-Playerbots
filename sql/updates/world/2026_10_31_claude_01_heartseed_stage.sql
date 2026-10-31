-- T-020: Heartseed stages (docs/DESIGN.md, The Heartseed, Progression) and their weapon glows.
-- The glow is display only: the enchant ID is written to the visible item field, never to the item, so the
-- enchant's own effects never apply. Glow IDs are placeholders picked for having a visual; choose the final
-- looks in the client. path 0 is the glow with no dominant Celestial path; path rows 1-4 (Xuen, Chi-Ji,
-- Niuzao, Yu'lon) override it once paths exist (T-021).

CREATE TABLE IF NOT EXISTS `heartseed_stage` (
  `stage` TINYINT UNSIGNED NOT NULL,
  `min_level` TINYINT UNSIGNED NOT NULL COMMENT 'Character level the stage starts at',
  `name` VARCHAR(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`stage`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed stages by character level';

CREATE TABLE IF NOT EXISTS `heartseed_glow` (
  `stage` TINYINT UNSIGNED NOT NULL,
  `path` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 = none, 1 Xuen, 2 Chi-Ji, 3 Niuzao, 4 Yu''lon',
  `enchant_id` SMALLINT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'SpellItemEnchantment.dbc ID shown as the glow; 0 = no glow',
  `comment` VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`stage`, `path`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed weapon glow by stage and dominant path';

DELETE FROM `heartseed_stage`;
INSERT INTO `heartseed_stage` (`stage`, `min_level`, `name`) VALUES
(0, 1, 'Seed'),
(1, 10, 'Sprout'),
(2, 30, 'Sapling'),
(3, 60, 'Bloom'),
(4, 85, 'Full Bloom'),
(5, 90, 'Celestial Weapon');

DELETE FROM `heartseed_glow`;
INSERT INTO `heartseed_glow` (`stage`, `path`, `enchant_id`, `comment`) VALUES
(0, 0, 1900, 'Seed: faint (placeholder: Crusader)'),
(1, 0, 4446, 'Sprout: soft (placeholder: River''s Song)'),
(2, 0, 4441, 'Sapling: clear (placeholder: Windsong)'),
(3, 0, 4442, 'Bloom: bright (placeholder: Jade Spirit)'),
(4, 0, 4444, 'Full Bloom: radiant (placeholder: Dancing Steel)'),
(5, 0, 4442, 'Celestial Weapon (placeholder: Jade Spirit)');

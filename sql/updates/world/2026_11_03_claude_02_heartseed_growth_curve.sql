-- Heartseed Growth stats approach a ceiling instead of growing 1% per point forever: the bonus is
-- max_pct * points / (points + half_points), to the nearest whole percent. A percent of crit or haste is worth
-- more than a percent of stamina, so each stat has its own ceiling. Static data: the table is rebuilt whole.
DROP TABLE IF EXISTS `heartseed_growth_stat`;
CREATE TABLE `heartseed_growth_stat` (
  `id` TINYINT UNSIGNED NOT NULL COMMENT 'Saved in character_heartseed_growth_point.stat_id: append only',
  `name` VARCHAR(50) NOT NULL DEFAULT '',
  `main_stat` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = strength, agility or intellect by the character''s specialization (spells 310011-310013)',
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Passive aura whose effect 0 is the bonus; unused for main_stat',
  `max_pct` INT NOT NULL DEFAULT 0 COMMENT 'Ceiling the bonus approaches, in percent of the stat',
  `half_points` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Points that give half of max_pct; keep max_pct / (1 + half_points) >= 0.5 so the first point shows',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Growth stat points (T-083)';

INSERT INTO `heartseed_growth_stat` (`id`, `name`, `main_stat`, `spell_id`, `max_pct`, `half_points`, `comment`) VALUES
(1, 'Stamina',         0, 310010, 30, 20, 'Up to +30% stamina, +15% at 20 points'),
(2, 'Main stat',       1,      0, 20, 20, 'Up to +20% strength, agility or intellect by specialization, +10% at 20 points'),
(3, 'Critical strike', 0, 310014, 10, 10, 'Up to +10% critical strike chance, +5% at 10 points'),
(4, 'Haste',           0, 310015, 10, 10, 'Up to +10% attack and casting speed, +5% at 10 points');

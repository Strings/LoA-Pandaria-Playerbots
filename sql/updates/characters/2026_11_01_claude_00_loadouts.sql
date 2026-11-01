-- T-017: spec loadouts. Any number per character (one per specialization plus named extras); the client's two
-- talent groups each hold one of them. Idempotent: creates missing tables only, never drops player data.

CREATE TABLE IF NOT EXISTS `character_loadout` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `id` INT UNSIGNED NOT NULL COMMENT 'Per character, never reused while the row exists',
  `name` VARCHAR(32) NOT NULL DEFAULT '',
  `spec_id` SMALLINT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'ChrSpecialization.dbc ID',
  `is_default` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '1 = the specialization''s own loadout, not deletable',
  `talent_group` TINYINT UNSIGNED NOT NULL DEFAULT '255' COMMENT 'Talent group holding it (0 or 1), 255 = none',
  `captured` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT '0 until it has held real talents and bars',
  PRIMARY KEY (`guid`, `id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Spec loadouts';

CREATE TABLE IF NOT EXISTS `character_loadout_talent` (
  `guid` INT UNSIGNED NOT NULL,
  `loadout_id` INT UNSIGNED NOT NULL,
  `spell` INT UNSIGNED NOT NULL COMMENT 'Talent spell id, as character_talent.spell',
  PRIMARY KEY (`guid`, `loadout_id`, `spell`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Talents of a spec loadout';

CREATE TABLE IF NOT EXISTS `character_loadout_glyph` (
  `guid` INT UNSIGNED NOT NULL,
  `loadout_id` INT UNSIGNED NOT NULL,
  `slot` TINYINT UNSIGNED NOT NULL COMMENT 'Glyph slot index 0-5',
  `glyph` INT UNSIGNED NOT NULL COMMENT 'GlyphProperties.dbc ID',
  PRIMARY KEY (`guid`, `loadout_id`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Glyphs of a spec loadout';

CREATE TABLE IF NOT EXISTS `character_loadout_action` (
  `guid` INT UNSIGNED NOT NULL,
  `loadout_id` INT UNSIGNED NOT NULL,
  `button` TINYINT UNSIGNED NOT NULL,
  `action` INT UNSIGNED NOT NULL,
  `type` TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`, `loadout_id`, `button`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Action bars of a spec loadout, as character_action';

CREATE TABLE IF NOT EXISTS `character_loadout_part` (
  `guid` INT UNSIGNED NOT NULL,
  `loadout_id` INT UNSIGNED NOT NULL,
  `part` VARCHAR(32) NOT NULL COMMENT 'System that owns the data (heartseed, ...)',
  `data` TEXT NOT NULL,
  PRIMARY KEY (`guid`, `loadout_id`, `part`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Other systems'' state saved with a spec loadout';

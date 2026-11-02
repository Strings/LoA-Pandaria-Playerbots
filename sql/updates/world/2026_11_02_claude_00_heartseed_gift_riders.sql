-- T-082: Celestial Gift riders. Each Heartseed stage adds one to the Gift (Sapling 2, Bloom 3, Full Bloom 4).
-- `kind` is Heartseed::GiftRiderKind (HeartseedRules.h). Helper spells are client spells whose effect 0 amount and
-- duration the rider replaces. Numbers are placeholders until playtests; .reload heartseed picks up changes.
-- Idempotent: the table holds only these rows.

CREATE TABLE IF NOT EXISTS `heartseed_gift_rider` (
  `trait_id` INT UNSIGNED NOT NULL COMMENT 'heartseed_gift trait the rider belongs to',
  `stage` TINYINT UNSIGNED NOT NULL COMMENT 'heartseed_stage that adds it; later stages keep it',
  `kind` TINYINT UNSIGNED NOT NULL COMMENT '1 self aura, 2 extra targets, 3 kill cooldown, 4 heal %, 5 target aura, 6 absorb per target, 7 widen, 8 leech %, 9 target DoT, 10 hit cooldown',
  `amount` INT NOT NULL DEFAULT '0',
  `duration` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'ms; 0 keeps the helper spell''s own',
  `spell_id` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Helper spell for aura kinds',
  `comment` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`trait_id`, `stage`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Heartseed Celestial Gift riders per stage';

DELETE FROM `heartseed_gift_rider`;
INSERT INTO `heartseed_gift_rider` (`trait_id`, `stage`, `kind`, `amount`, `duration`, `spell_id`, `comment`) VALUES
(2, 2,  1,   15, 10000,  26297, 'Call of Xuen, Sapling: 15% haste while the tiger fights (Berserking aura)'),
(2, 3,  2,    2,     0,      0, 'Call of Xuen, Bloom: the tiger''s lightning arcs to 2 more enemies'),
(2, 4,  3, 1000,     0,      0, 'Call of Xuen, Full Bloom: each kill cuts the cooldown by 1 s'),
(4, 2,  4,   10,     0,      0, 'Crane Rush, Sapling: heals 10% of maximum health'),
(4, 3,  1,   40,  4000,   2983, 'Crane Rush, Bloom: 40% movement speed for 4 s (Sprint aura)'),
(4, 4,  3,    0,     0,      0, 'Crane Rush, Full Bloom: a killing blow resets the cooldown'),
(6, 2,  5,  -20,  8000,  51340, 'Ox Stomp, Sapling: stunned enemies deal 20% less damage for 8 s'),
(6, 3,  6,    5, 15000,  11426, 'Ox Stomp, Bloom: absorbs 5% of maximum health per enemy hit for 15 s (Ice Barrier aura)'),
(6, 4,  7,   50,  1000,      0, 'Ox Stomp, Full Bloom: 50% wider, and the stun lasts 1 s longer'),
(8, 2,  8,   30,     0,      0, 'Serpent''s Breath, Sapling: heals for 30% of the damage done'),
(8, 3,  9,   25,  4000, 118540, 'Serpent''s Breath, Bloom: jade fire burns each enemy hit for 25% of the Gift power per second for 4 s'),
(8, 4, 10, 1500,     0,      0, 'Serpent''s Breath, Full Bloom: each enemy hit cuts the cooldown by 1.5 s');

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_heartseed_ox_stomp';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(119381, 'spell_heartseed_ox_stomp');

-- Serpent's Breath: a tick is now 68% of the Gift power (a level base plus weighted attack or spell power, see
-- Heartseed.Gift.LevelBasePct), so an empty cast is about twice the class's filler and a full meter four times.
UPDATE `heartseed_gift` SET `base_amount` = 68,
    `comment` = 'Serpent''s Breath: Celestial Breath (creature spell); tick = % of the Gift power' WHERE `trait_id` = 8;

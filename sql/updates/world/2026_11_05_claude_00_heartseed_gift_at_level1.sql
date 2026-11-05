-- T-091: the Celestial Gift opens at level 1 (Seed stage 0). Niuzao's Retort shield was its Sprout (stage 1)
-- rider; the design makes the shield part of the Gift itself, so it comes with the Gift at stage 0.
UPDATE `heartseed_gift_rider` SET `stage` = 0,
    `comment` = 'With the Gift: shield for 30% of the damage answered (Ice Barrier as helper)'
WHERE `trait_id` = 6 AND `stage` = 1 AND `kind` = 11;

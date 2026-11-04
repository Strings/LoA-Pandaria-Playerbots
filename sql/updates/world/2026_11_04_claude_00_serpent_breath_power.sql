-- Serpent's Breath (T-086i): each tick 38% of attack or spell power, up from 30% (+25%), now that the breath
-- casts at once (cast_time_index 1 in tools/clientpatch/edits.json).
UPDATE `heartseed_gift` SET `base_amount` = 38 WHERE `trait_id` = 8;

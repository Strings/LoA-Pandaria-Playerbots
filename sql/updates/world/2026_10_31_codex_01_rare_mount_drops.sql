-- T-012: rare mount baseline 10%, across all 12 standard item-bearing loot stores.
-- Current v1.0.4 data; explicit scoped absolute updates, safe to reapply.
-- Independent mount rolls avoid mixed-group starvation; mount-only reference pools stay grouped.
-- Guaranteed/higher-chance leaves and eligibility/loot modes/counts remain unchanged.
-- See companion audit for reference math, coverage and caveats.
START TRANSACTION;
-- Reins of the Onyxian Drake; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=10184 AND item=49636 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Reins of the Onyxian Drake; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=10184 AND item=49636 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 0.9984, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=10440 AND item=13335 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Razzashi Raptor; mount floor; old chance 0.9144, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=11382 AND item=19872 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Zulian Tiger; mount floor; old chance 0.6064, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=14509 AND item=19902 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Fiery Warhorse's Reins; mount floor; old chance 2, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=15550 AND item=30480 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Fiery Warhorse's Reins; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=16152 AND item=30480 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Ashes of Al'ar; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=19622 AND item=32458 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Raven Lord; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=23035 AND item=32768 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift White Hawkstrider; mount floor; old chance 3, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=24664 AND item=35513 AND (lootmode+0)=4 AND mincountOrRef=1;
-- Reins of the Blue Proto-Drake; mount floor; old chance 0.9467, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=26693 AND item=44151 AND (lootmode+0)=2 AND mincountOrRef=1;
-- Reins of the Blue Proto-Drake; mount floor; old chance 0.8, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=26693 AND item=44151 AND (lootmode+0)=4 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=31125 AND item=43959 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=31125 AND item=44083 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Mimiron's Head; mount floor; old chance 3, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=33288 AND item=45693 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=33993 AND item=43959 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=33993 AND item=44083 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=35013 AND item=43959 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=35013 AND item=43959 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=35013 AND item=44083 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=35013 AND item=44083 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=38433 AND item=43959 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=38433 AND item=43959 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=38433 AND item=44083 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Reins of the Grand Black War Mammoth; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=38433 AND item=44083 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Reins of the Vitreous Stone Drake; mount floor; old chance 0.7, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=43214 AND item=63043 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the North Wind; mount floor; old chance 0.7, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=43873 AND item=63040 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 0.7879, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=45412 AND item=13335 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=46753 AND item=63041 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Swift Zulian Panther; mount floor; old chance 2, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52059 AND item=68824 AND (lootmode+0)=4 AND mincountOrRef=1;
-- Armored Razzashi Raptor; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52151 AND item=68823 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Smoldering Egg of Millagazor; mount floor; old chance 2, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52409 AND item=69224 AND (lootmode+0)=1 AND mincountOrRef=1;
-- Smoldering Egg of Millagazor; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52409 AND item=69224 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Flametalon of Alysrazor; mount floor; old chance 2, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52530 AND item=71665 AND (lootmode+0)=1 AND mincountOrRef=1;
-- Flametalon of Alysrazor; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52530 AND item=71665 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Flametalon of Alysrazor; mount floor; old chance 2, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52530 AND item=71665 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Spawn of Horridon; mount floor; old chance 2.5, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=68476 AND item=93666 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Spawn of Horridon; mount floor; old chance 2.5, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=68476 AND item=93666 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Spawn of Horridon; mount floor; old chance 5, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=68476 AND item=93666 AND (lootmode+0)=32 AND mincountOrRef=1;
-- Spawn of Horridon; mount floor; old chance 5, group 1
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=68476 AND item=93666 AND (lootmode+0)=64 AND mincountOrRef=1;
-- Clutch of Ji-Kun; mount floor; old chance 2.5, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69712 AND item=95059 AND (lootmode+0)=8 AND mincountOrRef=1;
-- Clutch of Ji-Kun; mount floor; old chance 2.5, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69712 AND item=95059 AND (lootmode+0)=16 AND mincountOrRef=1;
-- Clutch of Ji-Kun; mount floor; old chance 5, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69712 AND item=95059 AND (lootmode+0)=32 AND mincountOrRef=1;
-- Clutch of Ji-Kun; mount floor; old chance 5, group 2
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69712 AND item=95059 AND (lootmode+0)=64 AND mincountOrRef=1;
-- Reins of the Thundering Onyx Cloud Serpent; mount floor; old chance 1, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=73167 AND item=104269 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.01, group 1
UPDATE fishing_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=5805 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.01, group 1
UPDATE fishing_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=5840 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.01, group 1
UPDATE fishing_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=6661 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.01, group 1
UPDATE fishing_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=6757 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42519 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42520 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42522 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42523 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42525 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42526 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=42528 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Astral Cloud Serpent; mount floor; old chance 5, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=43504 AND item=87777 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Astral Cloud Serpent; mount floor; old chance 10, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=44053 AND item=87777 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192046 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192048 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192049 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192050 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192051 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192052 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192053 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192054 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192057 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=192059 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blue Drake; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=193905 AND item=43953 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Azure Drake; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=193967 AND item=43952 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=194479 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202776 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202777 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202778 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202779 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202780 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=202781 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.1, group 1
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=207724 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=207891 AND item=63041 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=207892 AND item=63041 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=207893 AND item=63041 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=207894 AND item=63041 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Smoldering Egg of Millagazor; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=208967 AND item=69224 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Smoldering Egg of Millagazor; mount floor; old chance 1, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=209261 AND item=69224 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Sea Turtle; mount floor; old chance 0.01, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=210216 AND item=46109 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blazing Drake; mount floor; old chance 5, group 0
UPDATE gameobject_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=210217 AND item=77067 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Magic Broom; mount floor; old chance 0.1, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=37586 AND item=37011 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Green Proto-Drake; mount floor; old chance 2, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=39883 AND item=44707 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the White Polar Bear; mount floor; old chance 1, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=44751 AND item=43962 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Springstrider; mount floor; old chance 0.0003, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=45072 AND item=72145 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blue Drake; mount floor; old chance 5, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=52676 AND item=43953 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Flying Broom; mount floor; old chance 5, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=54516 AND item=33176 AND (lootmode+0)=0 AND mincountOrRef=1;
-- The Horseman's Reins; mount floor; old chance 1, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=54516 AND item=37012 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Brewfest Ram; mount floor; old chance 5, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=54535 AND item=33977 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Great Brewfest Kodo; mount floor; old chance 5, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=54535 AND item=37828 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Big Love Rocket; mount floor; old chance 1, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=54537 AND item=50250 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 0.0975, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=13335 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Raven Lord; mount floor; old chance 0.0878, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=32768 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift White Hawkstrider; mount floor; old chance 0.2184, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=35513 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blue Drake; mount floor; old chance 0.3413, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=43953 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the White Polar Bear; mount floor; old chance 0.3452, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=43962 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blue Proto-Drake; mount floor; old chance 0.2184, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=44151 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Green Proto-Drake; mount floor; old chance 0.3296, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=69903 AND item=44707 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Mechano-Hog; mount floor; old chance 0.01, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=86623 AND item=41508 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Mekgineer's Chopper; mount floor; old chance 0.01, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=86623 AND item=44413 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Depleted-Kyparium Rocket; mount floor; old chance 0.01, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=86623 AND item=87250 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Geosynchronous World Spinner; mount floor; old chance 0.01, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=86623 AND item=87251 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 1, group 2
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=89613 AND item=13335 AND (lootmode+0)=1 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 1, group 2
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=92813 AND item=13335 AND (lootmode+0)=1 AND mincountOrRef=1;
-- Deathcharger's Reins; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=13335 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Razzashi Raptor; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=19872 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Zulian Tiger; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=19902 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Riding Turtle; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=23720 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Ashes of Al'ar; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=32458 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Raven Lord; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=32768 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift White Hawkstrider; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=35513 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the White Polar Bear; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=43962 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blue Proto-Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=44151 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Plagued Proto-Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=44175 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Green Proto-Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=44707 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Mimiron's Head; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=45693 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Rusted Proto-Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=45802 AND (lootmode+0)=0 AND mincountOrRef=1;
-- X-51 Nether-Rocket X-TREME; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=49286 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Onyxian Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=49636 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Invincible's Reins; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=50818 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Wooly White Rhino; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=54068 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the North Wind; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=63040 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the South Wind; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=63041 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Vitreous Stone Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=63043 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Smoldering Egg of Millagazor; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=69224 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Flametalon of Alysrazor; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=71665 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Blazing Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=97565 AND item=77067 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Riding Turtle; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=102137 AND item=23720 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Vitreous Stone Drake; mount floor; old chance 0, group 1
UPDATE item_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=102137 AND item=63043 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Red Qiraji Resonating Crystal; mount floor; old chance 2, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=21321 AND item=21321 AND (lootmode+0)=0 AND mincountOrRef=1;
-- X-51 Nether-Rocket; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=35225 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Fossilized Raptor; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=60954 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Drake of the East Wind; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=62901 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Dark Phoenix; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=63125 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Armored Razzashi Raptor; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=68823 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Zulian Panther; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=68824 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Swift Forest Strider; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=72140 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Imperial Quilen; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=85870 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Son of Galleon's Saddle; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=89783 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Hearthsteed; mount floor; old chance 9.4, group 1
UPDATE reference_loot_template SET ChanceOrQuestChance=10, groupid=1 WHERE entry=62901 AND item=98618 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Riding Turtle; mount floor; old chance 0.5, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=23720 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Big Battle Bear; mount floor; old chance 0.5, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=49282 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Spectral Tiger; mount floor; old chance 1, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=49283 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Swift Spectral Tiger; mount floor; old chance 0.5, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=49284 AND (lootmode+0)=0 AND mincountOrRef=1;
-- X-51 Nether-Rocket; mount floor; old chance 1, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=49285 AND (lootmode+0)=0 AND mincountOrRef=1;
-- X-51 Nether-Rocket X-TREME; mount floor; old chance 0.5, group 1
UPDATE spell_loot_template SET ChanceOrQuestChance=10, groupid=0 WHERE entry=85325 AND item=49286 AND (lootmode+0)=0 AND mincountOrRef=1;
-- Reins of the Slate Primordial Direhorn; reference gate: every pool mount >=10% per source roll; old chance 1.5, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=30.3031, groupid=0 WHERE entry=69768 AND item=94229 AND (lootmode+0)=0 AND mincountOrRef=-94229;
-- Reins of the Slate Primordial Direhorn; reference gate: every pool mount >=10% per source roll; old chance 5, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=30.3031, groupid=0 WHERE entry=69769 AND item=94229 AND (lootmode+0)=0 AND mincountOrRef=-94229;
-- Reins of the Slate Primordial Direhorn; reference gate: every pool mount >=10% per source roll; old chance 5, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=30.3031, groupid=0 WHERE entry=69841 AND item=94229 AND (lootmode+0)=0 AND mincountOrRef=-94229;
-- Reins of the Slate Primordial Direhorn; reference gate: every pool mount >=10% per source roll; old chance 5, group 0
UPDATE creature_loot_template SET ChanceOrQuestChance=30.3031, groupid=0 WHERE entry=69842 AND item=94229 AND (lootmode+0)=0 AND mincountOrRef=-94229;
-- mount reference 150011; reference gate: every pool mount >=10% per source roll; old chance 0.7, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=60.0, groupid=0 WHERE entry=32544 AND item=150011 AND (lootmode+0)=1 AND mincountOrRef=-150011;
-- mount reference 150011; reference gate: every pool mount >=10% per source roll; old chance 1.5, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=60.0, groupid=0 WHERE entry=32545 AND item=150011 AND (lootmode+0)=1 AND mincountOrRef=-150011;
-- Reins of the Green Proto-Drake; reference gate: every pool mount >=10% per source roll; old chance 1, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=70.0, groupid=0 WHERE entry=90818 AND item=44707 AND (lootmode+0)=0 AND mincountOrRef=-44707;
-- Reins of the Green Proto-Drake; reference gate: every pool mount >=10% per source roll; old chance 2, group 0
UPDATE item_loot_template SET ChanceOrQuestChance=70.0, groupid=0 WHERE entry=104260 AND item=44707 AND (lootmode+0)=0 AND mincountOrRef=-44707;
COMMIT;

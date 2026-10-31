-- T-045: 7% DPS-weighted stat-budget target above matching heirlooms.
-- Generated against v1.0.4. Absolute values: safe to reapply.
-- Stamina weight .25; primary/hit/expertise/mastery 1; crit/haste 1.25; attack power .5.
-- Stamina is never increased. Missing matching-heirloom offense stats use empty slots; no proc/socket/requirements changes.
-- Weapon DPS floor: 1.07 * same-subclass matching heirloom at first selected endpoint.
-- See docs/twinks.md for limitations and per-item audit.
START TRANSACTION;
-- 19: Blackrock Champion's Axe; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_type2=32, stat_value2=7, stat_unk1_2=7778, ItemLevel=25, stat_unk1_1=8889 WHERE entry=1455;
-- 19: Loch Modan Musket; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_value1=6, stat_unk1_1=12000, stat_type4=32, stat_value4=10, stat_unk1_4=20000 WHERE entry=59023;
-- 19: Legacy of Teronis; comparator 69893 Bloodsoaked Skullforge Reaver
UPDATE item_template SET stat_value1=4, stat_unk1_1=5000, ItemLevel=44, stat_unk1_2=1250, stat_unk1_3=2500 WHERE entry=56696;
-- 19: Shadowfang; comparator 42945 Venerable Dal'Rend's Sacred Charge
UPDATE item_template SET stat_type2=32, stat_value2=4, stat_unk1_2=4445, ItemLevel=38, stat_unk1_1=6667 WHERE entry=1482;
-- 19: Twisted Chanter's Staff; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_type3=32, stat_value3=7, stat_unk1_3=5385, stat_type4=45, stat_value4=42, stat_unk1_4=32308 WHERE entry=890;
-- 19: Staff of Westfall; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_value2=8, stat_unk1_2=6154, stat_type3=32, stat_value3=7, stat_unk1_3=5385, stat_type4=45, stat_value4=42, stat_unk1_4=32308 WHERE entry=2042;
-- 19: Filthy Paw; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_value1=4, stat_unk1_1=8000, stat_type2=36, stat_value2=3, stat_unk1_2=6000, stat_type3=31, stat_value3=3, stat_unk1_3=6000, ItemLevel=31 WHERE entry=58978;
-- 19: Dissector; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_type3=32, stat_value3=4, stat_unk1_3=4445, stat_type4=31, stat_value4=2, stat_unk1_4=2223, ItemLevel=38, stat_unk1_1=5556, stat_unk1_2=3334 WHERE entry=56679;
-- 19: Scout's Blade; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_type3=32, stat_value3=4, stat_unk1_3=4445, stat_type4=31, stat_value4=2, stat_unk1_4=2223, ItemLevel=38, stat_unk1_1=4445, stat_unk1_2=2223 WHERE entry=20441;
-- 19: Signet of Argas; comparator 62036 Ornate Band of Accuria
UPDATE item_template SET stat_type3=32, stat_value3=3, stat_unk1_3=4286, stat_type4=36, stat_value4=3, stat_unk1_4=4286 WHERE entry=24118;
-- 19: Faerie Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_type3=32, stat_value3=6, stat_unk1_3=6667 WHERE entry=5820;
-- 19: Ghostly Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value1=6, stat_unk1_1=7500, stat_type3=32, stat_value3=6, stat_unk1_3=7500 WHERE entry=3324;
-- 19: Sacred Burial Trousers; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value3=7, stat_unk1_3=5834, stat_type4=32, stat_value4=5, stat_unk1_4=4167, stat_type5=36, stat_value5=5, stat_unk1_5=4167 WHERE entry=6282;
-- 19: Inferno Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_type2=32, stat_value2=7, stat_unk1_2=4375 WHERE entry=2231;
-- 19: Efflorescent Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value1=7, stat_unk1_1=7000 WHERE entry=7334;
-- 19: Rugged Spaulders; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type3=38, stat_value3=10, stat_unk1_3=12500, stat_type4=31, stat_value4=4, stat_unk1_4=5000 WHERE entry=5254;
-- 19: Jerkin of the Untamed Spirit; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_type3=31, stat_value3=7, stat_unk1_3=1429 WHERE entry=28051;
-- 19: Cleansed Pauldrons; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_value1=6, stat_unk1_1=7500, stat_type4=32, stat_value4=6, stat_unk1_4=7500 WHERE entry=65985;
-- 19: Deadskull Shield; comparator 93902 Flamescarred Draconian Deflector
UPDATE item_template SET stat_value1=4, stat_unk1_1=5715 WHERE entry=3761;
-- 29: Nail Spitter; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_value2=14, stat_unk1_2=12728 WHERE entry=17042;
-- 29: Legionnaire's Sword; comparator 69893 Bloodsoaked Skullforge Reaver
UPDATE item_template SET ItemLevel=55, stat_unk1_1=5385, stat_unk1_2=2308 WHERE entry=19553;
-- 29: Loksey's Training Stick; comparator 79131 Burnished Warden Staff
UPDATE item_template SET stat_value3=7, stat_unk1_3=3685, stat_type4=36, stat_value4=7, stat_unk1_4=3685 WHERE entry=7710;
-- 29: Illusionary Rod; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_value3=10, stat_unk1_3=5264, stat_type4=45, stat_value4=61, stat_unk1_4=32106 WHERE entry=7713;
-- 29: Bloody Brass Knuckles; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_type3=36, stat_value3=3, stat_unk1_3=3334, ItemLevel=36, stat_unk1_1=5556, stat_unk1_2=5556 WHERE entry=7683;
-- 29: Torturing Poker; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_value1=6, stat_unk1_1=4616, stat_type2=32, stat_value2=5, stat_unk1_2=3847, stat_type3=31, stat_value3=3, stat_unk1_3=2308, ItemLevel=55 WHERE entry=7682;
-- 29: Jaina's Signet Ring; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_value1=6, stat_unk1_1=7500, stat_type3=32, stat_value3=4, stat_unk1_3=5000, stat_type4=31, stat_value4=4, stat_unk1_4=5000 WHERE entry=6757;
-- 29: Omega Orb; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type2=36, stat_value2=6, stat_unk1_2=6667 WHERE entry=7749;
-- 29: Berylline Pads; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value1=8, stat_unk1_1=5334 WHERE entry=4197;
-- 29: Mantle of Honor; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value1=8, stat_unk1_1=8000, stat_type3=32, stat_value3=8, stat_unk1_3=8000 WHERE entry=3560;
-- 29: Resilient Tunic; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value2=10, stat_unk1_2=7693, stat_type3=32, stat_value3=10, stat_unk1_3=7693 WHERE entry=14398;
-- 29: Zealot's Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_type4=32, stat_value4=10, stat_unk1_4=3847 WHERE entry=17043;
-- 29: Spirit Hunter Headdress; comparator 61937 Stained Shadowcraft Cap
UPDATE item_template SET stat_type3=32, stat_value3=7, stat_unk1_3=4667 WHERE entry=6720;
-- 29: Defiant Spire Shoulderguard; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type4=38, stat_value4=14, stat_unk1_4=10000 WHERE entry=55910;
-- 29: Vile Protector; comparator 93902 Flamescarred Draconian Deflector
UPDATE item_template SET stat_value1=6, stat_unk1_1=6667 WHERE entry=7747;
-- 39: Bonebiter; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_type3=32, stat_value3=14, stat_unk1_3=5600 WHERE entry=6830;
-- 39: Fras Siabi's Cigar Cutter; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_value1=14, stat_unk1_1=5600, stat_type4=32, stat_value4=14, stat_unk1_4=5600 WHERE entry=65973;
-- 39: Outrider's Bow; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_value2=14, stat_unk1_2=5834, stat_type3=32, stat_value3=14, stat_unk1_3=5834, stat_type4=31, stat_value4=14, stat_unk1_4=5834 WHERE entry=19560;
-- 39: Outrunner's Bow; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_value2=14, stat_unk1_2=5834, stat_type3=32, stat_value3=14, stat_unk1_3=5834, stat_type4=31, stat_value4=14, stat_unk1_4=5834 WHERE entry=19564;
-- 39: Lakota's Mace; comparator 48716 Venerable Mass of McGowan
UPDATE item_template SET stat_value1=6, stat_unk1_1=4000, stat_type4=36, stat_value4=4, stat_unk1_4=2667, stat_value3=4, stat_unk1_3=2667, ItemLevel=80, stat_unk1_2=3334 WHERE entry=63656;
-- 39: Nimboya's Mystical Staff; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_value3=15, stat_unk1_3=5770, stat_type4=32, stat_value4=12, stat_unk1_4=4616, stat_type5=45, stat_value5=81, stat_unk1_5=31154 WHERE entry=4134;
-- 39: Staff of Athen'a; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_value1=15, stat_unk1_1=6000, stat_type4=32, stat_value4=12, stat_unk1_4=4800, stat_type5=45, stat_value5=81, stat_unk1_5=32400 WHERE entry=65994;
-- 39: Initiate's Handwraps; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_value1=6, stat_unk1_1=6667, stat_value4=4, stat_unk1_4=4445, stat_type5=31, stat_value5=4, stat_unk1_5=4445, ItemLevel=52, stat_unk1_2=5556, stat_unk1_3=2223 WHERE entry=89570;
-- 39: Jademoon Orb; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_value2=8, stat_unk1_2=8000, stat_type4=36, stat_value4=8, stat_unk1_4=8000 WHERE entry=11859;
-- 39: Mistscape Wizard Hat; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_value2=14, stat_unk1_2=7778, stat_type4=32, stat_value4=9, stat_unk1_4=5000, stat_type5=36, stat_value5=9, stat_unk1_5=5000 WHERE entry=6429;
-- 39: Corpseburner's Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value1=10, stat_unk1_1=7143, stat_value3=10, stat_unk1_3=7143 WHERE entry=62999;
-- 39: Silksand Tunic; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_type3=32, stat_value3=14, stat_unk1_3=7778 WHERE entry=14417;
-- 39: Rainstrider Leggings; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_type4=32, stat_value4=9, stat_unk1_4=2904, stat_type5=36, stat_value5=9, stat_unk1_5=2904 WHERE entry=11123;
-- 39: Everglowing Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=14, stat_unk1_3=6667, stat_type4=32, stat_value4=14, stat_unk1_4=6667 WHERE entry=19129;
-- 39: Zorbin's Water Resistant Hat; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_type4=32, stat_value4=9, stat_unk1_4=4500 WHERE entry=19039;
-- 39: Withering Mask; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_value1=14, stat_unk1_1=7369, stat_type4=32, stat_value4=9, stat_unk1_4=4737 WHERE entry=63664;
-- 39: Failed Flying Experiment; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type3=38, stat_value3=20, stat_unk1_3=13334, stat_type4=31, stat_value4=7, stat_unk1_4=4667 WHERE entry=9647;
-- 39: Loreskin Shoulders; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type3=38, stat_value3=20, stat_unk1_3=14286 WHERE entry=11502;
-- 39: Leggings of the Thousand Needles; comparator 62027 Preened Wildfeather Leggings
UPDATE item_template SET stat_value1=14, stat_unk1_1=5385, stat_type4=32, stat_value4=9, stat_unk1_4=3462 WHERE entry=63668;
-- 39: Leggings of the Thousand Needles; comparator 62027 Preened Wildfeather Leggings
UPDATE item_template SET stat_value1=14, stat_unk1_1=5385, stat_type4=32, stat_value4=9, stat_unk1_4=3462 WHERE entry=63674;
-- 39: Helm of Exile; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_value3=14, stat_unk1_3=4517, stat_type4=36, stat_value4=9, stat_unk1_4=2904 WHERE entry=11124;
-- 39: Pauldrons of the Thousand Needles; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_type4=32, stat_value4=10, stat_unk1_4=5264 WHERE entry=63671;
-- 39: Pauldrons of the Thousand Needles; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_type4=32, stat_value4=10, stat_unk1_4=5264 WHERE entry=63677;
-- 39: Chestguard of the Thousand Needles; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_value1=14, stat_unk1_1=5385, stat_value3=14, stat_unk1_3=5385 WHERE entry=63670;
-- 39: Legwraps of the Slain; comparator 62025 Mystical Kilt of Elements
UPDATE item_template SET stat_value1=14, stat_unk1_1=7369, stat_type4=32, stat_value4=9, stat_unk1_4=4737 WHERE entry=63017;
-- 39: Dirty Work Greaves; comparator 62025 Mystical Kilt of Elements
UPDATE item_template SET stat_value1=14, stat_unk1_1=7369, stat_type4=32, stat_value4=9, stat_unk1_4=4737 WHERE entry=63665;
-- 39: Helm of the Thousand Needles; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_value3=14, stat_unk1_3=5385, stat_type4=32, stat_value4=9, stat_unk1_4=3462 WHERE entry=63672;
-- 39: Helm of the Thousand Needles; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_value3=14, stat_unk1_3=5385, stat_type4=32, stat_value4=9, stat_unk1_4=3462 WHERE entry=63678;
-- 39: Vigorsteel Spaulders; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_value2=13, stat_unk1_2=7223, stat_type5=32, stat_value5=9, stat_unk1_5=5000 WHERE entry=88357;
-- 39: Legguards of Disfavor; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_value1=14, stat_unk1_1=7778, stat_value3=9, stat_unk1_3=5000, stat_type4=37, stat_value4=9, stat_unk1_4=5000 WHERE entry=63636;
-- 39: Legguards of Disfavor; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_value1=14, stat_unk1_1=7778, stat_value3=9, stat_unk1_3=5000, stat_type4=37, stat_value4=9, stat_unk1_4=5000 WHERE entry=63639;
-- 39: Well Hammered Shield; comparator 93903 Weathered Observer's Shield
UPDATE item_template SET stat_value1=8, stat_unk1_1=8000 WHERE entry=63657;
-- 49: Beastkiller; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_value1=18, stat_unk1_1=7827, stat_value3=18, stat_unk1_3=7827 WHERE entry=53584;
-- 49: Will of the Mountain Giant; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_type3=32, stat_value3=18, stat_unk1_3=6207 WHERE entry=10652;
-- 49: Screeching Bow; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_value1=18, stat_unk1_1=6000, stat_type4=32, stat_value4=18, stat_unk1_4=6000, stat_type5=31, stat_value5=18, stat_unk1_5=6000 WHERE entry=18729;
-- 49: Houndmaster's Rifle; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_type3=32, stat_value3=24, stat_unk1_3=18462 WHERE entry=11629;
-- 49: Confiscated Poacher's Gun; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_value1=13, stat_unk1_1=14445, stat_type4=32, stat_value4=24, stat_unk1_4=26667 WHERE entry=53569;
-- 49: Tidecrest Blade; comparator 69893 Bloodsoaked Skullforge Reaver
UPDATE item_template SET ItemLevel=97, stat_unk1_1=4211, stat_unk1_2=2632 WHERE entry=15705;
-- 49: Glowing Brightwood Staff; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_type5=32, stat_value5=15, stat_unk1_5=3750, stat_type6=45, stat_value6=100, stat_unk1_6=25000 WHERE entry=812;
-- 49: Hunt Tracker Blade; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_value2=9, stat_unk1_2=4737, stat_type3=32, stat_value3=7, stat_unk1_3=3685, ItemLevel=97, stat_unk1_1=2632 WHERE entry=15706;
-- 49: Lagrave's Seal; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_type4=32, stat_value4=6, stat_unk1_4=3334 WHERE entry=12038;
-- 49: Thrall's Resolve; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type4=31, stat_value4=6, stat_unk1_4=3334 WHERE entry=12544;
-- 49: Spirit of Aquementas; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type2=36, stat_value2=9, stat_unk1_2=6429 WHERE entry=11904;
-- 49: Eye of Flame; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=11, stat_unk1_5=2683, stat_type6=36, stat_value6=11, stat_unk1_6=2683 WHERE entry=3075;
-- 49: Gemburst Circlet; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_value3=18, stat_unk1_3=5807, stat_type4=32, stat_value4=11, stat_unk1_4=3549, stat_type5=36, stat_value5=11, stat_unk1_5=3549 WHERE entry=10751;
-- 49: Soulstealer Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value1=13, stat_unk1_1=5653, stat_type4=32, stat_value4=13, stat_unk1_4=5653 WHERE entry=13374;
-- 49: Imperial Red Pants; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value2=18, stat_unk1_2=7827, stat_type3=32, stat_value3=11, stat_unk1_3=4783, stat_type4=36, stat_value4=11, stat_unk1_4=4783 WHERE entry=8251;
-- 49: Royal Highmark Vestments; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=18, stat_unk1_3=5807, stat_type4=32, stat_value4=18, stat_unk1_4=5807 WHERE entry=9649;
-- 49: Serpentskin Helm; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_value1=18, stat_unk1_1=7827, stat_type3=32, stat_value3=11, stat_unk1_3=4783 WHERE entry=8261;
-- 49: Penance Spaulders; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type4=38, stat_value4=24, stat_unk1_4=10000 WHERE entry=11963;
-- 49: Clouddrift Mantle; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type3=38, stat_value3=24, stat_unk1_3=13334, stat_type4=31, stat_value4=8, stat_unk1_4=4445 WHERE entry=11874;
-- 49: Windshear Leggings; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type4=32, stat_value4=11, stat_unk1_4=3438, stat_type5=36, stat_value5=11, stat_unk1_5=3438 WHERE entry=12041;
-- 49: Gruffscale Leggings; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_value1=18, stat_unk1_1=7500, stat_type3=32, stat_value3=11, stat_unk1_3=4584, stat_type4=36, stat_value4=11, stat_unk1_4=4584 WHERE entry=45052;
-- 49: Flamestrider Robes; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value3=18, stat_unk1_3=6000 WHERE entry=11747;
-- 49: Conservator Helm; comparator 61936 Mystical Coif of Elements
UPDATE item_template SET stat_value1=18, stat_unk1_1=5625, stat_type4=32, stat_value4=11, stat_unk1_4=3438 WHERE entry=12018;
-- 49: Windshrieker Pauldrons; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_value1=13, stat_unk1_1=5653, stat_type4=32, stat_value4=13, stat_unk1_4=5653 WHERE entry=13538;
-- 49: Vest of the Den Watcher; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_type3=32, stat_value3=18, stat_unk1_3=7200 WHERE entry=21320;
-- 49: Leggings of the Ursa; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_value1=18, stat_unk1_1=7500, stat_type3=37, stat_value3=11, stat_unk1_3=4584 WHERE entry=21316;
-- 59: Blade of the Unyielding; comparator 42945 Venerable Dal'Rend's Sacred Charge
UPDATE item_template SET stat_value1=11, stat_unk1_1=4231, stat_type4=32, stat_value4=11, stat_unk1_4=4231, ItemLevel=121, stat_unk1_2=6539, stat_unk1_3=2693 WHERE entry=29108;
-- 59: Warden's Ring of Precision; comparator 62036 Ornate Band of Accuria
UPDATE item_template SET stat_value2=15, stat_unk1_2=6250, stat_type4=32, stat_value4=10, stat_unk1_4=4167, stat_type5=36, stat_value5=10, stat_unk1_5=4167 WHERE entry=27733;
-- 59: Expedition Caster's Band; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_value3=15, stat_unk1_3=6250, stat_value2=10, stat_unk1_2=4167, stat_type5=36, stat_value5=10, stat_unk1_5=4167 WHERE entry=27734;
-- 59: Demoniac Soul Prison; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type4=36, stat_value4=15, stat_unk1_4=6819 WHERE entry=31715;
-- 59: Mantle of Magical Might; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value4=20, stat_unk1_4=5406, stat_value2=20, stat_unk1_2=5406 WHERE entry=25718;
-- 59: Shadowcast Tunic; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value1=26, stat_unk1_1=5099 WHERE entry=31717;
-- 59: Deadly Borer Leggings; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value4=26, stat_unk1_4=5099, stat_type5=36, stat_value5=18, stat_unk1_5=3530 WHERE entry=25711;
-- 59: Watcher's Cloak of Vigilance; comparator 62038 Worn Stoneskin Gargoyle Cape
UPDATE item_template SET stat_value1=15, stat_unk1_1=6250, stat_type4=32, stat_value4=10, stat_unk1_4=4167, stat_type5=36, stat_value5=10, stat_unk1_5=4167 WHERE entry=27730;
-- 59: Zandalar Demoniac's Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=26, stat_unk1_3=5532, stat_type4=32, stat_value4=26, stat_unk1_4=5532 WHERE entry=20033;
-- 59: Zandalar Illusionist's Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=26, stat_unk1_3=5532, stat_type4=32, stat_value4=26, stat_unk1_4=5532 WHERE entry=20034;
-- 59: Moonkin Headdress; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_value4=26, stat_unk1_4=5099 WHERE entry=25710;
-- 59: Wild Shoulderpads; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type4=38, stat_value4=37, stat_unk1_4=11563, stat_value1=19, stat_unk1_1=5938 WHERE entry=27724;
-- 59: Darkstorm Tunic; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_type3=31, stat_value3=26, stat_unk1_3=5099 WHERE entry=31718;
-- 59: Stormstrike Vest; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_type4=32, stat_value4=26, stat_unk1_4=5099 WHERE entry=31719;
-- 59: Scaled Legs of Ruination; comparator 62025 Mystical Kilt of Elements
UPDATE item_template SET stat_value3=26, stat_unk1_3=5099 WHERE entry=25702;
-- 59: Breastplate of Retribution; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_value2=30, stat_unk1_2=5883, stat_value3=26, stat_unk1_3=5099 WHERE entry=25701;
-- 59: Flintlocke's Piloting Pants; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_value1=26, stat_unk1_1=6500, stat_type4=32, stat_value4=18, stat_unk1_4=4500, stat_type5=37, stat_value5=18, stat_unk1_5=4500 WHERE entry=25979;
-- 60: Mag'har Bow; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_type3=32, stat_value3=27, stat_unk1_3=6280, stat_type4=31, stat_value4=27, stat_unk1_4=6280 WHERE entry=25496;
-- 60: Blade of Misfortune; comparator 44092 Reforged Truesilver Champion
UPDATE item_template SET stat_value1=38, stat_unk1_1=7756 WHERE entry=31134;
-- 60: Handwraps of Meditation; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_value1=12, stat_unk1_1=6000, stat_value4=8, stat_unk1_4=4000, stat_type5=31, stat_value5=8, stat_unk1_5=4000, ItemLevel=101, stat_unk1_2=7000, stat_unk1_3=3000 WHERE entry=89573;
-- 60: Amaan's Signet; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type4=32, stat_value4=10, stat_unk1_4=4167, stat_value3=10, stat_unk1_3=4167 WHERE entry=25485;
-- 60: Signet of Aeranas; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_value2=15, stat_unk1_2=6250, stat_type4=32, stat_value4=10, stat_unk1_4=4167, stat_type5=36, stat_value5=10, stat_unk1_5=4167 WHERE entry=25488;
-- 60: Ancient Crystal Talisman; comparator 42992 Discerning Eye of the Beast
UPDATE item_template SET stat_value1=26, stat_unk1_1=7879 WHERE entry=25620;
-- 60: Mirren's Drinking Hat; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type4=32, stat_value4=19, stat_unk1_4=4635, stat_type5=36, stat_value5=19, stat_unk1_5=4635 WHERE entry=23587;
-- 60: Researcher's Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value3=21, stat_unk1_3=6775, stat_type4=32, stat_value4=21, stat_unk1_4=6775 WHERE entry=31659;
-- 60: Chemise of Rebirth; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=27, stat_unk1_3=6586, stat_type4=32, stat_value4=27, stat_unk1_4=6586 WHERE entry=31657;
-- 60: Frostfire Leggings; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value1=19, stat_unk1_1=3016, stat_type5=36, stat_value5=19, stat_unk1_5=3016 WHERE entry=22497;
-- 60: Windtalker's Cloak; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_value3=15, stat_unk1_3=6250, stat_type4=32, stat_value4=10, stat_unk1_4=4167, stat_type5=36, stat_value5=10, stat_unk1_5=4167 WHERE entry=25489;
-- 60: Ceremonial Robes; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=27, stat_unk1_3=6280, stat_type4=32, stat_value4=27, stat_unk1_4=6280 WHERE entry=25510;
-- 60: Frostfire Robe; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value4=27, stat_unk1_4=4091 WHERE entry=22496;
-- 60: Clefthoof Hide Mask; comparator 61937 Stained Shadowcraft Cap
UPDATE item_template SET stat_value3=27, stat_unk1_3=6280, stat_type4=31, stat_value4=19, stat_unk1_4=4419 WHERE entry=25513;
-- 60: Omenai Vest; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value2=27, stat_unk1_2=6280, stat_type4=32, stat_value4=27, stat_unk1_4=6280 WHERE entry=25508;
-- 60: Bonescythe Breastplate; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_value4=27, stat_unk1_4=4091 WHERE entry=22476;
-- 60: Leggings of Telhamat; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_value3=27, stat_unk1_3=6280, stat_type4=36, stat_value4=19, stat_unk1_4=4419 WHERE entry=25507;
-- 60: Explorer's Leggings; comparator 62027 Preened Wildfeather Leggings
UPDATE item_template SET stat_value3=27, stat_unk1_3=6280, stat_type4=32, stat_value4=19, stat_unk1_4=4419 WHERE entry=25518;
-- 60: Cenarion Vestments; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value3=27, stat_unk1_3=5625, stat_type5=32, stat_value5=27, stat_unk1_5=5625 WHERE entry=16833;
-- 60: Vindicator's Chain Helm; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_value1=27, stat_unk1_1=6280, stat_type3=36, stat_value3=19, stat_unk1_3=4419, stat_type4=31, stat_value4=19, stat_unk1_4=4419 WHERE entry=25506;
-- 60: Blacksting Shoulders; comparator 42950 Champion Herod's Shoulder
UPDATE item_template SET stat_value2=21, stat_unk1_2=6775, stat_value1=21, stat_unk1_1=6775 WHERE entry=31786;
-- 60: Feralfen Beastmaster's Hauberk; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_value1=27, stat_unk1_1=5745, stat_value3=27, stat_unk1_3=5745 WHERE entry=25614;
-- 60: Tribal Hauberk; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_value1=27, stat_unk1_1=6280, stat_type4=32, stat_value4=27, stat_unk1_4=6280 WHERE entry=25512;
-- 60: Expedition Forager Leggings; comparator 62024 Tarnished Leggings of Destruction
UPDATE item_template SET stat_value3=27, stat_unk1_3=6586, stat_value1=19, stat_unk1_1=4635, stat_type4=36, stat_value4=19, stat_unk1_4=4635 WHERE entry=27717;
-- 60: Earthfury Vestments; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_value4=27, stat_unk1_4=5625, stat_value3=27, stat_unk1_3=5625 WHERE entry=16841;
-- 60: Circle's Stalwart Helmet; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_value1=27, stat_unk1_1=6586, stat_value3=19, stat_unk1_3=4635, stat_type4=31, stat_value4=19, stat_unk1_4=4635 WHERE entry=27715;
-- 60: Marshstrider's Spaulders; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_value1=26, stat_unk1_1=8125, stat_type3=32, stat_value3=18, stat_unk1_3=5625 WHERE entry=25522;
-- 60: Dreadnaught Breastplate; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_value1=32, stat_unk1_1=4849, stat_type5=32, stat_value5=27, stat_unk1_5=4091 WHERE entry=22416;
-- 69: Axe of Frozen Death; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_value1=49, stat_unk1_1=5904, stat_value4=49, stat_unk1_4=5904 WHERE entry=38237;
-- 69: Warsong Longbow; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_value1=49, stat_unk1_1=6806, stat_type3=32, stat_value3=49, stat_unk1_3=6806 WHERE entry=36882;
-- 69: Fury of the Raging Dragon; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_type4=32, stat_value4=68, stat_unk1_4=19429 WHERE entry=38241;
-- 69: Borean Smasher; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_value1=21, stat_unk1_1=5250, stat_type4=36, stat_value4=13, stat_unk1_4=3250, stat_type5=31, stat_value5=13, stat_unk1_5=3250, ItemLevel=151, stat_unk1_2=8500, stat_unk1_3=4250 WHERE entry=38238;
-- 69: Fang of the Desolate Soul; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_value1=26, stat_unk1_1=5653, stat_value2=21, stat_unk1_2=4566, stat_type4=31, stat_value4=12, stat_unk1_4=2609, ItemLevel=165, stat_unk1_3=7609 WHERE entry=38242;
-- 69: Ring of Decimation; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_type4=32, stat_value4=18, stat_unk1_4=3334, stat_type5=36, stat_value5=18, stat_unk1_5=3334 WHERE entry=38219;
-- 69: Tome of Alacrity; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_value2=27, stat_unk1_2=6750 WHERE entry=35858;
-- 69: Benevolent Hood; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=32, stat_value4=33, stat_unk1_4=4584, stat_type5=36, stat_value5=33, stat_unk1_5=4584 WHERE entry=35814;
-- 69: Shock-Resistant Hood; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=36, stat_value4=33, stat_unk1_4=4584 WHERE entry=35844;
-- 69: Shoulders of the Northern Lights; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value2=36, stat_unk1_2=5000 WHERE entry=43181;
-- 69: Stretch-Hide Spaulders; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value2=36, stat_unk1_2=6793, stat_type3=32, stat_value3=36, stat_unk1_3=6793 WHERE entry=35818;
-- 69: Fire-Purifying Tunic; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_value1=49, stat_unk1_1=6806 WHERE entry=35859;
-- 69: Earth-Infused Leggings; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_value2=33, stat_unk1_2=4584, stat_type4=36, stat_value4=33, stat_unk1_4=4584 WHERE entry=35863;
-- 69: Regal Pantaloons; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=32, stat_value4=33, stat_unk1_4=4584, stat_value2=33, stat_unk1_2=4584 WHERE entry=35884;
-- 69: Mantle of Keristrasza; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_type4=32, stat_value4=18, stat_unk1_4=3215, stat_type5=36, stat_value5=18, stat_unk1_5=3215 WHERE entry=38222;
-- 69: Indigo Robe of Replenishment; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=32, stat_value4=49, stat_unk1_4=6806 WHERE entry=35871;
-- 69: Imperious Worghide Cap; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806 WHERE entry=35833;
-- 69: Fizznik's Patented Earwarmer; comparator 61937 Stained Shadowcraft Cap
UPDATE item_template SET stat_value2=33, stat_unk1_2=4584, stat_type4=31, stat_value4=33, stat_unk1_4=4584 WHERE entry=35864;
-- 69: Cured Mammoth Hide Mantle; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type4=38, stat_value4=67, stat_unk1_4=9306, stat_type5=31, stat_value5=22, stat_unk1_5=3056 WHERE entry=43182;
-- 69: Worgskin Shoulders; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type3=38, stat_value3=67, stat_unk1_3=12642, stat_value2=34, stat_unk1_2=6416, stat_type4=31, stat_value4=22, stat_unk1_4=4151 WHERE entry=35877;
-- 69: Bone-Threaded Harness; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type2=31, stat_value2=49, stat_unk1_2=6806 WHERE entry=35815;
-- 69: Tribal Chestguard; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806 WHERE entry=35885;
-- 69: Grounded Pants; comparator 62027 Preened Wildfeather Leggings
UPDATE item_template SET stat_value2=33, stat_unk1_2=4584 WHERE entry=35868;
-- 69: Iron-Studded Leggings; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=36, stat_value4=33, stat_unk1_4=4584 WHERE entry=35872;
-- 69: Robes of Summer Flame; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value3=49, stat_unk1_3=7778, stat_type4=32, stat_value4=49, stat_unk1_4=7778 WHERE entry=34796;
-- 69: Hex-Linked Stronghelm; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=31, stat_value4=33, stat_unk1_4=4584 WHERE entry=35869;
-- 69: Magispike Helm; comparator 61936 Mystical Coif of Elements
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806, stat_type4=32, stat_value4=33, stat_unk1_4=4584 WHERE entry=35882;
-- 69: Tundra Tracker's Shoulderguards; comparator 42950 Champion Herod's Shoulder
UPDATE item_template SET stat_type4=32, stat_value4=36, stat_unk1_4=5000 WHERE entry=43183;
-- 69: Magdun Spaulders; comparator 42950 Champion Herod's Shoulder
UPDATE item_template SET stat_value3=36, stat_unk1_3=6793 WHERE entry=35890;
-- 69: Azure Chain Hauberk; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_type3=32, stat_value3=49, stat_unk1_3=6806 WHERE entry=35842;
-- 69: Reinforced Tuskhide Hauberk; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_value3=49, stat_unk1_3=6806 WHERE entry=35898;
-- 69: Dusk-Linked Leggings; comparator 62024 Tarnished Leggings of Destruction
UPDATE item_template SET stat_value1=33, stat_unk1_1=4584, stat_type4=36, stat_value4=33, stat_unk1_4=4584 WHERE entry=35878;
-- 69: Runeplate Helm; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_value2=49, stat_unk1_2=6806, stat_type4=32, stat_value4=33, stat_unk1_4=4584, stat_type5=31, stat_value5=33, stat_unk1_5=4584 WHERE entry=35891;
-- 69: Tundra Pauldrons; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_value2=45, stat_unk1_2=6250, stat_type5=32, stat_value5=30, stat_unk1_5=4167 WHERE entry=43184;
-- 69: Ramshorn-Inlaid Shoulders; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_value1=45, stat_unk1_1=8491, stat_type4=32, stat_value4=30, stat_unk1_4=5661 WHERE entry=35843;
-- 69: Scavenged Tirasian Plate; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_value1=56, stat_unk1_1=7778 WHERE entry=35822;
-- 69: Antique Reinforced Legguards; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type4=32, stat_value4=33, stat_unk1_4=4584, stat_type5=37, stat_value5=33, stat_unk1_5=4584 WHERE entry=35835;
-- 69: Nerubian Inner Husk; comparator 93902 Flamescarred Draconian Deflector
UPDATE item_template SET stat_value1=27, stat_unk1_1=6750 WHERE entry=35817;
-- 69: Munificent Bulwark; comparator 93903 Weathered Observer's Shield
UPDATE item_template SET stat_value2=27, stat_unk1_2=6750 WHERE entry=35857;
-- 70: Ingvar's Monolithic Cleaver; comparator 42943 Bloodied Arcanite Reaper
UPDATE item_template SET stat_type3=32, stat_value3=51, stat_unk1_3=5258 WHERE entry=35576;
-- 70: Thori'dal, the Stars' Fury; comparator 42946 Charmed Ancient Bone Bow
UPDATE item_template SET stat_type5=31, stat_value5=51, stat_unk1_5=4215 WHERE entry=34334;
-- 70: Brutal Gladiator's Greatsword; comparator 44092 Reforged Truesilver Champion
UPDATE item_template SET stat_value2=70, stat_unk1_2=6307 WHERE entry=35015;
-- 70: Staff of Disintegration; comparator 42947 Dignified Headmaster's Charge
UPDATE item_template SET stat_type4=45, stat_value4=290, stat_unk1_4=21642 WHERE entry=30313;
-- 70: Crux of the Apocalypse; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_value2=27, stat_unk1_2=5193, stat_type4=32, stat_value4=22, stat_unk1_4=4231, stat_type5=31, stat_value5=13, stat_unk1_5=2500 WHERE entry=34329;
-- 70: Brutal Gladiator's Endgame; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type4=36, stat_value4=28, stat_unk1_4=4308 WHERE entry=35008;
-- 70: Brutal Gladiator's Grimoire; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type5=36, stat_value5=28, stat_unk1_5=4308 WHERE entry=35016;
-- 70: Cowl of Light's Purity; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=34, stat_unk1_5=2810, stat_value3=34, stat_unk1_3=2810 WHERE entry=34339;
-- 70: Dark Conjuror's Collar; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_value3=34, stat_unk1_3=2810 WHERE entry=34340;
-- 70: Shawl of Wonderment; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_type5=32, stat_value5=38, stat_unk1_5=4419 WHERE entry=34202;
-- 70: Amice of the Convoker; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_value3=38, stat_unk1_3=4419 WHERE entry=34210;
-- 70: D.E.H.T.A. Overshirt; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=51, stat_unk1_3=7084, stat_value2=51, stat_unk1_2=7084 WHERE entry=37005;
-- 70: Pantaloons of Calming Strife; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value3=34, stat_unk1_3=2932, stat_type5=36, stat_value5=34, stat_unk1_5=2932 WHERE entry=34170;
-- 70: Leggings of Calamity; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_value2=34, stat_unk1_2=2932, stat_value3=34, stat_unk1_3=2932 WHERE entry=34181;
-- 70: Cloak of Unforgivable Sin; comparator 62039 Inherited Cape of the Black Baron
UPDATE item_template SET stat_value1=28, stat_unk1_1=4118, stat_type4=32, stat_value4=19, stat_unk1_4=2795 WHERE entry=34241;
-- 70: Tattered Cape of Antonidas; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_type4=32, stat_value4=19, stat_unk1_4=2795 WHERE entry=34242;
-- 70: Fel Conquerer Raiments; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value3=51, stat_unk1_3=4397 WHERE entry=34232;
-- 70: Robes of Ghostly Hatred; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_value4=51, stat_unk1_4=4397 WHERE entry=34399;
-- 70: Duplicitous Guise; comparator 61937 Stained Shadowcraft Cap
UPDATE item_template SET stat_type5=32, stat_value5=34, stat_unk1_5=2810, stat_value2=34, stat_unk1_2=2810 WHERE entry=34244;
-- 70: Cover of Ursol the Wise; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_type4=32, stat_value4=34, stat_unk1_4=2810 WHERE entry=34245;
-- 70: Shoulderpads of Vehemence; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type5=38, stat_value5=69, stat_unk1_5=8024 WHERE entry=34195;
-- 70: Spaulders of Reclamation; comparator 42984 Preened Ironfeather Shoulders
UPDATE item_template SET stat_type5=32, stat_value5=38, stat_unk1_5=4419 WHERE entry=34209;
-- 70: Harness of Carnal Instinct; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_value3=51, stat_unk1_3=4397 WHERE entry=34211;
-- 70: Sunglow Vest; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=51, stat_unk1_5=4397 WHERE entry=34212;
-- 70: Leggings of the Immortal Night; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_value4=34, stat_unk1_4=2932, stat_type5=36, stat_value5=34, stat_unk1_5=2932 WHERE entry=34188;
-- 70: Thunderheart Tunic; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_type4=32, stat_value4=51, stat_unk1_4=4952 WHERE entry=31041;
-- 70: Thunderheart Chestguard; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_type4=31, stat_value4=51, stat_unk1_4=4952 WHERE entry=31042;
-- 70: Coif of Alleria; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_value4=34, stat_unk1_4=2810, stat_type5=31, stat_value5=34, stat_unk1_5=2810 WHERE entry=34333;
-- 70: Equilibrium Epaulets; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_type5=32, stat_value5=38, stat_unk1_5=4419 WHERE entry=34208;
-- 70: Erupting Epaulets; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_value3=38, stat_unk1_3=4419 WHERE entry=34390;
-- 70: Vicious Hawkstrider Hauberk; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_value4=51, stat_unk1_4=4397 WHERE entry=34228;
-- 70: Garments of Serene Shores; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_value2=51, stat_unk1_2=4397 WHERE entry=34229;
-- 70: Starstalker Legguards; comparator 62024 Tarnished Leggings of Destruction
UPDATE item_template SET stat_type5=36, stat_value5=34, stat_unk1_5=2932 WHERE entry=34168;
-- 70: Brutal Gladiator's Linked Armor; comparator 48677 Champion's Deathdealer Breastplate
UPDATE item_template SET stat_value2=51, stat_unk1_2=4397, stat_value3=51, stat_unk1_3=4397 WHERE entry=35042;
-- 70: Brutal Gladiator's Mail Armor; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_value5=51, stat_unk1_5=4397, stat_value2=51, stat_unk1_2=4397 WHERE entry=35048;
-- 70: Crown of Anasterian; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_type5=31, stat_value5=34, stat_unk1_5=2810 WHERE entry=34345;
-- 70: Pauldrons of Perseverance; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_value3=48, stat_unk1_3=5582, stat_type4=32, stat_value4=32, stat_unk1_4=3721 WHERE entry=34192;
-- 70: Spaulders of the Thalassian Savior; comparator 44100 Pristine Lightforge Spaulders
UPDATE item_template SET stat_value2=32, stat_unk1_2=3721 WHERE entry=34193;
-- 70: Warharness of Reckless Fury; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_value3=51, stat_unk1_3=4397 WHERE entry=34215;
-- 70: Heroic Judicator's Chestguard; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_type4=32, stat_value4=51, stat_unk1_4=4397 WHERE entry=34216;
-- 70: Felfury Legplates; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=37, stat_value5=34, stat_unk1_5=2932 WHERE entry=34180;
-- 78: Steelbender's Masterpiece; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_type5=32, stat_value5=29, stat_unk1_5=1677 WHERE entry=55272;
-- 78: Toxidunk Dagger; comparator 42944 Balanced Heartseeker
UPDATE item_template SET stat_type5=32, stat_value5=29, stat_unk1_5=1677 WHERE entry=55790;
-- 78: Entwined Nereis; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type5=32, stat_value5=25, stat_unk1_5=1112 WHERE entry=55201;
-- 78: Anthia's Ring; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_type5=32, stat_value5=25, stat_unk1_5=1112, stat_type6=36, stat_value6=25, stat_unk1_6=1112 WHERE entry=55236;
-- 78: Bioluminescent Lamp; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type5=36, stat_value5=38, stat_unk1_5=1689 WHERE entry=55259;
-- 78: Beauty's Favorite Bone; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type5=36, stat_value5=38, stat_unk1_5=1689 WHERE entry=55277;
-- 78: Aurelian Mitre; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=45, stat_unk1_5=1114, stat_type6=36, stat_value6=45, stat_unk1_6=1114 WHERE entry=55198;
-- 78: Kyrstel Mantle; comparator 42985 Tattered Dreadmist Mantle
UPDATE item_template SET stat_type5=32, stat_value5=51, stat_unk1_5=1700 WHERE entry=55786;
-- 78: Periwinkle Cloak; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_type5=32, stat_value5=25, stat_unk1_5=1112 WHERE entry=55202;
-- 78: Eagle Ray Cloak; comparator 62038 Worn Stoneskin Gargoyle Cape
UPDATE item_template SET stat_type5=32, stat_value5=25, stat_unk1_5=1112 WHERE entry=55206;
-- 78: Inquisition Robes; comparator 48691 Tattered Dreadmist Robe
UPDATE item_template SET stat_type5=32, stat_value5=68, stat_unk1_5=1684 WHERE entry=55278;
-- 78: Caridean Epaulettes; comparator 38694 Family Shoulderpads
UPDATE item_template SET stat_type5=38, stat_value5=94, stat_unk1_5=3134, stat_type6=31, stat_value6=30, stat_unk1_6=1000 WHERE entry=55204;
-- 78: Beauty's Chew Toy; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type5=32, stat_value5=45, stat_unk1_5=1114 WHERE entry=55273;
-- 78: Cosmos Vestments; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_value3=68, stat_unk1_3=5397, stat_value2=68, stat_unk1_2=5397 WHERE entry=35677;
-- 78: Renouncer's Cowl; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_type5=36, stat_value5=45, stat_unk1_5=1114 WHERE entry=55263;
-- 78: Chromis Chestpiece; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_type5=32, stat_value5=68, stat_unk1_5=1684 WHERE entry=55205;
-- 78: Anomuran Helm; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_type5=31, stat_value5=45, stat_unk1_5=1114 WHERE entry=55229;
-- 78: Harp Shell Pauldrons; comparator 44100 Pristine Lightforge Spaulders
UPDATE item_template SET stat_type5=32, stat_value5=42, stat_unk1_5=1400 WHERE entry=55207;
-- 78: Raz's Pauldrons; comparator 42949 Polished Spaulders of Valor
UPDATE item_template SET stat_type5=32, stat_value5=43, stat_unk1_5=1434 WHERE entry=55779;
-- 78: Beauty's Plate; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_type5=32, stat_value5=68, stat_unk1_5=1684 WHERE entry=55274;
-- 78: Triton Legplates; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=32, stat_value5=45, stat_unk1_5=1114 WHERE entry=55249;
-- 78: Alpheus Legguards; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=37, stat_value5=45, stat_unk1_5=1114 WHERE entry=55260;
-- 80: Penate's Penance-Procuring Pistol; comparator 44093 Upgraded Dwarven Hand Cannon
UPDATE item_template SET stat_type5=32, stat_value5=102, stat_unk1_5=2303 WHERE entry=87477;
-- 80: Ced's Crusher; comparator 48716 Venerable Mass of McGowan
UPDATE item_template SET stat_type5=36, stat_value5=21, stat_unk1_5=475 WHERE entry=87515;
-- 80: Handwraps of Serenity; comparator 92948 Brawler's Razor Claws
UPDATE item_template SET stat_type5=31, stat_value5=21, stat_unk1_5=1449 WHERE entry=89575;
-- 80: Signet of the Scorned; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type5=32, stat_value5=27, stat_unk1_5=469 WHERE entry=87442;
-- 80: Kelp Forest Vest; comparator 48689 Stained Shadowcraft Tunic
UPDATE item_template SET stat_type5=31, stat_value5=73, stat_unk1_5=1807 WHERE entry=65659;
-- 80: Acid-Eaten Vest; comparator 48687 Preened Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=73, stat_unk1_5=1807 WHERE entry=66886;
-- 80: Hexu's Amplifying Helm; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_type5=36, stat_value5=49, stat_unk1_5=474 WHERE entry=87456;
-- 80: Damselfish Shoulderguards; comparator 42951 Mystical Pauldrons of Elements
UPDATE item_template SET stat_type5=32, stat_value5=55, stat_unk1_5=2209 WHERE entry=59783;
-- 80: Leggings of the Scorched Man; comparator 62025 Mystical Kilt of Elements
UPDATE item_template SET stat_type5=32, stat_value5=49, stat_unk1_5=474 WHERE entry=87464;
-- 80: Wrathful Gladiator's Ringmail Armor; comparator 48683 Mystical Vest of Elements
UPDATE item_template SET stat_type5=32, stat_value5=73, stat_unk1_5=2240 WHERE entry=51497;
-- 80: Deathspite Breastplate; comparator 48685 Polished Breastplate of Valor
UPDATE item_template SET stat_type5=32, stat_value5=73, stat_unk1_5=706 WHERE entry=87426;
-- 84: Lhakaz's Missing Ribspreader; comparator 93857 Vengeful Heartseeker
UPDATE item_template SET stat_type5=31, stat_value5=69, stat_unk1_5=1145 WHERE entry=87508;
-- 84: Sharpened Multi-Prong Dagger; comparator 93857 Vengeful Heartseeker
UPDATE item_template SET stat_type5=31, stat_value5=69, stat_unk1_5=1447 WHERE entry=87514;
-- 84: Signet of the Binding Wall; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_type5=36, stat_value5=99, stat_unk1_5=1375 WHERE entry=87438;
-- 84: Pestilential Ring; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type5=31, stat_value5=99, stat_unk1_5=1375 WHERE entry=87439;
-- 84: Bone-Inlaid Fan; comparator 93904 Musty Tome of the Lost
UPDATE item_template SET stat_type5=36, stat_value5=148, stat_unk1_5=2056 WHERE entry=87458;
-- 84: Hood of Alchemical Vapors; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=177, stat_unk1_5=1259 WHERE entry=87450;
-- 84: Spaulders of Five Rings; comparator 93859 Bloodstained Dreadmist Mantle
UPDATE item_template SET stat_type5=32, stat_value5=197, stat_unk1_5=2380 WHERE entry=87491;
-- 84: Very Manly Leggings; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_type5=32, stat_value5=177, stat_unk1_5=1475 WHERE entry=87463;
-- 84: Drape of Flowing Gauze; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_type5=36, stat_value5=99, stat_unk1_5=1263 WHERE entry=87429;
-- 84: Enokian Growth Shoulderguards; comparator 93864 Majestic Ironfeather Shoulders
UPDATE item_template SET stat_type5=32, stat_value5=197, stat_unk1_5=2209 WHERE entry=87493;
-- 84: Mantle of Bestial Force; comparator 93862 Supple Shadowcraft Spaulders
UPDATE item_template SET stat_type5=31, stat_value5=155, stat_unk1_5=1872 WHERE entry=87492;
-- 84: Chestguard of the Solicitious Pandaren; comparator 93865 Majestic Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=266, stat_unk1_5=1892 WHERE entry=87425;
-- 84: Leggings of Perfect Balance; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type5=32, stat_value5=177, stat_unk1_5=1475 WHERE entry=87462;
-- 84: Dills' Primal Leggings; comparator 62024 Tarnished Leggings of Destruction
UPDATE item_template SET stat_type5=36, stat_value5=177, stat_unk1_5=1259 WHERE entry=87459;
-- 84: Endwyn's Toothy Visage; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_type5=32, stat_value5=177, stat_unk1_5=1369 WHERE entry=87453;
-- 84: Greatheart Pants; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=32, stat_value5=177, stat_unk1_5=1475 WHERE entry=87461;
-- 84: Legplates of the Wandering Warrior; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=37, stat_value5=177, stat_unk1_5=1589 WHERE entry=87460;
-- 85: Ennadee's Twirling Longbow; comparator 93855 War-Torn Ancient Bone Bow
UPDATE item_template SET stat_type5=31, stat_value5=286, stat_unk1_5=1888 WHERE entry=87474;
-- 85: Vishanka, Jaws of the Earth; comparator 93855 War-Torn Ancient Bone Bow
UPDATE item_template SET stat_type3=32, stat_value3=286, stat_unk1_3=2591, stat_type4=31, stat_value4=286, stat_unk1_4=2591 WHERE entry=78471;
-- 85: Cataclysmic Gladiator's Rifle; comparator 93841 Smoothbore Dwarven Hand Cannon
UPDATE item_template SET stat_value3=395, stat_unk1_3=8837 WHERE entry=73427;
-- 85: Kaleiki's Lost Training Staff; comparator 93844 Refinished Warden Staff
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1261 WHERE entry=87419;
-- 85: Fist of Fate; comparator 93858 Brawler's Bladed Claws
UPDATE item_template SET stat_type5=36, stat_value5=82, stat_unk1_5=1264, stat_type6=31, stat_value6=82, stat_unk1_6=1264 WHERE entry=87509;
-- 85: Anaxo's Multi-Layered Seal; comparator 62037 Gleaming Seal of the Archmagus
UPDATE item_template SET stat_type5=32, stat_value5=106, stat_unk1_5=1256 WHERE entry=87437;
-- 85: Beachcomber's Hat; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1667 WHERE entry=87604;
-- 85: Corsair's Overshirt; comparator 93860 Bloodstained Dreadmist Robe
UPDATE item_template SET stat_value1=286, stat_unk1_1=5670, stat_value3=286, stat_unk1_3=5670 WHERE entry=65174;
-- 85: Legwraps of Dying Light; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1830 WHERE entry=78719;
-- 85: Blade-Dulling Greatcloak; comparator 62038 Worn Stoneskin Gargoyle Cape
UPDATE item_template SET stat_type5=32, stat_value5=106, stat_unk1_5=1256, stat_type6=36, stat_value6=106, stat_unk1_6=1256 WHERE entry=87427;
-- 85: Mazu's Robe; comparator 93860 Bloodstained Dreadmist Robe
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=1888 WHERE entry=87634;
-- 85: Amber-Starched Robes; comparator 93860 Bloodstained Dreadmist Robe
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=1888 WHERE entry=87635;
-- 85: Zom's Misty Hood; comparator 61937 Stained Shadowcraft Cap
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261, stat_type6=31, stat_value6=191, stat_unk1_6=1261 WHERE entry=87452;
-- 85: Crest of the Red Crane; comparator 61942 Preened Tribal War Feathers
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1667 WHERE entry=87605;
-- 85: Jiao-Skin Tunic; comparator 93865 Majestic Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2073 WHERE entry=87631;
-- 85: Blackfang Battleweave Legguards; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1830 WHERE entry=78708;
-- 85: Deep Earth Robes; comparator 93865 Majestic Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2740 WHERE entry=78660;
-- 85: Deep Earth Vestment; comparator 93865 Majestic Ironfeather Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2740 WHERE entry=78662;
-- 85: Helm of Elemental Binding; comparator 61936 Mystical Coif of Elements
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261 WHERE entry=87451;
-- 85: Imperfect Specimens 27 and 28; comparator 93876 Awakened Pauldrons of Elements
UPDATE item_template SET stat_type5=32, stat_value5=212, stat_unk1_5=2736 WHERE entry=78435;
-- 85: Spiritwalker's Legguards; comparator 62024 Tarnished Leggings of Destruction
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1830 WHERE entry=78704;
-- 85: Spiritwalker's Hauberk; comparator 93885 Awakened Vest of Elements
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2740 WHERE entry=78723;
-- 85: Spiritwalker's Cuirass; comparator 93888 Furious Deathdealer Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2740 WHERE entry=78724;
-- 85: Brackenshell Shoulderplates; comparator 93890 Gleaming Spaulders of Valor
UPDATE item_template SET stat_type5=32, stat_value5=180, stat_unk1_5=2323 WHERE entry=78367;
-- 85: Kunchong Carapace Chestguard; comparator 93891 Gleaming Breastplate of Valor
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=2073 WHERE entry=87627;
-- 85: Immolation Battleplate; comparator 93891 Gleaming Breastplate of Valor
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=3273 WHERE entry=71512;
-- 89: Claws of Gekkan; comparator 93858 Brawler's Bladed Claws
UPDATE item_template SET stat_type5=31, stat_value5=82, stat_unk1_5=1264 WHERE entry=85183;
-- 89: Mantid Trochanter; comparator 93857 Vengeful Heartseeker
UPDATE item_template SET stat_type5=31, stat_value5=74, stat_unk1_5=1141 WHERE entry=80933;
-- 89: Blade Trap Signet; comparator 62035 Antique Myrmidon's Signet
UPDATE item_template SET stat_type5=31, stat_value5=106, stat_unk1_5=1256 WHERE entry=85188;
-- 89: Fallout Filter; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261 WHERE entry=80920;
-- 89: Breezeswept Hood; comparator 61958 Tattered Dreadmist Mask
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261 WHERE entry=100971;
-- 89: Regal Silk Shoulderpads; comparator 93859 Bloodstained Dreadmist Mantle
UPDATE item_template SET stat_type5=32, stat_value5=212, stat_unk1_5=1883 WHERE entry=85194;
-- 89: Whisperwind Spaulders; comparator 93859 Bloodstained Dreadmist Mantle
UPDATE item_template SET stat_type5=32, stat_value5=212, stat_unk1_5=1883 WHERE entry=100972;
-- 89: Leggings of the Frenzy; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261 WHERE entry=80926;
-- 89: Cournith Waterstrider's Silken Finery; comparator 62029 Tattered Dreadmist Leggings
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1385 WHERE entry=90721;
-- 89: Cloak of Cleansing Flame; comparator 62040 Ancient Bloodmoon Cloak
UPDATE item_template SET stat_type5=36, stat_value5=106, stat_unk1_5=1256 WHERE entry=85182;
-- 89: Wall-Breaker Legguards; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261, stat_type6=36, stat_value6=191, stat_unk1_6=1261 WHERE entry=80929;
-- 89: Silent Leggings of the Ghostpaw; comparator 62026 Stained Shadowcraft Pants
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1385, stat_type6=36, stat_value6=191, stat_unk1_6=1385 WHERE entry=90720;
-- 89: Swarmcall Helm; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1261, stat_type6=31, stat_value6=191, stat_unk1_6=1261 WHERE entry=80925;
-- 89: Crest of the Clan Lords; comparator 61935 Tarnished Raging Berserker's Helm
UPDATE item_template SET stat_type5=36, stat_value5=191, stat_unk1_5=1261, stat_type6=31, stat_value6=191, stat_unk1_6=1261 WHERE entry=85175;
-- 89: Galedodger Chestguard; comparator 93888 Furious Deathdealer Breastplate
UPDATE item_template SET stat_type5=32, stat_value5=286, stat_unk1_5=1888 WHERE entry=100968;
-- 89: Arness's Scaled Leggings; comparator 62025 Mystical Kilt of Elements
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1385 WHERE entry=90723;
-- 89: Meteoric Greathelm; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_type5=31, stat_value5=191, stat_unk1_5=1261 WHERE entry=85179;
-- 89: Sightfinder Helm; comparator 61931 Polished Helm of Valor
UPDATE item_template SET stat_type5=31, stat_value5=191, stat_unk1_5=1261 WHERE entry=100959;
-- 89: Spaulders of Immovable Stone; comparator 93890 Gleaming Spaulders of Valor
UPDATE item_template SET stat_type5=32, stat_value5=180, stat_unk1_5=1599 WHERE entry=100970;
-- 89: Sap-Encrusted Legplates; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=32, stat_value5=191, stat_unk1_5=1261, stat_type6=37, stat_value6=191, stat_unk1_6=1261 WHERE entry=100954;
-- 89: Qu'nas' Apocryphal Legplates; comparator 62023 Polished Legplates of Valor
UPDATE item_template SET stat_type5=37, stat_value5=191, stat_unk1_5=1385 WHERE entry=90717;
DELETE FROM hotfix_data WHERE type=2442913102 AND hotfixDate='2026-10-31 12:00:00' AND entry IN (1455,59023,56696,1482,890,2042,58978,56679,20441,24118,5820,3324,6282,2231,7334,5254,28051,65985,3761,17042,19553,7710,7713,7683,7682,6757,7749,4197,3560,14398,17043,6720,55910,7747,6830,65973,19560,19564,63656,4134,65994,89570,11859,6429,62999,14417,11123,19129,19039,63664,9647,11502,63668,63674,11124,63671,63677,63670,63017,63665,63672,63678,88357,63636,63639,63657,53584,10652,18729,11629,53569,15705,812,15706,12038,12544,11904,3075,10751,13374,8251,9649,8261,11963,11874,12041,45052,11747,12018,13538,21320,21316,29108,27733,27734,31715,25718,31717,25711,27730,20033,20034,25710,27724,31718,31719,25702,25701,25979,25496,31134,89573,25485,25488,25620,23587,31659,31657,22497,25489,25510,22496,25513,25508,22476,25507,25518,16833,25506,31786,25614,25512,27717,16841,27715,25522,22416,38237,36882,38241,38238,38242,38219,35858,35814,35844,43181,35818,35859,35863,35884,38222,35871,35833,35864,43182,35877,35815,35885,35868,35872,34796,35869,35882,43183,35890,35842,35898,35878,35891,43184,35843,35822,35835,35817,35857,35576,34334,35015,30313,34329,35008,35016,34339,34340,34202,34210,37005,34170,34181,34241,34242,34232,34399,34244,34245,34195,34209,34211,34212,34188,31041,31042,34333,34208,34390,34228,34229,34168,35042,35048,34345,34192,34193,34215,34216,34180,55272,55790,55201,55236,55259,55277,55198,55786,55202,55206,55278,55204,55273,35677,55263,55205,55229,55207,55779,55274,55249,55260,87477,87515,89575,87442,65659,66886,87456,59783,87464,51497,87426,87508,87514,87438,87439,87458,87450,87491,87463,87429,87493,87492,87425,87462,87459,87453,87461,87460,87474,78471,73427,87419,87509,87437,87604,65174,78719,87427,87634,87635,87452,87605,87631,78708,78660,78662,87451,78435,78704,78723,78724,78367,87627,71512,85183,80933,85188,80920,100971,85194,100972,80926,90721,85182,80929,90720,80925,85175,100968,90723,85179,100959,100970,100954,90717);
INSERT INTO hotfix_data (entry,type,hotfixDate) VALUES (1455,2442913102,'2026-10-31 12:00:00'),(59023,2442913102,'2026-10-31 12:00:00'),(56696,2442913102,'2026-10-31 12:00:00'),(1482,2442913102,'2026-10-31 12:00:00'),(890,2442913102,'2026-10-31 12:00:00'),(2042,2442913102,'2026-10-31 12:00:00'),(58978,2442913102,'2026-10-31 12:00:00'),(56679,2442913102,'2026-10-31 12:00:00'),(20441,2442913102,'2026-10-31 12:00:00'),(24118,2442913102,'2026-10-31 12:00:00'),(5820,2442913102,'2026-10-31 12:00:00'),(3324,2442913102,'2026-10-31 12:00:00'),(6282,2442913102,'2026-10-31 12:00:00'),(2231,2442913102,'2026-10-31 12:00:00'),(7334,2442913102,'2026-10-31 12:00:00'),(5254,2442913102,'2026-10-31 12:00:00'),(28051,2442913102,'2026-10-31 12:00:00'),(65985,2442913102,'2026-10-31 12:00:00'),(3761,2442913102,'2026-10-31 12:00:00'),(17042,2442913102,'2026-10-31 12:00:00'),(19553,2442913102,'2026-10-31 12:00:00'),(7710,2442913102,'2026-10-31 12:00:00'),(7713,2442913102,'2026-10-31 12:00:00'),(7683,2442913102,'2026-10-31 12:00:00'),(7682,2442913102,'2026-10-31 12:00:00'),(6757,2442913102,'2026-10-31 12:00:00'),(7749,2442913102,'2026-10-31 12:00:00'),(4197,2442913102,'2026-10-31 12:00:00'),(3560,2442913102,'2026-10-31 12:00:00'),(14398,2442913102,'2026-10-31 12:00:00'),(17043,2442913102,'2026-10-31 12:00:00'),(6720,2442913102,'2026-10-31 12:00:00'),(55910,2442913102,'2026-10-31 12:00:00'),(7747,2442913102,'2026-10-31 12:00:00'),(6830,2442913102,'2026-10-31 12:00:00'),(65973,2442913102,'2026-10-31 12:00:00'),(19560,2442913102,'2026-10-31 12:00:00'),(19564,2442913102,'2026-10-31 12:00:00'),(63656,2442913102,'2026-10-31 12:00:00'),(4134,2442913102,'2026-10-31 12:00:00'),(65994,2442913102,'2026-10-31 12:00:00'),(89570,2442913102,'2026-10-31 12:00:00'),(11859,2442913102,'2026-10-31 12:00:00'),(6429,2442913102,'2026-10-31 12:00:00'),(62999,2442913102,'2026-10-31 12:00:00'),(14417,2442913102,'2026-10-31 12:00:00'),(11123,2442913102,'2026-10-31 12:00:00'),(19129,2442913102,'2026-10-31 12:00:00'),(19039,2442913102,'2026-10-31 12:00:00'),(63664,2442913102,'2026-10-31 12:00:00'),(9647,2442913102,'2026-10-31 12:00:00'),(11502,2442913102,'2026-10-31 12:00:00'),(63668,2442913102,'2026-10-31 12:00:00'),(63674,2442913102,'2026-10-31 12:00:00'),(11124,2442913102,'2026-10-31 12:00:00'),(63671,2442913102,'2026-10-31 12:00:00'),(63677,2442913102,'2026-10-31 12:00:00'),(63670,2442913102,'2026-10-31 12:00:00'),(63017,2442913102,'2026-10-31 12:00:00'),(63665,2442913102,'2026-10-31 12:00:00'),(63672,2442913102,'2026-10-31 12:00:00'),(63678,2442913102,'2026-10-31 12:00:00'),(88357,2442913102,'2026-10-31 12:00:00'),(63636,2442913102,'2026-10-31 12:00:00'),(63639,2442913102,'2026-10-31 12:00:00'),(63657,2442913102,'2026-10-31 12:00:00'),(53584,2442913102,'2026-10-31 12:00:00'),(10652,2442913102,'2026-10-31 12:00:00'),(18729,2442913102,'2026-10-31 12:00:00'),(11629,2442913102,'2026-10-31 12:00:00'),(53569,2442913102,'2026-10-31 12:00:00'),(15705,2442913102,'2026-10-31 12:00:00'),(812,2442913102,'2026-10-31 12:00:00'),(15706,2442913102,'2026-10-31 12:00:00'),(12038,2442913102,'2026-10-31 12:00:00'),(12544,2442913102,'2026-10-31 12:00:00'),(11904,2442913102,'2026-10-31 12:00:00'),(3075,2442913102,'2026-10-31 12:00:00'),(10751,2442913102,'2026-10-31 12:00:00'),(13374,2442913102,'2026-10-31 12:00:00'),(8251,2442913102,'2026-10-31 12:00:00'),(9649,2442913102,'2026-10-31 12:00:00'),(8261,2442913102,'2026-10-31 12:00:00'),(11963,2442913102,'2026-10-31 12:00:00'),(11874,2442913102,'2026-10-31 12:00:00'),(12041,2442913102,'2026-10-31 12:00:00'),(45052,2442913102,'2026-10-31 12:00:00'),(11747,2442913102,'2026-10-31 12:00:00'),(12018,2442913102,'2026-10-31 12:00:00'),(13538,2442913102,'2026-10-31 12:00:00'),(21320,2442913102,'2026-10-31 12:00:00'),(21316,2442913102,'2026-10-31 12:00:00'),(29108,2442913102,'2026-10-31 12:00:00'),(27733,2442913102,'2026-10-31 12:00:00'),(27734,2442913102,'2026-10-31 12:00:00'),(31715,2442913102,'2026-10-31 12:00:00'),(25718,2442913102,'2026-10-31 12:00:00'),(31717,2442913102,'2026-10-31 12:00:00'),(25711,2442913102,'2026-10-31 12:00:00'),(27730,2442913102,'2026-10-31 12:00:00'),(20033,2442913102,'2026-10-31 12:00:00'),(20034,2442913102,'2026-10-31 12:00:00'),(25710,2442913102,'2026-10-31 12:00:00'),(27724,2442913102,'2026-10-31 12:00:00'),(31718,2442913102,'2026-10-31 12:00:00'),(31719,2442913102,'2026-10-31 12:00:00'),(25702,2442913102,'2026-10-31 12:00:00'),(25701,2442913102,'2026-10-31 12:00:00'),(25979,2442913102,'2026-10-31 12:00:00'),(25496,2442913102,'2026-10-31 12:00:00'),(31134,2442913102,'2026-10-31 12:00:00'),(89573,2442913102,'2026-10-31 12:00:00'),(25485,2442913102,'2026-10-31 12:00:00'),(25488,2442913102,'2026-10-31 12:00:00'),(25620,2442913102,'2026-10-31 12:00:00'),(23587,2442913102,'2026-10-31 12:00:00'),(31659,2442913102,'2026-10-31 12:00:00'),(31657,2442913102,'2026-10-31 12:00:00'),(22497,2442913102,'2026-10-31 12:00:00'),(25489,2442913102,'2026-10-31 12:00:00'),(25510,2442913102,'2026-10-31 12:00:00'),(22496,2442913102,'2026-10-31 12:00:00'),(25513,2442913102,'2026-10-31 12:00:00'),(25508,2442913102,'2026-10-31 12:00:00'),(22476,2442913102,'2026-10-31 12:00:00'),(25507,2442913102,'2026-10-31 12:00:00'),(25518,2442913102,'2026-10-31 12:00:00'),(16833,2442913102,'2026-10-31 12:00:00'),(25506,2442913102,'2026-10-31 12:00:00'),(31786,2442913102,'2026-10-31 12:00:00'),(25614,2442913102,'2026-10-31 12:00:00'),(25512,2442913102,'2026-10-31 12:00:00'),(27717,2442913102,'2026-10-31 12:00:00'),(16841,2442913102,'2026-10-31 12:00:00'),(27715,2442913102,'2026-10-31 12:00:00'),(25522,2442913102,'2026-10-31 12:00:00'),(22416,2442913102,'2026-10-31 12:00:00'),(38237,2442913102,'2026-10-31 12:00:00'),(36882,2442913102,'2026-10-31 12:00:00'),(38241,2442913102,'2026-10-31 12:00:00'),(38238,2442913102,'2026-10-31 12:00:00'),(38242,2442913102,'2026-10-31 12:00:00'),(38219,2442913102,'2026-10-31 12:00:00'),(35858,2442913102,'2026-10-31 12:00:00'),(35814,2442913102,'2026-10-31 12:00:00'),(35844,2442913102,'2026-10-31 12:00:00'),(43181,2442913102,'2026-10-31 12:00:00'),(35818,2442913102,'2026-10-31 12:00:00'),(35859,2442913102,'2026-10-31 12:00:00'),(35863,2442913102,'2026-10-31 12:00:00'),(35884,2442913102,'2026-10-31 12:00:00'),(38222,2442913102,'2026-10-31 12:00:00'),(35871,2442913102,'2026-10-31 12:00:00'),(35833,2442913102,'2026-10-31 12:00:00'),(35864,2442913102,'2026-10-31 12:00:00'),(43182,2442913102,'2026-10-31 12:00:00'),(35877,2442913102,'2026-10-31 12:00:00'),(35815,2442913102,'2026-10-31 12:00:00'),(35885,2442913102,'2026-10-31 12:00:00'),(35868,2442913102,'2026-10-31 12:00:00'),(35872,2442913102,'2026-10-31 12:00:00'),(34796,2442913102,'2026-10-31 12:00:00'),(35869,2442913102,'2026-10-31 12:00:00'),(35882,2442913102,'2026-10-31 12:00:00'),(43183,2442913102,'2026-10-31 12:00:00'),(35890,2442913102,'2026-10-31 12:00:00'),(35842,2442913102,'2026-10-31 12:00:00'),(35898,2442913102,'2026-10-31 12:00:00'),(35878,2442913102,'2026-10-31 12:00:00'),(35891,2442913102,'2026-10-31 12:00:00'),(43184,2442913102,'2026-10-31 12:00:00'),(35843,2442913102,'2026-10-31 12:00:00'),(35822,2442913102,'2026-10-31 12:00:00'),(35835,2442913102,'2026-10-31 12:00:00'),(35817,2442913102,'2026-10-31 12:00:00'),(35857,2442913102,'2026-10-31 12:00:00'),(35576,2442913102,'2026-10-31 12:00:00'),(34334,2442913102,'2026-10-31 12:00:00'),(35015,2442913102,'2026-10-31 12:00:00'),(30313,2442913102,'2026-10-31 12:00:00'),(34329,2442913102,'2026-10-31 12:00:00'),(35008,2442913102,'2026-10-31 12:00:00'),(35016,2442913102,'2026-10-31 12:00:00'),(34339,2442913102,'2026-10-31 12:00:00'),(34340,2442913102,'2026-10-31 12:00:00'),(34202,2442913102,'2026-10-31 12:00:00'),(34210,2442913102,'2026-10-31 12:00:00'),(37005,2442913102,'2026-10-31 12:00:00'),(34170,2442913102,'2026-10-31 12:00:00'),(34181,2442913102,'2026-10-31 12:00:00'),(34241,2442913102,'2026-10-31 12:00:00'),(34242,2442913102,'2026-10-31 12:00:00'),(34232,2442913102,'2026-10-31 12:00:00'),(34399,2442913102,'2026-10-31 12:00:00'),(34244,2442913102,'2026-10-31 12:00:00'),(34245,2442913102,'2026-10-31 12:00:00'),(34195,2442913102,'2026-10-31 12:00:00'),(34209,2442913102,'2026-10-31 12:00:00'),(34211,2442913102,'2026-10-31 12:00:00'),(34212,2442913102,'2026-10-31 12:00:00'),(34188,2442913102,'2026-10-31 12:00:00'),(31041,2442913102,'2026-10-31 12:00:00'),(31042,2442913102,'2026-10-31 12:00:00'),(34333,2442913102,'2026-10-31 12:00:00'),(34208,2442913102,'2026-10-31 12:00:00'),(34390,2442913102,'2026-10-31 12:00:00'),(34228,2442913102,'2026-10-31 12:00:00'),(34229,2442913102,'2026-10-31 12:00:00'),(34168,2442913102,'2026-10-31 12:00:00'),(35042,2442913102,'2026-10-31 12:00:00'),(35048,2442913102,'2026-10-31 12:00:00'),(34345,2442913102,'2026-10-31 12:00:00'),(34192,2442913102,'2026-10-31 12:00:00'),(34193,2442913102,'2026-10-31 12:00:00'),(34215,2442913102,'2026-10-31 12:00:00'),(34216,2442913102,'2026-10-31 12:00:00'),(34180,2442913102,'2026-10-31 12:00:00'),(55272,2442913102,'2026-10-31 12:00:00'),(55790,2442913102,'2026-10-31 12:00:00'),(55201,2442913102,'2026-10-31 12:00:00'),(55236,2442913102,'2026-10-31 12:00:00'),(55259,2442913102,'2026-10-31 12:00:00'),(55277,2442913102,'2026-10-31 12:00:00'),(55198,2442913102,'2026-10-31 12:00:00'),(55786,2442913102,'2026-10-31 12:00:00'),(55202,2442913102,'2026-10-31 12:00:00'),(55206,2442913102,'2026-10-31 12:00:00'),(55278,2442913102,'2026-10-31 12:00:00'),(55204,2442913102,'2026-10-31 12:00:00'),(55273,2442913102,'2026-10-31 12:00:00'),(35677,2442913102,'2026-10-31 12:00:00'),(55263,2442913102,'2026-10-31 12:00:00'),(55205,2442913102,'2026-10-31 12:00:00'),(55229,2442913102,'2026-10-31 12:00:00'),(55207,2442913102,'2026-10-31 12:00:00'),(55779,2442913102,'2026-10-31 12:00:00'),(55274,2442913102,'2026-10-31 12:00:00'),(55249,2442913102,'2026-10-31 12:00:00'),(55260,2442913102,'2026-10-31 12:00:00'),(87477,2442913102,'2026-10-31 12:00:00'),(87515,2442913102,'2026-10-31 12:00:00'),(89575,2442913102,'2026-10-31 12:00:00'),(87442,2442913102,'2026-10-31 12:00:00'),(65659,2442913102,'2026-10-31 12:00:00'),(66886,2442913102,'2026-10-31 12:00:00'),(87456,2442913102,'2026-10-31 12:00:00'),(59783,2442913102,'2026-10-31 12:00:00'),(87464,2442913102,'2026-10-31 12:00:00'),(51497,2442913102,'2026-10-31 12:00:00'),(87426,2442913102,'2026-10-31 12:00:00'),(87508,2442913102,'2026-10-31 12:00:00'),(87514,2442913102,'2026-10-31 12:00:00'),(87438,2442913102,'2026-10-31 12:00:00'),(87439,2442913102,'2026-10-31 12:00:00'),(87458,2442913102,'2026-10-31 12:00:00'),(87450,2442913102,'2026-10-31 12:00:00'),(87491,2442913102,'2026-10-31 12:00:00'),(87463,2442913102,'2026-10-31 12:00:00'),(87429,2442913102,'2026-10-31 12:00:00'),(87493,2442913102,'2026-10-31 12:00:00'),(87492,2442913102,'2026-10-31 12:00:00'),(87425,2442913102,'2026-10-31 12:00:00'),(87462,2442913102,'2026-10-31 12:00:00'),(87459,2442913102,'2026-10-31 12:00:00'),(87453,2442913102,'2026-10-31 12:00:00'),(87461,2442913102,'2026-10-31 12:00:00'),(87460,2442913102,'2026-10-31 12:00:00'),(87474,2442913102,'2026-10-31 12:00:00'),(78471,2442913102,'2026-10-31 12:00:00'),(73427,2442913102,'2026-10-31 12:00:00'),(87419,2442913102,'2026-10-31 12:00:00'),(87509,2442913102,'2026-10-31 12:00:00'),(87437,2442913102,'2026-10-31 12:00:00'),(87604,2442913102,'2026-10-31 12:00:00'),(65174,2442913102,'2026-10-31 12:00:00'),(78719,2442913102,'2026-10-31 12:00:00'),(87427,2442913102,'2026-10-31 12:00:00'),(87634,2442913102,'2026-10-31 12:00:00'),(87635,2442913102,'2026-10-31 12:00:00'),(87452,2442913102,'2026-10-31 12:00:00'),(87605,2442913102,'2026-10-31 12:00:00'),(87631,2442913102,'2026-10-31 12:00:00'),(78708,2442913102,'2026-10-31 12:00:00'),(78660,2442913102,'2026-10-31 12:00:00'),(78662,2442913102,'2026-10-31 12:00:00'),(87451,2442913102,'2026-10-31 12:00:00'),(78435,2442913102,'2026-10-31 12:00:00'),(78704,2442913102,'2026-10-31 12:00:00'),(78723,2442913102,'2026-10-31 12:00:00'),(78724,2442913102,'2026-10-31 12:00:00'),(78367,2442913102,'2026-10-31 12:00:00'),(87627,2442913102,'2026-10-31 12:00:00'),(71512,2442913102,'2026-10-31 12:00:00'),(85183,2442913102,'2026-10-31 12:00:00'),(80933,2442913102,'2026-10-31 12:00:00'),(85188,2442913102,'2026-10-31 12:00:00'),(80920,2442913102,'2026-10-31 12:00:00'),(100971,2442913102,'2026-10-31 12:00:00'),(85194,2442913102,'2026-10-31 12:00:00'),(100972,2442913102,'2026-10-31 12:00:00'),(80926,2442913102,'2026-10-31 12:00:00'),(90721,2442913102,'2026-10-31 12:00:00'),(85182,2442913102,'2026-10-31 12:00:00'),(80929,2442913102,'2026-10-31 12:00:00'),(90720,2442913102,'2026-10-31 12:00:00'),(80925,2442913102,'2026-10-31 12:00:00'),(85175,2442913102,'2026-10-31 12:00:00'),(100968,2442913102,'2026-10-31 12:00:00'),(90723,2442913102,'2026-10-31 12:00:00'),(85179,2442913102,'2026-10-31 12:00:00'),(100959,2442913102,'2026-10-31 12:00:00'),(100970,2442913102,'2026-10-31 12:00:00'),(100954,2442913102,'2026-10-31 12:00:00'),(90717,2442913102,'2026-10-31 12:00:00');
COMMIT;

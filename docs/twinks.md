# Twink gear research (T-045)

Research snapshot: 2026-10-31, imported v1.0.4 world_codex (80,064 item templates).
Design: common bracket gear should slightly outperform matching heirlooms. Brackets use
x9 endpoints from DESIGN.md; expansion/raid breakpoints additionally include 60, 70,
78, 80 and 85. Level 89 covers pre-cap Pandaria gear. Level 90 endgame is outside this task.

## What this catalogue establishes

The table below lists the two highest raw item-level candidates per weapon/armor subclass
and inventory type at each endpoint. IDs and names come from this server, not modern Classic
or retail databases. Equal item levels are ordered by quality then ID. It includes known
vendor/loot/reference-loot entries and quest rewards; quest-only items must meet the minimum
quest level. RequiredLevel=0 items need a qualifying quest source rather than being assumed
available at level 1. This is a candidate shortlist, **not a verified per-spec best-in-slot list**.

Acquisition still needs checking: reference-loot reachability, disabled spawns/vendors,
quest-chain prerequisites and faction gates, crafting/profession requirements, sockets,
random suffixes, proc effects, PvP ratings and class eligibility are not ranked here.
Craft-only and DBC-only items can be missing. Do not apply blanket buffs from this table.

## Hotfix implementation findings

- ObjectMgr::LoadItemTemplates overlays item_template onto Item-sparse.db2.
- WorldSession::SendItemSparseDb2Reply sends those template values to the client;
  hotfix_data registers the changed entry and DB2 type with a fixed migration timestamp.
- Item::GetStatValue recalculates stats from stat_unk1_N allocations when item level
  changes. Updating only stat_valueN would lose the buff for upgraded/scaled items.
- Weapon damage and armor are derived from item level, quality and stat scaling factor;
  adding primary stats alone cannot establish that a weapon beats an heirloom.
- Heirlooms use ScalingStatDistribution; their item_template zero stats are not the
  comparison baseline. Compare effective values at every endpoint using server DBC curves.

## Applied target (user decision)

The user selected **7%, weighted toward damage**, on 2026-10-31. The SQL uses the
highest offense-scoring matching heirloom at the first endpoint where the candidate
is shortlisted. Matching requires item class, weapon/armor subclass, slot (robes
share the chest slot), identical primary-stat types and caster-weapon flag.
The comparison respects the heirloom scaling distribution's maximum level.

Primary and existing heirloom offensive stats get a 107% floor (rounded up);
missing offensive stats use empty stat slots. Existing stronger values remain.
Stamina is retained without increases: damage gets the extra budget. The comparator
ranking weights stamina 0.25, crit/haste 1.25, attack power 0.5, and other supported
offense 1. These weights choose a comparator; they are a heuristic, **not a combat
simulation or a guarantee of 7% more character DPS**. A 7% rating increase is not
seven percentage points of crit. No universal raw-crit bonus is added.

Weapon DPS also gets a 107% floor using the same subclass's effective heirloom
curve; the smallest sufficient item level is used. Armor and durability derived
from that item level consequently change for affected weapons. Required levels,
class/faction restrictions, acquisition, sockets, procs and set bonuses remain.
Spell power remains spell power instead of being converted into intellect; the
caster heirloom's additional ScalingStatValues spell power is included.

All modified stat allocations are rebased on the final item level, including socket
costs, so normal upgrade scaling follows the new budget. Random-property/suffix
items are excluded. Slots without a matching heirloom remain stock. This is a
first data pass; class-specific proc, socket and acquisition optimization still
needs playtesting. The shortlist is not an exhaustive per-spec BiS assertion.

Migration: `sql/updates/world/2026_10_31_codex_00_twink_gear.sql` (314 item updates).
Applied by hand twice to world_codex; full selected item/hotfix snapshots were
identical. Main and Claude databases were not updated. SQL uses absolute values
and a fixed Item-sparse hotfix timestamp; the DB2 table hash was verified from the
local client data header. Restart the world server to load templates/hotfix notices.

## Modified-item audit

Comparators are selected from effective heirloom stats at the indicated endpoint.
The original and modified item levels show weapon-DPS changes. Item stat changes
are listed explicitly in the SQL migration.

| Endpoint | ID | Item | Comparator heirloom | Old iLvl | New iLvl |
|---|---|---|---|---|---|
| 19 | 1455 | Blackrock Champion's Axe | 42943 | 24 | 25 |
| 19 | 59023 | Loch Modan Musket | 44093 | 20 | 20 |
| 19 | 56696 | Legacy of Teronis | 69893 | 25 | 44 |
| 19 | 1482 | Shadowfang | 42945 | 24 | 38 |
| 19 | 890 | Twisted Chanter's Staff | 42947 | 24 | 24 |
| 19 | 2042 | Staff of Westfall | 42947 | 24 | 24 |
| 19 | 58978 | Filthy Paw | 92948 | 11 | 31 |
| 19 | 56679 | Dissector | 42944 | 24 | 38 |
| 19 | 20441 | Scout's Blade | 42944 | 23 | 38 |
| 19 | 24118 | Signet of Argas | 62036 | 32 | 32 |
| 19 | 5820 | Faerie Mantle | 42985 | 32 | 32 |
| 19 | 3324 | Ghostly Mantle | 42985 | 28 | 28 |
| 19 | 6282 | Sacred Burial Trousers | 62029 | 32 | 32 |
| 19 | 2231 | Inferno Robe | 48691 | 40 | 40 |
| 19 | 7334 | Efflorescent Robe | 48691 | 28 | 28 |
| 19 | 5254 | Rugged Spaulders | 38694 | 21 | 21 |
| 19 | 28051 | Jerkin of the Untamed Spirit | 48689 | 85 | 85 |
| 19 | 65985 | Cleansed Pauldrons | 42951 | 21 | 21 |
| 19 | 3761 | Deadskull Shield | 93902 | 32 | 32 |
| 29 | 17042 | Nail Spitter | 44093 | 45 | 45 |
| 29 | 19553 | Legionnaire's Sword | 69893 | 33 | 55 |
| 29 | 7710 | Loksey's Training Stick | 79131 | 34 | 34 |
| 29 | 7713 | Illusionary Rod | 42947 | 34 | 34 |
| 29 | 7683 | Bloody Brass Knuckles | 92948 | 31 | 36 |
| 29 | 7682 | Torturing Poker | 42944 | 34 | 55 |
| 29 | 6757 | Jaina's Signet Ring | 62035 | 38 | 38 |
| 29 | 7749 | Omega Orb | 93904 | 41 | 41 |
| 29 | 4197 | Berylline Pads | 42985 | 35 | 35 |
| 29 | 3560 | Mantle of Honor | 42985 | 35 | 35 |
| 29 | 14398 | Resilient Tunic | 48691 | 33 | 33 |
| 29 | 17043 | Zealot's Robe | 48691 | 45 | 45 |
| 29 | 6720 | Spirit Hunter Headdress | 61937 | 37 | 37 |
| 29 | 55910 | Defiant Spire Shoulderguard | 38694 | 33 | 33 |
| 29 | 7747 | Vile Protector | 93902 | 41 | 41 |
| 39 | 6830 | Bonebiter | 42943 | 44 | 44 |
| 39 | 65973 | Fras Siabi's Cigar Cutter | 42943 | 44 | 44 |
| 39 | 19560 | Outrider's Bow | 42946 | 43 | 43 |
| 39 | 19564 | Outrunner's Bow | 42946 | 43 | 43 |
| 39 | 63656 | Lakota's Mace | 48716 | 45 | 80 |
| 39 | 4134 | Nimboya's Mystical Staff | 42947 | 46 | 46 |
| 39 | 65994 | Staff of Athen'a | 42947 | 44 | 44 |
| 39 | 89570 | Initiate's Handwraps | 92948 | 35 | 52 |
| 39 | 11859 | Jademoon Orb | 93904 | 45 | 45 |
| 39 | 6429 | Mistscape Wizard Hat | 61958 | 44 | 44 |
| 39 | 62999 | Corpseburner's Mantle | 42985 | 44 | 44 |
| 39 | 14417 | Silksand Tunic | 48691 | 44 | 44 |
| 39 | 11123 | Rainstrider Leggings | 62029 | 55 | 55 |
| 39 | 19129 | Everglowing Robe | 48691 | 49 | 49 |
| 39 | 19039 | Zorbin's Water Resistant Hat | 61942 | 48 | 48 |
| 39 | 63664 | Withering Mask | 61942 | 45 | 45 |
| 39 | 9647 | Failed Flying Experiment | 38694 | 48 | 48 |
| 39 | 11502 | Loreskin Shoulders | 38694 | 46 | 46 |
| 39 | 63668 | Leggings of the Thousand Needles | 62027 | 45 | 45 |
| 39 | 63674 | Leggings of the Thousand Needles | 62027 | 45 | 45 |
| 39 | 11124 | Helm of Exile | 61935 | 55 | 55 |
| 39 | 63671 | Pauldrons of the Thousand Needles | 42951 | 45 | 45 |
| 39 | 63677 | Pauldrons of the Thousand Needles | 42951 | 45 | 45 |
| 39 | 63670 | Chestguard of the Thousand Needles | 48677 | 45 | 45 |
| 39 | 63017 | Legwraps of the Slain | 62025 | 45 | 45 |
| 39 | 63665 | Dirty Work Greaves | 62025 | 45 | 45 |
| 39 | 63672 | Helm of the Thousand Needles | 61931 | 45 | 45 |
| 39 | 63678 | Helm of the Thousand Needles | 61931 | 45 | 45 |
| 39 | 88357 | Vigorsteel Spaulders | 42949 | 43 | 43 |
| 39 | 63636 | Legguards of Disfavor | 62023 | 43 | 43 |
| 39 | 63639 | Legguards of Disfavor | 62023 | 43 | 43 |
| 39 | 63657 | Well Hammered Shield | 93903 | 45 | 45 |
| 49 | 53584 | Beastkiller | 42943 | 53 | 53 |
| 49 | 10652 | Will of the Mountain Giant | 42943 | 51 | 51 |
| 49 | 18729 | Screeching Bow | 42946 | 53 | 53 |
| 49 | 11629 | Houndmaster's Rifle | 44093 | 52 | 52 |
| 49 | 53569 | Confiscated Poacher's Gun | 44093 | 52 | 52 |
| 49 | 15705 | Tidecrest Blade | 69893 | 57 | 97 |
| 49 | 812 | Glowing Brightwood Staff | 42947 | 54 | 54 |
| 49 | 15706 | Hunt Tracker Blade | 42944 | 57 | 97 |
| 49 | 12038 | Lagrave's Seal | 62037 | 56 | 56 |
| 49 | 12544 | Thrall's Resolve | 62035 | 56 | 56 |
| 49 | 11904 | Spirit of Aquementas | 93904 | 56 | 56 |
| 49 | 3075 | Eye of Flame | 61958 | 55 | 55 |
| 49 | 10751 | Gemburst Circlet | 61958 | 54 | 54 |
| 49 | 13374 | Soulstealer Mantle | 42985 | 53 | 53 |
| 49 | 8251 | Imperial Red Pants | 62029 | 54 | 54 |
| 49 | 9649 | Royal Highmark Vestments | 48691 | 55 | 55 |
| 49 | 8261 | Serpentskin Helm | 61942 | 54 | 54 |
| 49 | 11963 | Penance Spaulders | 38694 | 56 | 56 |
| 49 | 11874 | Clouddrift Mantle | 38694 | 56 | 56 |
| 49 | 12041 | Windshear Leggings | 62026 | 56 | 56 |
| 49 | 45052 | Gruffscale Leggings | 62026 | 55 | 55 |
| 49 | 11747 | Flamestrider Robes | 48687 | 52 | 52 |
| 49 | 12018 | Conservator Helm | 61936 | 56 | 56 |
| 49 | 13538 | Windshrieker Pauldrons | 42951 | 53 | 53 |
| 49 | 21320 | Vest of the Den Watcher | 48677 | 58 | 58 |
| 49 | 21316 | Leggings of the Ursa | 62023 | 55 | 55 |
| 59 | 29108 | Blade of the Unyielding | 42945 | 87 | 121 |
| 59 | 27733 | Warden's Ring of Precision | 62036 | 93 | 93 |
| 59 | 27734 | Expedition Caster's Band | 62037 | 93 | 93 |
| 59 | 31715 | Demoniac Soul Prison | 93904 | 87 | 87 |
| 59 | 25718 | Mantle of Magical Might | 42985 | 85 | 85 |
| 59 | 31717 | Shadowcast Tunic | 48691 | 88 | 88 |
| 59 | 25711 | Deadly Borer Leggings | 62029 | 88 | 88 |
| 59 | 27730 | Watcher's Cloak of Vigilance | 62038 | 93 | 93 |
| 59 | 20033 | Zandalar Demoniac's Robe | 48691 | 65 | 65 |
| 59 | 20034 | Zandalar Illusionist's Robe | 48691 | 65 | 65 |
| 59 | 25710 | Moonkin Headdress | 61942 | 88 | 88 |
| 59 | 27724 | Wild Shoulderpads | 38694 | 93 | 93 |
| 59 | 31718 | Darkstorm Tunic | 48689 | 88 | 88 |
| 59 | 31719 | Stormstrike Vest | 48683 | 88 | 88 |
| 59 | 25702 | Scaled Legs of Ruination | 62025 | 88 | 88 |
| 59 | 25701 | Breastplate of Retribution | 48685 | 88 | 88 |
| 59 | 25979 | Flintlocke's Piloting Pants | 62023 | 87 | 87 |
| 60 | 25496 | Mag'har Bow | 42946 | 93 | 93 |
| 60 | 31134 | Blade of Misfortune | 44092 | 85 | 85 |
| 60 | 89573 | Handwraps of Meditation | 92948 | 93 | 101 |
| 60 | 25485 | Amaan's Signet | 62035 | 93 | 93 |
| 60 | 25488 | Signet of Aeranas | 62037 | 93 | 93 |
| 60 | 25620 | Ancient Crystal Talisman | 42992 | 96 | 96 |
| 60 | 23587 | Mirren's Drinking Hat | 61958 | 90 | 90 |
| 60 | 31659 | Researcher's Mantle | 42985 | 90 | 90 |
| 60 | 31657 | Chemise of Rebirth | 48691 | 90 | 90 |
| 60 | 22497 | Frostfire Leggings | 62029 | 88 | 88 |
| 60 | 25489 | Windtalker's Cloak | 62040 | 93 | 93 |
| 60 | 25510 | Ceremonial Robes | 48691 | 93 | 93 |
| 60 | 22496 | Frostfire Robe | 48691 | 92 | 92 |
| 60 | 25513 | Clefthoof Hide Mask | 61937 | 93 | 93 |
| 60 | 25508 | Omenai Vest | 48687 | 93 | 93 |
| 60 | 22476 | Bonescythe Breastplate | 48689 | 92 | 92 |
| 60 | 25507 | Leggings of Telhamat | 62026 | 93 | 93 |
| 60 | 25518 | Explorer's Leggings | 62027 | 93 | 93 |
| 60 | 16833 | Cenarion Vestments | 48687 | 66 | 66 |
| 60 | 25506 | Vindicator's Chain Helm | 61935 | 93 | 93 |
| 60 | 31786 | Blacksting Shoulders | 42950 | 90 | 90 |
| 60 | 25614 | Feralfen Beastmaster's Hauberk | 48677 | 102 | 102 |
| 60 | 25512 | Tribal Hauberk | 48677 | 93 | 93 |
| 60 | 27717 | Expedition Forager Leggings | 62024 | 90 | 90 |
| 60 | 16841 | Earthfury Vestments | 48683 | 66 | 66 |
| 60 | 27715 | Circle's Stalwart Helmet | 61931 | 90 | 90 |
| 60 | 25522 | Marshstrider's Spaulders | 42949 | 93 | 93 |
| 60 | 22416 | Dreadnaught Breastplate | 48685 | 92 | 92 |
| 69 | 38237 | Axe of Frozen Death | 42943 | 138 | 138 |
| 69 | 36882 | Warsong Longbow | 42946 | 138 | 138 |
| 69 | 38241 | Fury of the Raging Dragon | 44093 | 138 | 138 |
| 69 | 38238 | Borean Smasher | 92948 | 138 | 151 |
| 69 | 38242 | Fang of the Desolate Soul | 42944 | 138 | 165 |
| 69 | 38219 | Ring of Decimation | 62037 | 155 | 155 |
| 69 | 35858 | Tome of Alacrity | 93904 | 138 | 138 |
| 69 | 35814 | Benevolent Hood | 61958 | 138 | 138 |
| 69 | 35844 | Shock-Resistant Hood | 61958 | 138 | 138 |
| 69 | 43181 | Shoulders of the Northern Lights | 42985 | 155 | 155 |
| 69 | 35818 | Stretch-Hide Spaulders | 42985 | 138 | 138 |
| 69 | 35859 | Fire-Purifying Tunic | 48691 | 138 | 138 |
| 69 | 35863 | Earth-Infused Leggings | 62029 | 138 | 138 |
| 69 | 35884 | Regal Pantaloons | 62029 | 138 | 138 |
| 69 | 38222 | Mantle of Keristrasza | 62040 | 159 | 159 |
| 69 | 35871 | Indigo Robe of Replenishment | 48691 | 138 | 138 |
| 69 | 35833 | Imperious Worghide Cap | 61942 | 138 | 138 |
| 69 | 35864 | Fizznik's Patented Earwarmer | 61937 | 138 | 138 |
| 69 | 43182 | Cured Mammoth Hide Mantle | 38694 | 155 | 155 |
| 69 | 35877 | Worgskin Shoulders | 38694 | 138 | 138 |
| 69 | 35815 | Bone-Threaded Harness | 48689 | 138 | 138 |
| 69 | 35885 | Tribal Chestguard | 48687 | 138 | 138 |
| 69 | 35868 | Grounded Pants | 62027 | 138 | 138 |
| 69 | 35872 | Iron-Studded Leggings | 62026 | 138 | 138 |
| 69 | 34796 | Robes of Summer Flame | 48687 | 115 | 115 |
| 69 | 35869 | Hex-Linked Stronghelm | 61935 | 138 | 138 |
| 69 | 35882 | Magispike Helm | 61936 | 138 | 138 |
| 69 | 43183 | Tundra Tracker's Shoulderguards | 42950 | 155 | 155 |
| 69 | 35890 | Magdun Spaulders | 42950 | 138 | 138 |
| 69 | 35842 | Azure Chain Hauberk | 48677 | 138 | 138 |
| 69 | 35898 | Reinforced Tuskhide Hauberk | 48683 | 138 | 138 |
| 69 | 35878 | Dusk-Linked Leggings | 62024 | 138 | 138 |
| 69 | 35891 | Runeplate Helm | 61931 | 138 | 138 |
| 69 | 43184 | Tundra Pauldrons | 42949 | 155 | 155 |
| 69 | 35843 | Ramshorn-Inlaid Shoulders | 42949 | 138 | 138 |
| 69 | 35822 | Scavenged Tirasian Plate | 48685 | 138 | 138 |
| 69 | 35835 | Antique Reinforced Legguards | 62023 | 138 | 138 |
| 69 | 35817 | Nerubian Inner Husk | 93902 | 138 | 138 |
| 69 | 35857 | Munificent Bulwark | 93903 | 138 | 138 |
| 70 | 35576 | Ingvar's Monolithic Cleaver | 42943 | 155 | 155 |
| 70 | 34334 | Thori'dal, the Stars' Fury | 42946 | 164 | 164 |
| 70 | 35015 | Brutal Gladiator's Greatsword | 44092 | 154 | 154 |
| 70 | 30313 | Staff of Disintegration | 42947 | 175 | 175 |
| 70 | 34329 | Crux of the Apocalypse | 42944 | 164 | 164 |
| 70 | 35008 | Brutal Gladiator's Endgame | 93904 | 159 | 159 |
| 70 | 35016 | Brutal Gladiator's Grimoire | 93904 | 159 | 159 |
| 70 | 34339 | Cowl of Light's Purity | 61958 | 164 | 164 |
| 70 | 34340 | Dark Conjuror's Collar | 61958 | 164 | 164 |
| 70 | 34202 | Shawl of Wonderment | 42985 | 159 | 159 |
| 70 | 34210 | Amice of the Convoker | 42985 | 159 | 159 |
| 70 | 37005 | D.E.H.T.A. Overshirt | 48691 | 138 | 138 |
| 70 | 34170 | Pantaloons of Calming Strife | 62029 | 159 | 159 |
| 70 | 34181 | Leggings of Calamity | 62029 | 159 | 159 |
| 70 | 34241 | Cloak of Unforgivable Sin | 62039 | 164 | 164 |
| 70 | 34242 | Tattered Cape of Antonidas | 62040 | 164 | 164 |
| 70 | 34232 | Fel Conquerer Raiments | 48691 | 159 | 159 |
| 70 | 34399 | Robes of Ghostly Hatred | 48691 | 159 | 159 |
| 70 | 34244 | Duplicitous Guise | 61937 | 164 | 164 |
| 70 | 34245 | Cover of Ursol the Wise | 61942 | 164 | 164 |
| 70 | 34195 | Shoulderpads of Vehemence | 38694 | 159 | 159 |
| 70 | 34209 | Spaulders of Reclamation | 42984 | 159 | 159 |
| 70 | 34211 | Harness of Carnal Instinct | 48689 | 159 | 159 |
| 70 | 34212 | Sunglow Vest | 48687 | 159 | 159 |
| 70 | 34188 | Leggings of the Immortal Night | 62026 | 159 | 159 |
| 70 | 31041 | Thunderheart Tunic | 48687 | 146 | 146 |
| 70 | 31042 | Thunderheart Chestguard | 48689 | 146 | 146 |
| 70 | 34333 | Coif of Alleria | 61935 | 164 | 164 |
| 70 | 34208 | Equilibrium Epaulets | 42951 | 159 | 159 |
| 70 | 34390 | Erupting Epaulets | 42951 | 159 | 159 |
| 70 | 34228 | Vicious Hawkstrider Hauberk | 48677 | 159 | 159 |
| 70 | 34229 | Garments of Serene Shores | 48683 | 159 | 159 |
| 70 | 34168 | Starstalker Legguards | 62024 | 159 | 159 |
| 70 | 35042 | Brutal Gladiator's Linked Armor | 48677 | 159 | 159 |
| 70 | 35048 | Brutal Gladiator's Mail Armor | 48683 | 159 | 159 |
| 70 | 34345 | Crown of Anasterian | 61931 | 164 | 164 |
| 70 | 34192 | Pauldrons of Perseverance | 42949 | 159 | 159 |
| 70 | 34193 | Spaulders of the Thalassian Savior | 44100 | 159 | 159 |
| 70 | 34215 | Warharness of Reckless Fury | 48685 | 159 | 159 |
| 70 | 34216 | Heroic Judicator's Chestguard | 48685 | 159 | 159 |
| 70 | 34180 | Felfury Legplates | 62023 | 159 | 159 |
| 78 | 55272 | Steelbender's Masterpiece | 42944 | 308 | 308 |
| 78 | 55790 | Toxidunk Dagger | 42944 | 308 | 308 |
| 78 | 55201 | Entwined Nereis | 62035 | 308 | 308 |
| 78 | 55236 | Anthia's Ring | 62037 | 308 | 308 |
| 78 | 55259 | Bioluminescent Lamp | 93904 | 308 | 308 |
| 78 | 55277 | Beauty's Favorite Bone | 93904 | 308 | 308 |
| 78 | 55198 | Aurelian Mitre | 61958 | 308 | 308 |
| 78 | 55786 | Kyrstel Mantle | 42985 | 308 | 308 |
| 78 | 55202 | Periwinkle Cloak | 62040 | 308 | 308 |
| 78 | 55206 | Eagle Ray Cloak | 62038 | 308 | 308 |
| 78 | 55278 | Inquisition Robes | 48691 | 308 | 308 |
| 78 | 55204 | Caridean Epaulettes | 38694 | 308 | 308 |
| 78 | 55273 | Beauty's Chew Toy | 62026 | 308 | 308 |
| 78 | 35677 | Cosmos Vestments | 48687 | 183 | 183 |
| 78 | 55263 | Renouncer's Cowl | 61935 | 308 | 308 |
| 78 | 55205 | Chromis Chestpiece | 48683 | 308 | 308 |
| 78 | 55229 | Anomuran Helm | 61931 | 308 | 308 |
| 78 | 55207 | Harp Shell Pauldrons | 44100 | 308 | 308 |
| 78 | 55779 | Raz's Pauldrons | 42949 | 308 | 308 |
| 78 | 55274 | Beauty's Plate | 48685 | 308 | 308 |
| 78 | 55249 | Triton Legplates | 62023 | 308 | 308 |
| 78 | 55260 | Alpheus Legguards | 62023 | 308 | 308 |
| 80 | 87477 | Penate's Penance-Procuring Pistol | 44093 | 409 | 409 |
| 80 | 87515 | Ced's Crusher | 48716 | 409 | 409 |
| 80 | 89575 | Handwraps of Serenity | 92948 | 289 | 289 |
| 80 | 87442 | Signet of the Scorned | 62035 | 409 | 409 |
| 80 | 65659 | Kelp Forest Vest | 48689 | 308 | 308 |
| 80 | 66886 | Acid-Eaten Vest | 48687 | 308 | 308 |
| 80 | 87456 | Hexu's Amplifying Helm | 61935 | 409 | 409 |
| 80 | 59783 | Damselfish Shoulderguards | 42951 | 288 | 288 |
| 80 | 87464 | Leggings of the Scorched Man | 62025 | 409 | 409 |
| 80 | 51497 | Wrathful Gladiator's Ringmail Armor | 48683 | 270 | 270 |
| 80 | 87426 | Deathspite Breastplate | 48685 | 409 | 409 |
| 84 | 87508 | Lhakaz's Missing Ribspreader | 93857 | 442 | 442 |
| 84 | 87514 | Sharpened Multi-Prong Dagger | 93857 | 417 | 417 |
| 84 | 87438 | Signet of the Binding Wall | 62037 | 433 | 433 |
| 84 | 87439 | Pestilential Ring | 62035 | 433 | 433 |
| 84 | 87458 | Bone-Inlaid Fan | 93904 | 433 | 433 |
| 84 | 87450 | Hood of Alchemical Vapors | 61958 | 442 | 442 |
| 84 | 87491 | Spaulders of Five Rings | 93859 | 417 | 417 |
| 84 | 87463 | Very Manly Leggings | 62029 | 425 | 425 |
| 84 | 87429 | Drape of Flowing Gauze | 62040 | 442 | 442 |
| 84 | 87493 | Enokian Growth Shoulderguards | 93864 | 425 | 425 |
| 84 | 87492 | Mantle of Bestial Force | 93862 | 417 | 417 |
| 84 | 87425 | Chestguard of the Solicitious Pandaren | 93865 | 442 | 442 |
| 84 | 87462 | Leggings of Perfect Balance | 62026 | 425 | 425 |
| 84 | 87459 | Dills' Primal Leggings | 62024 | 442 | 442 |
| 84 | 87453 | Endwyn's Toothy Visage | 61931 | 433 | 433 |
| 84 | 87461 | Greatheart Pants | 62023 | 425 | 425 |
| 84 | 87460 | Legplates of the Wandering Warrior | 62023 | 417 | 417 |
| 85 | 87474 | Ennadee's Twirling Longbow | 93855 | 450 | 450 |
| 85 | 78471 | Vishanka, Jaws of the Earth | 93855 | 416 | 416 |
| 85 | 73427 | Cataclysmic Gladiator's Rifle | 93841 | 410 | 410 |
| 85 | 87419 | Kaleiki's Lost Training Staff | 93844 | 450 | 450 |
| 85 | 87509 | Fist of Fate | 93858 | 450 | 450 |
| 85 | 87437 | Anaxo's Multi-Layered Seal | 62037 | 450 | 450 |
| 85 | 87604 | Beachcomber's Hat | 61958 | 420 | 420 |
| 85 | 65174 | Corsair's Overshirt | 93860 | 346 | 346 |
| 85 | 78719 | Legwraps of Dying Light | 62029 | 410 | 410 |
| 85 | 87427 | Blade-Dulling Greatcloak | 62038 | 450 | 450 |
| 85 | 87634 | Mazu's Robe | 93860 | 450 | 450 |
| 85 | 87635 | Amber-Starched Robes | 93860 | 450 | 450 |
| 85 | 87452 | Zom's Misty Hood | 61937 | 450 | 450 |
| 85 | 87605 | Crest of the Red Crane | 61942 | 420 | 420 |
| 85 | 87631 | Jiao-Skin Tunic | 93865 | 440 | 440 |
| 85 | 78708 | Blackfang Battleweave Legguards | 62026 | 410 | 410 |
| 85 | 78660 | Deep Earth Robes | 93865 | 410 | 410 |
| 85 | 78662 | Deep Earth Vestment | 93865 | 410 | 410 |
| 85 | 87451 | Helm of Elemental Binding | 61936 | 450 | 450 |
| 85 | 78435 | Imperfect Specimens 27 and 28 | 93876 | 410 | 410 |
| 85 | 78704 | Spiritwalker's Legguards | 62024 | 410 | 410 |
| 85 | 78723 | Spiritwalker's Hauberk | 93885 | 410 | 410 |
| 85 | 78724 | Spiritwalker's Cuirass | 93888 | 410 | 410 |
| 85 | 78367 | Brackenshell Shoulderplates | 93890 | 410 | 410 |
| 85 | 87627 | Kunchong Carapace Chestguard | 93891 | 440 | 440 |
| 85 | 71512 | Immolation Battleplate | 93891 | 391 | 391 |
| 89 | 85183 | Claws of Gekkan | 93858 | 450 | 450 |
| 89 | 80933 | Mantid Trochanter | 93857 | 450 | 450 |
| 89 | 85188 | Blade Trap Signet | 62035 | 450 | 450 |
| 89 | 80920 | Fallout Filter | 61958 | 450 | 450 |
| 89 | 100971 | Breezeswept Hood | 61958 | 450 | 450 |
| 89 | 85194 | Regal Silk Shoulderpads | 93859 | 450 | 450 |
| 89 | 100972 | Whisperwind Spaulders | 93859 | 450 | 450 |
| 89 | 80926 | Leggings of the Frenzy | 62029 | 450 | 450 |
| 89 | 90721 | Cournith Waterstrider's Silken Finery | 62029 | 440 | 440 |
| 89 | 85182 | Cloak of Cleansing Flame | 62040 | 450 | 450 |
| 89 | 80929 | Wall-Breaker Legguards | 62026 | 450 | 450 |
| 89 | 90720 | Silent Leggings of the Ghostpaw | 62026 | 440 | 440 |
| 89 | 80925 | Swarmcall Helm | 61935 | 450 | 450 |
| 89 | 85175 | Crest of the Clan Lords | 61935 | 450 | 450 |
| 89 | 100968 | Galedodger Chestguard | 93888 | 450 | 450 |
| 89 | 90723 | Arness's Scaled Leggings | 62025 | 440 | 440 |
| 89 | 85179 | Meteoric Greathelm | 61931 | 450 | 450 |
| 89 | 100959 | Sightfinder Helm | 61931 | 450 | 450 |
| 89 | 100970 | Spaulders of Immovable Stone | 93890 | 450 | 450 |
| 89 | 100954 | Sap-Encrusted Legplates | 62023 | 450 | 450 |
| 89 | 90717 | Qu'nas' Apocryphal Legplates | 62023 | 440 | 440 |

## Candidate catalogue

Columns: endpoint; item ID/name; item class/subclass; inventory type; item level;
equip minimum; lowest direct quest minimum if present. Numeric types are the core's
ItemClass, ItemSubclass and InventoryType values. Two rows per group retain alternatives;
high item level is a research filter, not proof of best combat performance.

| Level | ID | Item | Class/subclass | Slot | iLvl | Equip min | Quest min |
|---|---|---|---|---|---|---|---|
| 19 | 885 | Black Metal Axe | 2/0 | 13 | 24 | 19 | - |
| 19 | 1459 | Shadowhide Scalper | 2/0 | 13 | 24 | 19 | - |
| 19 | 1455 | Blackrock Champion's Axe | 2/1 | 17 | 24 | 19 | - |
| 19 | 15269 | Massive Battle Axe | 2/1 | 17 | 24 | 19 | - |
| 19 | 3742 | Bow of Plunder | 2/2 | 15 | 28 | 0 | 17 |
| 19 | 20437 | Outrider's Bow | 2/2 | 15 | 23 | 18 | - |
| 19 | 13136 | Lil Timmy's Peashooter | 2/3 | 26 | 21 | 16 | - |
| 19 | 59023 | Loch Modan Musket | 2/3 | 26 | 20 | 0 | 15 |
| 19 | 2256 | Skeletal Club | 2/4 | 13 | 24 | 19 | - |
| 19 | 15223 | Jagged Star | 2/4 | 13 | 24 | 19 | - |
| 19 | 6093 | Orc Crusher | 2/5 | 17 | 27 | 0 | 18 |
| 19 | 5322 | Demolition Hammer | 2/5 | 17 | 26 | 0 | 17 |
| 19 | 5200 | Impaling Harpoon | 2/6 | 17 | 20 | 15 | - |
| 19 | 59580 | Sharp Poker | 2/6 | 17 | 18 | 0 | 14 |
| 19 | 56696 | Legacy of Teronis | 2/7 | 13 | 25 | 0 | 19 |
| 19 | 1482 | Shadowfang | 2/7 | 13 | 24 | 19 | - |
| 19 | 3822 | Runic Darkblade | 2/8 | 17 | 32 | 0 | 19 |
| 19 | 5813 | Emil's Brand | 2/8 | 17 | 32 | 0 | 19 |
| 19 | 890 | Twisted Chanter's Staff | 2/10 | 17 | 24 | 19 | - |
| 19 | 2042 | Staff of Westfall | 2/10 | 17 | 24 | 0 | 14 |
| 19 | 58978 | Filthy Paw | 2/13 | 13 | 11 | 0 | 9 |
| 19 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 19 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 19 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 19 | 56679 | Dissector | 2/15 | 13 | 24 | 0 | 18 |
| 19 | 20441 | Scout's Blade | 2/15 | 13 | 23 | 18 | - |
| 19 | 56720 | Security Crossbow | 2/18 | 26 | 22 | 0 | 19 |
| 19 | 65672 | Crossbow of the Twice-Slain | 2/18 | 26 | 21 | 0 | 19 |
| 19 | 7001 | Gravestone Scepter | 2/19 | 26 | 29 | 0 | 18 |
| 19 | 5250 | Charred Wand | 2/19 | 26 | 28 | 0 | 17 |
| 19 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 19 | 45858 | Nat's Lucky Fishing Pole | 2/20 | 17 | 20 | 0 | 0 |
| 19 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 19 | 20442 | Scout's Medallion | 4/0 | 2 | 23 | 18 | - |
| 19 | 20444 | Sentinel's Medallion | 4/0 | 2 | 23 | 18 | - |
| 19 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 19 | 3760 | Band of the Undercity | 4/0 | 11 | 32 | 0 | 19 |
| 19 | 24118 | Signet of Argas | 4/0 | 11 | 32 | 0 | 19 |
| 19 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 19 | 19979 | Hook of the Master Angler | 4/0 | 12 | 65 | 0 | 1 |
| 19 | 34140 | Dark Iron Tankard | 4/0 | 23 | 59 | 0 | 1 |
| 19 | 1131 | Totem of Infliction | 4/0 | 23 | 25 | 0 | 18 |
| 19 | 19972 | Lucky Fishing Hat | 4/1 | 1 | 40 | 0 | 1 |
| 19 | 5820 | Faerie Mantle | 4/1 | 3 | 32 | 0 | 19 |
| 19 | 3324 | Ghostly Mantle | 4/1 | 3 | 28 | 0 | 18 |
| 19 | 14133 | Ritual Tunic | 4/1 | 5 | 24 | 19 | - |
| 19 | 14175 | Buccaneer's Vest | 4/1 | 5 | 23 | 18 | - |
| 19 | 4708 | Bright Belt | 4/1 | 6 | 24 | 19 | - |
| 19 | 4785 | Brimstone Belt | 4/1 | 6 | 24 | 19 | - |
| 19 | 6282 | Sacred Burial Trousers | 4/1 | 7 | 32 | 0 | 19 |
| 19 | 10043 | Pious Legwraps | 4/1 | 7 | 31 | 0 | 13 |
| 19 | 16985 | Windseeker Boots | 4/1 | 8 | 25 | 0 | 17 |
| 19 | 14374 | Sanguine Sandals | 4/1 | 8 | 24 | 19 | - |
| 19 | 3647 | Bright Bracers | 4/1 | 9 | 23 | 18 | - |
| 19 | 56715 | Oily Bracers | 4/1 | 9 | 22 | 0 | 19 |
| 19 | 10654 | Jutebraid Gloves | 4/1 | 10 | 31 | 0 | 15 |
| 19 | 3066 | Bright Gloves | 4/1 | 10 | 24 | 19 | - |
| 19 | 98336 | Tyrannical Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 19 | 103635 | Prideful Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 19 | 2231 | Inferno Robe | 4/1 | 20 | 40 | 0 | 17 |
| 19 | 7334 | Efflorescent Robe | 4/1 | 20 | 28 | 0 | 17 |
| 19 | 59084 | Boran's Shoulderguards | 4/2 | 3 | 22 | 0 | 19 |
| 19 | 5254 | Rugged Spaulders | 4/2 | 3 | 21 | 16 | - |
| 19 | 28050 | Sacred Feather Vest | 4/2 | 5 | 85 | 0 | 0 |
| 19 | 28051 | Jerkin of the Untamed Spirit | 4/2 | 5 | 85 | 0 | 0 |
| 19 | 5355 | Beastmaster's Girdle | 4/2 | 6 | 28 | 0 | 10 |
| 19 | 16987 | Screecher Belt | 4/2 | 6 | 27 | 0 | 18 |
| 19 | 6587 | Scouting Trousers | 4/2 | 7 | 24 | 19 | - |
| 19 | 15117 | Rigid Leggings | 4/2 | 7 | 24 | 19 | - |
| 19 | 10653 | Trailblazer Boots | 4/2 | 8 | 30 | 0 | 15 |
| 19 | 6335 | Grizzled Boots | 4/2 | 8 | 29 | 0 | 16 |
| 19 | 56694 | Silverwind Bracers | 4/2 | 9 | 25 | 0 | 19 |
| 19 | 3202 | Forest Leather Bracers | 4/2 | 9 | 24 | 19 | - |
| 19 | 14572 | Bristlebark Gloves | 4/2 | 10 | 24 | 19 | - |
| 19 | 6586 | Scouting Gloves | 4/2 | 10 | 23 | 18 | - |
| 19 | 59082 | Merchant Marine Pauldrons | 4/3 | 3 | 22 | 0 | 19 |
| 19 | 65985 | Cleansed Pauldrons | 4/3 | 3 | 21 | 0 | 15 |
| 19 | 28057 | Bonechewer Berserker's Vest | 4/3 | 5 | 85 | 0 | 0 |
| 19 | 1275 | Deputy Chain Coat | 4/3 | 5 | 25 | 0 | 19 |
| 19 | 11229 | Brightscale Girdle | 4/3 | 6 | 32 | 0 | 19 |
| 19 | 56706 | Greenpaw Belt | 4/3 | 6 | 26 | 0 | 19 |
| 19 | 2545 | Malleable Chain Leggings | 4/3 | 7 | 26 | 0 | 18 |
| 19 | 6087 | Chausses of Westfall | 4/3 | 7 | 24 | 0 | 14 |
| 19 | 10658 | Quagmire Galoshes | 4/3 | 8 | 25 | 0 | 10 |
| 19 | 56695 | Wolfrunner Boots | 4/3 | 8 | 25 | 0 | 19 |
| 19 | 4534 | Steel-Clasped Bracers | 4/3 | 9 | 29 | 0 | 16 |
| 19 | 9811 | Fortified Bracers | 4/3 | 9 | 24 | 19 | - |
| 19 | 16986 | Sandspire Gloves | 4/3 | 10 | 25 | 0 | 17 |
| 19 | 12994 | Thorbia's Gauntlets | 4/3 | 10 | 23 | 18 | - |
| 19 | 28054 | Fleshripper's Bladed Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 19 | 28055 | Gilded Crimson Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 19 | 3761 | Deadskull Shield | 4/6 | 14 | 32 | 0 | 19 |
| 19 | 7002 | Arctic Buckler | 4/6 | 14 | 29 | 0 | 18 |
| 29 | 2044 | Crescent of Forlorn Spirits | 2/0 | 13 | 35 | 0 | 22 |
| 29 | 6738 | Bleeding Crescent | 2/0 | 13 | 35 | 0 | 28 |
| 29 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 29 | 12250 | Midnight Axe | 2/1 | 17 | 34 | 29 | - |
| 29 | 2299 | Burning War Axe | 2/1 | 17 | 33 | 28 | - |
| 29 | 3037 | Whipwood Recurve Bow | 2/2 | 15 | 34 | 29 | - |
| 29 | 19561 | Outrider's Bow | 2/2 | 15 | 33 | 28 | - |
| 29 | 17042 | Nail Spitter | 2/3 | 26 | 45 | 0 | 28 |
| 29 | 13137 | Ironweaver | 2/3 | 26 | 34 | 29 | - |
| 29 | 17039 | Skullbreaker | 2/4 | 13 | 45 | 0 | 28 |
| 29 | 6804 | Windstorm Hammer | 2/4 | 13 | 34 | 0 | 25 |
| 29 | 7730 | Cobalt Crusher | 2/5 | 17 | 34 | 29 | - |
| 29 | 1991 | Goblin Power Shovel | 2/5 | 17 | 34 | 29 | - |
| 29 | 13057 | Bloodpike | 2/6 | 17 | 28 | 23 | - |
| 29 | 59200 | Decalcified Elven Spear | 2/6 | 17 | 28 | 0 | 26 |
| 29 | 13033 | Zealot Blade | 2/7 | 13 | 34 | 29 | - |
| 29 | 19553 | Legionnaire's Sword | 2/7 | 13 | 33 | 28 | - |
| 29 | 2000 | Archeus | 2/8 | 17 | 35 | 0 | 25 |
| 29 | 2877 | Combatant Claymore | 2/8 | 17 | 33 | 28 | - |
| 29 | 7710 | Loksey's Training Stick | 2/10 | 17 | 34 | 29 | - |
| 29 | 7713 | Illusionary Rod | 2/10 | 17 | 34 | 29 | - |
| 29 | 7683 | Bloody Brass Knuckles | 2/13 | 13 | 31 | 26 | - |
| 29 | 2942 | Iron Knuckles | 2/13 | 13 | 26 | 21 | - |
| 29 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 29 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 29 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 29 | 7682 | Torturing Poker | 2/15 | 13 | 34 | 29 | - |
| 29 | 7714 | Hypnotic Blade | 2/15 | 13 | 34 | 29 | - |
| 29 | 13037 | Crystalpine Stinger | 2/18 | 26 | 32 | 27 | - |
| 29 | 59242 | Ironsight Crossbow | 2/18 | 26 | 32 | 0 | 29 |
| 29 | 6806 | Dancing Flame | 2/19 | 26 | 40 | 0 | 25 |
| 29 | 5247 | Rod of Sorrow | 2/19 | 26 | 39 | 0 | 29 |
| 29 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 29 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 29 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 29 | 7746 | Explorers' League Commendation | 4/0 | 2 | 41 | 0 | 28 |
| 29 | 5180 | Necklace of Harmony | 4/0 | 2 | 34 | 29 | - |
| 29 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 29 | 6757 | Jaina's Signet Ring | 4/0 | 11 | 38 | 0 | 28 |
| 29 | 34227 | Deadman's Hand | 4/0 | 11 | 36 | 29 | - |
| 29 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 29 | 19979 | Hook of the Master Angler | 4/0 | 12 | 65 | 0 | 1 |
| 29 | 34140 | Dark Iron Tankard | 4/0 | 23 | 59 | 0 | 1 |
| 29 | 7749 | Omega Orb | 4/0 | 23 | 41 | 0 | 28 |
| 29 | 19972 | Lucky Fishing Hat | 4/1 | 1 | 40 | 0 | 1 |
| 29 | 7357 | Elder's Hat | 4/1 | 1 | 34 | 29 | - |
| 29 | 4197 | Berylline Pads | 4/1 | 3 | 35 | 0 | 23 |
| 29 | 3560 | Mantle of Honor | 4/1 | 3 | 35 | 0 | 20 |
| 29 | 9819 | Durable Tunic | 4/1 | 5 | 34 | 29 | - |
| 29 | 14398 | Resilient Tunic | 4/1 | 5 | 33 | 28 | - |
| 29 | 6726 | Razzeric's Customized Seatbelt | 4/1 | 6 | 41 | 0 | 29 |
| 29 | 9853 | Conjurer's Cinch | 4/1 | 6 | 34 | 29 | - |
| 29 | 6737 | Dryleaf Pants | 4/1 | 7 | 35 | 0 | 28 |
| 29 | 7368 | Elder's Pants | 4/1 | 7 | 34 | 29 | - |
| 29 | 3764 | Mantis Boots | 4/1 | 8 | 40 | 0 | 29 |
| 29 | 6791 | Hellion Boots | 4/1 | 8 | 40 | 0 | 27 |
| 29 | 4744 | Arcane Runed Bracers | 4/1 | 9 | 39 | 0 | 29 |
| 29 | 2032 | Gallan Cuffs | 4/1 | 9 | 36 | 0 | 27 |
| 29 | 4040 | Nightsky Gloves | 4/1 | 10 | 34 | 29 | - |
| 29 | 14211 | Vital Handwraps | 4/1 | 10 | 34 | 29 | - |
| 29 | 98336 | Tyrannical Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 29 | 103635 | Prideful Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 29 | 17043 | Zealot's Robe | 4/1 | 20 | 45 | 0 | 28 |
| 29 | 2231 | Inferno Robe | 4/1 | 20 | 40 | 0 | 17 |
| 29 | 4124 | Cap of Harmony | 4/2 | 1 | 40 | 0 | 26 |
| 29 | 6720 | Spirit Hunter Headdress | 4/2 | 1 | 37 | 0 | 26 |
| 29 | 15140 | Cutthroat's Mantle | 4/2 | 3 | 34 | 29 | - |
| 29 | 55910 | Defiant Spire Shoulderguard | 4/2 | 3 | 33 | 0 | 29 |
| 29 | 28050 | Sacred Feather Vest | 4/2 | 5 | 85 | 0 | 0 |
| 29 | 28051 | Jerkin of the Untamed Spirit | 4/2 | 5 | 85 | 0 | 0 |
| 29 | 6409 | Insignia Belt | 4/2 | 6 | 34 | 29 | - |
| 29 | 15148 | Ghostwalker Belt | 4/2 | 6 | 34 | 29 | - |
| 29 | 7414 | Infiltrator Pants | 4/2 | 7 | 34 | 29 | - |
| 29 | 9833 | Scaled Leather Leggings | 4/2 | 7 | 33 | 28 | - |
| 29 | 4055 | Insignia Boots | 4/2 | 8 | 34 | 29 | - |
| 29 | 15350 | Headhunter's Slippers | 4/2 | 8 | 34 | 29 | - |
| 29 | 9857 | Archer's Bracers | 4/2 | 9 | 34 | 29 | - |
| 29 | 6410 | Insignia Bracers | 4/2 | 9 | 33 | 28 | - |
| 29 | 6727 | Razzeric's Racing Grips | 4/2 | 10 | 41 | 0 | 29 |
| 29 | 6732 | Gnomish Mechanic's Gloves | 4/2 | 10 | 38 | 0 | 28 |
| 29 | 15540 | Wicked Chain Helmet | 4/3 | 1 | 34 | 29 | - |
| 29 | 5819 | Sunblaze Coif | 4/3 | 1 | 33 | 28 | - |
| 29 | 3765 | Brigand's Pauldrons | 4/3 | 3 | 40 | 0 | 29 |
| 29 | 6747 | Enforcer Pauldrons | 4/3 | 3 | 40 | 0 | 25 |
| 29 | 28057 | Bonechewer Berserker's Vest | 4/3 | 5 | 85 | 0 | 0 |
| 29 | 15536 | Wicked Chain Chestpiece | 4/3 | 5 | 34 | 29 | - |
| 29 | 6742 | Stonefist Girdle | 4/3 | 6 | 35 | 0 | 23 |
| 29 | 4717 | Mail Combat Belt | 4/3 | 6 | 34 | 29 | - |
| 29 | 7423 | Phalanx Leggings | 4/3 | 7 | 34 | 29 | - |
| 29 | 55911 | Stubborn Legguards | 4/3 | 7 | 33 | 0 | 29 |
| 29 | 7417 | Phalanx Boots | 4/3 | 8 | 34 | 29 | - |
| 29 | 20111 | Highlander's Lamellar Greaves | 4/3 | 8 | 33 | 28 | - |
| 29 | 4745 | War Rider Bracers | 4/3 | 9 | 40 | 0 | 29 |
| 29 | 9865 | Renegade Bracers | 4/3 | 9 | 34 | 29 | - |
| 29 | 4075 | Mail Combat Gauntlets | 4/3 | 10 | 34 | 29 | - |
| 29 | 15560 | Pillager's Gloves | 4/3 | 10 | 34 | 29 | - |
| 29 | 28054 | Fleshripper's Bladed Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 29 | 28055 | Gilded Crimson Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 29 | 7747 | Vile Protector | 4/6 | 14 | 41 | 0 | 28 |
| 29 | 17508 | Forcestone Buckler | 4/6 | 14 | 41 | 0 | 28 |
| 39 | 63634 | Lasting Peace | 2/0 | 13 | 43 | 0 | 38 |
| 39 | 63637 | Lasting Peace | 2/0 | 13 | 43 | 0 | 38 |
| 39 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 39 | 6830 | Bonebiter | 2/1 | 17 | 44 | 0 | 34 |
| 39 | 65973 | Fras Siabi's Cigar Cutter | 2/1 | 17 | 44 | 0 | 39 |
| 39 | 19560 | Outrider's Bow | 2/2 | 15 | 43 | 38 | - |
| 39 | 19564 | Outrunner's Bow | 2/2 | 15 | 43 | 38 | - |
| 39 | 17042 | Nail Spitter | 2/3 | 26 | 45 | 0 | 28 |
| 39 | 3430 | Sniper Rifle | 2/3 | 26 | 44 | 39 | - |
| 39 | 17039 | Skullbreaker | 2/4 | 13 | 45 | 0 | 28 |
| 39 | 63656 | Lakota's Mace | 2/4 | 13 | 45 | 0 | 38 |
| 39 | 9678 | Tok'kar's Murloc Basher | 2/5 | 17 | 43 | 0 | 35 |
| 39 | 1722 | Thornstone Sledgehammer | 2/5 | 17 | 42 | 37 | - |
| 39 | 13058 | Khoo's Point | 2/6 | 17 | 44 | 39 | - |
| 39 | 65924 | Shen'dralar Trident | 2/6 | 17 | 44 | 0 | 39 |
| 39 | 4987 | Dwarf Captain's Sword | 2/7 | 13 | 45 | 0 | 35 |
| 39 | 6802 | Sword of Omen | 2/7 | 13 | 44 | 0 | 30 |
| 39 | 1982 | Nightblade | 2/8 | 17 | 44 | 39 | - |
| 39 | 13051 | Witchfury | 2/8 | 17 | 44 | 39 | - |
| 39 | 4134 | Nimboya's Mystical Staff | 2/10 | 17 | 46 | 0 | 30 |
| 39 | 65994 | Staff of Athen'a | 2/10 | 17 | 44 | 0 | 39 |
| 39 | 17738 | Claw of Celebras | 2/13 | 13 | 37 | 32 | - |
| 39 | 89570 | Initiate's Handwraps | 2/13 | 13 | 35 | 30 | - |
| 39 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 39 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 39 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 39 | 6831 | Black Menace | 2/15 | 13 | 44 | 0 | 34 |
| 39 | 3187 | Sacrificial Kris | 2/15 | 13 | 44 | 39 | - |
| 39 | 62975 | Plaguefix Launcher | 2/18 | 26 | 42 | 0 | 39 |
| 39 | 13038 | Swiftwind | 2/18 | 26 | 40 | 35 | - |
| 39 | 88341 | Necromantic Wand | 2/19 | 26 | 43 | 39 | - |
| 39 | 13064 | Jaina's Firestarter | 2/19 | 26 | 42 | 37 | - |
| 39 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 39 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 39 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 39 | 4743 | Pulsating Crystalline Shard | 4/0 | 2 | 50 | 0 | 30 |
| 39 | 7888 | Jarkal's Enhancing Necklace | 4/0 | 2 | 47 | 0 | 37 |
| 39 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 39 | 19038 | Ring of Subtlety | 4/0 | 11 | 48 | 0 | 39 |
| 39 | 4549 | Seafire Band | 4/0 | 11 | 46 | 0 | 35 |
| 39 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 39 | 19979 | Hook of the Master Angler | 4/0 | 12 | 65 | 0 | 1 |
| 39 | 34140 | Dark Iron Tankard | 4/0 | 23 | 59 | 0 | 1 |
| 39 | 11859 | Jademoon Orb | 4/0 | 23 | 45 | 0 | 37 |
| 39 | 6429 | Mistscape Wizard Hat | 4/1 | 1 | 44 | 39 | - |
| 39 | 14246 | Darkmist Wizard Hat | 4/1 | 1 | 44 | 39 | - |
| 39 | 14247 | Lunar Mantle | 4/1 | 3 | 44 | 39 | - |
| 39 | 62999 | Corpseburner's Mantle | 4/1 | 3 | 44 | 0 | 39 |
| 39 | 14417 | Silksand Tunic | 4/1 | 5 | 44 | 39 | - |
| 39 | 9874 | Sorcerer Drape | 4/1 | 5 | 43 | 38 | - |
| 39 | 4117 | Scorching Sash | 4/1 | 6 | 44 | 0 | 38 |
| 39 | 9906 | Royal Sash | 4/1 | 6 | 44 | 39 | - |
| 39 | 11123 | Rainstrider Leggings | 4/1 | 7 | 55 | 0 | 38 |
| 39 | 7469 | Regal Leggings | 4/1 | 7 | 44 | 39 | - |
| 39 | 9645 | Gnomish Inventor Boots | 4/1 | 8 | 49 | 0 | 30 |
| 39 | 63659 | Starion's Slippers | 4/1 | 8 | 45 | 0 | 38 |
| 39 | 10705 | Firwillow Wristbands | 4/1 | 9 | 47 | 0 | 38 |
| 39 | 11469 | Bloodband Bracers | 4/1 | 9 | 46 | 0 | 31 |
| 39 | 9634 | Skilled Handling Gloves | 4/1 | 10 | 46 | 0 | 38 |
| 39 | 9910 | Royal Gloves | 4/1 | 10 | 44 | 39 | - |
| 39 | 98336 | Tyrannical Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 39 | 103635 | Prideful Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 39 | 19129 | Everglowing Robe | 4/1 | 20 | 49 | 0 | 30 |
| 39 | 17043 | Zealot's Robe | 4/1 | 20 | 45 | 0 | 28 |
| 39 | 19039 | Zorbin's Water Resistant Hat | 4/2 | 1 | 48 | 0 | 35 |
| 39 | 63664 | Withering Mask | 4/2 | 1 | 45 | 0 | 38 |
| 39 | 9647 | Failed Flying Experiment | 4/2 | 3 | 48 | 0 | 35 |
| 39 | 11502 | Loreskin Shoulders | 4/2 | 3 | 46 | 0 | 38 |
| 39 | 28050 | Sacred Feather Vest | 4/2 | 5 | 85 | 0 | 0 |
| 39 | 28051 | Jerkin of the Untamed Spirit | 4/2 | 5 | 85 | 0 | 0 |
| 39 | 63020 | Belt of the Gentle Lady | 4/2 | 6 | 45 | 0 | 39 |
| 39 | 63658 | Bloodstained Twilight Belt | 4/2 | 6 | 45 | 0 | 38 |
| 39 | 63668 | Leggings of the Thousand Needles | 4/2 | 7 | 45 | 0 | 38 |
| 39 | 63674 | Leggings of the Thousand Needles | 4/2 | 7 | 45 | 0 | 38 |
| 39 | 9630 | Pratt's Handcrafted Boots | 4/2 | 8 | 45 | 0 | 34 |
| 39 | 9633 | Jangdor's Handcrafted Boots | 4/2 | 8 | 45 | 0 | 34 |
| 39 | 63016 | Corpsebeast Armbands | 4/2 | 9 | 45 | 0 | 39 |
| 39 | 63660 | Codemaster's Cuffs | 4/2 | 9 | 45 | 0 | 38 |
| 39 | 9631 | Pratt's Handcrafted Gloves | 4/2 | 10 | 45 | 0 | 34 |
| 39 | 9632 | Jangdor's Handcrafted Gloves | 4/2 | 10 | 45 | 0 | 34 |
| 39 | 11124 | Helm of Exile | 4/3 | 1 | 55 | 0 | 38 |
| 39 | 9932 | Brigade Circlet | 4/3 | 1 | 44 | 39 | - |
| 39 | 63671 | Pauldrons of the Thousand Needles | 4/3 | 3 | 45 | 0 | 38 |
| 39 | 63677 | Pauldrons of the Thousand Needles | 4/3 | 3 | 45 | 0 | 38 |
| 39 | 28057 | Bonechewer Berserker's Vest | 4/3 | 5 | 85 | 0 | 0 |
| 39 | 63670 | Chestguard of the Thousand Needles | 4/3 | 5 | 45 | 0 | 38 |
| 39 | 10706 | Nightscale Girdle | 4/3 | 6 | 47 | 0 | 38 |
| 39 | 6425 | Blackforge Girdle | 4/3 | 6 | 44 | 39 | - |
| 39 | 63017 | Legwraps of the Slain | 4/3 | 7 | 45 | 0 | 39 |
| 39 | 63665 | Dirty Work Greaves | 4/3 | 7 | 45 | 0 | 38 |
| 39 | 4653 | Ironheel Boots | 4/3 | 8 | 45 | 0 | 35 |
| 39 | 15599 | Ancient Greaves | 4/3 | 8 | 44 | 39 | - |
| 39 | 65950 | Shackles of Punishment | 4/3 | 9 | 44 | 0 | 38 |
| 39 | 88337 | Shadow Puppet Bracers | 4/3 | 9 | 43 | 39 | - |
| 39 | 63021 | Gloves of Journey's End | 4/3 | 10 | 45 | 0 | 39 |
| 39 | 63661 | Fastfuse Gloves | 4/3 | 10 | 45 | 0 | 38 |
| 39 | 63672 | Helm of the Thousand Needles | 4/4 | 1 | 45 | 0 | 38 |
| 39 | 63678 | Helm of the Thousand Needles | 4/4 | 1 | 45 | 0 | 38 |
| 39 | 19037 | Emerald Peak Spaulders | 4/4 | 3 | 48 | 0 | 39 |
| 39 | 88357 | Vigorsteel Spaulders | 4/4 | 3 | 43 | 39 | - |
| 39 | 28054 | Fleshripper's Bladed Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 39 | 28055 | Gilded Crimson Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 39 | 62998 | Barthalomew's Belt | 4/4 | 6 | 43 | 0 | 39 |
| 39 | 62950 | Joseph's Spare Belt | 4/4 | 6 | 41 | 0 | 39 |
| 39 | 63636 | Legguards of Disfavor | 4/4 | 7 | 43 | 0 | 38 |
| 39 | 63639 | Legguards of Disfavor | 4/4 | 7 | 43 | 0 | 38 |
| 39 | 9646 | Gnomish Water Sinking Device | 4/4 | 8 | 49 | 0 | 30 |
| 39 | 88348 | Wraithplate Treads | 4/4 | 8 | 43 | 39 | - |
| 39 | 63666 | Dupe's Bracers | 4/4 | 9 | 45 | 0 | 38 |
| 39 | 63627 | Drinker's Bracers | 4/4 | 9 | 42 | 0 | 38 |
| 39 | 63651 | Trapper Punchers | 4/4 | 10 | 44 | 0 | 38 |
| 39 | 88353 | Dark Blaze Gauntlets | 4/4 | 10 | 43 | 39 | - |
| 39 | 4652 | Salbac Shield | 4/6 | 14 | 45 | 0 | 35 |
| 39 | 63657 | Well Hammered Shield | 4/6 | 14 | 45 | 0 | 38 |
| 49 | 13014 | Axe of Rin'ji | 2/0 | 13 | 53 | 48 | - |
| 49 | 18737 | Bone Slicing Hatchet | 2/0 | 13 | 53 | 48 | - |
| 49 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 49 | 53584 | Beastkiller | 2/1 | 17 | 53 | 0 | 49 |
| 49 | 10652 | Will of the Mountain Giant | 2/1 | 17 | 51 | 0 | 40 |
| 49 | 2824 | Hurricane | 2/2 | 15 | 53 | 48 | - |
| 49 | 18729 | Screeching Bow | 2/2 | 15 | 53 | 48 | - |
| 49 | 11629 | Houndmaster's Rifle | 2/3 | 26 | 52 | 47 | - |
| 49 | 53569 | Confiscated Poacher's Gun | 2/3 | 26 | 52 | 0 | 49 |
| 49 | 11120 | Belgrom's Hammer | 2/4 | 13 | 55 | 0 | 44 |
| 49 | 810 | Hammer of the Northern Wind | 2/4 | 13 | 54 | 49 | - |
| 49 | 2915 | Taran Icebreaker | 2/5 | 17 | 52 | 47 | - |
| 49 | 11803 | Force of Magma | 2/5 | 17 | 52 | 47 | - |
| 49 | 18725 | Peacemaker | 2/6 | 17 | 53 | 48 | - |
| 49 | 13059 | Stoneraven | 2/6 | 17 | 52 | 47 | - |
| 49 | 10696 | Enchanted Azsharite Felbane Sword | 2/7 | 13 | 60 | 0 | 45 |
| 49 | 15705 | Tidecrest Blade | 2/7 | 13 | 57 | 0 | 49 |
| 49 | 11086 | Jang'thraze the Protector | 2/7 | 21 | 49 | 44 | - |
| 49 | 15254 | Dark Espadon | 2/8 | 17 | 54 | 49 | - |
| 49 | 13505 | Runeblade of Baron Rivendare | 2/8 | 17 | 53 | 48 | - |
| 49 | 10698 | Enchanted Azsharite Felbane Staff | 2/10 | 17 | 60 | 0 | 45 |
| 49 | 812 | Glowing Brightwood Staff | 2/10 | 17 | 54 | 49 | - |
| 49 | 13399 | Gargoyle Shredder Talons | 2/13 | 13 | 53 | 48 | - |
| 49 | 11863 | White Bone Shredder | 2/13 | 13 | 52 | 0 | 48 |
| 49 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 49 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 49 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 49 | 10697 | Enchanted Azsharite Felbane Dagger | 2/15 | 13 | 60 | 0 | 45 |
| 49 | 15706 | Hunt Tracker Blade | 2/15 | 13 | 57 | 0 | 49 |
| 49 | 18392 | Distracting Dagger | 2/15 | 22 | 45 | 40 | - |
| 49 | 18738 | Carapace Spine Crossbow | 2/18 | 26 | 53 | 48 | - |
| 49 | 13039 | Skull Splitting Crossbow | 2/18 | 26 | 48 | 43 | - |
| 49 | 13396 | Skul's Ghastly Touch | 2/19 | 26 | 53 | 48 | - |
| 49 | 13534 | Banshee Finger | 2/19 | 26 | 53 | 48 | - |
| 49 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 49 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 49 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 49 | 10778 | Necklace of Sanctuary | 4/0 | 2 | 60 | 0 | 45 |
| 49 | 11196 | Mindburst Medallion | 4/0 | 2 | 57 | 0 | 40 |
| 49 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 49 | 12038 | Lagrave's Seal | 4/0 | 11 | 56 | 0 | 48 |
| 49 | 12544 | Thrall's Resolve | 4/0 | 11 | 56 | 0 | 48 |
| 49 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 49 | 19979 | Hook of the Master Angler | 4/0 | 12 | 65 | 0 | 1 |
| 49 | 34140 | Dark Iron Tankard | 4/0 | 23 | 59 | 0 | 1 |
| 49 | 11904 | Spirit of Aquementas | 4/0 | 23 | 56 | 0 | 47 |
| 49 | 3075 | Eye of Flame | 4/1 | 1 | 55 | 49 | - |
| 49 | 10751 | Gemburst Circlet | 4/1 | 1 | 54 | 0 | 40 |
| 49 | 14452 | Highborne Pauldrons | 4/1 | 3 | 54 | 49 | - |
| 49 | 13374 | Soulstealer Mantle | 4/1 | 3 | 53 | 48 | - |
| 49 | 14437 | Venomshroud Vest | 4/1 | 5 | 54 | 49 | - |
| 49 | 9946 | Abjurer's Tunic | 4/1 | 5 | 53 | 48 | - |
| 49 | 15707 | Brantwood Sash | 4/1 | 6 | 58 | 0 | 48 |
| 49 | 10103 | Councillor's Sash | 4/1 | 6 | 54 | 49 | - |
| 49 | 11123 | Rainstrider Leggings | 4/1 | 7 | 55 | 0 | 38 |
| 49 | 8251 | Imperial Red Pants | 4/1 | 7 | 54 | 49 | - |
| 49 | 11865 | Rancor Boots | 4/1 | 8 | 56 | 0 | 48 |
| 49 | 11908 | Archaeologist's Quarry Boots | 4/1 | 8 | 55 | 0 | 49 |
| 49 | 11917 | Shizzle's Nozzle Wiper | 4/1 | 9 | 55 | 0 | 49 |
| 49 | 10096 | Councillor's Cuffs | 4/1 | 9 | 54 | 49 | - |
| 49 | 21318 | Earth Warder's Gloves | 4/1 | 10 | 58 | 0 | 45 |
| 49 | 10176 | Mystical Gloves | 4/1 | 10 | 54 | 49 | - |
| 49 | 98336 | Tyrannical Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 49 | 103635 | Prideful Gladiator's Greatcloak | 4/1 | 16 | 90 | 1 | - |
| 49 | 9649 | Royal Highmark Vestments | 4/1 | 20 | 55 | 0 | 40 |
| 49 | 14445 | Venomshroud Silk Robes | 4/1 | 20 | 54 | 49 | - |
| 49 | 21317 | Helm of the Pathfinder | 4/2 | 1 | 55 | 0 | 45 |
| 49 | 8261 | Serpentskin Helm | 4/2 | 1 | 54 | 49 | - |
| 49 | 11963 | Penance Spaulders | 4/2 | 3 | 56 | 0 | 48 |
| 49 | 11874 | Clouddrift Mantle | 4/2 | 3 | 56 | 0 | 49 |
| 49 | 28050 | Sacred Feather Vest | 4/2 | 5 | 85 | 0 | 0 |
| 49 | 28051 | Jerkin of the Untamed Spirit | 4/2 | 5 | 85 | 0 | 0 |
| 49 | 11909 | Excavator's Utility Belt | 4/2 | 6 | 55 | 0 | 49 |
| 49 | 10109 | Wanderer's Belt | 4/2 | 6 | 54 | 49 | - |
| 49 | 12041 | Windshear Leggings | 4/2 | 7 | 56 | 0 | 48 |
| 49 | 45052 | Gruffscale Leggings | 4/2 | 7 | 55 | 48 | - |
| 49 | 13111 | Sandals of the Insurgent | 4/2 | 8 | 54 | 49 | - |
| 49 | 10183 | Swashbuckler's Boots | 4/2 | 8 | 54 | 49 | - |
| 49 | 10107 | Wanderer's Bracers | 4/2 | 9 | 54 | 49 | - |
| 49 | 13537 | Chillhide Bracers | 4/2 | 9 | 53 | 48 | - |
| 49 | 15708 | Blight Leather Gloves | 4/2 | 10 | 58 | 0 | 48 |
| 49 | 21319 | Gloves of the Pathfinder | 4/2 | 10 | 58 | 0 | 45 |
| 49 | 11747 | Flamestrider Robes | 4/2 | 20 | 52 | 47 | - |
| 49 | 12018 | Conservator Helm | 4/3 | 1 | 56 | 0 | 48 |
| 49 | 11124 | Helm of Exile | 4/3 | 1 | 55 | 0 | 38 |
| 49 | 15656 | Merciless Epaulets | 4/3 | 3 | 54 | 49 | - |
| 49 | 13538 | Windshrieker Pauldrons | 4/3 | 3 | 53 | 48 | - |
| 49 | 28057 | Bonechewer Berserker's Vest | 4/3 | 5 | 85 | 0 | 0 |
| 49 | 21320 | Vest of the Den Watcher | 4/3 | 5 | 58 | 0 | 45 |
| 49 | 15654 | Merciless Belt | 4/3 | 6 | 54 | 49 | - |
| 49 | 20088 | Highlander's Chain Girdle | 4/3 | 6 | 53 | 48 | - |
| 49 | 11882 | Outrider Leggings | 4/3 | 7 | 56 | 0 | 48 |
| 49 | 10199 | Crusader's Leggings | 4/3 | 7 | 54 | 49 | - |
| 49 | 15694 | Merciless Greaves | 4/3 | 8 | 54 | 49 | - |
| 49 | 20091 | Highlander's Chain Greaves | 4/3 | 8 | 53 | 48 | - |
| 49 | 12112 | Crypt Demon Bracers | 4/3 | 9 | 56 | 0 | 48 |
| 49 | 10126 | Ornate Bracers | 4/3 | 9 | 54 | 49 | - |
| 49 | 11918 | Grotslab Gloves | 4/3 | 10 | 55 | 0 | 47 |
| 49 | 12051 | Brazen Gauntlets | 4/3 | 10 | 54 | 0 | 48 |
| 49 | 10749 | Avenguard Helm | 4/4 | 1 | 54 | 0 | 40 |
| 49 | 8317 | Alabaster Plate Helmet | 4/4 | 1 | 54 | 49 | - |
| 49 | 14929 | Lofty Shoulder Pads | 4/4 | 3 | 54 | 49 | - |
| 49 | 10245 | Heavy Lamellar Pauldrons | 4/4 | 3 | 53 | 48 | - |
| 49 | 28054 | Fleshripper's Bladed Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 49 | 28055 | Gilded Crimson Chestplate | 4/4 | 5 | 85 | 0 | 0 |
| 49 | 12115 | Stalwart Clutch | 4/4 | 6 | 56 | 0 | 48 |
| 49 | 15709 | Gearforge Girdle | 4/4 | 6 | 56 | 0 | 48 |
| 49 | 21316 | Leggings of the Ursa | 4/4 | 7 | 55 | 0 | 45 |
| 49 | 10244 | Heavy Lamellar Leggings | 4/4 | 7 | 54 | 49 | - |
| 49 | 12021 | Shieldplate Sabatons | 4/4 | 8 | 56 | 0 | 48 |
| 49 | 11919 | Cragplate Greaves | 4/4 | 8 | 55 | 0 | 47 |
| 49 | 10171 | Templar Bracers | 4/4 | 9 | 54 | 49 | - |
| 49 | 53593 | Sticky Bracers | 4/4 | 9 | 54 | 0 | 49 |
| 49 | 13539 | Banshee's Touch | 4/4 | 10 | 53 | 48 | - |
| 49 | 18722 | Death Grips | 4/4 | 10 | 53 | 48 | - |
| 49 | 10686 | Aegis of Battle | 4/6 | 14 | 55 | 0 | 40 |
| 49 | 11915 | Shizzle's Drizzle Blocker | 4/6 | 14 | 55 | 0 | 49 |
| 59 | 25200 | Jagged Broadaxe | 2/0 | 13 | 87 | 59 | - |
| 59 | 25199 | Knight's War Axe | 2/0 | 13 | 84 | 58 | - |
| 59 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 59 | 25214 | Mok'Nathal Battleaxe | 2/1 | 17 | 87 | 59 | - |
| 59 | 25213 | Fel-Touched Axe | 2/1 | 17 | 84 | 58 | - |
| 59 | 25242 | Telaari Longbow | 2/2 | 15 | 87 | 59 | - |
| 59 | 25241 | Ashenvale Longbow | 2/2 | 15 | 84 | 58 | - |
| 59 | 25270 | Gnomish Assault Rifle | 2/3 | 26 | 87 | 59 | - |
| 59 | 25269 | Longbeard Rifle | 2/3 | 26 | 84 | 58 | - |
| 59 | 25116 | Pneumatic War Hammer | 2/4 | 13 | 87 | 59 | - |
| 59 | 25312 | Glorious Scepter | 2/4 | 13 | 87 | 59 | - |
| 59 | 25130 | Gronn-Bone Club | 2/5 | 17 | 87 | 59 | - |
| 59 | 25129 | Giant's Leg Bone | 2/5 | 17 | 84 | 58 | - |
| 59 | 25228 | Halberd Polearm | 2/6 | 17 | 87 | 59 | - |
| 59 | 25227 | Sha'tari Longspear | 2/6 | 17 | 84 | 58 | - |
| 59 | 25144 | Skettis Curved Blade | 2/7 | 13 | 87 | 59 | - |
| 59 | 29108 | Blade of the Unyielding | 2/7 | 13 | 87 | 0 | 58 |
| 59 | 12940 | Dal'Rend's Sacred Charge | 2/7 | 21 | 60 | 55 | - |
| 59 | 11086 | Jang'thraze the Protector | 2/7 | 21 | 49 | 44 | - |
| 59 | 12939 | Dal'Rend's Tribal Guardian | 2/7 | 22 | 60 | 55 | - |
| 59 | 25158 | Skeletal Broadsword | 2/8 | 17 | 87 | 59 | - |
| 59 | 25157 | Serpentlord Claymore | 2/8 | 17 | 84 | 58 | - |
| 59 | 29399 | Rod of the Void Caller | 2/10 | 17 | 93 | 0 | 58 |
| 59 | 25172 | Jinbali Warp-Staff | 2/10 | 17 | 87 | 59 | - |
| 59 | 25186 | Vampiric Handscythes | 2/13 | 13 | 87 | 59 | - |
| 59 | 25185 | Thrasher Blades | 2/13 | 13 | 84 | 58 | - |
| 59 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 59 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 59 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 59 | 25102 | Jaedenis Dagger | 2/15 | 13 | 87 | 59 | - |
| 59 | 25298 | Combustion Dagger | 2/15 | 13 | 87 | 59 | - |
| 59 | 29914 | Hellfire Skiver | 2/15 | 22 | 81 | 0 | 58 |
| 59 | 18392 | Distracting Dagger | 2/15 | 22 | 45 | 40 | - |
| 59 | 25255 | Ram's Head Crossbow | 2/18 | 26 | 84 | 58 | - |
| 59 | 25254 | Tower Crossbow | 2/18 | 26 | 81 | 57 | - |
| 59 | 25284 | Purpleheart Wand | 2/19 | 26 | 87 | 59 | - |
| 59 | 25283 | Crystallized Ebony Wand | 2/19 | 26 | 84 | 58 | - |
| 59 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 59 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 59 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 59 | 27735 | Pendant of the Marsh | 4/0 | 2 | 93 | 0 | 59 |
| 59 | 25714 | Crimson Pendant of Clarity | 4/0 | 2 | 88 | 0 | 59 |
| 59 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 59 | 27733 | Warden's Ring of Precision | 4/0 | 11 | 93 | 0 | 59 |
| 59 | 27734 | Expedition Caster's Band | 4/0 | 11 | 93 | 0 | 59 |
| 59 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 59 | 25786 | Hypnotist's Watch | 4/0 | 12 | 93 | 0 | 58 |
| 59 | 25088 | Laughing Skull Orb | 4/0 | 23 | 87 | 59 | - |
| 59 | 31715 | Demoniac Soul Prison | 4/0 | 23 | 87 | 0 | 58 |
| 59 | 24601 | Starfire Circlet | 4/1 | 1 | 87 | 59 | - |
| 59 | 24593 | Fireheart Skullcap | 4/1 | 1 | 84 | 58 | - |
| 59 | 24603 | Starfire Mantle | 4/1 | 3 | 87 | 59 | - |
| 59 | 25718 | Mantle of Magical Might | 4/1 | 3 | 85 | 0 | 59 |
| 59 | 31717 | Shadowcast Tunic | 4/1 | 5 | 88 | 0 | 58 |
| 59 | 24599 | Starfire Vest | 4/1 | 5 | 87 | 59 | - |
| 59 | 25483 | Fine Sash | 4/1 | 6 | 93 | 0 | 58 |
| 59 | 24597 | Starfire Sash | 4/1 | 6 | 87 | 59 | - |
| 59 | 25711 | Deadly Borer Leggings | 4/1 | 7 | 88 | 0 | 59 |
| 59 | 24602 | Starfire Trousers | 4/1 | 7 | 87 | 59 | - |
| 59 | 27728 | Cushy Cenarion Walkers | 4/1 | 8 | 93 | 0 | 58 |
| 59 | 24598 | Starfire Sandals | 4/1 | 8 | 87 | 59 | - |
| 59 | 24604 | Starfire Wristwraps | 4/1 | 9 | 87 | 59 | - |
| 59 | 24596 | Fireheart Bracers | 4/1 | 9 | 84 | 58 | - |
| 59 | 24600 | Starfire Gloves | 4/1 | 10 | 87 | 59 | - |
| 59 | 24592 | Fireheart Gloves | 4/1 | 10 | 84 | 58 | - |
| 59 | 27730 | Watcher's Cloak of Vigilance | 4/1 | 16 | 93 | 0 | 59 |
| 59 | 29400 | Abyssal Shroud | 4/1 | 16 | 93 | 0 | 58 |
| 59 | 20033 | Zandalar Demoniac's Robe | 4/1 | 20 | 65 | 0 | 58 |
| 59 | 20034 | Zandalar Illusionist's Robe | 4/1 | 20 | 65 | 0 | 58 |
| 59 | 25710 | Moonkin Headdress | 4/2 | 1 | 88 | 0 | 59 |
| 59 | 24713 | Vengeance Helm | 4/2 | 1 | 87 | 59 | - |
| 59 | 27724 | Wild Shoulderpads | 4/2 | 3 | 93 | 0 | 58 |
| 59 | 24715 | Vengeance Pauldrons | 4/2 | 3 | 87 | 59 | - |
| 59 | 31718 | Darkstorm Tunic | 4/2 | 5 | 88 | 0 | 58 |
| 59 | 24711 | Vengeance Chestpiece | 4/2 | 5 | 87 | 59 | - |
| 59 | 27723 | Belt of the Moonkin | 4/2 | 6 | 93 | 0 | 58 |
| 59 | 24709 | Vengeance Belt | 4/2 | 6 | 87 | 59 | - |
| 59 | 24714 | Vengeance Legguards | 4/2 | 7 | 87 | 59 | - |
| 59 | 24706 | Haal'eshi Leggings | 4/2 | 7 | 84 | 58 | - |
| 59 | 27727 | Swift Cenarion Footwear | 4/2 | 8 | 93 | 0 | 58 |
| 59 | 24710 | Vengeance Boots | 4/2 | 8 | 87 | 59 | - |
| 59 | 24716 | Vengeance Bands | 4/2 | 9 | 87 | 59 | - |
| 59 | 24708 | Haal'eshi Bindings | 4/2 | 9 | 84 | 58 | - |
| 59 | 24712 | Vengeance Gloves | 4/2 | 10 | 87 | 59 | - |
| 59 | 24704 | Haal'eshi Gloves | 4/2 | 10 | 84 | 58 | - |
| 59 | 11747 | Flamestrider Robes | 4/2 | 20 | 52 | 47 | - |
| 59 | 24826 | Netherstalker Helmet | 4/3 | 1 | 87 | 59 | - |
| 59 | 24818 | Felstone Helm | 4/3 | 1 | 84 | 58 | - |
| 59 | 24828 | Netherstalker Mantle | 4/3 | 3 | 87 | 59 | - |
| 59 | 24820 | Felstone Spaulders | 4/3 | 3 | 84 | 58 | - |
| 59 | 31719 | Stormstrike Vest | 4/3 | 5 | 88 | 0 | 58 |
| 59 | 24824 | Netherstalker Armor | 4/3 | 5 | 87 | 59 | - |
| 59 | 27726 | Hearty Cenarion Cincture | 4/3 | 6 | 93 | 0 | 58 |
| 59 | 24822 | Netherstalker Belt | 4/3 | 6 | 87 | 59 | - |
| 59 | 25702 | Scaled Legs of Ruination | 4/3 | 7 | 88 | 0 | 59 |
| 59 | 24827 | Netherstalker Legguards | 4/3 | 7 | 87 | 59 | - |
| 59 | 25482 | Venn'ren's Boots | 4/3 | 8 | 93 | 0 | 58 |
| 59 | 24823 | Netherstalker Greaves | 4/3 | 8 | 87 | 59 | - |
| 59 | 24829 | Netherstalker Bracer | 4/3 | 9 | 87 | 59 | - |
| 59 | 24821 | Felstone Bindings | 4/3 | 9 | 84 | 58 | - |
| 59 | 27722 | Gloves of Marshmanship | 4/3 | 10 | 93 | 0 | 58 |
| 59 | 24825 | Netherstalker Gloves | 4/3 | 10 | 87 | 59 | - |
| 59 | 24938 | Darkcrest Helm | 4/4 | 1 | 87 | 59 | - |
| 59 | 24930 | Ango'rosh Helm | 4/4 | 1 | 84 | 58 | - |
| 59 | 27725 | Expedition Mantle | 4/4 | 3 | 93 | 0 | 58 |
| 59 | 24940 | Darkcrest Pauldrons | 4/4 | 3 | 87 | 59 | - |
| 59 | 25701 | Breastplate of Retribution | 4/4 | 5 | 88 | 0 | 59 |
| 59 | 31720 | Battlemaster's Breastplate | 4/4 | 5 | 88 | 0 | 58 |
| 59 | 24934 | Darkcrest Belt | 4/4 | 6 | 87 | 59 | - |
| 59 | 24926 | Ango'rosh Belt | 4/4 | 6 | 84 | 58 | - |
| 59 | 24939 | Darkcrest Legguards | 4/4 | 7 | 87 | 59 | - |
| 59 | 25979 | Flintlocke's Piloting Pants | 4/4 | 7 | 87 | 0 | 58 |
| 59 | 27721 | Expedition Footgear | 4/4 | 8 | 93 | 0 | 58 |
| 59 | 24935 | Darkcrest Sabatons | 4/4 | 8 | 87 | 59 | - |
| 59 | 24941 | Darkcrest Bracers | 4/4 | 9 | 87 | 59 | - |
| 59 | 24933 | Ango'rosh Vambraces | 4/4 | 9 | 84 | 58 | - |
| 59 | 25481 | Sunstrider's Gauntlets | 4/4 | 10 | 93 | 0 | 58 |
| 59 | 24937 | Darkcrest Gauntlets | 4/4 | 10 | 87 | 59 | - |
| 59 | 25074 | Telaari Shield | 4/6 | 14 | 87 | 59 | - |
| 59 | 25073 | Emperor Shield | 4/6 | 14 | 84 | 58 | - |
| 60 | 25201 | Reaver's Sickle | 2/0 | 13 | 90 | 60 | - |
| 60 | 25200 | Jagged Broadaxe | 2/0 | 13 | 87 | 59 | - |
| 60 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 60 | 25215 | Spiked Battle Axe | 2/1 | 17 | 90 | 60 | - |
| 60 | 25214 | Mok'Nathal Battleaxe | 2/1 | 17 | 87 | 59 | - |
| 60 | 25496 | Mag'har Bow | 2/2 | 15 | 93 | 0 | 60 |
| 60 | 25243 | Windtalker Bow | 2/2 | 15 | 90 | 60 | - |
| 60 | 25271 | Croc-Hunter's Rifle | 2/3 | 26 | 90 | 60 | - |
| 60 | 25270 | Gnomish Assault Rifle | 2/3 | 26 | 87 | 59 | - |
| 60 | 25492 | Earthcaller's Mace | 2/4 | 13 | 93 | 0 | 60 |
| 60 | 25519 | Warden's Hammer | 2/4 | 13 | 93 | 0 | 60 |
| 60 | 25131 | Hateful Bludgeon | 2/5 | 17 | 90 | 60 | - |
| 60 | 25130 | Gronn-Bone Club | 2/5 | 17 | 87 | 59 | - |
| 60 | 25229 | Partisan Polearm | 2/6 | 17 | 90 | 60 | - |
| 60 | 25228 | Halberd Polearm | 2/6 | 17 | 87 | 59 | - |
| 60 | 25915 | Fallen Vindicator's Blade | 2/7 | 13 | 93 | 0 | 60 |
| 60 | 25920 | Sedai's Blade | 2/7 | 13 | 93 | 0 | 60 |
| 60 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 60 | 12940 | Dal'Rend's Sacred Charge | 2/7 | 21 | 60 | 55 | - |
| 60 | 19866 | Warblade of the Hakkari | 2/7 | 22 | 66 | 60 | - |
| 60 | 12939 | Dal'Rend's Tribal Guardian | 2/7 | 22 | 60 | 55 | - |
| 60 | 25158 | Skeletal Broadsword | 2/8 | 17 | 87 | 59 | - |
| 60 | 31134 | Blade of Misfortune | 2/8 | 17 | 85 | 60 | - |
| 60 | 25494 | Totemic Staff | 2/10 | 17 | 93 | 0 | 60 |
| 60 | 29399 | Rod of the Void Caller | 2/10 | 17 | 93 | 0 | 58 |
| 60 | 89573 | Handwraps of Meditation | 2/13 | 13 | 93 | 60 | - |
| 60 | 25187 | Shekketh Talons | 2/13 | 13 | 90 | 60 | - |
| 60 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 60 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 60 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 60 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 60 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 60 | 25495 | Wolfrider's Dagger | 2/15 | 13 | 93 | 0 | 60 |
| 60 | 25103 | Nightstalker Dagger | 2/15 | 13 | 90 | 60 | - |
| 60 | 29914 | Hellfire Skiver | 2/15 | 22 | 81 | 0 | 58 |
| 60 | 18392 | Distracting Dagger | 2/15 | 22 | 45 | 40 | - |
| 60 | 25257 | Citadel Crossbow | 2/18 | 26 | 90 | 60 | - |
| 60 | 25255 | Ram's Head Crossbow | 2/18 | 26 | 84 | 58 | - |
| 60 | 25284 | Purpleheart Wand | 2/19 | 26 | 87 | 59 | - |
| 60 | 25283 | Crystallized Ebony Wand | 2/19 | 26 | 84 | 58 | - |
| 60 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 60 | 19970 | Arcanite Fishing Pole | 2/20 | 17 | 20 | 0 | 1 |
| 60 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 60 | 25484 | Telhamat Pendant | 4/0 | 2 | 93 | 0 | 60 |
| 60 | 25487 | Wind Dancer's Pendant | 4/0 | 2 | 93 | 0 | 60 |
| 60 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 60 | 25485 | Amaan's Signet | 4/0 | 11 | 93 | 0 | 60 |
| 60 | 25488 | Signet of Aeranas | 4/0 | 11 | 93 | 0 | 60 |
| 60 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 60 | 25620 | Ancient Crystal Talisman | 4/0 | 12 | 96 | 0 | 60 |
| 60 | 25089 | Supplicant's Rod | 4/0 | 23 | 90 | 60 | - |
| 60 | 25088 | Laughing Skull Orb | 4/0 | 23 | 87 | 59 | - |
| 60 | 23587 | Mirren's Drinking Hat | 4/1 | 1 | 90 | 0 | 60 |
| 60 | 24609 | Laughing Skull Cap | 4/1 | 1 | 90 | 60 | - |
| 60 | 24611 | Laughing Skull Shoulderpads | 4/1 | 3 | 90 | 60 | - |
| 60 | 31659 | Researcher's Mantle | 4/1 | 3 | 90 | 0 | 60 |
| 60 | 24607 | Laughing Skull Tunic | 4/1 | 5 | 90 | 60 | - |
| 60 | 31657 | Chemise of Rebirth | 4/1 | 5 | 90 | 0 | 60 |
| 60 | 25480 | Wastewalker's Sash | 4/1 | 6 | 93 | 0 | 60 |
| 60 | 25483 | Fine Sash | 4/1 | 6 | 93 | 0 | 58 |
| 60 | 24610 | Laughing Skull Pants | 4/1 | 7 | 90 | 60 | - |
| 60 | 22497 | Frostfire Leggings | 4/1 | 7 | 88 | 60 | 60 |
| 60 | 27728 | Cushy Cenarion Walkers | 4/1 | 8 | 93 | 0 | 58 |
| 60 | 24606 | Laughing Skull Boots | 4/1 | 8 | 90 | 60 | - |
| 60 | 31770 | Marsh Bracers | 4/1 | 9 | 96 | 0 | 60 |
| 60 | 24612 | Laughing Skull Bracelets | 4/1 | 9 | 90 | 60 | - |
| 60 | 25613 | Feralfen Mystic's Handwraps | 4/1 | 10 | 96 | 0 | 60 |
| 60 | 24608 | Laughing Skull Gloves | 4/1 | 10 | 90 | 60 | - |
| 60 | 25621 | Serpent Spirit's Drape | 4/1 | 16 | 96 | 0 | 60 |
| 60 | 25489 | Windtalker's Cloak | 4/1 | 16 | 93 | 0 | 60 |
| 60 | 25510 | Ceremonial Robes | 4/1 | 20 | 93 | 0 | 60 |
| 60 | 22496 | Frostfire Robe | 4/1 | 20 | 92 | 60 | 60 |
| 60 | 25513 | Clefthoof Hide Mask | 4/2 | 1 | 93 | 0 | 60 |
| 60 | 24721 | Dreghood Cowl | 4/2 | 1 | 90 | 60 | - |
| 60 | 27724 | Wild Shoulderpads | 4/2 | 3 | 93 | 0 | 58 |
| 60 | 24723 | Dreghood Pauldrons | 4/2 | 3 | 90 | 60 | - |
| 60 | 25508 | Omenai Vest | 4/2 | 5 | 93 | 0 | 60 |
| 60 | 22476 | Bonescythe Breastplate | 4/2 | 5 | 92 | 60 | 60 |
| 60 | 31660 | Feralfen Skulker's Belt | 4/2 | 6 | 102 | 0 | 60 |
| 60 | 89564 | Red Belt of Gentle Persuasion | 4/2 | 6 | 93 | 0 | 60 |
| 60 | 25507 | Leggings of Telhamat | 4/2 | 7 | 93 | 0 | 60 |
| 60 | 25518 | Explorer's Leggings | 4/2 | 7 | 93 | 0 | 60 |
| 60 | 25524 | Cenarion Expedition Boots | 4/2 | 8 | 93 | 0 | 60 |
| 60 | 27727 | Swift Cenarion Footwear | 4/2 | 8 | 93 | 0 | 58 |
| 60 | 25486 | Demonslayer's Wristguards | 4/2 | 9 | 93 | 0 | 60 |
| 60 | 24724 | Dreghood Bands | 4/2 | 9 | 90 | 60 | - |
| 60 | 24720 | Dreghood Gloves | 4/2 | 10 | 90 | 60 | - |
| 60 | 31788 | Blacksting Gloves | 4/2 | 10 | 90 | 0 | 60 |
| 60 | 16833 | Cenarion Vestments | 4/2 | 20 | 66 | 60 | - |
| 60 | 11747 | Flamestrider Robes | 4/2 | 20 | 52 | 47 | - |
| 60 | 25506 | Vindicator's Chain Helm | 4/3 | 1 | 93 | 0 | 60 |
| 60 | 24834 | Nexus-Strider Helmet | 4/3 | 1 | 90 | 60 | - |
| 60 | 24836 | Nexus-Strider Mantle | 4/3 | 3 | 90 | 60 | - |
| 60 | 31786 | Blacksting Shoulders | 4/3 | 3 | 90 | 0 | 60 |
| 60 | 25614 | Feralfen Beastmaster's Hauberk | 4/3 | 5 | 102 | 0 | 60 |
| 60 | 25512 | Tribal Hauberk | 4/3 | 5 | 93 | 0 | 60 |
| 60 | 27726 | Hearty Cenarion Cincture | 4/3 | 6 | 93 | 0 | 58 |
| 60 | 24830 | Nexus-Strider Belt | 4/3 | 6 | 90 | 60 | - |
| 60 | 24835 | Nexus-Strider Legwraps | 4/3 | 7 | 90 | 60 | - |
| 60 | 27717 | Expedition Forager Leggings | 4/3 | 7 | 90 | 0 | 60 |
| 60 | 25479 | Boots of the Earthcaller | 4/3 | 8 | 93 | 0 | 60 |
| 60 | 25482 | Venn'ren's Boots | 4/3 | 8 | 93 | 0 | 58 |
| 60 | 24837 | Nexus-Strider Bracer | 4/3 | 9 | 90 | 60 | - |
| 60 | 25598 | Fen Strider's Bracer | 4/3 | 9 | 90 | 0 | 60 |
| 60 | 25523 | Windcaller's Gauntlets | 4/3 | 10 | 93 | 0 | 60 |
| 60 | 27722 | Gloves of Marshmanship | 4/3 | 10 | 93 | 0 | 58 |
| 60 | 16841 | Earthfury Vestments | 4/3 | 20 | 66 | 60 | - |
| 60 | 24946 | Bloodscale Helm | 4/4 | 1 | 90 | 60 | - |
| 60 | 27715 | Circle's Stalwart Helmet | 4/4 | 1 | 90 | 0 | 60 |
| 60 | 25522 | Marshstrider's Spaulders | 4/4 | 3 | 93 | 0 | 60 |
| 60 | 27725 | Expedition Mantle | 4/4 | 3 | 93 | 0 | 58 |
| 60 | 22416 | Dreadnaught Breastplate | 4/4 | 5 | 92 | 60 | 60 |
| 60 | 22425 | Redemption Tunic | 4/4 | 5 | 92 | 60 | 60 |
| 60 | 24942 | Bloodscale Belt | 4/4 | 6 | 90 | 60 | - |
| 60 | 25597 | Vindicator's Cinch | 4/4 | 6 | 90 | 0 | 60 |
| 60 | 25511 | Thunderforge Leggings | 4/4 | 7 | 93 | 0 | 60 |
| 60 | 24947 | Bloodscale Legguards | 4/4 | 7 | 90 | 60 | - |
| 60 | 25615 | Feralfen Champion's Boots | 4/4 | 8 | 96 | 0 | 60 |
| 60 | 27721 | Expedition Footgear | 4/4 | 8 | 93 | 0 | 58 |
| 60 | 24949 | Bloodscale Bracers | 4/4 | 9 | 90 | 60 | - |
| 60 | 22423 | Dreadnaught Bracers | 4/4 | 9 | 88 | 60 | 60 |
| 60 | 25478 | Defender's Gauntlets | 4/4 | 10 | 93 | 0 | 60 |
| 60 | 25481 | Sunstrider's Gauntlets | 4/4 | 10 | 93 | 0 | 58 |
| 60 | 25075 | Hardened Steel Shield | 4/6 | 14 | 90 | 60 | - |
| 60 | 25074 | Telaari Shield | 4/6 | 14 | 87 | 59 | - |
| 69 | 36573 | Primeval Adze | 2/0 | 13 | 138 | 69 | - |
| 69 | 37032 | Edge of the Tuskarr | 2/0 | 13 | 138 | 0 | 68 |
| 69 | 9608 | Shoni's Disarming Tool | 2/0 | 22 | 31 | 0 | 20 |
| 69 | 38237 | Axe of Frozen Death | 2/1 | 17 | 138 | 0 | 68 |
| 69 | 35870 | Stoneground Cleaver | 2/1 | 17 | 138 | 0 | 68 |
| 69 | 36615 | Expert's Longbow | 2/2 | 15 | 138 | 69 | - |
| 69 | 36882 | Warsong Longbow | 2/2 | 15 | 138 | 0 | 68 |
| 69 | 38241 | Fury of the Raging Dragon | 2/3 | 26 | 138 | 0 | 68 |
| 69 | 35856 | Hair-Trigger Blunderbuss | 2/3 | 26 | 138 | 0 | 68 |
| 69 | 35810 | Pacifying Pummeler | 2/4 | 13 | 138 | 0 | 68 |
| 69 | 36489 | Iron Flanged Scepter | 2/4 | 13 | 138 | 69 | - |
| 69 | 35812 | Cragthumper | 2/5 | 17 | 138 | 0 | 68 |
| 69 | 36503 | Toothless Bludgeon | 2/5 | 17 | 138 | 69 | - |
| 69 | 36601 | Patient Harpoon | 2/6 | 17 | 138 | 69 | - |
| 69 | 36600 | Village Scythe | 2/6 | 17 | 134 | 68 | - |
| 69 | 35824 | Stoneblade Slicer | 2/7 | 13 | 138 | 0 | 68 |
| 69 | 36517 | Iron-Grip Shortsword | 2/7 | 13 | 138 | 69 | - |
| 69 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 69 | 12940 | Dal'Rend's Sacred Charge | 2/7 | 21 | 60 | 55 | - |
| 69 | 19866 | Warblade of the Hakkari | 2/7 | 22 | 66 | 60 | - |
| 69 | 12939 | Dal'Rend's Tribal Guardian | 2/7 | 22 | 60 | 55 | - |
| 69 | 35849 | Featherweight Claymore | 2/8 | 17 | 138 | 0 | 68 |
| 69 | 36531 | Sawtooth Greatsword | 2/8 | 17 | 138 | 69 | - |
| 69 | 38239 | Tower of the Infinite Mind | 2/10 | 17 | 138 | 0 | 68 |
| 69 | 38240 | Staff of the Purposeful Mendicant | 2/10 | 17 | 138 | 0 | 68 |
| 69 | 38238 | Borean Smasher | 2/13 | 13 | 138 | 0 | 68 |
| 69 | 35830 | Worn Vrykul Smasher | 2/13 | 13 | 138 | 0 | 68 |
| 69 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 69 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 69 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 69 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 69 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 69 | 38242 | Fang of the Desolate Soul | 2/15 | 13 | 138 | 0 | 68 |
| 69 | 35809 | Earthspike | 2/15 | 13 | 138 | 0 | 68 |
| 69 | 29914 | Hellfire Skiver | 2/15 | 22 | 81 | 0 | 58 |
| 69 | 18392 | Distracting Dagger | 2/15 | 22 | 45 | 40 | - |
| 69 | 35811 | Elekk-Horn Crossbow | 2/18 | 26 | 138 | 0 | 68 |
| 69 | 36629 | Ferocious Crossbow | 2/18 | 26 | 138 | 69 | - |
| 69 | 35826 | Whelpling-Skull Zapper | 2/19 | 26 | 138 | 0 | 68 |
| 69 | 35827 | Regal Sceptre | 2/19 | 26 | 138 | 0 | 68 |
| 69 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 69 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 69 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 69 | 44374 | Amulet of the Tranquil Mind | 4/0 | 2 | 151 | 0 | 68 |
| 69 | 44375 | Razor-Blade Pendant | 4/0 | 2 | 151 | 0 | 68 |
| 69 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 69 | 38218 | Executioner's Band | 4/0 | 11 | 155 | 0 | 68 |
| 69 | 38219 | Ring of Decimation | 4/0 | 11 | 155 | 0 | 68 |
| 69 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 69 | 36871 | Fury of the Encroaching Storm | 4/0 | 12 | 138 | 0 | 68 |
| 69 | 35858 | Tome of Alacrity | 4/0 | 23 | 138 | 0 | 68 |
| 69 | 36461 | Demon-Skull Orb | 4/0 | 23 | 138 | 69 | - |
| 69 | 35814 | Benevolent Hood | 4/1 | 1 | 138 | 0 | 68 |
| 69 | 35844 | Shock-Resistant Hood | 4/1 | 1 | 138 | 0 | 68 |
| 69 | 43181 | Shoulders of the Northern Lights | 4/1 | 3 | 155 | 0 | 68 |
| 69 | 35818 | Stretch-Hide Spaulders | 4/1 | 3 | 138 | 0 | 68 |
| 69 | 35859 | Fire-Purifying Tunic | 4/1 | 5 | 138 | 0 | 68 |
| 69 | 24671 | Eldr'naan Jerkin | 4/1 | 5 | 114 | 68 | - |
| 69 | 35888 | Embossed Ermine Girdle | 4/1 | 6 | 138 | 0 | 68 |
| 69 | 35929 | Gholamcloth Wrap | 4/1 | 6 | 138 | 0 | 68 |
| 69 | 35863 | Earth-Infused Leggings | 4/1 | 7 | 138 | 0 | 69 |
| 69 | 35884 | Regal Pantaloons | 4/1 | 7 | 138 | 0 | 68 |
| 69 | 42760 | Sandals of Mystical Evolution | 4/1 | 8 | 155 | 0 | 69 |
| 69 | 35880 | Earthwell Footwraps | 4/1 | 8 | 138 | 0 | 68 |
| 69 | 39676 | Wraps of the San'layn | 4/1 | 9 | 155 | 0 | 68 |
| 69 | 35896 | Icestriker Bands | 4/1 | 9 | 138 | 0 | 69 |
| 69 | 42758 | Time-Twisted Wraps | 4/1 | 10 | 155 | 0 | 69 |
| 69 | 35832 | Blauvelt's Special Occasion Gloves | 4/1 | 10 | 138 | 0 | 68 |
| 69 | 38221 | Shroud of Fluid Strikes | 4/1 | 16 | 159 | 0 | 69 |
| 69 | 38222 | Mantle of Keristrasza | 4/1 | 16 | 159 | 0 | 69 |
| 69 | 35871 | Indigo Robe of Replenishment | 4/1 | 20 | 138 | 0 | 68 |
| 69 | 35973 | Mur'ghoul Robe | 4/1 | 20 | 138 | 69 | - |
| 69 | 35833 | Imperious Worghide Cap | 4/2 | 1 | 138 | 0 | 68 |
| 69 | 35864 | Fizznik's Patented Earwarmer | 4/2 | 1 | 138 | 0 | 69 |
| 69 | 43182 | Cured Mammoth Hide Mantle | 4/2 | 3 | 155 | 0 | 68 |
| 69 | 35877 | Worgskin Shoulders | 4/2 | 3 | 138 | 0 | 68 |
| 69 | 35815 | Bone-Threaded Harness | 4/2 | 5 | 138 | 0 | 68 |
| 69 | 35885 | Tribal Chestguard | 4/2 | 5 | 138 | 0 | 68 |
| 69 | 35901 | Rejuvenating Cord | 4/2 | 6 | 138 | 0 | 68 |
| 69 | 36083 | Riplash Belt | 4/2 | 6 | 138 | 69 | - |
| 69 | 35868 | Grounded Pants | 4/2 | 7 | 138 | 0 | 68 |
| 69 | 35872 | Iron-Studded Leggings | 4/2 | 7 | 138 | 0 | 68 |
| 69 | 42762 | Treads of Torn Future | 4/2 | 8 | 155 | 0 | 69 |
| 69 | 35860 | Flexible Leather Footwraps | 4/2 | 8 | 138 | 0 | 68 |
| 69 | 39678 | Vendetta Bindings | 4/2 | 9 | 155 | 0 | 68 |
| 69 | 35889 | Bone-Inlaid Bracers | 4/2 | 9 | 138 | 0 | 68 |
| 69 | 42761 | Time-Stop Gloves | 4/2 | 10 | 155 | 0 | 69 |
| 69 | 35820 | Deacon's Wraps | 4/2 | 10 | 138 | 0 | 68 |
| 69 | 34796 | Robes of Summer Flame | 4/2 | 20 | 115 | 68 | - |
| 69 | 16833 | Cenarion Vestments | 4/2 | 20 | 66 | 60 | - |
| 69 | 35869 | Hex-Linked Stronghelm | 4/3 | 1 | 138 | 0 | 68 |
| 69 | 35882 | Magispike Helm | 4/3 | 1 | 138 | 0 | 68 |
| 69 | 43183 | Tundra Tracker's Shoulderguards | 4/3 | 3 | 155 | 0 | 68 |
| 69 | 35890 | Magdun Spaulders | 4/3 | 3 | 138 | 0 | 68 |
| 69 | 35842 | Azure Chain Hauberk | 4/3 | 5 | 138 | 0 | 68 |
| 69 | 35898 | Reinforced Tuskhide Hauberk | 4/3 | 5 | 138 | 0 | 69 |
| 69 | 35861 | Inescapable Girdle | 4/3 | 6 | 138 | 0 | 68 |
| 69 | 35873 | Interlinked Chain Girdle | 4/3 | 6 | 138 | 0 | 68 |
| 69 | 35816 | Blood-Stained Chain Leggings | 4/3 | 7 | 138 | 0 | 68 |
| 69 | 35878 | Dusk-Linked Leggings | 4/3 | 7 | 138 | 0 | 68 |
| 69 | 42766 | Spiked Treads of Mutation | 4/3 | 8 | 155 | 0 | 69 |
| 69 | 35821 | Streamlined Stompers | 4/3 | 8 | 138 | 0 | 68 |
| 69 | 39679 | Runecaster's Bracers | 4/3 | 9 | 155 | 0 | 68 |
| 69 | 35894 | Purestrike Bracers | 4/3 | 9 | 138 | 0 | 68 |
| 69 | 42763 | Bindings of Sabotage | 4/3 | 10 | 155 | 0 | 69 |
| 69 | 35834 | Nimblefinger Scaled Gloves | 4/3 | 10 | 138 | 0 | 68 |
| 69 | 16841 | Earthfury Vestments | 4/3 | 20 | 66 | 60 | - |
| 69 | 35875 | Beneficent Skullcap | 4/4 | 1 | 138 | 0 | 68 |
| 69 | 35891 | Runeplate Helm | 4/4 | 1 | 138 | 0 | 68 |
| 69 | 43184 | Tundra Pauldrons | 4/4 | 3 | 155 | 0 | 68 |
| 69 | 35843 | Ramshorn-Inlaid Shoulders | 4/4 | 3 | 138 | 0 | 68 |
| 69 | 35822 | Scavenged Tirasian Plate | 4/4 | 5 | 138 | 0 | 68 |
| 69 | 35862 | Light-Bound Chestguard | 4/4 | 5 | 138 | 0 | 68 |
| 69 | 35899 | Bloodbinder's Girdle | 4/4 | 6 | 138 | 0 | 69 |
| 69 | 36307 | Baleheim Belt | 4/4 | 6 | 138 | 69 | - |
| 69 | 35835 | Antique Reinforced Legguards | 4/4 | 7 | 138 | 0 | 68 |
| 69 | 35848 | Munificent Legguards | 4/4 | 7 | 138 | 0 | 68 |
| 69 | 42767 | Invigorating Sabatons | 4/4 | 8 | 155 | 0 | 69 |
| 69 | 42768 | Boots of the Unbowed Protector | 4/4 | 8 | 155 | 0 | 69 |
| 69 | 39680 | Vambraces of the Vengeance Bringer | 4/4 | 9 | 155 | 0 | 68 |
| 69 | 35910 | Shaleground Bracers | 4/4 | 9 | 138 | 0 | 68 |
| 69 | 42765 | Gauntlets of the Disturbed Giant | 4/4 | 10 | 155 | 0 | 69 |
| 69 | 35883 | Silversteel Gauntlets | 4/4 | 10 | 138 | 0 | 68 |
| 69 | 35817 | Nerubian Inner Husk | 4/6 | 14 | 138 | 0 | 68 |
| 69 | 35857 | Munificent Bulwark | 4/6 | 14 | 138 | 0 | 68 |
| 70 | 34996 | Brutal Gladiator's Cleaver | 2/0 | 13 | 154 | 70 | - |
| 70 | 35110 | Brutal Gladiator's Waraxe | 2/0 | 13 | 154 | 70 | - |
| 70 | 34995 | Brutal Gladiator's Chopper | 2/0 | 22 | 154 | 70 | - |
| 70 | 35017 | Brutal Gladiator's Hacker | 2/0 | 22 | 154 | 70 | - |
| 70 | 30316 | Devastation | 2/1 | 17 | 175 | 70 | - |
| 70 | 35576 | Ingvar's Monolithic Cleaver | 2/1 | 17 | 155 | 70 | - |
| 70 | 30318 | Netherstrand Longbow | 2/2 | 15 | 175 | 70 | - |
| 70 | 34334 | Thori'dal, the Stars' Fury | 2/2 | 15 | 164 | 70 | - |
| 70 | 35075 | Brutal Gladiator's Rifle | 2/3 | 26 | 154 | 70 | - |
| 70 | 34530 | Vengeful Gladiator's Rifle | 2/3 | 26 | 146 | 70 | - |
| 70 | 30317 | Cosmic Infuser | 2/4 | 13 | 175 | 70 | - |
| 70 | 34335 | Hammer of Sanctification | 2/4 | 13 | 164 | 70 | - |
| 70 | 34988 | Brutal Gladiator's Bonecracker | 2/4 | 22 | 154 | 70 | - |
| 70 | 33662 | Vengeful Gladiator's Bonecracker | 2/4 | 22 | 146 | 70 | - |
| 70 | 34989 | Brutal Gladiator's Bonegrinder | 2/5 | 17 | 154 | 70 | - |
| 70 | 33663 | Vengeful Gladiator's Bonegrinder | 2/5 | 17 | 146 | 70 | - |
| 70 | 34183 | Shivering Felspine | 2/6 | 17 | 154 | 70 | - |
| 70 | 35064 | Brutal Gladiator's Painsaw | 2/6 | 17 | 154 | 70 | - |
| 70 | 30311 | Warp Slicer | 2/7 | 13 | 175 | 70 | - |
| 70 | 34164 | Dragonscale-Encrusted Longblade | 2/7 | 13 | 154 | 70 | - |
| 70 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 70 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 70 | 32838 | Warglaive of Azzinoth | 2/7 | 22 | 156 | 70 | - |
| 70 | 35072 | Brutal Gladiator's Quickblade | 2/7 | 22 | 154 | 70 | - |
| 70 | 34247 | Apolyon, the Soul-Render | 2/8 | 17 | 164 | 70 | - |
| 70 | 35015 | Brutal Gladiator's Greatsword | 2/8 | 17 | 154 | 70 | - |
| 70 | 30313 | Staff of Disintegration | 2/10 | 17 | 175 | 70 | - |
| 70 | 34182 | Grand Magister's Staff of Torrents | 2/10 | 17 | 154 | 70 | - |
| 70 | 34331 | Hand of the Deceiver | 2/13 | 13 | 164 | 70 | - |
| 70 | 35573 | Arm Blade of Augelmir | 2/13 | 13 | 155 | 70 | - |
| 70 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 70 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 70 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 70 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 70 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 70 | 30312 | Infinity Blade | 2/15 | 13 | 175 | 70 | - |
| 70 | 34329 | Crux of the Apocalypse | 2/15 | 13 | 164 | 70 | - |
| 70 | 35058 | Brutal Gladiator's Mutilator | 2/15 | 22 | 154 | 70 | - |
| 70 | 35095 | Brutal Gladiator's Shiv | 2/15 | 22 | 154 | 70 | - |
| 70 | 35018 | Brutal Gladiator's Heavy Crossbow | 2/18 | 26 | 154 | 70 | - |
| 70 | 33006 | Vengeful Gladiator's Heavy Crossbow | 2/18 | 26 | 146 | 70 | - |
| 70 | 34347 | Wand of the Demonsoul | 2/19 | 26 | 154 | 70 | - |
| 70 | 34348 | Wand of Cleansing Light | 2/19 | 26 | 154 | 70 | - |
| 70 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 70 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 70 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 70 | 34177 | Clutch of Demise | 4/0 | 2 | 154 | 70 | - |
| 70 | 34178 | Collar of the Pit Lord | 4/0 | 2 | 154 | 70 | - |
| 70 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 70 | 38218 | Executioner's Band | 4/0 | 11 | 155 | 0 | 68 |
| 70 | 38219 | Ring of Decimation | 4/0 | 11 | 155 | 0 | 68 |
| 70 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 70 | 44073 | Frenzyheart Insignia of Fury | 4/0 | 12 | 200 | 70 | - |
| 70 | 35008 | Brutal Gladiator's Endgame | 4/0 | 23 | 159 | 70 | - |
| 70 | 35016 | Brutal Gladiator's Grimoire | 4/0 | 23 | 159 | 70 | - |
| 70 | 34339 | Cowl of Light's Purity | 4/1 | 1 | 164 | 70 | - |
| 70 | 34340 | Dark Conjuror's Collar | 4/1 | 1 | 164 | 70 | - |
| 70 | 34202 | Shawl of Wonderment | 4/1 | 3 | 159 | 70 | - |
| 70 | 34210 | Amice of the Convoker | 4/1 | 3 | 159 | 70 | - |
| 70 | 35859 | Fire-Purifying Tunic | 4/1 | 5 | 138 | 0 | 68 |
| 70 | 37005 | D.E.H.T.A. Overshirt | 4/1 | 5 | 138 | 0 | 70 |
| 70 | 35153 | Guardian's Dreadweave Belt | 4/1 | 6 | 159 | 70 | - |
| 70 | 35159 | Guardian's Mooncloth Belt | 4/1 | 6 | 159 | 70 | - |
| 70 | 34170 | Pantaloons of Calming Strife | 4/1 | 7 | 159 | 70 | - |
| 70 | 34181 | Leggings of Calamity | 4/1 | 7 | 159 | 70 | - |
| 70 | 35138 | Guardian's Dreadweave Stalkers | 4/1 | 8 | 159 | 70 | - |
| 70 | 35144 | Guardian's Mooncloth Slippers | 4/1 | 8 | 159 | 70 | - |
| 70 | 39676 | Wraps of the San'layn | 4/1 | 9 | 155 | 0 | 68 |
| 70 | 34434 | Bracers of Absolution | 4/1 | 9 | 154 | 70 | - |
| 70 | 34342 | Handguards of the Dawn | 4/1 | 10 | 164 | 70 | - |
| 70 | 34344 | Handguards of Defiled Worlds | 4/1 | 10 | 164 | 70 | - |
| 70 | 34241 | Cloak of Unforgivable Sin | 4/1 | 16 | 164 | 70 | - |
| 70 | 34242 | Tattered Cape of Antonidas | 4/1 | 16 | 164 | 70 | - |
| 70 | 34232 | Fel Conquerer Raiments | 4/1 | 20 | 159 | 70 | - |
| 70 | 34399 | Robes of Ghostly Hatred | 4/1 | 20 | 159 | 70 | - |
| 70 | 34244 | Duplicitous Guise | 4/2 | 1 | 164 | 70 | - |
| 70 | 34245 | Cover of Ursol the Wise | 4/2 | 1 | 164 | 70 | - |
| 70 | 34195 | Shoulderpads of Vehemence | 4/2 | 3 | 159 | 70 | - |
| 70 | 34209 | Spaulders of Reclamation | 4/2 | 3 | 159 | 70 | - |
| 70 | 34211 | Harness of Carnal Instinct | 4/2 | 5 | 159 | 70 | - |
| 70 | 34212 | Sunglow Vest | 4/2 | 5 | 159 | 70 | - |
| 70 | 35152 | Guardian's Dragonhide Belt | 4/2 | 6 | 159 | 70 | - |
| 70 | 35154 | Guardian's Kodohide Belt | 4/2 | 6 | 159 | 70 | - |
| 70 | 34169 | Breeches of Natural Aggression | 4/2 | 7 | 159 | 70 | - |
| 70 | 34188 | Leggings of the Immortal Night | 4/2 | 7 | 159 | 70 | - |
| 70 | 35137 | Guardian's Dragonhide Boots | 4/2 | 8 | 159 | 70 | - |
| 70 | 35139 | Guardian's Kodohide Boots | 4/2 | 8 | 159 | 70 | - |
| 70 | 35579 | Vrykul Shackles | 4/2 | 9 | 155 | 70 | - |
| 70 | 39678 | Vendetta Bindings | 4/2 | 9 | 155 | 0 | 68 |
| 70 | 34234 | Shadowed Gauntlets of Paroxysm | 4/2 | 10 | 159 | 70 | - |
| 70 | 34408 | Gloves of the Forest Drifter | 4/2 | 10 | 159 | 70 | - |
| 70 | 31041 | Thunderheart Tunic | 4/2 | 20 | 146 | 70 | - |
| 70 | 31042 | Thunderheart Chestguard | 4/2 | 20 | 146 | 70 | - |
| 70 | 34332 | Cowl of Gul'dan | 4/3 | 1 | 164 | 70 | - |
| 70 | 34333 | Coif of Alleria | 4/3 | 1 | 164 | 70 | - |
| 70 | 34208 | Equilibrium Epaulets | 4/3 | 3 | 159 | 70 | - |
| 70 | 34390 | Erupting Epaulets | 4/3 | 3 | 159 | 70 | - |
| 70 | 34228 | Vicious Hawkstrider Hauberk | 4/3 | 5 | 159 | 70 | - |
| 70 | 34229 | Garments of Serene Shores | 4/3 | 5 | 159 | 70 | - |
| 70 | 35151 | Guardian's Chain Girdle | 4/3 | 6 | 159 | 70 | - |
| 70 | 35157 | Guardian's Linked Girdle | 4/3 | 6 | 159 | 70 | - |
| 70 | 34168 | Starstalker Legguards | 4/3 | 7 | 159 | 70 | - |
| 70 | 34186 | Chain Links of the Tumultuous Storm | 4/3 | 7 | 159 | 70 | - |
| 70 | 35136 | Guardian's Chain Sabatons | 4/3 | 8 | 159 | 70 | - |
| 70 | 35142 | Guardian's Linked Sabatons | 4/3 | 8 | 159 | 70 | - |
| 70 | 44669 | Worgen-Scored Shackles | 4/3 | 9 | 159 | 70 | - |
| 70 | 39679 | Runecaster's Bracers | 4/3 | 9 | 155 | 0 | 68 |
| 70 | 34343 | Thalassian Ranger Gauntlets | 4/3 | 10 | 164 | 70 | - |
| 70 | 34409 | Gauntlets of the Ancient Frostwolf | 4/3 | 10 | 159 | 70 | - |
| 70 | 35042 | Brutal Gladiator's Linked Armor | 4/3 | 20 | 159 | 70 | - |
| 70 | 35048 | Brutal Gladiator's Mail Armor | 4/3 | 20 | 159 | 70 | - |
| 70 | 34243 | Helm of Burning Righteousness | 4/4 | 1 | 164 | 70 | - |
| 70 | 34345 | Crown of Anasterian | 4/4 | 1 | 164 | 70 | - |
| 70 | 34192 | Pauldrons of Perseverance | 4/4 | 3 | 159 | 70 | - |
| 70 | 34193 | Spaulders of the Thalassian Savior | 4/4 | 3 | 159 | 70 | - |
| 70 | 34215 | Warharness of Reckless Fury | 4/4 | 5 | 159 | 70 | - |
| 70 | 34216 | Heroic Judicator's Chestguard | 4/4 | 5 | 159 | 70 | - |
| 70 | 35155 | Guardian's Lamellar Belt | 4/4 | 6 | 159 | 70 | - |
| 70 | 35160 | Guardian's Ornamented Belt | 4/4 | 6 | 159 | 70 | - |
| 70 | 34167 | Legplates of the Holy Juggernaut | 4/4 | 7 | 159 | 70 | - |
| 70 | 34180 | Felfury Legplates | 4/4 | 7 | 159 | 70 | - |
| 70 | 35140 | Guardian's Lamellar Greaves | 4/4 | 8 | 159 | 70 | - |
| 70 | 35145 | Guardian's Ornamented Greaves | 4/4 | 8 | 159 | 70 | - |
| 70 | 39680 | Vambraces of the Vengeance Bringer | 4/4 | 9 | 155 | 0 | 68 |
| 70 | 34431 | Lightbringer Bands | 4/4 | 9 | 154 | 70 | - |
| 70 | 34341 | Borderland Paingrips | 4/4 | 10 | 164 | 70 | - |
| 70 | 34240 | Gauntlets of the Soothed Soul | 4/4 | 10 | 159 | 70 | - |
| 70 | 30314 | Phaseshift Bulwark | 4/6 | 14 | 175 | 70 | - |
| 70 | 34986 | Brutal Gladiator's Barrier | 4/6 | 14 | 159 | 70 | - |
| 78 | 55203 | Lightning Whelk Axe | 2/0 | 13 | 308 | 78 | - |
| 78 | 55282 | Steelspark Axe | 2/0 | 13 | 278 | 78 | - |
| 78 | 34995 | Brutal Gladiator's Chopper | 2/0 | 22 | 154 | 70 | - |
| 78 | 35017 | Brutal Gladiator's Hacker | 2/0 | 22 | 154 | 70 | - |
| 78 | 55252 | Whitefin Axe | 2/1 | 17 | 308 | 78 | - |
| 78 | 55291 | Steelspark Broadaxe | 2/1 | 17 | 278 | 78 | - |
| 78 | 55782 | Amber Messenger | 2/2 | 15 | 308 | 78 | - |
| 78 | 55300 | Steelspark Bow | 2/2 | 15 | 278 | 78 | - |
| 78 | 55354 | Steelspark Gun | 2/3 | 26 | 278 | 78 | - |
| 78 | 55353 | Razorwind Gun | 2/3 | 26 | 272 | 77 | - |
| 78 | 55777 | Torturer's Mercy | 2/4 | 13 | 308 | 78 | - |
| 78 | 55363 | Swamplight Scepter | 2/4 | 13 | 278 | 78 | - |
| 78 | 34988 | Brutal Gladiator's Bonecracker | 2/4 | 22 | 154 | 70 | - |
| 78 | 33662 | Vengeful Gladiator's Bonecracker | 2/4 | 22 | 146 | 70 | - |
| 78 | 55390 | Steelspark Hammer | 2/5 | 17 | 278 | 78 | - |
| 78 | 55389 | Razorwind Hammer | 2/5 | 17 | 272 | 77 | - |
| 78 | 55408 | Steelspark Spear | 2/6 | 17 | 278 | 78 | - |
| 78 | 55407 | Razorwind Spear | 2/6 | 17 | 272 | 77 | - |
| 78 | 55453 | Steelspark Sword | 2/7 | 13 | 278 | 78 | - |
| 78 | 55452 | Razorwind Sword | 2/7 | 13 | 272 | 77 | - |
| 78 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 78 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 78 | 32838 | Warglaive of Azzinoth | 2/7 | 22 | 156 | 70 | - |
| 78 | 35072 | Brutal Gladiator's Quickblade | 2/7 | 22 | 154 | 70 | - |
| 78 | 55462 | Steelspark Greatsword | 2/8 | 17 | 278 | 78 | - |
| 78 | 55461 | Razorwind Greatsword | 2/8 | 17 | 272 | 77 | - |
| 78 | 55228 | Cerith Spire Staff | 2/10 | 17 | 308 | 78 | - |
| 78 | 55789 | Berto's Staff | 2/10 | 17 | 308 | 78 | - |
| 78 | 55345 | Steelspark Knuckles | 2/13 | 13 | 278 | 78 | - |
| 78 | 55344 | Razorwind Knuckles | 2/13 | 13 | 272 | 77 | - |
| 78 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 78 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 78 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 78 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 78 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 78 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 78 | 55272 | Steelbender's Masterpiece | 2/15 | 13 | 308 | 78 | - |
| 78 | 55790 | Toxidunk Dagger | 2/15 | 13 | 308 | 78 | - |
| 78 | 35058 | Brutal Gladiator's Mutilator | 2/15 | 22 | 154 | 70 | - |
| 78 | 35095 | Brutal Gladiator's Shiv | 2/15 | 22 | 154 | 70 | - |
| 78 | 55318 | Steelspark Crossbow | 2/18 | 26 | 278 | 78 | - |
| 78 | 55317 | Razorwind Crossbow | 2/18 | 26 | 272 | 77 | - |
| 78 | 55267 | Corla's Baton | 2/19 | 26 | 308 | 78 | - |
| 78 | 55480 | Swamplight Wand | 2/19 | 26 | 278 | 78 | - |
| 78 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 78 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 78 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 78 | 55258 | Pipefish Cord | 4/0 | 2 | 308 | 78 | - |
| 78 | 55262 | Barnacle Pendant | 4/0 | 2 | 308 | 78 | - |
| 78 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 78 | 55201 | Entwined Nereis | 4/0 | 11 | 308 | 78 | - |
| 78 | 55236 | Anthia's Ring | 4/0 | 11 | 308 | 78 | - |
| 78 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 78 | 55237 | Porcelain Crab | 4/0 | 12 | 308 | 78 | - |
| 78 | 55259 | Bioluminescent Lamp | 4/0 | 23 | 308 | 78 | - |
| 78 | 55277 | Beauty's Favorite Bone | 4/0 | 23 | 308 | 78 | - |
| 78 | 55198 | Aurelian Mitre | 4/1 | 1 | 308 | 78 | - |
| 78 | 55741 | Mistmantle Cowl | 4/1 | 1 | 278 | 78 | - |
| 78 | 55786 | Kyrstel Mantle | 4/1 | 3 | 308 | 78 | - |
| 78 | 55759 | Mistmantle Shoulderpads | 4/1 | 3 | 278 | 78 | - |
| 78 | 35859 | Fire-Purifying Tunic | 4/1 | 5 | 138 | 0 | 68 |
| 78 | 37005 | D.E.H.T.A. Overshirt | 4/1 | 5 | 138 | 0 | 70 |
| 78 | 55275 | Beauty's Silken Ribbon | 4/1 | 6 | 308 | 78 | - |
| 78 | 55768 | Mistmantle Cord | 4/1 | 6 | 278 | 78 | - |
| 78 | 55750 | Mistmantle Leggings | 4/1 | 7 | 278 | 78 | - |
| 78 | 55749 | Ameth'Aran Leggings | 4/1 | 7 | 272 | 77 | - |
| 78 | 55723 | Mistmantle Sandals | 4/1 | 8 | 278 | 78 | - |
| 78 | 55722 | Ameth'Aran Sandals | 4/1 | 8 | 272 | 77 | - |
| 78 | 55705 | Mistmantle Wristwraps | 4/1 | 9 | 278 | 78 | - |
| 78 | 55704 | Ameth'Aran Wristwraps | 4/1 | 9 | 272 | 77 | - |
| 78 | 55255 | Mnemiopsis Gloves | 4/1 | 10 | 308 | 78 | - |
| 78 | 55732 | Mistmantle Handwraps | 4/1 | 10 | 278 | 78 | - |
| 78 | 55202 | Periwinkle Cloak | 4/1 | 16 | 308 | 78 | - |
| 78 | 55206 | Eagle Ray Cloak | 4/1 | 16 | 308 | 78 | - |
| 78 | 55278 | Inquisition Robes | 4/1 | 20 | 308 | 78 | - |
| 78 | 55714 | Mistmantle Robe | 4/1 | 20 | 278 | 78 | - |
| 78 | 55785 | Willowy Crown | 4/2 | 1 | 308 | 78 | - |
| 78 | 55669 | Mosshide Hood | 4/2 | 1 | 278 | 78 | - |
| 78 | 55204 | Caridean Epaulettes | 4/2 | 3 | 308 | 78 | - |
| 78 | 55687 | Mosshide Shoulders | 4/2 | 3 | 278 | 78 | - |
| 78 | 55642 | Mosshide Jerkin | 4/2 | 5 | 278 | 78 | - |
| 78 | 55641 | Stagalbog Jerkin | 4/2 | 5 | 272 | 77 | - |
| 78 | 55696 | Mosshide Waistband | 4/2 | 6 | 278 | 78 | - |
| 78 | 55695 | Stagalbog Waistband | 4/2 | 6 | 272 | 77 | - |
| 78 | 55273 | Beauty's Chew Toy | 4/2 | 7 | 308 | 78 | - |
| 78 | 55678 | Mosshide Britches | 4/2 | 7 | 278 | 78 | - |
| 78 | 55235 | Decapod Slippers | 4/2 | 8 | 308 | 78 | - |
| 78 | 55651 | Mosshide Boots | 4/2 | 8 | 278 | 78 | - |
| 78 | 55264 | Armbands of Change | 4/2 | 9 | 308 | 78 | - |
| 78 | 55633 | Mosshide Bindings | 4/2 | 9 | 278 | 78 | - |
| 78 | 55660 | Mosshide Gloves | 4/2 | 10 | 278 | 78 | - |
| 78 | 55659 | Stagalbog Gloves | 4/2 | 10 | 272 | 77 | - |
| 78 | 35677 | Cosmos Vestments | 4/2 | 20 | 183 | 77 | - |
| 78 | 31041 | Thunderheart Tunic | 4/2 | 20 | 146 | 70 | - |
| 78 | 55263 | Renouncer's Cowl | 4/3 | 1 | 308 | 78 | - |
| 78 | 55597 | Southfury Helm | 4/3 | 1 | 278 | 78 | - |
| 78 | 55615 | Southfury Spaulders | 4/3 | 3 | 278 | 78 | - |
| 78 | 55614 | Nazferiti Spaulders | 4/3 | 3 | 272 | 77 | - |
| 78 | 55205 | Chromis Chestpiece | 4/3 | 5 | 308 | 78 | - |
| 78 | 55253 | Wentletrap Vest | 4/3 | 5 | 308 | 78 | - |
| 78 | 55624 | Southfury Belt | 4/3 | 6 | 278 | 78 | - |
| 78 | 55623 | Nazferiti Belt | 4/3 | 6 | 272 | 77 | - |
| 78 | 55269 | Heat Wave Leggings | 4/3 | 7 | 308 | 78 | - |
| 78 | 55606 | Southfury Legguards | 4/3 | 7 | 278 | 78 | - |
| 78 | 55579 | Southfury Greaves | 4/3 | 8 | 278 | 78 | - |
| 78 | 55578 | Nazferiti Greaves | 4/3 | 8 | 272 | 77 | - |
| 78 | 55279 | Manacles of Pain | 4/3 | 9 | 308 | 78 | - |
| 78 | 55561 | Southfury Bracers | 4/3 | 9 | 278 | 78 | - |
| 78 | 55195 | Wrasse Handwraps | 4/3 | 10 | 308 | 78 | - |
| 78 | 55588 | Southfury Gauntlets | 4/3 | 10 | 278 | 78 | - |
| 78 | 35042 | Brutal Gladiator's Linked Armor | 4/3 | 20 | 159 | 70 | - |
| 78 | 35048 | Brutal Gladiator's Mail Armor | 4/3 | 20 | 159 | 70 | - |
| 78 | 55229 | Anomuran Helm | 4/4 | 1 | 308 | 78 | - |
| 78 | 55525 | Rethban Helm | 4/4 | 1 | 278 | 78 | - |
| 78 | 55207 | Harp Shell Pauldrons | 4/4 | 3 | 308 | 78 | - |
| 78 | 55779 | Raz's Pauldrons | 4/4 | 3 | 308 | 78 | - |
| 78 | 55254 | Abalone Plate Armor | 4/4 | 5 | 308 | 78 | - |
| 78 | 55274 | Beauty's Plate | 4/4 | 5 | 308 | 78 | - |
| 78 | 55552 | Rethban Girdle | 4/4 | 6 | 278 | 78 | - |
| 78 | 55551 | Dreadmaul Girdle | 4/4 | 6 | 272 | 77 | - |
| 78 | 55249 | Triton Legplates | 4/4 | 7 | 308 | 78 | - |
| 78 | 55260 | Alpheus Legguards | 4/4 | 7 | 308 | 78 | - |
| 78 | 55507 | Rethban Sabatons | 4/4 | 8 | 278 | 78 | - |
| 78 | 55506 | Dreadmaul Sabatons | 4/4 | 8 | 272 | 77 | - |
| 78 | 55268 | Bracers of Cooled Anger | 4/4 | 9 | 308 | 78 | - |
| 78 | 55489 | Rethban Vambraces | 4/4 | 9 | 278 | 78 | - |
| 78 | 55784 | Clutches of Dying Light | 4/4 | 10 | 308 | 78 | - |
| 78 | 55516 | Rethban Gauntlets | 4/4 | 10 | 278 | 78 | - |
| 78 | 55778 | Shield of the Iron Maiden | 4/6 | 14 | 308 | 78 | - |
| 78 | 55788 | Crepuscular Shield | 4/6 | 14 | 308 | 78 | - |
| 79 | 55203 | Lightning Whelk Axe | 2/0 | 13 | 308 | 78 | - |
| 79 | 55283 | Angerfang Axe | 2/0 | 13 | 283 | 79 | - |
| 79 | 34995 | Brutal Gladiator's Chopper | 2/0 | 22 | 154 | 70 | - |
| 79 | 35017 | Brutal Gladiator's Hacker | 2/0 | 22 | 154 | 70 | - |
| 79 | 55252 | Whitefin Axe | 2/1 | 17 | 308 | 78 | - |
| 79 | 55292 | Angerfang Broadaxe | 2/1 | 17 | 283 | 79 | - |
| 79 | 55782 | Amber Messenger | 2/2 | 15 | 308 | 78 | - |
| 79 | 55301 | Angerfang Bow | 2/2 | 15 | 283 | 79 | - |
| 79 | 55355 | Angerfang Gun | 2/3 | 26 | 283 | 79 | - |
| 79 | 55354 | Steelspark Gun | 2/3 | 26 | 278 | 78 | - |
| 79 | 55777 | Torturer's Mercy | 2/4 | 13 | 308 | 78 | - |
| 79 | 55364 | Sishir Scepter | 2/4 | 13 | 283 | 79 | - |
| 79 | 34988 | Brutal Gladiator's Bonecracker | 2/4 | 22 | 154 | 70 | - |
| 79 | 33662 | Vengeful Gladiator's Bonecracker | 2/4 | 22 | 146 | 70 | - |
| 79 | 55391 | Angerfang Hammer | 2/5 | 17 | 283 | 79 | - |
| 79 | 55390 | Steelspark Hammer | 2/5 | 17 | 278 | 78 | - |
| 79 | 55409 | Angerfang Spear | 2/6 | 17 | 283 | 79 | - |
| 79 | 55408 | Steelspark Spear | 2/6 | 17 | 278 | 78 | - |
| 79 | 55454 | Angerfang Sword | 2/7 | 13 | 283 | 79 | - |
| 79 | 55453 | Steelspark Sword | 2/7 | 13 | 278 | 78 | - |
| 79 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 79 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 79 | 32838 | Warglaive of Azzinoth | 2/7 | 22 | 156 | 70 | - |
| 79 | 35072 | Brutal Gladiator's Quickblade | 2/7 | 22 | 154 | 70 | - |
| 79 | 55463 | Angerfang Greatsword | 2/8 | 17 | 283 | 79 | - |
| 79 | 55462 | Steelspark Greatsword | 2/8 | 17 | 278 | 78 | - |
| 79 | 55228 | Cerith Spire Staff | 2/10 | 17 | 308 | 78 | - |
| 79 | 55789 | Berto's Staff | 2/10 | 17 | 308 | 78 | - |
| 79 | 55346 | Angerfang Knuckles | 2/13 | 13 | 283 | 79 | - |
| 79 | 55345 | Steelspark Knuckles | 2/13 | 13 | 278 | 78 | - |
| 79 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 79 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 79 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 79 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 79 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 79 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 79 | 55272 | Steelbender's Masterpiece | 2/15 | 13 | 308 | 78 | - |
| 79 | 55790 | Toxidunk Dagger | 2/15 | 13 | 308 | 78 | - |
| 79 | 35058 | Brutal Gladiator's Mutilator | 2/15 | 22 | 154 | 70 | - |
| 79 | 35095 | Brutal Gladiator's Shiv | 2/15 | 22 | 154 | 70 | - |
| 79 | 55319 | Angerfang Crossbow | 2/18 | 26 | 283 | 79 | - |
| 79 | 55318 | Steelspark Crossbow | 2/18 | 26 | 278 | 78 | - |
| 79 | 55267 | Corla's Baton | 2/19 | 26 | 308 | 78 | - |
| 79 | 55481 | Sishir Wand | 2/19 | 26 | 283 | 79 | - |
| 79 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 79 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 79 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 79 | 55258 | Pipefish Cord | 4/0 | 2 | 308 | 78 | - |
| 79 | 55262 | Barnacle Pendant | 4/0 | 2 | 308 | 78 | - |
| 79 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 79 | 55201 | Entwined Nereis | 4/0 | 11 | 308 | 78 | - |
| 79 | 55236 | Anthia's Ring | 4/0 | 11 | 308 | 78 | - |
| 79 | 86579 | Bottled Tornado | 4/0 | 12 | 400 | 1 | - |
| 79 | 55237 | Porcelain Crab | 4/0 | 12 | 308 | 78 | - |
| 79 | 55259 | Bioluminescent Lamp | 4/0 | 23 | 308 | 78 | - |
| 79 | 55277 | Beauty's Favorite Bone | 4/0 | 23 | 308 | 78 | - |
| 79 | 55198 | Aurelian Mitre | 4/1 | 1 | 308 | 78 | - |
| 79 | 55742 | Baradin Cowl | 4/1 | 1 | 283 | 79 | - |
| 79 | 55786 | Kyrstel Mantle | 4/1 | 3 | 308 | 78 | - |
| 79 | 55760 | Baradin Shoulderpads | 4/1 | 3 | 283 | 79 | - |
| 79 | 35859 | Fire-Purifying Tunic | 4/1 | 5 | 138 | 0 | 68 |
| 79 | 37005 | D.E.H.T.A. Overshirt | 4/1 | 5 | 138 | 0 | 70 |
| 79 | 55275 | Beauty's Silken Ribbon | 4/1 | 6 | 308 | 78 | - |
| 79 | 55769 | Baradin Cord | 4/1 | 6 | 283 | 79 | - |
| 79 | 55751 | Baradin Leggings | 4/1 | 7 | 283 | 79 | - |
| 79 | 55750 | Mistmantle Leggings | 4/1 | 7 | 278 | 78 | - |
| 79 | 55724 | Baradin Sandals | 4/1 | 8 | 283 | 79 | - |
| 79 | 55723 | Mistmantle Sandals | 4/1 | 8 | 278 | 78 | - |
| 79 | 55706 | Baradin Wristwraps | 4/1 | 9 | 283 | 79 | - |
| 79 | 55705 | Mistmantle Wristwraps | 4/1 | 9 | 278 | 78 | - |
| 79 | 55255 | Mnemiopsis Gloves | 4/1 | 10 | 308 | 78 | - |
| 79 | 55733 | Baradin Handwraps | 4/1 | 10 | 283 | 79 | - |
| 79 | 55202 | Periwinkle Cloak | 4/1 | 16 | 308 | 78 | - |
| 79 | 55206 | Eagle Ray Cloak | 4/1 | 16 | 308 | 78 | - |
| 79 | 55278 | Inquisition Robes | 4/1 | 20 | 308 | 78 | - |
| 79 | 55715 | Baradin Robe | 4/1 | 20 | 283 | 79 | - |
| 79 | 55785 | Willowy Crown | 4/2 | 1 | 308 | 78 | - |
| 79 | 55670 | Yojamba Hood | 4/2 | 1 | 283 | 79 | - |
| 79 | 55204 | Caridean Epaulettes | 4/2 | 3 | 308 | 78 | - |
| 79 | 55688 | Yojamba Shoulders | 4/2 | 3 | 283 | 79 | - |
| 79 | 55643 | Yojamba Jerkin | 4/2 | 5 | 283 | 79 | - |
| 79 | 55642 | Mosshide Jerkin | 4/2 | 5 | 278 | 78 | - |
| 79 | 55697 | Yojamba Waistband | 4/2 | 6 | 283 | 79 | - |
| 79 | 55696 | Mosshide Waistband | 4/2 | 6 | 278 | 78 | - |
| 79 | 55273 | Beauty's Chew Toy | 4/2 | 7 | 308 | 78 | - |
| 79 | 55679 | Yojamba Britches | 4/2 | 7 | 283 | 79 | - |
| 79 | 55235 | Decapod Slippers | 4/2 | 8 | 308 | 78 | - |
| 79 | 55652 | Yojamba Boots | 4/2 | 8 | 283 | 79 | - |
| 79 | 55264 | Armbands of Change | 4/2 | 9 | 308 | 78 | - |
| 79 | 55634 | Yojamba Bindings | 4/2 | 9 | 283 | 79 | - |
| 79 | 55661 | Yojamba Gloves | 4/2 | 10 | 283 | 79 | - |
| 79 | 55660 | Mosshide Gloves | 4/2 | 10 | 278 | 78 | - |
| 79 | 35677 | Cosmos Vestments | 4/2 | 20 | 183 | 77 | - |
| 79 | 31041 | Thunderheart Tunic | 4/2 | 20 | 146 | 70 | - |
| 79 | 55263 | Renouncer's Cowl | 4/3 | 1 | 308 | 78 | - |
| 79 | 55598 | Sundown Helm | 4/3 | 1 | 283 | 79 | - |
| 79 | 55616 | Sundown Spaulders | 4/3 | 3 | 283 | 79 | - |
| 79 | 55615 | Southfury Spaulders | 4/3 | 3 | 278 | 78 | - |
| 79 | 55205 | Chromis Chestpiece | 4/3 | 5 | 308 | 78 | - |
| 79 | 55253 | Wentletrap Vest | 4/3 | 5 | 308 | 78 | - |
| 79 | 55625 | Sundown Belt | 4/3 | 6 | 283 | 79 | - |
| 79 | 55624 | Southfury Belt | 4/3 | 6 | 278 | 78 | - |
| 79 | 55269 | Heat Wave Leggings | 4/3 | 7 | 308 | 78 | - |
| 79 | 55607 | Sundown Legguards | 4/3 | 7 | 283 | 79 | - |
| 79 | 55580 | Sundown Greaves | 4/3 | 8 | 283 | 79 | - |
| 79 | 55579 | Southfury Greaves | 4/3 | 8 | 278 | 78 | - |
| 79 | 55279 | Manacles of Pain | 4/3 | 9 | 308 | 78 | - |
| 79 | 55562 | Sundown Bracers | 4/3 | 9 | 283 | 79 | - |
| 79 | 55195 | Wrasse Handwraps | 4/3 | 10 | 308 | 78 | - |
| 79 | 55589 | Sundown Gauntlets | 4/3 | 10 | 283 | 79 | - |
| 79 | 35042 | Brutal Gladiator's Linked Armor | 4/3 | 20 | 159 | 70 | - |
| 79 | 35048 | Brutal Gladiator's Mail Armor | 4/3 | 20 | 159 | 70 | - |
| 79 | 55229 | Anomuran Helm | 4/4 | 1 | 308 | 78 | - |
| 79 | 55526 | Jasperlode Helm | 4/4 | 1 | 283 | 79 | - |
| 79 | 55207 | Harp Shell Pauldrons | 4/4 | 3 | 308 | 78 | - |
| 79 | 55779 | Raz's Pauldrons | 4/4 | 3 | 308 | 78 | - |
| 79 | 55254 | Abalone Plate Armor | 4/4 | 5 | 308 | 78 | - |
| 79 | 55274 | Beauty's Plate | 4/4 | 5 | 308 | 78 | - |
| 79 | 55553 | Jasperlode Girdle | 4/4 | 6 | 283 | 79 | - |
| 79 | 55552 | Rethban Girdle | 4/4 | 6 | 278 | 78 | - |
| 79 | 55249 | Triton Legplates | 4/4 | 7 | 308 | 78 | - |
| 79 | 55260 | Alpheus Legguards | 4/4 | 7 | 308 | 78 | - |
| 79 | 55508 | Jasperlode Sabatons | 4/4 | 8 | 283 | 79 | - |
| 79 | 55507 | Rethban Sabatons | 4/4 | 8 | 278 | 78 | - |
| 79 | 55268 | Bracers of Cooled Anger | 4/4 | 9 | 308 | 78 | - |
| 79 | 55490 | Jasperlode Vambraces | 4/4 | 9 | 283 | 79 | - |
| 79 | 55784 | Clutches of Dying Light | 4/4 | 10 | 308 | 78 | - |
| 79 | 55517 | Jasperlode Gauntlets | 4/4 | 10 | 283 | 79 | - |
| 79 | 55778 | Shield of the Iron Maiden | 4/6 | 14 | 308 | 78 | - |
| 79 | 55788 | Crepuscular Shield | 4/6 | 14 | 308 | 78 | - |
| 80 | 87516 | Scalebreaker Axe | 2/0 | 13 | 409 | 80 | - |
| 80 | 55203 | Lightning Whelk Axe | 2/0 | 13 | 308 | 78 | - |
| 80 | 51440 | Wrathful Gladiator's Dicer | 2/0 | 22 | 277 | 80 | - |
| 80 | 51526 | Wrathful Gladiator's Splitter | 2/0 | 22 | 277 | 80 | - |
| 80 | 55252 | Whitefin Axe | 2/1 | 17 | 308 | 78 | - |
| 80 | 55293 | Nethergarde Broadaxe | 2/1 | 17 | 289 | 80 | - |
| 80 | 55782 | Amber Messenger | 2/2 | 15 | 308 | 78 | - |
| 80 | 55302 | Nethergarde Bow | 2/2 | 15 | 289 | 80 | - |
| 80 | 87477 | Penate's Penance-Procuring Pistol | 2/3 | 26 | 409 | 80 | - |
| 80 | 66953 | Snub-Nosed Pistol | 2/3 | 26 | 292 | 80 | - |
| 80 | 87515 | Ced's Crusher | 2/4 | 13 | 409 | 80 | - |
| 80 | 55777 | Torturer's Mercy | 2/4 | 13 | 308 | 78 | - |
| 80 | 51446 | Wrathful Gladiator's Punisher | 2/4 | 22 | 277 | 80 | - |
| 80 | 51445 | Wrathful Gladiator's Bonecracker | 2/4 | 22 | 264 | 80 | - |
| 80 | 55392 | Nethergarde Hammer | 2/5 | 17 | 289 | 80 | - |
| 80 | 55391 | Angerfang Hammer | 2/5 | 17 | 283 | 79 | - |
| 80 | 55410 | Nethergarde Spear | 2/6 | 17 | 289 | 80 | - |
| 80 | 50735 | Oathbinder, Charge of the Ranger-General | 2/6 | 17 | 284 | 80 | - |
| 80 | 55455 | Nethergarde Sword | 2/7 | 13 | 289 | 80 | - |
| 80 | 50732 | Bloodsurge, Kel'Thuzad's Blade of Agony | 2/7 | 13 | 284 | 80 | - |
| 80 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 80 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 80 | 51448 | Wrathful Gladiator's Swiftblade | 2/7 | 22 | 277 | 80 | - |
| 80 | 51447 | Wrathful Gladiator's Quickblade | 2/7 | 22 | 264 | 80 | - |
| 80 | 87423 | Bjam's Door-Breaker | 2/8 | 17 | 409 | 80 | - |
| 80 | 66978 | Rage-Possessed Greatsword | 2/8 | 17 | 292 | 80 | - |
| 80 | 87422 | Matsuba's Breadmaker | 2/10 | 17 | 409 | 80 | - |
| 80 | 55228 | Cerith Spire Staff | 2/10 | 17 | 308 | 78 | - |
| 80 | 55347 | Nethergarde Knuckles | 2/13 | 13 | 289 | 80 | - |
| 80 | 89575 | Handwraps of Serenity | 2/13 | 13 | 289 | 80 | - |
| 80 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 80 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 80 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 80 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 80 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 80 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 80 | 87467 | Ravenclaw Harbinger | 2/15 | 13 | 409 | 80 | - |
| 80 | 55272 | Steelbender's Masterpiece | 2/15 | 13 | 308 | 78 | - |
| 80 | 51442 | Wrathful Gladiator's Dirk | 2/15 | 22 | 277 | 80 | - |
| 80 | 51528 | Wrathful Gladiator's Eviscerator | 2/15 | 22 | 277 | 80 | - |
| 80 | 55320 | Nethergarde Crossbow | 2/18 | 26 | 289 | 80 | - |
| 80 | 50733 | Fal'inrush, Defender of Quel'thalas | 2/18 | 26 | 284 | 80 | - |
| 80 | 55267 | Corla's Baton | 2/19 | 26 | 308 | 78 | - |
| 80 | 55482 | Mirkfallon Wand | 2/19 | 26 | 289 | 80 | - |
| 80 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 80 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 80 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 80 | 55258 | Pipefish Cord | 4/0 | 2 | 308 | 78 | - |
| 80 | 55262 | Barnacle Pendant | 4/0 | 2 | 308 | 78 | - |
| 80 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 80 | 87442 | Signet of the Scorned | 4/0 | 11 | 409 | 80 | - |
| 80 | 55201 | Entwined Nereis | 4/0 | 11 | 308 | 78 | - |
| 80 | 87498 | Spiritwhisper Conch | 4/0 | 12 | 409 | 80 | - |
| 80 | 87499 | Grakl's Gleaming Talisman | 4/0 | 12 | 409 | 80 | - |
| 80 | 55259 | Bioluminescent Lamp | 4/0 | 23 | 308 | 78 | - |
| 80 | 55277 | Beauty's Favorite Bone | 4/0 | 23 | 308 | 78 | - |
| 80 | 55198 | Aurelian Mitre | 4/1 | 1 | 308 | 78 | - |
| 80 | 55743 | Darrowmere Cowl | 4/1 | 1 | 289 | 80 | - |
| 80 | 55786 | Kyrstel Mantle | 4/1 | 3 | 308 | 78 | - |
| 80 | 55761 | Darrowmere Shoulderpads | 4/1 | 3 | 289 | 80 | - |
| 80 | 46130 | Conqueror's Kirin Tor Tunic | 4/1 | 5 | 226 | 80 | - |
| 80 | 45368 | Valorous Kirin Tor Tunic | 4/1 | 5 | 219 | 80 | - |
| 80 | 55275 | Beauty's Silken Ribbon | 4/1 | 6 | 308 | 78 | - |
| 80 | 55770 | Darrowmere Cord | 4/1 | 6 | 289 | 80 | - |
| 80 | 55752 | Darrowmere Leggings | 4/1 | 7 | 289 | 80 | - |
| 80 | 57280 | Kilt of Reborn Future | 4/1 | 7 | 288 | 0 | 80 |
| 80 | 55725 | Darrowmere Sandals | 4/1 | 8 | 289 | 80 | - |
| 80 | 55724 | Baradin Sandals | 4/1 | 8 | 283 | 79 | - |
| 80 | 87520 | Glowing Wind Bracers | 4/1 | 9 | 409 | 80 | - |
| 80 | 67088 | Falling Snow Bracers | 4/1 | 9 | 292 | 80 | - |
| 80 | 87447 | Toad-Slime Gloves | 4/1 | 10 | 409 | 80 | - |
| 80 | 55255 | Mnemiopsis Gloves | 4/1 | 10 | 308 | 78 | - |
| 80 | 55202 | Periwinkle Cloak | 4/1 | 16 | 308 | 78 | - |
| 80 | 55206 | Eagle Ray Cloak | 4/1 | 16 | 308 | 78 | - |
| 80 | 55278 | Inquisition Robes | 4/1 | 20 | 308 | 78 | - |
| 80 | 65657 | Deepmist Robes | 4/1 | 20 | 308 | 0 | 80 |
| 80 | 55785 | Willowy Crown | 4/2 | 1 | 308 | 78 | - |
| 80 | 55671 | Aboraz Hood | 4/2 | 1 | 289 | 80 | - |
| 80 | 55204 | Caridean Epaulettes | 4/2 | 3 | 308 | 78 | - |
| 80 | 55689 | Aboraz Shoulders | 4/2 | 3 | 289 | 80 | - |
| 80 | 65659 | Kelp Forest Vest | 4/2 | 5 | 308 | 0 | 80 |
| 80 | 55644 | Aboraz Jerkin | 4/2 | 5 | 289 | 80 | - |
| 80 | 55698 | Aboraz Waistband | 4/2 | 6 | 289 | 80 | - |
| 80 | 55697 | Yojamba Waistband | 4/2 | 6 | 283 | 79 | - |
| 80 | 55273 | Beauty's Chew Toy | 4/2 | 7 | 308 | 78 | - |
| 80 | 55680 | Aboraz Britches | 4/2 | 7 | 289 | 80 | - |
| 80 | 55235 | Decapod Slippers | 4/2 | 8 | 308 | 78 | - |
| 80 | 55653 | Aboraz Boots | 4/2 | 8 | 289 | 80 | - |
| 80 | 87522 | Star-Carrier Bracers | 4/2 | 9 | 409 | 80 | - |
| 80 | 55264 | Armbands of Change | 4/2 | 9 | 308 | 78 | - |
| 80 | 55662 | Aboraz Gloves | 4/2 | 10 | 289 | 80 | - |
| 80 | 57266 | Gloves of Nurtured Truth | 4/2 | 10 | 288 | 0 | 80 |
| 80 | 66886 | Acid-Eaten Vest | 4/2 | 20 | 308 | 0 | 80 |
| 80 | 50649 | Deathwhisper Raiment | 4/2 | 20 | 277 | 80 | - |
| 80 | 87456 | Hexu's Amplifying Helm | 4/3 | 1 | 409 | 80 | - |
| 80 | 55263 | Renouncer's Cowl | 4/3 | 1 | 308 | 78 | - |
| 80 | 55617 | Bramblescar Spaulders | 4/3 | 3 | 289 | 80 | - |
| 80 | 59783 | Damselfish Shoulderguards | 4/3 | 3 | 288 | 0 | 80 |
| 80 | 55205 | Chromis Chestpiece | 4/3 | 5 | 308 | 78 | - |
| 80 | 55253 | Wentletrap Vest | 4/3 | 5 | 308 | 78 | - |
| 80 | 65658 | Stonespeaker's Belt | 4/3 | 6 | 308 | 0 | 80 |
| 80 | 66887 | Beauty Jr.'s Collar | 4/3 | 6 | 308 | 0 | 80 |
| 80 | 87464 | Leggings of the Scorched Man | 4/3 | 7 | 409 | 80 | - |
| 80 | 55269 | Heat Wave Leggings | 4/3 | 7 | 308 | 78 | - |
| 80 | 55581 | Bramblescar Greaves | 4/3 | 8 | 289 | 80 | - |
| 80 | 57265 | Treads of Restoration | 4/3 | 8 | 288 | 0 | 80 |
| 80 | 55279 | Manacles of Pain | 4/3 | 9 | 308 | 78 | - |
| 80 | 55563 | Bramblescar Bracers | 4/3 | 9 | 289 | 80 | - |
| 80 | 55195 | Wrasse Handwraps | 4/3 | 10 | 308 | 78 | - |
| 80 | 55590 | Bramblescar Gauntlets | 4/3 | 10 | 289 | 80 | - |
| 80 | 51497 | Wrathful Gladiator's Ringmail Armor | 4/3 | 20 | 270 | 80 | - |
| 80 | 51503 | Wrathful Gladiator's Linked Armor | 4/3 | 20 | 270 | 80 | - |
| 80 | 55229 | Anomuran Helm | 4/4 | 1 | 308 | 78 | - |
| 80 | 55527 | Direforge Helm | 4/4 | 1 | 289 | 80 | - |
| 80 | 87494 | Shoulderguards of Honest Labor | 4/4 | 3 | 409 | 80 | - |
| 80 | 55207 | Harp Shell Pauldrons | 4/4 | 3 | 308 | 78 | - |
| 80 | 87426 | Deathspite Breastplate | 4/4 | 5 | 409 | 80 | - |
| 80 | 55254 | Abalone Plate Armor | 4/4 | 5 | 308 | 78 | - |
| 80 | 87506 | Cheng's Irrepressible Girdle | 4/4 | 6 | 409 | 80 | - |
| 80 | 65654 | Belt of a Thousand Deaths | 4/4 | 6 | 308 | 0 | 80 |
| 80 | 55249 | Triton Legplates | 4/4 | 7 | 308 | 78 | - |
| 80 | 55260 | Alpheus Legguards | 4/4 | 7 | 308 | 78 | - |
| 80 | 55509 | Direforge Sabatons | 4/4 | 8 | 289 | 80 | - |
| 80 | 57260 | Nemesis Crushers | 4/4 | 8 | 288 | 0 | 80 |
| 80 | 87521 | Zoid's Scorched Bracers | 4/4 | 9 | 409 | 80 | - |
| 80 | 55268 | Bracers of Cooled Anger | 4/4 | 9 | 308 | 78 | - |
| 80 | 55784 | Clutches of Dying Light | 4/4 | 10 | 308 | 78 | - |
| 80 | 55518 | Direforge Gauntlets | 4/4 | 10 | 289 | 80 | - |
| 80 | 87488 | Don Carlos' Drifting Shield | 4/6 | 14 | 417 | 80 | - |
| 80 | 55778 | Shield of the Iron Maiden | 4/6 | 14 | 308 | 78 | - |
| 84 | 87513 | Axe of the Iron Price | 2/0 | 13 | 417 | 81 | - |
| 84 | 87516 | Scalebreaker Axe | 2/0 | 13 | 409 | 80 | - |
| 84 | 51440 | Wrathful Gladiator's Dicer | 2/0 | 22 | 277 | 80 | - |
| 84 | 51526 | Wrathful Gladiator's Splitter | 2/0 | 22 | 277 | 80 | - |
| 84 | 87421 | Fellworth's Repetitive Greataxe | 2/1 | 17 | 425 | 82 | - |
| 84 | 82047 | Bronzed Broadaxe | 2/1 | 17 | 377 | 84 | - |
| 84 | 82046 | Bronzed Bow | 2/2 | 15 | 377 | 84 | - |
| 84 | 82018 | Polished Bow | 2/2 | 15 | 364 | 83 | - |
| 84 | 87476 | Wall Guardian's Longrifle | 2/3 | 26 | 425 | 82 | - |
| 84 | 87477 | Penate's Penance-Procuring Pistol | 2/3 | 26 | 409 | 80 | - |
| 84 | 87465 | Solianti's Insect Smasher | 2/4 | 13 | 442 | 84 | - |
| 84 | 87512 | Porter's Tooth-Marked Mace | 2/4 | 13 | 442 | 84 | - |
| 84 | 51446 | Wrathful Gladiator's Punisher | 2/4 | 22 | 277 | 80 | - |
| 84 | 51445 | Wrathful Gladiator's Bonecracker | 2/4 | 22 | 264 | 80 | - |
| 84 | 82052 | Bronzed Hammer | 2/5 | 17 | 377 | 84 | - |
| 84 | 82008 | Polished Hammer | 2/5 | 17 | 364 | 83 | - |
| 84 | 87420 | Ritter's Rat Render | 2/6 | 17 | 433 | 83 | - |
| 84 | 82056 | Bronzed Spear | 2/6 | 17 | 377 | 84 | - |
| 84 | 87511 | Horrifying Meatsaw | 2/7 | 13 | 433 | 83 | - |
| 84 | 87466 | Ritualistic Sickle | 2/7 | 13 | 425 | 82 | - |
| 84 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 84 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 84 | 51448 | Wrathful Gladiator's Swiftblade | 2/7 | 22 | 277 | 80 | - |
| 84 | 51447 | Wrathful Gladiator's Quickblade | 2/7 | 22 | 264 | 80 | - |
| 84 | 87423 | Bjam's Door-Breaker | 2/8 | 17 | 409 | 80 | - |
| 84 | 82050 | Bronzed Greatsword | 2/8 | 17 | 377 | 84 | - |
| 84 | 87417 | Staff of Broken Hopes | 2/10 | 17 | 442 | 84 | - |
| 84 | 87422 | Matsuba's Breadmaker | 2/10 | 17 | 409 | 80 | - |
| 84 | 82054 | Bronzed Knuckles | 2/13 | 13 | 377 | 84 | - |
| 84 | 82013 | Polished Knuckles | 2/13 | 13 | 364 | 83 | - |
| 84 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 84 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 84 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 84 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 84 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 84 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 84 | 87508 | Lhakaz's Missing Ribspreader | 2/15 | 13 | 442 | 84 | - |
| 84 | 87514 | Sharpened Multi-Prong Dagger | 2/15 | 13 | 417 | 81 | - |
| 84 | 51442 | Wrathful Gladiator's Dirk | 2/15 | 22 | 277 | 80 | - |
| 84 | 51528 | Wrathful Gladiator's Eviscerator | 2/15 | 22 | 277 | 80 | - |
| 84 | 82048 | Bronzed Crossbow | 2/18 | 26 | 377 | 84 | - |
| 84 | 82016 | Polished Crossbow | 2/18 | 26 | 364 | 83 | - |
| 84 | 87475 | Wand of Stultifying Smoke | 2/19 | 26 | 433 | 83 | - |
| 84 | 87478 | Wand of the Exiled Path | 2/19 | 26 | 417 | 81 | - |
| 84 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 84 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 84 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 84 | 87470 | Scabripper Necklace | 4/0 | 2 | 433 | 83 | - |
| 84 | 87471 | Stitcher's Spare Needles | 4/0 | 2 | 425 | 82 | - |
| 84 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 84 | 87438 | Signet of the Binding Wall | 4/0 | 11 | 433 | 83 | - |
| 84 | 87439 | Pestilential Ring | 4/0 | 11 | 433 | 83 | - |
| 84 | 87495 | Gerp's Perfect Arrow | 4/0 | 12 | 442 | 84 | - |
| 84 | 87497 | Core of Decency | 4/0 | 12 | 433 | 83 | - |
| 84 | 87458 | Bone-Inlaid Fan | 4/0 | 23 | 433 | 83 | - |
| 84 | 82062 | Gemmed Crystal | 4/0 | 23 | 377 | 84 | - |
| 84 | 87450 | Hood of Alchemical Vapors | 4/1 | 1 | 442 | 84 | - |
| 84 | 82030 | Reed Cowl | 4/1 | 1 | 377 | 84 | - |
| 84 | 87490 | Orb-Bound Shoulders | 4/1 | 3 | 433 | 83 | - |
| 84 | 87491 | Spaulders of Five Rings | 4/1 | 3 | 417 | 81 | - |
| 84 | 46130 | Conqueror's Kirin Tor Tunic | 4/1 | 5 | 226 | 80 | - |
| 84 | 45368 | Valorous Kirin Tor Tunic | 4/1 | 5 | 219 | 80 | - |
| 84 | 82029 | Reed Cord | 4/1 | 6 | 377 | 84 | - |
| 84 | 81966 | Stitched Cord | 4/1 | 6 | 364 | 83 | - |
| 84 | 87463 | Very Manly Leggings | 4/1 | 7 | 425 | 82 | - |
| 84 | 82032 | Reed Leggings | 4/1 | 7 | 377 | 84 | - |
| 84 | 87434 | Sandals of Sympathetic Healing | 4/1 | 8 | 425 | 82 | - |
| 84 | 82034 | Reed Sandals | 4/1 | 8 | 377 | 84 | - |
| 84 | 87520 | Glowing Wind Bracers | 4/1 | 9 | 409 | 80 | - |
| 84 | 82036 | Reed Wristwraps | 4/1 | 9 | 377 | 84 | - |
| 84 | 87447 | Toad-Slime Gloves | 4/1 | 10 | 409 | 80 | - |
| 84 | 82031 | Reed Handwraps | 4/1 | 10 | 377 | 84 | - |
| 84 | 87429 | Drape of Flowing Gauze | 4/1 | 16 | 442 | 84 | - |
| 84 | 87430 | Brilliant Cloak of Wu the Younger | 4/1 | 16 | 442 | 84 | - |
| 84 | 87484 | Robes of the Quiet Isle | 4/1 | 20 | 433 | 83 | - |
| 84 | 87485 | Robes of Forgone Hope | 4/1 | 20 | 417 | 81 | - |
| 84 | 82041 | Whisper Hood | 4/2 | 1 | 377 | 84 | - |
| 84 | 81977 | Silent Hood | 4/2 | 1 | 364 | 83 | - |
| 84 | 87493 | Enokian Growth Shoulderguards | 4/2 | 3 | 425 | 82 | - |
| 84 | 87492 | Mantle of Bestial Force | 4/2 | 3 | 417 | 81 | - |
| 84 | 87425 | Chestguard of the Solicitious Pandaren | 4/2 | 5 | 442 | 84 | - |
| 84 | 82042 | Whisper Jerkin | 4/2 | 5 | 377 | 84 | - |
| 84 | 87507 | Melador's Mercurial Wrap | 4/2 | 6 | 417 | 81 | - |
| 84 | 82044 | Whisper Waistband | 4/2 | 6 | 377 | 84 | - |
| 84 | 87462 | Leggings of Perfect Balance | 4/2 | 7 | 425 | 82 | - |
| 84 | 82039 | Whisper Britches | 4/2 | 7 | 377 | 84 | - |
| 84 | 87432 | Tesslah's Ticking Treads | 4/2 | 8 | 433 | 83 | - |
| 84 | 82038 | Whisper Boots | 4/2 | 8 | 377 | 84 | - |
| 84 | 87522 | Star-Carrier Bracers | 4/2 | 9 | 409 | 80 | - |
| 84 | 82037 | Whisper Bindings | 4/2 | 9 | 377 | 84 | - |
| 84 | 87443 | Syrio's Gloves of Tutelage | 4/2 | 10 | 442 | 84 | - |
| 84 | 82040 | Whisper Gloves | 4/2 | 10 | 377 | 84 | - |
| 84 | 66886 | Acid-Eaten Vest | 4/2 | 20 | 308 | 0 | 80 |
| 84 | 50649 | Deathwhisper Raiment | 4/2 | 20 | 277 | 80 | - |
| 84 | 87456 | Hexu's Amplifying Helm | 4/3 | 1 | 409 | 80 | - |
| 84 | 82025 | Steppe Helm | 4/3 | 1 | 377 | 84 | - |
| 84 | 82027 | Steppe Spaulders | 4/3 | 3 | 377 | 84 | - |
| 84 | 81983 | Saurok Spaulders | 4/3 | 3 | 364 | 83 | - |
| 84 | 82028 | Steppe Vest | 4/3 | 5 | 377 | 84 | - |
| 84 | 81988 | Saurok Vest | 4/3 | 5 | 364 | 83 | - |
| 84 | 87503 | Repin's Ammo-Laden Belt | 4/3 | 6 | 433 | 83 | - |
| 84 | 87505 | Spirit-Speaking Belt | 4/3 | 6 | 425 | 82 | - |
| 84 | 87459 | Dills' Primal Leggings | 4/3 | 7 | 442 | 84 | - |
| 84 | 87464 | Leggings of the Scorched Man | 4/3 | 7 | 409 | 80 | - |
| 84 | 87435 | Treads of Exotic Mastery | 4/3 | 8 | 425 | 82 | - |
| 84 | 82024 | Steppe Greaves | 4/3 | 8 | 377 | 84 | - |
| 84 | 87519 | Misty Morning Wristguards | 4/3 | 9 | 417 | 81 | - |
| 84 | 82022 | Steppe Bracers | 4/3 | 9 | 377 | 84 | - |
| 84 | 87448 | Toremu's Crackling Grips | 4/3 | 10 | 417 | 81 | - |
| 84 | 82023 | Steppe Gauntlets | 4/3 | 10 | 377 | 84 | - |
| 84 | 51497 | Wrathful Gladiator's Ringmail Armor | 4/3 | 20 | 270 | 80 | - |
| 84 | 51503 | Wrathful Gladiator's Linked Armor | 4/3 | 20 | 270 | 80 | - |
| 84 | 87454 | Light-Sealed Casque | 4/4 | 1 | 442 | 84 | - |
| 84 | 87453 | Endwyn's Toothy Visage | 4/4 | 1 | 433 | 83 | - |
| 84 | 87494 | Shoulderguards of Honest Labor | 4/4 | 3 | 409 | 80 | - |
| 84 | 82072 | Brick Pauldrons | 4/4 | 3 | 377 | 84 | - |
| 84 | 87426 | Deathspite Breastplate | 4/4 | 5 | 409 | 80 | - |
| 84 | 82067 | Brick Breastplate | 4/4 | 5 | 377 | 84 | - |
| 84 | 87504 | Waistguard of Flaming Words | 4/4 | 6 | 417 | 81 | - |
| 84 | 87506 | Cheng's Irrepressible Girdle | 4/4 | 6 | 409 | 80 | - |
| 84 | 87461 | Greatheart Pants | 4/4 | 7 | 425 | 82 | - |
| 84 | 87460 | Legplates of the Wandering Warrior | 4/4 | 7 | 417 | 81 | - |
| 84 | 87433 | Blackfooted Greatboots | 4/4 | 8 | 433 | 83 | - |
| 84 | 87436 | Sollerets of Immovability | 4/4 | 8 | 417 | 81 | - |
| 84 | 87518 | Bracers of the Neux Dawn | 4/4 | 9 | 433 | 83 | - |
| 84 | 87521 | Zoid's Scorched Bracers | 4/4 | 9 | 409 | 80 | - |
| 84 | 87445 | Gauntlets of Blurring Fingers | 4/4 | 10 | 442 | 84 | - |
| 84 | 87446 | Gauntlets of the Solemn Charge | 4/4 | 10 | 442 | 84 | - |
| 84 | 87486 | Skyrider's Shining Shield | 4/6 | 14 | 442 | 84 | - |
| 84 | 87488 | Don Carlos' Drifting Shield | 4/6 | 14 | 417 | 80 | - |
| 85 | 87646 | Needlefang Throatripper | 2/0 | 13 | 420 | 85 | - |
| 85 | 87513 | Axe of the Iron Price | 2/0 | 13 | 417 | 81 | - |
| 85 | 59462 | Maimgor's Bite | 2/0 | 22 | 359 | 85 | - |
| 85 | 57928 | Windslicer | 2/0 | 22 | 346 | 85 | - |
| 85 | 44924 | Sun-Lute of the Phoenix King | 2/1 | 17 | 483 | 85 | - |
| 85 | 87421 | Fellworth's Repetitive Greataxe | 2/1 | 17 | 425 | 82 | - |
| 85 | 87474 | Ennadee's Twirling Longbow | 2/2 | 15 | 450 | 85 | - |
| 85 | 78471 | Vishanka, Jaws of the Earth | 2/2 | 15 | 416 | 85 | - |
| 85 | 87476 | Wall Guardian's Longrifle | 2/3 | 26 | 425 | 82 | - |
| 85 | 73427 | Cataclysmic Gladiator's Rifle | 2/3 | 26 | 410 | 85 | - |
| 85 | 87465 | Solianti's Insect Smasher | 2/4 | 13 | 442 | 84 | - |
| 85 | 87512 | Porter's Tooth-Marked Mace | 2/4 | 13 | 442 | 84 | - |
| 85 | 51446 | Wrathful Gladiator's Punisher | 2/4 | 22 | 277 | 80 | - |
| 85 | 51445 | Wrathful Gladiator's Bonecracker | 2/4 | 22 | 264 | 80 | - |
| 85 | 87418 | Lerah's Ribsmasher | 2/5 | 17 | 450 | 85 | - |
| 85 | 87643 | Fangcracker Battlemace | 2/5 | 17 | 420 | 85 | - |
| 85 | 87420 | Ritter's Rat Render | 2/6 | 17 | 433 | 83 | - |
| 85 | 78473 | Kiril, Fury of Beasts | 2/6 | 17 | 416 | 85 | - |
| 85 | 87511 | Horrifying Meatsaw | 2/7 | 13 | 433 | 83 | - |
| 85 | 87466 | Ritualistic Sickle | 2/7 | 13 | 425 | 82 | - |
| 85 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 85 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 85 | 57929 | Dawnblaze Blade | 2/7 | 22 | 346 | 85 | - |
| 85 | 51448 | Wrathful Gladiator's Swiftblade | 2/7 | 22 | 277 | 80 | - |
| 85 | 78478 | Gurthalak, Voice of the Deeps | 2/8 | 17 | 416 | 85 | - |
| 85 | 73442 | Cataclysmic Gladiator's Greatsword | 2/8 | 17 | 410 | 85 | - |
| 85 | 87419 | Kaleiki's Lost Training Staff | 2/10 | 17 | 450 | 85 | - |
| 85 | 87417 | Staff of Broken Hopes | 2/10 | 17 | 442 | 84 | - |
| 85 | 87509 | Fist of Fate | 2/13 | 13 | 450 | 85 | - |
| 85 | 73418 | Cataclysmic Gladiator's Slasher | 2/13 | 13 | 410 | 85 | - |
| 85 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 85 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 85 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 85 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 85 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 85 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 85 | 87508 | Lhakaz's Missing Ribspreader | 2/15 | 13 | 442 | 84 | - |
| 85 | 87514 | Sharpened Multi-Prong Dagger | 2/15 | 13 | 417 | 81 | - |
| 85 | 77949 | Golad, Twilight of Aspects | 2/15 | 21 | 416 | 85 | 85 |
| 85 | 77947 | The Sleeper | 2/15 | 21 | 406 | 85 | 85 |
| 85 | 77950 | Tiriosh, Nightmare of Ages | 2/15 | 22 | 416 | 85 | 85 |
| 85 | 73428 | Cataclysmic Gladiator's Shiv | 2/15 | 22 | 410 | 85 | - |
| 85 | 87650 | Fishsticker Crossbow | 2/18 | 26 | 420 | 85 | - |
| 85 | 73430 | Cataclysmic Gladiator's Heavy Crossbow | 2/18 | 26 | 410 | 85 | - |
| 85 | 87475 | Wand of Stultifying Smoke | 2/19 | 26 | 433 | 83 | - |
| 85 | 87478 | Wand of the Exiled Path | 2/19 | 26 | 417 | 81 | - |
| 85 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 85 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 85 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 85 | 87469 | Astrylian's Cheerful Charm | 4/0 | 2 | 450 | 85 | - |
| 85 | 87622 | Yoke of Niuzao | 4/0 | 2 | 440 | 85 | - |
| 85 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 85 | 87437 | Anaxo's Multi-Layered Seal | 4/0 | 11 | 450 | 85 | - |
| 85 | 87438 | Signet of the Binding Wall | 4/0 | 11 | 433 | 83 | - |
| 85 | 104321 | Captain Zvezdan's Lost Leg | 4/0 | 12 | 496 | 85 | - |
| 85 | 75274 | Zen Alchemist Stone | 4/0 | 12 | 458 | 85 | 85 |
| 85 | 87457 | Record of Mysterious Deeds | 4/0 | 23 | 450 | 85 | - |
| 85 | 87458 | Bone-Inlaid Fan | 4/0 | 23 | 433 | 83 | - |
| 85 | 87450 | Hood of Alchemical Vapors | 4/1 | 1 | 442 | 84 | - |
| 85 | 87604 | Beachcomber's Hat | 4/1 | 1 | 420 | 85 | - |
| 85 | 87490 | Orb-Bound Shoulders | 4/1 | 3 | 433 | 83 | - |
| 85 | 87491 | Spaulders of Five Rings | 4/1 | 3 | 417 | 81 | - |
| 85 | 65174 | Corsair's Overshirt | 4/1 | 5 | 346 | 85 | - |
| 85 | 46130 | Conqueror's Kirin Tor Tunic | 4/1 | 5 | 226 | 80 | - |
| 85 | 87502 | Repression Belt | 4/1 | 6 | 450 | 85 | - |
| 85 | 78391 | Cord of the Slain Champion | 4/1 | 6 | 410 | 85 | - |
| 85 | 87463 | Very Manly Leggings | 4/1 | 7 | 425 | 82 | - |
| 85 | 78719 | Legwraps of Dying Light | 4/1 | 7 | 410 | 85 | - |
| 85 | 87434 | Sandals of Sympathetic Healing | 4/1 | 8 | 425 | 82 | - |
| 85 | 78449 | Janglespur Jackboots | 4/1 | 8 | 410 | 85 | - |
| 85 | 87615 | Yakwasher's Bracers | 4/1 | 9 | 430 | 85 | - |
| 85 | 87616 | Mountain Trailblazer's Cuffs | 4/1 | 9 | 430 | 85 | - |
| 85 | 87444 | Wind-Kissed Handwraps | 4/1 | 10 | 450 | 85 | - |
| 85 | 78461 | Gloves of Liquid Smoke | 4/1 | 10 | 416 | 85 | - |
| 85 | 87427 | Blade-Dulling Greatcloak | 4/1 | 16 | 450 | 85 | - |
| 85 | 87429 | Drape of Flowing Gauze | 4/1 | 16 | 442 | 84 | - |
| 85 | 87634 | Mazu's Robe | 4/1 | 20 | 450 | 85 | - |
| 85 | 87635 | Amber-Starched Robes | 4/1 | 20 | 450 | 85 | - |
| 85 | 87452 | Zom's Misty Hood | 4/2 | 1 | 450 | 85 | - |
| 85 | 87605 | Crest of the Red Crane | 4/2 | 1 | 420 | 85 | - |
| 85 | 87493 | Enokian Growth Shoulderguards | 4/2 | 3 | 425 | 82 | - |
| 85 | 87492 | Mantle of Bestial Force | 4/2 | 3 | 417 | 81 | - |
| 85 | 87425 | Chestguard of the Solicitious Pandaren | 4/2 | 5 | 442 | 84 | - |
| 85 | 87631 | Jiao-Skin Tunic | 4/2 | 5 | 440 | 85 | - |
| 85 | 87507 | Melador's Mercurial Wrap | 4/2 | 6 | 417 | 81 | - |
| 85 | 78388 | Belt of Flayed Skin | 4/2 | 6 | 410 | 85 | - |
| 85 | 87462 | Leggings of Perfect Balance | 4/2 | 7 | 425 | 82 | - |
| 85 | 78708 | Blackfang Battleweave Legguards | 4/2 | 7 | 410 | 85 | - |
| 85 | 87432 | Tesslah's Ticking Treads | 4/2 | 8 | 433 | 83 | - |
| 85 | 78462 | Molten Blood Footpads | 4/2 | 8 | 416 | 85 | - |
| 85 | 87517 | Bracers of Rolling Fields | 4/2 | 9 | 450 | 85 | - |
| 85 | 87613 | Frozen Zandalari Bracer | 4/2 | 9 | 430 | 85 | - |
| 85 | 87443 | Syrio's Gloves of Tutelage | 4/2 | 10 | 442 | 84 | - |
| 85 | 78676 | Deep Earth Gloves | 4/2 | 10 | 410 | 85 | - |
| 85 | 78660 | Deep Earth Robes | 4/2 | 20 | 410 | 85 | - |
| 85 | 78662 | Deep Earth Vestment | 4/2 | 20 | 410 | 85 | - |
| 85 | 87451 | Helm of Elemental Binding | 4/3 | 1 | 450 | 85 | - |
| 85 | 87606 | Tidehunter Helm | 4/3 | 1 | 420 | 85 | - |
| 85 | 87489 | Fletcher's Sharpening Shoulders | 4/3 | 3 | 450 | 85 | - |
| 85 | 78435 | Imperfect Specimens 27 and 28 | 4/3 | 3 | 410 | 85 | - |
| 85 | 87630 | Chestpiece of Twinkling Stars | 4/3 | 5 | 440 | 85 | - |
| 85 | 87632 | Fearsworn Chestpiece | 4/3 | 5 | 440 | 85 | - |
| 85 | 87503 | Repin's Ammo-Laden Belt | 4/3 | 6 | 433 | 83 | - |
| 85 | 87505 | Spirit-Speaking Belt | 4/3 | 6 | 425 | 82 | - |
| 85 | 87459 | Dills' Primal Leggings | 4/3 | 7 | 442 | 84 | - |
| 85 | 78704 | Spiritwalker's Legguards | 4/3 | 7 | 410 | 85 | - |
| 85 | 87435 | Treads of Exotic Mastery | 4/3 | 8 | 425 | 82 | - |
| 85 | 78405 | Mindstrainer Treads | 4/3 | 8 | 410 | 85 | - |
| 85 | 87614 | Kafa Picker's Bracers | 4/3 | 9 | 430 | 85 | - |
| 85 | 87618 | Ice Encrusted Bracer | 4/3 | 9 | 430 | 85 | - |
| 85 | 87448 | Toremu's Crackling Grips | 4/3 | 10 | 417 | 81 | - |
| 85 | 78362 | Sporebeard Gauntlets | 4/3 | 10 | 410 | 85 | - |
| 85 | 78723 | Spiritwalker's Hauberk | 4/3 | 20 | 410 | 85 | - |
| 85 | 78724 | Spiritwalker's Cuirass | 4/3 | 20 | 410 | 85 | - |
| 85 | 87454 | Light-Sealed Casque | 4/4 | 1 | 442 | 84 | - |
| 85 | 87453 | Endwyn's Toothy Visage | 4/4 | 1 | 433 | 83 | - |
| 85 | 78465 | Backbreaker Spaulders | 4/4 | 3 | 416 | 85 | - |
| 85 | 78367 | Brackenshell Shoulderplates | 4/4 | 3 | 410 | 85 | - |
| 85 | 87424 | Cryptwarden's Breastplate | 4/4 | 5 | 450 | 85 | - |
| 85 | 87627 | Kunchong Carapace Chestguard | 4/4 | 5 | 440 | 85 | - |
| 85 | 87501 | Unbreakable Waistplate | 4/4 | 6 | 450 | 85 | - |
| 85 | 87504 | Waistguard of Flaming Words | 4/4 | 6 | 417 | 81 | - |
| 85 | 87461 | Greatheart Pants | 4/4 | 7 | 425 | 82 | - |
| 85 | 87460 | Legplates of the Wandering Warrior | 4/4 | 7 | 417 | 81 | - |
| 85 | 87433 | Blackfooted Greatboots | 4/4 | 8 | 433 | 83 | - |
| 85 | 87436 | Sollerets of Immovability | 4/4 | 8 | 417 | 81 | - |
| 85 | 87518 | Bracers of the Neux Dawn | 4/4 | 9 | 433 | 83 | - |
| 85 | 87619 | Terracotta Guardian's Bracer | 4/4 | 9 | 430 | 85 | - |
| 85 | 87445 | Gauntlets of Blurring Fingers | 4/4 | 10 | 442 | 84 | - |
| 85 | 87446 | Gauntlets of the Solemn Charge | 4/4 | 10 | 442 | 84 | - |
| 85 | 71439 | Clutch of the Firemother | 4/4 | 20 | 391 | 85 | - |
| 85 | 71512 | Immolation Battleplate | 4/4 | 20 | 391 | 85 | - |
| 85 | 87487 | Turpster's Sonorous Shield | 4/6 | 14 | 450 | 85 | - |
| 85 | 87486 | Skyrider's Shining Shield | 4/6 | 14 | 442 | 84 | - |
| 89 | 80883 | Crescent of Ichor | 2/0 | 13 | 435 | 87 | - |
| 89 | 82265 | Engraved Axe | 2/0 | 13 | 429 | 88 | - |
| 89 | 59462 | Maimgor's Bite | 2/0 | 22 | 359 | 85 | - |
| 89 | 57928 | Windslicer | 2/0 | 22 | 346 | 85 | - |
| 89 | 44924 | Sun-Lute of the Phoenix King | 2/1 | 17 | 483 | 85 | - |
| 89 | 82267 | Engraved Broadaxe | 2/1 | 17 | 429 | 88 | - |
| 89 | 87474 | Ennadee's Twirling Longbow | 2/2 | 15 | 450 | 85 | - |
| 89 | 100960 | Tempestuous Longbow | 2/2 | 15 | 450 | 89 | - |
| 89 | 82271 | Engraved Gun | 2/3 | 26 | 429 | 88 | - |
| 89 | 87476 | Wall Guardian's Longrifle | 2/3 | 26 | 425 | 82 | - |
| 89 | 80932 | Carapace Breaker | 2/4 | 13 | 450 | 87 | - |
| 89 | 86518 | Yaungol Fire Carrier | 2/4 | 13 | 450 | 86 | - |
| 89 | 51446 | Wrathful Gladiator's Punisher | 2/4 | 22 | 277 | 80 | - |
| 89 | 51445 | Wrathful Gladiator's Bonecracker | 2/4 | 22 | 264 | 80 | - |
| 89 | 87418 | Lerah's Ribsmasher | 2/5 | 17 | 450 | 85 | - |
| 89 | 80936 | Warmace of Taran Zhu | 2/5 | 17 | 435 | 87 | - |
| 89 | 85185 | Ghostheart | 2/6 | 17 | 450 | 88 | - |
| 89 | 86520 | Malik's Stalwart Spear | 2/6 | 17 | 450 | 86 | - |
| 89 | 86522 | Blade of the Prime | 2/7 | 13 | 450 | 86 | - |
| 89 | 86527 | Blade of the Poisoned Mind | 2/7 | 13 | 450 | 86 | - |
| 89 | 32837 | Warglaive of Azzinoth | 2/7 | 21 | 156 | 70 | - |
| 89 | 19865 | Warblade of the Hakkari | 2/7 | 21 | 68 | 60 | - |
| 89 | 57929 | Dawnblaze Blade | 2/7 | 22 | 346 | 85 | - |
| 89 | 51448 | Wrathful Gladiator's Swiftblade | 2/7 | 22 | 277 | 80 | - |
| 89 | 86523 | Swarming Cleaver of Ka'roz | 2/8 | 17 | 450 | 86 | - |
| 89 | 82270 | Engraved Greatsword | 2/8 | 17 | 429 | 88 | - |
| 89 | 86524 | Dissector's Staff of Mutation | 2/10 | 17 | 450 | 86 | - |
| 89 | 87419 | Kaleiki's Lost Training Staff | 2/10 | 17 | 450 | 85 | - |
| 89 | 85183 | Claws of Gekkan | 2/13 | 13 | 450 | 88 | - |
| 89 | 87509 | Fist of Fate | 2/13 | 13 | 450 | 85 | - |
| 89 | 77559 | Replica Grand Marshal's Left Hand Blade | 2/13 | 22 | 78 | 60 | - |
| 89 | 77583 | Replica High Warlord's Left Claw | 2/13 | 22 | 78 | 60 | - |
| 89 | 32912 | Yellow Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 89 | 33016 | Blue Brewfest Stein | 2/14 | 13 | 5 | 1 | 1 |
| 89 | 41755 | The Fire Extinguisher | 2/14 | 17 | 158 | 0 | 74 |
| 89 | 88535 | Sharpened Tuskarr Spear | 2/14 | 21 | 10 | 1 | - |
| 89 | 80933 | Mantid Trochanter | 2/15 | 13 | 450 | 87 | - |
| 89 | 85190 | Firescribe Dagger | 2/15 | 13 | 450 | 88 | - |
| 89 | 77949 | Golad, Twilight of Aspects | 2/15 | 21 | 416 | 85 | 85 |
| 89 | 77947 | The Sleeper | 2/15 | 21 | 406 | 85 | 85 |
| 89 | 77950 | Tiriosh, Nightmare of Ages | 2/15 | 22 | 416 | 85 | 85 |
| 89 | 73428 | Cataclysmic Gladiator's Shiv | 2/15 | 22 | 410 | 85 | - |
| 89 | 82268 | Engraved Crossbow | 2/18 | 26 | 429 | 88 | - |
| 89 | 87650 | Fishsticker Crossbow | 2/18 | 26 | 420 | 85 | - |
| 89 | 87475 | Wand of Stultifying Smoke | 2/19 | 26 | 433 | 83 | - |
| 89 | 82286 | Immaculate Wand | 2/19 | 26 | 429 | 88 | - |
| 89 | 25978 | Seth's Graphite Fishing Pole | 2/20 | 17 | 60 | 0 | 62 |
| 89 | 19022 | Nat Pagle's Extreme Angler FC-5000 | 2/20 | 17 | 50 | 0 | 29 |
| 89 | 65665 | Burgy Blackheart's Handsome Hat | 4/0 | 1 | 1 | 1 | - |
| 89 | 80934 | Pendant of Precise Timing | 4/0 | 2 | 450 | 87 | - |
| 89 | 85177 | Whirling Dervish Choker | 4/0 | 2 | 450 | 88 | - |
| 89 | 50287 | Boots of the Bay | 4/0 | 8 | 1 | 0 | 1 |
| 89 | 85188 | Blade Trap Signet | 4/0 | 11 | 450 | 88 | - |
| 89 | 87437 | Anaxo's Multi-Layered Seal | 4/0 | 11 | 450 | 85 | - |
| 89 | 104321 | Captain Zvezdan's Lost Leg | 4/0 | 12 | 496 | 85 | - |
| 89 | 75274 | Zen Alchemist Stone | 4/0 | 12 | 458 | 85 | 85 |
| 89 | 87457 | Record of Mysterious Deeds | 4/0 | 23 | 450 | 85 | - |
| 89 | 87458 | Bone-Inlaid Fan | 4/0 | 23 | 433 | 83 | - |
| 89 | 80920 | Fallout Filter | 4/1 | 1 | 450 | 87 | - |
| 89 | 100971 | Breezeswept Hood | 4/1 | 1 | 450 | 89 | - |
| 89 | 85194 | Regal Silk Shoulderpads | 4/1 | 3 | 450 | 88 | - |
| 89 | 100972 | Whisperwind Spaulders | 4/1 | 3 | 450 | 89 | - |
| 89 | 65174 | Corsair's Overshirt | 4/1 | 5 | 346 | 85 | - |
| 89 | 46130 | Conqueror's Kirin Tor Tunic | 4/1 | 5 | 226 | 80 | - |
| 89 | 87502 | Repression Belt | 4/1 | 6 | 450 | 85 | - |
| 89 | 82249 | Waterfall Cord | 4/1 | 6 | 429 | 88 | - |
| 89 | 80926 | Leggings of the Frenzy | 4/1 | 7 | 450 | 87 | - |
| 89 | 90721 | Cournith Waterstrider's Silken Finery | 4/1 | 7 | 440 | 87 | - |
| 89 | 85192 | Soulbinder Treads | 4/1 | 8 | 450 | 88 | - |
| 89 | 82254 | Waterfall Sandals | 4/1 | 8 | 429 | 88 | - |
| 89 | 100956 | Bombardment Bracers | 4/1 | 9 | 450 | 89 | - |
| 89 | 87615 | Yakwasher's Bracers | 4/1 | 9 | 430 | 85 | - |
| 89 | 85178 | Conflagrating Gloves | 4/1 | 10 | 450 | 88 | - |
| 89 | 87444 | Wind-Kissed Handwraps | 4/1 | 10 | 450 | 85 | - |
| 89 | 85182 | Cloak of Cleansing Flame | 4/1 | 16 | 450 | 88 | - |
| 89 | 87427 | Blade-Dulling Greatcloak | 4/1 | 16 | 450 | 85 | - |
| 89 | 87634 | Mazu's Robe | 4/1 | 20 | 450 | 85 | - |
| 89 | 87635 | Amber-Starched Robes | 4/1 | 20 | 450 | 85 | - |
| 89 | 87452 | Zom's Misty Hood | 4/2 | 1 | 450 | 85 | - |
| 89 | 100952 | Hood of Viridian Residue | 4/2 | 1 | 450 | 89 | - |
| 89 | 82263 | Vine Shoulders | 4/2 | 3 | 429 | 88 | - |
| 89 | 87493 | Enokian Growth Shoulderguards | 4/2 | 3 | 425 | 82 | - |
| 89 | 100957 | Chestwrap of Arcing Flame | 4/2 | 5 | 450 | 89 | - |
| 89 | 87425 | Chestguard of the Solicitious Pandaren | 4/2 | 5 | 442 | 84 | - |
| 89 | 85176 | Hurricane Belt | 4/2 | 6 | 450 | 88 | - |
| 89 | 82264 | Vine Waistband | 4/2 | 6 | 429 | 88 | - |
| 89 | 80929 | Wall-Breaker Legguards | 4/2 | 7 | 450 | 87 | - |
| 89 | 90720 | Silent Leggings of the Ghostpaw | 4/2 | 7 | 440 | 87 | - |
| 89 | 80922 | Airstream Treads | 4/2 | 8 | 450 | 87 | - |
| 89 | 85187 | Boots of Plummeting Death | 4/2 | 8 | 450 | 88 | - |
| 89 | 80921 | Saboteur's Stabilizing Bracers | 4/2 | 9 | 450 | 87 | - |
| 89 | 87517 | Bracers of Rolling Fields | 4/2 | 9 | 450 | 85 | - |
| 89 | 87443 | Syrio's Gloves of Tutelage | 4/2 | 10 | 442 | 84 | - |
| 89 | 82260 | Vine Gloves | 4/2 | 10 | 429 | 88 | - |
| 89 | 78660 | Deep Earth Robes | 4/2 | 20 | 410 | 85 | - |
| 89 | 78662 | Deep Earth Vestment | 4/2 | 20 | 410 | 85 | - |
| 89 | 80925 | Swarmcall Helm | 4/3 | 1 | 450 | 87 | - |
| 89 | 85175 | Crest of the Clan Lords | 4/3 | 1 | 450 | 88 | - |
| 89 | 87489 | Fletcher's Sharpening Shoulders | 4/3 | 3 | 450 | 85 | - |
| 89 | 80916 | Shadowspine Shoulderguards | 4/3 | 3 | 435 | 87 | - |
| 89 | 100968 | Galedodger Chestguard | 4/3 | 5 | 450 | 89 | - |
| 89 | 87630 | Chestpiece of Twinkling Stars | 4/3 | 5 | 440 | 85 | - |
| 89 | 80923 | Impaler's Girdle | 4/3 | 6 | 450 | 87 | - |
| 89 | 100974 | Belt of Totemic Binding | 4/3 | 6 | 450 | 89 | - |
| 89 | 87459 | Dills' Primal Leggings | 4/3 | 7 | 442 | 84 | - |
| 89 | 90723 | Arness's Scaled Leggings | 4/3 | 7 | 440 | 87 | - |
| 89 | 80928 | Treads of Fixation | 4/3 | 8 | 450 | 87 | - |
| 89 | 100975 | Airbender Sandals | 4/3 | 8 | 450 | 89 | - |
| 89 | 85189 | Groundshaker Bracers | 4/3 | 9 | 450 | 88 | - |
| 89 | 87614 | Kafa Picker's Bracers | 4/3 | 9 | 430 | 85 | - |
| 89 | 85184 | Hexxer's Lethargic Gloves | 4/3 | 10 | 450 | 88 | - |
| 89 | 100958 | Archer's Precision Grips | 4/3 | 10 | 450 | 89 | - |
| 89 | 78723 | Spiritwalker's Hauberk | 4/3 | 20 | 410 | 85 | - |
| 89 | 78724 | Spiritwalker's Cuirass | 4/3 | 20 | 410 | 85 | - |
| 89 | 85179 | Meteoric Greathelm | 4/4 | 1 | 450 | 88 | - |
| 89 | 100959 | Sightfinder Helm | 4/4 | 1 | 450 | 89 | - |
| 89 | 80924 | Acid-Scarred Spaulders | 4/4 | 3 | 450 | 87 | - |
| 89 | 100970 | Spaulders of Immovable Stone | 4/4 | 3 | 450 | 89 | - |
| 89 | 80930 | Swarmbringer Chestguard | 4/4 | 5 | 450 | 87 | - |
| 89 | 85193 | Mind's Eye Breastplate | 4/4 | 5 | 450 | 88 | - |
| 89 | 87501 | Unbreakable Waistplate | 4/4 | 6 | 450 | 85 | - |
| 89 | 100953 | Girdle of Soothing Detonation | 4/4 | 6 | 450 | 89 | - |
| 89 | 100954 | Sap-Encrusted Legplates | 4/4 | 7 | 450 | 89 | - |
| 89 | 90717 | Qu'nas' Apocryphal Legplates | 4/4 | 7 | 440 | 87 | - |
| 89 | 85180 | Glintrok Sollerets | 4/4 | 8 | 450 | 88 | - |
| 89 | 100969 | Anchoring Sabatons | 4/4 | 8 | 450 | 89 | - |
| 89 | 100964 | Siegeworn Bracers | 4/4 | 9 | 450 | 89 | - |
| 89 | 87518 | Bracers of the Neux Dawn | 4/4 | 9 | 433 | 83 | - |
| 89 | 80927 | Hive Protector's Gauntlets | 4/4 | 10 | 450 | 87 | - |
| 89 | 85186 | Axebreaker Gauntlets | 4/4 | 10 | 450 | 88 | - |
| 89 | 71439 | Clutch of the Firemother | 4/4 | 20 | 391 | 85 | - |
| 89 | 71512 | Immolation Battleplate | 4/4 | 20 | 391 | 85 | - |
| 89 | 80931 | Shield of the Protectorate | 4/6 | 14 | 450 | 87 | - |
| 89 | 87487 | Turpster's Sonorous Shield | 4/6 | 14 | 450 | 85 | - |

## Reproduce the database shortlist

Run this read-only MySQL 8 query against the imported world database.

```sql
WITH rewards AS (SELECT RewardItem1 item, MinLevel lvl FROM quest_template WHERE RewardItem1>0 UNION ALL SELECT RewardItem2 item, MinLevel lvl FROM quest_template WHERE RewardItem2>0 UNION ALL SELECT RewardItem3 item, MinLevel lvl FROM quest_template WHERE RewardItem3>0 UNION ALL SELECT RewardItem4 item, MinLevel lvl FROM quest_template WHERE RewardItem4>0 UNION ALL SELECT RewardChoiceItemID1 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID1>0 UNION ALL SELECT RewardChoiceItemID2 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID2>0 UNION ALL SELECT RewardChoiceItemID3 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID3>0 UNION ALL SELECT RewardChoiceItemID4 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID4>0 UNION ALL SELECT RewardChoiceItemID5 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID5>0 UNION ALL SELECT RewardChoiceItemID6 item, MinLevel lvl FROM quest_template WHERE RewardChoiceItemID6>0), q AS (SELECT item,MIN(lvl) lvl FROM rewards GROUP BY item), sources AS (SELECT item FROM creature_loot_template WHERE item>0 UNION SELECT item FROM gameobject_loot_template WHERE item>0 UNION SELECT item FROM item_loot_template WHERE item>0 UNION SELECT item FROM reference_loot_template WHERE item>0 UNION SELECT item FROM fishing_loot_template WHERE item>0 UNION SELECT item FROM npc_vendor WHERE item>0), levels AS (SELECT 19 lvl UNION ALL SELECT 29 lvl UNION ALL SELECT 39 lvl UNION ALL SELECT 49 lvl UNION ALL SELECT 59 lvl UNION ALL SELECT 60 lvl UNION ALL SELECT 69 lvl UNION ALL SELECT 70 lvl UNION ALL SELECT 78 lvl UNION ALL SELECT 79 lvl UNION ALL SELECT 80 lvl UNION ALL SELECT 84 lvl UNION ALL SELECT 85 lvl UNION ALL SELECT 89 lvl), ranked AS (
SELECT l.lvl bracket,i.entry,i.name,i.Class,i.SubClass,i.InventoryType,i.ItemLevel,i.RequiredLevel, q.lvl quest_min,
ROW_NUMBER() OVER (PARTITION BY l.lvl,i.Class,i.SubClass,i.InventoryType ORDER BY i.ItemLevel DESC,i.Quality DESC,i.entry) rnk
FROM item_template i CROSS JOIN levels l LEFT JOIN q ON q.item=i.entry
WHERE i.Quality BETWEEN 2 AND 5 AND i.InventoryType IN (1,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,20,21,22,23,25,26)
AND i.Class IN (2,4) AND i.ScalingStatDistribution=0 AND i.RequiredLevel<=l.lvl
AND (EXISTS(SELECT 1 FROM sources s WHERE s.item=i.entry) OR q.lvl<=l.lvl)
AND (i.RequiredLevel>0 OR q.lvl<=l.lvl)
AND i.name NOT REGEXP '(^QA|Deprecated|Test |PH]|OLD|UNUSED)' )
SELECT bracket,entry,name,Class,SubClass,InventoryType,ItemLevel,RequiredLevel,COALESCE(quest_min,'-') quest_min FROM ranked WHERE rnk<=2 ORDER BY bracket,Class,SubClass,InventoryType,rnk;
```

## Verification commands

Run the migration twice against world_codex and compare ordered item_template rows
for the migration's item IDs and ordered hotfix_data rows. Repetition must not change
either snapshot. Restart the Codex worldserver and verify 315 hotfix notices load
(one pre-existing plus 314 additions). Use the headless client on auth 3744/world
8105, then request Item-sparse entry 1482: Shadowfang must report item level 38,
stat slot 2 type 32 (crit) and value 4. Visual tooltip rendering still needs a
human client check; the headless client can verify the server's payload only.

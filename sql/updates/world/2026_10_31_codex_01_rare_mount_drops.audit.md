# T-012 rare mount loot audit

Snapshot: imported v1.0.4 world_codex, 2026-10-31. Mount items are item_template
class 15/subclass 5, joined to the live loot stores; currencies and reference
placeholder rows are not mistaken for direct items. All 12 standard item-bearing
loot stores were scanned: creature, disenchant, fishing, gameobject, item, mail,
milling, pickpocketing, prospecting, reference, skinning and spell. This finds
199 mount leaf rows / 91 distinct items. The differently shaped world-drop table
has no mount rows; personal/bonus tables describe dispatch, not direct items.

The migration changes 159 rows (direct leaves plus eight reference gates).
Every changed row is scoped by table, entry, item, loot mode and reference ID.
Counts, modes, item IDs and conditions are retained. Guaranteed and already-higher
chances stay. Temporary broom items classified as mounts are included in the
catalogue and use the same baseline policy.

## Group and reference policy

Rare direct mounts become independent 10% rolls (group 0), avoiding starvation
behind mixed equipment groups and preventing new explicit group totals >100%.
A container can now award multiple distinct mounts; its normal equipment group
still rolls. Mount-only reference pools remain mutually exclusive:

- Reference 21321: Red Qiraji Crystal 2% -> 10%; other crystal colors stay 15%.
- Reference 62901: ten mount leaves 9.4% -> 10% each; the guaranteed source now
  selects one of ten mounts at 10% apiece.
- Reference 44707: seven equal mounts stay 1/7; source gates 1%/2% -> 70%, making
  every mount's baseline source probability 10%.
- Reference 150011: six equal mounts stay 1/6; source gates .7%/1.5% -> 60%, making
  every mount's baseline source probability 10%.
- Reference 94229: Direhorn leaves stay 33/33/34%; four source gates -> 30.3031%.
  This yields at least 10% per color (third color about 10.303%).
- Reference 94292: guaranteed egg with three equal raptors stays unchanged.

All incoming mount-only reference edges were traced across the scanned stores;
these edges use one reference roll and group selector 0, with no nested incoming
reference edge. The listed formulas therefore cover their actual baseline path.
Grouped equal-chance rows use (100 - explicit total)/equal-row count, matching
the core's residual-roll model before player-specific filtering.

These are baseline probabilities at stock drop/reference-rate multipliers, before
eligibility, conditions and loot-size/duplicate limits. Bonus-roll eligibility,
missing loot sources, encounter scripts, vendors, achievements and quest rewards
are not created or rewritten. In this dump Sha of Anger/Galleon have no direct
mount leaf rows; Oondasta has lootid 0. Their missing stock acquisition paths
remain separate content gaps, not asserted fixed by chance updates.

## Validation

Applied twice by hand to world_codex. Ordered snapshots of every affected loot
entry were identical after both applications: SHA256
10b7f21199bc525158a79beba69d9e889241ac9c92250233185f69ac38ecf456.
All unrelated rows and non-chance/non-group fields in those entries matched
before/after. Reference probability floors were checked algebraically. Live
SOAP `reload all loot` succeeded. No custom tables or core changes; no build.
A random drop observation alone is not a statistical proof of a drop rate.

## Full mount-leaf catalogue

Old/new below are leaf probabilities; reference gate corrections are listed above.
Zero-coded grouped rows show their derived share. Mode is the integer loot mask.

| Store | Loot entry | Mode | Item | Name | Old leaf % | New leaf % |
|---|---|---|---|---|---|---|
| creature_loot_template | 10184 | 8 | 49636 | Reins of the Onyxian Drake | 1 | 10 |
| creature_loot_template | 10184 | 16 | 49636 | Reins of the Onyxian Drake | 1 | 10 |
| creature_loot_template | 10440 | 0 | 13335 | Deathcharger's Reins | 0.9984 | 10 |
| creature_loot_template | 11382 | 0 | 19872 | Swift Razzashi Raptor | 0.9144 | 10 |
| creature_loot_template | 14509 | 0 | 19902 | Swift Zulian Tiger | 0.6064 | 10 |
| creature_loot_template | 15550 | 8 | 30480 | Fiery Warhorse's Reins | 2 | 10 |
| creature_loot_template | 16152 | 0 | 30480 | Fiery Warhorse's Reins | 1 | 10 |
| creature_loot_template | 19622 | 0 | 32458 | Ashes of Al'ar | 2 | 10 |
| creature_loot_template | 23035 | 0 | 32768 | Reins of the Raven Lord | 2 | 10 |
| creature_loot_template | 24664 | 4 | 35513 | Swift White Hawkstrider | 3 | 10 |
| creature_loot_template | 26693 | 2 | 44151 | Reins of the Blue Proto-Drake | 0.9467 | 10 |
| creature_loot_template | 26693 | 4 | 44151 | Reins of the Blue Proto-Drake | 0.8 | 10 |
| creature_loot_template | 28860 | 16 | 43954 | Reins of the Twilight Drake | 100 | 100 |
| creature_loot_template | 28860 | 8 | 43986 | Reins of the Black Drake | 100 | 100 |
| creature_loot_template | 31125 | 16 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 31125 | 16 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 32273 | 0 | 43951 | Reins of the Bronze Drake | 100 | 100 |
| creature_loot_template | 32491 | 0 | 44168 | Reins of the Time-Lost Proto-Drake | 100 | 100 |
| creature_loot_template | 33288 | 16 | 45693 | Mimiron's Head | 3 | 10 |
| creature_loot_template | 33993 | 16 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 33993 | 16 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 35013 | 8 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 35013 | 16 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 35013 | 8 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 35013 | 16 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 36597 | 64 | 50818 | Invincible's Reins | 10 | 10 |
| creature_loot_template | 38433 | 8 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 38433 | 16 | 43959 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 38433 | 8 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 38433 | 16 | 44083 | Reins of the Grand Black War Mammoth | 2 | 10 |
| creature_loot_template | 43214 | 0 | 63043 | Reins of the Vitreous Stone Drake | 0.7 | 10 |
| creature_loot_template | 43873 | 0 | 63040 | Reins of the Drake of the North Wind | 0.7 | 10 |
| creature_loot_template | 45412 | 0 | 13335 | Deathcharger's Reins | 0.7879 | 10 |
| creature_loot_template | 46753 | 8 | 63041 | Reins of the Drake of the South Wind | 1 | 10 |
| creature_loot_template | 50005 | 0 | 67151 | Reins of Poseidus | 100 | 100 |
| creature_loot_template | 50062 | 0 | 63042 | Reins of the Phosphorescent Stone Drake | 100 | 100 |
| creature_loot_template | 50245 | 0 | 63046 | Reins of the Grey Riding Camel | 100 | 100 |
| creature_loot_template | 52059 | 4 | 68824 | Swift Zulian Panther | 2 | 10 |
| creature_loot_template | 52151 | 0 | 68823 | Armored Razzashi Raptor | 1 | 10 |
| creature_loot_template | 52409 | 1 | 69224 | Smoldering Egg of Millagazor | 2 | 10 |
| creature_loot_template | 52409 | 8 | 69224 | Smoldering Egg of Millagazor | 1 | 10 |
| creature_loot_template | 52530 | 1 | 71665 | Flametalon of Alysrazor | 2 | 10 |
| creature_loot_template | 52530 | 8 | 71665 | Flametalon of Alysrazor | 2 | 10 |
| creature_loot_template | 52530 | 16 | 71665 | Flametalon of Alysrazor | 2 | 10 |
| creature_loot_template | 64403 | 0 | 90655 | Reins of the Thundering Ruby Cloud Serpent | 100 | 100 |
| creature_loot_template | 68476 | 8 | 93666 | Spawn of Horridon | 2.5 | 10 |
| creature_loot_template | 68476 | 16 | 93666 | Spawn of Horridon | 2.5 | 10 |
| creature_loot_template | 68476 | 32 | 93666 | Spawn of Horridon | 5 | 10 |
| creature_loot_template | 68476 | 64 | 93666 | Spawn of Horridon | 5 | 10 |
| creature_loot_template | 69099 | 0 | 95057 | Reins of the Thundering Cobalt Cloud Serpent | 15 | 15 |
| creature_loot_template | 69712 | 8 | 95059 | Clutch of Ji-Kun | 2.5 | 10 |
| creature_loot_template | 69712 | 16 | 95059 | Clutch of Ji-Kun | 2.5 | 10 |
| creature_loot_template | 69712 | 32 | 95059 | Clutch of Ji-Kun | 5 | 10 |
| creature_loot_template | 69712 | 64 | 95059 | Clutch of Ji-Kun | 5 | 10 |
| creature_loot_template | 71865 | 32 | 104253 | Kor'kron Juggernaut | 100 | 100 |
| creature_loot_template | 71865 | 64 | 104253 | Kor'kron Juggernaut | 100 | 100 |
| creature_loot_template | 73167 | 0 | 104269 | Reins of the Thundering Onyx Cloud Serpent | 1 | 10 |
| fishing_loot_template | 5805 | 0 | 46109 | Sea Turtle | 0.01 | 10 |
| fishing_loot_template | 5840 | 0 | 46109 | Sea Turtle | 0.01 | 10 |
| fishing_loot_template | 6661 | 0 | 46109 | Sea Turtle | 0.01 | 10 |
| fishing_loot_template | 6757 | 0 | 46109 | Sea Turtle | 0.01 | 10 |
| gameobject_loot_template | 42519 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42520 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42522 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42523 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42525 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42526 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 42528 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 43504 | 0 | 87777 | Reins of the Astral Cloud Serpent | 5 | 10 |
| gameobject_loot_template | 44053 | 0 | 87777 | Reins of the Astral Cloud Serpent | 10 | 10 |
| gameobject_loot_template | 186672 | 0 | 69747 | Amani Battle Bear | 100 | 100 |
| gameobject_loot_template | 192046 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192048 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192049 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192050 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192051 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192052 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192053 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192054 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192057 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 192059 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 193905 | 0 | 43953 | Reins of the Blue Drake | 1 | 10 |
| gameobject_loot_template | 193967 | 0 | 43952 | Reins of the Azure Drake | 1 | 10 |
| gameobject_loot_template | 194479 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202776 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202777 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202778 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202779 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202780 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 202781 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 207724 | 0 | 46109 | Sea Turtle | 0.1 | 10 |
| gameobject_loot_template | 207891 | 0 | 63041 | Reins of the Drake of the South Wind | 1 | 10 |
| gameobject_loot_template | 207892 | 0 | 63041 | Reins of the Drake of the South Wind | 1 | 10 |
| gameobject_loot_template | 207893 | 0 | 63041 | Reins of the Drake of the South Wind | 1 | 10 |
| gameobject_loot_template | 207894 | 0 | 63041 | Reins of the Drake of the South Wind | 1 | 10 |
| gameobject_loot_template | 208967 | 0 | 69224 | Smoldering Egg of Millagazor | 1 | 10 |
| gameobject_loot_template | 209261 | 0 | 69224 | Smoldering Egg of Millagazor | 1 | 10 |
| gameobject_loot_template | 210216 | 0 | 46109 | Sea Turtle | 0.01 | 10 |
| gameobject_loot_template | 210217 | 0 | 77067 | Reins of the Blazing Drake | 5 | 10 |
| gameobject_loot_template | 210218 | 0 | 77067 | Reins of the Blazing Drake | 15 | 15 |
| gameobject_loot_template | 210219 | 0 | 77067 | Reins of the Blazing Drake | 45 | 45 |
| gameobject_loot_template | 210220 | 0 | 77067 | Reins of the Blazing Drake | 55 | 55 |
| item_loot_template | 37586 | 0 | 37011 | Magic Broom | 0.1 | 10 |
| item_loot_template | 39883 | 0 | 44707 | Reins of the Green Proto-Drake | 2 | 10 |
| item_loot_template | 44751 | 0 | 43962 | Reins of the White Polar Bear | 1 | 10 |
| item_loot_template | 45072 | 0 | 72145 | Swift Springstrider | 0.0003 | 10 |
| item_loot_template | 52676 | 0 | 43953 | Reins of the Blue Drake | 5 | 10 |
| item_loot_template | 54516 | 0 | 33176 | Flying Broom | 5 | 10 |
| item_loot_template | 54516 | 0 | 37011 | Magic Broom | 10 | 10 |
| item_loot_template | 54516 | 0 | 37012 | The Horseman's Reins | 1 | 10 |
| item_loot_template | 54535 | 0 | 33977 | Swift Brewfest Ram | 5 | 10 |
| item_loot_template | 54535 | 0 | 37828 | Great Brewfest Kodo | 5 | 10 |
| item_loot_template | 54537 | 0 | 50250 | Big Love Rocket | 1 | 10 |
| item_loot_template | 69903 | 0 | 13335 | Deathcharger's Reins | 0.0975 | 10 |
| item_loot_template | 69903 | 0 | 32768 | Reins of the Raven Lord | 0.0878 | 10 |
| item_loot_template | 69903 | 0 | 35513 | Swift White Hawkstrider | 0.2184 | 10 |
| item_loot_template | 69903 | 0 | 43953 | Reins of the Blue Drake | 0.3413 | 10 |
| item_loot_template | 69903 | 0 | 43962 | Reins of the White Polar Bear | 0.3452 | 10 |
| item_loot_template | 69903 | 0 | 44151 | Reins of the Blue Proto-Drake | 0.2184 | 10 |
| item_loot_template | 69903 | 0 | 44707 | Reins of the Green Proto-Drake | 0.3296 | 10 |
| item_loot_template | 77956 | 0 | 76889 | Spectral Gryphon | 100 | 100 |
| item_loot_template | 77956 | 0 | 76902 | Spectral Wind Rider | 100 | 100 |
| item_loot_template | 86623 | 0 | 41508 | Mechano-Hog | 0.01 | 10 |
| item_loot_template | 86623 | 0 | 44413 | Mekgineer's Chopper | 0.01 | 10 |
| item_loot_template | 86623 | 0 | 87250 | Depleted-Kyparium Rocket | 0.01 | 10 |
| item_loot_template | 86623 | 0 | 87251 | Geosynchronous World Spinner | 0.01 | 10 |
| item_loot_template | 89613 | 1 | 13335 | Deathcharger's Reins | 1 | 10 |
| item_loot_template | 92813 | 1 | 13335 | Deathcharger's Reins | 1 | 10 |
| item_loot_template | 97565 | 0 | 13335 | Deathcharger's Reins | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 19872 | Swift Razzashi Raptor | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 19902 | Swift Zulian Tiger | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 23720 | Riding Turtle | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 32458 | Ashes of Al'ar | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 32768 | Reins of the Raven Lord | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 35513 | Swift White Hawkstrider | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 43962 | Reins of the White Polar Bear | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 44151 | Reins of the Blue Proto-Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 44175 | Reins of the Plagued Proto-Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 44707 | Reins of the Green Proto-Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 45693 | Mimiron's Head | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 45802 | Reins of the Rusted Proto-Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 49286 | X-51 Nether-Rocket X-TREME | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 49636 | Reins of the Onyxian Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 50818 | Invincible's Reins | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 54068 | Wooly White Rhino | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 63040 | Reins of the Drake of the North Wind | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 63041 | Reins of the Drake of the South Wind | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 63043 | Reins of the Vitreous Stone Drake | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 69224 | Smoldering Egg of Millagazor | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 71665 | Flametalon of Alysrazor | 0.154799 | 10 |
| item_loot_template | 97565 | 0 | 77067 | Reins of the Blazing Drake | 0.154799 | 10 |
| item_loot_template | 102137 | 0 | 23720 | Riding Turtle | 4 | 10 |
| item_loot_template | 102137 | 0 | 63043 | Reins of the Vitreous Stone Drake | 4 | 10 |
| mail_loot_template | 211 | 0 | 37676 | Vengeful Nether Drake | 100 | 100 |
| mail_loot_template | 262 | 0 | 43516 | Brutal Nether Drake | 100 | 100 |
| mail_loot_template | 266 | 0 | 46708 | Deadly Gladiator's Frost Wyrm | 100 | 100 |
| mail_loot_template | 267 | 0 | 46171 | Furious Gladiator's Frost Wyrm | 100 | 100 |
| mail_loot_template | 286 | 0 | 47840 | Relentless Gladiator's Frost Wyrm | 100 | 100 |
| mail_loot_template | 287 | 0 | 50435 | Wrathful Gladiator's Frost Wyrm | 100 | 100 |
| reference_loot_template | 21321 | 0 | 21218 | Blue Qiraji Resonating Crystal | 15 | 15 |
| reference_loot_template | 21321 | 0 | 21321 | Red Qiraji Resonating Crystal | 2 | 10 |
| reference_loot_template | 21321 | 0 | 21323 | Green Qiraji Resonating Crystal | 15 | 15 |
| reference_loot_template | 21321 | 0 | 21324 | Yellow Qiraji Resonating Crystal | 15 | 15 |
| reference_loot_template | 44707 | 0 | 13335 | Deathcharger's Reins | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 32768 | Reins of the Raven Lord | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 35513 | Swift White Hawkstrider | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 43953 | Reins of the Blue Drake | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 43962 | Reins of the White Polar Bear | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 44151 | Reins of the Blue Proto-Drake | 14.2857 | 14.2857 |
| reference_loot_template | 44707 | 0 | 44707 | Reins of the Green Proto-Drake | 14.2857 | 14.2857 |
| reference_loot_template | 62901 | 0 | 35225 | X-51 Nether-Rocket | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 60954 | Fossilized Raptor | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 62901 | Reins of the Drake of the East Wind | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 63125 | Reins of the Dark Phoenix | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 68823 | Armored Razzashi Raptor | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 68824 | Swift Zulian Panther | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 72140 | Swift Forest Strider | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 85870 | Imperial Quilen | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 89783 | Son of Galleon's Saddle | 9.4 | 10 |
| reference_loot_template | 62901 | 0 | 98618 | Hearthsteed | 9.4 | 10 |
| reference_loot_template | 94229 | 0 | 94229 | Reins of the Slate Primordial Direhorn | 33 | 33 |
| reference_loot_template | 94229 | 0 | 94230 | Reins of the Amber Primordial Direhorn | 33 | 33 |
| reference_loot_template | 94229 | 0 | 94231 | Reins of the Jade Primordial Direhorn | 34 | 34 |
| reference_loot_template | 94292 | 0 | 94291 | Reins of the Red Primal Raptor | 33.3333 | 33.3333 |
| reference_loot_template | 94292 | 0 | 94292 | Reins of the Black Primal Raptor | 33.3333 | 33.3333 |
| reference_loot_template | 94292 | 0 | 94293 | Reins of the Green Primal Raptor | 33.3333 | 33.3333 |
| reference_loot_template | 150011 | 1 | 62900 | Reins of the Volcanic Stone Drake | 16.6667 | 16.6667 |
| reference_loot_template | 150011 | 1 | 63041 | Reins of the Drake of the South Wind | 16.6667 | 16.6667 |
| reference_loot_template | 150011 | 1 | 71665 | Flametalon of Alysrazor | 16.6667 | 16.6667 |
| reference_loot_template | 150011 | 1 | 79771 | Feldrake | 16.6667 | 16.6667 |
| reference_loot_template | 150011 | 1 | 89783 | Son of Galleon's Saddle | 16.6667 | 16.6667 |
| reference_loot_template | 150011 | 1 | 97989 | Enchanted Fey Dragon | 16.6667 | 16.6667 |
| spell_loot_template | 85325 | 0 | 23720 | Riding Turtle | 0.5 | 10 |
| spell_loot_template | 85325 | 0 | 49282 | Big Battle Bear | 0.5 | 10 |
| spell_loot_template | 85325 | 0 | 49283 | Reins of the Spectral Tiger | 1 | 10 |
| spell_loot_template | 85325 | 0 | 49284 | Reins of the Swift Spectral Tiger | 0.5 | 10 |
| spell_loot_template | 85325 | 0 | 49285 | X-51 Nether-Rocket | 1 | 10 |
| spell_loot_template | 85325 | 0 | 49286 | X-51 Nether-Rocket X-TREME | 0.5 | 10 |
| spell_loot_template | 129675 | 0 | 38265 | [PH] Reins of the Black Warp Stalker | 99 | 99 |

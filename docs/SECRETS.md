# SPOILERS: the sealed secrets log

**Don't read this unless you want the answers.** Agents record every planted secret here (DESIGN.md, Sealed
secrets), so nothing is ever unrecoverable. The in-game hint ladder will read from this log once it exists; until
then this file is the only way to look something up.

Rules for agents:

- Every treasure, hidden quest, emote trigger, secret boss or hidden vendor gets an entry before its PR merges.
- Keep solutions out of commit messages, PR descriptions, status fragments, `STATUS.md` and `TASKS.md`. Write "N
  treasures in <zone>, all pass, see SECRETS.md" instead.
- Each entry has a four-step hint ladder (vague, zone, location, solution) and says how it is validated
  automatically, so nobody has to look at it to know it works.

## Index

| Id | Kind | Zone | Added |
|---|---|---|---|
| T-DUR-01 to T-DUR-12 | Treasures | Durotar | T-093 |
| T-DUR-13 to T-DUR-47 | Treasures | Durotar | T-097 |

## Treasures: Durotar (T-093)

Validated by `.treasure check all` (all 12 pass) and the `treasure-1-loot` / `treasure-2-other` scenarios. Spawn
guids 9090001-9090012, zone file `tools/treasures/zones/durotar.json`. `.treasure go <guid>` takes a GM there.

| Id | Vague hint | Zone hint | Location | Solution |
|---|---|---|---|---|
| T-DUR-01 | "The Burning Blade kept a coffer where the trials begin." | Valley of Trials | Burning Blade Coven cave | Burning Blade Coffer at -132.6 -4315.2 66.1, inside the cave |
| T-DUR-02 | "Deep under the skull, the cult hid its strongbox." | Skull Rock | Lower tunnels | Burning Blade Strongbox at 1489.1 -4821.8 8.6 |
| T-DUR-03 | "The felweaver left his satchel behind." | Skull Rock | Felweaver Scornn's hall | Felweaver's Satchel at 1468.1 -4692.9 6.5 |
| T-DUR-04 | "Cultists stash things where the dust blows in." | Dustwind Cave | Inside the cave | Cultist's Cache at 915.2 -4748.2 20.1 |
| T-DUR-05 | "Harpies hoard what they steal, high and out of the sun." | Drygulch Ravine | Under the harpy overhang | Harpy's Hoard at 993.1 -4680.1 46.5 |
| T-DUR-06 | "Pillagers hide loot in the windy canyon." | Razorwind Canyon | Under the canyon wall | Pillager's Stash at 722.2 -4448.6 15.7 |
| T-DUR-07 | "The Razormane keep a war chest near their grounds." | Razormane Grounds | Under the rock shelter | Razormane War Cache at 66.3 -4567.1 57.9 |
| T-DUR-08 | "A Kul Tiran sailor never cleared out his footlocker." | Tiragarde Keep | Inside the keep | Kul Tiran Footlocker at -244.8 -5088.5 23.7 |
| T-DUR-09 | "The sea gave back a chest from a wreck." | Scuttle Coast | By the shipwreck | Waterlogged Sea Chest at 226.7 -5141.2 2.3 |
| T-DUR-10 | "The Darkspear left an offering on their old islands." | Echo Isles | Under cover on the isles | Darkspear Offering at -1127.8 -5134.2 4.2 |
| T-DUR-11 | "Northwatch soldiers forgot their supplies." | Northwatch Foothold | Under cover at the foothold | Northwatch Supply Cache at -973.0 -4480.0 25.7 |
| T-DUR-12 | "Someone hid a cache among the boars." | Jaggedswine Farm | Under cover at the farm | Swineherd's Cache at 1184.5 -4625.2 18.4 |

Loot: a curio, a green from the spot's theme in `tools/treasures/themes.json`, 8% (15% in Burning Blade caves) for a
Ragefire-era blue, and some copper. Each character can open each chest once.

## Treasures: Durotar, second pass (T-097)

Validated by `.treasure check all` (all 47 pass) and the treasure scenarios. Spawn guids 9090013-9090047, same zone
file. Not every treasure is a chest: sacks, jugs, bones, clams, books and crates sit in camps and towns. Those marked
"no sparkle" are hidden in plain sight and never show the sparkle, even with the Heartseed.

| Id | Vague hint | Zone hint | Location | Solution |
|---|---|---|---|---|
| T-DUR-13 | "New recruits never check the top of the stores." | Valley of Trials | the Den | Den Supply Crate at -603.8 -4193.9 41.1, guid 9090013 |
| T-DUR-14 | "A farmer hid seed where the boars dig." | Valley of Trials | the farm plots | Farmer's Seed Sack at -498.7 -4277.6 44.2, guid 9090014 |
| T-DUR-15 | "Training gear piles up past the Den." | Valley of Trials | north of the Den | Training Weapon Crate at -586.8 -4119.5 43.8, guid 9090015 |
| T-DUR-16 | "A peon napped with his jug and never came back for it." | Valley of Trials | the lumber ridge | Lazy Peon's Jug at -371.8 -4012.9 50.8, guid 9090016, no sparkle |
| T-DUR-17 | "A boar captain buries his take by the sea." | Northern Durotar | Captain Flat Tusk's beach | Flat Tusk's Stash at 23.3 -3998.6 20.1, guid 9090017 |
| T-DUR-18 | "The Alliance road loses crates." | Northwatch caravan road | west | Northwatch Supply Crate at -1039.9 -4563.9 46.0, guid 9090018 |
| T-DUR-19 | "Someone dropped a weapon crate on the run." | Northwatch caravan road | east | Abandoned Weapon Crate at -926.9 -4691.1 25.4, guid 9090019 |
| T-DUR-20 | "The river washes barrels ashore." | Southfury River | northern bank | Waterlogged Barrel at 415.9 -3928.3 22.3, guid 9090020 |
| T-DUR-21 | "A geolord stole more than he could carry." | Southfury Watershed | Geolord Mottle's camp | Geolord's Stolen Sack at 456.0 -4267.5 22.9, guid 9090021 |
| T-DUR-22 | "Raptors hoard shiny things too." | Bloodtalon raptor grounds | Bloodtalon raptor grounds | Scythemaw Nest at 1070.2 -4589.9 26.0, guid 9090022 |
| T-DUR-23 | "Not every harpy victim was found." | Drygulch Ravine | eastern ledge | Harpy Victim's Remains at 1254.8 -4673.0 16.1, guid 9090023 |
| T-DUR-24 | "The storm witches keep their feathers close." | Drygulch Ravine | the storm witches' perch | Storm Witch's Satchel at 1101.6 -4716.3 32.6, guid 9090024, drops Windrider's Plume |
| T-DUR-25 | "The Shadow Council's relics sleep deepest in the dust." | Dustwind Cave | the deep chamber | Shadow Council Reliquary at 807.2 -4752.6 37.6, guid 9090025, drops Felweaver's Fang (epic) |
| T-DUR-26 | "The Blade camps where the wind cuts." | Razorwind Canyon floor | Burning Blade camp | Burning Blade Cache at 672.6 -4725.7 -9.8, guid 9090026, drops Burning Blade Brand |
| T-DUR-27 | "Old bones in the scorpid flats." | South of Razor Hill | scorpid flats | Bleached Kodo Bones at 85.4 -4967.1 14.3, guid 9090027 |
| T-DUR-28 | "The Kor'kron leave supplies right under your nose." | Below Orgrimmar's gate | Below Orgrimmar's gate | Kor'kron Supply Crate at 1461.1 -4417.1 25.4, guid 9090028, no sparkle |
| T-DUR-29 | "Another Kor'kron crate, in plain sight." | East of Orgrimmar | Kor'kron post | Kor'kron Weapon Crate at 1677.7 -4440.0 19.7, guid 9090029, no sparkle |
| T-DUR-30 | "Skulls mark where the Darkspear trained." | Echo Isles | Darkspear training grounds | Voodoo Skull Pile at -1172.2 -5485.1 12.0, guid 9090030 |
| T-DUR-31 | "The mad witch doctor kept a cache." | Echo Isles | Zalazane's old haunt | Zalazane's Cache at -1275.8 -5363.4 14.9, guid 9090031, drops Zalazane's Hex Doll |
| T-DUR-32 | "Something big sits shut on the bay shore." | Bladefist Bay shore | Bladefist Bay shore | Monstrous Clam at 1176.4 -5105.7 4.4, guid 9090032, drops Tidewalker's Charm |
| T-DUR-33 | "A jug the innkeeper forgot." | Razor Hill | the inn | Forgotten Jug at 341.2 -4669.7 16.5, guid 9090033, no sparkle |
| T-DUR-34 | "The barracks have one crate too many." | Razor Hill Barracks | Razor Hill Barracks | Barracks Weapon Crate at 313.5 -4829.9 9.6, guid 9090034, no sparkle |
| T-DUR-35 | "Look up from Razor Hill." | Razor Hill Watchtower | the top | Lookout's Stash at 395.2 -4584.6 76.6, guid 9090035 |
| T-DUR-36 | "That haystack looks suspicious." | Razor Hill outskirts | Razor Hill outskirts | Suspicious Haystack at 398.7 -4738.9 9.7, guid 9090036, no sparkle |
| T-DUR-37 | "A witch doctor left a book open." | Sen'jin Village | a witch doctor's hut | Witch Doctor's Tome at -779.9 -4947.8 22.2, guid 9090037, no sparkle |
| T-DUR-38 | "Darkspear stores sit out of the sun." | Sen'jin Village | under the shade | Darkspear Supply Basket at -839.0 -4889.9 23.5, guid 9090038 |
| T-DUR-39 | "Powder kegs at the blockade." | Dranosh'ar Blockade | west | Dranosh'ar Powder Barrel at 1395.9 -4912.5 11.3, guid 9090039 |
| T-DUR-40 | "A laborer left his tools." | Dranosh'ar Blockade | north | Laborer's Toolbox at 1417.6 -4650.1 35.1, guid 9090040 |
| T-DUR-41 | "Kul Tirans store powder in the yard." | Tiragarde Keep | the yard | Kul Tiran Powder Crate at -100.5 -5074.4 17.6, guid 9090041 |
| T-DUR-42 | "Another clam, where sailors drowned." | Scuttle Coast | the drowned sailors' beach | Monstrous Clam at -71.4 -5175.8 1.0, guid 9090042 |
| T-DUR-43 | "A family hid its savings on the farm." | Tor'kren Farm | Tor'kren Farm | Tor'kren Family Savings at 760.0 -4239.0 20.6, guid 9090043 |
| T-DUR-44 | "Quilboar loot from the north camp." | Razormane Grounds | north camp | Razormane Loot Sack at 33.5 -4264.0 41.0, guid 9090044, drops Thornhide Talisman |
| T-DUR-45 | "An adventurer never made it between two villages." | The hills between Razor Hill and Sen'jin | The hills between Razor Hill and Sen'jin | Lost Adventurer's Remains at -314.0 -4847.0 40.3, guid 9090045 |
| T-DUR-46 | "Something still smoulders high above the valley." | Shrine of the Dormant Flame | Shrine of the Dormant Flame | Offering of the Dormant Flame at -250.3 -3976.9 158.7, guid 9090046, drops Ember of the Dormant Flame (epic) |
| T-DUR-47 | "The spirits take offerings below their rock." | The Hidden Path below Spirit Rock | The Hidden Path below Spirit Rock | Spirit Offering at -819.0 -4281.5 65.5, guid 9090047 |

Items (each drops from exactly one spot, always):

| Item | Kind | Effect |
|---|---|---|
| Zalazane's Hex Doll | blue trinket, level 12 | Use: Hex an enemy into a frog for 8 sec, 2 min cooldown |
| Windrider's Plume | blue trinket, level 8 | Use: slow fall for 30 sec, 1 min cooldown |
| Tidewalker's Charm | blue trinket, level 8 | Use: water walking for 10 min, 2 min cooldown |
| Thornhide Talisman | blue trinket, level 10 | Use: thorns for 20 sec (5 Nature damage), 90 sec cooldown |
| Burning Blade Brand | blue trinket, level 10 | Use: instant 45 Shadow damage, 90 sec cooldown |
| Ember of the Dormant Flame | epic trinket, level 10 | +5 stamina, +3 crit; Use: 70 Fire damage nova, 2 min cooldown |
| Felweaver's Fang | epic dagger, level 12 | +4 agility, +3 stamina; chance on hit (2.5 per minute): 22 Shadow damage |

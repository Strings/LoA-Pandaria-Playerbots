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

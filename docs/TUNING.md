# Server & Module Tuning Reference

This document catalogs server tuning configuration keys from `src/server/worldserver/worldserver.conf.dist`
and `modules/mod_playerbots/config/playerbots.conf.dist` that playtests and developers are most likely to
adjust. Each setting lists its configuration file, shipped default, exact effect, and whether
executing `.reload config` in-game (or via SOAP/console) updates the active server without a restart.

---

## 1. Heartseed Artifact System

Config file: `worldserver.conf.dist`  
Code reference: `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp`, `heartseed_scripts.cpp`  
All Heartseed settings are reloaded on `.reload config` via `HeartseedMgr::LoadConfig()`.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `Heartseed.Enabled` | `1` | Master toggle enabling the Heartseed artifact system | Yes |
| `Heartseed.Core.WeaponDamagePct` | `5` | Percentage of weapon damage dealt by the passive core aura | Yes |
| `Heartseed.Proc.SpellId` | `16614` | Spell ID triggered when the weapon damage proc occurs | Yes |
| `Heartseed.Proc.Chance` | `10` | Percentage proc chance on weapon swing/ability | Yes |
| `Heartseed.Proc.WeaponDamagePct` | `50` | Percentage of weapon damage dealt on a proc | Yes |
| `Heartseed.Proc.Cooldown` | `3000` | Cooldown in milliseconds between weapon procs | Yes |
| `Heartseed.Rank.CapPerLevel` | `1` | Additional Heartseed rank cap granted per character level | Yes |
| `Heartseed.Rank.CapPerAwakening` | `5` | Additional Heartseed rank cap granted per completed awakening flag | Yes |
| `Heartseed.Rank.AwakeningFlags` | `"1 2 3"` | Space-separated story flag IDs that count as rank awakenings | Yes |
| `Heartseed.Rank.GrowthBase` | `100` | Base Growth points required to reach rank 1 | Yes |
| `Heartseed.Rank.GrowthPerRank` | `20` | Additional Growth points scaling per subsequent rank | Yes |
| `Heartseed.Feed.OnReplace` | `1` | Automatically feeds discarded weapon when equipping a higher item-level weapon | Yes |
| `Heartseed.Feed.GrowthBase` | `20` | Base Growth points awarded when feeding a weapon | Yes |
| `Heartseed.Feed.GrowthPerItemLevel` | `2` | Additional Growth points awarded per item level of fed weapon | Yes |
| `Heartseed.Feed.QualityPct` | `"50 100 150 250 400 0 0 0"` | Growth multiplier percentages by weapon item quality (Poor to Heirloom) | Yes |
| `Heartseed.Essence.ProcCooldown` | `3000` | Global internal cooldown in ms between essence ability procs | Yes |
| `Heartseed.Essence.AutoSlot` | `1` | Automatically slots newly collected essences into available open essence slots | Yes |
| `Heartseed.Essence.SlotLevels` | `"10 40 70"` | Character levels unlocking essence slots 1, 2, and 3 | Yes |
| `Heartseed.Majors.TierLevels` | `"10 25 40 55 70 85"` | Character levels unlocking the six major trait tiers | Yes |
| `Heartseed.Story.Hooks` | `1` | Enables automatic story flag progression on dungeon boss kills, Pandaria entry, and world boss kills | Yes |
| `Heartseed.Story.PandariaMaps` | `"870"` | Map IDs that grant the `reached_pandaria` story flag | Yes |
| `Heartseed.Story.WorldBosses` | `"60491 62346 69099 69161 71952 71953 71954 71955 72057 6109 12397 14887 14888 14889 14890 17711 18728"` | Creature IDs that grant the `first_world_boss` story flag | Yes |
| `Heartseed.Story.AwakeningSound` | `0` | Sound kit ID broadcast when a story beat raises the rank cap (0 = none) | Yes |
| `Heartseed.Grant.QuestId` | `90000` | Quest ID that grants the initial Heartseed | Yes |
| `Heartseed.Grant.OfferOnLogin` | `1` | Offers the granting quest upon login to seedless eligible characters | Yes |
| `Heartseed.Grant.OfferDelay` | `5000` | Delay in milliseconds after login before the quest prompt appears | Yes |
| `Heartseed.Choice.PromptOnLogin` | `1` | Automatically displays the major trait choice window at login if a tier is unchosen | Yes |
| `Heartseed.Choice.PromptDelay` | `6000` | Delay in milliseconds after login before the choice window displays | Yes |
| `Heartseed.Choice.ChangeAtShrineOnly` | `1` | Restricts changing chosen major traits to Celestial shrines (0 allows anywhere) | Yes |

---

## 2. Mount Travel Speed

Config file: `worldserver.conf.dist`  
Code reference: `src/server/game/Entities/Unit/MountedSpeed.cpp`, `src/server/game/World/World.cpp`  
Reloaded on `.reload config` via `MountedSpeed::Load()`.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `Rate.MountedSpeed` | `2.0` | Multiplier for player ground and flying mount speed bonuses (2.0 doubles bonuses, e.g. +100% becomes +200%). Active mounts refresh on remount or `.mountspeed`. | Yes |

---

## 3. Respec, Talents & Glyphs

Config file: `worldserver.conf.dist`  
Code reference: `src/server/game/World/RespecConfig.cpp`, `src/server/game/World/World.cpp`  
Reloaded on `.reload config` via `RespecConfig::Load()` and `World::LoadConfigSettings()`.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `NoResetTalentsCost` | `1` | Removes gold cost when resetting class talents or specializations at class trainers | Yes |
| `Respec.FreeReagents` | `1` | Removes reagent requirements (e.g. Tome of the Clear Mind) for changing talents and glyphs | Yes |
| `Respec.AutoDualSpec` | `1` | Automatically teaches dual specialization to players upon reaching eligible level (applied on login/level-up) | Yes |
| `MinDualSpecLevel` | `10` | Minimum character level required to acquire dual specialization (fork lowered stock 30 to 10) | Yes |

---

## 4. Solo Experience & Quality of Life (T-019)

Config file: `worldserver.conf.dist`  
Code reference: `src/server/game/World/World.cpp`, `src/server/game/Entities/Player/Player.cpp`, `tools/devenv/SOLO_CONFIG.md`  
All solo configuration keys are cached in `World::LoadConfigSettings()` and updated on `.reload config`.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `AllFlightPaths` | `1` | Automatically unlocks all taxi flight nodes on player login | Yes (applies on next login) |
| `AllowTwoSide.Accounts` | `1` | Allows creating characters of both factions on a single account | Yes |
| `AllowTwoSide.Interaction.Calendar` | `1` | Allows cross-faction calendar event invitations | Yes |
| `AllowTwoSide.Interaction.Channel` | `1` | Allows cross-faction custom chat channels | Yes |
| `AllowTwoSide.Interaction.Group` | `1` | Allows cross-faction party and raid grouping | Yes |
| `AllowTwoSide.Interaction.Guild` | `1` | Allows cross-faction guild invites and membership | Yes |
| `AllowTwoSide.Interaction.Auction` | `1` | Allows cross-faction auction house bidding and buying | Yes |
| `AllowTwoSide.Interaction.Mail` | `1` | Allows sending mail between Alliance and Horde characters | Yes |
| `AllowTwoSide.Interaction.Chat` | `1` | Allows cross-faction public say, yell, and whisper communication | Yes |
| `AllowTwoSide.Interaction.DungeonFinder` | `1` | Allows queuing and matching together in Dungeon Finder across factions | Yes |
| `AllowTwoSide.Trade` | `1` | Allows direct player-to-player trading across factions | Yes |
| `AllowTwoSide.WhoList` | `1` | Displays characters of both factions in the `/who` list | Yes |
| `AllowTwoSide.AddFriend` | `1` | Allows adding cross-faction characters to friend lists | Yes |
| `SkillChance.Orange` | `100` | Skill-up chance percentage for orange crafting and gathering recipes | Yes |
| `SkillChance.Yellow` | `100` | Skill-up chance percentage for yellow crafting and gathering recipes | Yes |
| `SkillChance.Green` | `100` | Skill-up chance percentage for green crafting and gathering recipes | Yes |
| `SkillChance.Grey` | `100` | Skill-up chance percentage for grey crafting and gathering recipes | Yes |
| `SkillChance.Fishing.Always` | `1` | Successful fishing catches always attempt a skill-up below learned cap | Yes |
| `SkillChance.Prospecting` | `1` | Enables skill-up gains when prospecting ore | Yes |
| `SkillChance.Milling` | `1` | Enables skill-up gains when milling herbs | Yes |
| `SkillChance.MiningSteps` | `0` | Disables step-based chance decay for mining node gains | Yes |
| `SkillChance.SkinningSteps` | `0` | Disables step-based chance decay for skinning gains | Yes |
| `Currency.WeeklyCaps.Enable` | `0` | Enables weekly earning caps on currencies like Valor/Conquest (0 = uncapped earning; relog to refresh client displays) | Yes |
| `DurabilityLoss.OnDeath` | `0` | Durability loss percentage on PvE death (0 disables death equipment damage) | Yes |
| `DurabilityLoss.InPvP` | `0` | Durability loss percentage on PvP death | Yes |
| `Death.SicknessLevel` | `91` | Minimum character level at which resurrection sickness applies (91 disables it at MoP level 90 cap) | Yes |
| `Death.CorpseReclaimDelay.PvE` | `0` | Corpse reclaim delay escalation for PvE deaths (0 eliminates reclaim timer escalation) | Yes |
| `Death.CorpseReclaimDelay.PvP` | `0` | Corpse reclaim delay escalation for PvP deaths | Yes |

---

## 5. Dungeons, Raids & AutoBalance Scaling

Config file: `worldserver.conf.dist`  
Code reference: `src/server/game/World/World.cpp`, `src/server/game/LFGAnyLevel/LFGAnyLevel.cpp`, `src/server/game/AutoBalanceLevelScaling/AutoBalanceLevelScaling.cpp`  
All listed scaling and dungeon rules are reloaded on `.reload config`.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `Instance.IgnoreRaid` | `0` | Allows players to enter raid instances without being in a raid group | Yes |
| `Quests.IgnoreRaid` | `0` | Allows completing non-raid quest objectives while inside a raid group | Yes |
| `LFGSolo.Enabled` | `0` | Allows queuing into the Dungeon Finder system solo | Yes |
| `LFG.AnyLevel.Enable` | `0` | Removes character level requirements when queuing for Dungeon Finder dungeons | Yes |
| `LFG.AnyLevel.Raids` | `0` | Extends AnyLevel access to Raid Finder (LFR) wings | Yes |
| `Rate.InstanceResetTime` | `1.0` | Multiplier for global instance reset intervals (higher value means faster resets) | Yes |
| `AutoBalance.Enable` | `0` | Dynamically scales dungeon and raid mob health, damage, and level to group size | Yes |
| `AutoBalance.MinPlayers.Raid` | `1` | Target minimum player count used for scaling normal raid encounters | Yes |
| `AutoBalance.MinPlayers.RaidHeroic` | `1` | Target minimum player count used for scaling heroic raid encounters | Yes |
| `AutoBalance.InflectionPointRaid` | `0.5` | Inflection ratio along player count scaling curve for raid encounters | Yes |
| `AutoBalance.StatModifierRaid.Global` | `1.0` | Global stat multiplier applied to scaled raid creatures | Yes |

*Note on Raid Retention (T-013):* The raid-only retention lifecycle (`Raid.Retention.Enable`, `Raid.ResetPeriodSeconds`, `Raid.Retention.NotifyOnEnter`) is currently pending merge in branch `codex/T-013-raid-resets` and will be documented here once landed.

---

## 6. GM Tools, Capture & Testing

Config file: `worldserver.conf.dist`  
Code reference: `src/server/scripts/Custom/GMTools/`, `src/server/scripts/Custom/Capture/`, `src/server/scripts/Custom/Reload/`

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `GMTools.Clock.MaxSpeed` | `3600` | Maximum speed multiplier permitted by the `.clock speed` command | Yes (read dynamically per command execution) |
| `GMTools.Clock.MaxJumpSeconds` | `2592000` | Maximum time leap in seconds permitted by a single `.clock jump` command (default 30 days) | Yes (read dynamically per command execution) |
| `GMTools.Spectate.AttachTimeoutMs` | `10000` | Timeout in milliseconds for `.spectate` to attach GM to target map before aborting | Yes (read dynamically per command execution) |
| `Capture.TicketsDir` | `""` | Destination directory for `.idea` and `.bug` ticket files (empty string defaults to `<repo>/tickets/`) | Yes (`capture_scripts.cpp` `OnConfigLoad`) |
| `Capture.NearbyBotRadius` | `100` | Radius in yards around the capturing player within which playerbots are captured in tickets | Yes (`capture_scripts.cpp` `OnConfigLoad`) |
| `Random.TestSeed.Enabled` | `0` | Enables deterministic pseudo-random number generator seeding for unit/regression tests | Yes (`CustomReload.cpp`) |
| `Random.TestSeed.Value` | `1` | Deterministic seed value used when `Random.TestSeed.Enabled` is enabled | Yes (`CustomReload.cpp`) |

---

## 7. Playerbots & Living World Population

Config file: `modules/mod_playerbots/config/playerbots.conf.dist`  
Code reference: `modules/mod_playerbots/src/mod_playerbots.cpp`  
*Important:* In `mod_playerbots.cpp`, `PlayerbotsWorldScript::OnConfigLoad` explicitly checks `if (!reloaded)` before loading configuration. Therefore, **none** of the playerbot configuration settings are refreshed by `.reload config`; updating these values requires a full worldserver process restart.

| Key | Default | Effect | Reloadable (.reload config) |
|---|---|---|---|
| `AiPlayerbot.Enabled` | `1` | Master toggle enabling the Playerbots artificial intelligence module | No (server restart required) |
| `AiPlayerbot.RandomBotAutologin` | `1` | Automatically populates the world with background bots on server startup | No (server restart required) |
| `AiPlayerbot.MinRandomBots` | `500` | Target lower bound of active random bots roaming the world | No (server restart required) |
| `AiPlayerbot.MaxRandomBots` | `500` | Target upper bound of active random bots roaming the world | No (server restart required) |
| `AiPlayerbot.RandomBotAccountCount` | `200` | Number of pool accounts generated to host background random bots | No (server restart required) |
| `AiPlayerbot.DisabledWithoutRealPlayer` | `0` | Suspends playerbot update loops when no human players are logged into the realm | No (server restart required) |
| `AiPlayerbot.SummonWhenGroup` | `1` | Automatically summons recruited bots to player master when party forms | No (server restart required) |
| `AiPlayerbot.AllowSummonInCombat` | `1` | Permits summoning bots while party members or master are engaged in combat | No (server restart required) |
| `AiPlayerbot.FreeFood` | `1` | Bots generate their own consumables without requiring items in bag inventory | No (server restart required) |
| `AiPlayerbot.AutoTrainSpells` | `yes` | Bots learn new class spells automatically upon leveling up without trainer visits | No (server restart required) |
| `AiPlayerbot.AutoEquipUpgradeLoot` | `1` | Bots immediately equip items that upgrade their gear upon receiving loot | No (server restart required) |
| `AiPlayerbot.KillXPRate` | `1` | Multiplier for experience awarded to bots for creature kills | No (server restart required) |
| `AiPlayerbot.FollowDistance` | `1.5` | Standard follow distance in yards behind party leader | No (server restart required) |
| `AiPlayerbot.ReactDelay` | `100` | Reaction time latency in milliseconds for bot AI decisions | No (server restart required) |
| `AiPlayerbot.IterationsPerTick` | `10` | Maximum number of decision iterations processed per bot per server tick | No (server restart required) |
| `AiPlayerbot.RandomBotTalk` | `1` | Enables bots chatting in global channels and replying to player whispers | No (server restart required) |

---

## 8. Twink Gear & Loadouts Systems

- **Twinks (T-045, T-048, T-050):** Twink item definitions and checkpoint templates are fully data-driven via SQL tables (`gmtools_twink_candidates`, `gmtools_twink_checkpoints`, `heartseed_twink_item`). They do not utilize `.conf` file keys. Checkpoint gear selection is governed by `TwinkGearRules.cpp` and checkpoint commands (`.checkpoint apply <name>`).
- **Spec Loadouts (T-017):** Server-side spec loadouts (`Loadouts.Enable`, `Loadouts.MaxNamed`) are implemented in pull request #62 (`claude/T-017-loadouts`) and will be added to this document once merged into `playerbot`.

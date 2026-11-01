# Custom Scripts Config Key Audit

Audit of all configuration keys read by `sConfigMgr` under `src/server/scripts/Custom/` against `src/server/worldserver/worldserver.conf.dist`.

> **Task Reference (T-057):** Report only. Do not edit `worldserver.conf.dist`.

## 1. Executive Summary

- **Total `sConfigMgr` calls under `src/server/scripts/Custom/`:** 333
- **Unique configuration keys read:** 333
- **Documented in `worldserver.conf.dist`:** 333 (100%)
- **Missing from `worldserver.conf.dist`:** 0 (0%)

All 333 configuration key lookups under `src/server/scripts/Custom/` are fully documented with default settings in `src/server/worldserver/worldserver.conf.dist`.

### Subsystem Breakdown

| Subsystem / Path | Total Lookups | Unique Keys | Missing Keys |
|---|---|---|---|
| `src/server/scripts/Custom/GMTools/Clock/gmtools_clock.cpp` | 2 | 2 | 0 |
| `src/server/scripts/Custom/GMTools/Spectate/gmtools_spectate.cpp` | 1 | 1 | 0 |
| `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | 29 | 29 | 0 |
| `src/server/scripts/Custom/Reload/CustomReload.cpp` | 2 | 2 | 0 |
| `src/server/scripts/Custom/custom_reward.cpp` | 1 | 1 | 0 |
| `src/server/scripts/Custom/solocraft.cpp` | 298 | 298 | 0 |

---

## 2. Missing Keys

**None.** Every key read by `sConfigMgr` under `src/server/scripts/Custom/` is present and documented in `src/server/worldserver/worldserver.conf.dist`.

---

## 3. Inventory of All Config Key Lookups (File and Line)

The table below lists each configuration key lookup with its source file, line number, configuration key name, and documentation status in `worldserver.conf.dist`.

| # | File | Line | Configuration Key | Status in `worldserver.conf.dist` |
|---|---|---|---|---|
| 1 | `src/server/scripts/Custom/custom_reward.cpp` | L21 | `PlayedTimeReward.vp` | Documented |
| 2 | `src/server/scripts/Custom/GMTools/Clock/gmtools_clock.cpp` | L102 | `GMTools.Clock.MaxSpeed` | Documented |
| 3 | `src/server/scripts/Custom/GMTools/Clock/gmtools_clock.cpp` | L120 | `GMTools.Clock.MaxJumpSeconds` | Documented |
| 4 | `src/server/scripts/Custom/GMTools/Spectate/gmtools_spectate.cpp` | L82 | `GMTools.Spectate.AttachTimeoutMs` | Documented |
| 5 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L166 | `Heartseed.Enabled` | Documented |
| 6 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L167 | `Heartseed.Core.WeaponDamagePct` | Documented |
| 7 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L168 | `Heartseed.Proc.SpellId` | Documented |
| 8 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L169 | `Heartseed.Proc.Chance` | Documented |
| 9 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L170 | `Heartseed.Proc.WeaponDamagePct` | Documented |
| 10 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L171 | `Heartseed.Proc.Cooldown` | Documented |
| 11 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L174 | `Heartseed.Rank.CapPerLevel` | Documented |
| 12 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L175 | `Heartseed.Rank.CapPerAwakening` | Documented |
| 13 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L176 | `Heartseed.Rank.AwakeningFlags` | Documented |
| 14 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L177 | `Heartseed.Rank.GrowthBase` | Documented |
| 15 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L178 | `Heartseed.Rank.GrowthPerRank` | Documented |
| 16 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L179 | `Heartseed.Feed.OnReplace` | Documented |
| 17 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L180 | `Heartseed.Feed.GrowthBase` | Documented |
| 18 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L181 | `Heartseed.Feed.GrowthPerItemLevel` | Documented |
| 19 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L182 | `Heartseed.Feed.QualityPct` | Documented |
| 20 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L183 | `Heartseed.Essence.ProcCooldown` | Documented |
| 21 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L184 | `Heartseed.Essence.AutoSlot` | Documented |
| 22 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L185 | `Heartseed.Story.Hooks` | Documented |
| 23 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L186 | `Heartseed.Story.PandariaMaps` | Documented |
| 24 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L187 | `Heartseed.Story.WorldBosses` | Documented |
| 25 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L190 | `Heartseed.Story.AwakeningSound` | Documented |
| 26 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L191 | `Heartseed.Grant.QuestId` | Documented |
| 27 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L192 | `Heartseed.Grant.OfferOnLogin` | Documented |
| 28 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L193 | `Heartseed.Grant.OfferDelay` | Documented |
| 29 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L194 | `Heartseed.Choice.PromptOnLogin` | Documented |
| 30 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L195 | `Heartseed.Choice.PromptDelay` | Documented |
| 31 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L196 | `Heartseed.Choice.ChangeAtShrineOnly` | Documented |
| 32 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L199 | `Heartseed.Majors.TierLevels` | Documented |
| 33 | `src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp` | L201 | `Heartseed.Essence.SlotLevels` | Documented |
| 34 | `src/server/scripts/Custom/Reload/CustomReload.cpp` | L116 | `Random.TestSeed.Enabled` | Documented |
| 35 | `src/server/scripts/Custom/Reload/CustomReload.cpp` | L122 | `Random.TestSeed.Value` | Documented |
| 36 | `src/server/scripts/Custom/solocraft.cpp` | L62 | `Solocraft.Enable` | Documented |
| 37 | `src/server/scripts/Custom/solocraft.cpp` | L63 | `Solocraft.Announce` | Documented |
| 38 | `src/server/scripts/Custom/solocraft.cpp` | L64 | `SoloCraft.Debuff.Enable` | Documented |
| 39 | `src/server/scripts/Custom/solocraft.cpp` | L65 | `SoloCraft.Spellpower.Mult` | Documented |
| 40 | `src/server/scripts/Custom/solocraft.cpp` | L66 | `SoloCraft.Stats.Mult` | Documented |
| 41 | `src/server/scripts/Custom/solocraft.cpp` | L70 | `SoloCraft.Warrior` | Documented |
| 42 | `src/server/scripts/Custom/solocraft.cpp` | L71 | `SoloCraft.Paladin` | Documented |
| 43 | `src/server/scripts/Custom/solocraft.cpp` | L72 | `SoloCraft.Hunter` | Documented |
| 44 | `src/server/scripts/Custom/solocraft.cpp` | L73 | `SoloCraft.Rogue` | Documented |
| 45 | `src/server/scripts/Custom/solocraft.cpp` | L74 | `SoloCraft.Priest` | Documented |
| 46 | `src/server/scripts/Custom/solocraft.cpp` | L75 | `SoloCraft.Death.Knight` | Documented |
| 47 | `src/server/scripts/Custom/solocraft.cpp` | L76 | `SoloCraft.Shaman` | Documented |
| 48 | `src/server/scripts/Custom/solocraft.cpp` | L77 | `SoloCraft.Mage` | Documented |
| 49 | `src/server/scripts/Custom/solocraft.cpp` | L78 | `SoloCraft.Warlock` | Documented |
| 50 | `src/server/scripts/Custom/solocraft.cpp` | L79 | `SoloCraft.Monk` | Documented |
| 51 | `src/server/scripts/Custom/solocraft.cpp` | L80 | `SoloCraft.Druid` | Documented |
| 52 | `src/server/scripts/Custom/solocraft.cpp` | L83 | `Solocraft.XP.Enabled` | Documented |
| 53 | `src/server/scripts/Custom/solocraft.cpp` | L84 | `Solocraft.XP.Balancing.Enabled` | Documented |
| 54 | `src/server/scripts/Custom/solocraft.cpp` | L85 | `Solocraft.Max.Level.Diff` | Documented |
| 55 | `src/server/scripts/Custom/solocraft.cpp` | L86 | `Solocraft.Dungeon.Level` | Documented |
| 56 | `src/server/scripts/Custom/solocraft.cpp` | L91 | `Solocraft.Stockades.Level` | Documented |
| 57 | `src/server/scripts/Custom/solocraft.cpp` | L92 | `Solocraft.WailingCaverns.Level` | Documented |
| 58 | `src/server/scripts/Custom/solocraft.cpp` | L93 | `Solocraft.RazorfenKraul.Level` | Documented |
| 59 | `src/server/scripts/Custom/solocraft.cpp` | L94 | `Solocraft.BlackfathomDeeps.Level` | Documented |
| 60 | `src/server/scripts/Custom/solocraft.cpp` | L95 | `Solocraft.Uldaman.Level` | Documented |
| 61 | `src/server/scripts/Custom/solocraft.cpp` | L96 | `Solocraft.Gnomeregan.Level` | Documented |
| 62 | `src/server/scripts/Custom/solocraft.cpp` | L97 | `Solocraft.SunkenTemple.Level` | Documented |
| 63 | `src/server/scripts/Custom/solocraft.cpp` | L98 | `Solocraft.RazorfenDowns.Level` | Documented |
| 64 | `src/server/scripts/Custom/solocraft.cpp` | L99 | `Solocraft.ZulFarrak.Level` | Documented |
| 65 | `src/server/scripts/Custom/solocraft.cpp` | L100 | `Solocraft.BlackRockSpire.Level` | Documented |
| 66 | `src/server/scripts/Custom/solocraft.cpp` | L101 | `Solocraft.BlackrockDepths.Level` | Documented |
| 67 | `src/server/scripts/Custom/solocraft.cpp` | L102 | `Solocraft.OnyxiaLair.Level` | Documented |
| 68 | `src/server/scripts/Custom/solocraft.cpp` | L103 | `Solocraft.Stratholme.Level` | Documented |
| 69 | `src/server/scripts/Custom/solocraft.cpp` | L104 | `Solocraft.Mauradon.Level` | Documented |
| 70 | `src/server/scripts/Custom/solocraft.cpp` | L105 | `Solocraft.RagefireChasm.Level` | Documented |
| 71 | `src/server/scripts/Custom/solocraft.cpp` | L106 | `Solocraft.MoltenCore.Level` | Documented |
| 72 | `src/server/scripts/Custom/solocraft.cpp` | L107 | `Solocraft.DireMaul.Level` | Documented |
| 73 | `src/server/scripts/Custom/solocraft.cpp` | L108 | `Solocraft.BlackwingLair.Level` | Documented |
| 74 | `src/server/scripts/Custom/solocraft.cpp` | L109 | `Solocraft.RuinsOfAhnQiraj.Level` | Documented |
| 75 | `src/server/scripts/Custom/solocraft.cpp` | L110 | `Solocraft.TempleOfAhnQiraj.Level` | Documented |
| 76 | `src/server/scripts/Custom/solocraft.cpp` | L112 | `Solocraft.TheBlackMorass.Level` | Documented |
| 77 | `src/server/scripts/Custom/solocraft.cpp` | L113 | `Solocraft.Karazahn.Level` | Documented |
| 78 | `src/server/scripts/Custom/solocraft.cpp` | L114 | `Solocraft.TheBattleForMountHyjal.Level` | Documented |
| 79 | `src/server/scripts/Custom/solocraft.cpp` | L115 | `Solocraft.TheShatteredHalls.Level` | Documented |
| 80 | `src/server/scripts/Custom/solocraft.cpp` | L116 | `Solocraft.TheBloodFurnace.Level` | Documented |
| 81 | `src/server/scripts/Custom/solocraft.cpp` | L117 | `Solocraft.HellfireRampart.Level` | Documented |
| 82 | `src/server/scripts/Custom/solocraft.cpp` | L118 | `Solocraft.MagtheridonsLair.Level` | Documented |
| 83 | `src/server/scripts/Custom/solocraft.cpp` | L119 | `Solocraft.TheSteamVault.Level` | Documented |
| 84 | `src/server/scripts/Custom/solocraft.cpp` | L120 | `Solocraft.TheUnderbog.Level` | Documented |
| 85 | `src/server/scripts/Custom/solocraft.cpp` | L121 | `Solocraft.TheSlavePens.Level` | Documented |
| 86 | `src/server/scripts/Custom/solocraft.cpp` | L122 | `Solocraft.SerpentshrineCavern.Level` | Documented |
| 87 | `src/server/scripts/Custom/solocraft.cpp` | L123 | `Solocraft.TheEye.Level` | Documented |
| 88 | `src/server/scripts/Custom/solocraft.cpp` | L124 | `Solocraft.TheArcatraz.Level` | Documented |
| 89 | `src/server/scripts/Custom/solocraft.cpp` | L125 | `Solocraft.TheBotanica.Level` | Documented |
| 90 | `src/server/scripts/Custom/solocraft.cpp` | L126 | `Solocraft.TheMechanar.Level` | Documented |
| 91 | `src/server/scripts/Custom/solocraft.cpp` | L127 | `Solocraft.ShadowLabyrinth.Level` | Documented |
| 92 | `src/server/scripts/Custom/solocraft.cpp` | L128 | `Solocraft.SethekkHalls.Level` | Documented |
| 93 | `src/server/scripts/Custom/solocraft.cpp` | L129 | `Solocraft.ManaTombs.Level` | Documented |
| 94 | `src/server/scripts/Custom/solocraft.cpp` | L130 | `Solocraft.AuchenaiCrypts.Level` | Documented |
| 95 | `src/server/scripts/Custom/solocraft.cpp` | L131 | `Solocraft.OldHillsbradFoothills.Level` | Documented |
| 96 | `src/server/scripts/Custom/solocraft.cpp` | L132 | `Solocraft.BlackTemple.Level` | Documented |
| 97 | `src/server/scripts/Custom/solocraft.cpp` | L133 | `Solocraft.GruulsLair.Level` | Documented |
| 98 | `src/server/scripts/Custom/solocraft.cpp` | L134 | `Solocraft.SunwellPlateau.Level` | Documented |
| 99 | `src/server/scripts/Custom/solocraft.cpp` | L135 | `Solocraft.MagistersTerrace.Level` | Documented |
| 100 | `src/server/scripts/Custom/solocraft.cpp` | L137 | `Solocraft.Naxxramas.Level` | Documented |
| 101 | `src/server/scripts/Custom/solocraft.cpp` | L138 | `Solocraft.UtgardeKeep.Level` | Documented |
| 102 | `src/server/scripts/Custom/solocraft.cpp` | L139 | `Solocraft.UtgardePinnacle.Level` | Documented |
| 103 | `src/server/scripts/Custom/solocraft.cpp` | L140 | `Solocraft.Oculus.Level` | Documented |
| 104 | `src/server/scripts/Custom/solocraft.cpp` | L141 | `Solocraft.TheCullingOfStratholme.Level` | Documented |
| 105 | `src/server/scripts/Custom/solocraft.cpp` | L142 | `Solocraft.HallsOfStone.Level` | Documented |
| 106 | `src/server/scripts/Custom/solocraft.cpp` | L143 | `Solocraft.DrakTharonKeep.Level` | Documented |
| 107 | `src/server/scripts/Custom/solocraft.cpp` | L144 | `Solocraft.AzjolNerub.Level` | Documented |
| 108 | `src/server/scripts/Custom/solocraft.cpp` | L145 | `Solocraft.HallsOfLighting.Level` | Documented |
| 109 | `src/server/scripts/Custom/solocraft.cpp` | L146 | `Solocraft.Ulduar.Level` | Documented |
| 110 | `src/server/scripts/Custom/solocraft.cpp` | L147 | `Solocraft.GunDrak.Level` | Documented |
| 111 | `src/server/scripts/Custom/solocraft.cpp` | L148 | `Solocraft.VioletHold.Level` | Documented |
| 112 | `src/server/scripts/Custom/solocraft.cpp` | L149 | `Solocraft.TheObsidianSanctum.Level` | Documented |
| 113 | `src/server/scripts/Custom/solocraft.cpp` | L150 | `Solocraft.TheEyeOfEternity.Level` | Documented |
| 114 | `src/server/scripts/Custom/solocraft.cpp` | L151 | `Solocraft.AhnkahetTheOldKingdom.Level` | Documented |
| 115 | `src/server/scripts/Custom/solocraft.cpp` | L152 | `Solocraft.IcecrownCitadel.Level` | Documented |
| 116 | `src/server/scripts/Custom/solocraft.cpp` | L153 | `Solocraft.TheForgeOfSouls.Level` | Documented |
| 117 | `src/server/scripts/Custom/solocraft.cpp` | L154 | `Solocraft.TrialOfTheCrusader.Level` | Documented |
| 118 | `src/server/scripts/Custom/solocraft.cpp` | L155 | `Solocraft.TrialOfTheChampion.Level` | Documented |
| 119 | `src/server/scripts/Custom/solocraft.cpp` | L156 | `Solocraft.PitOfSaron.Level` | Documented |
| 120 | `src/server/scripts/Custom/solocraft.cpp` | L157 | `Solocraft.HallsOfReflection.Level` | Documented |
| 121 | `src/server/scripts/Custom/solocraft.cpp` | L158 | `Solocraft.TheRubySanctum.Level` | Documented |
| 122 | `src/server/scripts/Custom/solocraft.cpp` | L160 | `Solocraft.ShadowfangKeep.Level` | Documented |
| 123 | `src/server/scripts/Custom/solocraft.cpp` | L161 | `Solocraft.DeadMines.Level` | Documented |
| 124 | `src/server/scripts/Custom/solocraft.cpp` | L162 | `Solocraft.BlackrockCaverns.Level` | Documented |
| 125 | `src/server/scripts/Custom/solocraft.cpp` | L163 | `Solocraft.ThroneOfTheTides.Level` | Documented |
| 126 | `src/server/scripts/Custom/solocraft.cpp` | L164 | `Solocraft.TheVortexPinnacle.Level` | Documented |
| 127 | `src/server/scripts/Custom/solocraft.cpp` | L165 | `Solocraft.TheStonecore.Level` | Documented |
| 128 | `src/server/scripts/Custom/solocraft.cpp` | L166 | `Solocraft.LostCityOfTheTol'vir.Level` | Documented |
| 129 | `src/server/scripts/Custom/solocraft.cpp` | L167 | `Solocraft.HallsOfOrigination.Level` | Documented |
| 130 | `src/server/scripts/Custom/solocraft.cpp` | L168 | `Solocraft.GrimBatol.Level` | Documented |
| 131 | `src/server/scripts/Custom/solocraft.cpp` | L169 | `Solocraft.BlackwingDescent.Level` | Documented |
| 132 | `src/server/scripts/Custom/solocraft.cpp` | L170 | `Solocraft.TheBastionOfTwilight.Level` | Documented |
| 133 | `src/server/scripts/Custom/solocraft.cpp` | L171 | `Solocraft.ThroneOfTheFourWinds.Level` | Documented |
| 134 | `src/server/scripts/Custom/solocraft.cpp` | L172 | `Solocraft.BaradinHold.Level` | Documented |
| 135 | `src/server/scripts/Custom/solocraft.cpp` | L173 | `Solocraft.Firelands.Level` | Documented |
| 136 | `src/server/scripts/Custom/solocraft.cpp` | L174 | `Solocraft.DragonSoul.Level` | Documented |
| 137 | `src/server/scripts/Custom/solocraft.cpp` | L175 | `Solocraft.EndTime.Level` | Documented |
| 138 | `src/server/scripts/Custom/solocraft.cpp` | L176 | `Solocraft.WellOfEternity.Level` | Documented |
| 139 | `src/server/scripts/Custom/solocraft.cpp` | L177 | `Solocraft.HourOfTwilight.Level` | Documented |
| 140 | `src/server/scripts/Custom/solocraft.cpp` | L178 | `Solocraft.Zul'gurub.Level` | Documented |
| 141 | `src/server/scripts/Custom/solocraft.cpp` | L179 | `Solocraft.ZulAman.Level` | Documented |
| 142 | `src/server/scripts/Custom/solocraft.cpp` | L180 | `Solocraft.Nexus.Level` | Documented |
| 143 | `src/server/scripts/Custom/solocraft.cpp` | L182 | `Solocraft.ShadoPanMonastery.Level` | Documented |
| 144 | `src/server/scripts/Custom/solocraft.cpp` | L183 | `Solocraft.Scholomance.Level` | Documented |
| 145 | `src/server/scripts/Custom/solocraft.cpp` | L184 | `Solocraft.ScarletMonastery.Level` | Documented |
| 146 | `src/server/scripts/Custom/solocraft.cpp` | L185 | `Solocraft.Mogu'shanPalace.Level` | Documented |
| 147 | `src/server/scripts/Custom/solocraft.cpp` | L186 | `Solocraft.Mogu'shanVaults.Level` | Documented |
| 148 | `src/server/scripts/Custom/solocraft.cpp` | L187 | `Solocraft.SiegeOfOrgrimmar.Level` | Documented |
| 149 | `src/server/scripts/Custom/solocraft.cpp` | L188 | `Solocraft.ThroneOfThunder.Level` | Documented |
| 150 | `src/server/scripts/Custom/solocraft.cpp` | L189 | `Solocraft.HeartOfFear.Level` | Documented |
| 151 | `src/server/scripts/Custom/solocraft.cpp` | L190 | `Solocraft.TerraceOfEndlessSpring.Level` | Documented |
| 152 | `src/server/scripts/Custom/solocraft.cpp` | L191 | `Solocraft.ScarletHalls.Level` | Documented |
| 153 | `src/server/scripts/Custom/solocraft.cpp` | L192 | `Solocraft.GateOfTheSettingSun.Level` | Documented |
| 154 | `src/server/scripts/Custom/solocraft.cpp` | L193 | `Solocraft.SiegeOfNiuzaoTemple.Level` | Documented |
| 155 | `src/server/scripts/Custom/solocraft.cpp` | L194 | `Solocraft.TempleOfTheJadeSerpent.Level` | Documented |
| 156 | `src/server/scripts/Custom/solocraft.cpp` | L195 | `Solocraft.StormstoutBrewery.Level` | Documented |
| 157 | `src/server/scripts/Custom/solocraft.cpp` | L199 | `Solocraft.Dungeon` | Documented |
| 158 | `src/server/scripts/Custom/solocraft.cpp` | L200 | `Solocraft.Heroic` | Documented |
| 159 | `src/server/scripts/Custom/solocraft.cpp` | L201 | `Solocraft.Raid25` | Documented |
| 160 | `src/server/scripts/Custom/solocraft.cpp` | L202 | `Solocraft.Raid40` | Documented |
| 161 | `src/server/scripts/Custom/solocraft.cpp` | L207 | `Solocraft.Stockades` | Documented |
| 162 | `src/server/scripts/Custom/solocraft.cpp` | L208 | `Solocraft.WailingCaverns` | Documented |
| 163 | `src/server/scripts/Custom/solocraft.cpp` | L209 | `Solocraft.RazorfenKraul` | Documented |
| 164 | `src/server/scripts/Custom/solocraft.cpp` | L210 | `Solocraft.BlackfathomDeeps` | Documented |
| 165 | `src/server/scripts/Custom/solocraft.cpp` | L211 | `Solocraft.Uldaman` | Documented |
| 166 | `src/server/scripts/Custom/solocraft.cpp` | L212 | `Solocraft.Gnomeregan` | Documented |
| 167 | `src/server/scripts/Custom/solocraft.cpp` | L213 | `Solocraft.SunkenTemple` | Documented |
| 168 | `src/server/scripts/Custom/solocraft.cpp` | L214 | `Solocraft.RazorfenDowns` | Documented |
| 169 | `src/server/scripts/Custom/solocraft.cpp` | L215 | `Solocraft.ZulFarrak` | Documented |
| 170 | `src/server/scripts/Custom/solocraft.cpp` | L216 | `Solocraft.BlackRockSpire` | Documented |
| 171 | `src/server/scripts/Custom/solocraft.cpp` | L217 | `Solocraft.BlackrockDepths` | Documented |
| 172 | `src/server/scripts/Custom/solocraft.cpp` | L218 | `Solocraft.OnyxiaLair` | Documented |
| 173 | `src/server/scripts/Custom/solocraft.cpp` | L219 | `Solocraft.Stratholme` | Documented |
| 174 | `src/server/scripts/Custom/solocraft.cpp` | L220 | `Solocraft.Mauradon` | Documented |
| 175 | `src/server/scripts/Custom/solocraft.cpp` | L221 | `Solocraft.RagefireChasm` | Documented |
| 176 | `src/server/scripts/Custom/solocraft.cpp` | L222 | `Solocraft.MoltenCore` | Documented |
| 177 | `src/server/scripts/Custom/solocraft.cpp` | L223 | `Solocraft.DireMaul` | Documented |
| 178 | `src/server/scripts/Custom/solocraft.cpp` | L224 | `Solocraft.BlackwingLair` | Documented |
| 179 | `src/server/scripts/Custom/solocraft.cpp` | L225 | `Solocraft.RuinsOfAhnQiraj` | Documented |
| 180 | `src/server/scripts/Custom/solocraft.cpp` | L226 | `Solocraft.TempleOfAhnQiraj` | Documented |
| 181 | `src/server/scripts/Custom/solocraft.cpp` | L228 | `Solocraft.TheBlackMorass` | Documented |
| 182 | `src/server/scripts/Custom/solocraft.cpp` | L229 | `Solocraft.Karazahn` | Documented |
| 183 | `src/server/scripts/Custom/solocraft.cpp` | L230 | `Solocraft.TheBattleForMountHyjal` | Documented |
| 184 | `src/server/scripts/Custom/solocraft.cpp` | L231 | `Solocraft.TheShatteredHalls` | Documented |
| 185 | `src/server/scripts/Custom/solocraft.cpp` | L232 | `Solocraft.TheBloodFurnace` | Documented |
| 186 | `src/server/scripts/Custom/solocraft.cpp` | L233 | `Solocraft.HellfireRampart` | Documented |
| 187 | `src/server/scripts/Custom/solocraft.cpp` | L234 | `Solocraft.MagtheridonsLair` | Documented |
| 188 | `src/server/scripts/Custom/solocraft.cpp` | L235 | `Solocraft.TheSteamVault` | Documented |
| 189 | `src/server/scripts/Custom/solocraft.cpp` | L236 | `Solocraft.TheUnderbog` | Documented |
| 190 | `src/server/scripts/Custom/solocraft.cpp` | L237 | `Solocraft.TheSlavePens` | Documented |
| 191 | `src/server/scripts/Custom/solocraft.cpp` | L238 | `Solocraft.SerpentshrineCavern` | Documented |
| 192 | `src/server/scripts/Custom/solocraft.cpp` | L239 | `Solocraft.TheEye` | Documented |
| 193 | `src/server/scripts/Custom/solocraft.cpp` | L240 | `Solocraft.TheArcatraz` | Documented |
| 194 | `src/server/scripts/Custom/solocraft.cpp` | L241 | `Solocraft.TheBotanica` | Documented |
| 195 | `src/server/scripts/Custom/solocraft.cpp` | L242 | `Solocraft.TheMechanar` | Documented |
| 196 | `src/server/scripts/Custom/solocraft.cpp` | L243 | `Solocraft.ShadowLabyrinth` | Documented |
| 197 | `src/server/scripts/Custom/solocraft.cpp` | L244 | `Solocraft.SethekkHalls` | Documented |
| 198 | `src/server/scripts/Custom/solocraft.cpp` | L245 | `Solocraft.ManaTombs` | Documented |
| 199 | `src/server/scripts/Custom/solocraft.cpp` | L246 | `Solocraft.AuchenaiCrypts` | Documented |
| 200 | `src/server/scripts/Custom/solocraft.cpp` | L247 | `Solocraft.OldHillsbradFoothills` | Documented |
| 201 | `src/server/scripts/Custom/solocraft.cpp` | L248 | `Solocraft.BlackTemple` | Documented |
| 202 | `src/server/scripts/Custom/solocraft.cpp` | L249 | `Solocraft.GruulsLair` | Documented |
| 203 | `src/server/scripts/Custom/solocraft.cpp` | L250 | `Solocraft.SunwellPlateau` | Documented |
| 204 | `src/server/scripts/Custom/solocraft.cpp` | L251 | `Solocraft.MagistersTerrace` | Documented |
| 205 | `src/server/scripts/Custom/solocraft.cpp` | L253 | `Solocraft.Naxxramas` | Documented |
| 206 | `src/server/scripts/Custom/solocraft.cpp` | L254 | `Solocraft.UtgardeKeep` | Documented |
| 207 | `src/server/scripts/Custom/solocraft.cpp` | L255 | `Solocraft.UtgardePinnacle` | Documented |
| 208 | `src/server/scripts/Custom/solocraft.cpp` | L256 | `Solocraft.Oculus` | Documented |
| 209 | `src/server/scripts/Custom/solocraft.cpp` | L257 | `Solocraft.TheCullingOfStratholme` | Documented |
| 210 | `src/server/scripts/Custom/solocraft.cpp` | L258 | `Solocraft.HallsOfStone` | Documented |
| 211 | `src/server/scripts/Custom/solocraft.cpp` | L259 | `Solocraft.DrakTharonKeep` | Documented |
| 212 | `src/server/scripts/Custom/solocraft.cpp` | L260 | `Solocraft.AzjolNerub` | Documented |
| 213 | `src/server/scripts/Custom/solocraft.cpp` | L261 | `Solocraft.HallsOfLighting` | Documented |
| 214 | `src/server/scripts/Custom/solocraft.cpp` | L262 | `Solocraft.Ulduar` | Documented |
| 215 | `src/server/scripts/Custom/solocraft.cpp` | L263 | `Solocraft.GunDrak` | Documented |
| 216 | `src/server/scripts/Custom/solocraft.cpp` | L264 | `Solocraft.VioletHold` | Documented |
| 217 | `src/server/scripts/Custom/solocraft.cpp` | L265 | `Solocraft.TheObsidianSanctum` | Documented |
| 218 | `src/server/scripts/Custom/solocraft.cpp` | L266 | `Solocraft.TheEyeOfEternity` | Documented |
| 219 | `src/server/scripts/Custom/solocraft.cpp` | L267 | `Solocraft.AhnkahetTheOldKingdom` | Documented |
| 220 | `src/server/scripts/Custom/solocraft.cpp` | L268 | `Solocraft.IcecrownCitadel` | Documented |
| 221 | `src/server/scripts/Custom/solocraft.cpp` | L269 | `Solocraft.TheForgeOfSouls` | Documented |
| 222 | `src/server/scripts/Custom/solocraft.cpp` | L270 | `Solocraft.TrialOfTheCrusader` | Documented |
| 223 | `src/server/scripts/Custom/solocraft.cpp` | L271 | `Solocraft.TrialOfTheChampion` | Documented |
| 224 | `src/server/scripts/Custom/solocraft.cpp` | L272 | `Solocraft.PitOfSaron` | Documented |
| 225 | `src/server/scripts/Custom/solocraft.cpp` | L273 | `Solocraft.HallsOfReflection` | Documented |
| 226 | `src/server/scripts/Custom/solocraft.cpp` | L274 | `Solocraft.TheRubySanctum` | Documented |
| 227 | `src/server/scripts/Custom/solocraft.cpp` | L276 | `Solocraft.ShadowfangKeep` | Documented |
| 228 | `src/server/scripts/Custom/solocraft.cpp` | L277 | `Solocraft.DeadMines` | Documented |
| 229 | `src/server/scripts/Custom/solocraft.cpp` | L278 | `Solocraft.BlackrockCaverns` | Documented |
| 230 | `src/server/scripts/Custom/solocraft.cpp` | L279 | `Solocraft.ThroneOfTheTides` | Documented |
| 231 | `src/server/scripts/Custom/solocraft.cpp` | L280 | `Solocraft.TheVortexPinnacle` | Documented |
| 232 | `src/server/scripts/Custom/solocraft.cpp` | L281 | `Solocraft.TheStonecore` | Documented |
| 233 | `src/server/scripts/Custom/solocraft.cpp` | L282 | `Solocraft.LostCityOfTheTol'vir` | Documented |
| 234 | `src/server/scripts/Custom/solocraft.cpp` | L283 | `Solocraft.HallsOfOrigination` | Documented |
| 235 | `src/server/scripts/Custom/solocraft.cpp` | L284 | `Solocraft.GrimBatol` | Documented |
| 236 | `src/server/scripts/Custom/solocraft.cpp` | L285 | `Solocraft.BlackwingDescent` | Documented |
| 237 | `src/server/scripts/Custom/solocraft.cpp` | L286 | `Solocraft.TheBastionOfTwilight` | Documented |
| 238 | `src/server/scripts/Custom/solocraft.cpp` | L287 | `Solocraft.ThroneOfTheFourWinds` | Documented |
| 239 | `src/server/scripts/Custom/solocraft.cpp` | L288 | `Solocraft.BaradinHold` | Documented |
| 240 | `src/server/scripts/Custom/solocraft.cpp` | L289 | `Solocraft.Firelands` | Documented |
| 241 | `src/server/scripts/Custom/solocraft.cpp` | L290 | `Solocraft.DragonSoul` | Documented |
| 242 | `src/server/scripts/Custom/solocraft.cpp` | L291 | `Solocraft.Zul'gurub` | Documented |
| 243 | `src/server/scripts/Custom/solocraft.cpp` | L292 | `Solocraft.ZulAman` | Documented |
| 244 | `src/server/scripts/Custom/solocraft.cpp` | L293 | `Solocraft.Nexus` | Documented |
| 245 | `src/server/scripts/Custom/solocraft.cpp` | L295 | `Solocraft.ShadoPanMonastery` | Documented |
| 246 | `src/server/scripts/Custom/solocraft.cpp` | L296 | `Solocraft.Scholomance` | Documented |
| 247 | `src/server/scripts/Custom/solocraft.cpp` | L297 | `Solocraft.ScarletMonastery` | Documented |
| 248 | `src/server/scripts/Custom/solocraft.cpp` | L298 | `Solocraft.Mogu'shanPalace` | Documented |
| 249 | `src/server/scripts/Custom/solocraft.cpp` | L299 | `Solocraft.Mogu'shanVaults` | Documented |
| 250 | `src/server/scripts/Custom/solocraft.cpp` | L300 | `Solocraft.SiegeOfOrgrimmar` | Documented |
| 251 | `src/server/scripts/Custom/solocraft.cpp` | L301 | `Solocraft.ThroneOfThunder` | Documented |
| 252 | `src/server/scripts/Custom/solocraft.cpp` | L302 | `Solocraft.HeartOfFear` | Documented |
| 253 | `src/server/scripts/Custom/solocraft.cpp` | L303 | `Solocraft.TerraceOfEndlessSpring` | Documented |
| 254 | `src/server/scripts/Custom/solocraft.cpp` | L304 | `Solocraft.ScarletHalls` | Documented |
| 255 | `src/server/scripts/Custom/solocraft.cpp` | L305 | `Solocraft.GateOfTheSettingSun` | Documented |
| 256 | `src/server/scripts/Custom/solocraft.cpp` | L306 | `Solocraft.SiegeOfNiuzaoTemple` | Documented |
| 257 | `src/server/scripts/Custom/solocraft.cpp` | L307 | `Solocraft.TempleOfTheJadeSerpent` | Documented |
| 258 | `src/server/scripts/Custom/solocraft.cpp` | L308 | `Solocraft.StormstoutBrewery` | Documented |
| 259 | `src/server/scripts/Custom/solocraft.cpp` | L314 | `Solocraft.TheBlackMorassH` | Documented |
| 260 | `src/server/scripts/Custom/solocraft.cpp` | L315 | `Solocraft.TheShatteredHallsH` | Documented |
| 261 | `src/server/scripts/Custom/solocraft.cpp` | L316 | `Solocraft.TheBloodFurnaceH` | Documented |
| 262 | `src/server/scripts/Custom/solocraft.cpp` | L317 | `Solocraft.HellfireRampartH` | Documented |
| 263 | `src/server/scripts/Custom/solocraft.cpp` | L318 | `Solocraft.TheSteamVaultH` | Documented |
| 264 | `src/server/scripts/Custom/solocraft.cpp` | L319 | `Solocraft.TheUnderbogH` | Documented |
| 265 | `src/server/scripts/Custom/solocraft.cpp` | L320 | `Solocraft.TheSlavePensH` | Documented |
| 266 | `src/server/scripts/Custom/solocraft.cpp` | L321 | `Solocraft.TheArcatrazH` | Documented |
| 267 | `src/server/scripts/Custom/solocraft.cpp` | L322 | `Solocraft.TheBotanicaH` | Documented |
| 268 | `src/server/scripts/Custom/solocraft.cpp` | L323 | `Solocraft.TheMechanarH` | Documented |
| 269 | `src/server/scripts/Custom/solocraft.cpp` | L324 | `Solocraft.ShadowLabyrinthH` | Documented |
| 270 | `src/server/scripts/Custom/solocraft.cpp` | L325 | `Solocraft.SethekkHallsH` | Documented |
| 271 | `src/server/scripts/Custom/solocraft.cpp` | L326 | `Solocraft.ManaTombsH` | Documented |
| 272 | `src/server/scripts/Custom/solocraft.cpp` | L327 | `Solocraft.AuchenaiCryptsH` | Documented |
| 273 | `src/server/scripts/Custom/solocraft.cpp` | L328 | `Solocraft.OldHillsbradFoothillsH` | Documented |
| 274 | `src/server/scripts/Custom/solocraft.cpp` | L329 | `Solocraft.MagistersTerraceH` | Documented |
| 275 | `src/server/scripts/Custom/solocraft.cpp` | L331 | `Solocraft.NaxxramasH` | Documented |
| 276 | `src/server/scripts/Custom/solocraft.cpp` | L332 | `Solocraft.UtgardeKeepH` | Documented |
| 277 | `src/server/scripts/Custom/solocraft.cpp` | L333 | `Solocraft.UtgardePinnacleH` | Documented |
| 278 | `src/server/scripts/Custom/solocraft.cpp` | L334 | `Solocraft.OculusH` | Documented |
| 279 | `src/server/scripts/Custom/solocraft.cpp` | L335 | `Solocraft.TheCullingOfStratholmeH` | Documented |
| 280 | `src/server/scripts/Custom/solocraft.cpp` | L336 | `Solocraft.HallsOfStoneH` | Documented |
| 281 | `src/server/scripts/Custom/solocraft.cpp` | L337 | `Solocraft.DrakTharonKeepH` | Documented |
| 282 | `src/server/scripts/Custom/solocraft.cpp` | L338 | `Solocraft.AzjolNerubH` | Documented |
| 283 | `src/server/scripts/Custom/solocraft.cpp` | L339 | `Solocraft.HallsOfLightingH` | Documented |
| 284 | `src/server/scripts/Custom/solocraft.cpp` | L340 | `Solocraft.UlduarH` | Documented |
| 285 | `src/server/scripts/Custom/solocraft.cpp` | L341 | `Solocraft.GunDrakH` | Documented |
| 286 | `src/server/scripts/Custom/solocraft.cpp` | L342 | `Solocraft.VioletHoldH` | Documented |
| 287 | `src/server/scripts/Custom/solocraft.cpp` | L343 | `Solocraft.TheObsidianSanctumH` | Documented |
| 288 | `src/server/scripts/Custom/solocraft.cpp` | L344 | `Solocraft.TheEyeOfEternityH` | Documented |
| 289 | `src/server/scripts/Custom/solocraft.cpp` | L345 | `Solocraft.AhnkahetTheOldKingdomH` | Documented |
| 290 | `src/server/scripts/Custom/solocraft.cpp` | L346 | `Solocraft.IcecrownCitadelH` | Documented |
| 291 | `src/server/scripts/Custom/solocraft.cpp` | L347 | `Solocraft.TheForgeOfSoulsH` | Documented |
| 292 | `src/server/scripts/Custom/solocraft.cpp` | L348 | `Solocraft.TrialOfTheCrusaderH` | Documented |
| 293 | `src/server/scripts/Custom/solocraft.cpp` | L349 | `Solocraft.TrialOfTheChampionH` | Documented |
| 294 | `src/server/scripts/Custom/solocraft.cpp` | L350 | `Solocraft.PitOfSaronH` | Documented |
| 295 | `src/server/scripts/Custom/solocraft.cpp` | L351 | `Solocraft.HallsOfReflectionH` | Documented |
| 296 | `src/server/scripts/Custom/solocraft.cpp` | L352 | `Solocraft.TheRubySanctumH` | Documented |
| 297 | `src/server/scripts/Custom/solocraft.cpp` | L354 | `Solocraft.ShadowfangKeepH` | Documented |
| 298 | `src/server/scripts/Custom/solocraft.cpp` | L355 | `Solocraft.DeadMinesH` | Documented |
| 299 | `src/server/scripts/Custom/solocraft.cpp` | L356 | `Solocraft.BlackrockCavernsH` | Documented |
| 300 | `src/server/scripts/Custom/solocraft.cpp` | L357 | `Solocraft.ThroneOfTheTidesH` | Documented |
| 301 | `src/server/scripts/Custom/solocraft.cpp` | L358 | `Solocraft.TheVortexPinnacleH` | Documented |
| 302 | `src/server/scripts/Custom/solocraft.cpp` | L359 | `Solocraft.TheStonecoreH` | Documented |
| 303 | `src/server/scripts/Custom/solocraft.cpp` | L360 | `Solocraft.LostCityOfTheTol'virH` | Documented |
| 304 | `src/server/scripts/Custom/solocraft.cpp` | L361 | `Solocraft.HallsOfOriginationH` | Documented |
| 305 | `src/server/scripts/Custom/solocraft.cpp` | L362 | `Solocraft.GrimBatolH` | Documented |
| 306 | `src/server/scripts/Custom/solocraft.cpp` | L363 | `Solocraft.BlackwingDescentH` | Documented |
| 307 | `src/server/scripts/Custom/solocraft.cpp` | L364 | `Solocraft.TheBastionOfTwilightH` | Documented |
| 308 | `src/server/scripts/Custom/solocraft.cpp` | L365 | `Solocraft.ThroneOfTheFourWindsH` | Documented |
| 309 | `src/server/scripts/Custom/solocraft.cpp` | L366 | `Solocraft.BaradinHoldH` | Documented |
| 310 | `src/server/scripts/Custom/solocraft.cpp` | L367 | `Solocraft.FirelandsH` | Documented |
| 311 | `src/server/scripts/Custom/solocraft.cpp` | L368 | `Solocraft.DragonSoulH` | Documented |
| 312 | `src/server/scripts/Custom/solocraft.cpp` | L369 | `Solocraft.EndTimeH` | Documented |
| 313 | `src/server/scripts/Custom/solocraft.cpp` | L370 | `Solocraft.WellOfEternityH` | Documented |
| 314 | `src/server/scripts/Custom/solocraft.cpp` | L371 | `Solocraft.HourOfTwilightH` | Documented |
| 315 | `src/server/scripts/Custom/solocraft.cpp` | L372 | `Solocraft.Zul'gurubH` | Documented |
| 316 | `src/server/scripts/Custom/solocraft.cpp` | L373 | `Solocraft.ZulAmanH` | Documented |
| 317 | `src/server/scripts/Custom/solocraft.cpp` | L374 | `Solocraft.NexusH` | Documented |
| 318 | `src/server/scripts/Custom/solocraft.cpp` | L376 | `Solocraft.ShadoPanMonasteryH` | Documented |
| 319 | `src/server/scripts/Custom/solocraft.cpp` | L377 | `Solocraft.ScholomanceH` | Documented |
| 320 | `src/server/scripts/Custom/solocraft.cpp` | L378 | `Solocraft.ScarletMonasteryH` | Documented |
| 321 | `src/server/scripts/Custom/solocraft.cpp` | L379 | `Solocraft.Mogu'shanPalaceH` | Documented |
| 322 | `src/server/scripts/Custom/solocraft.cpp` | L380 | `Solocraft.Mogu'shanVaultsH` | Documented |
| 323 | `src/server/scripts/Custom/solocraft.cpp` | L381 | `Solocraft.SiegeOfOrgrimmarH` | Documented |
| 324 | `src/server/scripts/Custom/solocraft.cpp` | L382 | `Solocraft.ThroneOfThunderH` | Documented |
| 325 | `src/server/scripts/Custom/solocraft.cpp` | L383 | `Solocraft.HeartOfFearH` | Documented |
| 326 | `src/server/scripts/Custom/solocraft.cpp` | L384 | `Solocraft.TerraceOfEndlessSpringH` | Documented |
| 327 | `src/server/scripts/Custom/solocraft.cpp` | L385 | `Solocraft.ScarletHallsH` | Documented |
| 328 | `src/server/scripts/Custom/solocraft.cpp` | L386 | `Solocraft.GateOfTheSettingSunH` | Documented |
| 329 | `src/server/scripts/Custom/solocraft.cpp` | L387 | `Solocraft.SiegeOfNiuzaoTempleH` | Documented |
| 330 | `src/server/scripts/Custom/solocraft.cpp` | L388 | `Solocraft.TempleOfTheJadeSerpentH` | Documented |
| 331 | `src/server/scripts/Custom/solocraft.cpp` | L389 | `Solocraft.StormstoutBreweryH` | Documented |
| 332 | `src/server/scripts/Custom/solocraft.cpp` | L392 | `Solocraft.ArgentTournamentRaidH10` | Documented |
| 333 | `src/server/scripts/Custom/solocraft.cpp` | L393 | `Solocraft.ArgentTournamentRaidH25` | Documented |

---

## 4. Ancillary Custom Module Config Probes (Outside `Custom/`)

Several custom subsystems register their script hooks in `src/server/scripts/Custom/` but load their config caches in `src/server/game/`. For completeness, their config keys were also verified against `worldserver.conf.dist`:

| Subsystem | Source File | Configuration Key | Status in `worldserver.conf.dist` |
|---|---|---|---|
| Mounted Speed | `src/server/game/Entities/Unit/MountedSpeed.cpp:16` | `Rate.MountedSpeed` | Documented |
| Respec | `src/server/game/World/RespecConfig.cpp:12` | `Respec.FreeReagents` | Documented |
| Respec | `src/server/game/World/RespecConfig.cpp:14` | `Respec.AutoDualSpec` | Documented |
| Solo Config | `src/server/game/World/SoloConfig.cpp:17` | `Currency.WeeklyCaps.Enable` | Documented |
| Solo Config | `src/server/game/World/SoloConfig.cpp:19` | `SkillChance.Fishing.Always` | Documented |

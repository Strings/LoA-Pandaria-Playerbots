# Inherited Custom Scripts Audit

Comprehensive audit of loose inherited scripts located directly under `src/server/scripts/Custom/` (not within custom subdirectories).

> **Task Reference (T-067):** Report only. Do not edit code or SQL files.

---

## 1. Executive Summary

A survey was performed on all C++ source files directly in `src/server/scripts/Custom/` that are not contained within modular subdirectories (`Checkpoints`, `Clock`, `Feedback`, `Heartseed`, `MountedSpeed`, `Reload`, `Respec`, `Scenarios`, `SoloConfig`, `Spectate`).

### Script Inventory Overview

| File | Primary Purpose | ScriptLoader Status | Default Enabled | Overlap System |
|---|---|---|---|---|
| `boost_profession.cpp` | Item-based profession level booster (to 300 / 600) | Commented out (`L2170`) & typo | No | Starting Kit (T-011) / GM commands |
| `custom_items.cpp` | Consumable currency tokens (Honor, Justice, Valor, Conquest) | Commented out (`L2171`) & typo | No | GM commands (`.modify currency`) |
| `custom_reward.cpp` | Played time BattlePay VP coins reward | Commented out (`L2169`) | No | Server economy / Donor rewards |
| `emo_stone.cpp` | Item to summon a temporary teleporter NPC | Not registered (missing) | No | Starting Kit (T-011) / GM Tools (`.tele`) |
| `emo_teleporter.cpp` | Full-world teleportation NPC (dungeons, raids, cities) | Not registered (missing) | No | GM Tools (`.tele`, `.checkpoint`) |
| `npc_teleport.cpp` | City teleportation NPC for faction capitals & hubs | Commented out (`L2167`) | No | GM Tools (`.checkpoint hs-10`, `.tele`) |
| `PlayerStartScript.cpp` | Teaches all glyphs, all riding, gives polar bear, bags, tomes | Not registered (missing) | No | Starting Kit (T-011), Respec (T-040) |
| `protected_zone.cpp` | Teleports non-GMs out of high-end Pandaria islands | Not registered (missing & commented) | No | Gating / Story progression |
| `solocraft.cpp` | Instance stat multiplier scaling system for small groups | Registered (`L2172`) | No (`Solocraft.Enable = 0`) | SoloConfig (T-043) |
| `wow_token.cpp` | Consumable donor coin tokens | Commented out (`L2168`) | No | Donor system |

---

## 2. Detailed Audit by Script

### 2.1. `solocraft.cpp`
- **Location:** [`src/server/scripts/Custom/solocraft.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp) (739 lines)
- **What it does:**
  Implements a dynamic dungeon/raid player buffing system (`solocraft_system_announce` at [L464](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L464) and `solocraft_system_player_instance_handler` at [L496](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L496)). When a player enters an instance, it calculates an offset based on the instance's intended group size vs. current group count. It multiplies primary player stats (Strength, Agility, Stamina, Intellect, Spirit) via `HandleStatModifier` ([L717](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L717)), provides flat spellpower bonuses ([L722](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L722)), and optionally throttles or balances XP gains ([L727](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L727)). Upon instance exit, buffs are removed and state is purged ([L703-L732](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L703-L732)).
- **Configuration Keys:**
  - `Solocraft.Enable` ([L62](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L62), default `0`)
  - `Solocraft.Announce` ([L63](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L63), default `1`)
  - `SoloCraft.Debuff.Enable` ([L64](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L64), default `1`)
  - `SoloCraft.Spellpower.Mult` ([L65](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L65), default `2.5`)
  - `SoloCraft.Stats.Mult` ([L66](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L66), default `100.0`)
  - `SoloCraft.Warrior` through `SoloCraft.Druid` ([L68-L81](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L68-L81), default `100`)
  - `Solocraft.XP.Enabled` ([L83](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L83), default `1`)
  - `Solocraft.XP.Balancing.Enabled` ([L84](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L84), default `1`)
  - `Solocraft.Max.Level.Diff` ([L85](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L85), default `10`)
  - `Solocraft.Dungeon.Level` ([L86](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L86), default `90`)
  - 286 individual per-dungeon level threshold keys ([L88-L459](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L88-L459)).
- **Database Tables Created or Read:**
  - Reads and writes to `characters.custom_solocraft_character_stats` ([L705, L730](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L705-L730)).
  - Reads localization strings from `world.trinity_string` (IDs 30000 to 30006, defined in enum at [L35-L44](file:///I:/UltimateMoP/src/server/scripts/Custom/solocraft.cpp#L35-L44)).
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_solocraft_system();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2151`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2151).
  - Registered: Yes, `AddSC_solocraft_system();` is called at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2172`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2172).
- **Enabled by Default:**
  - **No.** `Solocraft.Enable` defaults to `0` in code and config.
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - **Direct conflict with `SoloConfig` (T-043):** `SoloConfig` scales instance creatures and boss mechanics down to fit party size (1–5 players), allowing authentic character stats and bot integration. `solocraft.cpp` takes the opposite (legacy) approach by heavily inflating player stats and spellpower into unnatural values.

---

### 2.2. `emo_stone.cpp`
- **Location:** [`src/server/scripts/Custom/emo_stone.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_stone.cpp) (43 lines)
- **What it does:**
  Implements `custom_emo_stone` ([L3](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_stone.cpp#L3)), an `ItemScript` triggered on item use. Shows a gossip menu offering to summon creature entry 128 (teleporter NPC) at the player's coordinates for 60 seconds ([L27](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_stone.cpp#L27)) using `SummonCreature(128, ..., TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000ms)`.
- **Configuration Keys:** None.
- **Database Tables Created or Read:** None.
- **ScriptLoader Registration:**
  - Declared: **No.**
  - Registered: **No.** The entry function `AddSC_custom_emo_stone()` ([L40](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_stone.cpp#L40)) is never called in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.** (Completely unlinked/dead code).
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Overlaps with starting convenience items and GM Tools `.tele`. In UltimateMoP, convenience travel is handled via standard hearthstones, flight paths, and `.checkpoint` GM commands.

---

### 2.3. `emo_teleporter.cpp`
- **Location:** [`src/server/scripts/Custom/emo_teleporter.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_teleporter.cpp) (389 lines)
- **What it does:**
  Implements `npc_world_teleporter` ([L352](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_teleporter.cpp#L352)), a `CreatureScript` providing a comprehensive, multi-tiered gossip teleportation menu. Drives teleports across Eastern Kingdoms, Kalimdor, Outland, Northrend, Pandaria, vanilla/TBC/WotLK/MoP dungeons, raids, world bosses (Sha of Anger, Galleon, Nalak, Oondasta, Celestials, Ordos), and PvP arenas from a hardcoded coordinate table `pandariq[]` ([L16-L349](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_teleporter.cpp#L16-L349)).
- **Configuration Keys:** None.
- **Database Tables Created or Read:** None.
- **ScriptLoader Registration:**
  - Declared: **No.**
  - Registered: **No.** `AddSC_npc_world_teleporter()` ([L386](file:///I:/UltimateMoP/src/server/scripts/Custom/emo_teleporter.cpp#L386)) is not referenced in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.** (Dead code).
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Bypasses game world travel and progression. Overlaps with GM tools `.tele` and `.checkpoint` (e.g. `.checkpoint apply hs-10` teleporting to faction capitals).

---

### 2.4. `npc_teleport.cpp`
- **Location:** [`src/server/scripts/Custom/npc_teleport.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/npc_teleport.cpp) (172 lines)
- **What it does:**
  Implements `npc_teleport` ([L73](file:///I:/UltimateMoP/src/server/scripts/Custom/npc_teleport.cpp#L73)), a `ScriptedAI` for creature teleportation. Provides faction-specific gossip choices to teleport to capital cities (Stormwind, Ironforge, Darnassus, Exodar, Shrine of Seven Stars vs. Orgrimmar, Undercity, Thunder Bluff, Silvermoon, Shrine of Two Moons) plus Shattrath and Dalaran ([L84-L131](file:///I:/UltimateMoP/src/server/scripts/Custom/npc_teleport.cpp#L84-L131)).
- **Configuration Keys:** None.
- **Database Tables Created or Read:**
  - Reads gossip menu 900000 and broadcast text ID 48 from `world.gossip_menu_option` / `world.npc_text` ([L138, L163](file:///I:/UltimateMoP/src/server/scripts/Custom/npc_teleport.cpp#L138-L163)).
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_npc_teleport();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2146`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2146).
  - Registered: **No.** Commented out at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2167`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2167) (`//AddSC_npc_teleport();`).
- **Enabled by Default:** **No.**
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Overlaps with GM tools `.tele` and `.checkpoint` capital teleport logic.

---

### 2.5. `wow_token.cpp`
- **Location:** [`src/server/scripts/Custom/wow_token.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp) (83 lines)
- **What it does:**
  Implements `battle_pay_token` ([L43](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp#L43)), an `ItemScript` template instantiated for 4 item scripts: `wow_token_1`, `wow_token_2`, `wow_token_5`, and `wow_token_10` ([L79-L82](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp#L79-L82)). Consuming the item calls `player->AddDonateTokenCount(Coins)` ([L63](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp#L63)) to add donation coins.
- **Configuration Keys:**
  - `CONFIG_WOW_TOKEN` ([L54](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp#L54), read via `sWorld->getBoolConfig`).
- **Database Tables Created or Read:**
  - Reads `trinity_string` entries 30007–30010 ([L23-L29](file:///I:/UltimateMoP/src/server/scripts/Custom/wow_token.cpp#L23-L29)).
  - Updates donation tokens in the database via `player->SaveToDB()`.
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_wow_token();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2147`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2147).
  - Registered: **No.** Commented out at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2168`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2168) (`//AddSC_wow_token();`).
- **Enabled by Default:** **No.**
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Private server donation coin mechanic; no relevance to solo/small group gameplay.

---

### 2.6. `custom_items.cpp`
- **Location:** [`src/server/scripts/Custom/custom_items.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_items.cpp) (131 lines)
- **What it does:**
  Implements 4 `ItemScript` classes: `honor_1000` ([L5](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_items.cpp#L5)), `justice_1000` ([L35](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_items.cpp#L35)), `valor_1000` ([L65](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_items.cpp#L65)), and `conquest_1000` ([L95](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_items.cpp#L95)). When used outside combat/PvP, each consumes 1 item and grants 1000 points of the respective currency via `player->ModifyCurrency(...)`.
- **Configuration Keys:** None.
- **Database Tables Created or Read:** None directly; calls `player->SaveToDB()`.
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_custom_items();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2150`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2150).
  - Registered: **No.** Commented out at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2171`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2171) (`//AddSC_custom_items();`). Additionally, the definition function name has PascalCase mismatch: `AddSC_Custom_Items()` in `custom_items.cpp:125` vs `AddSC_custom_items()` in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.**
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Overlaps with GM currency commands (`.modify currency`).

---

### 2.7. `custom_reward.cpp`
- **Location:** [`src/server/scripts/Custom/custom_reward.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp) (32 lines)
- **What it does:**
  Implements `Played_Time_Reward` ([L4](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L4)), a `PlayerScript`. In `OnUpdate` ([L10](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L10)), tracks accumulated played time. At intervals equal to `player->ptr_Interval`, if `CONFIG_BONUS_TIME_REWARD` is enabled, awards BattlePay VP coins to the account in `auth` database using prepared statement `LOGIN_UPD_BATTLEPAY_VP_COINS` ([L20-L23](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L20-L23)).
- **Configuration Keys:**
  - `CONFIG_BONUS_TIME_REWARD` ([L16](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L16), read via `sWorld->getBoolConfig`).
  - `PlayedTimeReward.vp` ([L21](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L21), read via `sConfigMgr->GetIntDefault("PlayedTimeReward.vp", 0)`).
- **Database Tables Created or Read:**
  - Writes to `auth.battlenet_accounts` via `LOGIN_UPD_BATTLEPAY_VP_COINS` ([L20](file:///I:/UltimateMoP/src/server/scripts/Custom/custom_reward.cpp#L20)).
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_custom_reward();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2148`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2148).
  - Registered: **No.** Commented out at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2169`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2169) (`//AddSC_custom_reward();`).
- **Enabled by Default:** **No.**
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Donor reward mechanic; not relevant to gameplay.

---

### 2.8. `boost_profession.cpp`
- **Location:** [`src/server/scripts/Custom/boost_profession.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp) (318 lines)
- **What it does:**
  Implements two `ItemScript` classes: `Boost_Profession` ([L43](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L43), boosts to 600) and `Boost_Profession_Small` ([L192](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L192), boosts to 300). When clicked, presents a gossip menu listing the player's learned professions ([L53-L82](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L53-L82)). Selecting one teaches the max rank spell (e.g. `SPELL_600_*`), sets skill points, and logs to `sServiceMgr->ExecutedServices(...)` ([L172, L302](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L172-L302)).
- **Configuration Keys:** None.
- **Database Tables Created or Read:**
  - Reads gossip item DB entries from `world.gossip_menu_option` (menu 51002, [L54](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L54)).
  - Writes service records via `sServiceMgr->ExecutedServices` (`characters.services_executed`).
- **ScriptLoader Registration:**
  - Declared: Yes, `void AddSC_boost_profession();` at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2149`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2149).
  - Registered: **No.** Commented out at [`src/server/scripts/ScriptLoader/ScriptLoader.cpp:2170`](file:///I:/UltimateMoP/src/server/scripts/ScriptLoader/ScriptLoader.cpp#L2170) (`//AddSC_boost_profession();`). Note casing discrepancy: `AddSC_Boost_Profession()` at [L314](file:///I:/UltimateMoP/src/server/scripts/Custom/boost_profession.cpp#L314) vs `AddSC_boost_profession()` in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.**
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Overlaps with GM profession commands (`.setskill`) and rate configurations in `worldserver.conf` (`SkillGain.Crafting`, etc.).

---

### 2.9. `protected_zone.cpp`
- **Location:** [`src/server/scripts/Custom/protected_zone.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/protected_zone.cpp) (32 lines)
- **What it does:**
  Implements `protected_zone` ([L8](file:///I:/UltimateMoP/src/server/scripts/Custom/protected_zone.cpp#L8)), a `PlayerScript`. The intent was to restrict non-GM characters entering Timeless Isle (6757), Isle of Giants (6661), or Isle of Thunder (6507) by teleporting them to Dalaran Crater. However, the entire body of `OnUpdateZone` ([L14-L26](file:///I:/UltimateMoP/src/server/scripts/Custom/protected_zone.cpp#L14-L26)) is commented out.
- **Configuration Keys:** None.
- **Database Tables Created or Read:** None.
- **ScriptLoader Registration:**
  - Declared: **No.**
  - Registered: **No.** `AddSC_protected_zone()` ([L29](file:///I:/UltimateMoP/src/server/scripts/Custom/protected_zone.cpp#L29)) is not referenced in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.** (Completely empty/dead code).
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - Overlaps with content gating and Heartseed story flags (`reached_pandaria`, etc.).

---

### 2.10. `PlayerStartScript.cpp`
- **Location:** [`src/server/scripts/Custom/PlayerStartScript.cpp`](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp) (577 lines)
- **What it does:**
  Implements `player_start_spells` ([L527](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L527)), a `PlayerScript` on `OnLogin`. Contains hardcoded arrays of all glyph spell IDs for all 11 classes ([L24-L520](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L24-L520)). On player login, it iterates and teaches every glyph spell to the player ([L537-L548](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L537-L548)), teaches all riding ranks (Apprentice through Master Riding, Flight Master's License, Cold Weather Flying, Wisdom of Four Winds, Cloud Serpent) ([L550-L559](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L550-L559)), and gives 1 Reins of the White Polar Bear, 4 Illusionary Bags, and 200 Tomes of Clear Mind ([L561-L563](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L561-L563)). Note: the `if (player->HasAtLoginFlag(AT_LOGIN_FIRST))` check is commented out ([L533, L570](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L533-L570)), meaning if enabled, it would run on every single login.
- **Configuration Keys:** None.
- **Database Tables Created or Read:** None directly; calls `player->SaveToDB()`.
- **ScriptLoader Registration:**
  - Declared: **No.**
  - Registered: **No.** `AddSC_player_start_spells()` ([L574](file:///I:/UltimateMoP/src/server/scripts/Custom/PlayerStartScript.cpp#L574)) is not registered in `ScriptLoader.cpp`.
- **Enabled by Default:** **No.** (Dead code).
- **Overlap with SoloConfig, Starting Kit (T-011), and GM Tools:**
  - **Massive overlap with Starting Kit (T-011):** T-011 was specifically created to provide clean starting kits (`.kit bags`, `.kit mounts`, starting 36-slot bags) without bypassing progression.
  - **Overlap with Respec (T-040):** `Respec.FreeReagents = 1` removes the reagent requirement for changing talents and glyphs, making 200 Tomes of Clear Mind redundant.
  - **Overlap with Checkpoints (`GMTools/Checkpoints`):** Checkpoints like `.checkpoint apply hs-10` manage riding skills and gear systematically based on level bracket. Giving end-game Pandaria flying (`Wisdom of Four Winds`) and Cloud Serpent riding at level 1 breaks the intentional progression design in `docs/DESIGN.md`.

---

## 3. Conclusions and Recommended Dispositions

1. **Delete Dead Unlinked Scripts:**
   `emo_stone.cpp`, `emo_teleporter.cpp`, `PlayerStartScript.cpp`, and `protected_zone.cpp` are completely unlinked in `ScriptLoader.cpp`, contain hardcoded IDs, and contradict the clean design of UltimateMoP (Starting Kit T-011, Respec T-040, GM Tools T-039/T-048).
2. **Remove Commented Out Donor / Web Shop Scripts:**
   `boost_profession.cpp`, `custom_items.cpp`, `custom_reward.cpp`, `npc_teleport.cpp`, and `wow_token.cpp` are commented out in `ScriptLoader.cpp` and belong to an inherited private server donor economy. Their loading declarations should be pruned from `ScriptLoader.cpp` and the files removed in a future cleanup pass.
3. **Deprecate `solocraft.cpp` in favor of `SoloConfig`:**
   `solocraft.cpp` is the only active script registered in `ScriptLoader.cpp`, but disabled by default (`Solocraft.Enable = 0`). Because `SoloConfig` (T-043) is the decided solo/small group scaling engine that scales instances rather than inflating player stats, `solocraft.cpp` can safely be unhooked and removed.

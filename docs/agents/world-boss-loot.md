# World Boss Loot Audit (T-072)

**Audit Date:** 2026-11-01  
**Scope:** All MoP world bosses and Timeless Isle rare elites/rares in `world_gemini`.

---

## 1. Executive Summary

An audit was conducted across all MoP world bosses and Timeless Isle rare elites to verify loot template integrity (`creature_loot_template`, `reference_loot_template`, and associated container/gameobject tables).

### Findings Overview
- **Complete & Intact:**
  - **Nalak (69099)**: 252 items in `creature_loot_template` (Tier 15 gloves/legs, Season 13/14 Tyrannical PvP gear, plus mount `95057` Reins of the Thundering Cobalt Cloud Serpent at 15%).
  - **The Four Celestials (71952 Chi-Ji, 71953 Yu'lon, 71954 Niuzao, 71955 Xuen)**: Each has 256 items in `creature_loot_template` (Tier 16 gloves/legs, Season 14/15 Prideful/Grievous PvP gear).
  - **Ordos (72057)**: 59 items in `creature_loot_template` (Warforged ilvl 559 equipment).
  - **Timeless Isle Rares (31 active creatures)**: All 31 active rares (Huolon, Cinderfall, Garnia, Imperial Python, Jakur of Ordon, Champion of the Black Flame, Evermaw, Dread Ship Vazuvius, Golganarr, Tsavo'ka, Bufo, etc.) possess full loot tables (9 to 16 rows each).
- **Bosses with Missing Loot Data (Mount Only):**
  - **Sha of Anger (60491)**
  - **Galleon (62346)**
  - **Oondasta (69161)**

These three bosses currently have only the 10% mount drop added in PR #72 (T-064). A thorough search of `world_gemini` confirmed that **no unlinked or orphaned `reference_loot_template` tables, container tables, or gameobject loot tables exist** for their remaining equipment or quest drops.

Per the T-072 design constraints, item lists and drop chances must not be invented without source data. The three bosses lacking equipment loot are detailed below.

---

## 2. Bosses with Missing Loot Data

### 1. Sha of Anger (Creature Entry: `60491`, `lootid: 60491`)
- **Zone:** Kun-Lai Summit
- **Current Loot (`creature_loot_template`):**
  - Item `87771`: *Reins of the Heavenly Onyx Cloud Serpent* (10% drop chance, group 0).
- **What's Missing:**
  - **Quest Starter:** Item `89317` (*Claw of Anger*), which initiates the quest *Remnants of Anger* (awards ilvl 476 boots). In retail MoP, this was a 100% personal loot quest starter on first defeat.
  - **Gold Cache:** Item `90839` (*Cache of Sha-Touched Gold*), which was the consolation drop awarded when bonus rolls failed.
  - **Raid Tier 14 Equipment (ilvl 496):** Tier 14 hands and legs for all classes (e.g., *Gauntlets of the Lost Catacomb*, *Leggings of the Shadow Gaze*, *Firebird's Gloves*, etc.).
  - **PvP Season 12 Equipment (ilvl 483):** Malevolent Gladiator belts, bracers, rings, cloaks, and necks.
- **Where Data Could Come From:**
  1. Upstream commit archive: The upstream file is present in the repository at sql/old/Merged to World DB 30_12_2023/2022_05_16_01_add loot for some world bosses.sql. However, its Sha of Anger rows consist largely of vanilla-era junk items (8480, 8500, 8502, and others) at 0.8% drop chances. This file is not applied and should not be applied.
  2. TrinityCore 5.4.8 world database dumps / sniffs.
  3. Wowhead 5.4.8 historical snapshot of Sha of Anger drop tables.

---

### 2. Galleon / Salyis's Warband (Creature Entry: `62346`, `lootid: 62346`)
- **Zone:** Valley of the Four Winds
- **Current Loot (`creature_loot_template`):**
  - Item `89783`: *Son of Galleon's Saddle* (10% drop chance, group 0).
- **What's Missing:**
  - **Raid Equipment (ilvl 496):** Non-set raid-equivalent armor and jewelry (e.g., *Galleon's Cleft Eyetooth*, waist pieces, boots, cloaks, rings).
  - **PvP Season 12 Equipment (ilvl 483):** Malevolent Gladiator non-set gear (bracers, belts, boots).
- **Where Data Could Come From:**
  1. Upstream commit `c6ada96d` (`sql/updates/world/2022_05_16_01_add loot for some world bosses.sql`), which drafted Galleon's standard drops before being superseded.
  2. TrinityCore MoP 5.4.8 database dumps / retail packet captures.
  3. Wowhead 5.4.8 historical loot table for Galleon.

---

### 3. Oondasta (Creature Entry: `69161`, `lootid: 69161`)
- **Zone:** Isle of Giants
- **Current Loot (`creature_loot_template`):**
  - Item `94228`: *Reins of the Cobalt Primordial Direhorn* (10% drop chance, group 0).
- **What's Missing:**
  - **Raid Equipment (ilvl 522):** Throne of Thunder Normal equivalent raid loot across all slots (e.g., *Belt of the Prehistoric Marauder*, *Girdle of the Grotesque Tooth*, weapons, shields, and trinkets).
  - **Quest / Miscellaneous:** Item `95164` (*Eye of Oondasta*).
- **Where Data Could Come From:**
  1. Upstream commit `c6ada96d` (`sql/updates/world/2022_05_16_01_add loot for some world bosses.sql`), which included an item list for Oondasta.
  2. TrinityCore MoP 5.4.8 reference loot extracts or retail 5.4.8 sniffs.
  3. Wowhead 5.4.8 drop tables for Oondasta.

---

## 3. Recommended Next Steps

1. **Extract Upstream Item Lists:**
   Source authentic retail 5.4.8 item lists for 60491, 62346, and 69161 from verified sniffs/Wowhead (the legacy upstream file contains invalid vanilla items).
2. **Structure via Reference Tables:**
   Rather than hundreds of flat rows in `creature_loot_template`, organize Sha of Anger, Galleon, and Oondasta drops into dedicated `reference_loot_template` pools (e.g., T14 Armor, S12 PvP, ilvl 522 Weapons/Armor), mirroring the architecture used by Nalak and the Celestials.
3. **Quest Item Hook:**
   Add *Claw of Anger* (`89317`) as a quest-flagged drop in `creature_loot_template` with `QuestChance = 100` for Sha of Anger.

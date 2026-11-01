# Heartseed Weapon Glow Visual Candidates

This document catalogs weapon-glow enchantments discovered via `tools/content-search` (`SpellItemEnchantment.dbc` -> `ItemVisuals.dbc` -> `ItemVisualEffects.dbc`) for the four Celestial Heartseed paths (**T-058**).

> **Design Reference:** Per `docs/DESIGN.md` (The Heartseed) and `sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql` (`heartseed_glow` table), weapon glows are cosmetic client display auras applied to the visible enchant slot. They progress through stages 0 to 5 as the Heartseed matures:
> - **Stage 0:** Seed (Lv 1+)
> - **Stage 1:** Sprout (Lv 10+)
> - **Stage 2:** Sapling (Lv 30+)
> - **Stage 3:** Bloom (Lv 60+)
> - **Stage 4:** Full Bloom (Lv 85+)
> - **Stage 5:** Celestial Weapon (Lv 90)

Data only; Claude picks the final assignments for `heartseed_glow`.

---

## 1. Path 1: Xuen (The White Tiger)
**Visual Themes:** Lightning crackle, electric sparks, swirling wind currents, brilliant white/cyan tiger energy.

### Candidate Visual Enchants

| Enchant ID | Name / Description | ItemVisual ID | Visual Model(s) | Suggested Stage | Visual Notes |
|---|---|---|---|---|---|
| **255** | `+$k1 Spirit` | 103 | `SPELLS\ENCHANTMENTS\WHITEGLOW_LOW.MDX` | Stage 0 (Seed) | Subtle white mystical shimmer, clean starter glow. |
| **283** | `Windfury` | 81 | `SPELLS\ENCHANTMENTS\SHAMAN_WIND.MDX` | Stage 1 (Sprout) | Whirling white gusts of wind wrapping the blade. |
| **3093** | `+$37362s1 AP vs Undead/Demons` | 104 | `SPELLS\ENCHANTMENTS\WHITEGLOW_HIGH.MDX` | Stage 2 (Sapling) | Intense radiant white tiger light. |
| **4441** | `Windsong` | 252 | `SPELLS\ENCHANTMENTS\SOULFROSTGLOW_HIGH.MDX`<br>`SPELLS\ENCHANTMENTS\YELLOWGLOW_LOW.MDX` | Stage 3 (Bloom) | Crackling cyan-white air and frost lightning arcs. |
| **2673** | `Mongoose` | 160 | `SPELLS\ENCHANTMENTS\MONGOOSEGLOW_HIGH.MDX` | Stage 4 (Full Bloom) | High-voltage electric lightning dancing along the weapon. |
| **5029** | `Crackling Lightning` | 246 | `SPELLS\MONK_CRACKLINGLIGHTNING_PRECAST_BLUE.MDX` | Stage 5 (Celestial) | Full Monk blue-white lightning aura, true Celestial Tiger manifestation. |

*Alternative candidate:*
- **4083** (`Hurricane`, Visual 187, `SPELLS\ENCHANTMENTS\BLUEFLAME_LOW.MDX` + `SPELLS\ENCHANTMENTS\MONGOOSEGLOW_HIGH.MDX`): Swirling air storm with active lightning discharges.

---

## 2. Path 2: Chi-Ji (The Red Crane)
**Visual Themes:** Red flame, flickering ember, golden sunfire, blazing crane vitality.

### Candidate Visual Enchants

| Enchant ID | Name / Description | ItemVisual ID | Visual Model(s) | Suggested Stage | Visual Notes |
|---|---|---|---|---|---|
| **249** | `+$7784s1 Beastslaying` | 31 | `SPELLS\ENCHANTMENTS\REDGLOW_LOW.MDX` | Stage 0 (Seed) | Soft warm crimson ember glow at the hilt/edge. |
| **803** | `Fiery Weapon` | 25 | `SPELLS\ENCHANTMENTS\REDFLAME_LOW.MDX` | Stage 1 (Sprout) | Flickering red flames licking the length of the blade. |
| **3846** | `+$k1 Spell Power` | 102 | `SPELLS\ENCHANTMENTS\YELLOWGLOW_HIGH.MDX` | Stage 2 (Sapling) | Brilliant golden radiant fire of hope and dawn. |
| **5** | `Flametongue` | 32 | `SPELLS\ENCHANTMENTS\SHAMAN_FIRE.MDX` | Stage 3 (Bloom) | Blazing crimson elemental flame enveloping the weapon. |
| **2671** | `Sunfire` | 158 | `SPELLS\ENCHANTMENTS\SUNFIREGLOW_HIGH.MDX` | Stage 4 (Full Bloom) | Blinding golden-orange solar aura resembling crane feathers. |
| **4097** | `Power Torrent` | 196 | `SPELLS\ENCHANTMENTS\FIRE_HIGH.MDX` | Stage 5 (Celestial) | Massive roaring vortex of golden-red celestial flame. |

*Alternative candidates:*
- **3789** (`Berserking`, Visual 156, `SPELLS\ENCHANTMENTS\REDGLOW_HIGH.MDX` + `SPELLS\ENCHANTMENTS\SAVAGERYGLOW_HIGH.MDX`): Intense dark crimson flame.
- **4443** (`Elemental Force`, Visual 236, `SUNFIREGLOW_HIGH` + `REDFLAME_LOW` + `PURPLEGLOW`): Prismatic fiery radiance.

---

## 3. Path 3: Niuzao (The Black Ox)
**Visual Themes:** Earthen stone, jagged rock shards, amber fortitude, unbreakable obsidian defense.

### Candidate Visual Enchants

| Enchant ID | Name / Description | ItemVisual ID | Visual Model(s) | Suggested Stage | Visual Notes |
|---|---|---|---|---|---|
| **723** | `+$k1 Intellect` | 29 | `SPELLS\ENCHANTMENTS\YELLOWGLOW_LOW.MDX` | Stage 0 (Seed) | Warm amber-stone sheen, subtle mineral glow. |
| **2669** | `+$k1 Spell Power` | 159 | `SPELLS\ENCHANTMENTS\BATTLEMASTERGLOW_HIGH.MDX` | Stage 1 (Sprout) | Solid bronze-amber battlemaster glow of resilience. |
| **3847** | `Stoneskin Gargoyle` | 159 | `SPELLS\ENCHANTMENTS\BATTLEMASTERGLOW_HIGH.MDX` | Stage 2 (Sapling) | Heavy metallic stone shield appearance. |
| **3021** | `Rockbiter` | 61 | `SPELLS\ENCHANTMENTS\SHAMAN_ROCK.MDX` | Stage 3 (Bloom) | Spinning jagged earth fragments and boulder spikes. |
| **4445** | `Colossus` | 239 | `SPELLS\ENCHANTMENTS\BLUEFLAME_LOW.MDX`<br>`SPELLS\ENCHANTMENTS\MONGOOSEGLOW_HIGH.MDX` | Stage 4 (Full Bloom) | Towering guardian ward with pulsing defensive power. |
| **4099** | `Landslide` | 195 | `SPELLS\ENCHANTMENTS\EARTHEN_HIGH.MDX`<br>`SPELLS\ENCHANTMENTS\SUNFIREGLOW_HIGH.MDX`<br>`SPELLS\ENCHANTMENTS\YELLOWFLAME_LOW.MDX` | Stage 5 (Celestial) | Cracking earthen crust with glowing molten rock veins. |

*Alternative candidate:*
- **3869** (`Blade Ward`, Visual 177, `BLUEFLAME_LOW` + `BLUEGLOW_LOW` + `MONGOOSEGLOW_HIGH`): Protective parry barrier visual.

---

## 4. Path 4: Yu'lon (The Jade Serpent)
**Visual Themes:** Jade serpent mist, flowing crystal water, emerald life energy, swirling river currents.

### Candidate Visual Enchants

| Enchant ID | Name / Description | ItemVisual ID | Visual Model(s) | Suggested Stage | Visual Notes |
|---|---|---|---|---|---|
| **1103** | `+$k1 Agility` | 125 | `SPELLS\ENCHANTMENTS\GREENGLOW_LOW.MDX` | Stage 0 (Seed) | Delicate pale green jade luminescence. |
| **2668** | `+$k1 Strength` | 106 | `SPELLS\ENCHANTMENTS\GREENGLOW_HIGH.MDX` | Stage 1 (Sprout) | Vibrant pure emerald glow. |
| **4066** | `Mending` | 194 | `SPELLS\ENCHANTMENTS\NATURE_HIGH.MDX` | Stage 2 (Sapling) | Swirling soothing jade mist and nature motes. |
| **4446** | `River's Song` | 237 | `SPELLS\ENCHANTMENTS\BLUEFLAME_LOW.MDX`<br>`SPELLS\FLOWINGWATER_HIGH.MDX` | Stage 3 (Bloom) | Flowing crystalline green-blue water wrapping the weapon. |
| **4227** | `+$k1 Agility` | 201 | `SPELLS\ENCHANTMENTS\NATURE_HIGH.MDX`<br>`SPELLS\ENCHANTMENTS\GREENGLOW_HIGH.MDX`<br>`SPELLS\ENCHANTMENTS\GREENFLAME_LOW.MDX` | Stage 4 (Full Bloom) | Layered emerald flames and serpent nature aura. |
| **4442** | `Jade Spirit` | 238 | `SPELLS\JADESPIRIT_HIGH.MDX` | Stage 5 (Celestial) | Signature Pandaria Jade Spirit serpent dragon glow. |

*Alternative candidate:*
- **5035** (`Spirit of Conquest`, Visual 257, `SPELLS\WEAPONENCHANT_PVPPANDARIAS2.MDX`): Pandaria seasonal PvP serpent mist effect.

---

## 5. Candidate Mapping Summary Matrix

| Stage | Path 1: Xuen | Path 2: Chi-Ji | Path 3: Niuzao | Path 4: Yu'lon |
|---|---|---|---|---|
| **0 (Seed)** | `255` (White Glow Low) | `249` (Red Glow Low) | `723` (Amber Glow Low) | `1103` (Green Glow Low) |
| **1 (Sprout)** | `283` (Windfury Wind) | `803` (Fiery Weapon) | `2669` (Battlemaster Amber) | `2668` (Green Glow High) |
| **2 (Sapling)** | `3093` (White Glow High) | `3846` (Yellow Glow High) | `3847` (Stoneskin Fortitude) | `4066` (Mending Mist) |
| **3 (Bloom)** | `4441` (Windsong Electric) | `5` (Flametongue Blaze) | `3021` (Rockbiter Earthen) | `4446` (River's Song Water) |
| **4 (Full Bloom)** | `2673` (Mongoose Lightning) | `2671` (Sunfire Solar) | `4445` (Colossus Guardian) | `4227` (Emerald Flame/Nature) |
| **5 (Celestial)** | `5029` (Crackling Lightning) | `4097` (Power Torrent Fire) | `4099` (Landslide Earthen) | `4442` (Jade Spirit) |

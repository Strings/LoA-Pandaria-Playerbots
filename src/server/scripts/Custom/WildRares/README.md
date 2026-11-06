# Wild rares (T-100)

A second, lesser tier of rares (DESIGN.md, Rares and discovery). Ordinary hostile open-world creatures sometimes
spawn as a *wild rare*: one affix, sometimes two, from the shared affix module (`Custom/Affixes/`) at reduced
intensity, and more health. Fixed rares (`Custom/Rares/`) stay the big-reward tier.

- **Eligible** (`WildRareRules.h`, `Judge`): a database spawn of normal rank, alive, attackable, hostile (or neutral
  with `WildRares.IncludeNeutral`), outside instances and cities (capital, sanctuary and city area flags). Never NPCs
  (any NPC flag or a gossip menu), pets, summons, guardians, critters, triggers, elites or rares, and never an entry
  in `rare_wild_denylist` (world DB, `.reload rare_wild_denylist`).
- **Roll:** on every spawn and respawn, `WildRares.ChancePct` (2%), unless the zone already has `WildRares.ZoneCap`
  live wild rares or the spawn point turned wild within `WildRares.SpawnCooldownSeconds` (custom clock).
- **Strength:** affix count from `WildRares.AffixCountWeights` (one or two), rolled from affixes with host bit 8
  (`HOST_WILD_RARE`), applied at `WildRares.AffixIntensity` (0.55). Health +50 to 100%.
- **Death reset:** when a human player dies (killed by a creature or a player) while a wild rare is fighting them,
  killed them, or stands within `WildRares.DeathReset.Radius`, its affixes, extra health, marker and star go and it is
  an ordinary creature again. Bot deaths count only with `WildRares.DeathReset.BotsCount`.
- **Rewards on the kill:** XP of 10% of a level plus 5% per affix, capped at 20% (`WildRares.Xp.*`), for the killer
  and group members in range; `WildRares.ExtraLootRolls` extra rolls of its loot table; `WildRares.EssenceChancePct`
  for a random Heartseed essence. The fixed-rare payout skips it (the affix module records which host applied them).
- **Presentation:** the affix buffs, the *Wild Rare* marker aura (318100, client-patched clone; its script frees the
  zone slot and star on death), a spawn emote (`WildRares.SpawnEmote`) and a minimap star (`WildRares.Vignette.Id`).

## Interfaces it adds elsewhere

- **Affix intensity:** `sAffixMgr->Apply(creature, ids, intensity, host)`; `Intensity(guid)` and `Host(guid)`.
  Amounts scale with intensity; affixes with no amount (Teleporting, Sha-touched) fire less often instead; add counts
  scale but never drop below one (`AffixRules.h`).
- **Vignettes from code:** `CreatureHooks::RegisterVignetteResolver` gives a creature a minimap star its template
  lacks; `VignetteMgr.cpp` asks it when a player starts seeing a creature. Wild rares use it, and so do open-world
  rares from before MoP (no vignette in their data), through `Rares.Vignette.Id`.

## GM commands (`.wildrare`, permission 98001)

| Command | Does |
|---|---|
| `.wildrare spawn [entry] [affix ...]` | turns the selected creature wild (or, with an entry, a temporary hostile one at you), with those affixes or rolled ones |
| `.wildrare explain` | the selected creature or nearest wild rare: wild state, eligibility and why, zone count, spawn-point cooldown |
| `.wildrare reset` | wears it off, as a player's death nearby would |
| `.wildrare stats` | live count per zone, chance, rolls, turns, misses, cap and cooldown hits, kills, resets |
| `.wildrare chance [pct\|reset]` | shows or overrides `WildRares.ChancePct` until restart |

Unit tests: `tests/wild_rares` (eligibility, roll, cap, cooldown, rewards) and `tests/affixes` (intensity).
Regression scenario: `tests/regression/scenarios/wild-rares.json`.

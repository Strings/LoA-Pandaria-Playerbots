# Affixed rares (T-089)

Open-world rares and rare elites roll 0-3 affixes from the shared affix module (`Custom/Affixes/`) when they
spawn or respawn, show them as buffs, and pay for them.

- **Bots wait for a human.** An affixed rare carries *Unclaimed* (318000): bots don't target it and it doesn't
  aggro bots (`CreatureHooks` fight filter, read by `Creature::_IsTargetAcceptable` and the playerbots attacker and
  grind values). The first damage from a human or their pet claims it; an evade makes it unclaimed again.
- **Rewards:** extra kill XP as a share of a level (`Rares.Xp.*`, plus each affix's `xp_bonus_pct`) for the killer
  and group members in range, extra rolls of the loot table and more gold per affix (`loot_rolls`). Heartseed Growth
  already counts rares (T-084).
- **Core hooks:** `src/server/game/Scripting/CreatureHooks.h` (spawned, respawned, evaded, removed; fight filter).
- **Config:** `Rares.*` in `worldserver.conf.dist`.

## GM commands (`.rare`, permission 98000)

| Command | Does |
|---|---|
| `.rare near` | the selected or nearest rare within 50 yd: affixes, claim state, health |
| `.rare go [n]` | teleport to the nth closest living rare spawn on your map (default 1; `.rare go 2` for the next) |
| `.rare spawn <entry> [affix ...]` | a temporary hostile rare at you with those affixes (or one rolled) |
| `.rare reroll [count]` | rolls new affixes on the selected or nearest rare |
| `.rare set [affix ...]` | sets exact affixes (none clears them) |
| `.rare claim` | claims it as if you struck it |
| `.rare affixes` | lists the affix table |

Affix names match by prefix (`mol`, `jugg`).

## Not yet

Signature drops per rare, minimap stars (vignettes), bots calling rares out in chat, borrowed boss abilities and
legendary rares.

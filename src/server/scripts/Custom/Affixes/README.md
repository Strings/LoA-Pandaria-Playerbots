# Affixes (shared module)

Diablo-style modifiers for creatures. Built once for every system that needs them (DESIGN.md, Rares and
discovery): affixed rares now (`Custom/Rares/`), the corruption dial and Brawler mutators later.

**Data:** `affix_template` (world DB), reloadable with `.reload affix_template`. Each row is one affix: its kind
(the mechanic in code), the visible marker aura (`aura_spell`, client-patched clones 318001+), the client spell it
casts, its period, an amount as a share of health, XP and loot bonuses, a roll weight and which hosts may roll it.

**Interface (`AffixMgr.h`, `sAffixMgr`):** `Roll(host, count)`, `Apply(creature, ids)` (remembers them),
`Reapply(creature)` after an evade, `Clear`, `Forget`, `Get(guid)`, `Describe(ids)`. Pure roll and reward math is in
`AffixRules.h`, unit-tested in `tests/affixes`.

**How an affix runs:** the marker aura is the affix. Its script (`spell_affix_marker`) sets the aura's period from
the template and runs the mechanic on each tick while the creature fights; applying and removing it handles lasting
changes (Juggernaut) and death effects (Molten). Vampiric heals in `affix_unitscript::OnDamage`.

| Kind | Affix | Mechanic |
|---|---|---|
| 1 | Molten | Fire Nova (11969) around it, `amount_pct` of the victim's health; explodes on death for double |
| 2 | Frozen | Frost Nova (12748): damage and root |
| 3 | Vampiric | heals for `amount_pct` of the damage it deals |
| 4 | Shielded | Power Word: Shield (17) on itself for `amount_pct` of its health |
| 5 | Teleporting | blinks behind its victim |
| 6 | Juggernaut | +`amount_pct`% health, 25% bigger, 25% slower, immune to root, snare, stun |
| 7 | Summoner | `summon_count` of `summon_entry` (Lesser Sha), each `amount_pct` of its health |
| 8 | Sha-touched | Psychic Scream (13704): fears those close |

A new kind needs code (`Affixes::Kind` and `Pulse`); a new affix of an existing kind is a row plus a marker clone
in `tools/clientpatch/edits.json` (`build.py sql` regenerates the server spell SQL).

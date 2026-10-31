# T-021: Heartseed traits, core passive and first major choice

**Agent:** claude
**Branch:** claude/T-021-heartseed-traits (stacked on T-024, PR #11, which is stacked on T-020, PR #9)
**Task status:** review

## What changed
- **Traits as data rows:** world table `heartseed_trait` (path, kind, major tier or minor rank, passive aura
  spell, `base_amount + amount_per_ilvl * main-hand item level`). Eight placeholder traits: one minor and
  one level 10 major per Celestial path, on server-side passive auras 310000-310007.
- **Majors** go into the active specialization's loadout through `HeartseedMgr::ChooseMajor` (level check,
  tier check, kind check; GM commands skip the level check). **Minors** fill in by rank on every path the
  loadout has a major in. A spec with no loadout starts from the one the character just left, so picking a
  spec after the level 10 major keeps it. `GetDominantPath` now counts majors, so the glow can follow the path.
- **Core passive:** bonus weapon damage (`Heartseed.Core.WeaponDamagePct` of the main-hand's average damage,
  as a flat physical damage aura, spell 310008) and a proc (`Heartseed.Proc.*`: chance, share of weapon
  damage, cooldown, spell used for the combat log). Rolled on any damage the character deals, dealt one
  update later.
- **Refresh:** traits are diffed and reapplied on login, level change, main-hand change, spec change and
  every state change. Passive auras are not saved with the character and not sent to the client.
- **Core hooks** (small, in `ScriptMgr` and `Player.cpp`/`SkillHandler.cpp`):
  `PlayerScript::OnVisibleItemChanged` (from `Player::SetVisibleItemSlot`) and
  `PlayerScript::OnSpecializationChanged` (dual-spec swap, choosing a spec, both talent resets).
- **GM commands:** `.heartseed traits`, `.heartseed major <10|25|...> <trait|0>`, `.heartseed reload`;
  `.heartseed info` shows the loadout, applied auras with amounts, proc damage and procs since login.
- Level-up message when a major tier opens and is unchosen (the choice itself comes with T-023's gossip).
- Headless client: `set-spec TAB` verb (CMSG_SET_PRIMARY_TALENT_TREE).

## How it was tested
Claude test server, headless client, character `Seedtest` (human warrior 30). SQL applied twice to
`world_claude`.
- `.heartseed traits` lists all 8; fresh grant applies only the core aura.
- `major 10 3` (a minor), `major 25 2` (wrong tier), `major 10 99` (unknown) all refused; `major 10 2`
  applies Xuen major + Xuen minor (`.list auras` shows 310000/310001 with amounts 3 and 1), dominant path Xuen.
- `.botscenario equip blue` (new main-hand): core aura appears at 6, proc damage 4 -> 57.
- `major 10 6` swaps to the Niuzao pair (5 armor %, -2 damage taken %), dominant path Niuzao.
- Relog: auras reapplied from the saved loadout.
- `.reset talents` then `set-spec 1`: spec 72 got a copy of the loadout (2 loadouts saved); `major 10 8`
  changed only spec 72's.
- Proc (chance 100 locally): `.damage 1` on a 42 HP Blackrock Battle Worg -> proc counted, worg at 0 HP.
  Zero-damage hits (training dummies cap damage) do not roll.
- `revoke`: all Heartseed auras removed (`.list auras`), rows deleted.
- Built with no new warnings in touched files.

## Decisions made
- **Minors fill in on paths with at least one major** (path 0 minors on every Heartseed). DESIGN.md says
  minors "fill in automatically" without saying which; this is one rule in `HeartseedRules.h::MinorActive`.
  The user left the call to me; kept it: committing to a path is what unlocks its minors, so each major
  choice shapes the whole jewel and the four paths stay distinct, while path 0 minors keep rank growth
  rewarding for everyone. Changing it later is one function.
- **Core bonus is a share of weapon damage** (5%) rather than a flat number per item level; weapon damage
  already grows with item level and stays proportionate at every level. Trait amounts use item level.
- **The proc deals its damage directly** (`Unit::SpellNonMeleeDamageLog`) instead of casting: in this core
  casts check range and facing even when triggered, which dropped procs. The configured spell only names it
  in the combat log. The default 16614 (Lightning Strike) is a placeholder.
- **Spell block moved to 310000-310999** (and the next system to 311000): upstream `spell_dbc` already uses
  300000-300109. `docs/ID_RANGES.md` updated.
- Majors are keyed by specialization id, as the T-020 schema already was; new specs copy the last loadout.
- Trait effects (run speed, crit, healing, armor, damage taken, mana regen, spell power) are placeholders.

## For the other agent
- New hooks in `ScriptMgr.h` (rebuilds most scripts): `OnVisibleItemChanged`, `OnSpecializationChanged`.
- Apply `sql/updates/world/2026_10_31_claude_03_heartseed_trait.sql` to your world DB; the server reads
  `spell_dbc` at startup only, so restart after applying.
- New `worldserver.conf` keys: `Heartseed.Core.WeaponDamagePct`, `Heartseed.Proc.SpellId`, `.Chance`,
  `.WeaponDamagePct`, `.Cooldown` (defaults apply when missing).
- Pre-existing core crash found while testing: `.npc add 31144` crashes in `Creature::Create`
  (`GetPhasesForGroup` returns null for the creature's phase group, `Creature.cpp:915`). Not touched here.
- Headless: SOAP verbs need `MOP_SOAP_USER`/`MOP_SOAP_PASSWORD`; `target npc` does not resolve, pass the raw
  guid from `npcguid`.

## Next
- T-023 shrine gossip calls `ChooseMajor`; T-022 feeding and rank cap; real trait design replaces the rows.
- `.reload heartseed` once the shared reload command exists (T-038); `.heartseed reload` covers it until then.

## Map additions
- None (all in `src/server/scripts/Custom/Heartseed/`).

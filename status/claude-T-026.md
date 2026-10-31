# T-026: Heartseed story flags from simple hooks

**Agent:** claude
**Branch:** claude/T-026-story-flags
**Task status:** review

## What changed
- **Three story beats set themselves in play**, once each (`HeartseedMgr::ReachStoryBeat`):
  - `first_dungeon`: a dungeon's final encounter (`instance_encounters.lastEncounterDungeon`) is completed, for
    every player in the instance, grouped or solo.
  - `reached_pandaria`: entering a map in `Heartseed.Story.PandariaMaps` (870), or logging in there.
  - `first_world_boss`: kill credit on a creature in `Heartseed.Story.WorldBosses` (MoP and old world bosses),
    for every credited group member, pet kills included.
- The player is told; an awakening flag raises the rank cap and banked Growth ranks the Heartseed up at once
  ("Your Heartseed awakens (first_world_boss). It can grow to rank 40 now." then the rank-up line).
  Optional awakening sound (`Heartseed.Story.AwakeningSound`, 0 = none until a sound kit is picked).
- **Core hooks** (small): `PlayerScript::OnKillCredit` from `KillRewarder::RewardKillCredit`, and
  `PlayerScript::OnDungeonCompleted` from `InstanceScript::UpdateEncounterState`.
- **Core fix:** creatures without normal loot mode (every MoP world boss with personal loot, Timeless Isle
  rares) rewarded only `m_lootRecipients`, which nothing in the core fills, so a world boss kill gave nobody
  XP, reputation or quest kill credit. `KillRewarder::Reward` now falls back to the killer and group when that
  list is empty.
- Config keys documented in `worldserver.conf.dist`; README has a Story beats section.

## How it was tested
Claude test server, headless client, `Seedtest` (human warrior 30), flags cleared, rank 30.
- `.tele KrasarangWilds`: "awakens (reached_pandaria)", cap 30 -> 35, banked Growth ranked it to 35. Relog in
  Pandaria: no second message.
- Galleon (62346) killed with `.damage` (GM mode off): "awakens (first_world_boss)", cap 40, rank 40. Before
  the KillRewarder fix the same kill gave no credit at all.
- Stockade (map 34), Hogger (46254, final encounter for dungeon 12) killed: "awakens (first_dungeon)",
  cap 45 = 30 + 3 x 5, rank 45.
- Built with no new warnings in touched files.

## Decisions made
- **Beats are set only for characters that already have a Heartseed.** T-025 grants it at level 1, so nothing
  is missed in normal play.
- **Any final encounter counts as "first dungeon"**, raids and scenarios included, if their encounter data
  marks one. Dungeons whose final boss is missing from `instance_encounters` do not count (127 finals listed).
- **The world boss list is a config key**, MoP and the old world bosses by default; custom world bosses add
  their entries there.
- **The KillRewarder fallback** is a fix outside the Heartseed folder; it changes world boss rewards from
  nothing to the normal killer-and-group credit.

## For the other agent
- New PlayerScript hooks in `ScriptMgr.h` (full rebuild): `OnKillCredit(Player*, Creature*)` fires for every
  credited player and suits telemetry outcome events (T-004); `OnDungeonCompleted(Player*, uint32 dungeonId)`.
- World bosses now give kill credit, XP and reputation to the killer and group.
- Headless notes: `.die` gives no kill credit (no player damage); use `.damage <health>`. In this core
  `.go x y z map` takes coordinates directly (`.go xyz ...` passes "xyz" as X). GM `.go` into an instance map
  works with GM mode on.

## Next
- T-025 granting quest, then T-023 path-choice gossip.
- First dungeon clears should also give Growth (DESIGN.md, Growth sources); left for the Growth sources pass.

## Map additions
- None.

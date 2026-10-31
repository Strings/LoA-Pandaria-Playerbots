# Project status

Last folded: 2026-10-31 (design complete, handoff 001). Agents: read this, never edit it; write a
fragment in `status/`.

## Current milestone: v0.1 The jewel run

**Test:** is leveling a fresh character more fun than stock MoP?

**Scope:** Phase 0 (foundation and all quick wins), Phase 0a (fast iteration, GM tools, save safety,
capture), Phase 0b (Heartseed MVP), plus the timeline decision. No bot behavior work, no client
patch, no ML. Task list: `TASKS.md`. First session: `HANDOFF.md`.

## State

| Area | State |
|---|---|
| Fork, build, game data, databases | Not started (T-001, Claude Code) |
| Agent environments (worktrees, per-agent DBs and ports) | Not started |
| Game client | In place: `mop-clean/`, 5.4.8.18414 enUS, not patched |
| Quick wins | Not started |
| Heartseed (top priority) | Designed; not started |
| Heartseed intro scenario | Designed; waits on the timeline decision and the Horde figure |

**In progress:** nothing yet.
**Blocked:** nothing yet.

## Recent decisions (newest first)

- 2026-10-31: Build for the end state: systems are built in final shape; stopgaps the long-term plan would break are not built unless the fix is easy. Accepted interim states are listed in DESIGN.md (Heartseed pacing tuned in v0.5, one storyteller director, outleveled queue solo-only until bot pools).
- 2026-10-31: Milestones regrouped: Phase 5 (bot pools) in v0.2; call board, skill and plays (5b) in v0.5; side fun (11b) in v1.0; Sha of Grief in Beyond.
- 2026-10-31: v0.1 quick wins: all professions, no item binding, free respecs from level 10, spec loadouts with gear pages, cross-faction play, all flight paths, guaranteed skill-ups, no weekly currency caps, lighter death penalties, guild perks for everyone, unrestricted transmog, bigger stacks, free character services, shorter hearthstone, starting 10-slot bags, reagent bank, XP toggle with a single battleground queue, twink gear buffs (priority), outleveled dungeon queue.
- 2026-10-31: Twinks: one battleground queue, no normalization (jewel rank is level-capped and scales with weapon item level); Twinks tab in Collections with up to three templates per bracket per class; random upgrades and tertiary stats on all gear, rare on bot twinks.
- 2026-10-31: Modern stats (versatility, speed, leech, avoidance) as server auras, shown on a patched character sheet; random upgrades extend Thunderforged and Warforged; tertiary and socket upgrade track.
- 2026-10-31: Talent rebalance (buff weaker choices, measured with the balance sim); a second talent per tier unlocks every few lives.
- 2026-10-31: LLM features ship in two phases: scripted providers first (Phase A, with personalities), LLM providers later behind the same interfaces, with a circuit breaker. Tests always run scripted.
- 2026-10-31: PvP coordination: per-team call board, per-bot skill, technique library, terrain-aware plays, profession gadgets, class plays (Warsong Gulch and Arathi Basin first, then Alterac Valley).
- 2026-10-31: True Horde hierarchy: Sha hunters are Garrosh's champions and Alliance defectors; the Kor'kron champion from the intro is the apex nemesis; succession after each Siege kill.
- 2026-10-31: Sealed secrets: agents implement secrets from the human's briefs and keep them sealed; every secret is in a sealed log with an in-game hint ladder.
- 2026-10-31: Chronicle panel instead of interruptions; settings panel, progression journal, custom achievements and audio cues.
- 2026-10-31: Save safety: nightly snapshots and migrations tested against a copy of the real save. GM tools and `/idea` capture are v0.1.
- 2026-10-31: Also adopted: Sunsong housing with click placement, side fun (always-on Darkmoon Faire, fishing, archaeology, bot city raids and more), retail borrows (reagent bank, class campaigns, keystone runs, guild mission table), legacy consumables and world buffs, Zul'Gurub rebuilt as a raid, your own guild, enhanced graphics toggle, Sha of Grief mini-expansion (long-term). Demon hunters left out unless a client decomp appears.
- 2026-10-30: The living jewel is named the Heartseed and is the top priority. Lore, intro, Celestial paths, progression and a flag-driven MVP are in DESIGN.md; IDs in docs/ID_RANGES.md.
- 2026-10-30: Priority rule: cheap work that speeds up early phases is built first.
- 2026-10-30: Every timer is a config key; pure logic gets unit tests; content is prototyped in Eluna Lua.
- 2026-10-30: Every new player-facing system ships with a brief intro quest or popup.
- 2026-10-30: Modular where possible: systems are reusable modules with clean interfaces (see AGENTS.md).
- 2026-10-30: Raids reset every 2 days, no weekly lockouts, no vault. Rare mounts drop at 10% baseline.
- 2026-10-30: Mount speeds doubled (120 / 200 / 300 / 560 / 620%); starter mount and Apprentice Riding at character creation.
- 2026-10-30: Group corruption: any member at 0 means 0, otherwise the floor of the average.
- 2026-10-30: Up to 8 players; friends level normally, no level normalization.
- 2026-10-30: Broken is the reward: no power budget, the corruption dial is the counterweight.

## Known facts and gotchas

- `mop-clean/` is the 5.4.8.18414 enUS client (24 GB). Never commit it; keep it clean (extract into `gamedata/`). `_Wow.exe` and `_Wow-64.exe` skip file verification, so custom patch MPQs load with them. `WTF/Config.wtf` still points at Tauri's realmlist. The client needs the connection patch from `contrib/SkyFire Patcher/548`; patch copies, not the originals.
- The MoP legendary cloak questline (Wrathion, quests 31454 to 33104) is scripted in `src/server/scripts/Pandaria/zone_the_veiled_stair.cpp`; chapter 2 runs through Krasarang (Lion's Landing and Domination Point).
- Eluna (Lua) is compiled in but off (`Eluna.Enabled = 0`); Lua scripts reload at runtime. There is no C++ script hot-reload and no unit test framework yet.
- The 25-quest log cap is a client limit (fixed 750-field update block); don't try to raise it. A server-side backlog is the provisional workaround.
- The core already has the battle pet system (`src/server/game/BattlePet/`) and vignettes (`src/server/game/Vignette/`).
- Default branch is `playerbot`.
- `Updates.EnableDatabases = 0` is the shipped default; apply SQL updates by hand to your own DBs.
- All 43 MoP raid bosses and every Classic to Cata raid boss are scripted. There are no bot raid strategies; bot dungeon strategies exist for all nine MoP dungeons.
- Wintergrasp is on by default; Tol Barad is ported but off (`TolBarad.Enable = 0`) and its SQL is marked draft.
- Bots can join battlefields (`AiPlayerbot.RandomBotJoinBattlefield`, default 0).
- The 5.4.8 client almost certainly rejects modified FrameXML without an exe patch (untested). The bundled patcher is `contrib/SkyFire Patcher/548`.
- `src/server/scripts/Custom/` already has `PlayerStartScript.cpp` and `solocraft.cpp`; check them before writing new start-of-character or scaling code.

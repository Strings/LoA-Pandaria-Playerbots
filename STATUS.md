# Project status

Last folded: 2026-11-04 by Claude Code (66 fragments, T-001 to T-086j, BUG-004, neutral-aggro fix). Agents: read this, never edit
it; write a fragment in `status/`. Claude Code folds them.

## Current milestone: v0.1 The jewel run

**Test:** is leveling a fresh character more fun than stock MoP?

**Scope:** Phase 0 (foundation and all quick wins), Phase 0a (fast iteration, GM tools, save safety,
capture), Phase 0b (Heartseed MVP), plus the timeline decision. No bot behavior work, no ML. Client
patches only as small data and interface edits through `tools/clientpatch` (the human approved). Task list: `TASKS.md`.

## State

| Area | State |
|---|---|
| Fork, build, game data, databases | Done (T-001). Builds with Ninja and sccache through `tools\build\build.ps1`, which queues builds machine-wide |
| Agent environments | Done: worktrees, per-agent databases and ports for Claude Code, Codex and Gemini (T-002, T-005) |
| Fast iteration (Phase 0a) | Done except fixtures (T-007) and faster builds (T-060): test profiles, loader profiler, unit tests (CTest, 15 suites), Eluna prototypes, headless regression runner, save snapshots and migration checks, `.reload`, seeded test RNG, GM tools (`.clock`, `.scenario`, `.spectate`, `.bot why/state/...`), `/idea` and `/bug` capture (players too since BUG-004), regression scenarios covering every custom command set (professions added, T-074) |
| Heartseed (Phase 0b) | MVP done (T-020 to T-026, T-049): account-wide seed, stage glows, traits and majors, path-choice gossip, feeding and essences, story flags, GM commands. The level 10 major is an active Celestial Gift (T-080): Call of Xuen, Crane Rush, Ox Stomp, Serpent's Breath, with a redesigned addon panel (mandala, castable Gift button, sounds) and art slots awaiting generated art. Each later stage adds a rider to the Gift (T-082), and Call of Xuen keeps up with a mounted owner. Active play (T-083): path meters, 20 to 60 s Gift cooldowns shortened by play, Crane's Plume and Niuzao's Retort, proc-gated casts with a button glow. The rank cap is gone: a per-character Growth XP track (battleground wins, dungeon and raid bosses, quests and rares, T-084) gives stat points the player spends, with diminishing returns (`max_pct * points / (points + half_points)`, T-086c); reset and major change no longer need a shrine. The Gifts and rider auras are their own client-patched spells (T-086, clones 310100 to 310114) with their own names, tooltips, cooldowns and placeholder icons. Serpent's Breath is instant, channels while moving, reaches 12 yd with a larger jade flame (T-086f to j). The panel swaps the Gift from its Celestial icons (`.seed choose`). All numbers are placeholders until v0.5; glow IDs await a pick in the client |
| GM test suite | Done (T-048): checkpoints (`hs-10` and more), twink checkpoints, kits, ability tester, GMSuite addon |
| Quick wins | Done: mount speed x2 (T-010), rare mounts 10% including the world bosses (T-012, T-064), free respec and dual spec from 10 (T-016), solo config defaults (T-019), twink gear +7% (T-045, generator T-050), eleven primary professions with the `/profbar` addon (T-014), trade goods stack to 1000 (T-062), 36-slot bags from vendors (T-042, T-061). Raid saves kept until finished or abandoned, resets every 2 days (T-013); first login grants riding, the racial mount and four 10-slot bags (T-011); no binding for ordinary gear, quest and progression items stay bound (T-015); guild perks for everyone, free character-service stewards in the capitals, 5-minute hearthstone, mixed-type transmog (T-041). Riding bonus x2.4 (`Rate.MountedSpeed`), flight paths at 2x Master Riding flying (`Taxi.SpeedOfTopFlight`), ghosts run at 200% (`Death.GhostSpeed`), no durability loss on death (T-086g to j). Mounts drag from the journal to the bar (client patch, T-085). Open: T-044, T-046, T-047 |
| Loadouts and detached storage | Done: spec loadouts with `/lo` panel (T-017), gear pages per loadout (T-018), shared detached item storage, reagent bank with crafting drawing from it and `/rb` panel (T-043). Twink templates will use the same storage. Addon panels await a real-client playtest |
| Tooling | Done: live server MCP, crash triage, SQL validator, DBC reader and content search, SQL, config, command and legacy-script audits, `tools/map-check` for `docs/MAP.md`, `.areaprobe` (measures a spell's reach, cone and circle with probe dummies, T-086f). Open: balance sim (T-031) |
| World density | Spawn thinning (T-081): hostile open-world spawns within 10 yd of an older one are left out of the grids at startup (18,495 of 85,544); world data untouched, `.spawnthin` to inspect. Awaiting playtest feedback |
| Telemetry (T-004) | Done: server-side JSON-lines logging of human play (movement, decisions with context snapshots, outcomes, episodes), `.telemetry` player and GM commands, automatic pauses. On for main, writing to `I:/UltimateMoP/telemetry`. The client addon for UI signals is still to do |
| Bot load baseline (T-003) | Not started. Main with 500 bots settles at about 9.9 GB private memory within the first hour and stays flat |
| Timeline decision (T-036) | Waiting on the human |
| Game client | `mop-clean/`, 5.4.8.18414 enUS. Patched by update archives in `Data/` built from `tools/clientpatch/edits.json` (T-085: DBC, DB2 and Blizzard interface edits, spell clones with matching server SQL); launch with `_Wow-64.exe` |

**In progress:** T-044 (Codex PR 86, ready) is under review by Claude Code; T-046 (PR 91) and T-047 (draft PR 99) wait for Codex. Flash's T-075 smoke run is handed out again. Claude's T-060 (draft PR 78) is shelved. T-086 waits on a real-client look; T-087 (taxi mount vanishing on fast flights) is next for Claude. Main runs the T-086j build. The human is playtesting.

**Blocked:** nothing.

## Recent decisions (newest first)

- 2026-11-03: Flight paths fly at `Taxi.SpeedOfTopFlight` (2.0) times Master Riding flying through `Rate.MountedSpeed`; temporary buffs don't count. The spline speed cap skips units in flight.
- 2026-11-03: Serpent's Breath: instant channel, keeps going while moving, 38% of attack or spell power per tick, 12 yd reach (`Heartseed.Gift.BreathRange`). Riding multiplier 2.0 -> 2.4 ("another 20%"); ghosts at 200%; `DurabilityLoss.OnDeath = 0` now really means none.
- 2026-11-03: Growth stats have diminishing returns (the human's call): stamina 30/20, main stat 20/20, crit 10/10, haste 10/10 (`max_pct`/`half_points`). Changing a major and resetting Growth no longer need a shrine (`...ShrineOnly = 0`).
- 2026-11-03: Custom player-visible spells are clones of client spells in the system's ID block, never renamed borrowed spells; `tools/clientpatch` generates both the client archive and the server SQL. Placeholder icons until custom art.
- 2026-11-03: Client patches are update archives per chain named after the client build, built from `edits.json`; edits name what they replace so a source change fails the build. Edited Blizzard addons get a placeholder `.toc.sig`. The MountBar addon is retired.
- 2026-11-02: Quests and rares feed Heartseed Growth (T-084): 15 per quest turn-in, 40 per rare, within `LevelRange` of the character; repeatable quests other than dailies and weeklies give none. First dungeon clears are not a source; bosses already pay per kill.
- 2026-11-02: Telemetry (T-004) writes JSON-lines files, not database tables; rows wait 15 minutes in memory so `.telemetry discard` really deletes them. The module lives in `Custom/Telemetry/`, not `game/Telemetry/`, so the core's CMake stays untouched. GM commands pause logging until the next move or cast.
- 2026-11-02: Growth XP (T-083 part B, the human's call): minors unlock by character level, stat points are chosen by the player. Twinks with XP off earn no Growth unless `Heartseed.Growth.TwinksEarn = 1`. A reset is free but needs a Celestial shrine.
- 2026-11-02: Active play (T-083): Crane's Plume has one 15 s charge (the reused client spell has no charge category); Call of Xuen stays at 20 s (DESIGN.md updated). Meter state is runtime only.
- 2026-11-02: Gift riders (T-082, the human's call): Xuen haste / lightning arcs to 2 more / each kill cuts 1 s; Chi-Ji heal 10% / speed / killing blow resets; Niuzao -20% enemy damage / shield per enemy hit / wider, longer stun; Yu'lon leech 30% / jade DoT / each enemy hit cuts 1.5 s. Cooldown cuts halved from the first proposal so the Gift isn't up too often. Only the player's own killing blows count. Riders borrow client auras (Berserking, Sprint, Curse of Impotence, Ice Barrier, Jade Serpent Wave) with amounts and durations as data.
- 2026-11-02: Call of Xuen keeps up with a mounted owner: out of combat the tiger runs at its owner's speed and jumps to them past 25 yd (config keys).
- 2026-11-02: Codex's four approved PRs merged as one batch (PR 108, the human's OK); hotspot conflicts resolved by keeping both sides.
- 2026-11-02: Spawn thinning (T-081) is cross-type (one camp per 10 yd, whatever the mob), greedy by spawn id so the oldest spawn stays. Scripted, formation, linked, pooled, event and waypoint spawns, rares, bosses, critters and NPCs with flags are never thinned.
- 2026-11-02: Call of Xuen: 20 s cooldown, 10 s tiger, so it is part of the rotation (the human's call). `heartseed_gift.cooldown` and `duration` override the client spell's own; Xuen's lightning plays Chain Lightning's visual (`Heartseed.Gift.XuenLightningVisual`).
- 2026-11-01: The level 10 Heartseed major is an active Celestial Gift borrowing a client spell (the human's call); the old stat majors became minors 9 to 12. Gift cooldowns and power are data, like other trait numbers.
- 2026-11-01: The first v0.1 playtest starts once the quick wins land; telemetry (T-004), the balance-sim dummy room (T-031) and the bot benchmark (T-003) stay in v0.1 but may land after it. A few hours of untracked play are worth an earlier hands-on feel.
- 2026-11-01: Reagent bank (T-043) is per character (account-scope detached items still die with their owning character), usable anywhere, no stack limit by default (`ReagentBank.MaxStacks`). Any item some spell uses as a reagent counts, except quest, unique and timed items. Every spell's reagent check takes from the bags first, then the bank (`ReagentSource` core hook).
- 2026-11-01: Gear pages (T-018) hold only the pieces that differ: a switch stores the worn pieces the incoming page replaces and leaves every other slot as worn, so nothing is ever stripped. The worn loadout's page is empty; a loadout whose page holds pieces can't be deleted; pieces that can't be worn go to the bags, or stay on the page as spares when the bags are full.
- 2026-11-01: Detached item storage keeps an item's `item_instance` row and swaps its inventory row for a `detached_item` row (the mail path), so enchants, gems, transmog and binding survive. Gear pages, the reagent bank and twink templates each get their own kind.
- 2026-11-01: Loadouts (T-017): a switch writes the inactive talent group and runs the normal spec activation; talent changes made in the normal UI are captured into the active loadout; a new named loadout copies the active one; Heartseed majors ride per loadout (no shrine to swap saved loadouts, a shrine still to change a major).
- 2026-11-01: Claude Code folds status fragments into this file; agents push their branches as they go so another agent can finish a task when a session ends (AGENTS.md).
- 2026-11-01: Raids (T-013): unfinished raid saves are kept until finished or abandoned, only the leader can abandon, resets every 2 days (config key), no migration of old saves.
- 2026-11-01: The custom clock is separate from the core clock (the core reads the time in ~270 places). Custom timers read `CustomClock::Now()` so `.clock speed|jump` can test them; core respawns and resets keep their own GM commands.
- 2026-11-01: `.bot why` shows the playerbots engine's relevance values, the bots' real utility today. Tracing is per bot and opt-in. Later utility systems should feed relevance so the trace keeps explaining them.
- 2026-11-01: GM scenarios are plain command lists (any system's GM commands, no code); a scenario cannot start another.
- 2026-10-31: Twink checkpoints pick gear at apply time with an outlier filter (115% of the best leveled item level within ten levels), instead of hand-kept kits per class.
- 2026-10-31: Checkpoints confirm before applying (level-downs can't be undone) and leave lower tiers' majors alone.
- 2026-10-31: Heartseed is account-wide: a migrated account keeps its furthest character's rank and Growth; Growth spends up to the cap of whichever character earns it; weapon appearances stay per character until Collections.
- 2026-10-31: Twink gear is +7% over matching heirlooms, weighted toward damage (stamina .25, crit and haste 1.25). A stat-budget heuristic, not simulated DPS.
- 2026-10-31: Heartseed feeding (the human's call): prompt on upgrade; twink weapons carry a Twink flag and feed only when deleted, with a one-time tutorial; every fed or deleted weapon's appearance is recorded for later transmog.
- 2026-10-31: Heartseed path choice: the first major is chosen anywhere, changes only at a shrine (the four temples and the Celestial Court). Before Pandaria a chosen major can't change. `.seed` reopens the window.
- 2026-10-31: Heartseed minors fill in on paths with at least one major (path 0 minors always); the core bonus is 5% of weapon damage; the proc deals damage directly (casts dropped procs on range and facing).
- 2026-10-31: Heartseed glow lives only in the visible item field (no enchant effects, nothing travels with a traded weapon). Story flags are rows with a time, for the chronicle. Bots skip Heartseed loading.
- 2026-10-31: Heartseed granting quest is self-given at any level (works in every starting zone and for existing characters). The Theramore scenario later changes how it is delivered, not the quest.
- 2026-10-31: Any final encounter counts as a first dungeon clear. World bosses now give normal kill credit (KillRewarder fix).
- 2026-10-31: Mount speed multiplies the bonus above base movement, matching the decided riding percentages.
- 2026-10-31: MySQL user `ultimatemop` for every database set; databases from the v1.0.4 dump.
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

## Open follow-ups

- Spawn thinning: ask the human how density feels; raise `SpawnThinning.Radius` if still packed, consider faster respawns if zones feel empty.
- Celestial Gifts: the lightning chain, kill cooldown cuts, Serpent's Breath riders and Xuen's mounted catch-up have no automated check; look in a playtest. Generated art (`addons/Heartseed/art/`, then `USE_ART = true`); Xuen's lightning visual needs a client look. Gift clones (T-086): check names, tooltips, icons and the breath flame's size in the real client; custom icon art (BLP writer, SpellIcon rows) when the panel art lands; the triggered helpers (Xuen's lightning 123996, the Breath tick 124947) still show their own names in the combat log.
- Heartseed: the gossip "Change it." path didn't swap the Gift in the real client (the headless client swaps fine); the panel icons work around it.
- T-087: on long flight paths at the doubled speed the taxi mount model vanishes partway.
- Client patch: GlueXML and FrameXML are signed differently and untested; expect an exe patch for those.
- Possess `AddAllowedMover` assert crashed main once; needs a look.
- Main characters DB: apply PR 69's soulbound cleanup while main is stopped for the next restart (the human approved; back up `item_instance` first). Main world and auth already have the batch and T-082 SQL.
- Professions (T-014): the intro shows on every login; limit it to characters with fewer than 3 primaries. The conf comment says "Range: 0-10" with default 11.
- Raid retention (T-013): `SetBossState` keeps a DONE boss DONE while retention is on; raid scripts that reset a boss on purpose need a look. Saves commit synchronously on the map thread.
- No binding (T-015): about 65,000 item-sparse hotfix rows make each login send about 780 KB of hotfix info. Watch login time.
- Sha of Anger, Galleon and Oondasta drop only their mounts; their gear needs real 5.4.8 loot data (`docs/agents/world-boss-loot.md`). Not v0.1.
- Legacy scripts: `docs/agents/legacy-scripts.md` recommends pruning unregistered donor scripts and unhooking `solocraft.cpp` in favor of SoloConfig.

- Reagent bank: check in the real client whether the profession window allows Create when the reagents are only in the bank. If the client blocks it, the addon needs a craft-from-bank button (`docs/PLAYTEST.md`).
- Loadouts and gear pages: the `/lo` panel, gear page drag and drop and gem links need a real-client look; the bags-full path (spares) is untested live.
- Five systems borrow core permissions (830, 811, 509; see `docs/agents/command-audit.md`), `.startingkit` among them; give them their own IDs from their blocks.
- Pick the final Heartseed glows in the client (candidates in `docs/agents/heartseed-glows.md`), and a real proc spell instead of the placeholder 16614.
- Growth from custom rares (4d), events and Sha hunters (7) calls `HeartseedMgr::AddGrowthXp` when they land. Quest and rare pacing needs a playtest look.
- Telemetry: check the first real session file on main; the client addon for UI signals is a later task.
- Green "Twink" tooltip tag needs item hotfixes or the client patch.
- Appearance collection from fed and deleted weapons (Collections).
- Twink brackets need a playtest; proc and socket value is ignored when picking gear.
- `server.commit_hash` in `/idea` tickets reads "Archive": the build doesn't pick up the git revision.
- sccache caches nothing while the precompiled header is on (T-060).
- Seen once: the world thread hung in `Player::~Player` when a master relogged while its bot alt was grouped with it. Watch for it.

## Known facts and gotchas

- **Build:** `tools\build\build.ps1 -Target worldserver` from your worktree; add `-Reconfigure` after adding source files, or you get link errors. Builds wait on a machine-wide lock; two overlapping builds on this machine (6 cores, 32 GB) run about six times slower. Binaries land in `build\bin\`; copy `.conf.dist` files from `src/`.
- **Rebuild cost:** editing `ScriptMgr.h`, `Player.h`, `Unit.h`, `World.h`, `Engine.h` or `PlayerbotAI.h` recompiles hundreds of files for every agent. Prefer a side table or a small new header.
- **Runtime:** `legacy.dll` from the OpenSSL bin folder must sit next to `worldserver.exe`, or the first world connection crashes in `ARC4::ARC4`.
- **SQL:** `Updates.EnableDatabases = 0`; apply updates by hand. The server reads `spell_dbc` only at startup. Run `tools/sql-validator` on new SQL before a PR (run its tests from its own folder). `core.autocrlf=true` with no `.gitattributes` for `*.sql`, so working-tree SQL has CRLF.
- **Twink SQL:** `tools/twink-tuning` writes to stdout by default; a retune goes into a new dated update, never over an applied one.
- **Scenarios that spawn NPCs** start with `.go -9339.7 170.2 61.6 0` (Elwynn): `checkpoint-hs10` leaves Seedtest in Stormwind, where guards kill spawned kobolds.
- **Chi Torpedo** is cast as 121828 when the client asks for 115008; scripts on the Gift hook both.
- **Headless client:** SOAP verbs need `MOP_SOAP_USER`/`MOP_SOAP_PASSWORD`; `target npc` doesn't resolve, pass the raw guid. Self-given quests use the character's own guid. `.die` gives no kill credit (use `.damage <health>`). `.go x y z map` takes coordinates directly. Path choice is two `gossip-select` steps.
- **Test profile:** set `Heartseed.Grant.OfferOnLogin = 0` and `Heartseed.Choice.PromptOnLogin = 0` if the login windows get in the way.
- **Reload:** `.reload config` reopens logs in overwrite mode, so startup lines vanish from `Server.log`. Lua reloads through `.reload config`, not `.reload eluna`.
- **Bots:** register a `.bot` section with `BotInspect::Register` for new bot systems; register checkpoint steps for new player state (see `checkpoint_heartseed.cpp`). Telemetry (T-004) should pause around `CheckpointMgr::Apply`. `OnKillCredit` suits telemetry outcome events.
- **MAP.md:** run `python tools/map-check/check_map.py` after adding a folder; it fails on missing or stale paths.
- **Tickets** from `/idea` and `/bug` land in `<repo>/tickets/` (gitignored, format in `tickets/README.md`).
- **Snapshots** (T-037): capture blocks writes while dumping; restore needs the same MySQL version and a separate instance.
- `mop-clean/` is the 5.4.8.18414 enUS client (24 GB). Never commit it; keep it clean (extract into `gamedata/`). `_Wow.exe` and `_Wow-64.exe` skip file verification, so custom patch MPQs load with them. `WTF/Config.wtf` still points at Tauri's realmlist. The client needs the connection patch from `contrib/SkyFire Patcher/548`; patch copies, not the originals.
- The MoP legendary cloak questline (Wrathion, quests 31454 to 33104) is scripted in `src/server/scripts/Pandaria/zone_the_veiled_stair.cpp`; chapter 2 runs through Krasarang (Lion's Landing and Domination Point).
- The 25-quest log cap is a client limit (fixed 750-field update block); don't try to raise it. A server-side backlog is the provisional workaround.
- Eluna (Lua) is compiled in and off in the shipped config (`Eluna.Enabled = 0`); the T-009 test profiles turn it on.
- The core already has the battle pet system (`src/server/game/BattlePet/`) and vignettes (`src/server/game/Vignette/`).
- Default branch is `playerbot`. Use `--repo Strings/LoA-Pandaria-Playerbots` with `gh`.
- All 43 MoP raid bosses and every Classic to Cata raid boss are scripted. There are no bot raid strategies; bot dungeon strategies exist for all nine MoP dungeons.
- Wintergrasp is on by default; Tol Barad is ported but off (`TolBarad.Enable = 0`) and its SQL is marked draft.
- Bots can join battlefields (`AiPlayerbot.RandomBotJoinBattlefield`, default 0).
- The 5.4.8 client almost certainly rejects modified FrameXML without an exe patch (untested). The bundled patcher is `contrib/SkyFire Patcher/548`.
- `src/server/scripts/Custom/` already has `PlayerStartScript.cpp` and `solocraft.cpp`; check them before writing new start-of-character or scaling code.
- **Detached storage:** `sDetachedStorage` (`Custom/DetachedStorage/`). Stored items are loaded per character at login (not for bots). Account-scope items still have a character as owner; deleting that character deletes them. `.pdump` doesn't carry stored items.
- **Loadout listeners:** systems that follow the worn loadout register `Loadouts::Listener` (`OnSwitched`, `CanDelete`); per-spec state registers a `Loadouts::Part`.
- **Reagent sources:** `src/server/game/Spells/ReagentSource.h` is the one hook for reagents outside the bags; one source can be registered.
- **Regression runs** need a generated test-profile runtime: `python tools/devenv/setup.py --agent <you> --gamedata I:/UltimateMoP/gamedata --binaries build/bin --output build/devenv --profile test --load none --db-user ultimatemop` (with `MOP_DB_PASSWORD` set), start `build/devenv/worldserver.exe -c <absolute conf path>`, then `tests/regression/run.py --runtime build/devenv`. Regression SOAP replies are complete and the none bot profile is supported (Codex fix).
- **Config:** the core refuses to start on a duplicate key in `worldserver.conf`. Before adding a key to `.conf.dist`, grep for it.
- **Client edits** go in `tools/clientpatch/edits.json`, never by hand into the client; Claude Code installs the archives on deploy. A new custom spell: add a `spell` edit, run `build.py sql`, commit the regenerated SQL. Use `.areaprobe` to check any AoE you add (25 spokes avoid the cone edge).
- **Core fixes worth knowing:** `spelleffect_dbc` now fills MiscValueB (upstream SQL summons got their summon properties back; a changed scripted summon may be why). `MoveSplineInit` doesn't cap velocity for units in flight. Neutral mobs accept a target already on their threat list, so casters who never auto-attack get fought back.
- BUG-003: deleting a newly added training dummy (31144) can crash in aura removal. Avoid in GM tests until fixed.

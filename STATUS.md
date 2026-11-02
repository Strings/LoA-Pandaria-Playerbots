# Project status

Last folded: 2026-11-02 by Claude Code (58 fragments, T-001 to T-082, BUG-004). Agents: read this, never edit
it; write a fragment in `status/`. Claude Code folds them.

## Current milestone: v0.1 The jewel run

**Test:** is leveling a fresh character more fun than stock MoP?

**Scope:** Phase 0 (foundation and all quick wins), Phase 0a (fast iteration, GM tools, save safety,
capture), Phase 0b (Heartseed MVP), plus the timeline decision. No bot behavior work, no client
patch, no ML. Task list: `TASKS.md`.

## State

| Area | State |
|---|---|
| Fork, build, game data, databases | Done (T-001). Builds with Ninja and sccache through `tools\build\build.ps1`, which queues builds machine-wide |
| Agent environments | Done: worktrees, per-agent databases and ports for Claude Code, Codex and Gemini (T-002, T-005) |
| Fast iteration (Phase 0a) | Done except fixtures (T-007) and faster builds (T-060): test profiles, loader profiler, unit tests (CTest, 15 suites), Eluna prototypes, headless regression runner, save snapshots and migration checks, `.reload`, seeded test RNG, GM tools (`.clock`, `.scenario`, `.spectate`, `.bot why/state/...`), `/idea` and `/bug` capture (players too since BUG-004), regression scenarios covering every custom command set (professions added, T-074) |
| Heartseed (Phase 0b) | MVP done (T-020 to T-026, T-049): account-wide seed, stage glows, traits and majors, path-choice gossip, feeding and essences, rank cap, story flags, GM commands. The level 10 major is an active Celestial Gift (T-080): Call of Xuen, Crane Rush, Ox Stomp, Serpent's Breath, with a redesigned addon panel (mandala, castable Gift button, sounds) and art slots awaiting generated art. Each later stage adds a rider to the Gift (T-082), and Call of Xuen keeps up with a mounted owner. All numbers are placeholders until v0.5; glow IDs await a pick in the client |
| GM test suite | Done (T-048): checkpoints (`hs-10` and more), twink checkpoints, kits, ability tester, GMSuite addon |
| Quick wins | Done: mount speed x2 (T-010), rare mounts 10% including the world bosses (T-012, T-064), free respec and dual spec from 10 (T-016), solo config defaults (T-019), twink gear +7% (T-045, generator T-050), eleven primary professions with the `/profbar` addon (T-014), trade goods stack to 1000 (T-062), 36-slot bags from vendors (T-042, T-061). Raid saves kept until finished or abandoned, resets every 2 days (T-013); first login grants riding, the racial mount and four 10-slot bags (T-011); no binding for ordinary gear, quest and progression items stay bound (T-015); guild perks for everyone, free character-service stewards in the capitals, 5-minute hearthstone, mixed-type transmog (T-041). Open: T-044, T-046, T-047 |
| Loadouts and detached storage | Done: spec loadouts with `/lo` panel (T-017), gear pages per loadout (T-018), shared detached item storage, reagent bank with crafting drawing from it and `/rb` panel (T-043). Twink templates will use the same storage. Addon panels await a real-client playtest |
| Tooling | Done: live server MCP, crash triage, SQL validator, DBC reader and content search, SQL, config, command and legacy-script audits, `tools/map-check` for `docs/MAP.md`. Open: balance sim (T-031) |
| World density | Spawn thinning (T-081): hostile open-world spawns within 10 yd of an older one are left out of the grids at startup (18,495 of 85,544); world data untouched, `.spawnthin` to inspect. Awaiting playtest feedback |
| Telemetry (T-004), bot load baseline (T-003) | Not started |
| Timeline decision (T-036) | Waiting on the human |
| Game client | `mop-clean/`, 5.4.8.18414 enUS, not patched |

**In progress:** Codex's T-044 (PR 86), T-046 (PR 91) and T-047 (draft PR 99) open; Flash on the T-075 smoke run; Claude on T-060 (draft PR 78). Main still runs the T-081 build: the batch (T-011, T-013, T-015, T-041) and T-082 deploy at the next restart, with PR 69's characters cleanup. The human is playtesting T-080 and T-081.

**Blocked:** nothing.

## Recent decisions (newest first)

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
- Celestial Gifts: the lightning chain, kill cooldown cuts, Serpent's Breath riders and Xuen's mounted catch-up have no automated check; look in a playtest. Generated art (`addons/Heartseed/art/`, then `USE_ART = true`); Xuen's lightning visual needs a client look. Spell tooltips show the borrowed spells' text until a client patch.
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
- Growth from other sources (quests, rares, first dungeon clears, Sha hunters) calls `HeartseedMgr::AddGrowth`; not wired yet.
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
- BUG-003: deleting a newly added training dummy (31144) can crash in aura removal. Avoid in GM tests until fixed.

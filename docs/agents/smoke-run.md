# T-075 Pre-Playtest Smoke Run Results

- **playerbot commit:** `edca33f5`
- **Date:** 2026-11-04
- **Agent:** Gemini (Antigravity)
- **Worktree:** `I:\UltimateMoP\.worktrees\gemini`
- **Runtime:** `build/devenv` (agent ports: 3754 / 8115 / 7908)

---

## Scenario Results

| Scenario | Result | Failing Step and Server Reply |
|---|---|---|
| `areaprobe-breath` | FAIL | Failing step: `say .go -9339.7 170.2 61.6 0`<br>Server reply / crash: Crash in `npc_hallowend::npc_hallowend` at `src/server/scripts/Events/hallows_end.cpp:273`<br>`wait-for {'evt': 'areaprobe_settle'} timed out after 8s` / `logout: no SMSG_LOGOUT_COMPLETE within 30s` |
| `breath-move` | FAIL | Failing step: World session connection handshake<br>Server reply: `ConnectionResetError: [WinError 10054] An existing connection was forcibly closed by the remote host` |
| `capture-idea` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `capture` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `checkpoint-hs10` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `custom-reload-rng` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `ghost-speed` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `gmtools` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-change` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-choose` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-gifts` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-gm` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-meters` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-riders` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-sync` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `heartseed-xuen` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `loadouts` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `login` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `mountspeed` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `neutral-aggro` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `professions` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `reagent-bank` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `respec` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `solo` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `spawn-thinning` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `taxi-speed` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |
| `telemetry` | FAIL | Failing step: WorldClient connect<br>Server reply: `ConnectionRefusedError: [WinError 10061] No connection could be made because the target machine actively refused it` |

---

## Diagnostic Notes on Failure

During the first scenario (`areaprobe-breath.json`), step 7 executes `.go -9339.7 170.2 61.6 0` (Goldshire, Elwynn Forest). Because current calendar date (2026-11-04) falls within the active Hallow's End world event window, the Goldshire grid loader attempts to spawn the event NPC `npc_hallowend`. Its constructor in `src/server/scripts/Events/hallows_end.cpp:273` dereferences a null pointer (`me->SummonCreature(...)->ToCreature()`), causing an immediate fatal `ACCESS_VIOLATION` (exception `C0000005`) in `worldserver.exe`.

This crash was verified twice:
1. Full suite execution at 17:35:58: generated `Crashes/Archive_worldserver.exe_[4-11_17-35-58].txt` pointing directly to `npc_hallowend::npc_hallowend+140 hallows_end.cpp line 273`. With `worldserver.exe` terminated, all subsequent scenarios in the run were unable to connect.
2. Isolated re-run of `areaprobe-breath.json` at 17:41:30: generated `Crashes/Archive_worldserver.exe_[4-11_17-41-30].txt` with the exact same stack trace at `hallows_end.cpp line 273`.

---

## SOAP Command Outputs

### `.checkpoint list` (first 10 lines)

```text
18 checkpoints (.checkpoint explain <name>, .checkpoint apply <name>):
 hs-10 [heartseed] Level 10, the first major choice waiting. No awakenings.
 hs-25 [heartseed] Level 25 major tier; first dungeon done.
 hs-40 [heartseed] Level 40 major tier.
 hs-55 [heartseed] Level 55 major tier.
 legendary-1 [legendary] First legacy legendary unlock (placeholder level 60 until the pacing pass).
 hs-70 [heartseed] Level 70 major tier.
 legendary-2 [legendary] Second legacy legendary unlock (placeholder level 80 until the pacing pass).
 hs-85 [heartseed] Level 85, ready to sail for Pandaria (its awakening still ahead).
 max-90 [heartseed] Level 90 with every awakening, epic gear, at the faction shrine.
```

### `.spawnthin` (first 10 lines)

```text
Spawn thinning: radius 10.0 yd, maps 0 1 530 571 870. 18495 of 85544 hostile spawns left out since startup. Changes apply after a restart; .spawnthin near [yards] lists the ones around you.
```

---

## Summary Line of run.py

```text
I:\UltimateMoP\.worktrees\gemini\build\regression\20261104T223552Z-21c54b11
```
(27 tests, 27 failures)

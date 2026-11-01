# GM tools

GM commands for testing systems without playing to them (DESIGN.md, "GM tools (decided)"). Each later system adds
its own force and inspect commands; these are the shared ones. Permissions come from the GM tools block in
`docs/ID_RANGES.md` (91000 and up), linked to role 197 (Gamemaster Commands).

| Folder | Task | Commands |
|---|---|---|
| `Checkpoints/` | T-048 | `.checkpoint`, `.kit`, `.gmsuite` (see its README) |
| `Clock/` | T-039 | `.clock info`, `.clock speed <x>`, `.clock jump <duration>`, `.clock reset` |
| `Scenarios/` | T-039 | `.scenario list`, `.scenario explain <name>`, `.scenario load <name> [args]` |
| `Spectate/` | T-039 | `.spectate <name>` (or selected), `.spectate stop` |

Bot inspection (`.bot why`, `.bot state`, `.bot traits|memory|skill`) lives in the playerbots module, in
`modules/mod_playerbots/src/Inspect/`, because it reads the bot engine.

## Clock

`CustomClock` (`src/server/game/Time/CustomClock.h`) is the clock custom systems read for the durations they own:
war front cycles, director cooldowns, custom respawns and resets. It equals wall time until `.clock` speeds it up or
jumps it. **New custom timers should read `CustomClock::Now()`, not `time(nullptr)`,** so they can be tested this way.

- `.clock speed 60` makes one real second one clock minute; `.clock speed 1` returns to normal and keeps the time
  gained. The maximum is `GMTools.Clock.MaxSpeed`.
- `.clock jump 6h` (also `90s`, `15m`, `2d`, `1d12h`) moves it forward. The clock never runs backwards, except through
  `.clock reset`, which returns it to wall time. The maximum single jump is `GMTools.Clock.MaxJumpSeconds`.
- Clients follow it: their day and night use the shifted time and rate, also for characters logging in later.
- Core timers (creature respawns, instance resets, auctions, mail) keep reading the real clock, because shifting the
  core clock would move some of them and not others. Use the core's commands for those (`.respawn`, `.instance
  unbind`). Not saved: a restart returns to wall time.

## Scenarios

A scenario is data: ordered GM command lines in `gmtools_scenario` and `gmtools_scenario_step` (world database), run
as the calling GM. So it can drive `.botscenario`, `.tele`, `.checkpoint`, `.clock` and any later system's commands
with no new code. Reload with `.reload gmtools_scenario`; a bad row keeps the previous set.

Placeholders: `{player}`, `{target}` (selected unit), `{map}`, `{zone}`, `{x}`, `{y}`, `{z}`, `{o}` (the GM's
position) and `{arg1}`, `{arg2}`, ... (words after the name). Every step is expanded before any runs, so a missing
argument stops the scenario with nothing done. `{{` writes a literal `{`. A step may not start another scenario.

Seed scenarios: `bot-wander-here <seconds>`, `bot-fight-here <entry> <seconds>`, `clock-next-day`.

## Spectate

`.spectate <name>` makes the GM invisible and moves their camera to the target (the core's far-sight viewpoint); the
GM's body stays where it was, on the target's map. If the target is on another open-world map, the GM is teleported
there first, and the camera attaches on arrival (`GMTools.Spectate.AttachTimeoutMs`). Instanced targets need the GM
inside the same instance first. It ends on `.spectate stop`, when the target logs out, changes maps or leaves the
GM's map, and GM visibility returns to what it was.

The viewpoint has no aura, so this script detaches it itself, before a target logs out (the core asserts if a unit
is deleted with viewers attached).

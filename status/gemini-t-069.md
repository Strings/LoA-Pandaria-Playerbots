# T-069: Verify commands, config keys, and addon slash commands in docs/PLAYTEST.md

**Agent:** gemini
**Branch:** gemini/T-069-playtest-check
**Task status:** review

## What changed
- Verified all commands named in `docs/PLAYTEST.md` against C++ CommandScript tables:
  - `.bug`, `.idea` (`src/server/scripts/Custom/Capture/capture_scripts.cpp`)
  - `.seed` (`src/server/scripts/Custom/Heartseed/heartseed_scripts.cpp`)
  - `.clock speed`, `.clock reset` (`src/server/scripts/Custom/GMTools/Clock/gmtools_clock.cpp`)
  - `.spectate`, `.spectate stop` (`src/server/scripts/Custom/GMTools/Spectate/gmtools_spectate.cpp`)
  - `.bot why` (`modules/mod_playerbots/src/Inspect/cs_bot_inspect.cpp`)
- Verified all addon slash commands and UI button labels against addon Lua files:
  - `/gms`, `/gmsuite` (`addons/GMSuite/GMSuite.lua`)
  - `/lo`, `/loadouts`, button **Gear** (`addons/Loadouts/Loadouts.lua`)
  - `/rb`, `/reagentbank`, button **Deposit all reagents**, tooltip line `Reagent bank: N` (`addons/ReagentBank/ReagentBank.lua`)
  - `/bug`, `/idea`, `/capture` (`addons/Capture/Capture.lua`)
- Verified checkpoints (`hs-10`, `hs-25`) against `src/server/scripts/Custom/GMTools/Checkpoints/checkpoint_scripts.cpp`.
- Updated Reporting section in `docs/PLAYTEST.md` to note `/bug` and `/idea` slash commands (which trigger the screenshot capture when using the Capture addon) alongside `.bug` and `.idea`.

## How it was tested
- Audited against command tables, `worldserver.conf.dist`, and Lua addon scripts.

## Decisions made
- None.

## For the other agent
- All playtest commands, slash commands, and addon paths are validated and up to date.

## Next
- None (queue completed).

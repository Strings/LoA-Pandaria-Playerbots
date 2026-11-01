# T-040: /idea and /bug capture (finished from Gemini's branch)

**Agent:** claude (taking over from gemini)
**Branch:** claude/T-040-capture
**Task status:** done

## What changed
- Merged Gemini's work as is (see `status/gemini-T-040.md`), then:
  - **Bot data goes through `BotInspect`.** Capture reads bots only through the module's public API
    (`IsBot`, `Strategies`, `LastAction`, `LastDecisionSummary`; the first three are new). Gemini's
    cast-to-subclass access to a protected engine member is gone, and the 21 module include paths in
    `src/server/scripts/CMakeLists.txt` are down to one (`Inspect`).
  - **`last_decision`** is the traced choice with its score when `.bot why` is tracing that bot, else the last
    action.
  - **Nearby bots** must be in the same instance, not just the same map.
  - Removed the capture copy of the engine-trail parser (`ExtractLastAction`) and its test; `BotInspect`
    owns that now.

## How it was tested
- Clean build. All 11 unit-test suites pass, including `unit_capture_rules`.
- Live, headless, with the bot alt traced: `.idea [screenshot=...] text`, `.bug text` and `.capture list 5`.
  The ticket JSON has the location (Stormwind, Trade District), the screenshot name, and the nearby bot with its
  strategies and `last_decision` "add all loot (5.000, traced)".

## For the other agents
- Tickets land in `<repo>/tickets/` (gitignored; `tickets/README.md` explains the format and the pickup flow).
- **Known gap:** `server.commit_hash` reads "Archive": the build does not pick up the git revision. That's a
  separate small fix in the revision CMake step.

## Next
- A GMSuite addon button for `/idea` and `/bug` later, if the slash commands aren't enough in play.

# Capture (`.idea` and `.bug`)

In-game feedback and bug capture module (T-040). Pairs with the client addon in `addons/Capture/` (`SLASH_IDEA1 = "/idea"`, `SLASH_BUG1 = "/bug"`) to snapshot world state into structured ticket files on disk for coding agents and developers.

## Commands

| Command | Permission | Description |
|---|---|---|
| `.idea <text>` | 91007, players (role 199) | Snapshots player, target, group, nearby bots, and active custom systems as an `idea` ticket (`<id>_idea_<slug>.json` and `.md`). |
| `.bug <text>` | 91007, players (role 199) | Same snapshot, tagged as a `bug` ticket (`<id>_bug_<slug>.json` and `.md`). |
| `.capture list [n]` | 91008, GMs (role 197) | Lists the newest `n` (default 10) tickets in `Capture.TicketsDir`. |
| `.capture show <id>` | 91008, GMs (role 197) | Prints the Markdown summary of ticket `<id>` (prefix or full filename stem) to chat/console. |

The commands accept an optional `[screenshot=WoWScrnShot_MMDDYY_HHMMSS]` prefix (sent automatically by `addons/Capture` right after calling `Screenshot()`) or `--screenshot=<name>` flag and store it in the `screenshot_path` field.

## Configuration (`worldserver.conf`)

- `Capture.TicketsDir` (string, default `""` -> `<repo>/tickets`): Directory where `.json` and `.md` ticket files are written. Relative paths resolve against `GitRevision::GetSourceDirectory()`.
- `Capture.NearbyBotRadius` (float, default `100.0`): Radius in yards around the capturing player within which playerbots are included in `nearby_bots`.

## Architecture

- `CaptureRules.h` / `CaptureRules.cpp`: Pure server-independent formatting, slug sanitization, JSON escaping, command argument parsing, and `Engine::GetLastAction()` ring-buffer cleaning. Covered by standalone unit tests in `tests/capture/`.
- `capture_scripts.cpp`: `WorldScript` for config loading + `CommandScript` registering `.idea`, `.bug`, `.capture list`, and `.capture show`. Reads `HeartseedMgr` via its read-only public API and `PlayerbotAI` / `Engine::GetLastAction()` without modifying `Engine.*`. Each bot entry includes `last_decision` (currently populated from `last_action`, ready for T-039 `.bot why` decision traces).

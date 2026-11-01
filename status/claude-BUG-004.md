# BUG-004: players can use .idea and .bug

## What changed
- `.idea` and `.bug` use new permission 91007, linked to Player Commands (role 199); `.capture list/show` use 91008, linked to GM Commands (role 197). They used the ticket permission (742), which only moderators have.
- Auth update `sql/updates/master/auth/2026_11_01_claude_04_capture_commands.sql` (applied to auth_claude; applied to main `auth` after merge).
- Regression scenario `tests/regression/scenarios/capture.json` (GM). Verified by hand at gmlevel 0: `.bug` captures, `.capture list` is refused.

## Others must know
- IDs 91007/91008 are now taken in the GM tools block of `docs/ID_RANGES.md`.

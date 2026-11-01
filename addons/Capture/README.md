# Capture Addon (`/idea` and `/bug`)

Client-side companion addon for the server's `.idea`, `.bug`, and `.capture` GM commands (T-040).

## Installation

Copy or symlink `addons/Capture` into `<WoW Client>/Interface/AddOns/Capture`.

## Usage

- `/idea <text>` — Calls `Screenshot()`, computes the expected screenshot stem (`WoWScrnShot_MMDDYY_HHMMSS`), and sends `.idea [screenshot=WoWScrnShot_MMDDYY_HHMMSS] <text>` to the server.
- `/bug <text>` — Calls `Screenshot()`, computes the expected screenshot stem (`WoWScrnShot_MMDDYY_HHMMSS`), and sends `.bug [screenshot=WoWScrnShot_MMDDYY_HHMMSS] <text>` to the server.
- `/capture` or `/capture list [n]` or `/capture show <id>` — Forwards to `.capture` on the server.

## Screenshot Naming Note (Best-Effort)

The 5.4.8 client saves screenshots asynchronously to `<WoW Client>/Screenshots/WoWScrnShot_MMDDYY_HHMMSS.jpg` (or `.tga`) using the client machine's local clock at the moment `Screenshot()` runs. Because the Lua environment does not receive the final file path back from `Screenshot()`, the addon records the expected `WoWScrnShot_MMDDYY_HHMMSS` stem from `date("%m%d%y_%H%M%S")` as a best-effort match; if a screenshot is taken right on a second boundary, the file on disk may be 1 second later.

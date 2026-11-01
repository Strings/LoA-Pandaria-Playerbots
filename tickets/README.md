# Captured Tickets (`/idea` and `/bug`)

This directory holds in-game feedback and bug snapshots written by `.idea <text>` and `.bug <text>` (or the `addons/Capture/` `/idea` and `/bug` slash commands).

Ticket files (`*.json` and `*.md` except this `README.md`) are gitignored so local playtest captures do not pollute the working tree.

## File Naming

Each capture writes a pair of files with the same stem:

- `<id>_<kind>_<slug>.json` — Full machine-readable snapshot.
- `<id>_<kind>_<slug>.md` — Human-readable Markdown summary with embedded JSON block.

Where:

- `<id>` is `YYYYMMDD-HHMMSS-NNN` (UTC timestamp + 3-digit sequence number).
- `<kind>` is `idea` or `bug`.
- `<slug>` is a sanitized lowercase slug of the ticket text (up to 32 characters).

## JSON Schema

```json
{
  "id": "20261031-223015-001",
  "kind": "bug",
  "text": "Quest giver floats above the bridge",
  "realm_time": "2026-10-31T22:30:15Z",
  "account": {
    "id": 1,
    "name": "ADMIN"
  },
  "character": {
    "guid": 10,
    "name": "Testchar"
  },
  "location": {
    "map_id": 0,
    "map_name": "Eastern Kingdoms",
    "zone_id": 12,
    "zone_name": "Elwynn Forest",
    "area_id": 87,
    "area_name": "Goldshire",
    "x": -9464.0,
    "y": 62.0,
    "z": 56.0,
    "o": 1.57
  },
  "target": {
    "guid": 17379391068755656951,
    "entry": 240,
    "name": "Marshal Dughan",
    "health": 4500,
    "max_health": 4500
  },
  "group_members": [],
  "nearby_bots": [
    {
      "name": "Botalpha",
      "class": "Warrior",
      "level": 10,
      "distance": 12.5,
      "strategies": ["dps", "close"],
      "last_action": "melee - OK",
      "last_decision": "melee - OK"
    }
  ],
  "custom_systems": {
    "heartseed": {
      "enabled": true,
      "has_seed": true,
      "rank": 4,
      "effective_rank": 4,
      "rank_cap": 10,
      "growth": 420
    }
  },
  "server": {
    "commit_hash": "e68d7112",
    "build": "..."
  },
  "screenshot_path": "WoWScrnShot_103126_183015"
}
```

> **Note on `last_decision`:** Each entry in `nearby_bots` includes `last_decision`, currently populated from `last_action` (`Engine::GetLastAction()`). Task T-039 (`.bot why` decision trace) will wire the structured decision trace into this field.

## How Coding Agents Pick Up a Ticket

1. **List or locate the ticket:**
   - On disk: inspect the newest `.json` / `.md` file in `tickets/` (or the path configured in `Capture.TicketsDir`).
   - In game / via SOAP: run `.capture list [n]` and `.capture show <id>`.
2. **Reproduce the exact world state:**
   - Read `location` (`map_id`, `x`, `y`, `z`, `o`) and teleport there with `.go xyz <x> <y> <z> <map_id> <o>`.
   - Read `target` (`entry`, `guid`, `name`, `health`) to identify the exact NPC/unit involved.
   - Read `custom_systems.heartseed` (`rank`, `growth`, `effective_rank`, `rank_cap`) and match state with `.heartseed` GM commands if relevant.
   - Read `nearby_bots` (`strategies`, `last_action`, `last_decision`) to inspect bot behavior at the moment of capture.
   - If `screenshot_path` is set, check `<WoW Client>/Screenshots/<screenshot_path>.jpg` (or `.tga`) for visual context.
3. **Resolve and clean up:**
   - Reference the ticket ID in your fix commit or status fragment if applicable, then remove or archive the local ticket file once resolved.

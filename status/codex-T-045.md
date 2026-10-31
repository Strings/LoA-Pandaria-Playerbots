# T-045: DPS-focused twink gear hotfixes

**Agent:** codex
**Branch:** codex/T-032-live-server-mcp
**Task status:** review

## What changed
- docs/twinks.md: 1,711 database candidate rows across 14 bracket endpoints/breakpoints, reproducible read-only shortlist query, comparator policy and 314-item audit.
- 2026_10_31_codex_00_twink_gear.sql: absolute-value stat changes and Item-sparse notices. Random property/suffix items excluded; requirements, sockets and procs retained.
- Offensive stat floors and weapon DPS target matching heirlooms +7%; stamina values stay stock. New offensive stats use spare slots. Allocation budgets account for final item level/socket costs.

## How it was tested
- Read local DBC scaling/damage/property-point curves with the same formulas as Player/Item; asserted offensive floors, weapon DPS floor and allocation reconstruction within one point.
- Applied SQL twice by hand to world_codex; complete selected item/hotfix snapshots identical (SHA256 06ade03e27619554bceeb30e158f208c390a89e44a735d4aedaadb787f7749af).
- No new custom tables; T-037 migration harness does not exist yet.
- Restart loaded 315 notices (one existing + 314 new). Authenticated headless login received the notice and requested Shadowfang Item-sparse data; asserted item level 38 and added crit rating 4. Reproducible smoke script in tools/live-server-mcp/.
- No full build (user requested serialization with Claude). No C++ changes.

## Decisions made
- User chose 7%, weighted toward damage. Comparator weights: stamina .25, crit/haste 1.25, attack power .5, other supported offense 1. Stat floors preserve stat types; spell power is not converted into intellect.
- This is a stat-budget heuristic, not simulated +7% character DPS; acquisition and proc/socket optimization are documented limitations. The catalogue is not an exhaustive per-spec BiS claim.

## For the other agent
- Main/Claude DBs unchanged; manually apply the SQL after merging and restart worldserver. Updates.EnableDatabases remains 0.
- Item-sparse table hash 2442913102 verified from the DB2 header; existing core provides the hotfix transport.

## Next
- Human review and client tooltip playtest; refine class-specific acquisition, proc and socket choices with balance tooling.

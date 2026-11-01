# Legacy solo dungeon travel (T-046)

Specific normal/heroic five-player dungeons can be entered solo when the player's level is at least the native upper cap plus `LFG.LegacySolo.LevelsOverCap` (20). If no upper cap exists, use the native minimum. This is the accepted interim lookup until the Phase 6 scaling cap table exists. Below-margin bot parties await the Phase 5 instance pool.

`LFG.LegacySolo.Enable=1` enables new entries. `LFG.LegacySolo.IntroOnLogin=1` introduces eligible humans on login/first eligible level. Invalid/nonpositive margins use 20. No duration is added.

Public `.legacy list`, `status [id]`, `enter <id>` and `out` use self. GM permission 96001 allows selected-player targets and `force <id>` bypassing the level margin; native access/lock restrictions and travel safety remain. Permission 96000 is linked to Player Commands (199), 96001 to GM Commands (197).

The DungeonFinding facade delegates upper-level lock, join and native teleport requests to the adapter. Native entrance/access/lock checks apply; one specific dungeon, no group, no existing queue, combat/flight/vehicle safety. It creates no matchmaking/reward queue. The custom character_legacy_trip table stores original world location and difficulty before transfer. Login reloads the anchor; completed return/portal/hearth arrivals or character deletion remove it. An existing trip can return with the entry key disabled. Normal battleground return data is not reused.

AutoBalance recognizes an active legacy visit, leaving creatures at original difficulty, and refreshes/reverts on visit changes. Corruption remains a separate system. Core hooks require human review/merge.

SQL: characters 2026_11_01_codex_06_legacy_dungeons.sql; auth 2026_11_01_codex_07_legacy_dungeons.sql. Apply manually with Updates.EnableDatabases=0. This state table is read at login and inspected through commands; it is not reloadable design data. Config reload refreshes native lock caches.

Pure rules live in SoloDungeonPolicy.h and tests/legacy_dungeons. Production build/live/migration checks remain pending; status/codex-T-046.md records the current recovery point. T-041/T-044 verification has priority while this is prepared in the secondary checkout.

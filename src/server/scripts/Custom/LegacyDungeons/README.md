# Legacy solo dungeon travel (T-046)

Preparation while the earlier tasks wait for the shared build queue. T-041 live verification takes priority when its primary build completes.

Design: specific outleveled 5-man dungeons enter solo at a configurable margin (default 20), without matchmaking/group rewards. Below the margin, bot parties remain deferred until the instance pool. Use the native DBC upper level (minimum if no upper level) as the accepted interim cap until the scaling table exists.

Planned interface: narrow DungeonFinding callbacks for high-level locks, join and return; custom adapter owns eligibility, saved return anchor and GM/player commands. Reuse native entrances/access checks and the configurable scaling module, keeping legacy runs at original difficulty. New state stays in a custom character table; core tables are not altered.

No implementation or build/live pass is claimed yet. Next: audit exact APIs, pure policy, minimal hooks/adapter/SQL, copy-of-save checks, normal build and native scenarios. No new duration.

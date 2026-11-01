# Raid retention

`Raid.Retention.Enable = 1` selects the new raid-only lifecycle. Set 0 for the
legacy global cleanup path. `Raid.ResetPeriodSeconds = 172800` defines the shared
map/difficulty cadence; any positive number of seconds is supported for tests.
Invalid periods fall back to 172800. Restart applies a new period immediately;
reload affects subsequent cycles. Non-raid resets remain unchanged.

At each deadline, unstarted and fully completed raid saves reset. An active boss
fight defers the boundary even before the first encounter credit. Partial clears
keep script state, respawns and player/group binds across empty periods/restarts.
Both ordinary encounter credits and script BossState saves are covered: the
shipped world data omits MoP raid credit definitions. Script completion uses
existing boss indices, LFGDungeons.dbc wing IDs (excluding off-wing DONE states),
and excludes heroic-only Ra-den in normal Throne of Thunder. No raid scripts or
canonical encounter bits are rewritten.

Progress and script state commit together. Existing instance.resettime carries
metadata: 0 = no saved progress, UINT32_MAX = partial script/encounter progress,
other positive values = a finished raid's deadline. Wing completion remains
finished across subsequent script saves. Client/save deadlines always use the
map/difficulty cadence. Legacy scheduling also recognizes the reserved partial
marker when the feature is disabled. Unknown definitions preserve known progress
conservatively; listing exposes expected=0 and script-progress for inspection.

Players use `.raidprogress list` and `.raidprogress abandon <instance id>`.
Abandonment clears that instance and ALL its binds: a bound group's current
leader, or an ungrouped owner of its personal bind, may do it alone. Bot sessions
cannot initiate abandonment and no member-consent/vote mechanism exists, so bots
never block a human leader. Occupied maps use the existing global-reset eviction
and unload path; IDs are not freed while a loaded map still exists.

GM `.raidprogress list` supports a selected player. `.raidprogress force-reset
<id>` and `.raidprogress force-abandon <id>` bypass ownership and progress, and
allow SOAP/console with existing instance-unbind permission. They are deliberate
aliases for removing a raid save and all bindings. Non-raid IDs are rejected.
GM `current` may replace the reset/abandon ID while inside a raid.
`.raidprogress credit <creature entry>` runs the normal encounter-credit path
inside the current raid for progress/completion verification.
`.raidprogress boss <index> [not-started|in-progress|done]` inspects or drives the
normal script BossState/save path, including MoP raids without credit rows.
`.raidprogress status` reports the enabled flag and effective period.
`Raid.Retention.NotifyOnEnter = 1` explains retention on human raid entry.

No table/schema migration: existing instance masks, instance_reset deadlines and
bind tables are used. The human confirmed no existing raid saves at rollout.

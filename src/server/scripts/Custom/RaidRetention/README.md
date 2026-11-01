# Raid retention

`Raid.Retention.Enable = 1` selects the new raid-only lifecycle. Set 0 for the
legacy global cleanup path. `Raid.ResetPeriodSeconds = 172800` defines the shared
map/difficulty cadence; any positive number of seconds is supported for tests.
Invalid periods fall back to 172800. Restart applies a new period immediately;
reload affects subsequent cycles. Non-raid resets remain unchanged.

At each deadline, unstarted and fully completed raid saves reset. An active boss
fight defers the boundary even before the first encounter credit. A nonzero partial
completed-encounter mask without a completion marker keeps its save, respawns and player/group binds, even
when empty or across restarts. Completion masks use existing world
instance_encounters rows and DBC indices, with the core's difficulty expansion.
LFR wing finals mark completion without requiring other wings' bosses. Unknown
definitions retain nonzero progress conservatively; GM listing exposes expected=0
for data repair. Encounter credits atomically save the mask, script state and a
completion marker in existing instance.resettime (positive = finished deadline,
zero = unfinished). This also covers credits arriving after a boss's DONE save.

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
`.raidprogress status` reports the enabled flag and effective period.
`Raid.Retention.NotifyOnEnter = 1` explains retention on human raid entry.

No table/schema migration: existing instance masks, instance_reset deadlines and
bind tables are used. The human confirmed no existing raid saves at rollout.

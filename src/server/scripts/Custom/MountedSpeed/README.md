# Mounted speed

`Rate.MountedSpeed` multiplies players' mounted run/flying speed bonus, default
2.4: 60/100/150/280/310% riding becomes 144/240/360/672/744% above ordinary
movement. Set 1.0 for stock speed. Values must be finite and positive; invalid
values fall back to 2.4 with a startup/reload error. Config reload updates the
cached multiplier; remount or use `.mountspeed` to refresh an existing mount.

The core applies the bonus before normalization and slows. Unmounted characters,
ghost wolf, druid travel/flight forms, taxi flights, swimming/backward movement,
non-player units keep their existing paths.

`Death.GhostSpeed` (default 2.0) is the least a ghost runs and swims at, against
the Ghost aura's stock 150%; faster effects keep their speed and 0 turns it off.

`Taxi.SpeedOfTopFlight` (default 2.0) makes flight paths that many times as fast as Master Riding flying
(about 118 yd/s at the shipped settings, against the stock 32); the spline speed cap skips flight paths.

GM `.mountspeed` recalculates and reports the selected player's run/flight rates,
mount/taxi state and effective multiplier (defaults to self; reload permission).
Unit tests cover riding tiers, stock/custom multipliers and invalid input.
No data tables or client changes are required.

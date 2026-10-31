# T-010: Mounted movement speed

**Agent:** codex
**Branch:** codex/T-010-mount-speed
**Task status:** review

## What changed
- Rate.MountedSpeed defaults to 2.0 and multiplies the mounted movement bonus: +60/+100/+150/+280/+310% becomes +120/+200/+300/+560/+620%. Set 1.0 for stock behavior.
- Core run/flight update applies it only to mounted players, before normalization/slows. Ghost wolf, travel/flight forms, taxis, swimming/backwards and creatures retain their paths.
- Cached config reload, finite/positive validation, and a GM .mountspeed force/inspect probe. No World.h layout changes.

## How it was tested
- Release worldserver build with no warnings in touched files.
- CTest: 6 suites pass, including five riding tiers, stock/custom rates and invalid multipliers. Developer-environment tests: 4 pass.
- Codex test/none live checks compare Rate.MountedSpeed=1 and 2 on ground/flying mounts and confirm identical unmounted/ghost-wolf/druid-travel/druid-flight rates. Invalid 0 falls back to 2.0; reload and headless regression pass.
- Observed ground run rate 2.0 -> 3.0 and mounted flight 2.5 -> 4.0. Ghost wolf 1.3, travel 1.4 and flight-form flight 2.5 were unchanged.
- Live evidence: build/T010-stock.log, T010-doubled.log, T010-invalid.log, and build/regression/T010-login.

## Decisions made
- Followed the decided riding percentages by multiplying the bonus above base movement, not doubling base movement itself. No provisional form/taxi bonus implemented.

## For the other agent
- Independent of T-019; no Heartseed source edits, new SQL, or shared runtime changes. Claude reviews; leave PR open.
- Reload updates the multiplier; remount or run .mountspeed to refresh an already active mount. GM probe defaults to selected player/self, with reload-config permission.

## Next
- Claude review, then human merge.
- Old private world process PID 32728 hung after shutdown; Windows denied termination. Its ports were closed. Live validation uses the built worldserver-T010.exe copy in the same private runtime, on the assigned Codex ports. No shared processes were stopped.

## Map additions
- Entities/Unit/MountedSpeed.{h,cpp}: pure bonus math plus cached config.
- Custom/MountedSpeed/: GM probe and config/interface README; tests/mounted_speed/: pure unit tests.

# T-080 follow-up: Call of Xuen tuning and lightning visual

**Agent:** claude
**Branch:** claude/T-080b-xuen-tuning
**Task status:** done

## What changed
- `heartseed_gift` gains `duration`; cooldown and duration now replace the client spell's own (0 keeps it).
- Call of Xuen: 20 s cooldown, 10 s tiger (human's call: part of the rotation, not all the time).
- Xuen's Crackling Tiger Lightning plays Chain Lightning's visual to each target (`Heartseed.Gift.XuenLightningVisual`,
  default 36, 0 = off). Applies to monks' Xuen too.

## How it was tested
- New scenario `heartseed-xuen` (cast, refused on cooldown, tiger gone by 12 s, cast again) plus `heartseed-gifts`
  and `heartseed-sync` pass on the claude devenv. The visual itself needs a look in the client.

## For the other agent
- Apply `sql/updates/world/2026_11_01_claude_07_heartseed_gift_duration.sql` (rebuilds `heartseed_gift`).
- New config key `Heartseed.Gift.XuenLightningVisual`.

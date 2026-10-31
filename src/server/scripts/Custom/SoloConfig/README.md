# Solo configuration probes

GM commands use the selected player, or yourself when no player is selected.
They require reload-config permission and an in-game session.

- `.solo info`: each loaded solo setting, login taxi access, reclaim delays, Valor/Conquest weekly caps and fishing skill.
- `.solo fishskill`: run one normal fishing skill-up check, reporting gain/cap.
- `.solo award <currency id> <raw amount>`: a regular positive currency award,
  respecting normal multipliers and limits. Raw amounts use the currency's
  internal precision (for Valor/Conquest, 150000 means 1500 displayed points).

These are state-changing test commands. Use disposable characters in your own
test profile. They make weekly-cap and fishing behavior testable without
playing to a reward or fishing catch.

The small game-side World/SoloConfig module caches both flags in one atomic
value, refreshed by World::LoadConfigSettings. Player uses the accessors without
adding fields or enum slots to World.h. Missing keys retain legacy behavior:
weekly caps enabled, ordinary fishing chances. The shipped solo defaults turn
weekly caps off and guaranteed fishing skill-ups on. Learned skill caps and
currency holding limits continue to apply.

With a GM fixture logged into the test/none profile, run the per-key live check:
`python tests/regression/run.py --runtime build/devenv --suite tools/devenv/solo_smoke.json`.

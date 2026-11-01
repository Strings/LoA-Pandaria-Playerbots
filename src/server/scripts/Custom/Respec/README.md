# Free talent, glyph and specialization changes

Defaults: `NoResetTalentsCost = 1`, `Respec.FreeReagents = 1`,
`Respec.AutoDualSpec = 1`, and existing `MinDualSpecLevel = 10`.

Gold-free reset quotes and actual talent/specialization resets use the existing
core cost key. The reagent exemption is restricted to APPLY_GLYPH and
REMOVE_TALENT spell effects, so both validation and consumption use the same
CanNoReagentCast decision. Learning glyphs and unrelated crafting/combat
reagents remain normal. Level, combat, cooldown and challenge-mode restrictions
stay intact.

Human sessions unlock the standard two client talent groups on login and level
change at the configured minimum (never before 10), through the existing trainer
learning spells. Existing higher-level characters are supported. BotFactory
continues to manage bot talent groups. Disabling auto unlock does not remove
already learned dual spec. Reload affects future checks/unlocks; no timers.

GM commands (selected player or self, reload-config permission):
- `.respec info`: costs/config, specs, glyph slots and reagent-exemption probes.
- `.respec reset talents|spec`: normal paid-reset path, reporting actual charge.
- `.respec learn <talent id>`: normal level/class/row-checked talent learn.
- `.respec cast <spell id> <index>`: normal, non-triggered glyph/talent-change or
  spec-switch spell. Index is glyph slot or talent id; ordinary spell checks and
  reagent consumption apply. Inspect state after completion; request != success.

Pure unlock policy tests live in tests/respec. No SQL or client patch is required.

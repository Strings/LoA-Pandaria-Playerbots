# T-080: Celestial Gifts and the Heartseed panel

**Agent:** claude
**Branch:** claude/T-080-celestial-gifts
**Task status:** review

## What changed
- The level 10 major is now an active Celestial Gift (DESIGN.md updated, decided by the human): Call of Xuen
  (Invoke Xuen 123904), Crane Rush (Chi Torpedo 115008), Ox Stomp (Leg Sweep 119381), Serpent's Breath (Celestial
  Breath 117960). New `heartseed_gift` table; the old stat majors moved to minors 9 to 12. Trait ids 2, 4, 6, 8
  stay the majors, so existing loadouts need no migration.
- `RefreshTraits` teaches the active Gift and unlearns the rest (a monk's own talent is kept). Gift cooldowns and
  Serpent's Breath damage come from data via two spell scripts.
- `.seed sync` M lines add the Gift spell id and the trait description.
- Heartseed addon redesign: mandala with the four Celestials at their compass points, path-tinted pulsing seed,
  castable Gift button with cooldown, major nodes, sounds, art slots with built-in fallbacks.
- `tools/art-slicer/` and `addons/Heartseed/art/` (prompt sheet and manifest) for generated art.

## How it was tested
- New regression scenario `heartseed-gifts` (teach, cast and swap all four Gifts, revoke) and `heartseed-sync`
  pass on the claude devenv; revoke leaves no Gift spells in `character_spell`.
- Addon: 8 lupa tests. Art slicer: 4 unit tests.
- SQL applied twice to `world_claude` (idempotent).

## Decisions made
- Spell choices and cooldowns (Xuen keeps its 3 minutes; Crane Rush 60 s, Ox Stomp 90 s, Serpent's Breath 60 s).
  Tuning is placeholder until playtests.
- Gift cooldowns are data (`heartseed_gift.cooldown`), not config keys, like the other trait numbers.

## For the other agent
- Apply `sql/updates/world/2026_11_01_claude_06_heartseed_gifts.sql` to your world DB if you test the Heartseed.
- Spell tooltips show the borrowed spells' client text until a client patch; the panel shows ours.

## Next
- Gift riders per stage (Sapling, Bloom, Full Bloom); later majors that modify the Gift.
- Generated art: drop files in `addons/Heartseed/art/`, set `USE_ART = true`.

## Map additions
- `tools/art-slicer/` (added).

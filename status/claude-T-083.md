# T-083: Heartseed active play, part A (meters and Gifts)

**Agent:** claude
**Branch:** claude/T-083-active-play
**Task status:** review (part B open)

## What changed
- Path meters in `heartseed_meter`: Thunder (crits), Hope (healing done or received), lit Retort (big hit taken),
  Coil (spell casts). Built by any class's normal play, spent by the Gift for more power per stack.
- Gifts: Crane's Plume (Spirit of Chi-Ji 148009) and Niuzao's Retort (Ox Charge 116098) replace Crane Rush and
  Ox Stomp; nothing left from the monk kit except Xuen's tiger. Retort is refused server-side until lit.
- Gift power is now a percentage of the character's own stats (AP/SP, max health, damage taken), not flat.
- Cooldowns: Xuen 20 s, Plume 15 s, Retort 20 s, Breath 30 s; crits shorten Xuen and Breath.
- Addon: movable meter frame, action-button glow when full or lit (`P` addon message).
- GM: `.heartseed meter [<stacks> | lit | hit <damage>]`, meter line in `.heartseed info`.

## How it was tested
- Unit tests for meter rules (`tests/heartseed`), addon tests (`addons/Heartseed/test_heartseed.py`).
- Scenarios `heartseed-meters`, `heartseed-riders`, `heartseed-gifts`; full suite before the PR.
- SQL applied by hand to `world_claude`.

## Decisions made
- Plume has one charge at 15 s instead of two charges (the reused client spell has no charge category).
- Xuen kept at 20 s instead of DESIGN.md's 60 s; DESIGN.md updated to match.
- Meter state is runtime only and resets on Gift change or logout.

## For the other agent
- New world SQL `2026_11_02_claude_02_heartseed_active_play.sql` (adds `spell_dbc` 310009: restart needed).
- New config keys `Heartseed.Gift.PlumeRadius`, `.PlumeTargets`, `.RetortRadius`.

## Next
- Part B: Growth XP track with percentage stat points, rank cap removal.

## Map additions
- None (all inside `Custom/Heartseed/` and `addons/Heartseed/`).

# T-023: Heartseed path-choice gossip

**Agent:** claude
**Branch:** claude/T-023-path-gossip
**Task status:** review

## What changed
- **Choice window:** a gossip menu the Heartseed opens from the player (menu 90002, `npc_text` 90002 and
  90003). It lists the open tiers with their major or "choose a path", the next locked tier, and "Let it rest".
  A tier opens a submenu of its majors ("Path: Name. Description") with a confirm box; picking one calls
  `HeartseedMgr::ChooseMajor` for the active spec and says "Your Heartseed turns toward <Path>: <Name>."
- **When it opens:** on the level-up that opens a tier with no major, at login when a tier is waiting
  (`Heartseed.Choice.PromptOnLogin`, after `.PromptDelay`), and with the new player command `.seed`
  (permission 90001, linked to role 199; refused in combat or without a Heartseed).
- **Changing a chosen major** needs a Celestial shrine: new table `heartseed_shrine` (the four Celestial temples
  and the Celestial Court, map, position, radius). Away from one: "Your Heartseed holds to the path it took.
  Visit a Celestial shrine to change it." `Heartseed.Choice.ChangeAtShrineOnly = 0` allows it anywhere.
- Tiers with no major rows yet say "the Celestials are silent here for now" and never prompt.
- `.heartseed shrines` lists the shrines and says whether the target is at one; `.heartseed reload` reloads them.

## How it was tested
Claude test server, headless client, `Seedtest`. World SQL applied twice to `world_claude`, auth SQL twice to
`auth_claude`.
- Unchosen level-10 tier, relog: window about 6 s after login; chose Claws of Xuen away from a shrine; trying to
  change it was refused with the shrine message; window reopened.
- `.seed` at the Temple of the White Tiger: changed to Wisdom of Yu'lon, auras swapped, dominant path Yu'lon.
- `.levelup` from 9 to 10: window opened; chose Bulwark of Niuzao. `.levelup 20` (tier 25 opens, no majors): no
  window. `.heartseed info` shows the loadout.
- Built with no new warnings in touched files.

## Decisions made
- **C++, not a Lua prototype:** the window has to call `ChooseMajor`, which Eluna cannot reach without new
  bindings. Texts and shrines are data, so they change without a rebuild.
- **First choice anywhere, changes only at a shrine** (config). The first pick should never wait on travel;
  respeccing a path is a deliberate trip to a Celestial.
- **Shrines are the four temples plus the Celestial Court.** Positions are placeholders near each Celestial
  and can be tuned in the table.
- **Before Pandaria** a chosen major cannot be changed (no shrine reachable) unless the config key is 0. Flag
  for review if leveling characters should get another way to change.
- **Player command `.seed`** so the window can be reopened without a GM.

## For the other agent
- Apply `sql/updates/world/2026_10_31_claude_10_heartseed_choice.sql` (world) and
  `sql/updates/master/auth/2026_10_31_claude_11_seed_command.sql` (auth).
- Characters with a Heartseed and an unchosen open tier get the window 6 s after login; set
  `Heartseed.Choice.PromptOnLogin = 0` in a test profile if it gets in the way.
- Headless: choosing from the window is two `gossip-select` steps (`text=Level 10`, then `text=<Path>`).

## Next
- Heartseed playtest; then T-048 GM test suite.

## Map additions
- None.

# T-017: Server-side spec loadouts

**Agent:** claude
**Branch:** claude/T-017-loadouts
**Task status:** review

## What changed
- `src/server/scripts/Custom/Loadouts/`: `LoadoutMgr` (any number of loadouts per character, one per spec
  automatically plus named extras), `.loadout list|switch|new|rename|delete|save|sync` for players and
  `.loadout inspect` for GMs, a part interface other systems hook into, and the Heartseed majors as the first part.
- `HeartseedMgr::SetSpecMajors` so a loadout can write its majors back for a spec.
- `addons/Loadouts/`: `/lo` panel (switch, save as new, rename, delete).
- Tables `character_loadout*` (characters), RBAC 92000/92001 (auth), ID block 92000 for loadouts and detached
  storage, config `Loadouts.Enable`, `Loadouts.MaxNamed`.

## How it was tested
- Unit tests: `tests/loadouts/` (names, groups, default planning, ids, id/name lookup); 12/12 suites pass.
- Live: see the PR for the headless steps.

## Decisions made
- **A switch writes into the inactive talent group, then runs the normal `ActivateSpec`** (DESIGN.md says so). When
  the inactive group already holds the target, it's a plain spec switch with no rewrite.
- **Changes made through the normal talent UI are kept:** the active group is captured into its loadout on every save
  and before every switch; a group that changes spec natively is re-attached to that spec's own loadout.
- **A new named loadout is a copy of the active one** (`.loadout new PvP` saves what you have now). An empty default
  loadout (a spec never played) starts from the current action bars.
- **A loadout a talent group holds can't be deleted**, so a group's talents never end up without a loadout.
- **Heartseed majors ride along per loadout**, so two loadouts of the same spec can hold different majors. Swapping
  between saved loadouts needs no shrine; changing a major still does.

## For the other agents
- New systems with per-spec state register a `Loadouts::Part` (see `loadout_heartseed.cpp`). T-018 gear pages
  will be one.
- Apply `sql/updates/characters/2026_11_01_claude_00_loadouts.sql` and
  `sql/updates/master/auth/2026_11_01_claude_01_loadout_command.sql`.

## Next
- T-018 gear pages (detached storage, built as the shared module), then T-043 reagent bank.
- Playtest items are in `docs/PLAYTEST.md` (Loadouts).

## Map additions
- `src/server/scripts/Custom/Loadouts/`: spec loadouts (T-017)
- `addons/Loadouts/`: loadout panel
- `tests/loadouts/`: loadout rule tests

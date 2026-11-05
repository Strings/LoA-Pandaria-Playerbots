# T-096: Profession tome pet

**Agent:** claude
**Branch:** claude/T-096-profession-tome
**Task status:** review

## What changed
- New `src/server/scripts/Custom/ProfessionTome/`: the stock Gusting Grimoire journal pet (creature 69208,
  species 1174) trains every primary and secondary profession at every rank when summoned. Gossip lists one
  "Teach me <profession>" line per skill and opens that profession's trainer page.
- Account-wide through the pet journal: a human character's first login adds the pet if the account lacks it.
- World SQL gives 69208 gossip and trainer flags and builds its `npc_trainer` list as the union of every stock
  trainer's rows for the 15 profession skills (2,134 spells on world_claude).
- Small core helper `TrainerListPacket::Send` (new header in `game/Handlers/`), split out of
  `WorldSession::SendTrainerList` so a script can send a filtered list. No shared header edited.
- `.proftome status|grant|summon|check <spell>` (RBAC 101000, auth SQL update).
- Headless client: `trainer-buy SPELL [guid=]` verb; SMSG_TRAINER_BUY_FAILED and SMSG_LEARNED_SPELL logged.
- Regression scenario `profession-tome` and unit test `unit_profession_tome`.

## How it was tested
- SQL applied twice to world_claude and auth_claude with the same result; sql-validator 0 issues.
- See the PR for the build, unit test and scenario run.

## Decisions made
- Reused the stock Gusting Grimoire instead of a custom species: MoP companions live in the client's
  BattlePetSpecies DB2, so a custom pet needs client rows; the stock one works with no patch. Custom art and
  name are in ART-NEEDED.
- Normal trainer costs and requirements apply; anyone can use a summoned tome.
- Trainer creature lists at or above id 200000 are ignored by the core (`TRINITY_TRAINER_START_REF`), so the
  list lives on 69208 itself and the script filters it per profession.

## For the other agent
- Config: `ProfessionTome.Enable`, `ProfessionTome.GrantOnFirstLogin` (worldserver.conf.dist).
- Apply `sql/updates/world/2026_11_05_claude_10_profession_tome.sql` and
  `sql/updates/master/auth/2026_11_05_claude_11_profession_tome_command.sql`.
- ID block 101000 (Profession tome) allocated in docs/ID_RANGES.md; next free is 102000.
- The scenario needs a fixture character Tometest (see tests/regression/README.md).

## Next
- Custom tome model, name and journal entry once the client patch can add BattlePetSpecies rows.

## Map additions
- `src/server/scripts/Custom/ProfessionTome/`, `tests/profession_tome/`, `src/server/game/Handlers/TrainerListPacket.h`

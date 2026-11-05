# Profession tome (T-096)

A companion pet that trains every profession wherever you are. Summon it from the pet journal, click it,
pick a profession, and its trainer window opens with every rank and recipe any stock trainer of that
profession teaches, at the normal cost and requirements.

## How it works

- **Pet:** the stock Gusting Grimoire (creature 69208, journal species 1174, summon spell 136484), an animated
  tome. The client already has its model, icon and journal entry, so no client patch is needed. Custom art is
  tracked in `docs/ART-NEEDED.md`.
- **Account-wide:** MoP keeps companions in the account's pet journal (`characters.account_battle_pet`), and
  every character on the account learns the summon spell from the journal at login. The first login of a
  human character adds the pet if the account lacks it (`FirstLoginHooks`). Bots are skipped.
- **Trainer:** `sql/updates/world/2026_11_05_claude_10_profession_tome.sql` gives 69208 gossip and trainer
  flags and fills its `npc_trainer` list with the union of every stock trainer's rows for the 15 profession
  skills (11 primaries plus Archaeology, Cooking, First Aid, Fishing). Re-running the update rebuilds the list
  from the stock rows.
- **Pages:** the gossip menu has one "Teach me <profession>" line per skill; choosing one sends that
  profession's slice of the list (`TrainerListPacket::Send`, a small core helper split out of
  `WorldSession::SendTrainerList`). Buying goes through the normal trainer handler, which checks the full list.
- Anyone can use a summoned tome, not only its owner.

## Config

| Key | Default | Effect |
|---|---|---|
| `ProfessionTome.Enable` | 1 | 0 closes the tome's menu; the pet stays in the journal |
| `ProfessionTome.GrantOnFirstLogin` | 1 | add the pet to the account's journal on a human character's first login |

Both are read live, so `.reload config` applies them. No timers.

## GM commands (`.proftome`, RBAC 101000)

Each acts on the selected player, or yourself.

- `status`: enabled, owned, summon spell known, summoned, size of the full list and of each profession page.
- `grant`: add the pet to the account's journal and teach the summon spell (no-op if already there).
- `summon`: summon it the way the journal does.
- `check <spell>`: whether the spell is on the tome's list and whether the player knows it.

## Tests

- Unit: `tests/profession_tome/` (profession table covers every primary, no duplicate skills).
- Regression: `tests/regression/scenarios/profession-tome.json` summons the tome, opens its Cooking page and
  buys Spice Bread with the headless `trainer-buy` verb. It logs in as the fixture character `Tometest`.

# Checkpoints and kits (GM test suite, T-048)

Jump a character to a known point in the game in one command, so a playtest starts where it needs to instead of
being played to. The GMSuite addon (`addons/GMSuite/`) puts these commands on buttons.

## Interface

- **Checkpoint:** a named, ordered list of typed steps (`gmtools_checkpoint`, `gmtools_checkpoint_step`).
  `CheckpointMgr::Apply` runs the steps in order on the target and reports each one.
- **Step types are registered by systems** (`CheckpointMgr::RegisterStep(type, handler, explainer)`), so a
  later system adds its own fields to the template without a schema change. `checkpoint_heartseed.cpp` is the
  pattern: the Heartseed's step types live beside the engine, not inside it.
- **Kit:** a named list of items (`gmtools_kit`, `gmtools_kit_item`), per class through `class_mask`, equipped
  (`equip = 1`) or put in the bags. Bag items only fill empty bag slots, so applying a kit twice is safe.

## Step types

| Type | Args | Does |
|---|---|---|
| `level` | arg1 level | Sets the level (down works too) and clears experience |
| `class_spells` | | `.learn all my class` (only when applied to yourself) |
| `gear` | arg1 quality (2 green, 3 blue, 4 epic), text `nocap` | Playerbots gear factory via `.botscenario <name> equip`; upgrades only |
| `kit` | text kit name | Applies a kit |
| `item` | arg1 entry, arg2 count | Adds an item |
| `spell` | arg1 spell | Learns a spell |
| `quest` | arg1 quest | Completes and turns in a quest; put it before `level`, which resets the experience |
| `reputation` | arg1 faction, arg2 standing | Sets reputation (3000 friendly, 9000 honored, 21000 revered, 42000 exalted) |
| `tele` | text `game_tele` name, or `Alliance|Horde` | Teleports |
| `command` | text, `$name` = target | Runs any GM command; the escape hatch until a system has its own step |
| `heartseed` | | Grants a Heartseed if there is none |
| `heartseed_rank`, `heartseed_growth` | arg1 | Sets rank (ignores the cap) or banked Growth |
| `heartseed_major` | arg1 tier level, arg2 trait (0 clears) | Sets or clears a major in the active spec |
| `heartseed_flag` | text flag name (or arg1 id), arg2 1 set / 0 clear | Story flags; awakenings raise the cap |
| `heartseed_essence` | arg1 essence, arg2 slot (0 = just add) | Gives and slots an essence |

## Shipped checkpoints

`hs-10` to `hs-85` at each Heartseed major tier, `legendary-1` (60) and `legendary-2` (80) at placeholder levels
until the pacing pass, and `max-90`. Each grants a Heartseed, sets the level, learns class spells, fills the bag
slots, equips green (blue from 60, epic at 90) gear, sets rank to the cap at that point (awakenings assumed at 15,
85 and 90), sets every story flag explicitly, clears its own tier's major so the choice window asks, and
teleports to the faction capital (`max-90`: the faction shrine). Twink kits per bracket come with T-045.

## Commands

- `.checkpoint list | explain <name> | apply <name> | reload` (permission 91000)
- `.kit list | explain <name> | apply <name>` (91001)
- `.gmsuite refresh`: ability tester; resets cooldowns, fills health and every power, revives (91002)
- `.gmsuite sync`: sends the lists to the GMSuite addon as addon messages, prefix `GMSuite`:
  `C\t<name>\t<category>\t<description>`, `K\t<name>\t<min>-<max>\t<description>`, then `E`.

Commands apply to the selected player, or to you. The `gear` step needs permission 1345 (`.botscenario`), linked
to Administrator Commands by the T-048 auth update.

## Data

World: `sql/updates/world/2026_10_31_claude_12_gmtools_checkpoint.sql`. Auth:
`sql/updates/master/auth/2026_10_31_claude_13_gmtools_commands.sql`. No config keys.

## Later

- Telemetry (T-004) pauses around a checkpoint apply (DESIGN.md, Keeping test play out); hook it in `Apply`.
- `.reload` integration once T-038 lands; `.checkpoint reload` covers it until then.

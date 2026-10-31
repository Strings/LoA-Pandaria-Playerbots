# Heartseed

The living jewel that rides on a character's main-hand weapon and grows with it. Design:
`docs/DESIGN.md`, "The Heartseed". IDs come from the Heartseed block in `docs/ID_RANGES.md`.

## Files

| File | What |
|---|---|
| `HeartseedRules.h` | Pure rules with no server dependencies (stage by level, dominant path, tier levels, minor activation, trait and core passive amounts, rank cap, Growth per rank, feeding Growth, essence slots). Unit-test these. |
| `HeartseedMgr.h/.cpp` | State cache per character, loading and saving, stage and glow lookup, traits, major choice, core and essence procs, Growth and rank-ups, feeding, essences. |
| `heartseed_scripts.cpp` | World, player, unit and command scripts. |

## Data

Characters database (`sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql`):

- `heartseed_state`: one row per character that has a Heartseed (rank, Growth, grant time). Having a
  row is what "has a Heartseed" means to code; the granting quest (T-025) creates it.
- `heartseed_loadout`: the major chosen at each of the six major tiers, per specialization.
- `heartseed_essence`: essences collected and which slot (1 to 3) each is active in.
- `heartseed_story_flag`: story beats reached and when (`Heartseed::StoryFlag`).

Characters database (`sql/updates/characters/2026_10_31_claude_05_heartseed_fed_weapon.sql`):

- `heartseed_fed_weapon`: every weapon entry the Heartseed has eaten, how often and when first.

Characters database (`sql/updates/characters/2026_10_31_claude_08_weapon_appearance.sql`):

- `character_weapon_appearance`: weapon appearances unlocked by feeding (source 1) or deleting (source 2) a
  weapon. Only recorded here; the appearance collection (transmog) reads it later.

World database (`sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql`):

- `heartseed_stage`: stage number, starting character level, name.
- `heartseed_glow`: glow enchant per stage and dominant path (path 0 = none).

World database (`sql/updates/world/2026_10_31_claude_09_heartseed_grant_quest.sql`): quest 90000, the granting quest.

World database (`sql/updates/world/2026_10_31_claude_10_heartseed_choice.sql`): `heartseed_shrine` (Celestial
shrines: map, position, radius, name) and the choice window's texts. Auth
(`sql/updates/master/auth/2026_10_31_claude_11_seed_command.sql`): permission 90001 for `.seed`, linked to
role 199 (Player Commands).

World database (`sql/updates/world/2026_10_31_claude_03_heartseed_trait.sql`):

- `heartseed_trait`: one row per trait: path, kind (1 minor, 2 major), major tier or minor rank, the
  passive aura spell it applies, and its amount (`base_amount + amount_per_ilvl * main-hand item level`,
  written to the spell's effect 0).
- The aura spells are server-side rows in `spell_dbc`, `spellmisc_dbc` and `spelleffect_dbc`, in the
  Heartseed spell block (310000+). The server reads those tables at startup only.

World database (`sql/updates/world/2026_10_31_claude_04_heartseed_essence.sql`):

- `heartseed_essence_template`: essence types: name, the spell shown for its proc, proc chance and
  damage as a share of weapon damage. Placeholders: 1 Sha-touched (shadow), 2 Mogu (lightning).
- `heartseed_essence_source`: which fed weapons can leave an essence, by item entry or by a text in the
  item name (`sha-`, `mogu`), and the chance per weapon.

World database (`sql/updates/world/2026_10_31_claude_06_heartseed_feed_prompt.sql`,
`2026_10_31_claude_07_twink_item.sql`):

- `npc_text` 90001: the feed prompt's text.
- `twink_item`: item entries that are twink gear (seeded from Codex's T-045 twink gear list). Not
  Heartseed-specific; other systems that consume gear should honor it too.

## Growth, rank and the cap

- **Growth** is earned through `HeartseedMgr::AddGrowth` (feeding now; quests, rares, events and dungeons
  later). It is spent on ranks: rank R to R + 1 costs `Heartseed.Rank.GrowthBase + GrowthPerRank * (R - 1)`.
- **Rank cap** = `level * Heartseed.Rank.CapPerLevel + awakenings * Heartseed.Rank.CapPerAwakening`, at least
  1. Awakenings are the story flags listed in `Heartseed.Rank.AwakeningFlags` that the character has.
- Growth past the cap is **banked**. A level-up, a new awakening or a login spends it (`ApplyRankUps`).
- GM `.heartseed rank` and a bare `.heartseed growth N` set values directly and ignore the cap.

## Choosing majors

A gossip window the Heartseed opens from the player (menu 90002, texts 90002 and 90003 in `npc_text`):
the open tiers with their major or "choose a path", the next locked tier, then a tier's majors with a
confirm box. Picking one calls `HeartseedMgr::ChooseMajor` for the active specialization.

- **Opens** when a level opens a tier with no major, at login when one is waiting
  (`Heartseed.Choice.PromptOnLogin`, after `.PromptDelay`), and with the player command `.seed`.
- **A tier with no major yet can be chosen anywhere.** Changing a chosen major needs a Celestial shrine
  (`heartseed_shrine`: the four Celestial temples and the Celestial Court) unless
  `Heartseed.Choice.ChangeAtShrineOnly = 0`.
- A tier with no major rows says the Celestials are silent there and never prompts.
- C++, not a Lua prototype: the window calls `ChooseMajor`, which Eluna cannot reach without new bindings.
  The texts are `npc_text` rows and the shrines are data, so both change without a rebuild.

## Granting quest

Quest 90000 "The Heartseed" (`sql/updates/world/2026_10_31_claude_09_heartseed_grant_quest.sql`) is the
canonical "has a Heartseed" marker: rewarding it grants the Heartseed (`PlayerScript::OnQuestRewarded`),
however it is completed. The Theramore intro scenario will later complete the same quest.

- At login, a character with no Heartseed that has not done the quest is offered it after
  `Heartseed.Grant.OfferDelay` ms. The player is its own quest giver (quest flags AUTO_TAKE | AUTO_COMPLETE),
  so no NPC is needed. Accepting completes it and opens the reward window; turning it in grants the Heartseed.
- Declining offers it again at the next login. Logging out at the reward window reopens the reward window.
- `.heartseed revoke` also resets the quest; `.heartseed grant` grants without it (GM shortcut).

## Story beats

Simple hooks set the story flags until later quests take over (they can call
`HeartseedMgr::ReachStoryBeat` with the same flags). A beat is set once; the player is told, and when it
raises the cap (an awakening) banked Growth ranks the Heartseed up at once.

| Flag | Set when | Hook |
|---|---|---|
| `first_dungeon` (1) | A dungeon's final encounter is completed (`instance_encounters.lastEncounterDungeon`), for every player in the instance, in or out of a group | `PlayerScript::OnDungeonCompleted`, from `InstanceScript::UpdateEncounterState` |
| `reached_pandaria` (2) | Entering a map in `Heartseed.Story.PandariaMaps`, or logging in there | `PlayerScript::OnMapChanged`, plus `OnLogin` |
| `first_world_boss` (3) | Kill credit on a creature in `Heartseed.Story.WorldBosses`, for every credited group member (pet kills too) | `PlayerScript::OnKillCredit`, from `KillRewarder::RewardKillCredit` |

Dungeons whose instance has no script never call `UpdateEncounterState`, so they do not count.

## Feeding and essences

- **Feeding** consumes a weapon from the bags for `(Heartseed.Feed.GrowthBase + GrowthPerItemLevel * item
  level) * Heartseed.Feed.QualityPct[quality] / 100` Growth, and rolls each essence whose source matches it.
  Fishing poles, miscellaneous weapons, quest items, refundable items and qualities worth 0 (legendary,
  artifact, heirloom by default) are never fed. Each feed is logged in `heartseed_fed_weapon`.
- **On replacement, with a prompt** (`Heartseed.Feed.OnReplace`): when a weapon gained since login goes into
  the main hand with a higher item level, the Heartseed asks, one update later, whether to feed the weapon it
  replaced (a gossip menu opened from the player, with a confirm box). Nothing is consumed until the player
  confirms. "Keep" means it is not asked about that weapon again this session. Swapping between weapons the
  character already carried never asks, and a weapon moved to the off hand, the bank or a vendor is left alone.
- **Twink gear** (`twink_item`) is never offered on replacement. Deleting a twink weapon from the bags feeds
  it instead (`PlayerScript::OnItemDestroyedByPlayer`, a small core hook in `HandleDestroyItemOpcode`). The first
  time a character puts a twink weapon in the main hand, a notification and a chat message explain this
  (remembered as story flag 1001, a hint that never counts as an awakening).
- **Appearances:** every weapon fed or deleted is recorded in `character_weapon_appearance`.
- **Essences** fill the first free open slot when first collected (`Heartseed.Essence.AutoSlot`). Slots
  open at 10, 40 and 70; an essence in an open slot rolls its proc on damage the character deals, with its
  own `Heartseed.Essence.ProcCooldown`, dealt like the core proc. `SetEssenceSlot` is what the shrine
  gossip (T-023) will call.

## Traits and the core passive

- **Majors** are chosen, one per tier (levels 10, 25, 40, 55, 70, 85), into the loadout of the active
  specialization. `HeartseedMgr::ChooseMajor` validates the choice; the shrine gossip (T-023) calls it.
- **Minors** fill in by rank on every path the active loadout has at least one major in (path 0 = every
  Heartseed).
- **Loadouts per specialization:** a specialization with no saved loadout starts from the one the
  character just left, so choosing a spec after the level 10 major, or adding a second spec, keeps it.
- **Core passive**, every Heartseed:
  - bonus weapon damage: `Heartseed.Core.WeaponDamagePct` of the main-hand weapon's average damage per
    swing, applied as a flat physical damage aura (spell 310008), so it follows the weapon's item level;
  - a proc: damage the character deals rolls `Heartseed.Proc.Chance`; a hit deals
    `Heartseed.Proc.WeaponDamagePct` of the current average weapon damage to the target as spell
    `Heartseed.Proc.SpellId` (name and school in the combat log), then waits `Heartseed.Proc.Cooldown` ms.
    The damage is dealt directly, not cast, because casts check range and facing even when triggered.
- `RefreshTraits` diffs the wanted auras against the applied ones and runs on login, level change,
  main-hand change (`PlayerScript::OnVisibleItemChanged`), specialization change
  (`PlayerScript::OnSpecializationChanged`) and every state change. Passive auras are not saved with the
  character and are not sent to the client.
- Trait effects are placeholders until the trait design is written.

## Weapon glow

The glow is display only. `Player::SetVisibleItemSlot` asks `PlayerScript::OnVisibleItemEnchant` for a
visual whenever the temporary-enchant half of the visible item field is empty, and the Heartseed answers
for the main hand with the `heartseed_glow` enchant for the character's stage. Nothing is written to the
item, so the enchant's effects never apply and the glow never travels with a traded weapon. A real
temporary enchant (poison, oil, imbue) shows instead while it lasts.

The glow enchant IDs shipped are placeholders that have a visual; pick the final looks in the client.

## Config

worldserver.conf:

- `Heartseed.Enabled` (default 1): off keeps state but shows no glow and applies no traits or core passive.
- `Heartseed.Core.WeaponDamagePct` (5), `Heartseed.Proc.SpellId` (16614, placeholder look),
  `Heartseed.Proc.Chance` (10), `Heartseed.Proc.WeaponDamagePct` (50), `Heartseed.Proc.Cooldown` (3000 ms).
- Cap and ranks: `Heartseed.Rank.CapPerLevel` (1), `.CapPerAwakening` (5), `.AwakeningFlags` ("1 2 3"),
  `.GrowthBase` (100), `.GrowthPerRank` (20).
- Feeding: `Heartseed.Feed.OnReplace` (1), `.GrowthBase` (20), `.GrowthPerItemLevel` (2), `.QualityPct`
  ("50 100 150 250 400 0 0 0", poor to heirloom).
- Essences: `Heartseed.Essence.ProcCooldown` (3000 ms), `.AutoSlot` (1).
- Granting quest: `Heartseed.Grant.QuestId` (90000), `.OfferOnLogin` (1), `.OfferDelay` (5000 ms).
- Choices: `Heartseed.Choice.PromptOnLogin` (1), `.PromptDelay` (6000 ms), `.ChangeAtShrineOnly` (1).
- Story beats: `Heartseed.Story.Hooks` (1), `.PandariaMaps` ("870"), `.WorldBosses` (MoP and old world boss
  entries), `.AwakeningSound` (0 = none).
- All pacing numbers are placeholders until v0.5.

## GM commands

Permission 90000 (`Command: heartseed`, Gamemaster role). Target is the selected player or yourself.

- `.heartseed info`: rank, Growth, stage, loadout, essences, glow, story flags and the main hand's
  visible enchant field as the client receives it.
- `.heartseed grant`: give a Heartseed (rank 1, no Growth).
- `.heartseed revoke`: remove it with all its rows, and reset the granting quest.
- `.heartseed offer`: show the granting quest now (no Heartseed and quest not done).
- `.heartseed shrines`: list the Celestial shrines and say whether the target is at one.
- `.seed` (player command, permission 90001): open your own choice window.
- `.heartseed rank <1-255>`: set the rank. Ignores the level cap, so states can be tested without leveling.
- `.heartseed growth <n> | +<n> | -<n>`: set Growth or take from it (never below 0); `+<n>` earns it,
  ranking up to the cap like real Growth.
- `.heartseed flag list | set <name|id> | clear <name|id>`: story flags by name (`first_dungeon`,
  `reached_pandaria`, `first_world_boss`) or number. Setting keeps the first time a flag was set.
- `.heartseed traits`: every trait, marking the ones active on the target.
- `.heartseed major <10|25|40|55|70|85> <trait id | 0>`: set or clear a major in the active spec's
  loadout. Skips the level check.
- `.heartseed reload`: reload stages, glows, traits, essences and shrines, and refresh the target.
- `.heartseed cap`: the cap and how it is made up (level, awakening flags), rank, Growth, next rank cost.
- `.heartseed feed <item link | entry>`: feed one such weapon from the target's bags (no prompt, no upgrade
  check; twink items allowed).
- `.heartseed feed preview <item link | entry>`: Growth and essence chances, whether it is twink gear, and
  whether a carried copy can be fed on replacement and by command, without consuming anything.
- `.heartseed essence list`: essence types, their sources and what the target holds.
- `.heartseed essence add <id> [count]` / `remove <id>`: give or take an essence.
- `.heartseed essence slot <id> <0-3>`: move an essence to a slot (0 = out). Skips the level check; a slot
  the level has not opened stays inactive.

`.heartseed info` also shows the loadout, the applied auras with their amounts, and the proc damage.

Every change is saved at once, not with the next character save.

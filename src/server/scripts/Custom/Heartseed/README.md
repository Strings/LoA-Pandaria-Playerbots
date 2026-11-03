# Heartseed

The living jewel that rides on a character's main-hand weapon and grows with it. Design:
`docs/DESIGN.md`, "The Heartseed". IDs come from the Heartseed block in `docs/ID_RANGES.md`.

## Files

| File | What |
|---|---|
| `HeartseedRules.h` | Pure rules with no server dependencies (stage by level, dominant path, tier levels, minor activation, trait and core passive amounts, Growth XP levels and stat points, main stat by spec, essence slots, meters). Unit-test these. |
| `HeartseedMgr.h/.cpp` | State cache per character (account fields shared across the account's loaded characters), loading and saving, stage and glow lookup, traits, major choice, core and essence procs, Growth XP and stat points, feeding, essences. |
| `heartseed_scripts.cpp` | World, player, unit and command scripts. |

## Data

One seed per account (T-049, DESIGN.md "One seed per account"). Account tables
(`sql/updates/characters/2026_10_31_claude_16_heartseed_account.sql`, which also migrates pre-T-049 rows):

- `account_heartseed`: one row per account that has a Heartseed (grant time). Having a row is what "has a
  Heartseed" means to code, for every character of the account; the granting quest (T-025) creates it. Its
  `rank` and `growth` columns are retired (T-083) and keep their defaults.
- `account_heartseed_essence`: essences the account has collected, and how many times.
- `account_heartseed_fed_weapon`: every weapon entry the seed has eaten, how often and when first.

Character tables (`sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql`):

- `heartseed_loadout`: the major chosen at each of the six major tiers, per specialization.
- `heartseed_essence`: which slot (1 to 3) each of the account's essences is active in for this character
  (only slotted essences have a row; `count` repeats the account's).
- `heartseed_story_flag`: story beats this character reached and when (`Heartseed::StoryFlag`).
- `heartseed_state` and `heartseed_fed_weapon` (T-020, T-022) are no longer written; kept for the migration.
- `character_heartseed_growth` and `character_heartseed_growth_point`
  (`sql/updates/characters/2026_11_02_claude_03_heartseed_growth.sql`, T-083): Growth level and XP, and the
  stat points spent, per character.

Every loaded character of an account holds the same shared values (`State::AccountId`, grant time, essence
counts, weapons fed); a change through one is copied to the others under the manager's lock, so the player's
alts logged in as playerbots see it at once.

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

- `heartseed_trait`: one row per trait: path, kind (1 minor, 2 major), major tier or the character level a
  minor fills in at (column `min_rank`, a level since T-083), the
  passive aura spell it applies, and its amount (`base_amount + amount_per_ilvl * main-hand item level`,
  written to the spell's effect 0).
- The aura spells are server-side rows in `spell_dbc`, `spellmisc_dbc` and `spelleffect_dbc`, in the
  Heartseed spell block (310000+). The server reads those tables at startup only.

World database (`sql/updates/world/2026_11_01_claude_06_heartseed_gifts.sql`, T-080):

- `heartseed_gift` (rebuilt by `2026_11_01_claude_07_heartseed_gift_duration.sql`): the Celestial Gift a major
  teaches: the client spell, its cooldown and summon duration in ms (each replaces the spell's own; 0 keeps it, so
  Call of Xuen runs 20 s / 10 s against Invoke Xuen's 3 min / 45 s), and its power `base_amount` as a percentage
  (T-083), so the Gift scales with the character instead of being overtuned at low levels: Serpent's Breath ticks
  for that % of the higher of attack and spell power, Crane's Plume heals that % of the caster's max health,
  Niuzao's Retort deals that % of the damage taken in its window to each enemy. `amount_per_ilvl` adds flat power
  per main-hand item level (0 in shipped data). The tier-0 majors (traits 2, 4, 6, 8) are the four Gifts; their
  old stat auras are minors 9 to 12. A trait with `spell_id` 0 applies no aura.
- Gifts are their own spells (T-086): clones of client spells with the source's effects and visuals but their own
  name, tooltip and icon, defined in `tools/clientpatch/edits.json`. The client patch adds them to the client and
  the generated `2026_11_03_claude_00_heartseed_gift_spells.sql` to the server. Call of Xuen (310100) clones Invoke
  Xuen (123904), Crane's Plume (310101) Spirit of Chi-Ji (148009), Niuzao's Retort (310102) Ox Charge (116098),
  and Serpent's Breath (310103) Celestial Breath (117960); the rider auras are 310110 to 310114. The sources stay
  untouched, so monks, trolls, rogues and mages keep their own abilities. `RefreshTraits` teaches the active Gift
  and unlearns the others; it also takes back the client spells Gifts used to be (`RETIRED_GIFT_SPELLS`), except a
  talent a monk took. A new Gift or rider aura is a new `spell` edit plus `build.py sql`.
  `spell_heartseed_gift` adds the Gift cooldown, gates and spends the meter; `spell_heartseed_celestial_breath`
  (124947) replaces the creature spell's damage; `spell_heartseed_crane_plume` picks the caster plus the most
  wounded group members in `Heartseed.Gift.PlumeRadius` (`Heartseed.Gift.PlumeTargets` of them) and heals through
  `GiftHeal`; `spell_heartseed_niuzao_retort` strikes enemies in `Heartseed.Gift.RetortRadius`. The class scripts
  the sources use (`spell_monk_invoke_xuen`, `spell_item_spirit_of_chi_ji_heal`) are bound to the clones too.
  `spell_heartseed_xuen_lightning` (123996) plays `Heartseed.Gift.XuenLightningVisual` (default Chain Lightning's
  visual) from Xuen to each target his lightning hits, for monks' Xuen too.
- `.seed sync` M lines carry the Gift spell id and the trait description, so the addon can show and cast it.
- `heartseed_gift_rider` (`2026_11_02_claude_00_heartseed_gift_riders.sql`, T-082): one rider per Gift per stage
  (Sapling 2, Bloom 3, Full Bloom 4); a character keeps every rider its stage has reached. `kind` is
  `Heartseed::GiftRiderKind` (`HeartseedRules.h`); `amount`, `duration` and the helper `spell_id` are data. Aura
  kinds `AddAura` the helper spell and overwrite its effect 0 amount and duration. Hooks: `spell_heartseed_gift`
  (self aura, heal, shield from the damage window), `spell_heartseed_crane_plume` (extra targets),
  `spell_heartseed_niuzao_retort` (target aura, widen, cooldown per enemy hit), `spell_heartseed_xuen_lightning`
  (extra targets), `spell_heartseed_celestial_breath` (leech, DoT, cooldown per enemy hit) and the player kill hook
  (kill cooldown, 0 = reset). Stage 1 (Sprout) riders come with the Gift itself. `.heartseed info` lists the
  active riders.
- `heartseed_meter` (`2026_11_02_claude_02_heartseed_active_play.sql`, T-083): one meter per path, built by normal
  play of any class and spent by the Gift (DESIGN.md "Active play, not big cooldowns"). `source` is
  `Heartseed::MeterSource`: crits dealt (Xuen's Thunder), health actually healed, done or received (Chi-Ji's Hope,
  one stack per `threshold_pct` of max health), a hit taken of `threshold_pct` of max health (lights Niuzao's
  Retort for `lit_duration`; a gated Gift fails to cast while dark), spell casts that are not triggered (Yu'lon's
  Coil). Casting the Gift spends the stacks for `spend_bonus_pct` more power each (Xuen: tiger duration).
  `crit_cooldown_cut` shortens the Gift on each crit (at most once per `build_cooldown`), `decay` empties idle
  stacks, `window` is how much damage taken Retort remembers. Meter state is runtime only (never saved) and
  resets when the Gift changes. Pure rules live in `HeartseedRules.h` (`MeterBuild`, `MeterSpend`,
  `HealingStacks`, ...) and are unit-tested.
- Crits reach the meter through the server-side passive 310009 (`spell_dbc`, applied with the Gift): its
  `spell_proc_event` row procs on every critical damage or healing hit, triggered spells included
  (`SPELL_ATTR3_CAN_PROC_WITH_TRIGGERED`), and `spell_heartseed_meter` feeds the proc in. Healing comes from
  the `OnHeal` unit hook (spell heals reach only that one); the Gift's own heals go through `GiftHeal`, which
  the hook skips. Damage taken comes from `OnDamage`, spell casts from the player `OnSpellCast` hook.
- The server tells the addon about every change with an addon whisper `P <stacks> <max> <lit ms left> <decay ms>
  <Gift spell>`; the addon shows a small movable meter and glows the Gift on the action bars when full or lit.
- Call of Xuen's tiger keeps up with a mount: out of combat it runs at its owner's speed and jumps to them past
  `Heartseed.Gift.XuenCatchUpDistance` (25 yd), checked every `Heartseed.Gift.XuenFollowInterval` ms.

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

## Growth XP and stat points (T-083)

DESIGN.md Progression, "Three tracks, no rank cap". There is no rank and no cap any more: character level unlocks
stages, majors and minors (`min_rank` is a level), feeding gives appearances and essences, and Growth XP gives
power, each the character's own.

- **Growth XP** comes from battleground wins (`Heartseed.Growth.BattlegroundWin`, arenas excluded), quest
  turn-ins (`.Quest`; repeatable quests other than dailies and weeklies give none), rare kills (`.Rare`, creature
  rank rare or rare elite) and dungeon and raid boss kills (`.DungeonBoss`, `.RaidBoss`). Quests, rares and
  bosses count at most `.LevelRange` levels below the character; kills pay every credited group member. Growth
  level L to L + 1 costs `.XpBase + .XpPerLevel * L`; no cap. `HeartseedMgr::AddGrowthXp` is the one entry
  point for later sources (custom rares, events, Sha hunters).
- The battleground result reaches the script through `BattlegroundHooks` (`src/server/game/Scripting/`), a
  small dispatch the core calls from `Battleground::EndBattleground`, so `ScriptMgr.h` stays untouched.
- **Only the character being played earns.** The player's alts logged in as playerbots keep their points but
  earn nothing; random bots never load a seed. A character with XP gain turned off (a twink) earns nothing
  unless `Heartseed.Growth.TwinksEarn = 1`, so its bracket's balance holds.
- **Stat points**: one per Growth level. The player spends them in the addon panel (`.seed spend <stat>`) on a
  row of `heartseed_growth_stat` (`sql/updates/world/2026_11_03_claude_02_heartseed_growth_curve.sql`): Stamina,
  Main stat, Critical strike, Haste. Points have diminishing returns: the bonus is
  `max_pct * points / (points + half_points)` to the nearest percent (`Heartseed::StatPointAmount`), so each
  stat approaches its own ceiling (stamina 30%, main stat 20%, crit and haste 10%) and points never stop
  counting. Each stat is a server-side passive (310010 stamina, 310014 crit, 310015 haste; main stat 310011
  strength, 310012 agility or 310013 intellect by the specialization, or the class before one is chosen) whose
  effect 0 is that bonus, applied by `RefreshTraits` like the trait auras.
- `.seed reset` gives every point back anywhere; `Heartseed.Growth.ResetAtShrineOnly = 1` limits it to a Celestial shrine.
- The first Growth level shows a short introduction (notification plus a chat line) once per character
  (story flag 1002, `hint_growth_points`).

## Choosing majors

A gossip window the Heartseed opens from the player (menu 90002, texts 90002 and 90003 in `npc_text`):
the open tiers with their major or "choose a path", the next locked tier, then a tier's majors with a
confirm box. Picking one calls `HeartseedMgr::ChooseMajor` for the active specialization.

- **Opens** when a level opens a tier with no major, at login when one is waiting
  (`Heartseed.Choice.PromptOnLogin`, after `.PromptDelay`), and with the player command `.seed`.
- **Majors can be chosen and changed anywhere.** `Heartseed.Choice.ChangeAtShrineOnly = 1` limits changing a
  chosen major to a Celestial shrine (`heartseed_shrine`: the four Celestial temples and the Celestial Court).
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
`HeartseedMgr::ReachStoryBeat` with the same flags). A beat is set once and the player is told; beats unlock
lore and cosmetics later, never power.

| Flag | Set when | Hook |
|---|---|---|
| `first_dungeon` (1) | A dungeon's final encounter is completed (`instance_encounters.lastEncounterDungeon`), for every player in the instance, in or out of a group | `PlayerScript::OnDungeonCompleted`, from `InstanceScript::UpdateEncounterState` |
| `reached_pandaria` (2) | Entering a map in `Heartseed.Story.PandariaMaps`, or logging in there | `PlayerScript::OnMapChanged`, plus `OnLogin` |
| `first_world_boss` (3) | Kill credit on a creature in `Heartseed.Story.WorldBosses`, for every credited group member (pet kills too) | `PlayerScript::OnKillCredit`, from `KillRewarder::RewardKillCredit` |

Dungeons whose instance has no script never call `UpdateEncounterState`, so they do not count.

## Feeding and essences

- **Feeding** consumes a weapon from the bags for its appearance and a roll on each essence whose source matches
  it; it gives no power (T-083). Fishing poles, miscellaneous weapons, quest items, refundable items and
  qualities outside `Heartseed.Feed.Qualities` (legendary, artifact, heirloom by default) are never fed. Each feed is logged in `heartseed_fed_weapon`.
- **On replacement, with a prompt** (`Heartseed.Feed.OnReplace`): when a weapon gained since login goes into
  the main hand with a higher item level, the Heartseed asks, one update later, whether to feed the weapon it
  replaced (a gossip menu opened from the player, with a confirm box). Nothing is consumed until the player
  confirms. "Keep" means it is not asked about that weapon again this session. Swapping between weapons the
  character already carried never asks, and a weapon moved to the off hand, the bank or a vendor is left alone.
- **Twink gear** (`twink_item`) is never offered on replacement. Deleting a twink weapon from the bags feeds
  it instead (`PlayerScript::OnItemDestroyedByPlayer`, a small core hook in `HandleDestroyItemOpcode`). The first
  time a character puts a twink weapon in the main hand, a notification and a chat message explain this
  (remembered as story flag 1001, a hint).
- **Appearances:** every weapon fed or deleted is recorded in `character_weapon_appearance`.
- **Essences** fill the first free open slot when first collected (`Heartseed.Essence.AutoSlot`). Slots
  open at 10, 40 and 70; an essence in an open slot rolls its proc on damage the character deals, with its
  own `Heartseed.Essence.ProcCooldown`, dealt like the core proc. `SetEssenceSlot` is what the shrine
  gossip (T-023) will call.

## Traits and the core passive

- **Majors** are chosen, one per tier (levels 10, 25, 40, 55, 70, 85), into the loadout of the active
  specialization. `HeartseedMgr::ChooseMajor` validates the choice; the shrine gossip (T-023) calls it.
- **Minors** fill in by character level on every path the active loadout has at least one major in (path 0 =
  every Heartseed).
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
- Growth XP (T-083): `Heartseed.Growth.XpBase` (300), `.XpPerLevel` (50), `.BattlegroundWin` (100),
  `.DungeonBoss` (50), `.RaidBoss` (100), `.Quest` (15, T-084), `.Rare` (40, T-084), `.LevelRange` (5), `.TwinksEarn` (0), `.ResetAtShrineOnly` (0).
- Feeding: `Heartseed.Feed.OnReplace` (1), `.Qualities` ("0 1 2 3 4", poor to epic).
- Essences: `Heartseed.Essence.ProcCooldown` (3000 ms), `.AutoSlot` (1), `.SlotLevels` ("10 40 70").
- Major tiers: `Heartseed.Majors.TierLevels` ("10 25 40 55 70 85"). The number of tiers and slots is fixed;
  saved loadouts keep their tier index.
- Granting quest: `Heartseed.Grant.QuestId` (90000), `.OfferOnLogin` (1), `.OfferDelay` (5000 ms).
- Choices: `Heartseed.Choice.PromptOnLogin` (1), `.PromptDelay` (6000 ms), `.ChangeAtShrineOnly` (0).
- Gift areas (T-083): `Heartseed.Gift.PlumeRadius` (30 yd), `.PlumeTargets` (3, caster not counted),
  `.RetortRadius` (8 yd). Meter numbers live in `heartseed_meter`.
- Story beats: `Heartseed.Story.Hooks` (1), `.PandariaMaps` ("870"), `.WorldBosses` (MoP and old world boss
  entries), `.AwakeningSound` (0 = none).
- All pacing numbers are placeholders until v0.5.

## GM commands

Permission 90000 (`Command: heartseed`, Gamemaster role). Target is the selected player or yourself.

- `.heartseed info`: stage, Growth level, XP and points, loadout, essences, glow, story flags and the main hand's
  visible enchant field as the client receives it.
- `.heartseed grant`: give a Heartseed (no Growth).
- `.heartseed revoke`: remove it with all its rows, and reset the granting quest.
- `.heartseed offer`: show the granting quest now (no Heartseed and quest not done).
- `.heartseed shrines`: list the Celestial shrines and say whether the target is at one.
- `.seed` (player command, permission 90001): open your own choice window. `.seed spend <stat> [count]` spends
  stat points (stat id or name prefix), `.seed reset` takes them all back at a shrine; the addon uses both.
- `.heartseed growth [+<xp> | <level> [xp]]`: show Growth; `+<xp>` earns XP as in play (the bot and twink rules
  apply); `<level> [xp]` sets it, refunding points past the new level.
- `.heartseed points [spend <stat> [count] | reset]`: list the stats with points, percentages and the aura each
  uses; spend for the target, or reset anywhere.
- `.heartseed flag list | set <name|id> | clear <name|id>`: story flags by name (`first_dungeon`,
  `reached_pandaria`, `first_world_boss`) or number. Setting keeps the first time a flag was set.
- `.heartseed traits`: every trait, marking the ones active on the target.
- `.heartseed major <10|25|40|55|70|85> <trait id | 0>`: set or clear a major in the active spec's
  loadout. Skips the level check.
- `.heartseed meter [<stacks> | lit | hit <damage>]`: show the path meter, set its stacks, light it, or feed it
  a hit taken (Niuzao: records the damage and lights the meter if the hit is big enough).
- `.heartseed reload`: reload stages, glows, traits, essences, shrines, meters and Growth stats, and refresh the target.
- `.heartseed feed <item link | entry>`: feed one such weapon from the target's bags (no prompt, no upgrade
  check; twink items allowed).
- `.heartseed feed preview <item link | entry>`: essence chances, whether it is twink gear, and
  whether a carried copy can be fed on replacement and by command, without consuming anything.
- `.heartseed essence list`: essence types, their sources and what the target holds.
- `.heartseed essence add <id> [count]` / `remove <id>`: give or take an essence.
- `.heartseed essence slot <id> <0-3>`: move an essence to a slot (0 = out). Skips the level check; a slot
  the level has not opened stays inactive.

`.heartseed info` also shows the loadout, the applied auras with their amounts, and the proc damage.

Every change is saved at once, not with the next character save.

## Shared table reload (T-038)

AddSC_heartseed registers the related world caches with Custom/Reload. Use
`.reload heartseed` for the bundle, or any registered table name (for example
`.reload heartseed_trait`); each reloads the coherent bundle. The existing
`.heartseed reload` delegates to the same registry and refreshes its target.
Failed queries or invalid rows leave the previous cache intact. Empty optional
tables are allowed; heartseed_stage requires at least stage 0. Row counts refer
to the whole bundle, including the twink_item feeding lookup. See
`../Reload/README.md` for registration, permissions and command precedence.

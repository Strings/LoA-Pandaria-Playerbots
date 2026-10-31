# T-022: Heartseed feeding, essences and the rank cap

**Agent:** claude
**Branch:** claude/T-022-heartseed-feeding (stacked on T-021, PR #12, which is stacked on #11 and #9)
**Task status:** review

## What changed
- **Growth and rank-ups:** `HeartseedMgr::AddGrowth` is the one way to earn Growth (feeding now; quests,
  rares, events and dungeons later). Growth goes to ranks: rank R to R + 1 costs
  `Heartseed.Rank.GrowthBase + GrowthPerRank * (R - 1)`. Growth past the cap is banked and spent when the cap
  rises (level-up, new awakening, login): `ApplyRankUps`. The player is told about each rank, and told once
  per session when the Heartseed is full.
- **Rank cap:** `level * Heartseed.Rank.CapPerLevel + awakenings * CapPerAwakening`, at least 1.
  Awakenings are the story flags listed in `Heartseed.Rank.AwakeningFlags` (default the three beats).
- **Feeding:** `Feed` destroys a weapon from the bags for `(GrowthBase + GrowthPerItemLevel * item level) *
  QualityPct[quality] / 100` Growth and rolls each matching essence. Fishing poles, miscellaneous weapons,
  quest items, refundable items and qualities set to 0 (legendary, artifact, heirloom) are refused. Every feed is
  logged per entry in the new characters table `heartseed_fed_weapon`, for the later appearance collection.
  The first feed explains feeding in chat (the intro for this part of the system).
- **Feeding on replacement, with a prompt** (`Heartseed.Feed.OnReplace`, default on): when a weapon gained
  since login goes into the main hand with a higher item level, the Heartseed asks whether to feed the weapon it
  replaced: a gossip menu opened from the player (menu and `npc_text` 90001) with a confirm box. Nothing is
  consumed until the player confirms; "Keep" stops it asking about that weapon this session. Swapping between
  weapons the character already carried never asks.
- **Twink gear:** world table `twink_item` (314 entries seeded from Codex's T-045 list). Twink weapons are never
  offered on replacement; deleting one from the bags feeds it, through a new core hook
  `PlayerScript::OnItemDestroyedByPlayer` (called in `HandleDestroyItemOpcode`). The first twink weapon in the
  main hand shows a one-time notification and chat message explaining this (hint flag 1001, stored with the
  story flags, never an awakening).
- **Appearance record:** characters table `character_weapon_appearance` gets every weapon fed or deleted
  (entry, display id, source, time), for the appearance collection to read.
- **Two essence types** as world data: `heartseed_essence_template` (Sha-touched: shadow, Mogu: lightning;
  placeholders) and `heartseed_essence_source` (by item entry or a text in the item name: `sha-`, `mogu`; 25%
  per weapon fed). A new essence fills the first free open slot (slots open at 10, 40, 70). A slotted essence
  in an open slot rolls its own proc on damage dealt, like the core proc, with `Heartseed.Essence.ProcCooldown`.
  `SetEssenceSlot` is what the T-023 shrine gossip will call.
- **Pure rules** in `HeartseedRules.h`: `RankCap`, `GrowthForNextRank`, `ApplyGrowth`, `FeedGrowth`,
  `EssenceSlotsForLevel`, `FreeEssenceSlot`, `AddCapped`.
- **GM commands:** `.heartseed cap`, `.heartseed feed <item>`, `.heartseed feed preview <item>`,
  `.heartseed essence list | add <id> [n] | remove <id> | slot <id> <0-3>`. `.heartseed growth +N` now earns
  Growth (ranks up to the cap); a bare number and `-N` still set it raw. `.heartseed info` shows cap, cost of the
  next rank, weapons fed and essences; `.heartseed reload` also reloads essences.
- **Headless client:** `autoequip BAG SLOT` verb (CMSG_AUTOEQUIP_ITEM), to equip like a right-click.

## How it was tested
Claude test server, headless client, character `Seedtest` (human warrior 30, Corpsemaker ilvl 35 equipped).
All five SQL files applied twice to `world_claude` / `characters_claude`.
- Rules: standalone checks of every new `HeartseedRules.h` function (g++ -Wall -Wextra, no warnings), until
  T-008 brings the unit test framework.
- `grant`, `cap`: cap 30. `growth +50`: rank stays 1. `growth +100000`: rank 30, rest banked, "full" notice.
  `flag set first_dungeon`: cap 35 and rank 35 at once. `.levelup 1`: cap 36, rank 36 at once.
- `feed preview` for Sha-Warped Blade (1227 Growth, Sha-touched 25%), Mogu Sportsman's Bow (3888, Mogu 25%),
  Corpsemaker (225, none). `.additem 81556` + `feed 81556`: consumed, first-feed intro, logged. Fishing pole
  refused; equipped Corpsemaker not found in bags.
- Replacement prompt (`.additem`, then `autoequip`, then `gossip-select` on the player-sourced menu 90001):
  Lupine Axe (20) replacing Corpsemaker (35) asked nothing. Darksteel Bastard Sword (30) replacing Lupine opened
  the prompt ("Feed Lupine Axe ... (+90 Growth)" / "Keep Lupine Axe."). Keep left Lupine in the bags, and
  swapping back and forth did not ask again. Woodsman Sword replaced by Darksteel asked; Feed consumed it.
- Twink: Blackrock Champion's Axe (twink entry 1455) in the main hand showed the tutorial once (hint flag 1001
  saved). Darksteel replacing it asked nothing. `destroy-item` on it fed it (+102). Deleting a fishing pole
  recorded its appearance as "deleted". `character_weapon_appearance` holds fed (1) and deleted (2) rows.
- Essences: add, auto-slot into slot 1, second essence stays out, `slot 1 1` takes the other out, slot 2 shows
  as locked at level 30 and counts as inactive. With chance 100 (local only), `.damage 1` on a Blackrock Battle
  Worg counted one essence proc; the unslotted essence never procs.
- Restart and relog: rank, Growth, flags, essences and slots load back; fed-weapon rows present.
- Built with no warnings in touched files.

## Decisions made
- **Feeding trigger, decided by the user** (relayed by the starting session): "auto with prompting… make
  transmogs automatic on deletion the way they are in modern retail… twink items get a special flag… a green
  Twink tag that keeps them out of this system unless you delete them manually… Equip a twink weapon and a
  tutorial thing pops up explaining that these don't automatically feed your seed unless you delete them from
  your inventory." Built as: prompt on upgrade, twink weapons excluded and fed by deleting them, a one-time
  tutorial on the first twink weapon equipped, and every fed or deleted weapon's appearance recorded.
- **Prompt mechanism:** a gossip menu sourced from the player, the 5.4.8 client's server-driven dialog with a
  confirm box (Eluna uses the same path). No client patch needed.
- **Twink flag as `twink_item`,** a general table rather than `heartseed_*`, since other systems that consume
  gear should honor it. The green "Twink" tooltip tag needs item hotfixes or the client patch: not done.
- **Delete-to-feed only for twink weapons,** as asked. Deleting any other weapon only records its appearance.
- **Growth is spent on ranks** (it is "Growth toward the next rank", as the T-020 schema says) and banked at the
  cap, so an awakening ranks the Heartseed up at once.
- **Which flags are awakenings** is a config list, so a flag added later for something else (an intro seen, say)
  doesn't raise the cap by accident.
- **Essence numbers live in the world table** (chance, damage, sources), like trait amounts; global tuning
  (cooldown, auto-slot, feeding and cap formulas) is in `worldserver.conf`.
- **Essence origin by item name** (`sha-`, `mogu`) as well as entry; origin by where the weapon dropped would
  need loot tracking. Names are matched on the English template name.
- Essence `count` is kept but has no effect yet (the morph reads absorbed essences later).
- All pacing numbers are placeholders until v0.5 (accepted interim state).

## For the other agent
- Codex: `twink_item` copies the 314 entries your T-045 update buffs; add rows there when the twink list grows.
- New core hook: `PlayerScript::OnItemDestroyedByPlayer` (ScriptMgr.h, rebuilds most scripts).
- Apply also `sql/updates/world/2026_10_31_claude_06_heartseed_feed_prompt.sql`,
  `sql/updates/world/2026_10_31_claude_07_twink_item.sql` and
  `sql/updates/characters/2026_10_31_claude_08_weapon_appearance.sql`.
- Apply `sql/updates/world/2026_10_31_claude_04_heartseed_essence.sql` and
  `sql/updates/characters/2026_10_31_claude_05_heartseed_fed_weapon.sql`.
- New `worldserver.conf.dist` keys: `Heartseed.Rank.*` (CapPerLevel, CapPerAwakening, AwakeningFlags,
  GrowthBase, GrowthPerRank), `Heartseed.Feed.*` (OnReplace, GrowthBase, GrowthPerItemLevel, QualityPct),
  `Heartseed.Essence.*` (ProcCooldown, AutoSlot). Defaults apply when missing.
- `HeartseedMgr::CastProc` now takes an essence id (0 = core proc).
- Headless: new `autoequip BAG SLOT` verb.

## Next
- T-023 gossip: essence slotting through `SetEssenceSlot(..., force = false)`, and possibly a "feed this" option.
- Other Growth sources call `AddGrowth` (quests, rares in 4d, first dungeon clears, Sha hunters).
- **Appearance collection** (follow-up, out of scope here): transmog from consumed and deleted weapons,
  regardless of proficiency, as in modern retail; reads `character_weapon_appearance`.
- Green "Twink" tooltip tag once item hotfixes or the client patch can carry it.

## Map additions
- None (all in `src/server/scripts/Custom/Heartseed/` and the SQL update folders).

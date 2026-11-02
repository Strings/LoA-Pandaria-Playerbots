# T-082: Celestial Gift riders per stage, Xuen catch-up

**Agent:** claude
**Branch:** claude/T-082-gift-riders
**Task status:** done

## What changed
- New `heartseed_gift_rider` table: one rider per Gift per stage. Sapling, Bloom and Full Bloom each add one,
  and a character keeps every rider its stage has reached. The human approved the twelve riders on 2026-11-02,
  halving the cooldown cuts:
  - Xuen: 15% haste for 10 s / lightning arcs to 2 more enemies / each kill cuts the cooldown by 1 s
  - Chi-Ji: heal 10% / 40% speed for 4 s / a killing blow resets the cooldown
  - Niuzao: stunned enemies deal 20% less damage for 8 s / a shield of 5% health per enemy hit / 50% wider and
    1 s longer stun
  - Yu'lon: heals for 30% of damage / jade-fire DoT / each enemy hit cuts the cooldown by 1.5 s
- Pure rules in `HeartseedRules.h` (`GiftRiderKind`, `ActiveRiders`, `PercentOf`, `CooldownCut`), unit-tested.
- Call of Xuen keeps up with a mounted owner: out of combat the tiger runs at its owner's speed and jumps to them
  past 25 yd. Config keys `Heartseed.Gift.XuenCatchUpDistance` (25) and `Heartseed.Gift.XuenFollowInterval`
  (1000 ms).
- Fix: the server casts Chi Torpedo as 121828, so Crane Rush's cooldown (T-080) never applied. The Gift script
  now hooks 121828 too.
- `.heartseed info` lists the active Gift and its riders.

## How it was tested
- New scenario `heartseed-riders` at level 85. Ox Stomp gives Ice Barrier with 2 × 5% health (4,584), Crane Rush
  gives Sprint at 40% for 4 s, Call of Xuen gives Berserking at 15% for 10 s, and `.heartseed info` lists the
  riders.
- The full suite (18 scenarios) and all 20 unit tests pass on the claude devenv.
- `heartseed-xuen`, `heartseed-gifts` and `heartseed-riders` now start with `.go` to an Elwynn spot. After
  `checkpoint-hs10` teleports Seedtest to Stormwind, guards kill the spawned kobold, so `heartseed-xuen` failed
  whenever it ran after a passing checkpoint scenario.
- SQL applied twice to `world_claude`.

## Decisions made
- Helper auras are borrowed client spells with their amount and duration overwritten: Berserking 26297, Sprint
  2983, Curse of Impotence 51340, Ice Barrier 11426 and Jade Serpent Wave 118540. Tooltips show those names
  until a client patch.
- Kills count only when the player lands the killing blow (`OnCreatureKill`), not the tiger's kills. Critters
  don't count.

## Not verified live
- The Xuen lightning chain, kill cooldown cuts, the Serpent's Breath riders and the mounted catch-up have no
  automated check. They need a playtest look.

## For the other agent
- Apply `sql/updates/world/2026_11_02_claude_00_heartseed_gift_riders.sql` if you test the Heartseed.

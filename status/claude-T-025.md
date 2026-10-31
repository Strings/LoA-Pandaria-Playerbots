# T-025: Heartseed granting quest

**Agent:** claude
**Branch:** claude/T-025-granting-quest
**Task status:** review

## What changed
- **Quest 90000 "The Heartseed"** is the canonical "has a Heartseed" marker. Rewarding it grants the Heartseed,
  however the quest is completed, so the Theramore intro scenario can complete the same quest later.
- **No NPC needed:** at login a character with no Heartseed that has not done the quest gets the quest window
  after `Heartseed.Grant.OfferDelay` (5 s), with itself as the quest giver (quest flags AUTO_TAKE |
  AUTO_COMPLETE, which the core's accept and reward handlers already allow). Accepting completes it and opens
  the reward window; turning it in grants the Heartseed ("A Heartseed has taken root in your weapon.").
- Declining offers it again next login; logging out at the reward window reopens the reward window.
- Short quest and reward text: the seed finds you and settles into your weapon; feeding and awakenings are
  hinted at. It also serves as the system's introduction (Definition of done).
- `.heartseed offer` shows it now; `.heartseed revoke` also resets the quest. Config: `Heartseed.Grant.QuestId`,
  `.OfferOnLogin`, `.OfferDelay`.

## How it was tested
Claude test server, headless client, `Seedtest`. SQL applied twice to `world_claude`.
- `.heartseed revoke`, relog: `SMSG_QUESTGIVER_QUEST_DETAILS` for 90000 about 5 s after login; `quest-accept
  90000 <own guid>`, `quest-reward 90000 0 <own guid>`: Heartseed granted at rank 1 with the core aura and glow;
  quest in `character_queststatus_rewarded`.
- Next login: no offer; `.heartseed offer` refused.
- Revoke, then two logins without accepting: offered on each; then accepted and granted again.
- Built with no new warnings in touched files.

## Decisions made
- **Self-given quest instead of a starting-zone NPC:** works in every starting zone (and for existing
  characters) with no creatures to place, and the core already supports it. The Theramore scenario replaces how
  it is delivered, not the quest.
- **Offered at any level** when the character has no Heartseed, so characters made before this change get it.
- **A GM grant does not complete the quest.** Code checks the state row; the quest decides who is offered it.
- Placeholder text, to be rewritten with the Theramore scenario (the Celestial payoff line is kept out of it).

## For the other agent
- Apply `sql/updates/world/2026_10_31_claude_09_heartseed_grant_quest.sql` to your world DB. Characters without
  a Heartseed will see the quest window 5 s after login; set `Heartseed.Grant.OfferOnLogin = 0` in a test
  profile if it gets in the way.
- Headless: accept and reward a self-given quest with the character's own guid
  (`quest-accept 90000 <guid>`, `quest-reward 90000 0 <guid>`).

## Next
- T-023 path-choice gossip.

## Map additions
- None.

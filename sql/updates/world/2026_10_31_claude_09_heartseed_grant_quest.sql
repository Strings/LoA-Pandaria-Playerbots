-- Heartseed granting quest (T-025): the canonical "has a Heartseed" marker. The server offers it with the player
-- as quest giver (Flags: AUTO_TAKE 0x200000 | AUTO_COMPLETE 0x10000); rewarding it grants the Heartseed. The
-- Theramore intro scenario will later complete this same quest. Quest ID from the Heartseed block (docs/ID_RANGES.md).
DELETE FROM `quest_template` WHERE `ID` = 90000;
INSERT INTO `quest_template` (`ID`, `QuestType`, `QuestLevel`, `MinLevel`, `QuestSortID`, `QuestInfoID`, `RewardXPDifficulty`, `Flags`, `AllowableRaces`,
    `LogTitle`, `LogDescription`, `QuestDescription`, `AreaDescription`, `QuestCompletionLog`, `VerifiedBuild`) VALUES
(90000, 2, -1, 1, 0, 0, 0, 2162688, 0,
    'The Heartseed',
    'Let the seed take root in your weapon.',
    'A mote of light drifts down out of nowhere, slows, and settles into your open hand. It is warm. It is small. And, unmistakably, it is alive.$B$BIt passed by stronger hands to reach yours, and it shows no sign of wanting to leave. When you hold your weapon, it presses toward it, the way a seed presses toward soil.$B$BWhatever it is, it wants to grow with you.',
    '', '', 0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 90000;
INSERT INTO `quest_offer_reward` (`ID`, `RewardText`, `VerifiedBuild`) VALUES
(90000, 'The seed sinks into your weapon with a faint glow, and something in it settles, content.$B$BIt grows as you do. When you replace your weapon, it will ask to be fed the old one: weapons you outgrow are what it feeds on. Some places and moments will wake it further.', 0);

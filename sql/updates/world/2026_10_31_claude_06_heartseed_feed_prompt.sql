-- T-022: text of the Heartseed's feed prompt, a gossip menu the Heartseed opens from the player when a better
-- weapon replaces the main hand (Heartseed gossip block, docs/ID_RANGES.md). The options name the weapon.
-- Idempotent: the row is replaced by id.

DELETE FROM `npc_text` WHERE `ID` = 90001;
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `BroadcastTextID0`, `lang0`, `Probability0`) VALUES
(90001, 'Your Heartseed stirs and turns toward the weapon you just set aside. It is hungry for it.$B$BFeed it, and the weapon is gone for good; keep it, and the Heartseed will not ask about it again until you log in.', '', 0, 0, 1);

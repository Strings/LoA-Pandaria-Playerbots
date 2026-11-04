-- T-089: permission for '.rare' (GM: inspect, spawn, reroll, set and claim affixed rares). ID from the affixes block
-- (docs/ID_RANGES.md); role 197, Gamemaster Commands. Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (98000, 'Command: rare') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 98000) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

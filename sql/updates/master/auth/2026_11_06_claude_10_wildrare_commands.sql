-- T-100: permission for '.wildrare' (GM: spawn, explain, reset, stats and chance for wild rares). ID from the affixes
-- and rares block (docs/ID_RANGES.md); role 197, Gamemaster Commands. Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (98001, 'Command: wildrare') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 98001) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

-- T-093: permission for '.treasure' (GM: list, go, spawn, reset, explain, validate and check world treasures). ID from
-- the treasures block (docs/ID_RANGES.md); role 197, Gamemaster Commands. Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (99000, 'Command: treasure') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 99000) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

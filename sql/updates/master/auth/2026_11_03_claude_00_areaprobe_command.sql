-- T-086: .areaprobe, from the GM tools block (docs/ID_RANGES.md), linked to role 197 (Gamemaster Commands).
-- Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (91010, 'Command: areaprobe')
    ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 91010)
    ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

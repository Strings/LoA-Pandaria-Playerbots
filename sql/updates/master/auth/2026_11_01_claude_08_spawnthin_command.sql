-- T-081: '.spawnthin' (info, near) for GMs, role 197. ID from the GM tools block (docs/ID_RANGES.md). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (91009, 'Command: spawnthin') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 91009) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

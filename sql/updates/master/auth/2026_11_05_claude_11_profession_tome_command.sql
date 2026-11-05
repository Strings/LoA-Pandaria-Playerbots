-- T-096: permission for '.proftome' (GM: status, grant, summon and check the profession tome pet). ID from
-- the profession tome block (docs/ID_RANGES.md); role 197, Gamemaster Commands. Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (101000, 'Command: proftome') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 101000) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

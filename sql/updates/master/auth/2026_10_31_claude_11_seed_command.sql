-- T-023: player permission for '.seed' (opens the Heartseed's choice window). ID from the Heartseed gossip block
-- (docs/ID_RANGES.md). Linked to role 199 (Player Commands). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (90001, 'Command: seed') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (199, 90001) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

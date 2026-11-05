-- T-094: '.multiclass' (GM: inspect and set second classes) and '.mc' (players: the Multiclass addon's sync and
-- switch). IDs from the multi-classing block (docs/ID_RANGES.md). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (100000, 'Command: multiclass'),
    (100001, 'Command: mc') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 100000),
    (199, 100001) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

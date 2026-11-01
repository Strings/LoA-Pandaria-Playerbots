-- T-043: '.reagent' for players (role 199, Player Commands) and '.reagent inspect/add' for GMs (role 197).
-- IDs from the Loadouts and detached storage block (docs/ID_RANGES.md). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (93002, 'Command: reagent'),
    (93003, 'Command: reagent inspect, add') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (199, 93002),
    (197, 93003) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

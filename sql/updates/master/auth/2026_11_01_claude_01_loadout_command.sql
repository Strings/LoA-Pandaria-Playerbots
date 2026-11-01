-- T-017: '.loadout' for players (role 199, Player Commands) and '.loadout inspect' for GMs (role 197).
-- IDs from the Loadouts gossip block (docs/ID_RANGES.md). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (93000, 'Command: loadout'),
    (93001, 'Command: loadout inspect') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (199, 93000),
    (197, 93001) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

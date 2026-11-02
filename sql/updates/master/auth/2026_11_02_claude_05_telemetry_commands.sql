-- T-004: permissions for '.telemetry'. IDs from the telemetry block (docs/ID_RANGES.md). Players control their
-- own logging (status, pause, resume, discard, tag: role 199, Player Commands); GMs inspect it (snapshot, flush,
-- track: role 197, Gamemaster Commands). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (95000, 'Command: telemetry'),
    (95001, 'Command: telemetry gm') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (199, 95000),
    (197, 95001) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

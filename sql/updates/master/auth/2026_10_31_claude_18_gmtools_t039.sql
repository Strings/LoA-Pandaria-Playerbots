-- T-039: GM tools, first set. IDs from the GM tools block (docs/ID_RANGES.md), linked to role 197
-- (Gamemaster Commands). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (91003, 'Command: clock'),
    (91004, 'Command: bot (inspect)'),
    (91005, 'Command: scenario'),
    (91006, 'Command: spectate')
    ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 91003), (197, 91004), (197, 91005), (197, 91006)
    ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

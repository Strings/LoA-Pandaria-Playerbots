-- T-048: GM test suite commands. IDs from the GM tools block (docs/ID_RANGES.md), linked to role 197
-- (Gamemaster Commands). Also adds the playerbots test commands' permission (1345, .pmon, .botscenario, .probe):
-- the module never shipped its row, so only the console could run them; checkpoint gear steps use .botscenario.
-- Linked to role 196 (Administrator Commands). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (91000, 'Command: checkpoint'),
    (91001, 'Command: kit'),
    (91002, 'Command: gmsuite')
    ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 91000), (197, 91001), (197, 91002)
    ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (1345, 'Command: pmon, botscenario, probe (playerbots test commands)')
    ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (196, 1345)
    ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

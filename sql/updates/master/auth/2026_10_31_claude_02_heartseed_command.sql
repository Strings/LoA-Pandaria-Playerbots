-- T-020: GM permission for '.heartseed' (Custom/Heartseed/heartseed_commandscript.cpp). The ID is taken from
-- the Heartseed block (docs/ID_RANGES.md). Linked to role 197 (Gamemaster Commands). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (90000, 'Command: heartseed') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (197, 90000) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

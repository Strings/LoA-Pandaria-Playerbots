-- BUG-004: '.idea' and '.bug' for players (role 199, Player Commands); '.capture list/show' for GMs (role 197),
-- since they read every player's tickets. They used the ticket permission (742), which only moderators have.
-- IDs from the GM tools block (docs/ID_RANGES.md). Idempotent.
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
    (91007, 'Command: idea, bug'),
    (91008, 'Command: capture') ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
    (199, 91007),
    (197, 91008) ON DUPLICATE KEY UPDATE `linkedId` = VALUES(`linkedId`);

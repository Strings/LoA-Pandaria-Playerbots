DELETE FROM `rbac_linked_permissions` WHERE (`id`=199 AND `linkedId`=96000) OR (`id`=197 AND `linkedId`=96001);
DELETE FROM `rbac_permissions` WHERE `id` IN (96000,96001);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES (96000,'Command: legacy (self)'),(96001,'Command: legacy (force and selected player)');
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES (199,96000),(197,96001);

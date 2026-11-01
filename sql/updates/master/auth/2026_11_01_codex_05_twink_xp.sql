DELETE FROM `rbac_linked_permissions` WHERE (`id`=199 AND `linkedId`=95000) OR (`id`=197 AND `linkedId`=95001);
DELETE FROM `rbac_permissions` WHERE `id` IN (95000,95001);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES (95000,'Command: xp (self)'),(95001,'Command: xp (selected player)');
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES (199,95000),(197,95001);

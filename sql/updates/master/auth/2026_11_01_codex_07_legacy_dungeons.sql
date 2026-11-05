DELETE FROM `rbac_linked_permissions` WHERE (`id`=199 AND `linkedId`=97000) OR (`id`=197 AND `linkedId`=97001);
DELETE FROM `rbac_permissions` WHERE `id` IN (97000,97001);
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES (97000,'Command: legacy (self)'),(97001,'Command: legacy (force and selected player)');
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES (199,97000),(197,97001);

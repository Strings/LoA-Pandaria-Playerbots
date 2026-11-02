-- SoloQoL GM hooks, reserved gossip/RBAC block; no player chat permission needed for gossip.
DELETE FROM `rbac_linked_permissions` WHERE `id`=197 AND `linkedId`=94000;
DELETE FROM `rbac_permissions` WHERE `id`=94000;
INSERT INTO `rbac_permissions` (`id`,`name`) VALUES (94000,'Command: soloqol');
INSERT INTO `rbac_linked_permissions` (`id`,`linkedId`) VALUES (197,94000);

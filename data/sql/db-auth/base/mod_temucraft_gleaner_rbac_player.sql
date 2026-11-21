-- mod-temucraft-gleaner: RBAC permissions 103750-103799, linked to the stock player role 195 and the Temucraft player role 1010 so every
-- account has them. Remove a link (or .rbac account deny <account> <id>) to take the feature away.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 103750 AND 103799;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 103750 AND 103799;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(103750, 'Temucraft: use the Gleaner-3000');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 103750),
(1010, 103750);

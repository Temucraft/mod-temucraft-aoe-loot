-- mod-temucraft-gleaner: the Gleaner-3000 item (sold on the Temucraft Curator's Gameplay list for 250 tokens,
-- ItemExtendedCost 3211 from mod-temucraft-shop), its bot (the stock mechanical squirrel look, bigger) and texts.
SET @ITEM := 610105; SET @BOT := 608970; SET @GAMEPLAY := 608003; SET @PRICE := 3211;
DELETE FROM `item_template` WHERE `entry` = @ITEM;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `ItemLevel`, `maxcount`, `stackable`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `bonding`, `description`, `Material`, `ScriptName`) VALUES
(@ITEM, 15, 0, -1, 'Gleaner-3000', 34494, 3, 0, 1, 0, 0, 0, 1, 1, 1, 18282, 0, 0, 3000, 1, 'Calls (or sends home) a loot bot: while it follows you and you play alone in the open world, what you kill drops straight into your bags.', 4, 'item_temucraft_gleaner');
DELETE FROM `creature_template_model` WHERE `CreatureID` = @BOT;
DELETE FROM `creature_template` WHERE `entry` = @BOT;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `unit_class`, `unit_flags`, `type`, `flags_extra`) VALUES
(@BOT, 'Gleaner-3000', 'Loot Bot', 1, 1, 35, 0, 1.2, 1.5, 1, 2 | 256 | 512 | 33554432, 9, 2);
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`) VALUES (@BOT, 0, 7937, 1.6, 1);
DELETE FROM `npc_vendor` WHERE `entry` = @GAMEPLAY AND `item` = @ITEM;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`) VALUES (@GAMEPLAY, 101, @ITEM, 0, 0, @PRICE, 0);

DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-gleaner';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-gleaner', 1, '|cff66ccffGleaner-3000:|r powering down.'),
('mod-temucraft-gleaner', 2, '|cff66ccffGleaner-3000:|r not now.'),
('mod-temucraft-gleaner', 3, '|cff66ccffGleaner-3000:|r online. Loot goes to your bags while you play alone in the open world.'),
('mod-temucraft-gleaner', 4, '|cff66ccffGleaner-3000:|r online.'),
('mod-temucraft-gleaner', 5, 'You can''t use this here.');

-- mod-temucraft-aoe-loot: player-facing texts. Translations go in module_string_locale (same module and id, plus the locale).
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-aoe-loot';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-aoe-loot', 1, '|cff66ccffLooted {} more corpse{}|r ({} item{}{}).');

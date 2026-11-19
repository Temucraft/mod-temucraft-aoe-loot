# mod-temucraft-aoe-loot

Open one corpse and you also loot the others around it: their gold and plain items are moved into the corpse you
opened (30 yards, up to 10 corpses).

- **Safe with the client.** The 3.3.5 client stores loot in an 18-slot table and writes each item at the slot number
  the server sends without checking it; a slot of 18 or more corrupts its memory and crashes it. Merging always stops
  before plain items plus quest items reach 18.
- **Nothing duplicated or lost.** Quest items, free-for-all items and items with conditions are tracked per player, so
  they stay on their own corpse, which stays lootable until they are taken. Corpses emptied by the merge stop being
  lootable and decay as usual.
- **Groups:** only with free-for-all loot, so need/greed rolls and master loot work as before.
- Skinning, mining and other gathering loot are not merged.

C++ only, no SQL, no client changes. Settings: `conf/mod-temucraft-aoe-loot.conf.dist`.

## Texts and translations

The 1 player-facing texts (chat messages, gossip options) are in the `module_string` table under the module name
`mod-temucraft-aoe-loot`, so they can be changed without rebuilding. Translations go in `module_string_locale` with the same
module and id plus the locale (`deDE`, `esES`, `frFR`, ...). If a row is missing the English text in the code is used.

Part of the Temucraft modules for AzerothCore 3.3.5a.

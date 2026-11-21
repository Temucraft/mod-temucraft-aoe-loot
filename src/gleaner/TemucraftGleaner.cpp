/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Gleaner-3000: a little loot bot. While it follows you and you play alone in the open world, everything a monster you
// (or your pet) kill drops goes straight into your bags: gold and items. Quest items, items you aren't allowed and
// anything that doesn't fit stay on the corpse. Not in groups or instances by default, so group loot rules stay fair.

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "Item.h"
#include "LootMgr.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PetDefines.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "TemporarySummon.h"
#include "TemucraftModuleText.h"
#include <unordered_map>

namespace
{
    constexpr uint32 PERM_USE = 103750;   // RBAC permission (db-auth SQL), linked to the player roles
    char const* const MODULE = "mod-temucraft-gleaner";

    struct Settings
    {
        bool enable = true;
        uint32 creature = 608970;
        bool inGroups = false, inInstances = false;
        float range = 60.0f;
    } cfg;

    std::unordered_map<ObjectGuid::LowType, ObjectGuid> bots;   // player -> its bot

    char const* T(Player* p, uint32 id, char const* fb) { return TemucraftModuleText(MODULE, p, id, fb); }

    Creature* BotOf(Player* player)
    {
        auto it = bots.find(player->GetGUID().GetCounter());
        if (it == bots.end())
            return nullptr;
        Creature* bot = ObjectAccessor::GetCreature(*player, it->second);
        if (!bot || !bot->IsInWorld() || bot->GetMapId() != player->GetMapId())
        {
            bots.erase(it);
            return nullptr;
        }
        return bot;
    }

    void Dismiss(Player* player)
    {
        if (Creature* bot = BotOf(player))
            bot->DespawnOrUnsummon();
        bots.erase(player->GetGUID().GetCounter());
    }

    void Glean(Player* player, Creature* killed)
    {
        if (!cfg.enable || !player || !killed || !BotOf(player))
            return;
        if ((!cfg.inGroups && player->GetGroup()) || (!cfg.inInstances && player->GetMap()->Instanceable()))
            return;
        if (!player->isAllowedToLoot(killed) || !killed->IsWithinDistInMap(player, cfg.range))
            return;
        Loot& loot = killed->loot;
        bool took = false;
        for (LootItem& li : loot.items)
        {
            if (li.is_looted || li.needs_quest || li.is_blocked || !li.AllowedForPlayer(player, killed->GetGUID()))
                continue;
            ItemPosCountVec dest;
            if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, li.itemid, li.count) != EQUIP_ERR_OK)
                continue;
            if (Item* item = player->StoreNewItem(dest, li.itemid, true, li.randomPropertyId))
            {
                player->SendNewItem(item, li.count, false, false, true);
                li.is_looted = true;
                if (loot.unlootedCount)
                    --loot.unlootedCount;
                took = true;
            }
        }
        if (loot.gold)
        {
            player->ModifyMoney(int32(loot.gold));
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, loot.gold);
            loot.gold = 0;
            took = true;
        }
        if (took && loot.isLooted() && loot.quest_items.empty())
        {
            killed->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
            killed->AllLootRemovedFromCorpse();
        }
    }
}

class TemucraftGleaner_Player : public PlayerScript
{
public:
    TemucraftGleaner_Player() : PlayerScript("TemucraftGleaner_Player") { }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override { Glean(killer, killed); }
    void OnPlayerCreatureKilledByPet(Player* owner, Creature* killed) override { Glean(owner, killed); }
    void OnPlayerLogout(Player* player) override { Dismiss(player); }
    void OnPlayerMapChanged(Player* player) override { Dismiss(player); }
};

// the Gleaner-3000 item: calls the bot, or sends it home
class item_temucraft_gleaner : public ItemScript
{
public:
    item_temucraft_gleaner() : ItemScript("item_temucraft_gleaner") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) override
    {
        ChatHandler handler(player->GetSession());
        if (!cfg.enable)
            return true;
        if (!player->GetSession()->HasPermission(PERM_USE))
        {
            handler.SendSysMessage(T(player, 5, "You can't use this here."));
            return true;
        }
        if (BotOf(player))
        {
            Dismiss(player);
            handler.SendSysMessage(T(player, 1, "|cff66ccffGleaner-3000:|r powering down."));
            return true;
        }
        if (player->IsInCombat() || player->GetMap()->IsBattlegroundOrArena())
        {
            handler.SendSysMessage(T(player, 2, "|cff66ccffGleaner-3000:|r not now."));
            return true;
        }
        float x, y, z;
        player->GetClosePoint(x, y, z, player->GetObjectSize(), PET_FOLLOW_DIST);
        if (TempSummon* bot = player->SummonCreature(cfg.creature, x, y, z, player->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
        {
            bot->SetOwnerGUID(player->GetGUID());
            bot->SetFaction(player->GetFaction());
            bot->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            bots[player->GetGUID().GetCounter()] = bot->GetGUID();
            handler.SendSysMessage((!cfg.inGroups || !cfg.inInstances)
                ? T(player, 3, "|cff66ccffGleaner-3000:|r online. Loot goes to your bags while you play alone in the open world.")
                : T(player, 4, "|cff66ccffGleaner-3000:|r online."));
        }
        return true;
    }
};

class TemucraftGleaner_World : public WorldScript
{
public:
    TemucraftGleaner_World() : WorldScript("TemucraftGleaner_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        cfg.enable = sConfigMgr->GetOption<bool>("Temucraft.Gleaner.Enable", true);
        cfg.creature = sConfigMgr->GetOption<uint32>("Temucraft.Gleaner.Creature", 608970);
        cfg.inGroups = sConfigMgr->GetOption<bool>("Temucraft.Gleaner.InGroups", false);
        cfg.inInstances = sConfigMgr->GetOption<bool>("Temucraft.Gleaner.InInstances", false);
        cfg.range = sConfigMgr->GetOption<float>("Temucraft.Gleaner.Range", 60.0f);
    }
};

void AddTemucraftGleanerScripts()
{
    new TemucraftGleaner_Player();
    new item_temucraft_gleaner();
    new TemucraftGleaner_World();
}

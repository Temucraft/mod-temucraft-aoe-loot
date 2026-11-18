/*
 * mod-temucraft-aoe-loot: loot several nearby corpses at once.
 *
 * When you open a corpse, the gold and the plain items of other corpses you may loot within range are moved into
 * it. Quest items, free-for-all items and items with conditions stay on their own corpse (they are tracked per
 * player), so nothing is duplicated or lost; a corpse that still holds such items stays lootable.
 *
 * The 3.3.5 client keeps an 18-slot loot table and writes every item into it at the slot number the server sends
 * without checking it, so a slot of 18 or more corrupts its memory and crashes it. The server numbers plain items
 * first and quest items after them, so merging stops while plain items + quest items would go past 18.
 *
 * In a group only free-for-all loot is merged (Temucraft.AoeLoot.Groups), so need/greed rolls and master loot are
 * never bypassed.
 */
#include "CellImpl.h"
#include "TemucraftModuleText.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "LootMgr.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptMgr.h"
#include "WorldScript.h"
#include <algorithm>
#include <list>

namespace
{
    constexpr uint32 CLIENT_LOOT_SLOTS = 18;

    struct Settings
    {
        bool enable = true;
        float range = 30.0f;
        uint32 maxCorpses = 10;
        bool groups = true;
        bool message = true;
    } sSet;

    void LoadConfig()
    {
        sSet.enable = sConfigMgr->GetOption<bool>("Temucraft.AoeLoot.Enable", true);
        sSet.range = sConfigMgr->GetOption<float>("Temucraft.AoeLoot.Range", 30.0f);
        sSet.maxCorpses = sConfigMgr->GetOption<uint32>("Temucraft.AoeLoot.MaxCorpses", 10);
        sSet.groups = sConfigMgr->GetOption<bool>("Temucraft.AoeLoot.Groups", true);
        sSet.message = sConfigMgr->GetOption<bool>("Temucraft.AoeLoot.Message", true);
    }

    bool Movable(LootItem const& item)
    {
        return !item.is_looted && !item.is_blocked && !item.freeforall && !item.needs_quest && item.conditions.empty()
            && item.rollWinnerGUID.IsEmpty();
    }

    struct DeadCreatureCheck
    {
        explicit DeadCreatureCheck(Creature const* except) : _except(except) { }
        bool operator()(Creature* c) const { return c != _except && !c->IsAlive(); }
        Creature const* _except;
    };
}

class TemucraftAoeLoot_Player : public PlayerScript
{
public:
    TemucraftAoeLoot_Player() : PlayerScript("TemucraftAoeLoot_Player", { PLAYERHOOK_ON_BEFORE_SEND_LOOT }) { }

    void OnPlayerBeforeSendLoot(Player* player, ObjectGuid lootGuid, Loot* loot) override
    {
        if (!sSet.enable || !loot || loot->loot_type != LOOT_CORPSE || !lootGuid.IsCreatureOrVehicle())
            return;
        if (Group* group = player->GetGroup())
            if (!sSet.groups || group->GetLootMethod() != FREE_FOR_ALL)
                return;

        Creature* anchor = player->GetMap()->GetCreature(lootGuid);
        if (!anchor)
            return;

        std::list<Creature*> corpses;
        DeadCreatureCheck check(anchor);
        Acore::CreatureListSearcher<DeadCreatureCheck> searcher(player, corpses, check);
        Cell::VisitObjects(anchor, searcher, sSet.range);
        corpses.sort([anchor](Creature* a, Creature* b) { return anchor->GetDistance(a) < anchor->GetDistance(b); });

        uint32 merged = 0, corpsesUsed = 0;
        uint32 goldBefore = loot->gold;
        for (Creature* c : corpses)
        {
            if (corpsesUsed >= sSet.maxCorpses)
                break;
            Loot& src = c->loot;
            if (src.loot_type == LOOT_SKINNING || src.isLooted() || !player->isAllowedToLoot(c))
                continue;
            if (!src.roundRobinPlayer.IsEmpty() && src.roundRobinPlayer != player->GetGUID())
                continue;

            bool used = false;
            if (src.gold)
            {
                loot->gold += src.gold;
                src.gold = 0;
                used = true;
            }
            for (uint8 i = 0; i < src.items.size(); ++i)
            {
                LootItem& item = src.items[i];
                if (!Movable(item))
                    continue;
                // client limit: plain items first, quest items after them, all below slot 18
                if (loot->items.size() + loot->quest_items.size() >= CLIENT_LOOT_SLOTS)
                    break;
                LootItem copy = item;
                copy.itemIndex = loot->items.size();
                loot->items.push_back(copy);
                ++loot->unlootedCount;
                item.is_looted = true;
                if (src.unlootedCount)
                    --src.unlootedCount;
                src.NotifyItemRemoved(i);
                ++merged;
                used = true;
            }
            if (!used)
                continue;
            ++corpsesUsed;
            if (src.isLooted())
            {
                c->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
                c->AllLootRemovedFromCorpse();
            }
            if (loot->items.size() + loot->quest_items.size() >= CLIENT_LOOT_SLOTS)
                break;
        }

        if (corpsesUsed && sSet.message)
            ChatHandler(player->GetSession()).PSendSysMessage(TemucraftModuleText("mod-temucraft-aoe-loot", player->GetSession(), 1, "|cff66ccffLooted {} more corpse{}|r ({} item{}{})."), corpsesUsed,
                corpsesUsed == 1 ? "" : "s", merged, merged == 1 ? "" : "s", loot->gold > goldBefore ? ", and their gold" : "");
    }
};

class TemucraftAoeLoot_World : public WorldScript
{
public:
    TemucraftAoeLoot_World() : WorldScript("TemucraftAoeLoot_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override { LoadConfig(); }
};

void AddTemucraftAoeLootScripts()
{
    new TemucraftAoeLoot_Player();
    new TemucraftAoeLoot_World();
}

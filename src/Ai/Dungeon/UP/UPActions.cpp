/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UPActions.h"
#include "GameObject.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Spell.h"
#include "Playerbots.h"
#include "UPTriggers.h"

bool YmironBaneStopAttackAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "king ymiron");
    if (!boss)
        return false;

    Spell* spell = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    return bot->GetVictim() == boss || AI_VALUE(Unit*, "current target") == boss ||
        (spell && spell->m_targets.GetUnitTarget() == boss);
}

bool YmironBaneStopAttackAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "king ymiron");
    if (!boss)
        return false;

    if (bot->GetVictim() == boss)
        bot->AttackStop();

    for (CurrentSpellTypes type : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL })
        if (Spell* spell = bot->GetCurrentSpell(type))
            if (spell->m_targets.GetUnitTarget() == boss)
                bot->InterruptSpell(type);

    if (AI_VALUE(Unit*, "current target") == boss)
    {
        context->GetValue<Unit*>("current target")->Set(nullptr);
        bot->SetTarget(ObjectGuid::Empty);
    }
    return true;
}

bool AvoidFreezingCloudAction::Execute(Event /*event*/)
{
    Unit* closestTrigger = nullptr;
    GuidVector objects = AI_VALUE(GuidVector, "nearest hostile npcs");

    for (auto i = objects.begin(); i != objects.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->GetEntry() == NPC_BREATH_TRIGGER)
        {
            if (!closestTrigger || bot->GetExactDist2d(unit) < bot->GetExactDist2d(closestTrigger))
            {
                closestTrigger = unit;
            }
        }
    }

    if (!closestTrigger) { return false; }

    float distance = bot->GetExactDist2d(closestTrigger->GetPosition());
    float radius = 3.0f;
    // Large buffer for this - the radius of the breath is a lot smaller than the graphic, but it looks dumb
    // if the bot stands just outside the hitbox but still visibly in the cloud patches.
    float distanceExtra = 3.0f;

    if (distance < radius + distanceExtra - 1.0f)
    {
        // bot->Yell("MOVING", LANG_UNIVERSAL);
        return MoveAway(closestTrigger, radius + distanceExtra - distance);
    }

    return false;
}

bool AvoidSkadiWhirlwindAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "skadi the ruthless");
    if (!boss) { return false; }

    float distance = bot->GetExactDist2d(boss->GetPosition());
    float radius = 5.0f;
    float distanceExtra = 2.0f;

    if (distance < radius + distanceExtra)
    {
        if (botAI->IsTank(bot))
        {
            // The boss chases tank during this, leads to jittery stutter-stepping
            // by the tank if we don't pre-move additional range. 2*radius seems ok
            return MoveAway(boss, (2.0f * radius) + distanceExtra - distance);
        }
        // else
        return MoveAway(boss, radius + distanceExtra - distance);
    }

    return false;
}

bool SkadiHarpoonPickupAction::Execute(Event /*event*/)
{
    GameObject* harpoon = bot->FindNearestGameObject(GO_HARPOON, 60.0f);
    if (!harpoon || !harpoon->isSpawned())
        return false;

    if (bot->GetDistance(harpoon) > INTERACTION_DISTANCE - 1.0f)
        return MoveTo(bot->GetMapId(), harpoon->GetPositionX(), harpoon->GetPositionY(), harpoon->GetPositionZ());

    // Same path as a client click (the GO is a goober with no lock): it casts Create Harpoon on the user.
    WorldPacket usePacket(CMSG_GAMEOBJ_USE);
    usePacket << harpoon->GetGUID();
    bot->GetSession()->HandleGameObjectUseOpcode(usePacket);
    return true;
}

bool SkadiHarpoonLaunchAction::Execute(Event /*event*/)
{
    Item* item = bot->GetItemByEntry(ITEM_HARPOON);
    Creature* grauf = bot->FindNearestCreature(NPC_GRAUF, 250.0f, true);
    if (!item || !grauf)
        return false;

    GameObject* launcher = nullptr;
    for (uint32 entry : { GO_HARPOON_LAUNCHER_1, GO_HARPOON_LAUNCHER_2, GO_HARPOON_LAUNCHER_3 })
        if (GameObject* go = bot->FindNearestGameObject(entry, 250.0f))
            if (!launcher || bot->GetDistance(go) < bot->GetDistance(launcher))
                launcher = go;
    if (!launcher)
        return false;

    if (bot->GetDistance(launcher) > INTERACTION_DISTANCE - 1.0f)
        return MoveTo(bot->GetMapId(), launcher->GetPositionX(), launcher->GetPositionY(), launcher->GetPositionZ());

    // The launcher fires a 60 yd cone that only reaches Grauf at his hover point; wait for him there.
    if (grauf->GetExactDist2d(GRAUF_HOVER_POSITION) > 15.0f || bot->IsNonMeleeSpellCast(false))
    {
        if (!sPlayerbotAIConfig.logInGroupOnly)
            LOG_DEBUG("playerbots", "skadi-harpoon bot={} waiting grauf=({:.1f},{:.1f},{:.1f}) hoverDist={:.1f}",
                      bot->GetName(), grauf->GetPositionX(), grauf->GetPositionY(), grauf->GetPositionZ(),
                      grauf->GetExactDist2d(GRAUF_HOVER_POSITION));
        return false;
    }

    uint32 spellId = 0;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        if (item->GetTemplate()->Spells[i].SpellId > 0)
            spellId = item->GetTemplate()->Spells[i].SpellId;
    if (!spellId)
        return false;

    // Use the item on the launcher the way the client does (CMSG_USE_ITEM with a gameobject target), so the
    // launcher's lock checks the harpoon.
    WorldPacket packet(CMSG_USE_ITEM);
    packet << item->GetBagSlot() << item->GetSlot() << uint8(1) << spellId << item->GetGUID() << uint32(0)
           << uint8(0) << uint32(TARGET_FLAG_GAMEOBJECT);
    packet << launcher->GetGUID().WriteAsPacked();
    bot->GetSession()->HandleUseItemOpcode(packet);
    if (!sPlayerbotAIConfig.logInGroupOnly)
        LOG_DEBUG("playerbots", "skadi-harpoon bot={} used launcher={} spell={} stillHasItem={}", bot->GetName(),
                  launcher->GetEntry(), spellId, bot->HasItemCount(ITEM_HARPOON, 1));
    return true;
}

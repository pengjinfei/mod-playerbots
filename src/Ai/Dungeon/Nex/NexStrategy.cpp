/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexStrategy.h"
#include "NexMultipliers.h"
#include "NexTriggers.h"

WotlkDungeonNexStrategy::WotlkDungeonNexStrategy(PlayerbotAI* ai) : TrashCcPullStrategy(ai)
{
    // 守卫组里的治疗小怪，控制优先分给它们（普通/英雄 entry）。
    TrashCcRegisterHealerEntries({ NPC_CRYSTALLINE_TENDER, NPC_CRYSTALLINE_TENDER_HEROIC,
                                   NPC_MAGE_HUNTER_INITIATE, NPC_MAGE_HUNTER_INITIATE_HEROIC });
}

void WotlkDungeonNexStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // Horde Commander (Alliance N)/Commander Kolurg (Alliance H)
    // or
    // Alliance Commander (Horde N)/Commander Stoutbeard (Horde H)
    triggers.push_back(new TriggerNode("faction commander whirlwind",
        { NextAction("move from whirlwind", ACTION_MOVE + 5) }));
    // TODO: Handle fear? (tremor totems, fear ward etc.)

    // Grand Magus Telestra
    triggers.push_back(new TriggerNode("telestra firebomb",
        { NextAction("firebomb spread", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("telestra split phase",
        { NextAction("telestra split target", ACTION_RAID + 1) }));
    // TODO: Add priority interrupt on the frost split's Blizzard casts

    // Anomalus
    triggers.push_back(new TriggerNode("chaotic rift",
        { NextAction("chaotic rift target", ACTION_RAID + 1) }));

    // 前置守卫组的清怪控制链：共享实现在 TrashCcPullStrategy（MarkRtiStrategy.h），这里只是挂上。
    TrashCcPullStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode("intense cold",
        { NextAction("intense cold jump", ACTION_MOVE + 5) }));
    // Flank dragon positioning
    triggers.push_back(new TriggerNode("keristrasza positioning",
        { NextAction("rear flank", ACTION_MOVE + 4) }));
    // TODO: Add frost resist aura for paladins?
}

void WotlkDungeonNexStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new FactionCommanderMultiplier(botAI));
    multipliers.push_back(new TelestraMultiplier(botAI));
    multipliers.push_back(new AnomalusMultiplier(botAI));
    multipliers.push_back(new OrmorokMultiplier(botAI));
    multipliers.push_back(new NexusNoKnockbackMultiplier(botAI));
}

/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "ANStrategy.h"
#include "ANMultipliers.h"

void WotlkDungeonANStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // 清怪控制链：门厅那两组守望者（6 只，彼此只隔 15 码）在基线 run436 里 3–5 秒内一起进战斗，
    // 四场承伤 424,711、五场只杀掉 2 只。整本小怪是亡灵，唯一有效的控制是牧师束缚亡灵。
    TrashCcPullStrategy::InitTriggers(triggers);

    // Krik'thir the Gatewatcher
    // TODO: Add CC trigger while web wraps are casting?
    // TODO: Bring healer closer than ranged dps to avoid fixates?
    triggers.push_back(new TriggerNode("krik'thir web wrap",
        { NextAction("attack web wrap", ACTION_RAID + 5) }));
    triggers.push_back(new TriggerNode("krik'thir watchers",
        { NextAction("krik'thir priority", ACTION_RAID + 4) }));

    // Hadronox
    // The core AC triggers are very buggy with this boss, but default strat seems to play correctly

    //Anub'arak
    // 穿刺：上游原注释说追踪不到这些尖刺，实测不成立——它是 creature 29184，
    // 生成后 4 秒才落伤害，半径只有 4 码（英雄 7,539 + 击退），走开两步就能躲。
    // 不躲的代价：run445 a3 一场吃了 105,838，约占全队承伤的四分之一。
    // 与躲践踏同优先级：践踏只在 boss 浮出时、穿刺只在下潜期，两者不会同时触发。
    triggers.push_back(new TriggerNode("anub'arak impale",
        { NextAction("dodge impale", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("anub'arak pound",
        { NextAction("dodge pound", ACTION_MOVE + 5) }));
}

void WotlkDungeonANStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new KrikthirMultiplier(botAI));
}

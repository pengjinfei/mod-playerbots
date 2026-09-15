/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "UKStrategy.h"
#include "UKMultipliers.h"

void WotlkDungeonUKStrategy::InitTriggers(std::vector<TriggerNode*> &triggers)
{
    // Prince Keleseth
    triggers.push_back(new TriggerNode("keleseth frost tomb",
             { NextAction("attack frost tomb", ACTION_RAID + 1) }));

    // Skarvald the Constructor & Dalronn the Controller
    triggers.push_back(new TriggerNode("dalronn priority",
             { NextAction("attack dalronn", ACTION_RAID + 1) }));

    // Ingvar the Plunderer

    // No easy way to check LoS here, the pillars do not seem to count as gameobjects.
    // Not implemented for now, unsure if this is needed as a good group can probably burst through the boss
    // and just eat the debuff.
    // triggers.push_back(new TriggerNode("ingvar dreadful roar",
    //          { NextAction("ingvar hide los", ACTION_RAID + 1) }));
    triggers.push_back(new TriggerNode("ingvar smash tank",
            { NextAction("ingvar dodge smash", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("ingvar dark smash non tank",
            { NextAction("ingvar evade dark smash", ACTION_MOVE + 5) }));
    triggers.push_back(new TriggerNode("ingvar contact clearance",
            // Healer's generic close-range flee is ACTION_MOVE + 9.  Once a
            // non-tank is inside the safety ring, use the validated rear
            // point instead of allowing that generic movement to win.
            { NextAction("ingvar clear contact", ACTION_MOVE + 10) }));
    triggers.push_back(new TriggerNode("not behind ingvar",
            { NextAction("ingvar get behind", ACTION_MOVE + 1) }));
    triggers.push_back(new TriggerNode("ingvar shadow axe",
            { NextAction("ingvar avoid shadow axe", ACTION_MOVE + 6) }));
    // Standing formation for members that do not need melee contact: leave the 10 yd
    // radius of the smash cone entirely, instead of relying on the momentary rear arc.
    triggers.push_back(new TriggerNode("ingvar ranged clearance",
            { NextAction("ingvar keep range", ACTION_MOVE + 3) }));
    // Standing formation: below both smash responses and the axe evade, above the
    // ordinary rear-arc move. A single axe covers 5 yd, so members that do not need
    // melee contact hold more than that from each other before one lands.
    triggers.push_back(new TriggerNode("ingvar spread",
            { NextAction("ingvar spread", ACTION_MOVE + 2) }));
    // 视线恢复要排在散开(+2)/保持距离(+3)之上——正是它们把后排挪进柱子影里的；
    // 又要排在躲猛击(+5)/躲斧(+6)之下：那两条是即时致命伤害，看不见也得先躲。
    triggers.push_back(new TriggerNode("ingvar los lost",
            { NextAction("ingvar regain los", ACTION_MOVE + 4) }));
    // 补位治疗：治疗阵亡或没蓝时由还有蓝的非治疗、非坦克成员顶上。19 场实测这个窗口
    // 占 9.9 秒/场、出现在 7/19 场——团灭时治疗死在 77–104 秒而击杀耗时 118–122 秒，
    // 整段没有任何治疗。相关性压在所有躲技能之下：补一发治疗不值得站在斧上。
    triggers.push_back(new TriggerNode("party needs offheal",
            { NextAction("offheal", ACTION_MOVE + 1) }));

}

void WotlkDungeonUKStrategy::InitMultipliers(std::vector<Multiplier*> &multipliers)
{
    multipliers.push_back(new PrinceKelesethMultiplier(botAI));
    multipliers.push_back(new SkarvaldAndDalronnMultiplier(botAI));
    multipliers.push_back(new IngvarThePlundererMultiplier(botAI));
}

/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "NexStrategy.h"
#include "NexMultipliers.h"
#include "NexTriggers.h"
#include "PlayerbotAI.h"

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

    // 前置守卫组的清怪控制链（docs/testing/TRASH-CC-PULL-DESIGN.md）。
    // 副本策略同时挂在 combat / non-combat 两个引擎上，所以这四条既管开怪前的
    // 指派与首次上控，也管战斗中的挪骷髅与重新上控。
    // 标记是瞬时的、放最高；上控放在移动之上、打断与紧急之下——开怪前它是这一刻唯一
    // 要做的事，战斗中则不能压过奥莫洛克躲尖刺(ACTION_MOVE + 5)这类保命走位。
    triggers.push_back(new TriggerNode("trash cc mark",
        { NextAction("trash cc mark", ACTION_RAID) }));
    triggers.push_back(new TriggerNode("trash cc polymorph",
        { NextAction("trash cc polymorph", ACTION_MOVE + 3) }));
    triggers.push_back(new TriggerNode("trash cc hex",
        { NextAction("trash cc hex", ACTION_MOVE + 3) }));
    triggers.push_back(new TriggerNode("trash cc sap",
        { NextAction("trash cc sap", ACTION_MOVE + 3) }));

    // Ormorok the Tree-Shaper
    // Tank trigger to stack inside boss. Can also add return action to prevent boss repositioning
    // if it becomes too much of a problem. He usually dies before he's up against a wall though
    triggers.push_back(new TriggerNode("ormorok spikes",
        { NextAction("dodge spikes", ACTION_MOVE + 5) }));
    // Non-tank trigger to stack. Avoiding the spikes at range is.. harder than it seems.
    // TODO: This turns hunters into melee marshmallows, have not come up with a better solution yet
    triggers.push_back(new TriggerNode("ormorok stack",
        { NextAction("dodge spikes", ACTION_MOVE + 5) }));
    // TODO: Add handling for spell reflect... best to spam low level/weak spells but don't want
    // to hardcode spells per class, might be difficult to dynamically generate this.
    // Will revisit if I find my altbots killing themselves in heroic, just heal through it for now

    // Keristrasza
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

// 被控制图标钉住的怪对 DPS 选目标不可见：上游的 smart 选目标只硬编码跳过月亮，不认 "rti cc"，
// 这里把三个控制图标都排除掉——DPS 一律留在骷髅上，被控的怪掉了控制也由控制职业补，不由 DPS 打。
// 坦克只排除**此刻真的被控着**的：控制掉了、或者压根没放出来（run393/attempt2 的闷棍目标一直
// 在打治疗，坦克却因为它有图标而不去抓），松掉的怪就是坦克的活。
// 坦克把骷髅挪到某只被控的怪上时，核心会顺手清掉它的控制图标，于是它自然回到可选范围——
// 这就是「被控的最后杀」。
void WotlkDungeonNexStrategy::AppendTargetExclusions(GuidSet& exclusions, TargetValueExclusionType type)
{
    // 开怪前骷髅也要排除：上游 AttackersValue 会把骷髅目标塞进 "attackers"，于是坦克一打标记，
    // 治疗就用魔杖射了骷髅（run402 attempt1/2：5019 Shoot 在 7.5 秒/2.9 秒把整组拉起来，
    // 控制一个都没落地）。开怪指令由编排层下达，之前谁都不该碰它。
    Player* bot = botAI->GetBot();
    if (!bot->IsInCombat() && TrashCcPullInProgress(botAI))
        if (Unit* skull = TrashCcIconUnit(botAI, TRASH_CC_ICON_SKULL))
            exclusions.insert(skull->GetGUID());

    for (uint8 icon : { TRASH_CC_ICON_MOON, TRASH_CC_ICON_SQUARE, TRASH_CC_ICON_CROSS })
    {
        Unit* unit = TrashCcIconUnit(botAI, icon);
        if (!unit)
            continue;

        if (type == TargetValueExclusionType::Tank && !TrashCcIncapacitated(unit, bot))
            continue;

        exclusions.insert(unit->GetGUID());
    }

    // 「放出来打」的那只（骷髅挪到它身上、控制还在）：近战与坦克**不追**，由远程先打破控制，
    // 怪自己会跑到队伍这边来，坦克在原地接。追过去等于把全队带进它乱走到的地方——run407 a1 萨满
    // 就是追被羊的最后一只时进了泰蕾斯特拉 22 码仇恨半径死的；链式场景里这会直接把 boss 拉进来。
    bool const melee = type == TargetValueExclusionType::Tank || !PlayerbotAI::IsRanged(bot);
    if (melee)
        if (Unit* skull = TrashCcIconUnit(botAI, TRASH_CC_ICON_SKULL))
            if (TrashCcIncapacitated(skull, bot))
                exclusions.insert(skull->GetGUID());
}

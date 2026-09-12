/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RTITARGETVALUE_H
#define PLAYERBOTS_RTITARGETVALUE_H

#include "TargetValue.h"

#include <initializer_list>
#include <vector>

class Creature;
class Player;

class PlayerbotAI;
class Unit;

class RtiTargetValue : public TargetValue
{
public:
    RtiTargetValue(PlayerbotAI* botAI, std::string const type = "rti", std::string const name = "rti target")
        : TargetValue(botAI, name), type(type)
    {
    }

    static int32 GetRtiIndex(std::string const rti);
    Unit* Calculate() override;
    static const int8 starIndex = 0;
    static const int8 circleIndex = 1;
    static const int8 diamondIndex = 2;
    static const int8 triangleIndex = 3;
    static const int8 moonIndex = 4;
    static const int8 squareIndex = 5;
    static const int8 crossIndex = 6;
    static const int8 skullIndex = 7;

private:
    std::string const type;
};

class RtiCcTargetValue : public RtiTargetValue
{
public:
    RtiCcTargetValue(PlayerbotAI* botAI, std::string const name = "rti cc target")
        : RtiTargetValue(botAI, "rti cc", name)
    {
    }
};

// ---- 清怪控制链（docs/testing/TRASH-CC-PULL-DESIGN.md）----
// 真人打法的开怪流程：坦克脱战时指派 → 控制落在满血未进战斗的怪身上 → 坦克再开怪 →
// 只要还有控制在就全队不放 AoE → 单体按序击杀 → 被控的最后杀、掉了重新上。
// 图标约定与上游 RtiTargetValue::GetRtiIndex 的编号一致；月亮(4) 同时是上游各处硬编码的 CC 图标：
//   骷髅 = 先杀（坦克抓）、月亮 = 法师变形术、方块 = 萨满妖术、十字 = 盗贼闷棍。
// 编排层（mod-raidtest）只做两件事：把本次拉怪目标钉在坦克的 "pull target" 上作为
// 「准备开这组」的信号，以及等控制落地后再对骷髅下达开怪。指派、上控、不放 AoE、
// 换目标全部是 bot 自己的决策。
enum TrashCcIcon : uint8
{
    TRASH_CC_ICON_TRIANGLE = 3,   // 牧师束缚亡灵：亡灵本（艾卓-尼鲁布…）里唯一能落地的控制
    TRASH_CC_ICON_MOON   = 4,
    TRASH_CC_ICON_SQUARE = 5,
    TRASH_CC_ICON_CROSS  = 6,
    TRASH_CC_ICON_SKULL  = 7,
};

struct TrashCcRole
{
    uint8 icon;
    char const* iconName;   // 写进各控制 bot 的 "rti cc"，让上游 IsCcTarget 等判据同步
    char const* spell;      // 按名字在运行时查（"polymorph" / "hex" / "sap"），没学会就不参与
    uint8 casterClass;
    // 该法术的 Spell.dbc TargetCreatureType 掩码（位 = 1 << (CreatureType - 1)），0 = 不限。
    // 必须按法术数据填，不能凭印象：实测 妖术(51514)=65 只对野兽+人形，**对元素和亡灵都无效**，
    // 而这里原先写的是「妖术没有生物类型限制」。艾卓-尼鲁布整本小怪都是亡灵(type 6, 掩码位 32)，
    // 变形/妖术/闷棍一个都落不下去，只有束缚亡灵(32) 可以。
    uint32 targetTypeMask;
};

// 四个控制职业的固定分工（法师→月亮/变形术，萨满→方块/妖术，盗贼→十字/闷棍，
// 牧师→三角/束缚亡灵）。没有则 nullptr。
TrashCcRole const* TrashCcRoleForClass(uint8 playerClass);
// 按图标反查分工；不是控制图标则 nullptr。
TrashCcRole const* TrashCcRoleForIcon(uint8 icon);
// 该单位身上是否有「让它脱离战斗」的友方控制光环：变形/妖术/致盲/恐惧/闷棍。
// 定身与减速不算（被定住的怪照样在打人）；普通昏迷（制裁之锤）也不算，只认 MECHANIC_SAPPED。
bool TrashCcIncapacitated(Unit* unit, Player* bot);
// 队伍图标所指、且活着的敌对单位；不满足返回 nullptr。
Unit* TrashCcIconUnit(PlayerbotAI* botAI, uint8 icon);
// 队伍里是否还有任何控制图标钉在活着的怪身上（= 一次控制链拉怪正在进行）。
bool TrashCcPullInProgress(PlayerbotAI* botAI);
// 某个控制职业此刻该对谁上控（开怪前首次上控与战斗中重新上控共用）。
// trigger 与 action 共用同一判据，避免「触发了但选不出目标」这类静默失败。
Unit* TrashCcCastTarget(PlayerbotAI* botAI, Player* bot, TrashCcRole const& role);
// 坦克此刻需不需要（重新）打标记：开怪前有信号且分工没配齐；战斗中骷髅目标已死。
bool TrashCcMarkNeeded(PlayerbotAI* botAI, Player* bot);
// 该 bot 第一次看到这个图标当前目标以来过了多少毫秒（没有目标返回 0）。
uint32 TrashCcIconAgeMs(PlayerbotAI* botAI, uint8 icon);
// 拉怪目标周围成组的怪（含它本身），按 GUID 排序；不足 3 只返回空。
std::vector<Creature*> TrashCcCollectPack(PlayerbotAI* botAI, Player* bot, Unit* pull);
// 队里在场且学会该控制的成员；没有则 nullptr。
Player* TrashCcFindCaster(Player* bot, TrashCcRole const& role);
// 这个控制能不能真的落在这只怪身上：**生物类型**（按法术自己的 TargetCreatureType 掩码判）
// 与**机制免疫**（creature_immunities 的 MechanicsMask）两关都要过。caster 用来解析法术 id
// 并做 IsImmunedToSpell 的施法者判定。
bool TrashCcSpellFits(Player* caster, TrashCcRole const& role, Creature* creature);
// 控制候选的优先级：治疗(0) > 有法力的施法者(1) > 其它(2)。数值越小越先被控，剩下的留给坦克。
int TrashCcPreference(Creature* creature);
// 各副本在自己的策略构造时登记治疗小怪的 entry（普通/英雄 entry 都可）。
void TrashCcRegisterHealerEntries(std::initializer_list<uint32> entries);

#endif

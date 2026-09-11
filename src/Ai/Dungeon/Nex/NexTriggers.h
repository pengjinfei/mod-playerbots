/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEXTRIGGERS_H
#define PLAYERBOTS_NEXTRIGGERS_H

#include "DungeonStrategyUtils.h"
#include "GenericTriggers.h"
#include "PlayerbotAIConfig.h"
#include "Trigger.h"

enum NexusIDs
{
    // Faction Commander
    NPC_ALLIANCE_COMMANDER          = 27949,
    NPC_HORDE_COMMANDER             = 27947,
    NPC_COMMANDER_STOUTBEARD        = 26796,
    NPC_COMMANDER_KOLURG            = 26798,
    // SPELL_FRIGHTENING_SHOUT         = 19134,
    SPELL_WHIRLWIND                 = 38618,

    // Grand Magus Telestra
    NPC_TELESTRA                    = 26731,
    NPC_FIRE_MAGUS                  = 26928,
    NPC_FROST_MAGUS                 = 26930,
    NPC_ARCANE_MAGUS                = 26929,

    // Anomalus
    BUFF_RIFT_SHIELD                = 47748,
    NPC_CHAOTIC_RIFT                = 26918,
    NPC_CHAOTIC_RIFT_HEROIC         = 30522,

    // Ormorok the Tree Shaper
    // NPC_CRYSTAL_SPIKE               = 27099,
    GO_CRYSTAL_SPIKE                = 188537,

    // 前置守卫组里的治疗小怪（普通/英雄两个 entry）。这两组 4 只等级 80 精英是
    // 泰蕾斯特拉与奥莫洛克完整链路的实测瓶颈，见 docs/testing/bosses/heroic-nexus/TRASH-TACTICS.md。
    NPC_CRYSTALLINE_TENDER          = 28231,  // 引导群疗：宁静(57054)，打不断（引导标志位不满足）
    NPC_CRYSTALLINE_TENDER_HEROIC   = 30525,
    NPC_MAGE_HUNTER_INITIATE        = 26728,  // 瞬发单奶：恢复(25058)，定义上无法打断
    NPC_MAGE_HUNTER_INITIATE_HEROIC = 30478,
};

// Anomalus 的混乱空间裂隙不在仇恨表里，AI_VALUE2("find target") 找不到它，
// 因此按 entry 扫描 "possible targets no los" 取最近的一只存活裂隙。
// trigger 与 action 共用同一判据，避免「触发了但选不出目标」这类静默失败。
Unit* FindNearestChaoticRift(PlayerbotAI* botAI, Player* bot, AiObjectContext* context);

// 挑一只值得控住的治疗小怪。只在魔枢的前置守卫组里用：这两个治疗的法术
// **本质上打不断**（宁静的引导标志位不满足核心 EffectInterruptCast 的要求；恢复是瞬发），
// 而「优先击杀治疗」实测更差（把 DPS 锁到坦克没抓的怪身上，丢掉坦克保护）。
// trigger 与 action 共用同一判据，避免「触发了但选不出目标」这类静默失败。
//
// spell：按名字在运行时查（"hex" / "polymorph"），学不会就不触发。
// casterClass：只让该职业出手，避免把判据散进各职业文件。
// farthest：萨满取最近、法师取最远。守卫组只有两只治疗，这样两个控制不会撞同一只；
//   顺带也符合真人习惯（远程羊离近战最远的那只）。
Unit* FindCcableTrashHealer(PlayerbotAI* botAI, Player* bot, AiObjectContext* context,
                            std::string const& spell, uint8 casterClass, bool farthest);

class FactionCommanderWhirlwindTrigger : public Trigger
{
public:
    FactionCommanderWhirlwindTrigger(PlayerbotAI* ai) : Trigger(ai, "faction commander whirlwind") {}
    bool IsActive() override;
};

class TelestraFirebombTrigger : public Trigger
{
public:
    TelestraFirebombTrigger(PlayerbotAI* ai) : Trigger(ai, "telestra firebomb spread") {}
    bool IsActive() override;
};

class TelestraSplitPhaseTrigger : public Trigger
{
public:
    TelestraSplitPhaseTrigger(PlayerbotAI* ai) : Trigger(ai, "telestra split phase") {}
    bool IsActive() override;
};

class ChaoticRiftTrigger : public Trigger
{
public:
    ChaoticRiftTrigger(PlayerbotAI* ai) : Trigger(ai, "chaotic rift") {}
    bool IsActive() override;
};

class TrashHealerHexTrigger : public Trigger
{
public:
    TrashHealerHexTrigger(PlayerbotAI* ai) : Trigger(ai, "trash healer hex") {}
    bool IsActive() override;
};

class TrashHealerPolymorphTrigger : public Trigger
{
public:
    TrashHealerPolymorphTrigger(PlayerbotAI* ai) : Trigger(ai, "trash healer polymorph") {}
    bool IsActive() override;
};

class OrmorokSpikesTrigger : public Trigger
{
public:
    OrmorokSpikesTrigger(PlayerbotAI* ai) : Trigger(ai, "ormorok spikes") {}
    bool IsActive() override;
};

class OrmorokStackTrigger : public Trigger
{
public:
    OrmorokStackTrigger(PlayerbotAI* ai) : Trigger(ai, "ormorok stack") {}
    bool IsActive() override;
};

class IntenseColdTrigger : public Trigger
{
public:
    IntenseColdTrigger(PlayerbotAI* ai) : Trigger(ai, "intense cold") {}
    bool IsActive() override;
};

class KeristraszaPositioningTrigger : public Trigger
{
public:
    KeristraszaPositioningTrigger(PlayerbotAI* ai) : Trigger(ai, "keristrasza positioning") {}
    bool IsActive() override;
};

#endif

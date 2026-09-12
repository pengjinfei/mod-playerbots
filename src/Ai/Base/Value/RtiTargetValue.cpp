/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "RtiTargetValue.h"
#include "AttackersValue.h"
#include "Creature.h"
#include "Group.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "Timer.h"

#include <algorithm>
#include <map>
#include <set>

int32 RtiTargetValue::GetRtiIndex(std::string const rti)
{
    int32 index = -1;
    if (rti == "star")
        index = 0;
    else if (rti == "circle")
        index = 1;
    else if (rti == "diamond")
        index = 2;
    else if (rti == "triangle")
        index = 3;
    else if (rti == "moon")
        index = 4;
    else if (rti == "square")
        index = 5;
    else if (rti == "cross")
        index = 6;
    else if (rti == "skull")
        index = 7;

    return index;
}

Unit* RtiTargetValue::Calculate()
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    std::string const rti = AI_VALUE(std::string, type);
    int32 index = GetRtiIndex(rti);

    if (index == -1)
        return nullptr;

    ObjectGuid guid = group->GetTargetIcon(index);
    if (!guid)
        return nullptr;

    //////////////////////////////////////////////////////begin: delete below check
    // Some units that need to be killed in battle are not on the list of attackers,
    // such as the Kor'kron Battle-Mage in Icecrown Citadel.

    // GuidVector attackers = context->GetValue<GuidVector >("attackers")->Get();
    // if (find(attackers.begin(), attackers.end(), guid) == attackers.end())
    //     return nullptr;
    //
    //////////////////////////////////////////////////////end: delete below check

    Unit* unit = botAI->GetUnit(guid);
    if (!unit || unit->isDead() || !bot->IsWithinLOSInMap(unit) || !AttackersValue::IsValidTarget(unit, bot) ||
        ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, unit),
                                             sPlayerbotAIConfig.sightDistance))
        return nullptr;

    // Also prevent chasing raid icon targets that are too far away from the master,
    // even if they are technically visible to the bot.
    if (Player* master = botAI->GetMaster())
    {
        if (master->IsInWorld() && master->GetMapId() == unit->GetMapId() &&
            ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(master, unit),
                sPlayerbotAIConfig.sightDistance))
            return nullptr;
    }

    return unit;
}

// ---- 清怪控制链的共享判据（docs/testing/TRASH-CC-PULL-DESIGN.md）。原在 Ai/Dungeon/Nex，2026-09-12 归位。----

namespace
{
    TrashCcRole const kTrashCcRoles[] =
    {
        // 顺序即控制质量：变形术可反复上，妖术 45 秒冷却，闷棍只能在脱战时用一次。
        // 指派时按这个顺序把最值得控的怪（治疗）分给最可靠的控制。
        { TRASH_CC_ICON_MOON,   "moon",   "polymorph", CLASS_MAGE },
        { TRASH_CC_ICON_SQUARE, "square", "hex",       CLASS_SHAMAN },
        { TRASH_CC_ICON_CROSS,  "cross",  "sap",       CLASS_ROGUE },
    };

    // 同一组怪的判定半径：拉怪目标周围这个距离内的敌人算一组。
    constexpr float kTrashPackRadius = 15.0f;
    // 只对成组的怪启用控制链；落单/两只直接 A 掉更快。
    constexpr size_t kTrashPackMinSize = 3;
    // 控制职业离坦克多远以内算「在场」。
    constexpr float kCasterPresenceRange = 60.0f;
    // 羊/妖术最多等闷棍先落地多久（盗贼潜行绕到目标背后约 30 码，6–9 秒）。
    constexpr uint32 kSapLeadMs = 12000;

    bool IsCcablePackMember(Unit* unit, Player* bot)
    {
        Creature* creature = unit ? unit->ToCreature() : nullptr;
        if (!creature || !creature->IsAlive() || creature->IsDungeonBoss() || creature->isWorldBoss())
            return false;

        if (creature->IsPet() || creature->IsTotem() || creature->IsSummon())
            return false;

        return AttackersValue::IsPossibleTarget(creature, bot);
    }
}

namespace
{
    // 各副本登记的「治疗小怪」entry：优先分给控制。
    std::set<uint32>& TrashCcHealerEntries()
    {
        static std::set<uint32> entries;
        return entries;
    }
}

void TrashCcRegisterHealerEntries(std::initializer_list<uint32> entries)
{
    TrashCcHealerEntries().insert(entries.begin(), entries.end());
}

int TrashCcPreference(Creature* creature)
{
    if (TrashCcHealerEntries().count(creature->GetEntry()) || TrashCcHealerEntries().count(creature->GetOriginalEntry()))
        return 0;

    return creature->getPowerType() == POWER_MANA ? 1 : 2;
}

TrashCcRole const* TrashCcRoleForClass(uint8 playerClass)
{
    for (TrashCcRole const& role : kTrashCcRoles)
        if (role.casterClass == playerClass)
            return &role;

    return nullptr;
}

bool TrashCcSpellFits(TrashCcRole const& role, Creature* creature)
{
    uint32 const type = creature->GetCreatureType();
    switch (role.icon)
    {
        case TRASH_CC_ICON_MOON:    // 变形术
            return type == CREATURE_TYPE_HUMANOID || type == CREATURE_TYPE_BEAST || type == CREATURE_TYPE_CRITTER;
        case TRASH_CC_ICON_CROSS:   // 闷棍
            return type == CREATURE_TYPE_HUMANOID || type == CREATURE_TYPE_BEAST || type == CREATURE_TYPE_DEMON ||
                   type == CREATURE_TYPE_DRAGONKIN;
        default:                    // 妖术没有生物类型限制
            return type != CREATURE_TYPE_MECHANICAL;
    }
}

bool TrashCcIncapacitated(Unit* unit, Player* bot)
{
    for (auto const& applied : unit->GetAppliedAuras())
    {
        AuraApplication const* application = applied.second;
        Aura* aura = application ? application->GetBase() : nullptr;
        SpellInfo const* auraInfo = aura ? aura->GetSpellInfo() : nullptr;
        if (!auraInfo)
            continue;

        // 只认友方施加的控制。怪自己身上的变形/潜行不是我们要护着的东西。
        Unit* caster = aura->GetCaster();
        if (!caster || !bot->IsFriendlyTo(caster))
            continue;

        for (uint8 effect = EFFECT_0; effect <= EFFECT_2; ++effect)
        {
            switch (auraInfo->Effects[effect].ApplyAuraName)
            {
                case SPELL_AURA_MOD_CONFUSE:          // 变形术、致盲
                case SPELL_AURA_MOD_FEAR:             // 恐惧类
                case SPELL_AURA_MOD_PACIFY_SILENCE:   // 妖术
                case SPELL_AURA_TRANSFORM:            // 变形术、妖术的变形部分
                    return true;
                case SPELL_AURA_MOD_STUN:
                    // 闷棍是昏迷，但制裁之锤/肾击也是昏迷——那些是输出手段，不是控制链的一部分。
                    if (auraInfo->Mechanic == MECHANIC_SAPPED || auraInfo->Effects[effect].Mechanic == MECHANIC_SAPPED)
                        return true;
                    break;
                default:
                    break;
            }
        }
    }

    return false;
}

Unit* TrashCcIconUnit(PlayerbotAI* botAI, uint8 icon)
{
    Player* bot = botAI->GetBot();
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    ObjectGuid const guid = group->GetTargetIcon(icon);
    if (!guid || !guid.IsCreatureOrVehicle())
        return nullptr;

    Unit* unit = botAI->GetUnit(guid);
    if (!unit || !unit->IsAlive() || unit->IsFriendlyTo(bot))
        return nullptr;

    return unit;
}

bool TrashCcPullInProgress(PlayerbotAI* botAI)
{
    for (TrashCcRole const& role : kTrashCcRoles)
        if (TrashCcIconUnit(botAI, role.icon))
            return true;

    return false;
}

Player* TrashCcFindCaster(Player* bot, TrashCcRole const& role)
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member->getClass() != role.casterClass ||
            member->GetMapId() != bot->GetMapId() || bot->GetDistance(member) > kCasterPresenceRange)
            continue;

        PlayerbotAI* memberAI = GET_PLAYERBOT_AI(member);
        if (!memberAI || !memberAI->HasSpell(role.spell))
            continue;

        return member;
    }

    return nullptr;
}

std::vector<Creature*> TrashCcCollectPack(PlayerbotAI* botAI, Player* bot, Unit* pull)
{
    std::vector<Creature*> pack;
    if (!IsCcablePackMember(pull, bot))
        return pack;

    pack.push_back(pull->ToCreature());

    GuidVector const candidates = botAI->GetAiObjectContext()->GetValue<GuidVector>("possible targets")->Get();
    for (ObjectGuid const& guid : candidates)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || unit == pull || !IsCcablePackMember(unit, bot) || unit->GetDistance(pull) > kTrashPackRadius)
            continue;

        pack.push_back(unit->ToCreature());
    }

    if (pack.size() < kTrashPackMinSize)
        pack.clear();

    std::sort(pack.begin(), pack.end(),
              [](Creature* a, Creature* b) { return a->GetGUID() < b->GetGUID(); });
    return pack;
}

Unit* TrashCcCastTarget(PlayerbotAI* botAI, Player* bot, TrashCcRole const& role)
{
    if (bot->getClass() != role.casterClass)
        return nullptr;

    // 名字解析失败（没学会/不在 spellmap）就别让触发器每 tick 空转。
    if (!botAI->GetAiObjectContext()->GetValue<uint32>("spell id", role.spell)->Get())
        return nullptr;

    Unit* unit = TrashCcIconUnit(botAI, role.icon);
    if (!unit || !AttackersValue::IsValidTarget(unit, bot))
        return nullptr;

    // 已经控住就不重复上（run390 实测这条判据让妖术每场只放 1 次、不重复）。
    if (TrashCcIncapacitated(unit, bot))
        return nullptr;

    if (bot->IsInCombat())
    {
        // 闷棍只能在脱战、目标未进战斗时用；战斗中掉了就交给坦克。
        if (role.icon == TRASH_CC_ICON_CROSS)
            return nullptr;

        // 它已经是当前击杀目标：坦克把骷髅挪过来时控制图标会被顶掉，这里是兜底。
        AiObjectContext* context = botAI->GetAiObjectContext();
        if (context->GetValue<Unit*>("current target")->Get() == unit)
            return nullptr;

        if (Unit* tank = context->GetValue<Unit*>("main tank")->Get())
            if (tank->GetVictim() == unit)
                return nullptr;

        // 它是最后一只活着的怪：别再控，直接打。
        bool othersAlive = false;
        for (ObjectGuid const& guid : context->GetValue<GuidVector>("attackers")->Get())
        {
            Unit* other = botAI->GetUnit(guid);
            if (other && other != unit && other->IsAlive())
            {
                othersAlive = true;
                break;
            }
        }
        if (!othersAlive)
            return nullptr;
    }
    else if (role.icon == TRASH_CC_ICON_CROSS)
    {
        if (unit->IsInCombat())
            return nullptr;
    }
    else
    {
        // 顺序是真人打法的顺序：**闷棍先**（不进战斗），羊/妖术后。羊一落地这组就会进战斗
        // （run403：变形术命中的同一秒 pack_engaged，其它怪冲上来），之后闷棍就不可能了。
        // 盗贼要潜行走过去，给它最多 kSapLeadMs；超时或它已进战斗就不再等。
        if (Unit* sapTarget = TrashCcIconUnit(botAI, TRASH_CC_ICON_CROSS))
            if (!TrashCcIncapacitated(sapTarget, bot) && !sapTarget->IsInCombat() &&
                TrashCcIconAgeMs(botAI, TRASH_CC_ICON_CROSS) < kSapLeadMs)
                return nullptr;
    }

    return unit;
}

uint32 TrashCcIconAgeMs(PlayerbotAI* botAI, uint8 icon)
{
    // 每个 bot 各记一份「这个图标当前指向的目标是什么时候第一次看到的」。
    // 同一 spawn 的怪在每个实例里 GUID 相同，所以按 (bot, icon) 记目标 + 首见时刻，目标一换就重置。
    // 同一 spawn 在每个实例里 GUID 相同（run405：上一场的十字目标 GUID 与这一场一样，年龄直接
    // 算成了上一场的），所以还要连实例 id 一起记。
    struct Seen { ObjectGuid target; uint32 instance; uint32 firstSeen; };
    static std::map<std::pair<ObjectGuid, uint8>, Seen> seen;

    Player* bot = botAI->GetBot();
    Group* group = bot->GetGroup();
    ObjectGuid const target = group ? group->GetTargetIcon(icon) : ObjectGuid::Empty;
    uint32 const instance = bot->GetInstanceId();
    uint32 const now = getMSTime();

    Seen& entry = seen[{bot->GetGUID(), icon}];
    if (entry.target != target || entry.instance != instance || getMSTimeDiff(entry.firstSeen, now) > 120000)
    {
        entry.target = target;
        entry.instance = instance;
        entry.firstSeen = now;
    }

    return target ? getMSTimeDiff(entry.firstSeen, now) : 0;
}

bool TrashCcMarkNeeded(PlayerbotAI* botAI, Player* bot)
{
    if (!PlayerbotAI::IsTank(bot) || !bot->GetGroup())
        return false;

    if (bot->IsInCombat())
    {
        // 战斗中：控制链拉怪进行中、骷髅目标已死/失效 → 该挪骷髅了。
        return TrashCcPullInProgress(botAI) && !TrashCcIconUnit(botAI, TRASH_CC_ICON_SKULL);
    }

    // 开怪前：编排层把本次拉怪目标钉在 "pull target" 上作为「准备开这组」的信号。
    // 没有信号就不打标记——boss 房间准备点周围也可能看得见一组小怪，别乱标。
    Unit* pull = botAI->GetUnit(botAI->GetAiObjectContext()->GetValue<ObjectGuid>("pull target")->Get());
    if (!pull)
        return false;

    std::vector<Creature*> const pack = TrashCcCollectPack(botAI, bot, pull);
    if (pack.empty())
        return false;

    auto inPack = [&](Unit* unit) { return unit && std::find(pack.begin(), pack.end(), unit->ToCreature()) != pack.end(); };

    if (!inPack(TrashCcIconUnit(botAI, TRASH_CC_ICON_SKULL)))
        return true;

    // 最多控 pack.size()-1 只，至少留一只给坦克；控制职业比可控的怪多时不算「没配齐」。
    size_t assigned = 0;
    size_t casters = 0;
    for (TrashCcRole const& role : kTrashCcRoles)
    {
        if (!TrashCcFindCaster(bot, role))
            continue;

        ++casters;
        if (inPack(TrashCcIconUnit(botAI, role.icon)))
            ++assigned;
    }

    return assigned < std::min(casters, pack.size() - 1);
}

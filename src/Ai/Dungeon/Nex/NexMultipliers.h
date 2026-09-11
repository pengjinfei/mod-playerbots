/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NEXMULTIPLIERS_H
#define PLAYERBOTS_NEXMULTIPLIERS_H

#include "Multiplier.h"

class FactionCommanderMultiplier : public Multiplier
{
    public:
        FactionCommanderMultiplier(PlayerbotAI* ai) : Multiplier(ai, "faction commander") {}

    public:
        float GetValue(Action* action) override;
};

class TelestraMultiplier : public Multiplier
{
    public:
        TelestraMultiplier(PlayerbotAI* ai) : Multiplier(ai, "grand magus telestra") {}

    public:
        float GetValue(Action* action) override;
};

class AnomalusMultiplier : public Multiplier
{
    public:
        AnomalusMultiplier(PlayerbotAI* ai) : Multiplier(ai, "anomalus") {}

    public:
        float GetValue(Action* action) override;
};

// 魔枢到处是平台与深坑：雷霆风暴的击退会把怪打下平台（run402/403/412 实测 z 掉到 -22 / -47），
// 怪再绕路爬回来时穿过 boss 房间、把 boss 一起带回，队伍也会追着掉下去。这个副本里干脆不放。
class NexusNoKnockbackMultiplier : public Multiplier
{
    public:
        NexusNoKnockbackMultiplier(PlayerbotAI* ai) : Multiplier(ai, "nexus no knockback") {}

    public:
        float GetValue(Action* action) override;
};

class OrmorokMultiplier : public Multiplier
{
    public:
        OrmorokMultiplier(PlayerbotAI* ai) : Multiplier(ai, "ormorok the tree-shaper") {}

    public:
        float GetValue(Action* action) override;
};

#endif

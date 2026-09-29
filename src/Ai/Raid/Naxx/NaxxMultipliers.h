/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_NAXXMULTIPLIERS_H
#define PLAYERBOTS_NAXXMULTIPLIERS_H

#include "Multiplier.h"
#include "NaxxBossHelper.h"

// Patchwerk's Hateful Strike (10-man) hits the highest-threat player in melee range other than his victim. Until an
// off-tank stands there to take it, a non-tank melee that reaches him first is that player and dies to one strike.
class PatchwerkMeleeWaitMultiplier : public Multiplier
{
public:
    PatchwerkMeleeWaitMultiplier(PlayerbotAI* ai) : Multiplier(ai, "patchwerk melee wait") {}

    float GetValue(Action* action) override;
};

class GrobbulusMultiplier : public Multiplier
{
public:
    GrobbulusMultiplier(PlayerbotAI* ai) : Multiplier(ai, "grobbulus") {}

public:
    float GetValue(Action* action) override;
};

class HeiganDanceMultiplier : public Multiplier
{
public:
    explicit HeiganDanceMultiplier(PlayerbotAI* ai) : Multiplier(ai, "heigan dance"), helper(ai) {}

public:
    float GetValue(Action* action) override;

private:
    HeiganBossHelper helper;
};

class LoathebGenericMultiplier : public Multiplier
{
public:
    LoathebGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "loatheb generic") {}

public:
    float GetValue(Action* action) override;
};

class ThaddiusGenericMultiplier : public Multiplier
{
public:
    ThaddiusGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "thaddius generic"), helper(ai) {}

public:
    float GetValue(Action* action) override;

private:
    ThaddiusBossHelper helper;
};

class SapphironGenericMultiplier : public Multiplier
{
public:
    SapphironGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "sapphiron generic"), helper(ai) {}

    float GetValue(Action* action) override;

private:
    SapphironBossHelper helper;
};

class InstructorRazuviousGenericMultiplier : public Multiplier
{
public:
    InstructorRazuviousGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "instructor razuvious generic"), helper(ai) {}
    float GetValue(Action* action) override;

private:
    RazuviousBossHelper helper;
};

class KelthuzadGenericMultiplier : public Multiplier
{
public:
    KelthuzadGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kelthuzad generic"), helper(ai) {}
    float GetValue(Action* action) override;

private:
    KelthuzadBossHelper helper;
};

class MaexxnaGenericMultiplier : public Multiplier
{
public:
    MaexxnaGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "maexxna generic") {}

public:
    float GetValue(Action* action) override;
};

class AnubrekhanGenericMultiplier : public Multiplier
{
public:
    AnubrekhanGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "anubrekhan generic") {}

public:
    float GetValue(Action* action) override;
};

class FourHorsemenGenericMultiplier : public Multiplier
{
public:
    FourHorsemenGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "four horsemen generic") {}

public:
    float GetValue(Action* action) override;
};

// class GothikGenericMultiplier : public Multiplier
// {
// public:
//     GothikGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "gothik generic") {}

// public:
//     float GetValue(Action* action) override;
// };

class GluthGenericMultiplier : public Multiplier
{
public:
    GluthGenericMultiplier(PlayerbotAI* ai) : Multiplier(ai, "gluth generic"), helper(ai) {}
    float GetValue(Action* action) override;

private:
    GluthBossHelper helper;
};

#endif

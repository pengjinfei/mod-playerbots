/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_ANMULTIPLIERS_H
#define PLAYERBOTS_ANMULTIPLIERS_H

#include "Multiplier.h"

class KrikthirMultiplier : public Multiplier
{
    public:
        KrikthirMultiplier(PlayerbotAI* ai) : Multiplier(ai, "krik'thir the gatewatcher") {}

    public:
        float GetValue(Action* action) override;
};

// Anub'arak: a fire mage at ilvl 187 (~13k mana) is out of mana by 90-120 s because it spreads Living Bomb
// over every add (19 casts / 255 s, ~720 each) and channels Blizzard (~2,400 each) / Flamestrike (~980)
// on the 7.5x-damage adds. The adds are tank-held and low on health; single-target casts are cheaper per
// kill. Zero those three actions while Anub'arak is the encounter (run 461 mana curves, 2026-09-13).
class AnubarakMageManaMultiplier : public Multiplier
{
    public:
        AnubarakMageManaMultiplier(PlayerbotAI* ai) : Multiplier(ai, "anub'arak mage mana") {}

    public:
        float GetValue(Action* action) override;
};

// 哈多诺克斯粉碎者阶段：三包（3 粉碎者 + 6 随从 + 门怪）几乎同时到，火法前 60 秒群攻打出 36–49 万，
// 把粉碎者仇恨拉走后 56–59 秒被砸死（10 场 4 次）；法师一死 boss 阶段输出不足，拖成 900 秒僵持。
// 附近有活着的粉碎者时归零群攻，单体不变；粉碎者死光后自动恢复。
class HadronoxCrusherMageMultiplier : public Multiplier
{
    public:
        HadronoxCrusherMageMultiplier(PlayerbotAI* ai) : Multiplier(ai, "hadronox crusher mage") {}

    public:
        float GetValue(Action* action) override;
};

// 把英勇/嗜血按在最后冲刺之前不许放：共享层的 BoostTrigger 不看阶段，39 场里 17 场落在第二次潜地附近，
// 那 40 秒 boss 不可选中，急速全打在小怪身上。归零后由 AnubarakHeroismTrigger 在第三次出土后放。
class AnubarakHeroismMultiplier : public Multiplier
{
    public:
        AnubarakHeroismMultiplier(PlayerbotAI* ai) : Multiplier(ai, "anub'arak heroism") {}

    public:
        float GetValue(Action* action) override;
};

#endif

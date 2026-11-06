/*
 * UltimateMoP: units of creature_classlevelstats base damage.
 *
 * damage_base .. damage_exp3 hold the client game tables NpcDamageByClass / Exp1-3 (levels 1-85), which are damage per
 * second; damage_exp4 holds per-hit values. With Creature.BaseDamagePerSecond on (default), a creature reading an
 * expansion 0-3 column multiplies its weapon damage by its attack time, as TrinityCore does. Off restores the old core
 * formula (weapon damage used per hit), for comparison only: the shipped table values assume it is on.
 */

#ifndef ULTIMATEMOP_CREATURE_DAMAGE_H
#define ULTIMATEMOP_CREATURE_DAMAGE_H

#include "Config.h"
#include "Define.h"

namespace CreatureDamage
{
    inline bool PerSecondColumns()
    {
        return sConfigMgr->GetBoolDefault("Creature.BaseDamagePerSecond", true, true);
    }

    // Factor that turns a base damage column value into damage per hit at this attack time.
    inline float PerHitFactor(uint32 column, uint32 attackTimeMs)
    {
        return column < 4 && PerSecondColumns() ? float(attackTimeMs) / 1000.0f : 1.0f;
    }
}

#endif

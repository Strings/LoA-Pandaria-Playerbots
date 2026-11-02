#ifndef ULTIMATEMOP_TELEMETRY_HOOKS_H
#define ULTIMATEMOP_TELEMETRY_HOOKS_H

#include "Define.h"

class Map;
class Player;
class Spell;
class WorldObject;

// Core call sites the telemetry module (T-004) needs that ScriptMgr has no hook for. Kept out of the
// widely included ScriptMgr.h; each handler is a plain pointer the module sets once at startup, so a
// call site costs one null check when telemetry is not loaded.
namespace TelemetryHooks
{
    struct Handlers
    {
        void (*CastStart)(Player* player, Spell const* spell) = nullptr;         // a player-initiated cast began
        void (*CastCancel)(Player* player, Spell const* spell) = nullptr;        // the player cancelled a cast or channel
        void (*Command)(Player* player, char const* text) = nullptr;            // a chat command ran
        void (*LootOpen)(Player* player, WorldObject const* source) = nullptr;
        void (*LootRoll)(Player* player, uint32 itemId, uint8 choice) = nullptr;
        void (*WorldState)(Map* map, uint32 field, uint32 value) = nullptr;     // battleground objective state
        void (*BossState)(Map* map, uint32 bossId, uint32 state) = nullptr;     // instance encounter state
    };

    // Register at script startup only.
    void Set(Handlers const& handlers);
    Handlers const& Get();

    inline void CastStart(Player* player, Spell const* spell) { if (Get().CastStart) Get().CastStart(player, spell); }
    inline void CastCancel(Player* player, Spell const* spell) { if (Get().CastCancel) Get().CastCancel(player, spell); }
    inline void Command(Player* player, char const* text) { if (Get().Command) Get().Command(player, text); }
    inline void LootOpen(Player* player, WorldObject const* source) { if (Get().LootOpen) Get().LootOpen(player, source); }
    inline void LootRoll(Player* player, uint32 itemId, uint8 choice) { if (Get().LootRoll) Get().LootRoll(player, itemId, choice); }
    inline void WorldState(Map* map, uint32 field, uint32 value) { if (Get().WorldState) Get().WorldState(map, field, value); }
    inline void BossState(Map* map, uint32 bossId, uint32 state) { if (Get().BossState) Get().BossState(map, bossId, state); }
}

#endif

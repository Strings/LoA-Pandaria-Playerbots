#ifndef ULTIMATEMOP_CREATURE_HOOKS_H
#define ULTIMATEMOP_CREATURE_HOOKS_H
#include <cstdint>
class Creature;
class Unit;
// Creature lifecycle and targeting hooks for custom systems (affixed rares first). A small header of its own so the
// hooks reach Creature.cpp and the playerbots module without touching Creature.h or ScriptMgr.h.
namespace CreatureHooks
{
    enum class Event
    {
        Spawned,    // added to the world (grid load or summon), after its AI exists
        Respawned,  // came back from a corpse in place
        Evaded,     // reset to home; every aura was just removed
        Removed     // leaving the world
    };
    using Callback = void (*)(Creature*, Event);
    // True when `creature` and `other` must not fight, in either direction. Runs on map threads: keep it cheap and
    // read only the two units.
    using FightFilter = bool (*)(Creature const* creature, Unit const* other);
    // The minimap vignette (Vignette.db2 id) a creature shows when its template names none, or 0. Runs on map
    // threads whenever a player starts or stops seeing a creature: keep it cheap and read only the creature.
    using VignetteResolver = uint32_t (*)(Creature const* creature);

    // Register at script startup only; callbacks live for the server lifetime.
    bool Register(Callback callback);
    bool RegisterFightFilter(FightFilter filter);
    bool RegisterVignetteResolver(VignetteResolver resolver);
    void Dispatch(Creature* creature, Event event);
    bool BlocksFight(Creature const* creature, Unit const* other);
    uint32_t VignetteFor(Creature const* creature);   // the first resolver's non-zero answer
}
#endif

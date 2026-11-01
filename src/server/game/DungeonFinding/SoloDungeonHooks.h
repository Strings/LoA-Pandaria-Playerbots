#ifndef ULTIMATEMOP_SOLO_DUNGEON_HOOKS_H
#define ULTIMATEMOP_SOLO_DUNGEON_HOOKS_H
#include "LFG.h"
class Player;
class Map;
namespace lfg
{
    struct LFGDungeonData;
    namespace SoloDungeon
    {
        // Adapter owns policy/state; core retains native behavior without a provider.
        struct Hooks
        {
            bool (*UnlockUpperLevel)(Player*, LFGDungeonData const&);
            bool (*Join)(Player*, LfgDungeonSet const&);
            bool (*Teleport)(Player*, bool);
            bool (*OriginalDifficulty)(Map const*);
        };
        void Install(Hooks const* hooks);
        bool UnlockUpperLevel(Player* player, LFGDungeonData const& dungeon);
        bool Join(Player* player, LfgDungeonSet const& dungeons);
        bool Teleport(Player* player, bool out);
        bool OriginalDifficulty(Map const* map);
    }
}
#endif

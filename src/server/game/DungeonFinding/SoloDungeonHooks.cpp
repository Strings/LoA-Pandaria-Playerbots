#include "SoloDungeonHooks.h"
#include <atomic>
namespace lfg::SoloDungeon
{
    namespace { std::atomic<Hooks const*> provider{ nullptr }; }
    void Install(Hooks const* hooks) { provider.store(hooks, std::memory_order_release); }
    bool UnlockUpperLevel(Player* player, LFGDungeonData const& dungeon, uint32 level)
    {
        auto hooks = provider.load(std::memory_order_acquire);
        return hooks && hooks->UnlockUpperLevel && hooks->UnlockUpperLevel(player, dungeon, level);
    }
    bool Join(Player* player, LfgDungeonSet const& dungeons)
    {
        auto hooks = provider.load(std::memory_order_acquire);
        return hooks && hooks->Join && hooks->Join(player, dungeons);
    }
    bool Teleport(Player* player, bool out)
    {
        auto hooks = provider.load(std::memory_order_acquire);
        return hooks && hooks->Teleport && hooks->Teleport(player, out);
    }
    bool OriginalDifficulty(Map const* map)
    {
        auto hooks = provider.load(std::memory_order_acquire);
        return hooks && hooks->OriginalDifficulty && hooks->OriginalDifficulty(map);
    }
}

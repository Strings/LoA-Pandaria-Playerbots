#ifndef ULTIMATEMOP_BATTLEGROUND_HOOKS_H
#define ULTIMATEMOP_BATTLEGROUND_HOOKS_H
class Player;
// Custom systems hear about each player's battleground result here instead of through a new ScriptMgr hook, so
// the widely included ScriptMgr.h stays untouched. Arenas are not dispatched.
namespace BattlegroundHooks
{
    using Callback = void (*)(Player* player, bool won);
    // Register at script startup only; callbacks live for the server lifetime.
    bool Register(Callback callback);
    void Dispatch(Player* player, bool won);
}
#endif

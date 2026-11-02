#ifndef ULTIMATEMOP_FIRST_LOGIN_HOOKS_H
#define ULTIMATEMOP_FIRST_LOGIN_HOOKS_H
class Player;
namespace FirstLoginHooks
{
    using Callback = void (*)(Player*);
    // Register at script startup only; callbacks live for the server lifetime.
    bool Register(Callback callback);
    void Dispatch(Player* player);
}
#endif

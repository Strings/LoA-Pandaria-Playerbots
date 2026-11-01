/*
 * Reagent bank (T-043): crafting reagents kept in detached storage instead of the bags. Spells draw from it after
 * the bags (ReagentSource), so every profession crafts straight from the bank. See README.md.
 */

#ifndef ULTIMATEMOP_REAGENT_BANK_H
#define ULTIMATEMOP_REAGENT_BANK_H

#include "Define.h"

#include <vector>

class Item;
class Player;

namespace ReagentBank
{
    enum class DepositResult
    {
        Ok,
        Partial,       // the bank filled up; the rest stayed in the bags
        Disabled,
        NotReagent,
        NotInBags,
        NotStorable,   // timed item
        Full,
        InCombat,
    };

    enum class WithdrawResult
    {
        Ok,
        Partial,       // the bags filled up
        Disabled,
        Empty,
        BagsFull,
        InCombat,
    };

    struct Holding
    {
        uint32 ItemEntry = 0;
        uint32 Count = 0;
        uint32 Stacks = 0;
    };

    bool Enabled();
    void Register();   // finds the reagents and hooks the bank into spell reagent checks

    // Items some spell uses as a reagent, minus quest and unique items (their counts must stay in the bags).
    bool IsReagent(uint32 itemEntry);

    DepositResult Deposit(Player* player, Item* item, uint32& deposited);
    // Every reagent in the backpack and the equipped bags. Returns how many items went in.
    uint32 DepositAll(Player* player, uint32& stacks, bool& full);
    WithdrawResult Withdraw(Player* player, uint32 itemEntry, uint32 count, uint32& withdrawn);

    uint32 Count(Player* player, uint32 itemEntry);
    // Uses items up (crafting). Returns how many it used.
    uint32 Use(Player* player, uint32 itemEntry, uint32 count);
    // GM: new items straight into the bank, ignoring the stack limit.
    uint32 Add(Player* player, uint32 itemEntry, uint32 count);

    std::vector<Holding> Contents(Player* player);
}

#endif

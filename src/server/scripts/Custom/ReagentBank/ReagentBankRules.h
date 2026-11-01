/*
 * Reagent bank (T-043): server-independent stack rules, kept apart so they can be unit tested.
 *
 * The bank holds ordinary stacks (never above an item's stack size), one per detached storage slot. Deposits top up
 * the stacks already there before opening new ones; crafting and withdrawals use the smallest stacks first, so the
 * bank settles into few, full stacks.
 */

#ifndef ULTIMATEMOP_REAGENT_BANK_RULES_H
#define ULTIMATEMOP_REAGENT_BANK_RULES_H

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ReagentBank
{
    struct Stack
    {
        uint32_t Slot = 0;
        uint32_t Count = 0;
    };

    struct Move
    {
        uint32_t Slot = 0;
        uint32_t Amount = 0;
        bool operator==(Move const& other) const { return Slot == other.Slot && Amount == other.Amount; }
    };

    inline uint32_t Total(std::vector<Stack> const& stacks)
    {
        uint32_t total = 0;
        for (Stack const& stack : stacks)
            total += stack.Count;
        return total;
    }

    // How much of an incoming stack goes onto each existing stack with room, in slot order. What is left over
    // (remainder) becomes a new stack.
    inline std::vector<Move> PlanDeposit(std::vector<Stack> stacks, uint32_t maxStack, uint32_t incoming, uint32_t& remainder)
    {
        std::sort(stacks.begin(), stacks.end(), [](Stack const& a, Stack const& b) { return a.Slot < b.Slot; });
        std::vector<Move> moves;
        for (Stack const& stack : stacks)
        {
            if (!incoming)
                break;
            if (stack.Count >= maxStack)
                continue;
            uint32_t amount = std::min(incoming, maxStack - stack.Count);
            moves.push_back({ stack.Slot, amount });
            incoming -= amount;
        }
        remainder = incoming;
        return moves;
    }

    // What to take from each stack to use need items, smallest stacks first. missing is what the stacks lack.
    inline std::vector<Move> PlanWithdraw(std::vector<Stack> stacks, uint32_t need, uint32_t& missing)
    {
        std::sort(stacks.begin(), stacks.end(), [](Stack const& a, Stack const& b)
        {
            return a.Count != b.Count ? a.Count < b.Count : a.Slot < b.Slot;
        });
        std::vector<Move> moves;
        for (Stack const& stack : stacks)
        {
            if (!need)
                break;
            uint32_t amount = std::min(need, stack.Count);
            if (!amount)
                continue;
            moves.push_back({ stack.Slot, amount });
            need -= amount;
        }
        missing = need;
        return moves;
    }

    // The first slot nothing uses.
    inline uint32_t FreeSlot(std::vector<uint32_t> const& taken)
    {
        uint32_t slot = 0;
        while (std::find(taken.begin(), taken.end(), slot) != taken.end())
            ++slot;
        return slot;
    }

    // Whether the bank has room for one more stack; maxStacks 0 means no limit.
    inline bool HasRoom(uint32_t stacks, uint32_t maxStacks)
    {
        return !maxStacks || stacks < maxStacks;
    }
}

#endif

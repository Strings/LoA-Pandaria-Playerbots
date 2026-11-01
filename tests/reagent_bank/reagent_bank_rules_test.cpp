#include "TestMain.h"
#include "ReagentBankRules.h"

using namespace ReagentBank;

namespace
{
    void Deposit()
    {
        uint32_t remainder = 99;
        // Empty bank: everything becomes a new stack.
        UMP_CHECK(PlanDeposit({ }, 20, 15, remainder).empty());
        UMP_CHECK(remainder == 15);

        // Tops up stacks with room in slot order, skips full ones.
        std::vector<Move> moves = PlanDeposit({ { 3, 18 }, { 0, 20 }, { 1, 5 } }, 20, 10, remainder);
        UMP_CHECK((moves == std::vector<Move>{ { 1, 10 } }));
        UMP_CHECK(remainder == 0);

        moves = PlanDeposit({ { 3, 18 }, { 0, 20 }, { 1, 5 } }, 20, 30, remainder);
        UMP_CHECK((moves == std::vector<Move>{ { 1, 15 }, { 3, 2 } }));
        UMP_CHECK(remainder == 13);

        // Unstackable items never merge.
        UMP_CHECK(PlanDeposit({ { 0, 1 } }, 1, 1, remainder).empty());
        UMP_CHECK(remainder == 1);
    }

    void Withdraw()
    {
        uint32_t missing = 99;
        std::vector<Move> moves = PlanWithdraw({ { 0, 20 }, { 1, 3 }, { 2, 7 } }, 8, missing);
        UMP_CHECK((moves == std::vector<Move>{ { 1, 3 }, { 2, 5 } }));
        UMP_CHECK(missing == 0);

        moves = PlanWithdraw({ { 0, 20 }, { 1, 3 } }, 30, missing);
        UMP_CHECK((moves == std::vector<Move>{ { 1, 3 }, { 0, 20 } }));
        UMP_CHECK(missing == 7);

        UMP_CHECK(PlanWithdraw({ }, 5, missing).empty());
        UMP_CHECK(missing == 5);

        // Equal stacks go in slot order.
        moves = PlanWithdraw({ { 4, 5 }, { 2, 5 } }, 6, missing);
        UMP_CHECK((moves == std::vector<Move>{ { 2, 5 }, { 4, 1 } }));
    }

    void Slots()
    {
        UMP_CHECK(FreeSlot({ }) == 0);
        UMP_CHECK(FreeSlot({ 0, 1, 3 }) == 2);
        UMP_CHECK(Total({ { 0, 4 }, { 9, 6 } }) == 10);
        UMP_CHECK(HasRoom(500, 0));
        UMP_CHECK(HasRoom(9, 10));
        UMP_CHECK(!HasRoom(10, 10));
    }
}

void RunTests()
{
    Deposit();
    Withdraw();
    Slots();
}

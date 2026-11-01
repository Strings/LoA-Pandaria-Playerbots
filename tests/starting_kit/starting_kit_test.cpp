#include "TestMain.h"
#include "StartingKitPolicy.h"
#include "FirstLoginHooks.h"
namespace
{
    int sequence = 0;
    void First(Player*)
    {
        sequence = sequence * 10 + 1;
    }
    void Second(Player*)
    {
        sequence = sequence * 10 + 2;
    }
}
void RunTests()
{
    using StartingKit::MountForRace;
    UMP_CHECK(!FirstLoginHooks::Register(nullptr));
    UMP_CHECK(FirstLoginHooks::Register(&First));
    UMP_CHECK(!FirstLoginHooks::Register(&First));
    UMP_CHECK(FirstLoginHooks::Register(&Second));
    FirstLoginHooks::Dispatch(nullptr);
    UMP_CHECK(sequence == 12);
    UMP_CHECK(MountForRace(1) == 458);
    UMP_CHECK(MountForRace(2) == 6654);
    UMP_CHECK(MountForRace(3) == 6899);
    UMP_CHECK(MountForRace(4) == 8394);
    UMP_CHECK(MountForRace(5) == 17462);
    UMP_CHECK(MountForRace(6) == 18989);
    UMP_CHECK(MountForRace(7) == 10969);
    UMP_CHECK(MountForRace(8) == 10796);
    UMP_CHECK(MountForRace(9) == 87090);
    UMP_CHECK(MountForRace(10) == 34795);
    UMP_CHECK(MountForRace(11) == 34406);
    UMP_CHECK(MountForRace(22) == 103195);
    UMP_CHECK(MountForRace(24) == MountForRace(25));
    UMP_CHECK(MountForRace(24) == MountForRace(26));
    UMP_CHECK(MountForRace(24) != 0);
    UMP_CHECK(MountForRace(0) == 0);
    UMP_CHECK(MountForRace(255) == 0);
}

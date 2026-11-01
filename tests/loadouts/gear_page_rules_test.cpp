#include "TestMain.h"
#include "GearPageRules.h"

using namespace GearPages;

namespace
{
    void Slots()
    {
        UMP_CHECK(SlotsFor(InvHead, false, false) == std::vector<uint8_t>{ 0 });
        UMP_CHECK(SlotsFor(InvRobe, false, false) == std::vector<uint8_t>{ 4 });
        UMP_CHECK((SlotsFor(InvFinger, false, false) == std::vector<uint8_t>{ 10, 11 }));
        UMP_CHECK(SlotsFor(InvWeapon, false, false) == std::vector<uint8_t>{ MainHand });
        UMP_CHECK((SlotsFor(InvWeapon, true, false) == std::vector<uint8_t>{ MainHand, OffHand }));
        UMP_CHECK(SlotsFor(InvTwoHand, true, false) == std::vector<uint8_t>{ MainHand });
        UMP_CHECK((SlotsFor(InvTwoHand, true, true) == std::vector<uint8_t>{ MainHand, OffHand }));
        UMP_CHECK(SlotsFor(InvRangedRight, false, false) == std::vector<uint8_t>{ MainHand });   // MoP: ranged in the main hand
        UMP_CHECK(SlotsFor(InvShield, false, false) == std::vector<uint8_t>{ OffHand });
        UMP_CHECK(SlotsFor(18, false, false).empty());                                           // bags aren't equipment here
    }

    void TwoHands()
    {
        UMP_CHECK(BlocksOffHand(InvTwoHand, 1, false));
        UMP_CHECK(!BlocksOffHand(InvTwoHand, 1, true));                    // Titan's Grip
        UMP_CHECK(BlocksOffHand(InvTwoHand, SubclassStaff, true));         // but not with a staff
        UMP_CHECK(BlocksOffHand(InvTwoHand, SubclassPolearm, true));
        UMP_CHECK(BlocksOffHand(InvRanged, 2, false));                     // bow
        UMP_CHECK(!BlocksOffHand(InvRangedRight, SubclassWand, false));
        UMP_CHECK(!BlocksOffHand(InvWeapon, 7, false));
    }

    void Choose()
    {
        std::vector<uint8_t> rings = { 10, 11 };
        UMP_CHECK(ChooseSlot(rings, { }, NoSlot) == 10);
        UMP_CHECK(ChooseSlot(rings, { 10 }, NoSlot) == 11);
        UMP_CHECK(ChooseSlot(rings, { 10, 11 }, NoSlot) == 10);           // both full: replaces the first
        UMP_CHECK(ChooseSlot(rings, { }, 11) == 11);
        UMP_CHECK(ChooseSlot(rings, { }, 12) == NoSlot);                  // a ring isn't a trinket
        UMP_CHECK(ChooseSlot({ }, { }, NoSlot) == NoSlot);
    }

    void Plan()
    {
        // The page brings a helm and a ring: only those come off.
        UMP_CHECK((SlotsToStore({ 0, 10 }, { 0, 1, 10, 11, 15 }, false, false) == std::vector<uint8_t>{ 0, 10 }));
        // A slot that is empty now needs nothing stored.
        UMP_CHECK((SlotsToStore({ 0, 4 }, { 4 }, false, false) == std::vector<uint8_t>{ 4 }));
        // Incoming two-hander: the worn off hand comes off too.
        UMP_CHECK((SlotsToStore({ MainHand }, { MainHand, OffHand }, true, false) == std::vector<uint8_t>{ MainHand, OffHand }));
        // Incoming one-hander keeps the worn off hand.
        UMP_CHECK((SlotsToStore({ MainHand }, { MainHand, OffHand }, false, false) == std::vector<uint8_t>{ MainHand }));
        // Incoming shield while a two-hander is worn: the two-hander comes off.
        UMP_CHECK((SlotsToStore({ OffHand }, { MainHand }, false, true) == std::vector<uint8_t>{ MainHand }));
        // Overflow slots on the incoming page never map to worn slots.
        UMP_CHECK(SlotsToStore({ 100 }, { 0 }, false, false).empty());
        UMP_CHECK(SlotsToStore({ }, { 0, 1, 2 }, false, false).empty());
    }

    void Overflow()
    {
        UMP_CHECK(FreeOverflow({ }) == OverflowStart);
        UMP_CHECK(FreeOverflow({ 0, OverflowStart, OverflowStart + 1 }) == OverflowStart + 2);
        UMP_CHECK(FreeOverflow({ OverflowStart + 1 }) == OverflowStart);
    }
}

void RunTests()
{
    Slots();
    TwoHands();
    Choose();
    Plan();
    Overflow();
}

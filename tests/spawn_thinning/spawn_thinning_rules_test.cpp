#include "TestMain.h"
#include "SpawnThinningRules.h"

#include <algorithm>

using namespace SpawnThinning;

namespace
{
    Spawn At(uint64_t id, float x, float y, uint32_t map = 0, uint32_t entry = 0)
    {
        Spawn spawn;
        spawn.Id = id;
        spawn.SpawnGroup = { map, 1, 0, 0, entry };
        spawn.X = x;
        spawn.Y = y;
        return spawn;
    }

    bool Has(std::vector<uint64_t> const& ids, uint64_t id)
    {
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    }

    void OffKeepsEverything()
    {
        UMP_CHECK(SelectThinned({ At(1, 0, 0), At(2, 1, 1) }, 0.0f).empty());
    }

    void OldestOfACrowdStays()
    {
        auto thinned = SelectThinned({ At(1, 0, 0), At(2, 3, 0), At(3, 0, 4) }, 10.0f);
        UMP_CHECK(thinned.size() == 2);
        UMP_CHECK(!Has(thinned, 1));
    }

    void SpacedSpawnsStay()
    {
        UMP_CHECK(SelectThinned({ At(1, 0, 0), At(2, 10.5f, 0), At(3, 0, -12) }, 10.0f).empty());
    }

    void ThinnedSpawnsDoNotBlock()
    {
        // 2 crowds 1 and is dropped; 3 is close to 2 only, so it stays.
        auto thinned = SelectThinned({ At(1, 0, 0), At(2, 6, 0), At(3, 13, 0) }, 10.0f);
        UMP_CHECK(thinned.size() == 1 && thinned[0] == 2);
    }

    void AcrossCellBorders()
    {
        // Neighbouring cells are searched: 9.9 and 10.1 sit in different cells but 0.2 yd apart.
        auto thinned = SelectThinned({ At(1, 9.9f, 9.9f), At(2, 10.1f, 10.1f), At(3, -0.1f, -0.1f), At(4, 0.1f, 0.1f) }, 10.0f);
        UMP_CHECK(Has(thinned, 2) && Has(thinned, 4) && !Has(thinned, 1) && !Has(thinned, 3));
    }

    void GroupsAreSeparate()
    {
        // Other map, other entry (same-entry mode): no crowding.
        UMP_CHECK(SelectThinned({ At(1, 0, 0, 0), At(2, 1, 0, 1) }, 10.0f).empty());
        UMP_CHECK(SelectThinned({ At(1, 0, 0, 0, 478), At(2, 1, 0, 0, 97) }, 10.0f).empty());
        UMP_CHECK(SelectThinned({ At(1, 0, 0, 0, 478), At(2, 1, 0, 0, 478) }, 10.0f).size() == 1);
    }

    void NegativeCoordinates()
    {
        auto thinned = SelectThinned({ At(1, -10022.0f, 635.0f), At(2, -10025.0f, 633.0f), At(3, -10040.0f, 635.0f) }, 10.0f);
        UMP_CHECK(thinned.size() == 1 && thinned[0] == 2);
    }
}

void RunTests()
{
    OffKeepsEverything();
    OldestOfACrowdStays();
    SpacedSpawnsStay();
    ThinnedSpawnsDoNotBlock();
    AcrossCellBorders();
    GroupsAreSeparate();
    NegativeCoordinates();
}

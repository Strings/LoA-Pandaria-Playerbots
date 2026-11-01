/*
 * The Heartseed's major choices as a loadout part (T-017). The Heartseed keeps one set of majors per spec; a
 * loadout keeps its own, so two loadouts of the same spec (a PvP build, say) can hold different choices.
 */

#include "LoadoutMgr.h"
#include "../Heartseed/HeartseedMgr.h"
#include "Player.h"

#include <sstream>

void AddLoadoutHeartseedPart()
{
    Loadouts::Part part;
    part.Name = "heartseed";
    // "t0 t1 t2 t3 t4 t5": the trait chosen at each major tier, 0 for none. Empty without a Heartseed.
    part.Capture = [](Player* player, uint16 specId) -> std::string
    {
        Heartseed::State state;
        if (!sHeartseedMgr->GetState(player, state))
            return "";
        std::array<uint32, Heartseed::MaxMajorTiers> majors = { };
        auto loadout = state.Loadouts.find(specId);
        if (loadout != state.Loadouts.end())
            majors = loadout->second;
        std::ostringstream out;
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
            out << (tier ? " " : "") << majors[tier];
        return out.str();
    };
    part.Apply = [](Player* player, uint16 specId, std::string const& data)
    {
        if (data.empty() || !sHeartseedMgr->HasHeartseed(player))
            return;
        std::array<uint32, Heartseed::MaxMajorTiers> majors = { };
        std::istringstream in(data);
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers && (in >> majors[tier]); ++tier)
            ;
        sHeartseedMgr->SetSpecMajors(player, specId, majors);
    };
    sLoadoutMgr->RegisterPart(std::move(part));
}

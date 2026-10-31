/*
 * Heartseed step types for checkpoints. The checkpoint engine stays generic; each system adds its own steps.
 */

#include "CheckpointMgr.h"
#include "../../Heartseed/HeartseedMgr.h"
#include "Player.h"

#include <algorithm>

namespace
{
    using Checkpoints::Step;

    bool NeedsHeartseed(Player* target, std::string& message)
    {
        if (sHeartseedMgr->HasHeartseed(target))
            return true;
        message = "no Heartseed (add a heartseed step first)";
        return false;
    }

    uint16 FlagFromStep(Step const& step)
    {
        if (!step.Text.empty())
            return HeartseedMgr::GetStoryFlagByName(step.Text);
        return uint16(step.Arg1);
    }
}

void AddCheckpointHeartseedSteps()
{
    sCheckpointMgr->RegisterStep("heartseed",
        [](ChatHandler* /*handler*/, Player* target, Step const& /*step*/, std::string& message)
        {
            // A GM grant: the granting quest stays as it is.
            message = sHeartseedMgr->HasHeartseed(target) ? "Heartseed already there" : "Heartseed granted";
            return sHeartseedMgr->HasHeartseed(target) || sHeartseedMgr->Grant(target);
        },
        [](Step const& /*step*/) { return std::string("Grant a Heartseed if there is none (the granting quest is left alone)."); });

    sCheckpointMgr->RegisterStep("heartseed_rank",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!NeedsHeartseed(target, message))
                return false;
            // The rank is the account's: a checkpoint never lowers it, so a twink checkpoint leaves the main alone.
            // The character uses it up to its own cap.
            Heartseed::State state;
            sHeartseedMgr->GetState(target, state);
            uint8 rank = uint8(std::clamp(step.Arg1, 1, int32(Heartseed::MaxRank)));
            if (state.Rank >= rank)
            {
                message = "account rank already " + std::to_string(state.Rank);
                return true;
            }
            message = "account rank " + std::to_string(rank);
            return sHeartseedMgr->SetRank(target, rank);
        },
        [](Step const& step) { return "Raise the account's Heartseed rank to at least " + std::to_string(step.Arg1) + " (the character uses it up to its cap)."; });

    sCheckpointMgr->RegisterStep("heartseed_growth",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!NeedsHeartseed(target, message))
                return false;
            message = "Growth " + std::to_string(step.Arg1);
            return sHeartseedMgr->SetGrowth(target, uint32(std::max(0, step.Arg1)));
        },
        [](Step const& step) { return "Set the account's banked Growth to " + std::to_string(step.Arg1) + "."; });

    sCheckpointMgr->RegisterStep("heartseed_major",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!NeedsHeartseed(target, message))
                return false;
            uint8 tier = step.Arg1 > 0 && step.Arg1 < 256 ? Heartseed::MajorTierForLevel(uint8(step.Arg1)) : Heartseed::MaxMajorTiers;
            if (tier >= Heartseed::MaxMajorTiers)
            {
                message = "no major tier at level " + std::to_string(step.Arg1);
                return false;
            }
            message = "level " + std::to_string(step.Arg1) + " major " + (step.Arg2 ? std::to_string(step.Arg2) : "cleared");
            return sHeartseedMgr->ChooseMajor(target, tier, uint32(std::max(0, step.Arg2)), true) == Heartseed::ChoiceResult::Ok;
        },
        [](Step const& step)
        {
            return step.Arg2 ? "Set the level " + std::to_string(step.Arg1) + " major to trait " + std::to_string(step.Arg2) + "."
                             : "Clear the level " + std::to_string(step.Arg1) + " major, so the choice window asks again.";
        });

    sCheckpointMgr->RegisterStep("heartseed_flag",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!NeedsHeartseed(target, message))
                return false;
            uint16 flag = FlagFromStep(step);
            if (!flag)
            {
                message = "unknown story flag " + step.Text;
                return false;
            }
            message = std::string("story flag ") + HeartseedMgr::GetStoryFlagName(flag) + (step.Arg2 ? " set" : " cleared");
            return sHeartseedMgr->SetStoryFlag(target, flag, step.Arg2 != 0);
        },
        [](Step const& step)
        {
            return std::string(step.Arg2 ? "Set" : "Clear") + " story flag " + (step.Text.empty() ? std::to_string(step.Arg1) : step.Text) +
                (step.Arg2 ? " (an awakening raises the rank cap)." : ".");
        });

    sCheckpointMgr->RegisterStep("heartseed_essence",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!NeedsHeartseed(target, message))
                return false;
            Heartseed::State state;
            sHeartseedMgr->GetState(target, state);
            if (!state.Essences.count(uint32(step.Arg1)) &&
                sHeartseedMgr->AddEssence(target, uint32(step.Arg1), 1) != Heartseed::EssenceResult::Ok)
            {
                message = "essence " + std::to_string(step.Arg1) + " could not be added";
                return false;
            }
            message = "essence " + std::to_string(step.Arg1) + (step.Arg2 ? " in slot " + std::to_string(step.Arg2) : "");
            if (!step.Arg2)
                return true;
            return sHeartseedMgr->SetEssenceSlot(target, uint32(step.Arg1), uint8(step.Arg2), true) == Heartseed::EssenceResult::Ok;
        },
        [](Step const& step)
        {
            return "Give essence " + std::to_string(step.Arg1) + (step.Arg2 ? " and put it in slot " + std::to_string(step.Arg2) : "") + ".";
        });
}

/*
 * GM test suite checkpoints and kits. See CheckpointMgr.h and README.md.
 */

#include "CheckpointMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "RBAC.h"
#include "ReputationMgr.h"
#include "Timer.h"
#include "TwinkGearRules.h"
#include "WorldSession.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace
{
    std::string Lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return text;
    }

    // "Alliance text|Horde text" picks by the target's team; text without '|' is used for both.
    std::string ForTeam(Player const* target, std::string const& text)
    {
        size_t bar = text.find('|');
        if (bar == std::string::npos)
            return text;
        return target->GetTeam() == ALLIANCE ? text.substr(0, bar) : text.substr(bar + 1);
    }

    std::string QualityName(int32 quality)
    {
        switch (quality)
        {
            case 2: return "green";
            case 3: return "blue";
            case 4: return "epic";
            default: return "quality " + std::to_string(quality);
        }
    }

    // Moves an equipped item to the bags; false when they are full.
    bool UnequipToBags(Player* target, uint8 slot)
    {
        Item* item = target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            return true;
        ItemPosCountVec bagDest;
        if (target->CanStoreItem(NULL_BAG, NULL_SLOT, bagDest, item, false) != EQUIP_ERR_OK)
            return false;
        target->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);
        target->StoreItem(bagDest, item, true);
        return true;
    }

    // Equips an item in one equipment slot: a copy already in the bags if there is one (unique items, and applying a
    // checkpoint twice stays tidy), else a new one. Whatever the slot held goes to the bags, so nothing is lost; when
    // the bags are full the old item stays and the new one is not equipped.
    bool EquipNewItemIn(Player* target, uint8 slot, uint32 entry)
    {
        if (Item* old = target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (old->GetEntry() == entry)
                return true;

        if (Item* carried = target->GetItemByEntry(entry))
        {
            uint16 dest = 0;
            if (!carried->IsEquipped() && target->CanEquipItem(slot, dest, carried, true) == EQUIP_ERR_OK)
            {
                target->SwapItem(carried->GetPos(), dest);
                target->AutoUnequipOffhandIfNeed();
                return target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) == carried;
            }
        }

        if (Item* old = target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemPosCountVec bagDest;
            if (target->CanStoreItem(NULL_BAG, NULL_SLOT, bagDest, old, false) != EQUIP_ERR_OK)
                return false;
            target->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);
            target->StoreItem(bagDest, old, true);
        }
        uint16 dest = 0;
        if (target->CanEquipNewItem(slot, dest, entry, false) != EQUIP_ERR_OK || !target->EquipNewItem(dest, entry, true))
            return false;
        target->AutoUnequipOffhandIfNeed();
        return true;
    }

    uint8 PrimaryStats(ItemTemplate const* proto)
    {
        uint8 stats = 0;
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            switch (proto->ItemStat[i].ItemStatType)
            {
                case ITEM_MOD_STRENGTH: stats |= TwinkGear::StatStrength; break;
                case ITEM_MOD_AGILITY: stats |= TwinkGear::StatAgility; break;
                case ITEM_MOD_INTELLECT:
                case ITEM_MOD_SPELL_POWER: stats |= TwinkGear::StatIntellect; break;
                default: break;
            }
        }
        return stats;
    }

    // Runs a GM command as the applying GM, so a step can reuse an existing command instead of duplicating it.
    bool RunCommand(ChatHandler* handler, std::string command)
    {
        if (command.empty())
            return false;
        if (command[0] != '.')
            command = "." + command;
        return handler->ParseCommands(command.c_str());
    }
}

CheckpointMgr* CheckpointMgr::instance()
{
    static CheckpointMgr instance;
    return &instance;
}

CheckpointMgr::CheckpointMgr()
{
    using Checkpoints::Step;

    RegisterStep("level",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (step.Arg1 < 1 || step.Arg1 > MAX_LEVEL)
            {
                message = "bad level " + std::to_string(step.Arg1);
                return false;
            }
            // The same calls as .character level, so level-down works too.
            if (target->GetLevel() != uint8(step.Arg1))
            {
                target->GiveLevel(uint8(step.Arg1));
                target->InitTalentForLevel();
            }
            target->SetUInt32Value(PLAYER_FIELD_XP, 0);
            message = "level " + std::to_string(step.Arg1);
            return true;
        },
        [](Step const& step) { return "Set level " + std::to_string(step.Arg1) + "."; });

    // Gear comes from the playerbots gear factory (.botscenario equip), the same code that gears bots by level.
    RegisterStep("gear",
        [](ChatHandler* handler, Player* target, Step const& step, std::string& message)
        {
            if (handler->GetSession() && !handler->HasPermission(rbac::RBAC_PERM_COMMAND_PMON))
            {
                message = "needs permission 1345 (.botscenario) for gear";
                return false;
            }
            std::string command = ".botscenario " + target->GetName() + " equip " + std::to_string(step.Arg1);
            if (!step.Text.empty())
                command += " " + step.Text;
            message = QualityName(step.Arg1) + " gear for level " + std::to_string(target->GetLevel());
            return RunCommand(handler, command);
        },
        [](Step const& step)
        {
            return "Equip " + QualityName(step.Arg1) + " gear for the level (upgrades only" +
                (step.Text == "nocap" ? ", no gear-score cap" : "") + ").";
        });

    RegisterStep("class_spells",
        [](ChatHandler* handler, Player* target, Step const& /*step*/, std::string& message)
        {
            // .learn all my class only works on the GM's own character.
            if (!handler->GetSession() || handler->GetSession()->GetPlayer() != target)
            {
                message = "class spells only on yourself; skipped";
                return false;
            }
            message = "class spells";
            return RunCommand(handler, ".learn all my class");
        },
        [](Step const& /*step*/) { return std::string("Learn every class spell for the level (only when applied to yourself)."); });

    RegisterStep("spell",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            if (!sSpellStore.LookupEntry(uint32(step.Arg1)))
            {
                message = "unknown spell " + std::to_string(step.Arg1);
                return false;
            }
            if (!target->HasSpell(uint32(step.Arg1)))
                target->LearnSpell(uint32(step.Arg1), false);
            message = "spell " + std::to_string(step.Arg1);
            return true;
        },
        [](Step const& step) { return "Learn spell " + std::to_string(step.Arg1) + "."; });

    RegisterStep("item",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            uint32 count = step.Arg2 > 0 ? uint32(step.Arg2) : 1;
            message = "item " + std::to_string(step.Arg1) + " x" + std::to_string(count);
            if (!sObjectMgr->GetItemTemplate(uint32(step.Arg1)))
            {
                message = "unknown item " + std::to_string(step.Arg1);
                return false;
            }
            return target->AddItem(uint32(step.Arg1), count);
        },
        [](Step const& step) { return "Add item " + std::to_string(step.Arg1) + " x" + std::to_string(std::max(1, step.Arg2)) + "."; });

    // Marks a quest done with its normal rewards. Put quest steps before the level step: their experience is
    // reset by it.
    RegisterStep("quest",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(uint32(step.Arg1));
            if (!quest)
            {
                message = "unknown quest " + std::to_string(step.Arg1);
                return false;
            }
            message = "quest " + std::to_string(step.Arg1);
            if (target->GetQuestRewardStatus(quest->GetQuestId()))
                return true;
            if (target->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_NONE)
                target->AddQuest(quest, target);
            target->CompleteQuest(quest->GetQuestId(), true, true);
            target->RewardQuest(quest, 0, target, false);
            return target->GetQuestRewardStatus(quest->GetQuestId());
        },
        [](Step const& step) { return "Complete and turn in quest " + std::to_string(step.Arg1) + "."; });

    RegisterStep("reputation",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            FactionEntry const* faction = sFactionStore.LookupEntry(uint32(step.Arg1));
            if (!faction || !faction->CanHaveReputation())
            {
                message = "faction " + std::to_string(step.Arg1) + " has no reputation";
                return false;
            }
            message = "reputation " + std::to_string(step.Arg1) + " = " + std::to_string(step.Arg2);
            return target->GetReputationMgr().SetReputation(faction, float(step.Arg2));
        },
        [](Step const& step)
        {
            return "Set reputation with faction " + std::to_string(step.Arg1) + " to " + std::to_string(step.Arg2) + " (0 = neutral, 3000 friendly, 9000 honored, 21000 revered, 42000 exalted).";
        });

    RegisterStep("tele",
        [](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            std::string name = ForTeam(target, step.Text);
            GameTele const* tele = sObjectMgr->GetGameTeleExactName(name);
            if (!tele)
                tele = sObjectMgr->GetGameTele(name);
            if (!tele)
            {
                message = "no game_tele named " + name;
                return false;
            }
            message = "teleport to " + tele->name;
            if (target->IsInFlight())
            {
                target->GetMotionMaster()->MovementExpired();
                target->CleanupAfterTaxiFlight();
            }
            return target->TeleportTo(tele->mapId, tele->position_x, tele->position_y, tele->position_z, tele->orientation);
        },
        [](Step const& step) { return "Teleport to " + step.Text + " (game_tele; Alliance|Horde)."; });

    // Best twink gear for the target's class, spec and current level, from the twink candidates (T-045's shortlist
    // and its tuned items). The class spells must be learned first: armor and weapon proficiencies come with them.
    RegisterStep("twink_gear",
        [this](ChatHandler* /*handler*/, Player* target, Step const& /*step*/, std::string& message)
        {
            std::vector<std::pair<uint32, uint8>> pool;
            {
                std::lock_guard<std::mutex> guard(_lock);
                pool = _twinkCandidates;
            }
            std::vector<TwinkGear::Candidate> candidates;
            for (auto const& [entry, bracket] : pool)
            {
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);
                if (!proto || bracket > target->GetLevel() || target->CanUseItem(proto) != EQUIP_ERR_OK)
                    continue;
                candidates.push_back({ entry, uint8(proto->Class), uint8(proto->SubClass), uint8(proto->InventoryType),
                    proto->ItemLevel, uint8(proto->Quality), PrimaryStats(proto), uint8(proto->RequiredLevel) });
            }
            candidates = TwinkGear::WithinLevel(candidates, target->GetLevel());

            // After a level-down, equipped gear the character can no longer use goes to the bags.
            uint32 unequipped = 0;
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                Item* item = target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (item && target->CanUseItem(item->GetTemplate()) != EQUIP_ERR_OK && UnequipToBags(target, slot))
                    ++unequipped;
            }

            TwinkGear::Profile profile = TwinkGear::ProfileFor(target->GetClass(), target->GetTalentSpecialization());
            std::vector<TwinkGear::Pick> picks = TwinkGear::Select(profile, candidates);
            uint32 equipped = 0;
            std::string missed;
            std::set<uint8> picked;
            for (TwinkGear::Pick const& pick : picks)
            {
                picked.insert(pick.Slot);
                if (EquipNewItemIn(target, pick.Slot, pick.Entry))
                    ++equipped;
                else
                    missed += " " + std::to_string(pick.Entry);
            }
            // The slots twink gear manages end up with twink gear or nothing, so a checkpoint gives the same result
            // whatever was worn before. Shirt, tabard and ranged are left alone.
            for (uint8 slot = EQUIPMENT_SLOT_START; slot <= EQUIPMENT_SLOT_OFFHAND; ++slot)
                if (slot != EQUIPMENT_SLOT_BODY && !picked.count(slot) && target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) &&
                    UnequipToBags(target, slot))
                    ++unequipped;
            message = std::to_string(equipped) + " of " + std::to_string(picks.size()) + " twink items equipped from " +
                std::to_string(candidates.size()) + " usable candidates" +
                (unequipped ? "; " + std::to_string(unequipped) + " unusable items moved to the bags" : "") +
                (missed.empty() ? "" : "; not equipped:" + missed);
            return !picks.empty() && missed.empty();
        },
        [](Step const& /*step*/)
        {
            return std::string("Equip the best twink gear for the class, spec and level (armor type, primary stat, weapon style).");
        });

    RegisterStep("kit",
        [this](ChatHandler* /*handler*/, Player* target, Step const& step, std::string& message)
        {
            Checkpoints::Kit kit;
            if (!GetKit(step.Text, kit))
            {
                message = "no kit named " + step.Text;
                return false;
            }
            ApplyKit(target, kit, message);
            return true;
        },
        [](Step const& step) { return "Apply kit " + step.Text + "."; });

    // Escape hatch for anything without its own step type yet. $name is the target's name.
    RegisterStep("command",
        [](ChatHandler* handler, Player* target, Step const& step, std::string& message)
        {
            std::string command = step.Text;
            for (size_t at = command.find("$name"); at != std::string::npos; at = command.find("$name"))
                command.replace(at, 5, target->GetName());
            message = command;
            return RunCommand(handler, command);
        },
        [](Step const& step) { return "Run " + step.Text + "."; });
}

void CheckpointMgr::RegisterStep(std::string const& type, Checkpoints::StepHandler handler, Checkpoints::StepExplainer explainer)
{
    std::lock_guard<std::mutex> guard(_lock);
    _stepTypes[Lower(type)] = { std::move(handler), std::move(explainer) };
}

bool CheckpointMgr::IsKnownStep(std::string const& type) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _stepTypes.count(Lower(type)) != 0;
}

void CheckpointMgr::LoadData()
{
    uint32 oldMSTime = getMSTime();
    std::map<std::string, Checkpoints::Checkpoint> checkpoints;
    std::map<std::string, Checkpoints::Kit> kits;
    uint32 steps = 0, items = 0;

    if (QueryResult result = WorldDatabase.Query("SELECT `name`, `category`, `description`, `sort_order` FROM `gmtools_checkpoint`"))
    {
        do
        {
            Field* fields = result->Fetch();
            Checkpoints::Checkpoint checkpoint;
            checkpoint.Name = fields[0].GetString();
            checkpoint.Category = fields[1].GetString();
            checkpoint.Description = fields[2].GetString();
            checkpoint.SortOrder = fields[3].GetUInt32();
            checkpoints[Lower(checkpoint.Name)] = checkpoint;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `checkpoint`, `step`, `type`, `arg1`, `arg2`, `text`, `comment` FROM `gmtools_checkpoint_step` ORDER BY `checkpoint`, `step`"))
    {
        do
        {
            Field* fields = result->Fetch();
            auto checkpoint = checkpoints.find(Lower(fields[0].GetString()));
            if (checkpoint == checkpoints.end())
            {
                TC_LOG_ERROR("sql.sql", "gmtools_checkpoint_step: checkpoint '%s' does not exist, skipped.", fields[0].GetCString());
                continue;
            }
            Checkpoints::Step step;
            step.Order = fields[1].GetUInt8();
            step.Type = Lower(fields[2].GetString());
            step.Arg1 = fields[3].GetInt32();
            step.Arg2 = fields[4].GetInt32();
            step.Text = fields[5].GetString();
            step.Comment = fields[6].GetString();
            if (!IsKnownStep(step.Type))
                TC_LOG_ERROR("sql.sql", "gmtools_checkpoint_step: '%s' step %u has unknown type '%s'; it will fail when applied.",
                    fields[0].GetCString(), uint32(step.Order), step.Type.c_str());
            checkpoint->second.Steps.push_back(step);
            ++steps;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `name`, `description`, `min_level`, `max_level` FROM `gmtools_kit`"))
    {
        do
        {
            Field* fields = result->Fetch();
            Checkpoints::Kit kit;
            kit.Name = fields[0].GetString();
            kit.Description = fields[1].GetString();
            kit.MinLevel = fields[2].GetUInt8();
            kit.MaxLevel = fields[3].GetUInt8();
            kits[Lower(kit.Name)] = kit;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `kit`, `item`, `count`, `class_mask`, `equip` FROM `gmtools_kit_item`"))
    {
        do
        {
            Field* fields = result->Fetch();
            auto kit = kits.find(Lower(fields[0].GetString()));
            if (kit == kits.end())
            {
                TC_LOG_ERROR("sql.sql", "gmtools_kit_item: kit '%s' does not exist, skipped.", fields[0].GetCString());
                continue;
            }
            Checkpoints::KitItem item;
            item.Entry = fields[1].GetUInt32();
            item.Count = std::max<uint32>(1, fields[2].GetUInt32());
            item.ClassMask = fields[3].GetUInt32();
            item.Equip = fields[4].GetUInt8() != 0;
            if (!sObjectMgr->GetItemTemplate(item.Entry))
            {
                TC_LOG_ERROR("sql.sql", "gmtools_kit_item: kit '%s' item %u does not exist, skipped.", fields[0].GetCString(), item.Entry);
                continue;
            }
            kit->second.Items.push_back(item);
            ++items;
        } while (result->NextRow());
    }

    // Twink candidates: the shortlist plus the tuned twink items (Heartseed's twink_item), each at its lowest bracket.
    std::map<uint32, uint8> twink;
    if (QueryResult result = WorldDatabase.Query("SELECT `entry`, `bracket` FROM `gmtools_twink_candidate` UNION ALL SELECT `entry`, `bracket` FROM `twink_item`"))
    {
        do
        {
            Field* fields = result->Fetch();
            auto [itr, added] = twink.emplace(fields[0].GetUInt32(), fields[1].GetUInt8());
            if (!added)
                itr->second = std::min(itr->second, fields[1].GetUInt8());
        } while (result->NextRow());
    }

    {
        std::lock_guard<std::mutex> guard(_lock);
        _checkpoints.swap(checkpoints);
        _kits.swap(kits);
        _twinkCandidates.assign(twink.begin(), twink.end());
    }
    TC_LOG_INFO("server.loading", ">> Loaded %u checkpoints (%u steps), %u kits (%u items) and %u twink candidates in %u ms",
        uint32(_checkpoints.size()), steps, uint32(_kits.size()), items, uint32(twink.size()), GetMSTimeDiffToNow(oldMSTime));
}

std::vector<Checkpoints::Checkpoint> CheckpointMgr::GetCheckpoints() const
{
    std::vector<Checkpoints::Checkpoint> list;
    {
        std::lock_guard<std::mutex> guard(_lock);
        for (auto const& [name, checkpoint] : _checkpoints)
            list.push_back(checkpoint);
    }
    std::stable_sort(list.begin(), list.end(), [](Checkpoints::Checkpoint const& a, Checkpoints::Checkpoint const& b) { return a.SortOrder < b.SortOrder; });
    return list;
}

bool CheckpointMgr::GetCheckpoint(std::string const& name, Checkpoints::Checkpoint& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _checkpoints.find(Lower(name));
    if (itr == _checkpoints.end())
        return false;
    out = itr->second;
    return true;
}

std::vector<Checkpoints::Kit> CheckpointMgr::GetKits() const
{
    std::vector<Checkpoints::Kit> list;
    std::lock_guard<std::mutex> guard(_lock);
    for (auto const& [name, kit] : _kits)
        list.push_back(kit);
    return list;
}

bool CheckpointMgr::GetKit(std::string const& name, Checkpoints::Kit& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _kits.find(Lower(name));
    if (itr == _kits.end())
        return false;
    out = itr->second;
    return true;
}

uint32 CheckpointMgr::Apply(ChatHandler* handler, Player* target, Checkpoints::Checkpoint const& checkpoint)
{
    uint32 failed = 0;
    for (Checkpoints::Step const& step : checkpoint.Steps)
    {
        Checkpoints::StepHandler stepHandler;
        {
            std::lock_guard<std::mutex> guard(_lock);
            auto type = _stepTypes.find(step.Type);
            if (type != _stepTypes.end())
                stepHandler = type->second.Handler;
        }

        std::string message;
        bool ok = false;
        if (!stepHandler)
            message = "unknown step type " + step.Type;
        else
            ok = stepHandler(handler, target, step, message);
        if (!ok)
            ++failed;
        handler->PSendSysMessage(" %u %s: %s%s", uint32(step.Order), step.Type.c_str(), message.c_str(), ok ? "" : " (failed)");
    }
    TC_LOG_INFO("gmtools", "Checkpoint '%s' applied to %s (%u failed steps).", checkpoint.Name.c_str(), target->GetName().c_str(), failed);
    return failed;
}

std::vector<std::string> CheckpointMgr::Explain(Checkpoints::Checkpoint const& checkpoint) const
{
    std::vector<std::string> lines;
    std::lock_guard<std::mutex> guard(_lock);
    for (Checkpoints::Step const& step : checkpoint.Steps)
    {
        auto type = _stepTypes.find(step.Type);
        std::string line = std::to_string(step.Order) + ". " +
            (type != _stepTypes.end() ? type->second.Explainer(step) : "Unknown step type " + step.Type + ".");
        if (!step.Comment.empty())
            line += " (" + step.Comment + ")";
        lines.push_back(line);
    }
    return lines;
}

uint32 CheckpointMgr::ApplyKit(Player* target, Checkpoints::Kit const& kit, std::string& message)
{
    uint32 classBit = 1u << (target->GetClass() - 1);
    uint32 equipped = 0, bagged = 0, skipped = 0, failed = 0;
    for (Checkpoints::KitItem const& item : kit.Items)
    {
        if (item.ClassMask && !(item.ClassMask & classBit))
            continue;

        if (item.Equip)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item.Entry);
            uint8 slot = target->FindEquipSlot(proto, NULL_SLOT, true);
            // Bag slots all full: the kit's bags are already there; swapping out a bag would empty it.
            if (proto->InventoryType == INVTYPE_BAG && slot < INVENTORY_SLOT_BAG_END && target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                ++skipped;
                continue;
            }
            if (slot < EQUIPMENT_SLOT_END && EquipNewItemIn(target, slot, item.Entry))
            {
                ++equipped;
                continue;
            }
        }

        if (target->AddItem(item.Entry, item.Count))
            ++bagged;
        else
            ++failed;
    }
    message = "kit " + kit.Name + ": " + std::to_string(equipped) + " equipped, " + std::to_string(bagged) + " to bags" +
        (skipped ? ", " + std::to_string(skipped) + " bags already there" : "") + (failed ? ", " + std::to_string(failed) + " did not fit" : "");
    return failed;
}

#include "NoBinding.h"
#include "BindingRules.h"
#include "Bag.h"
#include "Config.h"
#include "DB2Enums.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <ctime>
#include <memory>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace
{
    struct Settings
    {
        bool Enabled = true;
        uint32 Version = 0;
        std::unordered_set<uint32> ProtectedEntries;
    };
    std::shared_ptr<Settings const> settings = std::make_shared<Settings>();
    std::vector<NoBinding::ProtectionCheck> protectionChecks;

    BindingRules::Item Describe(ItemTemplate const* item, Settings const& config)
    {
        if (!item)
        {
            BindingRules::Item unknown;
            unknown.Known = false;
            return unknown;
        }
        return { true, item->Class, item->Bonding, item->Quality, item->StartQuest,
            config.ProtectedEntries.count(item->ItemId) != 0, false };
    }
}

void NoBinding::Load()
{
    auto next = std::make_shared<Settings>();
    next->Enabled = sConfigMgr->GetBoolDefault("NoBinding.Enable", true);
    std::string entries = sConfigMgr->GetStringDefault("NoBinding.ProtectedItemIds", "");
    std::replace(entries.begin(), entries.end(), ',', ' ');
    std::istringstream input(entries);
    std::string token;
    while (input >> token)
    {
        uint32 entry = 0;
        auto parsed = std::from_chars(token.data(), token.data() + token.size(), entry);
        if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size() || !entry)
        {
            next->Enabled = false;
            TC_LOG_ERROR("server.loading", "NoBinding disabled: invalid protected item ID '%s'", token.c_str());
        }
        else
            next->ProtectedEntries.insert(entry);
    }
    auto previous = std::atomic_load(&settings);
    next->Version = BindingRules::NextVersion(previous->Version, uint32(time(nullptr)));
    std::atomic_store(&settings, std::shared_ptr<Settings const>(next));
}

bool NoBinding::Enabled()
{
    return std::atomic_load(&settings)->Enabled;
}

bool NoBinding::RegisterProtectionCheck(ProtectionCheck check)
{
    if (!check || std::find(protectionChecks.begin(), protectionChecks.end(), check) != protectionChecks.end())
        return false;
    protectionChecks.push_back(check);
    return true;
}

bool NoBinding::SkipBinding(Object const& object)
{
    if (!object.isType(TYPEMASK_ITEM))
        return false;
    Item const& item = static_cast<Item const&>(object);
    auto config = std::atomic_load(&settings);
    auto description = Describe(item.GetTemplate(), *config);
    if (!BindingRules::Suppress(config->Enabled, description))
        return false;
    for (ProtectionCheck check : protectionChecks)
        if (check(item))
            return false;
    return true;
}

bool NoBinding::SkipTemplateBinding(ItemTemplate const* item)
{
    auto config = std::atomic_load(&settings);
    return BindingRules::Suppress(config->Enabled, Describe(item, *config));
}

uint32 NoBinding::TooltipBonding(ItemTemplate const* item)
{
    return SkipTemplateBinding(item) ? BIND_NONE : item->Bonding;
}

uint32 NoBinding::TooltipFlags(ItemTemplate const* item)
{
    return BindingRules::TooltipFlags(item->Flags, SkipTemplateBinding(item));
}

uint32 NoBinding::HotfixTimestamp(uint32 entry, uint32 type, uint32 original)
{
    if (type == DB2_REPLY_SPARSE)
        if (ItemTemplate const* item = sObjectMgr->GetItemTemplate(entry))
            // Announce both enabled and disabled data so a cached tooltip can revert too.
            if (item->Bonding || (item->Flags & ITEM_PROTO_FLAG_BIND_TO_ACCOUNT))
                return std::max(original, std::atomic_load(&settings)->Version);
    return original;
}

uint32 NoBinding::RefreshPlayer(Player* player)
{
    uint32 count = 0;
    auto clear = [&](Item* item)
    {
        if (item && item->IsSoulBound() && SkipBinding(*item))
        {
            item->SetBinding(false);
            item->SetState(ITEM_CHANGED, player);
            ++count;
        }
    };
    for (uint16 slot = 0; slot < PLAYER_SLOT_END; ++slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(slot));
        clear(item);
        if (item && item->IsBag())
            for (uint32 inner = 0; inner < static_cast<Bag*>(item)->GetBagSize(); ++inner)
                clear(static_cast<Bag*>(item)->GetItemByPos(uint8(inner)));
    }
    return count;
}

void NoBinding::SendHotfixInfo(Player* player)
{
    HotfixData const& hotfix = sObjectMgr->GetHotfixData();
    size_t offset = 0;
    do
    {
        uint32 count = BindingRules::HotfixBatchRecords(uint32(hotfix.size() - offset));
        WorldPacket packet(SMSG_HOTFIX_INFO, count * 12 + 3);
        packet.WriteBits(count, 20);
        packet.FlushBits();
        for (size_t i = offset; i < offset + count; ++i)
        {
            auto const& row = hotfix[i];
            packet << uint32(HotfixTimestamp(row.Entry, row.TableHash, uint32(row.Timestamp)));
            packet << uint32(row.Entry);
            packet << uint32(row.TableHash);
        }
        player->GetSession()->SendPacket(&packet);
        offset += count;
    } while (offset < hotfix.size());
}

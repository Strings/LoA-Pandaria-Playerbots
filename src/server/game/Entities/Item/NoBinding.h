#ifndef ULTIMATEMOP_NO_BINDING_H
#define ULTIMATEMOP_NO_BINDING_H

#include <cstdint>

class Object;
class Item;
class Player;
struct ItemTemplate;

namespace NoBinding
{
    using ProtectionCheck = bool (*)(Item const&);

    void Load();
    bool Enabled();
    // Register at startup, before player loading. Detached/progression systems own instance rules.
    bool RegisterProtectionCheck(ProtectionCheck check);
    bool SkipBinding(Object const& object);
    bool SkipTemplateBinding(ItemTemplate const* item);
    std::uint32_t TooltipBonding(ItemTemplate const* item);
    std::uint32_t TooltipFlags(ItemTemplate const* item);
    std::uint32_t HotfixTimestamp(std::uint32_t entry, std::uint32_t type, std::uint32_t original);
    std::uint32_t RefreshPlayer(Player* player);
    void SendHotfixInfo(Player* player);
}
#endif

/*
 * Spec loadouts (T-017). See LoadoutMgr.h and README.md.
 */

#include "LoadoutMgr.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

#include <algorithm>
#include <set>
#include <sstream>

using namespace Loadouts;

namespace
{
    // Characters mid-switch, so the OnSpecializationChanged that ActivateSpec fires is ours, not the client's.
    std::mutex switchingLock;
    std::set<uint32> switching;

    struct SwitchGuard
    {
        explicit SwitchGuard(uint32 guid) : Guid(guid)
        {
            std::lock_guard<std::mutex> guard(switchingLock);
            switching.insert(guid);
        }
        ~SwitchGuard()
        {
            std::lock_guard<std::mutex> guard(switchingLock);
            switching.erase(Guid);
        }
        uint32 Guid;
    };

    bool IsSwitching(uint32 guid)
    {
        std::lock_guard<std::mutex> guard(switchingLock);
        return switching.count(guid) != 0;
    }

    // Loadouts are for players: random bots and the player's own alts logged in as bots skip the queries.
    bool Applies(Player const* player)
    {
        return player && !player->GetSession()->IsBot() && sLoadoutMgr->Enabled();
    }
}

LoadoutMgr* LoadoutMgr::instance()
{
    static LoadoutMgr instance;
    return &instance;
}

bool LoadoutMgr::Enabled() const
{
    return sConfigMgr->GetBoolDefault("Loadouts.Enable", true);
}

uint32 LoadoutMgr::MaxNamed() const
{
    return uint32(sConfigMgr->GetIntDefault("Loadouts.MaxNamed", 10));
}

void LoadoutMgr::RegisterPart(Part part)
{
    std::lock_guard<std::mutex> guard(_lock);
    _parts.push_back(std::move(part));
}

std::vector<uint16> LoadoutMgr::ClassSpecs(Player* player) const
{
    std::vector<uint16> specs;
    for (uint32 spec : dbc::GetClassSpecializations(player->GetClass()))
        if (spec && sChrSpecializationStore.LookupEntry(spec))
            specs.push_back(uint16(spec));
    return specs;
}

std::string LoadoutMgr::Fingerprint(Loadout const& loadout)
{
    std::ostringstream out;
    out << loadout.Name << '\t' << loadout.SpecId << '\t' << loadout.IsDefault << '\t' << uint32(loadout.Group) << '\t'
        << loadout.Captured << "\tT";
    for (uint32 talent : loadout.Talents)
        out << ' ' << talent;
    out << "\tG";
    for (uint32 glyph : loadout.Glyphs)
        out << ' ' << glyph;
    out << "\tA";
    for (ActionEntry const& action : loadout.Actions)
        out << ' ' << uint32(action.Button) << ':' << action.Action << ':' << uint32(action.Type);
    for (auto const& [name, data] : loadout.Parts)
        out << "\tP" << name << '=' << data;
    return out.str();
}

void LoadoutMgr::LoadPlayer(Player* player)
{
    if (!Applies(player))
        return;

    uint32 guid = player->GetGUID().GetCounter();
    PlayerData data;

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `id`, `name`, `spec_id`, `is_default`, `talent_group`, `captured` "
        "FROM `character_loadout` WHERE `guid` = %u", guid))
    {
        do
        {
            Field* fields = result->Fetch();
            Loadout loadout;
            loadout.Id = fields[0].GetUInt32();
            loadout.Name = fields[1].GetString();
            loadout.SpecId = fields[2].GetUInt16();
            loadout.IsDefault = fields[3].GetUInt8() != 0;
            loadout.Group = fields[4].GetUInt8();
            loadout.Captured = fields[5].GetUInt8() != 0;
            if (loadout.Group >= MAX_TALENT_SPECS)
                loadout.Group = NoGroup;
            data.Loadouts[loadout.Id] = loadout;
        } while (result->NextRow());
    }

    auto find = [&data](uint32 id) -> Loadout*
    {
        auto itr = data.Loadouts.find(id);
        return itr != data.Loadouts.end() ? &itr->second : nullptr;
    };

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `loadout_id`, `spell` FROM `character_loadout_talent` WHERE `guid` = %u", guid))
        do
            if (Loadout* loadout = find((*result)[0].GetUInt32()))
                loadout->Talents.push_back((*result)[1].GetUInt32());
        while (result->NextRow());

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `loadout_id`, `slot`, `glyph` FROM `character_loadout_glyph` WHERE `guid` = %u", guid))
        do
            if (Loadout* loadout = find((*result)[0].GetUInt32()))
                if ((*result)[1].GetUInt8() < GlyphSlots)
                    loadout->Glyphs[(*result)[1].GetUInt8()] = (*result)[2].GetUInt32();
        while (result->NextRow());

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `loadout_id`, `button`, `action`, `type` FROM `character_loadout_action` "
        "WHERE `guid` = %u ORDER BY `button`", guid))
        do
            if (Loadout* loadout = find((*result)[0].GetUInt32()))
                loadout->Actions.push_back({ (*result)[1].GetUInt8(), (*result)[2].GetUInt32(), (*result)[3].GetUInt8() });
        while (result->NextRow());

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `loadout_id`, `part`, `data` FROM `character_loadout_part` WHERE `guid` = %u", guid))
        do
            if (Loadout* loadout = find((*result)[0].GetUInt32()))
                loadout->Parts[(*result)[1].GetString()] = (*result)[2].GetString();
        while (result->NextRow());

    for (auto& [id, loadout] : data.Loadouts)
    {
        std::sort(loadout.Talents.begin(), loadout.Talents.end());
        data.Saved[id] = Fingerprint(loadout);
    }

    std::lock_guard<std::mutex> guard(_lock);
    _players[guid] = std::move(data);
}

void LoadoutMgr::SavePlayer(Player* player)
{
    if (!Applies(player) || IsSwitching(player->GetGUID().GetCounter()))
        return;

    CaptureActive(player);

    uint32 guid = player->GetGUID().GetCounter();
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(guid);
    if (itr == _players.end())
        return;
    PlayerData& data = itr->second;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    bool any = false;
    auto deleteRows = [&trans, guid](uint32 id)
    {
        trans->PAppend("DELETE FROM `character_loadout` WHERE `guid` = %u AND `id` = %u", guid, id);
        trans->PAppend("DELETE FROM `character_loadout_talent` WHERE `guid` = %u AND `loadout_id` = %u", guid, id);
        trans->PAppend("DELETE FROM `character_loadout_glyph` WHERE `guid` = %u AND `loadout_id` = %u", guid, id);
        trans->PAppend("DELETE FROM `character_loadout_action` WHERE `guid` = %u AND `loadout_id` = %u", guid, id);
        trans->PAppend("DELETE FROM `character_loadout_part` WHERE `guid` = %u AND `loadout_id` = %u", guid, id);
    };

    for (uint32 id : data.Deleted)
    {
        deleteRows(id);
        data.Saved.erase(id);
        any = true;
    }
    data.Deleted.clear();

    for (auto const& [id, loadout] : data.Loadouts)
    {
        std::string fingerprint = Fingerprint(loadout);
        auto saved = data.Saved.find(id);
        if (saved != data.Saved.end() && saved->second == fingerprint)
            continue;

        deleteRows(id);
        std::string name = loadout.Name;
        CharacterDatabase.EscapeString(name);
        trans->PAppend("INSERT INTO `character_loadout` (`guid`, `id`, `name`, `spec_id`, `is_default`, `talent_group`, `captured`) "
            "VALUES (%u, %u, '%s', %u, %u, %u, %u)", guid, id, name.c_str(), uint32(loadout.SpecId), uint32(loadout.IsDefault),
            uint32(loadout.Group), uint32(loadout.Captured));
        for (uint32 talent : loadout.Talents)
            trans->PAppend("INSERT INTO `character_loadout_talent` (`guid`, `loadout_id`, `spell`) VALUES (%u, %u, %u)", guid, id, talent);
        for (uint8 slot = 0; slot < GlyphSlots; ++slot)
            if (loadout.Glyphs[slot])
                trans->PAppend("INSERT INTO `character_loadout_glyph` (`guid`, `loadout_id`, `slot`, `glyph`) VALUES (%u, %u, %u, %u)",
                    guid, id, uint32(slot), loadout.Glyphs[slot]);
        for (ActionEntry const& action : loadout.Actions)
            trans->PAppend("INSERT INTO `character_loadout_action` (`guid`, `loadout_id`, `button`, `action`, `type`) VALUES (%u, %u, %u, %u, %u)",
                guid, id, uint32(action.Button), action.Action, uint32(action.Type));
        for (auto const& [part, partData] : loadout.Parts)
        {
            std::string partName = part;
            std::string value = partData;
            CharacterDatabase.EscapeString(partName);
            CharacterDatabase.EscapeString(value);
            trans->PAppend("INSERT INTO `character_loadout_part` (`guid`, `loadout_id`, `part`, `data`) VALUES (%u, %u, '%s', '%s')",
                guid, id, partName.c_str(), value.c_str());
        }
        data.Saved[id] = fingerprint;
        any = true;
    }

    if (any)
        CharacterDatabase.CommitTransaction(trans);
}

void LoadoutMgr::UnloadPlayer(Player* player)
{
    std::lock_guard<std::mutex> guard(_lock);
    _players.erase(player->GetGUID().GetCounter());
}

void LoadoutMgr::DeletePlayer(uint32 guid)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        _players.erase(guid);
    }
    for (char const* table : { "character_loadout", "character_loadout_talent", "character_loadout_glyph", "character_loadout_action",
        "character_loadout_part" })
        CharacterDatabase.PExecute("DELETE FROM `%s` WHERE `guid` = %u", table, guid);
}

Loadout* LoadoutMgr::AttachedTo(PlayerData& data, uint8 group)
{
    for (auto& [id, loadout] : data.Loadouts)
        if (loadout.Group == group)
            return &loadout;
    return nullptr;
}

Loadout* LoadoutMgr::DefaultFor(PlayerData& data, uint16 specId)
{
    for (auto& [id, loadout] : data.Loadouts)
        if (loadout.IsDefault && loadout.SpecId == specId)
            return &loadout;
    return nullptr;
}

void LoadoutMgr::CaptureGroup(Player* player, Loadout& loadout, uint8 group)
{
    loadout.SpecId = uint16(player->GetTalentSpecialization(group));

    loadout.Talents.clear();
    for (auto const& [spell, talent] : *player->GetTalentMap(group))
        if (talent->state != PLAYERSPELL_REMOVED)
            loadout.Talents.push_back(spell);
    std::sort(loadout.Talents.begin(), loadout.Talents.end());

    for (uint8 slot = 0; slot < GlyphSlots; ++slot)
        loadout.Glyphs[slot] = player->GetGlyph(group, slot);

    if (group == player->GetActiveSpec())
    {
        loadout.Actions.clear();
        for (auto const& [button, action] : player->GetActionButtons())
            if (action.uState != ACTIONBUTTON_DELETED)
                loadout.Actions.push_back({ button, action.GetAction(), uint8(action.GetType()) });
    }
    else
    {
        // The inactive group's bars live only in character_action; they were written when the group was last
        // switched away from, long before any loadout switch can run.
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS_SPEC);
        stmt->setUInt32(0, player->GetGUID().GetCounter());
        stmt->setUInt8(1, group);
        loadout.Actions.clear();
        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
            do
                loadout.Actions.push_back({ (*result)[0].GetUInt8(), (*result)[1].GetUInt32(), (*result)[2].GetUInt8() });
            while (result->NextRow());
    }

    for (Part const& part : _parts)
        loadout.Parts[part.Name] = part.Capture(player, loadout.SpecId);

    loadout.Captured = true;
}

void LoadoutMgr::AttachGroups(Player* player, PlayerData& data)
{
    for (uint8 group = 0; group < player->GetSpecsCount() && group < MAX_TALENT_SPECS; ++group)
    {
        uint16 spec = uint16(player->GetTalentSpecialization(group));
        Loadout* attached = AttachedTo(data, group);
        if (attached && attached->SpecId == spec && spec)
            continue;

        // The group changed spec outside a loadout switch (a talent reset, then a new spec in the client UI):
        // it now holds that spec's default loadout. A spec held by the other group already stays unattached here.
        if (attached)
            attached->Group = NoGroup;
        if (!spec)
            continue;
        Loadout* target = DefaultFor(data, spec);
        if (!target || target->Group != NoGroup)
            continue;
        target->Group = group;
        CaptureGroup(player, *target, group);
    }
}

void LoadoutMgr::EnsureDefaults(Player* player)
{
    if (!Applies(player))
        return;
    std::vector<uint16> specs = ClassSpecs(player);
    // Specializations unlock with the second talent group (T-016); a character that already picked one counts too.
    if (specs.empty() || (player->GetLevel() < sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL) && !player->GetTalentSpecialization(player->GetActiveSpec())))
        return;

    bool first = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _players.find(player->GetGUID().GetCounter());
        if (itr == _players.end())
            return;
        PlayerData& data = itr->second;
        first = data.Loadouts.empty();

        std::vector<uint16> defaults;
        std::vector<uint32> ids;
        for (auto const& [id, loadout] : data.Loadouts)
        {
            ids.push_back(id);
            if (loadout.IsDefault)
                defaults.push_back(loadout.SpecId);
        }

        for (uint16 spec : MissingDefaults(specs, defaults))
        {
            Loadout loadout;
            loadout.Id = NextId(ids);
            ids.push_back(loadout.Id);
            ChrSpecializationEntry const* entry = sChrSpecializationStore.LookupEntry(spec);
            loadout.Name = entry && entry->Name && *entry->Name ? entry->Name : "Spec " + std::to_string(spec);
            loadout.SpecId = spec;
            loadout.IsDefault = true;
            data.Loadouts[loadout.Id] = loadout;
        }

        AttachGroups(player, data);
    }

    if (first)
        ChatHandler(player->GetSession()).PSendSysMessage("|cff00ccffLoadouts:|r each specialization now keeps its own talents, "
            "glyphs, action bars and Heartseed choices. Type .loadout to list them and .loadout switch <name> to change, "
            "or open the Loadouts addon with /lo.");
}

void LoadoutMgr::OnSpecializationChanged(Player* player)
{
    if (!Applies(player) || IsSwitching(player->GetGUID().GetCounter()))
        return;
    EnsureDefaults(player);
}

void LoadoutMgr::CaptureActive(Player* player)
{
    if (!Applies(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return;
    Loadout* active = AttachedTo(itr->second, player->GetActiveSpec());
    if (active && active->SpecId == uint16(player->GetTalentSpecialization()))
        CaptureGroup(player, *active, player->GetActiveSpec());
}

std::vector<Loadout> LoadoutMgr::GetLoadouts(Player* player)
{
    std::vector<uint16> specs = ClassSpecs(player);
    std::vector<Loadout> loadouts;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _players.find(player->GetGUID().GetCounter());
        if (itr == _players.end())
            return loadouts;
        for (auto const& [id, loadout] : itr->second.Loadouts)
            loadouts.push_back(loadout);
    }
    auto specOrder = [&specs](uint16 spec)
    {
        auto pos = std::find(specs.begin(), specs.end(), spec);
        return std::size_t(pos - specs.begin());
    };
    std::stable_sort(loadouts.begin(), loadouts.end(), [&specOrder](Loadout const& a, Loadout const& b)
    {
        if (a.IsDefault != b.IsDefault)
            return a.IsDefault;
        if (a.IsDefault)
            return specOrder(a.SpecId) < specOrder(b.SpecId);
        return a.Id < b.Id;
    });
    return loadouts;
}

uint32 LoadoutMgr::GetActiveId(Player* player)
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return 0;
    Loadout* active = AttachedTo(itr->second, player->GetActiveSpec());
    return active ? active->Id : 0;
}

uint32 LoadoutMgr::Resolve(Player* player, std::string const& arg)
{
    std::vector<Entry> entries;
    for (Loadout const& loadout : GetLoadouts(player))
        entries.push_back({ loadout.Id, loadout.Name });
    return Loadouts::Resolve(arg, entries);
}

void LoadoutMgr::WriteActions(Player* player, Loadout const& loadout, uint8 group)
{
    uint32 guid = player->GetGUID().GetCounter();
    std::vector<ActionEntry> actions = loadout.Actions;
    if (!loadout.Captured)
    {
        // A loadout that never held bars starts from the current ones: class spells keep their places and spec
        // spells the new spec lacks show as empty buttons until moved.
        actions.clear();
        for (auto const& [button, action] : player->GetActionButtons())
            if (action.uState != ACTIONBUTTON_DELETED)
                actions.push_back({ button, action.GetAction(), uint8(action.GetType()) });
    }

    // Written straight away: ActivateSpec reads the group's bars from the database with a synchronous query.
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM `character_action` WHERE `guid` = %u AND `spec` = %u", guid, uint32(group));
    for (ActionEntry const& action : actions)
        if (action.Button < MAX_ACTION_BUTTONS)
            trans->PAppend("INSERT INTO `character_action` (`guid`, `spec`, `button`, `action`, `type`) VALUES (%u, %u, %u, %u, %u)",
                guid, uint32(group), uint32(action.Button), action.Action, uint32(action.Type));
    CharacterDatabase.DirectCommitTransaction(trans);
}

SwitchResult LoadoutMgr::Switch(Player* player, uint32 loadoutId)
{
    if (!Applies(player))
        return SwitchResult::Disabled;
    if (ClassSpecs(player).empty())
        return SwitchResult::NoSpecs;
    if (player->GetSpecsCount() < 2)
        return SwitchResult::NoSecondGroup;
    if (player->IsInCombat())
        return SwitchResult::InCombat;
    if (!player->IsAlive() || player->InArena())
        return SwitchResult::Busy;

    uint32 guid = player->GetGUID().GetCounter();
    uint8 active = player->GetActiveSpec();
    uint8 group = TargetGroup(active);
    Loadout target;
    bool rewrite = true;
    std::vector<Part> parts;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _players.find(guid);
        if (itr == _players.end())
            return SwitchResult::Disabled;
        PlayerData& data = itr->second;
        auto found = data.Loadouts.find(loadoutId);
        if (found == data.Loadouts.end())
            return SwitchResult::UnknownLoadout;
        if (found->second.Group == active)
            return SwitchResult::AlreadyActive;

        // Nothing the player changed since the last save may be lost: capture what both groups hold now.
        if (Loadout* current = AttachedTo(data, active))
            if (current->SpecId == uint16(player->GetTalentSpecialization(active)))
                CaptureGroup(player, *current, active);

        if (found->second.Group == group && found->second.SpecId == uint16(player->GetTalentSpecialization(group)))
            rewrite = false;    // the inactive group already holds it: a plain spec switch
        else
        {
            if (Loadout* evicted = AttachedTo(data, group))
            {
                if (evicted->SpecId == uint16(player->GetTalentSpecialization(group)))
                    CaptureGroup(player, *evicted, group);
                evicted->Group = NoGroup;
            }
            found->second.Group = group;
        }
        target = found->second;
        parts = _parts;
    }

    SwitchGuard switchGuard(guid);

    if (rewrite)
    {
        player->SetTalentSpecialization(group, Specializations(target.SpecId));

        // Replace the inactive group's talents. Its rows go now; the new ones are saved as new talents.
        CharacterDatabase.DirectPExecute("DELETE FROM `character_talent` WHERE `guid` = %u AND `spec` = %u", guid, uint32(group));
        PlayerTalentMap* talents = player->GetTalentMap(group);
        for (auto& [spell, talent] : *talents)
            delete talent;
        talents->clear();
        for (uint32 spell : target.Talents)
            player->AddTalent(spell, group, true);

        WriteActions(player, target, group);
    }

    // Parts first, so systems that react to the spec change (the Heartseed's traits) already see the new values.
    for (Part const& part : parts)
    {
        auto data = target.Parts.find(part.Name);
        if (data != target.Parts.end())
            part.Apply(player, target.SpecId, data->second);
    }

    player->ActivateSpec(group);

    if (rewrite)
        for (uint8 slot = 0; slot < GlyphSlots; ++slot)
            if (player->GetGlyph(group, slot) != target.Glyphs[slot] && (!target.Glyphs[slot] || player->GetGlyphSlot(slot)))
                player->SetGlyph(slot, target.Glyphs[slot]);

    // ActivateSpec leaves the visible spec id and the talent frame to the caller.
    player->SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID, target.SpecId);
    player->SendTalentsInfoData();

    TC_LOG_DEBUG("scripts", "Loadouts: %s switched to loadout %u '%s' (spec %u, group %u%s)", player->GetName().c_str(), target.Id,
        target.Name.c_str(), uint32(target.SpecId), uint32(group), rewrite ? ", rewritten" : "");
    return SwitchResult::Ok;
}

EditResult LoadoutMgr::Create(Player* player, std::string const& name, uint32& newId)
{
    if (!ValidName(name))
        return EditResult::BadName;
    CaptureActive(player);

    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return EditResult::NoSpecs;
    PlayerData& data = itr->second;
    Loadout* active = AttachedTo(data, player->GetActiveSpec());
    if (!active)
        return EditResult::NoSpecs;

    uint32 named = 0;
    std::vector<uint32> ids;
    for (auto const& [id, loadout] : data.Loadouts)
    {
        if (SameName(loadout.Name, name))
            return EditResult::NameTaken;
        ids.push_back(id);
        named += loadout.IsDefault ? 0 : 1;
    }
    if (named >= MaxNamed())
        return EditResult::TooMany;

    Loadout copy = *active;
    copy.Id = NextId(ids);
    copy.Name = name;
    copy.IsDefault = false;
    copy.Group = NoGroup;
    data.Loadouts[copy.Id] = copy;
    newId = copy.Id;
    return EditResult::Ok;
}

EditResult LoadoutMgr::Rename(Player* player, uint32 loadoutId, std::string const& name)
{
    if (!ValidName(name))
        return EditResult::BadName;
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return EditResult::UnknownLoadout;
    auto found = itr->second.Loadouts.find(loadoutId);
    if (found == itr->second.Loadouts.end())
        return EditResult::UnknownLoadout;
    for (auto const& [id, loadout] : itr->second.Loadouts)
        if (id != loadoutId && SameName(loadout.Name, name))
            return EditResult::NameTaken;
    found->second.Name = name;
    return EditResult::Ok;
}

EditResult LoadoutMgr::Delete(Player* player, uint32 loadoutId)
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return EditResult::UnknownLoadout;
    auto found = itr->second.Loadouts.find(loadoutId);
    if (found == itr->second.Loadouts.end())
        return EditResult::UnknownLoadout;
    if (found->second.IsDefault)
        return EditResult::IsDefault;
    // A talent group still holds it; deleting it would leave that group's talents without a loadout.
    if (found->second.Group != NoGroup)
        return EditResult::IsActive;
    itr->second.Deleted.push_back(loadoutId);
    itr->second.Loadouts.erase(found);
    return EditResult::Ok;
}

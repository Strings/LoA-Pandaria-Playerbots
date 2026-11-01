/*
 * Spec loadouts (T-017). The 5.4.8 client knows exactly two talent groups; the server keeps any number of
 * loadouts per character (one per specialization by default, plus named extras). A loadout holds the spec,
 * talents, glyphs, action bars and whatever registered parts add (the Heartseed's majors; gear pages in T-018).
 * Switching captures the active group into its loadout, writes the chosen loadout into the inactive group and
 * runs the normal spec switch. See README.md.
 */

#ifndef ULTIMATEMOP_LOADOUT_MGR_H
#define ULTIMATEMOP_LOADOUT_MGR_H

#include "Define.h"
#include "LoadoutRules.h"

#include <array>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace Loadouts
{
    constexpr uint8 GlyphSlots = 6;                    // MAX_GLYPH_SLOT_INDEX

    struct ActionEntry
    {
        uint8 Button = 0;
        uint32 Action = 0;
        uint8 Type = 0;
    };

    struct Loadout
    {
        uint32 Id = 0;
        std::string Name;
        uint16 SpecId = 0;
        bool IsDefault = false;                        // the automatic one for its spec; can't be deleted
        uint8 Group = NoGroup;                         // talent group holding it, or NoGroup
        bool Captured = false;                         // false until it has held real talents and bars
        std::vector<uint32> Talents;                   // talent spell ids
        std::array<uint32, GlyphSlots> Glyphs = { };
        std::vector<ActionEntry> Actions;
        std::map<std::string, std::string> Parts;      // part name -> that part's saved data
    };

    // Another system's state that rides along with a loadout. Capture reads it for a spec; Apply writes it back
    // before the spec switch, so that system's OnSpecializationChanged handling sees the new values.
    struct Part
    {
        std::string Name;
        std::function<std::string(Player* player, uint16 specId)> Capture;
        std::function<void(Player* player, uint16 specId, std::string const& data)> Apply;
    };

    enum class SwitchResult
    {
        Ok,
        AlreadyActive,
        UnknownLoadout,
        Disabled,
        NoSpecs,             // below specialization level
        NoSecondGroup,       // dual specialization not learned
        InCombat,
        Busy,                // dead, in an arena or casting
    };

    enum class EditResult
    {
        Ok,
        UnknownLoadout,
        BadName,
        NameTaken,
        TooMany,
        IsDefault,
        IsActive,
        NoSpecs,
    };
}

class LoadoutMgr
{
public:
    static LoadoutMgr* instance();

    void RegisterPart(Loadouts::Part part);

    void LoadPlayer(Player* player);
    void SavePlayer(Player* player);                   // captures the active group, then writes changed loadouts
    void UnloadPlayer(Player* player);
    void DeletePlayer(uint32 guid);

    // Creates the missing default loadouts and attaches the talent groups to theirs. Safe to call at any time.
    void EnsureDefaults(Player* player);
    // After a spec change through the client's own talent UI: re-attaches the groups and captures them.
    void OnSpecializationChanged(Player* player);

    std::vector<Loadouts::Loadout> GetLoadouts(Player* player);   // ordered: defaults by spec order, then by id
    uint32 GetActiveId(Player* player);
    uint32 Resolve(Player* player, std::string const& arg);

    Loadouts::SwitchResult Switch(Player* player, uint32 loadoutId);
    Loadouts::EditResult Create(Player* player, std::string const& name, uint32& newId);   // copy of the active one
    Loadouts::EditResult Rename(Player* player, uint32 loadoutId, std::string const& name);
    Loadouts::EditResult Delete(Player* player, uint32 loadoutId);
    void CaptureActive(Player* player);

    bool Enabled() const;
    uint32 MaxNamed() const;

private:
    LoadoutMgr() = default;

    struct PlayerData
    {
        std::map<uint32, Loadouts::Loadout> Loadouts;
        std::map<uint32, std::string> Saved;           // id -> fingerprint last written, to skip unchanged writes
        std::vector<uint32> Deleted;
    };

    // Callers hold _lock.
    Loadouts::Loadout* AttachedTo(PlayerData& data, uint8 group);
    Loadouts::Loadout* DefaultFor(PlayerData& data, uint16 specId);
    void AttachGroups(Player* player, PlayerData& data);
    void CaptureGroup(Player* player, Loadouts::Loadout& loadout, uint8 group);
    void WriteActions(Player* player, Loadouts::Loadout const& loadout, uint8 group);
    std::vector<uint16> ClassSpecs(Player* player) const;

    static std::string Fingerprint(Loadouts::Loadout const& loadout);

    mutable std::mutex _lock;
    std::unordered_map<uint32, PlayerData> _players;   // guid counter -> loadouts
    std::vector<Loadouts::Part> _parts;
};

#define sLoadoutMgr LoadoutMgr::instance()

#endif

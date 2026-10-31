/*
 * GM test suite (T-048): checkpoint templates and kits. A checkpoint is an ordered list of typed steps (level,
 * gear, spells, Heartseed state, quests, reputations, location) applied to one character in one command; a kit
 * is a list of items. Both are world-database rows and reload without a restart. See README.md.
 */

#ifndef ULTIMATEMOP_CHECKPOINT_MGR_H
#define ULTIMATEMOP_CHECKPOINT_MGR_H

#include "Define.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class ChatHandler;
class Player;

namespace Checkpoints
{
    struct Step
    {
        uint8 Order = 0;
        std::string Type;
        int32 Arg1 = 0;
        int32 Arg2 = 0;
        std::string Text;
        std::string Comment;
    };

    struct Checkpoint
    {
        std::string Name;
        std::string Category;
        std::string Description;
        uint32 SortOrder = 0;
        std::vector<Step> Steps;
    };

    struct KitItem
    {
        uint32 Entry = 0;
        uint32 Count = 1;
        uint32 ClassMask = 0;               // 0 = every class
        bool Equip = false;
    };

    struct Kit
    {
        std::string Name;
        std::string Description;
        uint8 MinLevel = 0;
        uint8 MaxLevel = 0;
        std::vector<KitItem> Items;
    };

    // What a step does to its target. handler is the GM's; target is the character the checkpoint is applied to.
    // Returns false when the step failed; the message says why.
    using StepHandler = std::function<bool(ChatHandler* handler, Player* target, Step const& step, std::string& message)>;
    // Plain-language line for .checkpoint explain.
    using StepExplainer = std::function<std::string(Step const& step)>;
}

class CheckpointMgr
{
public:
    static CheckpointMgr* instance();

    // Each system registers its own step types, so later systems add fields to the template without schema changes.
    void RegisterStep(std::string const& type, Checkpoints::StepHandler handler, Checkpoints::StepExplainer explainer);
    bool IsKnownStep(std::string const& type) const;

    void LoadData();                         // world: gmtools_checkpoint, gmtools_checkpoint_step, gmtools_kit, gmtools_kit_item

    std::vector<Checkpoints::Checkpoint> GetCheckpoints() const;     // by sort order
    bool GetCheckpoint(std::string const& name, Checkpoints::Checkpoint& out) const;
    std::vector<Checkpoints::Kit> GetKits() const;
    bool GetKit(std::string const& name, Checkpoints::Kit& out) const;

    // Runs every step in order and reports each one to the handler. Returns the number of failed steps.
    uint32 Apply(ChatHandler* handler, Player* target, Checkpoints::Checkpoint const& checkpoint);
    std::vector<std::string> Explain(Checkpoints::Checkpoint const& checkpoint) const;
    // Adds a kit's items for the target's class: equips those marked equip, the rest go to the bags.
    uint32 ApplyKit(Player* target, Checkpoints::Kit const& kit, std::string& message);

private:
    CheckpointMgr();

    struct StepType
    {
        Checkpoints::StepHandler Handler;
        Checkpoints::StepExplainer Explainer;
    };

    mutable std::mutex _lock;
    std::map<std::string, StepType> _stepTypes;
    std::map<std::string, Checkpoints::Checkpoint> _checkpoints;   // lower-case name -> checkpoint
    std::map<std::string, Checkpoints::Kit> _kits;                 // lower-case name -> kit
};

#define sCheckpointMgr CheckpointMgr::instance()

#endif

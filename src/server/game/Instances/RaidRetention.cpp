#include "RaidRetention.h"
#include "RaidResetPolicy.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "DB2Stores.h"
#include "Log.h"
#include "InstanceScript.h"
#include "InstanceSaveMgr.h"
#include "Map.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Random.h"
#include <mutex>
#include <atomic>
#include <unordered_map>
namespace
{
    std::atomic<bool> enabled{ true };
    std::atomic<uint32> period{ 172800 };
    std::unordered_map<uint32, uint32> masks;
    std::mutex masksLock;
}
void RaidRetention::LoadConfig()
{
    enabled.store(sConfigMgr->GetBoolDefault("Raid.Retention.Enable", true), std::memory_order_relaxed);
    int32 value = sConfigMgr->GetIntDefault("Raid.ResetPeriodSeconds", 172800);
    if (value <= 0)
        TC_LOG_ERROR("server.loading", "Raid.ResetPeriodSeconds must be positive; using 172800.");
    period.store(RaidResetPolicy::Period(value), std::memory_order_relaxed);
}
bool RaidRetention::Enabled()
{
    return enabled.load(std::memory_order_relaxed);
}
uint32 RaidRetention::PeriodSeconds()
{
    return period.load(std::memory_order_relaxed);
}
void RaidRetention::LoadEncounterMasks()
{
    std::unordered_map<uint32, uint32> next;
    // The validated ObjectMgr encounter cache loads later than instance cleanup.
    // Mirror its difficulty expansion here using the same world rows and DBC indices.
    if (QueryResult result = WorldDatabase.Query("SELECT entry, difficulty+0 FROM instance_encounters"))
        do
        {
            Field* fields = result->Fetch();
            DungeonEncounterEntry const* encounter = sDungeonEncounterStore.LookupEntry(fields[0].GetUInt32());
            if (!encounter || encounter->encounterIndex >= 32)
                continue;
            MapEntry const* map = sMapStore.LookupEntry(encounter->mapId);
            if (!map || !map->IsRaid())
                continue;
            uint32 difficulty = fields[1].GetUInt32();
            if (difficulty)
                next[MAKE_PAIR32(encounter->mapId, difficulty)] |= uint32(1) << encounter->encounterIndex;
            else
                for (uint32 mode = 0; mode < MAX_DIFFICULTY; ++mode)
                    if (sDBCManager.GetMapDifficultyData(encounter->mapId, Difficulty(mode)))
                        next[MAKE_PAIR32(encounter->mapId, mode)] |= uint32(1) << encounter->encounterIndex;
        } while (result->NextRow());
    std::lock_guard<std::mutex> lock(masksLock);
    masks.swap(next);
}
uint32 RaidRetention::ExpectedMask(uint32 mapId, Difficulty difficulty)
{
    std::lock_guard<std::mutex> lock(masksLock);
    auto it = masks.find(MAKE_PAIR32(mapId, difficulty));
    return it == masks.end() ? 0 : it->second;
}
void RaidRetention::DeleteStoredInstance(uint32 instanceId)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    for (auto statement : { CHAR_DEL_INSTANCE_BY_INSTANCE, CHAR_DEL_CHAR_INSTANCE_BY_INSTANCE, CHAR_DEL_GROUP_INSTANCE_BY_INSTANCE })
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(statement);
        stmt->setUInt32(0, instanceId);
        trans->Append(stmt);
    }
    CharacterDatabase.DirectCommitTransaction(trans);
}

void RaidRetention::RefreshCycleData(uint32 mapId, time_t next)
{
    if (mapId != 1008)
        return;
    uint32 previous = 0;
    if (QueryResult row = WorldDatabase.Query("SELECT resettime FROM instance_mogushan_system ORDER BY id DESC LIMIT 1"))
        previous = (*row)[0].GetUInt32();
    if (previous >= uint32(next))
        return;
    uint32 const guards[] = { 59915, 60043, 60047, 60051 };
    WorldDatabase.DirectPExecute("INSERT INTO instance_mogushan_system (creature_id,resettime) VALUES (%u,%u)", guards[urand(0, 3)], uint32(next));
}

void RaidRetention::SaveProgress(Map* map, InstanceScript* script, bool wingFinished)
{
    uint32 wing = 0;
    if (map->ToInstanceMap()->IsLFGMap())
        if (Group* group = map->GetInstanceGroup())
            if (group->isLFGGroup())
                wing = sLFGMgr->GetDungeon(group->GetGUID());
    uint32 bossMask = 0;
    uint32 count = script->GetEncounterCount();
    for (uint32 i = 0; i < count && i < 32; ++i)
        if (script->GetBossState(i) == DONE)
            bossMask |= uint32(1) << i;
    uint32 expectedBosses = RaidResetPolicy::ScriptMask(map->GetId(), count, map->IsHeroic(), wing);
    uint32 completed = script->GetCompletedEncounterMask();
    bool finished = wingFinished || RaidResetPolicy::Complete(bossMask, expectedBosses, false) ||
        RaidResetPolicy::Complete(completed, ExpectedMask(map->GetId(), map->GetDifficulty()), false);
    bool progress = completed || (bossMask & expectedBosses);
    time_t deadline = sInstanceSaveMgr->GetResetTimeFor(map->GetId(), map->GetDifficulty());
    if (!deadline)
        deadline = time(NULL) + PeriodSeconds();
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* data = CharacterDatabase.GetPreparedStatement(CHAR_UPD_INSTANCE_DATA);
    data->setUInt32(0, completed);
    data->setString(1, script->GetSaveData());
    data->setUInt32(2, map->GetInstanceId());
    trans->Append(data);
    // Keep wing completion sticky when a later script save has no LFG context.
    // The reserved partial marker leaves canonical encounter bits untouched.
    trans->PAppend("UPDATE instance SET resettime=IF(resettime NOT IN (0,%u) OR %u,%u,%u) WHERE id=%u",
        RaidResetPolicy::PartialMarker, uint32(finished), uint32(deadline), progress ? RaidResetPolicy::PartialMarker : 0, map->GetInstanceId());
    CharacterDatabase.DirectCommitTransaction(trans);
}

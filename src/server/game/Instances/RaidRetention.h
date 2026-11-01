#ifndef ULTIMATEMOP_RAID_RETENTION_H
#define ULTIMATEMOP_RAID_RETENTION_H
#include "Define.h"
#include "DBCEnums.h"
#include <ctime>
namespace RaidRetention
{
    void LoadConfig();
    bool Enabled();
    uint32 PeriodSeconds();
    void LoadEncounterMasks();
    void RefreshCycleData(uint32 mapId, time_t next);
    uint32 ExpectedMask(uint32 mapId, Difficulty difficulty);
    void DeleteStoredInstance(uint32 instanceId);
}
#endif

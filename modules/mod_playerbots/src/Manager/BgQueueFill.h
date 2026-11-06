/*
 * Battleground queue fill (beyond AC): when a real player waits in a battleground queue, queue enough random bots of
 * that bracket, both factions, for the match to pop at once.
 *
 * The stock path (CheckBgQueue census every AiPlayerbot.BgCheckInterval s, then each bot's own "bg join" on its random
 * "often" trigger, after 120 s in world) takes from half a minute to several minutes and only ever draws on the bots
 * that happen to be online in that bracket, which on a 500-bot server is often fewer than a match needs. This module
 * runs on the world thread every AiPlayerbot.BgFill.CheckIntervalMs, reads every real player's queue entries, and for
 * each (queue, bracket) a real player is waiting in:
 *   1. queues free online random bots of that bracket directly (the tail of BGJoinAction::JoinQueue), per faction, up
 *      to AiPlayerbot.BgFill.TeamSize (0 = the battleground's max per team);
 *   2. if a faction is still short, logs in offline random bots of that bracket and faction (at most
 *      AiPlayerbot.BgFill.MaxExtraLogins online at once), which step 1 queues on a later pass once they are in world;
 *   3. logs those extra bots out again AiPlayerbot.BgFill.ExtraBotStaySeconds after they were last in a battleground
 *      or queue, so the online bot count returns to MaxRandomBots.
 * With AiPlayerbot.BgFill.FastAccept the bots it queued accept their invitation on the next pass instead of waiting
 * for their own "bg status" trigger.
 * The core pops a match at the minimum per team; while bots it queued still wait, the fill schedules a queue update
 * every pass so the match is topped up to the maximum, and after AiPlayerbot.BgFill.QueueTimeoutSeconds with no real
 * player waiting they leave the queue.
 *
 * GM: .playerbots bgfill [status|now] (cs_playerbots.cpp).
 */

#ifndef _PLAYERBOT_BGQUEUEFILL_H
#define _PLAYERBOT_BGQUEUEFILL_H

#include "Define.h"

#include <string>

namespace BgQueueFill
{
    // World thread (PlayerbotsScript::OnPlayerbotUpdate), every world tick; self-throttled.
    void Update(uint32 diff);

    // Run a pass on the next world tick, ignoring CheckIntervalMs and DelaySeconds once.
    void ForceNextPass();

    // One line per (queue, bracket) a real player waits in, plus the extra-login census.
    std::string Status();
}

#endif

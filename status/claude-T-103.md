# T-103: Battleground bot fill; wild rares 2.5%

**Agent:** claude
**Branch:** claude/bg-bot-fill
**Task status:** review

## What changed
- New `modules/mod_playerbots/src/Manager/BgQueueFill.{h,cpp}`, run every world tick from
  `PlayerbotsScript::OnPlayerbotUpdate` (before the map update and `BattlegroundMgr::Update`). Each
  `AiPlayerbot.BgFill.CheckIntervalMs` (1 s) it reads the battleground queues directly. For each (queue, bracket) a
  real player waits in (not invited), it queues free random bots of that bracket for both factions, up to
  `AiPlayerbot.BgFill.TeamSize` (0 = the battleground's max per team). It uses the same tail as `BGJoinAction::JoinQueue`,
  so the queue pops in the same tick.
- Too few free bots online in a bracket: it logs in offline random bots of that bracket and faction
  (`LoginOffline`, at most `MaxExtraLogins` = 60 on top of MaxRandomBots). Their randomize and teleport are held so
  they keep their level. They are released (`add` = 0, the manager logs them out) `ExtraBotStaySeconds` (300) after
  they were last in a battleground, a queue or a group.
- `FastAccept`: bots it queued accept their invite on the next pass (`bg status` action), not on their own trigger.
- GM: `.playerbots bgfill [status|now]`.
- Wild rare chance `WildRares.ChancePct` 2.0 -> 2.5 (conf.dist, code default, README, DESIGN.md).

## How it was tested
- See the PR: Claude test server with 40 random bots online, a level-57 test character queued Warsong Gulch through the
  headless client, and the time from queue to invite was measured. `.playerbots bgfill status` was checked.

## Decisions made
- Bot pool per bracket on the main server (500 online, levels 1-90): roughly 3-30 online per faction per bracket,
  fewer free. That is short of Arathi Basin / Eye of the Storm minimums (8) in several brackets and of 85-89 even for
  Warsong Gulch, so offline bracket bots are logged in on demand rather than raising MaxRandomBots. The total bot
  count stays the same; at most 60 extra are online while a queue is waiting or for 5 minutes after.
- Bots are only taken when ungrouped, out of combat, not in LFG, a queue or a battleground, and not held by a
  `.botscenario`.

## For the other agent
- New config keys `AiPlayerbot.BgFill.*` in `playerbots.conf.dist` (main `playerbots.conf` does not need them; the code
  defaults match).
- On by default; `AiPlayerbot.BgFill.Enable = 0` restores the old census-only behaviour.

## Next
- Watch the human's first solo BG queues: if a whole bracket has no offline bots of a faction (bracket 85-89 has
  11 Horde total), consider a level-bracket distribution change; that is a design call.

## Map additions
- `modules/mod_playerbots/src/Manager/BgQueueFill.*`: battleground queue fill for real players.

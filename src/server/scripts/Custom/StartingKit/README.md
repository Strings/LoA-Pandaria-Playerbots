# Starting kit

First login grants Apprentice Riding (75) and a basic racial ground mount, plus
Heavy Brown Bags (4497, ten slots) in empty bag slots 19–22. Existing bags and
higher riding skills are preserved. All three Pandaren races use the Green
Dragon Turtle, so neutral characters need no faction selection first.

`StartingKit.Mount.Enable` and `StartingKit.Bags.Enable` default to 1. Config
reload affects subsequent first logins; existing characters are not backfilled.
The core owns the durable AT_LOGIN_FIRST flag; the new C++ first-login callback
runs before it is cleared. The legacy, unregistered PlayerStartScript stays off.

WorldSession::IsBot is set before playerbot login, so this callback skips bot
sessions entirely. BotFactory keeps its own riding/mount and bag initialization;
no bot settings, gear, or initialization order changes. Human-controlled alt
characters receive the normal kit when their first login is a human session.

GM `.startingkit` inspects the selected player (otherwise self): config values,
bot/race, riding, racial spell, and all four bag slots. A short first-login
notification introduces the kit. No SQL or timers are needed.

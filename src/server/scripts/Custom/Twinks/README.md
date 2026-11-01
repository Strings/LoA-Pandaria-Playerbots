# Free experience toggle (T-044)

`.xp on|off|status` is free and usable anywhere, defaults to status, and changes only the caller for ordinary players. Permission 95000 is linked to Player Commands (199). Permission 95001, linked to GM Commands (197), allows the same command to force/inspect a selected player; no selection uses self.

`Twinks.FreeXPToggle=1` enables changes. `Twinks.IntroOnLogin=1` gives humans a short introduction. Disabling the feature rejects command changes, preserves the chosen no-XP flag, and restores the existing paid Experience Eliminator path and greeting. Inspection remains available. No duration is added.

`Twinks::SetExperienceEnabled(Player*,bool)` is the reusable adapter for the future settings panel and the existing NPC. It changes only the native persistent PLAYER_FLAGS_NO_XP_GAIN bit, saves changed state, and never modifies money. Repeated requests are no-ops. NPC greeting 95000 explains the free service without the native fee text.

Audit: Player::GiveXP checks PLAYER_FLAGS_NO_XP_GAIN. BattlegroundQueue, Group and battleground join handlers do not inspect that flag or carry an XP-lock queue dimension; players and bots already share queues. No core-game queue change or meaningless config switch is added.

Pure command parsing tests are in tests/twink_xp. Before ready: apply the two scoped SQL files to owned DBs; test ordinary/GM permissions, unchanged money, relog persistence, an actual XP reward with on/off, NPC free/disabled paid behavior, and the three headless regressions. Default build and actual results remain pending in the status fragment.

# Free experience toggle (T-044)

Planned interface: player `.xp on|off|status`, self only. A separate GM permission allows selected-player force/inspection. All changes use the existing persistent no-XP flag and never charge money. Existing Experience Eliminator gossip uses the same free adapter when enabled, preserving its native paid path when disabled.

Audit: Player::GiveXP checks PLAYER_FLAGS_NO_XP_GAIN. Neither BattlegroundQueue, Group nor the battleground join handlers use that flag or carry an XP-lock queue dimension; no queue core change is needed.

Production configuration, commands and actual test results will be added before ready. No new duration.

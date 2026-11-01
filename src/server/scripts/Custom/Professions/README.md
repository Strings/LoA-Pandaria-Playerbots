# All professions

The existing `MaxPrimaryTradeSkill` setting ships as 11. Trainers still enforce
their normal level, gold and skill requirements; profession perks stack. The
upstream fallback remains two if that config line is omitted.

Login/config reload recomputes remaining slots from the eleven primary skill
lines. Lowering the cap never removes a skill, and free slots cannot underflow.
The client still has two native profession slots; the optional ProfessionBar
addon opens known extras. `Professions.IntroOnLogin` controls the brief notice.
No timers or SQL are introduced.

GM `.professions status` inspects selected player/self; `.professions learnall`
forces the eleven apprentice skills using the normal learning API when the
configured cap is at least 11. Existing higher skills are preserved. Both reuse
the existing additem permission. Use a dedicated fixture for the force command.

`ProfessionRules.h` contains the server-independent skill IDs and free-slot
policy, with unit coverage. Addon rank chains are verified against spell_ranks.

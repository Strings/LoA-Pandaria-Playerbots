# Solo quality of life (T-041 script half)

- `SoloQoL.GuildPerksForAll=1`: grant native guild perks regardless of guild level/membership. Leaving/joining queues reconciliation after native guild work completes, on the next world update; no poll timer. Disabling restores the native level/membership perk set on login/reload.
- Existing Transmogrification.AllowMixedArmorTypes/AllowMixedWeaponTypes and legendary/requirement options already ship enabled. This module inspects them; it adds no duplicate transmog implementation.
- `SoloQoL.CharacterServices=1`: city stewards offer free rename, customization, race and faction service flags. Log out to complete through the normal selection screen. One pending service replaces other service flags, preserving unrelated flags. Bots, neutral Pandaren and combat are excluded.
- `SoloQoL.ShortHearthstone=1`, `SoloQoL.HearthstoneCooldownSeconds=300`: the normal Hearthstone spell's AfterCast replaces spell/category cooldown using existing SpellHistory APIs and sends the client cooldown. Read each cast; explicit final duration avoids a flat Hasty Hearth discount underflowing short profiles. Zero clears it; negative leaves native behavior. Other spells are not rebound.
- `SoloQoL.ServicesRespawnSeconds=300`: NPC delay read at AI spawn/reset/corpse removal. Existing pending timers are not rewritten. Core corpse decay uses the existing core config.
- `SoloQoL.IntroOnLogin=1`: brief human intro. NPC gossip also explains services.

GM `.soloqol status`, `perks` (reconcile), `hearth` (force configured cooldown), `hearthreset`, `service rename|customize|race|faction` use selected player/self. The reserved 94000 permission is linked to the GM role. `.npc add 904000`/904001 can force the service NPC for an owned test.

Services were prototyped first in `tools/soloqol/services_prototype.lua`, with Lua VM tests. It is GM-only and explicitly loaded, never automatically loaded alongside production C++. Pure policies are in QoLPolicy.h. Flash's stack data is excluded.

The Lua prototype exercises single choices. Production C++ also replaces prior service flags: Eluna exposes SetAtLoginFlag (adds bits), but no removal counterpart in this fork. The Lua VM mock models that additive behavior.

# T-019 config audit

All settings below exist and are read by this fork's World::LoadConfigSettings.
The shipped defaults and optional T-002 dev/test overlay match. Existing private
runtimes can run `python tools/devenv/apply_solo.py build/devenv`, then `reload config`
over SOAP. Log in again for login-initialized features such as all taxi paths.

| Setting | Solo default | Effect |
|---|---|---|
| AllFlightPaths | 1 | CharacterHandler enables taxi-cheat access on login, including existing characters |
| AllowTwoSide.Accounts / WhoList / AddFriend | 1 | Mixed accounts, who visibility and friends |
| AllowTwoSide.Interaction.Calendar / Channel / Group / Guild / Auction / Mail / Chat / DungeonFinder | 1 | Existing cross-faction social/group/economy/LFG hooks |
| AllowTwoSide.Trade | 1 | Existing cross-faction trade hook |
| SkillChance.Orange / Yellow / Green / Grey | 100 | Guaranteed crafting and configured gathering skill rolls below learned cap |
| SkillChance.Prospecting / Milling | 1 | Enable those existing skill-up paths |
| SkillChance.MiningSteps / SkinningSteps | 0 | No configured step-based chance decay (mining key is loaded but mining currently uses SkillGainChance directly) |
| DurabilityLoss.OnDeath / InPvP | 0 | No ordinary death/PvP death durability loss |
| Death.SicknessLevel | 91 | Above MoP's level-90 cap; disables resurrection sickness |
| Death.CorpseReclaimDelay.PvE / PvP | 0 | No escalation; PvE returns zero, PvP retains core base delay |

This does not change combat wear or the spirit healer's separate hardcoded 25%
durability charge. Disabling reclaim escalation is not a claim of zero PvP delay.
Skill training/rank limits, reagents and specialization eligibility remain.

## Parts without working config switches

**Weekly currency caps:** Player::GetCurrencyWeekCap reads CurrencyTypes.dbc;
Conquest meta-currency caps are derived from arena/rated-BG rating calculators.
Currency.ConquestPointsWeekCap exists, but World rejects zero, and its loaded
enum has no consumers outside the loader/header. Raising it or setting zero
cannot remove Valor/Conquest limits. No unsupported DisableWeeklyCaps key is
written and the legacy value stays at 1650. Total holding caps are a separate
rule and are unchanged.

**Fishing:** Player::UpdateFishingSkill hardcodes chance 100 below skill 75,
otherwise 10000/(skill+25). It does not read SkillChance.*. Guaranteed fishing
therefore needs a core hook too. Crafting/gathering settings do not claim to
override this formula, discovery rolls or other special skill systems.

Those two pieces are pending authorization for a core/config extension; compiled
validation must wait for Claude's build tooling as instructed. Core hooks would
need source tests plus real currency/fishing scenarios after a build. The config-
only changes can be reviewed/applied independently; T-019 is not marked complete.

Source evidence: World.cpp config loading; CharacterHandler.cpp all-taxi login;
Player.cpp SkillGainChance, UpdateCraftSkill, UpdateGatherSkill, UpdateFishingSkill,
GetCurrencyWeekCap, GetCorpseReclaimDelay and spirit-healer resurrection handling.

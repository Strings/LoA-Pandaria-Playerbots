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
| SkillChance.Fishing.Always | 1 | New cached core hook: successful catches always attempt a skill-up below the learned cap |
| Currency.WeeklyCaps.Enable | 0 | New cached core hook: no DBC/rating weekly earning limits, including Conquest meta-currency awards |
| SkillChance.Prospecting / Milling | 1 | Enable those existing skill-up paths |
| SkillChance.MiningSteps / SkinningSteps | 0 | No configured step-based chance decay (mining key is loaded but mining currently uses SkillGainChance directly) |
| DurabilityLoss.OnDeath / InPvP | 0 | No ordinary death/PvP death durability loss |
| Death.SicknessLevel | 91 | Above MoP's level-90 cap; disables resurrection sickness |
| Death.CorpseReclaimDelay.PvE / PvP | 0 | No escalation; PvE returns zero, PvP retains core base delay |

This does not change combat wear or the spirit healer's separate hardcoded 25%
durability charge. Disabling reclaim escalation is not a claim of zero PvP delay.
Skill training/rank limits, reagents and specialization eligibility remain.

## Core switches

The user authorized these hooks on 2026-10-31. World::LoadConfigSettings caches
both bools, including on reload. Shipped defaults and dev/test overlays select
uncapped currency and guaranteed fishing. Existing configs that omit the keys
retain stock behavior: weekly enforcement true and guaranteed fishing false.

**Currency.WeeklyCaps.Enable = 0** makes GetCurrencyWeekCap return the existing
zero/unlimited sentinel before DBC/rating cap calculations. Both clamp sites in
ModifyCurrencyMetaConquest now respect that sentinel too, so arena/rated-BG
rewards do not become zero or negative when uncapped. Weekly/season accounting,
weekly resets and total holding limits remain. Client cap packets use the same
getter; relog to refresh all displays after changing the setting. Setting 1
restores stock DBC/rating cap enforcement.

Valor of the Ancients keeps the DBC weekly threshold as a bonus milestone,
independent of cap enforcement; earning past it remains allowed. Existing bonus
flags are not cleared. The legacy Currency.ConquestPointsWeekCap key remains at
1650: its loaded enum has no consumers beyond loader/header, and zero is rejected.

**SkillChance.Fishing.Always = 1** supplies the existing 100% skill-up chance to
UpdateSkillPro after a successful catch. Learned rank cap and normal gain amount
remain enforced there. Setting 0 restores the exact original formula: 100 below
skill 75, otherwise integer 10000/(skill+25). Discovery rolls and special skill
systems are separate.

## Verification

Run `python tools/devenv/test_solo.py`, the standalone CTest unit suite, and
`python tests/regression/run.py --runtime build/devenv --suite tools/devenv/solo_smoke.json`
with GM fixture credentials. The live smoke checks each effective setting, taxi
access after login, PvE/PvP reclaim delays, and the weekly-cap getter. It checks
configuration and these direct probes; it does not simulate every social feature.

Use `.solo fishskill` below/at the learned cap and `.solo award` on disposable
currency fixtures for the new hooks. Check weekly caps both disabled and enabled,
including re-enabling while weekly earnings already exceed the cap: a reward must
never deduct currency. `tests/solo_config` covers the cap arithmetic, uncapped
awards, partial awards, deductions, and integer boundaries. Total holding limits
and existing Valor bonus thresholds stay in place.

Source evidence: World.cpp config loading; CharacterHandler.cpp all-taxi login;
Player.cpp SkillGainChance, UpdateCraftSkill, UpdateGatherSkill, UpdateFishingSkill,
GetCurrencyWeekCap, GetCorpseReclaimDelay and spirit-healer resurrection handling.

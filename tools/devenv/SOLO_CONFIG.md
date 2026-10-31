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

## New core switches (compiled validation deferred)

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

## Validation boundary and post-build checks

The two Python checks verify loaded keys/defaults/profile consistency and byte-
idempotent runtime application. Earlier live tests covered the existing alchemy
config path only. **The new C++ hooks have not been compiled or run.** Keep this
PR draft until Claude's Ninja/sccache tooling lands and the following checks pass
on an isolated Codex test runtime:

1. Build with the approved tooling; check touched files for new warnings.
2. Seed a fixture's Valor weekly count at its DBC limit, total holding count below
   the holding cap. Earn Valor via a normal reward path, not a GM grant that may
   bypass limits. With WeeklyCaps.Enable=0 the reward must arrive; with 1 the
   same path must stop at the DBC cap. Test spending/earning across the threshold.
3. Seed Conquest meta-arena/meta-rated-BG counts at/beyond their rating caps,
   total holding count below its cap. Trigger normal arena/random-BG and rated-BG
   rewards. With 0 each must award a positive expected count, update meta/main
   counters consistently and never subtract currency; with 1 stock clamps apply.
4. Verify weekly reset/accounting and total holding caps still work, and the
   Valor bonus unlocks at the original milestone while currency can exceed it.
5. On a fishing fixture at 200/600, enable entities.player.skills DEBUG and catch
   fish. With Fishing.Always=1 every valid catch must log 100% and raise skill;
   with 0 the log must show the original integer chance (44% at 200). At learned
   cap no catch may raise skill further. Failed casts/catches must grant nothing.
6. Toggle/reload both keys, then relog and inspect currency/PvP cap displays.
   Run the headless regression suite on test/none after the rebuilt server starts.

Source evidence: World.cpp config loading; CharacterHandler.cpp all-taxi login;
Player.cpp SkillGainChance, UpdateCraftSkill, UpdateGatherSkill, UpdateFishingSkill,
GetCurrencyWeekCap, GetCorpseReclaimDelay and spirit-healer resurrection handling.

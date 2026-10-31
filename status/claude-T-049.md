# T-049: Heartseed account-wide

**Agent:** claude
**Branch:** claude/T-049-account-heartseed
**Task status:** review

## What changed
- One seed per account (DESIGN.md, One seed per account). New characters tables `account_heartseed`
  (rank, Growth, grant time), `account_heartseed_essence` (collection counts) and
  `account_heartseed_fed_weapon`. Loadouts, essence slots and story flags stay per character.
- `HeartseedMgr`: every loaded character of an account holds the shared fields; a change through one is
  copied to the others (`ShareAccountState`). `GetEffectiveRank` = min(account rank, the character's cap),
  used for minors. Grant and Revoke act on the whole account.
- The player's alts logged in as playerbots load the seed (cached `_seedAccounts`, so random bots cost no
  query), use it at their own cap and proc, but earn no Growth and get no story beats or prompts.
- Config keys `Heartseed.Majors.TierLevels` ("10 25 40 55 70 85") and `Heartseed.Essence.SlotLevels`
  ("10 40 70") replace the hard-coded unlock levels (counts stay fixed).
- Checkpoint `heartseed_rank` only raises the account rank; the `heartseed_growth` steps are gone from the
  checkpoint data, so a twink checkpoint never resets the main's seed.

## How it was tested
- Unit tests: `tests/heartseed/` (effective rank, default, configured and rejected unlock levels); all 3
  unit suites pass.
- Migration: cloned the human's real `characters` save (2,200 characters, read-only dump) into a scratch
  database, seeded pre-T-049 rows for two accounts (three characters, overlapping essences and fed weapons),
  ran the update twice: identical dumps, core tables unchanged, rows merged as intended (highest rank wins,
  counts summed). Scratch database dropped. T-037's checker rejects data migrations (no SELECT/JOIN), so this
  was done by hand with the same steps.
- In game (headless, my server): an alt on the Seedtest account showed account rank 64 at its own cap (1 at
  level 1, 20 at level 20); Growth it earned showed on the main. The alt as a playerbot loaded the seed,
  earned nothing from `.heartseed growth +100`, and saw the main's new Growth at once. A shared essence kept a
  different slot per character. Revoke removed the seed from both; regrant, relog fine.
- Applied `2026_10_31_claude_16_heartseed_account.sql` to `characters_claude` and
  `2026_10_31_claude_17_checkpoint_account_seed.sql` to `world_claude`, twice each.

## Decisions made
- The migrated account keeps its furthest character's rank and Growth (highest rank, then Growth, then
  oldest grant); essence counts and fed weapons add up.
- Growth is spent up to the cap of whichever character earns or logs in (a higher-level alt can raise the
  shared rank further); bots never spend it.
- `heartseed_state` and `heartseed_fed_weapon` are kept, not dropped; Revoke clears them too so re-running the
  migration cannot bring a revoked seed back.
- Weapon appearances (`character_weapon_appearance`) stay per character: the Collections task owns the
  account-wide appearance tables (DESIGN.md, Collections). Soul-level story flags wait for the first one
  (Theramore); their table is one CREATE TABLE.

## For the other agents
- **Apply both SQL files** before running this build: without `account_heartseed`, existing seeds don't load.
- Codex: T-038 also rewrites `HeartseedMgr::LoadData` and the reload path. This PR leaves `LoadData`
  untouched and adds `LoadAccounts()` next to it on startup; the conflict should be in neighbouring lines only.
- Seen once, not reproduced in four further tries: the world thread hung in `Player::~Player` (a deque
  teardown) inside `HandlePlayerLoginOpcode` -> `LogoutPlayer`, when a master relogged while its bot alt was
  grouped with it. Stack from cdb; no Heartseed frames. Likely core or playerbots; worth a look if it recurs.

## Next
- Fresh-seed character challenge and bot recognition of the seed, when their systems exist.
- Optional `docs/TUNING.md` (all tuning keys in one place) not written yet.

## Map additions
- `tests/heartseed/`: unit tests for `HeartseedRules.h`.

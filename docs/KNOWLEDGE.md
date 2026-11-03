# Knowledge: lessons that cost time to learn

Non-obvious things we learned the hard way, written to transfer to the next problem. Project state and per-system
gotchas live in `STATUS.md` (Known facts and gotchas); this file holds the methods and internals behind them. Add an
entry when something took more than one wrong turn to find, and say what would have found it sooner.

## Debugging method

- **Split at the client/server line first.** Before theorising, find out which side refuses. The server logs most
  refusals (for example `Player::IsActionButtonDataValid` logs every rejected action-bar placement); no log line
  means the client never sent the request. In the client, call the API the UI calls from chat and print the result:
  `/run PickupCompanion("MOUNT",1) print(GetCursorInfo())`, `PlaceAction(12)` then `GetActionInfo(12)`,
  `SomeButton:IsEnabled()`. Two such one-liners found in minutes what three patch rounds missed (T-085).
- **Prove your code runs before debugging its logic.** When a patched file seems to have no effect, add a marker
  (`UMOP_SOMETHING = true`) and print it in game. A `nil` marker means the file never loaded, which is a different
  problem from wrong logic.
- **Read Blizzard's own interface code before blaming data or rules.** Greyed-out UI is often a hard-coded Lua check,
  not game data: the mount journal greys every mount below `PLAYER_MOUNT_LEVEL = 20` in
  `Blizzard_PetJournal.lua`, whatever the character can ride, and editing riding's spell level did nothing. Get the
  current file with `tools/clientpatch/client.py` (`read_current`), which applies Blizzard's delta patches.
- **When the human's observation contradicts the model, the model is wrong.** "I can use every mount but can't drag
  them" ruled out every usability theory; re-derive from that fact instead of patching around it.
- **Wait for a plateau before calling a leak.** The main worldserver with 500 bots climbs to about 9.9 GB private
  memory within the first hour, then stays flat. Sample over an hour before investigating.
- **Check another project's history before re-solving.** `I:\wowserver` (the earlier wowfusion project) hit many of
  the same client walls; its `docs/handoffs/` and `docs/DECISIONS.md` record what worked and what was deferred.

## The 5.4.8 client

- **Two archive chains.** Files live in the `base` chain (`Data/*.MPQ`, `Data/wow-update-base-<build>.MPQ`) or the
  locale chain (`Data/enUS/locale-enUS.MPQ`, `Data/enUS/wow-update-enUS-<build>.MPQ`). The client patches a file only
  from update archives of the chain that holds it. A Blizzard addon spans both: `Blizzard_PetJournal.lua` is in
  base, its `.toc` and `.toc.sig` in enUS. `client.chain_of` tells which chain holds a file.
- **Update archives must name their chain.** Each carries a `(patch_metadata)` file holding `base` or `enUS`.
  Without it the client attaches the archive to the wrong chain and its files silently never apply; no error, no
  crash. This is why our first patch "loaded" but changed nothing.
- **Load order is by build number,** so an archive named `...-18414.MPQ` loads after Blizzard's last update (18273)
  and wins.
- **MPQ layout matters.** Store uncompressed files the way Blizzard's update archives do (plain sectors, flags
  `0x80000000`). An uncompressed single-unit file crashes the client with "Reached the end of the file". Third-party
  readers (mpyq) misread this layout; trust `tools/clientpatch`'s own reader.
- **Blizzard ships most interface files as delta patches** (PTCH blocks with COPY or BSD0 bsdiff, MD5 checked) on
  top of an old full copy in `interface.MPQ`. Never edit the base copy; rebuild the current file through every delta
  first. Quirks: BSD0 bodies may be RLE-packed, and the bsdiff seek is 32-bit sign-magnitude.
- **Blizzard addon signatures.** A `Blizzard_*` addon's `.toc.sig` is a 276-byte signed hash. An edited addon with
  its original signature fails as "CORRUPT". A `.toc.sig` of any other size counts as absent and the addon loads
  unsigned (tainted, like a user addon). `tools/clientpatch` ships such a placeholder automatically. The `.pub` files
  in `Interface/AddOns/Blizzard_*` are written by the client itself and change nothing.
- **GlueXML and FrameXML are another matter.** They are checked against `GLUEXML.TOC.SIG` and similar; wowfusion saw
  "GlueXML is modified or corrupt" and a crash on an edited `CharacterCreate.lua`. Expect to need an exe patch
  (`_Wow.exe` copy) for those, and prefer a Blizzard addon or a server-side fix when one exists.
- **Mounts are companions in the UI.** The journal summons through `CallCompanion` and drags through
  `PickupCompanion("MOUNT", index)`. `IsSpellKnown(mountSpell)` returned false for mounts the character could ride,
  so it is no signal for mount problems.
- **Prefer client patches over addon shims** (the human's call): a small edit in `tools/clientpatch/edits.json`
  beats an addon that hooks around Blizzard code.

## Server core

- **Custom spell tables are read by position, not by column name.** `spell_dbc`, `spellmisc_dbc` and
  `spelleffect_dbc` fill the positions marked `p` in the `Custom*fmt` strings in `DBCfmt.h`, in column order. Some
  column names lie: in `spell_dbc` the columns named Levels, TargetRestrictions and Interrupts fill Interrupts,
  Levels and TargetRestrictions; in `spelleffect_dbc` the column named EffectDieSides fills ItemType.
  `spelleffect_dbc` couldn't set MiscValueB at all until T-086, so summons defined in SQL lost their summon
  properties: the spell cast fine and nothing appeared. Map by the fmt string (as `tools/clientpatch/spells.py` does)
  and test a new SQL spell in game, not just its cast.
- **Clone a client spell instead of renaming it.** Renaming a borrowed spell in the client renames the real ability
  for everyone (Berserking is the troll racial). A clone in the custom block keeps the effects and visuals under its
  own name, tooltip and icon; bind the source's class scripts to the clone and port its `SpellMgr` corrections into
  the clone's attributes.

## Tools and environment

- **Gemini CLI can't use a subscription.** Google sign-in succeeds, then the CLI is refused: "no longer supported
  for Gemini Code Assist for individuals", pointing to Antigravity. Flash is reachable only inside the Antigravity
  app, so hand it research through a prompt the human pastes.
- **Bash heredocs on this machine mangle backslashes** in Python, Lua and JSON. Write scripts and data with the Write
  tool (into the scratchpad when the path is outside the session's worktree, then copy) and run them.
- **Python is 3.9:** no `Path.write_text(newline=...)`, no `match`.
- **Check which exe a test ran against.** A "PASS" from a devenv still running the old `worldserver.exe` proves
  nothing. Compare the exe's timestamp with the build before trusting a result.
- **Agent "done" means the agent stopped,** not that the work is pushed. Check the branch before relying on it.

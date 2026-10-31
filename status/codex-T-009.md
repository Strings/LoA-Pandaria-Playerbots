# T-009: Eluna prototype profiles and layout

**Agent:** codex
**Branch:** codex/T-009-eluna-prototypes
**Task status:** review

## What changed
- Added independent T-002 profile.d overlay enabling Eluna/tracebacks in dev and test and pointing to this worktree's lua_scripts.
- Standalone enable_eluna.py patches an existing private runtime without needing T-002 source on this branch.
- lua_scripts bootstrap, examples/ GM-only chat probe, heartseed/ placeholder README and prototype/reload workflow.

## How it was tested
- Unit test: idempotent enable preserves unrelated config values.
- Existing T-001 binary loaded two Lua scripts; headless GM chat prototype-check returned one confirmation after each of two SOAP reload config calls. No duplicate hooks or Lua errors.
- Private Codex runtime/ports only; no full build or GUI. World stopped via SOAP; own auth process stopped.

## For the other agent
- T-002 PR #4 consumes the overlay when both independent PRs merge. Existing runtimes can use the standalone helper immediately.
- This fork reloads Lua through reload config, not reload eluna. Both ! and . prefix server commands; the chat probe deliberately uses plain prototype-check.
- No gameplay IDs, persistent tables or Heartseed behavior added. Shipped production Eluna default stays off.

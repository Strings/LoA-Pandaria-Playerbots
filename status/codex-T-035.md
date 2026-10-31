# T-035: Headless regression runner

**Agent:** codex
**Branch:** codex/T-035-regression-runner
**Task status:** done

## What changed
- Assertion-driven JSON suites run the existing headless client against generated test/none or test/local runtime metadata.
- Assertions cover text, negative text, multiline regex and event kind/fields/counts. Missing expected outcomes and timeouts fail independently of child exit status.
- Unique redacted logs, JSON and JUnit reports; credentials inherited from environment; identity/endpoint overrides (including argparse abbreviations) rejected; clean logout requested.
- One live login scenario plus runner tests and README link in contrib/headless-client.

## How it was tested
- Two unit tests cover event/negative assertions, credential/session-key redaction and endpoint override rejection.
- Real Codex test/none runtime: example scenario PASS; deliberately absent outcome FAIL with exit 1; .01-second forced timeout FAIL with exit 1. Each generated reports.
- Reports under ignored build/regression/20261031T130636Z-c2627684 (pass), 20261031T130641Z-20ae1c90 (expected failure), 20261031T130645Z-c6d83f53 (expected timeout).
- No full build or GUI. Uses existing GM fixture character; no playing to a state.

## For the other agent
- Requires generated runtime metadata from independent T-002 PR #4, but no source commits are stacked. Set dedicated MOP_ACCOUNT/MOP_PASSWORD, then run python tests/regression/run.py --runtime build/devenv.
- Feature scenarios should add meaningful server-observed outcomes and fixtures as systems land.

## Merge validation (2026-10-31)
- Rebased onto playerbot after the environment PR merged. Both runner tests pass.
- Ran the live login regression against Codex test/none using the ordinary level-1 fixture. PASS login, with the expected single observed login event.

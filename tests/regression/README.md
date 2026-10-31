# Headless regression suite

Python 3.9+ standard library; no game client, build or external test framework.
Start your generated **test/none** runtime from T-002/T-005 (local is supported
when the scenario needs bots). Set MOP_ACCOUNT/MOP_PASSWORD for a dedicated
fixture account with a character. No account creation is implicit.

```
python tests/regression/run.py --runtime build/devenv
python tests/regression/run.py --runtime build/devenv --suite tests/regression/scenarios/login.json
```

The runner takes agent ports from environment.json and forces localhost; scenarios
cannot override identity, endpoints or credentials. It sets HEADLESS_LOGOUT=1.
Scenarios run sequentially to avoid sharing account state. Each run gets an
ignored output directory with redacted logs, results.json and junit.xml. Exit 0
means all assertions passed; exit 1 means a scenario failed, and invalid setup
exits nonzero. Timeout is always a failure and terminates that child process.

Add JSON scenarios with a safe unique name, positive timeout_seconds, headless
args/steps and at least one assertion. Types: contains, not_contains, regex
(multiline), event (kind, optional exact fields, minimum/maximum counts).
Assertions inspect server-observed client output, not just the process exit.
Use fixtures/GM state commands, never play to a state. Set MOP_SOAP_USER and
MOP_SOAP_PASSWORD for SOAP steps; all endpoints still come from the runtime.
Scenario files are trusted executable test plans: GM steps can change state.

The example verifies authenticated world login and exactly one login event.
Add feature-specific outcomes and failure/timeout cases as systems land. Missing
events fail rather than silently counting a scenario as covered. Bot fixtures
and seeded randomness integrate through T-007/T-038 when available.

Unit tests: `python -m unittest discover -s tests/regression -v`.

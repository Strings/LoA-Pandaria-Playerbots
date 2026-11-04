# Private test fixture presets

characters.json is the versioned five-level fixture catalog and operation limits. The dated SQL adds only fixture-* data rows to the existing T-048 checkpoint tables; it is idempotent and test-only, not a production migration. Never apply it to main. tools/devenv/fixtures/README.md documents provisioning, stable GUID/equipment manifests, the pre-generated bot baseline and T-037 cold-file restore. No raw character/item save dumps belong in Git.

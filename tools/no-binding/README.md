# No-binding cleanup generator

Exports only template numbers, never character names or credentials. Supply paths
explicitly; the generator does not connect to any database. Use a numeric TSV
export with these column headers:

```sql
SELECT entry,class,bonding,Quality AS quality,startquest,flags
FROM item_template ORDER BY entry;
```

```powershell
python tools/no-binding/generate.py --catalog build/item-binding.tsv `
  --protected-entries "" --protected-guids "" `
  --hotfix-date "2026-11-01 00:00:00" `
  --world-output sql/updates/world/YYYY_MM_DD_codex_NN_no_binding_hotfix.sql `
  --characters-output sql/updates/characters/YYYY_MM_DD_codex_NN_no_binding_cleanup.sql
python -m unittest discover -s tools/no-binding -p test_generate.py
```

Match `--protected-entries` to `NoBinding.ProtectedItemIds`. Progression/storage
providers with instance-specific protection must also supply their low
item_instance GUIDs through `--protected-guids` before generating a cleanup.
Unknown templates, quest items, quest starters and artifact quality are excluded
automatically. The checked-in one-time update targets the current catalog; new
imports use the runtime binding hook/load guard. Regenerate a new dated update
for another catalog or new protection rules, never overwrite an applied update.

Back up a save and run on a real-save copy first. Apply the character update only
when no-binding is enabled. It changes only the soulbound flag and is idempotent.
The world update preserves hotfix history and original template binding, so
disabling the config can serve original tooltip data. It is also idempotent.
Updates.EnableDatabases remains off; apply updates manually to the intended DB.

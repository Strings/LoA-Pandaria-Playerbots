# BUG-002: Migration checker source column

**Agent:** codex
**Branch:** codex/BUG-002-migration-source-column
**Task status:** done

## What changed
- Distinguish quoted source columns from SQL client SOURCE directives. The merged weapon-appearance migration has a legitimate quoted source column.
- Client SOURCE remains forbidden at statement or line boundaries, including inside multi-line input; the existing custom-table, database and statement restrictions remain.

## How it was tested
- All three save-safety tests pass, including the real appearance DDL and rejected SOURCE directives.
- Applied the three merged Heartseed character migrations twice to a copy of the real Codex save, with sentinels in all six custom tables. Core tables and sentinels were preserved; migration data/schema was idempotent.
- Python compilation and diff checks pass. No shared service or original save was changed by the migration test.

## Next
- Continue rebuilt private-server checks. No database update belongs to this fix.

# Fix: client patch keeps Spell.dbc sorted by id

**Agent:** claude
**Branch:** claude/multiclass-key-combo-6f93c2
**Task status:** done

## What changed
- `tools/clientpatch/spells.py` inserts each spell clone at its sorted position in Spell.dbc instead of appending it.
  The client finds a spell's row by its rank among the ids, so the Durotar trinket clones (3190xx, appended after
  the 3200xx multiclass clones) made every multiclass spell show as a trinket spell (Hurl Axe dragged as Windrider's
  Grace, Bone Spear listed as Dormant Flame). Wild Rare (318100, before 318001-318008) shifted the affix markers the
  same way.

## How it was tested
- New unit test `test_spell_rows_stay_sorted_by_id`; clientpatch suite passes.
- Rebuilt and installed the patch; the installed Spell.dbc is sorted. In-game check is up to the human (client restart).

## For the other agent
- Stock Spell.dbc is the only spell table the client needs sorted (SpellMisc, SpellEffect, SkillLineAbility ship
  unsorted). Added to docs/KNOWLEDGE.md.

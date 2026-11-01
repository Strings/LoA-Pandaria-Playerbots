"""Unit tests for the trade goods stack size generator (T-062)."""

import unittest
from pathlib import Path

try:
    from tools.stack_sizes.stack_sizes import (
        get_qualifying_items,
        generate_sql,
        HOTFIX_TYPE_ITEM,
    )
except ImportError:
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from stack_sizes import (
        get_qualifying_items,
        generate_sql,
        HOTFIX_TYPE_ITEM,
    )


class TestStackSizes(unittest.TestCase):
    def setUp(self):
        self.sample_items = [
            {"entry": 1001, "name": "Non-stacking Herb", "stackable": 1},
            {"entry": 1002, "name": "Small Stack Ore", "stackable": 20},
            {"entry": 1003, "name": "Large Stack Cloth", "stackable": 200},
            {"entry": 1004, "name": "Already Big Material", "stackable": 1000},
            {"entry": 1005, "name": "Giant Stacking Item", "stackable": 5000},
        ]

    def test_qualifying_items_filtering(self):
        """Only items that already stack (> 1) and are below target size qualify."""
        qualifying = get_qualifying_items(self.sample_items, target_size=1000)
        entries = [item["entry"] for item in qualifying]
        # 1001 (stackable=1), 1004 (stackable=1000), 1005 (stackable=5000) are excluded
        self.assertEqual(entries, [1002, 1003])

    def test_custom_target_size_flag(self):
        """Custom target size properly adjusts qualifying items and SQL output."""
        qualifying_500 = get_qualifying_items(self.sample_items, target_size=500)
        self.assertEqual([item["entry"] for item in qualifying_500], [1002, 1003])

        qualifying_100 = get_qualifying_items(self.sample_items, target_size=100)
        self.assertEqual([item["entry"] for item in qualifying_100], [1002])

        sql = generate_sql([1002], target_size=500, hotfix_date="2026-11-01 12:00:00")
        self.assertIn("UPDATE item_template SET stackable = 500 WHERE entry IN (1002);", sql)
        self.assertIn("(1002,2442913102,'2026-11-01 12:00:00')", sql)

    def test_generate_sql_structure(self):
        """Generated SQL has transaction wrapper, update, delete, and insert."""
        entries = [2001, 2002, 2003]
        sql = generate_sql(entries, target_size=1000, hotfix_date="2026-11-01 12:00:00")

        self.assertTrue(sql.startswith("-- T-062"))
        self.assertIn("START TRANSACTION;", sql)
        self.assertIn("UPDATE item_template SET stackable = 1000 WHERE entry IN (2001,2002,2003);", sql)
        self.assertIn(f"DELETE FROM hotfix_data WHERE type = {HOTFIX_TYPE_ITEM} AND hotfixDate = '2026-11-01 12:00:00' AND entry IN (2001,2002,2003);", sql)
        self.assertIn("INSERT INTO hotfix_data (entry, type, hotfixDate) VALUES", sql)
        self.assertIn("(2001,2442913102,'2026-11-01 12:00:00')", sql)
        self.assertIn("(2002,2442913102,'2026-11-01 12:00:00')", sql)
        self.assertIn("(2003,2442913102,'2026-11-01 12:00:00')", sql)
        self.assertTrue(sql.endswith("COMMIT;\r\n"))

    def test_generate_sql_crlf_endings(self):
        """Generated SQL must strictly use CRLF line endings."""
        sql = generate_sql([1002], target_size=1000)
        self.assertIn("\r\n", sql)
        self.assertNotIn("(?<!\r)\n", sql)

    def test_empty_entries_handling(self):
        """When no items qualify, generated SQL remains syntactically valid."""
        sql = generate_sql([], target_size=1000)
        self.assertIn("START TRANSACTION;", sql)
        self.assertIn("COMMIT;", sql)
        self.assertNotIn("INSERT INTO hotfix_data", sql)


if __name__ == "__main__":
    unittest.main()

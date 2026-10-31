import unittest
from startup_profile import parse_runs


class Tests(unittest.TestCase):
    def test_units_runs_and_runtime_exclusion(self):
        runs = parse_runs('Using configuration file a\n>> Loaded 12 things in 7 ms\n'
                          '>> Initialized data in 1.5 seconds\nWorld initialized in 0 minutes 2 seconds\n'
                          '>> Loaded runtime cache in 9999 ms\nUsing configuration file b\n'
                          '>> Loaded other things in 4 milliseconds\n')
        self.assertEqual(len(runs), 2)
        self.assertEqual([r['milliseconds'] for r in runs[0]], [7, 1500])
        self.assertEqual(runs[1][0]['milliseconds'], 4)
        self.assertEqual(runs[1][0]['line'], 7)

    def test_empty_and_partial(self):
        self.assertEqual(parse_runs('unrelated log'), [])
        self.assertEqual(parse_runs('>> Loaded data in 2 s')[0][0]['milliseconds'], 2000)


if __name__ == '__main__':
    unittest.main()

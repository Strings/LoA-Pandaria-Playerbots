"""Rank loader timings emitted by the server; never infer disjoint wall time."""
import argparse
import json
from pathlib import Path
import re

TIMING = re.compile(r'(?P<label>.+?)\s+in\s+(?P<value>\d+(?:\.\d+)?)\s*(?P<unit>ms|milliseconds?|seconds?|s)\b', re.I)


def parse_runs(text):
    runs, current = [], []
    in_startup = True
    for number, line in enumerate(text.splitlines(), 1):
        if 'Using configuration file ' in line:
            if current:
                runs.append(current)
            current, in_startup = [], True
        if not in_startup:
            continue
        match = TIMING.search(line)
        if match and ('Loaded' in line or 'Initialized' in line or 'Loading' in line):
            value = float(match['value'])
            if not match['unit'].lower().startswith('m'):
                value *= 1000
            label = match['label'].split('>>')[-1].strip()
            current.append({'label': label, 'milliseconds': value, 'line': number})
        if 'World initialized' in line:
            in_startup = False
    if current:
        runs.append(current)
    return runs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--run', type=int, default=-1, help='zero-based startup index; default last')
    parser.add_argument('--top', type=int, default=20)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    if args.top < 1:
        parser.error('--top must be positive')
    try:
        runs = parse_runs(args.log.read_text(encoding='utf-8-sig', errors='replace'))
        rows = sorted(runs[args.run], key=lambda row: row['milliseconds'], reverse=True)
    except (OSError, IndexError) as exc:
        parser.error('cannot select startup: ' + str(exc))
    result = {'log': str(args.log.resolve()), 'runs_found': len(runs), 'run': args.run % len(runs),
              'timed_rows': len(rows), 'timed_sum_ms': sum(r['milliseconds'] for r in rows),
              'note': 'Loader timings can overlap or nest. Their sum is not startup wall time.',
              'top': rows[:args.top]}
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"Startup {result['run']} of {len(runs)}; {len(rows)} timing rows")
        print(result['note'])
        for row in rows[:args.top]:
            print(f"{row['milliseconds']:10.1f} ms  L{row['line']}: {row['label']}")


if __name__ == '__main__':
    main()

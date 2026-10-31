"""Apply verified config-only solo settings to an existing private runtime."""
import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def apply(runtime):
    runtime = Path(runtime).resolve()
    if ROOT not in runtime.parents:
        raise ValueError('runtime must be inside this worktree')
    values = json.loads(Path(__file__).with_name('profile.d').joinpath('019-solo.json').read_text())['dev']['world']
    config = runtime / 'worldserver.conf'
    text = config.read_text(encoding='utf-8-sig')
    for key, value in values.items():
        pattern = re.compile(r'^\s*' + re.escape(key) + r'\s*=.*$', re.M)
        if len(pattern.findall(text)) != 1:
            raise ValueError('expected one existing setting: ' + key)
        text = pattern.sub(lambda _: key + ' = ' + value, text)
    config.write_text(text, encoding='utf-8')
    return config


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runtime', type=Path)
    print(apply(parser.parse_args().runtime))

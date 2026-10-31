"""Apply solo settings to a private runtime; new hooks require a rebuilt server."""
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
        matches = len(pattern.findall(text))
        introduced = key in ('Currency.WeeklyCaps.Enable', 'SkillChance.Fishing.Always')
        if matches > 1 or (not matches and not introduced):
            raise ValueError('expected one existing setting: ' + key)
        text = pattern.sub(lambda _: key + ' = ' + value, text) if matches else text.rstrip() + '\n' + key + ' = ' + value + '\n'
    config.write_text(text, encoding='utf-8')
    return config


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runtime', type=Path)
    print(apply(parser.parse_args().runtime))

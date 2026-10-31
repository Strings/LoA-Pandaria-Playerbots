"""Enable worktree Lua prototypes in an already generated, private runtime."""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def enable(runtime):
    runtime = Path(runtime).resolve()
    if ROOT not in runtime.parents:
        raise ValueError('runtime must be in this worktree')
    config = runtime / 'worldserver.conf'
    text = config.read_text(encoding='utf-8-sig')
    values = {'Eluna.Enabled': '1', 'Eluna.TraceBack': 'true',
              'Eluna.ScriptPath': '"' + (ROOT / 'lua_scripts').as_posix() + '"'}
    for key, value in values.items():
        pattern = re.compile(r'^' + re.escape(key) + r'\s*=.*$', re.M)
        if len(pattern.findall(text)) != 1:
            raise ValueError('expected one existing config entry: ' + key)
        text = pattern.sub(lambda _: key + ' = ' + value, text)
    config.write_text(text, encoding='utf-8')
    return config


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runtime', type=Path)
    print(enable(parser.parse_args().runtime))

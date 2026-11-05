#!/usr/bin/env python3
"""Places hand-picked world treasures for one zone, driven by a zone file (see README.md).

  scan   <area> <roadX> <roadY> <roadZ>   rank existing spawn spots in an area as treasure candidates
  check  <zone.json>                      validate every spot and list anything standing on it
  settle <zone.json> [--write]            try small offsets around each spot, keep the best-covered pass
  sql    <zone.json> --out <file.sql>     write the idempotent world SQL update for the zone

The validator itself is the server's `.treasure validate` (src/server/scripts/Custom/Treasures), reached over SOAP.
Connection settings come from the environment: MOP_SOAP_HOST/PORT/USER/PASSWORD for SOAP, MOP_DB_HOST/PORT/USER/
PASSWORD, MOP_WORLD_DB and optionally MOP_MYSQL_BIN for the world database.
"""
import argparse
import html
import json
import math
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'contrib' / 'headless-client'))

HEAD_RE = re.compile(r"\(([-\d.]+), ([-\d.]+), ([-\d.]+)\) map (\d+): (PASS|FAIL[^\n]*?), score (-?\d+)")
SKY_RE = re.compile(r"\((\d)/5 rays")
PATH_RE = re.compile(r"path (ok|none) (\d+) yd")


# --- connections -----------------------------------------------------------------------------------------------

def soap(command):
    import headless
    user, password = os.environ.get('MOP_SOAP_USER'), os.environ.get('MOP_SOAP_PASSWORD')
    if not user or password is None:
        sys.exit('set MOP_SOAP_USER and MOP_SOAP_PASSWORD (and MOP_SOAP_PORT for a non-default server)')
    _, body = headless.soap_command(os.environ.get('MOP_SOAP_HOST', '127.0.0.1'), int(os.environ.get('MOP_SOAP_PORT', '7878')),
                                    user.upper(), password, command, timeout=60.0)
    m = re.search(r"<result>(.*?)</result>", body, re.S)
    return html.unescape(m.group(1) if m else body).strip()


def world_query(sql):
    password, db = os.environ.get('MOP_DB_PASSWORD'), os.environ.get('MOP_WORLD_DB')
    if password is None or not db:
        sys.exit('set MOP_DB_PASSWORD and MOP_WORLD_DB (plus MOP_DB_HOST/PORT/USER if not the defaults)')
    mysql_bin = os.environ.get('MOP_MYSQL_BIN', '')
    exe = str(Path(mysql_bin) / ('mysql.exe' if os.name == 'nt' else 'mysql')) if mysql_bin else 'mysql'
    env = dict(os.environ, MYSQL_PWD=password)
    res = subprocess.run([exe, '--protocol=TCP', '--host=' + os.environ.get('MOP_DB_HOST', '127.0.0.1'),
                          '--port=' + os.environ.get('MOP_DB_PORT', '3306'), '--user=' + os.environ.get('MOP_DB_USER', 'ultimatemop'),
                          '--batch', '--raw', '--skip-column-names', db, '-e', sql],
                         env=env, capture_output=True, text=True)
    if res.returncode != 0:
        sys.exit('mysql failed: ' + res.stderr.strip()[:300])
    return [line.split('\t') for line in res.stdout.splitlines()]


# --- validation ------------------------------------------------------------------------------------------------

class Result:
    def __init__(self, text):
        self.text = text
        head = HEAD_RE.search(text)
        sky = SKY_RE.search(text)
        path = PATH_RE.search(text)
        self.ok = bool(head) and head.group(5) == 'PASS'
        self.verdict = head.group(5) if head else text.splitlines()[0][:80] if text else 'no answer'
        self.score = int(head.group(6)) if head else -99
        self.pos = tuple(float(head.group(i)) for i in (1, 2, 3)) if head else None
        self.sky = int(sky.group(1)) if sky else 0
        self.path = int(path.group(2)) if path else 0
        self.indoors = 'indoors' in text


def validate(map_id, pos, road):
    return Result(soap('.treasure validate %.2f %.2f %.2f %d %.1f %.1f %.1f' % (tuple(pos) + (map_id,) + tuple(road))))


def occupants(map_id, pos, radius):
    x, y, z = pos
    d = 'SQRT(POW(position_x-(%f),2)+POW(position_y-(%f),2)+POW(position_z-(%f),2))' % (x, y, z)
    rows = world_query(
        "SELECT 'object', t.name, ROUND(%s,1) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id "
        "WHERE g.map=%d AND g.guid NOT BETWEEN %d AND %d AND %s < %f "
        "UNION ALL SELECT 'npc', t.name, ROUND(%s,1) FROM creature c JOIN creature_template t ON t.entry=c.id "
        "WHERE c.map=%d AND %s < %f LIMIT 6" % (d, map_id, 9090000, 9099999, d, radius, d, map_id, d, radius))
    return ['%s %s (%s yd)' % tuple(r) for r in rows]


# --- commands --------------------------------------------------------------------------------------------------

def load_zone(path):
    zone = json.loads(Path(path).read_text(encoding='utf-8'))
    themes = json.loads((HERE / 'themes.json').read_text(encoding='utf-8'))
    for spot in zone['spots']:
        if spot['theme'] not in themes['themes']:
            sys.exit('%s: unknown theme %s' % (spot['label'], spot['theme']))
    return zone, themes


def cmd_scan(args):
    rows = world_query(
        "SELECT ROUND(position_x,1), ROUND(position_y,1), ROUND(position_z,1) FROM creature WHERE map=%d AND areaId=%d "
        "UNION SELECT ROUND(position_x,1), ROUND(position_y,1), ROUND(position_z,1) FROM gameobject WHERE map=%d AND areaId=%d"
        % (args.map, args.area, args.map, args.area))
    points = []
    for r in rows:
        p = tuple(map(float, r))
        if all((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 > args.spread ** 2 for q in points):
            points.append(p)
    results = []
    for p in points[:args.max]:
        results.append((validate(args.map, p, (args.road_x, args.road_y, args.road_z)), p))
    results.sort(key=lambda r: -r[0].score)
    for res, p in results[:args.limit]:
        print('%4d %7.1f %7.1f %6.1f | %-32s | %s | sky %d/5 | path %d' % (
            res.score, p[0], p[1], res.pos[2] if res.pos else p[2], res.verdict[:32],
            'in ' if res.indoors else 'out', res.sky, res.path))
    print('%d spots scanned' % len(results))


def cmd_check(args):
    zone, _ = load_zone(args.zone)
    passed = 0
    for spot in zone['spots']:
        res = validate(zone['map'], spot['pos'], spot['road'])
        passed += res.ok
        print('== %s: %s, score %d, sky %d/5, path %d yd' % (spot['label'], res.verdict, res.score, res.sky, res.path))
        if args.verbose or not res.ok:
            print(res.text)
        for o in occupants(zone['map'], spot['pos'], args.radius):
            print('   on the spot: ' + o)
    print('%d of %d spots pass.' % (passed, len(zone['spots'])))


def cmd_settle(args):
    zone, _ = load_zone(args.zone)
    for spot in zone['spots']:
        if args.only and spot['label'] not in args.only:
            continue
        base = validate(zone['map'], spot['pos'], spot['road'])
        best = None
        for i in range(8):
            a = i * math.pi / 4
            p = (spot['pos'][0] + args.step * math.cos(a), spot['pos'][1] + args.step * math.sin(a), spot['pos'][2])
            res = validate(zone['map'], p, spot['road'])
            if res.ok and res.sky >= base.sky and (not best or res.score + 3 * res.sky > best.score + 3 * best.sky):
                best = res
        if best and (not base.ok or best.score + 3 * best.sky > base.score + 3 * base.sky):
            print('%s: move to %.2f %.2f %.2f (score %d -> %d)' % ((spot['label'],) + best.pos + (base.score, best.score)))
            spot['pos'] = [round(v, 2) for v in best.pos]
        else:
            print('%s: keep (%s, score %d)' % (spot['label'], base.verdict, base.score))
            if base.pos:
                spot['pos'] = [round(v, 2) for v in base.pos]
    if args.write:
        Path(args.zone).write_text(json.dumps(zone, indent=2) + '\n', encoding='utf-8')
        print('wrote ' + args.zone)


def q(text):
    return text.replace('\\', '\\\\').replace("'", "''")


def cmd_sql(args):
    zone, themes = load_zone(args.zone)
    spots, pools = zone['spots'], themes['themes']
    entry0, guid0 = zone['entryBase'], zone['guidBase']
    entry1, guid1 = entry0 + len(spots) - 1, guid0 + len(spots) - 1
    used = sorted({pools[s['theme']]['ref'] for s in spots} | {themes['blue']['ref']})
    by_ref = {p['ref']: p['items'] for p in list(pools.values()) + [themes['blue']]}
    out = [
        '-- %s treasures, generated by tools/treasures/treasure_zone.py from %s.' % (zone['name'], Path(args.zone).name),
        '-- Templates %d-%d, spawn guids %d-%d; shared loot pools from tools/treasures/themes.json.' % (entry0, entry1, guid0, guid1),
        'DELETE FROM `gameobject_template` WHERE `entry` BETWEEN %d AND %d;' % (entry0, entry1),
        'DELETE FROM `gameobject_template_addon` WHERE `entry` BETWEEN %d AND %d;' % (entry0, entry1),
        'DELETE FROM `gameobject_loot_template` WHERE `entry` BETWEEN %d AND %d;' % (entry0, entry1),
        'DELETE FROM `reference_loot_template` WHERE `entry` IN (%s);' % ', '.join(map(str, used)),
        'DELETE FROM `gameobject` WHERE `guid` BETWEEN %d AND %d;' % (guid0, guid1),
        'DELETE FROM `treasure_spot` WHERE `spawn_guid` BETWEEN %d AND %d;' % (guid0, guid1),
        '',
        'INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `data0`, `data1`, `data10`, `ScriptName`, `VerifiedBuild`) VALUES',
        ',\n'.join("(%d, 3, %d, '%s', '', '', '', 1, 0, %d, 1, 'go_treasure', 0)" % (entry0 + i, s['display'], q(s['name']), entry0 + i)
                   for i, s in enumerate(spots)) + ';',
        '',
        'INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `mingold`, `maxgold`) VALUES',
        ',\n'.join('(%d, 0, 0, %d, %d)' % (entry0 + i, s['gold'][0], s['gold'][1]) for i, s in enumerate(spots)) + ';',
        '',
        'INSERT INTO `reference_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES',
        ',\n'.join("(%d, %d, 0, '', 1, 1, 1)" % (ref, item) for ref in used for item in by_ref[ref]) + ';',
        '',
        'INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) VALUES',
    ]
    loot = []
    for i, s in enumerate(spots):
        e, ref, blue = entry0 + i, pools[s['theme']]['ref'], themes['blue']['ref']
        loot.append("(%d, %d, 100, '', 0, 1, 1)" % (e, s['curio']))
        loot.append("(%d, %d, 100, '', 0, -%d, 1)" % (e, ref, ref))
        loot.append("(%d, %d, %d, '', 0, -%d, 1)" % (e, blue, s.get('blueChance', themes['blue']['chance']), blue))
    out.append(',\n'.join(loot) + ';')
    out += ['', 'INSERT INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES']
    out.append(',\n'.join('(%d, %d, %d, %d, %d, 1, 1, %.2f, %.2f, %.2f, %.2f, 0, 0, %.6f, %.6f, 300, 100, 1)' % (
        guid0 + i, entry0 + i, zone['map'], zone['zoneId'], s['area'], s['pos'][0], s['pos'][1], s['pos'][2], s['o'],
        math.sin(s['o'] / 2), math.cos(s['o'] / 2)) for i, s in enumerate(spots)) + ';')
    out += ['', 'INSERT INTO `treasure_spot` (`spawn_guid`, `label`, `landmark`, `road_x`, `road_y`, `road_z`) VALUES']
    out.append(',\n'.join("(%d, '%s', '%s', %.1f, %.1f, %.1f)" % ((guid0 + i, q(s['label']), q(s['landmark'])) + tuple(s['road']))
                          for i, s in enumerate(spots)) + ';')
    with open(args.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    print('wrote %s (%d spots)' % (args.out, len(spots)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('scan')
    s.add_argument('area', type=int)
    s.add_argument('road_x', type=float)
    s.add_argument('road_y', type=float)
    s.add_argument('road_z', type=float)
    s.add_argument('--map', type=int, default=1)
    s.add_argument('--limit', type=int, default=15, help='rows to print')
    s.add_argument('--max', type=int, default=45, help='spots to validate')
    s.add_argument('--spread', type=float, default=6.0, help='skip spots closer than this to one already taken')
    c = sub.add_parser('check')
    c.add_argument('zone')
    c.add_argument('--radius', type=float, default=4.0)
    c.add_argument('-v', '--verbose', action='store_true')
    t = sub.add_parser('settle')
    t.add_argument('zone')
    t.add_argument('--step', type=float, default=3.0)
    t.add_argument('--only', nargs='*')
    t.add_argument('--write', action='store_true')
    g = sub.add_parser('sql')
    g.add_argument('zone')
    g.add_argument('--out', required=True)
    args = ap.parse_args()
    {'scan': cmd_scan, 'check': cmd_check, 'settle': cmd_settle, 'sql': cmd_sql}[args.cmd](args)


if __name__ == '__main__':
    main()

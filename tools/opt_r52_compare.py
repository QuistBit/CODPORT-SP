#!/usr/bin/env python3
"""Compare completed server timing windows in two console logs, not rendered FPS."""
import argparse
import math
import re
from collections import defaultdict
from pathlib import Path


FIELDS = re.compile(r'([A-Za-z_][A-Za-z_0-9]*)=([^\s]+)')
START = re.compile(r'^\[GSC R77\] started at game time (\d+)ms')
LOAD = re.compile(r'^CL_InitCGame: ([0-9.]+) seconds')


def read_log(path):
    maps, counts, current = {}, defaultdict(int), None
    for number, line in enumerate(Path(path).read_text(encoding='utf-8', errors='replace').splitlines(), 1):
        if line.startswith('Server: '):
            name = line[8:].strip()
            counts[name] += 1
            current = {'start': None, 'native': False, 'load': [], 'pickups': [],
                       'windows': defaultdict(list), 'detail': defaultdict(list)}
            maps[name, counts[name]] = current
        if current is None:
            continue
        if line.startswith('[Campaign R47E] native map profile='):
            current['native'] = True
        m = START.match(line)
        if m:
            current['start'] = int(m[1])
        m = LOAD.match(line)
        if m:
            current['load'].append(float(m[1]))
        fields = dict(FIELDS.findall(line)) if line.startswith('[OPT_') else {}
        if line.startswith('[OPT_PICKUP]'):
            if int(fields['accepted']) > 0:
                current['pickups'].append(int(fields['elapsed_ms']))
        if line.startswith('[OPT_LOAD]') and fields.get('scope') in ('cg_graphics', 'cg_init_detail'):
            if 'ms' in fields:
                current['detail'][fields['scope'], fields['stage']].append(int(fields['ms']))
            elif 'total_ms' in fields:
                current['detail'][fields['scope'], 'total'].append(int(fields['total_ms']))
        if line.startswith(('[OPT_FRAME]', '[OPT_ANIM]')) and current['start'] is not None:
            try:
                first, last = int(fields['first']), int(fields['last'])
                hz, peak = int(fields['hz']), int(fields['max_tick_ticks'])
                if hz <= 0 or peak < 0 or last < first:
                    raise ValueError('invalid timer domain')
                if first < current['start']:
                    continue  # exclude startup and straddling windows
                group = line.split(']', 1)[0][1:]
                current['windows'][group, fields['stage']].append(peak * 1000 / hz)
            except (KeyError, ValueError) as exc:
                raise ValueError(f'{path}:{number}: invalid profile record: {exc}') from exc
    if not maps:
        raise ValueError(f'{path}: no Server map boundary')
    return maps


def stats(values):
    if not values:
        return '-'
    data = sorted(values)
    over = sum(v > 1000 / 60 for v in data)
    return f'{len(data)} / {data[math.ceil(.95 * len(data))-1]:.3f} / {data[-1]:.3f} / {over}'


def report(before, after):
    print('Window count / P95 of window peaks (ms) / max (ms) / windows above 16.67 ms')
    print('Inclusive server timings overlap. No FPS or frame-percentile claims.')
    for key in dict.fromkeys([*before, *after]):
        print(f'\n{key[0]} visit {key[1]}')
        a, b = before.get(key), after.get(key)
        if a is None or b is None:
            print('  map visit missing in one log; not comparable')
            continue
        for label, m in [('baseline', a), ('candidate', b)]:
            print(f'  {label}: native={m["native"]} start={m["start"]} '
                  f'CGAME_s={m["load"]} pickup_ms={m["pickups"]}')
        for stage in sorted(a['windows'].keys() | b['windows'].keys()):
            print(f'  {stage[0]}:{stage[1]}: {stats(a["windows"].get(stage))} -> {stats(b["windows"].get(stage))}')
        for stage in sorted(a['detail'].keys() | b['detail'].keys()):
            print(f'  load {stage[0]}:{stage[1]}: {a["detail"].get(stage, "-")} -> {b["detail"].get(stage, "-")} ms')
    print('\nCheck identical hardware, settings, route and workload separately; log markers do not prove playability.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline')
    parser.add_argument('candidate')
    args = parser.parse_args()
    try:
        report(read_log(args.baseline), read_log(args.candidate))
    except (OSError, KeyError, ValueError) as exc:
        parser.exit(2, f'Invalid log: {exc}\n')


if __name__ == '__main__':
    main()

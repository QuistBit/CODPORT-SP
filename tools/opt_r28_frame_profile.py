#!/usr/bin/env python3
"""Offline wall-time summary for paired native R241 captures (no game changes)."""
import argparse
import bisect
import csv
import math
import statistics
from pathlib import Path

IDENTITY = ('id', 'map', 'pid', 'frequency', 'start', 'deadline')
ENGINE = ('frame', 'partial', 'begin', 'end', 'mask', 'cap_enter', 'cap_exit',
          'sv_enter', 'sv_exit', 'end_enter', 'end_exit', 'swap_enter', 'swap_exit',
          'cg_count', 'cg0_stereo', 'cg0_time', 'cg0_begin', 'cg0_end', 'ends',
          'swaps', 'state', 'keycatchers')
CG = ('ordinal', 'mask', 'begin', 'view', 'world', 'hud_begin', 'hud_end',
      'end', 'input_time', 'stereo')


def metadata(line, prefix):
    if not line.startswith(prefix):
        raise ValueError('missing ' + prefix)
    return dict(item.split('=', 1) for item in line[len(prefix):].split(','))


def read_trace(path, required, engine=False):
    lines = Path(path).read_text(encoding='ascii').splitlines()
    if len(lines) < 3:
        raise ValueError('incomplete capture: ' + str(path))
    meta = metadata(lines[0], '#')
    end = metadata(lines[-1], '#END,')
    if any(int(meta[k]) != 0 for k in ('overflow', 'invalid')):
        raise ValueError('overflow/invalid capture; record a shorter fresh trace')
    if engine and (int(meta['cg_lost']) or int(meta['cg_ack']) != 1):
        raise ValueError('engine reports missing CG export acknowledgement')
    reader = csv.DictReader(lines[1:-1])
    fields = reader.fieldnames or []
    if len(fields) != len(set(fields)) or not set(required).issubset(fields):
        raise ValueError('unexpected CSV header: ' + str(path))
    rows = []
    for row in reader:
        if None in row or None in row.values():
            raise ValueError('CSV column count mismatch')
        rows.append({k: int(row[k]) for k in required})
    if (int(meta['id']) != int(end['id']) or
            len(rows) != int(end['rows']) or len(rows) != int(meta['count'])):
        raise ValueError('capture footer/count mismatch')
    order = 'frame' if engine else 'ordinal'
    if any(b[order] <= a[order] for a, b in zip(rows, rows[1:])):
        raise ValueError('non-increasing row identifier')
    return meta, rows


def ordered(row, names):
    values = [row[k] for k in names]
    return values[0] >= 0 and all(a <= b for a, b in zip(values, values[1:]))


LADDER = ('pred_pm_flags', 'snap_pm_flags', 'predict_mask',
          'predict_begin', 'predict_end')
PMF_LADDER = 4  # src/game/bg_public.h, not eFlags


def ladder_state(row):
    if row['pred_pm_flags'] < 0 or row['snap_pm_flags'] < 0:
        return None
    pred = bool(row['pred_pm_flags'] & PMF_LADDER)
    snap = bool(row['snap_pm_flags'] & PMF_LADDER)
    return ('on' if pred else 'off') if pred == snap else 'mixed'


def analyze(engine_path, cg_path, ladder=None):
    em, er = read_trace(engine_path, ENGINE, True)
    if ladder not in (None, 'on', 'off', 'mixed'):
        raise ValueError('unknown ladder group')
    cm, cr = read_trace(cg_path, CG + LADDER if ladder else CG)
    if any(int(em[k]) != int(cm[k]) for k in IDENTITY):
        raise ValueError('capture identity mismatch')
    hz = int(em['frequency'])
    if hz <= 0 or int(em['deadline']) <= int(em['start']):
        raise ValueError('invalid clock metadata')
    cg_rows = sorted(cr, key=lambda r: r['begin'])
    starts = [r['begin'] for r in cg_rows]
    metrics = {}
    eligible = paired = 0
    prev = None

    def add(name, a, b):
        metrics.setdefault(name, []).append((b - a) * 1000 / hz)

    for r in er:
        seq = ('begin', 'cap_enter', 'cap_exit', 'sv_enter', 'sv_exit',
               'cg0_begin', 'cg0_end', 'end_enter', 'swap_enter', 'swap_exit',
               'end_exit', 'end')
        if (r['partial'] or r['mask'] & 255 != 255 or r['cg_count'] != 1 or
                r['ends'] != 1 or r['swaps'] != 1 or r['state'] != 8 or
                r['keycatchers'] or not ordered(r, seq)):
            prev = None
            continue
        lo = bisect.bisect_left(starts, r['cg0_begin'])
        hi = bisect.bisect_right(starts, r['cg0_end'])
        matches = [c for c in cg_rows[lo:hi] if c['mask'] & 31 == 31 and
                   c['end'] <= r['cg0_end'] and c['stereo'] == r['cg0_stereo'] and
                   c['input_time'] == r['cg0_time'] and
                   ordered(c, ('begin', 'view', 'world', 'hud_begin', 'hud_end', 'end'))]
        if ladder:
            if (len(matches) != 1 or ladder_state(matches[0]) != ladder or
                    matches[0]['predict_mask'] != 3 or not ordered(matches[0],
                    ('begin', 'predict_begin', 'predict_end', 'view'))):
                prev = None
                continue
        eligible += 1
        for name, a, b in (
            ('frame_envelope', 'begin', 'end'),
            ('cap_events', 'cap_enter', 'cap_exit'),
            ('initial_server', 'sv_enter', 'sv_exit'),
            ('pre_cgame', 'sv_exit', 'cg0_begin'),
            ('cgame_inclusive', 'cg0_begin', 'cg0_end'),
            ('endframe_inclusive', 'end_enter', 'end_exit'),
            ('swap_inclusive', 'swap_enter', 'swap_exit'),
            ('post_endframe', 'end_exit', 'end'),
        ):
            add(name, r[a], r[b])
        if (prev is not None and r['frame'] == prev['frame'] + 1 and
                r['begin'] >= prev['end']):
            add('swap_cadence', prev['swap_exit'], r['swap_exit'])
        prev = r
        if len(matches) != 1:
            continue
        paired += 1
        c = matches[0]
        if ladder:
            add('cg_prediction', c['predict_begin'], c['predict_end'])
        for name, a, b in (
            ('cg_to_view', 'begin', 'view'),
            ('cg_view_to_world', 'view', 'world'),
            ('cg_world_to_hud', 'world', 'hud_begin'),
            ('cg_hud', 'hud_begin', 'hud_end'),
            ('cg_after_hud', 'hud_end', 'end'),
        ):
            add(name, c[a], c[b])
    if not eligible or not paired:
        raise ValueError('no eligible paired gameplay frames')
    return len(er), eligible, paired, metrics


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('engine_csv')
    p.add_argument('cgame_csv')
    p.add_argument('--ladder', choices=('on', 'off', 'mixed'),
                   help='R29: select matched frames by predicted/snapshot pm_flags')
    args = p.parse_args()
    try:
        total, eligible, paired, metrics = analyze(args.engine_csv, args.cgame_csv, args.ladder)
    except (OSError, ValueError, KeyError) as exc:
        p.exit(2, 'R241 capture rejected: ' + str(exc) + '\n')
    if args.ladder:
        print('ladder group=' + args.ladder + ' (matched frames only)')
    print(f'engine rows={total}, eligible={eligible}, paired={paired}')
    print('wall ms; inclusive intervals overlap; do not sum columns')
    print(f'{"interval":24} {"n":>6} {"median":>10} {"p95":>10} {"max":>10}')
    for name, values in metrics.items():
        values.sort()
        p95 = values[math.ceil(len(values) * .95) - 1]
        print(f'{name:24} {len(values):6} {statistics.median(values):10.3f} '
              f'{p95:10.3f} {values[-1]:10.3f}')


if __name__ == '__main__':
    main()

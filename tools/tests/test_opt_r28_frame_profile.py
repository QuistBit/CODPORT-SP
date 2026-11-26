import csv
import tempfile
import unittest
from pathlib import Path

from opt_r28_frame_profile import CG, ENGINE, analyze


class FrameProfileTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.engine = Path(self.tmp.name) / 'engine.csv'
        self.cg = Path(self.tmp.name) / 'cgame.csv'

    def frame(self, number, offset=0, **changes):
        row = dict.fromkeys(ENGINE, 0)
        row.update(frame=number, mask=255, cg_count=1, ends=1, swaps=1,
                   state=8, cg0_time=number * 10)
        for key, value in zip(('begin', 'cap_enter', 'cap_exit', 'sv_enter',
                               'sv_exit', 'cg0_begin', 'cg0_end', 'end_enter',
                               'swap_enter', 'swap_exit', 'end_exit', 'end'),
                              (0, 1, 3, 4, 7, 8, 14, 15, 17, 19, 20, 22)):
            row[key] = offset + value
        row.update(changes)
        return row

    def draw(self, number, offset=0, **changes):
        row = dict.fromkeys(CG, 0)
        row.update(ordinal=number, mask=31, input_time=number * 10)
        for key, value in zip(('begin', 'view', 'world', 'hud_begin', 'hud_end', 'end'),
                              (8, 9, 10, 11, 12, 13)):
            row[key] = offset + value
        row.update(changes)
        return row

    def write(self, path, fields, rows, **changes):
        meta = dict(id=1, map=2, pid=3, frequency=1000, start=100, deadline=20100,
                    count=len(rows), overflow=0, invalid=0, cg_ack=1, cg_lost=0)
        meta.update(changes)
        with path.open('w', newline='', encoding='ascii') as f:
            f.write('#' + ','.join(f'{k}={v}' for k, v in meta.items()) + '\n')
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader()
            writer.writerows(rows)
            f.write(f'#END,id={meta["id"]},rows={len(rows)}\n')

    def pair(self, engine_rows, cg_rows, **changes):
        self.write(self.engine, ENGINE, engine_rows)
        self.write(self.cg, CG, cg_rows, **changes)
        return analyze(self.engine, self.cg)

    def test_intervals_and_cadence(self):
        total, eligible, paired, metrics = self.pair(
            [self.frame(1), self.frame(2, 30)], [self.draw(1), self.draw(2, 30)])
        self.assertEqual((total, eligible, paired), (2, 2, 2))
        self.assertEqual(metrics['frame_envelope'], [22, 22])
        self.assertEqual(metrics['initial_server'], [3, 3])
        self.assertEqual(metrics['cgame_inclusive'], [6, 6])
        self.assertEqual(metrics['swap_cadence'], [30])
        self.assertEqual(metrics['cg_hud'], [1, 1])

    def test_filtered_rows_break_cadence(self):
        total, eligible, paired, metrics = self.pair(
            [self.frame(1), self.frame(2, 30, partial=1), self.frame(3, 60),
             self.frame(4, 90, keycatchers=1)],
            [self.draw(1), self.draw(3, 60)])
        self.assertEqual((total, eligible, paired), (4, 2, 2))
        self.assertNotIn('swap_cadence', metrics)

    def test_unmatched_count(self):
        total, eligible, paired, _ = self.pair(
            [self.frame(1), self.frame(2, 30)], [self.draw(1)])
        self.assertEqual((total, eligible, paired), (2, 2, 1))

    def test_reject_identity_and_overflow(self):
        for change in ({'pid': 4}, {'overflow': 1}, {'invalid': 1}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.pair([self.frame(1)], [self.draw(1)], **change)

    def test_reject_ambiguous_or_invalid_time(self):
        with self.assertRaises(ValueError):
            self.pair([self.frame(1)], [self.draw(1), self.draw(2, input_time=10)])
        with self.assertRaises(ValueError):
            self.pair([self.frame(1, swap_exit=16)], [self.draw(1)])

    def test_reject_truncation_and_bad_columns(self):
        self.write(self.engine, ENGINE, [self.frame(1)])
        self.write(self.cg, CG, [self.draw(1)])
        original = self.cg.read_text()
        for bad in (original.rsplit('#END', 1)[0], original.replace('#END,id=1,rows=1', '#END,id=1,rows=2'),
                    original.replace('ordinal,mask', 'ordinal,ordinal')):
            self.cg.write_text(bad)
            with self.assertRaises(ValueError):
                analyze(self.engine, self.cg)


if __name__ == '__main__':
    unittest.main()

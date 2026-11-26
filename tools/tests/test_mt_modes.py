"""Check the two launch modes use identical assets, settings and log policy."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from run_training import training_command


class ThreadModeLaunchTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        root = Path(self.directory.name)
        self.build = root / "build with spaces"
        self.game = root / "original game"
        for name in ("WolfSP.exe", "qagamex86.dll", "cgamex86.dll", "uix86.dll", "soft_oal.dll"):
            path = self.build / "bin" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        for path in (self.build / "profile/main/training_player.cfg", self.game / "main/pak0.pk3"):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()

    @staticmethod
    def settings(command):
        return {command[i + 1]: command[i + 2] for i, token in enumerate(command) if token == "+set"}

    def test_modes_change_only_compute_and_render(self):
        single = training_command(self.build, self.game, "single")
        multi = training_command(self.build, self.game, "multi")
        a, b = self.settings(single), self.settings(multi)
        self.assertEqual({key for key in a if a[key] != b[key]},
                         {"com_multithreading", "com_workerThreads", "r_smp"})
        self.assertEqual((a["com_multithreading"], a["com_workerThreads"], a["r_smp"]), ("0", "0", "0"))
        self.assertEqual((b["com_multithreading"], b["com_workerThreads"], b["r_smp"]), ("1", "-1", "1"))
        self.assertEqual(a["com_asyncLog"], "1")
        self.assertEqual(a["fs_homepath"], str(self.build / "profile"))
        self.assertEqual(a["fs_cdpath"], str(self.game))
        self.assertEqual(single[-2:], ["+exec", "training_player.cfg"])
        self.assertEqual(single[0], multi[0])

    def test_default_is_multi(self):
        self.assertEqual(training_command(self.build, self.game),
                         training_command(self.build, self.game, "multi"))

    def test_unknown_mode_rejected(self):
        with self.assertRaises(ValueError):
            training_command(self.build, self.game, "automatic")

    def test_missing_matching_module_rejected(self):
        (self.build / "bin/cgamex86.dll").unlink()
        with self.assertRaises(FileNotFoundError):
            training_command(self.build, self.game, "single")


if __name__ == "__main__":
    unittest.main()

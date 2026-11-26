"""Start Training using an existing Call of Duty installation and a local profile."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def training_command(build, game, thread_mode="multi"):
    if thread_mode not in ("single", "multi"):
        raise ValueError("thread_mode must be single or multi")
    runtime = build / "bin"
    for name in ("WolfSP.exe", "qagamex86.dll", "cgamex86.dll", "uix86.dll", "soft_oal.dll"):
        if not (runtime / name).is_file():
            raise FileNotFoundError(runtime / name)
    profile = build / "profile"
    config = profile / "main/training_player.cfg"
    if not config.is_file():
        raise FileNotFoundError(config)
    if not (game / "main/pak0.pk3").is_file():
        raise FileNotFoundError(game / "main/pak0.pk3")
    settings = {
        "fs_basepath": str(profile), "fs_homepath": str(profile), "fs_cdpath": str(game),
        "fs_game": "main", "r_fullscreen": "0", "r_mode": "-1", "r_customwidth": "800",
        "r_customheight": "600", "r_ignorehwgamma": "1", "sys_cpuMHz": "3000",
        "vm_game": "0", "vm_cgame": "0", "vm_ui": "0", "com_introplayed": "1",
        "com_maxfps": "60", "developer": "0", "logfile": "2",
        "r_smp": "0" if thread_mode == "single" else "1", "r_swapInterval": "0",
        "com_multithreading": "0" if thread_mode == "single" else "1",
        "com_workerThreads": "0" if thread_mode == "single" else "-1",
        "com_asyncLog": "1",
    }
    command = [str(runtime / "WolfSP.exe")]
    for key, value in settings.items():
        command.extend(["+set", key, value])
    return command + ["+exec", "training_player.cfg"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, required=True, help="Call of Duty installation containing main/pak0.pk3")
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--thread-mode", choices=("single", "multi"), default="multi",
                        help="single: game/render work on the main thread; multi: render thread and CPU workers")
    args = parser.parse_args()
    build = args.build.resolve()
    try:
        command = training_command(build, args.game.resolve(), args.thread_mode)
    except FileNotFoundError as error:
        parser.exit(1, f"Missing required file: {error}\n")
    print(f"Threading mode: {args.thread_mode}", flush=True)
    raise SystemExit(subprocess.call(command, cwd=build / "bin"))

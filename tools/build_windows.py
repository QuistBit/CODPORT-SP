"""Build the Training profile from an x86 MSVC developer command prompt."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def run(tool, arguments, directory, label):
    response = directory / (label + ".rsp")
    response.write_bytes(b"\xff\xfe" + subprocess.list2cmdline(arguments).encode("utf-16-le"))
    print(f"{directory.name}: {label}", flush=True)
    with (directory / (label + ".log")).open("wb") as log:
        result = subprocess.run([tool, "@" + str(response)], cwd=directory,
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise SystemExit(f"{tool} failed. See {log.name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build")
    args = parser.parse_args()
    if os.environ.get("VSCMD_ARG_TGT_ARCH", "").lower() != "x86":
        parser.error("Run from the x86 Native Tools Command Prompt for Visual Studio.")
    for tool in ("cl.exe", "link.exe", "lib.exe"):
        if not shutil.which(tool):
            parser.error(f"{tool} is missing from PATH.")
    build = args.output.resolve()
    if build.exists() and any(build.iterdir()):
        parser.error(f"Choose an empty output directory: {build}")
    runtime = build / "bin"
    runtime.mkdir(parents=True, exist_ok=True)
    targets = json.loads((ROOT / "tools/windows-x86.json").read_text(encoding="utf-8"))
    products = {}
    for name, target in targets.items():
        objdir = build / "obj" / name
        objdir.mkdir(parents=True)
        objects = []
        for index, group in enumerate(target["compile"]):
            sources = [ROOT / path for path in group["sources"]]
            flags = [*group["flags"], "/MP2", "/I" + str(ROOT / "src/game"),
                     "/I" + str(ROOT / "src/game/gsc_host")]
            run("cl.exe", flags + [str(path) for path in sources], objdir, f"compile-{index}")
            objects.extend(str(objdir / (path.stem + ".obj")) for path in sources)
        library = target["output"].endswith(".lib")
        product = (objdir if library else runtime) / target["output"]
        flags = []
        for flag in target["link"]:
            if flag.startswith("@"):
                flag = str(products[flag[1:]])
            elif flag.startswith("/DEF:"):
                flag = "/DEF:" + str(ROOT / flag[5:])
            flags.append(flag)
        run("lib.exe" if library else "link.exe", flags + ["/OUT:" + str(product), *objects],
            objdir, "link")
        products[name] = product

    main_dir = build / "profile/main"
    main_dir.mkdir(parents=True)
    shutil.copyfile(ROOT / "main/training_player.cfg", main_dir / "training_player.cfg")
    # The original sound menu selects the OpenAL path only for the EAX 3 provider.
    (main_dir / "wolfconfig.cfg").write_text(
        'seta mss_3d_provider "Creative Labs EAX 3 (TM)"\nseta mss_khz "44"\n',
        encoding="ascii")
    entry = "scripts/gsc_tests/r234_training_turret.gsc"
    with zipfile.ZipFile(main_dir / "training.pk3", "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(ROOT / "main" / entry, entry)
    print(f"Game modules: {runtime}\nTraining profile: {main_dir.parent}")


if __name__ == "__main__":
    main()

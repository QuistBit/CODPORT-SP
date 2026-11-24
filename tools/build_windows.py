"""Build the Training profile from an x86 MSVC developer command prompt."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

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
    parser.add_argument("--game-pgd", type=Path,
                        help="Use an existing compatible MSVC GAME profile for the final /LTCG link.")
    parser.add_argument("--openal", type=Path,
                        help="Copy an x86 OpenAL Soft DLL to bin/soft_oal.dll for the launcher.")
    args = parser.parse_args()
    for supplied in (args.game_pgd, args.openal):
        if supplied and not supplied.is_file():
            parser.error(f"File does not exist: {supplied}")
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
    header_sets = json.loads((ROOT / "tools/windows-headers.json").read_text(encoding="utf-8"))
    # The released modules inherited objects built against different header revisions.
    # Resolve relative includes in an isolated tree and keep the checkout untouched.
    inputs = build / "obj/inputs"
    shutil.copytree(ROOT / "src", inputs / "src")
    menu_header = Path("main/ui/menudef.h")
    (inputs / menu_header).parent.mkdir(parents=True)
    shutil.copyfile(ROOT / menu_header, inputs / menu_header)
    active_headers = {}
    products = {}
    for name, target in targets.items():
        objdir = build / "obj" / name
        objdir.mkdir(parents=True)
        dependencies = objdir / "dependencies"
        dependencies.mkdir()
        for index, group in enumerate(target["compile"]):
            headers = header_sets[group["headers"]] if "headers" in group else {}
            for header in active_headers.keys() | headers.keys():
                revision = headers.get(header, header)
                if revision != active_headers.get(header, header):
                    shutil.copyfile(ROOT / revision, inputs / header)
            active_headers = headers
            sources = [inputs / path for path in group["sources"]]
            flags = ["/I" + str(inputs / flag[2:]) if flag.startswith("/I") else flag
                     for flag in group["flags"]]
            flags.extend(["/MP2", "/sourceDependencies", str(dependencies)])
            run("cl.exe", flags + [str(path) for path in sources], objdir, f"compile-{index}")
        objects = [str(objdir / (Path(path).stem + ".obj")) for path in target["objects"]]
        library = target["output"].endswith(".lib")
        product = (objdir if library else runtime) / target["output"]
        flags = []
        for flag in target["link"]:
            if flag == "@objects":
                flags.extend(objects)
                continue
            if flag.startswith("@"):
                flag = str(products[flag[1:]])
            elif flag.startswith("/DEF:"):
                flag = "/DEF:" + str(ROOT / flag[5:])
            flags.append(flag)
        if name == "game" and args.game_pgd:
            # MSVC updates the working database during /USEPROFILE.
            profile = objdir / "training.pgd"
            shutil.copyfile(args.game_pgd, profile)
            print("GAME profile SHA-256", hashlib.sha256(profile.read_bytes()).hexdigest(), flush=True)
            flags.append("/USEPROFILE:PGD=" + str(profile))
        run("lib.exe" if library else "link.exe", flags + ["/OUT:" + str(product)],
            objdir, "link")
        products[name] = product

    main_dir = build / "profile/main"
    main_dir.mkdir(parents=True)
    shutil.copyfile(ROOT / "main/training_player.cfg", main_dir / "training_player.cfg")
    shutil.copyfile(ROOT / "packaging/windows/wolfconfig.cfg", main_dir / "wolfconfig.cfg")
    if args.openal:
        shutil.copyfile(args.openal, runtime / "soft_oal.dll")
    print(f"Game modules: {runtime}\nTraining profile: {main_dir.parent}")


if __name__ == "__main__":
    main()

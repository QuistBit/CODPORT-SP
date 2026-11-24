# CODPORT-SP

[Русский](README.md) · [Download 724A for Windows](https://github.com/QuistBit/CODPORT-SP/releases/tag/724A) · [Training development update](docs/devlog/training.en.md)

CODPORT-SP brings the single-player **Call of Duty (2003)** to the **Return to Castle Wolfenstein: Single Player** engine. Build **724A** focuses on Training at Camp Toccoa. The port reads maps, GSC mission scripts, animations and audio from your original game installation.

![Recruits crawl under the wire and run to the ladder in Training.](docs/images/training-wire-ladder.jpg)

## Play Training

1. Install the original Call of Duty (2003) on Windows. Its installation folder must contain `CoDSP.exe` and `main\pak0.pk3`.
2. Download **CODPORT-SP-724A-Windows-x86.zip** from the release page. Extract the complete archive into a separate writable folder.
3. Open **Play-Training.cmd** and select the original game's **CoDSP.exe**. Start a new game from the main menu, select a difficulty and click the diary page to enter the mission. The 32-bit port runs in a window and stores its settings and log in `profile\main`.

## What's new in Training

The recruits vault the fence, crawl under the wire, get back on their feet and run to the ladder. Each recruit starts climbing at its base. Captain Foley walks beside the wire as he watches them. The diary opens in silence. Once you enter Camp Toccoa, you can hear the camp behind Foley's voice and after the intro music ends.

The Training map also loads faster. Two normal runs to the first M1 at 1280 × 800 averaged 55.5 and 56.7 FPS. The wire section averaged 52–55 FPS. The [development update](docs/devlog/training.en.md) includes the loading figures and a screenshot from the obstacle course.

Testing for 724A covered a new game through picking up both carbines at the range. Restoring the running mission script from a checkpoint and supporting the rest of the campaign, including Pathfinder, are still in development.

## Build from source

Open an **x86 Native Tools Command Prompt for Visual Studio** with MSVC, the Windows SDK and Python 3 available. From the repository root, run the following command.

```bat
python tools\build_windows.py
```

This builds `WolfSP.exe`, `qagamex86.dll`, `cgamex86.dll` and `uix86.dll` in `build\bin`. Put the **32-bit** `soft_oal.dll` beside the executable. The prebuilt Windows release includes [OpenAL Soft 1.25.2](https://github.com/kcat/openal-soft/releases/tag/1.25.2).

The published `qagamex86.dll` uses profile-guided optimization (PGO) from a Training playthrough. The default source build uses link-time code generation (LTCG). A local build can therefore differ in DLL size and frame rate.

```bat
python tools\run_training.py --game "C:\Games\Call of Duty"
```

For another build directory, pass `--output` when building and `--build` when launching.

The [game module](src/game) contains actor movement, weapon handling and the [GSC runtime](src/game/cod1_gsc). The [client game module](src/cgame) handles poses, mission audio, the HUD and the compass. The port uses the original game's `maps/training.gsc` mission script and the [Training configuration](main/training_player.cfg).

Based on [id Software's RTCW-SP source release](https://github.com/id-Software/RTCW-SP). The engine license and third-party notices are in [COPYING.txt](COPYING.txt) and [README.txt](README.txt).

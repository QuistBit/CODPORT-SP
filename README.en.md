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

Install Visual Studio or Build Tools with **Desktop development with C++**, the x86/x64 MSVC tools and a Windows SDK. The build uses **Windows PowerShell 5.1**; Python is not required.

Double-click **Build-Windows.cmd**, or run it from the repository root in an ordinary Windows Command Prompt:

```bat
Build-Windows.cmd
```

The script finds Visual Studio and initializes its x86 environment. The command window waits for a key after completion or an error so the message remains visible. A successful build produces `WolfSP.exe`, `qagamex86.dll`, `cgamex86.dll` and `uix86.dll` in `build\bin`, with Training launchers in `build`. Choose a new or empty directory with `-Output` for another build.

Put the **32-bit** `soft_oal.dll` in the output `bin` directory, or provide it when building:

```bat
Build-Windows.cmd -Output build_opt_r30 -OpenAL "C:\path\soft_oal.dll"
```

The prebuilt 724A Windows release includes [OpenAL Soft 1.25.2](https://github.com/kcat/openal-soft/releases/tag/1.25.2).

The published `qagamex86.dll` uses profile-guided optimization (PGO) from a Training playthrough. The default source build uses link-time code generation (LTCG). A local build can therefore differ in DLL size and frame rate.

```bat
build\Play-Training.cmd "C:\Games\Call of Duty"
```

Without a game path, the launcher opens a `CoDSP.exe` picker.

To select a Visual Studio installation explicitly, pass its root directory. An existing compatible PGO database can be supplied with `-GamePgd`:

```bat
Build-Windows.cmd -Output build_opt_r30 -VisualStudio "C:\Program Files\Microsoft Visual Studio\18\Community"
Build-Windows.cmd -Output build_opt_r30_pgo -GamePgd "C:\path\training.pgd"
```

To run without the final keypress, invoke the PowerShell script directly:

```bat
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\build_windows.ps1 -Output build_opt_r30
```

The existing Python builder remains an optional alternative: `python tools\build_windows.py --output build_opt_r30` from an **x86 Native Tools Command Prompt for Visual Studio**. Python is needed only for that alternative or the Python development tools.

### Comparing execution modes — R30

The R30 source adds `single` and `multi` execution modes (multi is the default). The build script copies **Play-Training-Single.cmd** and **Play-Training-Multi.cmd** into the build directory for double-click launching. Choose the mode before starting the game.

```bat
Build-Windows.cmd -Output build_opt_r30
build_opt_r30\Play-Training-Single.cmd "C:\Games\Call of Duty"
build_opt_r30\Play-Training-Multi.cmd "C:\Games\Call of Duty"
```

`single` executes game/render computation on the main thread; `multi` enables the renderer thread and one shared pool for independent model, lighting and collision-loading work. Logging policy is identical; audio and log service threads can still exist in `single`. R30 still needs a native MSVC build and gameplay validation; the published 724A archive does not contain these changes. See [OPT R30](docs/optimization/OPT_R30_MULTITHREADING.md) for scope, checks and the comparison procedure.

The [game module](src/game) contains actor movement, weapon handling and the [GSC runtime](src/game/cod1_gsc). The [client game module](src/cgame) handles poses, mission audio, the HUD and the compass. The port uses the original game's `maps/training.gsc` mission script and the [Training configuration](main/training_player.cfg).

Based on [id Software's RTCW-SP source release](https://github.com/id-Software/RTCW-SP). The engine license and third-party notices are in [COPYING.txt](COPYING.txt) and [README.txt](README.txt).

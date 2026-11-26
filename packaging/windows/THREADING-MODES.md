# R30: два режима одной сборки / two execution modes

Эта инструкция относится к новой сборке из исходников R30. Опубликованный архив 724A не содержит её изменений. Проверка в настоящей игре и измерение скорости ещё нужны.

## Сборка из исходников

Нужны Visual Studio или Build Tools с набором **«Разработка классических приложений на C++»**, компонентами MSVC для x86/x64 и Windows SDK, а также **Windows PowerShell 5.1**. Python для сборки и запуска через CMD-файлы не требуется.

Из корня репозитория откройте **Build-Windows.cmd** двойным щелчком или выполните в обычной командной строке:

```bat
Build-Windows.cmd -Output build_opt_r30 -OpenAL "C:\path\soft_oal.dll"
```

Параметр `-OpenAL` необязателен; он копирует указанную 32-битную DLL в `bin`. Без `-Output` используется каталог `build`. Каталог для новой сборки должен быть новым или пустым. Скрипт находит Visual Studio и настраивает x86-окружение; после завершения или ошибки окно ждёт нажатия клавиши.

Для явного выбора установленной Visual Studio и для существующего совместимого профиля PGO доступны параметры:

```bat
Build-Windows.cmd -Output build_opt_r30 -VisualStudio "C:\Program Files\Microsoft Visual Studio\18\Community"
Build-Windows.cmd -Output build_opt_r30_pgo -GamePgd "C:\path\training.pgd"
```

Запуск без ожидания клавиши:

```bat
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\build_windows.ps1 -Output build_opt_r30
```

Прежний `python tools\build_windows.py --output build_opt_r30` остаётся дополнительным вариантом для x86 Native Tools Command Prompt с установленным Python. Windows-сборка и работа нового PowerShell-сборщика ещё требуют проверки на целевой машине.

## Запуск и сравнение

1. Положите 32-битную `soft_oal.dll` в `bin`, если сборщик не скопировал её через `-OpenAL`.
2. Откройте **Play-Training-Single.cmd** для последовательных расчётов или **Play-Training-Multi.cmd** для многопоточных.
3. Выберите `CoDSP.exe` из установленной оригинальной Call of Duty. В меню начните новую игру и выберите сложность.
4. Полностью закройте игру перед запуском другого режима. Оба запуска используют одинаковый профиль, ресурсы, разрешение, лимит кадров и настройки журнала.

Путь к игре можно передать напрямую:

```bat
Play-Training-Single.cmd "C:\Games\Call of Duty"
Play-Training-Multi.cmd "C:\Games\Call of Duty"
```

Обычный **Play-Training.cmd** выбирает `multi`. В `single` отключены поток рендера и вычислительный пул; служебные потоки звука и журнала могут оставаться. Это режим игровых/графических расчётов, а не обещание одного потока ОС на весь процесс.

Для проверки в консоли введите `mt_status` после загрузки и после одинакового участка игры. В `multi` смотрите фактическое `renderer active=1` и увеличение `workerChunks`; `created` показывает только число созданных потоков. `nonOwnerBatches` считает принятые задания от потока рендера. `contentionSerial` означает последовательное выполнение при занятом общем пуле и само по себе не является ошибкой.

Запуск сохраняет лимит **60 FPS**, который может скрыть ускорение. Сравнивайте одинаковые сцены и настройки. Сохраните `profile\main\rtcwconsole.log` каждого запуска до следующего: журнал может быть перезаписан. Полезные участки: начало Training, курсанты у проволоки, подход и подъём по лестнице, оружие/руки, стрельба, завершение Training и переход в `us_intro`.

Трассировщик R241/R29 рассчитан на выключенный поток рендера. Для подробного R29-захвата используйте `single`; он не даёт корректное сравнение полного `multi`. Для обычного сравнения производительности трассировку выключите.

## English

From the repository root, double-click **Build-Windows.cmd** or run
`Build-Windows.cmd -Output build_opt_r30` in an ordinary Windows Command Prompt.
This uses Windows PowerShell 5.1, locates Visual Studio and initializes the x86
environment. Install the **Desktop development with C++** workload, x86/x64 MSVC
tools and a Windows SDK. Python is optional. The wrapper waits for a key after
completion or failure; invoke `tools\build_windows.ps1` directly with
`powershell.exe -NoProfile -ExecutionPolicy Bypass -File` for no pause.

Optional build arguments are `-OpenAL "C:\path\soft_oal.dll"` for the 32-bit
audio DLL, `-GamePgd "C:\path\training.pgd"` for an existing compatible PGO
database, and `-VisualStudio "C:\Program Files\Microsoft Visual Studio\18\Community"`
to select an installation. Choose a new or empty output directory. The old
Python builder remains available from an x86 Native Tools Command Prompt.

Use **Play-Training-Single.cmd** or **Play-Training-Multi.cmd**, select the original `CoDSP.exe`, then start a new game. Close the application fully before switching modes. Both use the same profile, game assets, resolution, 60 FPS cap and logging policy. The default launcher selects `multi`. Single mode disables the render thread and compute pool; audio/log service threads may still exist.

Run `mt_status` before and after comparable gameplay. Check actual `renderer active=1` and increasing `workerChunks` in multi mode. `created` alone does not prove worker participation. Save each `profile\main\rtcwconsole.log` before starting another run. R241/R29 detailed frame tracing supports non-SMP rendering only, so use single mode for those captures and keep tracing off for the ordinary mode comparison.

This R30 source build still needs native Windows/game validation. No FPS or loading-time improvement has been measured, and the published 724A release archive has not been updated.

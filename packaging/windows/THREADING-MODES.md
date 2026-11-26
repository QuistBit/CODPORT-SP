# R30: два режима одной сборки / two execution modes

Эта инструкция относится к новой сборке из исходников R30. Опубликованный архив 724A не содержит её изменений. Проверка в настоящей игре и измерение скорости ещё нужны.

1. Положите 32-битную `soft_oal.dll` в `bin`, если сборщик не скопировал её через `--openal`.
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

Use **Play-Training-Single.cmd** or **Play-Training-Multi.cmd**, select the original `CoDSP.exe`, then start a new game. Close the application fully before switching modes. Both use the same profile, game assets, resolution, 60 FPS cap and logging policy. The default launcher selects `multi`. Single mode disables the render thread and compute pool; audio/log service threads may still exist.

Run `mt_status` before and after comparable gameplay. Check actual `renderer active=1` and increasing `workerChunks` in multi mode. `created` alone does not prove worker participation. Save each `profile\main\rtcwconsole.log` before starting another run. R241/R29 detailed frame tracing supports non-SMP rendering only, so use single mode for those captures and keep tracing off for the ordinary mode comparison.

This R30 source build still needs native Windows/game validation. No FPS or loading-time improvement has been measured, and the published 724A release archive has not been updated.

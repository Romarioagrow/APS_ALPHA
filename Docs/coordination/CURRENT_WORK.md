# Текущая координация — 2026-09-27

- Production: `F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA`, ветка при аудите `dev-3`.
- Рабочий архив: `F:/ChatGPT/APOSFERA`; старый C:-путь не использовать для новых файлов.
- Принятый визуальный checkpoint: `67fff3b4`.
- HEAD до документационного аудита: `c97357eb` — сохранение диагностического
  water-depth overlay-v21 и результатов, не установка overlay в production.
- Документационный аудит завершён в `f70e382e`. Текущая задача Codex:
  остаточные артефакты лавы, квадратные LOD-переходы при вертикальном подлёте,
  глубина/течения водных океанов. Область: PlanetarySurfaceGeneratorStreaming,
  terrain/liquid helpers, защищённые material builders и их проверки.
  Принятую палитру, seed, географию, персонажа и saves не менять.
- Claude получает отдельные задачи от Rio; конкретная задача/набор файлов пока
  этим документом ему не назначаются. Старые ledger-владельцы — контекст архитектуры.

## Перед общей работой

Проверить актуальные HEAD/status и процессы, согласовать пересекающиеся файлы
и одно тяжёлое UE-окно. На момент аудита пользовательский Editor был открыт;
это наблюдение, не вечная блокировка и не право завершить процесс.

Не присваивать второму агенту всю подсистему только потому, что в старом документе
она была закреплена за Dev Surface или Dev UI. При handoff записывать задачу,
конкретные файлы, основание/commit и границы проверки. Коммитить только своё.

В этой итерации добавлены общий liquid-lattice helper и исправление гонки фоновых
LOD-задач WorldScape. Последнее установлено в engine plugin с резервной DLL;
не заменять этот модуль старой сборкой. Орбитальный повтор Volcanic прошёл,
но угловатые берега ещё видны. Подробности и границы проверки:
[planet finishing checkpoint](../Diagnostics/2026-09-27-planet-finish.md).

Водные v22/v23 остаются изолированными экспериментами на основе v21; принятые
материалы не перезаписывались. Повторный in-process moving A/B/A и sprint-проход
не выполнены. Имя сохранения BEM ожидается для точного быстрого спуска.

## Передача редактора после правки берегов

По просьбе Rio редактор освобождён для работы Claude над кораблями. Новых
build/bake/render запусков Codex не делает. В production собрана настройка
общей terrain/liquid сетки 192 вместо 96 для generated full-scale планет;
это ещё не визуальная приёмка и не подтверждение сохранения 120 FPS.
Пути, обратимый переключатель старого бюджета и незакрытые пункты:
[coast refinement handoff](../Diagnostics/2026-09-27-coast-refinement-handoff.md).

## Claude: корабли, этап 1 (27.09, вечер)

Задача от Rio: корабли становятся Blueprint-классами в общей иерархии, а конвертация
«любой меш из `AI_Shpis` становится кораблём» удаляется. Эти файлы до handoff не трогать:

- `Source/APS_ALPHA/Pawns/Spaceships/Spaceship.h/.cpp`: явный нос `bUseAuthoredNoseDirection` и
  editor-only стрелка `NoseArrow`. `SpaceshipFleetSubsystem.*` удалена.
- `Source/APS_ALPHA/Pawns/Spaceships/APSShipCatalog.*` и `Generation/AstroGenerator.h/.cpp` — только
  поле `ShipCatalog` и выбор класса эскорта стартового флота.
- Тесты: `APSGameplayIntegrationTests.cpp` (`AuthoredNoseDirection`), `APSShipCatalogTests.cpp`.
- Контент: `Core/Spaceships/**` (новые BP и `DA_ShipCatalog`), поле `ShipCatalog` в `BP_AstroGenerator`,
  акторы кораблей на `L_APS_SinglePlay_StartLocation`.

Рабочие материалы, скрипты и отчёт: `F:/ChatGPT/APOSFERA/work/ships`.

## Claude: стартовая станция и иконки меню (28.09, ночь)

Задача от Rio: отдельный блок выбора старта в меню цивилизации и иконки Blueprint
вместо векторных глифов. Файлы до handoff не трогать:

- `Core/Enums/StartStation.h` (`EAPSStartStation`), поле `StartStation` в `SpawnParameters.h`,
  `SetStartStation` в `WorldGenerationViewModel.*`.
- `Actors/Tech/SpaceStation.*` (компонент `PlayerStartPoint`, `GetPlayerStartLocation()`),
  `SpaceHeadquarters.*` (override). `SpawnPoint` верфей по-прежнему точка вылета кораблей.
- `Generation/AstroGenerator.*`: только `StartStation`, `GetOrbitalStartStation()` и орбитальная
  ветка `ResolveSpawnLocation` / камера старта.
- `UI/MainMenu/SAPSMainMenuRoot.*`: вкладка START (шаг 04) и миниатюры на карточках;
  `UI/MainMenu/APSUIThumbnails.h`; `Editor/APSUIThumbnailCommandlet.*`.
- Контент: `UI/Thumbnails/**` (62 текстуры на прозрачном фоне; у 8 BP без видимого меша
  остаётся векторный значок). Пересборка: `-run=APSUIThumbnail -AllowCommandletRendering
  [-SkipExisting] [-Only=...]`. `PlayerStartPoint` записан в `SpaceInfrastructure/BP_SpaceHeadquarters`,
  `_V2`, `BP_SpaceShipyard`, `_B`; классовые BP кораблей (`BP_SpaceshipBase`, `_XXS/_S/_M/_L/_XL`)
  помечены abstract и не показываются в меню. `Config/DefaultGame.ini`: только строка cook.
- Тест `APS.Gameplay.Start.StationPlayerStart` (в `APSGameplayIntegrationTests.cpp`).

Проверено: сборка редактора, 9 автотестов (`APS.Gameplay.Start`, `APS.Gameplay.Vehicle`,
`APS.Civilization.Save`, `APS.World.Save`), кадры иконок. Не проверено: вид меню и старт в
штабе/на верфи в игре — визуальная проверка за Rio. Запечка иконок грузит GPU и память;
не запускать её одновременно с другими GPU-задачами (ComfyUI и т. п.).

Перенос `L_APS_MainMenu_Alpha` в `Levels/Alpha` сделан не Claude и в этот набор не входит.
Скрипты и отчёты: `F:/ChatGPT/APOSFERA/work/ui`, `F:/ChatGPT/APOSFERA/work/stations`.

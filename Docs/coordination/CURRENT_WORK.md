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

## Claude: правки по ревью Rio (28.09, день)

- Меню: базовый фильтр (стартовый BP должен ссылаться на меш, сам или через родителя),
  `UI/MainMenu/APSStartAssetFilter.*` + `UI/Thumbnails/DA_StartAssetFilter` (ручной
  `ExcludedClasses`, автоматический `ClassesWithoutVisuals` от запечки), запасное имя пункта,
  скрытый класс по умолчанию заменяется первым видимым, «START HERE» на карточках станции,
  штаба и верфи, строка `Start` в DEPLOYMENT.
- Иконки: маска из альфы рендера (запечка идёт с `r.PostProcessing.PropagateAlpha=2`
  только в своём процессе, проект не меняется), кадр по силуэту из рендера 512.
- Корабли: `ASpaceship::HullHasFittedConvexCollision()`; генерированные корпуса с выпуклыми
  оболочками меша больше не строят коробки-слайсы. M3 и другие детальные корабли в полёте
  по-прежнему переключаются на коробки. Выпуклые оболочки мешей корпусов (V-HACD) —
  `F:/ChatGPT/APOSFERA/work/ships/ship_convex_collision.py`.
- Станции: без авторской точки старта пилот появляется над корпусом, а не в центре актора.
- Машина `BP_Spaceship_XXS_P1_12` по просьбе Rio снова статик-меш на `L_APS_SinglePlay_StartLocation`.
- Просадка FPS у поверхности не исправлялась: подозрение на бюджет сетки берегов из
  `7b8c87dd` (`aps.Surface.CoastResolution`), это зона Codex; нужен A/B Rio.

## Claude: освещение объектов (28.09, день) — проверено Rio визуально, закоммичено

- `Core/Rendering/APSStellarVisualSubsystem.*` (файл Codex, точечно): ключевой свет берёт
  температуру звезды (`SurfaceTemperature`, `bUseTemperature`) вместо палитры класса;
  заполняющий свет станций из `aps.Lighting.StationFill` (было 28). Остальная логика не менялась.
- Новый `Core/Rendering/APSObjectLightingSubsystem.*`: заполняющий свет от камеры на канале 1
  для кораблей, станций и пилота (планеты остаются на канале 0), множитель ламп станций,
  опциональная автоэкспозиция. `Spaceship.cpp`: частный свет пилота только с `aps.Lighting.PilotFill 1`.
- Все параметры — консольные `aps.Lighting.*`; `aps.Lighting.StarKelvin 0`, `ObjectFill 0`,
  `StationLightScale 1`, `StationFill 28`, `PilotFill 1` возвращают прежний вид.

## Ночь 28→29.09: слот UE и полёт кораблей — Claude

По указанию Rio Claude сам закрывает/запускает редактор и собирает, пока Rio спит. Codex в это
время правит только исходники (как в его текущем документе) и не запускает UE. Claude не трогает
файлы Codex по планетам/WorldScape/воде/лаве. Зона Claude на ночь: `Pawns/Spaceships/**`
(существующий `ASpaceship` — только диагностика и точечные фиксы просадок), новый экспериментальный
корабль с отдельной моделью полёта, бенчмарк полёта. Незакоммиченные правки меню, иконок, коллизий
кораблей и стартов станций ждут проверки Rio и пока не коммитятся.

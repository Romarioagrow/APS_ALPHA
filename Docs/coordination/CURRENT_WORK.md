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

Окна Unreal: Codex 06:25–07:15, Claude 07:35–07:49 (`PLANET_EDITOR_WINDOW.md`). Файлы Claude по полёту,
не закоммичены, ждут проверки Rio:
- `Pawns/Spaceships/Spaceship.*`:
  - точки расширения для наследников, замер `aps.Ship.PerfLog`, отметки Insights;
  - `MoveShipKinematic`: сфера перед свипом корпуса, `aps.Ship.SweepPrecheck`;
  - кэш проекции и раскладки меток HUD навигации: 1 мс → 0,17 мс на кадр, позиции не изменились.
  Полёт существующих кораблей не меняется.
- `Pawns/Spaceships/APSShipFlightBenchmark.*` (новые): `aps.Ship.Benchmark`, `Report`, `Board`, `AutoBenchmark`.
- `Pawns/Spaceships/APSExperimentalShip.*`, `Tests/APSExperimentalFlightModelTests.cpp` (новые) и
  `Core/Spaceships/Experimental/BP_Spaceship_Experimental`.
- `Tests/APSGameplayIntegrationTests.cpp`: `ControlRoundTrip` приведён к опциональной подсветке пилота
  (`aps.Lighting.PilotFill`) из коммита `f7e58a65`.
Главная причина просадок — коллизия рельефа WorldScape вокруг пилота: до 4,5 мс на кадр на 1600 м/с.
Предложение передано Codex (зона планет) в `PLANET_EDITOR_WINDOW.md`. Цифры, команды и порядок проверки:
`F:/ChatGPT/APOSFERA/work/flight/README.md`.

## Claude: начало мира на точке спавна (29.09) — проверено Rio, закоммичено `b21445f9`

- Проблема: генерация ставит в 0,0,0 орбитальный штаб, а игрок при старте на поверхности появлялся в 636–10 839 км
  от начала мира (медиана 2 571 км по 46 стартам в логах).
- `Core/World/APSWorldOriginSubsystem.*` (новые): `RebaseOnto` сдвигает весь мир штатным `UWorld::SetNewWorldOrigin`,
  точка игрока становится 0,0,0. WorldScape и материалы планет уже слушают `Pre/PostWorldOriginOffset`.
  Выключатель `aps.WorldOrigin.RebaseOnSpawn 0`; команды `aps.WorldOrigin.Report`, `aps.WorldOrigin.RebaseHere`.
- Общие файлы, точечно:
  - `Generation/AstroGenerator.cpp` — вызов после установки пешки и в конце `TryFinalizeSurfaceSpawn`;
  - `Core/Controllers/GravityPlayerController.cpp` — сохранения остаются в прежней системе (штаб в 0,0,0), при
    загрузке позиции пересчитываются и мир снова переносится на игрока.
- Проверено:
  - тест `APS.World.Origin.Rebase` и тесты сохранений;
  - rendered `GeneratedSurfaceLightingDiagnostics` (Water): Success, игрок после переноса в 0,00–0,01 м от 0,0,0.
- Замер ходьбы, по одному прогону на вариант: GPU −10%, ходьба 91 → 98 FPS, бег без изменений, всплески p99 в обоих.
- Ограничения:
  - начало мира в движке хранится в int32 — до ±21 474 км по оси;
  - при полёте координаты снова растут, плавающее начало мира не сделано.

## Claude: рывки кораблей на третьей ступени (29.09, ночь) — не закоммичено, ждёт проверки Rio

- Жалоба Rio: M3 на третьей ступени (ORBITAL) при разгоне даёт 20–30 FPS с рывками.
- Воспроизведено маршрутом Rio (меню → Civilization → верфь → ORBITAL к планете) новой командой стенда.
  Виновник — `UAPSStellarVisualSubsystem` → `APS_GameplayStellarView`: корабль при разгоне плавно расширял FOV
  (до +12°). Звёздный вид считает любое изменение FOV сменой оптики: 6–8 мс спроса в каждом кадре и
  пересчёт размеров всех звёзд по 80–100 мс (`BuildTreeIfOutdated`, `MarkRenderStateDirty`) до 18 раз в секунду.
  Касается всех кораблей: у S_P1_01 с бустом было 49 рывков за 9 с.
- Фикс только в `Pawns/Spaceships/Spaceship.cpp`: FOV по скорости выключен. Скорость по-прежнему видна по отъезду
  камеры, виньетке и bloom. `aps.Ship.SpeedFov 1` возвращает прежнее расширение. Файлы звёзд (Codex) не менялись.
- Замеры, 1600×900 без окна: M3 — игровой поток 11–28 → 2,3–3,3 мс, p99 кадра 90 → 16 мс, рывков >33 мс 38 → 3;
  S_P1_01 с бустом — 30–43 → 2,2–2,5 мс, рывков 49 → 2. Оставшиеся 2–3 рывка приходятся на секунду посадки.
- Стенд (`Pawns/Spaceships/APSShipFlightBenchmark.*`): `aps.Ship.Drive <ступень> <газ> [буст] aim= pitch= minalt= yaw=`
  держит органы управления настоящим полётным кодом и пишет скорость, высоту и время кадра по секундам.
  `aps.Ship.StartGenerated` из меню повторяет старт Civilization. Раннер `F:/ChatGPT/APOSFERA/work/flight/run_ship_drive.ps1`,
  прогоны `runs/m3-drive-p3-*`, `runs/s01-drive-p3-boost-*`.
- Не проверено: ощущение полёта без расширения FOV — за Rio. На будущее: любой анимированный FOV
  (зум, прицел) снова вызовет пересчёт звёзд; допуск по FOV нужен в `APSGameplayStellarView.cpp` (файл Codex).

## Claude: пилот с режимами темпа (29.09, ночь) — не закоммичено, ждёт проверки Rio

- Новое: `Pawns/Characters/APSSpeedModeCharacter.*` (`AAPSSpeedModeCharacter : ACustomGravityCharacter`) и
  `Content/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter_SpeedModes` (копия штатного BP с новым родителем).
  Штатный `BP_CustomGravityCharacter` не менялся и остаётся выбором по умолчанию.
- Клавиши 1/2/3 пешком: 1 WALK (1,7 / 3,8 м/с), 2 INDOOR (5 / 8 м/с, по умолчанию), 3 OPEN — прежний темп со спринтом
  9 → 40 м/с (было 15) и буст-прыжком. Плавный поворот вместо постоянных 500°/с, сглаженные значения для ABP,
  предел лага камеры растёт со скоростью. Все числа — свойства BP.
- Общие зоны, точечно:
  - `CustomGravityCharacter.h` — `GetTraversalStatusText/HintText` стали `virtual`;
  - `SAPSMainMenuRoot.cpp` — каталог пилотов принимает BP-наследников `ACustomGravityCharacter` из `/Game/APS/APS_ALPHA/`,
    карточка «LOCKED» только при одном варианте;
  - `WorldGenerationViewModel.cpp` — выбор такого наследника не подменяется штатным при инициализации и коммите Civilization.
- Проверено: сборка, BP сверен со штатным. `-game` через меню на поверхность Frozen-планеты: новый пилот принят и заспавнен,
  переключение режима клавишей. Проба `aps.Char.Probe` (клавиши через PlayerController), слалом на 6 м/с: рывок угловой
  скорости поворота 78 754 → 6 010 °/с², скачки GroundSpeed в ABP 4 534 → 672 см/с², провал скорости 19% → 12%.
  На 31 м/с лаг камеры 474 из 533 см вместо упора 225/225.
- Не проверено: режимы 1–2 в движении, прыжки, руки Rio, каталог в меню вживую (прогоны задавали пилота командой),
  иконка пилота не запечена. Подробности: `F:/ChatGPT/APOSFERA/work/character/README.md`.

## Claude: межзвёздные 9 FPS, модель полёта по режимам, коллизии, мерцание корпуса (29.09, утро) — не закоммичено, ждёт проверки Rio

- 9 FPS в дальнем космосе, две причины, обе правлены точечно в файлах Codex (согласовано в `PLANET_EDITOR_WINDOW.md`):
  1. Цикл регенерации домашней планеты: после выгрузки WorldScape-семейства резолвер размещения снова активировал
     планету — 58 применений профиля за 49 с. `APSPlanetSurfacePlacementResolver.cpp` + флаг `HasObservedPawn()`
     в `APSPlanetEnvironmentStreamingSubsystem.*`: 2 применения за прогон.
  2. Звёздный фон (настоящий 3D-каталог, 61 455 точек): угловой критерий параллакса почти каждый кадр запускал полный
     пересчёт размеров (40–80 мс) и синхронный `BuildTree` (~22 мс). Теперь размер пересчитывается поштучно, когда
     ошибка точки достигает прежнего бюджета 0,05 px; дерево HISM строится асинхронно; во время сборки точки ждут.
     Файлы: `APSGameplayStellarView.cpp`, `APSGameplayStellarProjection.h`, `APSStellarVisualSubsystem.h`,
     `APSGameplayNativeStars.cpp`.
  - Итог, 90 с в режиме STELLAR до ~390 c (`runs/band-stellar-keep-1`): средний кадр 8,3 мс, p99 18,9 мс,
    рывков >33 мс 32; раньше на максимуме было 51 рывок за 45 с и до 400 мс. Звёздный вид ~1,2 мс на кадр.
    Оставшиеся рывки 31–35 мс даёт тик `WorldScapeRoot`, пока корабль уходит от планеты — передано Codex.
- Модель полёта по режимам для всех кораблей: `Pawns/Spaceships/APSShipFlightModel.*`, компонент `FlightModel`
  у `ASpaceship`. `aps.Ship.FlightModel 0` возвращает прежние ступени × двигатели.
  - Клавиши 1–5: MANEUVER 60 м/с (висение: ключи задают скорость), FLIGHT 1,2 км/с (как машина: держать W,
    отпустил — тормозит, тяга слабеет у предела), ORBITAL 100 км/с (скорость держится, снос гасится,
    летит куда нос), CRUISE до 200 c (круиз-контроль; нужен SpaceWrap), STELLAR до ~3·10⁷ c (нужен Offset).
  - Правый Shift/Ctrl — соседний режим; левый Shift — буст ×3–4; левый Ctrl — тормоз.
  - До 90% предела ~4–5 с. У планет, лун, звёзд и станций предел = 0,5 × расстояние до поверхности, при отлёте ×2.
    Внутри габаритов станции — 500 м/с. У земли FLIGHT/MANEUVER ограничены 2 × высота, но не ниже 250 м/с.
  - Высота над рельефом и морем выше ~140 м (где у WorldScape нет коллизии) берётся из шума WorldScape. Страж:
    за кадр корабль сближается с телом не больше чем на четверть зазора. Режимы без свипа свипают кадры короче 10 км.
  - Симуляция в тесте: 5 св. лет — 60 с, с бустом — 13 с; 1 а.е. → 1000 км — 22 с; без пролёта звезды даже при
    кадрах по 0,4 с.
- Коллизии: у M3 корень без коллизии, а боксы-прокси не свипались — корабль под пилотом проходил станции и землю.
  Теперь `MoveShipWithProxySweep` (`aps.Ship.ProxySweep 0` — как раньше). Проверено: нос в корпус станции —
  стоп; по курсу дока вылет без контактов; спуск ORBITAL встал на рельеф на ~0,7 км.
- Мерцание корпуса на скорости: через кадр мигает освещение центральной части корпуса. Копия корпуса в глобальном
  distance field отстаёт на сотни метров за кадр. `aps.Ship.HullSceneLightingInFlight 2` (корпус вне DF) даёт
  мерцание 27 → 4, но центр корпуса без переотражений темнее. По умолчанию 1 (как было), выбор за Rio;
  переключается в полёте консолью.
- Тесты `APS.Gameplay.Vehicle` + `Start` 9/9 (`DriveEnvironmentAndSteering` теперь явно проверяет прежнюю модель).
  Стенд `aps.Ship.Drive` умеет режимы (`shipdrive=<1-5>/...`), `aps.Ship.FlightLog 1` пишет контакты.
- Не проверено: полёт руками и ощущение управления, другие корпуса кроме M3, ручная стыковка, долгий полёт
  к звезде каталога. Подробно: `F:/ChatGPT/APOSFERA/work/flight/README.md`.

## Claude: вечер 29.09 — режимы AUTO, фризы, звёзды после спавна; спринт с 30.09 — не закоммичено, ждёт Rio

- Все требования Rio за вечер, статусы и план по этапам — [CLAUDE_SPRINT_2026-09-30](CLAUDE_SPRINT_2026-09-30.md).
  Рабочий список: `F:/ChatGPT/APOSFERA/work/flight/TASKS-2026-09-29-evening.md`.
- Сделано и собрано (DLL 22:49 и новее), тесты `APS.Gameplay.Vehicle` + `Start` 9/9:
  - режим корпуса `HullSceneLightingInFlight 2` по умолчанию;
  - AUTO-режимы полёта: коробка-автомат по окружению, клавиша 0; скорость держится после отпускания Shift;
  - граница 35 св. лет от начала мира (улёт за ~42 св. года давал ensure `DoubleFloat.cpp:18`);
  - HUD в две строки; камера отзывается на ускорение и смену режима, FOV не трогаем;
  - звёздный вид: выбор native-звёзд при повороте камеры — только по кандидатам; пересчёт размеров ждёт запаса бюджета.
- Прогон `auto-keep-1`: 7 автопереключений, средний кадр 7,6 мс, p99 18 мс, 6 рывков за 90 с.
- КРИТ фризов персонажа на поверхности — `WorldScapeRoot`: `WS_CollisionLodHandler` до 57 мс, `UpdateSection GT`
  до 8 мс (`F:/ChatGPT/APOSFERA/work/character/runs/surface-look-2`). Передано Codex, принят 23:04.
- Регрессия моего `b21445f9`: после спавна на поверхности не было звёзд каталога — сдвиг мира шёл до закрепления
  каталога, проверка границ в мировом кадре падала. Исправлено в `APSWorldOriginSubsystem`: сдвиг ждёт закрепления.
  Собрано, в игре ещё не проверено.

## Claude (сессия «3D корабль»): CargoShip01 → Unreal, класс S (01.10) — не закоммичено, ждёт проверки Rio
- Задача Rio: импортировать очищенный в Blender грузовой корабль (94,2 м, интерьер, простая коллизия), собрать материал, дать готовый статик-меш и Blueprint корабля своего класса.
  Исходники и отчёт: `F:/Rio/3D/ShipCleanup/cargo01/out` (`README_CargoShip01.md`, раздел «Unreal Engine»); скрипты, логи, кадры: `F:/ChatGPT/APOSFERA/work/ships/cargo01`.
- **Класс S**: по правилу проекта (`ASpaceship::InferSizeClassFromLength`, длина корпуса ≤ 150 м). Рядом S-корабли 55–118 м (S_P1_01/02/03/06/16/24); иерархия
  `ASpaceship > BP_SpaceshipBase > BP_Spaceship_S > BP_Spaceship_S_P3_01`.
- Новые файлы (не отслеживаются, 150 МБ, крупнейшие: меш 49 МБ, нормаль корпуса 35 МБ — под LFS-порогом, но перед коммитом решить про LFS):
  - `Content/APS/APS_ALPHA/Assets/AI_Shpis/Pack_3/Spaceship_S_P3_01/` — `SM_Spaceship_S_P3_01` (нос +X, 178 UCX, Nanite, сокеты PilotSeat/PilotExit), `_Glass`, `_Ramp`,
    `Props/` ×5 (в BP не подключены), `Materials/` (5 мастеров + 27 MI), `Textures/` (24).
  - `Content/APS/APS_ALPHA/Core/Spaceships/S/BP_Spaceship_S_P3_01` (корпус, `GlassPanes`, `EntranceRamp`, 30 ламп интерьера `IntLight_*`).
  - `Content/APS/APS_ALPHA/UI/Thumbnails/T_Thumb_BP_Spaceship_S_P3_01_f43b7ffb` (иконка меню, коммандлет `APSUIThumbnail -Only=BP_Spaceship_S_P3_01`).
- Общий файл, точечно: `Core/Spaceships/DA_ShipCatalog.uasset` — +1 запись (`bAllowInStartingFleet` выключен: в стартовый флот не попадает, в верфи есть). C++, конфиги, планеты не менялись.
- Проверено: импорт = Blender до 0,04 см; BP (нос, кресло, выход, сфера гравитации); 5 мастеров компилируются на SM6; **проход капсулой 42×96 по маршруту трап → шлюз → лестницы → мостик, 18/18**
  (нашёл и исправил ступеньку 33 см у порога шлюза); кадры scene capture корпуса и интерьера (`.../cargo01/renders`, копии в `out/previews/unreal`).
- Не проверено: путь через меню/верфь/полёт (прогон «домашний корабль» берёт flight-сессия), FPS у Rio, ощущение ходьбы живым персонажем, яркость ламп при игровой экспозиции, Nanite + Custom-узел на видеокарте Rio.
- Известные пределы: вблизи < 1,5 м текстура корпуса мягкая/угловатая (атлас 3,6 см/тексель), полы трюма в тесте тёмные; пак мелких деталей отдельно, не подключён.

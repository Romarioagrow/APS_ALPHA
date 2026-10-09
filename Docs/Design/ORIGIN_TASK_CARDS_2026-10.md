# Карточки задач: режимы, происхождения, лестница — к старту кода

Claude, 2026-10-08. Четвёртый документ набора: [концепт](GAME_CONCEPT_ORIGIN_2026-10-07.md) → [контент-план](CONTENT_PLAN_ORIGINS_2026-10.md)
→ [брифы](ASSET_BRIEFS_ORIGINS_2026-10.md) → **карточки**. Каждая карточка — задача одной сессии-агента: файлы,
что сделать, выключатель, сейв, приёмка. Пути и имена проверены по коду 07.10 (опись механик) — если файл
переехал, искать по имени функции. Без сроков; волны задают порядок и зависимости.

## 0. Правила для каждой карточки

- **Главное (Rio 08.10): не навредить.** SANDBOX и всё, что уже принято, должно работать и выглядеть ровно так же.
  Каждая правка — за выключателем `aps.Origin.*` (по умолчанию выкл.), с офскрин-A/B кадров и FPS против базы
  (`F:/ChatGPT/APOSFERA/work/flight/run_0606_perf.ps1`, эталон `runs/p1-far-h`, сцены q01–q10; для пешего — q06/q07/q09).
- Plain C++ там, где можно (как Древние и мегаструктуры): без новых UCLASS/USTRUCT/UENUM, чтобы cl-check ловил всё
  до UHT. Новые значения enum — только в конец. Форматы сейвов не менять; новое — через существующие блоки
  (`Unlocked`, миссии, SpawnParameters) или вычисляется заново.
- Детерминизм: только сид, StableId, ключи тел; никакого глобального RNG.
- Общие зоны (`SpawnParameters`, `WorldGenerationViewModel`, `SAPSMainMenuRoot`, `AstroGenerator`, сейвы, `Spaceship.cpp`,
  персонаж) — узкий handoff через `Docs/coordination/PLANET_EDITOR_WINDOW.md`, один тяжёлый процесс UE за раз,
  перечитать хвост файла окна перед каждым запуском.
- Автотесты без мира (`APS.Origin.*`) на всё, что считается: гейты, формулу K, порядок ресурсов, определения
  происхождений.
- Коммит только после проверки Rio, только свои файлы. Handoff: файлы, что проверено, что нет, что требует решения.

Статусы по волнам: **W1** — фундамент (ничего не меняет в SANDBOX), **W2** — первый час ARK, **W3** — лестница до
конца и два других происхождения. Внутри волны карточки независимы, если не сказано иначе.

---

## W1 · Фундамент

### T-01 · Поля режима и происхождения в SpawnParameters

- **Зачем:** концепт §2 (ручки), §2.1 (происхождение как выбор START).
- **Файлы:** `Core/Model/SpawnParameters.h/.cpp` (рядом с `TechnologyLevel`, clamp в `.cpp:13`);
  `Gameplay/Civilizations/Civilization.h/.cpp` (копия параметров, `.cpp:24`);
  `Gameplay/Civilizations/APSCivilizationRuntimeManifest.cpp` (хеш, `:158`); `Tests/APSCivilizationManifestTests.cpp`.
- **Что:** добавить в `USpawnParameters` поля (int8/enum class, значения в конец): `ModePreset` {Sandbox=0, Origin,
  Custom}, `OriginId` {None=0, Ark, Adrift, UnderTheRing}, `Reach` {Open=0, Ladder}, `Knowledge` {AllKnown=0,
  HomeKnown, Unknown}, `Resources` {Unlimited=0, Normal, Scarce}, `Others` {Alone=0, Traces, Neighbours, Unknown}.
  **Нулевые значения = сегодняшнее поведение.** В хеш манифеста поля входят только если не нулевые (как в roadmap
  04.10 §2.2), чтобы хеши старых сейвов не менялись. Копия в `UCivilization`. Clamp-тест как у `TechnologyLevel`.
- **Выключатель:** не нужен — значения по умолчанию ничего не меняют.
- **Сейв:** `SpawnParameters` уже сериализуется; старые сейвы читают нули.
- **Приёмка:** `APS.Origin.Params.Defaults` (нули = SANDBOX), `APS.Origin.Params.ManifestHashStable` (хеш с нулями
  равен хешу без полей); загрузка любого старого сейва без изменений.
- **Общая зона:** `SpawnParameters` — объявить в окне.

### T-02 · Происхождение как данные: `FAPSOriginDefinition`

- **Зачем:** §2.1, §7 п. 13 — четвёртое происхождение должно быть записью, не веткой.
- **Файлы:** новые `Gameplay/Origins/APSOriginDefinition.h/.cpp` (plain C++), `APSOriginsTests.cpp`.
- **Что:** структура: `Id`, `StartKind` {ArkOnSurface, ShipAdrift, ArkOnMoonUnderRing}, `ArkPackage` (что спавнится:
  ковчег / корабль / ковчег + кольцо + лифт), `GuaranteedSites` (какие сайты Древних/экспедиции гарантированы:
  H_MONUMENT, H_HULL, H_CIRCLE, DEAD_STATION, RING, ELEVATOR), `ResourceOrder` (порядок RES_*: ARK E→M→V→R→I; ADRIFT
  E→V→M→R→I; RING E→V→M→R→I), `Tier2Gate` {LaunchFromMonument, ReviveStation, ElevatorToRing}, `OnboardingRouteId`,
  `JournalKey` (префикс текстов). Три записи в статической таблице; `Find(OriginId)`. Текстов журнала — §4 этого документа.
- **Приёмка:** `APS.Origin.Definitions.ThreeValid` (все поля заполнены, порядок ресурсов содержит все пять).
- **Зависит от:** T-01.

### T-03 · Гейты как данные: токены `Unlocked` и их читатели

- **Зачем:** §3 принцип 3, §7 п. 2 — полёт и генерацию не трогаем, ворота — записи в наборе `Unlocked` доски миссий.
- **Файлы:** `Gameplay/Expansion/APSMissions.h/.cpp` (набор `Unlocked`, сейв v5 — уже есть; добавить константы токенов
  и `IsUnlocked(Token)` + `Unlock(Token)` с событием), `Gameplay/Fleet/APSFleetCommand.cpp` (верфь: класс корпуса
  доступен, если `Reach == Open` или токен; `LaunchShip`/очередь верфи), `Gameplay/Vehicles/APSGroundVehicles.cpp`
  (маска техники = `GroundVehicleMask & ProgressionMask(tokens)`), `Gameplay/Expansion/APSInfrastructureCatalog.cpp`
  + `APSInfrastructure.cpp` (`CheckBuild`: тип с `RequiresToken` → отказ «LOCKED · REQUIRES …»),
  `Pawns/Spaceships/Spaceship.cpp` (один флаг `bDriveDead`: нельзя сесть пилотом, подсказка «ARK — drive dead»;
  узкая правка в общей зоне).
- **Токены:** `LAUNCH` (корпуса XXS/XS/S + LaunchYard), `STELLAR_DRIVE` (корпуса M), `GALAXY_HULLS` (L/XL/XXL),
  `VEHICLES_ROVER`, `VEHICLES_HOVER`, `VEHICLES_DRONE`, `RES_METALS`, `RES_VOLATILES`, `RES_RESEARCH`, `RES_INFLUENCE`,
  `TAB_INFRASTRUCTURE`, `TAB_FLEET`, `TAB_MAP_F10` (для T-11). В SANDBOX (`Reach == Open`) все читатели считают всё
  открытым, ни одна проверка не выполняется.
- **Выключатель:** `aps.Origin.Ladder` (0 = как сейчас, даже в ORIGIN — аварийный).
- **Сейв:** `Unlocked` уже в v5; токены — строки.
- **Приёмка:** `APS.Origin.Gates.Shipyard` (без LAUNCH верфь отказывает по классу, с LAUNCH — S да, M нет),
  `.Vehicles`, `.CatalogRefusalText`, `.OpenReachIgnoresAll`; офскрин: SANDBOX-сцены q01–q10 без изменений FPS.
- **Зависит от:** T-01.

### T-04 · Экран NEW WORLD: пресет и пять ручек (общая зона меню)

- **Зачем:** §2 — режим = пресет; §2.1 — START = происхождение.
- **Файлы:** `UI/MainMenu/SAPSMainMenuRoot.cpp` (страница CIVILIZATION; рядом с `NumberControl(... "TECHNOLOGY LEVEL" ...)`
  ≈ `:5422` и карточкой `TECH LEVEL` ≈ `:5649`), `UI/MainMenu/WorldGenerationViewModel.*`.
- **Что:** строка пресета `MODE: ORIGIN / SANDBOX / CUSTOM` (ORIGIN выставляет START=ARK, REACH=LADDER,
  KNOWLEDGE=UNKNOWN, RESOURCES=NORMAL, OTHERS=TRACES; SANDBOX — нули; CUSTOM открывает ручки). Ручка START в ORIGIN
  показывает три происхождения с одной строкой описания (тексты в §4). `OTHERS`: NEIGHBOURS/UNKNOWN серые с
  пометкой «later». Архетип/правительство/экономика/общество — как были.
- **Выключатель:** `aps.Menu.OriginPresets` (0 — блок не рисуется).
- **Приёмка:** офскрин-кадры страницы CIVILIZATION в трёх пресетах (есть `APSGenerationMenuShots.cpp`); хеш
  манифеста при SANDBOX равен сегодняшнему; стиль по `feedback_ui_strict_colours` (строгая палитра, без цветастого).
- **Зависит от:** T-01. **Handoff:** владелец меню (UI-сессия) — объявить в окне до правки.

---

## W2 · Первый час ARK

### T-05 · Старт ARK: пакет «ковчег»

- **Зачем:** §3 ступень 0, §4.1, §7 п. 3. Решение Rio: ковчег — новый корабль A-00, он не летает.
- **Файлы:** `Core/Model/SpawnParameters.cpp` (clamp `Orbital` min 1 → 0 при `OriginId == Ark`), спавн цивилизации
  (`Gameplay/Civilizations/*`, `GravityGameModeBase`), `Actors/Tech/Colony.cpp:18` (спавн `BP_ColonyHQ` → для ARK
  база колонии = актор ковчега), `Gameplay/Vehicles/APSGroundVehicles.cpp` (мотопул 40–60 м от базы — от ковчега),
  `Gameplay/Construction/*` (зона стройки — вокруг персонажа, не меняется).
- **Что:** при `OriginId == Ark`: не спавнить HQ, станцию, верфь и флот (`StartingFleetSize` = 0, `Orbital` = 0);
  поставить ковчег на `LandingPad`-точку GROUND-старта с `settle` по рельефу (как у построек); пилот появляется
  внутри у консоли (`SetViewDirection` на голостол — см. `reference_gravity_character_camera`); ковчег — `ASpaceship`
  с `bDriveDead` (T-03), материалы в «мёртвом» состоянии до события ArkPowered. До готовности A-00 — заглушка
  `BP_Spaceship_S_P3_01` за CVar `aps.Origin.ArkClass`.
- **Сейв:** ковчег — как сохранённая техника/корабль (ключ `PilotedVehicleKey`/флот) — проверить, что после загрузки
  стоит на месте и база колонии снова он.
- **Приёмка:** офскрин: спавн внутри, выход, стройка SolarArray у трапа, сейв/загрузка, FPS против q07/q09.
- **Зависит от:** T-01, T-02, T-03.

### T-06 · Модули колонии с доходом, FAB, плинт

- **Зачем:** §5 — модули перестают быть декорацией; §4.1 шаги 4–7.
- **Файлы:** `Gameplay/Colony/APSColonyModuleCatalogue.cpp` (9 записей, меши `:8-11`), `APSColonyConstructionSubsystem.cpp`
  (время стройки `:453`), `Gameplay/Expansion/APSInfrastructure.h/.cpp` (`Rates`, `Stocks` `:231`; домашняя добыча `:47-48`).
- **Что:** поле `Yield[5]` у модуля: SolarArray +3 E, Greenhouse +1 V, Habitat +1 I, Storage ×1,5 к потолку запасов
  (или +0, если потолка нет — тогда Storage открывает `CargoCrates` на складе), CommsMast — открывает карту
  поверхности и сигналы (событие), Floodlight — ничего; новый модуль **Fab** (Surface, 16 с, после CommsMast) —
  открывает `VEHICLES_ROVER` (затем HOVER/DRONE по второй и третьей стройке или по времени); `FAPSInfrastructure::Rates`
  суммирует модули колонии (список построенных — из construction subsystem). Меши: до кита A1 — примитивы как
  сейчас; FAB временно = Storage ×1,5.
- **Выключатель:** `aps.Origin.ColonyYields` (в SANDBOX тоже можно включить — это улучшение, не гейт; решение Rio).
- **Сейв:** модули уже в сейве; доход вычисляется.
- **Приёмка:** `APS.Origin.ColonyYields.Rates` (сумма), офскрин: построить SolarArray → Energy растёт в HUD.

### T-07 · LaunchYard — наземная верфь

- **Зачем:** §3 ступень 1→2, §4.3 шаг 10.
- **Файлы:** `Gameplay/Expansion/APSInfrastructureCatalog.cpp` (новый тип `LaunchYard`: Industry, Surface, K0, L0, 60 с,
  120 M 40 E, `RequiresToken = LAUNCH`, `bActsAsShipyard`), `APSInfrastructure.cpp` (`CheckBuild`, `CompleteAt` из режима
  B — тип в списке Surface), `Gameplay/Fleet/APSFleetCommand.cpp` (`CountStructures(Body, Shipyard)` считает LaunchYard;
  очередь верфи привязывается к нему; взлёт с `LandingPad`).
- **Что:** верфь на поверхности без станции на орбите; стоимость заказа корпуса в ORIGIN — Metals + Volatiles по классу
  (XXS 40 M / S 120 M 30 V / M 400 M 120 V / L 900 M 300 V), в SANDBOX — только время (как сейчас).
- **Приёмка:** `APS.Origin.LaunchYard.CountsAsShipyard`, `.ShipCostByMode`; офскрин: заказ S → корабль на площадке → посадка в кресло.
- **Зависит от:** T-03.

### T-08 · Ресурсы по одному

- **Зачем:** §5 (идея Rio): ENERGY → METALS → VOLATILES → RESEARCH → INFLUENCE, порядок — из происхождения.
- **Файлы:** `Gameplay/Expansion/APSInfrastructure.cpp` (`Rates`: домашняя добыча закрытого ресурса 0; `CheckBuild`:
  тип с закрытым ресурсом в цене → «LOCKED · REQUIRES METALS»; `Reserve` при UNLIMITED всегда проходит, при SCARCE
  стартовые запасы ×0,5 и добыча ×0,5), `UI/Hud/*` и `UI/Colony/SAPSCivilizationOverview.cpp` (скрыть закрытые
  запасы), `UI/Colony/SAPSInfrastructurePanel.cpp` (отказ в карточке), `Gameplay/Expansion/APSMissions.cpp`
  (шаблоны `ReachStock` по закрытому ресурсу не предлагаются).
- **События открытия:** `RES_METALS` — первый `MiningOutpost` построен (или первая руда у FAB); `RES_VOLATILES` —
  первый `GasHarvester`/посадка на ледяной мир; `RES_RESEARCH` — первый Studied мир или шаг 2 Echoes; `RES_INFLUENCE` —
  первый claim чужой системы. Для ADRIFT/RING порядок из `ResourceOrder` (T-02): открытие срабатывает по
  первому событию, разрешённому порядком.
- **Выключатель:** `aps.Origin.ResourceLadder`.
- **Приёмка:** `APS.Origin.Resources.OrderByOrigin`, `.LockedRefusal`, `.UnlimitedReserve`; HUD-кадры с 1, 2 и 5 ресурсами.
- **Зависит от:** T-02, T-03.

### T-09 · Награды Древних по режиму, пробуждение корпуса

- **Зачем:** §3 — ворота ступеней на гарантированных сайтах; §3.2 шаг 0.
- **Файлы:** `Gameplay/Ancients/APSAncientsQuests.cpp` (награды шагов), `APSAncients.cpp` (`UpdateChain`,
  триггеры), `APSAncientsTypes.h`.
- **Что:** при `Reach == Ladder`: Echoes шаг 3 дополнительно `Unlock(LAUNCH)` (журнал: «…The Ark's fabricator can
  build a launch yard.»); Quiet Hull шаг 3 → `Unlock(STELLAR_DRIVE)`; Road шаг 3 → `Unlock(GALAXY_HULLS)`
  (JumpGate остаётся). Существующие награды не трогать. Пробуждение корпуса (швы загораются, сигнал) — по порогу
  K 1,9 (T-12); пока K нет — по событию «станция + штаб построены» за CVar `aps.Origin.HullWakeStandIn`.
- **Приёмка:** `APS.Ancients.Quests.Chains` расширить на токены; офскрин: `aps.Ancients.Advance` до шага 3 → токен в сейве.
- **Зависит от:** T-03.

### T-10 · Онбординг ORIGIN·ARK и тексты журнала

- **Зачем:** §4 — режиссура первого часа.
- **Файлы:** `Gameplay/Quests/APSEarlyAccessOnboardingDefinition.cpp` (маршрут `APS.Onboarding.EarlyAccess.FirstRoute`,
  15 шагов, события `CivilizationReady … SystemSelectionRestored`), `UAPSQuestSubsystem`,
  `UAPSCivilizationJournalSubsystem::Post`.
- **Что:** новый маршрут `APS.Onboarding.Origin.Ark` (выбирается по `OnboardingRouteId` из T-02): шаги
  `ArkAwake → ArkPowered → ConsoleOpened → FirstExit → SolarArrayBuilt → HabitatBuilt → CommsMastBuilt → SignalReceived
  → FabBuilt → RoverPossessed → MonumentReached (Echoes 1) → MonumentStudied (Echoes 3) → LaunchYardBuilt →
  ShipLaunched → TakeoffCommitted → OrbitReached → HullSignal`. Новые события: `ArkPowered` (Interaction рычага P-01;
  до пропса — любая интеракция с консолью), `StructurePlaced(ModuleId)` (есть; фильтр по id), `VehiclePossessed`
  (есть `ShipPossessed` — техника = ASpaceship), `AncientsStep` (из T-09). Тексты — §4 этого документа, через журнал.
- **Выключатель:** маршрут выбирается только при `OriginId != None`.
- **Сейв:** quest bytes v3 — как у текущего маршрута.
- **Приёмка:** офскрин-прогон первого часа с `aps.Fleet.WorkScale`/`aps.Ancients.Advance` → все шаги проходят, журнал
  в порядке; сейв/загрузка посреди маршрута.
- **Зависит от:** T-05, T-06, T-07, T-09.

### T-11 · Вкладки терминала и F10 по ступеням

- **Зачем:** §7 п. 8 — в первый час консоль показывает только нужное.
- **Файлы:** `UI/Colony/*` (терминал Tab: OVERVIEW, INFRASTRUCTURE, CONSTRUCTION, SYSTEM, JOURNAL, FLEET ORDERS,
  DIVISIONS), `UI/StrategicMap/*` и `Core/Controllers/GravityPlayerController.cpp` (F10).
- **Что:** в ORIGIN вкладки появляются по токенам `TAB_*` (T-03): до CommsMast — OVERVIEW + JOURNAL; после —
  CONSTRUCTION и карта этого мира; FLEET ORDERS и INFRASTRUCTURE — после LAUNCH; F10 — после STELLAR_DRIVE. Закрытые
  вкладки видны серыми с подписью, что их откроет.
- **Приёмка:** кадры терминала на каждой ступени; в SANDBOX всё как сейчас.
- **Зависит от:** T-03.

---

## W3 · Лестница до конца, ADRIFT и RING

### T-12 · Шкала K

- **Зачем:** §3.1 — измеряемый уровень цивилизации, пороги как триггеры.
- **Файлы:** новые `Gameplay/Civilizations/APSCivilizationScale.h/.cpp` (plain C++), показ:
  `UI/Colony/SAPSCivilizationOverview.cpp:168` (где `TechLevel`), строка статуса HUD (`UI/Hud/*`), позже `.apsmeta` v3.
- **Что:** `Compute(World) → {K, EraName}`; целая часть по опоре: 0 — нет запитанной базы; 1 — есть LAUNCH или орбитальная
  станция; 2 — есть claim/ворота вне домашней системы; 3 — опора вне скопления (звезда `G<n>`) или DysonSphere.
  Дробь: для 1.x = 0,5·(доля миров системы Surveyed/Studied) + 0,3·(станция, штаб, верфь по 0,1) + 0,2·min(1,
  Energy-rate/порог); для 2.x = доля систем скопления Scanned/Surveyed/Claimed + реле/ворота; для 3.x — системы галактики
  + мегаструктуры. Источники: `FAPSFleet` (surveys, `CountStructures`), `FAPSStarSystems` (knowledge, claims),
  `FAPSInfrastructure` (Rates, структуры). Пороги → события: 0,9 монумент-сигнал разрешён, 1,9 корпус просыпается (T-09),
  2,9 последний участок Road называет финал (T-16). Пересчёт раз в 2 с, без тика.
- **Выключатель:** `aps.Civ.Scale` (0 — показ TECH LEVEL как сейчас).
- **Сейв:** ничего (вычисляется).
- **Приёмка:** `APS.Civ.Scale.Monotonic`, `.Thresholds`; кадры OVERVIEW/HUD; в SANDBOX — только показ.

### T-13 · Флот между звёздами при REAL DISTANCES — замер и правило

- **Зачем:** §5, риск концепта §11: если приказы к другим звёздам занимают часы, ступень 3 проваливается.
- **Файлы:** `Gameplay/Fleet/APSFleetCommand.cpp` (кап 20 000 км/с без SpaceWrap, ~330 c с ним; базовые секунды `:532`).
- **Что:** сначала **замер** (офскрин, REAL DISTANCES): Probe и SurveySystem к трём ближним звёздам скопления —
  длительность в логе. Затем правило: время приказа между звёздами считать по сетке (реле/ворота) и классу корабля,
  а не по расстоянию: `T = Base × (1 + hops) / (1 + 0,2·level)`, где hop — переход между соседними claim-системами;
  без сетки — ×3. В SANDBOX — как сейчас, за CVar `aps.Fleet.NetworkTravel`.
- **Приёмка:** таблица «до/после» по трём звёздам; `APS.Origin.FleetTravel.Formula`.

### T-14 · ДОЛГОЕ ПАДЕНИЕ (большая, 5 подзадач по порядку)

- **Зачем:** §3.2 — переход «система → скопление», главный кадр трейлера. Решение Rio: цель — противоположная сторона галактики.
- **a · Цель.** `Core/Rendering/APSGalaxyGpuStars` (`FindNearStars`, индекс), `Gameplay/Expansion/APSStarSystems`
  (`Learn`, запись `G<n>`): выбрать звезду галактики с планетами в направлении «от дома»: если дом в ядре — на ~0,8
  радиуса; если у края — на другом краю; детерминированно от сида. Материализация как у любой `G<n>`.
- **b · Корпус-корабль.** `Gameplay/Ancients/APSAncients.cpp` + `Pawns/Spaceships/Spaceship.cpp` (узко): при пробуждении
  спавнится `ASpaceship` пресета XL с геометрией корпуса как компонент, автопилот к цели (a), `BoardShip` сажает игрока
  пассажиром после BOARD IT; палуба — интерьер A6 (до него — процедурный ангар). Звёздный привод по существующему пути J.
- **c · Падение.** Скрипт прибытия: за N секунд до цели — «глаз» гасит привод (журнал), спуск по кривой на
  поверхность, занавес прибытия (`APSArrivalCurtain`/существующий), спавн обломков (вид Derelict на поверхности,
  `APSAncientsSites` + settle), игрок у обломков; корабль S игрока — в трюме, `bDriveDead` до ремонта.
- **d · Пешая цепочка.** Шаги на доске миссий (как цепочки Древних): `A COLD DECK` (секция с питанием) → `THE PATH`
  (плиты к кольцу) → `THE ALTAR` (Interaction P-02) → `THE DRIVE` (перенос P-03 в корабль, ремонт) → `THROUGH` (пролёт).
- **e · Награда.** `Unlock(STELLAR_DRIVE)`, `Unlock(JUMPGATE)`, система цели Charted + Claimed (SurveyBeacon там), +1
  Exploration, +1 Industry, K = 2,0 (T-12).
- **Выключатель:** `aps.Origin.LongFall` (+ `aps.Origin.LongFall.Skip` для отладки: сразу награда).
- **Сейв:** шаги — миссии v5; корпус-корабль — флот; обломки и кольцо — из сида заново.
- **Приёмка:** офскрин: от BOARD IT до THROUGH с ускорением; кадры «дом уходит» с мостика; FPS на палубе летящего
  корпуса против q02.
- **Зависит от:** T-09, T-12, T-15 (пролёт домой).

### T-15 · Пара ворот: переход игрока и флота

- **Зачем:** §3.2 шаг 5, §7 п. 12 — JumpGate становится настоящим. Решение по объёму в EA — открыто (§10 п. 12);
  минимум для ДОЛГОГО ПАДЕНИЯ — квестовая пара.
- **Файлы:** `Gameplay/Expansion/APSInfrastructure.cpp` (тип JumpGate: `PairKey`), материализатор `FAPSSystemMaterializer`,
  `Core/World/APSWorldOriginSubsystem`, `Gameplay/Fleet/APSFleetCommand.cpp` (маршрут через ворота), `ShipNavigationComponent`.
- **Что:** ворота знают пару; пролёт сквозь кольцо (триггер) → занавес → корабль ставится у парных ворот (тот же путь,
  что прибытие в систему: материализация цели, world origin, скорость обнуляется); флот: приказ через пару — время
  как один hop (T-13). Своя стройка ворот (`JumpGate` из каталога) создаёт пару с ближайшими своими воротами или с
  домом.
- **Выключатель:** `aps.Origin.Gates`.
- **Сейв:** `PairKey` — в SiteKey структуры (строка), формат не меняется.
- **Приёмка:** офскрин: пролёт дом → дальняя система → дом, сейв/загрузка посередине; FPS против q01.

### T-16 · Финал: «мир, которого нет»

- **Зачем:** §4.4.
- **Файлы:** `Gameplay/Ancients/APSAncientsSites.cpp` (одна запись по сиду: система на краю, 25–35 св. лет), `APSAncientsGeometry.cpp`
  (игла + кольцо, своя подсветка), `APSAncientsQuests.cpp` (последний шаг Road называет систему; шаг `THE WORLD THAT ISN'T THERE`),
  титр (`UI/Hud` или занавес).
- **Приёмка:** `aps.Ancients.Teleport` к финалу → пешком к подножию → журнал, титр, достижение; сейв после.
- **Зависит от:** T-12 (порог 2,9), T-14 (корпуса L для дальности — или ворота).

### T-17 · Происхождение ADRIFT

- **Зачем:** §2.1, §4.5.
- **Файлы:** T-02 запись; спавн (`SpaceShip`-старт есть); `Gameplay/Fleet/APSFleetCommand.cpp` (`IssuePilotOrder` SURVEY —
  есть); `Gameplay/Expansion/APSInfrastructure.cpp` (Energy как топливо CRUISE: списание за перелёт при `OriginId == Adrift`);
  посадка = основание ковчега (T-05 на корабле игрока); мёртвая станция: спавн станционного BP (до A4 — `AI_Stations\03`
  с BP, затем A4) в «мёртвом» состоянии + сцена оживления (Interaction P-04 → свет секциями → станция считается
  построенной для `CountStructures`).
- **Выключатель:** `aps.Origin.Adrift`.
- **Приёмка:** офскрин: старт в корабле → SURVEY трёх миров → посадка → ковчег → стыковка → оживление → верфь.
- **Зависит от:** W2.

### T-18 · Происхождение UNDER THE RING

- **Зачем:** §2.1, §4.5.
- **Файлы:** T-02 запись; `MoonSurface`-старт (есть); `Gameplay/Megastructures/APSMegastructures.cpp` (кольцо и лифт как
  объекты Древних вокруг стартовой луны — `PlaceRoot`/`SpawnStructure` без стройки, «мёртвые» материалы);
  лифт: Interaction у подножия → занавес → игрок на противовесе/секции кольца (телепорт по сокету); секция кольца —
  станция (гравитационный объём `ASpaceStation::GravityCollisionZone`), `CountStructures` считает её.
- **Выключатель:** `aps.Origin.Ring`.
- **Приёмка:** офскрин: старт на луне → башня → подъём → секция → заказ корабля; FPS у кольца и башни против q07.
- **Зависит от:** W2, проверка спавнера мегаструктур в кадре (Rio).

### T-19 · Происхождение EXODUS (Rio 09.10; после галактических режимов — отдельная волна W4)

- **Зачем:** концепт §2.1 EXODUS: корабль поколений ищет живой мир по галактикам и находит единственный —
  колыбель Строителей. Начинается там, где другие заканчиваются.
- **Зависимости (блокирующие):** фаза 4 генератора (несколько галактик, прыжки между ними) или промежуточный
  вариант по решению §10 п. 15 (скопления нашей галактики, roadmap 04.10 «до 5 скоплений»); решение, связывать
  ли колыбель с «миром, которого нет» (T-16).
- **Файлы:** T-02 запись (`StartKind = GenerationShip`, `ResourceOrder` E→V→M…, `Tier2Gate = None`); `Core/Model/
  SpawnParameters` (`FoundingPopulation` 250 000, флот = зонды и разведчики корабля); `Pawns/Spaceships/Spaceship.cpp`
  (пресет Titan `:2094` — есть; флаг «привод выгорел» после последнего прыжка = `bDriveDead` из T-03);
  генератор: правило «в галактике/скоплении нет пригодных миров, кроме одного по сиду» (`AstroGenerator`,
  `PlanetarySurfaceGenerator` — общая зона, узкий handoff: только фильтр пригодности при `OriginId == Exodus`);
  прыжок: занавес прибытия + смена активной галактики/скопления + скачок времени (журнал: поколение +1, запасы −;
  `UAPSCivilizationJournalSubsystem`); `Gameplay/Fleet/APSFleetCommand.cpp` (`CountStructures` считает корабль
  станцией, штабом и верфью); A-00 лендер как корабль игрока в ангаре.
- **Что:** цикл «скан → прыжок» N раз (сид, 5–9), затем сигнал колыбели; высадка лендером; дальше W2 как ARK с
  орбитальной станцией-кораблём; ДОЛГОЕ ПАДЕНИЕ не запускается (корпус Строителей в этой системе — часть
  колыбели, даёт STELLAR_DRIVE обычной цепочкой); галактическая дальность возвращается через Дорогу и ворота.
- **Выключатель:** `aps.Origin.Exodus`.
- **Сейв:** номер прыжка и поколение — в миссиях/журнале (v5), без нового формата; активная галактика — то, что
  даст фаза 4.
- **Приёмка:** офскрин: мостик → три скана → прыжок с ускорением → сигнал → высадка → ступень 0; FPS на борту
  Titan против q05/q06.
- **Не делать до решений:** генератор не трогать, пока нет решения по промежуточному варианту.

### T-20 · Режим SPACE TRIPS (Rio 09.10; независим от лестницы — можно делать параллельно с W1–W2)

- **Зачем:** концепт §2.2 — третий режим: корабль, маршруты по красивым мирам, путеводитель, фоторежим, без
  экономики и целей. Самый дешёвый режим и первый источник видео.
- **Файлы:** T-01 (+ поле `Goals` {Civilization=0, None}); читатели `Goals == None`: `UI/Hud/*` (минимальный HUD),
  `UI/Colony/*` (терминал: только ROUTE, JOURNAL, SYSTEM), `Gameplay/Expansion/APSMissions.cpp` (доска не стартует),
  `Gameplay/Fleet/APSFleetCommand.cpp` (флот не спавнится, приказов нет), `Actors/Tech/Colony.cpp` (колония не
  спавнится), `Pawns/Spaceships/Spaceship.cpp` (узко: без урона, привод без ограничений — флаги); новый
  `Gameplay/Trips/APSTrips.h/.cpp` (plain C++): `FAPSTripStop {StableId, BodyKey, OrbitRadii, TerminatorSide,
  DwellSeconds}`, `FAPSTrip {Name, Stops, ShipClass}`, сборщик тематических маршрутов из каталога
  (`FAPSStarSystems`, данные планет генератора: тип, кольца, двойные звёзды, сайты Древних `APSAncientsSites`),
  кодирование/декодирование кода поездки (код мира + остановки), файл `Saved/Trips/*.json`;
  экскурсионный автопилот: над `APSShipFlightModel`/`ShipNavigationComponent` (SetCourse + автопилот есть) —
  последовательность остановок, правило парковки «освещённая сторона у терминатора» (вектор звезда→тело),
  стоянка, «дальше»; путеводитель — `UAPSCivilizationJournalSubsystem::Post(World, "Trip", …)`; фоторежим —
  камера корабля (`SpringArmComponent`, свободная орбита вокруг корабля в пределах 3 длин), HUD прочь, FOV,
  экспозиция, пауза, `HighResShot`; панель ROUTE в терминале; главное меню: вход TRIP (по решению §10 п. 16).
- **Выключатель:** `aps.Trips` (0 — пресет не показывается, читатели `Goals` не проверяются).
- **Сейв:** поездка — свой файл; сейв цивилизации не создаётся в этом режиме.
- **Тесты:** `APS.Trips.ThemedRoutesFromCatalogue` (по сиду, детерминированно, без мира), `.TripCodeRoundTrip`,
  `.ParkingSideLit`.
- **Приёмка:** офскрин: поездка из трёх остановок с ускорением — подлёт, парковка, путеводитель, фоторежим-кадр;
  FPS против q01/q10; SANDBOX и ORIGIN без изменений.
- **Зависит от:** T-01 (поле). Всё остальное — существующий полёт.

### T-21 · Авторские миры (Rio 09.10; независим от лестницы — параллельно с W1)

- **Зачем:** [AUTHORED_WORLDS_2026-10.md](AUTHORED_WORLDS_2026-10.md) — заданные миры на всех уровнях как пресеты
  генератора вместо ручных карт; одно зерно, имя, лор, картинка.
- **Файлы:** `UI/MainMenu/WorldGenerationViewModel.*` (загрузка модели как у сохранённого мира; замки по
  `APSWorldRoll::EScope` — REGENERATE крутит только открытые уровни; RESET TO AUTHORED), `UI/MainMenu/APSWorldRoll.*`
  (архетипы уже есть — авторский мир = архетип с закреплённым зерном), `UI/MainMenu/SAPSMainMenuRoot.cpp` (галерея
  AUTHORED WORLDS на месте карточки SINGLE GAME плана «Обсерватория»; общая зона — handoff), `Core/Model/
  GeneratedWorld.*` (сериализация модели в `world.json` — тот же формат, что сейв мира, без игрового состояния),
  `Gameplay/Civilizations/APSCivilizationRuntimeManifest.cpp` (id + версия пресета в хеше), новый
  `Core/Worlds/APSAuthoredWorlds.h/.cpp` (plain C++: каталог пресетов из `Content/APS/APS_ALPHA/Worlds/Authored/
  <id>/{world.json, card.json, thumb.png}`, ручки пресета: гарантированный сосед, плотность сайтов Древних,
  множители правил), консольная команда `aps.Worlds.ExportAuthored <id>` (модель + кадры трёх уровней через
  существующие снимки меню `APSGenerationMenuShots.cpp`).
- **Что:** W0 — ручная карта как сейчас; W1…W10 — по документу, сначала шесть ✔ (W1, W2, W4, W5, W7, W8).
- **Выключатель:** `aps.Worlds.Authored` (0 — галерея не показывается).
- **Сейв:** мир сохраняется как обычный сгенерированный + id пресета; старые сейвы без id.
- **Тесты:** `APS.Worlds.Authored.<id>` — золотой состав (звёзды, число и типы планет, дом) по каждому пресету;
  `.LocksRespectRoll`.
- **Приёмка:** кадры со старта, системы и галактики на каждый пресет; старт на поверхности офскрин; FPS против
  базы; Rio смотрит каждый мир.
- **Зависит от:** ничего из лестницы; W3/W6/W10 — от чёрной дыры, опции положения дома, туманностей.

### T-22 … T-26 · REAL SKY (Rio 09.10; отдельная линия, см. [REAL_UNIVERSE_MILKY_WAY_2026-10.md](REAL_UNIVERSE_MILKY_WAY_2026-10.md) §8)

- **T-22 Спайк реального неба.** HYG → бинарная таблица → записи каталога со статусом REAL вокруг Солнца в
  галактических координатах (Солнце в нуле); GPU-точки на настоящих местах; имена на F10 и в навигации; REAL
  DISTANCES. Файлы: `Core/Rendering/APSGalaxyGpuStars.*`, каталог (`FGalaxyCatalogDescriptor`), `Gameplay/Expansion/
  APSStarSystems.*` (имена, записи `G<n>`), новый `Core/Sky/APSRealSky.*` (plain C++, загрузка таблицы). CVar
  `aps.Sky.Real`. Приёмка: Альфа Центавра 4,37 св. года, Сириус 8,6, Барнард 6,0 — долететь; кадры; FPS против базы.
  Отвечает на вопрос, нужна ли переделка адресации (октодерево) сразу.
- **T-23 SOL · HOME.** Таблица тел Солнечной системы; орбиты с эксцентриситетом и наклоном (проверить `FOrbitInfo`/
  модель орбит); вращение тел и сутки (новая фича: всё на планете едет с ней); карты высот Луна → Марс → Земля через
  планетарную карту высот WorldScape (первый тест — Луна); текстуры, ночные огни Земли, облака; кольца Сатурна и
  пояс астероидов (контент-план A9/D4); старт на Земле; пресет W11 в AUTHORED WORLDS; маршрут SPACE TRIPS «GRAND
  TOUR». Общие зоны: `AstroGenerator`, `PlanetarySurfaceGenerator`, WorldScape — узкие handoff'ы.
- **T-24 Ближние звёзды и экзопланеты.** GCNS (100 пк), NASA Exoplanet Archive → системы из таблиц у звёзд с
  известными планетами (TRAPPIST-1 обязательно), остальное генератор по классу; маршрут «NEIGHBOURS».
- **T-25 Млечный Путь.** Морфология на настоящем масштабе (бар, 4 рукава + Местный, балдж, диск, гало, Солнце в
  26 000 св. лет от центра), октодерево ячеек с сидом на ячейку и адресацией по месту (= фаза 4), слой A в своих
  ячейках, туманности каталога через свечение GPU-плагина, пылевые полосы, Sgr A*. Это самая большая карточка плана.
- **T-26 Местная группа.** M31, M33, Магеллановы Облака на своих местах; прыжки между галактиками (EXODUS, T-15).
- Все пять — за выключателями, кадры и FPS против базы, режим GENERATED не меняется.

---

## 4. Тексты журнала и описаний (EN, как весь UI)

Категория журнала `Origin`. Фигурные скобки — подстановки из мира.

### Описания происхождений (экран NEW WORLD)

- **ARK · LANDING** — "The ark is down. It will never fly again. Power it, walk out, and make this ground yours."
- **ADRIFT · THE LONG NIGHT** — "Reactor at eleven percent and no surface on record. Find a world before the lights go."
- **UNDER THE RING · THE RING** — "A moon under a giant, and over the horizon a ring that is not yours. Its tower has stairs the right size for you."

### ARK — первый час

| Шаг (T-10) | Текст |
|---|---|
| ArkAwake | "ARK — power 4 %. Drive: dead. Crew: one. One console answers." |
| ArkPowered | "Main bus restored. Lights in the bridge and the hold. The hull is cold but whole." |
| ConsoleOpened | "Atmosphere breathable. Gravity {g} g. Stores: {metals} metals, {volatiles} volatiles, {energy} energy. First task: deploy the solar array." |
| FirstExit | "Ramp down. Nobody has stood on this ground before." |
| SolarArrayBuilt | "Solar array online: +3 energy per minute. Energy is the only currency we have." |
| HabitatBuilt | "Habitat sealed. A place to sleep that is not the hold." |
| CommsMastBuilt | "Comms mast raised. The deep radar wakes." |
| SignalReceived | (существующий текст Древних) "SIGNAL — {home}: the colony's deep radar draws {size} of straight edges at {lat}, {lon}, too regular for rock." |
| FabBuilt | "Fabricator running. First job: a rover. We stop walking today." |
| RES_METALS | "First ore in the bunker. Metals: +6 per minute." |
| MonumentReached | (Echoes 1, существующий) |
| MonumentStudied | (Echoes 3, существующий) + "The Ark's fabricator can build a launch yard." |
| LaunchYardBuilt | "Launch yard complete. The Ark will never fly again; something else will." |
| ShipLaunched | "Hull complete on the pad. Class S. She has no name yet." |
| TakeoffCommitted | "Lift-off. Below: one ramp, four modules, a mast. Home." |
| OrbitReached | "Orbit. The planet turns under us, and it is ours." |
| HullSignal | (Quiet Hull 1, существующий) |
| RES_VOLATILES | "Volatiles flowing from the giant. Ships can be fuelled now." |
| RES_RESEARCH | "The first study is logged. Research is a stock now, and the Builders pay in it." |
| HullWake (K 1,9) | "The hull answers the colony's relay. Its drive is spooling. It remembers a destination." |

### ДОЛГОЕ ПАДЕНИЕ

| Шаг | Текст |
|---|---|
| THE OLD COURSE | "The hull is under way and we are aboard. The bridge is open. Go and look." |
| J-событие | "The sky moves. Home is a point among points." |
| THE FALL | "Something ahead opened an eye. The drive is gone. We are falling." |
| A COLD DECK | "Down. The hull is broken on a world we do not know. One section still has power." |
| THE PATH | "Stones with lights in them lead away from the wreck. The Builders walked here." |
| THE ALTAR | "A ring on its edge, half in the ground. Under its centre, a table of black stone." |
| THE DRIVE | "The ring is lit. Its twin opened where the hull used to hang above home. The ship needs a drive; the wreck has one." |
| THROUGH | "Through the ring and home. We know the way back now, and the way out." |
| RES_INFLUENCE | "A beacon stands in a system that is not ours. Influence is a stock now." |

### ADRIFT — первые шаги

| Шаг | Текст |
|---|---|
| Wake | "Drift. Reactor 11 %. No surface on record." |
| WorldSurveyed (не пригоден) | "{world}: {reason}. Not here." (reasons: NO ATMOSPHERE / FROZEN / NO GROUND) |
| DerelictFound | "A probe with our markings, dead for years. Someone came this way before us." |
| WorldSurveyed (пригоден) | "{world}: air we can breathe. A dead station in its orbit." |
| Landed | "Down. The ship is the Ark now." |
| StationDocked | "The station holds. Someone kept it warm for us." |
| StationRevived | "Reactor up. Section by section the lights come back. We have a station and a yard." |

### EXODUS — первые шаги (описание на экране: **EXODUS · THE LONG VOYAGE** — "Hundreds of generations between the galaxies, scanning every star for a world that breathes. Nothing yet.")

| Шаг | Текст |
|---|---|
| Wake | "Generation {n}. {pop} souls aboard. Reserves: {years} years. No habitable world on record." |
| GalaxyScanned | "{count} systems scanned. Rock, ice, gas, fire. Nothing that breathes." |
| Jump | "Jump {k}. Another galaxy's stars fill the sky. The elders remember no sky but this hull." |
| ReservesLow | "Reserves: {years} years. The council speaks of staying wherever we are." |
| TheFind | "Signal. One world. Liquid water, an atmosphere we can breathe, and lights on the night side that are not ours." |
| Arrival | "We have arrived. The ship will stay in orbit; it has no legs for ground. A lander goes down." |
| Landed | "Ground. The first feet on soil in three hundred years." |
| Cradle | "The stone here was cut before we were born as a people. The Builders began here." |
| DriveBurnt | "The drive is spent. Whatever took us across the dark will not take us further. We build from here." |

### SPACE TRIPS — названия маршрутов и строки путеводителя (образцы)

Описание режима на экране: **SPACE TRIPS** — "A ship, a route, and nothing to do but look. Pick a tour or chart
your own."

Тематические маршруты: GIANTS AND RINGS · ICE WORLDS · LAVA AT DAWN · BINARY SUNRISE · THE CORE · THE EDGE ·
THE BUILDERS' ROAD · HOME SYSTEM, SLOWLY.

| Остановка | Строка фактов (из генератора) | Строка настроения |
|---|---|---|
| газовый гигант | "{name}: gas giant, radius {r} km, {moons} moons, ring system." | "Bands of weather older than any city." |
| ледяной мир | "{name}: frozen world, surface {t} °C, thin atmosphere." | "Nothing moves here but the light." |
| лавовый мир | "{name}: molten surface, {t} °C, no atmosphere." | "Approach from the night side; the ground is its own sunrise." |
| живой мир | "{name}: temperate, breathable, oceans and continents." | "Somebody could live here. Nobody does." |
| двойная звезда | "{name}: binary pair, separation {au} AU." | "Two shadows on the hull." |
| ядро галактики | "{count} stars within a light-year." | "No night here." |
| край галактики | "The last star of the disk. Beyond it, the halo." | "Turn around: the whole galaxy, edge on." |
| сайт Древних | (описание сайта из `APSAncientsSites`) | "Whoever cut this knew our sun." |

### UNDER THE RING — первые шаги

| Шаг | Текст |
|---|---|
| Wake | "Ark down on {moon}. {giant} fills the sky. Over the horizon: a thin arc that is not a cloud." |
| TowerSighted | "A tower on the equator, six kilometres of black stone. Its stairs are the right size for us." |
| TowerPowered | "The tower answers. A cabin waits." |
| RingReached | "A section of the ring breathes again. From here the moon is small." |
| RES_VOLATILES | "The giant gives. Volatiles first; metal will come from the moon." |

---

## 5. Что нужно до старта W2 (не код)

1. Решения §10 концепта, ещё открытые: 4 (история Строителей обязательна в ORIGIN — предлагаю да), 5 (финал в EA —
   да), 6 (темп — оставить), 7 (ручка OTHERS с «later» — да), 10 (показ K как «K 1.42 · INTERPLANETARY» — да),
   12 (настоящие ворота — для EA квестовая пара обязательна, своя стройка ворот как переход — после). Если Rio не
   против, берём эти значения по умолчанию и не ждём.
2. Три взгляда в кадре (Rio или офскрин): Древние (`aps.Ancients.List/Teleport`), мегаструктуры (`aps.Mega.Raise`),
   станция R2 — от них зависят T-09, T-18 и выбор станции для T-17.
3. T-13 замер флота — до правил ступени 3.
4. Долг по git до пуша (контент-план §6): LFS на M5-меши, untracked `ClassL` и `Common\Lighting`.

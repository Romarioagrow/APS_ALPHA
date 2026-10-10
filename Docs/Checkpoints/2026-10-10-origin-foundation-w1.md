# 10.10 — фундамент ORIGIN, лестница, авторские миры, SPACE TRIPS (волна W1), SOURCE READY

Claude (сессия «Аналитика»), 10.10 00:20–02:50. Рабочее дерево `dev-3` (HEAD `9e64ed5f`). Не собрано, не
запускалось: cl-check всех затронутых TU по rsp игрового target — OK, без ошибок и без предупреждений в новом коде.
Ничего не закоммичено (ветку `content` создаю при первом коммите после проверки Rio). Карточки:
[ORIGIN_TASK_CARDS](../Design/ORIGIN_TASK_CARDS_2026-10.md) T-01…T-04, T-06…T-09, T-20, T-21; план —
[IMPLEMENTATION_PLAN](../Design/IMPLEMENTATION_PLAN_2026-10.md) волна 1. Сборка 0.6.4.4 Claude flight (02:04) уже
включала первый набор (00:39); второй набор и SPACE TRIPS пойдут в следующую.

Главный инвариант: всё новое читается только когда мир просит (`Reach = Ladder` для ORIGIN, `Goals = None` для
SPACE TRIPS). SANDBOX и старые сейвы ведут себя как сегодня. Видимые в SANDBOX отличия — ровно два: тип LAUNCH YARD в
каталоге построек и модуль FABRICATION BAY в терминале колонии (без дохода при `aps.Colony.ModuleYields=0`).

## Набор 1 (00:20–00:39) — правила мира, происхождения, экран NEW WORLD, авторские миры

| Карточка | Файлы | Суть |
|---|---|---|
| T-01 правила мира | `Core/Model/APSWorldRules.h/.cpp` (новые), `Core/Model/GeneratedWorld.h` (+8 `uint8` UPROPERTY `Rules*`, `FString AuthoredWorldId`, `uint8 AuthoredLocks` в конце класса) | Режим / происхождение / дальность / знание / цели / ресурсы / другие / небо живут на `UGeneratedWorld` байтами: ноль = сегодняшняя игра и любой старый сейв; едут в любой маршрут и в снимок мира. `APSWorldRules::Of / OfGame / Write / ApplyPreset / IsLadder / IsTrip / Describe`. CVar `aps.Origin.Enable` (1). В хеши ничего не добавлено: `Describe` пуст для сегодняшних правил. |
| T-02 происхождения как данные | `Gameplay/Origins/APSOrigins.h/.cpp` (новые) | Четыре записи (ARK, ADRIFT, UNDER THE RING, EXODUS = `bAvailable false`): имя, подзаголовок, описание (EN), место старта, порядок открытия ресурсов, id маршрута онбординга. Токены лестницы `APSProgressionTokens` (LAUNCH, STELLAR_DRIVE, GALAXY_HULLS, VEHICLES_*, RES_*, TAB_*). |
| T-03 (часть) | `Gameplay/Expansion/APSMissions.h/.cpp` | `FAPSMissionBoard::Unlock(FName)` — токен в сохраняемый набор `Unlocked` (сейв цивилизации v5). |
| T-04 экран NEW WORLD | `UI/MainMenu/SAPSMainMenuRoot.h/.cpp` | Ряд MODE (ORIGIN / SANDBOX / SPACE TRIPS) и, для ORIGIN, чипы происхождений с описанием; видны для путей CIVILIZATION и SPACE. SPACE TRIPS переключает путь на SPACE, ORIGIN/SANDBOX — на CIVILIZATION; выбор происхождения ставит место старта пилота. |
| T-21 авторские миры | `Core/Worlds/APSAuthoredWorlds.h/.cpp` (новые), `UI/MainMenu/WorldGenerationViewModel.cpp` (замки), `SAPSMainMenuRoot` (сетка), `APS_ALPHA.Build.cs` (+Json, JsonUtilities), `Config/DefaultGame.ini` (+`DirectoriesToAlwaysStageAsUFS` `APS/APS_ALPHA/Worlds`), `Content/APS/APS_ALPHA/Worlds/Authored/<id>/{card.json, world.json}` ×10 | Каталог карточек из JSON; `Apply` кладёт `world.json` на модель отражением (`FJsonObjectConverter`), пишет id и замки, место старта; `Export` (`aps.Worlds.ExportAuthored <id>`) пишет модель меню в `world.json`; `aps.Worlds.List`. Замки по уровням `APSWorldRoll::EScope`: REGENERATE на закрытом уровне отказывает со статусом в превью. В SINGLE GAME сетка: ручная карта + 10 миров + «MORE WORLDS», три колонки, прокрутка; клик применяет пресет и открывает экран генерации. |

Десять пресетов: tidal, two_suns, the_well, ten_thousand_islands, ember, the_cradle, the_stream, the_wheel, hourglass,
the_nebula — поля из [AUTHORED_WORLDS](../Design/AUTHORED_WORLDS_2026-10.md) §3. Все с замком `all`. Точных правок
звёзд/тел в пресетах пока нет: Rio крутит мир в меню → `aps.Worlds.ExportAuthored <id>` перезапишет `world.json`.

## Набор 2 (02:00–02:25) — читатели токенов, доход модулей, LAUNCH YARD, ресурсы по одному, награды Древних

| Карточка | Файлы | Суть |
|---|---|---|
| T-03 читатели токенов | `Gameplay/Fleet/APSFleetCommand.h/.cpp`, `Gameplay/Vehicles/APSGroundVehicles.cpp`, `Gameplay/Expansion/APSInfrastructureCatalog.h/.cpp`, `APSInfrastructure.cpp` | Верфь (`GetShipyardOptions` / `OrderShip`): XXS/XS/S нужен LAUNCH, M — STELLAR_DRIVE, L и выше — GALAXY_HULLS; в лестнице корпус стоит металлов и энергии по классу (60/20 … 4000/1200), списывается при заказе; у `FAPSShipyardOption` новые поля `Refusal`/`CostText` для панели флота (панель их пока не показывает — зона UI). Техника: маска меню ∧ токены VEHICLES_* (ровер открывает FABRICATION BAY, ховер и дрон — LAUNCH YARD); спавнер сам доносит технику после открытия. Каталог: поле `RequiresToken` → отказ «LOCKED: NEEDS …». |
| T-06 модули с доходом | `Gameplay/Colony/APSColonyModuleCatalogue.h/.cpp`, `APSColonyConstructionSubsystem.h/.cpp` | `YieldPerMinute[5]` и `UnlocksToken` у модуля; доходы: Habitat I+1, Greenhouse V+1, SolarArray E+3, CommsMast R+1, SolarWing E+4, CargoPods M+1, NavBeacon I+1; новый модуль **FAB — FABRICATION BAY** (поверхность, 18 с, M+2, открывает ровер; корпус из примитивов: плита, стены, крыша с балкой, принтер с экраном). `CountBuilt(id)`, хук `OnModuleStood` после постройки/восстановления → пересчёт доходов, грант токена. Считаются в лестнице; в SANDBOX только при `aps.Colony.ModuleYields=1` (по умолчанию 0). |
| T-07 LaunchYard | `APSInfrastructureCatalog.cpp` | **LAUNCH YARD**: Industry, Surface, K0, L0, 60 с, 120 M + 40 E, `RequiresToken` LAUNCH, visual Shipyard (спавнится `ASpaceShipyard` → верфь флота, `GetShipyards` её видит; меш — орбитальной верфи, стоит на земле, до своего ассета). |
| T-08 ресурсы по одному | `APSInfrastructure.h/.cpp`, `APSOrigins.cpp` | `ClosedMask` по токенам RES_* (энергия открыта всегда): закрытый ресурс — домашний доход 0, доходы построек 0, стартовый запас 0; постройка с ценой или доходом в закрытом ресурсе отказывает «LOCKED: … NOT OPEN YET. RAISE A … FIRST», кроме открывающей (MiningOutpost → METALS, GasHarvester → VOLATILES, ResearchStation → RESEARCH, любая claims-постройка → INFLUENCE), которая платит без закрытой части (`EffectiveCost`). `Complete` → `Grant` токена + строка журнала (`APSProgressionTokens::Opened`). Маска пересчитывается при постройках, загрузке, открытии токена и раз в секунду (борд создаётся после инфраструктуры, сейв возвращает токены чуть позже). `Resources = Unlimited`: ничего не списывается; `Scarce`: ×0.5 запасы и доходы. `Spend`, `RefreshRates`, `IsResourceOpen` — публичные. |
| T-09 награды Древних | `Gameplay/Ancients/APSAncients.cpp` | В лестнице: сигнал монумента (Echoes) приходит только когда стоит CommsMast; конец цепочки Echoes → LAUNCH, QuietHull → STELLAR_DRIVE, последний сайт Road → GALAXY_HULLS (`Grant`, журнал Ancients). |
| Токены | `APSOrigins.h/.cpp` | `Title(token)`, `Opened(token)` (строки журнала EN), `Grant(World, token, category)` — один вход для всех наград. Консоль: `aps.Origin.Grant <token|all>`, `aps.Origin.Tokens`. |
| Тесты | `Tests/APSWorldRulesTests.cpp` | +`APS.Origin.Ladder.CatalogueTokens`: LAUNCH YARD и его токен, открыватели ресурсов, у каждого закрытого ресурса есть открыватель, у токенов есть заголовок и строка, FAB открывает ровер. |

## SPACE TRIPS runtime (02:25–02:50) — T-20

| Файлы | Суть |
|---|---|
| `Gameplay/Trips/APSSpaceTrips.h/.cpp` (новые), `Gameplay/Fleet/APSFleetCommandSubsystem.cpp` (+include, +4 строки тика рядом с `APSGroundVehicles::Tick`) | Просыпается только в мире с `Goals = None` (`APSWorldRules::IsTrip`, CVar `aps.Trip.Enable`=1); в остальных мирах спит, команды консоли работают везде. **Маршруты** строятся из каталога звёзд (`FAPSStarSystems`) от текущего места корабля: HOME WATERS (миры своей звезды, все), THE NEAREST SUNS, RED AND COLD (M/K), BLUE AND WHITE (O/B/A), DOUBLES AND TRIPLES, THE DARK (NS/BH/PS), THE CROWDED SKIES (≥6 планет); по 3–4 остановки, порядок — ближайшая следующая; у чужой звезды показываются `aps.Trip.WorldsPerStop` (3) мира. **Экскурсовод**: к чужой звезде — `EngageStarDrive`, нос доворачивается на звезду со скоростью `aps.Trip.TurnRate` (15°/с) через `SetActorRotation` (как делает автопилот), привод сам гасится на входе в систему; ждём, пока материализатор поставит систему и планеты (до 60 с), затем к каждому миру штатный `EngageAutopilot`, у мира пауза `aps.Trip.HoldSeconds` (25 с) и строка путеводителя. Если привод отказывает (корпус меньше M) или нога длится дольше `aps.Trip.FoldAfterSeconds` (120 с) — «складываем расстояние» (`VisitForTest`, как `aps.Stars.Visit`) с честной строкой в журнале. Вмешательство пилота (поворот >2° при приводе, автопилот сброшен не у цели, другая цель) → PAUSED «You have the helm», `aps.Trip.Resume`. **Путеводитель** — журнал категории `Guide` (в терминале рендерится стилем по умолчанию, подпись GUIDE): строка звезды (спектр, число солнц и миров, «чудо» по типу звезды) и строка мира (тип, радиус, луны, «чудо» по типу планеты). **Фото**: `aps.Trip.Photo [name]` → `Saved/Screenshots/Trips/Trip_<name>_NN.png` без HUD. В мире SPACE TRIPS приветствие в журнале и автостарт HOME WATERS через 6 с после запуска двигателя (`aps.Trip.AutoStart`=1). Консоль: `aps.Trip.Routes / Start <id|n> / Next / Pause / Resume / Stop / Photo / Status`. |

## Что проверить в игре (после сборки)

1. NEW WORLD → CIVILIZATION: ряд MODE; ORIGIN показывает четыре чипа (EXODUS серый); SPACE TRIPS переключает карточку на SPACE. Лог `[APS.WorldRules] NEW WORLD mode …`.
2. NEW WORLD → SINGLE GAME: сетка «AUTHORED WORLDS · 11 WORLDS»; клик по TIDAL открывает генерацию с луной гиганта, REGENERATE на любом уровне пишет «TIDAL: THIS LEVEL IS AUTHORED AND LOCKED»; лог `[APS.Worlds] applied 'tidal' …`.
3. Консоль: `aps.Worlds.List`; после правки мира в меню `aps.Worlds.ExportAuthored tidal` → файл перезаписан.
4. SANDBOX и старые сейвы — ничего не меняется (тест `DefaultsAreToday`, хеши манифеста не трогались); доходы как раньше (`aps.Colony.ModuleYields` 0).
5. Автотесты `APS.Origin.*` — 5 штук.
6. ORIGIN (ARK), новый мир: запасы металлов/волатилов/исследований/влияния 0, энергия 150 (`aps.Origin.Tokens` → всё `-`). Постройка MINING OUTPOST (цена 40 M + 10 E) берёт только 10 E; по готовности журнал «METALS: the outpost's crushers run…», запас металлов начинает расти. GAS HARVESTER / RESEARCH STATION / SURVEY BEACON — то же для своих ресурсов. Любая другая постройка с металлами в цене до этого: «LOCKED: METALS NOT OPEN YET. RAISE A MINING OUTPOST FIRST.»
7. ORIGIN, верфь: заказ корпуса → «LOCKED: S HULLS NEED THE LAUNCH». `aps.Origin.Grant LAUNCH` (или цепочка монумента до конца) → заказ S берёт 160 M + 60 E; LAUNCH YARD строится на поверхности и появляется в списке верфей; после него в журнале THE HOVER / THE DRONE и техника выезжает.
8. ORIGIN, терминал колонии: FABRICATION BAY → по готовности «THE ROVER…», ровер появляется у базы (до этого техники нет вовсе, даже выбранной в меню).
9. ORIGIN, Древние: пока нет COMMS MAST — сигнал монумента не приходит; с мачтой — как раньше; конец цепочки монумента → «THE LAUNCH: …».
10. SPACE TRIPS: NEW WORLD → MODE SPACE TRIPS → старт в корабле; журнал «Welcome aboard…»; с запущенным двигателем через 6 с ROUTE HOME WATERS, корабль сам летит к мирам, у каждого стоит 25 с, в журнале строки GUIDE. `aps.Trip.Routes`, `aps.Trip.Start red` — звёздный привод, нос доворачивается на звезду (корпус M+; на меньшем — «складывает расстояние»). Повернуть руль → «You have the helm», `aps.Trip.Resume`. `aps.Trip.Photo` → файл в Saved/Screenshots/Trips.

## Не сделано (следующие волны)

Старт ARK (T-05: ковчег A-00, пустая поляна, ничего не построено), онбординг (T-10), вкладки по токенам TAB_* (T-11),
шкала K (T-12), UI: скрыть закрытые ресурсы в панелях запасов, показать `Refusal`/`CostText` на панели верфи, панель ROUTE
для SPACE TRIPS и стиль/глиф категории Guide (зона DEV UI), свой меш LAUNCH YARD и FAB (ассеты), привязки клавиш для
`aps.Trip.*` (зона контроллера).

## Общие зоны и риски

- Правки в `SAPSMainMenuRoot`, `WorldGenerationViewModel`, `GeneratedWorld.h`, `Build.cs`, `DefaultGame.ini`, `APSFleetCommand`,
  `APSFleetCommandSubsystem`, `APSInfrastructure*`, `APSColony*`, `APSAncients`, `APSGroundVehicles` — объявлены в окне
  (строки 00:36, 00:39, 02:16, 02:24, 02:29). `Pawns/Spaceships` не трогал.
- Новые UPROPERTY в `GeneratedWorld.h` — UBT 02:03 уже прогнал UHT (сборка 0.6.4.4 собралась с ними).
- `DirectoriesToAlwaysStageAsUFS` нужен, чтобы JSON пресетов попали в упакованный билд; в редакторе читаются из `Content`.
- Лестница: инфраструктура создаётся раньше борда заданий (`UAPSFleetCommandSubsystem::Initialize`), поэтому первую
  секунду все RES_* считаются закрытыми; стартовые запасы обнуляются только в конструкторе, загрузка кладёт сохранённые
  запасы поверх — потерь нет. `aps.Origin.Grant all` — быстрый обход для тестов.
- SPACE TRIPS: доворот носа при звёздном приводе не проверялся в кадре (привод может гаснуть на промежуточных системах —
  перезапуск каждые 3 с, через 120 с «складывание»); при входе на большой скорости материализатор может не поставить
  систему — таймаут 60 с и следующая остановка. Категория журнала `Guide` без своего глифа.
- Толщина `SBox.MaxDesiredHeight(720)` сетки авторских миров и три колонки — на глаз, Rio посмотрит на 4K.

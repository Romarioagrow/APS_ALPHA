# APOSFERA / APS_ALPHA — аудит состояния проекта

Дата: 2026-09-27. Основание: `dev-3`, HEAD до этого документа
`c97357eb2a9c192f8d57023b824ee5035e85171e`.

## 1. Область и пределы проверки

Это обзорный статический аудит всего проекта по основным подсистемам: структура,
архитектурные границы, конфигурация, зависимости, сохранения, интерфейс, gameplay,
визуальный pipeline, тестовый инвентарь и документы предыдущих итераций.
Проверены Git-инвентарь, ключевые интерфейсы/точки передачи данных, текущие настройки,
наличие внешних зависимостей и контрольной копии; сопоставлены датированные отчёты.

Это **не** построчный review всех исходников, полный аудит безопасности, cook,
проверка всех Blueprint-графов/asset references или новый игровой regression run.
Не выполнялись сборки, commandlet, bake, тесты в UE и съёмка кадров. Работал
пользовательский UnrealEditor PID 25480; его не закрывали и не модифицировали.
В этом проходе меняется только документация. Ниже отдельно указано, что подтверждено
кодом, сообщением пользователя или историческим отчётом.

## 2. Краткий итог

1. Рабочая основа существует: каноническая модель генерации, отдельный preview,
   generated/authored маршруты, WorldScape-поверхность, атмосферы, звёздный renderer,
   управление/корабли, Civilization, save v3, UI и набор automation-тестов.
2. Принятый пользователем визуальный результат сохранён в `67fff3b4`.
   Не нужно заново исправлять принятую лаву или звёзды по старому сентябрьскому отчёту.
3. Позднейший water-depth overlay-v21 — сохранённое исследование, не production.
   Наличие его коммитов не означает, что новые водные шейдеры установлены в игре.
4. Git не представляет всю среду: WorldScape вне repo, AtmoScape игнорируется,
   а конфигурация ссылается на существующий локально, но не tracked legacy GameMode.
5. После переноса рабочий архив находится на F:. Новый build/run приватного стенда
   после переноса ещё не доказан; не путать сохранность файлов с переносимостью среды.
6. Тесты и изолированные domain-контракты нельзя приравнивать к полной готовности
   игры, packaged build или сквозному сохранению всех gameplay-подсистем.

## 3. Каталоги, версии, восстановление

| Назначение | Текущий путь / состояние |
| --- | --- |
| Основной проект | `F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/APS_ALPHA.uproject` |
| Git | Основной repo, `dev-3`; при начале аудита clean |
| Рабочий архив | `F:/ChatGPT/APOSFERA` — отдельный Git-каталог, до аудита без первого коммита, много untracked исследований |
| Приватный водный стенд | `F:/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-gameplay` |
| Движок | `C:/Program Files/Epic Games/UE/UE_5.4` |
| WorldScape | `C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4` |
| AtmoScape | `Plugins/AtmoScape` в основном проекте |
| Внешняя контрольная копия | `Saved/AcceptedVisualCheckpoint_20260927` основного проекта |

`APS_ALPHA.uproject`: UE 5.4, один runtime-модуль APS_ALPHA. Installed WorldScape
descriptor: VersionName 1.1, EngineVersion 5.4.0; AtmoScape: VersionName 3.0.
Это значения локальных descriptors, не проверка новейших marketplace-релизов.

По tracked-инвентарю на исходном HEAD: 539 файлов Source, 1267 Content, 5 Config,
52 Docs, 24 Tools, 106 output. Числа — файлы, не количество независимых функций.
Tracked-файлов Plugins нет. `.gitignore` исключает Plugins, Saved, Binaries,
Intermediate, DDC и Content вне исключения для `Content/APS`.
`.gitattributes` содержит два адресных LFS-правила, а не общую политику для uasset.
Изменять LFS/историю или добавлять весь Content в рамках этого аудита не следует.

Контрольная копия описана в [принятом checkpoint](../Checkpoints/2026-09-27-worldscape-accepted.md).
Текущий SHA256 её manifest.json повторно проверен:
`AAE64298A3ACB4D612FB42882B0D4E07E231DB86681FA2C9C0880E7610659C89`.
Заявленные в checkpoint 971 файлов / 3,349,316,804 байт — результат предыдущей
проверки; все файлы повторно не хэшировались в этом аудите. Копия содержит
WorldScape, AtmoScape и editor runtime, не является доказательством полного backup
всех ignored legacy assets, пользовательских saves или несохранённого Editor state.

## 4. Карта архитектуры

Все пути Source ниже относительно `Source/APS_ALPHA`.

| Подсистема | Точки входа | Обнаруженное устройство и граница |
| --- | --- | --- |
| Модель мира | `Core/Model/GeneratedWorld.*`, `APSCanonicalStellarDataset.h`, `SpawnParameters.*` | Модель и recipe отделены от отображающих акторов; ID/seed нельзя переизобретать в UI/Quest |
| Генератор / preview | `Generation/AstroGenerator.*`, `UI/MainMenu/WorldGenerationViewModel.*`, `SWorldGenerationPanel.*` | Редактирование модели, preview и commit gameplay; крупная общая зона для нескольких задач |
| Передача в gameplay | `WorldGenerationViewModel.cpp::CommitAndOpenLevel`, `Core/Instances/MainGameplayInstance.*`, `Core/GameModes/GravityGameModeBase.*` | GeneratedWorld и SpawnParameters дублируются в GameInstance-owned состояние до travel |
| Authored SinglePlay | `Core/Loading/APSAuthoredLevelLaunchSubsystem.*`, `APSAuthoredLevelLaunchGate.h` | Async preload, наблюдение готовности, cancel/failure и отдельный authored-флаг |
| Поверхность | `Generation/PlanetarySurfaceGenerator.*`, `PlanetarySurfaceGeneratorStreaming.cpp`, `APSWorldScapePlanetNoise.*` | WorldScape geometry/LOD/noise/collision, а не Unreal Landscape |
| Профили / жидкости | `Core/Planetary/APSPlanetSurfaceProfile.*`, `APSSharedGeneratedLiquidMaterial.h`, `Editor/APSPlanetSurfaceAssetCommandlet.cpp` | Общие профили и выбор материалов; изменение C++/HLSL не тождественно bake сохранённого material asset |
| Streaming / placement | `Core/World/APSPlanetSurfaceTransitionSubsystem.*`, `APSPlanetSurfacePlacementResolver.*`, `APSPlanetEnvironmentStreamingSubsystem.*` | Поверхностные переходы, readiness и стабильные placement anchors; не новая модель мира |
| Атмосферы | `Actors/Planetary/PlanetAtmosphere.*`, `Generation/APSAtmosphereGeneration.h`, `Core/Structs/PlanetAtmosphereModel.h`, AtmoScape | Физическая модель и presentation/light binding; default Opacity и MultiScattering в модели оба 1.0 |
| Звёзды | `Core/Rendering/APSStellarVisualSubsystem.*`, `APSGameplayStellarProjection.h`, `APSCanonicalStellarProjection.h` | Общий каталог / физическая проекция / оптика и отдельные presentation-пути; не брать compressed preview coordinates за физические |
| Сохранения | `Core/Saves/GameSave.h`, `APSWorldSaveSnapshot.*`, `Core/Controllers/GravityPlayerController.cpp` | SaveFormatVersion 3, world/spawn snapshots, actor/manifest/player state, legacy migration |
| Персонаж / корабли | `Pawns/Characters/CustomGravityCharacter.*`, `Pawns/Spaceships/Spaceship.*`, `Spacecraft.*`, `ShipNavigationComponent.*` | Гравитация, possession, движение/flight/navigation; не переоткрывать принятый character baseline без задачи |
| Civilization | `Gameplay/Civilizations/APSCivilizationRuntimeManifest.*`, `APSCivilizationMaterializationSubsystem.*`, `APSCivilizationIdentityComponent.*` | StableEntityId, manifest, starter materialization и actor-ready lifecycle |
| Interaction / Production | `Gameplay/Interaction/*`, `Gameplay/Production/*` | Запросы/проверка действий, production runtime/events и persistence API; сквозную интеграцию проверять отдельно |
| Quest / Onboarding | `Gameplay/Quests/*`, `Gameplay/Quests/Adapters/*` | Детерминированный runtime, события, typed identity, save API и presentation session; не автор выполнения gameplay-действий |
| UI | `UI/MainMenu/SAPSMainMenuRoot.*`, `UI/StrategicMap/*`, `UI/CivilizationMenu/*` | Slate menu/generation/map плюс civilization widgets; визуальная приёмка не выводится из контрактных тестов |

### Три разных маршрута

```text
Редактирование / preview
  GeneratedWorld -> ViewModel -> временное визуальное представление
  Commit -> копия модели/recipe в MainGameplayInstance -> generated gameplay

Start Single Game
  authored launch subsystem -> preload/readiness -> authored flag -> сохранённая карта
  GravityGameModeBase не запускает generated-ветку при authored flag

Existing Worlds
  выбранный save -> Restore модели/recipe -> replay hierarchy -> actor/player restoration
```

Нельзя объединять эти ветки по совпадению названия планеты/актора. Для нового мира
`CommitAndOpenLevel` явно сбрасывает loading/authored и ставит civilization-флаг
по маршруту. Сохранять эту изоляцию при любой работе с меню и запуском.

Authored gate в текущем коде имеет default deadlines 600 секунд на подготовку и
60 секунд на opening, две последовательные ready-проверки. Это подтверждает
наличие ограниченного state machine, **не доказывает** отсутствие блокирующей
работы внутри движка или отсутствие стартового фриза на каждой машине.

## 5. Build, config, зависимости

Game и Editor targets присутствуют; оба используют BuildSettings V5 / include
order Unreal5_4. Сохранены workaround-настройки `bUndefinedIdentifierErrors=false`
и `__has_feature(x)=0`; не удалять их как косметику без отдельной проверки toolchain.

Build.cs зависит от WorldScapeCore/Common/Noise/Volume/Foliages, AtmoScape,
ProceduralMeshComponent, DirGravity, EnhancedInput, Slate/UMG/MVVM, AssetRegistry,
RenderCore. Editor-ветка добавляет WorldScapeEditor, UnrealEd, AssetTools,
MaterialEditor, RHI. В uproject также включены Water, SunPosition и editor ModelingTools.
OnlineSubsystem в Build.cs закомментирован; сетевую готовность это не подтверждает.

DefaultEngine.ini: стартовая main-menu map, DX12/SM6, Lumen/VSM/TSR-конфигурация,
отключённый ray tracing, фиксированная exposure policy; `r.VelocityOutputPass=2`
и `r.UseVisibilityOctree=0` сохранены. Настройки не измерялись новым runtime run.

`GlobalDefaultGameMode` указывает на
`/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/BLUEPRINTS/GM/APS_GM_Single_Gravity`.
Соответствующий uasset существует локально, отсутствует в tracked-инвентаре и
попадает под ignore. Это конкретная внешняя content-зависимость для воспроизводимости.
Не менять reference на другой GameMode только ради устранения записи в аудите.

В DDC config есть writable FileSystem backend с `UE-LocalDataCachePath` override.
Путь по умолчанию использует `%ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache`:
перенос рабочего архива на F: сам по себе не переносит engine/user DDC.
Глобальные настройки DDC не менялись. Для будущего частного стенда сначала
проверить реальный writable cache path и достаточный объём диска.

## 6. Реальный визуальный статус

**Пользовательская приёмка:** в сохранённом checkpoint 27 сентября отмечены хорошие
планеты с орбиты/поверхности, исправленная лава, атмосферы, звёзды/кластеры без
замеченного мерцания, переходы, существенно меньший тайлинг и около 120 FPS.
Это важнее старой гипотезы о сломанной лаве, но не заменяет family/seed matrix.
Стиль — реалистичность по предоставленным Elite Dangerous references, не копирование
географии и не замена WorldScape.

**Следующая полировка по запросу:** Temperate/Terrestrial, Ocean/Living/Forest/
Oasis/Savanna; глубина воды, менее искусственная граница суша/вода, небольшие
угловатые переходы и сдержанная детализация. Защищать уже принятую палитру/континенты.

**Отдельное исследование:** [water-depth integration](../../Tools/Diagnostics/WorldScapeWaterDepthIntegration/README.md)
содержит 19-файловый overlay-v21, manifest и ссылку на приватный host. HEAD `c97357eb`
сохраняет grounded shore-walking evidence и ограничения paired timing.
Сохранённые отчёты Water/Terrestrial относятся к частному стенду и конкретным
условиям, не к новой production-установке. У candidate оставались длинные streaks
блика и резкая береговая граница; zero-shimmer/global visual PASS не заявляется.
Нельзя сравнивать пользовательские 120 FPS с offscreen-замерами без одинаковых условий.

## 7. Saves, gameplay и границы готовности

Сейчас GameSave содержит actor records, civilization manifest, world model bytes,
точные spawn parameters, player pawn class/transform/control rotation и legacy
summary. `APSWorldSaveSnapshot::LatestSaveFormatVersion` и GameSave default равны 3.
При restore сначала восстанавливается/мигрируется модель; флаги replay/readiness
нужны для порядка восстановления hierarchy и акторов. Нельзя просто повторно
рандомизировать мир из видимого summary и считать его тем же сохранением.

Quest предоставляет ExportQuestSaveData/RestoreQuestSaveData; Production —
Export/RestorePersistenceState. В просмотренных GameSave.h и
GravityPlayerController.cpp нет явной записи этих payload. Поиск C++ вызовов
Quest save API вне реализаций/тестов также не показал подключения к world save.
**Вывод:** domain save API есть; сквозное сохранение quest/production через
Existing Worlds не доказано. Blueprint wiring не проверялось, поэтому это
интеграционный риск, а не утверждение, что никакого возможного save-пути нет.

В старых APS-80/81 ledger некоторые строки относятся к состоянию до реализации.
Код runtime/adapters/tests уже присутствует. Нельзя объявить всю подсистему ни
отсутствующей по старой строке, ни завершённой по наличию файлов.

## 8. Проверки: инвентарь и пробелы

В `Source/APS_ALPHA/Tests` найдено 123 строки регистрации automation-макросов
в 50 translation units. Complex tests могут раскрывать несколько случаев;
это не число выполненных/успешных тестов. В этом аудите запущено **0 UE-тестов**.

- Identity/generation: determinism, canonical projection, home continuity,
  orbit layout, preview edits/display names, presets, habitability.
- Поверхность/визуал: profile/placement/transition, WorldScape payload/foliage,
  shared liquid selection, atmosphere presentation/light binding, water sampling.
- Runtime/rendered: authored route, generated handoff, preview/stellar/planet
  gallery/refinement, UI contrast/style. Название Rendered само по себе не PASS.
- Gameplay: character gravity, vehicle control/flight, Civilization manifest/save,
  interaction/production contracts, quest framework/adapters/onboarding.
- В tracked-инвентаре не найдены `.github` workflows, GitLab CI, Azure Pipelines
  или Jenkinsfile. Внешняя CI могла быть настроена отдельно — это не проверялось.
- Shipping/package/cook, длительные перелёты, все planet families/seeds, полный
  пользовательский save/load цикл и стабильность на разных GPU сейчас не прогонялись.

## 9. Приоритетные находки

| ID / приоритет | Доказательство и риск | Следующее действие |
| --- | --- | --- |
| A01 / высокий | WorldScape вне Git, AtmoScape игнорируется, legacy GameMode локальный ignored asset | Перед переносом/восстановлением составить полный dependency manifest; существующую внешнюю копию не удалять. Clean-machine restore проверять отдельно |
| A02 / высокий | Есть quest/production persistence API, но world-save binding не подтверждён просмотренным native save path | Отдельная задача владельцу saves/gameplay: проверить Blueprint/native wiring, затем round-trip с реальным slot и отсутствие duplicate rewards/jobs |
| A03 / средний | Старые отчёты с C:-путями и датированными словами «сломано/готово» противоречат более поздней приёмке | В этом проходе добавлены текущий индекс, вводная и historical banners; старые доказательства не переписаны |
| A04 / средний | Приватный host переехал; Content сейчас обычная папка, не junction | Не удалять/пересоздавать Content вслепую. До следующего запуска проверить абсолютные пути скриптов/intermediates и отдельный writable DDC |
| A05 / средний | Тестовый набор есть, общего свежего regression/package отчёта нет | Согласовать одно тестовое окно и запускать по затронутым маршрутам, не весь набор наугад |
| A06 / средний | Water-depth candidate — не production, с незакрытыми визуальными/временными ограничениями | Продолжать изолированный bounded A/B; не продвигать до same-body visual/performance acceptance |
| A07 / организационный | Большие shared файлы AstroGenerator, menu root, view model, material commandlet и крупные smoke probes | Явно согласовывать файлы Claude/Codex; избегать общего форматирования/рефакторинга одновременно с feature work |
| A08 / инфраструктура | DDC и сохранённая настройка рабочего каталога инструмента не следуют автоматически за переносом архива | Указывать F:-workdir явно; отдельно проверить настройки хоста. Этот аудит не менял глобальные настройки приложения |

Приоритеты отражают риск потери/невоспроизводимости и интеграции, а не утверждение
о воспроизведённом crash. Исправление кода по этим пунктам не выполнялось.

## 10. Практическая очередь и критерии приёмки

1. **Вход Claude:** открыть основной repo, прочитать CLAUDE.md и этот аудит;
   получить отдельную задачу, согласовать только реально пересекающиеся файлы.
2. **Восстановимость:** сохранить accepted checkpoint + внешние плагины + legacy
   dependencies + нужные saves; проверить на отдельном target, не поверх живого Editor.
3. **Persistence integration:** подтвердить реальные save/load bindings quest и
   production; не менять schema без миграции и повторной загрузки старого сохранения.
4. **Визуальная полировка:** одна ограниченная гипотеза/семейство за проход,
   та же планета/seed/camera/settings, orbit и surface, затем соседние семейства.
5. **Окно тестирования:** auth SinglePlay (cold/warm/cancel/failure), generated
   routes, subtype changes без смены базового type, Existing Worlds save/load,
   дневной surface spawn, surface-to-orbit transition, ship possession/gravity.
6. **Release confidence:** отдельные clean build/cook/packaged smoke и измерения
   CPU/GPU при зафиксированных разрешении, качестве, hardware и baseline.

Сборка проверяет компиляцию; тест проверяет свой контракт; кадры/движение проверяют
вид; benchmark — свои условия. В отчёте следующей задачи указывать каждый из этих
результатов отдельно, вместе с commit и оставшимися ограничениями.

## 11. Что обновлено этим аудитом

- Созданы корневой CLAUDE.md, Docs/README.md, этот аудит и CURRENT_WORK.md.
- Старым stellar/surface handoff добавлены датированные указатели, история сохранена.
- В water-depth README исправлен текущий путь на F: и отмечены ограничения переноса.
- В рабочем архиве обновлён PROJECT_MEMORY.md; он не заменяет production source of truth.
- Код, конфигурация, материалы, plugin modules и пользовательские saves не изменены.

Документационный commit фиксирует этот срез, не завершение всех целей разработки.

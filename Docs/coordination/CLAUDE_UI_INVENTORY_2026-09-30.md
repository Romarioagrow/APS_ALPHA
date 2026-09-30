# UI и системы без интерфейса: инвентаризация (30.09.2026)

Основа для блока U реестра ([CLAUDE_SPRINT_2026-09-30.md](CLAUDE_SPRINT_2026-09-30.md)):
- **U1** — объекты базы;
- **U2** — терминал с картой;
- **U3/U4** — единое меню колонии и доступ к системам, до которых из игры сейчас не добраться;
- **U5** — журнал.

Факты собраны чтением кода (`S` = `Source/APS_ALPHA`, `C` = `Content/APS/APS_ALPHA`). Выводы о Blueprint и уровнях
сделаны по строкам внутри бинарных файлов — их нужно проверить в редакторе.

## Маршруты игры

- **Сгенерированная игра** (`L_WorldGeneration`): `AGravityGameModeBase` → `AGravityPlayerController`, HUD
  `ASMENU_HUD` (`S/Core/GameModes/GravityGameModeBase.cpp:16-17`).
- **Start Single Game** (`L_APS_SinglePlay_StartLocation`): `ACustomGravityPlayerController`. UI нет, F10 нет.

## Что есть из UI

- **Стиль `S/UI/Style`** (~960 строк).
  - `FAPSUIStyle`: палитра, сетка 4 px, кисти, шрифты Orbitron.
  - Компоненты: `SAPSUIFactChip`, `SAPSUIHierarchyCard`, `SAPSUIReadinessRow`, `SAPSUIActionButton`,
    `SAPSUIParameterSlider`, `SAPSUIDisplayProfileSelector`.
  - Используются только FactChip, HierarchyCard и ActionButton, и только картой F10.
- **Главное меню `SAPSMainMenuRoot`** (~4,2 тыс. строк, только на уровне меню). Полезные куски объявлены прямо в .cpp —
  для повторного использования их придётся вынести в заголовки:
  - иконки `SVectorMenuGlyph`;
  - рамки `SChamferedFrame` / `SChamferedSurface`;
  - `ChamferPanel`, `Badge`, `SectionHeading`, `BuildHeader`, `BuildPathCard`;
  - браузер сохранений (сетка, детали, поиск, фильтры, сортировка, страницы);
  - карточки спавна с миниатюрами;
  - тайлы страницы цивилизации: `EnumControl`, `NumberControl`, `ReadinessControl`, `InfoPanel`, `MetricTile`,
    `EditorTab`;
  - `BuildAuxiliaryPage` — хост UMG-виджета внутри Slate.
- **Панель генерации `SWorldGenerationPanel`** (~2,7 тыс. строк).
  - Живое превью с орбитой, зумом и двойным кликом, подписи тел, иерархия тел, фабрики строк.
  - Данные идут через `UWorldGenerationViewModel` в `AAstroGenerator` (`GetPreviewBodyEntries`, `FocusPreviewBody…`).
- **Единственное меню в игре — `SAPSStrategicMapPanel`** (371 строка).
  - F10 в `AGravityPlayerController`: слой z=900, режим GameAndUI; закрывается по F10 или Esc.
  - Внутри 6 карточек фокуса и 5 фактов, только чтение.
- **HUD персонажа и корабля** — Slate.
  - Персонаж: подсказка «F TAKE CONTROL» на z=50, статус перемещения на z=40.
  - Корабль: статус, навигация и маркеры на z=60.
  - `ASMENU_HUD` пустой: BeginPlay закомментирован.
- **Старый UMG, уже заменённый Slate.**
  - `UW_CivMenuRoot`: переключатель экранов без данных. Открывается BP уровня SinglePlay, по Tab (вывод).
  - `WBP_InteractionMenu_HUD` ни на что не ссылается.
  - `WBP_PauseMenu` открывается BP уровня `L_WorldGeneration` (вывод).

## Системы без интерфейса или не подключённые

1. **Взаимодействие APS-81** (1,1 тыс. строк).
   - `UAPSInteractionSubsystem` создаётся автоматически. API: `QueryActor`, `ResolveFocus`, `ExecuteActor`,
     `OnExecutionPublished`; интерфейс `IAPSInteractable`.
   - Вне тестов его никто не вызывает: клавиша F персонажа работает только с `IVehicleControlling`.
   - Системе нужен `SubjectStableId` игрока, а идентичности игрока в коде нет.
2. **Производство APS-81** (3,8 тыс. строк).
   - `UAPSProductionSubsystem` тикает только в Game/PIE. API: `RegisterDefinition`, `RegisterContext`, `QuerySnapshot`,
     `RequestPanel`, `ExecuteCommand`, инвентарь.
   - Консоль `AAPSProductionConsole` открывает панель, берёт идентичность у `UAPSCivilizationIdentityComponent` и
     работает только в сгенерированной игре.
   - Не подключено:
     - определения не регистрируются;
     - консоль не спавнится;
     - `OnPanelRequested` никто не слушает.
3. **Квесты APS-80** (3,1 тыс. строк).
   - `UAPSQuestSubsystem` в GameInstance ставит онбординг из 15 узлов.
   - Событиями кормится только узел «цивилизация готова».
   - Производство шлёт `APS.Building.Build`, а квест ждёт `APS.Build.Place`.
   - Подсказки не отображаются, в `UGameSave` квесты не попадают.
4. **Цивилизация.**
   - Материализация работает только в сгенерированной игре.
   - `UCivilization` — только данные: дивизионы объявлены, но не реализованы.
   - В игре ничего из этого не видно.
5. **Экономики нет**: есть enum, `Credits` и инвентари производства.
6. **Прочее без UI**:
   - контакты навигации корабля;
   - `UAPSShipCatalog`;
   - переходы поверхности: `UAPSPlanetSurfaceTransitionSubsystem` в игре не вызывается.

## События для журнала (U5)

| Источник | Событие / API |
|---|---|
| Цивилизация | `OnMaterializationStateChanged(manifest, prev, cur)`; для позднего подписчика — `GetRuntimeManifest`, `IsMaterializationComplete` |
| Производство | `FOnAPSProductionEventPublished(FAPSProductionEvent)`: Verb, Result, DefinitionId, Quantity, Sequence, Timestamp |
| Панель производства | `OnSnapshotInvalidated(Guid)`, `OnPanelRequested(Snapshot)` |
| Взаимодействие | `FOnAPSInteractionExecutionPublished` |
| Квесты | `InstanceChanged`, `PromptPublished`, `RewardRequested`; для позднего подписчика — `TryGetCurrentPromptSnapshot` |
| Гравитация | `UGravityDetectorComponent::OnClosestGravityBodyChanged` |
| Корабль | `OnInterstellarMode`, `OnStellarMode`, `OnInterplanetaryMode` (без параметров) |
| Сохранение и посадка в корабль | делегатов нет, только логи `[APS.Save]`, `PossessedBy` и `UnPossessed` |

## Ввод

- **Карта клавиш в `DefaultInput.ini`:**
  - F — взаимодействие;
  - G — двигатели;
  - Esc — пауза (BP уровня);
  - T и V — корабль;
  - L и K — в C++ не привязаны.
- **Enhanced Input** — только движение и обзор.
- **F10** — только в `AGravityPlayerController`.
- **Tab** в C++ не привязан.
- **Слои экрана:** персонаж 40/50, корабль 60, карта 900, главное меню 1000.

## Что переиспользовать (кандидаты)

- Шаблон открытия и закрытия F10 и каркас `SAPSStrategicMapPanel` — для карты терминала.
- Иерархию, подписи и поверхность взаимодействия из `SWorldGenerationPanel` поверх `GetPreviewBodyEntries` — после
  выноса в заголовки.
- Компоненты `UI/Style`, тайлы и вкладки страницы цивилизации, карточки спавна, браузер «список + детали»,
  `BuildAuxiliaryPage`.
- `AAPSProductionConsole` и `OnPanelRequested` — как бэкенд терминала; снимок подсказки квеста — для целей.
- События из таблицы выше — как входы журнала.

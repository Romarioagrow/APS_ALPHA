# 13.10 — карточки миров и происхождений, сюжет происхождения, спавн ADRIFT/RING (волна W2), SOURCE READY

Claude (сессия «Аналитика»), 13.10 22:49 → ночь. Ветка `content` в worktree `APS_ALPHA-content`, база `dev-3-stabilization`
98acd1ad7. Не собрано, не запускалось: cl-check всех затронутых TU по rsp игрового target — OK, предупреждений в новом
коде нет. Бриф Rio — [CURRENT_WORK.md](../coordination/CURRENT_WORK.md), последний раздел; разбор спавна и экранов —
три read-only разведки по коду (file:line в этом документе).

## Макеты (структура экранов — «загвоздка»)

`F:/Rio/Aposfera/UI/Mockups/2026-10-13_cards/` — три артборда v1 (HTML 1920×1080, новые версии кладутся рядом, старые
не трогаются):

| Файл | Экран | Структура |
|---|---|---|
| `01_world_card_v1.html` | WORLD CARD — авторский мир | слева живой рендер превью с уровнями O / GX / CL / SYS / S / P (как в генерации, только смотреть), справа: кикер «AUTHORED WORLD · n / 10», имя, подзаголовок, теги, лор, WHERE YOU START (место, маршрут, правила), THE WORLD (живые факты модели), WHAT MAKES IT (теги); внизу статус превью и START |
| `02_origin_card_v1.html` | ORIGIN — происхождение | тот же каркас; справа переключатель ARK / ADRIFT / UNDER THE RING / EXODUS·LATER, описание, WHERE YOU START (место, корабль, мир, запасы), THE LADDER (ступени 0–4 и что их открывает), RULES (REACH / SKY / STOCKS / OTHERS), BEGIN |
| `03_origin_story_tab_v1.html` | ORIGIN — вкладка терминала | слева список шагов (DONE / ACTIVE / NEXT, остальное скрыто счётчиком), справа карточка текущего шага (история, OBJECTIVE, что открывает, действия), снизу полоса лестницы; уведомление HUD |

## Код

| Что | Файлы | Суть |
|---|---|---|
| Экран-карточка | `UI/MainMenu/SWorldGenerationPanel.h/.cpp` | `FAPSWorldCardInfo` + аргумент `Card` панели генерации: колонка ручек не строится, REGENERATE и переименование скрыты, превью шире, справа колонка карточки (`BuildCardColumn`: кикер, имя, теги, лор, WHERE YOU START, THE LADDER, THE WORLD = живая карта фактов `ModelCardBox`, RULES), подпись START / BEGIN. Без карточки — редактор как был. |
| Навигация | `UI/MainMenu/SAPSMainMenuRoot.h/.cpp` | `PlayAuthoredWorld` открывает карточку мира; ORIGIN на NEW WORLD (чипы + кнопка OPEN THE ORIGIN) открывает `OpenOriginCard` (пресет мира происхождения + правила + место старта); START/BEGIN идёт сразу в мир (`CommitAndOpenLevel`), страница CIVILIZATION только у редактора; BACK чистит карточку и замки (`ClearWorldCard`); SANDBOX/SPACE сбрасывают id авторского мира. Предпочтительные стартовые классы грузятся при открытии карточки (`DiscoverSpawnClassOptions`). |
| Пресеты происхождений | `Core/Worlds/APSAuthoredWorlds.h/.cpp`, `Content/APS/APS_ALPHA/Worlds/Origins/{ark,adrift,ring}/{card,world}.json` | `OriginRoot / OriginCatalogue / FindOrigin`; ARK — живой мир G-звезды, 4 планеты, 1 луна, старт на поверхности; ADRIFT — 5 миров K-звезды, старт в корабле; RING — тундровый мир с тремя лунами, старт на поверхности (см. риски). Карточка пресета задаёт место старта, иначе — запись происхождения. |
| Тексты карточки | `Gameplay/Origins/APSOrigins.h/.cpp` | `StartLine / ShipLine / WorldLine / StocksLine` у каждого происхождения. |
| Сюжет происхождения | `Gameplay/Origins/APSOriginQuests.h/.cpp` (новые), `Gameplay/Fleet/APSFleetCommandSubsystem.cpp` (+тик) | Цепочка шагов на происхождение (ARK 13, ADRIFT 12, RING 14): текст истории, цель, что открывает, ступень лестницы, триггер (терминал открыт, пешком от колонии, модуль построен, токен, постройка, цепочка Древних на борде, корабли спущены, в корабле над миром, обследовано миров, на земле дома). Прогресс — токены `ORIGIN_<Key>_<n>` на борде заданий (в сейве). Раз в секунду (CVar `aps.Origin.Story`, `aps.Origin.StoryCheckSeconds`): открытие шага → журнал (категория Origin) + уведомление HUD; выполнение → журнал + уведомление. Консоль `aps.Origin.Story`, `aps.Origin.Advance` (тест). |
| Вкладка ORIGIN | `UI/Colony/SAPSColonyTerminal.h/.cpp` | `ETab::Origin` (индекс 11, кнопка видна только в мире лестницы): шаги DONE / ACTIVE / NEXT + счётчик скрытых, карточка текущего шага, полоса лестницы; обновляется с живыми вкладками. Стиль журнала для категории Origin. |
| Уведомления HUD | `UI/Colony/APSMissionTracker.h/.cpp` | `APSMissionTracker::PushNotice` — уведомление в той же очереди и карточке, что и уведомления заданий. |
| Спавн ADRIFT | `Generation/AstroGenerator.cpp` (`ResolveSpawnLocation`), `Gameplay/Start/APSStartSequenceSubsystem.h/.cpp` | Пилот ставится у люка корабля (`PilotExitPoint`, не в origin актора внутри корпуса); до посадки в кресло держится там с нулевой скоростью, ждёт пока генератор не поставит стартовую иерархию (вместо жёстких 120 с), просит кресло до 5 раз с шагом 1,5 с. Причина падения по разбору: пилот стоял у origin корпуса без удержания, посадка — отдельной подсистемой с обрывом по 120 с; при любой её осечке гравитация планеты/станции уносила пилота. |
| Спавн RING / луна | `Generation/AstroGenerator.cpp` (`SpawnStartInteractiveActors`), `UI/MainMenu/WorldGenerationViewModel.cpp` (`CommitAndOpenLevel`) | Старт на луне без площадки → поверхность домашнего мира вместо ABORT (с отражением в SpawnParameters); в меню старт на луне без луны → поверхность (а не орбита штаба), орбита только если у мира нет поверхности. |
| Пресеты, которые не стартовали | `Content/.../Authored/tidal/{card,world}.json`, `hourglass/card.json`, `Origins/ring/*` | TIDAL и RING стартовали на ледяном гиганте — коммит отказывает («START PLANET IS A GAS GIANT»); теперь тундровый мир (7 400 км) с тремя лунами и старт на поверхности, лор подправлен; HOURGLASS — старт на поверхности вместо луны. |
| Тесты | `Tests/APSWorldRulesTests.cpp` | +`APS.Origin.Story.ChainsAndPresets`: у каждого доступного происхождения есть цепочка с уникальными токенами и пресет мира, который применяется с нужным местом старта. |
| SPACE TRIPS как карточка | `UI/MainMenu/SAPSMainMenuRoot.h/.cpp`, `Gameplay/Quests/Adapters/APSQuestCivilizationAdapterSubsystem.cpp` | Режим SPACE TRIPS больше не уходит на путь SPACE (там игрок — наблюдатель без корабля, и экскурсовод не мог работать): он идёт маршрутом CIVILIZATION со стартом в кресле домашнего корабля, кнопка OPEN THE ROUTE открывает карточку (маршруты, экскурсовод, фото, правила GOALS NONE), SET OUT стартует мир; обучающий маршрут Early Access в мире экскурсии не запускается. |
| Иконки авторских миров | `Content/.../Authored/<id>/thumb.png` ×10, скрипт `F:/ChatGPT/APOSFERA/work/content/make_thumbs.py` | Заглушки-эмблемы в палитре интерфейса (чёрный фон, мир с освещением от звезды своего класса, луны, кольцо у гигантов, туманность у вложенных скоплений, одна бирюзовая линия), 800×450, карточки сетки их подхватывают. Заменить рендерами, когда появятся. |

## Что проверить в игре (после сборки)

1. NEW WORLD → SINGLE GAME → любая карточка: экран WORLD CARD (рендер слева, карточка справа, START), без ручек; START
   сразу в мир, без страницы CIVILIZATION; BACK → NEW WORLD, после него SANDBOX снова редактирует и REGENERATE работает.
2. NEW WORLD → ORIGIN → чип ARK → OPEN THE ORIGIN: экран ORIGIN (лестница, правила, BEGIN). ARK: старт на поверхности
   у колонии; журнал Origin «POWER THE ARK…», уведомление HUD; терминал (Tab) → вкладка ORIGIN; шаги идут: Tab → выйти
   на 40 м → SOLAR ARRAY → HABITAT → COMMS MAST → сигнал Древних → FABRICATION BAY → монумент до конца (LAUNCH) →
   LAUNCH YARD → корпус S → 60 км над колонией → QUIET HULL → STELLAR DRIVE. `aps.Origin.Story` в консоли показывает шаг,
   `aps.Origin.Advance` прокручивает.
3. ADRIFT: старт в кресле корабля (лог `[APS.Start] ship start (attempt 1): … in the pilot seat`), никакого падения;
   шаги DRIFT → NOT HERE (3 обследования) → DOWN (сесть на живой мир и выйти) → дальше как ARK без верфи.
4. UNDER THE RING: старт на поверхности тундрового мира с тремя лунами (не на луне — см. риски); цепочка с шагом
   THE GIANT GIVES (GAS HARVESTER → VOLATILES).
5. TIDAL / HOURGLASS стартуют (раньше TIDAL отказывал на START).
6. SANDBOX и старые сейвы — без изменений; автотесты `APS.Origin.*` — 6 штук.

## Не сделано / риски (честно)

- Старт на луне гиганта («гигант во всё небо») не реализован: у луны нет наземной базы колонии и площадки посадки
  (T-18); RING и TIDAL пока на поверхности тундрового мира. Решение Rio: оставить так до генератора лун-колоний.
- SPACE TRIPS стартует с полным набором цивилизации (колония, штаб, станция, верфь): экскурсия поверх обычного мира;
  «чистое небо без колонии» — после того как путь SPACE получит корабль игрока.
- Иконки авторских миров — процедурные заглушки, не рендеры; стиль строгий, но это не кадр мира.
- Имя цивилизации на карточках не редактируется (по брифу «ничего не выбираем»); при желании — одно поле на карточке.
- В ORIGIN-мире полный набор стартовых актёров как в CIVILIZATION (штаб, станция, верфь, флот); «пустая поляна» ARK —
  T-05.
- Откат стартовой иерархии по-прежнему не возвращает сдвиг домашней системы (AstroGenerator ~14683): при провале
  поверхности планеты пилот останется на орбите как раньше; теперь это только если и луна, и планета без площадки.
- Посадка в кресло: синхронной посадки сразу после коммита нет (только подсистема старта с удержанием и повторами);
  проверить лог `[APS.Start]`.
- Вкладка ORIGIN без горячей клавиши; уведомления HUD в стиле уведомлений заданий.
- Претензии на общие зоны (AstroGenerator, Start, WorldGenerationViewModel, SAPSMainMenuRoot, APSFleetCommandSubsystem,
  SAPSColonyTerminal, APSMissionTracker) объявлены в окне 13.10 23:xx.

## Ветка и вливание

`content` = `dev-3-stabilization` 98acd1ad7 + мои коммиты; вливать fast-forward/merge в `dev-3-stabilization` в окно без
сборки (мои файлы не пересекаются с незакоммиченными файлами APS DEV: `APSPlanetEnvironmentStreamingSubsystem.cpp`,
`PlanetarySurfaceGeneratorStreaming.cpp`). Сборка 0.6.5-alpha — APS DEV.

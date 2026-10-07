# ContinuityV1: Forest, Savanna, SuperEarth, Pangea

29.09.2026. Цель ACTIVE. Предыдущий dry-release завершил проверенное включение
четырёх типов; это прогресс, не завершение всей задачи планет.

## Область текущего прохода

Проверяется перенос существующего Combined/ContinuityV1 на четыре биомных типа.
Не меняются география, seed, рельеф, биомные палитры, вода, атмосфера, сетка256,
коллизия, сохранения, освещение, звёзды или корабельные исходники.
Foliage остаётся OFF. Ассеты не перезапекаются и Shared защищён hash-снимками.
После A/B gate расширяется с десяти до четырнадцати типов добавлением только
Forest/Savanna/SuperEarth/Pangea. Остальные типы/ручные материалы не переключаются.
Rollback: aps.Surface.TerrainContinuity 0, затем пересоздать планету/PIE;
переключатель не меняет существующие MID мгновенно.

Расширены только явные диагностические маршруты FieldsFlight Combined/Published:
Forest (без активации foliage), Savanna, SuperEarth, Pangea. Другие ограниченные
маршруты теста не расширяются. Сборка4actions/max2/13,41с прошла; DLL SHA256:
D8128030BEB55F4084F7743A1439553923BBAB68F6BBD8CA08CED069B6DF55AF.

Сборка release после узкого включения:11actions/max2/37,25с, успешна.
Текущий release DLL SHA256:
4A713D98AF7DC596A52CCFB8BEC38A54318515831CAE587B87A7ADF7A059222B.

## Меню A/B

Результаты: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/.
matrix-biomes-combined-menu-v1, UE21844, запуск13:11:24, завершён к13:14:45.
48PNG, evidence checks PASS, Shared8 без изменений; runtime candidate Combined.
Просмотрены21/48: Native/Candidate03close и04close каждого типа,
Candidate00whole каждого типа и дополнительно ForestNative00whole. Остальные кадры не объявлены
визуально принятыми. В просмотренных кадрах1920x1082, Pangea1914x1076.

На Pangea/Savanna/SuperEarth ослаблена регулярная тканая сетка; вместо неё
менее периодическая детализация, крупные биомные области остаются на месте.
На Forest изменение слабее, исходный контроль достаточно тёмный. Это не
исправление плоского орбитального освещения, широкой песчаной обводки или
резкой границы синей воды — они остаются видимыми и открытыми в эпике.
В04close особенно заметны угловатые берега Forest/SuperEarth: эти кадры
сохранены как контроль оставшегося дефекта, а не доказательство его исправления.

Средний async GPU по6ракурсам каждого варианта (не fullscreen FPS/flight hitches):

| Тип | Native, мс | Combined, мс | Разница, мс |
| --- | ---: | ---: | ---: |
| Forest | 3,243 | 3,707 | +0,464 |
| Savanna | 3,268 | 3,752 | +0,484 |
| SuperEarth | 3,253 | 3,741 | +0,488 |
| Pangea | 3,223 | 3,629 | +0,406 |

## Gameplay A/B и решение

Все четыре <type>-biomes-combined-ground-v1 завершены, каждый2/2 теста
(1success,1warning). PID Forest12888/Savanna20116/SuperEarth31064/Pangea13700.
Последний TestExit13:24:09. Обычная атмосфера, живой WorldScape,
100км->2м(hold4с)->100км,4Hz1280x722. Всего653samples с10presented/0incomplete.
Просмотрены12/653PNG: Native039/Candidate039/Candidate010 каждого типа.
Логи покрытия не доказывают отсутствия popping во всех записанных кадрах.

| Тип | Samples native/candidate | HoldGPU native/candidate, мс | Hold samples |
| --- | --- | --- | --- |
| Forest | 82/81 | 4,098 / 4,490 | 10/10 |
| Savanna | 82/82 | 4,115 / 4,525 | 10/10 |
| SuperEarth | 82/82 | 4,128 / 4,662 | 10/10 |
| Pangea | 81/81 | 4,074 / 4,547 | 10/10 |

Hold=9..11,5с, async GPU с overhead захвата; не fullscreen игровой FPS.
Мелкая фактура в просмотренных парах сохранена; крупное normal-затенение
меняется, особенно SuperEarth. Пиксельная идентичность не заявляется.
Для039 native/candidate RGB8 MAE всего кадра: Forest0,275/255,
Savanna4,936/255, SuperEarth12,254/255, Pangea4,950/255; p99 абсолютной
разницы соответственно2/13/34/15. Это описание разницы, не порог приёмки.
Forest тёмный и до/после: освещённые склоны им не проверены. В подлёте
Savanna остаются полосы и резкие синие озёра. Shared8 сверены для каждого
A/B, изменений0. Решение — узкое включение4типов с post-install проверкой,
не объявление всего P0/берегов/гаммы завершёнными.

## Post-install

matrix-biomes-release-default-menu-v1, UE12352:10/10 тестов
(5success,5warning), завершён13:30:46.30PNG, просмотрены10/30:
00whole/03close для Forest/Savanna/SuperEarth/Pangea и неизменённого Basalt.
Меню запускается без CVar override (-PublishedDefault), runtime фиксирует
noTestSubstitution=1. Четыре новых типа используют ContinuityV1; Basalt31
сохраняет Shared/MI_APS_SharedTerra и прежнюю заметную регулярность.
В просмотренных кадрах1920x1082, кроме Forest1914x1076; между разными
размерами не заявляется пиксельная идентичность.

Четыре <type>-biomes-release-ground-v1 завершены по4/4 теста
(3success,1warning), каждый runtime parent ContinuityV1,
noMaterialSubstitution=1. Launcher задаёт CVar1, default1 проверен меню выше.
Маршрут100км->2м(hold4с)->100км,4Hz1280x722, обычная атмосфера/live WorldScape.

| Тип | PID | TestExit, местное время | Samples / PNG | HoldGPU, мс |
| --- | ---: | --- | ---: | ---: |
| Forest | 18716 | 13:32:32 | 82 | 4,512 |
| Savanna | 23884 | 13:36:32 | 81 | 4,497 |
| SuperEarth | 26820 | 13:38:27 | 81 | 4,619 |
| Pangea | 9376 | 13:40:34 | 81 | 4,466 |

Всего325samples:10presented/0incomplete во всех, просмотрены8/325PNG,
Published010/039 каждого типа. В039 requestedKm=renderedCameraKm=0,002;
это фактический наземный контроль. Мелкая фактура соответствует кандидату;
на промежуточной высоте сохраняются полосы и резкие угловатые озёра Savanna.
Forest всё ещё тёмный контроль, SuperEarth меняет крупное normal-затенение.
Серия4Hz не является покадровой записью всего движения и не доказывает
отсутствия всех LOD popping/яркостных скачков. HoldGPU включает overhead захвата.

После включения всего26/26 тестов,0failed. Warning-events относятся к
LogTemp генерации/гравитации/анимации; есть прежнее сообщение о незарегистрированных
static-mesh bounds корабельной верфи. Её коллизии этим проходом не проверялись.
Shared8 сверены для всех пяти запусков, изменений0. DLL всех четырёх полётов
соответствует release SHA256 выше. Три ContinuityV1 uasset не менялись:

- M_APS_ContinuousTerrain:3D15ADBB53DB4B260D8A112778F5C8682628E4964927A86E4EDF187367C209F6.
- MF_APS_ContinuousNormalCoordinates:ED753868D31C9940B6E7F385FC01CAFC7AAAD130CB78EEF5BB5B6F7AF48AB961.
- MI_APS_ContinuousTerra:779EF3D74FA69AD316E9CA58559DA836A09DF0C9AA5811A0D7A352E325466517.

Все собственные UE завершены, на13:42 живых UE/compiler нет; окно возвращено.
Пример сравнения Savanna04close:
[до](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-biomes-combined-menu-v1/Saved/Automation/PlanetRefinement/Savanna/TerrainPixelAB/04-close-2000km-native.png),
[кандидат](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-biomes-combined-menu-v1/Saved/Automation/PlanetRefinement/Savanna/TerrainPixelAB/04-close-2000km-combined-fields-normal.png).
Наземный установленный материал:
[Savanna2м](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/savanna-biomes-release-ground-v1/Saved/Screenshots/Windows/APS_FieldsFlight_32_Published_039.png).

## Открытая область

Остальные семейства, остаточные полосы, весь диапазон LOD/скоростей, цельные
переходы до орбиты, гамма/берега/вода/атмосфера/облака/наполнение остаются
в полном объёме цели. Используется unreal-rendered-visual-validation:
отдельно установлено, визуально подтверждено, не проверено и ещё сломано.

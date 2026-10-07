# ContinuityV1: Rocky, Desert, Sand, HighMountain

29.09.2026. Цель ACTIVE; это перенос уменьшения тайлинга, не завершение P0
или художественная приёмка всех планет. Предыдущий проход cold-release был
прогрессом: установлено расширение на три типа и проверен обычный runtime.

## Изменение

В APSTerrainContinuityMaterial::Allows добавлены четыре явно проверенных типа:
Rocky, Desert, Sand, HighMountain. Теперь default1 охватывает десять типов:
Terrestrial/Frozen/Oasis/Ice/Tundra/Nordic/Rocky/Desert/Sand/HighMountain.
Остальные типы остаются на старом стеке; ручные материалы не переназначаются.
Старый путь: aps.Surface.TerrainContinuity 0, затем пересоздание планеты/PIE.
Переключение CVar не меняет уже созданные MID мгновенно.

В diagnostic launcher и C++ добавлены только четыре явно запрошенных маршрута
Combined/Published. Тест gate/rollback всё ещё проверяет256 значений enum при0/1.
Сетка256, seed, география, геометрия, коллизия, вода, атмосфера, освещение,
звёзды и корабельные файлы в этом проходе не изменялись. Foliage OFF.
Три ContinuityV1 uasset не перезапекались; общий Shared защищён снимками.

Diagnostic build4actions/13,40с, DLL SHA256
4060E905C864EEDFE8B1C30B61DF8C945CAD8C32F6C0971D1E0893D2B0FEA901.
Release build11actions/38,79с, max2, DLL SHA256
13790BABD030710D70BDE4D449D3AC254016254673D2444A71EAD16548E7DA0C.
Scoped git diff --check прошёл; только уведомления нормализации CRLF.

## Проверенное A/B

Корень результатов: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/.
matrix-dry-combined-menu-v1, UE3184, запуск12:31:44:7/7 тестов
(3success,4warning),48PNG. Evidence checks passed; Shared8 без изменений.
Просмотрены13PNG: native/candidate03close всех четырёх; candidate00whole
всех четырёх и дополнительно native00whole Desert. Это не просмотр48/48.
1920x1082, кроме HighMountain1914x1076; внутри A/B размер одинаковый.
Мелкий регулярный клетчатый рисунок ослаблен, крупные маски сохранены.
Крупная плоскость/дымка, угловатые берега HighMountain не исправлены.

Средний GPU по6ракурсам каждого варианта меню:

| Тип | Native, мс | Combined, мс | Разница, мс |
| --- | ---: | ---: | ---: |
| Rocky | 3,217 | 3,755 | +0,538 |
| Desert | 3,219 | 3,753 | +0,534 |
| Sand | 3,227 | 3,767 | +0,540 |
| HighMountain | 3,240 | 3,695 | +0,455 |

Наземные <type>-dry-combined-ground-v1: каждый2/2 (1success,1warning),
обычная атмосфера, живой WorldScape,100км->2м(hold4с)->100км,4Hz PNG1280x722.
PID Rocky30144, Desert11828, Sand22520, HighMountain27372. Последний вышел12:46:09.
Все659samples presented10/incomplete0. Просмотрены по3 кадра:
Native039, Candidate039 и Candidate010; всего12PNG, не все659.

| Тип | Samples native/candidate | Hold GPU, мс | Hold samples | Ground039 MAE / p99 RGB8 |
| --- | ---: | --- | --- | --- |
| Rocky | 83/82 | 4,083 -> 4,487 | 10/9 | 7,792 / 28 |
| Desert | 83/83 | 4,045 -> 4,449 | 10/10 | 2,400 / 11 |
| Sand | 83/82 | 4,046 -> 4,512 | 10/10 | 4,820 / 16 |
| HighMountain | 81/82 | 4,075 -> 4,538 | 9/10 | 0,826 / 3 |

Мелкая наземная фактура визуально сохранена; крупное normal-затенение меняется,
поэтому нет заявления о пиксельной идентичности. HighMountain очень тёмный
и до, и после; этот контроль не доказывает качество всех освещённых склонов.
Hold GPU — mean9..11,5с, async counter с overhead захвата. Не игровой FPS,
не проверка полного разрешения/рывков и не доказательство непрерывности всех кадров.

## Обычный путь после включения

Завершено29.09 к13:04. matrix-dry-release-default-menu-v1, UE13968:
10/10 тестов (5success,5warning),30PNG. Четыре изменённых типа используют
ContinuityV1; Basalt31 сохраняет Shared. Меню запущено без CVar override,
noTestSubstitution=1. Просмотрены10/30 кадров:00whole/03close всех пяти типов.
1920x1082, кроме Desert1914x1076. Это не сопоставление пикселей с A/B другой
размерности. В Basalt сохраняется старый регулярный рисунок: контроль не
переключён, его визуальный дефект не объявляется исправленным.

Production-ground <type>-dry-release-ground-v1: каждый4/4 теста
(3success,1warning), обычный runtime parent ContinuityV1,
noMaterialSubstitution=1. Launcher задаёт CVar1; значение по умолчанию
отдельно проверено меню выше. Маршрут100км->2м(hold4с)->100км,4Hz,1280x722.

| Тип | PID | TestExit, местное время | Samples / PNG | Просмотрены |
| --- | ---: | --- | ---: | --- |
| Rocky | 32364 | 12:54:18 | 83 | Published010/039 |
| Desert | 20472 | 12:55:45 | 83 | Published010/039 |
| Sand | 3384 | 12:57:16 | 83 | Published010/039 |
| HighMountain | 32408 | 13:03:44 | 81 | Published010/039 |

Всего330samples:presented10/incomplete0 во всех; просмотрено8/330PNG.
Непрерывная запись получена, но отсутствие скачков на каждом её кадре не
подтверждено. Геометрическая готовность не заменяет просмотр движения.
В просмотренных кадрах мелкая фактура соответствует проверенному кандидату;
HighMountain остаётся тёмным контролем, не доказательством всех освещённых
склонов. Остаточные полосы на промежуточной дистанции сохраняются.

В сумме после включения26/26 тестов,0failed. Warning-events относятся к
LogTemp генерации/гравитации/анимации; среди них есть сообщение о незарегистрированных
static-mesh bounds корабельной верфи. Это не проверка/исправление корабельных
коллизий. Дополнительных warning/error вне этих групп в отчётах не найдено.
Shared8 сверены для всех пяти post-install запусков, изменений0; SHA256 DLL
соответствует release выше. SHA256 трёх ContinuityV1 uasset совпадают с
предыдущим релизом: ассеты не менялись. Последний собственный UE завершён,
на13:05 живых UE/compiler нет; окно возвращено записью координации.

Пример сохранённого сравнения Rocky, меню03close:
[до](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-dry-combined-menu-v1/Saved/Automation/PlanetRefinement/Rocky/TerrainPixelAB/03-close-2000km-native.png),
[кандидат](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-dry-combined-menu-v1/Saved/Automation/PlanetRefinement/Rocky/TerrainPixelAB/03-close-2000km-combined-fields-normal.png),
[после включения](F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-dry-release-default-menu-v1/Saved/Automation/PlanetRefinement/Rocky/TerrainNativeViews/03-close-2000km-published.png).

## Граница результата

Остальные типы, остаточные полосы, весь диапазон live LOD/скоростей, берега,
глубина/вода, гамма, атмосфера/облака и наполнение остаются открытыми в эпике.
Сборка/ready-логи не подменяют кадры — применена процедура
unreal-rendered-visual-validation. Публикация ограничена проверенными типами.

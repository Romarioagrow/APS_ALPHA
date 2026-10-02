# Terrain Continuity V1 — ограниченное включение Terrestrial/Frozen

## Статус 29.09, 11:01

Материал опубликован в отдельную production-папку ContinuityV1. Меню и gameplay
проверены с настоящим runtime-выбором, без диагностической подмены MID. После
проверок default переключён с0 на1. Сборка и smoke без CVar override завершены.
Эпик ACTIVE, это частичное улучшение материала, не закрытие всего P0.

## Что включается и что защищено

- Только generated Terrestrial и Frozen. Другие типы, неизвестные значения enum,
  вручную назначенные материалы и magmatic permutation остаются на прежнем пути.
- Пять macro-normal вызовов получают проверенный NormalHex sampling; дальние поля
  — проверенный FieldsV3. Размеры мелкой фактуры <=50м сохраняются; переход
  normal50..200м и orbital5..50км не заменяют seed/height/mesh/коллизию.
- Palette, texture bindings, точный high/low planet frame и inverse scale сохраняются.
- Существующие Shared uasset и каталог не перезаписаны. Настройки света, вода,
  корабли, атмосфера и foliage этим включением не меняются. Foliage остаётся OFF.
- Откат: `aps.Surface.TerrainContinuity 0`, затем пересоздать планету/restart PIE.
  Уже созданный MID не подменяется посреди кадра. Возврат1 также требует пересоздания.

## Публикация и происхождение

Корень свидетельств: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`.
Publisher: `bake-continuityrelease-verified-combined-v1`, UE4196,10:31:59–10:32:48.
Завершение:0errors/9warnings; предупреждения относятся к временным pin bindings
до восстановления; после восстановления закончена shader compilation и проверен LocalVF.
Publisher DLL SHA256:99C856ECD7E5B265861A57F3818F87C8FF1273B9CB1129B18B6C0F15679F6ABE.

Исходники скопированы из ContinuityCombined20260929V1 с проверкой SHA1 до/после:

| Source asset | SHA1 |
| --- | --- |
| M_APS_NormalWarpTerrain | 537341BE0160BF155C3DFDF898A743429E0E11ED |
| MF_APS_NormalMacroCoordinates | E5A466D571C3FBB52B6871E0352ED6FB1A685285 |
| MI_APS_NormalWarpTerra | 48D9E89E9C4FD85657B6329B7E86A011A0FBA887 |

Сохранённые production assets, SHA256:

| Asset в Content/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1 | SHA256 |
| --- | --- |
| M_APS_ContinuousTerrain.uasset | 3D15ADBB53DB4B260D8A112778F5C8682628E4964927A86E4EDF187367C209F6 |
| MF_APS_ContinuousNormalCoordinates.uasset | ED753868D31C9940B6E7F385FC01CAFC7AAAD130CB78EEF5BB5B6F7AF48AB961 |
| MI_APS_ContinuousTerra.uasset | 779EF3D74FA69AD316E9CA58559DA836A09DF0C9AA5811A0D7A352E325466517 |

Publisher отказывается перезаписывать существующие destinations, сверяет исходники,
перепривязывает пять normal function calls, запрещает прямые Diagnostic dependencies.
Папка уже покрыта существующим cook-каталогом PlanetSurface; packaging не запускался.

## Проверки обычного runtime-пути

| Run | Результат | Визуальные свидетельства |
| --- | --- | --- |
| matrix-release-route-menu-v1 | 7pass/1fail тестовой установки CVar; все3 render cases и assets contract прошли | 18PNG; просмотрено10: Frozen все6, Terrestrial00/03, Oasis00/03 |
| terrestrial-release-route-ground-v1 | 3pass/1fail диагностического допуска Terrestrial; маршрут не начат | Нет кадров пролёта, не считать визуальной проверкой |
| terrestrial-release-route-ground-v2 | 4/4,81 samples/81 flight PNG, incomplete0, presented10 | Просмотрены Published010 и039; хороший мелкий наземный материал сохранён |
| frozen-release-route-ground-v1 | 4/4,83 samples/83 flight PNG, incomplete0, presented10 | Просмотрены Published010 и039; мелкая снежная фактура сохранена |

Оба gameplay run:100км ->2м,hold4с ->100км; обычная атмосфера, живые WorldScape LOD,
no material substitution/no terrain freeze. DLL этих двух run:
3A23C2BAC4E7ED42266CDFB84C05493A5912D9023EDD7B84BDCD7E512BDD117E.
Материал на всех текущих terrain sections проверялся тестом. Меню подтвердило
production parent для Terrestrial/Frozen и прежний parent у Oasis-контроля.
Shared8 сверены после обоих gameplay run: без изменений.

Причины первых отказов устранены именно в тестах: SetByCode не мог перебить
SetByConsole из ExecCmds; теперь тест устанавливает Console priority и восстанавливает
исходные flags/value. Второй отказ был allow-list типа планеты в diagnostic flight.
Обе исправленные проверки прошли в v2 и Frozen; результаты проваленных run сохранены.

## Эффект, цена, ограничения

Предшествующий A/B Combined показал уменьшение регулярной мелкой сетки в Frozen
и Terrestrial. Полные сведения:2026-09-29-fields-matrix-and-ground-prototype.md.
GPU прибавка кандидата в сопоставимых кадрах:Frozen1280x722 около0,4..0,7мс;
Terrestrial3440x1342 около0,62..1,62мс. Это не общий FPS готовой игры.
Наземный hold A/B:Terrestrial4,1268->4,5054мс;Frozen3,9813->4,4203мс.
Normal-затенение не идентично старому:ground039 MAE4,31/5,09 на шкале255.

Не закрыты: крупные полосы/геометрическое затенение, все типы планет, непрерывность
всего движения, берега/фасетная вода/цвет, атмосфера/облака/наполнение.
Сохранённые PNG и логи полноты не доказывают отсутствие popping между всеми кадрами.
Отдельные красивые кадры не объявляются полной приёмкой. Нет packaged/cooked smoke.

## Следующий шаг

Отдельная ограниченная итерация по остаточным геометрическим полосам
и появлению участков. Не повышать разрешение256 и не менять хороший наземный вид
вместо выяснения причины. Дальнейшие этапы исходного эпика сохраняются.

## Собранный default1 — финальный smoke 11:01

Сборка:11actions/max2,37,00с,успех. DLL SHA256:
0367C77FAE261E1AD8754C1C2C8D377E3682B9DCEA08FDADB0857A8F3E441B8E.
Run:`frozen-release-default-wheel-v1`,UE28680,10:58:32–10:59:51(тесты).
6/6 тестов (5success/1successWithWarnings),0fail. ExecCmds не содержит установки
TerrainContinuity; лог подтверждает ContinuityV1 parent/noTestSubstitution=1.
21PNG, просмотрены00/01/09/10/11/20: масштабируется тот же рисунок, прежняя
мелкая регулярная клетка не доминирует. Вид всё ещё мягкий/плоский, не объявляется
финальным художественным результатом. Маршрут1000->137,45->1000км, fixed retained
mesh и settled свет: это тест материала/шага колеса, НЕ live WorldScape-streaming.
Живое обновление LOD проверялось отдельно в обоих gameplay run выше.
Shared8 и каталог после прогона сверены без изменений; процессы завершены.
Для пользовательского теста нужен новый запуск Unreal с этой DLL и новая
генерация/пересоздание планеты. Никаких ручных включающих команд не требуется.

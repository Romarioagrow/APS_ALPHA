# Живой океан: обновление и стоимость в движении

Статус30.09 01:04: цель ACTIVE. Это проверка водной ветки, НЕ завершённая
режимная оптимизация корабельного полёта и НЕ приёмка нового берега.

## Что установлено

Development Editor APS DLL `A0FEEC9A1B8839309E059BA02161F61D2B84B554A9EA10D719E658B602761473`.
Core без изменения: `58AC6497F0073A3376294ED7984AE38A1B9B3A8CCFE3628950878815B46A60B5`.
Две узкие сборки по4actions,22.51/22.61с. Никаких production defaults/запечек.
23защищённых uasset проверены:0изменений. Корабли/звёзды/спавн не редактировались.
Backup исходной APS F3B39294...: `F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/before-wet-flight-probe`.
Новый UV1/материал по-прежнему НЕ включены обычной игрой.

Добавлены `APSWaterFlightProbe.h` и режимы существующего RunPlanetFieldsFlight.
Явный Terrestrial seed424242, ocean=true; высота100км→50м→100км над морем,
17с; реальный unfrozen WorldScape и обычная атмосфера/освещение. Положение
персонажа/камеры меняет диагностический сценарий: это НЕ физическое управление
кораблём, не проверка относительной скорости, торможения или eviction.
Фактический GAME viewport1280x722, несмотря на запрос окна1600x1000.
Пара имеет точно одинаковые root radius637100032, NoiseScale478,
NoiseIntensity924956, outward/tangent и seed; рисунок планеты не менялся.

Материал-кандидат привязывается через UpdateOceanMaterial к текущим и будущим
секциям, transient lease восстанавливает оригинал. Никакой заморозки mesh,
записи UV1 или CPU snapshot: UV1 приходит только из native worker.
Флаг WaterMaterial требует WaterPayload, неверный/сухой мир отклоняется.
Проверяется90вершин из30опубликованных oceansections, раз в секунду в perf
или при кадре в visual. Это выборочная, не полная проверка всех вершин.

В существующей live-flight диагностике исправлена гонка: HasCompletePayload
читает worker staging arrays и разрешён лишь после drain. В движении вместо
него теперь HasPublishedSections: только GT mesh sections, наличие и
выборочная целостность. Игра/плагин этим исправлением не менялись.

## Сопоставимая пара v2 без PNG и CPU trace

Общая база: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`.
Прогоны `terrestrial-wet-depth-off-v2` и `terrestrial-wet-depth-on-v2`.
Одна DLL, один обычный материал. Меняется только native depth sampler.
Три ground PNG сделаны ДО измеряемого маршрута; в маршруте PNG нет.
После него сохраняется Saved/Diagnostics/WaterFlight.csv. Каждый кадр
учтён: gap=1, пропусков0. GT/RT/GPU counters предыдущие/асинхронные,
их нельзя считать синхронной разбивкой того же Present.

| Метрика, мс если не указано | depth OFF | depth ON |
| --- | ---: | ---: |
| Кадров | 1104 | 1245 |
| Frame median | 12.839 | 11.293 |
| Frame p95 | 26.514 | 22.811 |
| Frame p99 | 88.530 | 74.393 |
| Frame max | 107.721 | 115.899 |
| Кадров >33.33мс | 40 | 41 |
| GT p99 | 89.548 | 75.621 |
| GPU p95 | 6.127 | 6.612 |
| GPU p99 | 9.280 | 8.996 |
| Работников в очереди, max | 20 | 20 |
| Process working set, sampled MiB min–max | 11316.71–11447.68 | 11364.76–11453.73 |

Одна последовательная пара НЕ доказывает ускорение от дополнительной работы:
median/p95 колеблются. Она показывает сохраняющиеся тяжёлые пики и отсутствие
падения/утраты UV1. Утверждать приемлемую цену воды по ней нельзя. Это только
working set процесса, не размер планетных ресурсов; VRAM residency/eviction
этим прогоном НЕ измерены. В обоих13изменений опубликованного waterpayload,
17проверок, wet samples есть; последняя depth oracle ошибка0.000038978м.
В каждом4/4tests,2теста сwarnings,0failed. Warnings включают существующие
runtime warning-логи и FindConsoleObject lookup warning; не выдавать за ноль
предупреждений. Полные warnings в report/index.json.

## Трасса и кадры v1: отдельные, с ограничениями

До исправления диагностического staging read были сняты wet-depth-off/on-v1
с CPU trace. Они заменены v2 как основной timing baseline. В on-v1
`insights/water.csv` подтверждает настоящий WS_OceanDepth:
9780440вызовов/47.020с summed по workerthreads внутри17с региона
APS_WaterFlight. WS_Mesh_CalculateNoise106.714с (вложено, не складывать).
Это НЕ47с задержки GT и не чистая цена без per-vertex instrumentation.
WS_CollisionLodHandler max113.863мс; нужен дальнейший разбор пиков,
не только среднего времени. Не доказано, что все пики вызваны водой.

`terrestrial-wet-depth-material-v1`: candidate depth+art palette, native
worker,64route PNG+3ground.4/4tests,2сwarnings;603наблюдаемых кадра,
16изменений опубликованной геометрии. Просмотрены3/67PNG:
APS_FieldsFlight_1_Published_004/029/052. Материал остаётся на живых секциях,
но мелководье пятнистое/бугристое издали, линия берега резкая, блик широкий;
кадры004/052 не подтверждают качество берега50м (029в основном вода).
Это не запись каждого кадра и не доказательство полного отсутствия popping.
Новую воду нельзя объявить законченной или подменить ей обычный material
без следующей художественной/динамической проверки.

## Воспроизведение

Проверить координацию и реальные процессы, новый Label на каждый запуск:

```powershell
./Tools/Diagnostics/RunPlanetFieldsFlight.ps1 -Family Terrestrial -Isolation Published -WaterFlight -Performance -Label unique-off
./Tools/Diagnostics/RunPlanetFieldsFlight.ps1 -Family Terrestrial -Isolation Published -WaterFlight -WaterPayload -Performance -Label unique-on
./Tools/Diagnostics/RunPlanetFieldsFlight.ps1 -Family Terrestrial -Isolation Published -WaterFlight -WaterPayload -WaterMaterial -Label unique-visual
```

CpuTrace допустим только с Performance и предназначен для разбора,
не для парной оценки стоимости с per-vertex trace. Сессии строго по очереди.
Материалы не сохраняются; свежий обычный запуск не включает experimental flags.

## Следующие обязательные практические шаги

1. Устранить пики генерации/GT publication/collision по событиям, сохранить
   согласованность соседних LOD и физику. Не повышать сетку вслепую.
2. Довести цвет по глубине/shoreline и спокойную воду; включение обычного
   игрового пути остаётся обязательным после проверки, не заменить его demo.
3. Реальная motion-aware residency: ground/low-flight защищены независимо
   от скорости; приближение/торможение упреждают подготовку, уход освобождает
   ближние mesh/foliage/collision после готовности согласованного дальнего вида.
   Использовать относительное движение и высоту, гистерезис, отделить rebase/
   телепорт; текущий FrozenVisible не освобождает mesh и не закрывает пункт.
4. Подтвердить разгон, торможение, разворот, быстрый низкий полёт, смену тела:
   frame/CPU/GPU p95/p99, RAM/VRAM/очереди и фактическое освобождение. Не
   считать scripted observer route этой приёмкой. Облака/атмосфера/наполнение
   и остальные семейства остаются в полном эпике, не потеряны.

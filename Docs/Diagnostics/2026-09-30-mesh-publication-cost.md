# Стоимость публикации WorldScape: два отклонённых кандидата

Статус30.09 01:45: для обоих кандидатов **выполнен откат**. Обычная игра сохранила
Core `58AC6497F0073A3376294ED7984AE38A1B9B3A8CCFE3628950878815B46A60B5`
и APS `A0FEEC9A1B8839309E059BA02161F61D2B84B554A9EA10D719E658B602761473`.
Нет принятого ускорения, новой воды или режимной выгрузки в этом проходе.
Цель ACTIVE; не повторять перебор числа потоков вместо работы с публикацией.

## Что доказано профилем

База evidence: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`.
Трассы с `-statnamedevents`, экспорт только GameThread и региона
`APS_WaterFlight`; не путать summed worker time с задержкой игрового потока.

`terrestrial-wet-gt-detail-v1/insights/water-gt.csv` (старый Core):
за17с WorldScapeRoot1.606447с, UpdateSection GT1.323199с/1200вызовов.
CollisionLodHandler0.087495с,max39.492мс; Update bCollision0.005389с.
Поэтому задуманный collision-cook batching НЕ внедряли: не основной расход.

Последняя подробная трасса `terrestrial-wet-fast-copy-trace-v1`:

| Участок GT | Всего, с | Вызовы | Max, мс |
| --- | ---: | ---: | ---: |
| WorldScapeRoot | 1.605904 | 1020 | 79.797 |
| UpdateSection GT | 1.322522 | 1200 | 5.392 |
| Bounds | 0.076745 | 1200 | 0.831 |
| Vertex batch | 0.517697 | 1200 | 2.535 |
| Render-buffer allocation/copy | 0.723273 | 1200 | 3.355 |

Нижние3строки вложены в UpdateSection; не складывать с ним. В этой трассе
примерно55% его времени — `SectionData->NewVertexBuffer = Section.PlanetVertexBuffer`.
Метка включает allocation и copy вместе, НЕ доказывает отдельно стоимость
memcpy/pagefault/allocator. Публикация соседних LOD остаётся общей пачкой:
не разносить её вслепую по кадрам — есть риск швов/дыр.

## Кандидаты и решение

1. Ограниченные joined vertex-copy batches1/2/4, только APS-owned/tagged
   визуальные mesh с NoCollision и>=16384вершин. Все задачи завершаются до
   bounds/physics/render publication. Public ABI/layout не менялись.
   Core `2A368E9A...`. Две чистые пары в порядке1→4→4→1:

| Режим/run | Кадры | Frame p95, мс | p99 | Max | >33.333мс |
| --- | ---: | ---: | ---: | ---: | ---: |
| serial-v1 | 1118 | 27.126 | 86.329 | 99.274 | 39 |
| parallel-v1 | 1191 | 31.206 | 77.383 | 90.971 | 57 |
| parallel-v2 | 1210 | 28.611 | 71.548 | 91.748 | 53 |
| serial-v2 | 1142 | 24.343 | 85.090 | 119.313 | 38 |

Папки имеют префикс `terrestrial-wet-publish-`. P99 улучшился, но средних
рывков больше, p95 хуже; не принято. В отдельной trace UpdateSection GT
1.266625с против исходных1.323199с: маленькая экономия не закрывает статтер.

2. Проверки длин каналов вынесены из внутреннего цикла, копирование через
   заранее проверенные pointers. Serial default, без дополнительных задач.
   Core `39023136...`. Тест original/fast1/2/4/untagged включает пустые
   optional channels и неравные размеры batch; сравнивает все поля/UV/границы/
   topology точно. NoCollision/tag guards сохраняются.

| Режим/run | Кадры | Frame p95, мс | p99 | Max | >33.333мс |
| --- | ---: | ---: | ---: | ---: | ---: |
| fast-copy-on-v1 | 1138 | 23.991 | 92.307 | 113.425 | 40 |
| fast-copy-off-v1 | 1201 | 24.566 | 75.817 | 90.568 | 42 |

Папки `terrestrial-wet-<run>`. Одна пара не даёт причинно-статистического
вывода, но пользы для пиков НЕ подтверждает. Кандидат тоже откатан.
Не выдавать отклонённые исходники/сборки за действующее улучшение.

## Условия и проверка сохранности

Одна APS DLL, обычный material/depth OFF, явный wet Terrestrial seed424242,
тот же100км→50м→100км сценарий17с. Настоящий GAME viewport1280x722.
Во время чистого замера нет PNG/CPUtrace;3наземных PNG сняты до него.
Процентили nearest-rank по dt_ms. GT/RT/GPU counters предыдущие/асинхронные,
не считать их синхронной раскладкой одного кадра. Не физический полёт корабля.

9gameplay прогонов (6timing+3trace) по4теста:36/36,18tests сwarnings,
0failed/notRun. Полные warnings оставлены в report/index.json. Ниже не
подразумевается, что в логах только18warning-сообщений.
Private v1/v2/v3: по8/8,0testwarnings/failed; последний тест проверил
2631720vertex comparisons. Первый build нового теста исправлен: сравнение
TArray<uint32> с TArray<int32>; последующие сборки успешны. Это не C1060.

Просмотрены2PNG: `terrestrial-wet-publish-serial-v2` и
`terrestrial-wet-fast-copy-on-v1`, оба
`Saved/Screenshots/Windows/APS_SurfaceLighting_1_Plus8s.png`.
В этой паре сохраняются рельеф, фактура и положение персонажа. Это НЕ
видеоприёмка пролёта и НЕ визуальная проверка всех семейств. По навыку
Unreal rendered validation build/parity/timing не подменяют кадры.
8Shared uasset проверены по before manifest:0изменений. В этом проходе
не перепроверены все23защищённых assets прежнего прохода. Нет запечки,
правок кораблей/звёзд/освещения/спавна/высот. FPS выигрыш не заявляется.

## Обратимость и следующий шаг

Оба offline install/rollback завершены без чужих UE-процессов. Backup:
`F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/mesh-publication-install-v1`
и `mesh-publication-install-v2`. Восстановлены исходные Core dll/pdb/lib и
WorldScapeMeshComponent.cpp; удалён лишь добавленный тест из installedplugin,
он сохранён в privatehost и Tools/Diagnostics/WorldScapeMeshPublication.
APS не пересобиралась: публичные headers/UHT/layout проверены неизменными.
Private DLL/исходник остаются экспериментальными, НЕ устанавливать без новой
проверки. Install.ps1 проверяет остальные source, BuildId, хеш tested DLL,
резервирует5файлов и имеет guard от перезаписи чужих последующих изменений.

Следующая адресная работа: подготовка immutable vertex/render payload в
имеющемся worker job, затем лёгкая согласованная публикация после всех
соседей. Нужны ownership/worker fence, лимит одновременно подготовленных
данных и освобождение после RT; не держать безлимитный pool или удвоенную
планету в RAM. Это **ещё не реализовано**. Не возвращаться к уменьшению
сетки/ухудшению принятой поверхности ради обхода CPU копирования.

Полный эпик сохраняет motion-aware preload при подлёте/торможении,
реальную eviction при удалении/транзите, защиту низкого быстрого полёта,
ограничение очереди, RAM/VRAM и rebase. Эти тесты не доказывают их выполнение.
Вода/берег должны стать ordinary-enabled после проверки; облака, атмосфера,
детализация других типов и ограниченный foliage также остаются в плане.

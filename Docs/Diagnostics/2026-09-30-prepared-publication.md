# WorldScape: сокращение тяжёлых рывков публикации

30.09 02:55: v3 установлен с ограниченным auto-режимом;
Terrestrial default+water payload и Frozen/Oasis default runtime проверены.
Не приёмка всего эпика и не реализация режимной выгрузки.

## Изменение

Существующая LodGenerationThread после нормалей/швов готовит CPU-пакет:
новый опубликованный vertex buffer, резервную render-копию и упакованные
position/tangent/UV0..3/color CPU-буферы. RHI/InitResource на worker не вызывается.
GT по прежнему IsDone-fence всей пачки проверяет все три секции и меняет
владение массивами. RT проверяет форматы/размеры, memcpy и прежняя GPU-загрузка.
При несовместимости/исчерпании бюджета — прежний путь. Топология, рельеф,
материалы, collision mesh и момент общей публикации соседних LOD не меняются.

Глобальный лимит временных резервов384MiB (0..512). Лизинг переживает задачу
в render-command; старые CPU-данные освобождаются перед лизингом. При сбросе
root workers сначала завершаются. Это НЕ cap всей RAM/VRAM планеты, не cache
и не выгрузка ближних сеток при транзите. Published buffers заимствуются
worker под существующим root fence: нельзя менять их внешним API параллельно.

## Почему понадобились два варианта

V1 перенёс только GT-обработку. На одной DLL полный frame p99 OFF81.80мс,
ON90.97мс; GT counter p99 82.08→23.93мс. Как самостоятельное ускорение отвергнут.
Профиль ON за17с: WorldScapeRoot GT0.149305с вместо старых1.606447с;
1200 WS_PublishPreparedSection суммарно0.000409с. Но UpdateSection RT1.593965с,
и FrameSync GT ожидание8.934020с,max98.751мс. Нельзя выдавать падение GT
за плавный кадр. All-thread worker суммы не являются wall-clock задержкой.

V2 перенёс также CPU-упаковку render-буферов. Две чистые пары, порядок
ON1→OFF1→OFF2→ON2, одна APS2E6E7E6F/CoreBC8D4211 DLL. Без PNG/CPUtrace внутри
измерения,17с Terrestrial wet100км→50м→100км, seed424242. GAME viewport1280x722,
RTX5080/i9-10900KF. Это scripted observer, НЕ физический полёт корабля.

| Прогон | Кадры | Frame p95,мс | p99 | max | >33.333мс | >50мс |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ON1 | 1351 | 21.344 | 37.208 | 68.547 | 38 | 1 |
| OFF1 | 1280 | 21.678 | 75.963 | 92.403 | 37 | 21 |
| OFF2 | 1287 | 22.425 | 73.912 | 93.611 | 38 | 20 |
| ON2 | 1237 | 22.762 | 41.357 | 75.457 | 39 | 2 |

Подтверждён узкий выигрыш по тяжёлым публикационным пикам; p95 и число средних
рывков почти не меняются, устойчивый прирост среднего FPS НЕ доказан.
GPU p99 OFF8.91/9.38мс,ON8.86/9.38мс; нет подтверждённого GPU ускорения.
GT/RT/GPU counters предыдущие/асинхронные, не синхронная раскладка одного кадра.
Процентили nearest-rank. Первый RAM peak sampled OFF11415.55MiB,
ON11566.52MiB (~151MiB разница). Полная VRAM динамика не снималась.
В чистом ON резерв peak382795520байт (~365MiB), после RT flush0, fallback0.
В v3 Terra water payload+PNG peak401360480байт (<384MiB),117fallback,
309built/published, после flush0: тяжёлый водный путь сохраняет ограничение.

Финальная attribution-трасса V3 default (без WaterPayload/PNG в ROI):
UpdateSection RT0.511958с/1200вызовов против V1 1.593965с (~68% меньше);
WorldScapeRoot GT0.139205с, WS_PublishPreparedSection0.000397с/1200.
FrameSync max32.726мс против V1 98.751мс. Worker preparation6.276619с
суммарно по400задачам — это параллельная CPU сумма, не wall-clock/frame time.
466built/published, fallback0, peak382795520, liveBytes0 после RT flush.
Нельзя подмешивать этот trace к четырём чистым timing-прогонам выше.

## Ограниченное включение и откат

V3: `worldscape.PreparedMesh=-1` default, auto требует BOTH native-owner tag
`APS.Collision.ParallelSamples` и `APS.Mesh.PreparedPublication`.
APS выставляет второй только generated full-scale Terrestrial/Frozen/Oasis;
manual, scaled preview и другие presets остаются legacy. Collision исключён.
`worldscape.PreparedMesh 0` — немедленный возврат к прежним будущим публикациям,
1 — диагностический opt-in всех подходящих APS roots. `worldscape.PreparedRender 0`
изолирует отвергнутый V1, default1. Не оставлять V1 как рекомендуемый режим.

Установлены согласованные8plugin modules+11native source+3UHT и APS:
Core00E857570217EC4E5A4969F8DE78FC0DEDEAD95C55BC2D6C8F4876A81EA1A162,
APS1FCD5A7045AE542D40E6DF21F6F4796DF55E9B002C503C1BA49810DB19334638.
Task C++ layout изменён, одна DLL без совместимой APS недопустима.
UHT Root/Mesh отличались префиксом FID private-host; matching headers установлены.
Backup `prepared-publication-install-v1/v2/v3`, installer+hash-guard:
`Tools/Diagnostics/WorldScapePreparedPublication/Install.ps1`.
`-Rollback` возвращает v3 к v2(defaultOFF). Полный возврат до эксперимента:
последовательно v3,v2,v1 с явным BackupLabel. Исходник узкого APS tag-патча
сохранён отдельно в `prepared-publication-project-source`; не затирать будущие правки.

## Проверки и evidence

Native private v1/v2/v3 по8/8,0testwarnings/failed: точные498576vertex comparisons,
UV0..3, missing channels, линейный color, handedness, bounds,indices,
ограничение бюджета/лизинг/отмена/reset, worker ownership. V2/v3 побайтовые
packed CPU-buffer проверки и format-fallback, v3 auto/off/force owner guards.
NullRHI не доказывает визуал. Кадры грунта off-v1/on-v1/packed-on-v2 просмотрены:
география, рисунок и свет совпадают в этих контролях.

V3 Terra default, native water payload opt-in, original water material:
4/4test (2сwarnings),18sample changes, max UV1 error в пределах0.01м,
30oceansections; PNG000/020/030/060 просмотрены. Вода с широким бликом,
бугристостью и рваным берегом осталась как прежде; НЕ водная визуальная приёмка.
Frozen/Oasis default: каждый4/4(2сwarnings), обычные type14/type25 подтверждены;
просмотрены Plus8s ground и Published000/030/070 у каждого. Не новая палитра,
не all-frame бесшовность: только отсутствие регрессии в этих контролях.
За3default-прогона12/240PNG просмотрены,12/12tests (6сwarnings),0failed/notRun.
Warnings включают старые startup-log warnings и предупреждение о частых
FindConsoleObject для TerrainContinuity; не скрыты и не названы чистыми тестами.
Полные report/index.json сохранены. Все8Shared uasset совпали сbefore-хэшами
во всех11завершённыхпрогонах (44/44tests,22tests сwarnings). Последний trace
default отдельный; его времена не смешаны сclean pairs. Private3suite24/24
безtestwarnings. Всего14контролируемыхUE-сессий, все завершены. Последний
UE17236 вышел02:52:41, Insights10656 —02:53:45; процессовUE/compiler/Insights0.
Общий PNG count243 дляdefault3+trace;12из240 визуальныхdefaultкадров просмотрены,
ещё3наземныхконтроля изстарыхoff/onпар просмотрены отдельно. Не весь ролик.

База evidence `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`:
- terrestrial-wet-prepared-off-v1/on-v1/trace-v1 (V1, не принят);
- terrestrial-wet-packed-on-v1/off-v1/off-v2/on-v2 (чистые V2 пары);
- terrestrial-prepared-default-water-motion-v1 (V3, WaterPayload без замены материала);
- frozen-prepared-default-motion-v1 (V3, ground hold + движение).
- oasis-prepared-default-motion-v1 (V3, ground hold + движение);
- terrestrial-prepared-default-trace-v1 (V3 default attribution, не чистый timing).
Private tests/builds/install manifests и `prepared-publication-v2-timings.json`:
`F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/`.

## Дальше по полной цели

Открыты настоящее preload/eviction по относительной скорости/направлению,
низкий быстрый физический полёт, rebase/смена тела и RAM/VRAM/очереди;
сохранение поверхности при долгой смене режима. FrozenVisible всё ещё держит mesh.
Вода/берег ordinary-enable, остаточный визуал orbit↔ground, облака/атмосфера,
бюджетное наполнение и остальные семейства остаются в плане. Этот выигрыш
не подменяет их и не означает завершение спринта. Корабли/звёзды/свет не правились.

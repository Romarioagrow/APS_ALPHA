# Фоновая физическая глубина воды — 30.09.2026

## Цель и граница изменения

Заменить диагностический GT snapshot514760 вершин (~1.5с) данными из
существующего native LOD-worker. Сохранить геометрию, цвета RGBA, UV0,
коллизии, seam normals и принятую схему владения задачами. Не переносить
старые ripple/fill/lighting эксперименты вместе с полезным каналом UV1.

TEXCOORD1=(signed depth km, valid). Отрицательная глубина суши сохраняется
до интерполяции; физическая глубина вычисляется из ocean/ground displacement
в одной системе координат. NaN/Inf/неподдерживаемый масштаб дают valid0.
Отключённый тег не делает второго height query. Повторно используемый LOD
обнуляет прежний payload, не оставляя воду от предыдущего назначения.

## Изолированная проверка

Private host: F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild.
Перенесены9 native-файлов; полный source comparison исключает любые другие
смысловые изменения. Root_Main отличается лишь переводами строк.
Полная сборка8модулей202 actions/max2 прошла за801.71с. Для UE5.4/MSVC14.50
нужен тот же compatibility define, что в APS target: __has_feature(x)=0.
Первый запуск без него остановлен; системные engine headers не правились.

native-payload-v1:6/7 PASS. Единственная ошибка —18 старых ожиданий записи
worker в pending-map. Принятый worker-fence специально запрещает такую запись;
тест приведён к актуальному контракту (pending сохранён/false после future.Get),
а реальная GT-публикация проверяется отдельным WorkerCompletionOwnership.
Production код ради прохождения теста не ослаблен.

native-payload-v2:7/7 PASS,0warnings,0notRun; 0.589с тестов.
Water Units/Payload/WorkerCoordinates + CollisionSamplingParity,
WorkerCompletionOwnership, LodCoincidentNormalPayload, LodGeneratedSewingNormals.
600 аналитических coordinate samples (546wet/54dry);47204 actual generated
vertices приresolution96/LOD0,5,9, два subposition, три секции. Geometry/RGBA
точно совпадают on/off; после opt-out reused payload очищен.
Это NullRHI, НЕ визуальная или игровая performance-приёмка.

Evidence: F:/ChatGPT/APOSFERA/work/planet_water_live_20260929/native-payload-v2/.
Reproduction: Tools/Diagnostics/WorldScapeWaterDepthIntegration/
RunNativePayloadTests.ps1, native-worker-20260930.patch (9files/661lines,
reverse-check PASS). Старый integration.patch целиком НЕ применять.

## Установка и защита

InstallNativePayload.ps1 ограничен Development Editor и фиксированными путями.
Он проверяет7/7 и BuildId33043543, сравнивает исходники, копирует backups,
обновляет все8модулей, import libraries и три совпадающих UHT generated headers,
затем пересобирает APS. При ошибке возвращает plugin и APS DLL/PDB/modules.
Сессии Unreal не прерываются. Packaging/DebugGame этим НЕ проверены.

Первая транзакция before-installed полностью отменена с восстановлением: source/generated
LodData.h не совпали по GENERATED_BODY line keys. Прежние APS F7A03587...
и Core02ACA9A8... hashes после отката подтверждены. Заголовки добавлены
в согласованный набор; затем исправлен const-pointer в read-only diagnostic
GetProcMeshSection; вторая транзакция также автоматически восстановлена.

00:17: транзакция before-installed-v3 успешно завершена,4actions/15.65с
после исправления diagnostic const. Все8 native DLL/import libraries и три
UHT generated headers согласованы с новой APS DLL. Нет чужих source-изменений.
APS SHA256 F3B392948B3AE42448A4215563D04F664F410D690188EB0F069202CCA4E9904E.
Core SHA256 58AC6497F0073A3376294ED7984AE38A1B9B3A8CCFE3628950878815B46A60B5.
Rollback: InstallNativePayload.ps1 -Rollback -BackupLabel before-installed-v3,
только при отсутствии редактора/компилятора. Не возвращать отдельно один Core.

Первый игровой native-column прогон Terrestrial2км:514760 water vertices,
318583wet/257602shallow80m;1031oracle samples, max error0.00003385м.
UVwrites=0, diagnostic readback10.013мс (разовая проверка, НЕ цена игры).
Тяжёлый snapshot1.5с в этом пути не выполняется.

00:22: оба rendered Terrestrial2км/50м закончились по1/1 сwarning, без errors.
В50м прогоне514760vertices,410986wet/410646shallow80m,1031oracle samples,
max error0.00000958м, diagnostic check9.383мс, UVwrites0. Во всех фазах
hashes terrain/ocean совпали; оригинал восстановлен после candidate.
Оба прогона сохранили все23защищённых uasset без изменений.

Просмотрено6/30PNG:2км Oblique0/1 и Limb0/1,50м Oblique0/1. Более тёмная
зелёно-синяя гамма читается, но жёсткая граница суши осталась;50м Oblique
вообще не содержит берега и НЕ подтверждает его качество. Блики/слабые
угловатые контуры на ближней воде ещё видны. Default material не включён.
Evidence: F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/
terrestrial-native-column-art-v1-2km и terrestrial-native-column-art-v1-50m.
Это static material-only A/B, не летящий корабль и не готовое мелководье.

Далее RunPayloadMotion.ps1: собственный game20s run+sprint+look40,
warmup30s,1600x900, одинаковая DLL/генерация. Off/on меняет ТОЛЬКО worker UV1,
материал прежний. ВАЖНО: фактическая проверка показала, что этот launcher
использует выбранный мир меню, а не фиксированный Terrestrial; см. ниже.

## 00:33 — движение: Frozen opt-out, НЕ стоимость воды

В обоих новых game logs: subtype=Frozen, ocean=false, seed1337,
OceanLods=0. Включение process flag корректно не запускает WS_OceanDepth:
в обоих trace0 вызовов за20 регионов CharProbe. Поэтому пара НЕ измеряет
цену активной воды. Эта граница обнаружена проверкой trace/Profile и явно
исправляет первоначальную рабочую интерпретацию.

| Метрика | Flag off | Flag on (Frozen, gate OFF) |
| --- | ---: | ---: |
| Frames | 2328 | 2317 |
| Avg / median, ms | 8.59 / 7.96 | 8.63 / 7.99 |
| p95 / p99, ms | 12.35 / 19.61 | 12.93 / 19.54 |
| Max, ms | 35.80 | 35.70 |
| Frames >33ms | 1 | 1 |
| WS_OceanDepth calls | 0 | 0 |

Один парный dry-world regression, не ship FPS и не цена wet-ветки.
Спавн в обоих случаях WorldScapeMeshCollision: expected23574.05/hit23574.07,
ShouldMove drops0, speed1477/1478cm/s. Traces разобраны all-thread statistics
для20 CharProbe regions; процессы завершились. Нельзя складывать вложенные
scopes или выдавать summed worker seconds за GT frame time.
Evidence: work/planet_water_live_20260929/motion-payload-{off,on}-v1.

Следующий wet performance probe должен принудительно выбирать Terrestrial
через существующий rendered fixture, заранее проверять ocean=true и наличие
WS_OceanDepth, а не повторять launcher текущего мира меню. Модифицировать
файлы кораблей ради такой фикстуры не нужно. Предыдущий collision A/B также
оказался Frozen; подпись исправлена в2026-09-29-flight-performance.md.

Канонический opt-in: -APSWaterDepthPayload только owned full-scale generated
native Water roots, не manual/scaled preview/lava/ice. Default off.
RunPlanetWaterNormalAB.ps1 -NativeColumn -ColumnArtPalette включает A/B,
в котором UV1 только читается, oracle выборочно сверяется с физическим дном;
никакого запасного GT snapshot, если payload отсутствует. Сам A/B замораживает
геометрию после завершения workers и НЕ доказывает движущуюся стоимость.

## Что ещё обязательно

Игровой native-column rendered A/B и source/UVwrites0 validation; затем
движущаяся граница/повторная загрузка и цена дополнительного noise query,
очередь/CPU/GPU/RAM. При успешной проверке — обычный выбор материала/глубины,
а не вечный выключенный эксперимент. Ни вода, ни motion-aware residency
не считаются законченными этим инфраструктурным шагом.

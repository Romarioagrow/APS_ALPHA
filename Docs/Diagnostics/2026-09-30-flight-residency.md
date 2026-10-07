# Flight residency — checkpoint30.09 04:37, default OFF

## V6 установлен, visual acceptance НЕ пройдена (04:37)

APS A4D1A5BE860C8B5E0DA2D509B6F5239300D9C4AEDA0085664CB6FB6AF2E1AB96;
Core 78A611F1052141150419E61A3E6BA20A9BB66C50C7770929941C01C78FF94B6A.
Private9/9 без warnings: инкрементальная и прежняя сборка геометрии совпадают,
3,289,056 vertex comparisons. Это не визуальная приёмка. Покадровое создание
только скрытого кандидата; один base LOD за кадр. Collision/retirement прежние.
Первая APS сборка встретила параллельно изменённый ship header после UHT;
повторная сборка устойчивых источников PASS. Чужие исходники не правились.
Backup base-batch-install-v1; rollback пересобирает текущий общий APS source,
не возвращает старую DLL поверх работы Claude.

Frozen V6:4/4tests(2 с warnings),3028frames,p95=14.483,p99=16.465,max101.869ms,
3кадра>50ms — все на возврате. Начальная подготовка~31ms, но общий максимум
остаётся~102ms. Чистой same-DLL synchronous/staged пары нет; общий FPS выигрыш
не заявляется. Evidence frozen-residency-v6-batch1.

Terrestrial и Oasis V6 visual: каждый4/4tests(2 с warnings),2prepare/commit/
retirement. PNG внутри маршрута: эти frame timings НЕ performance benchmark.
Terrestrial реальные ocean=true. Просмотр кадров020->021->022 и074->079
выявил изменение контура/детальности берега при256<->64. Даже без дыр это
НЕ соответствует требованию непрерывного вида. Автотесты проверяют coverage,
но не эту визуальную регрессию. Поэтому обычное включение запрещено до
исправления screen-space качества/непрерывности береговой геометрии.
Oasis runtime завершён; его visual acceptance ещё не выполнена.

Evidence: terrestrial-residency-v6-visual1, oasis-residency-v6-visual1 в
F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/. Все эти процессы завершены.
aps.Surface.FlightResidency=0; подготовка не включена в обычный полёт.
Следующая практическая ветка — доведение воды/берега до обычного включения;
пики swap/collision и видимость transit LOD остаются отдельной открытой работой.
Вода/облака/атмосфера/наполнение и остальные типы из эпика не исключены.

## V5 и адресная подготовка V6 (04:15)

Установленная APS DLL C929E7D01A771AB0322D43D7D5BE25A2A08204AF5370902F2BE8C737C27649A1;
native Core00E85757 пока без изменений. Пустой кандидат заранее подтверждает
Prev_* profile cache: это убирает вторую GenerateBaseMesh в первом Tick.
Private FreshRoot+ownership+publication3/3 (один тест с8fixture warnings).
Сравнены1,644,528вершин; dry256 NullRHI124.6→65.0мс, wet256284.9→139.9мс.
Это микротест, НЕ игровой кадр и не готовая плавность.

Frozen V5 ON:4/4(2сwarnings),2prepare/commit/retire;3401frames, p95=11.684,
p99=14.272,max102.236мс,4кадра>50мс — все возврат. Транзит max15.680мс.
Снижение максимума против V4~164мс — наблюдение двух прогонов, не чистая
same-DLL A/B и не среднийFPS. Registered buffers102.31→5.53→95.95MiB;
RAM10830.6→~10803→11064MiB. Полная память/VRAM всё ещё не доказаны.
Evidence: frozen-residency-v5-on1 в прежней evidence базе.

Отдельная CPU trace frozen-residency-v5-trace1:4/4(2warnings), безPNG вROI.
В экспортированном GameThread CSV:
-72.1698–72.2800:WorldScapeRoot110.228мс в первом Tick кандидата;
-72.7342–72.7929:EndOfFrame component update58.668мс;
-72.7993–72.8885:CollisionLodHandler89.267мс;
-74.9413–75.0033:CollisionLodHandler62.045мс.
Это разные пики, не один буфер/одна причина. В первом scope нет внутренней
метки GenerateBaseMesh; добавлена WS_BaseMeshBatch в private кандидате для
следующей атрибуции. У Insights есть MemAlloc TagTracker ошибки (memory
канал не записывался); никакие выводы по allocation/VRAM из него не делаются.

V6 пока ТОЛЬКО исходник/private build: общий base-construction helper,
пустая скрытая tick-disabled замена получает1LOD закадр, ни один worker
не видит частичную base mesh. Старый root остаётся видим до прежней полной
coverage/height проверки. Коллизии и retirement не меняются. APS источник
подготовлен под новую native функцию, поэтому до согласованной установки
не считать рабочее дерево собранным. RunPlanetFieldsFlight теперь проверяет
свежесть этого CPP. Guarded installer InstallFlightBaseBatch.ps1 требует9чистых
private tests и защищает production baseline hashes. FlightResidency default0.

## Предыдущее подтверждение V4 (03:43)

## Итог этого прохода — не готово к ordinary enable

Установлен APS `1502B51FFEE583A784C6B14B31A6B7DD9F75EC826E2C26562E5D6D00386A9A80`.
Native Core остаётся `00E857570217EC4E5A4969F8DE78FC0DEDEAD95C55BC2D6C8F4876A81EA1A162`.
Сборки v1/v2/v3/v4 успешны; v4 подхватила параллельные source-изменения
другого разработчика (их не редактировала и не откатывала). Не откатывать DLL
к старому backup поверх этих чужих изменений. Переключатель OFF — безопасный
возврат к обычному пути; старые prepared-publication улучшения остаются включены.

V4 Frozen ON:4/4 tests (2сwarnings),2prepare/2commit/2retirement, без отмен.
Маршрут100→8000→100км,33с, реальный pawn перемещается скриптом в gameplay;
это НЕ проверка физической модели корабля. Во всех диагностических срезах
нет incomplete/hidden root; на100км возврате восстановлен generate-collision.
Начальные зарегистрированные буферы:611935vertices /102.31MiB /33meshes.
Устойчивый транзит:33416vertices /5.53MiB /10meshes,0collisionLods,0workers.
Возврат:573910vertices /95.95MiB /24meshes,14collisionLods,4anchors.
Удаление старого root подтверждено штатным drain, но это не вся RAM/VRAM:
RAM процесса вначале10849.6MiB, транзит10857.1MiB, конец11294.1MiB.
Глобальное освобождение памяти не доказано; UObject lifetime/GC ещё проверить.

V4 ON:3521frames, p95=11.521ms,p99=13.908ms,max164.198ms,t=19.944s.
4кадра>50ms, все на возврате; transit-hold p99=9.964ms,max13.093ms,>50ms=0.
Периодический конфликт коллизий V3 устранён, но пика подготовки dense root
и последующих пиков~90/75/68ms достаточно, чтобы НЕ принимать плавность.
Нативный CreateMesh всё ещё синхронно создаёт/инициализирует сразу всеLOD;
это следующий кандидат для атрибуции trace, а не уже доказанная причина164ms.

V4 OFF:4/4(2сwarnings), без передачи. **Нельзя использовать как чистую пару**:
ROI начат03:39:21.218, другой обычный UnrealEditor15872 запущен03:39:37.852
с `APS_ALPHA.uproject -skipcompile`, то есть наложился на последние16с.
Этот процесс не мой, не закрывала. Поэтому не заявляю процент выигрышаFPS.
`v4-summary.json` сохраняет сырые метрики, а не признак валидной A/B-пары.

Всего5completed rendered/session запусков этой итерации:20/20tests,
10tests сwarnings. При этом **видео визуальная приёмка V4 не выполнена**:
внутри performanceROI PNG не снимались. Просмотрен V4 Frozen Plus8s ground
как pre-flight контроль: принятый снежный грунт/рельеф сохранены.
У V1 ранее просмотрены route PNG018/020/110; это не доказательство V4
и не покрытие мокрых миров. Terrestrial/Oasis ещё не запускались с новым tier.
Все8Sharedassets неизменны относительно before во всех5запусках.
Selection-test fixture восстановлена до backup побайтово (SHA25668B5FDCC...).
В обычной игре новые режимные переключения **OFF**. Корабли, звёзды,
география, материалы и native plugin этим проходом не изменены.

Следующее: trace пика подготовки/публикации; ограниченное по кадрам создание
скрытого replacement без чтения/очистки worker-owned buffers; чистая OFF/ON
сериализация запусков; rendered-маршруты мокрых семейств; низкий быстрый
полёт/коллизия и rebase. Не ждать бесконечно подтверждения чужого окна:
пока оно занято, доступен read-only source/trace audit и изолированная подготовка.

## Задача и ограничения

Требование пользователя: на подлёте заранее готовить качественную поверхность,
при удалении/транзите освобождать ближнюю нагрузку, но не терять грунт/коллизию
при быстром низком полёте. Это часть полного планетного эпика, не его замена.
Принятый наземный вид, материалы, seed и география сохраняются.

## Реализованный кандидат

- `aps.Surface.FlightResidency=0` по умолчанию, только явные диагностические ON.
- Решение по высоте, радиальному движению и ближайшей точке прогноза на8с.
  Внутренняя граница max(200км,0.20R), внешняя max(500км,0.35R), dwell1.5с.
  Скорость измеряется относительно центра планеты; общая смена начала координат
  должна сокращаться. Реальный ship/rebase пока не проверен визуально.
- Одна скрытая замена WorldScape globally. Плотность256→64 при неизменном
  диапазоне колец, те же материалы/шум. Старый root видим до полного payload
  и height-oracle проверки нового. На возврате заранее строится плотный root.
- После атомарной передачи используется существующий native worker drain +
  Destroy. Ручная очистка внутренних буферов была отклонена проверкой
  безопасности и НЕ применена. CPU память до GC не объявляется освобождённой.
- Диагностика отдельно считает зарегистрированные mesh/published buffer bytes,
  RAM всего процесса и кадры без PNG внутри измеряемого маршрута.
- Сейчас только активное full-scale generated тело без активного foliage.
  FrozenVisible siblings, облака/эффекты и foliage этим не оптимизированы.

## Промежуточные результаты (03:36, НЕ окончательная приёмка)

V1 rendered Frozen:4/4 (2tests сwarnings), маршрут100→8000→100км.
Передача состоялась, но возврат повторно отменял prefetch из-за длинного кадра.
V2/V3 исправляют оценку скорости и добавляют выдержку до отмены near-prefetch.
V3 Frozen ON/OFF: каждый4/4 (2сwarnings), ON ровно2prepare/2commit/2retirement.
Однако ON дал31кадр>50мс против0 OFF. Коллизии площадок повторно включались
и уничтожались каждым streaming refresh; кандидат НЕ включён в обычной игре.
V4 исправляет конфликт и сохраняет actor identity placement anchors при swap.
Проверка V4 идёт; не переносить предварительные цифры V3 в итоговый результат.

Policy-v1: FlightResidencyPolicy и StreamingSelectionPolicy PASS;
старый StreamingSelectionAndStandby FAIL на регистрации первого controller,
до обращения к flight residency. Попытка поправить fixture через
InitializeActorsForPlay вызвала invalid WorldContext assertion (policy-v2);
эта тестовая правка полностью отменена. Policy-v2 FlightResidencyPolicy PASS,
но весь запуск упал, общего PASS нет.

Evidence: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`:
`frozen-residency-v1`, `frozen-residency-v3-on1/off1`, `frozen-residency-v4-on1/off1`.
Backup/build/policy: `F:/ChatGPT/APOSFERA/work/planet_flight_residency_20260930`.

## Оставшаяся приёмка

Чистая same-DLL OFF/ON; пики подготовки/передачи/уничтожения; реальная VRAM и
CPU lifetime после штатного GC; мокрые Terrestrial/Oasis и проверка их моря;
возврат/быстрый низкий полёт/смена тела/origin rebase; непрерывные кадры,
а не только coverage-флаги. До этого не называть режим готовой оптимизацией.
Вода/берега, орбитальная детализация, атмосфера/облака и наполнение остаются
открыты в `2026-09-28-planet-visual-continuity-epic.md`.

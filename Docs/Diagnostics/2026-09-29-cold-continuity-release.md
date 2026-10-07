# ContinuityV1: Ice, Tundra, Nordic

29.09.2026. Цель ACTIVE. Узкое расширение уже проверенного материала;
не художественная приёмка всех планет и не закрытие WorldScape P0.

## Изменение и защита

В точный список APSTerrainContinuityMaterial::Allows добавлены Ice/Tundra/Nordic.
Теперь ContinuityV1 default1 для Terrestrial/Frozen/Oasis/Ice/Tundra/Nordic.
Остальные типы, включая магматические, сохраняют прежний материал. Ручные
материалы не переназначаются. Откат: aps.Surface.TerrainContinuity 0 и
пересоздание планеты/PIE; существующие MID мгновенно не переключаются.

Три ContinuityV1 uasset не перезапекались, SHA256 совпадают с релизом11:01.
Shared8 защищены снимками хешей. Не изменялись география/seed, шум высот,
позиции/нормали сетки, коллизия, вода, атмосфера, свет, звёзды или корабли.
Foliage OFF. Сетка256 сохранена. Непиксельно идентичное macro-normal
затенение — ожидаемая часть замены сэмплинга, а не скрытая смена света.

Из diagnostic changes: launcher принимает Ice/Tundra/Nordic; C++ Tundra/Nordic
разрешены только для явно выбранного normal-hex/Combined либо Published flight.
Gate test по-прежнему проверяет все256 enum при0/1, включая неизвестные типы.

Diagnostic build4actions/13,44с. Release build11actions/37,43с, max2.
Release DLL SHA256: D4C8E464D1EB5D738F3A7378A5CB83258C2C77B89CCCB19650957EB2715CCD77.
Scoped git diff --check прошёл, только предупреждения нормализации CRLF.

## A/B до включения

Корень: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/.
Все четыре A/B run использовали DLL
D3C409323ECC88B4BD9CCB5A27CA4957D86AD7321AB1B80F2CEB6449C5D1AF4D.

matrix-cold-combined-menu-v1: UE25188,12:00:55–12:03:25,6/6 тестов,
36PNG. Просмотрено24: все6candidate каждого типа и native00whole/03close.
Ice/Tundra1920x1082, Nordic1914x1076 — фактический размер PNG, а не ResX.
Whole20000км и close2000км, по3ракурса. Регулярная мелкая клетка ослаблена;
крупная снежная маска Nordic стала цельнее. Плоская палитра, угловатые берега
Nordic и общая художественная незавершённость остаются открытыми.
Статические варианты внутри каждой пары имеют одинаковый размер; нельзя
использовать межсемейные числа как строго одинаковое сравнение разрешений.

Средний GPU по6ракурсам меню, native -> candidate:

| Тип | Native, мс | Candidate, мс | Разница, мс |
| --- | ---: | ---: | ---: |
| Ice | 3,237 | 3,783 | +0,546 |
| Tundra | 3,255 | 3,809 | +0,555 |
| Nordic | 3,234 | 3,694 | +0,460 |

Наземные run <type>-cold-combined-ground-v1: каждый2/2 (1warning-test),
обычная атмосфера, живой root,100км->2м(hold4с)->100км,4Hz PNG1280x722.
Ice UE27688,12:05:14–12:06:44; Tundra UE15700,12:07:44–12:09:15;
Nordic UE17964,12:10:04–12:11:34. Просмотрены по4flightPNG:
Native039, Candidate039/010/020. Это не просмотр каждого кадра пролёта.

| Тип | Samples native/candidate | Ground GPU, мс | Hold samples | Ground039 MAE / p99 RGB8 |
| --- | ---: | --- | --- | --- |
| Ice | 83/83 | 4,091 -> 4,474 | 9/10 | 10,093 / 21 |
| Tundra | 83/83 | 3,990 -> 4,390 | 9/9 | 3,309 / 7 |
| Nordic | 82/82 | 4,131 -> 4,526 | 10/10 | 1,374 / 6 |

Все496 записанных samples: presented10/incomplete0. Мелкая наземная фактура
в просмотренных парах сохранена; Ice заметнее меняет крупное затенение.
Ground GPU — mean9..11,5с hold, async counter с диагностическим overhead;
не игровой FPS, не target-resolution benchmark и не проверка hitch budget.
Тестовый сухой участок не доказывает каждый биом/берег/seed.

## Проверка обычного пути

matrix-cold-release-default-menu-v1: UE17116,12:14:49–12:17:10, без CVar
override. 9/9 тестов (5 success,4 warning),0 failed;24PNG, просмотрены8:
00whole/03close каждого Ice/Tundra/Nordic/Rocky. Новый материал выбран у трёх
изменённых типов; Rocky как отрицательный контроль сохранил Shared.
noTestSubstitution=1. Размеры как в A/B:1920x1082, у Nordic1914x1076.

Production-ground <type>-cold-release-ground-v1, без подмены материала тестом:

| Тип | UE PID | Выход процесса | Tests success/warning/failed | Flight PNG/samples |
| --- | ---: | --- | --- | ---: |
| Ice | 18780 | 12:19:40 | 3/1/0 | 83 |
| Tundra | 12808 | 12:21:33 | 3/1/0 | 83 |
| Nordic | 23236 | 12:26:06 | 3/1/0 | 82 |

Runtime parent ContinuityV1, noMaterialSubstitution=1. Каждый gameplay run
задаёт CVar1; отсутствие override и сам default отдельно проверены меню.
Все248 recorded samples presented10/incomplete0. Просмотрены6PNG:
Published010 и Published039 каждого типа. Детализация согласуется с A/B,
заметных новых дыр на этих кадрах нет. Бледный снег Tundra, тёмный Nordic
и крупные полосы не объявлены художественно завершёнными. Сэмплирование4Hz
и шесть просмотренных кадров не доказывают отсутствие popping между кадрами.

Все8 run snapshots Shared8 перепроверены: изменений0; SHA256 DLL совпадает
с release выше. Три ContinuityV1 uasset также неизменны. Все процессы вышли,
после12:26 остаётся только анализ/документация, окно редактора возвращено.

## Незавершённое

Другие типы, вся непрерывная траектория/скорости/LOD и пользовательская
художественная приёмка остаются в эпике. Берега, вода, гамма, атмосфера,
облака и наполнение здесь не менялись. Проверка реального рендера отделена
от успешной сборки согласно unreal-rendered-visual-validation.

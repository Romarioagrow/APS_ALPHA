# APOSFERA / APS_ALPHA — вводная для Claude

Обновлено 2026-09-27. Конкретные задачи выдаёт Rio; этот документ не поручает
самостоятельно переделывать все подсистемы.

## Сначала прочитать

1. [Аудит проекта](Docs/Audit/2026-09-27-project-state.md).
2. [Принятый визуальный checkpoint](Docs/Checkpoints/2026-09-27-worldscape-accepted.md).
3. [Указатель документации](Docs/README.md) и [координация](Docs/coordination/CURRENT_WORK.md).
4. [Evolution over rewrite](Docs/ADR_EVOLUTION_OVER_REWRITE.md).

## Пути и baseline

- Основной Git/Unreal: `F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA`.
- Ветка при аудите: `dev-3`. Каждый раз проверять HEAD и `git status`.
- Рабочие отчёты/стенды: `F:/ChatGPT/APOSFERA`; это не production checkout.
- Старый `C:/Users/Rio/Documents/ChatGPT/APOSFERA` больше не рабочий.
  Новые отчёты и стенды размещать на F:, не C:. X: не текущий путь.
- UE 5.4: `C:/Program Files/Epic Games/UE/UE_5.4`; движок не переносили.
- Принятые визуалы: коммит `67fff3b4`. Пользователь отметил хорошие планеты,
  лаву, атмосферы, звёзды, переходы и около 120 FPS в быстром тесте.
  Это пользовательская приёмка, не глобальный benchmark/закрытие всех дефектов.
- `Tools/Diagnostics/WorldScapeWaterDepthIntegration` — приватный эксперимент,
  **не установленный в production**. Не применять patch автоматически.

## Границы работы

- WorldScape остаётся terrain/LOD/collision backend. Не заменять его Landscape,
  второй сферой или отдельной моделью планеты.
- Сохранять seed, StableId, географию, палитру, радиусы/орбиты, save-контракты,
  гравитацию/управление и принятый вид персонажа.
- Preview, generated gameplay и authored Start Single Game — разные маршруты.
  Authored-карту не перегенерировать при входе.
- Dirty/staged изменения других участников не откатывать и не включать в свой
  коммит. До пересечения файлов согласовать владение. Старые Dev 2 / Dev Surface /
  Dev UI в ledger — исторические роли, не вечные блокировки.
- Общие зоны: AstroGenerator, WorldGenerationViewModel, SAPSMainMenuRoot,
  surface profile/material commandlet, Build.cs, Config, saves/controller,
  character и бинарные assets. Для них особенно важен узкий handoff.
- Не закрывать пользовательский Unreal и не заменять загруженные DLL/assets.
  Один тяжёлый build/render/bake за раз, в согласованное окно.
- IDE при необходимости — Rider, не Visual Studio; MSVC toolchain допустим.
- Перед запуском bake/recovery проверить параметры, версию модуля, точные
  targets и backup. Наличие скрипта не означает разрешение его запустить.
- Визуальный PASS требует кадров/движения затронутых семейств. Compile, hash и
  readiness-log подтверждают только свои свойства. Если тестирует пользователь,
  честно обозначить непроверенное.
- Коммитить только свой явный список файлов. Не делать общий `git add .`,
  force/reset/push или удаление данных без соответствующей задачи.
- Handoff: файлы, commit, выполненные проверки, ограничения и следующий шаг.
  Беречь лимит: переиспользовать сохранённые исследования, не повторять их вслепую.

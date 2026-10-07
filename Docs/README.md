# APOSFERA — документация проекта

Обновлено 2026-09-27. Новому участнику начать с [CLAUDE.md](../CLAUDE.md),
затем с [актуального аудита](Audit/2026-09-27-project-state.md).

## Текущее состояние

- [Аудит 2026-09-27](Audit/2026-09-27-project-state.md): структура, зависимости,
  границы реализации, риски восстановления, тесты и backlog.
- [Принятый визуальный checkpoint](Checkpoints/2026-09-27-worldscape-accepted.md):
  что пользователь одобрил и где внешняя резервная копия.
- [Совместная работа](coordination/CURRENT_WORK.md): актуальные пути и границы задач.
- [Water-depth integration](../Tools/Diagnostics/WorldScapeWaterDepthIntegration/README.md):
  сохранённый приватный эксперимент, не production rollout.

## Архитектура и направления

| Область | Входной документ | Как читать |
| --- | --- | --- |
| Общие границы | [Evolution over rewrite](ADR_EVOLUTION_OVER_REWRITE.md) | Каноническая модель и разделение маршрутов; статусы этапов датированы |
| Генерация | [Dev 2 journal](coordination/DEV2_CHECKPOINT_JOURNAL.md), [baseline](coordination/GENERATION_BASELINE_2026-09-11.md) | История решений и evidence pointers |
| WorldScape | [Surface ledger](coordination/DEV_SURFACE_TASK_LEDGER.md), [handoff](DEV_SURFACE_SHARED_HANDOFF_2026-08-14.md) | Старые гипотезы не переоткрывают принятый вид автоматически |
| Placement / Civilization | [Placement API](coordination/DEV_SURFACE_APS_78_PLACEMENT_API_CHECKPOINT.md), [UI contract](UI_UX_APOSFERA/CIVILIZATION_MATERIALIZATION_UI_CONTRACT.md) | StableId, actor-ready, идемпотентность |
| UI | [UI ledger](coordination/DEV_UI_CHECKPOINT_LEDGER.md), [QA matrix](UI_UX_APOSFERA/APOSFERA_UI_QA_MATRIX.md) | Дизайн, контракты и визуальная приёмка — разные статусы |
| Interaction / Production | [APS-81](coordination/DEV_GAMEPLAY_APS_81_INTERACTION_PRODUCTION.md) | Проверять интеграцию по коду, не только старую строку Status |
| Quest / Onboarding | [APS-80](coordination/DEV_QUEST_ONBOARDING_APS80.md) | События, identity, persistence API; не свидетельство полного прохождения игры |
| Звёзды | [Checkpoint 12 сентября](StellarRenderingCheckpoint/README.md) | Исторический; более поздняя пользовательская приёмка — 27 сентября |
| Визуальный ориентир | [Снимки NASA](Design/VISUAL_NORTH_STAR.md) | Канон Rio от 08.10 для всего космоса; эталон — галактика в меню генерации |

## Архив вне репозитория

Каталог `F:/ChatGPT/APOSFERA`: `PROJECT_MEMORY.md`, `WORLDSCAPE_DOCUMENTATION_2026-09-15.md`,
`STELLAR_BASELINE_2026-09-20.md`, `SURFACE_DETAIL_2026-09-15.md`, `work/`, `stage/`,
`recovery/`, `test_reports/`, `design/`. Старые абсолютные ссылки на C: внутри
исторических логов относятся к состоянию до переноса. Не переписывать хэшируемые
отчёты задним числом: добавлять текущий указатель или migration record.

Свежая проверка кода показывает текущее устройство; пользовательская приёмка —
проверенный им вид; automation/render-report — только свою сцену и условия.
План, patch и отчёт о сборке не означают установку в production или визуальный PASS.

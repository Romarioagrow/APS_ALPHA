# Civilization Gameplay Materialization — Dev UI contract

Parent milestone: Civilization Gameplay Materialization / Foundation vertical slice  
Dev UI checkpoint: `UI-001`  
Status: read-only contract; runtime bindings deferred

## Ownership boundary

- Dev 2 owns the canonical civilization/world model and shared manifest handoff.
- Dev Surface owns safe-ground selection, planet-relative orientation and terrain/WorldScape integration.
- Dev Civilization owns runtime base/pad/selected-ship materialization and save/idempotency.
- Dev UI owns presentation after those contracts exist. It must not choose a surface site, orient actors, add surface artificial gravity, reroll a ship or manufacture a parallel civilization/spawn model.
- `Source/APS_ALPHA/Generation/AstroGenerator.cpp` and `Source/APS_ALPHA/Tests/APSMainMenuPreviewSmokeTests.cpp` are temporarily owned by Dev 2 for the SYSTEM secondary-star pass and remain locked until freeze.

## Minimum read-only UI manifest

The later UI slice should consume one canonical runtime record with stable, displayable identity for:

- civilization ID and seed;
- home/start planet stable ID;
- starting-base kit catalog ID;
- landing-pad catalog ID;
- selected fleet ship ID/catalog entry;
- documented ship/base fallback reason when the requested asset is unavailable;
- deterministic or persisted placement identity/transform;
- materialization state: `Pending placement`, `Materialized`, `Loaded from save`, `Blocked`, or `Fallback asset`;
- save/version metadata sufficient to explain a migration or idempotency problem.

The exact shared data contract must be agreed with Dev 2 and Dev Civilization before any UI binding or ViewModel edit.

## Presentation rules

- Show base, pad and ship identity as facts, not spawn buttons, when the canonical manifest already owns the action.
- Pair every readiness/materialization state with text and a symbol; color is secondary.
- Surface base and pad copy says `planet-aligned` or equivalent. Artificial gravity appears only for an explicit orbital/free-space capability.
- Missing manifest fields are an actionable error state, not an empty card.
- A fallback must name the requested item, actual item and reason.
- Reload/return states must never suggest that a duplicate base or ship should be created.

## Acceptance checks for the later UI slice

- Menu and gameplay display the same civilization, planet, base, pad and ship stable IDs/catalog selections.
- A selected ship fallback, if any, is visible and matches the runtime manifest.
- Pending, materialized and loaded-from-save states match Dev Civilization actor/save evidence.
- UI has no placement RNG, no terrain query and no independent spawn command path.
- Same-seed and reload tests show one canonical base/ship identity without duplicate presentation.
- Generation → surface base → ship screenshots/video include the UI manifest facts without changing scene exposure.

## Current evidence

- `Docs/UI_UX_APOSFERA/APOSFERA_UI_AUDIT_2026-08-14.md`
- `Docs/UI_UX_APOSFERA/APOSFERA_UI_TOKENS.json`
- `Docs/UI_UX_APOSFERA/APOSFERA_UI_QA_MATRIX.md`
- `Docs/UI_UX_APOSFERA/mockups/aposfera-ui-evolution.html`
- `Docs/coordination/DEV_UI_CHECKPOINT_LEDGER.md`

Result: `PARTIAL` — the UI contract is defined, while runtime binding correctly waits for the coordinated canonical/placement/materialization contracts and file ownership handoff.


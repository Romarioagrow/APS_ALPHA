# APS-77 semantic-ready WorldScape fix

Date: 2026-08-14  
Commit: `1f117d0dec8b1f7457a4f0656acbb06dba9c35a4`  
Current result: **PARTIAL — source and owned translation-unit compile PASS; link/runtime pending**

## Production defect

`UAPSPlanetEnvironmentStreamingSubsystem` polls the nearest body every `0.5 s` and re-enters `Active` when `IsWorldScapeStreamingActive()` is false. The old predicate required both `bGenerateWorldScape` and `!bFreezeGeneration`.

WorldScape legitimately freezes its producer after publishing a stable resident batch. The old predicate therefore misclassified a ready `Active` root as inactive. Re-entering `Active` cleared `bWorldScapeSurfaceReady`, hid the same root and exposed repeated approximately 0.3-second hidden windows until the readiness latch recovered. Preset-4 lifecycle telemetry observed the exact `rootHidden 1 -> 0 -> 1 -> 0 -> 1 -> 0` pattern while registration, component visibility, mesh sections, TransformKeeper parent/unit scale and anchor remained valid.

## Narrow fix

`APlanetaryBody::IsWorldScapeStreamingActive()` now requires:

- semantic state `Active`;
- a valid generator/root;
- either the published `bWorldScapeSurfaceReady` latch, or live generation with `bGenerateWorldScape && !bFreezeGeneration`.

Initial activation, readiness reset, asynchronous root hide and first-complete-payload reveal remain unchanged. `Preloaded` and `FrozenVisible` are explicitly not reported as active.

The focused `WorldScapeFamilyLifecycle` regression simulates three subsystem polling decisions after a ready root freezes its producer and verifies:

- zero redundant activation requests;
- identical root pointer;
- readiness latch remains true;
- root remains visible;
- retained LOD sentinel survives;
- `Preloaded` and `FrozenVisible` remain semantically inactive.

## Evidence

- Patch-selective staging contained only the approved predicate and lifecycle regression; foreign same-file habitability/stellar/orbit WIP remained unstaged.
- `git diff --check` and committed show-check: PASS.
- Coordinated build at HEAD `1f117d0d` failed before link on foreign incomplete Quest presentation files, but adaptive non-unity actions independently completed:
  - `[3/8] Compile [x64] PlanetaryBody.cpp` — no errors;
  - `[5/8] Compile [x64] APSGameplayIntegrationTests.cpp` — no errors.
- Build log: `C:/Users/Rio/Documents/APOSFERA SPACETRIPS/APS_ALPHAEditor_build_1f117d0d_integrated_20260814.log`.

## Remaining gates

1. One stable-tree integrated link after the Quest owner checkpoints the crossed WIP.
2. Focused `APS.Gameplay.World.WorldScapeFamilyLifecycle` automation.
3. One fresh preset-4 lifecycle render diagnostic proving the repeated post-ready `rootHidden` transitions are absent while physical LOD0/material apply/restore/TestExit remain valid.
4. No visual PASS until the temporal rendered gate succeeds.

## Verification update (supersedes the status above)

Current result: **PARTIAL - integrated compile/link and focused semantic contract PASS; rendered temporal/visual verification pending**.

- Coordinated integrated compile/link at stable HEAD `3f0639b404a52417c74a4c8cd0106886bf527629` (containing Surface commit `1f117d0d`) passed and restored the editor DLL.
- The first focused launch attempt (PID `28308`) is **INVALID/UNVERIFIED**: array argument quoting lost the space-bearing project/evidence arguments, so exit code `0` produced no log or report and is not test evidence.
- The corrected single-string `ArgumentList` parsed through `CommandLineToArgvW` with `12/12` exact token matches, including `.uproject`, filter, `TestExit`, report and absolute log paths.
- The separately granted corrected NullRHI run (PID `34224`) passed the exact focused contract:
  - discovery: `Found 1 automation tests` for `APS.Gameplay.World.WorldScapeFamilyLifecycle`;
  - result: `Test Completed. Result={Success}`;
  - completion: `Automation Test Queue Empty 1 tests performed`;
  - process exit: `RequestExitWithStatus(..., 0)`;
  - report: state `Success`, errors `0`, failed/not-run/in-process `0`.
- Known non-fatal warning: temporary test-world cleanup reports `UWorld::DestroyActor: World has no context` for `WorldScapeRoot`; report classification is `succeededWithWarnings=1`.
- Evidence:
  - `Saved/Logs/APS-77-WorldScapeFamilyLifecycle-3f0639b4-ArgvFixed-20260814.log`;
  - `Saved/Automation/APS-77-WorldScapeFamilyLifecycle-3f0639b4-ArgvFixed-20260814/index.json`.

Remaining gate: one fresh preset-4 lifecycle render diagnostic. No visual PASS until that temporal rendered gate succeeds.

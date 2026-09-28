# Water normal and coast preflight (source only)

Status: **compiled; not rendered yet**. No production material, noise field,
saved asset, shader or default has changed in this pass. The accepted orbital
macro patch remains commit `ce6701fe`. The editor window belongs to Claude;
the next finite window was requested in `Docs/coordination/PLANET_EDITOR_WINDOW.md`.

## Why this experiment

Production uses saved `SharedLiquid/MI_APS_SharedWater`, with the existing
DefaultLit physical-water graph behind the shared material wrapper. Its broad
gradient-noise normal can produce the requested "lumpy water" symptom. That is
a hypothesis to isolate, not an established complete explanation. Prior custom
ripple/SingleLayerWater candidates have known separate shading/performance
problems and are not being promoted.

The old `GeneratedCivilizationWetOceanHandoff` fixture still asserts the legacy
unlit water master and nine ocean LODs. Do not use its pass/fail as current shared
water acceptance. The new optional route reuses the current rendered surface
fixture and actual authoritative water MID instead; it does not weaken the old
contract or change gameplay to satisfy a stale test.

## Controlled comparison

`RunPlanetWaterNormalAB.ps1` selects Water, Terrestrial or Oasis. It refuses to
start until Claude explicitly returns the window and no editor/compiler is active.
Every run requires a fresh evidence directory. The default camera is 50m above
the actual ocean datum, selected over a point 20m deep in the landing hemisphere.
`-OpenWater` instead selects a deeper wet location. Natural camera postprocess,
lighting, visible terrain and atmosphere are retained. No shadow/AO workaround.

For each nadir, 55-degree oblique and 5-degree grazing view:

1. `Water0Native`: original production material.
2. `Water1CopyControl`: temporary MID with all the same uniform parameters.
3. `Water2Normal40`: 40% of the actual saved/runtime wave strength.
4. `Water3Normal20`: 20% of that strength.
5. `Water4SmoothControl`: zero wave-normal strength, diagnostic control only.
6. `Water5NativeReturn`: original material again.

All terrain/ocean section payloads and component transforms are hashed and
checked at capture. Only water slot material pointers change. The original MID
is never edited. Workers must settle before the finite frozen-payload lease;
all bindings and prior tick/freeze states restore at completion and failure.
This isolation is not a production performance measurement. Candidate modes
are not installed by the diagnostic and cannot affect ordinary gameplay.

## Independent shore evidence

Coastal runs export `Saved/Diagnostics/WaterNormalAB/coast-depth.csv`: actual
`Root->GetGroundHeight` signed water depths over a 32km square, 129 x 129 samples,
250m spacing. Logs report wet fraction, sign-change edges, four-neighbour
components and fully enclosed wet/dry components. These are sampled field
statistics, **not** rendered island counts or evidence of LOD cracks. Small
features below sampling resolution are unresolved.

No shoreline is smoothed here. Full-scale physical relief near the sea retains
multiple nonzero-frequency bands while compressed preview omits them. We need
these measurements and rendered evidence before deciding whether to modify
near-coast relief, mesh resolution/coverage, optical depth, or more than one.

## Checks and next acceptance

- PowerShell parser and tracked diff whitespace checks passed.
- Claude's coordinated `work/flight/logs/build_editor5.log` compiled
  `APSGeneratedGameplayHandoffSmokeTests.cpp` and linked successfully at 07:35:53;
  the subsequent `build_editor6.log` relinked at 07:38:50 with `BUILD_EXIT=0`.
  The current DLL contains both `[APS.WaterNormalAB] BEGIN` and `[APS.CoastGrid]`
  markers. No separate Codex build was started. Screenshots and copy equivalence
  remain pending; compiler success does not establish a visual improvement.
- First render Terrestrial coast at 50m, then 2m open water; inspect native/copy/
  return controls before interpreting reduced normals. Follow with Water/Oasis
  and an orbital no-change control if a candidate is worth promoting.
- Saved Water MIC changes would affect more families than these three. Do not
  globally publish from this limited matrix; either expand family coverage or
  explicitly scope a production candidate after the first comparison.
- Still open: shoreline fragmentation/angular edges, physical shallow-water
  transition, other family coverage, atmosphere and live-flight performance.

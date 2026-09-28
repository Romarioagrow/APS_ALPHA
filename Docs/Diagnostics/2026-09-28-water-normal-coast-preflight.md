# Water normal and coast preflight

Status: **rendered; amplitude-only candidate not promoted**. No production material,
noise default, saved asset or shader changed in this experiment. The accepted
orbital macro patch remains commit `ce6701fe`. Claude released the window at
07:49; Codex ran the following two finite comparisons on build_editor8.

## Rendered evidence, 28 September

Evidence root: `F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928`.
`terrestrial-coast-50m-v1` and `water-open-2m-v1` each report one Success with
warnings, zero failed tests (55.61 / 54.94 seconds). Actual capture resolution
is 1280x722, not the requested command-line size. All three angles and six
phases completed; terrain/ocean payload hashes and transforms stayed fixed.

Native-copy and native-return controls have p99 absolute RGB error <=1/255 in
all six family/angle comparisons. This supports the comparison setup; it is
not an acceptance metric for the art. Reduced strength visibly changes water,
but broad undulations become a smoother/plastic gradient. The broad white glint
and oblique angular shading remain even in the zero-normal control. Do not
publish a strength reduction as a fix for the complete symptom.

The Terrestrial 32km grid contains wet fraction 0.631753, 2480 sign-change edges,
51 land components / 22 water components, including 40 enclosed dry and 18
enclosed wet components at 250m spacing. This is genuine fragmentation in the
sampled height field, not evidence that every rendered jagged edge is geometry.
The CSV is under that run's `Saved/Diagnostics/WaterNormalAB/coast-depth.csv`.

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
  were subsequently checked in the two runs above. Compiler success alone did
  not establish a visual improvement.
- Native/copy/return controls are checked. An amplitude-only variant is not
  worth promoting; wavelength/precision and separate shading remain open.
- Saved Water MIC changes would affect more families than these three. Do not
  globally publish from this limited matrix; either expand family coverage or
  explicitly scope a production candidate after the first comparison.
- Still open: shoreline fragmentation/angular edges, physical shallow-water
  transition, other family coverage, atmosphere and live-flight performance.

## Palette-depth source preparation (not built, baked or rendered)

The filtered depth builder now reuses the bounded palette response previously
measured in the private water-depth experiment. It resolves a uniform strength
from the actual deep/shallow colours with a 35% relative **linear palette
luminance** budget and a 20m half-depth. This is not a bound on final lighting,
tone mapping or brightness in captured pixels. It does not change normals,
roughness, light setup, endpoints, coverage or geometry. A float rounding guard
keeps the resolved strength on the conservative side of the bound. Invalid
palettes fail closed; equal-luminance/black endpoints give zero strength.

The new diagnostic output folder is `Diagnostics/WaterDepthFiltered20260928`.
Historical 20260927 packages and production Water/Ammonia packages remain
untouched. The PLANET depth probe now defaults to this filtered/budgeted route
when explicitly requested, copies the actual baseline's material uniforms and
resolves its strength from that palette. An explicit diagnostic strength still
overrides the budget and is logged as unbudgeted. The unfiltered historical
route is unchanged. Neither route is selected in ordinary gameplay.

Added `APS.Gameplay.World.PlanetSurface.WaterDepth.PaletteBounds` covers 12,100
combinations plus invalid inputs, but has NOT been compiled/run in this revision.
The shared production source hashes were rechecked unchanged:
`M_APS_SharedAmmonia=91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC`,
`MI_APS_SharedWater=30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8` (SHA1).

This preparation does not install the old private WorldScape UV1 worker overlay:
that overlay includes rejected ripple/lighting experiments and a wider ABI
change. Live physical depth delivery/performance, the new candidate bake and
paired rendered acceptance remain required before production publication.

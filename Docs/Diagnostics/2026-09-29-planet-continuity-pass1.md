# Planet continuity — first accepted-goal iteration, 2026-09-29

## Scope and protection

Active goal covers the complete accepted planet manifesto. This pass starts P0;
it does not declare shore/water/cloud/scatter work completed. Preserve the accepted
ground appearance, seeds, terrain, lighting, ships, and Claude's origin rebasing.

Claude released the editor at 00:29. Codex acquired the window at 00:31. A later
ordinary editor was launched from Rider at 00:45:20. With Rio's earlier explicit
restart permission, an identity-checked CloseMainWindow request closed that editor
normally; no force kill or save/discard action was performed. No UI input tool
was available in this session. Rebuilt only after the process exited.

## Rendered baseline

- Gameplay Frozen 100km macro A/B: existing diagnostic executed, not baked.
  `F:/ChatGPT/APOSFERA/work/planet_macro_20260928/frozen-orbit-100-sep29-baseline`.
  The old aperiodic macro candidate makes little visible difference over this
  snowy landing region. Not a demonstrated Frozen fix; do not enable globally.
- Menu Frozen: 21 settled native frames, 20000 -> 70 -> 20000 km.
  `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/frozen-native-v1`.
- Same positions, transient existing `APS_OrbitalMacroMode=1`:
  `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/frozen-macro-mode1-v1`.
  Both runs: 2/2 tests passed, 21 frames each. Eight root-level shared material
  asset SHA256 hashes remained unchanged (this is integrity, not visual acceptance).
  The material differs more near 120km than in the large snow pattern at 2000km.
  No candidate saved or adopted from this comparison.
- These are fixed-mesh material diagnostics, NOT a live WorldScape loading test.
  Actual captured viewport is 1280x722 despite the requested editor window size;
  do not call this a native-monitor-resolution performance test.

## Concrete camera defect and narrow correction

Production wheel input used `distance-to-centre * 0.82`. On a 6750km-radius
planet, a single click at 1000km altitude computes a point below the surface and
clamps directly to 67.5km. This magnifies close-up detail almost fifteen-fold in
one click and skips intermediate material footprints. This explains a component
of the reported one-wheel-step jump, not the periodic texture pattern itself.

`FAPSPreviewCameraBounds::ApplyWheel` now scales surface clearance for focused
PLANET views. One click at 1000km should reach 820km. Bounds, camera orientation,
physical mesh, materials, stars/system/galaxy law and interrupted-focus centre
law are preserved. Diagnostic inverse zoom uses the same clearance metric.
Claude's origin/spawn changes in AstroGenerator.cpp remain untouched.

Built with UE5.4, max two compiler actions, success (41.02s). This alone is not
rendered acceptance.

Runtime Frozen wheel check finished at 01:02:45: four tests passed (one rendered
test has resource/BeginPlay warnings), zero errors. Evidence:
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/frozen-wheel-fixed-v1`.
Twenty-one frames cover ten real +1 clicks and ten -1 clicks. Runtime height is
1000 -> 820 -> 672.4 -> ... -> 137.448 -> ... -> 1000.000068 km.
Inspected frames 00/01/02/10/20: adjacent early clicks no longer skip straight to
the fine-detail close-up; the return restores the initial large-scale view.
This confirms the camera correction on Frozen, NOT resolution of material tiling
or live clipmap transitions. Eight protected shared material assets are unchanged.
Do not use this run as a clean performance comparison: a separate ordinary editor
was opened from Rider at 01:01:28 while this run was in progress.

That separate editor was actively used for PIE, so it was not closed by Codex.
It exited independently at 01:10:19. At 01:11:44 the isolated Frozen warp A/B was
started after checking no editor/compiler remained. The old diagnostic candidate
is exploratory evidence only until its graph is verified against current production.

## Diagnostic correctness

- Removed silent fallback to Ocean for unlisted names such as Terrestrial/Oasis.
  Exact enum names are now resolved; hidden legacy/Unknown/typos are rejected.
- Native material checks may run on other families instead of requiring Tundra.
  Tundra-specific layer transfer remains restricted. The isolated warp-only
  candidate is permitted on other actual shared-terrain instances.
- Added repeatable altitude sweep and actual +1/-1 wheel sweep with logged
  physical heights, exact retained mesh payload/transform checks per pair,
  transient-only macro override with restoration, and settled GPU counters.
- New runner: `Tools/Diagnostics/RunPlanetMenuContinuity.ps1`; unique evidence
  directories, refusal while another editor/compiler runs, recursive shared-asset
  hash manifest for subsequent runs, no saved material edits.

## Second camera regression and material isolation

`terrestrial-wheel-route-v1` finished at 01:17:37: five tests passed, zero errors,
including `APS.Rendered.MainMenu.ContinuousPhysicalRoute`. Terrestrial produced
21 real wheel-step frames and returned to the original view. Inspected 00/01/05/20
and route planet/star controls. This validates focus/navigation plus the camera
law, not final planet art. The route resizes PIE, so its subsequent Terrestrial
captures are 1280x688; do not compare those GPU counters to Frozen 1280x722.
All eight protected production shared assets remained unchanged.

`frozen-warp-2000-v1`: six paired frames across three orbits, four tests passed.
The Sep26 candidate visibly differs in exposed-ice color as well as its evaluation
stage. It predates the Sep27 top-color bound and Sep28 macro installation: reject
this old comparison as a one-variable causal proof. No adoption.

Built a fresh diagnostic master/MIC at `Diagnostics/LodWarpPixel20260929` from the
CURRENT production source. Exactly five master interpolators bypassed; referenced
functions/palette/frame and production assets preserved. Diagnostic DLL build
succeeded in 73.47s with two compiler workers. Bake identity/hashes/log are in
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/bake-current-warp-v1`.
The commandlet first logged transient-pin compilation warnings, then repaired pins,
finished compilation, checked LocalVF/completeness/errors and saved two new assets.
This is not visual or fresh-DDC acceptance. Runtime A/B is `frozen-warp-current-2000-v1`.
Production shared hashes unchanged after bake.

Fresh runtime A/B completed at 01:24:28: four tests passed, six captured frames
across three fixed camera pairs. All six were inspected. Palette now matches
production; no convincing removal of the reported square repetition in these
views. The five-interpolator bypass is therefore NOT promoted. Settled GPU means
were native 3.192/3.322/3.239 ms versus candidate 3.167/3.262/3.205 ms (1280x722,
RTX 5080, one short run). Differences are too small for a performance claim, and
these are not live-flight/ground/residency measurements. All production hashes
remain unchanged; the diagnostic assets are not referenced by runtime selectors.

The installed WorldScape source already waits for every task in a batch before
publishing its meshes, and UpdateMesh explicitly preserves linear vertex RGB.
Do not repeat those older fixes or equate these settled-menu tests with proof
that live terrain LOD transitions are seamless. The next P0 investigation must
capture the actual objectionable square pattern at a matched camera footprint,
separate albedo/normal/material-distance changes, and reproduce the live approach.

Installed gameplay-facing delta of this iteration: PLANET preview wheel law only.
No terrain, liquid, atmosphere, cloud, foliage, lighting, collision or ship changes
were adopted. Claude committed origin rebasing as b21445f9 during this pass; current
AstroGenerator diff contains only the Codex camera include and two zoom branches.
No process remains from Codex's batch. Editor slot released at 01:27; goal ACTIVE,
not paused/blocked/completed. Re-check current coordination/processes before the
next render/build, and keep doing independent P0 work if another owner is active.

## Remaining verification

Material periodicity and mesh/LOD loading remain open. Need isolate the retained
vertex-domain coordinate warp versus pixel evaluation, not increase triangles
as a substitute. Frozen/Terrestrial camera and unrelated scope/transition controls
passed. The full goal remains active.

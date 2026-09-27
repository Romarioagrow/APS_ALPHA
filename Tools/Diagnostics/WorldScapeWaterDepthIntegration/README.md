# Isolated physical WorldScape water-depth integration, 2026-09-27

Source-only diagnostic overlay, **not installed or enabled in production**.
The accepted checkpoint remains `67fff3b4`; canonical source snapshot for this
experiment was clean `dbc3c430b5551ff3f405de41ac0017f419e631d2` on `dev-3`.
Production source, materials, engine plugin and binaries were not replaced.

## What is preserved here

`integration.patch` and `overlay-manifest.json` describe 12 source files:
the private WorldScape signed-kilometre UV1 data path and worker tests, an APS
process-wide opt-in restricted to owned full-scale generated Water roots, and
a bounded extension of the existing real gameplay liquid probe. There is no
new production material selector, saved default, geography or palette change.
The rejected compressed-preview height adapter is deliberately absent.

Patch SHA256: `21FEE5BA1DC0D4FA6914E73B2EBB7D4F0C7E8A2E3E1864EFBE6982D04E67053B`.
Manifest SHA256: `FE9525EAAA14C2A5588CE32A500324B46203ACB721BAFDE0B53D0EE8F1244E41`.
The patch was reverse-checked against the final isolated source tree.

## Evidence and limits

Private workspace:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-gameplay`.
It contains the full isolated `host/APS_ALPHA.uproject`, source snapshot,
build logs, manifests and two finite rendered diagnostic reports.

- A coherent build of APS, private WorldScape and private AtmoScape succeeded:
  470 actions, 327.11 seconds. No pre-existing plugin binaries were copied.
  All module manifests use engine BuildId `33043543`.
- `physical-water-v1`, PID 10780: real Water surface reached, three natural
  frames captured; no suitable wet triangle in the tested LOD0..4 footprint.
  The overall automation **failed before candidate/camera substitution**.
- `physical-water-v2`, PID 29156: added actual section diagnostics. All
  73,960 vertices across 10 ocean LODs / 30 sections had finite valid UV1.
  All depths were negative, approximately -260.342 to -121.4 metres: this
  dry landing is inland, not an appropriate shore A/B fixture.
  1,081 sampled vertices matched the actual root's ground/ocean noise oracle;
  maximum CPU coordinate-round-trip error was `1.65586989e-9` metres.
  No wet samples, candidate render, wet-coast quality or FPS acceptance.
- Both runs restored all 30 material slots and 73,960 original vertex colours.
  No camera replacement occurred before the failure. Both owned editors exited.
- Final source distinguishes valid all-dry data from a missing visible shore.
  That diagnostic wording/control-flow adjustment was compiled successfully
  (`build-20260927-104817.log`, 4 actions, 7.36 seconds), not rendered again.

Report v2 SHA256:
`8B260A7EE1DF6219736B2C91907B35D4AA70E1227F74BE37EB874A24F78996A2`.
Overall v2 result is 0 passed / 1 failed; narrow payload evidence does not turn
the visual test into a pass. Natural First/+3/+8-second frames show the accepted
material on dry ground, not the water-depth candidate.

## Reproduction and safety

Use a separate project, never the accepted installation. Copy APS Source/Config
and the uproject from the recorded base; place a source copy of the accepted
WorldScape under its local `Plugins/WorldScape`, and a matching source copy of
AtmoScape under `Plugins/AtmoScape`. Verify every non-null before-hash in the
manifest and absence of files whose before-hash is null. Apply the patch there
only, after `git apply --check`. Rebuild all dependent modules together.
`LodData` / `UWorldScapeLod` native layouts changed: never install only Core DLL.

The existing host has Content junctions to production. Treat them as read-only:
no bake, SavePackage, resave, content editing or recursive deletion. Build.ps1
and Run.ps1 operate a `host` adjacent to the scripts and refuse a running editor.
Run.ps1 uses a new evidence directory / UserDir and the existing natural Water
landing diagnostic plus `APSSharedLiquidCoverage`, `APSWaterDepthPayload` and
`APSProbeWaterDepthGameplay`. It will currently reject this inland fixture.

The visual probe is prepared to preserve ground visibility, physical root,
camera and every RGBA/UV/topology value while capturing original/repeat,
filtered candidate (strength .35, half-depth 20 m), zero-strength and original
return. The saved filtered Water MIC is its parameter authority. Alpha is not
repurposed as a depth mask. No other liquid family is eligible.

Next bounded step: a clearly labelled deterministic **physical shore** fixture
with real WorldScape streaming/collision convergence before freezing. Do not
hide the ground, clamp dry depths, move just the test camera into coarse distant
LOD or alter the production spawn rule to manufacture a passing water image.
Then assess actual wet rendering, zero/return controls and performance before
any production enablement or broader Water-family coverage.

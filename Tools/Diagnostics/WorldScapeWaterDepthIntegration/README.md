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

Current overlay: `overlay-v4` (the earlier dry-fixture overlay is in commit
`6e9e8e09771467eef2b0c2d9ccc4418bc339d106`).
Patch SHA256: `9422DCA55F8429F9FE85A38E40AEE4CEF41DC7B8B870D4A6DAF8C759FEC97614`.
Manifest SHA256: `89A0FEFEA83AE4C67176B8C22C87104DD2505A7C64D2D207754574610D891FF4`.
The patch was reverse-checked against the final isolated source tree.

## Evidence and limits

Private workspace:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-gameplay`.
It contains the full isolated `host/APS_ALPHA.uproject`, source snapshot,
build logs, manifests and finite rendered diagnostic reports.

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
- That revision distinguished valid all-dry data from a missing visible shore.
  That diagnostic wording/control-flow adjustment was compiled successfully
  (`build-20260927-104817.log`, 4 actions, 7.36 seconds), not rendered again.

Report v2 SHA256:
`8B260A7EE1DF6219736B2C91907B35D4AA70E1227F74BE37EB874A24F78996A2`.
Overall v2 result is 0 passed / 1 failed; narrow payload evidence does not turn
the visual test into a pass. Natural First/+3/+8-second frames show the accepted
material on dry ground, not the water-depth candidate.

## Real wet-shore follow-up

The diagnostic now searches deterministically for a day-side zero crossing of
the actual full-scale root's ground and ocean noise. It moves the real pawn
observer, waits for centred terrain/ocean LODs and actual WorldScape collision,
then freezes/drains before its existing A/B sequence. This is a labelled test
fixture, not the natural landing or a change to the production spawn rule.
It restores the original pawn ECEF position, rotation, velocity, manual-zero-G
state and control rotation, as well as the original camera/material state.
No seed, sea level, noise profile, terrain geometry, RGBA or UV0 is changed.

- `physical-water-shore-v1`, PID 30308: shore located but automation failed
  after 100 seconds waiting for collision at 500 m camera / 520 m observer.
  Inspection of WorldScape's collision altitude gate explained the unsuitable
  fixture; no candidate frames were produced. The owned editor exited.
- `physical-water-shore-v2`, PID 25984: camera 50 m above the actual shore,
  pawn observer 70 m above it, complex trace against the native collision LOD.
  Build `build-20260927-110112.log` succeeded (4 actions, 7.41 seconds).
  Automation finished **Success with warnings**, 1 test, 55 warnings,
  0 errors / 0 failed tests. Do not report a warning-free pass.
  The editor exited normally. Actual saved frames are 1280 x 722.
- The bounded search took 119 samples; sun dot 0.408206. Before freezing,
  observer delta was 0 cm, with 10 terrain and 10 ocean LODs, confirmed native
  collision, and stable profile signature 3027946924.
- UV1 contained 29,401 wet and 44,559 dry vertices, 0 invalid, across 30
  sections. 1,081 actual-root oracle samples had maximum CPU coordinate
  round-trip error `1.11742213e-9` metres. This is not a GPU depth-error bound.
- Captured original/repeat, filtered candidate, zero-strength and original
  return on the same physical shore with visible ground and unchanged camera,
  root frame, geometry, RGBA and UVs. Restored 30/30 material slots and 73,960
  colours, then returned the pawn to its original physical position.

Shore-v2 report SHA256:
`FB2DABBEDA73C577C4F8AAD0E3DC853D7CD6F37C853CDD48A1A7D597BE71ACE3`.

Visual outcome: the candidate makes near-shore water substantially brighter
and more cyan; land and shoreline placement remain visually consistent.
The water is still smooth/simple. This is **not accepted realism polish** and
the candidate remains disabled. Zero-strength and original-return controls
visually reproduce the accepted baseline.

For a reproducible, narrow control comparison, mean absolute 8-bit display-RGB
differences from the repeated original are below. Water ROI: x=[50,400),
y=[50,620); land ROI: x=[950,1250), y=[50,620). These fixed rectangles exclude
the HUD and shore, so they are not a whole-frame quality or luminance metric.

| Frame | Water mean abs | Land mean abs |
| --- | ---: | ---: |
| First original | 0.0846 | 0.1681 |
| Filtered candidate (.35 / 20 m) | 40.6759 | 0.1830 |
| Zero strength | 0.0865 | 0.1907 |
| Original return | 0.0865 | 0.1744 |

Controls have a 95th-percentile absolute difference of 1 in both ROIs; the
candidate has 66 in water and 1 on land. The strong water shift is attributable
to the candidate rather than a broad exposure/terrain change in this fixture.
No natural walking, LOD-traversal, broader-family or FPS acceptance is claimed.

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
`APSProbeWaterDepthGameplay`. That opt-in relocates the test observer to a
verified physical shore, with bounded failure and restoration paths.

The visual probe is prepared to preserve ground visibility, physical root,
camera and every RGBA/UV/topology value while capturing original/repeat,
filtered candidate (strength .35, half-depth 20 m), zero-strength and original
return. The saved filtered Water MIC is its parameter authority. Alpha is not
repurposed as a depth mask. No other liquid family is eligible.

Next bounded step: use the proven shore fixture to assess a less intrusive
water treatment that preserves the accepted palette while improving depth
readability. Do not hide the ground, clamp dry depths, move just the test camera
into coarse distant LOD or alter the production spawn rule. Assess natural
motion/LOD traversal, broader Water-family coverage and performance before
any production enablement. A successful diagnostic does not authorize rollout.

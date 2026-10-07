# Isolated physical WorldScape water-depth integration, 2026-09-27

## Current minimal native integration, 2026-09-30

`native-worker-20260930.patch` is a new NINE-file native-only diff, preserving
the current collision/worker-ownership/seam fixes. It does NOT contain the
old project's ripple/fill/light experiments below. Seven native tests pass;
the old worker-test pending-map assertion was adapted to current GT ownership.
It applies at a WorldScape plugin root (not the APS project root).

`InstallNativePayload.ps1` installs matching Editor modules, import libraries,
public source and UHT generated headers, backs up all targets and rebuilds APS.
The successful transaction is `before-installed-v3`; do not swap Core alone.
This is a Development Editor build, not a packaged/DebugGame validation.
`RunNativePayloadTests.ps1` reproduces the isolated seven-test suite.
The canonical process opt-in is `-APSWaterDepthPayload`; ordinary-game water
selection is NOT enabled yet. The new rendered probe is
`RunPlanetWaterNormalAB.ps1 -NativeColumn -ColumnArtPalette`.
See Docs/Diagnostics/2026-09-30-native-water-column.md for current evidence,
rollback, deployment failures recovered safely, and remaining performance work.
The older sections below are historical experiments, not current instructions.

Later optional v22/v23 current and physical-column experiments are recorded in
[CURRENTS.md](CURRENTS.md). They are not production-installed or visually accepted;
the existing v21 integration package below remains their reproducible baseline.

Source-only diagnostic overlay, **not installed or enabled in production**.
The accepted checkpoint remains `67fff3b4`; canonical source snapshot for this
experiment was clean `dbc3c430b5551ff3f405de41ac0017f419e631d2` on `dev-3`.
Production source, materials, engine plugin and binaries were not replaced.

## What is preserved here

`integration.patch` and `overlay-manifest.json` describe 19 source files:
the private WorldScape signed-kilometre UV1 data path and worker tests, an APS
process-wide opt-in restricted to owned full-scale generated Water roots, and
a bounded extension of the existing real gameplay liquid probe. There is no
new production material selector, saved default, geography or palette change.
The rejected compressed-preview height adapter is deliberately absent.

Current overlay: `overlay-v21` adds actual grounded input-driven shore walking
and original/candidate runs on Water and Terrestrial. The v20 optional-fill
and source-lifecycle controls are saved in `d3abccf47094f55d4629d766ed5882df35861e57`. v19's native
shore route/family fixtures are saved in `0d51fe72ff6d8713a72714f66a5d67cecf51e249`.
v18's source-
equivalent baseline control is saved in `7feae9e720933f95e83ba55779c95f1e66115224`.
(v17 is the first local fill A/B package. The v16 filtered-shadow result is in
`1563603de1a35beeb53c24b640deed64453548f1`; the three water-pass controls are preserved in
`2d5cad80d19acd0924b3369a9fa7b9e0f8c37e15`; the SingleLayerWater surface control and its
shore/lighting regression are preserved in `dfdcb10992d8e61f23608071021ee0e5ec14c5cd`;
the view-anchor and ShortRangeAO controls are
preserved in `bed23b70d55bc4de341081eb069de860fb1c80e6`; the preceding spectral/buffer diagnostic is
preserved in `d5bcb739be56a8bc04d5202a66a28251aff6eaa5`; the camera-relative close-view regression and
paired GPU benchmark are preserved in `bf32ab9bdc572adccb2e8412c2c67a135c5d3545`;
the preceding stochastic candidate is preserved
in commit `2092659f86e030ece6209e22eec27ddb86ab16b9`; oblique/live-LOD fixture
in commit `47ac2c4340f7103f869aebda30d4dd1c740e05ff`; palette-budget fixture in
`8248482c54d92e86a655412f2b82b530ad326a1f`; dry fixture in `6e9e8e09`).
Patch SHA256: `6E2134E2E42B35B23A36B4732FE081B4199894E7A9097E8FE78F9B3384EEB6D0`.
Manifest SHA256: `791D8B786EED5253E90AF70A1C60924E42A14945F649B1765D058FD49F400F25`.
The patch was reverse-checked against the final isolated source tree.
The optional normal-only ripple extension, private asset mount and its separate
rendered evidence are documented in `RIPPLES.md`. It has no production selector.
**Do not promote either ripple candidate:** the DefaultLit 2 m close-view
regression remains despite passing structural automation. The private
SingleLayerWater surface control removes that pattern but darkens the water.
Its shore fringe is removed in the v16 filtered-shadow fixture. The v18
scene-derived secondary-fill adapter brings the source-equivalent 2 m baseline
within 1% mean linear water luminance of the original, without palette gain.
The v19 moving shore fixture passed native LOD/binding checks on Water and
Terrestrial; inspected start/turn/return stills retain the ripple surface without
the prior noisy fringe in their limited visible shoreline. Neither moving run
changed the actual fill direction. v20 separately exercises rotation, reduced
energy, hidden light, unsupported settings and recovery on the actual tagged
source; the paused fixture's final frame is pixel-identical to its starting
frame. Natural unpaused light transitions remain untested. v21 covers actual
grounded walking on Water and Terrestrial, with native workers and the real
pawn camera; sprint, orbit and broader-family coverage remain open. Candidate
engine frame time increased in these separate runs, so no performance acceptance
or production promotion. The
paired GPU benchmark is useful evidence, not a visual or 120 FPS acceptance.
The latest lighting controls and view-anchor candidate are recorded in
`RIPPLES.md`. With GI and reflections still enabled, disabling ShortRangeAO
removes the broad angular patches in the 2 m fixture. This localizes an
interaction, not a shippable correction: do not disable scene AO as a workaround.
The current candidate still fails ordinary-lit acceptance with default settings.
`-SingleLayerSurfaceControl` is a separate opt-in for BakeRipples.ps1 and Run.ps1
(the latter also needs `-Ripples`). It retains opaque surface weight=1 and zero
volume coefficients; it is NOT a transparent-water migration. See `RIPPLES.md`
for source rationale, rendered views, hashes, performance and remaining gaps.
Three additional water-pass controls reused the same saved packages: the shore
fringe survives both capture-only reflections and disabled water DF shadows.
Disabling the full composite darkens water almost to black, so that is not a
valid visual correction. No new assets were baked. The read-only depth-prepass
setting also changes shader-map keys; do not try an invalid runtime toggle or
repeat the already-excluded controls. See the v15 evidence in `RIPPLES.md`.

The v16 one-frame GPU dump directly rules out coverage disagreement between
water depth/base passes in the tested view (587506 matching water pixels, zero
missing). The noisy pattern is already in main-sun lighting, not introduced by
the reflection composite. Explicit `-WaterFilteredShadows` enables native water
VSM filtering at process startup; inspected near-shore ripple-on/off frames lose
the fringe while preserving global AO and shadows. No new material bake or
production rollout. See `RIPPLES.md` for exact evidence and remaining darkening.

## Evidence and limits

Private workspace:
`F:/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-gameplay`.
It contains the full isolated `host/APS_ALPHA.uproject`, source snapshot,
build logs, manifests and finite rendered diagnostic reports.

Relocation note, 2026-09-27: historical logs may still contain the former
`C:/Users/Rio/Documents/ChatGPT/APOSFERA` prefix. The relocated `host/Content`
was observed as an ordinary directory, not a junction; do not remove or relink
it on the assumption that it is disposable. Rebuild/run portability after the
move has not been validated. Inspect old absolute paths before the next run;
do not rewrite hashed historical evidence. See the
[current project audit](../../../Docs/Audit/2026-09-27-project-state.md).

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

## Palette-budget refinement (still opt-in)

`APSWaterDepthPalette.h` resolves the uniform strength from the existing saved
deep and shallow colours instead of assuming .35 is equally subtle for every
palette. With a .35 relative linear-luminance budget it uses
`min(1, budget * min(Ydeep,Yshallow) / abs(Yshallow-Ydeep))`. Equal-luminance or
black endpoints produce zero strength; negative/nonfinite colours or an
out-of-range/nonfinite budget fail closed with zero output. The algorithm
retains both endpoints and the saved filtered 20 m depth falloff.

For the tested Water palette, Ydeep=.0164619, Yshallow=.147944996, so the resolved
strength is .0438205749. For the existing bounded legacy/filtered-depth alpha,
this bounds the change in the linear palette term by .35 of its baseline.
It does not bound displayed pixel brightness or simulate physical scattering.
It adds a small CPU uniform calculation, not a shader texture/noise loop.

The diagnostic requires `APSWaterDepthPaletteBudget` explicitly; existing
non-budget runs still use .35. Camera height may be 10..500 m, independently
of the real WorldScape observer held at 70 m for native collision convergence.
This is not an adaptive production LOD or camera change.

Evidence:

- Build `build-20260927-111351.log`: 4 actions, 8.81 seconds, success.
- `physical-water-budget-near-v1`, PID 29164, failed before any scene: sandbox
  restrictions made the common DDC read-only; Unreal requested exit status 3.
  No test or visual acceptance. The owned orphan crash monitor was removed
  only after verifying its exact `-MONITOR=29164` argument and editor exit.
  No user editor, project cache settings or production assets were changed.
- Retried with access to the existing writable DDC: `physical-water-budget-near-v2`
  (PID 4384, 50 m) and `physical-water-budget-coast-v1` (PID 26732, 500 m).
  Each completed two tests: palette arithmetic passed with 0 warnings, rendered
  gameplay passed with 55 logged warnings; 0 failed tests / 0 errors. Both
  editors requested exit status 0 and were confirmed no longer running.
- The C++ palette test covers 2,420 combinations of five endpoint pairs,
  four budgets, legacy alpha and depth falloff; includes reversed contrast,
  black/equal endpoints, HDR values, linear-scale invariance, zero/far identity
  and negative/NaN/infinite input rejection. It is not GPU or FPS proof.
- Both rendered runs validated actual ground/ocean collision and the same
  73,960 UV1 vertices, 1,081 root oracle samples, 30/30 restored slots and
  restored pawn/camera. Ground, topology, RGBA and UVs stayed unchanged per A/B.
- On inspected 50 m and 500 m captures, water retains a dark-blue appearance
  with a modest shallow-depth lift; the previous strong cyan shift is reduced.
  Water still lacks convincing fine detail and these top-down stills do not
  establish realistic oblique/walking appearance, all-family continuity or FPS.

Same fixed ROIs and mean absolute display-RGB metric as above:

| View / comparison to repeated original | Water | Land |
| --- | ---: | ---: |
| 50 m candidate | 6.4187 | 0.0682 |
| 50 m zero strength | 0.0860 | 0.1832 |
| 50 m original return | 0.0084 | 0.0954 |
| 500 m candidate | 4.5783 | 0.1503 |
| 500 m zero strength | 0.0859 | 0.1490 |
| 500 m original return | 0.0861 | 0.1504 |

Compare the former .35 candidate's 50 m water difference of 40.6759: the new
uniform measurably reduces the broad colour shift, while zero/return controls
stay near their baseline noise. No material asset was rebaked or replaced.

Reports SHA256:

- Near: `B9F5D74857FD2A8BFB44315FE36CB2019B72780C4DA159F0C1DFAE320054D83E`.
- Coast: `90CF78501F7D711EB45F4B109E578039E227CFD46DEC5A1964C17E129A3E3324`.

## Oblique view and live native LOD follow-up

`APSWaterDepthOblique` aims the diagnostic camera 30 degrees below the physical
horizon, using the real shore's wet/dry tangent and the root rotation. It does
not hide the ground, rotate the planet or substitute a flat test surface.

- `physical-water-budget-oblique-v1`, PID 22760: the rendered test failed before
  relocation because the tiny post-bisection dimensionless shore chord was
  normalized below FVector's safe-normal threshold. The palette test passed.
  Multiplying the chord by the actual planet radius before normalization fixes
  this units error without changing the shore search or geography.
- Build `build-20260927-112812.log`: 4 actions, 8.74 seconds, success.
  `physical-water-budget-oblique-v2`, PID 28716: both tests passed; rendered
  gameplay had 55 warnings, palette arithmetic 0, and there were 0 errors.
  The original/repeat/candidate/zero/return fixture retained visible land and
  its physical frame. Inspected oblique frames show a modest near-water lift,
  with the existing blue-to-cyan grazing-angle response. Water still looks
  overly smooth; this is not completed water-detail polish.

`APSWaterDepthLiveLod` adds a bounded optional phase after the static controls.
It leases the root's ocean default and the surface's resolved material together,
calls native `UpdateOceanMaterial` so subsequent LOD publications inherit the
candidate, then resumes the existing terrain/ocean workers and actor ticks.
The actual pawn observer follows 60 metres out and back at 6 m/s, 70 metres
above sea level; the 50 m oblique camera follows it. This scripted zero-G hover
is explicitly not normal character walking or a production spawn change.

- Build `build-20260927-113623.log`: 4 actions, 8.94 seconds, success.
- `physical-water-live-lod-v1`, PID 20296: both tests passed, rendered gameplay
  with 55 warnings, palette arithmetic with 0; no errors or failed tests.
- Across 2,035 callback frames, all 30 ocean material slots retained the
  candidate and 90 sampled vertices per frame retained finite valid UV1.
  The monitored published LOD0 vertex changed position 99 times; generation
  was actually running rather than merely enabled by a flag.
- After the return and worker drain, all 73,960 vertices had valid depth:
  29,401 wet / 44,559 dry / 0 invalid. The 1,081 actual-root oracle samples had
  maximum CPU coordinate-round-trip error `1.11742213e-9` metres.
- Start/turn/return stills show a moving shoreline without obvious holes or
  grid seams in those frames. Three stills do not prove absence of temporal
  flicker, normal walking quality, all-family coverage or frame-rate parity.
- Callback intervals were median 9.4313 ms / p95 12.3064 ms, excluding 0.5 s
  after captures. These instrumented callback timings are not GPU/Present
  measurements or acceptance of approximately 120 FPS; no baseline performance
  control was run. Do not convert them into a claimed gameplay FPS result.
- Restored 30/30 slots and the original pawn/camera/tick state. The live path
  intentionally restored zero old vertex colours: it never overwrites freshly
  regenerated native geometry with the earlier frozen snapshot. Both successful
  oblique/live editors exited normally and are no longer running.

Report SHA256:

- Oblique v2: `FA2BBFEE7D8B1A21847DEAE3F20DCEF22015397FC15B40488FE95DEB04970C97`.
- Live LOD v1: `C2FFB3ACEE30442129A354782F1E189B9E1D035F5548C453F8D43411CC89AB76`.

No production code, plugin DLL or material asset was installed or rebaked.
The next detail issue is separate from the now-tested native depth handoff.
The current material-builder source uses coarse 180 m / 280 m wave-noise scales;
UE 5.4's `FHLSLMaterialTranslator::VectorNoise` coerces position to Float3.
Reducing those scales blindly at planetary coordinates risks precision loss.
This is a source finding, not proof of a rendered precision defect: a finer
noise domain must preserve physical coordinates, continuity and pixel filtering.

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

The static visual probe is prepared to preserve ground visibility, physical root,
camera and every RGBA/UV/topology value while capturing original/repeat,
filtered candidate (strength .35, half-depth 20 m), zero-strength and original
return. The saved filtered Water MIC is its parameter authority. Alpha is not
repurposed as a depth mask. No other liquid family is eligible.

Palette-budget runs: `Run.ps1 -Label <new-label> -PaletteBudget -CameraHeightM 50`
or `500`; neither enables a production default. An isolated process still needs
access to a writable DDC. A DDC startup failure is not a material failure; do
not change the accepted project config or restart a live user editor for it.

Oblique live run: `Run.ps1 -Label <new-label> -PaletteBudget -Oblique -LiveLod`.
Omit `-LiveLod` for a frozen oblique A/B; live mode requires the oblique fixture.
In live mode native geometry is allowed to regenerate and restoration changes
material ownership only, never copying the frozen section buffers back.

Next bounded step: investigate the measured moving-frame cost with an in-process
grounded A/B/A comparison before rollout; include sprint and preserve the real
floor/camera/input gates. Do not substitute another static light-control pass.
The v21 same-protocol runs passed walking/LOD binding, but are NOT bitwise-matched
views or a proof of 120 FPS. Water fills a narrow distant band; Terrestrial has
more visible shore/water. Long highlight streaks remain in the candidate; finer
realism and temporal filtering are not finished. The v20 actual-light change/loss/recovery control passed in a
paused owned world, not a natural day/night/station transition.
The v19 per-frame adapter was rendered only on a 60 m out-and-back hover route,
not a production light lifecycle. `Run.ps1 -Planet Terrestrial` selects the
explicit Water-depth family fixture; other listed Water-bearing types are
allowed but NOT validated merely by being listed. The runtime still requires
actual Water liquid, native collision, stable root, payload and restoration.
The source-equivalent lighting control and v19/v20 limits are in `RIPPLES.md`.
Do not repeat the excluded prepass-coverage or reflection/DF controls, blind
coordinate/noise rewrites or promote a scene-wide AO disable. Sprint, broader
Water-family coverage and whole-pipeline performance remain unverified. Do not hide the ground,
clamp dry depths, move just the test camera into coarse distant LOD or alter the
production spawn rule. Keep production selection off until those requirements
are met. Successful diagnostic runs do not establish completed water realism
or authorize rollout by themselves.

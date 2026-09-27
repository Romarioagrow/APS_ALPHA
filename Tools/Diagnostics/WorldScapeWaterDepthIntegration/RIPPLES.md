# Isolated water-normal refinement, 2026-09-27

Not installed or selected in production. Accepted palette, existing coarse
waves, roughness, Fresnel colour, terrain, shoreline and native geometry stay
unchanged. New code is retained as a source overlay alongside the depth work.

## Design and protected outputs

- `APSWaterRippleMaterialBuilder.h` adds a stationary normal-detail band before
  the existing single planet-to-world normal rotation. It is not fluid simulation
  and does not move vertices or change the water level.
- Current candidate: two 3D gradient-noise bands at 370 cm and 83 cm, weights
  .65/.35, final slope bounded by .065. There is no latitude/longitude or local
  tangent UV basis to introduce a pole seam.
- Current physical positions reconstruct directly from the camera-relative pixel
  interpolant and the view's DoubleFloat translation, then use compensated frame and division,
  reusing the accepted terrain-detail coordinate contract and split transform
  rows. The earlier native tile/offset path is rejected by the 2 m close test.
  Noise receives a bounded 96-cell coordinate. UE 5.4's simplex Jacobian hashes
  `6*T`, so its hash period is 576, matching the Cartesian wrap exactly. This is
  periodic noise, not a claim of mathematically aperiodic texture everywhere.
- DFDdx/DdyDemote use the unwrapped compensated coordinate; the gradient fades between
  .25 and .5 noise cells per pixel footprint. No derivative is taken through
  the wrap discontinuity. This is a practical detail filter, not perfect
  band-limiting or a proof of temporal stability.
- LWC `Frac`/derivative behaviour and the six-times simplex hash were checked
  against the installed UE 5.4 translator and shader source, not guessed.
- `APSWaterRippleCandidate.h` mounts `/APSWaterDetail/` solely for explicit
  diagnostics at `host/Intermediate/WaterRippleAssets/`. **No output goes through
  the host's read-only Content junction.** `BakeRipples.ps1` refuses existing
  outputs/evidence and refuses an active editor. The original depth bake paths
  and ordinary gameplay selection are unchanged.
- Ripple A/B uses the same physical depth colouring in its enabled and
  `03-depth-only` disabled-ripple frames. It also captures accepted original,
  repeated original and return, then optionally performs the native live-LOD
  route. Thus a depth colour lift is not credited as added normal detail.

## First candidate rejected by rendered evidence

`overlay-v7` used six deterministic sine bands (31 cm..6.1 m). Build
`build-20260927-115148.log` and bake `bake-ripples-v1` succeeded (bake 0 errors /
7 warnings). The controlled-ripple probe was rebuilt in
`build-20260927-115346.log`. `physical-water-ripples-v1`, owned PID 29740,
passed both tests: rendered 55 warnings, palette 0, no errors. Its live phase
observed 98 published LOD0 changes across 1,968 callbacks with valid depth and
retained material binding. The images nevertheless showed conspicuous parallel
wave trains: this is not accepted realism. That evidence and the private assets
are retained, and the current code replaces this design rather than enabling it.

## Earlier stochastic candidate (superseded by close-view failure below)

Build `build-20260927-115854.log`: 5 actions, 11.55 seconds, success.
Bake `bake-stochastic-ripples-v1`, owned PID 24096: 0 errors / 7 warnings,
2 new packages under `/APSWaterDetail/StochasticRipples20260927/`.
Existing missing-reference warnings are recorded in the bake log; this is not
a warning-free bake. Candidate shader completeness/LocalVF was checked before
either package was saved. The owned commandlet exited normally.

Saved SHA256:

- Master: `1AB4C025934B1A5C3FD899024EA5F5669F6F387FAE70D8C58082BC1E6DAFA6A1`.
- MIC: `7F876A519239222E0E997CB731D4DB92815212AF0E4D53180CC4FAB8A1DD24DA`.

Rendered run `physical-water-stochastic-ripples-v1`, owned PID 24556, completed
both tests: rendered gameplay 55 warnings, palette arithmetic 0, no errors or
failures. The editor exited normally and was confirmed no longer running.
The exact candidate was bound from the private mount. Camera, frame, RGBA, UVs
and topology remained fixed during the enabled/disabled-ripple comparison.

Inspected 50 m oblique and live-turn frames show irregular fine surface detail
instead of the first candidate's parallel wave trains. The shoreline and land
are visually consistent with the disabled-ripple control; fine detail diminishes
in the distance. This is a modest visual improvement in this one fixture, not
final realistic water, a moving wave simulation or full-family acceptance.

On water ROI x=[50,400), y=[50,620), enabled vs depth-only mean absolute 8-bit
RGB difference is 1.70759 (p95 7), mean signed RGB approximately
[-.00004, -.10378, -.23690]. On land x=[1000,1250), y=[50,620), mean absolute
difference is .14861 (p95 1). Original-return vs repeated-original differences
are .09794 water / .15481 land. Thus the added local variation is not a broad
water colour lift, and land differences are comparable to baseline capture noise.
These fixed-ROI metrics are not a perceptual quality score.

Native live phase: 1,865 callbacks, 97 published LOD0 vertex changes; 30 material
slots and 90 depth samples checked per callback. Final full inventory: 73,960
valid vertices (29,401 wet, 44,559 dry), 1,081 oracle samples, maximum CPU
round-trip error `1.11742213e-9` metres. All slots, camera, pawn and tick state
were restored without overwriting the regenerated vertex buffers. Median/p95
callback intervals 10.0337/13.9811 ms are not a controlled CPU/GPU benchmark.
No claim of zero temporal shimmer is made from the three live stills.

Report SHA256:
`2F06FEC0350A87C600BAF82CE7C150C57D74DD4D2962734548B7B744D5D143AE`.

## Reproduce

Apply the manifest-checked source overlay only to an isolated host and rebuild
all dependent modules as described in README. Run `BakeRipples.ps1` with a new
label, then `Run.ps1 -Label <new-label> -PaletteBudget -Oblique -LiveLod -Ripples`.
The private assets are not automatically cooked, selected, or copied to /Game.
The latest two saved assets are checkpointed under
`Assets/CameraRelativeRipples20260927/`; they retain their `/APSWaterDetail/` package
identities. For exact-byte reproduction copy that folder to the isolated host's
`Intermediate/WaterRippleAssets/` and verify the hashes instead of rebaking.
Do not install any isolated DLL by itself into the accepted project/plugin.

Remaining gates for this earlier revision: near-surface and orbital distances, temporal traversal,
normal walking, other Water palettes/subtypes and measured CPU/GPU parity.
No full-family visual acceptance or preservation of approximately 120 FPS is
claimed from this isolated fixture or its callback timing.

## Close-view regression and paired GPU timing

The earlier 50 m result was insufficient: `water-ripples-near-perf-v1` at 2 m
shows large sharp square/polygonal normal patches absent from the depth-only
control. Both automation tests passed, but visual acceptance FAILED. That
candidate remains disabled and must not be installed based on the older images.
`water-ripples-far-perf-v1` at 500 m looked smooth, demonstrating why the near
test is necessary. Both owned editors exited; no production files were changed.

`Run.ps1 -MaterialPerf` adds an optional frozen A/B/A/B/A material comparison,
2 seconds warmup and 3 seconds recording per phase, with no screenshot, readback,
section hash or render flush in measurement windows. It is mutually exclusive
with `-LiveLod`. GPU uses engine completed-frame timestamps; CPU uses engine
thread counters. Raw samples are saved to `material-perf.tsv`. The camera height
range is now 1..500 m. Payload/geometry and material ownership are checked at
boundaries. Timestamp support and valid positive timing are required.

Earlier candidate paired GPU increments: 2 m +0.0426645/+0.0529452 ms;
500 m +0.0229818/+0.0520369 ms. Actual viewport 1280x722, VSync and editor VSync
enabled, no frame cap changed. This compares material cost only: depth payload
is enabled in both, and native generation is frozen. It does not measure the
worker cost, normal walking, Present latency or prove sustained 120 FPS.

## Compensated-coordinate correction

The candidate alone now mirrors `APSSharedTerrainMaterialBuilder::PatchDetailCoordinates`:
convert LWC inputs to DoubleFloat, subtract the planet centre, multiply by the
existing high/low physical transform rows, divide while compensated, derive
unwrapped pixel gradients, then demote only the wrapped 96-cell phase. Both
noise bands, weights, normal strength and palette remain unchanged. This replaces
a suspected precision path without rewriting accepted terrain code; the rendered
follow-up below shows it is not sufficient on its own.

Build `build-20260927-122056.log`: 5 actions, 10.17 seconds, success.
Bake `bake-compensated-ripples-v1`, owned PID 6856: 0 errors, 7 existing warnings,
2 new packages in `/APSWaterDetail/CompensatedRipples20260927/`, exited normally.
The previous private packages are preserved, not overwritten.

Saved master SHA256: `7D95F43B1DCFD5258A44B29F0F17D9C5E6B462691597719415427918EB93F9A7`.
Saved MIC SHA256: `12849DF15E712034FBD8918329E21944978514CE121522D1D213375E8DDF3832`.

`water-ripples-compensated-near-v1`, PID 10608 (exited): two tests passed, but
the 2 m image still shows large angular patches absent from its depth-only
control. Visual acceptance FAILED; do not promote the compensated-only package.
Paired GPU increments +0.0664906/+0.0687968 ms. Build/bake/test success is not
evidence of an artifact-free surface, and these values are not overall FPS.

## Direct camera-relative reconstruction

The next revision bypasses the legacy absolute tile/offset pixel-position path:
`DFSubtract(DFPromote(Parameters.WorldPosition_NoOffsets_CamRelative),
ResolvedView.PreViewTranslation)`, then subtracts the same planet centre and
uses the same compensated physical transform. A connected no-offset WorldPosition
input requests the required interpolant. The formula reconstructs planet-fixed
positions; it does not deliberately attach the noise pattern to the camera.
Its behaviour during camera movement still requires separate validation.

Build `build-20260927-122739.log`: 5 actions, 10.20 seconds, success.
Bake `bake-camera-relative-ripples-v1`, PID 17056 (exited): 0 errors, 7 existing
warnings; 2 new packages, no production binding or shared asset overwrite.
Master SHA256: `8859850919080D5E1C4B41F88680AB1F4A6560D1EB6CF28D4078A454DD034A59`.
MIC SHA256: `193600CCCF7849D411B0AEA7ECFB0B5B03264A8648DBB100B31F56301DF8F36E`.

Run `water-ripples-camera-relative-near-v1`, PID 24116 (confirmed exited):
both tests passed, rendered test 55 warnings / 0 errors, palette test 0 warnings.
The inspected 2 m image STILL shows angular blocks absent from the depth-only
control: visual acceptance FAILED. Paired GPU increments are
+0.0694332/+0.0686808 ms. No assertion that input-position reconstruction alone
fixes the defect is supported. The original camera, pawn, materials and tick
state were restored; production code/assets/plugin binaries remain unchanged.

Stop promotion and further speculative bake loops here. The next investigation
must isolate the noise/normal/filter contribution rather than assume the
remaining pattern is explained by coordinate precision. The current source and
private assets are a reproducible failed candidate, not a shipped improvement.
Near-water quality, temporal traversal, all Water palettes and whole-pipeline
performance remain open. The accepted game remains the user's checkpoint.

Report SHA256 (under the private workspace paths named above):

- `water-ripples-near-perf-v1`: `00B2672E4B3E57C0A0D2547B2E2E476AFE3DC28F503482DFCABA6C0072D18B4C`.
- `water-ripples-far-perf-v1`: `B7A9B714F4B96264AC45E66889983EFC050A77765AEDE6EE0E5ABF24F4651EDC`.
- `water-ripples-compensated-near-v1`: `39F27A1649C35914792C49F6E05E3598BFDBCDF314AC1EB839AEE063E17B33DE`.
- `water-ripples-camera-relative-near-v1`: `BC11053466411FE4FCF630E0565DB75612A467C55BD6BEFED29C507EC2709182`.

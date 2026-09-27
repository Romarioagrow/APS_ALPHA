# Isolated water-normal refinement, 2026-09-27

Not installed or selected in production. Accepted palette, existing coarse
waves, roughness, Fresnel colour, terrain, shoreline and native geometry stay
unchanged. New code is retained as a source overlay alongside the depth work.

## Design and protected outputs

- `APSWaterRippleMaterialBuilder.h` adds a stationary normal-detail band before
  the existing single planet-to-world normal rotation. It is not fluid simulation
  and does not move vertices or change the water level.
- Current candidate: two smooth twelve-wave bands at nominal 370 cm and 83 cm,
  weights .65/.35, final slope bounded by .065. There is no latitude/longitude or local
  tangent UV basis to introduce a pole seam.
- Current physical positions split a uniform DoubleFloat view-origin phase from
  the small camera-relative pixel delta. The same physical transform rows apply
  to both. The anchor is wrapped before adding the delta, avoiding per-pixel
  planetary-sized quotients; the sum is algebraically planet-fixed. Earlier
  per-pixel DoubleFloat and native tile/offset variants failed the 2 m close test.
  Each band receives a bounded 96-cell coordinate. Integer wave vectors preserve
  continuity across the 96-cell wrap, including negative axes. The two physical
  wavelengths differ. This is a finite periodic spectrum, not a claim of
  mathematically aperiodic texture everywhere.
- Ddx/ddy use the unwrapped small pixel delta; each wave fades between
  .25 and .5 cycles per pixel using its own projected frequency. No derivative is taken through
  the wrap discontinuity. This is a practical detail filter, not perfect
  band-limiting or a proof of temporal stability.
- The earlier native simplex implementation and LWC derivatives were checked
  against installed UE 5.4 source. Its smooth analytic limits alone did not
  guarantee a natural-looking reflected pattern in the close fixture.
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
`Assets/ViewAnchorRipples20260927/`; they retain their `/APSWaterDetail/` package
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

## Isolated normal controls and smooth-spectrum revision

The previous turn is progress, not completion: its close frames prevented an
incorrect candidate promotion. This follow-up changes the next action using
an explicit control, rather than assuming another precision rewrite will help.

Build `build-20260927-123704.log`: 5 actions, 10.45 seconds, success.
Bake `bake-isolation-ripples-v1`, PID 27192 (exited): 0 errors, 7 warnings.
Run `water-ripples-isolation-near-v1`, PID 11648 (exited), both tests passed:
the native gradient with its footprint filter disabled still shows angular
patches; a simple smooth sine control on the SAME physical phase does not.
This rules out the footprint filter as the sole explanation in that fixture.
It does NOT isolate the noise algorithm: the analytic control uses only band 0,
whereas the native candidate combines both bands. The spectral follow-up below
also retains an angular lit pattern, so the initial noise-only hypothesis is
not established. No Unreal native-noise implementation bug is demonstrated.

The isolation run's legacy filenames are important: `02-filtered-depth.png`
actually contains UNFILTERED native noise, and `03-depth-only.png` contains
the ANALYTIC CONTROL, not depth-only. Explicit WATER_RIPPLE_ISOLATION log
entries record both settings. Subsequent code gives these diagnostic frames
the unambiguous names `02-unfiltered-slope` and `03-analytic-control`.
`-RippleIsolation` requires `-Ripples` and refuses performance/live-LOD modes.
Report SHA256: `2AAA2C0AA506FA3529578E23AE6AD9F073A783F3CAE5513C5E1236D05D3B23A6`.
The full isolation source is preserved in private `overlay-v11`.

The next candidate replaces only the added native noise gradient with 12
distinct wave vectors per band (24 total), varied phases and frequencies,
analytic slopes and a separate per-frequency footprint filter. Its intent is
to reduce the first candidate's regular comb and the later gradient's faceted
appearance; the close-view result below does not establish that goal. Tangential projection, .065 slope bound, native
coarse water normals, colour/depth blend, shoreline and geometry are retained.
These are still stationary normal details, not a moving fluid simulation.

A double-precision numerical spot check of 1,000 phase points gave a maximum
96-cell periodic residual of 2.60e-13 and maximum slope change of 9.21e-6 for
a 1e-6-cell offset; a large footprint attenuated all waves to zero. This checks
the formula, not actual GPU precision or perceptual/temporal acceptance.
Build `build-20260927-124158.log`: 5 actions, 9.89 seconds, success.
Bake `bake-spectral-ripples-v1`, PID 30352 (exited): 0 errors, 7 existing warnings;
2 new packages in `/APSWaterDetail/SpectralRipples20260927/`, no production binding.

Saved master SHA256: `30271CD41A4BCBFD0E337421192E68188EEC89DA37F8F5BFC127A510397D906F`.
Saved MIC SHA256: `09C0DA35D778662ADD7161428AE35E059E96932736C4F673F9B9542E7A9C55F4`.

`water-ripples-spectral-near-v1`, owned PID 6692 (exited), completed both test
entries successfully: rendered test 55 warnings / 0 errors, palette test 0 / 0.
At 2 m, the lit image still shows subtle angular patches relative to depth-only.
Visual acceptance remains FAILED. Paired material-only GPU increments were
+0.0548465 / +0.0619483 ms, against original phase means
4.21155 / 4.20747 / 4.2136 ms. This is not full-pipeline or 120 FPS acceptance.

## Buffer isolation: latest evidence, not a production fix

The private static probe can now select `-BufferView WorldNormal`, `BaseColor`,
`Roughness` or `Specular`; legacy `-NormalBuffer` selects WorldNormal. Invalid
combinations with live/performance modes are rejected. The probe saves and
restores viewport show flags, view mode and buffer target. Its log explicitly
labels buffer images as diagnostics, not lit-water appearance. This path is
diagnostic opt-in only and was not installed into the accepted game.

Build `build-20260927-124819.log`: 4 actions, 7.76 seconds, success.
`water-ripples-normal-buffer-v1`, owned PID 10820 (exited): both test entries
succeeded (55 rendered warnings, no errors). Enabled and depth-only WorldNormal
images look broadly smooth without conspicuous rectangular discontinuities.
On ROI x=[50,650), y=[120,620), mean absolute RGB difference was 0.870564 code
values. Eight-bit PNGs cannot rule out small normal errors amplified by lighting.
The project uses `r.GBufferFormat=3`; do not infer an eight-bit GBuffer merely
from the exported image's precision.

Build `build-20260927-125254.log`: 4 actions, 7.90 seconds, success.
`water-ripples-basecolor-v1`, owned PID 23944 (confirmed exited): both test entries
succeeded (55 rendered warnings, no errors); buffer state and pawn/camera/material
state restored. Candidate and depth-only BaseColor both show a smooth colour
gradient. Same water ROI mean absolute RGB difference is 0.09403, p99 1 and
maximum 3 code values; land x=[1000,1250), y=[120,620) mean is 0.176453.
These small differences do not explain the observed lit angular pattern.
They narrow investigation toward normal response/lighting/reflections, but do
not identify a specific rendering defect. The accepted source Fresnel has an
explicit normal input; do not assume it automatically consumes the new pixel
normal. No lighting/reflection isolation has been performed yet.

All three latest runs preserve geometry, palette binding and depth payload;
their structural test success is separate from the FAILED lit quality gate.
No production source, plugin DLL, cooked selector or material was changed.
Stop speculative build/bake loops here. The next useful bounded test should
isolate lighting/specular contribution in this same frozen fixture, or compare
both bands using a matched control, before proposing a further shader rewrite.
Whole-family coverage, temporal walking and whole-pipeline performance are open.

Report SHA256:

- `water-ripples-spectral-near-v1`: `4EC44D59C4637BA57BA3DEEDD37ECE89BC8A6921E04F85784070CEBDA746BC70`.
- `water-ripples-normal-buffer-v1`: `203EC950E8D3E1C8D2FB3E47AC3D0FC955B60B5C591408520D21C67E37C07A61`.
- `water-ripples-basecolor-v1`: `E00EC899005932C73773E216105C9C486F2FED409B731735652747C9A566DD92`.

## Lighting controls and uniform view-anchor phase

Build `build-20260927-130501.log`: 4 actions, 8.87 seconds, success. The static
probe now accepts `-LightingIsolation NoSpecular` or `NoReflections`, mutually
exclusive with buffer, live and performance modes. It records actual show flags
and restores the entire original flag set. These are diagnostics, never a
production graphics setting or a way to hide a failed material.

Both owned controls exited normally with both test entries successful, 55
rendered warnings and no errors. Both still show the lit angular pattern:

- `water-ripples-no-specular-v1`, PID 22512: Specular=0, other reflection flags=1.
- `water-ripples-no-reflections-v1`, PID 29464: Specular=1, ReflectionEnvironment,
  ScreenSpaceReflections and LumenReflections=0.

Water ROI x=[50,650), y=[120,620) mean absolute RGB differences from the preceding
full-lit spectral candidate are respectively 0.195101 and 0.100397 code values.
The controls therefore do not justify changing production water reflectance or
reflection settings. They also do not prove that every lighting path is disabled.
The remaining field/normal calculation needs direct investigation.

Report SHA256:

- NoSpecular: `432C048C76D382EEE13B2197FAF0C85BD867733CDA2A157E36863D7AA73EE5F6`.
- NoReflections: `0B0C45142EBC14154A349618DA33B99EBE0D1A35869D59BA447891E94C324923`.

The next candidate changes only phase evaluation. Transform the uniform view
origin minus planet centre with DoubleFloat, wrap its phase, then add the
transformed small camera-relative pixel delta. This avoids repeatedly dividing
and wrapping planetary-sized per-pixel values. Derivatives operate on that
unwrapped delta. Wave spectrum, slope, depth palette, roughness, geometry and
reflection settings are unchanged. The two phase terms cancel camera motion
algebraically; rendered motion stability is a separate outstanding requirement.

A double/float32 arithmetic spot check (10,000 wavelength/point cases at up to
7e8 cm coordinates, pixel deltas up to 500 m, view shifts up to 60 m) measured
maximum double phase residual 1.44352e-11 cycles, float phase residual
9.50444e-7 cycles and float view-shift residual 1.43052e-6 cycles. This checks
the decomposition, not the actual shader's DoubleFloat implementation or GPU.

Build `build-20260927-131155.log`: 5 actions, 11.80 seconds, success.
Bake `bake-view-anchor-ripples-v1`, PID 15908 (confirmed exited): 0 errors,
7 existing warnings, 2 new private packages; no source asset overwritten.
Master SHA256: `833F076C5BE0FBDAD14445613CE80F99FD1862CE80D7A52A1E37F3086CCD63CC`.
MIC SHA256: `527F9B88F2498345E0A0AEAFD228EA7761F3807A340FF6D138C311BB53A1AAD9`.

`water-ripples-view-anchor-near-v1`, PID 6124 (confirmed exited): both tests
passed (55 rendered warnings, zero errors), but the 2 m image STILL contains
angular patches. Visual acceptance FAILED. The uniform-anchor decomposition
does not fix the reported appearance by itself. No production rollout is allowed.
Paired material-only GPU increments +0.0495978 / +0.0702724 ms, original means
4.21269 / 4.2091 / 4.21168 ms. This is not a whole-pipeline FPS measurement.
Report SHA256: `4636A11CED9538C19F1CD60599EF6F2DA638E3D9D4A8BB23EC6559020A3907DC`.

Build `build-20260927-131724.log` adds a third private lighting control,
`-LightingIsolation NoIndirect`, disabling GlobalIllumination and
LumenGlobalIllumination while retaining direct lights, specular and reflections.
Four actions, 9.19 seconds, success. This control does not rebake the material.

`water-ripples-no-indirect-v1`, PID 30420 (exited): both test entries succeeded,
55 rendered warnings / zero errors. With GI=0 and LumenGI=0, the broad angular
patches disappear in the inspected near image while smooth fine wave detail
remains. This is the first control to remove that observed artifact. It localizes
the problem to the interaction with indirect illumination, not a demonstrated
failure of phase arithmetic. It is NOT a proposed production fix: turning off
GI globally changes the scene and is not acceptable as material polish.
Report SHA256: `9B0626A015F92A3D25E21AFEBD46BB8D2B413556E146A74F1CF537236A6A0CBA`.

Build `build-20260927-132111.log`: 4 actions, 9.39 seconds, success. Additional
static controls set exactly one runtime CVar to zero and restore its original
value (with an assertion) on exit: `NoShortRangeAO` for
`r.Lumen.ScreenProbeGather.ShortRangeAO`, and `SHDiffuse` for
`r.Lumen.ScreenProbeGather.IrradianceFormat`. The latter uses UE's SH3 diffuse
irradiance representation rather than the default octahedral representation;
the installed engine labels it slower. Neither control modifies project config
or disables GI. They remain incompatible with live/performance modes so their
images cannot be mistaken for the ordinary material benchmark.

`water-ripples-no-short-ao-v1`, PID 27752 (confirmed exited), completed both tests
successfully, 55 rendered warnings and zero errors. Actual control state:
ShortRangeAO=0 (previously 1), GI=1, LumenGI=1, all specular/reflection flags=1.
The inspected 2 m image no longer has the broad angular patches while the small
water-normal variation and coast remain visible. Original CVar value and view
flags were restored; the restoration assertion passed. `SHDiffuse` has NOT
been run and has no evidence of benefit.
Report SHA256: `9B9B93DCFEAFC1D3B0CCC0FCF1D12D22A5AD784C9DCE6C23ECF2A7DC3786E656`.

This localizes the visible interaction to Lumen's ShortRangeAO path in this one
fixture. Engine source `LumenScreenSpaceBentNormal.usf` samples screen-space
rays from `Material.WorldNormal`, which includes the added ripple, against the
undisplaced depth surface. False self-occlusion is a plausible mechanism, not
yet proven by trace data. Raising slope tolerance blindly is not justified:
`SSRTRayCast.ush` uses it as a depth hit window, not simply a normal bias.

Next action: pursue a water-specific treatment of that interaction while keeping
indirect lighting and land/contact occlusion intact. Do not promote a global
`ShortRangeAO=0` or `GI=0` setting as a water-material correction. No production
renderer setting, source, plugin binary or asset was changed in these runs.
There is now evidence against further blind wave/coordinate rewrites as the
sole fix. The current view-anchor asset is still a FAILED ordinary-lit candidate;
its control-only improvement is not material acceptance or a 120 FPS result.

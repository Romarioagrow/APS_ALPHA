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
- Physical positions remain LWC through native frame, divide and `Frac` nodes.
  Noise receives a bounded 96-cell coordinate. UE 5.4's simplex Jacobian hashes
  `6*T`, so its hash period is 576, matching the Cartesian wrap exactly. This is
  periodic noise, not a claim of mathematically aperiodic texture everywhere.
- Native DDX/DDY use the unwrapped LWC coordinate; the gradient fades between
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

## Current stochastic candidate

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
The two saved assets are also checkpointed under
`Assets/StochasticRipples20260927/`; they retain their `/APSWaterDetail/` package
identities. For exact-byte reproduction copy that folder to the isolated host's
`Intermediate/WaterRippleAssets/` and verify the hashes instead of rebaking.
Do not install any isolated DLL by itself into the accepted project/plugin.

Remaining gates: near-surface and orbital distances, temporal traversal,
normal walking, other Water palettes/subtypes and measured CPU/GPU parity.
No full-family visual acceptance or preservation of approximately 120 FPS is
claimed from this isolated fixture or its callback timing.

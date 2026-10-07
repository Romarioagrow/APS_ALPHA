# Planet cloud volume — published prototype, visual limitations open

## Rendered checkpoint — 2026-10-02 01:45, V26 rejected pending correction

Same DLL `672291ED6094D72BBEC0BEAB7AE1B17882CD12DAAAE88D98108814BEDB2987BA`:
11/11 real NullRHI contracts PASS (2 atmosphere-test teardown warnings).
Wind wrap now uses UE_DOUBLE_PI, not the float PI macro, with unchanged test
tolerances. Atmosphere fixture/expectations corrected without production sky edits.
Gas CaptureEditorBuffer bounds now survive public Save/Load/Apply and snapshot.

Real V26 runtime binding logged at the expected candidate path; bottom3.313km,
thickness2.125km,coverage.529,pressure1,humidity.562,temperature.658,maxSamples32.
Matched menu captures at fixed2000km,Terrestrial6750km,seed1337:
`terrestrial-weather-v26-stabilize-20261002` (ON35936) and
`terrestrial-weather-v26-off-20261002` (OFF40616), both under
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929`.
Each report13/13 PASS with warnings, yet ON views1/2 have severe fine white
grain absent from matched OFF. V26 is NOT visually accepted or published.
OFF also exposes angular coastline; separate terrain issue, not cloud fix.
All six PNGs were inspected. No shader change/rebake/default flip in this pass.
No timing acceptance follows from these static frames. Actual Slate dragging,
flight MID lifecycle, other condensates and wet-world A/B remain pending.
Both processes ended normally; editor window released to Claude HQ.

## Build checkpoint — 2026-10-02 00:41, UE tests still pending

The cloud wind/throttle fixes are now compiled and linked: successful incremental
20-action build in `work/planet_continuity_20260929/build-cloud-gas-stabilization-20261002-0024.log`,
followed by a successful4-action gas test-only build. A peer-owned subsequent
build changed the shared DLL again (fresh00:37 SHA256
`FF1E978628A7190CB61A5FED4DCA974AAAC15A8063BF5961112AEBD7B8A50BD5`), so use fresh
runner hashes, not an older build identity, for future screenshots.
The new menu weather run includes Clouds.Weather unit tests as well as captures.

No new Unreal tests or rendered frames from Codex yet: the contract runner
stopped at its active-process guard before spawning UE. Claude's actual ship
A/B40772/a4-trace-2b was verified live; it owns the resource window. Its process
later exited normally, but its paired run/handback still takes priority.
V26 remains OFF by default, V24 untouched; baked is not visually accepted.

## Bake and control-stability checkpoint — 2026-10-02 00:14

The external/shared build completed23:21 after MaterialDomain.h and the split
HLSL literal fixes. DLL SHA256:
`65DA9CCDA6FCE73EB56BD8134DAA3EEC2269585B526184C384181728ACD57E1D`.
Protected Clouds bake23:35–23:36 saved exactly one V26 candidate with0errors,
7warnings; no shader compile error was reported. Evidence:
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/bake-clouds-weather-v26-stabilize-20261001`.
V26 SHA256 `F02A63EF55CEB55BD23C44D954E8B97A24DB76FECBEADDF7D6B0E71955E7B830`;
V24 unchanged `12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21`.
Shared assets plus surface catalog9/9 unchanged on fresh comparison.
Weather factory0/config unchanged: V26 is NOT published/render-accepted.

After that build, two source-only control defects were fixed:

- Runtime wind integrates delta-time phase instead of multiplying all elapsed
  time by the newest slider speed. Ordinary refresh and zero wind preserve the
  orientation. A recreated component still starts at phase0; no saved-weather
  phase contract was added. Pure WindContinuity UE regression was added but has
  NOT run. Actual-MID diagnostic now checks the component's current rotation.
- Cloud refresh is throttled120ms without restarting a pending timer on every
  slider event. The callback reads the latest committed settings; existing
  selection-change flush and immediate persistence remain unchanged.

Four real translation units passed MSVC/Zs with the existing UBT flags/PCH:
APSPlanetCloudComponent, APSPlanetCloudWeatherTests, WorldGenerationViewModel,
APSPlanetRefinementRenderedTests. No object output/link/DLL change. This does
not execute tests or validate UI/render. Source/CPU weather4096 and density50000
also pass; all five HLSL literals are below the new16000-byte UTF16 guard.

Prepatch source backups: TEMP/aps-cloud-wind-20261001-235943-200 and
TEMP/APS-cloud-controls-prepatch-20261002-000112. The shader has not changed since
the V26 bake; the later runtime wind fix still requires a rebuilt DLL.
Next: free editor window, incremental build, Weather UE tests, actual slider and
save replay, then paired rendered menu/flight controls with V26 and sky preserved.
New foreign/user editor20324 (started23:38) remains untouched while permission
is pending. No render/build reservation is held against Claude.

## Source audit checkpoint — 2026-10-01 22:26, NOT BUILT

User's editor40952 remains in use. No closure, build/link, bake or GPU run.
Claude's priority and the released resource window are unchanged.

- Fixed weather diagnostic ON/OFF pairing: both menu and flight now admit the
  explicit OFF control with the same candidate identity. Flight also sets the
  weather gate at startup, not only in delayed ExecCmds. No default/config flip.
- Added a deferred real-MID weather binding check (scalars, tint, seed, wind),
  followed by changed settings, zero-coverage retirement and exact restoration.
  The new assertions run only in visual diagnostics, outside timing captures;
  original settings restore on failure too. They are NOT executed yet and do
  not replace actual UI slider, save replay or rendered/performance acceptance.
- Cloud UI summary now invalidates for categorical pressure/humidity/wind,
  manual state, seismic activity and storm scale as well as numeric fields.
- CPU/source audit4096 PASS; PowerShell syntax0errors; isolated preflight guards
  7/7 expected outcomes (4valid ON/OFF combinations,3rejected ambiguous/default
  combinations). Only guard prefixes were executed, not diagnostic launch code.
  Protected asset manifest25/25 unchanged. New V26 is still unbaked/unpublished.

Model audit caveat: APlanetaryBody initializes pressure/humidity/wind to the
zero/No* defaults, while the surface resolver falls back to Earth-like pressure
and humidity. NoPressure alone therefore cannot safely be treated as an explicit
vacuum without suppressing ordinary generated worlds. No global atmosphere or
surface-profile fix was made. Synthetic vacuum tests cover resolved pressure0,
not this raw-model ambiguity. Generated-body climate eligibility (especially
default wind0 / inactive geology, which can suppress vortices or ash) must be
checked before claiming exotic family coverage; do not invent physical input
or random storms merely to make the test pass. Flight remains the three wet
families; other compositions need actual model/actor and rendered coverage.

## Cloud authoring checkpoint — 2026-10-01 22:02, SOURCE ONLY

User's21:24 screenshot confirms the V24 limitation: disconnected white patches,
no authoring controls, narrow water-only family support. The user explicitly
keeps editor PID40952 in use; no closure, build, bake or GPU process was started
for the changes below. V24 remains the active asset, with its earlier limitations.

Prepared (NOT installed/render-verified):

- Seven climate-relative controls: coverage (0 clears), optical density, weather
  feature size, altitude, wind (0 freezes), vortex strength, independent weather
  seed; reset to automatic climate. New reflected CloudSettings is retained in
  the shared editor buffer, stable per-body overrides, planet/moon generation
  records and tagged save/legacy summary. Moon data is retained, but the existing
  renderer remains planet-only; no claim of new moon/gas-giant cloud support.
- Separate CloudWeather policy reads resolved pressure, humidity, temperature,
  type, seismic activity and model wind. Water/ice, ammonia, acid aerosol, dust,
  ash and hydrocarbon-haze profiles differ in density, altitude, tint and shape.
  This is an explicit game climate approximation, NOT chemical equilibrium:
  species partial pressures are absent from the current model. Vacuum,
  incompatible climate and authored worlds remain excluded.
- V26 shader candidate: broad domain-warped fronts, smoother edge shoulder,
  seeded compact spherical vortices only for eligible wind/climate; entire field
  (macro + billows + light taps) advects coherently. No longitude seam/reseeding.
  Same16–32 primary intervals and2light taps; additional weather math means GPU
  cost still needs measurement. No claim that unchanged sample count means free.
- Cloud-only UI refresh debounced120ms, immediate model save before selection
  changes; no surface rebuild/camera refocus/AtmoScape writes. UI climate summary
  is cached instead of resolving a surface profile every Slate paint.

Rollout gate: new aps.Surface.CloudWeather factory0, unchanged project config.
Cloud controls explain the pending weather material and remain disabled at0.
Existing CloudVolume20261001V24 package is untouched. New offline-only output is
Diagnostics/CloudWeather20261001V26/M_APS_PlanetCloud; not baked yet. No substitute
material is silently created if the candidate is unavailable.

Checked now:

- CPU/source audit4096sphere samples PASS: bounded finite field, monotonic
  coverage shoulder, zero-swirl identity, continuous longitude boundary
  (max difference1.37e-7); NOT shader compilation or visual acceptance.
  Evidence: work/planet_continuity_20260929/cloud-weather-cpu-20261001-v26.json.
- Existing density filter audit50000samples PASS (max mean error0.006283).
- Three diagnostic PowerShell runners parse with0errors; targeted git diff
  whitespace check passed (only existing line-ending notices).
- Four new Unreal tests written: Weather.Climate/Bounds/SaveReplay/Controls.
  They have NOT run; UHT/C++ and the material graph have NOT been compiled.
-25protected assets unchanged; V24 hash remains12F4F889…98DC21.

Current disk DLL is5FB2FF9B0D09A0FF5A82D6859E84E6728749B89C368C0F8A0925A0974C9BE3D7,
timestamp21:17:58, before new cloud settings21:38 onward. Read-only UBT log
shows a successful external/shared build21:17:29, including an earlier water
binder, but later shore edits21:25–26 are newer too. Do not reuse the earlier
74A919… DLL marker or claim the present source was compiled by that build.

Next authorized window:

1. Fresh Claude/process/resource check; ordinary rebuild + four new cloud tests
   and prior cloud/save/preview regressions. No live reload of reflected fields.
2. Bake NEW V26 with RunPlanetPrototypeBake -Candidate Clouds and a fresh label.
3. Explicit CloudWeather menu A/B: Terrestrial/Oasis plus eligible Ice, Ammonia,
   Greenhouse/Sulfur, Volcanic and dry climate; actual parent/uniform assertions.
4. Exercise sliders, focus A/B, regenerate/save/reload/game handoff; confirm
   geography and accepted sky unchanged. Flight runner initially covers the
   established three wet families; exotic flight acceptance is still pending.
5. Same-DLL cloud OFF/V24/V26 timing, inside/below/above, fast departure/reentry.
   Publish only after frames and measured cost; retain V24 for rollback.

## Current state — 2026-10-01 12:29

V24 is published with `aps.Surface.Clouds=1` in DefaultEngine.ini. Ground,
inside-layer and orbit routes were rendered for eligible generated Terrestrial,
Water and Oasis worlds; current evidence and limitations are recorded in
`2026-09-28-planet-visual-continuity-epic.md`, sections 08:48–10:16.
The accepted sky/atmosphere is retained. V25 was rejected and V24 restored.
Round/flat cloud undersides, approach grain and in-layer artefacts remain open;
this is not final cloud art or completion of the planet sprint.

The following checkpoints describe earlier development states, not the current
default. The current V24 asset hash was rechecked unchanged at 12:29:
`12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21`.

## Scope and acceptance

User asks for inexpensive, fly-through clouds consistent from orbit to ground,
conditioned by planet climate. Preserve accepted terrain, water, stars and ships.
At the initial checkpoint `aps.Surface.Clouds` defaulted to **0**; see the current
published state above. This document is not completion of the sprint.

Implementation uses one non-colliding, non-shadowing mesh bounding an analytic
spherical volume. Current V20/V21 uses 16-32 primary intervals and two in-cloud
sunlight samples per occupied interval (V1 below originally used16/no self-shadow).
No texture allocation, particle population or secondary far/near representation.
The same three-dimensional field/seed is evaluated at every distance, including
inside. Scene depth and the planet sphere clip the integral. Planet-relative LWC
subtraction precedes conversion to kilometres. Tiny angular bodies fade out.

Initial eligibility is generated Terrestrial/Oasis/Water only, water chemistry,
finite atmosphere >=12 km, humidity >=.25, pressure >=.1 atm, normalized climate
.20..76. Manual worlds, moons and dedicated servers are excluded. This is a narrow
water-condensate prototype, not a model of all exotic atmospheric chemistry.

Source: `Core/Planetary/APSPlanetCloudPolicy.h`,
`Core/Rendering/APSPlanetCloudComponent.{h,cpp}`, editor builder/HLSL,
and a narrow call at the end of existing `InitAtmoScape`.
No saved-data or terrain-generation changes. Existing materials untouched.

## Current checkpoint 01.10 01:50 — V21 source only, default off

V20 is baked and rendered on Oasis/Terrestrial, but NOT visually accepted:
the hard horizon cut became a broader bright band; the atmospheric integration
and smooth cloud forms remain inadequate. Oasis9/9; Terrestrial8pass/1fail due
to a civilization placement overlap (reported to Claude, not suppressed).
See the epic's 01:50 checkpoint for exact artifacts/hashes/coverage.

V21 fixes a demonstrated compositing bug: front-air transmission attenuated
both cloud radiance AND opacity. A .999 opaque cloud behind .2 transmitting
air incorrectly exposed .8002 of its background. Opacity is now determined
only by cloud extinction; the same example exposes .001. This does NOT solve
foreground in-scattering. Do not publish an extinction-only approximation as
a complete atmospheric model or claim that the horizon band is fixed.

`AuditPlanetCloudCompositing.cjs` evaluates the four actual HLSL accumulation
expressions in scalar CPU arithmetic: V20 fails290 checks; V21 passes857.
It checks conserved cloud transmission, premultiplied radiance, variable front
air and background leakage. New UE AerialOcclusion float test is not yet run.
Density/weather/seed and sample budget were not changed by this correction.

User is working in the editor and explicitly said not to close it. No build,
bake, runtime or performance run was started for V21. Current compiled/runtime
state must not be inferred from the edited source. New destination is
`Diagnostics/CloudVolume20261001V21`; previous V20 package remains available.
The read-only `ExportPlanetSkyGraphs.py` is prepared, NOT yet executed. Use it
in a later authorized offline window to inspect the real atmosphere graph
before adding foreground air radiance; do not run a second UE beside the user.

## Checkpoint 12:10

- Build v1 found an unexported installed-engine SceneDepth StaticClass; fixed by
  resolving the registered editor expression class, not changing the engine.
- Build v2 succeeded. New-only bake `bake-clouds-volume-v1` saved one material,
  zero errors/seven existing unrelated warnings; LocalVF present.
- Material: `Diagnostics/CloudVolume20260930V1/M_APS_PlanetCloud`.
  SHA256 `DFEF80F23E650662B1AECD3460F804E98C664FD6C5AD2ED591EB1439AEAF4982`.
- `oasis-cloud-volume-v1`: production water route and cloud policy checks
  8/8 (one warning); material creation logged in gameplay, no runtime replacement.
  Reviewed initial ground sky and route010/020/029: clouds not visibly established.
- `oasis-cloud-volume-orbit-v1`: reviewed zoom00/04; likewise no visible clouds.
  Therefore **NOT visually accepted** and no performance conclusions yet.
- Next bounded investigation: actual component flags, transform/bounds, camera
  coordinates; confirm visibility before spending on paired performance runs.

Evidence root: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/`.
Do not hide absent output behind successful automation. Do not turn on default
until orbit, below/inside/above views, continuity, eligibility and cost are checked.

## Checkpoint 12:55 — volume is visible, not published

- The initial component was modified by generic planetary proxy enumeration
  (hidden and rescaled). Moving it to its own transient attached visual actor,
  with an absolute transform taken from the committed atmosphere frame, fixed
  that independently. Collision/navigation/shadows remain disabled.
- V2/V3 interval colours proved the SceneDepth cut removed the entire volume.
  A depth-bypass diagnostic showed the field, but was never a publication fix.
- V4 GPU numeric readout: camera radius8750km, layer entry~1992km, conventional
  scene depth~1956km. Engine `SceneView.cpp:594` subtracts1e-8 from the inverse
  depth coefficient to avoid division by zero. At this preview scale a2000km
  depth is reconstructed as1955.740km:44.260km of bias, far larger than clouds.
- V5 reconstructs positive perspective depth from raw device depth and the
  projection matrix. Zero/background is far; orthographic retains engine depth.
  No camera near clip, global renderer, engine or terrain material was changed.
- `oasis-cloud-depth-fixed-v5`:8/8; inspected00and02. Cloud field now visible
  with normal Debug=0 and scene occlusion enabled. Still a sharp polygon edge
  in one patch and unaccepted art/inside/horizon/cost. Default remains0.
- V4 digit display used floating pow cast for decimal divisors; corrected to an
  integer table in V5. Do not quote its displayed digits without that correction.
- Current material is new-only `Diagnostics/CloudVolume20260930V5`.
  Native gameplay descent/return is running separately; no outcome assumed.

## Checkpoint 13:19 — crossing and paired cost, default remains 0

V6 reconstructs rays from screen position. It did NOT remove the sharp polygon
edge in the menu: do not count that hypothesis as a visual fix. Both-sided bounds
mode covers the screen, but that does not yet prove ordinary back-face coverage.
V7 adds an isolated back-face bounds mode to resolve this specific uncertainty.

V6 material SHA256:
`66187D118CBDE0647A5957F854D72F041DBB89C9C273B714909CCA9ED75004CD`.
V5 gameplay under a naturally cloud-covered land point was viewed at 100 km,
inside the layer and below it (`oasis-cloud-ground-return-v5`, 6/6, one warning).
The same volume is present on descent/return; ground/character are occluded
correctly in the inspected frames. Appearance is still a flat white blanket,
not accepted final cloud art. The previous wet-selected route was mostly clear
sky and must not be used as proof of cloud penetration.

Dedicated ON/OFF camera route, no PNG during measurement:
`oasis-cloud-perf-on-v6` / `oasis-cloud-perf-off-v6`.
Both use DLL `C10BC6DB4729EFF615AAF7883A190D1ACFBB5A7EA7A2070F3FAC15F90D5CEE9A`.
ON 7/7 (one warning), OFF 5/5 (one warning); both above/inside/below and safety=1.
Perspective-depth algebra regression passed; it supplements, not replaces, GPU proof.

| Metric | ON | OFF |
| --- | ---: | ---: |
| Frames | 1873 | 1870 |
| Median ms | 9.675 | 9.799 |
| p95 ms | 16.175 | 15.834 |
| p99 ms | 54.242 | 36.549 |
| Max ms | 111.393 | 103.449 |
| Frames >50 ms | 19 | 15 |
| Mean GPU ms | 5.415 | 5.299 |
| End process RAM MiB | 11488.24 | 11542.31 |

This is one serial pair, not a statistical speedup or a real ship benchmark.
Hitches remain in both. RAM differences are not a cloud allocation measurement.
ON has a small 2-second diagnostic log absent in OFF. Approximate layer-height
bins have only 26/31 samples; mean GPU there is 5.965/5.638 ms and cannot establish
worst-case cloud cost. Need sustained inside/horizon views and multi-body limits.

## Checkpoint 13:51 — two rendered causes fixed; weather candidate not accepted yet

- V7 back-face-only bounds, fog OFF, V8 ray RGB, segment and integral-alpha
  runs each8/8 (one warning). The cut was present in alpha, not in ray/coverage.
  All eight protected Shared assets stayed unchanged in each run.
- Float32 hash sensitivity: at lattice(-37,-34,17), reassociated operations in
  a CPU model produce0.9995117 vs0.0009766. This model alone is not GPU proof.
- V9 makes only hash intermediates `precise`, retaining the formula and seed.
  `oasis-cloud-hash-invariant-v9`8/8; inspected00and02. The exact polygon cut
  at x~1360..1470/y~730..805 disappears in00; other cloud shapes remain coherent.
  Material SHA256 `93863960EABAF55200CDCA504DBEB064276472CF548AF4F416122EC848A26422`.
- `matrix-cloud-families-v9`:10/10 (three warnings),18PNG, all three wet families.
  Inspected Terrestrial whole globe: too uniformly speckled, NOT accepted art.
- Further source/frame diagnosis: the cloud sun subtracted a camera-local
  displayed atmosphere centre from a physical star position. It followed the
  observer in preview, incorrectly illuminating the night side. Cloud-only
  SunDirection now subtracts physical star and physical body before rotation;
  no stellar subsystem or global light changed. Rebase regression added.
- `terrestrial-cloud-physical-sun-v9`: whole-globe02 reviewed, night side now
  dark, same speckled field. This is a lighting correction, not weather acceptance.
- V10 source/bake underway: regional weather + fronts + cloudlets, one consistent
  3D field. Subpixel billows smoothly converge to their mean using physical ray
  footprint. At most16 primary steps; no shadow rays, texture or geometry added.
  This new art candidate needs its own rendered crossing and cost verification.

Default remains0. V6 performance numbers above must not be attributed to V10.

## Checkpoint 14:14 — gradient weather and lifecycle validated, horizon pending

V10 regional value-noise weather failed visual acceptance: excessive opaque
cover and square plateaus. Its 10/10 menu result was structural only.
V11 uses invariant integer-hashed gradients and rotated octaves; the threshold
defines coverage and the density thins at the front. No shader-side seed/field
switch with distance. `matrix-cloud-gradient-v11`10/10 (three warnings): inspected
Oasis whole, Terrestrial whole/close and Water night-side. The square plateaus
are absent in these views, but cloud shapes remain smooth and flat-looking;
this is not final cloud art. Material SHA256:
`68D12A1501DE1100933244AC36D2274808F6DF697C95A0E7C33B2FDD90DAF0DB`.

`oasis-cloud-gradient-flight-v11`7/7 (one warning),997 measured frames with PNG.
Inspected000/010/011/032: cloud above, attenuation inside, clear ground below.
The route looks downward; it does NOT establish horizon quality/cost.
The post-ROI lifecycle test passed in the real generated world: three refreshes
keep one owner; manual and disabled states destroy it; both returns create one.
It restores the original manual flag and CVar priority/value on every exit.
The physical-sun/rebase and raw-depth regression tests also passed.

Same-DLL downward cost pair (PNG disabled within ROI):
`oasis-cloud-gradient-perf-on-v11`7/7 / `oasis-cloud-gradient-perf-off-v11`5/5.
DLL `305A5C2576AA5A93DD100689646EB1EA714C372E47C8E999AC4A84D5F0C4F1F9`.

| Metric | ON | OFF |
| --- | ---: | ---: |
| Frames | 1795 | 1792 |
| Median ms | 10.002 | 10.138 |
| p95 ms | 17.264 | 18.247 |
| p99 ms | 59.271 | 38.418 |
| Max ms | 114.646 | 106.520 |
| Frames >50 ms | 20 | 14 |
| Mean GPU ms | 5.502 | 5.333 |

One serial pair; no statistical speedup claim. ON had two-second diagnostics
absent from OFF. Worst frame114.646ms is at0.946km, below cloud; async GT/GPU
counters alone cannot attribute the stall. Streaming performance remains open.
All five V10/V11 menu/flight/performance manifests protect the same8Shared
packages; checked30September14:13,0changes in all five.

Dedicated horizon probe compiled (4actions21.51s):24s,360-degree view, six
seconds at layer middle and four below, above/inside/below holds. This is a
camera/pawn-coordinate route, not physical ship dynamics. Performance mode
now omits cloud diagnostic logging as well as ROI screenshots in both arms.
`oasis-cloud-horizon-visual-v11` running; default0 remains pending its result.

## Remaining before publication

### 30 September 22:20 — V13 horizon integration tested, not accepted

V13 uses16-32 bounded samples with static per-pixel phase instead of aligned
midpoints; weather/noise seed unchanged. Saved in bake-clouds-horizon-integration-v13.
oasis-cloud-horizon-visual-v13:7/7 structural tests,1418 frames, above/inside/below
and safety passed. Reviewed000/035: horizontal integration bands reduced but
the layer still reads as a flat white strip at grazing view. Do NOT publish or
claim acceptance/performance from this capture. Default0 retained. The next
cloud change must address field/vertical silhouette, not blindly increase steps.

### 30 September 21:48 — V12 horizon rendered, rejected for banding

After explicit GPU release, oasis-cloud-horizon-visual-v12-released ran on
the already baked V12 asset:5 clean+2 warning passes,0failed. The actual world
passed above/inside/below and lifecycle/safety checks (1431observed frames).
Reviewed frames000/035/080: clouds visible, but the middle-layer grazing view
has strong horizontal bands and the upper silhouette remains too flat. This
is NOT visually accepted despite passing structural automation. Keep default0.
Do not attribute older V6/V11 performance to V12; this pass includes PNG capture.
Next isolate ray integration sampling/vertical profile at grazing angles and
then repeat the same horizon route plus a capture-free ON/OFF pair.

Visible correct output; horizon/inside validation; same-DLL ON/OFF cost; no collision,
correct teardown/type change; test affected wet families and an ineligible case;
multiple-body cost/visibility budget. Real ship transit remains a separate check.

Technical reference: Epic's [volumetric-cloud documentation](https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine)
describes ray-sample and secondary-shadow-march costs; this prototype deliberately
has a bounded primary integral and no per-sample shadow rays. It is a project
material, not a claim that the native UE global cloud component handles our
multiple arbitrary body centres unchanged.

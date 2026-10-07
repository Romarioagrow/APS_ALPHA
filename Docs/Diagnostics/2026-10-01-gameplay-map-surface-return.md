# F10 map must not take ownership of gameplay terrain

## Rendered lifecycle checkpoint — 2026-10-01 10:46

The previously blocked route ran with the current installed DLL:
`FD066F8650596117C8FF8791E4035A785F4929C9D2BE3C73D707C2C4AFA6576E`.
Evidence: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/water-map-return-runtime-20261001`.
Report: 3/3 passed (2 clean, 1 warning), 0 failed/notRun. Existing fix, no new
production source/material changes. This supersedes the historical resource
block below, but does not close the original coast/flight acceptance.

- Actual F10 retains the same root and resolution256. Two scripted pawn
  departures reach roots0 and Unloaded; both returns acquire a new root and
  complete visible pawn-centred terrain/ocean LOD0. Near map and remote map
  focus do not take ownership or switch the gameplay root to preview96.
- Four inspected matching ground PNGs (BeforeF10, AfterF10, Trip0/1 ReturnedGround)
  retain the same surface/colony/sky. Colony geometry obscures part of that view;
  this is recovery evidence, not broad coast/terrain quality acceptance.
- IMPORTANT failed framing: inspected moving In03/In05/Trip1 Out03/In04 frames
  point into space rather than at the planet. The controller orientation was
  preserved while the pawn travelled radially. These frames do NOT establish
  visual continuity on approach; fix the diagnostic view lease before using
  this route as continuous visual evidence. No production camera bug inferred.
- CSV: 6142 route frames; outbound2561, inbound2356. Outbound median8.90ms,
  p9511.74/max144.19; inbound median9.09/p9512.83/max184.59; Returned123frames,
  median13.96/p9536.50/max266.99. Screenshot/automation overhead included,
  not a clean ship FPS benchmark. Hitches remain explicitly open.
- selectedToSettled12.317/12.235s includes the12s inbound journey, not isolated
  loading latency. Process RAM about11.1–11.6GiB; roots0 does not imply every
  allocator/cache page is returned to the OS. No user save was loaded/changed.

Family Water639.1442km/seed487132; other fixture fields are generated defaults,
not an exact replay of Rio's save. Foliage, collisions, lava and real ship control
are not proven by this route. Default clouds remain enabled; sky settings intact.

## User priority and evidence

Rio's 2026-10-01 02:30–02:37 screenshots show a flat home surface and very large
polygonal land/water boundaries after distant flight, map navigation and return.
Orbital appearance is accepted; preserve it. Near-ground reload is the primary
task now. Missing foliage families and collision on substantial objects remain
open; clouds are deferred, not removed from the wider sprint.

The user's home in this session is **Water**, not the Terrestrial preset:
`BP_Planet_C_3`, radius 639.1442 km, seed 487132. Diagnosis must include that case.

Original log preserved at:
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/map-return-20261001-prepatch/user-APS_ALPHA.log`.
Times below are UTC (local time = UTC+5).

- 21:03:15: home gameplay profile `terrainLod=10x256@120`, `oceanLod=10x256@120`.
- 21:03:17–18: home family selected, gameplay surface ready.
- 21:15:38: map planet focus applies a preview profile to the same live home:
  `terrainLod=10x96@120`, then persistent preview `10x96@180`.
- 21:15:39: gameplay streaming unloads the home; preview reports ready instead.
- 21:15:52 and 21:16:09: additional preview A/B builds on repeated map focus.
- 21:34:39: remote volcanic family unloads; 21:35:06 gravity reacquires home;
  21:35:15 surface fill enables at ~50 km; 21:35:34 ship enters surface mode.
  Home **never gets selected again by WorldScape streaming** before session ends.

## Cause and narrow fix

`ToggleStrategicMap` and `SAPSStrategicMapPanel::Focus` use the live generator's
`FocusPreviewTarget`. Its presentation pass already rejects non-preview worlds,
but `SetPreviewWorldScapeBody` did not. The legacy preview path clears the live
body's `bStreamWorldScapeSurface`, takes its root, and applies the coarse preview
profile. The streaming subsystem correctly excludes bodies with that flag off,
so flying back cannot repair the body. This is not evidence that every coastline
problem is caused by this bug, nor a reason to increase global mesh budgets.

Added the matching `!bIsPreviewGeneration` guard at the surface-ownership boundary.
Map camera selection/zoom remains available. Menu preview generation remains on
its existing path. No terrain, material, ocean, seed or quality settings changed.
No speculative preview roots should be allocated by ordinary gameplay F10 focus.
This may reduce one source of root accumulation, but **does not prove the user's
02:42 out-of-memory crash is fully fixed**.

## Verification status

- Source backup: same evidence directory, `AstroGenerator.cpp` and
  `APSWorldScapeStreamingSelectionTests.cpp`.
- New test `APS.Gameplay.World.PlanetSurface.StrategicMapPreservesStreaming`:
  small Water home, repeated overview/planet/body focus, stable producer/root and
  resolution, no extra roots, pawn-centred selection, two unload/return cycles,
  remote map focus, preservation of intentionally non-streamed authored bodies.
- This test manually ticks selection, **not WorldScape workers or rendering**.
- Build PASS: `map-return-20261001-prepatch/build-test-context.log`, completed
  2026-10-01 ~03:19 local; the rebuilt production DLL is installed.
- Final automation **4/4 PASS**, process exit status 0 at 03:20 local:
  `map-return-20261001-cpu-v3/automation.log` and `report/index.json`.
  Includes `StrategicMapPreservesStreaming`, `StreamingSelectionAndStandby`,
  `AllSolidTypesResolve`, and `GroundScaleRelief`.
- Initial CPU run failed at fixture setup (unregistered controller); second run
  exposed missing WorldContext. Fixed the shared scoped-world fixture with a real
  engine context and actor initialization, still no BeginPlay/world tick/workers.
  Assertions were not removed or weakened. Earlier failure/crash evidence retained.
- GPU preflight after all test processes exited refused a rendered launch:
  **RAM 5.31 GiB free**, commit 18.28 GiB, VRAM 7.20 GiB, GPU 1%; RAM minimum is
  8 GiB. No background user workloads were stopped. **No rendered frames, coastline
  acceptance or FPS improvement claim from this patch yet.**
- Build exposed an unrelated unity name collision in Colony/Civilization code.
  Qualified two uses of `APSColonyConstruction::RetryIntervalSeconds`, preserving
  its existing value/logic; coordinated with Claude and backed up the file.

## Required rendered acceptance

1. Start fresh with the rebuilt module (or reopen the same saved world). A module
   rebuild does not retroactively repair a running world already damaged by F10.
2. Capture the coast/ground before using the map; keep the current accepted orbit.
3. F10: overview -> home planet several times -> close. Ground must remain detailed.
4. Fly far enough to release the home family; open/focus/close the map remotely.
   Return to the same coast, descend, leave the ship; repeat the trip once.
5. Require home family re-selection, complete pawn-centred render LOD0, terrain
   and ocean publication, no preview profile logs on a live gameplay body, and
   before/after frames without giant straight shoreline facets or flat fallback.
6. Measure frame-time/memory during unload and return. Keep terrain bounded at
   distance; do not mask failure by permanently retaining every surface.

Do not close a user's editor or GPU workload for this check. Claude owns the
separate atmosphere/daylight change; this fix does not alter that file.

## Prepared rendered route (03:36 follow-up)

`Tools/Diagnostics/RunPlanetMapReturn.ps1 -Family Water -Label v1` now provides an
opt-in route inside the existing generated gameplay handoff harness. It does not
load or modify the user's save. Water radius639.1442/seed487132 are diagnostic
inputs, **not a claim to reproduce every field/geographic feature of that save**.

- Starts after the existing natural-ground first/+3/+8s captures.
- Actually calls the gameplay controller's F10 toggle; focuses the live home
  three times; checks the map camera was really selected; closes F10 normally.
- A controlled pawn follows a continuous radial out/return path relative to the
  body's current centre, allowing actual origin rebasing and production streaming.
- Requires complete home unload at distance, then another F10 session remotely;
  does two return cycles, with moving frames every two seconds and matching
  before/after ground frames. It does not hide interim fallback to get a clean shot.
- Final readiness checks actual visible, current-pawn-centred LOD0 and exact
  terrain/ocean payload sets, not just a ready flag. Never edits the render profile,
  forcibly selects a body, or claims readiness on behalf of the production streamer.
- Writes `Saved/Diagnostics/MapReturn.csv` (also on failure) with phase, altitude,
  frame delta, home state, ready state, root/worker counts and process memory.
- Restores movement/view lease on completion/failure. This is **scripted pawn
  lifecycle/rendering evidence**, not a ship-control test or FPS acceptance. The
  original user coast and real piloted flight still need their own visual check.

Build PASS (~03:34), PowerShell syntax PASS, CPU **5/5 PASS** at03:35: the earlier
four tests plus bounded/monotonic/inverse route mathematics. Evidence:
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/map-return-render-probe-20261001/`.

Actual protected render launch at03:36 was refused **before starting Unreal**:
RAM4.77GiB (<8), commit17.92GiB, VRAM7.31GiB, GPU15%. No new rendered frame exists.
The runner refuses existing evidence directories, busy editors/compilers and
outdated DLLs. No assets or user workloads were modified by this follow-up.

Read-only foliage follow-up: `APSWorldScapeFoliagePolicy.cpp` currently sets
asset/unit collision false, `Foliage_ForceDisabledCollision=true`, and disables
collision pooling. This explains missing collision; it is not corrected here.
Next collision change must be bounded to nearby substantial objects and validated
with all-family placement/density/collision evidence, not blanket collision on.

## Foliage audit and handoff (03:44)

No further production or asset changes in this audit. Missing coverage and missing
collision are two explicit limitations, not evidence that F10 also broke scatter:

- `APSPlanetSurfaceScatter::IsPublishedProfile` currently admits Frozen, Desert,
  Volcanic, mineral Terrestrial/Tundra, and biological Forest. The other prepared
  profiles remain behind the publication gate; all-family acceptance is still open.
- The builder and runtime sanitization force visual-only entries, and the fresh
  root policy disables both direct foliage collision and WorldScape collision
  pooling. Merely changing an asset checkbox cannot overcome all these gates.
- Native WorldScape provides per-mesh `BasePoolSize`, near/far check radii, velocity
  limits and update-time limits. Its collision component preallocates that count
  and uses `StaticMesh->GetBodySetup()`. Our owned meshes duplicate vendor sources;
  the budget builder does not validate their simple collision hulls. Therefore a
  bounded pool alone is not sufficient proof of usable or cheap collision.
- Next implementation must validate the owned meshes' actual BodySetup first,
  exclude grass/pebbles, cap the TOTAL nearby collision bodies across admitted
  types, retain distance/speed unloading and verify origin rebasing plus repeated
  departure/return. Do not enable all HISM collisions or unvalidated family gates
  as a workaround. Native asynchronous pool lifetime/scheduling also needs review
  before adopting it; no plugin source has been changed.

The same graphical-validation blocker persisted across the user-triggered pass
and two continuations. Latest preflight: RAM **5.31 GiB** (<8), commit17.90GiB,
VRAM7.32GiB, GPU24%; an earlier reading in this pass was RAM2.95GiB. At03:44 there
were no UnrealEditor/UnrealEditor-Cmd/UBT/compiler processes to release. No user
application was stopped and no GPU run was started. This is a physical-RAM
headroom blocker, not waiting for an editor handoff from Claude.

The source fix, compiled DLL, five passing CPU tests and executable render route
are preserved. All remaining acceptance here requires a resource-safe runtime
session. Resume after freeing headroom: recheck coordination/processes and DLL
freshness, run `Tools/Diagnostics/RunPlanetMapReturn.ps1 -Family Water -Label v1`,
inspect every transition/ground frame and CSV, then perform the actual ship/map/
return route. Keep the overall sprint incomplete until the coast, performance,
all-family foliage/collision and remaining sky work are really accepted.

## Separate lava evidence (03:59 follow-up)

The user's `2026-10-01 023017` screenshot must not be closed by the home/F10
regression result. Its navigation target is HIXUZOL (Volcanic); matching log
events identify BP_Planet_C_4, seed73875, radius approximately5831.706km,
noiseScale303, noiseIntensity1110499, terrain/ocean10x256@120. The family was
selected at21:29:40UTC and its liquid/surface published at21:29:51UTC, about26s
before that screenshot (local UTC+5). This does NOT prove current-pawn-centred
LOD coverage or an absence of geometry defects at screenshot time.

The live liquid binding in that event is SharedLava, `context=0`. The audited
builder's gameplay opacity-mask branch is constant coverage; presentation-only
VertexColor alpha clipping is selected by context1. Therefore do not blindly
soften the preview alpha mask to fix this gameplay picture. A runtime wireframe/
depth/geometry check is needed to distinguish actual terrain islands, triangle
intersections and missing mesh coverage; a dark island alone is not proof of a hole.

Reviewed the existing V3 Volcanic menu frame
`volcanic-fields-v3-magma-family-v2/Saved/Automation/PlanetRefinement/Volcanic/TerrainPixelAB/04-close-2000km-orbital-color-fields.png`.
It also has angular shore segments, but is seed1337/radius6750km and retained
menu geometry, not this gameplay planet. No claim of reproduction or acceptance.
The generic map-return fixture likewise uses the SMALL Water reference radius/
seed even with another Family argument; it must not be labelled an exact
HIXUZOL reproduction. Use the actual save read-only or an explicit isolated
matching fixture for that next visual check.

No new production edits or GPU launches in this follow-up. RAM preflight at03:55
was3.59GiB (<8). The F10/streaming source and completed test handoff to Claude are
recorded in PLANET_EDITOR_WINDOW.md; his access has priority. The remaining
coast/lava investigation is independent, but requires runtime evidence before
changing accepted geometry, palette, or physical shoreline for appearance.

## User-owned verification handoff (04:18)

Rio explicitly chose to test the compiled fix in game himself. No private GPU
runs, application shutdowns or further material changes are queued for this
handoff. Provided: full editor restart, same coast before F10, home focus/close,
two distant departures and returns; report whether any remaining facets occur
before map use or only after returning. Lava is still explicitly unverified.

Read-only refresh at04:18: no Unreal/compiler processes; the main gameplay log
still ends at02:42:35. The production DLL is03:33:58, newer than the guarded
AstroGenerator.cpp03:02:23. There is no new rendered acceptance to infer. The
next evidence must be Rio's result/frames or an explicitly handed-back runtime
session. Do not repeat memory-only polling or claim the full sprint is complete.

## User feedback: improved baseline, residual moving coast artifacts (04:47)

Rio supplied `C:/Users/Rio/Pictures/Screenshots/Снимок экрана 2026-10-01 043907.png`
and explicitly identified this as an unlit view. The coast still has narrow
stepped land/water fragments. He confirms that these flicker/rearrange with small
camera movements, and accepts the overall planet appearance: remaining work is
detail refinement, not a new palette/terrain redesign. This is partial visual
improvement, not acceptance of the residual coastline bug or the entire sprint.

The refreshed gameplay log belongs to a different fixture from the original
small Water home: Terrestrial, seed1337, radius6750km, BP_Planet_C_0. It reports
terrain/ocean10x256@120 and the WaterV1 MI_APS_CoastalWater binding, context0,
at23:31:46UTC. The user's editor started04:29 against the DLL dated04:29:15.
These logs establish assigned profile/material, not correct LOD coverage at the
04:39 screenshot. The outpost's1.4km HUD range is NOT camera altitude.

The moving unlit artifact is consistent with a depth/geometry interaction, but
does not prove z-fighting: mesh regeneration, coverage and sampling must still
be distinguished. Do not reopen the old F10-only diagnosis or blindly repeat
the previously rejected water-wave coordinate rewrites. No project near-clip
override was found in Source/APS_ALPHA or Config. Source builders retain full
gameplay terrain/water coverage and audit away WPO/PDO; this is source evidence,
not inspection of the live GPU graph. Do not hide the defect with arbitrary sea
level bias, a global mesh-resolution increase, or a palette change.

Next useful runtime evidence is a same-coast geometry/depth comparison while
holding the generated meshes fixed versus normal updates; retain the same
camera/light/material and distinguish water/terrain overlap from a LOD update.
Rio owns runtime verification for now. No production source/assets were changed
and no Unreal/build/GPU processes were launched in this feedback pass. Claude's
file/editor priority and the accepted overall visual baseline remain protected.

### Runtime gate, 05:39 follow-up

The three editor-only compiler-diagnostic calls in APSSharedGeneratedLiquidMaterial.h,
APSSharedWaterMaterial.h and APSUnifiedLavaSurface.h are guarded by WITH_EDITOR.
Comparison against the saved originals confirms identical Editor tokens (ignoring
comments/whitespace); non-Editor retains complete-map and LocalVF checks. This
is a source-level build compatibility correction, not a coast fix or a full
Shipping build pass. The DLL dated05:31:30 predates these05:32 edits.

The next causal visual check remains frozen geometry plus camera movement, not
another palette/bake experiment. At05:39 a new UnrealEditor PID33344 (started05:36:30)
is live. Free RAM4.80GiB, commit5.04GiB and VRAM1307MiB fail the existing8/12/6GiB
GPU preflight. The same unavailable safe runtime condition has recurred across
three goal continuations; no processes/assets are reserved or modified to force
a test. External resources/runtime handoff or user-supplied causal evidence are
required to proceed. The sprint is incomplete, not visually accepted in full.

### Resumed check after Claude's 05:48 handoff

The05:39 memory snapshot above is historical, not the current gate. At05:50 the
new user Editor PID23220 (started05:47:09) is live: RAM14.16GiB, commit23.35GiB,
VRAM4437MiB. Claude reports releasing his own window to Rio; do not confuse this
with permission to interrupt Rio's session. DLL05:41:13 is newer than the three
source guards, but no Game/Shipping compile result has been inspected.

The live log shows Rio switching fixtures and then starting Terrestrial gameplay:
seed1337/radius6750km, full-scale terrain/ocean10x256@120, WaterV1 context0,
published00:52:22UTC. Readiness is not an after-fix coast image.

New source-level distinction: native UpdatePosition uses the gameplay pawn
location; the flight override likewise uses Observer->GetActorLocation, not the
camera's viewing rotation. Asked Rio once to stop the ship, let geometry settle,
then rotate only the camera at the affected coast. This is a low-interference
first discriminator, NOT proof that geometry is frozen (body motion/rebasing or
pending workers can still matter). Await his answer before selecting a raster-
versus-generation experiment. No live properties, camera, view mode, materials,
or application lifecycle have been changed by Codex.

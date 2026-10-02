# Water surface filtering — 30.09.2026 06:00

Status: WaterV1 enabled by default on generated Water/Terrestrial/Oasis (30.09 11:23).
Reviewed close-water coordinate defect fixed; complete shore/ship-flight/art acceptance remains open.
This is the water step of the complete planet visual epic, not its completion.

## Current 11:25 — versioned water published and default enabled

bake-wateranalytic-coastal-release-v1 saved two NEW WaterV1 packages,0errors,
7known warnings;21protected packages unchanged. Master SHA256
208708383617B399C0A6DAE49C281B30228F9F6BDFE180E1B71610CBE64688F4;
MIC BA50661C73688465EC06F42BF2BAF6232964BFEE81FC4A78A7E0ABB5545D1359.
Source10:58 prewarm/selection/depth integration built11:00. Runtime menu and
gameplay use the same owned material, UV1 physical depth from existing native
samples, no per-frame CPU bathymetry. Legacy packages retained for rollback:
aps.Surface.CoastalWater=0, recreate profile/restart PIE. Exact authored/manual
identities, dry profiles and other chemistry are excluded.

matrix-water-release-menu-v1:10/10(3warnings),18PNG;6viewed across all three
families (whole+close). oasis-water-release-flight-v1:6/6(1warning), reviewed
010/029/040/050. terrestrial-water-release-flight-v1:5PASS/1FAIL from known
colony role1/WorldScapeRoot_0 overlap; route completed949frames,17changed depth
samples, reviewed029/050. The unrelated error is NOT suppressed or called PASS.
water-water-release-flight-v3:7/7(1warning), reviewed010/029/050/057;
first two attempts failed the old test harness's family/terrain assumptions,
not published water. Water keeps SharedTerra; its terrain rollout was NOT widened.
All these runs use ordinary production factory, payload=1 material=0;
Shared8 hashes rechecked unchanged for all four runs.

Important correction: Oasis upper-orbit images060/063 are near-black. This is
also present in original-noise candidate AND legacy water control
oasis-water-native-near-visual-v1 (4/4). Reviewed native060. Existing surface
fill cuts 2.2lux abruptly at50km. APSPlanetSurfaceFill now preserves <=10km,
smoothly fades10-50km to zero, same cutoff, colour and no-specular policy.
Stars, station fill, key/exposure unchanged. SurfaceFillContinuity scalar test
passed in Water v3; final same-route visual check runs in
oasis-water-default-fill-flight-v1 (no CoastalWater CVar override) passed7/7
(1warning),1029frames,17native depth changes. Reviewed029 and054-058.
At sampled heights15.6/23.5/33.4/45.6/59.1km brightness24.97/20.64/10.88/2.26/1.71:
attenuation precedes the visibility boundary instead of 2.2lux switching off
there. The orbit remains dark in this fixture, not magically daytime. This
confirms default factory activation and the narrow fill-continuity change only.
Do not claim all-orbit lighting/shore geometry/ship performance complete.

build-water-default-foliage-return-v1:5actions18.29s PASS, default1 compiled.
Full sprint still ACTIVE. Global foliage remains off; repeated native
repopulation/retirement route added for subsequent bounded performance pairs.

## Previous 10:58 — original-noise anchor accepted in close views and live route; publication in progress

The diagnostic four-sine kernel produced a regular checker-like water pattern
at the coast. Do NOT publish it. WaterAnchoredNoise20260930 instead keeps the
original two gradient-noise kernels and only fixes the coordinate domain.
Bake-anchor-noise-v1 saved2,0errors/7warnings,21protected assets unchanged.
water-anchor-noise-open2m-v1:3/3 including1warning,18PNG; reviewed nadir
unsplit/split and split oblique. Angular patches vanish without zeroing waves.
23protected unchanged. terrestrial-anchor-noise-coast50m-v1:18PNG,2oblique
reviewed, irregular ripples replace the sine grid. Overall2PASS/1FAIL from
Claude's colony actor placement overlap (reported), not a clean automation pass.

oasis-water-anchor-noise-nearflight-v1:4/4 including1warning,67routePNG+3ground.
Reviewed route010/027/032/050; no angular patches in those views. Native ocean
sections regenerate:1085frames,16changed samples; no freeze or test UV writes.
Route100km->2m->100km, NOT physical ship/residency acceptance.
Separate no-PNG pair, same10:38DLL: new1455frames vs native1539; frame median
10.2498/9.3200ms, p95 18.1051/17.8923, p99 35.3471/34.1769,
max75.7118/75.2050, >50ms13/8; GPU median4.2849/4.2125ms. One pair, no
statistical gain/smooth-flight claim. Surface style adds about0.93ms median.

WaterV1 publication source compiled10:56 (10actions40.42s), bake running.
Exact source masterSHA1 21C652A544125339495E87E3A8767C00FDC2E8F3,
MIC133A0EF6927CA02B3EFA84ACD117A0BC141D9611. Release copies tested style
into2NEW assets, protected Shared/catalog remain unchanged. Runtime selector
aps.Surface.CoastalWater defaults0 until release-path renders. Scope Water,
Terrestrial,Oasis; manual/custom/other chemistry excluded. New source adds
native depth auto-tag, same saved material in preview/gameplay, preview UV1
from existing samples in physical km, transform/rebase frame binding.
Preview prewarm follow-up/source tests still need the next build. Whole water
is NOT announced enabled or accepted yet; clouds/sprint remain unfinished.

## Previous 10:28 — anchored pixel domain fixes the reproduced angular wave patches

Secondary-domain-isolation-v1:3/3(1warning),18PNG, reviewed three nadir modes.
Folded secondary phase and UNFILTERED gradient already contain the angular
patches; footprint visibility is smooth.23protected packages unchanged.
This specifically isolates the pre-kernel pixel coordinate computation, not
the wave kernel, normal assembly, GBuffer or scene illumination.

APSWaterAnalyticWaveBuilder -APSWaterAnchorSplit now makes two NEW packages in
WaterAnchored20260930. It folds the high-precision view-origin phase before
adding the small per-pixel camera-relative physical delta. Derivatives come
from that small delta before wrapping. Both bands, amplitudes, analytic kernel,
depth, coverage, normals and lighting remain the paired analytic control's.
Unsplit analytic source is phase1, anchored candidate phase2, native0/4.
Build-water-anchor-v1:5actions18.62s PASS. bake-wateranalytic-anchor-v1 saved2,
anchorSplit=1,0errors/7warnings;21protected packages unchanged.

water-anchor-split-v1:3/3(1warning),18PNG; reviewed4,nadir+oblique unsplit/split.
Large angular patches remain in the control and disappear in the candidate.
water-anchor-normal-v1:3/3(1warning),18PNG; reviewed2,nadir unsplit/split.
Candidate retains a varying, smooth normal field; this is not zero wave strength.
23protected packages unchanged in each run. Native return/Lit restoration logged.
This accepts the reproduced static close-water defect only, NOT whole-water art.

terrestrial-anchor-coast50m-v1 currently running (50m above coast, depth0.5m).
Moving-route runner accepts WaterSurfaceAnchored with the existing live native
payload/material path; no new C++ build needed. Next: inspect coast, live
Terrestrial/Oasis route including regenerated sections and a separate no-PNG
performance pair, then decide publication. Candidate is NOT runtime-enabled.

## Previous 10:15 — artifact localized to secondary wave before normal assembly

Final normal audit built/baked successfully: bake-waterdomainaudit-final-v2,
2 NEW outputs, 21 protected packages unchanged. The first bake refused before
save because the existing world-rotation custom retains an unnamed default pin;
the diagnostic now resolves V/X/Y/Z by name, without changing that adapter.
water-final-normal-isolation-v1 finished10:02:21,3/3(1warning),18PNG.
Reviewed5: secondary/local/world residual nadir plus secondary/world oblique.
The SECONDARY filtered gradient already shows angular patches, propagated into
local and world normals. The primary gradient, reviewed again, remains smooth.
23 protected packages unchanged. No GBuffer/lighting change is justified here.

SecondaryDomainAudit separates actual secondary folded phase, unfiltered
gradient and filter visibility, retaining native before/return. New folder
WaterSecondaryAudit20260930, no production selector. Build12actions43.24s PASS;
bake-waterdomainaudit-secondary-v1 saved2,0errors/7warnings. Render currently
running under water-secondary-domain-isolation-v1; interpretation still pending.

## Previous 09:55 — first domain stages rendered smooth; final normal still open

bake-waterdomainaudit-v1 saved2 NEW packages,0errors/7existing warnings;
Shared8/catalog1/liquid12 unchanged. water-domain-isolation-v1:3/3(1warning),
18PNG, reviewed7: native nadir plus raw/physical/gradient nadir+oblique.
Raw camera-relative phase, compensated physical phase and PRIMARY filtered
gradient have smooth fields, not the broad angular patches of lit/WorldNormal.
23protected packages unchanged. This rejects neither secondary-band faults
nor later normal assembly/GBuffer quantization; water remains unaccepted.

FinalNormalAudit SOURCE ONLY follows actual MP_Normal closure: secondary
filtered gradient, amplified local normal-minus-radial residual, amplified
world normal-minus-rotated-radial residual. Separate new destination
WaterFinalNormalAudit20260930, same native before/return and runtime phases.
Build then bake -Candidate WaterDomainAudit -FinalNormalAudit -Label <fresh>;
normal water runner adds -DomainAudit -FinalNormalAudit to analytic Lit route.
Do not alter scene lighting/AO or publish emissive diagnostics as water art.

## Previous 09:22 — coordinate isolation prepared, not run

WaterDomainAudit builder/probe is SOURCE ONLY. A separate two-package diagnostic
copy of the analytic water graph exposes three emissive modes on unchanged
published ocean geometry: camera-relative small-coordinate phase, compensated
planet-fixed phase, filtered analytic gradient. Native before/return included.
This is not water art and must never be published as such. It separates the
coordinate/interpolation fault from noise and lighting rather than retuning yet
another wave kernel. Inputs are pinned to the actual two analytic package SHA1s.
Both pinned hashes rechecked09:17. Next: build, new-output-only bake, same-view
Water2m test, inspect raw/physical/gradient together; no cause claim yet.
RunPlanetPrototypeBake.ps1 -Candidate WaterDomainAudit -Label <fresh>;
RunPlanetWaterNormalAB.ps1 -Family Water -NativeColumn -ColumnArtPalette
-SurfaceFilter -SurfaceRelative -SurfaceAnalytic -DomainAudit -OpenWater
-HeightKm .002 -DepthM20 -WaveScaleFactor .1 -Label <fresh>.

## Previous 08:45 — CPU reference retested; visual artifact still OPEN

forest-leaf-fade-isolation-v1 also retested WaterSurface.AnalyticKernelContract
and FootprintContract:both PASS after UE_DOUBLE_TWO_PI. This corrects only the
CPU reference failure, NOT the persistent water normal artifact.
Read-only leaf-water-hlsl-v1 exported actual translated master HLSL for leaf and
analytic water; commandlet0errors/9warnings. No material package writes.
Water HLSL confirms the expected two precise camera-relative folded domains,
two analytic gradients, footprint outputs and radial/world-normal projection;
no unexpected vertex interpolation was found in the water normal graph. This
translation is not proof of GPU coordinate accuracy or completed water quality.
Do not repeat kernel-only changes or publish the still angular candidate.

## Previous 08:18 — analytic candidate rejected on rendered evidence

Build-water-analytic-foliage-lod-v1:4actions/19.91s PASS. The other candidate
sources were already compiled by Claude's preceding shared build. Bake
wateranalytic-v1 saved2 new packages,21protected unchanged. No production bind.
water-analytic-kernel-v1 captured18PNG, reviewed4(nadir and oblique control/new
pairs). Broad angular patches remain with either gradient-noise or analytic
sine kernel: changing the kernel alone is NOT a fix. Protected23 unchanged.

Automation: footprint passed, rendered capture succeeded with warnings,
AnalyticKernelContract FAILED fold invariance. CPU reference used the float
UE_TWO_PI macro despite FVector<double>; changed to UE_DOUBLE_TWO_PI, retaining
the strict tolerances and unchanged GPU source. Retest pending, not counted PASS.
Next: inspect the saved actual graph and material path before another candidate.
No performance acceptance; no claim about clouds or whole-sprint completion.

## Previous 07:58 — wave-normal isolation completed, analytic candidate source only

water-surface-wave-isolation-v1:2/2 tests(one warning),18PNG, protected23/23
unchanged. Reviewed the3 nadir on/zero/on frames: angular patches disappear
when ONLY WaveNormalStrength becomes0 and return when restored to0.025.
Depth, palette, camera, geometry and illumination were held unchanged. This
localizes the artifact to the wave-normal path, not a publishable flat-water fix.

New APSWaterAnalyticWaves/APSWaterAnalyticWaveBuilder replace only the two
tetrahedral noise kernels with four-direction analytic gradients per band,
retaining the camera-relative domain/footprint/depth/frame. Integer/3 directions
preserve both fold periods. New-output-only destination WaterAnalytic20260930;
inputs pinned by saved SHA1. Commandlet/runner/test integration is SOURCE ONLY,
not built or baked yet. CPU JS reference64points: max derivative error1.29e-9,
max fold error5.80e-12. The UE automation test is not run; no GPU/cost claim.

Next coordinated slot: max2 build, WaterAnalytic bake, same-camera gradient vs
analytic Lit close-water comparison. Only proceed to moving/performance routes
if the actual candidate images are acceptable. Do not publish an untested kernel.

## Implemented candidate

- Two existing gradient-noise bands now use 1200/410 cm scales rather than
  18000/28000 cm. Normal amplitude and baseline roughness are unchanged.
- Fold the existing domain before VectorNoise's float conversion; simplex
  periods 1023/1533. Pixel-footprint filtering uses derivatives of the unfolded
  coordinates so periodic folding does not itself produce derivative seams.
  This does not prove that the saved upstream coordinate path is precise.
- Noise no longer modulates water dye or roughness. No geometry, displacement,
  collision, coverage, light, geography or existing texture change.
- Built from the previous filtered-depth candidate, retaining its native UV1
  depth response. This does NOT solve the separate coast-geometry defect.

Source: APSWaterSurfaceFilter.h, APSWaterSurfaceFilterBuilder.h,
APSWaterSurfaceFilterTests.cpp; diagnostic bindings APSWaterNormalABProbe.h and
APSWaterFlightProbe.h. All are under Source/APS_ALPHA. Before-integration copies:
F:/ChatGPT/APOSFERA/work/planet_water_surface_20260930/source-before-v1/.

## Verified source -> build -> bake chain

APS DLL used by bake:
BC43F3A46DEC20BA2BF4E851DF65CAF593BC32D471F9BF73A5077F81C64B6CEC
(build_k13_edge completed 04:54; subsequent ship builds may replace this DLL).

Read-only source SHA1:
- M_APS_WaterDepth: A711A09E59D490487A04CAC4E8BDB5621E8833D2
- MI_APS_WaterDepth: C3451635932D6FDE94FF4D23E2EEF177265C352F

Bake evidence:
F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/bake-watersurface-v1/
PID28056 finished 05:01:48; Saved=2 bound=0; complete shader maps/LocalVF.
Commandlet reports 0 errors, 7 warnings. Warnings include existing missing
WorldScape128 and AtmoScape Starfield inputs; no water shader error.

New packages under Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterSurface20260930/:
- M_APS_WaterSurface.uasset SHA256
  6FB47DABBDAAD5B62D784562DB7D6EBEEBB56C9B5C789EDB167806307505BF0A
- MI_APS_WaterSurface.uasset SHA256
  267C0EC8E262EE4CDE3D6E9FEB91604697071C9E5DBFEAA1F3455CFFE4479B55

Protected assets after bake: Shared 8/8, SharedLiquid 12/12, catalog 1/1
unchanged. These hashes establish preservation, not visual quality.

## Rendered evidence through 05:23

- Terrestrial `surface-filter-v1-50m`: 18 PNG, 2/2 tests (one warning result).
  Same-geometry control/new-wave images retain the shore; limb is land-only.
- Terrestrial `surface-filter-v1-2km`: 18 PNG, 2/2 tests (one warning result).
  Reviewed control/new-wave nadir, oblique and limb: coarse pillowy wave shading
  is reduced; hard shore and broad depth-colour patches remain.
- Water `surface-filter-v1-open-2m`: 18 PNG, 2/2 tests (one warning result).
  Reviewed oblique/limb pairs; close detail is still too broad.
- Water `surface-filter-fine-v2-open-2m`: scales 120/41cm, 18 PNG, 2/2 tests
  (one warning result). Nadir new-wave frame has large square/triangular patches:
  visually REJECTED despite structural success. No performance claim or rollout.
  Evidence root: F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/.
- Oasis `oasis-surface-filter-live-v1`: 4/4 tests (two warning results), 786 frames,
  18 changed water samples, native UV1 retained, no frozen root or snapshot writes.
  Four route PNG reviewed, not all-frame/video acceptance or physical ship flight.
  Evidence: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/oasis-surface-filter-live-v1/.

The planned no-PNG timing pair was not run after the close visual failure.
At 05:28 no owned UE/compiler remained; editor window released to Claude.

## AO isolation result (05:50)

`water-surface-filter-fine-ao-isolation-v1`: 21 PNG, 2/2 structural tests
(one warning result). DLL SHA256
7367A518C38EDEF8B64584B9FA067A3CEC4D0D139C7E5D4FE7E30467FD3EC4B7.
The same fine-wave/depth/palette material was rendered with ShortRangeAO
on/off/on; CVar readback and restoration to 1 passed at all three views.
Reviewed nadir phases 2/3/4 and oblique AO-off: angular square/triangular
patches REMAIN with AO off. Therefore the older RIPPLES.md AO diagnosis
does NOT explain this current candidate; do not use it to justify a new pass.
All 23 protected packages from the runner manifest remain unchanged.

The separate SingleLayerWater surface-only builder and actual-scene fill
binding compile, but have NOT been baked or enabled. They are an unverified
alternative, not the result of this control. Production AO/config are unchanged.

The next narrow inspection is WorldNormal, with the same material graph and
frozen geometry, using the existing buffer-visualization scope and restoring
the lit viewport afterwards. New diagnostic flag: SurfaceNormalBuffer.
Build-water-normal-buffer-v1 passed 10 actions, 40.20s; includes current
concurrent materialization sources, without editing their files.

### Normal-buffer result (05:59)

`water-surface-filter-fine-normal-buffer-v1`: 18 PNG, 2/2 structural tests
(one warning result), finished 05:59:13; UE process absent at 05:59:48.
DLL SHA256 753EDBB2E55A1D373F4963428CAB4E9EDD2951CD397AC6D7E0157076F19A37DD.
Reviewed nadir previous-depth control/new-wave pair and new-wave oblique:
the angular patches are present in WorldNormal itself, while the old broad
wave control is smooth. Thus this current failure exists before lighting;
it is not a reason to alter scene AO, atmosphere or use a different shading
pass. The viewport restored to Lit in all three views, 23 protected packages
unchanged. This test localizes the defect; it does NOT fix it or prove its
exact arithmetic origin. Next: inspect the saved upstream wave domain and
make one bounded coordinate/precision comparison, retaining this material as
the failing control. Do not repeat lighting-pass experiments for this defect.

### Precise-domain candidate checkpoint (06:44)

build-water-precise-palette-v1 passed (13 actions, 45.06s). The first bake
bake-watersurfaceprecise-v1 then failed at the compensated-domain guard BEFORE
saving any package: centre/size/period contract failed. Destination absent;
21 protected packages unchanged. This is not a baked or rendered candidate.

The original flat-expression search was replaced with bounded upstream closure
traversal from the actual wave domain; unrelated Fresnel centre parameters must
not count as wave inputs. Another confirmed issue in the new custom expression:
UE creates a default unnamed input. Clear Inputs before adding named inputs so
DX/DY connections cannot overwrite Noise/DX. Last helper change 06:32; included
in Claude's later 06:34 build, but the bake and GPU comparison remain pending.
The candidate retains both noise bands, coverage/depth/palette and adds a
DoubleFloat physical coordinate domain plus derivatives before bounded demotion.
Run a fresh v2 bake, then matched fold-vs-precise Lit/WorldNormal at 120/41cm.
Do not call the precision hypothesis proven before inspecting those frames.

### Precise v2: baked successfully, visual hypothesis rejected (07:08)

bake-watersurfaceprecise-v2 completed 06:50:59: Saved=2 bound=0 noiseBands=2
domainFold=1 footprintFilter=1 surfacePass=0 compensatedDomain=1, no errors.
21 protected packages unchanged; only the two new diagnostic assets saved.
water-surface-precise-fine-lit-v1 then passed 2/2 structural tests (one warning),
18 PNG. Inspected phase1/2 nadir and oblique pairs (4/18 PNG): angular patches
remain. The DoubleFloat-after-world-position hypothesis did NOT fix them.
No ordinary water defaults changed. Do not repeat identical precision/AO tests
or claim that a successful material bake is the requested visual improvement.

## Investigation rationale (before the AO result)

### Camera-relative reconstruction: visual hypothesis rejected (07:34)

bake-watersurfacerelative-v1 saved two NEW diagnostic assets, zero errors/seven
warnings, cameraRelative=1. All21 protected packages unchanged. The shader
reconstructs the planet-fixed DoubleFloat domain directly from the near-camera
position and PreViewTranslation, avoiding prior FWS reconstruction. Kernel,
normal strength, footprint, depth and shading model are unchanged.

water-surface-relative-fine-lit-v1:2/2 structural tests(one warning),18PNG.
Reviewed nadir absolute-control/camera-relative pair2/18: broad angular patches
remain. Protected23packages unchanged. This is a SECOND rejected precision
hypothesis, not a water fix. Ordinary water unchanged. Next diagnostic uses
normal-strength on/zero/on on this SAME MID with constant depth and light, no
additional bake. A flat result at zero is localization, not publishable water.

## Historical investigation rationale (before the AO result)

Do NOT assume that the angular close-water patches are precision loss. The prior
isolated RIPPLES.md records failed coordinate rewrites and successful localization
to Lumen ShortRangeAO in that older fixture. Revalidate on this current material:
same camera/geometry/depth/palette, fine waves, AO on/off/on, then native return.
The optional SurfaceAOIsolation probe restores the CVar and is diagnostic ONLY;
global AO removal is not a production fix. No new material baked for this check.

## Reproduction / outstanding acceptance

RunPlanetWaterNormalAB.ps1 -Family Terrestrial -NativeColumn -ColumnArtPalette
-SurfaceFilter -HeightKm 0.05 -DepthM 0.5 -Label surface-filter-v1-50m

One session, fixed geometry/camera/light, five phases:
0 native; 1 previous depth material; 2 filtered waves with same depth/palette;
3 new waves with depth response zero; 4 native return. Three view angles each.
The runner also executes WaterSurface.FootprintContract (numerical reference,
not proof of GPU precision). Inspect actual 1/2 frames and the 0/4 bracket.

Then check 2m open water and 2km approach, Water/Terrestrial/Oasis coverage,
and a live regenerating WaterFlight route. Measure performance separately with
-Performance (no PNG during the measured route), old-depth vs new-wave on the
same DLL and no competing workloads. Reject seams, tiling, lost coverage,
plastic flatness, unresolved flicker or material/performance regressions.

Early frame-run launches were correctly prevented by active concurrent
compiler/UE processes; no evidence folder was created by those attempts.
Only the reviewed mid-distance wave appearance is improved. Runtime enabling,
natural shore transition, full-family coverage, flight residency, clouds and
foliage remain requirements of the original epic.

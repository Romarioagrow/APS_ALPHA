# Replaceable foliage palettes: bounded prototype preparation

## 2 October 03:59 — generated solid-family defaults enabled by user request

Config now sets SurfaceScatter=1 with Enable=2. All 32 solid/legacy families
can use their existing habitat/mineral palettes on fresh generated runtime
roots; authored/manual/preview roots remain protected. Species/climate/water
filters and existing budgets are unchanged (1 collection, 5 roles, 64 attempts;
32 local collision proxies within 40m). The 02:20 Unreal contract run passed
ScatterAssetCoverage for all 64 assets and the scatter/collision contracts.
This is not all-family visual or flight/unload/reentry acceptance. Historical
publication-gate statements below describe the earlier state.

## Source-only batch, 2 October — all-role grass fade and collision retirement

Prepared for the shared build and subsequent sequential validation; NO Unreal,
build, bake or GPU run was launched for this batch. Existing publication gates
and accepted type/habitat combinations are unchanged. The 64 existing V2
collections (32 solid/legacy types, each habitat and mineral) and ten owned
meshes are already present; no extra assets need generating for basic coverage.

- Corrected the narrow Terrestrial-only grass repair: owned Grass and ColdGrass
  now use the existing leaf distance fade with their original near AND billboard
  texture atlases on every admitted family. DryGrass retains its existing two
  bindings. Exact mesh-role/source-material matching avoids replacing authored
  or unfamiliar materials. No geometry, seed, location, count, habitat, terrain,
  atlas or distance change. New families still require explicit scatter opt-in.
- Unregistered visual sectors are now ineligible for walking collision, so an
  existing local proxy cannot block invisible, temporarily unregistered objects.
  Re-registration restores the same bounded pool. Existing limits stay32 nearest
  proxies/40m/5Hz; RockA, RockB, SlabA, SlabB and lower trunks block, while small
  Pebble, grass and canopy intentionally remain nonblocking. No HISM-wide physics.
- Extended LeafFadeContract and WalkingCollisionLifecycle/Shapes for these cases.
  Added ScatterAssetCoverage: loads each pre-existing solid-family palette and
  checks five distinct meshes, material slot bindings, species habitat masks,
  finite scales/attempt budgets and outside-water flags. This is a structural
  contract, NOT proof of shader quality, five species at every site, or gameplay.

Backup: C:/Users/Rio/AppData/Local/Temp/APS-foliage-batch-20261002-0210.
Source review and whitespace checks only so far; tests wait for the common build.
No streaming/Claude files, Config, native plugin, shader, asset or save changes.

Final validation routes already available:
`RunPlanetFoliageGround.ps1 -Family <solid type> -SurfaceScatter -Label <fresh>`
uses Enable1/SurfaceScatter1 only in its isolated process and accepts all32 types.
Normal published regressions use `-Published`, without a forced palette. The
existing dedicated foliage flight runner currently admits Forest/Frozen and
Water/Oasis/Metallic/Crystal/Terrestrial; that allowlist is NOT all-family reentry
coverage. Each new published type/habitat needs its corresponding rendered and
return-path evidence first. Do not promote the whole list from asset presence.
Distance extension remains open: current leaf fade80–95m and mineral cull100–200m
are unchanged until the common performance pass justifies a bounded increase.

## Current checkpoint, 1 October — generated Terrestrial visual vegetation

Project default `aps.WorldScapeFoliage.TerrestrialVegetation=1` now admits a
presentation-only vegetation preset on generated non-manual Terrestrial roots
with zero/unconfigured biology and no authored foliage policy. Factory fallback0
and console0 retain the previous mineral palette on fresh resolved roots.
Explicit nonzero biomass/biodiversity, manual bodies, moons, other families and
authored enabled/disabled collections are not rewritten or newly published.

Why a separate field: Habitable classification does not populate biomass, and
fleet scans/anomalies consume resolved Biomass. The initial V2 experiment which
changed resolved biology was rejected. V3/V6 instead use transient
VisualFoliageDensity=.55 ONLY for foliage mask/admission and cache signature.
Body AND resolved gameplay Biomass/Biodiversity stay zero. Signature sequence
with zero visual density is unchanged; nonzero density intentionally refreshes
the visual profile. No save format change or fresh saved-world replay claim.

New tests verify finite/admission guards, authored overrides, exact publication
scope, and512 paired sampler points across4seeds: physical height, normalized
material height, temperature, humidity, water mask and hole state are identical.
Dry/submerged samples retain zero foliage mask. Existing climate, slope/water
gates and world/sector envelopes remain; no biome or spawn relocation to find
trees. Recipe remains Pebble/RockA/RockB/Grass/TreeA:12/8/5/20/4 attempts per
80/80/80/80/160m sector, total49 <=64. Grass is nonblocking; rocks/lower trunks
use existing nearest32/40m walking proxies, not HISM-wide physics.

Natural sample sites differ legitimately: seed424242 local humidity~.021 gives
zero vegetation;82276 humidity~.206 gives1grass/0trees;487132 humidity~.849
gives126grass/38trees. No increase in density to force dry-site acceptance.
The grass count initially lied about visibility: BOTH natural LOD and forcedLOD0
detail frames were empty. Only owned Terrestrial Grass entries now replace the
vendor radius-dependent fade with the existing APS80-95m leaf master, retaining
their original grass color/normal atlas. No mesh/vendor/material asset bake.
Fresh before/after natural frames visibly show the same grass clump after fix.
Other families keep their bindings. These are prototype assets, not final art.

Evidence root F:/ChatGPT/APOSFERA/work/planet_continuity_20260929; suffix20261001:

| Run prefix | Result | Actual evidence |
| --- | --- | --- |
| terrestrial-habitat-candidate-ground-v1 |19/19|Habitable input still zero biology/mineral-only; not vegetation acceptance |
| terrestrial-habitat-ecology-dry-v2 |Failed|Rejected resolved-biology design, NaN guard failure and absent fourth dry-site shape retained |
| terrestrial-vegetation-seed82276-v3 |19/20|No fifth natural shape; the single grass was also visually invisible |
| terrestrial-vegetation-seed487132-v3 |20/20|Natural tree visible; grass still invisible despite counts, NOT visual acceptance |
| terrestrial-vegetation-grass-ground-v4 |20/20|Corrected natural grass visible; tree/RockA/RockB reviewed |
| terrestrial-vegetation-candidate-reentry-v4 |22/22|Two grounded cycles, collision/exclusion and content return/retire |
| terrestrial-vegetation-default-ground-v5 |20/20|Query-only default1 from SystemSettingsIni, fresh natural grass/tree frames reviewed |
| terrestrial-vegetation-default-reentry-v5 |21/22|Diagnostic allowlist rejected default Terrestrial before scene; retained |
| terrestrial-vegetation-default-reentry-v6 |22/22|Diagnostic gate fixed, production unchanged; both grounded cycles pass |
| oasis-terrestrial-vegetation-regression-v6 |20/20|Accepted Oasis preserved; settled screenshot with clouds actually viewed |

20/20 includes18clean+2warning tests;22/22 includes20+2. Default wet-site counts
177/107/75/126/38. Detail Shape1 shows grass, Shape2 tree, Shape4/5 rocks. Shape3
pebble is partly hidden by a naturally generated tree: do NOT claim full visible
five-shape coverage from that image/count alone. Mineral geometry was previously
verified separately and unchanged. Normal colony ramp remains in settled frames;
no colony-art acceptance. Ground-facing flight frames are dark terrain closeups,
so lifecycle telemetry is NOT proof of attractive return-camera composition.

Default V6 has1852frames,503ground-hold frames allgrounded with10 active/allocated
proxies,199/207 structure exclusions.107settled-orbit frames have0 instances,
components, native foliage workers, active/allocated proxies, exclusions/pending.
Settled windows are20-21s and>=41s at>50km; reapproach at~22s can legitimately
start native work again. Peak601instances/process11754.422MiB (whole editor,
NOT incremental foliage memory). Camera-coordinate route is NOT ship dynamics.
Dry-site default reentryV6(seed424242) also22/22:1902frames,463ground frames,
2walking proxies/166exclusions,119settled-orbit frames allzero. This confirms
runtime recovery on the earlier dry site, not five locally visible species or
new per-species flight telemetry. The512 sampler test separately checks dry
mask zero. No biome is forced wet to obtain vegetation.

Screenshot-free pair `terrestrial-vegetation-perf-on/off-v6-20261001`:22/22 each,
same seed487132, DLL/native DLL, D3D12SM6,1600x1000 requested resolution and route.
Only foliage selection/master enable differ; no cloud/terrain option differs.
This pair uses the original controlled camera route, NOT grounded walking:
both have0 physics proxies, so it measures visual foliage, NOT collision cost.

| Metric | Foliage ON | Foliage OFF |
| --- | --- | --- |
| Total frames |3302|3509|
| Near-hold frames |762|789|
| Near frame p50 /p99 /max ms |10.3062 /13.4310 /14.0098|9.9883 /12.4778 /13.5948|
| Near GPU p50 ms |5.0484|5.0086|
| Full-route p50 /p99 /max ms |11.0648 /48.8929 /81.9857|10.4736 /35.9907 /73.8738|
| Full-route frames>50ms |33|22|
| Peak instances /whole-process RAM MiB |626 /11724.402|0 /11657.508|

One ordered pair, not statistical proof: near median difference~.318ms and no
near-hold frames>50ms in either leg. Full-route stalls remain in BOTH legs;
ON has more in this pair. Do not claim zero cost, universal optimization,
incremental RAM from peak subtraction or attribute every hitch to foliage.
Further flight profiling remains required, including actual ships. Reapproach
and retirement intervals retain raw CSV evidence; no stalled frame was removed.

BuildV6 DLL74A9198E7CCA437E571E1051194125DCA9F61EADF865DB9C099AA6A0E7035C43.
CloudV24/nativeCore/25protected assets unchanged in current checks. No accepted
terrain/geography/seed, sky/atmosphere/cloud, ship/character/colony changes in
this checkpoint. Remaining families, explicit populated Terrestrial profiles,
near-liquid defects, cloud art polish and flight stalls remain open.

## Previous checkpoint, 1 October — barren Metallic/Crystal published

Ordinary generated barren Metallic and Crystal roots now use the existing
five-geometry mineral palette: Pebble, RockA, RockB, SlabA, SlabB. Heights
22/55/95/140/190cm, attempts12/8/5/3/2 and sectors80/80/80/120/160m are unchanged;
30 attempts remain inside the existing64 collection cap. Only reviewed barren
profiles were promoted. These are replaceable stone prototypes, NOT final
metal/crystal art. No mesh/material bake, source terrain or atmosphere changes.

The first normal starts failed before foliage. Failure-only audit on seed424242
found157/157 sampled walkable nonzero-relief candidates in each family; largest
near relief259.344/263.084cm could never pass the mandatory300cm hero-composition
gate. `ResolveSpawnLocation` now recovers ONLY after both original searches fail
on a new daylit non-oceanic start: reuse the same deterministic directions with
the existing oceanic non-flat floor15/150/350cm at10/100/250m. Dry centre/nearby
clearance, daylight/eclipses, maximum slope.25, capsule clearance and subsequent
cooked-collision checks remain intact. No terrain/seed or colony-transform edit.
Accepted starts and saved replay do not enter recovery. The latter is covered
by the pure admission truth table and source gate, NOT a fresh save replay run.

New APSSurfaceLandingRelief helper/test covers unchanged hero thresholds, all
recovery-admission combinations, insufficient/flat and nonfinite relief. Final
natural landings have near/mid/far relief261/2134/3889cm (Metallic) and
262/2131/3872cm (Crystal). Oasis regression selects exactly its previous two
locations/relief values, without fallback; its fresh settled frame was reviewed
and clouds remain visible. Successful existing-start composition is preserved.

Evidence root: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929.
All listed run suffixes are20261001; failures are retained, not hidden.

| Runs before the date suffix | Result | Scope |
| --- | --- | --- |
| metallic/crystal-mineral-candidate-ground-v1 |17/18 each|Original ordinary-start rejection, no rendered acceptance |
| metallic/crystal-mineral-spawn-audit-v2 |17/18 each|Measured the same rejection without changing terrain |
| metallic/crystal-mineral-relief-ground-v3 |19/19 each|Natural start, all five natural-LOD detail frames actually reviewed |
| oasis-relief-regression-default-v3 |19/19|Normal defaults, unchanged successful landing selections, fresh frame |
| metallic-mineral-candidate-reentry-v3 |20/21|Diagnostic type-parser allowlist rejected the new route |
| metallic-mineral-candidate-reentry-v4 |20/21|Diagnostic expected ContinuousTerra instead of actual SharedTerra |
| metallic/crystal-mineral-candidate-reentry-v5 |21/21 each|Exact SharedTerra binding, two coordinate-controlled cycles |
| metallic/crystal-mineral-default-ground-v6 |19/19 each|No scatter overrides; fresh settled and natural detail images reviewed |
| metallic/crystal-mineral-default-reentry-v6 |21/21 each|Query-only normal collision/exclusion/palette, two grounded-return cycles |

19/19 means17 clean tests plus2 tests with existing warnings;21/21 means19+2.
The diagnostic parser/binding fixes did not substitute any production material.
Candidate natural counts were180/115/75/52/45 and191/125/72/56/38 respectively.
Acceptance images: APS_Scatter_20_Shape*_ExistingInstanceDetail.png and
APS_Scatter_34_Shape*_ExistingInstanceDetail.png under each run's
Saved/Screenshots/Windows. These are existing instances and natural LOD;
separate forced-LOD0 diagnostic images are not acceptance. The colony ramp and
prototype lighting remain visible in settled frames; no colony/art completion
claim. No new walking-around-object proof for these two families: previous
Frozen/Forest CharacterMovement tests remain separately scoped below.

Final v6 CSV measurements (one controlled route each, not ship physics):

| Metric | Metallic | Crystal |
| --- | --- | --- |
| Total frames / ground-hold frames |3530 /780|3762 /798|
| Ground active/allocated proxies, all grounded |5 /5|3 /3|
| Ground exclusions / pending |157 /0|190 /0|
| Settled-orbit frames, all content/physics/workers/cache zero |349|338|
| Peak instances / process RAM MiB |562 /11070.891|773 /10979.883|
| Ground median /p99 /max ms |9.9377 /13.6029 /14.2367|9.8395 /12.6431 /13.5755|
| Full-route median /p99 /max ms |10.5899 /35.7500 /72.7271|10.2576 /32.5898 /46.4639|
| Full-route frames above50ms |3|0|

Ground windows8-12s and29-33s below.1km; settled orbit20-29s or>=41s above50km.
Both returns restore content, bounded collision and exclusions. Both settings
read1 from SystemSettingsIni, not command-line ON. No universal hitch-free or
speedup claim; Metallic still hitches and other-family stalls remain open.

Builds v1-v6 succeeded; final DLL
AA882A4322BA5F5B6FCDB3029721A22C87744BDB99276DE9715F46990B1A2DFA.
CloudV24 12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21,
nativeCore031AE7C670B95F68FF6E2DA05AF0F8473B0D92CEDEED89DA81F8227283C7074C,
all25 protected assets unchanged. Existing policy's stale Oasis comments were
corrected without changing its budget. No foreign process was closed and no
newer Claude request/acknowledgement was observed. Remaining palettes, inhabited
Terrestrial, near-liquid visual acceptance and flight stalls remain OPEN.

## Previous checkpoint, 1 October 18:08 — habitat Oasis palette published

Fresh generated habitat Oasis roots now automatically use the five-shape APS
prototype palette: Pebble, RockA, RockB, DryGrass and TreeA. No other unpublished
family was promoted. Author/manual/scaled-preview paths, geography, climate,
biomass, seed, native WorldScape, material assets, sky and CloudV24 are unchanged.
These remain replaceable prototype meshes/materials, not final art.

Read-only habitat sampling found a sparse local native mask: mean .0152956 for
seed424242 and .0163213 for seed82276, each81 dry samples over a640m-wide grid.
Original3 tree candidates per180m sector produced0 trees at both test sites.
Only the transient copied Oasis TreeA entry now uses12 attempts from project
SystemSettingsIni; factory fallback3 remains the reversible old budget. Other
four entries are unchanged: total57 candidates <= existing64 cap. The pure
helper caps requested tree attempts at16 and only uses spare collection budget;
six new tests cover normal, unchanged, full/partial cap, excess and invalid input.
This does not bypass native water/slope/climate gates or guarantee every species
at every site/seed. Seed82276 is from the screenshot, but other generator sliders
were standard: not an exact replay of the user's authored Oasis.

All evidence below: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929.

| Run | Result | Evidence and scope |
| --- | --- | --- |
| oasis-habitat-audit-v1-20261001 |17/18|181/123/76 pebbles/rocks,3 grass,0 trees; retained diagnostic failure because visibility ray hit the subject's own new physics proxy |
| oasis-habitat-seed82276-v2-20261001 |17/18|174/123/65 minerals,6 grass,0 trees; proxy-only diagnostic fix works, fifth natural shape missing within1km |
| oasis-habitat-tree12-seed82276-v3-20261001 |18/18|Same minerals/grass plus3 trees; all five natural-LOD detail frames reviewed |
| oasis-habitat-tree12-seed424242-v3-20261001 |18/18|Same181/123/76 minerals,3 grass plus7 trees; all five natural-LOD detail frames reviewed |
| oasis-habitat-tree12-reentry-v4-20261001 |20/20|Two coordinate-controlled cycles;393 peak instances, full-route24 frames>50ms |
| oasis-habitat-tree3-reentry-control-v4-20261001 |20/20|Same DLL/seed route,383 peak instances, full-route26 frames>50ms |
| oasis-habitat-default-ground-v5-20261001 |18/18|Query-only normal project settings; fresh settled ground and natural TreeA frame reviewed, clouds visible |
| oasis-habitat-default-reentry-v5-20261001 |20/20|Query-only palette/density/collision/exclusion defaults; both return cycles restore content and physics |

18/18 means16 clean+2 existing warnings;20/20 means18 clean+2 warnings. Failed
pre-publication attempts are retained, not relabelled as passes. Accepted shape
PNGs are Saved/Screenshots/Windows/APS_Scatter_25_Shape*_ExistingInstanceDetail.png.
They use existing natural instances/LOD, no synthetic placement or forced LOD;
separate forced-LOD0 diagnostic images are not the acceptance evidence. Mineral
materials are dark placeholders; DryGrass remains bright green and TreeA stock
prototype art. Cloud visibility is reconfirmed on Oasis, not all other families.

Paired density12/3 ground holds: medians10.271/10.622ms,p9913.203/13.930ms,
max14.057/15.980ms,0 frames>50ms. Peak RAM11708/11630MiB. These bounded runs show
no observed extra hitch cost, not a speedup or universal budget guarantee.
Those routes were elevated coordinate flight, not grounded walking, so the
separate final default route below checks actual active-proxy lifecycle.

Final default CSV3421 frames:773 ground-hold frames ALL6 active/6 allocated
proxies,137 exclusions,0 pending;408 settled-orbit frames ALL0 instances/proxies/
allocation/exclusion/pending. Peak393 instances. WalkingCollision1,
StructureExclusion1 and OasisTreeAttempts12 all LastSetBy:SystemSettingsIni.
Ground median10.1813ms,p9913.1272,max15.9605,0 frames>50ms; full-route
median10.669898,p9938.168501,max92.778102,27 frames>50ms. Flight stalls remain.
This is lifecycle coverage, not ship dynamics or an Oasis tree walk-around;
previous Frozen/Forest actual locomotion evidence remains scoped as below.

Build v1-v5 succeeded. Publication patch first stopped at a cosmetic policy cpp
comment write after header/test changes; no UE run used the incomplete config
state. Separate checked patch completed config/runners before default runs.
Two old policy comments still describe Oasis as opt-in/trial; actual gate and
project settings above are authoritative. Final APS DLL
15D82EADA04FF9CEFC4FA167B685C1AFDA6D990E7F28BBBE14830BFCEE5C009C.
NativeCore031AE7C670B95F68FF6E2DA05AF0F8473B0D92CEDEED89DA81F8227283C7074C,
CloudV24/default1 and25 protected assets unchanged in final18:07 check.

Next: inhabited Terrestrial and remaining unpublished palettes, then measured
flight publication stalls and remaining near-liquid acceptance. Added optional
generator habitability fixture was NOT run and does not inject biomass; no
inhabited-Terrestrial coverage claim. Larger/elevated-colony access and all-seed
coverage remain open. No ship/character/colony edits. Sprint ACTIVE.

## Previous checkpoint, 1 October 16:54 — bounded walking collision published

Project `aps.WorldScapeFoliage.WalkingCollision=1` now joins StructureExclusion1.
Factory fallback remains0. Only existing admitted APS SurfaceScatter palettes get
the component: nearest32 rocks/lower trunks within40m, refresh5Hz; no grass,
pebble, canopy or HISM-wide physics. Boarding/non-character possession, fast
character travel and hidden/suspended/retired roots release the pool. No promise
of ship-to-rock collision: unloading it for flight is intentional. Console0 is
the reversible OFF path; no baked mesh/material or save-format change.

New `APSFoliageLocomotionProbe.h` and smoke-test glue exercise real grounded
CharacterMovement through AddMovementInput. ONE logged setup teleport beside an
existing natural instance (Frozen~32m; Forest117/132m), then no further teleports,
gravity/velocity/movement-mode overrides, synthetic rocks or forced collision
ticks. A tree initially beyond40m must gain its exact normal proxy after setup.
Inward steering follows normal tangential slide; acceptance requires >1m actual
travel, >=.75s grounded near-zero speed with capsule contact against the exact
instance's proxy, then a three-sided real walking route to the far side. Cameras
and pending diagnostic movement input are released on every cleanup. This is
NOT keyboard mapping, a walk from the colony spawn, navigation/pathfinding, or
ship-dynamics acceptance. Colony/character/ship source was not modified.

Evidence below is under F:/ChatGPT/APOSFERA/work/planet_continuity_20260929:

| Run | Result | Observed scope |
| --- | --- | --- |
| frozen-locomotion-v1, forest-locomotion-v1, frozen-locomotion-selection-v2 (all -20261001) |17/18 each|Selection failures preserved; Frozen natural rocks74-76cm were incorrectly rejected by diagnostic80cm cutoff, not missing ground collision |
| frozen-locomotion-v3-20261001 |17/18|Fixed forward input contacted/slid around rotated rock; no grounded stall, rejected, CSV retained |
| frozen-locomotion-v4-20261001 |18/18|RockB:237.616cm approach, .756s stall,1.844cm/s; far side292.756cm;410/410 grounded frames |
| forest-locomotion-tree-v1-20261001 |18/18|TreeA:221.685cm approach,.752s stall,.728cm/s; far side274.852cm;384/384 grounded frames |
| frozen-locomotion-default-20261001 |18/18|No forced ON:237.642cm approach,.755s stall;far side292.606cm;399/399 grounded |
| forest-locomotion-tree-default-20261001 |18/18|No forced ON:211.090cm approach,.751s stall,0cm/s;far side273.147cm;315/315 grounded |
| forest-walking-collision-default-reentry-20261001 |20/20|No forced ON; both grounded holds, retirement and return pass strict collision/exclusion checks |

18/18 above means16 clean+2 existing warnings; flight20/20 means18+2 warnings.
Both default ground logs and default flight report WalkingCollision1 and
StructureExclusion1 **LastSetBy:SystemSettingsIni**. Reviewed Blocked/Passed PNGs
for both default runs and experimental routes; unchanged rock/tree with the
character on opposite sides. Forest images are dark and foliage partly obscures
the contact, so CSV exact-proxy contact/grounding is complementary evidence,
not a claim of final tree art. Start/Aside/Around also retained under each
Saved/Screenshots/Windows/APS_FoliageWalk_*.png; per-frame FoliageLocomotion.csv.

Default reentry CSV3420 frames:787 ground-hold frames ALL3 active/3 allocated
proxies,165 exclusions,0 pending;438 settled-orbit frames ALL0 visuals/proxies/
allocation/exclusion records. Peak545 visuals. Two coordinate-controlled cycles,
not ship-control acceptance. Ground median10.007702ms,p9912.531299,max15.665602,
0 frames>50ms; full route median10.384198,p9966.275798,max94.134998,64 frames>50ms.
No new speedup or hitch-free claim; earlier ON/OFF costs below remain relevant.

Final APS DLL BE700C0BF0FF7256388599C0D184D9B89EE4CA43FEDE56C0A5D1FDD26D54D012.
NativeCore031AE7C670B95F68FF6E2DA05AF0F8473B0D92CEDEED89DA81F8227283C7074C,
CloudV24/default1 and25 protected assets unchanged. Build-v1 failed on a missing
direct include in WalkingRenderedProbe; fixed header self-containment. v2-v6
builds successful; v6 final24.17s. No native/material bake or promotion.

Next: expand still-unpublished palettes (especially inhabited terrestrial/Oasis
and remaining families), larger/elevated-colony access contract, and native
flight publication stalls. Current two locomotion families do NOT prove all
seeds, all obstacle variants, arbitrary layouts or frame budgets. Sprint ACTIVE.

## Previous checkpoint, 1 October 16:10 — structure clearance published

Project default `aps.WorldScapeFoliage.StructureExclusion=1` is now enabled.
Factory fallback remains0; `WalkingCollision=0` and SurfaceScatter2 unchanged.
CloudV24/default1, all25 protected terrain/liquid assets and native Core unchanged.
No colony, placement, character, ship, atmosphere or material implementation edits.

New root-owned APSFoliageExclusionComponent runs after native root publication,
before local walking proxies. It reads existing Base/LandingPad/Colony.Module
tags, AColony and body attachments plus VisualLocalBounds. Only owned scatter
meshes are filtered; invisible interaction/gravity bounds are not footprints.
Two-metre footprint clearance,5m below visible foundation, nearest-base-to-pad
6m-wide corridor with1km maximum reach; at most256 volumes and2048 examined
instances/tick. Footprint discovery at2Hz; unchanged sectors skip classification.
These are bounded prototype assumptions, NOT navigation/pathfinding acceptance
or coverage of arbitrary elevated structures/very large colonies.

Native classic per-sector HISM is published once and destroyed on retirement;
APS FCP is disabled. Filtering uses batched RemoveInstances, never changes native
worker arrays, seed, terrain, component ownership or native regeneration state.
Removed transforms are kept component-local behind weak component identities;
demolition/live OFF restores their positions, rebase retains them, sector
retirement releases records, returning sectors are filtered again. Unexpected
external count changes invalidate saved records instead of replaying stale data.
No zero-scale placeholders or full-planet foliage regeneration.

Evidence root: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929.

| Run suffix (family-prefix directories) | Result | Evidence boundary |
| --- | --- | --- |
| Forest structure-exclusion-ground-v1-20261001 |17/18,1FAIL|165 suppressed/0 remaining; old contact assertion failed because clearing removed nearby targets; retained |
| Forest/Frozen structure-exclusion-ground-v2-20261001 |18/18 each|165/161 excluded,0 remaining/pending; natural settled frames reviewed |
| Forest structure-exclusion-on-v1-20261001 |20/20|3123 frames; all700 ground-hold frames have165 excluded/3 proxies;424 orbital frames have0 records/0 allocated proxies |
| Forest structure-exclusion-off-v1-20261001 |20/20|3295 frames,0 exclusions,710 peak visuals versus545 ON |
| Forest structure-exclusion-on-repeat-v1-20261001 |20/20|3263 frames, both approaches filtered, both departures release records |
| Forest structure-exclusion-default-20261001 |18/18|Query-only CVar reports1 from SystemSettingsIni,165 excluded/0 remaining/pending; final frame reviewed |

Successful suites have2 pre-existing warning passes,0 failures/notRun.
Explicit exclusion mode requires natural removed objects, zero remaining
footprint overlaps and exact active-proxy matches. It does NOT require positive
capsule contact on a newly cleared pad. Ordinary contact mode still requires
positive contacts, unchanged; no walking acceptance inferred from the new mode.
CPU tests additionally exercise rotated/nonuniform bounds, mesh-edge overlap,
other-body/unowned rejection, body rebase, demolition, OFF restore, component
retirement and return. No test stages obstacles in the rendered scene.

Before/after natural Forest frames:
`forest-collision-ground-contact-v2-20261001/Saved/Screenshots/Windows/APS_SurfaceLighting_11_Settled.png`
and `forest-structure-exclusion-default-20261001/Saved/Screenshots/Windows/APS_SurfaceLighting_11_Settled.png`.
The protruding ramp-edge rock is gone; distant tree, terrain/sky and structures
remain. Frozen reviewed frame: analogous ground-v2 run, `_14_Settled.png`.

Timing is ON/OFF/ON-repeat, one DLL, no route PNG readback, explicit collision1
in all legs. Quantiles floor((N-1)*q); single ordered triple is not a universal
budget or causal attribution for every spike. First ON first-hold slowdown was
not repeated; do not omit the initial result.

| Milliseconds | ON | OFF | ON repeat |
| --- | ---: | ---: | ---: |
| Full median |11.504|10.577|10.822|
| Full p95/p99 |20.233/69.676|19.140/70.608|18.479/69.577|
| Worst / frames above50ms |96.617/63|82.918/67|113.008/66|
| Ground median/p99 |10.671/19.103|9.995/12.298|10.206/13.518|
| Ground mean Game/GPU |4.880/5.193|4.451/5.019|4.525/5.096|

All ground holds have0 frames above50ms. Modest steady overhead remains; global
flight hitches are NOT fixed. Root/component budgets and native streaming remain.
Builds28.09s(9 actions),23.70s(4 actions); final DLL
`2DC7F9CCA052519E50F6941284C6B198723227E6AE710A164FFB13D1D532411A`.
Native Core remains `031AE7C670B95F68FF6E2DA05AF0F8473B0D92CEDEED89DA81F8227283C7074C`.
No native plugin rebuild/install. CloudV24 remains
`12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21`.

Rollback is config/console StructureExclusion0 (restores positions on active
sectors); do not roll back native plugin or accepted sky/materials for this.
Next: actual character walking around natural obstacles and colony access before
publishing WalkingCollision1, then unfinished palettes and flight stalls.
Shared handoff recorded; no acknowledgement from Claude claimed.

## Current checkpoint, 1 October 15:34 — natural collision contacts and corrected reentry route

Production palette/defaults UNCHANGED: SurfaceScatter2, WalkingCollision0.
This pass changes diagnostics and regression tests only. No terrain/sky/cloud,
ship, character, colony or native-plugin source/asset changes.

Evidence base: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929.

| Run directory | Result | What it actually establishes |
| --- | --- | --- |
| forest-collision-on-trial-v1-20261001 |18/18| Old flight suite passed with zero active near proxies; NOT collision acceptance |
| forest-collision-ground-contact-v1-20261001 |15/16,1FAIL|17 exact natural instance/proxy matches and7 real capsule contacts; separate nearest-shape camera ray obstructed, failure retained |
| forest-collision-ground-contact-v2-20261001 |16/16| Dedicated three natural-ground frames +17 exact matches/7 contacts; NOT five-shape detail acceptance |
| frozen-collision-ground-contact-v2-20261001 |16/16|23 exact natural matches/9 settled capsule contacts; three natural frames retained, settled frame reviewed |
| forest-collision-on-strict-v2-20261001 |17/18,1FAIL| Correctly rejects old route: pawn still falling at4000cm/s, collision high-speed gate releases pool |
| forest-collision-landing-on-v3-20261001 |18/18| Two actual grounded holds with18 proxies; both orbital holds free all proxies; reentry restores18 |
| forest-collision-landing-off-v3-20261001 |18/18| Same DLL/route, zero proxies throughout; same710 peak visual instances |

Complete passes have two pre-existing warning passes; no notRun. Failed passes
have one warning pass plus the named diagnostic failure. No failures deleted.

New APSFoliageWalkingRenderedProbe performs world sweeps using the actual pawn
capsule size (34/88cm) against already generated physics, matching each enabled
box to the exact existing mesh instance transform/extent. It does NOT move pawn,
foliage or colony, create a test obstacle, force a collision tick, hide occluders
or disable gravity. Sweep contact is not manual/injected walking acceptance.
The dedicated contact mode ends after the three unchanged natural camera frames;
the ordinary five-shape rendered-detail gate remains intact and separate.

Old flight anchoring kept the pawn about10m above the surface while the camera
paused at2m. CharacterMovement legitimately reached terminal falling speed40m/s,
above the production30m/s proxy cutoff. Only an explicitly requested
WalkingCollision ON/OFF route now positions the real capsule at the local ground
height during the lower holds. Native gravity/movement/collision keep running;
there is no velocity override, movement disable or terrain freeze. Other routes
unchanged. CSV now records pawn speed/grounded state and strict ON requires
nonzero proxies while grounded in BOTH holds. OFF requires zero allocated/active.

ON:3075 total frames;737 lower-hold frames ALL grounded with18 active proxies;
198 orbital-hold frames ALL active0/allocated0. Maximum18 below the32/root cap.
OFF:3082 total frames;706 lower-hold frames ALL grounded, proxies0 throughout.
Both runs retain the same710 peak visual instances. Synthetic lifecycle tests
also cover boarding, high speed, root hide/tick/collision disable, live CVar OFF,
restore, instance removal and ghost-trace rejection. These are not ship dynamics.

Matched DLL: CB12A50DC3C194EEF65E7146F4938CA8B03AD76604C08A954F993AF86450C3D8.
The ON/OFF timing pair has no route PNG readback, same assets/command except the
collision value. One ordered pair cannot demonstrate a speedup or bound worst
hardware/placement cost. Quantiles use floor((N-1)*q).

| Scope (ms) | ON | OFF |
| --- | ---: | ---: |
| Full-route median |11.352|11.428|
| Full-route p95 / p99 |20.505 /72.411|20.514 /73.523|
| Full-route worst /frames over50ms |99.214 /64|105.136 /66|
| Ground-hold median /p99 |10.670 /14.835|11.054 /15.143|
| Ground-hold mean Game /GPU |4.839 /5.329|5.037 /5.358|

No measured regression from18 proxies in this pair; NOT hitch-free acceptance.
Flight hitches remain in both legs. Timing excludes capsule-sweep diagnostics.

Important open placement defect: the Forest settled frame shows a rock
intersecting the colony pad edge. Global WalkingCollision remains0 until colony
footprint/access exclusions and natural walking are validated. Handoff written
to PLANET_EDITOR_WINDOW.md; no acknowledgement from Claude claimed. Existing
APSSpawnPlacementSubsystem has VisualLocalBounds and request RouteTargets, but
no foliage exclusion contract has been connected or modified in this pass.
The earlier +3s proxy gap coincides with logged272m pilot relocation to the
colony and0.273km world rebase. It does not establish a stationary scatter bug.

Builds: natural-contact23.79s, contact-scope20.41s, landing-route23.46s PASS.
Ground v2 DLL A4F799715BB4B0C9C0EAA1DF580DC26E349858FE624176AEC5B540391793AC5C;
final landing DLL above. Installed WorldScapeCore still
031AE7C670B95F68FF6E2DA05AF0F8473B0D92CEDEED89DA81F8227283C7074C.
25 protected assets unchanged; cloudV24 remains
12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21/default1.
Next: coordinate/read the colony exclusion boundary, then natural walking and
safe collision publication; all-solid-family palette/art/coverage still open.

## Previous checkpoint, 1 October 11:16 — mineral Water published and reentry verified

Ordinary SurfaceScatter selection now also includes Water only when HasHabitat
is false. Habitat-positive Water and all other unpublished families remain gated.
Five replaceable mineral meshes: Pebble15.4cm, RockA38.5cm, RockB66.5cm,
SlabA98cm, SlabB133cm; cull100/150/200m, one collection, five types,
64attempts per sector. This is not new vegetation or underwater ecology.
WalkingCollision remains0: CPU lifecycle/shape tests do not establish rendered
walking or collision cost. No terrain, liquid, cloud, atmosphere or ship edit.

Production source: APSPlanetSurfaceScatter.h and its tests. Backup:
F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/water-scatter-publish-prepatch-20261001.
Build build-water-scatter-published-20261001.log PASS,9actions18.04s.
DLL SHA256:427DA43D58B5CF25F7FE74F44BFFD4E7C475EDE8B6AACDC768D2E96E8B160E57.

Evidence under F:/ChatGPT/APOSFERA/work/planet_continuity_20260929:

| Directory | Result | Evidence boundary |
| --- | --- | --- |
| water-scatter-trial-20261001 |16/16| Initial natural rocks visible; Pebble close-up occluded by pawn, not accepted as all-five visual proof |
| water-scatter-clear-trial-20261001 |15/16,1FAIL| Correct clear-ray rejection: selected instance inside pawn bounds; failure retained |
| water-scatter-clear-v2-trial-20261001 |16/16| All five existing natural-LOD shapes inspected; no scene objects moved or hidden |
| water-scatter-reentry-trial-20261001 |18/18| Explicit candidate, two100km departures/reentries; matching ground frames reviewed; PNG overhead contaminates timing |
| water-scatter-published-ground-20261001 |16/16| No foliage CVar override; natural ground and all five existing-shape close-ups inspected |
| water-scatter-published-reentry-20261001 |18/18| No foliage CVar override, no route PNG readback; near population, orbital retirement, repopulation and second retirement PASS |

All complete passes above have two warning passes, zero failures/notRun.
Fixture: Water seed424242/radius6371km, biomass/biodiversity0; NOT Rio's exact
saved Water487132/radius639.1442km. One low-mineral biome is not all-Water proof.
The diagnostic selects existing instances outside pawn bounds and clear rays
for every form (not just trees), retaining terrain clearance and shape coverage.
It does not change production placement, colony visibility or collision.

Final default reentry:3333frames/42s, peak655instances/138components, unsafe0;
100km instances/components/workers0; second near hold628/132 then0/0 again.
dt median10.7309ms,p9517.2094ms,p9960.1737ms,max91.3186ms,53frames>50ms;
meanGPU5.3932ms. No paired OFF run: these are total scene timings, not isolated
foliage cost. Scripted coordinate route, NOT physical ship dynamics or hitch-free
acceptance. Process RAM/caches need not return to the OS on component retirement.
Initial candidate ground +3s transient disappearance was observed and is not
explained/fixed by this publication. Minimum spacing, colony exclusion, final
materials, habitat Water, other family coverage and walking collisions remain open.

Eight protected Shared terrain/liquid assets unchanged. CloudV24 hash remains
12F4F889E43B319B95112854287AC23359ED17C4ED54A00FAE7618E19998DC21.
Current project cloud default is1; the V13/default0 statement in the historical
September checkpoint below is superseded. Cloud ground/above/inside proof and
remaining rendering limitations are tracked in the planet continuity epic.

## Previous checkpoint, 30 September 23:32 — first ordinary-default proof and visible DryGrass

ComfyUI20056 was absent at23:17 with RAM17.94GiB/VRAM14.3GiB available;
resumed the already authorized work without stopping any user process.
build-scatter-published-probe-drygrass-v1.log PASS:5actions19.90s.
Test DLL SHA256:98F69CFA91157ACE7C1A1426B86B2147D20864C72B1879A47B2BA9BCB2DB7C42.
This checkpoint supersedes the previous source-only status below.

Evidence under F:/ChatGPT/APOSFERA/work/planet_continuity_20260929:

| Directory | Result | What was actually established |
| --- | --- | --- |
| frozen-published-default-v2 | 12clean+2warning PASS | No foliage CVar override; ordinary V2 binding,583instances/103components; natural ground image has rocks |
| forest-published-default-v2 | 12clean+2warning PASS | No foliage CVar override;1013instances/176components; natural view plus both existing-tree close-ups reviewed |
| oasis-drygrass-opacity-v1 | 12clean+1warning PASS,1FAIL | Explicit candidate; original placement520/69; DryGrass NOW VISIBLE in Shape4 natural-LOD frame; no nearby fifth/tree instance |
| forest-published-default-reentry-v2 | 13clean+3warning PASS | Default mode, near population,100km retirement,repopulation,second retirement; unsafe0 |

The Oasis error remains specifically `No generated foliage instance within 1km`.
It was NOT waived or converted to a pass. The opacity defect is visibly fixed,
but this does not establish the complete five-form Oasis palette. Oasis stays
out of the automatic subset. Its surviving grass is green placeholder art.
Noise inspection explains a possible sparse-tree cause: biological probability
multiplies biomass, thermal/moisture suitability and regional vegetation/water
masks. This is a hypothesis, not measured tree-rejection evidence; do not disable
those gates or plant a staged tree to obtain a passing image.

Default flight:2852frames over42s, peaks710instances/120components, retirement
and return PASS. Median12.704ms,p9520.782ms,p9973.956ms,max98.833ms;
65frames over50ms; mean GPU7.932ms. No matching OFF run on this DLL, so these
are total scene timings, NOT isolated foliage cost or proof of ship performance.
Route is camera/pawn-coordinate motion, not physical ship dynamics. Terrain
streaming hitches remain. Previous paired measurements are a separate revision.

100 protected asset hashes rechecked: unchanged. No bake was needed for the
runtime-only material binding repair; the existing fixed leaf template is used.
Default integration proof still pending for Desert/Volcanic/mineral Tundra and
Terrestrial. Other families, minimum-spacing/colony exclusion and final art open.
CloudV13 remains rejected/default0; complete sprint NOT achieved.

Editor returned23:29, UE/UBT0, last owned PID27616 terminal. Claude acknowledged
and took a~12minute window for UBT, star POINTS bake and M3 ascent. Do not launch
another UE/UBT over his live work; inspect actual process state at handback.

## Previous checkpoint, 30 September 23:17 — CPU corrections prepared, NOT rebuilt/rendered

The new Published launch mode was incomplete: the ground C++ guard still required
trial Enable=1 and Snapshot only recognized Scatter=1. The default flight runner
also forced Scatter=0. Corrected these together: explicit ground Published flag,
strict Enable/Prototype/Scatter=2/0/2, shared UsesScatter selection, actual V2
root-tag requirement, and no foliage override in default flight. Trial isolation
and five-distinct-mesh/instances/shader/budget checks remain required.

PowerShell parse checks PASS for both runners. Seven switch-construction cases
executed from their actual AST statements PASS (no UE launch or scene rendering).
Both launchers correctly stop before launch on newer, unbuilt source. These are
diagnostic checks, NOT compiled C++ or gameplay acceptance.

Oasis DryGrass candidate is now in source: only owned SM_APS_Scatter_DryGrass
slots MI_Grass_Inst_2/MI_Grass_InstLOD_2 use the existing fixed opacity master,
preserving their respective grass and billboard color/normal atlases. No vendor
asset, palette, mesh or terrain is edited. Oasis remains excluded from automatic
selection. Missing local tree/placement-mask issue is still open.

Backup: scatter-published-probe-before-v1. All five changed source/runner files
are newer than the last DLL DFB28D81...; NONE of this checkpoint was compiled or
rendered. Do not attribute prior successful frames to this source revision.
At23:13 free system RAM was2.99GiB despite free VRAM7.5GiB/GPU0%; live ComfyUI
python20056 was not touched. No build, bake or UE was started. Await safe resource
headroom/user's pending reply; do not hold the editor window or reduce safety gates.
Next: one build, Published ground coverage and default flight, then explicit
Oasis dry-grass render. No new speculative cloud/material iterations meanwhile.

## Previous checkpoint, 30 September 23:01 — reviewed subset compiled, default proof pending

Additional sequential trials under F:/ChatGPT/APOSFERA/work/planet_continuity_20260929:

| Evidence directory | Report | Settled instances/components | Frame review |
| --- | --- | --- | --- |
| frozen-scatter-v2-frozen-material-v1 | 14/14 | 583/103 | Natural view and five mineral shapes |
| tundra-scatter-v2-tundra-material-v1 | 14/14 | 629/115 | Five minerals; biomass/biodiversity0, NOT ColdGrass proof |
| desert-scatter-v2-desert-material-v1 | 14/14 | 626/118 | Five warm-tinted mineral shapes |
| volcanic-scatter-v2-volcanic-material-v1 | 14/14 | 622/112 | Natural dark rocks and slabs; pawn partially occludes first two close-ups |
| forest-scatter-v2-tree-clear-v1 | 14/14 | 1013/176 | Grass and BOTH trees visible; low-poly placeholders, not final art |
| oasis-scatter-v2-oasis-material-v1 | 13/14 | 520/69 | Three rocks visible, DryGrass invisible even forced LOD0; fifth instance absent |

Each 14/14 above comprises12 clean and2 warning passes; unsafe flags0.
Tree-only diagnostic camera now selects an unobstructed ray (24 azimuths,
two elevations); no colony, pawn or instance is hidden/moved. Initial compile
failed on unbraced UE_LOG/else; braces fixed in build-scatter-tree-sight-v2.log.
Oasis has biomass0.42, biodiversity0: the earlier statement that it was barren
was incorrect. Its failure is not waived. Bindings alone do not prove instances.

Limited automatic selection was compiled successfully: build-scatter-published-subset-v1.log,
9actions25.87s; DLL DFB28D810BA11269CC57034ACF526F7E23969E996001DC98A53AFDA6ED28B55E.
SurfaceScatter source/DLL default is now2 (reviewed subset), 1 remains explicit
all-family trial, 0 restores sparse V1 selection. Eligible combinations:
Frozen/Desert/Volcanic minerals; Terrestrial/Tundra ONLY without habitat;
Forest ONLY with habitat. Other types/conditions are not silently accepted.
Master-off, authored/manual/preview gates and budgets are unchanged. Source
backup: scatter-rollout-source-before-v1. Extended all-type/habitat gate tests
COMPILE but have not run on this new DLL yet. Earlier test reports use prior DLL.

RunPlanetFoliageGround.ps1 -Published now omits ALL foliage console overrides.
Its six-family final queue was refused BEFORE launching UE at22:55: ComfyUI
python20056 had reappeared, GPU100%, free VRAM0.7GiB/RAM2.4GiB. No process was
stopped, no memory unloaded. User asked asynchronously for next free window.
Therefore automatic selection is installed/compiled, NOT yet default-render
verified. Next: run Published Forest,Frozen,Tundra,Desert,Volcanic,Terrestrial.
Do not conflate the completed opt-in frames with this pending integration proof.
Protected100 asset hashes rechecked23:01: all unchanged. Goal remains active.

Oasis CPU diagnosis: actual DryGrass slots are MI_Grass_Inst_2/MI_Grass_InstLOD_2,
NOT _3. Saved HLSL VertexInterpolator0 (lines3622 onward) multiplies opacity by
10*saturate(1-cameraDistance/(ObjectRadius^1.1*parameter)); the forced-LOD0
frame remains empty. Reuse the existing fixed leaf-opacity master while retaining
the GRASS near/billboard atlases as a next isolated trial, not random LOD/cull
changes. This repair is NOT installed yet. Missing local Oasis tree additionally
requires checking biological placement masks; do not invent biomass or stage a tree.

CloudV13 remains default0 and visually rejected at the horizon. Real ship flight,
minimum spacing/colony exclusion, remaining families and final art remain open.

## Previous checkpoint, 30 September 22:30 — living palette and paired residency

build-scatter-leaf-binding-v1 PASS,4actions12.48s. Runtime override preserves
TreeB tree_Color/tree_Normal and MI_LeafLOD billboard atlas on the existing
fixed leaf master, only for owned scatter. No vendor/saved palette mutation.
forest-scatter-v2-ground-leaf-binding-v1:12 clean+2 warning PASS,0failed,13PNG.
Actual Forest population includes Pebble/RockA/Grass/TreeA/TreeB; settled
1013instances176components, unsafe0. Reviewed natural view, grass,TreeB and
material bindings. TreeA detail view is OCCLUDED by the colony building: do not
count this as close-up acceptance of all five forms. TreeB foliage now renders;
these are sparse placeholders, not finished ecosystem art or minimum-spacing proof.

Same-DLL camera-coordinate residency/cost pair:
forest-scatter-v2-flight-perf-on-v1 / forest-scatter-v2-flight-perf-off-v1.
DLL240EF62C48100B6B5DCADBF9C8E711C9A96A3790434501EC4AD1332A4EA2770D.
Both16/16 PASS (13clean+3warning); no ROI screenshots. ON3196frames, OFF3181.
ON near generation, retirement at100km, reentry population and second retirement
all passed; unsafe0. ON peaks710instances120components; OFF0throughout.

| Metric, entire42s coordinate route | ON | OFF |
| --- | ---: | ---: |
| Median frame ms | 10.936 | 11.304 |
| p95 ms | 18.838 | 18.093 |
| p99 ms | 71.352 | 68.801 |
| Worst ms | 92.646 | 98.026 |
| Frames over50ms | 67 | 65 |
| Mean GPU ms | 6.007 | 5.755 |
| Process RAM peak MiB,1Hz | 11675.910 | 11551.422 |

Near holds t8-12and29-33: ON/OFF median10.510/10.690ms,
p9512.682/13.917ms, GPU5.510/5.344ms,0frames over50ms in either arm.
One serial pair, not a statistically significant speedup or an isolated byte
allocation measurement. Streaming hitches remain in BOTH arms; the ON-minus-OFF
GPU mean is~0.252ms, not a guarantee for arbitrary density/hardware. This is a
camera/pawn-coordinate route, NOT a physical ship-flight benchmark. Default0
retained pending family/art coverage, TreeA unobstructed view, and release gate.

## Previous checkpoint, 30 September 22:20 — mineral material rendered

ScatterMaterial20260930V1:6 NEW packages (one lightweight master, five normal-map
instances), baked by bake-scattermaterial-mineral-local-v1-cache. It replaces
MI_Cliff_5 only on transient owned Scatter1 mineral entries. The old shader
includes planet climate/snow masks with default centre/radius; this is not a
safe small-object material. New shader uses local UV, two texture samples,
non-emissive low-chroma rock tint and a correct normal map for each shape.
100 protected assets unchanged (Shared, catalog,74 scatter assets,vendor rocks).
The first bake crashed before saving because SetMaterialUsage compiled before
texture-reference caching. Fixed UpdateCachedExpressionData ordering; final
build-scatter-material-v1-texture-cache.log PASS,4actions11.02s. No old output
was overwritten. This is not a measured GPU speedup claim.

terrestrial-scatter-v2-ground-mineral-material-v1:13PNG;12 clean+1 warning PASS,
one rendered FAIL solely from the previously reported colony overlap. Reviewed
natural settled and all five existing-instance detail views: white snow-like
appearance reduced to grey stone, five distinct shapes/normal maps retained;
placement remains607instances105components, unsafe flags0. Terrain/material
palette and natural placement were not changed to stage the frames. Biological
rendering, other tint families, flight/reentry cost and default rollout remain.

Read-only scatter-material-source-audit-v2 confirms TreeB MI_Grass_Leaf1 has a
missing Light_Foliage master. Its TextureBaseColor=tree_Color and TextureNormal
=tree_Normal. MI_LeafLOD has the separate tree_Billboard_Color/Normal atlas.
Transient binding repair uses the existing fixed leaf master with each role's
own textures; source compiled/rendered status must be checked in the next run.
No vendor material or mesh is saved by this repair. Scatter stays default0.

## Historical checkpoint, 30 September 21:26 local — resources released, first bake complete

### Rendered follow-up 21:48 — contact corrected, art NOT accepted

terrestrial-scatter-v2-ground-gates produced13PNG. All13 non-rendered tests
passed (one with warnings); rendered test failed on an unrelated logged colony
role1/WorldScapeRoot_0 overlap. It nevertheless captured the natural pawn and
five existing mineral shapes. The generated Terrestrial profile is barren
(biomass/biodiversity0): these are the five MINERAL fallback shapes, NOT proof
of grass/tree visibility. Natural settled frame showed floating rocks. Close
frames showed an overly white/marbled MI_Cliff_5 material; not accepted art.

Cause of levitation verified in installed WorldScape: the Asset branch rotates
Offset but does not multiply it by sampled scale; only Blueprint does so.
The new bounded runtime policy now computes a world-centimetre offset on its
transient entries, covering both scale endpoints and a small ground embed.
Authored and sparse V1 palettes are unchanged. No native rebuild or rebake was
needed. Bounds/scale endpoint regression added. Build6actions15.43s PASS:
F:/ChatGPT/APOSFERA/work/build-scatter-native-offset.log.

terrestrial-scatter-v2-ground-contact:12 clean passes+1 warning pass+1 rendered
failure (the same colony overlap),13PNG. Reviewed settled pawn and five-shape
detail evidence; the large vertical gaps are removed in reviewed frames.
For the corresponding five instances, pivot height above ground changed from
58/295/310/645/620cm to7/19/32/46/65cm, appropriate to differently sized pivots;
these are pivot distances, NOT bottom-contact measurements. Settled snapshot
607instances/105components, collision/shadow/DF safety flags0. Snapshot values
are not peak memory/frame-cost acceptance. During colony arrival a3309km rebase
emptied and rebuilt local foliage; coordinate flight/reentry still needs its
own paired tests. Colony error reported in shared coordination file, not hidden.

Still default0. Next: mineral material (white is rejected), biological five-shape
frames, affected-family coverage, no-capture cost/reentry and eventual rollout.

Rio explicitly authorized UE/bake after ComfyUI ended. The earlier reservation
below is historical, not a current blocker. The staged five-file native seed
update was installed only after all baseline entries matched; installed hashes
match the compiled candidate. Backup and installed-hashes.json remain in
native-scatter-seed-cpu-prepared-v1. Native payload regression: 8/8 PASS in
planet_water_live_20260929/scatter-seed-v2-native-regression. This checks native
payload/sewing/worker contracts, NOT the appearance of seeded scatter.

APS build: 15 actions, 50.75s, build-surface-scatter-v2-installed.log.
New-only bake: bake-surfacescatter-first-v2-released, saved=74 (10 budget meshes,
64 per-type/habitat palettes), zero errors and 13 dependency warnings.
Protected 94 assets have unchanged hashes. Known vendor material/dependency
warnings require actual bound-material review, not blanket dismissal.

First Terrestrial ground run: terrestrial-scatter-v2-ground-first, 11/14 PASS,
3 failed BEFORE frames. New palette unit test passed. Two failures exposed a
real authored-profile regression: SurfaceScatter returned disabled rather than
falling through to existing authored mode-1 admission. The third was a legacy
diagnostic gate requiring Prototype=1, incompatible with the new explicit trial.
Source fixes retain authored behavior and require exactly one trial switch;
regressions are strengthened, not bypassed. Rebuild/rerun is pending the next
editor window. The diagnostic now inspects all five naturally generated shapes
after unchanged pawn views and adds isolated ON/OFF/reentry scatter flight.

New scatter remains default0; no rendered acceptance or cost claim yet.
Claude owns the agreed short UE window; source-only work continues meanwhile.

## Build checkpoint, 30 September 20:30 local — ComfyUI reserved by Rio

Rio explicitly uses ComfyUI and says not to touch it; he will notify when free.
No ComfyUI stop/unload, no UE/bake/GPU runs until that signal. Memory checks
showed about 3.4–3.8GiB free RAM and 3.49GiB free VRAM; the existing bake
headroom guard correctly stopped before creating output assets. This is a
resource reservation, not permission to reduce safeguards or close Python.

Completed since the source checkpoint:
- APS_ALPHAEditor build PASS, 12 actions, 49s total:
  F:/ChatGPT/APOSFERA/work/build-surface-scatter-v2.log.
  New builder, policy and UE tests compile; UE tests have NOT been executed.
- Integrated seed V2 in the MATCHED PRIVATE plugin source only, serial and
  parallel workers; fresh owned V2 roots opt in, older roots retain old streams.
  New helper matches repository SHA256
  C35A67A4ECE236EFBBE7AE7DE3D5B0843AD4AEC07FE95822A12447D6544ECDC1.
- Private WorldScape build PASS, 4 actions, MaxParallelActions=1, 25.57s:
  F:/ChatGPT/APOSFERA/work/build-worldscape-scatter-seed-v2.log.
  Only one C++ translation unit was compiled; no editor or GPU started.
  Private and installed module metadata both retain BuildId33043543.
- Standalone seed CPU regression rerun: 15/15 PASS, failures0.
- Tracked touched-source diff --check passes. Prototype bake process guard
  now tests an explicit process array/count and reports owners; no safety
  check was removed.
- 20:38: prepared a narrowly scoped installation/rollback bundle, NOT installed:
  F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/native-scatter-seed-cpu-prepared-v1.
  PrepareNativeSeedInstall.ps1 performs read-only checks by default; -Stage
  only writes a new evidence bundle and has no installation mode. It compares
  both source inventories, checks the exact two worker additions, helper,
  BuildId and source freshness, then freezes five artifacts (worker/helper/
  Core DLL/PDB/import lib) and old copies. All 210 unrelated sources match.
  Manifest records every installed source hash for later drift protection;
  all five candidate/backup hashes checked, installed targets still unchanged.
  UHT, project binaries, other modules and assets are not in this transaction.
  Parser errors0; seed CPU regression remains15/15. EOF newline normalization
  is the only non-semantic allowance in the exact worker delta comparison.

NOT delivered yet: installed native seed fix (installed Core DLL remains
04:17), candidate palettes/meshes (destination absent), natural ground frames,
all-family placement/art validation, cost/ship reentry, or visible clouds.
SurfaceScatter remains opt-in0 and clouds0. The previous source checkpoint
below is historical; compile completion does not make the candidate live.

Next safe window: revalidate the staged bundle against ALL current baseline
hashes (a prepared bundle is not permission to overwrite later Claude work);
install only the matched native seed delta,
new-only bake, execute compiled UE contracts, inspect all ten mesh bounds and
materials, then rendered placement/cost/reentry evidence before enabling.
Claude's newer 20:21 ship/star/UI edits were not included in this private build.

## Source checkpoint, 30 September 20:08 local — SurfaceScatterV2 NOT published

Rio reports no visible ground filling and no clouds. Requests 3–5 different
meshes per planet, plausible physical sizes/spacing and no repeating pattern.
This supersedes acceptance of a merely sparse technical population. The live
editor PID19116 remains Rio's; no UBT, bake, GPU run or editor closure this pass.
Cloud default0 is unchanged and still an open delivery item.

Implemented in SOURCE (not yet UE-built/baked/rendered):
- APSPlanetSurfaceScatter: five different source geometries per solid type;
  32 supported enums, including legacy/Unknown; no ground filling on giants.
- Ten owned budget-copy recipes: five mineral silhouettes, three grasses,
  two trees. Three LODs; caps 1024 triangles mineral /512 grass /2048 trees.
  New-only SurfaceScatter20260930V2 output, V1/vendor assets not overwritten.
- Separate mineral-only fallback for every type: zero biomass does not mean
  zero rocks, nor does it justify adding life. Surface profile/save not edited.
- Target heights roughly 0.15–2.5m minerals, 0.22–0.45m grass, 3.5–7m trees,
  +/-20% variation, plus explicit horizontal footprint cap (including scale).
  Offset derived from actual bounds; slight rock embed, random rotation.
- Local sectors 80–180m, weighted bounded attempts rather than an entire-planet
  object carpet. One palette/five types/64 total attempts per-sector budget,
  existing global admission and no-collision/no-shadow/HISM controls retained.
  Mixed palettes preserve per-entry biological masks; rocks do not inherit them.
- Explicit aps.WorldScapeFoliage.SurfaceScatter=1 trial only; default0. It
  preserves master-off/manual/authored/preview scopes. Requires native seed
  revision2, otherwise fails closed with a named diagnostic, not a silent fallback.
- Ground runner accepts every supported solid type in SurfaceScatter mode,
  and checks five distinct bound meshes. Natural frames are still required:
  five loaded meshes or a component count alone is NOT visible filling.

Specific repetition cause found in installed WorldScapeRoot_Foliages.cpp:
both serial and parallel worker seeds dot the sector position with
(size +100*collection, 2000*species, 50000*layer). At IDs0/0/0, Y/Z and
the planet Seed have no influence; entire columns can reuse random streams.
Prepared Tools/Diagnostics/WorldScapeFoliageScatter/native-seed-v2.patch plus
APSBoundedFoliageSeed.h. Integer hash includes all planet-local cell coordinates,
planet seed and IDs. Only fresh owned roots tagged APS.SurfaceScatter.V2 opt in;
authored/V1 sequences are preserved. This patch is NOT applied to the installed
plugin. Later integration must copy the header into WorldScapeCore/Public,
apply the checked patch to the matched private plugin source, build matched
plugin/APS binaries offline with backups, and repeat rendered/reentry tests.

Verified this pass:
- Standalone MSVC CPU executable:15/15 seed checks, failures0, including729
  distinct streams in the retained-cell fixture, negative/large cells, seed,
  species/layer/collection identity and invalid-input guards. No UE/GPU used.
  Executable: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/scatter-seed-cpu-v2/SeedTests.exe.
- Native patch git apply --check against installed source passes (read-only).
- Both modified PowerShell runners parse; tracked touched source diff --check passes.
- New UE automation contracts cover palette uniqueness, masks, scopes, source
  immutability and admission budgets. They have NOT been compiled/run yet.

Open before publishing: UE build/bake; actual bounds/materials of all ten copies;
second-tree and grass opacity (do not assume the first-tree leaf fix applies);
natural start view with useful coverage, 3–5 visible object classes over a walk;
slope/water/colony exclusion and distribution across sector boundaries;
all-family art/colour suitability; capture-free ON/OFF cost and real flight
retirement/reentry. Mineral meshes are geological placeholders, NOT finished
crystal/ice/sulfur-specific art. Random stream correction is not a guarantee
of minimum spacing or absence of cross-sector overlaps; assess those separately.

## Current checkpoint, 30 September 11:42 local

Forest repeat ON/OFF both15/15(2warnings each), including runtime-scope safety
tests. ON3465frames median10.214ms p9516.918 p9966.600 max101.889,61>50ms,
GPUmedian5.111ms; OFF3734 median9.685 p9514.990 p9960.207 max81.999,60>50ms,
GPUmedian5.069.22trees peak; RAM11543.4/11579.1MiB. Both near visits populate,
both departures retire completely, unsafe0. Sparse trees cost about0.53ms
median in this single pair; the numerous existing terrain hitches remain in
the OFF baseline. Do NOT call physical-ship performance complete.
Shared8 hashes unchanged for both Frozen and Forest ON/OFF pairs.

Runtime Enable now DEFAULT2 (11:40 source; build-foliage-default-v1 PASS,
8actions24.21s). Only generated Frozen/Forest empty profiles auto-receive the
already rendered sparse prototypes. Mode0 master-off remains available;
mode1 retains explicit prior authored/prototype behavior. Manual planets,
moons, scaled menu roots and authored collections are NOT auto-promoted.
No source palette/mesh saved in this rollout. Existing bounded allocations,
100m cull and no-shadow/no-collision flags retained. Recreate root/restartPIE.
Default runs with no foliage CVar override both passed15/15(2warnings each):
frozen-foliage-default-return-v1 / forest-foliage-default-return-v1.
Each uses -FoliageDefault -FoliageReentry (42s, PNG route, NOT a timing pair).
Both report near population, retirement, return population, second retirement,
unsafe0. Reviewed020/040/122/155 for each: terrain near/return unchanged;
objects are sparse and not prominent in this downward camera.020 is partly
occluded by colony geometry, NOT proof of a terrain defect or successful art.
Close object/leaf appearance is supported by the prior existing-instance detail
and leaf distance trials; this rollout changes eligibility only, not those assets.
Do not present sparse technical populations as final filled-world art.
WorldScape shared assets8 and own Forest collection hash remain unchanged.

## Previous checkpoint, 30 September 11:35 local

Frozen repeated ON/OFF route (42s, two ground visits, captures-free) passed5/5
each incl1warning. ON3964frames median9.744ms p9515.526 p9922.946 max82.808,
2frames>50ms, GPUmedian5.053ms; OFF4020 median9.728 p9514.754 p9921.567
max69.356,3frames>50ms,GPUmedian5.037. Peak79rocks, processRAM11150.1/11131.6MiB.
Native objects retire at departure and repopulate on return, then retire again;
unsafe0, far HISM/instances/workers0. One controlled pair, NOT VRAM/ship-speed
acceptance or a statistical perf win. Paths under planet_continuity_20260929:
frozen-foliage-return-on-v1 / frozen-foliage-return-off-v1.

Prepared runtime mode2 on aps.WorldScapeFoliage.Enable (still default0): only
Frozen/Forest with empty disabled foliage profiles, never manual or scaled
preview bodies, never authored enabled/disabled collections. Existing mode1
retains explicit profile/prototype opt-in; mode0 remains master-off. Habitat,
world reservations, transient collection safety and source assets unchanged.
Scope contracts cover every enum, manual/preview/master-off/invalid-mode,
authored collections and no-biomass Forest rejection. Built11:34,8actions23.01s.
Forest repeated ON/OFF pair and default activation still pending.

## Previous checkpoint, 30 September 09:55 local

leaf-prototype-install-v3 SAVED owned Forest collection via OverrideMaterial
[existing bark, owned leaf]. Mesh was NOT saved; its SHA256 remains original.
Backup: work/planet_continuity_20260929/leaf-prototype-install-v3/forest-before.uasset.
v1/v2 mesh-binding attempts both refused before save. v2 numeric logging proves
SetMaterial rebuild also changes numerical mesh bounds; not merely repr noise.
The collection-level override avoids this mesh rebuild entirely.

forest-leaf-installed-normal-lod-v2:12/12(10success+2warnings),5PNG, viewed
natural-player Settled and ExistingInstanceDetail. Existing detail has crown
with automatic LOD; native slot1 resolves owned leaf.30instances/30HISM,
unsafe0. Natural pawn view has no prominent tree; this is a sparse technical
prototype, not accepted density/art. Prior misleadingly named normal-lod-v1
was unmodified vendor control because installation had failed.

Forest flight same-DLL ON/OFF passed4/4 each (one warning), capture-free:
ON1839frames median9.736ms p95 15.844ms p99 61.082ms max74.947ms,29frames>50ms;
OFF1778frames median10.056ms p95 15.709ms p99 61.027ms max78.301ms,32frames>50ms.
ON max22instances, processRAMpeak11570.7MiB; OFF0instances,11476.0MiB.
ON reaches ground population and retires all HISM/instances/workers by ascent
4.13km, stays0 through100km. Unsafe flags0. One pair only; no statistically
proven speed gain, no VRAM measure, no real ship dynamics acceptance. Existing
terrain hitches remain in BOTH runs. Global foliage stillOFF; Frozen pair and
ship-speed/reentry lifecycle remain required before broader activation.

## Previous checkpoint, 30 September 09:37 local

forest-leaf-owned-clear-ray-v3:12/12(10success+2warnings),12PNG, reviewed7:
near,80,87.5,95,110m,near-return,vendor-return. One unobstructed ray selected
on first attempt; no scene geometry hidden. Crown retained near/return, thins
by95m, absent110m. Vendor return restores bare trunk. This accepts the bounded
leaf opacity fix, NOT final art, whole-frame lighting or flight performance.

leaf-prototype-install-v1 FAILED before save: post-bind invariant compared
string representations of Python bounds structs. Exact original mesh SHA256
8B5C305BEF7218DBCAA03CFB486963C39D6EA4B70BE806C5CE5BA5BEEED469DB
confirmed unchanged09:36. Backup tree-before.uasset retained. Installer now
compares numeric bounds and logs full before/after on failure; repeat required.
forest-leaf-installed-normal-lod-v1 ran accidentally after install refusal;
it therefore tests the UNCHANGED VENDOR LEAF and cannot accept installation.
Global foliage remains OFF. Flight ON/OFF probe compiled but not run.

## Previous checkpoint, 30 September 09:22 local

Owned leaf bake is DONE: bake-foliageleaf-v2 saved3 new packages, bound0,
0errors/9existing warnings; Shared8/catalog1/vendorLeaf3 hashes unchanged.
The first bake failed before saves (function input OutputIndex=-1); explicit0
fixed it. No vendor or accepted material was overwritten.

forest-leaf-owned-distance-v2 passed12/12 (10success+2warnings),12PNG.
Reviewed7/12: owned near,80,87.5,95,110m,owned near return,vendor return.
Near and return clearly retain the crown; vendor return removes it. Far frames
are OCCLUDED BY COLONY GEOMETRY and do not prove cull/fade behaviour. Do not
mistake the passing structural capture test for rendered distance acceptance.
Probe now selects one visibility-traced clear camera ray for all distances,
never hides/moves the colony, terrain or foliage. Repeat still required.
The new candidate warms asynchronously in the diagnostic readiness phase.

InstallPlanetPrototypeLeaf.py is prepared, NOT run: slot1 of the owned budget
tree only, retaining3LODs/3material slots/2sections perLOD, bark and unused slot.
Back up the exact tree before running; original SHA256 below. No default enable.

Dedicated published-terrain FoliageFlight route is SOURCE ONLY: Forest/Frozen,
100km->near ground hold4s->100km; captures-free performance mode and separate
OFF baseline. Records frame times, GT HISM/instance counts, worker status and
process RAM sampled1Hz; asserts near generation then far retirement. This is a
coordinate-driven streaming probe, NOT ship dynamics or a measured FPS result.
Runner: RunPlanetFieldsFlight.ps1 -Isolation Published -Family Forest|Frozen
-GroundHold -FoliageFlight [-FoliageOff] -Performance -Label <fresh>.
Global prototype gates remain OFF until rendered + streaming cost acceptance.

## Previous checkpoint, 30 September 08:45 local

Read-only saved graph + translated HLSL identify an additional vendor leaf mask:
camera distance / (ObjectRadius^1.1 * FadeDistance). MI_Grass_Leaf uses0.852.
forest-leaf-fade-isolation-v1:11/11(9success+2warnings),7PNG; reviewed2/7
LeafFade20_DIAGNOSTIC and LeafFadeOriginal_RETURN. Same camera, existing tree,
LOD0 and geometry:0.852=>bare trunk,20=>visible crown,return0.852=>bare trunk.
This confirms the radius-dependent opacity gate; it is not missing mesh triangles.
Protected Shared8+palette1 unchanged. No vendor packages saved.

APSFoliageLeafMaterialBuilder now creates a separate master+parent+leaf MIC,
preserving inheritance, colours, normal art and alpha cutout. Removes the hidden
radius-dependent gate, adds camera-relative pixel opacity fade80-95m ahead of
the existing100m HISM hard-cull. Three NEW outputs only; no mesh/palette bind.
Prepared distance trial:near,80,87.5,95,110m,near-return,vendor-return on ONE
existing component; changes camera/material only and restores on cleanup.
Builder not built/baked yet; ordinary foliage remainsOFF. Far fade, normal LOD,
ground flight residency/peak cost still unaccepted. Do not enable on scalar tests.

## Previous checkpoint, 30 September 08:18 local

forest-foliage-lod-detail-v1:9/9 tests(7success+2successWithWarnings),5PNG,
reviewed natural-detail and forcedLOD0 pair2/5. Both show a bare trunk.
LOD0 has2048tris:50bark(slot0),1998leaves(slot1 MI_Grass_Leaf); LOD1 has
35+989,LOD2 has16+240. The crown is not absent due to missing leaf triangles.
The single-component forcedLOD and camera are restored on cleanup. Protected
Shared8+palette1 unchanged. Ordinary foliage remainsOFF. Read-only leaf/water
graph export prepared; investigate actual opacity/WPO/material before replacing
meshes or increasing budgets. No accepted foliage art or flight-cost claim.

## Previous checkpoint, 30 September 07:58 local

foliage-detail-ground-v1: Frozen1/1 and Forest9/9(existing8policy contracts +
rendered test), both with one warning result. FourPNG each, reviewed existing
instance detail1/4 each. ProtectedShared8 + prototype palette1 unchanged each.
Frozen closest rock46.70m from pawn, anchor height error-0.001cm, visibly meets
snow at its base. Forest nearest tree78.01m, anchor height error+0.003cm, but
renders as bare trunk/branches: NOT accepted as working tree foliage. These
are close camera inspections of existing instances, not natural framing or new
instances. Natural-pawn screenshots remain available and unchanged in order.

Build-foliage-detail-drain-v1 passed8actions/20.14s. The same build adds a
non-blocking IsDone fence on the native foliage task before root unload/replace,
preventing entry into native EndPlay's worker join while that task is pending.
Root/queues remain owned until completion. Trials clean up successfully; this
is not measured ship-flight retirement/performance acceptance.

Next source-only diagnostic now prepared: report LOD section triangle/material
counts; capture natural detail, then forcedLOD0 for ONE existing component and
restore it on every cleanup path. This separates simplification loss from
missing leaf material. It is NOT built/run yet; prototype remains defaultOFF.

## Previous checkpoint, 30 September 07:34 local

Async warmup on the actual HISM materials/PSOs resolves the deferred shader-map
failure. Frozen and Forest foliage-warmup-ground-v2 each passed1/1 structural
test(with warning),3PNG per run. Both retain ordinary sky/lighting and natural
pawn camera; no spawned test instances or habitat overrides. Shared8packages
unchanged in each run. Frozen settled127instances/45HISM/45sectors; Forest
starts23trees/23HISM and settles30/30. All inspected shader maps complete,
unsafe flags0.

Rendered acceptance is still incomplete. Reviewed Frozen First/Settled2/3PNG:
one small grey stone visible on the right; Forest Settled1/3PNG has no clearly
visible tree. Instance counts are NOT visible placement acceptance. A bounded
post-natural-frame camera inspection of the nearest EXISTING instance is now
in source; it restores the player view and never moves/spawns instances/pawn.
The corrected base-sphere height calculation still needs the next build.

No production activation: sparse placement/ground contact, cull transitions and
high-speed retirement/performance must pass first. No measured peak RAM/FPS
claim from the warmup snapshots. Evidence roots are
F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/frozen-foliage-warmup-ground-v2/
and forest-foliage-warmup-ground-v2/.

## Previous checkpoint, 29 September 08:34 local

Natural-ground trials now ran, not accepted: Frozen59 stones/21HISM and Forest23
trees/23HISM, safe sampled collision/shadow/DF/cull flags, but incomplete shader
maps at +8s. Terrestrial seed424242 has biomass0/biodiversity0 and correctly
does not activate biological foliage. Tests FAIL and prototype remains OFF.
See `2026-09-29-fields-matrix-and-ground-prototype.md` for exact runs and limits.

## Historical bake checkpoint, 29 September 08:10 local

Bake SUCCESS in bake-foliage-reduced-tree-v2, DLL DE7CBA42, 07:59:13-07:59:30:
32 new collections (31 selectable supported types plus legacy Unknown), one new
owned tree. Actual tree LOD triangles 2048 / 1024 / 256; grass LOD0 633 (four LODs),
small-rock LOD0 2360 (one LOD). No limits raised. Tree SHA256:
8B5C305BEF7218DBCAA03CFB486963C39D6EA4B70BE806C5CE5BA5BEEED469DB.
Eight Shared assets, catalog and all five vendor mesh hashes unchanged.
Output directory now EXISTS: Diagnostics/FoliagePrototype20260929V1.
Do not rerun the no-overwrite baker into this directory.

Both runtime switches remain OFF; no rendered placement/residency trial yet.
Commandlet exit 0, zero errors/nine warnings. Warnings include WorldScape's
missing WorldScape128/default spawner texture, AtmoScape legacy starfield texture
and fallback, duplicate Python FlightRange, and TriTexture's missing default
/Game/HQ_ResidentialHouse/Textures/Ground/Grass_d. Non-null material slots are
not shader/render acceptance. Inspect actual prototype grass/tree/rock in the
world before any production activation. Next step is the bounded fresh-root
opt-in placement/ground-contact/cull/residency trial, not another palette bake.

## Historical failed bake and preparation

These are diagnostic collections, not enabled production content. Current 07:20:
the first real bake (`bake-foliage-v1`, DLL A0F72033) failed safely BEFORE saving.
The original tree has 46,223 LOD0 triangles and exceeds the 8,192 cap. Five other
meshes loaded with valid render data (rocks 2,360/2,774 triangles; three grasses
633 each). Rock_1 also imports a missing experimental material, so it is excluded
from the next set. Shared/catalog hashes unchanged, no destination directory.

New builder-only source creates an OWNED tree copy with three absolute LOD caps
2048/1024/256, disables its Nanite/distance-field generation and checks actual
built triangle counts before saving anything. Original mesh/materials are not
written. Outcrops temporarily use the intact 2,360-triangle small rock at the
recipe's normalized size. Added explicit missing-material/mesh-metric errors and
vendor-mesh hash snapshots to the bake launcher. This revision is NOT yet linked,
baked or placed in a rendered world. The FINAL source syntax check passed at
07:21 (MSVC /Zs, exit 0). Use Engine/Source as its working directory because
UBT's shared response file has relative include paths; the earlier attempt from
the project directory failed at CoreMinimal.h before checking this header.
The installed 07:17:41 DLL predates the final 07:18:13 reduction-trigger tweak.
Do not bake with that DLL; include the header in the next coordinated build.

## Prepared assets

`APSPlanetFoliagePrototypeBuilder` visits every currently WorldScape-supported
planet type (including legacy types); unsupported giants are skipped. Each gets
its own replaceable `FC_APS_Proto_<Type>` in `Diagnostics/FoliagePrototype20260929V1`.
The builder refuses existing packages and verifies all inputs before saving new
packages. The five current source mesh paths were resolved by the actual bake;
the newly reduced tree and saved collections still need their bake check.

First sparse recipes:

- Terrestrial, Pangea, SuperEarth: temperate grass, about 35cm, 12 attempts/sector.
- Nordic, Tundra: cold grass, 25cm, 8 attempts/sector, biological habitat required.
- Forest: test trees, 4.5m, 2 attempts/sector.
- Oasis, Savanna: dry/hot grass, 40cm, 8 attempts/sector.
- Ocean, Water, Archipelago: small exposed-shore rocks, 45cm, 4 attempts/sector.
- HighMountain and volcanic types: outcrops, 1.3m, 3 attempts/sector.
- Frozen/ice/rogue: temporary outcrop silhouettes, 1m, 3 attempts/sector.
- Crystal: temporary 1.8m outcrop silhouette, 2 attempts/sector.
- Other supported types: sparse 60cm mineral placeholders, 4 attempts/sector.

Ice/crystal entries are explicitly ROCK stand-ins, not claims of finished
ice/crystal art. No broad generic vegetation is assigned to barren worlds.
This first set establishes placement/size/residency before denser mixed palettes.

## Resource and habitat boundaries

One mesh type/collection, 200m sectors, 0.5 cull multiplier, no collision, shadows,
actor spawning, dedicated-server generation or distance-field lighting. Passes
through the existing HISM sanitization policy before serialization. Original
assets are read-only. Meshes above 8192 LOD0 triangles or four material slots are
rejected, not silently accepted. Height is normalized from actual mesh bounds.
Rendered ground contact/LOD/cull transitions are still required.

The installed activation gate provides the explicit default-off `aps.WorldScapeFoliage.Prototype`
key, also requiring `aps.WorldScapeFoliage.Enable=1`. It supplies a recipe only
when the world has no authored collections and foliage is disabled in its profile.
Configured authored palettes always retain their own rules. Geological prototype
recipes need no biomass and disable the biological noise mask; biological ones
retain the normal habitat/density gate. No Biomass, humidity, profile signature,
UPROPERTY/save schema or catalog value is changed. The selection is an in-memory
activation plan on a fresh gameplay root, subject to the same world/root budgets.

`PrototypePaletteGates` covers the two keys, preview/giant veto, habitat rules,
all supported type paths, authored palette preservation and unchanged input
profile/signature. This test and the previous seven all PASSED on A0F72033 in
`volcanic-fields-v3-magma-family-v2`. Actual prototype placement, residency and
performance were still unverified at that checkpoint. See the current bake
checkpoint above; generated collections are still NOT enabled by default.

## Next bounded verification

1. Include the latest builder in the next coordinated Editor build (max2).
   A source syntax pass alone does not create assets.
2. P0 Magma bake and Volcanic family-scale A/B are now done; ordinary-atmosphere
   material continuity and other families retain GPU priority.
3. `RunPlanetPrototypeBake.ps1 -Candidate Foliage -Label <fresh>` creates only new
   assets; verify the actual owned-tree triangle counts, materials, all saved
   collections/dependencies, unchanged vendor meshes and catalog hash.
4. Fresh-root opt-in trials, exact player-side ground placement, then traversal,
   world changes and measured component/instance/memory/frame-time peaks.
   The flight launcher has an explicit `-FoliagePrototype` option after the palette
   bake, for Frozen/Terrestrial alongside `-DefaultAtmosphere`. Missing palettes
   reject the run before UE starts. This option is preparation, not acceptance.

The launchers reject stale known bake sources and inadequate RAM/commit/VRAM.
Current ComfyUI workload is never stopped by these scripts. No source or
automation result is treated as rendered visual acceptance.

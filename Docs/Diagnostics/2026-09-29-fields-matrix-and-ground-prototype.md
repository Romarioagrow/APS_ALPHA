# Fields matrix and first natural-ground foliage trials

## Checkpoint 09:52 local, 29 September

Goal ACTIVE. These are bounded diagnostic results, not production activation or
completion of P0. No Shared material/catalog/vendor mesh was replaced in this pass.

## Five additional terrain families

Run: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/matrix-fields-v3-remaining-archetypes-v1`.
DLL SHA256 A381DC3AE7B115A926CD79A94503622A45D7D94A7C4072DA2FC41F0D6BC84110.
Ammonia, Desert, Forest, Ocean, Metallic: 12 PNGs each, 60 GPU rows;
five family tests and three contracts pass (8/8). Eight Shared hashes unchanged.

Visually inspected both variants at views00 (whole,20000km) and03 (close,2000km)
for all five types: **20 of 60 frames**. The other40 are captured, not yet inspected.
Fine woven/grid repetition is reduced most clearly on Metallic and Desert;
large regional shapes and liquid layout remain. Forest/Ocean difference is modest.
Ammonia's faceted shore, flat ocean colour and broad blurry fields are unresolved.
Do not infer continuous movement or full-family artistic acceptance from this set.

Mean of six view GPU means, native -> candidate (ms), approximately:
Ammonia3.18->3.29; Desert3.21->3.39; Forest3.24->3.39;
Ocean3.08->3.10; Metallic3.24->3.39. Asynchronous counters, fixed menu views:
not gameplay FPS, frame-time tails, or user's full-resolution performance.

Ammonia's previous exact-frame failure was a test error, not a planet-scale fix.
The fixture copied native double uniforms, then redundantly recomputed inverse:
270.19626159845501 became270.19626159845495 (delta-5.6843418860808015e-14).
Removed only that recomputation. The guard now checks five double vectors and
all six high/low detail rows exactly; tests reject altered scale and residual.
No tolerance relaxation or production transform change.

## Foliage: natural spawn, no artificial instances or habitat override

Runs in the same root: terrestrial-foliage-ground-v1; frozen-foliage-ground-v2;
frozen-foliage-ground-v3; forest-foliage-ground-v3; terrestrial-foliage-ground-v3.
Every run uses fresh opt-in CVars only, ordinary atmosphere and native materials.
v1 DLL195EA5F7; exact later DLL hashes are in each run's dll.json.
Actual viewport1280x722, controlled character naturally grounded on WorldScape.

- Terrestrial seed424242 resolves biomass0, biodiversity0, humidity0.525747.
  Profile is disabled/empty and biological prototype plan correctly stays OFF.
  Zero instances; explicit placement test FAIL. Do not change the biosphere to
  turn this empty-world test green. A naturally viable terrestrial seed or a
  separately labelled geological palette is needed for future grass evidence.
- Frozen:59 actual instances in21 HISM components/retained sectors, one queue
  type. One reserved root. All sampled snapshots stable. Cull end10000cm,
  collision/shadows/DF/never-cull all OFF. Inspected +3s frame: sparse small
  rocks, no foliage visual-quality acceptance. Vendor MI_Cliff_5 reports zero
  compile errors but incomplete shader map at first/+3/+8s; final test FAIL.
- Forest:23 actual tree instances in23 HISM components/sectors, one queue type,
  one reserved root. Biomass0.94, biodiversity0.78, humidity0.86; density0.870859.
  Uses owned reduced2048/1024/256 tree, not the original46223-triangle asset.
  Same safe component flags/cull100m. All three material slots report incomplete
  maps at +8s, zero compile errors; final test FAIL. Inspected +3s frame is too
  dark and shows no clear nearby tree: counts alone do not demonstrate appearance.

First-frame pending shaders are recorded, final completion is mandatory. The
probe does not force full shader compilation, so distinguish deferred shader-map
population from a real compile failure before changing material assets. Keep
the failing result; do not claim valid materials from non-null slots.

Process RAM snapshots: Frozen11117->10930MiB; Forest11571->11243MiB.
These include the whole editor/world, not foliage cost, delta, peak or VRAM.
No traversal/eviction stress or production FPS acceptance yet. Prototype remains
globally OFF. No new collision, save-schema, biome or vendor-content mutation.

## Back to P0: distance-gate isolation

Ordinary-atmosphere flights still show repeated texture at intermediate height.
Production macro V2 retains exact legacy below5km, finishing replacement at50km.
New separate MacroApproach20260929V1 changes ONLY that fade interval to0.5..5km;
same three fields, means/variance, domains, texture count, masks and geometry.
Exact shader/three-gate contract is checked before writing a NEW package.
Bake completed08:34, zero errors/nine warnings; three NEW diagnostic assets only.
Run: `F:/ChatGPT/APOSFERA/work/planet_macro_20260928/terrestrial-approach-3km-v1`.
Two tests pass, twelve orbit frames and three initial ground frames. Inspected
native/mean/aperiodic in all three views (9/12 orbit). The main repeated pattern
remains: candidate rejected as the solution, no production publication. The100m
acceptance run was skipped because this candidate already failed its purpose.
This run used the launcher's legacy stress atmosphere, NOT production atmosphere.
Fixed-camera GPU means native/candidate: nadir4.219/4.283ms, oblique4.424/4.471ms,
limb4.166/4.151ms. Not production FPS. Pixel comparison native/control p99<=2;
native/candidate MAE4.37/4.51/1.58 verifies visible application, not improvement.

## Gameplay native channels at3km

Run: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/terrestrial-gameplay-buffers-3km-v1`.
DLL SHA256 A6A46C8CF1B4BD5591B998D4DE81BD93CC2C43387F0708F4442031B65576ABF6.
Finished08:45;1/1 test;15 orbit channel frames at1280x722 plus3 ground frames.
ALL15 orbit frames inspected. Three views each: lit, BaseColor, WorldNormal,
Roughness, restored-lit. Native bound material, unchanged mesh payload and fixed
camera; ordinary production-CDO atmosphere. Shared eight hashes unchanged.

At3km the dominant woven/directional texture is stronger in lit than BaseColor;
WorldNormal has corresponding small-scale variation, roughness is nearly uniform
on land. All roughness captures are valid grayscale here. Native/restored images
retain the same structure. This implicates normal/lighting paths for THIS midrange
symptom, not the distant Frozen BaseColor grid, and does not yet distinguish mesh
normals from normal textures. It does not prove all terrain holes are shading.

## Normal source isolation at3km: rendered causal result

Run: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/terrestrial-normal-sources-3km-v1`.
DLL SHA256 3CC552E56806C654035A93A095A6C0F7EA37876D7FA738115BB9B4FD0B21B233.
Finished08:56:56;1/1 test;18/18 orbit frames inspected in three views. Native,
flat three macro normals, flat all seven normals, radial mesh normals only,
flat normals+radial mesh, restored native. Ordinary atmosphere, same camera,
no position/index/UV/colour/tangent changes. Non-texture uniforms are guarded;
original material and full mesh payload are restored. Eight Shared hashes intact.

The strong repeated small ridges disappear when ONLY SlopeNormal, SlopeNormal3,
MacroSlopeNormal are replaced with the engine flat normal. Flattening all seven
has virtually the same appearance at3km. Radial mesh normals alone remove the
larger dark grooves/strips, but the textured repetition remains. This separates
two contributors; it is not a prescription to flatten all terrain. Native
bookends restore the original appearance. All substitution modes are test-only.

New isolated NormalMacroWarp20260929V1 candidate keeps those original normal maps,
all geometric normals, three texture fetches per projection function and normal
axis convention. Only five audited macro-normal WAT calls use a separate copy
with bounded continuous planet-fixed coordinate shear. No camera-dependent
coordinates, <=50m texture periods unchanged; full effect above200m periods.
Control mode0 is the original domain. No production selection/publisher.
First bake09:07 refused before saving (Size scalar specialization), corrected
to float3 Period. Second bake09:10 succeeds,0 errors/9 warnings,3 new assets.
DLL SHA5191214105B9CD78D19ED90F3452D9739BE07FF35BBBFE0D7212E64A4CDE8316.
Master SHA DF7CF32861667E63158F0D86D7EC15ACA8C16A60951170830E66823402FB36A6;
function C7C87C337847CF16D533D5773B39872D9362830B6398840494E56F5118683B03;
instance8D71C9738C892DEFECEC6621FBE9535A132F207197F9155989B5289F0196EC74.

Run: planet_macro_20260928/terrestrial-normal-warp-3km-v1, finished09:13:11.
2/2 tests,12/12 orbit PNGs inspected across nadir/oblique/limb, ordinary atmosphere.
Native/control/restored retain the same appearance. Candidate changes straight
repetition into larger curved, directional ridges; rejected as a P0 solution.
No production publication. GPU native/candidate means4.223/4.286,4.418/4.490,
4.151/4.254ms, diagnostic only. Eight Shared hashes unchanged after render.

Next isolated candidate: NormalHex20260929V1, normal-only randomized triangular
lattice blending (Mikkelsen2022, https://jcgt.org/published/0011/03/05/).
Preserves original macro textures and micro sampling periods<=50m. Uses a
compensated planet-fixed cell domain, hashed translations/quarter-turns and inverse
normal rotation, explicit gradients, slope blend. Three samples per projection
instead of one (four in partial50..200m period gate); no altitude fade. Native
mode0 must match the original image. Build/bake/render are pending at09:24.
Do not treat additional texture work as free: measure the actual GPU delta.

## NormalHex V1: isolated rendered results,09:35

Build5 actions/13.83s. Bake normalhex-macro-normals-v1 completed09:27:05,
0 errors/9 warnings,3 new assets only. Source-to-build DLL SHA
850B2BB41F4232772EBF91449AD2BFC5352571C471F4FCBC2EE621B0A14A953F.
Master BA350182B52CCA161EA6BE37D57715C4BF51F3609FE98D306FD30E367315C304;
function D6B5796B5358D7912FBED8E1021B4E2A9D93024A1CD9B393FF4013058E8A1C71;
instance669AB16A1345CB7BB9F79A911AD53C1E263DA68A47DE26E10BB2D29C52A2037A.
Shared8 and catalog hashes unchanged after bake. CPU algebra spot check:3000
lattice-edge joins, weightL1<=1.400006e-6 across +/-1e-7; all4 quarter-turn
inverse transforms exact. This is not a GPU precision/full visual test.

Runs under planet_macro_20260928: terrestrial-normal-hex-3km-v1 (09:29:45)
and terrestrial-normal-hex-100m-v1(09:32:29). Each2/2 tests and12/12 orbit
frames inspected at1280x722, native/control/candidate/restored in three views,
ordinary atmosphere. Control/native RGB8 p99=1 in all3km views;
MAE.142/.195/.148; restored/native MAE.084/.185/.136. Unpacking/sampler
control matches, actual candidate binding and full mesh payload guarded.

At3km regular normal-ridge repetition is reduced; broad underlying shading
strips and blotchy colour remain. No visible hex-edge grid in these views.
GPU native/candidate meanMs4.226/4.643,4.427/4.930,4.165/4.630.
At100m small texture/terrain shapes retained, but broad normal-lighting differs;
not pixel-identical ground and not character-height acceptance. GPU means
4.331/4.618,4.409/4.773,4.158/4.569. Not production FPS or target-res budget.
No publication yet. Source prepared for a live-root flight down to2m with4s
ground hold, exact double-frame/high-low checks and automatic restoration.

## NormalHex live-ground and Frozen checks,09:44

Terrestrial run: planet_continuity_20260929/terrestrial-normal-hex-ground-flight-v1,
finished09:38:04,2/2 tests. Actual1280x722, normal atmosphere unchanged.
DLL1964E824D608A0B0796752E5B54CDFD9B25E02E380B1A4458EE83F5BF416EA1B.
Native82/candidate81 captures,100km->2m,4s hold,back to100km. No root freeze;
new/current LOD material and exact5double+6high-low frame uniforms guarded.
All163 logged samples presented10/incomplete0; workers remain active in motion.
This alone does not prove absence of visible popping between sampled frames.
Inspected22/163: Native10,12,14,18,39; Candidate0,10,12,14,16,17,18,19,20,
31,39,46,52,61,65,68,74. Near detail retained; several-km normal-repeat reduced,
broad geometric shading and saturated/faceted lakes remain. Ground lighting is
not identical: Native39/Candidate39 RGB8MAE4.241,p99=13. Candidate31/39 MAE.721,
39/46 MAE.195. Ground hold async GPU4.146->4.517ms; capture overhead applies.
Remaining141 frames are recorded, not visually accepted as continuous video.

Frozen: planet_macro_20260928/frozen-normal-hex-3km-v1,finished09:42:37,
2/2 tests,12/12 orbit frames inspected across3views and4variants.
Regular normal ridges reduced; coarse snow/rock distribution and broad shading
remain, no obvious hex edge in these frames. Native/control RGB8MAE.257/.209/.153,
p99=2/1/1; restoredMAE.234/.284/.132. GPU native/candidate means4.150/4.523,
4.321/4.723,3.990/4.475ms. All8Shared hashes unchanged checked09:49.
Not publication/target-resolution/Frozen-ground/menu acceptance.

## Combined candidate prepared09:52

New Diagnostics/ContinuityCombined20260929V1 combines unchanged HexV1 with
FieldsV3. Optional field enable shares APS_NormalMacroWarpMode: control0 keeps
the exact native field and sampling; candidate1 enables both. Frozen candidate
also sets APS_OrbitalMacroMode1 as in the previous FieldsV3 trial; control restores
the source value, guarded separately. Existing V3 and normal-only assets untouched.
The first compile caught an out-of-scope diagnostic error variable; changed to
the lease's existing deferred error channel (Validate fails before capture).
Second build13.65s succeeded. Bake/render still pending at this checkpoint.

## Combined V1 bake and controlled views,10:12

Separate ContinuityCombined20260929V1 baked09:54:25,0 errors/9 warnings;
final shader-completeness and LocalVF guards passed. DLL SHA
6D63A41ED52DE0EFC754EDEFA4E329995F12EB8C801560680292975591021CE6.
Master725CD0683527CB4C669849CC1E1A77806D3E2877ADA5DE96AF8D3A800F681DDD;
functionFDBD6F7BF8C0ABD2EB73372AB8E3212C7084ED33769C162FE04FA360EF3A20AB;
instance08A91F3139F709FDF96A7E5BB811A126BFA2F01367363D8A754DB99C602497EB.
Only three new diagnostic assets; no production selection.

Runs under planet_macro_20260928, each2/2 tests:
- frozen-combined-3km-v1,09:57:54,1280x722,12/12 frames inspected.
  Native/control/restored appearance retained; normal-repeat reduced as HexV1.
  Native/candidate GPU4.156/4.569,4.305/4.921,4.000/4.670ms in three views.
- frozen-combined-100km-wide-v1,10:00:16,actual1594x862,12/12 inspected.
  Requested3440x1340 was clamped by offscreen work-area size; not a wide benchmark.
  Fine repeat reduced, broad snow/rock blobs and flat appearance remain.
  Native/control RGB8MAE nadir/oblique/limb .050/.218/.096,restored .158/.223/.095.
  GPU4.083/4.722,4.165/5.006,3.371/3.952ms. Shared8 unchanged at10:06.
- terrestrial-combined-100km-wide-v1,10:08:35,actual3440x1342.
  6/12 inspected: all native/candidate views. Control/restored measured, not viewed.
  Native/control MAE .202/.197/.071,restored .218/.216/.078; p99=1 throughout.
  Regular detail repeat reduced; large terrain bands, blue faceted lakes and
  abrupt water boundaries remain. GPU5.774/7.069,5.831/7.450,4.938/5.556ms.
  Not complete planet-scene FPS and not final P0 acceptance.

Per-process PIE size override now includes a sufficient offscreen work area;
no saved user INI was changed. Runtime exact geometry/frame/mode guards passed.
Combined live-ground and menu checks completed; see the next checkpoint.

## Combined menu and ground checkpoint,10:32

Menu matrix-combined-menu-matrix-v1:5/5 tests,24 frames,16/24 visually inspected
(all12 candidate views plus native whole/close pairs for both types). Fine square
pattern reduced. Broad painted patches and polygonal liquid shores remain.
Build10:13 DLL F4BD3032E760A48023140CAE7EC9E82D28930DC948F7CA30B10958A17681AEFE.
This is not complete P0 acceptance or a proof of continuous motion.

terrestrial-combined-ground-flight-v1 and frozen-combined-ground-flight-v1:
2/2 tests each, ordinary atmosphere, live LOD,100km->2m(4s hold)->100km.
Each has4 inspected frames: Native039,Candidate039,Candidate010,Candidate013.
Fine ground structure remains; medium-distance terrain bands remain. These are
sampled captures, not an inspection of every frame or video. Both runs restored
the source materials. Shared8 unchanged across all three runs, checked10:31.

Next step is a separate immutable ContinuityV1 copy, then runtime integration
only for Terrestrial/Frozen after verifying assets. Initial attempt to switch
before creating those files was rejected by the safety check; no source changed
in that attempt. Runtime selection remains unchanged at this checkpoint.

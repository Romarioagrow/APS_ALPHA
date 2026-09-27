# Disabled Water depth candidate - 2026-09-27

Accepted recovery commit: `67fff3b4f8fa57cbbd47f29e880f3b80197f096f`.
Height-only sampler/preflight: `8ace4772`. The user-accepted production appearance
remains the default. This checkpoint saves an experiment, not a visual release.

## Isolated changes

- `OnlyWaterDepthCandidate` creates two new diagnostic packages, cloning accepted
  Water dependencies without changing their files or runtime selection.
- One graph edge changes: deep/shallow palette interpolation can use signed
  depth in UV1.X (km), gated by UV1.Y validity. Other outputs, coverage, waves,
  reflection and saved Water parameters are preserved. Missing/invalid depth
  and zero strength use the original interpolation.
- `APSProbeWaterDepth` runs only in dev automation, on the retained generated
  PLANET water mesh. Task-local physical-profile sampling fills transient UV1;
  geometry, RGBA, other UV channels and camera/mesh transforms are checked.
  Original material, UV1 and generator ticking are restored afterward.
- No WorldScape vendor files, worker layouts, production bindings, planet seeds,
  geography, saved worlds, stars or atmosphere defaults are changed.

The saved candidate defaults (.65 strength, 80 m half-depth) are experimental
artistic controls, not calibrated absorption physics. V2 overrides them with
.35 and 20 m. No default is approved for gameplay.

## Evidence and limitations

Builds succeeded (9 actions/34.22 s; V2 4 actions/9.02 s). Candidate bake saved
exactly two assets with no runtime binding. Its shaders and LocalVF compiled;
existing plugin/scene warnings remain separate from visual acceptance.

Two owned D3D12 SM6 runs exited 0 and produced six real composited PLANET images
each. The automation recorder reports success with warnings, not aesthetic
acceptance. Captures are 888x500. Only Ocean, seed 1337, radius 6750 km was covered;
neither run proves ground/shoreline LOD behavior, other families or 120 FPS.

V1's cyan shelves are too broad/mottled. V2 is visibly more restrained, but its
close-view accepted baseline differs globally in brightness from both fallback
frames. Mean absolute RGB differences over ROI [284,90,603,415], in 0-255 units:

| V2 comparison | Mean difference |
| --- | ---: |
| Accepted to zero strength | 4.3115 |
| Accepted to invalid payload | 4.3133 |
| Depth to zero strength | 1.8522 |
| Zero strength to invalid payload | 0.0396 |

Thus V2 is not a clean before/after comparison. Fixed geometry/transforms do not
rule out exposure, streaming or other render-state drift; the cause is not yet
established. V1 fallback differences were .1420/.1438, but that does not validate
V2. Do not fix this discrepancy by changing the accepted palette. Next diagnostic
must bracket the candidate with settled original-before/original-after controls
and inspect actual rendered view/exposure/streaming state before promotion.

V2 fixes physical-reference radius rounding: use the same full-scale root float
property conversion (675000000 cm), not division of rounded preview radius
(675000017 cm in V1). One-off parallel sampling of 99846 vertices took
37.429/38.382 ms; this is not a gameplay frame-time benchmark. Full-scale worker
integration, volume overrides, UV1 LOD/shoreline coverage and FPS remain open.

## Artifacts

Local evidence is retained under:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-visual`.
`render-v1` and `render-v2` contain logs, reports and
`Saved/Automation/PlanetRefinement/Ocean/WaterDepth20260927/*.png`.
`Bake.ps1`, `Render.ps1`, manifests and source-install receipts describe the runs.
`compare_frames.py` measures existing captures without editing them.

SHA-256 report hashes:

- V1: `FFC4DBAEE543C1B15D8FD56508E05077EB5C9E65E36187FF5D1E65C1AFE48809`
- V2: `CC0FDF22D4E0EF2B0E8AF29A01E33156590AA70BD4D4F2EFA2E5FFCAEA293B67`

Candidate package SHA-256 (under PlanetSurface/Diagnostics/WaterDepth20260927):

- Master: `215926D9FDEEEC7948D9B10EE4E828392A88A338E1EE485DD1171204393E7A49`
- MIC: `AFB0E5E322F177EE07206502DC457246646192A03F7739E1C459B55DF31FF3DC`

Accepted source assets were rechecked unchanged (SHA-1):

- SharedLiquid/M_APS_SharedAmmonia: `91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC`
- SharedLiquid/MI_APS_SharedWater: `30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8`

WorldScape remains the installed production terrain/LOD system. Its accepted
external snapshot and recovery instructions are in
`2026-09-27-worldscape-accepted.md`; no rollback or restoration was performed.

# Water depth: settled controls and Terrestrial coverage

Follows the disabled candidate checkpoint `79dd7bf7`. Accepted visual recovery is
still `67fff3b4`; no production material selector or appearance default changes.

## Test changes

The dev-only probe now captures the original material twice before the close
comparison and again after switching back. A post-freeze settling interval
separates camera ticking from rendered history. Read-only final game-view
observation records exposure/local-exposure settings and streaming demand;
original production material-frame invariants are checked at every capture.
No exposure, light, streaming or camera-production setting is overridden.

Explicit Water-family selection supports Ocean, Water, Terrestrial, Forest,
Oasis and Savanna. Unsupported names fail rather than silently testing Ocean.
Actual body type, resolved profile type, Water chemistry and ocean-enabled state
must match the requested family. These are test-fixture changes, not changes to
the user's planet-family/subtype selection UI.

## Actual rendered evidence

Builds V3/V4 succeeded (4 actions, 9.10/9.24 seconds). Owned hidden D3D12 SM6
processes 16856 and 21728 exited themselves with code 0. Both automation reports
record one success with pre-existing scene warnings and zero failures. Eight
888x500 real PLANET captures were inspected/compared per run. Recorder success
is not broad visual acceptance or a performance benchmark.

All differences below are mean absolute RGB in 0-255 units over the same ROI
[284,90,603,415]; low differences are not exact pixel identity.

| Check | Ocean V3 | Terrestrial V4 |
| --- | ---: | ---: |
| Original to zero strength | .1434 | ~.05 |
| Original to invalid depth payload | .1414 | ~.12 |
| Repeated original to returned original | .0487 | ~.13 |
| Original to enabled depth | 1.8523 | ~4.25 |

Ocean V3 no longer reproduces V2's 4.31 baseline drift. Its shallow/deep variation
is distinguishable from normal original-frame variation. This supports the
settled V3 comparison, not a retrospective claim about the exact cause of V2.

Terrestrial was actually generated as type 1 with Water and land coverage .64,
not an Ocean stand-in. It shows shallow coastal variation, but thin shoreline
outlines remain and some land darkens while the candidate is enabled. Example
land pixel (435,318): original RGB (38,56,59), candidate (36,53,56), returned
original (39,56,59). Thus zero-strength parity alone is insufficient for adoption.
The scene-wide lighting response and screen-scale depth transition still need
investigation before calling this a finished material improvement.

Final-view exposure settings stayed constant: method 0, min=max=-.263034433,
bias .1, physical exposure on. Local highlight/shadow scales remained 1, grey
bias 0. Streaming demand was 0 at every capture. These are CPU final-view
observations, not GPU eye-adaptation or indirect-light history readbacks.

Geometry CRCs stayed fixed (Ocean 3800427927, Terrestrial 4076693934). All material,
UV1 and generator ticking restoration paths completed. Physical-reference radius
was 675000000 cm in both. One-off sampling of 99846 vertices took 38.419/32.893 ms;
do not equate this with steady-state frame time.

## Decision / remaining work

Keep the depth material unbound in normal PLANET and gameplay. No accepted
palette, atmosphere, star, terrain/WorldScape module, world data or authored
SinglePlay change is required to preserve these results. Do not compensate the
land-darkening observation with global color/exposure changes.

Next work must separate candidate-induced lighting response from depth-color
response and account for thin/subpixel shoreline transitions. Then verify the
same physical depth data through actual WorldScape ocean LODs, including near
shore and family coverage, with measured gameplay cost before any default switch.
Forest/Oasis/Savanna, live surface rendering and ~120 FPS are not verified here.

## Reproducibility

Evidence workspace:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-visual`.
Run directories `render-v3` (Ocean) and `render-v4-terrestrial` contain logs,
reports and `Saved/Automation/PlanetRefinement/<family>/WaterDepth20260927` PNGs.
Both use strength .35, half-depth 20 m, seed 1337, radius 6750 km.

Report SHA-256:

- Ocean: `10A23C4D3CBFE1ABF606F32C0582D540A9098F65DA34CEA7150E91E0EBC202D1`
- Terrestrial: `8775C5C80B4ADF5D88E0125AFEEEF86116569A864B4C6483329A4385251CB847`

`probe-v3-manifest.json` / `probe-v4-manifest.json` and canonical
`Saved/WaterDepthVisualProbeV3_20260927` / `Saved/WaterDepthVisualProbeV4_20260927`
hold guarded installation/backup receipts. The accepted water master and MIC
were rechecked byte-identical to the SHA-1 values in the previous checkpoint.

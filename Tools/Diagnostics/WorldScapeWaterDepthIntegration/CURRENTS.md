# Ocean current / column experiments, 2026-09-27

Both candidates are source archives, NOT production-installed materials.
Accepted source assets, palette endpoints, terrain and spawn remain unchanged.
Private root: `F:/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-gameplay`.

## Reproduce without stacking increments

Start with the documented 19-file `overlay-v21` host and scripts. Apply either
`currents-v22.patch` OR `column-currents-v23.patch`, not both. Each manifest
records before/after SHA256 values relative to that same v21 baseline. The
v23 patch was reverse-checked against its actual final private source tree.
Build all matching private APS/WorldScape modules; never copy only one DLL.

The host's Content junctions were absent after the disk migration. Its three
ordinary Content directories were preserved as `Content.migration-stub-20260927`
and the read-only junctions restored. The failed v22 bake-v1 was missing assets,
not a shader rejection. No production Content was overwritten.

## v22: bounded currents and animated fine waves

Planet-fixed, double-float local coordinates drive three smooth interacting
phases with a 6 km scale and a continuous 4096 s wrap. Pixel-footprint filtering
suppresses unresolved current contrast. Default current modulation is bounded
to +/-4% reflectance and +/-0.015 roughness. No emissive ocean, silhouette/WPO
change, new shoreline mask or palette replacement. The existing analytic fine
wave phases animate only under this explicit candidate flag.

`BakeRipples.ps1 -Label bake-ocean-currents-v2 -SingleLayerSurfaceControl
-WaterSurfaceFill -WaterCurrents` succeeded: two private assets saved and shader
compilation completed. `ocean-currents-terrestrial-v1` produced all static views
but FAILED the material-performance setup guard. Do not count it as a performance
pass. A subsequent real grounded shore walk passed (58.57 m, 10.006 s), but the
rendered ocean remained excessively cyan/flat at grazing angles.

## v23: physical depth dominates the base-colour interpolation

Adds explicit `-WaterColumnContrast` (requires `-WaterCurrents`). Replaces the
additive-only shallow term with a 75% physical-depth / 25% legacy colour mix;
half-depth is 12 m. Invalid payload still falls back to the unchanged legacy
colour. The signed native depth, its validity, pixel-footprint integration,
palette endpoints and specular Fresnel are retained. This is still an opaque
SingleLayerWater surface-control experiment, NOT calibrated volume absorption,
transmission, refraction or a completed ocean model.

Build `build-20260927-201503.log` succeeded. Bake
`bake-ocean-column-currents-v1/bake.log` succeeded (2 saved, 0 errors, 7 warnings).
Run `ocean-column-terrestrial-walk-v1` used Terrestrial, Ripples,
SingleLayerSurfaceControl, WaterFilteredShadows, WaterSurfaceFill, WaterCurrents,
WaterColumnContrast, WaterShoreWalk, CameraHeightM=2.
Both automation tests passed; the palette unit test covers its pre-existing
additive budget, NOT the v23 contrast change. Native signed payload/oracle,
visible terrain, stable geometry/RGBA/UV and exact restoration were checked.
Actual grounded-input walk: 58.5263 m / 10.0103 s, 614 samples, zero unobserved
frames. Engine median/p95 14.3772/19.2527 ms; asynchronous GPU 4.3185/4.7035 ms.
These are offscreen editor counters, NOT Present timing or 120 FPS acceptance.

Inspected `Saved/Automation/WaterDepthGameplay/02-filtered-depth.png`,
`04-original-return.png` and `21-walk-end.png`: depth changes are visible, but
the broad blue/cyan band and flat near-water presentation are still too strong.
DO NOT promote either candidate on the strength of a green automation result.
New performance-rejection logging diagnoses the existing guard; it does not
relax it. Sprint, paired in-process moving A/B/A, other water-bearing families,
orbital views and natural lighting transitions remain unverified.

Next work must address the observed optical/shore appearance and obtain paired
moving-cost evidence. Do not repeat a bake of unchanged v22/v23 as a fix, lower
quality silently, weaken payload/floor guards or advertise this archive as an
installed improvement.

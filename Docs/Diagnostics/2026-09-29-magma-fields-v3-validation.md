# Magma orbital fields V3: rendered checkpoint

29 September 2026, 07:09 local. This is a scoped diagnostic result, not production
promotion or acceptance of all planet visuals. No original assets overwritten.

## Source-to-runtime chain

- Installed test/bake DLL: A0F720337971E035FDC9A1C97D08488F5D266BB06114980A09DAC6B17AAED43B.
- Bake: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/bake-magmafields-v1`.
- Commandlet completed 07:03:37: 0 errors, 9 existing function-pin repair warnings.
  Final SM6 shader readiness/LocalVF guard passed before either package was saved.
- Native template is SharedMagma, not Terra. `lavaPlanet=1` retained, Noise1 kept
  (1 sample/1 consumer), five other color samples/nine consumers replaced.
- New master SHA256: ABA374D474C89AF4EE0642433C8ADEEBBA7C1B87592B8D4BB9939FBF9C18B955.
- New MI SHA256: A1D789F8E1AF102F8F171365C9FD660E620AE37DC7B9995C2067410D4FAF77D4.
- Both are in `Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/OrbitalFields20260929MagmaV3`.
- Runtime guard confirmed actual SharedMagma parent, matching effective static
  switches/masks and original Noise2/3/4 textures. No guard was bypassed.
- All eight protected Shared assets and production catalog remained unchanged.

## Rendered evidence

Run: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/volcanic-fields-v3-magma-family-v2`.
Frames under `Saved/Automation/PlanetRefinement/Volcanic/TerrainPixelAB`.
Seed 1337, radius 6750km, actual viewport 1920x1082. Twelve PNGs inspected:

| Pair | Distance | Observed result |
| --- | --- | --- |
| 00 | 20000km | Whole disk: broad terrain colors, lava boundaries and rim retained; fine modulation changes subtly. |
| 01 | 20000km | Rotated daylight disk: no obvious broad-pattern, coverage or silhouette regression. |
| 02 | 20000km | Terminator/night control: lava and light preserved; dark areas do not establish fine-detail quality. |
| 03 | 2000km | Close ground: fine woven repetition reduced; broad existing billows remain. |
| 04 | 2000km | Close lit ground/lava boundary: fine grid reduced; existing angular shoreline is visibly still present in BOTH variants. |
| 05 | 2000km | Dark control: no gross coverage/light change; insufficient signal for detailed anti-tiling acceptance. |

Lava material and geometry were intentionally unchanged. The remaining shoreline
facets, coarse pattern and haze must not be described as fixed by this result.
These are settled retained-menu-mesh comparisons, NOT live WorldScape handover.

## Settled GPU observations

Menu asynchronous GPU counter, 87-91 samples per image. Not gameplay FPS, not
memory or traversal hitch acceptance. Same run/geometry/light for each pair.

| View | Native mean ms | Candidate mean ms | Delta ms |
| --- | ---: | ---: | ---: |
| 00 | 3.08430 | 3.26873 | +0.18443 |
| 01 | 3.08707 | 3.30245 | +0.21538 |
| 02 | 3.09536 | 3.28006 | +0.18470 |
| 03 | 3.43743 | 3.55676 | +0.11933 |
| 04 | 3.46849 | 3.57709 | +0.10860 |
| 05 | 3.49751 | 3.62752 | +0.13001 |

12/12 automation tests succeeded: rendered fixture, three family/camera
contracts, all eight foliage policy tests. Two tests have warnings: the
transient-world destroy test's no-world-context warning and existing generator
diagnostic logging. No test errors. Policy passes do not prove foliage placement,
object residency or actual memory/frame-time limits.

## Next acceptance gates

Keep V3 diagnostic-only until remaining surface families and native near-ground
control pass. Run ordinary-atmosphere inward/outward live traversal, inspect the
5-50km transition and actual WorldScape coverage separately. Preserve the new
branch-specific template and static guards during any production integration.

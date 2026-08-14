# APS-74 wet-ocean pixel diagnosis

Date: 2026-08-14

Result: **water visibility/material contribution PASS; wet color appearance
FAIL**.

Read-only sampled RGB analysis used the canonical seed-424242 artifacts:

- `APS_GeneratedCivilization_WetOcean_A.png`
- `APS_GeneratedCivilization_WetOcean_NoOcean.png`
- `APS_GeneratedCivilization_WetOcean_B.png`

PNG alpha is zero but RGB data is valid; statistics ignore alpha and sample
every second source pixel.

## Lower-frame results

| Capture | Mean RGB, lower 55% | Blue minus red | Mean chroma |
| --- | --- | ---: | ---: |
| Wet ocean A | 99.36 / 93.82 / 88.47 | -10.89 | 10.89 |
| Ocean hidden | 114.04 / 106.87 / 77.46 | -36.58 | 36.58 |
| Wet ocean B | 108.89 / 102.93 / 97.40 | -11.49 | 11.49 |

Pairwise lower-region evidence:

- Wet ocean A versus ocean-hidden: MAE `61.86`, RMSE `72.64`, and `77.83%`
  of samples have a max-channel delta greater than 15.
- Wet ocean A versus wet ocean B: MAE `9.72`, RMSE `12.85`, and `7.82%` of
  samples exceed the same threshold.

## Finding

The water layer is not missing or visually inert. Hiding it changes most of the
lower frame and makes the underlying result much more red-dominant. Visible
water substantially reduces that red dominance, consistent with the separate
geometry/visibility probe.

The remaining failure is that the visible wet result stays warm grey/brown:
blue remains below red in both open-water views. This narrows the open APS-74
regression to the wet material/palette/lighting output rather than WorldScape
ocean geometry, LOD presence, or visibility.

No Ocean/Living, Land Coverage, profile, material, water or WorldScape asset was
modified. A fix still requires explicit ownership of the shared wet material or
profile path and a current seed-424242 rendered rerun.


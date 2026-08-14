# APS-77 surface material runtime A/B result

Date: 2026-08-14

Result: **PASS for runtime apply/restore and functional non-regression; PARTIAL
for visual diagnosis**.

## Current-code baseline

- Automation: `APS.Rendered.Gameplay.GeneratedCivilizationHandoff`.
- Result: Success; normal `Automation Test Queue Empty` TestExit.
- Runtime: 37.2 seconds; no residual UE or ShaderCompileWorker process.
- Screenshot:
  `Saved/Automation/APS-77-MaterialAB-Evidence/Baseline_Current_MaterialAB0.png`.
- SHA-256:
  `A3DDCB5D57A9750021B83069C3A28380EB896405DE2A629039EFE34D886063A4`.
- Log: `Saved/Logs/APS-77-MaterialAB-Baseline.log`.

The older pre-run screenshot has a different hash and materially different
exposure/content statistics, so it is preserved for history but is not used as
the immediate A/B truth.

## Preset 1: aggregate near path disabled

- CVar: `aps.Surface.Diagnostics.MaterialAB 1` (`near-all-off`).
- Automation: same generated gameplay handoff test and fixed screenshot path.
- Result: Success; normal TestExit after 36.1 seconds.
- Runtime log proves application to canonical anchored material
  `MID_MI_APS_WS_Cryogenic_0`:
  - `NearColorStrength=0.0`
  - `DetailNormalStrength=0.0`
  - `DetailRoughnessStrength=0.0`
  - seam-normal values remain at the accepted baseline
    `2400 / 5000 / 0.88`.
- Runtime log proves `action=restore` on the same MID before TestExit.
- Screenshot:
  `Saved/Automation/APS-77-MaterialAB-Evidence/Preset1_NearAllOff.png`.
- SHA-256:
  `2DF0678BFAF6355C799B3C6B39060540C5349B8E351B1834A20F1F7D162FC937`.
- Log: `Saved/Logs/APS-77-MaterialAB-Preset1.log`.

## Same-camera sampled RGB delta

The PNGs are 876 x 388 with zero alpha but valid RGB data. Statistics sampled
every second source pixel and explicitly ignored alpha.

| Region | Mean absolute error | RMSE | Pixels delta > 5 | Pixels delta > 15 | Luma delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| Full frame | 8.011 | 17.213 | 48.68% | 12.16% | +3.912 |
| Sky, top 45% | 6.137 | 13.219 | 43.05% | 7.10% | +1.819 |
| Surface, bottom 55% | 9.534 | 19.862 | 53.31% | 16.33% | +5.647 |
| Surface, bottom 35% | 11.374 | 23.435 | 55.25% | 21.20% | +6.018 |

The preset response is stronger in the surface region than the sky region,
which supports a real contribution from the aggregate near material path.
However, non-zero sky delta shows that sequential captures also contain
temporal/exposure variation. This evidence does not yet isolate near color,
detail normal, or detail roughness individually and is not an accepted visual
fix.

## Non-regression and resources

- The canonical Cryogenic profile/MID identity remained stable.
- The generated gameplay handoff test passed in both baseline and preset runs.
- Preset values restored before world teardown.
- After the preset run: no heavy UE process remained, free RAM was 21.56 GB,
  and free space on F: was 482.16 GB.
- Ocean/Living and Land Coverage baselines were not edited.

## Remaining diagnosis gates

- Isolate LOD seam radial-normal blend (preset 2) from the aggregate near path.
- If still needed, isolate near color and detail roughness with presets 4 and 5.
- Capture temporal movement evidence at a fixed camera or an exposure-locked
  render before making a shared material/profile change.
- Do not classify compile-only, log-only, or this two-frame sequential A/B as a
  final crawling/clipping visual PASS.


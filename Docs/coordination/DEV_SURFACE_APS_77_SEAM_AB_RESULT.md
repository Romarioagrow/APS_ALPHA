# APS-77 LOD seam-normal A/B result

Date: 2026-08-14

Result: **PASS for runtime isolation/restore; PARTIAL for visual diagnosis**.

## Runtime contract

- CVar: `aps.Surface.Diagnostics.MaterialAB 2` (`seam-normal-off`).
- Automation: `APS.Rendered.Gameplay.GeneratedCivilizationHandoff`.
- Result: Success; normal TestExit after 35.7 seconds.
- Anchored material: `MID_MI_APS_WS_Cryogenic_0`.
- Preserved near-path values:
  - `NearColorStrength=0.0142`
  - `DetailNormalStrength=0.1225`
  - `DetailRoughnessStrength=0.0236`
- Seam values during preset: fade `2400 / 5000 cm`, far blend `0.000`.
- Runtime log proves `action=restore` on the same MID before TestExit.
- No residual UE/Shader process; free RAM 21.50 GB and free F: 482.14 GB.

Artifacts:

- `Saved/Logs/APS-77-MaterialAB-Preset2-Retry.log`
- `Saved/Automation/APS-77-MaterialAB-Preset2-Retry`
- `Saved/Automation/APS-77-MaterialAB-Evidence/Preset2_SeamNormalOff.png`
- Screenshot SHA-256:
  `3F39D83E83854D7E12E2AD1641D05B68B386FB932B529A5EB328EC3D97A4E17C`

The earlier preset-2 attempt never reached automation because a foreign
link-only failure had removed `UnrealEditor-APS_ALPHA.dll`. No screenshot from
that attempt was accepted. The current artifact was captured only after a
coordinated full build restored the DLL.

## Sampled RGB delta versus current baseline

PNG alpha is zero but RGB is valid. Statistics sample every second source pixel
and ignore alpha.

| Region | MAE | RMSE | Delta > 5 | Delta > 15 | Luma delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| Full frame | 3.334 | 12.616 | 10.78% | 7.41% | -0.606 |
| Sky, top 45% | 2.579 | 9.990 | 10.38% | 5.97% | -1.030 |
| Surface, bottom 55% | 3.948 | 14.443 | 11.11% | 8.52% | -0.285 |
| Surface, bottom 35% | 4.682 | 16.066 | 12.62% | 9.92% | +0.675 |

For comparison, aggregate near-path preset 1 produced surface MAE `9.534` and
bottom-35 MAE `11.374` against the same baseline. The seam-only response is
therefore real and mildly ground-weighted, but substantially smaller.

## Finding

The radial-normal far blend may contribute to LOD-bound shading changes, but it
is unlikely to be the dominant source of the observed crawling. The aggregate
near color/detail-normal/detail-roughness path has the stronger surface-region
response. Sequential captures still contain temporal/exposure variation, so
this remains a diagnostic classification rather than a final visual fix.

Next useful isolation, if resources and ownership permit, is near color versus
detail roughness (presets 4 and 5) or exposure-locked temporal capture. No
shared material/profile/WorldScape file was modified.


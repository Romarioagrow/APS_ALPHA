# APS-73 / APS-77 Relief and Feature Scale Diagnosis

Date: 2026-08-14  
Owner: Dev Surface  
Scope: source-derived comparison of the same generated gameplay surface under the default material path and APS-77 diagnostic presets 1 and 2. No shared WorldScape, generator, profile, or material asset was edited for this checkpoint.

## Result

**PARTIAL — geometry diagnosis PASS; final visual/material acceptance remains open.**

The selected LOD0 surface is not flat or relief-starved. Render mesh, collision mesh, local height samples, and walkable-surface slope remain materially unchanged across the A/B runs, while image-space ground contrast changes strongly when material paths are disabled. The weak shape cues and reported crawling are therefore primarily a shading/material/temporal problem at this camera, not evidence that `SurfaceFeatureScale` or `SurfaceReliefScale` collapsed the generated mesh.

## Controlled baseline

- Automation: `APS.Rendered.Gameplay.GeneratedCivilizationHandoff`
- World/model seed: `424242`
- Runtime family in these APS-77 captures: `EPlanetType::Frozen` -> `Cryogenic`, `ocean=false`
- Gameplay planet scale: `6.371000e+08 cm`
- Gameplay WorldScape: terrain `10x96@120`, collision `64x64@120`
- `SurfaceFeatureScale=1.0`, `SurfaceReliefScale=1.0`
- LOD0 render vertices: `8,836`
- Collision LODs/vertices: `26` / `109,850`
- Same automation camera and ground ROI: `(175,213)-(701,342)` at `876x388`

This Frozen/Cryogenic material A/B is not the APS-74 accepted Water/Oceanic parity truth. APS-74 canonical identity remains `SYS0/S0/P0`, Water/Oceanic seed `424242`; its wet appearance is still classified FAIL/open.

## Geometry evidence

| Measurement | Default (`MaterialAB=0`) | Preset 1 (near aggregate off) | Preset 2 (far radial seam blend off) |
|---|---:|---:|---:|
| Initial render LOD0 relief | 2,662.82 cm | 2,662.82 cm | 2,662.82 cm |
| Physical render relief | 2,687.57 cm | 2,640.02 cm | 2,687.57 cm |
| Collision relief | 12,409.69 cm | 12,409.69 cm | 12,409.69 cm |
| Local collision relief | 158.37 / 467.87 / 913.33 cm | 158.88 / 469.12 / 913.47 cm | 158.27 / 467.65 / 913.29 cm |
| Natural-surface relief | 314.70 / 3,503.98 / 9,448.28 cm | 315.65 / 3,492.37 / 9,417.15 cm | 314.52 / 3,505.93 / 9,453.56 cm |
| Natural-surface max slope | 0.22281 | 0.22242 | 0.22288 |
| Render/collision delta | 9.18 cm | 12.33 cm | 11.35 cm |
| Heightfield delta | 0.01 cm | 0.01 cm | 0.01 cm |
| Local collision/noise delta | 0.05 cm | 0.05 cm | 0.05 cm |
| Natural surface / physical surface | PASS / PASS | PASS / PASS | PASS / PASS |

The deterministic placement probes independently report `3.13-4.51 m` relief across 10 m, `35.22-37.61 m` across 100 m, and `80.39-94.93 m` across 250 m, with candidate max slope `0.2233-0.2370`. These values are identical across all three runs.

The small render/collision deltas are centimetre-scale against metre-to-hectometre relief and do not indicate terrain clipping. Character grounding remained valid with `2.61-2.64 cm` foot clearance.

## Image-space material response

| Ground ROI metric | Default | Preset 1 | Preset 2 |
|---|---:|---:|---:|
| Mean brightness | 141.482 | 140.971 | 130.028 |
| Variance | 1,071.284 | 979.922 | 823.641 |
| P10 / P90 / spread | 120 / 175 / 55 | 122 / 172 / 50 | 113 / 153 / 40 |
| Mean spatial delta | 2.914 | 1.374 | 2.679 |

- Preset 1 disables near color, detail normal, and detail roughness together. Geometry remains stable, but ground `meanSpatialDelta` falls by about **52.8%** (`2.914 -> 1.374`). The aggregate near path is therefore a major source of human-scale microcontrast and a leading crawling suspect.
- Preset 2 disables only the far radial-normal seam blend. It lowers brightness and tonal spread, but ground `meanSpatialDelta` changes by only about **8.1%** (`2.914 -> 2.679`). The seam-normal blend has a real response but is secondary to the aggregate near path.
- Separate screenshot comparison already measured default-to-preset-1 surface MAE `9.534` (bottom 55%) / `11.374` (bottom 35%), versus default-to-preset-2 `3.948` / `4.682`. This independently supports the same ordering.

## Diagnosis and next isolation

1. Do not increase Relief/Feature Scale as the first fix for the current weak shape cues. The physical terrain already carries substantial relief and stable slopes.
2. Isolate preset 4 (near color only off) and preset 5 (detail roughness only off) after the shared APS-81 compile blocker is fixed and a coordinated full editor build passes.
3. Treat the remaining response after presets 4 and 5 as the detail-normal contribution. Use the same camera/ROI and compare both spatial delta and surface-only MAE.
4. A final material change needs a same-camera rendered A/B and temporal camera-motion check. Compile-only or a static scalar apply/restore PASS is not a visual PASS.

## Non-regression matrix

| Adjacent scope | Result |
|---|---|
| Deterministic seed/profile handoff | PASS in all three logs (`resolverSig=813814363`) |
| Selected WorldScape LOD0 readiness | PASS, 10 LODs / 8,836 LOD0 vertices |
| Render/collision identity | PASS, centimetre-scale delta and `0.01 cm` heightfield delta |
| Landing/gravity/grounding | PASS, natural and physical surface checks |
| Material A/B apply/restore | PASS for presets 1 and 2; default restored before TestExit |
| APS-74 Water/Oceanic wet appearance | FAIL/open; not reclassified by this Frozen diagnostic |
| Final crawling/clipping visual acceptance | PARTIAL; temporal/manual movement evidence still required |

## Evidence paths

- `Saved/Logs/APS-77-MaterialAB-Baseline.log`
- `Saved/Logs/APS-77-MaterialAB-Preset1.log`
- `Saved/Logs/APS-77-MaterialAB-Preset2-Retry.log`
- `Saved/Automation/APS-77-MaterialAB-Evidence/Baseline_Current_MaterialAB0.png`
- `Saved/Automation/APS-77-MaterialAB-Evidence/Preset1_NearAllOff.png`
- `Saved/Automation/APS-77-MaterialAB-Evidence/Preset2_SeamNormalOff.png`

Current external blocker: the latest coordinated APS-80 compile gate failed on the foreign APS-81 `Gameplay/Production/APSProductionConsole.h` header. No further UE automation should run until that owner fixes the header and a coordinated editor build restores a clean shared compile state.

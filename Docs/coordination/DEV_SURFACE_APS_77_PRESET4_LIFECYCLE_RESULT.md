# APS-77 Preset 4 Lifecycle Diagnostic Result

Date: 2026-08-14  
Owner: Dev Surface  
Classification: **Lifecycle telemetry PASS; automation PASS; near-color visual isolation NOT VERIFIED**

## Run result

- Source checkpoint: `719c1443` (`[APS-77] trace WorldScape LOD lifecycle transitions`).
- Compile gate: UE 5.4 `APS_ALPHAEditor` PASS, 5/5 actions, 0 errors, DLL linked.
- Exactly one separately granted runtime process: PID `12676`, normal exit `0`, `35.9 s`.
- Exact test: `APS.Rendered.Gameplay.GeneratedCivilizationHandoff` with `MaterialAB=4`.
- Test result: `Success`; one test queue empty; report test state `Success`, errors `0`; TestExit status `0`.
- Preset apply and accepted-value restore both PASS.
- No retry or second UE process was launched.

Evidence:

- Log: `Saved/Logs/APS-77-MaterialAB-Preset4-Lifecycle.log`
- Report: `Saved/Automation/APS-77-MaterialAB-Preset4-Lifecycle`
- Fresh physical-surface screenshot: `Saved/Automation/APS-77-MaterialAB-Evidence/Preset4_NearColorOff_Lifecycle.png`
- Screenshot SHA256: `5897FBFD89759980A548BBAD7EF9A99F736B26D73B72C735F674121D57BAE26C`

## Exact lifecycle classification

The only predicate that changed during the run was the WorldScape root actor hidden state:

`rootHidden: 1 -> 0 -> 1 -> 0 -> 1 -> 0`

Every other logged contract remained stable across those transitions:

- mesh registered `1`
- component visible `1`
- component hidden-in-game `0`
- section count `3`, section 0 visible `1`
- TransformKeeper parent identity `1`
- root unit scale `1`
- LOD0 relative unit scale `1`
- player override active `1`
- pawn collision invoker present `1`
- override-to-pawn error `0.000 cm`

The natural and physical surface proofs both ran while `rootHidden=0` and passed. This resolves the earlier generic predicate failure: the previous preset-4 run sampled the same root during a transient root-hidden window. It was not a lost LOD, stale anchor, non-unit scale, detached TransformKeeper child, placement-anchor leak, or material scalar effect.

## Physical non-regression

- LOD0 vertices: `8,836`
- render relief: `2,687.57 cm`
- render/noise delta: `0.00 cm`
- render/collision delta: `10.39 cm`
- collision relief: `12,409.69 cm`
- collision vertices/LODs: `109,850 / 26`
- local collision relief: `158.31 / 467.73 / 913.29 cm`
- local collision/noise delta: `0.05 cm`
- physical surface result: PASS

Ground ROI in the fresh preset-4 frame:

- mean brightness `141.559`
- variance `1,069.957`
- P10/P90/spread `120 / 175 / 55`
- mean spatial delta `2.920`

These aggregate values are nearly identical to the earlier baseline (`141.482`, `1,071.284`, `120 / 175 / 55`, `2.914`).

## Visual A/B boundary

Cross-run pixel comparison against the older baseline produced full/surface/ground-ROI MAE `54.482 / 62.886 / 66.591`, but the sky alone also changed by MAE `44.145`. The large image delta conflicts with the nearly identical ground aggregate metrics and therefore reflects changed global render/lighting/composition state between non-paired runs. It cannot be attributed to the small `NearColorStrength` scalar.

Preset 4 therefore does **not** receive a visual PASS or FAIL. A valid attribution needs a paired baseline and preset capture inside the same execution, with the same generated world, camera, exposure, lighting state, and settled WorldScape root. Preset 5 should also wait for that paired-capture contract rather than producing another confounded cross-run screenshot.

## Owner boundary

- Surface owns the default-off telemetry and paired material diagnostic design.
- Shared WorldScape readiness owner should audit who toggles `AWorldScapeRoot::SetActorHiddenInGame` around camera/surface-fill transitions.
- APS-78 placement anchors remain correctly transient and are not implicated.
- No shared generator, streaming, test, material asset, or civilization file was edited by Surface for this result.

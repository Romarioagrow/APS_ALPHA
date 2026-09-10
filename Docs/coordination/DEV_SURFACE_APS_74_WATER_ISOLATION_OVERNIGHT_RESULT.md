# APS-74 Water Isolation Overnight Result

Date: 2026-08-14  
Scope: one granted UE 5.4 RenderOffscreen diagnostic with `-APSWaterIsolation` and `APS.Rendered.Gameplay.GeneratedCivilizationWetOceanHandoff`  
Classification: **FAIL / isolation not reached**

## Execution

- Exactly one owned Unreal process: PID `30656`.
- Normal process exit: `0`; `TestExit=0`.
- Wall time: `61.5 s`; automation duration: `33.201 s`.
- Minimum observed available RAM: approximately `13.98 GB`.
- No source or asset save/mutation; no second run.
- Heavy process count after exit: `0`.

## Evidence

- Log: `Saved/Logs/APS-74-WaterIsolation-Overnight.log`
- Automation report: `Saved/Automation/APS-74-WaterIsolation-Overnight`
- Prior fixed-name screenshots were preserved before launch in `Saved/Automation/APS-74-WaterIsolation-PriorFixed`.
- Report result: `0` succeeded, `1` failed, `0` not run.

The wet-ocean contract reached a coherent pre-render state:

- canonical `Water` / `M_APS_WorldScapeLivingWater` contract;
- `9` ocean LODs and `23` terrain collision LODs;
- ocean mesh geometry present with `4094` vertices;
- two open-water views selected with depth `[36137.03, 33840.89] cm`, clearance `[50000, 50000] cm`, direction dot `0.872846`, and camera travel `3213.12 km`.

The rendered proof then timed out on the composite ocean readiness predicate:

`authoritative ocean LOD is not yet centred/visible with exactly three sections lod=0 sections=3 mesh=WorldScapeMesh`

The failure occurred before `BeginWetOceanIsolation` and before any `[APS.Handoff.WetOcean.Isolation]` state capture. No fresh screenshot was produced. Existing fixed-name screenshots must not be treated as output of this run.

## Diagnosis boundary

The failing predicate combines:

1. complete centered payload;
2. effective component presentation (registered, component visible, not hidden in game, owner not hidden);
3. exactly three mesh sections;
4. visibility of all three sections.

The log proves the section count was `3`, but it does not identify which of the remaining predicate terms stayed false. This run therefore does not evaluate the isolated production-water palette/material and does not change APS-74 wet-appearance status. The result is a WorldScape ocean render-readiness blocker requiring lifecycle telemetry or an owner-approved contract handoff before any retry.

## Next safe step

- No unchanged-state retry.
- Keep shared smoke, streaming, generator, and material assets untouched until ownership coordination.
- If a future diagnostic is approved, add default-off transition telemetry that separates centered payload, actor/component hidden state, registration, and per-section visibility, then run one fresh paired isolation capture.

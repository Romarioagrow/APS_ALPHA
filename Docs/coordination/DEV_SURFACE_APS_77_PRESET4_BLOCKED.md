# APS-77 Preset 4 Near-Color Isolation — Blocked Run

Date: 2026-08-14  
Owner: Dev Surface  
Classification: **BLOCKED / automation FAIL; material apply/restore PASS; visual isolation NOT VERIFIED**

## Granted run

- Exactly one UE 5.4.4 `-RenderOffscreen` process was launched after Project Command grant.
- Owned PID: `8468`; normal process exit code `0`; wall time `36.1 s`.
- Test: `APS.Rendered.Gameplay.GeneratedCivilizationHandoff`.
- CVar: `aps.Surface.Diagnostics.MaterialAB 4` (`near-color-off`).
- Preflight: heavy owners `0`, free RAM `19.9 GB`, free F: disk `482.14 GB`, editor DLL present (`7,820,800` bytes).
- Minimum observed free RAM during the run: approximately `14.16 GB`.
- No retry or second UE process was launched.

## What passed

- Preset applied to `MID_MI_APS_WS_Cryogenic_0`:
  - `NearColorStrength=0.0`
  - `DetailNormalStrength=0.1225` preserved
  - `DetailRoughnessStrength=0.0236` preserved
  - seam parameters `2400 / 5000 / 0.880` preserved
- Deterministic gameplay hierarchy and civilization placement reached their normal handoff state.
- Initial visible WorldScape LOD0 proof passed:
  - 10 render LODs
  - 8,836 LOD0 vertices
  - 2,662.820 cm render relief
  - 0.002 cm render/noise delta
  - 99.495 cm observer centre offset
- A fresh non-blank handoff screenshot was written: mean brightness `64.005`, variance `3,734.556`, non-black ratio `0.54886`, CRC `952277687`.
- The subsystem restored the accepted MID scalar values before TestExit.
- Automation queue emptied, report exported, TestExit observed, and the owned process exited cleanly.

## Blocking failure

The exact test result was `Fail` (`0 succeeded / 1 failed`, two errors):

`natural production landing has no matching visible WorldScape LOD0: terrain render LOD0 is not an effectively visible unit-scale TransformKeeper child`

Primary evidence is in log lines approximately `1313`, `1319`, `1426`, and `1428`. The failure occurred about 0.77 seconds after the initial LOD0 proof passed and after the first viewport capture, but before the natural/physical surface proof and ground-ROI screenshot.

The failing predicate checks registered/visible/hidden state, section 0 visibility, root hidden state, TransformKeeper parent identity, root unit scale, and LOD0 relative unit scale. Read-only blame shows that predicate is unchanged since `d35ef7432`; the current dirty test changes do not edit it. Preset 4 changes only one MID scalar and cannot establish which lifecycle predicate changed.

This evidence therefore indicates a transient WorldScape visibility/scale/attachment lifecycle regression or race between the initial proof and natural-surface proof. It does **not** establish that near color is or is not the dominant crawling contributor.

## Screenshot boundary

The run failed before writing a new `APS_GeneratedCivilization_PhysicalSurface.png`. The older physical-surface file was explicitly rejected and was not retained as preset-4 evidence.

The only fresh frame is the earlier handoff capture:

- `Saved/Automation/APS-77-MaterialAB-Evidence/Preset4_NearColorOff_Handoff.png`
- SHA256 `B447C60ED69AA3E978F2649258019858E8EB517B908D51B0FE8A7E4C90EDCA76`
- 212,931 bytes; timestamp `2026-08-14 07:01:37 +05:00`

It is not the same physical-ground acceptance frame used for presets 1 and 2, so no same-camera surface MAE, ground ROI, or visual PASS is claimed.

## Evidence paths

- Log: `Saved/Logs/APS-77-MaterialAB-Preset4.log`
- Report: `Saved/Automation/APS-77-MaterialAB-Preset4`
- Fresh handoff frame: `Saved/Automation/APS-77-MaterialAB-Evidence/Preset4_NearColorOff_Handoff.png`

## Resume condition

Do not repeat preset 4 or run preset 5 through this rendered handoff test until the shared WorldScape/handoff owner changes the visibility lifecycle state or provides new evidence that the transient failure is resolved. A new run still requires a separate global heavy-slot grant.

# Dev Surface APS-73 / APS-77 checkpoint

Date: 2026-08-14

Result: **PARTIAL**. Source/build/runtime telemetry PASS; visual crawling fix and same-camera A/B remain open.

## Done and checked

- Added an opt-in, default-off World subsystem that reports the selected player-bound WorldScape root, anchor error, LOD0 publications, snap state, mesh counts, material identity and world origin.
- No shared WorldScape, generator, profile, material, foliage or UI file was changed.
- `APS_ALPHAEditor Win64 Development` built and linked successfully after removing one duplicate WorldScape header include. Final incremental build time was 12.6 seconds; only the known compiler-version warning remained.
- Offscreen PIE automation `APS.Rendered.Gameplay.GeneratedCivilizationHandoff` completed with `Success` in 4.74 seconds.
- The report is `succeededWithWarnings` only because existing generation, gravity and save messages use warning severity. No automation error was reported.
- Frozen-planet runtime evidence used `MID_MI_APS_WS_Cryogenic_0`; player-to-WorldScape anchor error remained `0.000 cm`; LOD0 remained at 8,836 vertices and 51,894 triangle indices; the material instance did not churn.
- Four LOD0 publication states were observed, including relative-center shifts of `49,980.983 cm` and `119.909 cm` against the current nominal ground half-step of `60 cm`.

## Finding

The runtime evidence supports the LOD resnap/publication plus camera-dependent shading hypothesis. It rules out player-anchor drift and material-instance replacement in this run, but does not yet prove whether geometry replacement, normals/semantic vertex payload, or depth/distance fades produce the visible crawl.

## Evidence

- `Source/APS_ALPHA/Core/World/APSPlanetSurfaceDiagnosticsSubsystem.h`
- `Source/APS_ALPHA/Core/World/APSPlanetSurfaceDiagnosticsSubsystem.cpp`
- `Saved/Logs/APS-77-AnchorDiagnostics-Run2.log`
- `Saved/Automation/APS-77-AnchorDiagnostics-Run2/index.html`
- `Saved/Automation/APS-77-AnchorDiagnostics-Run2/index.json`

Resource checkpoint after the run: no UnrealEditor, UnrealEditor-Cmd or ShaderCompileWorker owned by this task remained; approximately 15,524 MB free RAM and 482.2 GB free on F:.

## Remaining

- Capture same-seed, same-camera Lit/Unlit/Wireframe and temporal A/B evidence before changing relief or shared material code.
- Separate geometry publication motion from normal, roughness and depth-fade instability, then make the smallest owner-approved fix.
- Extend the same evidence matrix to APS-74 Desert/Frozen/Living parity while preserving Ocean/Living and Land Coverage baselines.

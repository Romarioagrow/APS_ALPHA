# APS-76 runtime style contract

The `FAPSUIStyle` foundation is presentation-only. Consumers may request palette, typography, spacing, control geometry, brushes and transition durations. They may not pass model/save/placement/spawn state into the style layer.

## Integration order after ownership release

1. Replace duplicated palette/font helpers in `SWorldGenerationPanel.cpp` while preserving its transparent root and existing 25/50/25 flow.
2. Apply the shared 44 px hierarchy geometry, fact chips and 20/24 px slider thumb style without changing delegates or `UWorldGenerationViewModel` semantics.
3. Apply the same tokens to `SAPSStrategicMapPanel.cpp` without changing focus/orbit/zoom input.
4. Keep Colonization and gameplay menu as presentation mockups until their existing UMG/runtime files receive a separate ownership handoff.

## Stable spaceship identity boundary

When the later UI displays the selected home spaceship, the value comes from committed `USpawnParameters::BP_HomeSpaceship`. Display/debug identity uses the exact generated class path exposed by the canonical reference (for example `GetPathName()`), not a reconstructed asset label. Commit must not normalize, shorten or replace the selected class path.

## Non-regression gates

- root viewport remains fully transparent;
- SYSTEM composition, selection, camera focus and smooth transitions remain unchanged;
- keyboard/controller focus is visible independently of hover/selection;
- reduced motion removes decorative timing without changing navigation state;
- no UI calibration profile changes scene rendering;
- exact slider and spin-box values agree in the same frame;
- same-camera Generation and Strategic Map A/B is required before visual PASS.


# Dev Surface APS-78 placement API checkpoint

Date: 2026-08-14

Result: **PARTIAL**. UHT and both new Surface compilation units PASS. Full editor link and runtime acceptance are pending two unrelated Civilization-owned compile fixes.

## Surface-owned API

- `TryResolveCivilizationFootprint(HomeBody, Request, Result)` consumes the active canonical WorldScape root/profile and defaults to an 80 x 55 metre base, 90 metre pad and 140 metre separation.
- `CandidateOrdinal`, `PlacementKey`, base/pad transforms and the planet-relative outward/forward/right frame depend on manifest seed plus canonical body surface seed/type/radius and sampled canonical terrain. They do not depend on `Root->OverridedPlayerPosition`.
- The resolver samples base perimeter, pad perimeter and a three-probe-wide route. It rejects liquid clearance below 10 metres, structure slopes above 0.12 and route slopes above 0.20 by default.
- `RequestPlacementAnchors` adds three idempotent transient WorldScape collision invokers at base, pad and route midpoint. It does not write the player/visual override, does not force-reset WorldScape and does not spawn civilization gameplay actors.
- The caller retries `TryResolveCivilizationFootprint` until canonical LOD0 is published and spatial base/pad/route collision traces match sampled height, then performs actor overlap finalization.
- `ReleasePlacementAnchors` removes only transient invokers matching the placement key after materialization or cancellation.
- Foliage clearance is explicitly reported as false/no-op in the foundation slice.

## Verification

- UnrealHeaderTool: PASS, 14 generated files.
- `APSPlanetSurfacePlacementResolver.cpp`: compile PASS.
- `APSPlanetSurfacePlacementResolverTests.cpp`: compile PASS.
- Full `APS_ALPHAEditor Win64 Development`: BLOCKED before link by Civilization WIP errors in `APSCivilizationRuntimeManifest.cpp` and `APSCivilizationMaterializationSubsystem.cpp`; neither error originates in Surface files.
- Focused runtime automation: pending an unblocked editor build. Compile-only is not a runtime or visual PASS.

## Files

- `Source/APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.h`
- `Source/APS_ALPHA/Core/World/APSPlanetSurfacePlacementResolver.cpp`
- `Source/APS_ALPHA/Tests/APSPlanetSurfacePlacementResolverTests.cpp`

## Non-regression

- No WorldScape plugin, shared generator/profile/material/foliage, save/controller, UI or Civilization actor file was changed.
- Player landing/gravity and visual WorldScape anchor semantics remain unchanged.
- No artificial gravity, foliage deletion, actor spawn, save mutation or global-Z orientation is introduced.

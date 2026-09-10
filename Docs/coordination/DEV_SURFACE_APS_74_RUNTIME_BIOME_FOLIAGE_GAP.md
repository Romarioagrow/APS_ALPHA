# APS-74 runtime biome and foliage evidence gap

Date: 2026-08-14  
Audit mode: read-only while the coordinated build freeze is active  
Result: **PARTIAL**

## What current evidence proves

The accepted seed-73991 menu/orbital gallery proves that the canonical resolver does not collapse the requested comparison families to one fallback profile:

| Fixture | Canonical mapping | Profile signature | Resolved terrain | Preview pixel evidence |
| --- | --- | ---: | --- | --- |
| Frozen | `Ice -> Cryogenic` | `1536386311` | `MI_APS_WS_Cryogenic` | mean `183.235`, variance `3480.142` |
| Desert | `Desert -> Desert` | `18392993` | `MI_APS_WS_Desert` | mean `182.126`, variance `3782.333` |
| Water | `Water -> Oceanic` | `830115675` | `MI_APS_WS_Oceanic` + water | mean `93.442`, variance `2957.494` |
| Living | `Forest -> Biosphere` | `396759706` | `MI_APS_WS_Biosphere` + water | mean `67.352`, variance `1003.733` |

All use body `SYS0/S0/P0`, seed `73991`, radius `6750 km` and the same six surface controls including Land Coverage `1.0`. The PNG identities and pairwise deltas are recorded in `DEV_SURFACE_APS_74_GALLERY_PIXEL_EVIDENCE.md`.

The seed-424242 `Water -> Oceanic` body remains the only canonical orbit-to-full-scale parity truth. The gallery fixture must not replace it.

## What is not yet proved

- `APS.Rendered.Gameplay.GeneratedCivilizationHandoff` exercises the full-scale Frozen path.
- `APS.Rendered.Gameplay.GeneratedCivilizationWetOceanHandoff` exercises the full-scale Water path.
- No registered equivalent currently exercises Desert or Forest/Biosphere through the same menu -> immutable model -> gameplay WorldScape -> physical ground screenshot chain.
- Consequently, preview family diversity is PASS, but Desert/Frozen/Living **ground** diversity and same-camera orbit-to-ground continuity are not fully verified.

## Biosphere-driven foliage classification

The production foundation exists:

- resolved profiles carry Biomass, Biodiversity and deterministic foliage settings in their signature;
- `FAPSWorldScapeFoliagePolicy` gates full-scale fresh-root activation, sanitizes mesh-only entries, rejects Blueprint/actor spawning, applies hard collection/type/instance/cluster budgets and forces collision off;
- `APlanetarySurfaceGenerator` applies the policy only to a fresh owned runtime root;
- source automation covers activation gates, transient budget normalization and fresh-root application.

The current production baseline intentionally keeps every native/catalog archetype `Foliage.bEnabled=false`, the global `aps.WorldScapeFoliage.Enable` switch defaults to `0`, and no accepted living-world runtime render demonstrates instantiated foliage. Therefore the policy foundation is source-covered, while visible biosphere-driven foliage materialization remains **BLOCKED/default-off**, not PASS.

## Required acceptance evidence

1. Preserve seed-424242 Ocean/Living identity and Land Coverage controls.
2. Add deterministic Desert and Forest/Biosphere variants of the existing full-scale handoff proof, with unique artifact paths.
3. Capture orbit/menu profile identity and same-camera physical ground evidence for Frozen, Desert and Living.
4. Compare canonical family/material signature, relief/LOD0/collision readiness, ground color/variance/spatial-detail metrics and same-condition frame time.
5. For foliage, author a reviewed mesh-only soft-reference collection for Biosphere, keep runtime opt-in explicit, verify deterministic placement and budgets on a fresh root, and provide a close-ground A/B plus RAM/VRAM/frame-time evidence before changing the default.

No shared smoke, generator, catalog or asset was changed by this audit.

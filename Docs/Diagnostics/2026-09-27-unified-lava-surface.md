# WorldScape: double-floor audit and isolated lava candidate

## Status

Source candidate only. **Not enabled in the game; no material has been baked.**
`aps.Surface.UnifiedLavaSurface` defaults to `0`. The accepted assets and engine
WorldScape DLL are unchanged. No editor launch, project build, hot reload, or
shader bake was performed: the editor/build window remains assigned to Claude.
Ships, character, AstroGenerator, maps, station assets and coordination ledger
were not edited by this task.

## Evidence and scope

The reported BEM gameplay log identifies Lava, seed 1337, radius 6750 km,
full-scale WorldScape, terrain/ocean 10 x 192, with runtime terrain collision.
The generator emits constant-radius ocean geometry and displaced terrain below
it. PlanetaryBodyStreaming explicitly disables collision on ocean components.
Thus the visible opaque lava top is not the collision floor. This explains the
reported descent through a flat top into dark terrain. It does **not** establish
the cause of the separate black rectangular strips or prove all other planets
show the same visual symptom.

Source audit of all 35 saved type IDs, including legacy/hidden presets:

| Default liquid route | Count | Presets |
| --- | ---: | --- |
| Lava + terrain | 3 | Lava, Melted, Volcanic |
| Water + seabed | 11 | Terrestrial, Pangea, Nordic, SuperEarth, HighMountain, Ocean, Water, Archipelago, Forest, Oasis, Savanna |
| Ammonia + seabed | 1 | Ammonia |
| No ocean | 17 | Rocky, Dwarf, Greenhouse, Desert, Sand, Ice, Frozen, Tundra, Rogue, Metal, Metallic, Carbon, Exoplanet, Unknown, Basalt, Sulfur, Crystal |
| No solid WorldScape surface | 3 | HotGiant, GasGiant, IceGiant |

This is code/default-profile coverage, **not 35 rendered landings**. Custom
catalogs/modifiers can alter a resolved profile; selection uses the resolved
liquid, not a hard-coded type list. Frozen's earlier settled views cannot verify
water/lava transitions. Actual water and ammonia require their upper liquid
surface: removing it globally would erase oceans. Their immersion behavior is
outside this patch and must not be silently changed to solid ground.

## Candidate design

- Full-scale generated lava roots only; authored/custom stacks and preview
  globes are excluded. Other liquids and dry worlds remain on the old path.
- One WorldScape noise field returns `max(rockHeight, lavaDatum)` for BOTH terrain
  rendering and terrain collision. No hidden lower collision floor in lava basins.
  Land above the datum, seeds, profile data and RGB climate/palette payload stay
  unchanged. This represents an opaque crust-like lava surface, not swimming,
  buoyancy or damage mechanics.
- A new protected material duplicates the accepted Magma terrain graph and
  accepted Lava graph. Their physical coordinate frame is shared; inherited lava
  parameters are frozen and namespaced to prevent terrain parameter collisions.
  Source packages are never edited, and existing candidate package names are
  refused rather than overwritten.
- The blend uses vertex-stage radial height, not pixel chord depth or WorldScape
  Hole alpha. Its small radius-dependent tolerance covers float quantization.
  Coast blending, additional interpolator/shader cost and performance still need
  actual frames and measurements. Pixel-identical orbital appearance is NOT proven.
- The new material, clamped noise and `bOcean=false` publish together before
  workers run. If the new opaque material/LocalVF shader is missing or incomplete,
  the entire old terrain+lava pair remains in use. No partial flattening/removal.
- Existing WorldScape worker synchronization and shared liquid lattice fixes stay
  intact. No plugin ABI or mesh payload change is required by this candidate.

## Checks completed without Unreal

- `Tools/Diagnostics/UnifiedLavaSurface/CheckMath.ps1`: PASS, 300003 envelope
  cases (positive/negative sea levels, identity when disabled, idempotence and
  eligibility guards). This tests the policy math, not an engine mesh.
- `CheckSyntax.ps1`: the noise, streaming, material commandlet and new regression
  test compile in `/Zs` mode against current UE 5.4 headers/PCH. This is not a UHT,
  module/link, shader, or runtime test.
- Added `APS.Gameplay.World.PlanetSurface.UnifiedLavaFamilyMatrix`: 35 type IDs,
  32 solid profiles x 2 seeds x 128 samples; checks family guards, exact envelope
  height and unchanged material channels. **Compiled only; not executed yet.**

Check outputs are on `F:/ChatGPT/APOSFERA/work/unified_lava_20260927/checks`.

## Next authorized Unreal window

1. Confirm Claude has released the editor/build window; do not close his process.
2. Build APS_ALPHAEditor and restart normally. Run the new family matrix and
   existing planet profile/noise determinism tests.
3. In a protected isolated bake invocation of APSPlanetSurfaceAssetCommandlet,
   use `-OnlyUnifiedLavaSurface -AllowCommandletRendering`. Output is ONLY
   `/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava`. Refuse existing candidates;
   do not delete or overwrite accepted Shared / SharedLiquid assets.
4. Set `aps.Surface.UnifiedLavaSurface 1` before creating the gameplay root (PIE
   stopped). Require `[APS.UnifiedLava] installed=1 ocean=0` and the exact new
   terrain parent, but do not count readiness logs as visual proof.
5. Compare BEM Lava seed 1337 and Melted/Volcanic: orbit, vertical approach,
   coast crossing, low flight, contact and return to orbit. Confirm one visible
   surface, matching collision, retained geography/style, no strips, and paired
   frame time/FPS. Check representative Water/Terrestrial, Ammonia and Frozen
   regressions; broader all-type landings remain pending.
6. Only after acceptance promote the default. Rollback is CVar `0` with PIE
   stopped/new root; no profile/save migration is involved.

# CoastalRelief V2: paired rendered comparison, not production

29 September 2026, 14:04 local. The V2 source that was previously unverified
has now been built and rendered. **Decision: retain as a disabled candidate.**
It substantially reduces the dense wet/dry fragmentation in this Terrestrial
coast, but it does not solve the hard blue-water boundary or angular mesh edge.
The close coast still loses too much small-scale shape. Do not publish from
the numerical improvement alone or claim that the planet epic/P0 is complete.

## Scope and chain of evidence

- Diagnostic-only changes: `APSWaterNormalABProbe.h` adds geometry-only capture
  (native water phase, then restore); `RunPlanetWaterNormalAB.ps1` adds that
  mode, ordinary atmosphere, coastal contracts, stale-DLL/process/headroom
  checks and manifests for Shared/SharedLiquid/ContinuityV1.
- GeometryOnly does not run the previously rejected water-normal-strength
  variants. Normal A/B without this option retains its existing route.
- One diagnostic build: 4 actions, max2, 16.02s total; successful.
- DLL SHA256: `ECE8886B60FC95B6B3FEC6A30DC71C8F93569062C14DD4CBF3F4536A9C6FAB5A`.
- No material bake, production geometry change, palette/light adjustment,
  density increase, plugin replacement or ship-file edit in this pass.
- Existing ContinuityV1 default for 14 types remains installed. The coast
  candidate remains OFF in ordinary gameplay; the historical development flag
  `-APSProbeCoastalReliefV1` selects the current V2 implementation in these tests.

All runs are under:
`F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/`.

| Directory suffix after `terrestrial-coastal-v2-` | Coast field | Sea-relative camera height | Finished local | Tests |
|---|---|---:|---|---|
| current-2km | V2 | 2 km | 13:55:25 | 3/3 |
| control-2km | native | 2 km | 13:57:04 | 3/3 |
| current-100km | V2 | 100 km | 13:58:27 | 3/3 |
| control-100km | native | 100 km | 14:03:41 | 3/3 |

These are three distinct tests repeated four times: two coastal contracts and
one rendered capture test. Each report has one success, two warning results,
zero failures and zero in-process tests. Warnings include temporary test-world
actor destruction and existing generator/gravity/animation diagnostics;
shipyard missing static-mesh bounds is not repaired by this planet experiment.

Every process used the same seed 424242, radius 637100032 cm, scale 478 and
intensity 924956. V2 selected a coast point with 0.5m depth, then both controls
replayed its exact camera frame, rather than finding their own shoreline:

```
direction = (0.32697962221269872, 0.9222597751647279, 0.20620677430857209)
tangent   = (0.76553284379257736, -0.13054967427422487, -0.63001289480592648)
```

The native depth at this same point is about 7.007m, as expected when comparing
two terrain fields. Tangent re-normalization differs by about 1e-15. Height is
relative to the same sea datum, not the locally changed terrain height.

Atmosphere logs in all four runs: GeneratedWorldCDO, height100km, opacity1,
multiscattering1, rayleigh8. Runtime terrain binds
`ContinuityV1/MI_APS_ContinuousTerra`; water binds
`SharedLiquid/MI_APS_SharedWater`, native wave strength0.025.
Each run has exactly three Water0Native captures, three successful unchanged
payload checks and three binding/freeze/tick restorations, no Water1..5 images.
`Water0Native` refers to the WATER MATERIAL, not the terrain field.
All 23 protected asset hashes remain unchanged against every run's manifest.

## Rendered result

All 12 paired coast screenshots were inspected: nadir, oblique and limb at
2km and 100km, control versus V2. These are static views, not a continuous
flight, walking, floating-origin or high-speed acceptance.

- At 100km the native fragmented region reproduces the user's many blue
  cutouts/islands. V2 gives a connected continental edge with some coves and
  islands. The improvement is visible in all three orbital angles.
- At 2km, especially the grazing view, repeated wet/dry breaks are reduced.
  The close boundary remains too smooth; this is not a finished natural beach.
- The opaque sand/blue boundary is still hard, individual edges still angular,
  and the broad white water glint remains. No shallows, wet-sand blend, cloud
  improvement or overall palette acceptance is claimed.
- Water and Oasis are covered numerically, not by rendered frames in this pass.
  No all-family visual extrapolation is justified.

### Direct comparison frames

- [100km native nadir](F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/terrestrial-coastal-v2-control-100km/Saved/Screenshots/Windows/APS_SurfaceLighting_1_OrbitNadir_Water0Native.png)
- [100km V2 nadir](F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/terrestrial-coastal-v2-current-100km/Saved/Screenshots/Windows/APS_SurfaceLighting_1_OrbitNadir_Water0Native.png)
- [2km native grazing coast](F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/terrestrial-coastal-v2-control-2km/Saved/Screenshots/Windows/APS_SurfaceLighting_1_OrbitLimb_Water0Native.png)
- [2km V2 grazing coast](F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928/terrestrial-coastal-v2-current-2km/Saved/Screenshots/Windows/APS_SurfaceLighting_1_OrbitLimb_Water0Native.png)

## Numerical contracts, not visual scores

Nine same-patch grids, 3 seeds (1337/424242/911) x Terrestrial/Water/Oasis,
500m spacing over32km: native sign-change edges9932, V2 edges2997.
Maximum full/height-sampler difference2.91038305e-11cm. Bounds, exact inland
path, excluded Ice/Rocky/Lava, compressed previews and worker/value snapshot
height/material/climate/water/foliage equality passed. This supersedes only
the former 'V2 not built/tested' status, not the historical V1 numbers.

The exact rendered location was also sampled on a129x129 grid,250m spacing:

| Field | Wet fraction | Sign-change edges | Land/water components | Enclosed islands/lakes |
|---|---:|---:|---:|---:|
| native | .550568 | 2468 | 38 / 43 | 25 / 35 |
| V2 | .582838 | 403 | 5 / 1 | 2 / 0 |

Four-neighbour field counts are not counts of visible holes, mesh triangles,
or a naturalness score. Both heights reproduce the same field counts.

## Performance and remaining work

Observed game-delta means were 8.766..8.887ms, p95 9.265..10.373ms; this is
the natural fixture phase before the frozen coast captures, with startup
outliers above200ms. **It is not a GPU/Present benchmark or a measured V2
cost.** No FPS or performance-regression acceptance follows from these runs.

Next scoped implementation should address the missing physical-depth-to-water
material transition without another blind amplitude/terrain-density sweep.
The retained-preview UV1 diagnostic is not proof that live WorldScape receives
physical depth. Inspect and supply that live channel narrowly, preserving
legacy fallback and unchanged geometry/normal/glint where not under test.
Do not install an old broad plugin overlay containing unrelated experiments.
Coast promotion still needs compatible orbital/surface consumers, saved-world
and collision consistency, near-ground appearance and measured live cost.

Last isolated UE15032 exited14:03:41. No UE/compiler remained; editor window
returned14:04. No more GPU runs in this slot. Full goal remains ACTIVE.

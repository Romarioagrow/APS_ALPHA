# Coastal relief candidate — disabled by default

This is a measured geometry candidate, **not a published visual fix**. The
production material assets, seeds, palette transfer, sea datum, grid resolution,
LOD lifecycle and ships are unchanged. The verified orbital macro fix remains
`ce6701fe`; do not confuse this experiment with that installed material change.

## Cause isolated

Full-scale physical bands retain a nonzero amplitude at the continental coast.
They cross sea level repeatedly, although the compressed preview deliberately
omits those physical bands. A higher mesh resolution alone cannot remove real
wet/dry sign changes. The first measured 32km coastal grid had 40 enclosed dry
components; this is sampled terrain fragmentation, not a count of visible holes.

## Implementation

`APSCoastalRelief::Height` keeps 35% of the existing 18km regional band as a broad
coastal landform and smoothly bounds the remaining physical detail to 65% of its
sea clearance. The bound is fully active at abs(continental signed field)<=0.025,
blends out to 0.12, and returns the exact old height outside that range. It does
not remove later authored crater/preset basin contributions. No additional noise
evaluations, vertices, surfaces, material layers or saved-profile fields.

Only Water-liquid Terrestrial/Water/Oasis profiles with partial land coverage
are eligible. All normal entry points default OFF. The immutable per-instance
flag is captured before any worker runs; the full and height-only value samplers
take the same explicit flag. No mutable worker CVar or per-LOD height function.
`-APSProbeCoastalReliefV1` enables the isolated development-test root route.
This flag is not a supported user setting or a production migration.

## Numerical evidence

Build `work/planet_water_normal_20260928/coastal-relief-build-v1.log` succeeded,
followed by `coastal-contract-v1`: 2 tests, 0 failures (one with asset warnings).
Tests cover 3 seeds (1337, 424242, 911) in Terrestrial/Water/Oasis and excluded
Ice/Rocky/Lava. They check finite bounded detail, continuity, exact distant
behavior, unchanged excluded families/compressed previews and full-vs-height
sampler error <=1e-6cm (observed maximum 2.18e-11cm).

Across nine paired 32km grids at 500m spacing, native/candidate sign-change edges
were 9775/1937. These counts are numerical evidence, not an art or FPS score.
`coastal-relief-build-v1b.log` additionally compiled worker/palette equivalence
assertions and exact camera-frame replay for the rendered comparisons.
The final `coastal-relief-build-v1c.log` preserves the original physical height
for climate cooling and defers the coast displacement until after authored
basins/craters. `coastal-contract-v1c` passed both tests (one asset-warning result,
zero failures, 0.51s): native worker/full/height-only agreement and exact palette/
temperature/humidity equality are now explicitly checked. Rendered frames below
are from v1b, before that climate-preservation correction; v1c needs a fresh
render before any production publication.

## Rendered evidence and decision

All runs are under `F:/ChatGPT/APOSFERA/work/planet_water_normal_20260928`.
The fixture uses real generated WorldScape geometry and the unchanged native
water MID. Only compare `Water0Native` between processes: its name means the
native WATER MATERIAL, not the native terrain field. No wave tuning is promoted.

- `terrestrial-coastal-relief-v1-2km`: successful, but the 20m-deep selected point
  was too far from the new gentle shore. Its all-wet grid is not shore acceptance.
- `terrestrial-coastal-relief-v1b-2km`: selected depth 0.5m. Actual grid has
  wet fraction .618833, 186 sign-change edges, one land and one water component.
- `terrestrial-coastal-native-sameframe-2km`: original field at the exact logged
  direction/tangent, radius/seed/height unchanged. Same grid has wet fraction
  .559822, 2489 sign-change edges, 40 land / 48 water components (30 enclosed
  islands / 40 enclosed lakes at 250m sampling).
- `terrestrial-coastal-relief-v1b-100km`: orbital inspection of the same coast.
  The dense fragmented boundary becomes a coherent shore with larger bays, but
  individual mesh edges remain angular. The 2km view loses too much fine coastal
  variation, and the hard opaque blue/sand boundary is still conspicuous.
- `terrestrial-coastal-native-sameframe-100km`: original field, successful test.
  Inspected nadir/oblique frames reproduce the user's dense fragmented coast;
  the candidate's corresponding views replace it with a coherent continental
  edge. Both processes use the same logged camera direction/tangent and height.

## Publication prerequisite

The instance flag must also be carried explicitly into any immutable profile
sampling consumers before a default is enabled. In particular, the closed
orbital mesh in `AstroGenerator.cpp` currently calls the static sampler with the
default OFF argument. Its compressed case is unchanged, but do not assume every
caller is compressed or publish a second inconsistent full-scale representation.
Do not simply flip the development command-line gate or static defaults globally.

Do not enable the default from component counts alone. The next refinement must
retain restrained intermediate-scale coves and address the separate shallow-water
optical transition. Water normal reduction was rejected in the companion report.
No all-family, walking, high-speed, floating-origin or performance acceptance is
claimed. The candidate is reversible and remains outside normal gameplay.

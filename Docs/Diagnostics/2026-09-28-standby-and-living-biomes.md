# WorldScape: standby, orbital squares, living biomes

Status: **source changes only, not built or executed**. Rio explicitly owns builds
and editor testing. No editor, commandlet, bake, automation run or shader compile
was launched. No ship, character, station, menu/generator, save or binary asset
was edited in this pass. The earlier dirty work remains in place.

## User evidence and scope

- 03:20-03:23 screenshots: FEZ/Oasis has a rectangular detail patch and repeated
  surface pattern; JUYIRUFI/Rocky moon appears as a gray globe; bright cyan limb.
- 03:05-03:11 screenshots: BEM/Terrestrial has uniform green land, blue ocean,
  missing sandy shores and weak biome separation compared with the supplied
  Elite Dangerous reference. This adds to, not replaces, the earlier defects.
- The inspected log already used terrain/ocean resolution 256. Increasing it
  again is not an explanation or a verified remedy for these defects.
- UTC 22:23:22.770: subsystem freezes parent and selects moon. 22:23:23.014:
  an external request resumes the parent with 10 workers. This is NOT evidence
  that the selection subsystem itself selected the parent again. No moon-ready
  event appears before session exit 22:23:29. Code contains two placement paths
  capable of reactivating the home body (search and anchor refresh).

## Changes installed in source

### Surface lifecycle

- Preload previously allocated/profiled a hidden root without actually generating
  its surface. After the active body publishes, the subsystem now starts one
  collision-free sibling producer. Its observer stays fixed until its initial
  payload passes the existing strict render/height/material checks.
- Publication transitions Preloaded -> FrozenVisible without losing the ready
  latch. The frozen state is established before refreshing visibility. First
  activation still cannot publish incomplete geometry.
- At most one speculative producer runs at once. Both hidden allocations and
  completed speculative warmups are bounded by MaxStandbyRoots (default 4).
  Already visited published surfaces remain governed by the existing retention
  policy. This is NOT a total GPU-memory cap; leaving a family still unloads it.
- Search and anchor refresh defer to the streaming owner while another body is
  selected. A frozen home is not restarted by placement polling. Cached search
  and existing anchor identity survive the pause; collision readiness is false
  until a return revalidates the site. Initial bootstrap and unstreamed/manual
  bodies retain their existing route.
- No changes to async worker ownership, no blocking joins, no all-planet mesh
  regeneration, no additional collision producer for warmup.

### Living terrain, same shared material in both contexts

Only Water + Temperate/Oceanic/Biosphere semantic mapping is retuned:

- Above-sea display height reaches .08.. .86 over the authored land span, instead
  of compressing that span to .08.. .47. It remains the low-pass palette field,
  not the full-frequency displacement. Data.Height and coastline geometry are
  unchanged. This is not a new terrain shell.
- Existing seeded humidity noise gets greater regional range, with a bounded
  subtropical inland drying term. No additional fBm or GPU texture sample is
  added. This changes the humidity-derived foliage mask too; it is not a change
  to physical height or liquid coverage.
- The native shore mask used shift -.509154 and contrast 59. Its inland mask was
  already saturated at the APS sea datum R=.08 for small variation. Generated
  SharedTerra living instances now use shift -.40 / contrast 12: sand at the
  datum, blended inland transition. Actual beach width/alignment remains a
  rendered check because R is a low-pass field and geometry has finer relief.
- ShiftSavhana=-.18 delays full wet-grass coverage. Exposed top/dry rock endpoints
  mix 70% of the authored slope/stone color into Highland. Vegetation Color3 and
  the separate Peak/snow endpoint are preserved. This does not claim that every
  high-elevation pixel now becomes rock: native layer masks still determine it.
- Native reference/bespoke parents and non-living palettes do not receive the
  living shoreline or palette overrides. No saved MIC/graph mutation or bake.

### Orbital shading and atmosphere

- Generated shared terrain's existing physical-distance normal filter now blends
  over 2..20 km instead of leaving mesh-dependent slope/projection weights active
  into orbit. Fine near-ground shading below 2 km is unchanged. This addresses a
  plausible source of the square shading boundary, NOT proof that geometric
  clipmap seams or texture tiling have all disappeared.
- Generated airglow is reduced to .0018.. .007. Rayleigh/Mie, haze height, optical
  density and sample counts are not changed. Explicit saved/body overrides remain
  authoritative. The cyan limb can also involve scattering/lighting; no rendered
  attribution of every bright pixel is claimed.
- Mesh resolution remains 256. Warmup adds bounded background CPU/GPU-memory cost;
  no FPS improvement or restoration of 120 FPS is claimed.

## Source-only regression coverage

Added/extended tests (NOT run):

- `APS.Gameplay.World.PlanetSurface.StreamingSelectionAndStandby`: one hidden
  collision-free warm producer, fixed warmup observer, remote placement cannot
  reclaim an unloaded family, existing published retention behavior.
- `APS.Gameplay.World.PlanetSurface.ResidentResume`: standby publication survives
  freeze, plus existing planet/moon return and profile invalidation cases.
- `APS.Surface.Placement.ReadyActivePreservation`: frozen site/anchor polling does
  not resume rendering/collision; unknown anchors are not falsely ready.
- `APS.Materials.SharedTerrain.LivingBiomeTransfer`: bounded monotone display
  height, dry/wet variation, native shore transfer algebra, family exclusions.
- `APS.Gameplay.World.PlanetSurface.LivingPaletteDetail`: stone tint scope and
  existing palette invariants.
- `APS.Gameplay.Generation.AtmosphereAirGlow.SubordinateEmission`: emission bounds.

Read-only diff/whitespace inspection was performed. It is not compilation or
rendered validation. State-machine tests use synthetic readiness, not real mesh.

## Still open / user-side validation after build

1. Revisit FEZ -> JUYIRUFI -> FEZ, including a rapid radial approach and possession
   changes. Expected new log: `Published standby terrain`. Check actual rendered
   landscape, not just that log. Capture the same square boundary and frame time.
2. BEM/Terrestrial: identical menu orbit, gameplay orbit, shore and ground views;
   check sandy shore, wet/dry regions, rock, snow, excessive height contour bands.
   Also compare Ocean/Water/Forest/Oasis, with Frozen/Rocky/Volcanic as controls.
3. Inspect atmosphere on lit/dark limbs and measure frame time plus memory while
   background warmup runs. Captured FPS from different views is not a benchmark.
4. Full non-tiling orbital pigment, sub-grid smooth coasts, ocean bathymetry/current
   detail and cloud patterns are NOT implemented by this pass. Earlier private
   water prototypes were not promoted. Existing roughness/specular refinement
   remains; it cannot create missing geographic water structure.
5. The entire planet is NOT held at maximum surface resolution. This pass retains
   published geometry and prepares nearby bodies earlier; it does not replace
   WorldScape clipmaps with a single uniformly detailed full globe.

Reference analysis used supplied images and prior exported native material graph
connections under F:/ChatGPT/APOSFERA/work/planet_refinement_20260920/native_palette_audit.
Those exports describe the existing graph lineage, not a fresh live MID capture.

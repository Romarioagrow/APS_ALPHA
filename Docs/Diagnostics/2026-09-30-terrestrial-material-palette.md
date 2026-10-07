# Terrestrial material palette — enabled for generated Earth-like terrain

Rio's requirement: Earth-like ground is still garish. Change the material
colours themselves, without dimming exposure, retuning stars or losing detail.

`APSTerrestrialMaterialPalette.h` refines only generated SharedTerra and
ContinuousTerra MIDs of type Terrestrial. No authored parent, ocean material,
geography, biome coordinates, seed or saved palette is modified. The switch
`aps.Surface.TerrestrialPalette` is now 1 after paired rendered checks. Setting
0 and recreating the generated profile restores the previous palette.

The named vegetation uniforms retain 52% of linear-RGB chroma; coast/highland
65%, dry branches 62%, exposed rock 80%. It is a convex mixture with Rec.709
luminance: luminance and alpha remain unchanged. Snow, slope, tint, emissive and
physical-coordinate uniforms remain exact. There are no extra texture fetches.
These are deliberately scoped coefficients, not a claim of completed realism.

## Published checkpoint, 30 September 07:08

Four paired runs used exactly DLL SHA256
3DF5F1B839B0F202FA662B6A8C0D8F7AAE1AFFC2DEB0B5AD2207A576C73B1DF9:

- terrestrial-palette-control-menu-v1 / refined-menu-v1: 9/9 each (one warning
  each), 6 PNG each. Reviewed whole globe and close dry-biome pairs (4/12 PNG).
- terrestrial-palette-control-ground-v2 / refined-ground-v1: 4/4 and 5/5 (one
  warning each), 84 PNG each. Reviewed paired natural Plus8s and flight frames
  000, 013, 039 (8/168 PNG): reduced emerald/yellow chroma with preserved
  terrain/detail pattern. This is sampled route evidence, NOT video acceptance.
- All four manifests independently verified: eight protected Shared packages
  unchanged. No asset bake or new shader sampling cost for the palette.
- build-palette-published-v1 passed (4 actions, 13.22s). CVar default now 1.
- terrestrial-palette-published-default-v1 finished 07:07:40: 8/8, 6 PNG;
  ordinary activation logged WITHOUT palette CVar override. Whole-globe frame
  reviewed. This confirms default publication, not all-biome/all-seed coverage.

Evidence root: F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/.
The fields runner now respects production defaults unless TerrestrialPalette
is explicitly bound; use -TerrestrialPalette:$false for an OFF control.

The same large black sky arc is visible in BOTH natural-ground controls; it
predates this change and remains an independent unresolved rendering defect.
Do not present these images as acceptance of the entire sky/ground scene.
Water, cloud system, foliage and flight residency are not fixed by this change.

## Earlier evidence boundaries

- `build-water-precise-palette-v1` compiled the helper and arithmetic/scope test.
- `terrestrial-palette-control-ground-v1`: 4/4 structural tests (one warning),
  84 PNG including three natural ground views. Four inspected. Palette OFF;
  this is a baseline only. Eight protected Shared packages unchanged.
- Claude then changed initial camera framing (N6). Compare the next pair on
  the SAME DLL; do not use the earlier natural-ground camera as a clean A/B.
- `build-palette-foliage-placement-v2`: 5 actions / 17.46 seconds, success.
  Candidate frames and the palette-specific test have not yet been accepted.

## Acceptance

Matched published Terrestrial menu/orbit pair with PaletteMode 0/1, then
published gameplay approach/ground hold pair without/with TerrestrialPalette.
Check native material bindings and logged activation; compare vegetation,
dry soil, coast, rock/snow, normal/detail preservation and distance continuity.
Do not count a darker scene alone as improvement. No production enable until
the rendered pair passes. Camera, star light and exposure are not experimental
variables in the pair. Asset hashes and arithmetic tests are safeguards, not
rendered proof.

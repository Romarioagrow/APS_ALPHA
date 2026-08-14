# APS-77 Surface material A/B checkpoint

Date: 2026-08-14

## What changed

- Added a Surface-owned, default-off world subsystem controlled by
  `aps.Surface.Diagnostics.MaterialAB`.
- The subsystem targets only the dynamic terrain material on the canonical
  player-anchored WorldScape root.
- Presets isolate near-color, detail-normal, detail-roughness, and radial
  LOD-seam normal contributions without editing shared material assets or
  planet profiles.
- Original scalar values are cached per active MID and restored when the
  preset is disabled, changed, the material changes, or the world subsystem
  deinitializes.

## Ownership and baseline safety

- Owned files:
  `APSPlanetSurfaceMaterialABSubsystem.h/.cpp`.
- Dev 2 confirmed this isolated scope is free of overlap.
- Shared WorldScape, generator, profile, material, and foliage files remain
  untouched.
- CVar default is `0`; accepted Ocean/Living, Land Coverage, seed/profile,
  rendering, and performance baselines are unchanged unless a developer
  explicitly enables an A/B preset.

## Verification state

- PASS: focused source review against the existing compiled Surface anchor
  diagnostics pattern.
- PASS: scoped diff is limited to the two new Surface-owned source files and
  this evidence note.
- PENDING: APS_ALPHAEditor build and opt-in runtime apply/restore log evidence.
  The global heavy slot is reserved by another developer; no UE/build/test
  process was started concurrently.
- PENDING: same-camera visual A/B. Compile-only or log-only evidence will not
  be classified as a visual PASS.

## Intended classification workflow

1. Preset `1`: determine whether crawl is dominated by the aggregate near
   color/detail-normal/detail-roughness path.
2. Preset `2`: isolate the LOD-seam radial-normal blend.
3. Preset `3`: compare forced radial normal with detail normal disabled.
4. Presets `4` and `5`: separate near-color and detail-roughness influence.
5. Return to preset `0` and confirm cached values restore before accepting any
   visual conclusion.


# User-accepted WorldScape visual checkpoint — 2026-09-27

This is a recovery checkpoint of the saved project state the user tested and
explicitly asked to commit. It is not a claim that every original defect is
exhaustively closed, and it does not enable any new material experiment.

## Acceptance reported by the user

- Major improvement of planets in PLANET, from orbit and on the surface.
- Volcanic lava material is now correctly applied and looks good.
- Atmospheres, stars and clusters look good; no observed star flicker.
- Transitions look good and texture tiling is now much less apparent.
- Approximately 120 FPS observed in the user's quick gameplay test.

The FPS and broad visual acceptance above are the user's observations, not a
new controlled benchmark. Earlier diagnostic reports remain useful for narrow
issues but must not override this newly accepted production baseline.

## Preserve

WorldScape remains the terrain/LOD implementation. Keep seeds, geography,
continents, accepted palette, planet scale/orbits, world persistence and authored
SinglePlay behavior. Preserve this baseline before subsequent aesthetic changes.
The checkpoint includes current code/configuration and saved project assets,
including dependencies and existing disabled diagnostic assets. It excludes
DerivedDataCache, Intermediate, Saved data and unsaved in-editor changes.

No new lava coherent-blending experiment was installed: its two preliminary
workspace files remain outside this project. Earlier radiance/plate experiments
remain disabled/unreferenced by the production lava master.

## External recovery dependency

The installed WorldScape plugin is outside this Git repository. A separate,
hash-verified local snapshot is retained at:

`F:/Rio/Projects/Unreal Projects/APS/APS_ALPHA/Saved/AcceptedVisualCheckpoint_20260927`

It contains WorldScape source, content, configuration and binaries; the local
AtmoScape plugin; and the current APS editor module, PDB, module manifest and
build target. The companion external manifest records exact paths/hashes.
Snapshot: 971 verified files, 3,349,316,804 bytes. Manifest SHA-256:
`AAE64298A3ACB4D612FB42882B0D4E07E231DB86681FA2C9C0880E7610659C89`.
Generated plugin intermediates/caches are excluded. A Git checkout alone on a
different machine does not install these external dependencies. No restoration
should overwrite a live editor's loaded modules; retain backups and compare
hashes before any future recovery. World saves are not modified by this snapshot.

## Next polish, not a replacement of the accepted look

Prioritize Temperate/Terrestrial and related Ocean, Living, Forest, Oasis and
Savanna variants: improve perceived water depth and land/water integration,
smooth unnatural small angular transitions, and add restrained multiscale detail.
Use the supplied Elite Dangerous images for lighting/material realism, not to
copy geography or replace WorldScape. Changes need same-planet comparisons and
performance checks against this accepted baseline before promotion. Do not
continue rejected lava experiments merely because older reports called it weak.

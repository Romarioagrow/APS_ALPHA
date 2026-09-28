# Orbital material repetition: controlled V2 refinement

## Scope

Preserve the user-confirmed living biomes checkpoint `741e25c8`. Change only
large-distance macro albedo variation on generated Terrestrial and Oasis.
No ship code, terrain geometry, collision, height/shore field, liquid material,
atmosphere, native authored references, or palette transfer was edited for this pass.

The shared macro function samples `T_Default_MacroVariation` at approximately
400 m, 5.43 km and 20 km. The existing metre-scale coordinate warp cannot hide
those long-range repeats. A constant-mean control removes the repetitive signal;
the replacement retains variation without wrapping texture coordinates.

V1 trilinear value noise produced large billows and was rejected. V2 uses a fixed,
planet-space tetrahedral gradient field, two derivative-filtered octaves, frequency
8 and contrast 0.5. It replaces exactly three macro XYZ outputs, not layer masks.
Physical LWC coordinates are divided by band size before custom-node float
demotion. Exact legacy output is retained within 5 km camera-to-pixel distance;
the blend finishes at 50 km. This is not a mesh swap or another terrain layer.

## Installation and recovery

`APSPlanetSurfaceAsset -OnlyInstallSharedTerrainMacroV2` is a guarded one-time
publisher of the rendered candidate. It checks source/candidate hashes, takes a
verified backup before mutation, checks shader completeness, rechecks original
files before saving, and refuses subsequent installation over an existing backup.
It saves only the new function and the existing shared master. Never run the
general material bake as a substitute for this publisher.

- New function: `/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MF_APS_OrbitalMacroV2`
- Master: `/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/M_APS_SharedWorldScapeTerrain`
- Backup: `Saved/SharedTerrainMacroBackup20260928`
- Master before: `0C5D1F194936CCC5B332D0A26000BDD7D77E57FF`
- Master after: `1712E8C32789C807E98B954FC2063F292D965968`
- New function: `C4701C8CE10766B17F82A317B76334DB9B0FDC01`
- Rendered diagnostic V2 function: `CF1CF65D8C3413DF8EB1D2C28CD884DA38F2A750`

Both existing shared MICs and the original macro function keep their bytes.
The new function defaults OFF. `APSNativeTerrainMaterial::ApplyPalette` enables
it only for generated shared Terrestrial/Oasis instances; every other family is
explicitly OFF. `APS.Contracts.PlanetSurface.OrbitalMacroFamilyGate` covers every
current enum value plus an invalid/future value. Expand this gate only after
rendered family checks. A fresh rebuild from original vendor assets would need
this transformation reapplied; the existing shared builder refuses overwrites.

For a targeted rollback, first coordinate and close all UE processes. Verify the
current master still has the above installed hash, then restore only that master
from the verified backup. Do not reset the working tree, restore ship files, or
delete unrelated assets. Leaving the disconnected new function is harmless.

## Evidence

Evidence root: `F:/ChatGPT/APOSFERA/work/planet_macro_20260928`.
Each run contains `surface.log`, `report/index.json` and original, unedited
Unreal screenshots under `Saved/Screenshots/Windows`. Generated gameplay uses the
real surface/material binding; the fixture freezes settled mesh payload for A/B.
Every phase checks the full vertex/index/normal/UV/color/collision payload.

Runs before installation:

- `oasis-orbit-100-v1`: rejected broad billows, not production.
- `oasis-orbit-100-v2`: 100 km; nadir, oblique, limb; four material phases.
- `terrestrial-orbit-100-v2`: same views on Terrestrial.
- `terrestrial-near-100m-v2`: 100 m, all three views; native detail control.

All these automation runs finished with zero failures. Source/shared-copy control
was visually equivalent before changing macro mode. Near Terrestrial native vs
V2 mean absolute RGB difference was 0.062 / 0.151 / 0.143 (0..255), p99=1, consistent
with temporal/render noise in the unchanged control. Do not interpret pixel
statistics alone as artistic acceptance.

Controlled GPU means, RTX 5080, actual captured viewport 1280 x 722:

| Family/view | Original ms | V2 ms |
| --- | ---: | ---: |
| Oasis nadir, 100 km | 4.1795 | 4.2704 |
| Oasis oblique, 100 km | 4.2828 | 4.3579 |
| Terrestrial nadir, 100 km | 4.2502 | 4.2700 |
| Terrestrial oblique, 100 km | 4.2869 | 4.3655 |
| Terrestrial nadir, 100 m | 4.4217 | 4.4184 |
| Terrestrial oblique, 100 m | 4.4853 | 4.5165 |

These are asynchronous GPU counters for a frozen comparison, not production FPS
or a guarantee at the user's full resolution. No texture samples or triangles
were added; there is additional filtered-noise ALU work at long range.

Fresh-load installation checks:

- `terrestrial-orbit-100-installed-v2`: 2 tests, 0 failures. The generated production
  slot (phase `Macro0Native`, historical label) matches isolated V2, not mode-OFF
  control. Mean RGB differences production vs V2: nadir 0.016, oblique 0.143,
  limb 0.011; p99=1 in all views. OFF control nadir differs by 2.483 / p99=15.
- `oasis-orbit-100-installed-v2`: 2 tests, 0 failures. Production matches isolated
  V2 (nadir/oblique/limb 0.146/0.150/0.016; p99=1); OFF control nadir differs by
  5.527 / p99=27. Original frames were inspected, including the limb.
- `ice-orbit-100-disabled-control-v2`: 2 tests, 0 failures. Production keeps
  native mode, with control mean RGB difference 0.157/0.194/0.101 and p99=1.
  No claim of an artistic improvement to Ice or other unselected families.

After installation `Macro0Native` means CURRENT PRODUCTION, not the old version.
`Macro1Control` explicitly selects the unchanged original texture sampling.
Never mislabel these as before/after without checking the installation state.

Build `planet-macro-install-build-20260928.log` succeeded with
`-NoXGE -MaxParallelActions=2`. Installation `install-v2/install.log` finished with
0 errors and 8 warnings. Warnings include pre-existing missing plugin resources
and the transient slope function pin before the existing pin repair; complete
LocalVF shader maps were required before saving. Fresh-load rendered evidence,
not this compile result alone, is the final installation check.

## Still open

This pass does not fix angular coast geometry, the fragmented wet/dry boundary,
overly lumpy close water, atmospheric washout or every other planet family.
Those remain separate visual work. Do not call the whole planet goal complete.
This pass does not establish live-flight, floating-origin, menu-preview or
full-resolution performance acceptance; its rendered scope is the comparisons above.

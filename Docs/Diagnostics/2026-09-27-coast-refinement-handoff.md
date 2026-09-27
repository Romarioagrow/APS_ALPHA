# Coast refinement handoff - 2026-09-27

## Installed, but not visually accepted

The canonical APS_ALPHAEditor module has been built successfully with generated
full-scale WorldScape ring resolution 192 (previously 96). Terrain and liquid
use the same lattice. This extends fine-LOD coverage; it does not change the
finest 120 cm spacing, height-field coefficients, seed, material or palette.
Authored roots retain resolution 96, scaled previews 48, and collision settings
are unchanged. The new policy tests compiled; they have not been executed in
this build.

The resolution is bounded to 96..192 and rounded up to a multiple of four.
`aps.Surface.MeshResolution 96` restores the previous geometry budget. Set it
with PIE stopped, then start a new gameplay session; active roots are not
reconfigured by changing the variable. Default 192 increases vertex workload
approximately fourfold per ring: unchanged FPS is NOT established.

Build evidence:
`F:/ChatGPT/APOSFERA/work/planet_finish_20260927/build-coast-refinement.log`.
APS_ALPHAEditor build exited 0, all seven actions completed.

## Why this change

The BEFORE run, at resolution 96, completed actual Volcanic shoreline captures
at 300 km, 100 km and 15 km with profile signature 1366783096:
`F:/ChatGPT/APOSFERA/work/planet_finish_20260927/lava-coast-geometry-volcanic-20260927-220247-554`.
The diagnostic sampled submitted mesh sections, not hypothetical noise alone.
Matched land/liquid vertices had zero measured lattice offset; shoreline edges
reached about 33 screen pixels and interpolation errors exceeded 200 m in
some sampled distant triangles. This supports coarse geometric sampling as a
remaining contributor, not another UV or alpha-mask repair.

The baseline test passed and its process exited 0. That result predates the
192 change and MUST NOT be used as an after-change visual or performance pass.
Higher resolution cannot recover detail below the finest lattice spacing and
is not proof that every polygonal shore artifact is eliminated.

## Editor handed back to Rio / Claude

Rio requested the editor back for Claude's ship work. No further build, bake,
test or Unreal launch is authorized by this handoff. No diagnostic processes
remained at handoff. Do not overwrite loaded modules, terminate user sessions,
modify ship assets, or stage their map changes.

Pending user checks: same Volcanic/Melted orbit coast before/after, frame time
and streaming stalls, then the saved BEM high-speed vertical descent. The exact
BEM save name has not yet been provided. The nested Frozen squares have not
been reproduced in a continuous-descent test and are NOT marked fixed.

Ocean depth/current v22/v23 remain PRIVATE candidates, not installed production
materials. Their cyan-band limitation and pending movement/performance evidence
remain open. This checkpoint is a compiled coastline refinement candidate,
not completion of the planet-material iteration or all-family validation.

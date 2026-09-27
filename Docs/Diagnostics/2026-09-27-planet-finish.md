# Planet finishing checkpoint — 2026-09-27

Partial progress; the full visual goal is NOT complete.

## Installed code

- Owned full-scale WorldScape oceans now share terrain MaxLod, LodResolution
  and TriangleSize regardless of liquid chemistry, extending the existing
  Water rule to lava/exotic liquids. Terrain, collision, authored roots and
  scaled previews are unchanged. `LiquidLattice` covers the eight ownership /
  preview / ocean combinations and passed after the canonical editor build.
- A real 300 -> 100 -> 15 km Volcanic shoreline run exposed a WorldScape worker
  crash. The unsynchronized pending-map access and early bool completion were
  replaced by game-thread map ownership and actual async completion fences;
  completed task allocations are released. Explicit regeneration drains workers.
  The independently built Core DLL was installed with backups and source checks.
  See [worker fix](../../Tools/Diagnostics/WorldScapeWorkerFence/README.md).
- The existing natural shoreline probe has an explicit `APSLavaFarShoreline`
  mode for 300/100/15 km. It keeps the real streaming pawn with the camera,
  visible terrain, original lava MID/RGBA and geography. It does not simulate
  continuous flight and cannot prove the user's fast-descent case.

## Actual evidence and limits

Evidence root: `F:/ChatGPT/APOSFERA/work/planet_finish_20260927`.

- `frozen-nadir-baseline` (300 km) and `frozen-nadir-10km`: captured and inspected
  settled Frozen nadir views. The user's nested square defect was NOT reproduced.
  BEM is reportedly saved in Existing Worlds; the save name is still pending.
- `lava-lattice-volcanic-shore-v1`, `lava-lattice-melted-shore-v1`: natural
  landing and actual visible coasts at 15 km / 1.5 km / 150 m passed. Inspected
  frames did not show detached triangular tears at these heights. Angular
  coast geometry is still present; these are not all-orbit acceptance tests.
- `lava-lattice-volcanic-far-v1`: FAILED by worker access violation after two
  orbital frames. Keep this evidence; do not relabel it a pass.
- `worker-fence-unit-v1`: isolated CPU worker-ownership and generated-seam
  regressions passed. NullRHI here is only unit-test evidence.
- `lava-lattice-volcanic-far-v2`: installed fix, four tests passed, actual
  300/100/15 km capture sequence completed and exited 0. Inspected 300 km frame
  STILL shows coarse angular lava/land boundaries. The material binds context=0
  (gameplay mask is opaque), so do not assume this is the preview Hole-alpha mask.
- `lava-lattice-melted-far-v2`: installed fix, rendered diagnostic passed and
  exited 0 after 300/100/15 km captures. Inspected 300 km and 15 km frames still
  show angular liquid/land contours; the orbital defect is not resolved.

## Ocean experiments — not installed

v22 currents and v23 physical-column contrast were built, baked and rendered in
the private water-depth host. The v23 real shore walk passed, but rendered water
remains too bright/flat. Do not promote it or infer 120 FPS from its automation.
Source archives, hashes, measurements and failure details are in
[CURRENTS.md](../../Tools/Diagnostics/WorldScapeWaterDepthIntegration/CURRENTS.md).

## Still open

Orbital coastline geometry/aliasing, the exact moving BEM reproduction, water
optics/contrast/currents, cross-family PLANET/gameplay detail coverage, Ocean/Metal
dark boundary, Ice seams, Tundra softness, delayed lighting and paired walking /
sprint performance. The task has not been narrowed to passing these tests.
No production material, palette, seed, save or character asset was changed here.

## Follow-up: coastline measurement

`APSCoastGeometry` adds a read-only measurement of the actual submitted terrain /
liquid triangles at each shoreline frame. It checks identical topology, records
screen-space edge size and angular lattice alignment, and compares interpolated
liquid depth with the authoritative native height field at triangle centroids.
At most 16 visible coast triangles per section per LOD receive native samples.
CSV samples are evidence for diagnosing geometry; they are not a visual fix.

`build-coast-geometry.log` succeeded. `lava-coast-geometry-v1` was stopped before
the far-shore measurements: a user editor (PID 34424, started 21:47:35) appeared
between the earlier process check and the diagnostic launch. Only the owned
diagnostic PID 8724 was stopped; the user editor was left running. This run is
NOT a pass and supplies no coastline measurement result.

Resume with `Tools/Diagnostics/Run-LavaCoastGeometry.ps1 -Planet Volcanic` only
after the editor is closed. The launcher fails closed when any Unreal editor
process exists, returns its owned PID, and creates a unique F: evidence folder.
Do not rebuild or replace loaded production DLLs while the user editor is open.

Claude's separate `Docs/Audit/2026-09-27-claude-full-audit.md` is not part of this
checkpoint and must not be staged with Codex changes.

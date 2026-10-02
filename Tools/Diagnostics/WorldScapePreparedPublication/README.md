# Worker-prepared WorldScape publication (scoped rollout)

01.10 compact v4 experiment was installed, measured, and fully rolled back.
`CompactRejectedV4.diff` is NOT active: it reduces the duplicate render vertex
152->88 bytes, with997152 exact comparisons and8/8 clean private tests, but
Water still falls back112/116times at384MiB and frame p99 remains61/63ms.
The private source was restored to the installed baseline at14:20 before the
separate collision-height experiment. Do not promote the old v4 green suite.
See the13:10–13:40 section in the planet continuity epic for trace attribution,
timings and rollback hashes. Ordinary installed runtime is the pre-v4 version.

Targets the measured game-thread vertex conversion + render-buffer allocation/copy.
The existing LOD worker builds two immutable vertex arrays after welding its staged
main/seam data. The game thread validates the complete three-section payload, then
swaps CPU ownership and enqueues the existing render update without a large copy.
V2 also prepares CPU-only packed position/tangent/UV/colour buffers; it never calls
InitResource or touches RHI on the worker. RT validates sizes/formats, copies these
buffers, and performs the original GPU upload. Unknown formats use the old loop.
The root still waits for the entire LOD task batch; no partial-neighbour publication.

## Ownership / bounds

- Capture occurs on GT before worker dispatch. Published buffers are borrowed under
  the existing root completion fence. Do not mutate these sections concurrently via
  external mesh APIs. Root cleanup drains workers before component destruction.
- Restricted to APS-tagged visual meshes, three sections, NoCollision. Other users,
  mismatched buffers, collision meshes and exhausted budgets use the legacy path.
- `worldscape.PreparedMesh -1/0/1`: auto/off/force APS-owned, default -1. Auto also
  requires `APS.Mesh.PreparedPublication`, added only to generated full-scale native
  Terrestrial/Frozen/Oasis. Manual roots, scaled previews and other presets remain
  legacy. `worldscape.PreparedRender 0/1` isolates v1/v2; default 1.
- `worldscape.PreparedMeshBudgetMiB 384`,
  clamped 0..512. Reservation includes two new arrays and conservative slack; actual
  TArray + packed CPU buffer sizes are checked. This is NOT a total planet/process/VRAM budget.
- A thread-safe reservation follows worker packets and queued render commands.
  After swapping, old CPU buffers retire with the render packet. The lease is
  released after arrays, including on cancellation/fallback. No persistent cache.
- UV0/1/2/3, optional channels, water tangents, linear vertex colours and bounds
  preserve the legacy contract. No geography, topology or material edits.

## Validation / install

Private host: `F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild`.
Native private suites v1/v2/v3: each 8/8, zero test warnings,
498576 exact vertex comparisons; budget exhaustion and worker-reset checks.
V2/v3 additionally compare every packed byte and reject mismatched formats. V3
verifies the auto-owner guard. Fresh suite label required after changing the DLL.
NullRHI is not rendered or performance acceptance.

`Install.ps1` checks the tested native hash, unrelated sources, engine BuildId,
backs up source/UHT/8 plugin modules/APS binaries, and rebuilds APS. The ordinary
C++ task layout changed: NEVER install only Core.dll or launch an old APS.dll.
`Install.ps1 -Rollback` checks hashes before restoring the v3 transaction (v2 was
default OFF). Immediate runtime rollback is `worldscape.PreparedMesh 0`.
For a full pre-experiment binary restore, roll back v3, v2, v1 in that order using
explicit BackupLabel values. The narrow APS owner-tag source change is separately
backed up in `prepared-publication-project-source`; do not overwrite later edits.
Generated Root/Mesh UHT differences are private-host FID prefixes, not new reflected
fields; matching headers are included nonetheless. Assets are never overwritten.

Clean same-DLL A/B: `RunPlanetFieldsFlight.ps1 -Family Terrestrial -Isolation
Published -WaterFlight -Performance -PreparedMesh:$false -Label <fresh-label>`;
repeat with `-PreparedMesh`. No PNG or CPU trace inside the measurement region.
`-WaterPayload` adds the native bathymetry regression. Do not pass old rejected
`MeshUpdateTasks` or `FastMeshCopy` options. Separately capture moving frames.
The probe dumps accounting after flushing render commands outside measurement.
V2 clean pair evidence: full-frame p99 OFF75.96/73.91 ms versus ON37.21/41.36;
frames >50ms OFF21/20 versus ON1/2. P95 and >33ms counts essentially unchanged.
This reduces severe publication hitches, not all stutter or a proven average FPS gain.

This is not motion-aware eviction/preload, full ship-flight validation or completed
water/atmosphere/clouds/foliage. Keep experimental until paired timings and real
rendered controls justify each family promotion. See the dated diagnostic report for results.

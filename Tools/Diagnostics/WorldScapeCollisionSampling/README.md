# Bounded native collision sampling

## 2026-10-01 height-only integration (default OFF)

`InstallHeightOnly.ps1` is a separate, reversible transaction. Unlike the older
patch below, it changes the noise base vtable and LOD layout: all native modules
and APS must be installed/rebuilt together. Do not copy only WorldScapeCore.dll.

`worldscape.CollisionHeightOnly 1` skips material/climate sampling only for an
exact immutable APS spherical generator with the existing bounded parallel tag.
Heightmaps, noise/height/hole volumes, inverted planetary blend, flat worlds,
unknown generators, visible collision and collision delegates keep the full path.
Density/topology/readiness are unchanged. Debug colours are restored from exact
original sample positions without replacing geometry or the cooked body.

Evidence: `work/planet_continuity_20260929/collision-height-private-v1` (8 clean)
and `collision-height-integration-v1` (16 passed, 3 existing cleanup warnings).
36 native cooked-mesh cases / 80,100 vertices across Water, Terrestrial, Oasis,
Frozen, Volcanic and Desert: identical float physics vertices, indices and debug
colours; cook succeeded and debug restoration retained the same cooked geometry.
This does not establish ship contact, smooth flight or final visual acceptance.

Install-v1 rolled back after a missing Chaos header in the new test. Fixed test
and coherent install-v2 succeeded, then was ROLLED BACK after the timing pair:
OFF/ON p99 60.096/65.755ms, worst 78.184/152.178ms, no demonstrated flight benefit.
Do not infer causal regression from one pair. Both routes passed (6 clean plus
one warning test); ON confirmed nine actual height-only collision patches.
The code is retained privately, NOT installed/enabled for ordinary gameplay.
Native source diff: `HeightOnlyNativeSource.diff`; final state in the planet epic.

The installer refuses foreign edits and restores the complete plugin, APS header
and binaries. Both install and rollback invalidate restored header timestamps:
without this, UBT reuses newer incompatible object files across a vtable rollback.
The first post-rollback diagnostic link exposed this and was not run; the baseline
is rebuilt with invalidated dependencies before further tests. Do not invoke an
old rollback transaction after that rebuild: its guarded APS hashes are obsolete.

## Historical 2026-09-29 parallel sampler

This source package targets the installed UE5.4 WorldScapeCore, preserving its
accepted worker-ownership and seam-normal fixes. Only collision sampling changes;
there is no native layout/header change and no geometry simplification.

`collision-sampling.patch` is a generated diff against the backed-up installed
`WorldScapeRoot_Collision.cpp`. `APSWorldScapeCollisionSamplingTests.cpp` is the
native exact-payload regression test. Do not apply the unrelated ocean UV1 overlay.

Build host: F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild.
Build UnrealEditor Win64 Development with that HostProject.uproject,
-Module=WorldScapeCore -NoUBTMakefiles -NoHotReloadFromIDE -NoXGE
-MaxParallelActions=2 -OverrideBuildEnvironment -Define:__has_feature(x)=0.
Installed and host BuildId must agree; compare ALL source trees before promotion,
not merely the touched files. Never replace a DLL while Unreal is running.

Private tests: APS.Gameplay.World.PlanetSurface.CollisionSamplingParity,
WorkerCompletionOwnership, LodGeneratedSewingNormals: 3/3 passed.
33552 vertices and complete semantic/topology arrays were exactly equal.
No claim of runtime performance follows from these tests.

Runtime opt-in: actor tag APS.Collision.ParallelSamples, immutable native sampler,
no heightmap/noise/hole volume overrides. Project integration tags only its owned,
full-scale exact UAPSWorldScapePlanetNoise class. Unknown samplers remain serial.
worldscape.CollisionSampleTasks 1 is the serial rollback, 4 is the bounded candidate.
ParallelFor joins before mesh publication; tasks never outlive the root/snapshot.

Backup and hashes: F:/ChatGPT/APOSFERA/work/planet_flight_performance_20260929/before.
Real-game evidence/status: Docs/Diagnostics/2026-09-29-flight-performance.md.
Old Core SHA256 0AD8D541CBE5DBC3BC729F2E8F1A3F8299BFE228A3CA2E0FA9EFD6CE22BADE22.
Candidate Core SHA256 02ACA9A8028B4065B1E9D9F55A47CCB58D81DECBF0BCEC8C732EB73138B2AC20.

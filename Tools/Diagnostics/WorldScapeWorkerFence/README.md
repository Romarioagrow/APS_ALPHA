# WorldScape LOD worker ownership fix — 2026-09-27

This patch is separate from ocean-material experiments. It changes two vendor
implementation files and adds a regression test; no public header, native layout,
material, noise, geography, palette or LOD density changes.

## Evidence and cause

Canonical `lava-lattice-volcanic-far-v1` captured the real coastline at 300 km and
100 km, then crashed while moving the streaming pawn to 15 km. The stack is
`LodGenerationThread::DoWork`, `WorldScapeRoot_Thread.cpp:621`, at the worker's
`WorldScapeLodInGeneration.Contains` access. Original sources show game-thread
`TMap::Add` interleaved with immediate worker startup. Those workers read and
write that same unsynchronized map. The bool completion flag also preceded the
worker's cleanup tail. The normal publish path emptied the task-pointer array
without deleting completed task objects.

Fix: the pending map is game-thread-owned; workers never access it. The existing
whole-batch publish waits for every `FAsyncTask::IsDone()` synchronization fence,
then updates meshes and deletes completed tasks. Explicit component regeneration
drains/cancels outstanding LOD tasks before destroying their components. Normal
frame polling does not call EnsureCompletion on unfinished work.

This is a concurrency/lifetime fix, NOT proof that all coastline polygons or
the user's nested Frozen rectangles are gone.

## Build / installation

Build host: `F:/ChatGPT/APOSFERA/work/surface_detail_20260915/PluginBuild`.
The stale build copy was first synchronized with the accepted installed
seam-normal welding code. A complete source-tree comparison then showed ONLY
the two worker files and new worker test differing from the installed plugin.

Build command: UE 5.4 `Build.bat UnrealEditor Win64 Development
-Project=<host>/HostProject.uproject -Module=WorldScapeCore -WaitMutex
-NoHotReloadFromIDE -NoXGE -OverrideBuildEnvironment -Define:__has_feature(x)=0`.
The final define is the same MSVC/UE compatibility workaround used by the
canonical APS editor target. Initial build without it failed; it was stopped,
not mistaken for a successful build. `build-worker-fence-v2.log` succeeded.

Private host test `worker-fence-unit-v1`: WorkerCompletionOwnership and
LodGeneratedSewingNormals both passed. This was NullRHI CPU regression testing,
not visual validation. The worker test grows the pending map while real tasks
run, checks three 32-task batches, and exercises immediate component cleanup.

Installed plugin: `C:/Program Files/Epic Games/UE/UE_5.4/Engine/Plugins/Marketplace/WorldScape_5.4`.
Only Core DLL/PDB and the three source files were updated; other modules, module
BuildId and all content were preserved. `manifest.json` records original/build
hashes. `installed-20260927.json` records actual installed hashes. Existing source
EOLs were preserved; normalized source text is exactly equal to the build inputs.
No water-depth private-plugin DLL or changed native layout was installed.

Rollback bytes (original Main/Thread sources, Core DLL/PDB) are preserved at
`F:/ChatGPT/APOSFERA/work/planet_finish_20260927/worldscape-worker-before`.
Never replace a DLL while any Unreal process is using it. Check hashes before
rollback so later plugin work is not overwritten. Removing this fix would
restore the known unsafe worker-map access; do not do it silently.

The original failure and all verification logs/frames are under
`F:/ChatGPT/APOSFERA/work/planet_finish_20260927`. The post-install real-render
recheck `lava-lattice-volcanic-far-v2` passed all four tests (worker ownership,
generated sewing normals, liquid lattice, rendered surface diagnostic) and
captured 300 km, 100 km and 15 km before exiting with status 0. The post-fix
300 km frame was inspected: the crash is addressed, but angular coastline
geometry remains visible. This is not a claim of completed lava visuals,
continuous-descent stability across all worlds, or preserved 120 FPS.

The second-family run `lava-lattice-melted-far-v2` also completed the real
300/100/15 km sequence and exited 0. Its 300 km and 15 km frames were inspected:
angular liquid/land contours remain. Neither successful sequence substitutes
for reproducing BEM's continuous high-speed descent.

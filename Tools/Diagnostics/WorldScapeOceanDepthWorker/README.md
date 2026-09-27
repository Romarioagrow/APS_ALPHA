# WorldScape ocean-depth worker preflight

Isolated diagnostic source overlays for the optional, uninstalled UV1 prototype.
They are not compiled into APS and must not be installed over the accepted
WorldScape plugin. Test noise is an analytic oracle, not a planet generator.

## Confirmed

The real `LodGenerationThread::CalculateNoise` ran on the engine thread pool:
600 checked samples, three sections, two local origins, LOD spacings 0/1/5/9.
546 wet and 54 dry samples retained the correct sign and physical kilometre
units. Results agreed at shared points after section reordering and nonzero
`OffSetHelper`. Ocean's actual height clamp was included. Opt-in retained exactly
the original ocean geometry and semantic colors. Call counters verified no
additional ground evaluation for untagged oceans, ground or flat worlds.

Full `DoWork` generated and published another 47,204 checked vertices through
the actual 96-resolution ring builder at levels 0/5/9, with both (0,0) and (1,1)
subpositions and a non-polar, off-radius starting origin. Published UV1 agreed
with the independent analytic column within 1e-9 km. Geometry/colors stayed
unchanged; reusing the same LOD after opt-out explicitly cleared old validity.
Temporary Main/A/B UV1 storage was empty after publication.

## Execution

`build.log`: 6 actions, 24.61 s (initial coordinate fixture).
`build-v2.log`: 4 actions, 20.86 s (adds full ring/publication coverage).
`run-v2/report/index.json`: 3 successful tests, 0 failed, 0 not run; two successes
carry warnings (4 in Payload, 1 in WorkerCoordinates) because this deliberately
content-free host lacks WorldScape_MPC. No warning suppression was added.
Owned PID 28968 exited itself with status 0.

Test filter: `WorldScape.APS.OceanDepth` (Units, Payload, WorkerCoordinates).
The test executable is UE 5.4 UnrealEditor with `-nullrhi`; these are CPU/data
tests, not rendered evidence. Actual APS catalog fields, volume overrides,
multiplayer/root destruction races, WorldScape gameplay visuals, LOD seams in
motion and the user's ~120 FPS remain unverified by this test.

## Reproduce

Evidence/source root:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-contract`.
Copy the two `Source/WorldScapeCore/Private` fixture files from `worker-tests`
into that root's isolated `host/Plugins/WorldScape/Source/WorldScapeCore/Private`,
not the installed Engine plugin. The host already contains the separate v1
plugin data-path prototype; its stale APS preview adapter is not used here.

Build target: `UnrealEditor Win64 Development`, host project and local plugin,
`-Define:__has_feature(x)=0 -NoUBTMakefiles -NoHotReloadFromIDE -NoEngineChanges
-NoXGE -WaitMutex`. `Run.ps1 -Label <unused-name>` refuses an active editor and
existing evidence directory, then runs the finite tests hidden without rendering.

SHA-256:

- Report: `BA887E4A2D327DBB5F2019B00ECCE998E50E5D1EB4DBB9F939DAEFDC5A7C405B`
- Isolated Core DLL: `CAFB44FF64C256DC8F27118F972DB26866047A2C70ED182DED0711D7DE3B6B65`
- Worker fixture: `B107430063A438C3DEAD34B91C078C256111EE0D9BC2FA08BAD037FF5B596CF2`
- Noise oracle header: `4501C2C62BEA4750B733E34F81877986246E7896EE2D16754FACB6EDB1BEDA0F`

Next integration step must use a matched rebuild of all dependent WorldScape and
APS modules: v1 changes `LodData`/`UWorldScapeLod` layouts. Never deploy a lone
host Core DLL beside installed APS/plugin modules. Physical data and the filtered
Water candidate must then be exercised on actual generated gameplay roots before
any production-default change. Keep the accepted checkpoint and its external
plugin/binary snapshot intact.

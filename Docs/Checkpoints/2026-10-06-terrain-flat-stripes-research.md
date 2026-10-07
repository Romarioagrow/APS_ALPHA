# 06.10 — flat terrain and square texture stripes: research (Claude flight, read-only)

Rio 06.10: some planets look flat and show square texture stripes (TEVYS); not a priority; research and perf-safe experiments only. Below: the merged plan (ranked causes, night experiment plan with perf gates, minimal code changes vs asset edits). Asset edits and installs need Rio's explicit OK; Codex planet owns these files.

**Planet flat look and square stripes: merged findings from the four investigations, night A/B plan and prep list (read-only, HEAD 61532ed6, still-ship)**

Rio's TEVYS complaint has two causes. The flat look comes from a deliberate 28.09 change that lights all terrain past 20 km as a smooth sphere. The square stripes most likely come from a tiled macro texture that came back into the view from orbit on 03.10. Both can be tested by changing material parameters only, at near-zero GPU cost. A test build with two dev CVars is needed first, because no existing switch covers Rocky gameplay planets. Nothing here has been rendered yet.

## 0. Corrections to the four investigations (I checked these myself)

- **C1. At orbit the WorldScape cell size is a fixed 1.2 km. It does not grow with radius.**
  - VERIFIED: `WorldScapeRoot_Main.cpp:1722` clamps the multiplier with `FMath::Clamp(Min(AltMult, ceil(128R/(Tri*Res*2^MaxLod))), 1, 999)`.
  - TEVYS: ceil = 1483, clamped to 999. That gives LOD0/1/2 cells of 1.2/2.4/4.8 km and rings 307/614/1228 km wide.
  - The half-diagonals of those rings are 217/434/868 km. They match the logged boundsRadius 218.6/435.4/860.5 km (`Saved/Logs/APS_ALPHA.log` ~8379-8384).
  - So the square-stripes lens is wrong on three points: its 1.78/3.56/7.1 km cells, "strength grows with radius", and its LOD1 match in the spectrum.
  - Every planet with R ≥ ~2455 km gets the same 1.2 km cells above ~72 km altitude.
  - Effect on the plan: `aps.Surface.MeshResolution 192` does not change the cell size. It only shrinks the rings to 3/4 (LOD0 230 km). That makes it a test of ring boundaries, not of the cell period.
- **C2. Lava also gets the far-normal values.**
  - VERIFIED: `APSUnifiedLavaSurface.h:99` calls `ApplyFarNormalPolicy`, and the policy only rejects `ETerrainGraph::Other` (`APSPlanetSurfaceMaterialPolicy.h:110`).
  - The performance lens's claim that lava is excluded is wrong. Any new override must keep lava out explicitly.
- **C3. The "no-build" A/Bs the lenses proposed mostly do not work for TEVYS-class planets.**
  - The existing far-normal option `-APSDiagnosticFarNormalStartKm/EndKm` works only on Ice and requires the `MI_APS_SharedTerra` parent (`APSGeneratedGameplayHandoffSmokeTests.cpp:1554-1566, 1581-1585`). It refuses ContinuousTerra and Rocky.
  - The menu `-MacroMode` exists (`RunPlanetMenuContinuity.ps1:5, 235` → `APSPlanetTerrainLodABProbe.h:628-636`).
  - But the menu script refuses MacroMode together with the paired `-PlanetRadiusKm/-SurfaceSeed` fixture or with `-Buffers` (`ps1:50, 127, 154`). So the square-stripes lens's exact TEVYS command would throw.
- **C4. Screenshot.** The right file is `C:/Users/Rio/Pictures/Screenshots/Снимок экрана 2026-10-06 051223.png`; the `images/` files 199-202 are other shots. I looked at the scratchpad crops of it:
  - The raw crop shows dark blotches at roughly regular spacing over a fine woven crosshatch.
  - The contrast-stretched crop shows hard-edged, stair-stepped white patches.
  - The band-pass map shows an orthogonal lattice that converges with perspective, so it is attached to the surface, not to the screen.
  - The editor counter reads 117.2 fps / 8.5 ms.
- **C5. When the macro mode changed.**
  - Codex changed the source to `APS_OrbitalMacroMode = 0` on 03.10 at 17:52-18:01 (`PLANET_EDITOR_WINDOW.md:4929, 4932`). It was committed only in 44678c8e on 05.10. So it is a regression since the 03.10 evening build, not 05.10.
  - Codex called it a "user-priority" removal of the 5..50 km colour substitution. Rio's own words were not found in the window. Turning Mode 1 back on must respect "the surface identity must not change during the approach".
  - The timing evidence is mixed. Rio also reported "square LOD seams" on 03.10 at 06:54, while Mode 1 was still on (`:4328`). That one was largely the gas-globe shadow square, fixed 04.10.

## 1. Ranked causes

### Flat look

1. **Past 20 km every pixel is lit as a perfect sphere.** VERIFIED in code, confidence 0.9.
   - `APSPlanetSurfaceMaterialPolicy.h:105-106` sets 2 km / 20 km. It is applied at `APSNativeTerrainMaterial.h:177` and in the master's HLSL `if(t>=1.0) return radial` (asset strings).
   - It came in with 741e25c8 (28.09), one day after the accepted checkpoint 67fff3b4, which used 200..700 km.
   - The stated reason was to hide a "square LOD0" (`APSNativeTerrainMaterial.h:168-172`). The 28.09 doc calls that fix "plausible, not proof" (`Docs/Diagnostics/2026-09-28-standby-and-living-biomes.md:72-76`).
   - The same filter also flattens the slope-driven colour (epic doc :3254-3258).
2. **The height field has almost no relief at the 5-300 km scales visible from orbit.** VERIFIED in code; the numbers are COMPUTED with an offline replica that runs ~15% high.
   - Height = clamp(HN, -0.35, 0.65) × NoiseIntensity. The macro coefficients are .13/.026/.008/.009/.058 (`APSWorldScapePlanetNoise.cpp:377-383, 612-616`).
   - NoiseIntensity is in absolute cm and never scaled with radius (`APSPlanetSurfaceProfile.cpp:782-797, 1010`).
   - TEVYS: the p1-p99 span is ~1.5 km, about 0.04% of R. Mean slope is 2.7° on a 1.2 km baseline and 0.28° at 20 km.
   - That is why Codex's lighting-only bypass gave only faint or grainy gains (`PLANET_EDITOR_WINDOW.md:5133, 5247, 5307`).
   - The generator itself did not change this week (05f544e6 diff covers collision and foliage only).
3. **At orbit the mesh cannot carry the relief bands.** VERIFIED (Codex measurement) + INFERRED, confidence 0.7.
   - With 1.2 km cells, the 1.8 km / 0.9 km / 240 m bands alias into grain.
   - Codex measured mean slopes on LOD0/1/2 of 1.26/0.65/0.43°, against 3.3-3.7° from the true field (`:5194`).
4. **Colour is deliberately decoupled from relief.** VERIFIED intent, confidence 0.55.
   - The vertex colour channel excludes bands of 1.8 km and finer (`APSWorldScapePlanetNoise.cpp:431-449`), and slope colour is radialised too.
5. **Lighting gives no relief cue at orbit.** Confidence 0.45, minor.
   - Terrain shadows only reach about 42 km (inferred from the UE default; no clipmap settings in config).
   - A 0.6 lux anti-sun fill switches on above 60 km (`APSObjectLightingSubsystem.cpp:61-74`).
- **Not a cause:** LOD density. The cells are already sub-pixel from orbit.

### Square stripes (separate from the flat look)

1. **The tiled `T_Default_MacroVariation` is back in the orbital view.** It repeats at 400 m / 5.43 km / 20 km and is projected along the planet axes, so it shows as squares near the axes and as stripes in the blend zones. Code VERIFIED; the link to the screenshot is INFERRED; confidence 0.6.
   - Mode 0 is set at `APSNativeTerrainMaterial.h:181`. Mode 1, the aperiodic 5..50 km field, was on from 28.09 to 03.10.
   - The macro output also shifts the layer thresholds, which are sharp (BottomLayerSharpness = 59 in the MI). A soft periodic signal therefore becomes hard-edged shapes.
   - Codex's 29.09 isolation: the woven grid sits in BaseColor, WorldNormal is smooth, and a mean-only macro removes it (`2026-09-29-planet-continuity-pass2.md:290, 370-372`).
   - Thin spots: that isolation was done on Frozen/Terrestrial, not TEVYS. The 5.43 km ≈ 20-22 px match assumes a 90° FOV.
2. **Vertex-stage noise warps are undersampled on the 1.2/2.4/4.8 km cells.** Graph-derived and INFERRED; confidence lowered to 0.45 after C1.
   - The five VertexInterpolators include Noise3 with 1.35 km voxels, which would need ≤0.67 km spacing.
   - Each triangle then carries an affine slice of the colour noise. The grain changes at ring boundaries, about 153/307/614 km from nadir.
   - Codex's per-pixel bypass of the five VIs was judged only against Lidim's "blue blankness", never against the grid (checkpoint JSON `zevetsPreviewGrid.priorExperimentScope`).
3. **Stair-stepped, axis-aligned blobs from the warp volume.** INFERRED, confidence 0.35.
   - The trilinear 19.5 km warp volume, plus DXT1 4×4 blocks (~78 km), sits behind a high-gain warp and a sharp threshold.
   - It fits the stair-stepped edges in the crop. Codex saw "larger blocky billows" left over with macro and albedo flattened (pass2 :371-372).
   - An uncompressed warp-only A/B has never been run.
4. **Ocean-glint squares (menu) are a separate symptom.** Coarse-mesh normals under a sharp specular are a guess, confidence 0.3. No diagnosis exists.

**Why only some planets:** the shader values are identical on every Continuous planet. Only the palette changes per planet (`APSPlanetSurfaceProfile.cpp:1041-1045`). The variation must come from palette contrast, how much of the planet sits near a layer threshold (seed), view direction against the planet axes, and sun angle. Radius matters only below 2455 km. The old anti-grid knobs in `PlanetarySurfaceGeneratorStreaming.cpp:1085-1111` set parameters that this master does not have, so they do nothing.

**Ruled out (VERIFIED):** coverage/atlas resolution (`aps.Surface.CanonicalRelief` = 0), mesh UVs (the graph has none), nearest filtering, float UV precision, and cube-face seams (the closed globe is hidden at 289 km). The 28 px/m lesson was about the HQ meshes.

**Latent hazard:** Vnoise1, Vnoise3 and Noise_Tex6 are in `TEXTUREGROUP_ColorLookupTable`. Re-saving them would strip their mips (`Texture.cpp:705-711`).

**How the two problems connect:** the far-normal filter made shading flat to hide square ring outlines. The macro mode is a separate knob. Both are uniforms, so a 3×3 matrix at one pose answers both questions.

**Do not repeat** (rejected, with rendered evidence): the static normal-only lighting bypass (Codex's lighting-only A/Bs from 04.10), mesh-curvature correction, NormalMacroWarp, OrbitalFields V1/V2, flattening the volumes, raising the mesh 256→512, a global relief cube, canonical pixel slope, the one-link distance colour selector, and the dense 2x/4x prefilter. Several far-view "negatives" were taken on unlit ground (`:5297`) and do not count.

## 2. Night experiment plan

**Before starting**
- Re-read the tail of `PLANET_EDITOR_WINDOW.md` and the task list, then CLAIM the window. Stamp lines with `date`.
- Rio's editor must be closed; `run_night_check.ps1:51-53` refuses otherwise. ComfyUI closed, at least 8 GiB of system commit free. One heavy UE process at a time.
- Use the same DLL for every A/B, in ABBA or ABA order.
- **Fixed pose:** `run_night_check.ps1 -Planets 4 -Aim next -JumpKm <alt> -Power 0 -Forward 0 -Res 2560x1440 -FixedFps 60 -HoldSeconds 60`. `jump=` puts the ship at that altitude (`APSShipFlightBenchmark.cpp:453-454, 766-769`).
  - Confirm "target for aim=" is BP_Planet_C_1 Rocky in its Profile line (seed 824823 in a4-trace-3). This is a TEVYS-class stand-in, because the bench has no type/seed argument.
  - Frames come from `-AutoExtra 'at=T:aps.Test.Shot+name'` (`:2046`). Uniforms are toggled with `at=T:<cvar>+<v>`.
- **Sun:** the bench cannot set the sun. Every frame needs a daylight check (terrain ROI mean brightness). A dark frame is invalid, not negative — Codex's 12:25 lesson. Codex's `-APSProbeFlightDaylight` route is the daylit fallback (Lidim only).

**N1. Baselines, no build (~30 min)**
- `run_0606_perf.ps1 -Only planet` twice, and `-Only far` once (also measures noise against p1-far-h).
- Idle and planet at 2560x1440 through `run_night_check` (`run_0606_perf` is fixed at 1600x900).
- Fixed-pose Rocky shots at 1000, 289, 100 and 30 km, plus the home Frozen planet at 289 km as a control.

**N2. Discriminators that need no build (~25 min)**
- **(a) Ring boundaries.** `-ExtraCmds 'aps.Surface.MeshResolution 192'` (ECVF_Default, read at profile creation; `PlanetarySurfaceGeneratorStreaming.cpp:69-75`), same poses.
  - If straight edges or grain changes move inward with the rings (LOD0 edge 153→115 km from nadir), stripe cause 2 is real.
  - If the lattice period stays the same, the cause is texture or macro.
  - Also log peakBytes and budgetFallbacks as a performance data point.
- **(b) Macro in the menu.** `RunPlanetMenuContinuity.ps1 -Family Rocky -MacroMode 0|1|2`, plus Frozen and Terrestrial. Lit frames only.
  - Caveat: the menu globe is scaled and uses a different mesh.
- **(c) FlatNormals isolation**, already compiled: `-APSProbeCanonicalFlatNormals`, control vs flat, on the daylit CoverageFar route (`Tests/APSCanonicalCoverageFlight.h:41`).
  - Separates grain from the authored normal textures from colour and mesh causes.
  - Drop any automatic install of the 5DBD7 candidate.

**N3. One build with the CVars K1+K2 (section 3)**
- cl-check first. All four target files are clean in the working tree at 61532ed6 (none are in git status).
- Then `run_regression_gate.ps1`. The defaults must be bit-identical; the existing asserts check mode 0 and the far-normal values (`APSPlanetSurfaceMaterialPolicyTests.cpp:166-192`, `APSGeneratedGameplayHandoffSmokeTests.cpp:2424, 2556`).
- Re-run p1-planet once and compare it to N1, within noise.

**N4. Uniform matrix in one run per pose (~40 min)**
- Toggle FarNormal {2/20, 20/200, 200/700 km} × Macro {0, 1, 2} live, ABA.
- Take a shot 3 frames after each toggle.
- Poses: Rocky at 1000/289/100/30/5 km, plus a ground pose at 0.5-1 m (needed for Rio's identity rule and the near look). Frozen home as control. Wet worlds are not in the harness; mark them UNVERIFIED.
- **Stripes PASS:** the lattice is gone by eye on nadir/mid/limb crops. The autocorrelation or spectral peak at the lattice lag drops by ≥50% (reuse `tevys/spec.py, acorr.py, hfmap.py`; copy them to `F:/ChatGPT/APOSFERA/work/`). No new straight edges at the ring positions.
- **Flat PASS:** high-pass terrain luminance std rises clearly at a low or mid sun, with no square LOD0/LOD1 brightness step returning. The 28.09 regression is the explicit thing to watch.
- **Identity PASS:** mean terrain colour and histogram at 5/20/50 km and at ground are nearly unchanged between A and B. The thresholds are proposals; Rio's eye decides.
- Also toggle `aps.Planet.NightFill 0` vs `0.6` at the terminator; it is minor.

**N5. Performance gate for the chosen candidate**
- **Within each run:** per-second GPU in the 5 s windows around each toggle stays within ±0.1 ms at 2560x1440. No hitch over 33 ms at the toggle. No `newProxies` (with `aps.Surface.ProxyProbe 1`). Uniforms only, so the expected delta is about 0. Codex measured MacroV2 at +0.02..0.09 ms at 1280x722.
- **Before proposing a default, run the full ABBA set:**
  - p1-far, p1-planet and idle at 1600x900, plus idle and planet at 2560x1440.
  - Whole run: avg ≤+5%, p95 ≤+10%, p99 ≤+15%, hitches ≤ base+3.
  - WorldScape window: LOD-generating seconds ≤+10%, window max ≤40 ms.
  - Prepared publication: budgetFallbacks = 0 and peakBytes ≤ 382,795,520.
  - GPU and render thread ≤ base +0.3 ms at 2560x1440.
  - [APS.Mem] peak ≤ base +300 MB.
  - No new Ensure, Fatal or Error lines.
- Rio's near-planet headroom is about zero: GPU 7.0-8.7 ms against 8.33 ms at VSync 120 (log 00:12). Anything over +0.2 ms GPU needs his OK.

**N6. Morning handoff**
- Side-by-side frames, each iteration kept as a new image.
- Honest list of what was not verified: TEVYS itself, wet worlds, offscreen vs PIE.
- No change of default until Rio plays it.

**Deferred**
- A relief gain in the generator changes heights. Spawn, saves and HQ placement depend on them, and the "preserve geography" rule applies, so it needs Rio's explicit OK. Do the offline hillshade sweep in `relief_sim.py` first.
- Codex's canonical coverage chart costs 64-192 MiB and ~21 s CPU per body, and no frame time has ever been measured for it.
- Band-limited WorldScape noise is a plugin edit in the engine folder.

## 3. What to prepare

### Code changes worth preparing (dev CVars, default = current behaviour, no asset change)

- **K1. `aps.Surface.Debug.OrbitalMacroMode`** (-1 = policy (0); 0/1/2).
  - Read at `APSNativeTerrainMaterial.h:181`.
  - A sink re-applies it on change: only `SetScalarParameterValue` on the existing Continuous MIDs (WorldScape LOD sections + closed globe). Never `SetMaterial`, which re-creates proxies at 7-19 ms each. Never UnifiedLava.
- **K2. `aps.Surface.Debug.FarNormalStartKm/EndKm`** (<0 = policy).
  - Lives in `APSPlanetSurfaceMaterialPolicy.h:108-116`, with lava excluded by default (C2).
  - Same live-reapply sink.
  - `APSPlanetReliefRuntime.cpp:104` and `APSCanonicalCoverageFlight.h:178` read these values back. That is fine as long as the defaults are kept.
- **K3. Test tooling only:** let `RunPlanetMenuContinuity.ps1:50, 127` and the probe guard accept paired radius/seed together with MacroMode, for a TEVYS-exact menu A/B at R3643.1 / seed 520970.
- **K4. Automation assert:** volumes Noise1-4 must have NumMips > 1, covering the latent hazard.
- **Optional, performance side** (Claude flight file): skip the `OverridedPlayerPosition` write while the observer is frozen far away (`APSPlanetEnvironmentStreamingSubsystem.cpp:285`). It lowers WorldScape batch rate on departure (expected 18 → ~8 s of LOD generation).
- **Uniform-only knobs to try inside N4** if Mode 1 kills the grid but changes identity: `APS_OrbitalMacroContrast` and `APS_OrbitalMacroFrequency`. They exist in the MI, master and MF strings; their meaning is unverified.

### Needs asset edits (Rio OK required)

- A distance-gated macro, e.g. Mode 1 only above ~50-100 km. The 5..50 km range is not a parameter; no distance parameters exist in the strings.
- Fading the Noise3/2 vertex warp with distance, or evaluating the five VIs per pixel. The per-pixel version adds 2 volume fetches, about +0.2-0.5 ms.
- Recompressing `Noise4_Volume` uncompressed (+55-65 MB).
- Fixing the ColorLookupTable group on the volumes.
- Changing the MI far-normal defaults.
- Adding `APS_CanonicalCoverage*` parameters to the master. The committed master has none, so `aps.Surface.CanonicalRelief 1` binds nothing today.

### Not recommended (performance)

- Higher LodResolution, more LODs or tangents. Prepared publication is already ~95% full (peak 401,360,480 B with 1 fallback in Rio's 05.10 session).
- Per-LOD materials or MIDs per batch. They re-create proxies about twice a second.
- WPO on terrain.
- Extending VSM shadows to orbit.

### Where evidence is thin

- No fixture has ever captured TEVYS.
- Every stripe hypothesis comes from the graph or from other planets.
- The pixel-period matches depend on an assumed FOV.
- No run-to-run noise has been measured for p1-planet.
- The bench cannot control the sun or reach wet worlds.
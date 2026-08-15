# APS-76 Strategic Map visual acceptance plan

Status: **QUEUED / NOT RUN**. Commit `68ce39aa` is compile/UHT/link GREEN,
but no runtime, visual, accessibility or performance PASS is claimed here.

## Evidence truth before the run

The following screenshots exist and may be used only as visual references:

- `Saved/Screenshots/Windows/APS_MainMenu_GalaxySmoke.png`
- `Saved/Screenshots/Windows/APS_MainMenu_ClusterSmoke.png`
- `Saved/Screenshots/Windows/APS_MainMenu_PlanetSmoke.png`
- `Saved/Screenshots/Windows/APS_MainMenu_StellarPresentation_SYSTEM.png`
- `Saved/Screenshots/Windows/APS_MainMenu_StellarPresentation_STAR.png`

They do not encode a seed, StableIds, camera transform, FOV, exposure,
resolution or matching Strategic Map frame. They are therefore **not** valid
same-camera A/B evidence. No pre-existing Strategic Map screenshot was found.

## Resource-safe run preconditions

1. APS-81, APS-80 and Surface focused gates have released the global heavy slot.
2. Project Command grants one explicit APS-76 capture slot.
3. Preflight records the running Editor/process owner and RAM/VRAM/disk state.
4. Use one UE process, one run and no implicit retry. Stop on timeout, RAM/VRAM
   growth or an invalid canonical world; preserve partial evidence as FAIL.
5. Confirm the executable contains `68ce39aa` and record the actual shared HEAD.
6. Do not save a map, asset, canonical model or gameplay state during capture.

The open interactive Editor F8 acceptance owned by Dev Gameplay is a separate
gate and must not be closed by this UI run.

## Exact run artifact paths

Use UTC `YYYYMMDDTHHMMSSZ` as `<RunId>` and write transient evidence to:

`Saved/Automation/APS76/StrategicMap/<RunId>/`

Required files:

- `00_run_manifest.json`
- `01_generation_overview.png`
- `02_strategic_overview.png`
- `03_generation_cluster.png`
- `04_strategic_cluster.png`
- `05_generation_galaxy.png`
- `06_strategic_galaxy.png`
- `07_generation_system.png`
- `08_strategic_system.png`
- `09_generation_star.png`
- `10_strategic_star.png`
- `11_generation_planet.png`
- `12_strategic_planet.png`
- `13_keyboard_focus.png`
- `14_narrow_1280x720.png`
- `15_scene_roi_difference.png`
- `focus_return_trace.log`
- `canonical_identity.json`
- `performance.csv`
- `resource_notes.txt`
- `acceptance.md`

After classification, copy only the manifest, acceptance report and selected
lossless evidence frames into the durable review directory:

`Docs/UI_UX_APOSFERA/evidence/APS76/StrategicMap/<RunId>/`

Do not copy logs containing machine paths, secrets, caches or unrelated WIP.

## Manifest contract

`00_run_manifest.json` must record:

- shared HEAD, executable/build identity and APS-76 commit ancestry;
- world seed plus canonical world/home cluster/galaxy/system/star/planet StableIds;
- target scope and focus enum for every pair;
- exact camera transform, FOV, exposure, resolution and scalability settings;
- UI display profile and reduced-motion state;
- UTC timestamps, process ID and evidence filenames;
- pre/post canonical revision or identity digest proving the read-only session.

If any required value is unavailable, classify the pair **INVALID** rather than
substituting a label, actor name or reconstructed asset identity.

## Same-camera Generation to Strategic Map pairs

For Overview, Cluster, Galaxy, System, Star and Planet:

1. Finalize one canonical generated universe and persist the manifest values.
2. Focus the Generation Map target and capture the odd-numbered `generation`
   frame.
3. Enter gameplay without regeneration, open Strategic Map with F10 and focus
   the same canonical target.
4. Restore the manifest camera transform/FOV/exposure and capture the matching
   even-numbered `strategic` frame.
5. Verify StableIds, hierarchy, names, planet/moon/star counts, relative scale
   and home identity before advancing to the next pair.

The Strategic Map is read-only. Any canonical revision, identity or count change
between the paired frames is an immediate FAIL.

## Acceptance gates

### Presentation and scene isolation

- APOSFERA palette, typography and 4 px spacing scale are visible on chrome.
- Center viewport remains readable and is not covered by a full-screen tint.
- Mask UI panels and help glyphs, then compare the same-camera scene ROI.
  Mean luminance delta must be at most 1%; no renderer gamma, exposure,
  post-process or material change is allowed.
- Live Model displays the canonical home star, spectrum/temperature, home
  planet/radius, planet/moon counts and cluster star count without truncating
  critical identity.
- Planet and satellites remain a clear hierarchy, not a flat button list.

### Navigation and non-regression

- Six hierarchy rows resolve one-to-one to the existing focus enums.
- RMB orbit, wheel scale and double-click focus behave exactly as Generation Map.
- F10 and Escape return to the previous gameplay view with the existing smooth
  transition; no duplicate map, lost pawn focus or camera jump is introduced.
- Opening, focusing and closing the map does not mutate generation or save data.

### Accessibility and responsive layout

- Tab order is hierarchy top-to-bottom, then Return to Game.
- Keyboard focus is visible independently of hover/selection.
- Enter/Space activates the focused control; F10/Escape remain available.
- Labels and status remain understandable without relying on color alone.
- At 1280x720, left hierarchy, unobstructed center and Live Model remain usable;
  no critical label overlaps or leaves the viewport.
- Reduced motion changes decorative timing only and does not alter navigation.

### Performance and resources

- Record average FPS, average frame time and 1% low for the same camera with
  Generation Map and Strategic Map.
- Strategic Map average FPS/frame time may regress by no more than 5% from the
  paired Generation frame. The observed approximately 120 FPS target should not
  fall below 114 FPS under otherwise identical settings.
- Record peak process RAM, VRAM if available and evidence-directory disk growth.

## Classification

- **PASS**: every pair and every gate above has valid evidence.
- **PARTIAL**: compile/static gates pass but visual, accessibility or performance
  evidence is incomplete.
- **FAIL**: canonical identity changes, camera/input flow regresses, scene ROI is
  altered beyond tolerance, layout is unusable or performance exceeds budget.
- **INVALID**: seed/camera/settings/build identity or paired artifacts cannot be
  proven.

Compile-only remains **PARTIAL**. A screenshot without its matching manifest is
not acceptance evidence.

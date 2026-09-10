# Dev Surface checkpoint ledger

Lightweight local source of truth while Jira MCP is unavailable. This ledger records only meaningful Surface milestones and evidence. It is not a gate for technical work.

Ownership: Dev Surface owns ground realization, safe surface placement and anchor-local gameplay. Dev 2 owns canonical planet/civilization identity and orbital representation. Dev Civilization owns runtime civilization manifest, base/pad/ship materialization and save/idempotency. Shared files require a short ownership note before editing.

## SURF-001 - Surface architecture and crawling baseline

- Parent epic: Planet Surface Gameplay & Ground Fidelity
- Type / priority / owner: Spike / P0 / Dev Surface
- Status / result: Complete / PASS for read-only audit only; no visual PASS claimed
- Dependencies: Dev 2 canonical body key, accepted Living/Ocean seed and biome-sample contract; explicit ownership handoff before shared-file edits; resource-safe UE capture window

### Done

- Audited canonical planet/surface data flow, WorldScape gameplay anchor, landing/gravity/collision readiness, LOD/streaming, material graph, water, atmosphere and foliage policy.
- Chosen architecture direction: retain WorldScape as authoritative height/collision; add deterministic anchor-local ground realization without a second planet model or terrain shell.
- Ranked the likely surface-crawling cause: approximately 60 cm LOD0 resnap/republication at the current 120 cm step, combined with replacement vertex normals/colors and camera-dependent material fades.
- Defined the vertical slice: one Dev 2-approved Living/Ocean seed, observation-first crawling classification, stable macro/mid/micro shading, canonical shore sampling, deterministic biosphere cells, then collision/nav and performance gates.

### Checked baseline

- Scope: full-scale ground gameplay; existing landing, gravity and hills remain accepted.
- Canonical seed/body key: not selected in this Surface thread; must come from Dev 2.
- Camera/FOV/resolution: not applicable to the code audit; must be recorded for the first rendered A/B capture.
- Render settings observed: fixed exposure policy, TSR, Lumen and VSM. Full-scale WorldScape uses `LodResolution=96`, `TriangleSize=120 cm`; collision uses the terrain LOD0 spacing.
- Provisional inherited surface performance: 89.2 FPS / 11.2 ms / approximately 2.53 GB. This is context only until resolution, quality and hardware are captured with the same-camera run.
- Surface baseline #2, screenshot dated 2026-08-14 04:35:39: 108.4 FPS / 9.2 ms / approximately 2352 MB. Landing, gravity and character remain functional; sky scale and star density remain strong. This is not a performance PASS against the earlier capture because camera and conditions differ.
- Runtime-log time binding strongly associates baseline #2 with `Melted`, `Magmatic`, seed `1337`, ocean enabled and `MID_MI_APS_WS_Magmatic_0`; the red/orange palette is therefore consistent with the selected subtype rather than an obvious fallback. Exact same-camera artifact binding is still required.
- Baseline #2 visual deficits: weak relief cues despite existing mesh hills, no human-scale roughness/detail/rocks/atmospheric depth, and hard dark/red horizon bands that may be a material, normal, LOD or lighting discontinuity.
- Resource checkpoint, 2026-08-14: external UnrealEditor active; 8.6 GB free RAM; 8969/16303 MB VRAM used; 482.2 GB free on F:. No Editor, build or render process was started by Dev Surface.
- Repository checkpoint: `06179080a824` on `codex/fullscale-gameplay-sprint`; shared checkout already contains unrelated Dev 2/UI work.

### Evidence map

- `Docs/DEV_SURFACE_SHARED_HANDOFF_2026-08-14.md`
- `Docs/VISUAL_FIDELITY_FULL_SCALE_COHERENCE_SPRINT.md`
- `Saved/Logs/APS_ALPHA.log`, full-scale profile and landing records around Unreal UTC 2026-08-13 23:34:51-54
- `Source/APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.cpp`
- `Source/APS_ALPHA/Actors/Astro/PlanetaryBodyStreaming.cpp`
- `Source/APS_ALPHA/Generation/PlanetarySurfaceGeneratorStreaming.cpp`
- `Source/APS_ALPHA/Generation/APSWorldScapePlanetNoise.cpp`
- `Source/APS_ALPHA/Editor/APSPlanetSurfaceAssetCommandlet.cpp`
- `Source/APS_ALPHA/Core/Planetary/APSWorldScapeFoliagePolicy.cpp`
- `Source/APS_ALPHA/Generation/AstroGenerator.cpp`
- Installed WorldScape 5.4 `WorldScapeRoot_Main.cpp`, especially clipmap snap/publication and material-instance refresh paths.
- User-provided Surface baseline #2 screenshot, dated 2026-08-14 04:35:39; artifact path is not available in this checkout.

### Non-regression checkpoint

- Canonical/orbital model: untouched.
- Landing, gravity, collision and existing relief: untouched.
- WorldScape plugin and shared generator/profile/catalog/material assets: untouched.
- Builds/renders: intentionally not run while another UnrealEditor owns the heavy-resource window.

### Remaining

- Receive canonical identity/seed/biome handoff from Dev 2.
- Confirm the exact screenshot-to-log binding for baseline #2 and compare the same seed orbit to surface.
- Compare actual mesh slopes against material normals/roughness and classify the hard horizon bands before changing relief geometry.
- Capture observation-only crawling evidence, including root/player/origin, snapped LOD centers, publication counts, unique LOD0, render/collision/noise height and same-camera settings.
- After ownership handoff, run focused Lit/Unlit/Wireframe and temporal/material A/B classification; compile-only will not count as visual PASS.

## SURF-002 - Starting base surface placement contract

- Parent epic: Civilization Gameplay Materialization & Starting Base
- Type / priority / owner: Story / P0 / Dev Surface for placement only
- Status / result: In progress / PARTIAL; audit and proposed contract complete, runtime vertical slice not implemented
- Dependencies: Dev 2 canonical civilization/home-planet IDs and seed; Dev Civilization runtime manifest/asset/save contract; resource-safe UE asset and render window
- Active freeze: Dev 2 owns `Source/APS_ALPHA/Generation/AstroGenerator.cpp` and `Source/APS_ALPHA/Tests/APSMainMenuPreviewSmokeTests.cpp`; Dev Surface will not edit them until freeze is lifted.

### Done and checked

- Reused the existing canonical path: `USpawnParameters` carries selected character, spaceship, station, headquarters and shipyard classes plus civilization/fleet/infrastructure values; `CurrentCivilization` reaches gameplay.
- Existing runtime hierarchy has duplicate rejection, partial-spawn rollback and exact-class/attachment validation. Recent log evidence shows exactly-once hierarchy creation with one selected ship and three infrastructure actors.
- Found the ground-placement defect: `GroundOutposts` spawn `AColony` at a fixed `planetRadius + 25000 cm` along a deterministic direction. The path does not sample WorldScape height, liquid, slope, LOD0 readiness, collision or footprint clearance.
- `AColony` is already a clear primitive placeholder (cylinder foundation, dome and towers), suitable for a foundation slice after placement is corrected. It is not final art and has no landing pad.
- Legacy short list exists under `Content/APS/APS_ALPHA/Core/Stations`, `Core/Spaceships` and duplicated older Blueprint paths. Tiny duplicated assets may be redirectors/stubs; visual/class/dependency validation requires a later resource-safe Editor audit.
- Generic save data stores actor name, class, transform, parent and byte payload, but the audited path does not yet prove stable civilization/home-planet/role IDs, manifest revision, persisted surface placement or reload no-duplicate behavior.

### Proposed Surface to Civilization contract

- Input owned by callers: home planet actor/body, canonical placement seed or persisted placement key, footprint radius, pad/ship/player clearances, maximum slope and minimum water margin. Dev Surface does not create a second civilization or planet ID.
- Output owned by Surface: deterministic world transform, local outward/up vector, terrain and collision heights, fitted footprint slope/deviation, liquid/water margin, LOD0/collision readiness, deterministic sample key and placement-contract version.
- Algorithm: reuse the accepted landing candidate rules; sample the complete base/pad/ship footprint against the same WorldScape noise/collision contract; fit a tangent plane; reject steep, wet, clipped or obstructed sites; derive rotation from planet outward plus deterministic tangent heading. Never use global Z for surface offsets.
- Surface returns placement evidence only. Dev Civilization selects/spawns actors, binds IDs/faction, persists transforms and enforces idempotency.

### Acceptance and non-regression

- Same canonical planet/civilization input or persisted key resolves the same site and local frame.
- Base, pad, selected ship and character clear terrain/water and each other; character has a walkable route.
- Natural planetary gravity remains authoritative; no artificial gravity is added to surface structures.
- Existing orbital starter hierarchy, selected ship class, landing/gravity and WorldScape readiness remain unchanged.
- No shared source or asset was edited in this checkpoint; no build/render was run.

### Remaining

- Get a short contract acknowledgement from Dev Civilization and canonical ID/seed handoff from Dev 2.
- After the `AstroGenerator.cpp` freeze, decide whether the accepted landing resolver is extracted into a shared Surface service or wrapped without duplicating its rules.
- Implement the resolver in isolated Surface-owned files with deterministic math tests, then hand the result to Dev Civilization for actor/save integration.
- Run resource-safe asset validation, source tests/build, same-seed surface capture and reload/no-duplicate acceptance with Dev Civilization.


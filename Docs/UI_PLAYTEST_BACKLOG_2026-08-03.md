# APOSFERA UI / Generation playtest backlog — 2026-08-03

This checklist records the complete mass-playtest brief. Items stay here until they are implemented and verified in a real playtest.

## 0. Global product and implementation rules

- Treat the menu flow as the first playable phase of the game, not as a detached settings frontend: the player is already observing and shaping the same persistent full-scale world that gameplay will use.
- The center of astronomical generation is always the real live scene. Do not cover it with a large translucent placeholder panel, fake screenshot or disconnected preview image.
- Keep all gameplay and UI logic in C++/Slate for this sprint. Do not require manual UMG or Blueprint graph edits to connect new behavior.
- Reuse and connect the existing generators, world data, actors, gravity/gameplay systems and authored levels. Do not replace working architecture merely to restyle the UI.
- UI changes must immediately update the underlying generation model and the visible scene. Scene selection must immediately update the UI selection and parameter context.
- Preserve a single persistent world/model identity through astronomical setup, civilization setup, save creation and the eventual gameplay level.
- Every expensive operation must be explicit, asynchronous or preview-capped. Opening a page, selecting a tab, moving a camera or editing one local value must not introduce visible multi-second stalls.
- Keep visual language consistent across Landing, Choose Path, Existing Worlds, Astronomical Generation, Civilization Parameters and the later strategic map: dark layered glass, cyan information hierarchy, amber primary/selected actions, chamfered geometry and clear hover/focus states.

## 1. Landing screen

- Preserve the authored live background, but fix the incomplete/cut planetary surface and missing planet chunks.
- Investigate remaining star flicker visible through/under the atmosphere without destabilizing the working gameplay renderer.
- Make the selected `Single Game` entry compact instead of stretching far beyond its label.
- Connect the already existing Settings screen to a styled, clickable entry.
- Connect Profile to a real page/state instead of leaving dead text.
- Give Settings, Profile and Quit consistent hover, pressed and selected feedback.
- Keep all authored actors intact when leaving the menu.

## 2. Choose Your Path

- Match the mockup proportions, padding, type scale and card hierarchy.
- Use cut/chamfered sci-fi corners, edge highlights and restrained cyan/amber glow.
- Give every card title and description a uniform reserved height.
- Add a readable icon/badge before every card title and a clear navigation arrow/state.
- Replace flat debug-looking dark overlays with layered translucent styling and subtle depth.
- Keep placeholder imagery correctly cropped/aligned until final art replaces it.
- Preserve hover/pressed/disabled states and make Story Mode visibly locked.

## 3. Start Single Game — blocking

- `Launch Game` must only open `/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation`.
- Do not run procedural astronomical generation, move authored actors or spawn an extra WorldScape surface.
- Preserve the authored Headquarters, Blueprint stations, ships, planet, sky and player start exactly as stored in the map.
- Do not reuse generation/civilization/save state from a previous menu session.

## 4. Visit Existing World

- Match the supplied mockup: padding, fonts, icons, section hierarchy, highlights and buttons.
- Use three smaller save cards per row; text does not need the current oversized cards.
- Narrow the right details panel substantially while keeping all useful data readable.
- Keep save discovery non-blocking; entering this screen must never freeze on large `.sav` files.
- Preserve search, sorting, favorites, pagination and filters; deeper filter UX can be polished after layout.

## 5. Generation route semantics

- `Generate Civilization`: astronomical generation -> civilization setup -> committed persistent gameplay world.
- `Generate Space`: astronomical generation -> generate/enter the space world without forcing civilization setup; later use a lightweight spectator/flying pawn or ship.
- `Create Planet`: open the same live generation foundation focused on planet creation with only relevant controls.
- All three routes may share code and data, but their Continue actions and visible settings must reflect their actual purpose.

## 6. Astronomical Generation — navigation and controls

- Present the main hierarchy in the logical order `Galaxy -> Cluster -> System -> Planet` (Overview may remain a separate global action).
- Replace the two giant always-visible control lists with a switcher: show only controls relevant to the selected hierarchy level.
- Each level needs a short explanation, meaningful values, presets and clear live-preview feedback.
- Changing a value inside the current level must not reset or jump the camera.
- Focus transitions between hierarchy levels must be smooth, deterministic and correctly framed.

### Galaxy

- Validate every galaxy type/class (`lenticular`, `spiral`, `barred`, etc.) against the actual generator.
- Make galaxy type, class, size, star count and density visibly affect generation.
- Support meaningful large values, including millions/tens of millions of modeled stars, while preview rendering remains bounded and performant.
- Generate both the galaxy hierarchy and its clusters; a cluster belongs inside a galaxy instead of replacing it.

### Cluster

- Keep size, type, population and composition meaningful and connected to generation.
- Add curated formation/shape presets in addition to raw parameters.
- Ensure stars/systems do not overlap: use generated system radii plus a configurable dead zone/minimum separation.
- Preserve model-level data even when preview instances are capped.

### System

- Fix the current System/Planet focus mix-up.
- Frame the entire star-system radius, not only the star or home planet.
- Render system orbits, planets and the existing in-world marker/label language in the preview.
- Add a readable hierarchy/list of planets and their moons.
- Allow clicking a planet in the scene or the hierarchy to focus it and expose its parameters.
- Selecting a planet must not randomly regenerate unrelated stars, orbits or planets.

### Planet

- Focus the selected planet, not always only the generated home planet.
- Show/edit the planet type, radius, atmosphere and moon data relevant to that selected planet.
- Fix the semi-transparent/placeholder WorldScape sphere in preview.
- Keep full WorldScape generation deferred/optimized while presenting a stable representative preview.

## 7. Scale and camera rules

- Use one explicit visualization policy per focus: true full scale where readable, otherwise a documented uniform preview normalization.
- Never mix unscaled stars with scaled planets so that thousands of stars appear inside a planet.
- Preserve hierarchy-relative positions and radii; prevent bodies and systems from intersecting.
- Derive focus distance from the selected bounds and FOV so every target fills the intended screen area.
- Parameter edits keep orbit/zoom; hierarchy changes animate to the new deterministic framing.

## 8. Civilization Parameters

- Match the dedicated civilization mockup: borders, spacing, cyan/amber hierarchy, icons, readable controls and bottom action.
- Replace the single carousel-only presentation with square card grids for Character, Spaceship, Space Station, Headquarters and Shipyard.
- Every option card should show image/fallback art, name, short description and selected state.
- Support several visible options plus vertical scrolling, so the player understands what is being selected.
- Keep every existing civilization/spawn/infrastructure/division parameter connected to the runtime model.
- Keep the final generated world persistent and use the chosen actor classes without manual Blueprint wiring.

## 9. Verification gates

- Build `APS_ALPHA Win64 Development` after each coherent code batch.
- Never require manual UMG/Blueprint edits for this sprint.
- Do not modify the authored maps or existing user-edited Blueprint assets unless explicitly approved.
- Test routes independently so a preview/generation session cannot contaminate Single Game or Existing World loading.

## 10. Detailed interaction checklist from the full playtest transcript

### Landing and route cards

- Keep the large Start Single Game card visually dominant, but do not let it distort the proportions of the smaller route cards.
- Make the whole route card clickable, not only its label or arrow, and provide immediate hover/pressed feedback without waiting for asset loads.
- Wire every advertised route (`Start Single Game`, `Visit Existing World`, `Generate Civilization`, `Generate Space`, `Create Planet`) to a distinct and correct flow.
- Keep every card title and description in a consistent reserved-height area so different text lengths do not shift the composition.
- Use title icons, navigation arrows, chamfered edges and layered translucent depth matching the supplied mockups rather than flat debug rectangles.

### Existing-world browser

- Keep selection stable while save metadata finishes loading or the list refreshes.
- Make the selected save unambiguous and update the narrow right detail panel immediately from indexed metadata.
- Preserve image aspect ratio; never stretch planet or galaxy art into an oval simply to fill a card.
- Continue opens the exact selected save; World Details exposes its model; Delete uses an explicit confirmation state.

### Live astronomical viewport

- Keep mouse interaction active in the real center scene: drag/orbit, scroll zoom and celestial-body picking must work without fighting Slate controls.
- Show a compact viewport hint/legend for drag, zoom and selection/double-click behavior without covering the generated objects.
- Clicking empty space must not destroy the current model, change hierarchy level or reset the camera.
- Double-clicking a visible star, planet or moon drills into that exact object; Back/Up returns to the previous hierarchy context and its useful camera state.
- `Regenerate` is the explicit whole-scope randomization operation. Normal control changes update only their affected scope.
- Continue commits the world currently visible in the preview; it must not silently generate a different world during transition.
- Use real units and domains in controls: kilometres, AU/light years, integer object counts, bounded atmosphere values and readable spectral/type names.

### Hierarchy switcher

- Use the order `Galaxy -> Cluster -> System -> Planet`; keep Overview as a separate global summary if it remains useful.
- Each level uses a proper panel switcher: only that level's settings, description and presets are visible.
- Switching level changes both the settings panel and the scene focus. Editing a value inside one level must not change focus or jump the camera.
- Provide a visible hierarchy breadcrumb/Up action such as `Galaxy / Cluster / System / Planet` so current scale is always understandable.
- Preserve per-level/per-object camera orbit and zoom when moving down and back up the hierarchy.

### Galaxy details

- Expose galaxy controls only in Galaxy mode and keep star-system, home-planet and atmosphere controls hidden there.
- Validate `Spiral`, `Barred`, `Elliptical`, `Lenticular` and other offered types against genuinely different generator distributions.
- Give presets short explanations/visual expectations rather than using different names for the same random layout.
- Support millions/tens of millions of modeled stars with deterministic preview sampling/instancing instead of expensive actor creation.
- Generate and retain clusters as children of the galaxy, with stable identifiers and membership for later selection and strategic-map use.

### Cluster details

- Frame the complete selected cluster while retaining its parent-galaxy context for Up/Back navigation.
- Allow a visible star/system point to be clicked and drilled into; maintain a stable mapping from preview instance to model identifier.
- Add curated formation presets beyond a family label: compact/open, globular-like, elongated/stream, ring/arc and other supported authored shapes.
- Make size, population and composition influence actual positions, star classes and spectral colors, not only UI text.
- Prevent systems from overlapping by using modeled system radii plus a configurable minimum separation/dead zone.

### System view — required feature set

- Fix the System/Planet focus mix-up: System frames the whole generated system radius, never just the home planet.
- Display the system star, system boundary/radius, orbital planes and every generated planet together.
- Draw orbit lines through the actual body paths, attached to world space rather than screen space, using the established non-debug visual style.
- Reuse the existing marker/flag visual language for bodies: each label has a clear leader line to the actual body center, type/distance data and correct occlusion when a planet blocks it.
- Add the specifically requested clickable planet/moon list with clear `planet -> moon` nesting, selected state and enough data to distinguish similarly named generated bodies.
- Clicking a planet in the scene, its marker or its list row must select the same model object and perform the same smooth camera focus.
- Clicking/double-clicking a moon must focus that exact moon while preserving its parent planet in the breadcrumb.
- Add an on-screen hint that planets/markers/list entries are clickable and that scrolling changes scale.
- When planet count, moon count, orbit distribution or start-planet index changes, update only the relevant system branch; keep the chosen star and parent cluster stable.
- Use star/planet/moon radii plus safe orbital spacing so generated bodies and their orbit bands cannot intersect.

### Planet and moon view

- Focus the selected planet or moon, not a hard-coded home planet.
- Show and edit type, true radius, atmosphere, moon data and home/start status for the selected object.
- Expose moons as nested list/cards with direct focus actions.
- Keep center, radius, atmosphere bounds, marker and displayed data synchronized when selection changes; no stale label at the previous body.
- Apply planet parameter changes immediately to its live mesh/material/atmosphere representation while retaining camera orbit and zoom.
- Separate cheap preview representation from heavy WorldScape activation. Warm heavy terrain before it becomes visibly close and never spawn a duplicate surface in authored Single Game.
- Make Start Planet Index choose and visibly mark the corresponding generated planet rather than merely changing a number.

### Full-scale visualization and camera

- Use true full scale where readable; otherwise use one explicit uniform preview normalization for the entire current view.
- Never mix differently scaled stars and planets so that thousands of stars appear inside a planet.
- The star, planets, moons, orbital distances and markers in one view must share the same transformation policy.
- Preserve physical radii/distances in the persistent model even when visual sizes are clamped for readability.
- Derive camera target and distance from generated bounds and FOV, never from hard-coded coordinates for one preset.
- Smoothly interpolate position, orientation and zoom; no teleport, unexpected roll or camera bounce when data refreshes.
- Scene picking on a normalized preview must resolve back to the exact full-scale model object.

### Civilization selection grids

- Keep the committed live star system as the visual context behind/around the Slate layout rather than replacing it with a disconnected image.
- Replace each one-item carousel with a square, vertically scrollable grid for Character, Spaceship, Space Station, Headquarters and Shipyard.
- Each option displays preview/fallback art, name, short description, selected state and enough simultaneous alternatives to make the choice understandable.
- Load only the selected soft class/preview asynchronously; never synchronously load every Blueprint just to build the page.
- Category selection updates both summary panels and the final spawn model.
- Keep civilization name, archetype, government, economy, society, population, credits, technology, start location/orbit and infrastructure counts editable and reflected in summaries.
- Generate World validates required selections, preserves the exact astronomical model and passes the exact civilization/spawn model into gameplay.

### Strategic-map compatibility that this iteration must preserve

- Treat this generation viewport as the foundation of the later strategic map; that version swaps generation controls for gameplay/strategy controls but reuses navigation and rendering.
- Keep hierarchy IDs, model queries, scene picking, breadcrumbs, focus APIs, orbit rendering and body markers reusable outside the menu.
- Support future navigation through `galaxy -> cluster -> system -> planet -> moon/station/settlement` without rebuilding an unrelated world representation.
- Store/query generated stars, planets, moons, stations and future settlements so the navigation UI can list, select and target them.
- Preserve hooks for distance readout, active target and future route construction between systems.
- Remote systems retain lightweight stable model data; expensive actors and WorldScape content activate only when the player approaches or explicitly focuses them.

### Acceptance checks for the above

- Regression-test `Start New Game` / `Start Single Game` against the last known-good legacy-menu behavior: it must open the authored start map exactly as saved, spawn the normal player at its authored start, retain the complete authored station/Headquarters/ship/planet/sky hierarchy and perform no procedural generation or actor replacement.
- Verify the authored Single Game route both from a fresh application start and after visiting/using every generation page, so stale preview, save, civilization or spawn state cannot leak into it.
- Regression-test `Generate Civilization` through the complete travel: the chosen character, ship, space station, Headquarters, Shipyard (when enabled), civilization model, spawn parameters and generated astronomical world must all be present after gameplay begins.
- Verify the generated gameplay hierarchy is structurally intact after travel: expected parent/child ownership, world-model identifiers, home star/system/planet links, civilization reference and selected actor soft classes all resolve to the same committed generation session.
- Verify generated actors spawn once, at the intended start location/orbit and with the intended selected classes; no missing Headquarters/station, duplicate WorldScape, fallback legacy pawn or actor left at preview coordinates.
- Every hierarchy button changes both the visible control panel and the correctly framed scene subject.
- Scene click, marker click, list click, double-click drill-down, breadcrumb/Up, drag/orbit and scroll zoom agree on the same selected object.
- System view visibly contains its star, expected planets/moons, correct orbit lines, leader-line labels and a usable planet/moon hierarchy list.
- A planet edit does not regenerate its star, siblings, parent cluster or galaxy; a system edit does not replace its parent hierarchy.
- Normalized preview edits persist as unchanged full-scale values in the committed world model.
- Generate Civilization, Generate Space and Create Planet produce their different intended handoffs from the shared generation foundation.
- After visiting any generation route, authored Single Game still opens its stored Headquarters, stations, sky, planet and PlayerStart with no generated duplicate WorldScape.

## 11. Playtest delta — 2026-08-04

This section is additive. Earlier requirements remain active unless explicitly superseded below.

### Main-menu landing scene

- Reduce the primary menu button and title-row widths to roughly half of their current oversized width while preserving a clear hit target.
- The authored 3D planet/station objects were intentionally removed from `L_APS_MainMenu_Alpha` on 2026-08-04. Do not restore, regenerate or depend on them.
- Treat the level as a clean menu/generation scene. Any decorative background is UI-owned and must not contaminate generated hierarchy bounds.
- Reuse the same clean level for the interactive generation preview where practical; preserve the committed model across UI pages and only travel when entering the actual authored/generated gameplay world.

### Choose Your Path visual pass

- Make the chamfered card shape real: background, border, hover and selection layers must use the same clipped geometry, with no square corner fragments visible underneath.
- Make the right-hand card grid exactly the same total height as the large `Start Single Game` card.
- Normalize title and description baselines, internal padding, gaps and bottom action areas across all cards.
- Replace arbitrary placeholder glyphs with meaningful route icons where a suitable existing asset/glyph is available; keep the icon slot stable for later final art replacement.
- Preserve all current route bindings while improving depth, translucency, cyan/amber hierarchy and hover/pressed feedback.

### Visit Existing World visual pass

- Add visible vertical spacing between filter/category buttons.
- Increase the right detail-panel typography, especially labels such as `SYSTEM TYPE`, without making the panel wider than necessary.
- Widen `PREVIOUS` and `NEXT` controls so labels never wrap, clip or shift.
- Normalize grid, detail panel and pagination padding for a tidy, consistent composition.
- Do not add blocking save loads while polishing the layout.

### Required astronomical focus hierarchy

- Restore all six distinct focus actions in this exact order: `OVERVIEW`, `GALAXY`, `CLUSTER`, `SYSTEM`, `STAR`, `PLANET`.
- `SYSTEM` and `STAR` are separate levels and must never alias to the same subject/settings page.
- `OVERVIEW` frames the complete generated hierarchy summary.
- `GALAXY` renders a bounded sampled galaxy preview and its cluster membership; it must not be an empty view.
- `CLUSTER` frames the selected hierarchical cluster, not the home star, and must retain a stable visible point-to-model mapping.
- `SYSTEM` preserves the currently successful orbit/marker/click behavior while framing the whole selected system.
- `STAR` frames and edits the selected star independently of the system diagram.
- `PLANET` frames and edits the exact selected planet/moon and activates its representative terrain.

### Explicit preview-scale policy

- Persist all physical values in full-scale units regardless of presentation.
- `GALAXY` and `CLUSTER` use one uniform normalized transform per view, including every visible child and marker.
- `SYSTEM` uses one coherent diagrammatic transform for star radii, planet radii and orbital distances so the complete system remains readable.
- `STAR` and `PLANET` return to physically coherent object-level presentation and WorldScape-compatible units.
- Never display authored main-menu/start-level planets, stations or the legacy gas giant as children/background objects inside the generator preview.
- Changing settings inside a focus level updates the model/scene without changing focus or resetting the user's camera orbit.

### System-model corrections and hard limits

- Preserve the current attractive System view: world-space orbits, labels, clickable bodies and camera focus are protected regression behavior.
- Make `Single Star`/`Double Star` materialize one/two stars and reflect the selection in hierarchy rows and bounds.
- Stellar type/radius changes must update visual scale and orbit clearance consistently.
- Derive the first safe orbit from the combined stellar extent plus clearance; derive subsequent orbits monotonically so no orbit enters a star or overlaps another band.
- Present `SYSTEM BODIES / LIVE MODEL` as spaced, selectable rows/cards with hierarchy, selected state and an explicit affordance for adding a planet.
- Cap planet radius at `20,000 km`, planets per system at `20`, and moons for the edited planet at `10` in both Slate controls and the view-model/model boundary.
- Visibly mark and optionally focus the selected `Start Planet Index` so the field is not an invisible data-only change.
- Keep preview actor/instance work bounded when counts change; a 20-planet system must remain responsive.

### Planet selection and WorldScape preview lifecycle

- Planet/marker/list clicks select the same stable model identifier and edit that exact body.
- Precreate or preload a representative WorldScape surface for every body in the currently previewed system, but keep non-selected surfaces frozen and non-updating.
- Activate/unfreeze/update only the selected planet or moon; freeze the previously selected surface rather than destroying the hierarchy.
- Planet type, radius and atmosphere edits refresh the selected terrain/material/atmosphere without regenerating its star, siblings, system, cluster or galaxy.
- Do not show a generic dark sphere once a selected body's representative surface is available.
- Keep the generated preview isolated from the authored main-menu background and prevent duplicate WorldScape roots.

### Generated-world handoff

- `Continue to Civilization` and `Generate World` must commit the exact visible hierarchy and current selected-body data.
- Generated gameplay must receive the same galaxy/cluster/system/star/planet IDs, selected civilization options and spawn settings; it must not silently fall back to unrelated map actors.
- `Start Single Game` remains a completely separate authored route and must still load the legacy authored start map unchanged after any generator session.

### Acceptance checks for this delta

- Landing remains clean after the authored 3D backdrop removal; opening generation creates only the requested preview hierarchy.
- Choose Path cards have truly clipped matching corners and equal column heights at the target viewport resolution.
- Existing Worlds typography and pagination remain readable without overflow.
- All six hierarchy buttons are visible and each frames the correct subject.
- Galaxy and Cluster are visible; Cluster never focuses the home star by mistake.
- System view retains its current working orbits, labels, picking and smooth camera behavior.
- Double Star visibly produces two stars; orbit paths remain outside the full stellar extent.
- UI/model reject radius above 20,000 km, planet count above 20 and moon count above 10.
- Selecting/editing one planet changes only that planet's active WorldScape representation.
- Generated Civilization launches the same committed hierarchy; authored Single Game still launches its original map hierarchy.

### Cluster and system additions from the follow-up playtest

- Preserve the current normalized Cluster/System presentation and improve it incrementally; do not replace the established generator architecture wholesale.
- In Cluster focus, mark the generated home system with the same clear flag/leader visual language used by the system navigation overlay.
- Make the right context panel focus-dependent: Cluster shows a selectable list of stars/systems instead of `SYSTEM BODIES` planet rows.
- Clicking a Cluster list row selects/highlights the corresponding stable model entry and smoothly frames it; drilling in moves to System without regenerating the cluster.
- Galaxy remains a separate hierarchy scope and must gain a visible bounded preview even though the legacy implementation previously modeled it mostly as data.
- In System focus, draw an explicit outer system boundary and an explicit stellar extent/safety boundary.
- Keep unrelated normalized cluster/galaxy stars outside the System view; they must not appear spawned inside the selected system.
- Preserve the currently successful orbital layout, Habitable Zone behavior and type distribution (inner terrestrial/continental bodies, habitable/ocean candidates, outer frozen bodies).
- Scale stellar extent, safe orbit clearance and outer system boundary together so changes never push orbit bands through a star.

### Preview-scene lifecycle additions

- Landing and Choose Your Path share the same live astronomical preview scene. The generated star field/space background is visible on Landing instead of appearing only after entering Single Game.
- Keep the user-cleaned `L_APS_MainMenu_Alpha` free of restored authored planets/stations; the background comes from the existing preview-generation path, not re-added level actors.
- A WorldScape/planet terrain root must never appear at the system origin behind or around the star. Terrain actors stay excluded/frozen outside Planet focus and activate only for the selected planet preview.
- System/Cluster normalized framing ignores terrain-root bounds so a planet surface cannot contain the whole generated system.

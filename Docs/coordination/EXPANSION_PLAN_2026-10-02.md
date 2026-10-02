# Expansion plan (Claude flight, 2026-10-02)

Rio's requests of 02.10, 04:30–05:30, in one plan. Status lines are updated as work lands.

## What Rio asked for
1. Colonize the other star systems of the cluster: scan / probe / visit in person / send a unit, then a beacon,
   relay, outpost or station; beacons of star systems on the maps and the HUD.
2. Cluster density: stars sit inside the home system's orbits. Dense compact systems for cluster stars, planets only
   where there is room, system spheres must not overlap; the home system stays realistic.
3. F10 strategic map = the main screen for expansion, management and development, in 3D: every real object
   labelled (planets, moons, surfaces, stations, outposts, ships), types, links, orbits, lines; a star list on the
   right; click a star for its parameters, build routes. Fix: HOME SYSTEM falls into the star, the planet is
   see-through, FPS.
4. Infrastructure: many more structures by division (mining, science, civil affairs, diplomacy, civil rights,
   military, fleet), transport structures, relays, megastructures; an interaction map for the infrastructure
   (transport first); manage and build them from every screen.
5. Anomalies: more than five kinds, clear detection, chains of activity; department missions of varied kinds
   (check, solve, find, install, fly, learn), offered by each division.
6. Build mode on a surface or in orbit: a separate mode, pick an object and drag it with the mouse; surface-aligned
   on a planet, free rotation in orbit; small objects here, strategic ones through the terminal.
7. Readability: DIVISIONS as visual parameters; system names readable; select any system cheaply.

## Architecture
Plain C++ runtime owned by `UAPSFleetCommandSubsystem` (no new UCLASS needed, so every step compile-checks before
UHT runs); state saved in the civilization save (`APSCivilizationSave`, version 5 block after the fleet extras).

| Part | Files | Owner |
|---|---|---|
| Star systems: catalogue view of the cluster (names, positions, room, home), knowledge, claims, network, anchors | `Gameplay/Expansion/APSStarSystems.*` | Claude flight |
| Infrastructure catalogue: structure types by department, placement, needs, build time, effects, costs | `Gameplay/Expansion/APSInfrastructureCatalog.*` | Claude flight |
| Economy: stocks and rates (metals, volatiles, energy, research, influence) from structures; costs | `Gameplay/Expansion/APSEconomy.*` | Claude flight |
| Fleet orders at stars (probe, survey system, build structure), structure spawn and save | `Gameplay/Fleet/APSFleetCommand.*` | Claude flight |
| Missions: each division offers missions from templates; objectives tracked from events; rewards | `Gameplay/Expansion/APSMissions.*` | Claude flight |
| Anomalies v2: planetary and deep-space kinds, detection, chains | `Gameplay/Fleet/APSFleetAnomalies.cpp` + missions | Claude flight |
| F10 strategic map v2: own strategic camera (no generator preview), overlay labels for every object, orbits, routes, territory, star list, object card, actions | `UI/StrategicMap/*`, `Core/Controllers/GravityPlayerController.cpp` (F10 toggle only) | agent |
| Infrastructure screen v2: catalogue by department, network/transport map, details and actions | `UI/Colony/SAPSInfrastructurePanel.*` + terminal hook | agent |
| Build mode on surface/orbit | `UI/Build/*`, `Gameplay/Colony/APSColonyConstructionSubsystem.*` (explicit transform), input hook | agent |
| Surface map (globe in ORDERS, survey gating, zoom, picks, modes) | `UI/Colony/SAPSSurfaceMap.*` + terminal hooks | agent (running) |
| DIVISIONS visual cards | `UI/Colony/SAPSDivisionsPanel.*` + terminal hook | agent (running) |
| Cluster density (dense systems, home clearance with dataset migration) | generator | Claude flight, after the above |

## Rules for everyone working on this plan
- Compile-check every edit batch; never leave Source failing.
- No writes while Rio's UBT build runs; one compile at a time.
- Do not touch another part's files; announce shared-file edits here or in PLANET_EDITOR_WINDOW.md.
- Saves: append, never renumber (EOrder values, save blocks).
- Seeds, StableIds, accepted visuals and the home system stay as they are.

## Status
- 05:2x: plan written; core headers next; UI agents start against the headers.

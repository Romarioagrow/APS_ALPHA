# Megastructures and space hubs

Status 2026-10-03: design plus code in `Source/APS_ALPHA/Gameplay/Megastructures/` and the infrastructure catalogue/runtime
(plain C++, no new UCLASS/USTRUCT/UPROPERTY, appended enum values only). Every changed file passed the cl syntax/semantic
check; **not built, not run, not seen in PIE**. Owner: Claude (megastructures session). Nothing committed.

## Кратко (для Rio)

- **Что стоит на ручной карте** (`L_APS_SinglePlay_StartLocation`, офлайн-разбор файла): у **луны** (R 637 км) —
  бронзовое **кольцо** `SM_Megastructure_P1_23` (Ø ≈ 2 400 км, 1,88 радиуса луны) и **космический лифт**
  `SM_MERGED_BP_SplineElevator_Vert36_2` (стоит вертикально на её поверхности); на **родной планете** (R 2 000 км) —
  **башня с кольцом-воротником** `AI_Buildings/02` (масштаб 10 000 → 6 км) и золотой шпиль `AI_Buildings/01`; рядом с
  HQ — **дисковый хаб со шпинделем** `AI_Stations/01` (11,5 км), **шпиль с фермами** `P1_13` (6 км), **колесо** `P1_19`
  (29 км), **крестовый хаб** `P1_20` (67 км), **звёздная станция** `P1_21` (≈ 900 км) и газовый гигант `AI_Planet/01`.
  Блюпринтов у этих мешей нет — только эта карта. Сама карта не менялась и не перегенерируется.
- **Новый ярус — ХАБЫ** (категория HUB, свой знак-октагон на карте инфраструктуры):
  **SPACE HUB** — дисковый хаб с карты в авторском размере (11,5 км), высокая орбита мира, шпиндель вниз к планете;
  **GRAND HUB** — крестовый хаб P1_20 (67 км). Хаб ускоряет работу во всей системе (+20 % / +30 %), даёт +2/+3 места
  для станций у своего мира, ресурсы, скорость флота и строительства.
- **Цепочки** (каждый шаг открывает следующий, в каталоге видно «LOCKED, REQUIRES …»):
  станция над миром → **SPACE HUB** → **GRAND HUB**;
  станция над миром → **SPACE ELEVATOR** (башня на экваторе, трос до противовеса на стационарной орбите) →
  **ORBITAL RING** (кольцо по экватору, трос лифта проходит через него) → **DYSON SWARM SEGMENT** (кольцо коллекторов
  вокруг звезды, нужно кольцо в системе, до 5 шт.) → **DYSON SPHERE** (нужны все 5 сегментов).
- **Стройка** — как вся инфраструктура: заказ в каталоге (BUILD...), строительный корабль летит, стоимость списывается.
  Корабль встаёт **рядом со своей стройкой** (над хабом, у троса лифта, у кольца), и пока он работает, на месте растёт
  **каркас**: сегменты кольца появляются по одному от точки лифта в обе стороны, башня лифта поднимается, трос растёт
  вверх, у хаба собирается обод и шпиндель. По готовности каркас заменяется авторскими мешами. Вручную (режим
  строительства B) хабы и мегаструктуры не ставятся — только кораблями.
- **Связано с остальной игрой**: хаб считается станцией мира (для верфи/штаба флота); на карте F10 хабы — станции, а
  орбитальное кольцо рисуется кругом вокруг своего мира; на странице объекта видны радиус кольца, высота противовеса и
  шаг цепочки («STEP 2 OF 5 / NEXT: ORBITAL RING …»); миссии: «A HUB FOR THE SYSTEM» → «THE GRAND HUB» (транспорт),
  «CLOSE THE SPHERE» (индустрия), а «THE RING PROJECT» теперь говорит, что сначала нужен лифт.
- **Размеры** считаются от мира: кольцо на 1,88 радиуса (как у луны на карте), противовес — на стационарной орбите по
  Кеплеру из плотности мира и его «суток» (у Земли вышло бы 6,6 R; ограничено 2,2–9 R), всё ниже ближайшей луны;
  если луна слишком близко — каталог так и говорит «No room …».
- **FPS**: ничего не тикает; дальность прорисовки ограничена (хаб ~2 600 км, кольцо 40 своих радиусов), тени выключены,
  коллизия только у башни лифта (CVar `aps.Mega.Collision`). Меши грузятся заранее, пока идёт стройка.
- **Сейвы не менялись**: мегаструктура сохраняется как обычная постройка (тип, место, трансформ корня); каркас
  восстанавливается из сохранённого прогресса корабля.
- **Проверить в игре**: `aps.Mega.Raise SpaceElevator`, `aps.Mega.Raise OrbitalRing`, `aps.Mega.Raise SpaceHub`
  (без затрат, на мире с колонией), `aps.Mega.Preview OrbitalRing 0.4` (каркас на 40 %), `aps.Mega.List`.
  Точные размеры мешей и форму лифта даст запрос `F:/ChatGPT/APOSFERA/work/megastructures/run_mega_query.ps1`
  (NullRHI, только чтение) — его ещё не запускали.

## 1. The request

Rio, 03.10 (with screenshots from the hand-made level): "Megastructures of more or less planetary scale; also system scale,
like a Dyson sphere. Space hubs: not the stations we usually build, huge, enormous ones. Megastructures buildable in chains:
a space elevator, then a space ring, one after another. Reuse what is already placed on the hand-made level and integrate
it."

## 2. Phase A: what the authored level holds

Source: `Content/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation.umap` (the "Start Single Game" route,
`UAPSAuthoredLevelLaunchSubsystem::MapPackage`). A classic level (no World Partition, no `__ExternalActors__`), 119 actors.
Read offline with the scratchpad's package parser (`mega/scan_level.py`: name/import/export tables and the tagged
properties of every actor and component export), mesh sizes from the simple-collision hulls (`mega/collision_box.py`,
`mega/hull_views.py`) and the editor thumbnails (`mega/thumbs2.py`). The scripts and their output live in the session
scratchpad (`.../scratchpad/mega/`).

Bodies: `BP_HomePlanet` (BP_Planet_C, RadiusKM 2000; WorldScapeRoot `WSR_HomePlanet`, PlanetScale 200 017 792 cm, ocean) at
world (488, -814, -1795) km; `BP_Moon_C_1` (RadiusKM 637, `WSR_HomeMoon1`) attached to the planet, 6 435 km from its
centre (3.2 planet radii), at (973, -5 723, 2 338) km. The HQ (`BP_SpaceHeadquarters1`) sits at the world origin, ~30 km
over the planet.

| Actor (label) | Mesh (package) | Mesh size (cm) | Authored scale | Size there | Where |
|---|---|---|---|---|---|
| Megastructure_P1_23 | `AI_Shpis/Pack_1/Megastructure_P1_23/SM_Megastructure_P1_23` | hoop: 10 thick (axis X) x 60 across, pivot under it | 399 993 x 4 000 000 x 4 000 000 | Ø 2 400 km, 40 km thick | round the **moon**: centre 34 km off the moon's, radius 1 200 km = **1.884 moon radii**, axis X |
| SM_MERGED_BP_SplineElevator_Vert36_2 | `APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_BP_SplineElevator_Vert36_2` | hulls 328 x 240 x 200 m (render bounds unknown) | 10, 10, 13.5 | ≥ 33 x 24 x 27 km | on the **moon**, 641.6 km from its centre (4.6 km over the ground), its Z along the local vertical |
| 88d53275...083a3 | `AI_Buildings/02/88d53275bba1c5c4b2730187583c083a` (tower with a ring collar) | 28 x 28 x 60 | 10 000 | 2.8 x 2.8 x 6 km | on the **home planet**, 2 000.4 km from its centre; also at 50 (30 m, planet) and 10 (6 m, moon) |
| 4c83ddeb...252c | `AI_Buildings/01/4c83ddebed96114d349f5a2081ea252c` (golden spire) | no hull | 10 000 | ? | home planet surface (2 000.8 km) |
| 501047c0... x4 | `AI_Buildings/03/501047c0ce61f93495d7986e18704564` (pavilion) | no hull | 20 / 5 | small | planet and moon surface |
| 73a7128a...c9 | `AI_Stations/01/73a7128ad60fd404a858c8197ba7d1c9` (**disc hub, spindle**) | 115 x 115 x 57 (spindle +Z) | 10 000, roll ~205° (spindle down) | 11.5 x 5.7 km | 50.7 km from the HQ |
| Megastructure_P1_13 | `.../Megastructure_P1_13/SM_Megastructure_P1_13` (spire hub with arms) | 41 x 66 x 60 | 10 000 | 4 x 6.6 x 6 km | 100.8 km from the HQ |
| Megastructure_P1_19 | `.../Megastructure_P1_19/SM_Megastructure_P1_19` (wheel round a hub) | 59 x 61 x 53 | 50 000 | 29 km wheel | 158 km |
| Megastructure_P1_20 | `.../Megastructure_P1_20/SM_Megastructure_P1_20` (four-armed hub) | 67 x 67 x 59 | 100 000 | 67 km | 974 km |
| Megastructure_P1_21 | `.../Megastructure_P1_21/SM_Megastructure_P1_21` (star-shaped spire station) | 86 x 90 x 60 | 1 000 000 | 860 x 900 x 600 km | 17 485 km |
| 4e2c6bfb...3443 | `AI_Planet/01/4e2c6bfb2c238d0b318e0ac8e36d3442` (banded gas giant sphere) | 60 | 150 000 000 | Ø 90 000 km | 100 364 km from the planet; `AtmoScape3` (PlanetRadius 44 700) rides it |

Also: the ordinary stations (`BP_STATION_LaunchStation` x3, `BP_STATION_HANGARS`, `BP_Station_PadsStation-PRE2`,
`BP_SpaceShipyard`, `BP_SpaceHeadquarters` + `SM_MERGED_StationDockingHub`), all attached to the HQ, and 30 ships.
Unplaced hub candidates in Content: `AI_Stations/03` (platforms + hanging spindle, 110 x 110 x 76 cm; `03_01`/`03_02` the same
shape in other materials) and `AI_Stations/04` (three rings on a spindle, 49 x 51 x 120 cm). No Blueprint references any of
these meshes; besides this level only `L_APS_Start-Stations` and `L_HomeStation` use the spline elevator.

The Pack_1 meshes are AI-generated, normalised to ~60 cm, without thumbnails; the `AI_Stations`/`AI_Buildings` ones have them.
The meshes are 17–46 MB each (high-poly, Nanite and LOD count unknown offline). In the level they wear their own material
(`Material_001` + `texture_pbr_*` in each mesh's folder, no overrides) and the default collision (BlockAllDynamic on the
movable ones, BlockAll on the ring): there the ring's and the hubs' simple hulls block.

**Not knowable offline** (needs the engine): the render bounds (the packages keep no `ExtendedBounds`; the sizes above are
the collision hulls'), LODs, Nanite, triangle counts, and above all the spline elevator's real shape (its thumbnail is black
with a small sphere at the bottom: likely a long thin cable whose hulls cover only some nodes). The ready read-only query:
`F:/ChatGPT/APOSFERA/work/megastructures/mega_level_query.py` with the runner `run_mega_query.ps1` (UnrealEditor-Cmd
-run=pythonscript -NullRHI; refuses to start while an editor, game, shader worker or UBT runs; loads the map and the meshes,
saves nothing) → `mega_level_query.json`: every interesting actor's class, label, world transform, attach parent, its body and
altitude in that body's radii, every static mesh component's mesh, world transform, bounds, collision, shadow, draw distance;
per mesh the bounds, LODs, Nanite, vertex count, simple collision count, materials, the vertex spread along its longest axis
(cable or not) and the radial spread round its thinnest axis (the ring's inner and outer radius). **Not run yet.** The runtime
code does not depend on it: every fit reads the mesh's bounds at spawn time.

## 3. Phase A: what the generated game already had

- `Gameplay/Expansion/APSInfrastructureCatalog.*`: 28 types; OrbitalRing, SpaceElevator, DysonSwarm and JumpGate existed
  "only in the catalogue": built, they spawned the home complex's headquarters/station Blueprint scaled x2.5–4.
- `Gameplay/Expansion/APSInfrastructure.*` (`FAPSInfrastructure`, owned by `UAPSFleetCommandSubsystem`): `CheckBuild`
  (placement, knowledge, mission unlock, `RequiresAtSite`, department level, limit, cost), `Reserve`/`Refund`,
  `Complete` (spawn + record + journal + missions), `CompleteAt` (build mode), saves (`FAPSInfrastructureSaveData` v2 in the
  civilization save's expansion blob), surface settling.
- `Gameplay/Fleet/APSFleetCommand.cpp`: order BUILD (`EOrder::BuildStructure`) → the nearest free construction ship →
  Departing → Transit → Arrive (work length = `BuildSeconds / (1 + 0.2 level + build bonuses + local work bonus)`) →
  `FinishWork` → `FAPSInfrastructure::Complete`. Progress and the type are saved with the unit.
- UI: `SAPSInfrastructurePanel` (network map with a shape per category, catalogue cards with NEEDS/COSTS/GIVES, the site
  picker with the runtime's refusal per place), `SAPSObjectPage` (object page; `APSObjectActions` "Build.<type>" actions
  with the same refusals). The strategic map (F10) lists built structures with their category and department colour.
- Stations (`ASpaceStation` family BPs: R2, shipyard B, HQ Alpha) are spawned by class at their Blueprint scale; the fleet
  counts them per body (`FAPSFleetCommand::CountStructures`).

## 4. Design

### 4.1 Types

| Id | Name | Dept / category | Where | Needs (chain) | Level, knowledge | Time | Cost | Per minute | Effects |
|---|---|---|---|---|---|---|---|---|---|
| SpaceHub | SPACE HUB | Transport / **Hub** | HighOrbit | an orbital station over the world | Transport 2, surveyed | 240 s | 400 M, 150 E, 40 I | 4 M, 2 E, 3 I | build +10 %, fleet +5 %, work there +50 %, work in the system +20 %, +2 station berths there |
| GrandHub | GRAND HUB | Transport / Hub | HighOrbit | SPACE HUB at the world | Transport 3, studied | 420 s | 900 M, 400 E, 60 R, 120 I | 8 M, 6 E, 6 I | build +10 %, fleet +10 %, work in the system +30 %, +3 berths |
| SpaceElevator | SPACE ELEVATOR | Industry / Megastructure | AroundWorld | a station over the world, solid ground, mission unlock (CIVIL AFFAIRS "THE VOICE OF THE PEOPLE" or the ancients' chain) | Industry 3, studied | 360 s | 600 M, 250 E, 50 I | 10 M, 2 I | work at the world +30 % |
| OrbitalRing | ORBITAL RING | Industry / Megastructure | AroundWorld | SPACE ELEVATOR at the world, unlock (INDUSTRY "FILL THE STORES") | Industry 3, studied | 480 s | 1000 M, 400 E, 80 R | 20 M, 5 I | build +20 % |
| DysonSwarm | DYSON SWARM SEGMENT | Industry / Megastructure | StarSystem | ORBITAL RING in the star system, unlock (SCIENCE "STELLAR CENSUS") | Industry 3, surveyed system | 420 s | 900 M, 150 R | 60 E | up to 5 per star |
| DysonSphere | DYSON SPHERE | Industry / Megastructure | StarSystem | 5 x DYSON SWARM SEGMENT at the star | Industry 3 | 900 s | 3000 M, 800 E, 400 R, 100 I | 300 E, 10 R | build +25 %, fleet +10 % |

Ids OrbitalRing, SpaceElevator, DysonSwarm and JumpGate keep their names (saves and missions name them). New fields of the
plain `APSInfrastructure::FType`: `RequiresAtSiteCount`, `RequiresInSystem`, `bNeedsStationHere`, `bNeedsGround`, `bFleetOnly`,
`HubBerths`, `SystemWorkSpeed`. Appended enum values: `ECategory::Hub` (before `Count`), `EPlacement::HighOrbit`,
`EPlacement::AroundWorld`, `EVisual::SpaceHub..DysonSphere` (none of them is saved).

Reserved for later tiers (authored, not used yet): the wheel `P1_19` (29 km, an orbital habitat wheel), the star station
`P1_21` (900 km: a system-scale citadel), `AI_Stations/03` and `/04`, the golden spire `AI_Buildings/01`.

### 4.2 Chains

`APSInfrastructure::GetChain` lists them; `ChainRefusal(Type, FChainState)` is the one rule (pure, tested), fed by
`CheckBuild` with the live counts:

- **station → SPACE HUB → GRAND HUB**: `bNeedsStationHere` (a catalogue station or hub at the body, or any fleet
  station/shipyard/HQ there: the generated home complex counts) → `RequiresAtSite = SpaceHub`.
- **station → SPACE ELEVATOR → ORBITAL RING → DYSON SWARM → DYSON SPHERE**: `bNeedsStationHere` →
  `RequiresAtSite = SpaceElevator` → `RequiresInSystem = OrbitalRing` (`FAPSInfrastructure::CountInSystem`: a planet or moon
  belongs to the home system) → `RequiresAtSite = DysonSwarm`, `RequiresAtSiteCount = 5`.

Refusals read "Requires SPACE ELEVATOR here first.", "Requires 5 x DYSON SWARM SEGMENT here first (now 3).", "Requires
ORBITAL RING in this star system first.", "Requires an orbital station over this world first (...)". The order in
`CheckBuild`: placement, giants, knowledge, mission unlock, **chain**, department level, limit (+ hub berths), **room**, cost.

### 4.3 Placement (`APSMegastructures::PlaceRoot`, `ComputeLayout`)

Everything scales from the body's radius `R` (`GetWorldScapeBodyRadiusCm`, the full-scale world):

- **Clearance** `C`: the nearest neighbour's surface from the body's centre (a planet: its moons; a moon: its planet and, for
  sister moons, the difference of the orbits). 0 = unlimited.
- **Ring radius** = min(1.884 R, 0.8 C); fits when ≥ 1.3 R. 1.884 is the authored ring's ratio round the level's moon.
- **Stationary orbit**: Kepler with M = ρ·4/3·π·R³, so r/R = (G ρ T² / 3π)^⅓ (Earth 5.51 g/cm³, 23.93 h → 6.62; Mars → 6.03).
  ρ: the body's `PlanetDensity` / `MoonDensity` when generated, else by type (giants 1.3–1.6, ices 2.4, oceans 3.5, metal
  7.5, rock 5.5). The generator gives the worlds no spin, so the day is the world's own: 18–36 h from a hash of its stable key.
  Counterweight radius = clamp(ratio, 2.2, 9) R, at least 1.2 x the ring (the tether crosses the ring), at most 0.75 C;
  the elevator fits when that floor is reachable.
- **The world's meridian** (`WorldMeridian`, does not move while things are built): through the colony on this body, else
  under the civilization's station over it, else a longitude from the world's key.
- **Hubs**: 1.6 fleet slots out (`APSFleet::SlotRadius`, the administration hub's high orbit; ≈ 2–2.4 R), ≤ 0.75 C, 60° east
  of the world's meridian (+40° for each hub already there), lifted 25° north (grand hub: 25° south, 1.15x farther) off the
  ring's plane. Independent of the builder, so the work slot, the scaffold and the finished hub agree.
- **Elevator meridian**: an elevator already there; else 5° east of the colony (the tower never stands on it but rises over
  its horizon: 56 km on a 637 km moon, 175 km on a 2 000 km world); else the world's meridian. The root stands on the
  equator at sea level, its Z the local up; it carries FAPSInfrastructure's surface tag, so it settles on the ground (or the
  sea: a sea platform) like every surface structure.
- **Work slot** (`WorkSlot`, used by `FAPSFleetCommand::SlotLocation`): the construction ship holds by the scaffold, not at
  the world's generic slot: 10 km over the hub's disc (grand hub: 40 km over it), beside the elevator's tether at the fleet's
  slot height 20 km off the cable, just outside the ring at its junction, over a swarm ring's root.
- **Ring root**: where the elevator's tether crosses the ring (the map marks it there); the hoop is fitted round the body's
  centre in its equatorial plane (axis = the body's up).
- **Swarm / sphere**: around the star (`AStar::StarRadiusKM`, else 7e10 cm): radius max(0.05 AU, 40 star radii) x (1 + 0.06 k)
  for the k-th segment, each ring's axis tilted 26°·k and turned 37°·k (five rings make a shell); the sphere at 1.25 x the
  fifth ring's radius, six rings about one diameter.

`CheckRoom` refuses "No room for a ring / a stationary orbit / a hub's high orbit: a neighbouring world circles too close."

### 4.4 Construction stages (`FAPSMegastructureYard`, owned by `FAPSInfrastructure`)

Twice a second the yard reads the fleet's units: every construction ship in phase Working on a hub or megastructure has a
scaffold at the structure's own place (the same `PlaceRoot`, the builder's slot as its "near"), grown to `StageOf(progress, 96)`;
only a changed stage touches it (instances added, the spine re-fitted). It goes when the order ends (built or cancelled; a
pilot at the helm cancels). Scaffolds are transient engine shapes (no asset loads); meanwhile the finished look's meshes
stream in (`UAssetManager` async), so completion does not wait for the disk.

- Hub: a rim of 24 frame segments (0–70 %), then the spindle (30–95 %).
- Elevator: the tower frame rises (0–25 %), the tether grows up from the collar (25–90 %), the counterweight's frame closes
  (90–100 %). It settles on the ground like the finished one.
- Ring: 48 segments round the world, from the junction both ways, one per step.
- Swarm: 36 collectors on its ring; sphere: 6 x 36.

Saves: nothing new. A scaffold is rebuilt from the construction ship's saved order and progress.

### 4.5 The finished look (`APSMegastructures::SpawnStructure`)

An `ATechInfrastructure` root (ship navigation lists it as INFRASTRUCTURE, fleet MOVE orders take it), attached to its place,
tick off; runtime components only. Meshes load by path with `LoadObject`; a missing one is logged once and an engine shape of
its size stands in.

- **SPACE HUB**: `AI_Stations/01` at 10 000, centred on the root, its mesh Z (the spindle) toward the world.
- **GRAND HUB**: `Megastructure_P1_20` at 100 000, upright.
- **SPACE ELEVATOR**: tower `AI_Buildings/02` at 10 000 standing on the root; counterweight `Megastructure_P1_13` at 10 000
  standing on the stationary orbit; tether from the tower's collar (90 % of its height) to the counterweight, thickness
  clamp(0.0012 R, 200 m, 3 km): the authored spline elevator when its bounds are a long thin mesh (longest axis ≥ 4x the
  others), else an engine cylinder (logged); 15 lit beads along it and a bright one where the ring will cross.
- **ORBITAL RING**: `Megastructure_P1_23`, its thinnest bounds axis along the body's up, outer radius = the layout's ring
  radius, thickness as authored (0.1 of the plane scale); a lit junction at the root. Fallback: 96 bronze segments.
- **DYSON SWARM / SPHERE**: instanced engine cubes as gold collectors (visual placeholder: "the visual can come later").

Performance (Rio: no per-frame cost):
- No tick anywhere; scaffold changes only on a new stage.
- Draw distances (`SetCullDistance`, scaled by `aps.Mega.CullScale`): hub ≈ 460 of its radii (11.5 km hub: 2 650 km), grand hub
  15 000 km, tower 2 000 km, tether/counterweight 40 R, beads 12 R, ring 40 ring radii, collectors never culled (system landmark).
- Shadows off (`aps.Mega.Shadows 1`: hubs, tower and counterweight cast); no distance-field or dynamic indirect lighting,
  no decals, no navigation, no overlaps.
- Collision (`aps.Mega.Collision`): 0 none; **1 (default) the elevator's tower** blocks with its simple hull (matches the
  plinth at the ground); 2 the hubs and the counterweight too (as they block on the hand-made level). The ring, tether and
  collectors never block: a hull hundreds of km wide would be an invisible wall (the hub's single convex hull wraps disc and
  spindle alike); the fleet's autopilot ignores structures anyway.

### 4.6 Integration and UI

- `FAPSInfrastructure::Complete` takes the root from `PlaceRoot` for these types and drops the scaffold; `CompleteAt` (build
  mode) refuses `bFleetOnly` types (the caller refunds; the journal says to order it from the catalogue). Build mode never
  lists them anyway: its list keeps only Orbit/Surface types.
- `LocalWorkBonus` adds the hubs' `SystemWorkSpeed` for work at any planet, moon or star of the home system (construction
  and surveys use it already); `BerthsAt` / `HubBerthsFor` raise the per-place limit of ordinary orbital stations.
- Restore: `ApplyPendingRestore` → `SpawnVisual` → `SpawnStructure` at the saved root (ring and tether re-fitted to the body).
  Legacy saves with an OrbitalRing or SpaceElevator from the old look load: the ring is re-centred on its world, the elevator
  stands where it stood.
- INFRASTRUCTURE → CONSTRUCTION CATALOGUE: hub/megastructure badges; NEEDS lists the chain needs; new row **CHAIN**: "1. AN
  ORBITAL STATION ..., 2. SPACE ELEVATOR: STANDS (1), 3. ORBITAL RING: READY TO BUILD, 4. DYSON SWARM SEGMENT: LOCKED, REQUIRES
  ORBITAL RING IN THE STAR SYSTEM ..." (standing / under construction / ready / waiting: its needs stand but an unlock, a level
  or the stocks are missing / locked and why), the card's own step in bold. GIVES shows berths and system work speed. The
  site picker shows the runtime's refusal per place as before.
- Network map: hubs drawn as an octagon round a ring, large; legend row HUB. Object page glyph for hubs: headquarters.
- Console: `aps.Mega.List`, `aps.Mega.Raise <type> [world]` (tests: no needs, no cost), `aps.Mega.Preview <type> <0..1|off> [world]`.

### 4.7 Tests (`APS.Megastructures.*`, `Gameplay/Megastructures/APSMegastructuresTests.cpp`, no world)

Chains.Catalogue (ids, chain order, each step stands on the one before, fleet-only, never Orbit/Surface), Chains.Rules
(`ChainRefusal` for every step, hub berths), Geometry.Layout (Kepler ratios, ring/counterweight/hub under the clearance, no
room cases, stages), Geometry.Fits (axis rotations, ring fit centre/radius/plane/thickness, cable span, tower foot, hub
spindle, ring points, equator direction), SaveFormat (still the v2 blob, round trip). cl-checked, not run.

## 5. Hooks in the shared files (applied 03.10, narrow edits)

- `Gameplay/Fleet/APSFleetCommand.cpp`: `SlotLocation` holds a hub's or megastructure's construction ship at
  `APSMegastructures::WorkSlot`; `CountStructures(Body, Station)` counts catalogue hubs over the world (the foothold, the base
  for a shipyard or a HQ, the faster crew there), while the "two stations" limit in `CheckOrder` counts the fleet's own only.
- `UI/StrategicMap/APSStrategicMapScene.cpp`: hubs are `EKind::Station` objects (priority 3); the orbital ring sets its
  object's orbit (centre = its world, plane = the equator, radius = `LayoutAt`'s ring radius), so the orbits layer draws it as
  a circle round its world through its marker. The F10 agent's and the ancients' changes there are untouched.
- `Gameplay/Expansion/APSMissions.cpp`: THE RING PROJECT's brief says the ring hangs on a space elevator raised first; new
  templates TRANSPORT "A HUB FOR THE SYSTEM" (SpaceHub, chains to "THE GRAND HUB") and INDUSTRY "CLOSE THE SPHERE"
  (DysonSphere); the board offers a fleet-only chain step only once the step before it stands (a hub, an elevator, five swarm
  rings at one star).
- `Gameplay/Expansion/APSObjectActions.cpp` `Describe`: on a built hub/megastructure ORBIT and BERTHS, COUNTERWEIGHT (height,
  radii), RING RADIUS, FROM THE STAR, and CHAIN ("STEP 2 OF 5 / NEXT: ORBITAL RING (READY TO BUILD | the refusal | STANDS)");
  on a world with ground RING / STATIONARY ORBIT (its layout, or "no room").
- `Gameplay/Construction/APSConstructionMode.cpp`: nothing needed (AroundWorld/HighOrbit/StarSystem types are not listed);
  if hubs should ever be placed by hand, the ghost (`StructureClass`) needs the hub mesh.
- Not done: walking/landing on a hub (needs a gravity volume like the stations'), anything asset-side.

## 6. Asset-side ideas (need Rio's OK; nothing was saved)

Enable Nanite (or build LODs) on the AI meshes (17–43 MB each, probably one LOD); complex-as-simple collision on the hubs
if docking/landing is wanted; a proper hoop collision for the ring is not advisable at this size.

## 7. To verify with frames / play

- The look at full scale: the ring round a 2 000 km world (3 770 km radius, ~130 km thick), the hub's spindle down, the tower
  on the equator 5° east of the colony and its 6 km height, the tether (authored mesh or cable: see the log line
  `[APS.Mega] ... spawned at ...: tether N km (authored|cable)`), the counterweight, the beads and the junction lamp.
- Precision and flicker of a mesh scaled x10⁷ near the camera; the draw distances from orbit and from the ground.
- The scaffold growing while a construction ship works (`aps.Fleet.WorkScale` speeds it), and its removal on completion.
- FPS near the elevator and a hub (the AI meshes' triangle count), the async preload (no hitch at completion), the load
  hitch when a save with megastructures restores (sync `LoadObject`).
- The CHAIN row and the refusals in the catalogue and the site picker; build mode does not list these types.
- The UE query (`run_mega_query.ps1`) to confirm the spline elevator's shape and the meshes' render bounds.

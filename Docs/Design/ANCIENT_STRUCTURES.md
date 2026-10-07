# Ancient structures — the Builders

Status 2026-10-03: design plus a working vertical slice in `Source/APS_ALPHA/Gameplay/Ancients/` (plain C++, no new
UCLASS/USTRUCT/UENUM, no UHT). Compiled in APS DEV's UBT build of 03.10 00:32 (exit 0); not yet seen in PIE by Rio.
Two later source edits (the sunken cube's footprint covers its shards; a dropped step's re-offer backs off 2→32 min) are
cl-checked and wait for the next build. Owner: Claude (ancients session). Nothing committed.

Files: `APSAncientsTypes.h` (kinds, sizes, chains, site data), `APSAncientsSites.h/.cpp` (texts of the kinds, the
deterministic site rolls), `APSAncientsGeometry.h/.cpp` (procedural shapes), `APSAncientsQuests.h/.cpp` (chains, mission
board bridge), `APSAncients.h/.cpp` (the per-world runtime, world hooks, object actions, console), `APSAncientsTests.cpp`
(automation, `APS.Ancients.*`).

## Кратко (для Rio)

- Древняя раса — «Строители» (the Builders). Их постройки: **Игла** (наклонный чёрный обелиск 2,4–4,2 км), **Сломанное
  кольцо** (кольцо 3–5 км на ребре, наполовину в земле, кусок дуги упал), **Лес шпилей** (18–30 ступенчатых башен,
  центральная до 2,6 км), **Утонувший куб** (угол куба 0,7–1,3 км торчит из земли), **Круг камней** (малый, 30–50 м,
  можно войти пешком), **Тихий корпус** (сломанный корабль 2–4 км на высокой орбите).
- **Гарантии в стартовой системе:** монумент на родной планете (видно с орбиты, ночью светится «корона»), круг камней на
  спутнике (если у планеты нет спутника с поверхностью — на ней же), корпус на высокой орбите родной планеты. На каждом
  другом мире стартовой системы — шанс ~22 % на «потерянную работу».
- **Соседние системы:** у 16 ближайших шанс 45 / 28 / 16 % по дальности; среди трёх ближайших хотя бы одна — всегда.
  Это только данные, пока система не материализована; постройка появляется, когда пилот прилетает.
- Всё детерминировано от сида мира, имён тел и StableId систем. В сохранение **ничего нового не пишется**: прогресс цепочек
  живёт в обычных миссиях департаментов (DIVISIONS, трекер миссии на HUD), награды — обычные запасы, уровни и открытия.
- Цепочки: **ECHOES OF THE BUILDERS** (сигнал → облёт → наука → пешком у подножия → SPACE ELEVATOR), **THE QUIET HULL**
  (облёт → скан → абордаж → ORBITAL RING), **THE CIRCLE**, **LOST WORKS**, **THE BUILDERS' ROAD** (карта Строителей ведёт
  от системы к системе → JUMP GATE).
- Проверка в PIE: Civilization-старт → через ~20 с в журнале сигнал, миссия в трекере. `aps.Ancients.List`,
  `aps.Ancients.Teleport 0`, `aps.Ancients.Advance 0`. Выключатель: `aps.Ancients.Enable 0`.

## 1. Concept

Rio, 02.10: "a separate concept: scatter ancient structures left by an unknown race. Monumental ones visible from
orbit, and of different sizes. Tie quest chains to them. Spawn them with some probability, but guarantee some in the
starting system, and something interesting in the nearest systems. Quest lines."

The Builders left stone, not machines: dark, faceted masses far older than the colony, with one recognisable signature —
thin teal-white glyph seams and a crown that glows at night. Every site reads as the same hand, so a player who has seen
one recognises the next from orbit. The existing fleet anomaly "ANCIENT RUINS" and the deep-space "ALIEN BEACON" fit the
same lore without changes.

## 2. Structure types and sizes

| Kind | Size class | Dimensions (scale 1) | Silhouette | Where |
|---|---|---|---|---|
| Monolith — THE NEEDLE | Monumental | 2.4–4.2 km tall, 160–360 m wide, leaning 1.5–5° | obelisk on an octagonal dais, 45°-turned capstone, glowing crown band, pyramidion; glyph seams down both broad faces, an "eye" diamond near the top; 9–14 standing stones 90–180 m tall in a ring 0.7–1.7 km out (some fallen outward, some broken); a processional path of slabs with small lights | home planet, nearby systems, large variant on other worlds |
| Broken ring — THE BROKEN RING | Monumental | ring 3–5 km across, centre 42–60 % of its radius above the ground | 44 segments on edge, lowest arc buried; a break of 4–7 segments tumbled beneath the gap, two loose gaps; footings, glowing inner seams, a beacon on the highest segment, a hexagonal altar under the centre | home planet, nearby systems |
| Spire field — THE SPIRE FOREST | Monumental / Large | 2.2–3.4 km wide grid, central spire 1.8–2.6 km | 18–30 stepped towers (3–5 tiers narrowing upward, pyramid tips), taller toward the centre; glowing ledges on the taller ones, a beacon on the central tip | all |
| Sunken cube — THE SUNKEN CUBE | Large | edge 0.7–1.3 km (scale 0.6–1.0), 30–55 % of an edge exposed | black cube with its body diagonal tilted 8–30° off vertical, one corner breaking the surface; the three edges at that corner glow; 4–6 half-buried shards | other worlds, nearby systems |
| Stone circle — THE STONE CIRCLE | Small (walkable) | ring 28–48 m across, stones 4.4–7.2 m | 9–13 standing stones, one trilithon lintel, an octagonal plinth with a glowing glass disc, a leaning tablet, glyph marks on the inner faces | home moon (or home planet), chance sites, nearby systems |
| Derelict — THE QUIET HULL | Monumental (orbital) | 2.2–4.2 km long, hull radius 5.5–7.5 % of its length | front section and nose, a debris-filled break, the rear section knocked 3–7° askew with an engine bell and fins; three rings of segments on spokes, some missing; glowing slits, a faint glow in the break, a nose light | home planet's high orbit (1.9–2.6 radii), nearby systems (orbiting a planet) |

"Large" variants are the same shapes at 0.40–0.55 of their size (cube 0.6–1.0, circle 1.0–1.4).

Readability from orbit: a 3–4 km needle or a 4 km ring is the size of a large mountain on a full-scale planet; it shows
as a distinct silhouette with a long shadow near the terminator from low orbit, and the crown band burns at night (bloom).
The top of a 3.6 km needle clears the horizon of an Earth-size planet from about 200 km away.

## 3. Placement (deterministic)

Code: `APSAncientsSites.*`. Every roll is `CRC32(parts joined by '|')` (`APSAncients::Hash`) of stable keys only — the
world seed (`UGeneratedWorld::GenerationSeed` of the committed model, the same in a new game and a load), body keys
(`FAPSFleetCommand::KeyOf` = `BODY:<catalogue name>`), the cluster seed and star system StableIds. Never the clock, the
player or the global random stream.

### Start system (guaranteed)

| Id | Kind | Body | Chain |
|---|---|---|---|
| `H_MONUMENT` | Monolith, broken ring or spire field (from the seed) | home planet | Echoes |
| `H_CIRCLE` | Stone circle | a home moon with a WorldScape surface, dry ones first, picked by seed among them sorted by name; else the home planet | Circle |
| `H_HULL` | Derelict | home planet, high orbit (1.9–2.6 radii) | Quiet Hull |

The home specs are built once the home planet's catalogue name has held for 3 s (the name and the moons settle with the
generation).

### Chance sites in the start system

Every other world of the home star with a surface (planets and moons, `SupportsWorldScape`) is rolled once from its own
key: 22 % carry a site (`H_LOST_<NAME>`, chain Lost Works): sunken cube 32 %, spire field 26 %, smaller needle 22 %, stone
circle 20 %. The order worlds are seen in does not matter.

### Nearby systems

From `FAPSStarSystems` once its catalogue is read: the 16 nearest systems with planets (not the home, not inside the
home sphere), nearest first. Chance by rank: 45 % for the 3 nearest, 28 % for ranks 4–8, 16 % beyond. If none of the
three nearest rolled a site, the nearest gets one (the guarantee). Kind: needle 20, ring 18, spires 18, cube 18, circle 10,
derelict 16; monumental 60 % / large 40 % for the big kinds. Id `N_<system digits>`, chain Road.

A nearby site is data until its system stands materialized (`FAPSSystemMaterializer`, when the pilot arrives). Once the
materializer's planet count has held for 3 s, the site picks its world by seed among the eligible worlds sorted by name
(surface worlds for surface kinds, any planet for a derelict; a system without a surface world gets a derelict). When the
system is released the site actor goes with it and comes back the same next time.

### The place on a surface

When the body's WorldScape root carries this body's current surface (`IsSurfaceProfileCurrent`), up to 48 seeded
candidate directions (latitude within ±55°) are tried in order, 6 per tick. A candidate fits when the ground at the centre
and at 8 points on the footprint radius is all above the sea (+20 m, +3 m for small sites) and the spread over the
footprint stays under the kind's slope limit (needle 0.22, ring 0.18, spires 0.25, cube 0.35, circle 0.20). The first
that fits wins; if none does, the least steep and least wet one. The heights come from `AWorldScapeRoot::GetGroundHeight`,
the function the terrain itself is built from, so the result is the same every session.

### In the ground, not on it

Every part is seated on the WorldScape height under its own footprint: the needle's shaft goes `max(4 % of its height,
80 m)` below the lowest ground under it; stones, slabs and spires use the lowest ground under each one; fallen stones and
ring segments lie half in the ground; the dais and the altar reach below the lowest point of their rim and above the
highest. Far terrain LODs differ from the noise by less than these bury depths at monument scale.

### Riding the body

The site actor is attached to its planet or moon (`KeepWorldTransform`), so the body's turn, the engine's world-origin
rebase and the floating origin carry it along. Its root is a navigation point above the site (structure height + 1.2–1.8
km): fleet MOVE orders park 1.5 km round that point, which keeps the slots above the ground, the cube's corner and the
hull from any side.

## 4. Visuals and performance

Code: `APSAncientsGeometry.*` (shapes), `FAPSAncients::SpawnSiteActor`.

- One `ATechInfrastructure` actor per site (an existing class: ship navigation lists it, fleet MOVE orders take it,
  `InGameName` names it), tick off, tags `APS.Ancient.Site`, `APS.Ancient.Site.<id>` and `APS.Fleet.Anomaly` (the colony
  map lists it as an anomaly site). Actor name `APS_Ancient_<id>` (stable: fleet orders aimed at it find it after a load).
  `RF_Transient`; `AWorldActor`s are not in the actor archive, so nothing of it is saved.
- Geometry: flat-shaded boxes and faceted frustums generated in the site's frame, gathered into **one procedural stone
  mesh** (`BasicShapeMaterial`, a per-kind colour: obsidian needle, bone-white ring, bronze spires, black cube, weathered
  circle, dark hull) and **one glow mesh with two sections** (`EmissiveMeshMaterial`: seams ×8, crowns ×60, teal-white;
  `aps.Ancients.Glow` scales them). Winding follows the engine's own box (`UKismetProceduralMeshLibrary::GenerateBoxMesh`).
  Procedural meshes need no material usage flag (the engine's emissive material is not flagged for instancing).
- Budget per site: 2 components, **3 draw calls** (stone + 2 glow sections) plus the stone's shadow depth passes;
  0.3–6 k triangles; **no lights**; one trimesh collision body (complex as simple, `BlockAll`, cooked asynchronously);
  no tick. The glow casts no shadows and does not collide.
- Culling: monuments and large sites draw out to 5 body radii (8 for orbital hulls), small sites to 20 km.
- Shadows: the stone casts dynamic and far-cascade shadows (`aps.Ancients.Shadows 0` turns them off for rebuilt sites).
- CPU: the runtime's logic runs 4 times a second (`TRACE_CPUPROFILER_EVENT_SCOPE(APSAncients_Tick)`); at most one heavy
  piece per tick — a batch of 6 candidates (≤ 54 height samples) or one build (≤ ~100 samples, mesh generation, one
  spawn); a few such ticks per site per session. Steady state: distance checks and a fleet-unit scan per site.
- At 120 FPS the steady cost is a few microseconds a frame plus 3 draw calls per visible site.

## 5. Discovery

- **Seen:** monuments are visible from orbit; every site actor is on the colony map (MAP tab, "ANOMALY SITE") once it is
  built; SET COURSE / AUTOPILOT from there pins it in the ship's navigation (infrastructure contacts are hidden in the HUD
  list by default — a pinned course always shows).
- **Named:** while the chains run, a site is "UNKNOWN STRUCTURE" until its first survey (a nearby one until it is found,
  the circle as its chain starts); then "ANCIENT SITE: THE NEEDLE". Without a civilization (no chains) it is named at once.
- **Detected:** the monument's signal comes with the game (20 s after the quests open); the hull's after the monument's
  survey or when the pilot passes within 1 500 km; the circle's after the monument's study, a fleet survey of its body or
  the pilot within 50 km; a lost work's on a fleet survey of its body or the pilot within 200 km; a nearby site's from the
  Builders' chart (the previous chain) or the pilot within 100 km.
- **Surveyed / studied:** in person (fly close, stay near, walk to it), by fleet ships holding at the site (any division
  for a survey, science for a study, exploration for an expedition), or — for worlds other than the home planet, which is
  known from the start — by the fleet's ordinary SURVEY and STUDY of that world.
- **Object page:** OPEN on the colony map shows the site's page: the Builders' description, SET COURSE, AUTOPILOT, SEND THE
  MAIN FLEET, and the ancients' own STUDY IT (SCIENCE SHIP) and SEND AN EXPEDITION (EXPLORATION SHIP), via the existing
  `APSObjectActions` provider registry.

## 6. Quest chains

Code: `APSAncientsQuests.*` (texts, rules), `FAPSAncients::UpdateChain`. Each step is a department mission on the existing
board (`FAPSMissionBoard`): DIVISIONS lists it, the HUD tracker shows it, the board pays its reward and posts "completed".
Template `Ancients.<Chain>.<site>.<step>`, subject `ANCIENT:<site>:<step>` (only this runtime notifies it). Objectives are
display-only and limited to ones no count-of-any or subjectless board template listens to: LOCATE AN ANOMALY, STUDY A
WORLD, INVESTIGATE AN ANOMALY, SURVEY A STAR SYSTEM. A step completes when any of its conditions holds; the runtime then
notifies the board, which completes and pays it; the runtime posts the step's story and puts the next step on the board
(active, tracked when nothing else is). A step the player drops comes back as an offer after 2 minutes.

### ECHOES OF THE BUILDERS — Science (the home monument) — *implemented end to end*

Signal: "SIGNAL — {home}: the colony's deep radar draws {size} of straight edges at {lat, lon}, too regular for rock.
The colony map marks it as an anomaly site (UNKNOWN STRUCTURE): set a course there."

| # | Title | Completes when | Reward | Journal on completion |
|---|---|---|---|---|
| 1 | A SHAPE ON THE HORIZON | pilot within 25 km, or any fleet ship holding at the site | 20 research | "{site} on {home}: {description} No record of ours mentions it. Its faces carry glyphs." |
| 2 | READ THE STONE | a science ship holding there 30 s, or the pilot within 3 km for 20 s | 60 research | "The glyphs … count in sevens. Beside a map of this system one sign repeats: the Builders knew our sun." |
| 3 | AT THE FOOT OF IT | the pilot on foot at its foot (dais/altar + 250 m), or an exploration ship holding there 60 s | 80 research, 40 influence, +1 Science level, unlocks SPACE ELEVATOR | the kind's story + "What the Builders knew of lifting stone to orbit is ours now." |

Completing it starts THE BUILDERS' ROAD for the nearest nearby site.

### THE QUIET HULL — Industry (the home derelict)

Starts after Echoes step 1, or when the pilot passes within 1 500 km.

| # | Title | Completes when | Reward |
|---|---|---|---|
| 1 | A COLD RETURN | pilot within 50 km, or any fleet ship holding at it | 40 metals |
| 2 | SCAN THE HULL | a science ship holding 30 s, or the pilot within 5 km for 20 s | 40 research |
| 3 | BOARD IT | the pilot's ship within 3 km for 10 s, or an exploration ship holding 60 s | 200 metals, 80 volatiles, 40 energy, +1 Industry level, unlocks ORBITAL RING |

### THE CIRCLE — Exploration (the small site)

Starts after Echoes step 2, a fleet survey of its moon, or the pilot within 50 km. One step, STONES IN A RING: walk
into the circle, an expedition, or a science study of the moon — 40 research, 20 influence. Story: the plinth's glass disc
shows this system with one world more than anyone can find (a hook for later content).

### LOST WORKS — Exploration (chance sites of the start system)

| # | Title | Completes when | Reward |
|---|---|---|---|
| 1 | LOST WORKS | pilot within 25 km, a fleet ship holding there, or a fleet survey of the world | 20 research |
| 2 | WHAT STANDS THERE | on foot, an expedition, a science ship holding 30 s, or a science study of the world | 50 research, 50 metals |

### THE BUILDERS' ROAD — Exploration (nearby systems, one after another)

The first Road chain starts when Echoes completes ("THE BUILDERS' CHART — one of their glyphs circles {system}"): the
system is learned as SCANNED (`FAPSStarSystems::Learn`) and its beacon anchor appears on the maps and in navigation. Each
next chain starts when the previous Road chain completes; any Road site also starts when the pilot comes within 100 km.

| # | Title | Completes when | Reward |
|---|---|---|---|
| 1 | THE STAR ON THE CHART | the system is SURVEYED (a probe + system survey, or a visit) | 30 research, 10 influence |
| 2 | THE NEXT SITE | pilot within 25 km, a fleet ship holding there, or a fleet survey of its world | 40 research |
| 3 | WHAT THEY LEFT | on foot (dock for a hull), an expedition, a science ship holding, or a study of its world | 120 research, 40 influence, +1 Exploration level, unlocks JUMP GATE; names the next charted system |

## 7. Integration points (existing systems, unchanged)

| System | How the ancients use it |
|---|---|
| World lifetime | `FDelayedAutoRegisterHelper` (EndOfEngineInit) registers `FWorldDelegates::OnPostWorldInitialization` / `OnWorldCleanup`; one `FAPSAncients` (an `FTickableGameObject` bound to its world, pauses with the game) per Game/PIE world. No subsystem or module file changed. |
| Generator | the non-preview `AAstroGenerator` with a `HomePlanet`; nothing on the authored single-play map (`bUseAuthoredSinglePlayWorld`) |
| WorldScape | `APlanetaryBody → PlanetaryEnvironmentGenerator → WorldScapeRootInstance → GetGroundHeight`; `ResolvedSurfaceProfile` for the sea |
| Mission board | `CaptureSave` + `RestoreSave` to add/edit missions (it has no add of its own), `Notify`, `GetTracked`/`SetTracked`, `GetRevision` |
| Fleet | `APSFleetFind`: units (MOVE + HOLDING at the site actor, division), `GetSurvey(body)`, `CheckOrder`/`IssueOrder` for the page's orders |
| Star systems | `APSStarSystemsFind`: catalogue, nearest, knowledge, `Learn`, `GetAnchor`; `GetMaterializer()->GetActiveIndex()/GetPlanets()` |
| Journal | `UAPSCivilizationJournalSubsystem::Post(World, "Ancients", text)` |
| Object pages | `APSObjectActions::RegisterProvider("Ancients", …)` |
| Colony map | the `APS.Fleet.Anomaly` tag |
| Navigation | `ATechActor` contacts; `InGameName` |
| Infrastructure catalogue | rewards unlock `SpaceElevator`, `OrbitalRing`, `JumpGate` (types that otherwise need a mission) |

## 8. Saves

- **Nothing new is written and no format changes.** Sites, places and shapes are derived again from the seeds every
  session; site actors are `AWorldActor`s, which the actor archive skips, and `RF_Transient`.
- Chain progress lives in the mission board's existing save (civilization save, version 5 expansion block): our missions
  (template, subject, state, progress, texts) and the board's `CompletedTemplates`, which keeps completed templates even
  after the board prunes old missions. Rewards, levels and unlocks are paid into the existing stocks/levels/unlocks;
  journal lines are saved with the journal; charted systems with the star systems' state.
- The board fills mission rewards from its own templates on load, so ours come back without them: the runtime puts them
  back (`HydrateRewards`, and again just before notifying a step).
- The chains wait for a load to be applied (`UMainGameplayInstance::bIsLoadingMode`), so the first look at the board is
  the saved one: steps done before are not announced again, an active step continues, a step between two others is put
  back on the board.
- Fleet ships holding at a site are saved by the fleet with the site actor's key (`ACTOR:APS_Ancient_<id>`); a load finds
  them again if the site actor exists by then (it is built as soon as its world's surface is up).
- Session-only: hold/linger timers, the 20 s signal delay, the re-offer timer.

## 9. Testing in PIE

1. Start a Civilization game (any start). Within a few seconds the log shows `[APS.Ancients] start system of …` and the
   `site …` lines; `built H_MONUMENT …` once the home surface is up, `built H_HULL …` at once.
2. 20 s after the quests open: journal "SIGNAL — …", HUD tracker "A SHAPE ON THE HORIZON". Colony terminal → MAP → the
   anomaly site → SET COURSE or OPEN (actions: study, expedition, main fleet).
3. Fly within 25 km → step 2. Hold within 3 km for 20 s (or send a science ship) → step 3. Land and walk to the foot → done:
   +1 Science level, SPACE ELEVATOR unlocked, THE BUILDERS' ROAD starts.
4. Console: `aps.Ancients.List`; `aps.Ancients.Teleport <index|id>` (in a ship: above the site; on foot: beside it; not
   built yet: to its world or its system's edge first); `aps.Ancients.Advance <index|id>` (starts a chain / completes the
   active step through the normal path); `aps.Ancients.StartAll`; `aps.Ancients.Rebuild`. CVars:
   `aps.Ancients.Enable`, `.Quests`, `.Shadows`, `.Glow`, `.SignalDelay`.
5. Save and load mid-chain: the step continues, rewards show in the tracker, nothing is announced twice.
6. Automation: `APS.Ancients.Geometry.Shapes`, `APS.Ancients.Sites.Placement`, `APS.Ancients.Quests.Chains`,
   `APS.Ancients.Quests.Board` (no world needed).

## 10. Limits and next steps

- Not seen in a rendered frame yet: proportions, colours, glow strength and readability from orbit need Rio's eye (the
  CVars and the per-kind numbers in `APSAncientsGeometry.cpp` are the knobs).
- UI (APS DEV's files) — suggested patches in the handoff: an ANCIENT SITE kind on object pages, the sites on the F10
  strategic map, an "ancient" navigation contact type shown by default.
- The mission board is reached through its save path; a small `FAPSMissionBoard::AddExternal()` / `Patch()` API would be
  cleaner (handoff). The formal quest runtime (`UAPSQuestSubsystem`) was not used: its single HUD prompt would compete with
  the onboarding (priority ties break by quest id, and "APS.Ancients" sorts before "APS.Onboarding"), and its event
  streams need saved sequence cursors.
- `FAPSSystemMaterializer` has no "all planets stand" accessor; the site waits for the planet count to hold 3 s.
- Later: authored meshes for the shapes (the procedural ones stay as the cheap far LOD), interiors for the cube and the hull,
  more chain branches (the planetarium's extra world), Builders' tech as research projects.

# Что известно о качестве 3D-ассетов (по документам и словам Rio, на 07.10.2026)

Собрано агентом по чекпойнтам, ledger'ам, отчётам Codex и памяти сессий. Текст — на английском, как пришёл;
цитаты Rio — дословно. Парная опись по папкам: [2026-10-07-asset-inventory.md](2026-10-07-asset-inventory.md).
Префиксы: R = репозиторий, CP = `R\Docs\Checkpoints\`, PEW = `R\Docs\coordination\PLANET_EDITOR_WINDOW.md`,
A = `F:\ChatGPT\APOSFERA\work\`, CX = `C:\Users\Rio\Documents\ChatGPT\APOSFERA\work\` (рабочая папка Codex по
кораблям), MEM = память Claude. [inf] — вывод агента.

## 0. Rio's overall verdict (07.10)
- «очень немного нормальных 3D-ассетов хороших. Есть… много уже заготовок… гораздо лучше, чем было… корабли, и
  станции. Но… станции сейчас хромают»; «по коду… всё нормально… проблема в арте»; «планеты… оставляют желать
  лучшего… надо контента… визуального… в космос».

## 1. Ships

**Ownership.** Claude "3D корабль в Blender": cargo `S_P3_01` v1/v2 (01–03.10), HQ Alpha, Colony HQ. Codex ("Codex
M5", later "Codex ships"): M01–M08, NextS (S03/S16/S24), L08/11/15/17, the later Cargo hull finish (Cargo13/19),
phase‑1 collision, lighting, wayfinding, airlocks. Claude flight: runtime collision (proxy, `UAPSShipHullComponent`).

**Cargo `S_P3_01`.** 01.10 Rio (`CP\2026-10-03-rio-requests-3-days.md:189-196`): cleanup «просто супер», interior
«очень круто», props and geometry «очень круто». Close-up hull textures went «побитые» → «ещё хуже», fixed with a
vector material (`MEM\feedback_closeup_texture_lessons.md`). Default ship since 02.10 21:32. 02.10 v2 complaints
(`CP\2026-10-02-cargo-ship-cockpit-redo.md:3-6`): gaps between glass and frames; "ugly" wall tile, wanted an
instrument panel and a centred chair; «проёмы в дверях багуются, аж глаза режут»; «заборчик… чтобы не наебнуться»;
stairs flush against walls; «у корпуса снаружи нет коллизии». v2 fixed (`F:\Rio\3D\ShipCleanup\cargo01\out_v2`: 208
UCX, 60k-tri `SM_CargoShip01_ShellCol`, 12 `SM_CargoCrate_*`), imported 03.10 03:25, walk test 21/23. 06.10 Rio
accepted "clean manufactured hull and cellular radiator grilles… do not redesign"; asked to fit 10 grilles, fix a
window corner, a pillar foot and the pod cassette. Cargo19 installed 07.10 ~10:25. The 29-light package was
deferred 07.10 20:55 (assertion); the old 35 lights remain. Collision 208 convex = "the template for a good
interior ship" (`CP\2026-10-06-ship-collision-audit.md:51`).

**M5 IntegratedV20 (M_P2_01/02/03/04/06, Codex).** 03.10 Rio «явно поручил» hulls, PBR, walkable interiors and
collision (PEW:4353); first install 03.10 10:00. 04.10 04:03 Rio: "finish **rejected** M interiors, fitted sealed
glazing, original canopy bridge, spacecraft stairs, less repetitive accepted panels, all displays in walls, routes to
every exit"; 04.10 04:45 "accepted functional v2 / **user-rejected visuals**" (PEW:5050, 5071). 05.10 04:34 Rio asked
for an immediate install for playtesting; SurfaceCleanupV21 followed (M01 Main 1.52M polys). M_P2_03 «твой корабль»;
M_P2_02 the bench ship. 06.10 regression «встал с кресла на скорости — провалился» fixed (walk shell = outer hull,
decks = root walk UCX). Collision before phase 1: M06 22,870 · M03 15,624 · M04 15,512 · M01 15,203 · M02 13,501
(93–95 % PhysicsOnly skin). Phase 1 (07.10 02:57): walk convex M02 969 / M03 983 / M04 938 / M06 2264 / M07 925 /
M08 891 / S24 2679; 78,297 outer forms retired; "walking UNCONFIRMED". Lighting 07.10: M01 62 Rect, M02 20, M03 21,
M04 22. Open (`CX\ships_current_handoff_20261006.json`): ceiling overexposure, M02 DeckC ceiling, M03 old navigation
and dark headers, no fixtures on M06/M07/M08/S24. Exterior airlocks: Blender candidates only.

**NextS (S03, S16, S24), M07/M08.** Installed 05.10 ~11:34. 06.10 Rio: S03 transparent bow glazing, open
double-height bow, smooth canopy frame; S16 glaze the opaque cockpit side zones. S24 "explicitly accepted
2026-10-06; preserve cockpit and glass". S03/S16 saved 07.10. Problems: 1.5M-tri unwelded query shells on
M07/M08/S24; M07 no Nanite; S24 BridgeSpine-659 fails the 42 cm capsule.

**Pack_1 L.** L07 removed at Rio's request. L08: Rio accepted "M01 exterior material and expanded observation halls"
+ seven fixes. L15: "full roof glass". L08/L11/L15/L17 installed 06.10; known: "L08 bridge light balance visibly
imperfect", "dark overhead bridge". 07.10 19:28: «Rio… просит как можно скорее лично посмотреть… L08/L11/L15/L17».

**Not refitted.** XL_P1_14 (52,066-convex candidate, frozen); S_P1_01/02/06 old candidates; M_P1_05 baseline;
XXS_P1_05/06/09/18/22 no interiors (use "near the ship" interact).

**Audit 27.09 baseline** (`Docs\Audit\2026-09-27-claude-full-audit.md:297-299`): 34 AI hulls 0.3–1.5M tris, Nanite
off, one LOD, one glTF material, no emissive; collision 16–80 MB per ship (~1.6 GB). Hand-made M3 = "hero ship".

**End of the ship workstream.** Rio 07.10 19:36: «Всё, давай там в целом как есть, заливай… Я потом всё уже
проверю… Так бесконечно их… доделывать будем». Codex delivered "FINAL AS-IS" 20:55. Deferred: M03/M04/M06/M07/M08/S24
wayfinding, fixtures, M02 ceiling. **Performance:** Rio's 06.10 rule — detailed collision at rest, primitive proxies
whenever the ship moves; 07.10 fleet proxies on by default (104 → 108 fps), uncommitted.

## 2. Stations
- **No concrete per-station complaint from Rio is documented** beyond «хромают» / «слабые».
- Audit 27.09 (:300-302): kitbash from 6 packs (KitBash3D, ModSci, SpaceColonies, SpaceshipInterior, archviz)
  "without a common material language"; base and pad = BasicShapes placeholders; stations go "black" without camera
  fill light; Lumen leaks in interiors. Station textures 40–70 MB each; `APS_PREA/TO_ALPHA` stations outside Git.
- R2 default since 02.10 21:32; nothing evaluates its look. Shipyard: Rio saw a launch over the yard 01.10 (✅).
  Station gravity after exiting a ship: «не исправил» twice (02–03.10); third fix built 03.10 02:09, never verified.

**HQ Alpha** (`CP\2026-10-02-hq-alpha-rework.md`, `2026-10-03-hq-alpha-passages.md`). Rio 02.10: rework «как
художник»; «во все проходы человек 2 метра ростом». Delivered: 13 ramps, 21 `MI_HQ_*`, hall glow fix
(`IMM_STATION_LIGHT` ×100 lilac → `MI_HQ_StationLight` ×4, frame p99 69 → 3.8). Complaints: 03.10 hub hatch
fragments, exit «в стене», invisible wall, glass must look like glass; 04.10 «вырезано окно», «комната дырявая,
кривая, не цельная»; 05.10 «комната квадратная, спереди панорама, по бокам шлюзы»; «дырки/щели… запинается у
круглого входа». v12 05.10 06:28, capsule 192/200 cm pass; no in-game verdict from Rio. Open: glass orrery 1.83–1.99
m above floor; hub trench; dais clearance 1.98–2.05. Orrery lift needs a ~150 MB hall-mesh duplicate (LFS). **Alpha
references git-ignored `/Game/APS_PREA/…/STATION_STR/SM_MERGED_vert_649xx`.** `SM_MERGED_Pads_Large` throws 40
VerifyImport warnings. HQ_Alpha meshes/MIs are tracked (10–30 MB each).

## 3. Colony HQ (`CP\2026-10-03-colony-hq.md`)
Rio called the stub «абстрактная залупа», wanted «реалистично, в минимализме… можно зайти… не прототип». v1 03.10
«прикольно», «доделай и собирай»; v2 «добавь текстуры… черно-белые… детализацию… стулья… панели» (imported 03.10
03:35); 06.10 «как школа», then «текстуры примитивные, сделай как Codex для кораблей; шкафчики заменить» → v3b;
«пол и стены сделай спокойнее и голостол почини» → v3c 19:14 with its own holotable. Uncommitted; 15 `T_HQ_Sci*`
untracked. No verdict on v3c yet.

## 4. Colony modules, infrastructure, megastructures, Ancients
- 30.09 Rio: «маленькие здания классные»; base and pad were «тестовые цилиндры»; pad on stilts. Phase-2 modular
  walkable kit never built [inf].
- Infrastructure reuses station BPs: `EVisual` "the generator's existing actor families until each type has its own
  model"; ~30 types on 5 families; Beacon scaled 0.6, **JumpGate = HQ BP ×3** (`APSInfrastructureCatalog.h:70`,
  `.cpp:231-239`, `APSInfrastructure.cpp:543-565`). Nobody has looked yet.
- Megastructures: AI Pack_1 meshes, "17–46 MB each, Nanite and LOD unknown offline"; «ни разу не поднимались в игре».
- Ancients: procedural flat-shaded boxes with `BasicShapeMaterial` + emissive mesh, 0.3–6k tris, no lights; "not
  seen in a rendered frame yet"; authored meshes "later".

## 5. Ground vehicles
Rio 02.10: «ровер круто ездит», «ховеры очень крутые», «колония играется». Rover wheels hung in the air («ничего не
изменилось» 03.10); `SKM_Offroad` suspension fix never checked by eye. Meshes: rover `/Game/Vehicles/OffroadCar`
(outside `Content/APS`), **hover = `SM_Spaceship_P1_06`**, **drone = `SM_Spaceship_P1_05`**, fallback cube.
`MI_OffroadCar_Lights` invalid for Nanite (153 warnings). Garage, slab, drone pad added 03.10.

## 6. Character / pilot
Audit 27.09 (:292-296): AstroGirl on the UE5 mannequin skeleton, 45k tris, 1 LOD; ~6 ground + ~20 zero-G clips, no
montages, no start/stop/turn; no boarding/seat animation (pilot hidden); **no first-person or cockpit view**.
`AnimBP_AstroGirl`, `/Bridge/MSPresets`, `XRBase` missing in Start Single Game. RANGER/STANDARD «названия ок».
SpeedModes pilot untested by Rio. Codex 07.10: the camera inside L17/S24 cockpits is poor.

## 7. Planet surface
27.09 accepted (`CP\2026-09-27-worldscape-accepted.md`): planets, lava, atmospheres, stars, transitions; tiling
much less visible; ~120 fps. Open: polygonal coasts on Water and Ammonia (`rio-requirements:315`); ~01.10 «на берегу
остаются полосы»; clouds: 02.10 Rio refused third-party clouds, 06.10 OK'd V33; flat look and stripes on TEVYS
(`CP\2026-10-06-terrain-flat-stripes-research.md`), waiting for Rio; close-up judged at 0.5–1 m; 07.10 «планеты…
оставляют желать лучшего».

## 8. Space
GPU stars 03.10; canon «эти сраные пятна вообще убрать… всё чётко, ничего не моргает». Nebulae none
(`NebulaSheetCount = 0`). Black hole V2 exists; Rio 02.10 «Чёрная дыра некрасивая»; never seen in a frame. No
supermassive black hole. Rio 04.10: objects so space is not «стерильным». Belts, rings, derelicts, comets do not exist.

## 9. Audio
27.09: 17 SoundWaves, no music. Codex added music, footsteps, ship sounds (`Docs\Audio\README.md`, LFS rules for
WAVs). "В игре не проверен".

## 10. Pipeline and Git state
Blender through BlenderMCP in the live window (Rio watches), headless verification; UE headless
`UnrealEditor-Cmd -run=pythonscript -NullRHI` + `-RenderOffscreen` for walks/frames; saving any asset needs Rio's
explicit OK; one heavy UE process at a time (PEW). LFS: per-file rules only; `SK_Spaceship_S_P1_24` 245 MB has one;
M5 meshes (63–68 MB), M07/M08 (81–85 MB), Cargo (52 MB) modified with no rule; GitHub limit 100 MB. 07.10 git: 155
modified, 84 untracked (ClassL, wayfinding/luminaire/airlock materials, Cargo thermal/coaming, `AI_Shpis/Common`,
ColonyHQ Sci textures). Nothing committed since 06.10. Blender folders `F:\Rio\3D`: Ships-GLB (44 GLB, Aug–Sep
2025), ShipCleanup\cargo01, HQRework (451 previews), Colony\HQ (v1–v3), CodexFleetWorkshop (03–07.10, JSON only).

## 11. Rio's art-direction rules (quoted)
- UI colour: «цветастые вообще не наш вариант… никаких лиловых с жёлтыми, нужны строгие, стильные»; teal «много
  такого видел» (06.10).
- Keep old versions: «старый не удаляй… как я сверять… буду» (06.10).
- Stars crisp, one material, nothing blinks or unloads, «чтобы видимой просадки не было. И фриза» (03.10).
- Clearance: «во все проходы человек 2 метра ростом проходил» (02.10).
- Close-ups judged in Blender Solid+Texture at 0.5–1 m; image sharpening rejected.
- Approaching stars like planets: the point grows, the real mesh appears in the same place (06.10).
- Scale: «реализм, но без пустых точек»; «Масштабы проебаны» (03.10).
- Regressions: «главное — отсутствие регрессий»; commit only after Rio checks.
- Colony HQ style went from «минимализм» to sci-fi after «как школа»; the ship-hull style (white enamel panels,
  orange accents) is the reference. No explicit "realistic + retro sci-fi" statement found.

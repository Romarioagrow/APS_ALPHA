# Опись 3D-ассетов проекта (07.10.2026)

Собрано агентом по папкам `Content` (пути, размеры, git-статус, кто на что ссылается), без запуска Unreal.
Используется в [контент-плане](../Design/CONTENT_PLAN_ORIGINS_2026-10.md) и
[брифах на ассеты](../Design/ASSET_BRIEFS_ORIGINS_2026-10.md). Текст отчёта — на английском, как пришёл.
Сокращения: **A\\** = `Content\APS\APS_ALPHA\Assets\`, **C\\** = `Content\APS\APS_ALPHA\Core\`,
**BPdir** = `Content\APS\APS_ALPHA\Blueprints\`. «Inferred» — вывод агента по именам/байт-скану, не проверенный
в редакторе. Парная опись по документам: [2026-10-07-asset-quality-notes.md](2026-10-07-asset-quality-notes.md).

## 0. Totals and git

- **Content total:** 16,587 files, 25.61 GiB. `Content\APS`: 2,234 files, 4,972 MB. Everything else (about
  20.7 GiB) is git-ignored.
- **Ignore rule:** `.gitignore:390 /Content/*`, with exceptions `!/Content/APS/` and `!/Content/AudioCredits/`.
  `/Plugins/*` is also ignored, except `APSStarRenderer` and `AtmoScape`. `APS_PREA/.../Meshes/STATION_STR/*` and
  `SM_MERGED_StaticMeshActor_337` are listed explicitly as well.
- **LFS:** 17 files, including `SK_Spaceship_S_P1_24`, `SM_Spaceship_S_P3_01`, three music waves and AtmoScape
  `GPU_ClipMapMesh`.
- **HEAD:** `61532ed6d` (2026-10-06). Working tree in `Content/APS`: 72 modified files (ship meshes, ship BPs,
  ColonyHQ, DA_ShipCatalog, thumbnails); 71 untracked (`AI_Shpis/Common` 3, `M5Workshop` 44 — ClassL 32, `Pack_3`
  8 materials, `ColonyHQ/Textures` 15, `WSC/.../CloudWeather20261006V33` 1).

**Ignored top-level Content folders (65):** 4K_WoodFlooring, AbstractFX, APS_ContentReview, APS_ContentReview_Full,
APS_PlanetCompare, APS_PREA, AstroGirl, BigCompanyArchViz, Blueprints, Brushify, Characters, CityofBrass_Enemies,
Collections, Developers, DriveableMarsRover, EnergyFieldsSFX, Examples, FlightLocomotion, Foliages, FootstepsMiniPack,
FreeFurniturePack, HugeAnimBundle, Interface_And_Item_Sounds, LevelPrototyping, LevitationFX, Mannequin,
MaterialFunctionCollection, MaterialHelpers, Materials, MBLS, Megascans, MGT, Migrated, missiontominerva,
MixOfAmbientMusic, ModSciInteriors, ModSci_Engineer, ModularSciFiOffice, MSPresets, P3_ComputerStation, Polar,
ProceduralBuildingGenerator, Props, Ressources, Restaurant, SciFiFlying, SciFiGameSounds, Sci_Fi_Character_08,
Showcase, Slate, SolarSystem, SpaceAmbBundle, SpaceColonies, SpaceCreatures, SpaceshipInterior, StarterBundle,
StarterContent, StorageHouse, Textures, ThirdPerson, TwinmotionMaterials, Underground, Vehicles, VehicleTemplate,
`__ExternalActors__`/`__ExternalObjects__`.

**15 biggest top-level folders (MB):** SpaceAmbBundle 6,096 · APS 4,972 · Showcase 1,144 · missiontominerva 1,004 ·
APS_PREA 993 · SolarSystem 946 · Ressources 935 · HugeAnimBundle 927 · SpaceColonies 818 · BigCompanyArchViz 724 ·
StarterContent 612 · TwinmotionMaterials 507 · MixOfAmbientMusic 439 · StarterBundle 437 · SpaceshipInterior 404.

**Inside `Content\APS\APS_ALPHA` (MB):** Assets 4,291 (1,079 files, 1,009 tracked) · Audio 341 · UI 42 · WSC 46 ·
Anims 18 · Blueprints 12 · Levels 11 · Core 4.4.

**Gravity:** no BP contains "GravityVolume". Gravity is native: stations `ASpaceStation::GravityCollisionZone`
(USphereComponent, `Actors\Tech\SpaceStation.h:40`; HQ radius multiplier 3.0), ships — a gravity sphere in
`Spaceship.cpp`. Every ship and station BP gets gravity from its C++ parent.

## 1. Ships

`A\AI_Shpis` is 581 files and 3,174 MB: M5Workshop 1,153 · Pack_1 1,092 · Pack_2 750 · Pack_3 176. BPs live in
`C\Spaceships\<class>\`. `C\Spaceships\DA_ShipCatalog` (modified 2026-10-06) has 25 entries: L 08/11/15/17; M P1_05,
P2_01/02/03/04/06/07/08; S P1_01/02/03/06/16/24, P3_01; XL P1_14; XXS 05/06/09/18/22.

| Ship | Class | Folders (MB) | Meshes used by the BP | Interior | Git / last modified |
|---|---|---|---|---|---|
| **S_P3_01 "CargoShip01"** | S | `Pack_3\Spaceship_S_P3_01` (94 files, 176.1; textures 124) | `SM_Spaceship_S_P3_01` (49.4, LFS), `_Glass`, `_Ramp`, `_ShellCol` (2.0); `Props\SM_..._Props_DeckA/B/C/Engine/Hold` | **Yes.** Bridge with centred pilot seat, dashboards, displays; decks A/B/C; hold gallery with rails; wide stairs; airlock; engine room. 208 hull convexes | 86/94 tracked; 8 new `M_Cargo_*` untracked; 2026-10-07. Default start ship (`SAPSMainMenuRoot.cpp:4507`) |
| **M_P2_01/02/03/04/06 (M5 "IntegratedV20")** | M | `M5Workshop\IntegratedV20_M01..M06\Meshes` (66.6 / 71.8 / 72.4 / 70.9 / 75.4); no M05; ramps in `M5Workshop\Spaceship_M_P2_0x` | Main SM 60–65 each; `_Glass` 3.3–3.9; `_ShellCollision` 2.1–7.0. M02–M04 also use `AI_Shpis\Common\Lighting\SM_CommonLighting124x24`. Materials from `M5Workshop\RefitV3` (41.6) + CanopyV12; LiveryV3 35.5, Textures 117 shared | **Yes (inferred):** RefitV3 has INT ceiling/floor/wall/trim/bedding materials; 800–2,260 interior convexes per hull | Meshes tracked and modified. M01/M02 new materials untracked. **`Common\Lighting` untracked but referenced by tracked M02–M04 BPs** |
| M_P2_07, M_P2_08 | M | `Pack_2\Spaceship_M_P2_07/08` (103.6 / 110.4) + `M5Workshop\NextM` (68.7) | SM 77.7 (no Nanite) / 80.6; NextM Glass/Ramp/ShellCollision (33 each) | Yes (inferred) | tracked; 2026-10-07 |
| S_P1_03, S_P1_16 | S | `Pack_1\Spaceship_S_P1_03/16` (34.5 / 33.7) + `M5Workshop\NextS` (39.6, shared with S_P1_24) | SM 25.2 / 22.9 + NextS Glass/Ramp/ShellCollision | Yes (inferred; RouteAirlock, lamp materials) | 6 new NextS materials untracked |
| S_P1_24 | S | `Pack_1\Spaceship_S_P1_24` (329.3) | `SK_Spaceship_S_P1_24` 233.6 (LFS) + `SM_` 59.0; NextS shell 33.3 (1.55M tris) | Yes (inferred) | tracked |
| L_P1_08/11/15/17 | L | `Pack_1\Spaceship_L_P1_xx` (37.1 / 30.8 / 38.7 / 31.8) + `M5Workshop\ClassL` (37.4) | SM 18–26 + ClassL Glass/Ramp/ShellCollision (shells 6.4–7.9) | Partial / unknown | **`M5Workshop\ClassL` (32 files) fully untracked but used by tracked, modified L BPs** |
| L_P1_07 | L | 20.4 | SM 9.8 | No | tracked; not in DA_ShipCatalog |
| XL_P1_14 | XL | 29.2 | SM 16.1 | No | tracked |
| S_P1_01, S_P1_02 | S | 27.9 / 21.3 | SM | No | tracked |
| P1_05 / P1_06 | unclassed | 59.3 / 20.9 | SM 15.8 / 9.6; BPs M/XXS/Experimental (05), S/XXS (06) | No | tracked |
| XXS_P1_09/18/22 | XXS | 21.3 / 28.3 / 57.5 | SM | No | tracked |
| Car_P1_12 | car | 17.2 | SM 10.1, no BP; placed in the start level | — | tracked |
| Unused_P1_25/26, Unused_P2_05 | — | 29.9 / 29.9 / 73.3 | no references | — | tracked |
| Pack_2 originals M_P2_01–04/06 | M | 462.9 total | SM 57–61 each; no BP references them (unused source) | — | tracked |
| M5Workshop CanopyV10/V12/… | M | ~425 | superseded canopy iterations; only CanopyV12 materials still used | — | tracked |
| Legacy M1/M2/M2M/M2X1/M3/MX | M | `C\Spaceships\BP_Spaceship_M*` + `A\Meshes\MSHIP_HULL_X_*`, `SM_MERGED_SHIPM_MESHES` (6.4) | M3 pulls APS_PREA gear/ramp/engines (ignored) | Partial | M3 = AstroGenerator default, child actor in HQ_Alpha |

## 2. Stations, HQs, shipyards

All tracked BPs in `C\Stations\` or `C\SpaceInfrastructure\` (mostly modified 2026-09-27/28). Menu defaults:
station `BP_SpaceStation_R2`, HQ `BP_SpaceHeadquarters_Alpha`.

| BP | Meshes (source, git) | Walkable interior |
|---|---|---|
| **BP_SpaceHeadquarters_Alpha** (2026-10-05) | `A\HQ_Alpha\Meshes`: SM_HQ_Bridge_Alpha 9.8, Hub 17.8, Pads 22.3, Passages 0.1, Ramps, ShipYard 28.7 (78.8, tracked). Also `/Game/APS_PREA/.../STATION_STR/SM_MERGED_vert_649xx` (143 MB, **ignored**), MTM CargoShip door, ModSciInteriors `SM_Stairs_A`, ModularSciFiOffice `SM_Floor`, child BP_Spaceship_M3. Depends on ignored packs missiontominerva, BigCompanyArchViz, StarterBundle, ModularSciFiOffice, Showcase | **Yes:** bridge cabin, hub, passages, airlocks, ramps |
| BP_SpaceHeadquarters (legacy, in the start level) | APS_PREA `TO_ALPHA\SM_MERGED_ShipYard10` (72), `StationDockingHub` (36.5), `Pads_Large`, `vert_649xx`, ModSci doorway | Partial |
| BP_SpaceHeadquarters_V2 | APS_PREA `TO_ALPHA\HS\HomeStationC` (25.5) + TO_ALPHA meshes | ? |
| BP_SpaceStation_R2 / R1 / H2 | `A\Meshes\SM_MERGED_StaticMeshActor_<n>` (tracked; candidates `_2` 0.8, `_124` 38.1, `_388` 45.6) | No (inferred) |
| BP_SpaceStation | `A\Meshes\SM_MERGED_STATION_RING` (3.1) | No |
| BP_SpaceStation_V1 | `SM_MERGED_BakedStaticMeshActor_C_826` (0.4) | No |
| BP_SpaceStation_H1 | only material `M_Station_MESH_PadsArm_V60` referenced | ? |
| BP_SpaceStation_SB1 | MTM `BldgLgPowerStation_A_LandingPad` (ignored) | No |
| BP_STATION_LaunchStation | APS_PREA `STATION_RING`, `PadsArm_V51/V60`, `SSS_Kernel_V60`, BakedStaticMeshActor; interior parts ModSci_Engineer doorway, SpaceColonies wall tiles, SpaceshipInterior wall, Props ceiling light; parent `APS_PREA\...\BP_STATION_TechGravity_VEA` | Yes (inferred) |
| BP_STATION_HANGARS / START / PadsStation-PRE2 | APS_PREA merged meshes + TechGravity_VEA | ? |
| BP_SpaceShipyard / _B | MTM LandingPad (1.4, ignored) + StarterContent hex tile | Pad only |
| BP_SpaceShipyard_XXX | no mesh references | — |

`C\BP_SpaceHeadquarters`, `BP_SpaceShipyard`, `BP_SpaceStation`, `BP_SpaceStation_SB1`, `BP_Spaceship_M1..M3` in
the `C\` root are ~1 KB each (redirectors).

**Station meshes with no BP:** `A\AI_Stations` (31 files, 560 MB, tracked): 01 (41.3; also SpaceHub megastructure
and the start level), 03 (41.0), 03_01 `station_clean_v02` (46.0), 03_02 `station_final` (41.4), 04 (40.9); 02 and
04_01 textures only. Dates 01/02 2026-02-19; 03/04 2026-08-02. Pack_1 `SpaceHeadquarters_P1_10` (26.3) and
`SpaceShipyard_P1_04` (25.9): placed in the start level only.

## 3. Buildings and colony

**Colony HQ.** BP `A\ColonyHQ\BP_ColonyHQ` (spawned by `Actors\Tech\Colony.cpp:18`), modified 2026-10-06. Meshes
SM_ColonyHQ_Shell 1.5, Trim 1.8, Floor, Glass, Collision; also 4 `SM_CargoCrate_L_*` and MTM table/kiosk/sofa
(ignored). Textures 59 files, 77.9 MB; 15 `T_HQ_Sci*` untracked. Source `F:\Rio\3D\Colony\HQ\out\`. Walkable: hall,
command zone with holotable, airlock, corridor, living area, storage racks.

**AI_Buildings.** `A\AI_Buildings\01` (15.9), `02` (16.0, tower with ring collar), `03` (13.5); tracked; all placed in
`L_APS_SinglePlay_StartLocation`. `A\Electronics\01`/`02` (15.7 / 15.6) also placed there.

**Colony module catalogue** (`Colony\APSColonyModuleCatalogue.cpp:8-11`) — pack meshes from SpaceColonies
(**ignored**), each with a primitive fallback: Habitat primitives · Greenhouse `SM_EvergreenTree` ×4 +
`MI_WindowGlass02` · Storage `SM_ColonyBox01` ×6 · Floodlight `SM_FieldLamp_Bottom` · SolarArray primitives ·
CommsMast primitives · SolarWing primitives · CargoPods `SM_ColonyBox01` ×6 · NavBeacon primitives.

**Construction props** (`Construction\APSConstructionCatalog.cpp:143-223`): CargoCrate SpaceColonies box (ignored) ·
CargoContainer KB3D MTM CargoB (ignored) · WaterTank BigCompanyArchViz (ignored) · Terminal ModularSciFiOffice
(ignored) · SignalTower `A\Meshes\KB3D_MTM_BldgLgTerraformer_A_ElevatorTowerB` (5.77, **tracked**) · LandingPad,
SolarPanel, LightMast, NavBeacon, Barrier primitives. The tracked `A\Props\CargoCrates` set (12 SM + 12 BP,
L/M/H × Blue/Grey/Orange/Red, 5.5 MB) is **not** used by the catalogue.

## 4. Infrastructure visuals (31 types)

`Expansion\APSInfrastructureCatalog.cpp`, `APSInfrastructure.cpp:524-575`: Station (11 types) → `BP_HomeSpaceStation`
(R2) · Outpost (6) → `AAutonomousOutpost` cylinder + 2 cube wings + cone · Beacon (3) → the outpost at 0.6 · Shipyard
(2) → `BP_HomeSpaceShipyard` · Headquarters (3) → `BP_HomeSpaceHeadquarters` (HQ_Alpha) · SpaceHub, GrandHub,
SpaceElevator, OrbitalRing → authored meshes (§5) · DysonSwarm, DysonSphere → engine shapes. 25 of 31 reuse the
home station/shipyard/HQ BPs or primitives.

## 5. Megastructures (`Megastructures\APSMegastructures.h:30-48`)

| Role | Asset | Mesh MB / folder | Git |
|---|---|---|---|
| SpaceHub (×10,000) | `A\AI_Stations\01\73a7128a…` | 41.3 / 85.0 | tracked |
| GrandHub (×100,000) | `Pack_1\Megastructure_P1_20` | 16.5 / 28.6 | tracked |
| Elevator tower | `A\AI_Buildings\02\88d53275…` | 16.0 / 25.9 | tracked |
| Elevator tether | `Content\APS_PREA\APS_APOSFERA_SPACETRIPS\Meshes\SM_MERGED_BP_SplineElevator_Vert36_2` | 43.9 | **ignored** |
| Counterweight | `Pack_1\Megastructure_P1_13` | 10.0 / 24.6 | tracked |
| Orbital ring | `Pack_1\Megastructure_P1_23` | 16.5 / 27.9 | tracked |
| Level-only | `Megastructure_P1_19`, `P1_21` | 16.5 each | tracked |

## 6. Ground vehicles (`Vehicles\APSGroundVehicles.cpp:54-58`, `Spaceship.cpp:7015-7043`)

Rover `/Game/Vehicles/OffroadCar/SM_Offroad_Body` (2.71), `SM_Offroad_Tire` (0.83), `SKM_Offroad` (8.5) — **ignored**
(Vehicles 200 MB). Hover `Pack_1\Spaceship_P1_06` (fallback XXS_P1_09). Drone `Pack_1\Spaceship_P1_05` (fallback
XXS_P1_22). Audio `Audio\Vehicles` (7 SW). Unused: `Content\DriveableMarsRover` (121 files, 376 MB, ignored), MTM
`VehicleRover_A` parts.

## 7. Characters

`BPdir\BP_CustomGravityCharacter` / `_SpeedModes` (menu default) → `A\ControlledPawns\GravityPawn\SkeletonMesh\
AstroGirl_Quinn\SK_AstroGirl_Quinn` (6.0, tracked); `/Game/Characters/Mannequins/Animations/ABP_Manny` (ignored);
C++ loads `BPdir\APS_ABP_Manny` and `ABP_ZeroGAnim`. `BP_APS_GravityPilot_Single_Renew` → `/Game/AstroGirl/…`
(ignored), `APS_AnimBP_AstroGirl` (tracked). The missing `/Game/APS/APS_ALPHA/Blueprints/AnimBP_AstroGirl` path
(status 03.10) does not exist; `BPdir\APS_AnimBP_AstroGirl` (tracked) and `Content\AstroGirl\BP\AnimBP_AstroGirl`
(ignored) do. Animations: `Anims` (74), `GravityPawn\Animations\FlightLocUE5` (165, 66.5 MB), AstroGirl_10. Walking
anims reference a missing `/Game/Player/SK_Pirate` (source skeleton only). Other skeletal meshes (ignored):
Sci_Fi_Character_08, SpaceCreatures, mannequins, HugeAnimBundle (7,782 files, 927 MB).

## 8. Interiors and props

Tracked: cargo-ship props, HQ_Alpha and ColonyHQ interiors, `A\Props\CargoCrates`, legacy `A\Meshes`. Ignored packs
(partly migrated): missiontominerva (39 meshes + 4 BPs, 1,004 MB: full `VehicleCargoShip_A` kit, PropHologramMap,
kiosks, sofa, stairs, landing pads, tunnel connector) · BigCompanyArchViz (81, 724) · StarterBundle (23, 437:
modular sci-fi halls, doorways, desks, vents) · ModularSciFiOffice (10, 84) · FreeFurniturePack (14, 196) ·
SpaceshipInterior (5, 404: door frames, pillar, stairs, railing) · ModSciInteriors (3, 366) · ModSci_Engineer (3,
190) · P3_ComputerStation (1, 281) · StorageHouse (2, 75) · Polar (1, 134) · Showcase (32 + 12 BPs, 1,144) ·
SciFiFlying (3 cockpit controls, 237).

## 9. Environment and planet props

Tracked `WSC\PlanetSurface\Diagnostics\`: `SurfaceScatter20260930V2` (8.2 MB: 10 SM_APS_Scatter_* — ColdGrass,
DryGrass, Grass, Pebble, RockA, RockB, SlabA, SlabB, TreeA, TreeB; 64 FC_), `FoliagePrototype20260929V1`;
`WSC\PlanetSurface\Materials`: 16 MI_APS_WS_* biome instances. Planet meshes `A\AI_Planet\01` (18.8), `02_Star`
(14.0), `A\AI_PLanetss\giant_icosphere_jupiter1to1_sub6`, `A\Star\XSM_APS_STAR_SPHERE_V10X2_AutoLOD`. Ignored:
`Ressources` (935 MB: cliffs, rocks, grass, trees, WorldScape master materials, moon heightmaps 130 MB, sand 308 MB),
`Foliages`, `Megascans` (no meshes), `Brushify`. `Plugins\AtmoScape` tracked (63.8 MB). Engine WorldScape content 2.3 GB.

## 10. Effects and materials

**No Niagara or Cascade system is referenced anywhere in Content/APS.** Available (ignored): SolarSystem (10:
Cat's Eye/Crab/Helix nebulae, quasar, asteroid trail, Saturn/Uranus rings, solar particles; + 15 meshes, 13 planet
BPs), AbstractFX (21), LevitationFX (29), StarterContent Cascade (6). Sky/star materials (tracked,
`A\Materials\Astro`): `M_APS_MainMenuNebula`, `_Cloud`, `_SoftCloud`, `_Stable`; `M_SpectralStarMat` + `_HISM`,
`_POINTS`, `_SUN`; `M_APS_GasGiantAtmosphere`, `M_SunMat`; `WSC\...\Atmosphere\M_APS_AtmosphereTail`; cloud volume
iterations V1–V33. APSStarRenderer is shaders and code only.

## 11. Audio

`Content\APS\APS_ALPHA\Audio` (70 files, 340.6 MB, tracked): 34 SC_/SCL_/concurrency + `DA_APSAudioBank`; ship cues
Idle/Mode/Start/Stop/Thrust/Warp, landing, steps, UI, ambience, music; footsteps 25 SW; vehicles 7; 4 music tracks
(339 MB, 3 in LFS). Cues pull waves from ignored packs: SpaceAmbBundle (1,224 files, 6,096 MB), EnergyFieldsSFX,
SciFiGameSounds, Interface_And_Item_Sounds, FootstepsMiniPack; MixOfAmbientMusic (439). Credits in `Content\AudioCredits`.

## 12. Third-party packs and VoxelPro

`APS_ALPHA.uproject` enables WorldScape, DirGravity, VoxelPro, Water, SunPosition, MVVM, ModelingTools,
APSStarRenderer. Engine Marketplace plugins: `Voxel\VoxelPro.uplugin` "Voxel Plugin Pro Legacy" (441 files, 206 MB,
2026-10-06), WorldScape_5.4 (655, 2,318 MB), DirGravity (287, 506 MB), RiderLink. VoxelPro is not in the project's
`Plugins\`; `BP_Planet_VoxelZones` (APS_PREA) is referenced by `L_APS_Start-Stations`.

## 13. Ignored or untracked but referenced

| Folder | Size | Referenced from Content/APS | Examples |
|---|---|---|---|
| `Content\APS_PREA` | 993 MB, 398 files (biggest `SM_MERGED_StaticMeshActor_337` 402 MB) | 21 files | HQ_Alpha → `STATION_STR\SM_MERGED_vert_649xx` (143 MB); `APSMegastructures.h:41` → SplineElevator tether; `BP_Spaceship_M3` → gears, ramp, engines; `BP_STATION_LaunchStation/HANGARS/START` → merged meshes + `BP_STATION_TechGravity_VEA`; `BP_SpaceHeadquarters/_V2` → `TO_ALPHA` meshes; start level → tether, DockingHub |
| `Content\Ressources` | 935 MB | 67 files + code | `WSC\...\ContinuityV1\M_APS_ContinuousTerrain`; `L_HomeStation`; start level (MI_Magma); `APSSharedTerrainMaterialBuilder.h` → `MM_WorldscapeMaterial_V2`; `APSSharedTerrainLodABBuilder.h` → `T_MountainSide` |
| missiontominerva | 1,004 MB | 32 files | HQ_Alpha meshes/materials, BP_ColonyHQ, shipyards, construction catalogue |
| Characters / AstroGirl | 402 / 303 MB | 423 / 81 files | all Anims, character BPs |
| SpaceColonies / Vehicles / BigCompanyArchViz / ModularSciFiOffice / StarterBundle / MBLS | — | code catalogues, HQ materials, menu widgets | — |
| Untracked inside `Content/APS` | — | used by tracked BPs | `AI_Shpis\M5Workshop\ClassL\` (L ship BPs), `AI_Shpis\Common\Lighting\` (M02–M04), new `Pack_3` materials, `ColonyHQ\Textures\T_HQ_Sci*` |

Code paths under `/Game/APS/WSC/*` in `PlanetarySurfaceGenerator.cpp:59-75` are commented out (harmless).

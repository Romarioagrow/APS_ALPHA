# 06.10 — ship collision audit and budget (Claude flight, read-only audit)

Rio 06.10: Codex's detailed collisions are good for walking (keep them at rest); while a ship moves, especially at speed, it must fly on primitive approximate collision. Walking aboard a flying ship drops 120 -> 40-60 fps, flying near the fleet 120 -> 90-100. This document: inventory decoded from the assets, budget rules for Codex, proposed code changes. Asset changes and installs need Rio's explicit OK.

# Ship collision performance: merged findings (APS_ALPHA, still-ship @ 61532ed6)

**What the merge confirmed.** Re-checked read-only on the current tree:
- **Restore CVar never read.** `aps.Ship.HullRestoreOnce` is declared at Spaceship.cpp:1146 (default 1) and read nowhere. The restore path at Spaceship.cpp:2992-3019 calls four engine setters one after another and then `RecreatePhysicsState`.
- **Proxy guards.** The detailed-hull proxy check is at Spaceship.cpp:2967-2969 (CVars `DetailedHullFlightProxy`=1, `DetailedHullShapes`=512). The `bGenerateSimpleHullCollision` early return is at :2960. The proxy is switched only from PossessedBy :5353 and UnPossessed :5391.
- **M5 sweep duplicates.** APSM5HullSweepComponent.cpp:122-123 calls `AddIgnoredActor` once per shape, with no dedupe.
- **Fleet moves teleport.** APSFleetCommand.cpp moves units with `TeleportPhysics` at :1357, :1619 and :1649.
- **Gravity sphere.** Spaceship.cpp:1226-1229: QueryOnly, Pawn=Overlap, overlap events on.
- **ShipReport blind spots.** APSShipFlightBenchmark.cpp:2317-2319 counts shapes only when collision is enabled, from AggGeom only, and :2377 stops the list early.
- **Defer distance.** `aps.RealScale.DeferShipClearKm` defaults to 1,000,000 km (APSWorldOriginSubsystem.cpp:155-159).

**Correction to lens 1:** the working tree is **not** clean. `git status` lists 16 modified source files from other workflows, including APSWorldOriginSubsystem.cpp and APSShipFlightModel.cpp. None of the collision files are dirty: Spaceship.cpp/.h, APSShipFlightBenchmark.cpp, APSM5HullSweepComponent.cpp, APSFleetCommand.cpp, APSRealScale.cpp, CustomGravityCharacter.cpp, GravityDetectorComponent.cpp.

---

## (1) Ship collision inventory

**How the counts were obtained (verified).**
- Convex counts were decoded from the .uasset bytes the blueprints reference.
- For 8 hulls the decode matches the logged `[APS.ShipReport] SpaceshipHull shapes=` exactly. Codex's own install reports match as well.
- Two decoders give slightly different PhysicsOnly counts: lens 1 counted `M5_SKIN_*` names, lens 3 counted `ECollisionEnabled::PhysicsOnly` references. Both are shown as a range.
- The "Query / interior" column is the remainder. The number in parentheses is from Codex's install logs.

### Installed detailed hulls (Codex pipeline)

All of these are CTF_UseSimpleAsComplex on the root `SpaceshipHull`, which moves with the ship. Each also has an `M5HullShellCollision` trimesh: QueryOnly, complex-as-simple, double-sided, tag `APS.Ship.CollisionShell`. Ramp: 1-2 convex.

| Ship BP | Hull convex | PhysicsOnly skin | Query / interior | Shell trimesh tris | Proxy today | Starting-fleet candidate | Note |
|---|---|---|---|---|---|---|---|
| M_P2_06 | **22,870** | 20,606-20,703 | ~2,170-2,260 | 357,663 | seat only | yes | worst; never measured; includes 11,565 observation-bay prisms |
| M_P2_03 | 15,624 | 14,641-14,747 | ~880-980 | 317,391 | seat only | yes | |
| M_P2_04 | 15,512 | 14,574-14,670 | ~840-940 | 321,141 | seat only | yes | |
| M_P2_01 | 15,203 | 14,301-14,395 | ~810-900 | 331,684 | seat only | yes | |
| M_P2_02 | **13,501** | 12,532-12,616 | ~890-970 (969) | 327,479 | seat only | yes | measured case; also seen parked at SPACESHIPYARD 1 in p1-far logs |
| S_P1_24 | 10,177 | ~7,500-7,750 | ~2,400-2,680 (2,679) | **1,551,017** (4.57 M verts) | seat only | yes | heaviest S on both counts |
| M_P2_08 | 5,574 | 4,661-4,755 | ~820-910 | **1,519,408** | seat only | yes | |
| S_P1_03 | 5,359 | 4,493-4,541 | ~820-870 (853) | 527,749 | seat only | yes | |
| M_P2_07 | 4,688 | 3,749-3,859 | ~830-940 (925) | **1,528,516** | seat only | yes | the visual mesh has no Nanite (1.5 M render tris) |
| S_P1_16 | 4,443 | 3,721-3,750 | ~690-720 (709) | 505,994 | seat only | yes | |

All 10 BPs override `bOptimizeCollisionWhilePiloted=false`. Codex set this on purpose in the 03.10 "BP5 disable solid flight proxy" change.

### Cheap reference ships

| Ship | Hull convex | Skin | Shell | Proxy today | Note |
|---|---|---|---|---|---|
| S_P3_01 cargo | 208 | 0 | 60,418 tris (ShellCol, QueryOnly) | 5 proxy boxes (bOptimize stays true); bodyShapes 8 while piloted | `bAllowInStartingFleet=false`. This is the template for a good interior ship. |
| M3 legacy | hull 1; body 92 shapes over ~10 components | 0 | none | 5 proxy boxes | Unknown: which component holds SM_MERGED_SHIPM_MESHES (491 convex), and whether it is enabled when unpiloted. An M3 child actor sits at HQ. |
| Pack_1 originals: S_P1_01 11, S_P1_02 25, S_P1_06/XXS_P1_06 9, M_P1_05/XXS_P1_05/Experimental 7, L_P1_07/11/17 12, L_P1_08 9, L_P1_15 10, XL_P1_14 12, XXS_P1_09 12, XXS_P1_18 2, XXS_P1_22 5 | 2-25 | 0 | none | never (`bGenerateSimpleHullCollision` early return) | harmless at these counts |
| Ground vehicles: ROVER 1, HOVER 9, DRONE 7; shipyard SM_SpaceShipyard_P1_04 1 | — | — | — | — | no action |

### Offline, not installed (verified from Codex reports)

These are a threat if installed as authored:
- L class: L07 26,017; L08 14,546; L11 23,462; L15 12,136 (the class_l_refit candidate has 1,994); L17 14,793 (3,041 after one merge).
- XL_P1_14: 52,066.
- Codex's 06.10 skins: S03 4,813 and S16 3,642.
- V5 interiors: S_P1_01 427, S_P1_02 346, S_P1_06 85, XL14 2,809.

**These BPs keep `bGenerateSimpleHullCollision=true`.** Any of these meshes installed without flipping that flag would fly with **all** its shapes and no proxy. The same applies to `aps.ExpShip.Spawn` (APSExperimentalShip.cpp:86-87,176). The flag check is at Spaceship.cpp:2960.

### Fleet exposure (verified)

- DA_ShipCatalog has 26 entries, all weight 1.0. 19 are escort candidates (XXS..M with `bAllowInStartingFleet`), and 10 of those are the heavy hulls in the first table.
- Escorts are full ASpaceship actors attached to HomeSpaceShipyard.
- Fleet units never get the proxy: it is switched only on possession.
- The real fleet composition per seed has to be read at runtime.

### Why the numbers matter

Verified from UE 5.4 source and logs:
- **Moves.** Chaos `UpdateShapeBounds` loops over every shape on each transform (ParticleHandle.h:2762-2772).
- **Queries.** The scene-query visitor tests each shape's AABB *before* PreFilter, and the query-disabled check lives inside PreFilter (SQVisitor.h:199-256; CollisionQueryFilterCallback.cpp:87). So PhysicsOnly skins are walked by every nearby query.
- **Measured on M_P2_02 (13.5k shapes):**

| Cost | ms |
|---|---|
| SetBodyTransform, per move | 1.3-1.6 |
| one take-off move | 9-17 |
| per frame with physics wait (06.10) | 7-11 |
| M5 sweep | 0.6-1.2 |
| gravity-sphere UpdateOverlaps | 0.65 |
| stand-up restore | 299-305 |

Inferred (assumes cost is linear in shape count, which is supported but not measured per ship):
- About 45-90 ns per shape per query.
- **Per near-hull query:** M02 0.6-1.2 ms, M06 1.0-2.0, M01/03/04 0.7-1.4, S24 0.45-0.9, M07/08/S03/S16 0.2-0.5, cargo or a ~900-shape walk set 0.04-0.08.
- **Per move:** M06 ~1.7× M02, an XL14 at 52k ~4× M02.
- **Restore hitch:** M06 ~500 ms.
- **What the skin is used for.** 93-95% of each hull is PhysicsOnly skin. No pawn query ever hits it, and in the measured flights the hull was kinematic (`physicalImpulse=0 simulating=0`). So the skin does almost nothing and still pays the full cost.

### Rio's symptoms explained (inferred, fits the numbers)

**Aboard at speed: 120 → 40-60 fps.**
- About 8-10 character queries run per frame: camera boom probe, interaction LineTraceMulti, gravity ECC_Pawn trace, CMC sweeps, floor and step checks, foot IK, capsule overlaps. Each starts inside the hull's ~90 m AABB and walks all its shapes: 5-12 ms.
- Hull moves (autopilot slerp `TeleportPhysics` every frame, flow Rest repayment) add 1.3-11 ms plus physics wait.
- The gravity sphere's UpdateOverlaps adds 0.65 ms.
- In the seat the body is out of the scene and the arm probe is skipped, so 120 comes back at once.

**Near the fleet: 120 → 90-100 fps.**
- Any ship within 1,000,000 km ends the still-ship deferral, so the world flows. Every shift sends a physics transform for every body, because Chaos does not support origin shifting (APSWorldOriginSubsystem.cpp:459-492).
- So each nearby fleet M5 pays a full bounds walk every frame, about 1.3-1.6 ms each. The player's proxy sweep and gravity sphere also walk them.
- 1-2 fleet M5s ≈ +1.3-3.2 ms on an 8.3 ms frame ≈ 87-104 fps.

**Credit to Codex (verified):**
- Strong fidelity gates: SHA-locked provenance, watertight/convex/winding checks, entry SAT, 2M-sample coverage audits, capsule walk routes.
- The PhysicsOnly filter keeps pawn and camera traces off the skins.
- Hash-guarded query UCX.
- A clean role split in the Blender sources (visual, _Glass, _ShellCollision, _Ramp, interior UCX <10000, skin ≥10000).
- APSM5HullSweepComponent's BVH cut the sweep to 0.6-1.2 ms.
- The newest L adapter already adopts a 2,000-shape cap.

The problem is that no gate measures runtime cost, so the pressure always pushed toward *more* shapes.

---

## (2) Budget rules for Codex (ready to post in PLANET_EDITOR_WINDOW.md)

**Claude flight 06.10 → Codex: ship collision budget (Rio asked us to check collisions together)**

Measured on M_P2_02: 13,501 hull convexes cost 7-11 ms per frame of flight, about 0.6-1.2 ms per query near the hull, and 300 ms each time the pilot stands up. Walking aboard at speed drops Rio from 120 to 40-60 fps. Your visuals and walk routes stay; these rules limit only what moves with the ship and what queries see.

1. **No PhysicsOnly skin layer in any runtime body.** About 93-95% of every installed hull is PhysicsOnly skin. Pawns never touch it, but every move and every nearby query pays for it.
2. **Exterior flight contact = one FlightHull mesh** (UCX only, root body, Pawn=Ignore).
   - Volumetric decomposition (V-HACD or CoACD) of a closed envelope, voxel-remesh or shrink-wrap at 0.5-1 m. No slabs thinner than 0.5 m. At most 32 verts per hull.
   - Budget: XXS ≤8, S ≤16, M ≤32, L ≤64, XL ≤128 convex.
   - Outward error: S ≤0.5 m, M ≤1 m, L/XL ≤2 m. An entry notch is allowed.
3. **Walk set:**
   - Interior is one QueryOnly trimesh (preferred: your merged boxes bake straight into one), or ≤512 primitives per component. Split by deck or zone above that; ≤1,000 walk shapes per ship in total.
   - Stair wedges may be UCX; plain boxes as UBX_.
4. **Exterior shell trimesh:**
   - Welded (verts ≤ ~0.6 × tris), QueryOnly. Pawn/Camera/Visibility Block, all else Ignore. Never on a simulating or root component.
   - Target ≤60k tris; the cargo ShellCol at 60k has 2.9 mm mean / 17 mm max deviation. Hard cap ≤150k for S/M, ≤300k for L/XL.
   - Fix S_P1_24, M_P2_07 and M_P2_08 first (1.5 M tris of unwelded soup each). The M5 shells (0.32-0.36 M) come second.
5. **Hard reject at import:**
   - Any BodySetup >512 elements (the `aps.Ship.DetailedHullShapes` threshold).
   - Simple shapes enabled while flying >64 (S/M) or >128 (L/XL).
   - Complex-as-simple on a root or simulating component.
6. **Feature revisions add no flight or physics convexes.** This covers canopy, windows, bays and nose. They may change the visual mesh, shell and interior only. The install manifest reports per-feature deltas in shapes and tris. On M_P2_02 the canopy and flush windows alone added about 7.5k shapes.
7. **Small parts:**
   - Ramp ≤4 convex. Glass NoCollision.
   - Landing gear ≤6 primitives. Docking: one QueryOnly trigger.
   - No overlap events on hull meshes.
8. **One mesh per role:** SM_<Ship> visual (no UCX, NoCollision), _FlightHull, _ShellCollision, _Interior, _Ramp, _Glass.
9. **BP flags at install.** A BP that receives a hull with >512 elements must not keep `bGenerateSimpleHullCollision=true`; otherwise it flies with no proxy at all. Flip it the way you did for S03/16/24. This applies to every L/XL/S_P1_01/02/06 V5 install.
10. **Runtime gate before "installed":**
    - Run `run_night_check.ps1 -Ship <id> -Trace` seated and walking aboard at cruise, plus the ShipReport (and `aps.Ship.CollisionReport` once it exists).
    - PASS: SetBodyTransform ≤0.3 ms per frame for the player ship and ≤0.05 ms per AI/fleet ship; a 10 cm capsule sweep ≤50 µs; walk-aboard at cruise ≥90% of seated fps; stand-up ≤16 ms.
    - Without PASS the status is "candidate".
11. **Freeze** installs of L07/08/11/15/17, XL14 and the 06.10 S03/S16 skins until they meet these rules.
    - Migration order: M_P2_02 (measured), then M_P2_06 (worst, 22,870), S_P1_24, M_P2_07/08, then the rest.
    - Per ship: drop the ≥10000-suffix UCX from Main, export the <10000 boxes as _Interior, author _FlightHull from the existing shell envelope, decimate the shell.
    - Visual, UV, paint and sockets stay as they are; your digest guards already prove that.
12. **Asset edits and imports** go through Rio's explicit OK and an agreed editor window. One heavy run at a time.

Numbers in rules 2-5 and 10 are proposals grounded in the measurements above. Rio and Codex may adjust them; the ordering and the "no skin in runtime bodies" rule are the load-bearing parts.

---

## (3) Code changes, grouped per file

**Shared rules:**
- All proposed, nothing written.
- New behaviour is CVar-gated with "as today" defaults for Rio's A/B. Only behaviour-identical fixes default to on.
- Ownership: Spaceship and fleet code belong to Claude flight. APSM5HullSweepComponent was written by Codex; tell him before editing. The character and save code is a shared zone per CLAUDE.md, so use a narrow handoff.
- Suggested order:
  1. Report, sweep dedupe, RestoreOnce.
  2. Generated-flag guard.
  3. Fleet out of scene.
  4. Motion-driven mode.
  5. Walk-body split.

**Pawns/Spaceships/APSShipFlightBenchmark.cpp** (clean)
- Fix the existing ShipReport:
  - Count trimesh triangles per component.
  - Split shapes into PhysicsOnly and query.
  - Print `IsPhysicsStateCreated`.
  - Count shapes even when collision is disabled but the body is in the scene (editor `bEnableTraceCollision`).
  - Remove the early `break` at :2377.
- New command `aps.Ship.CollisionReport [All|Piloted|Class=<BP>|Catalog] [Live N] [Csv=1]`. Writes `[APS.CollisionReport]` lines and an optional `Saved/Logs/APS_CollisionReport.csv`.
  - Per ship: class, mesh, mode, speed, autopilot, fleet phase, walkers aboard, frozen, attach parent, proxy boxes.
  - Per primitive: runtime shapes via `GetAllThreadShapes`, split into convex, box and trimesh, and into query and PhysicsOnly.
  - Measured costs:
    - moveMs: `SetBodyTransform` between T and T+1 cm, 20 times.
    - queryUs: from PilotChair, PilotExitPoint and the bounds centre, a 10 cm capsule sweep (r 42, hh 96, ECC_Pawn) without ignoring the ship, a 3 m Visibility line trace and a 40 cm sphere overlap, 20 each.
    - sphereOverlapMs: `UpdateOverlaps` 5 times.
  - Verdict PASS/WARN/FAIL against the rule-10 numbers.
  - `Catalog` spawns each DA_ShipCatalog class about 50 km away, measures it and destroys it, so it can run `-game -NullRHI`.
- Risks: low. Measure on the spawned instance, never on the player's ship in flight. Catalog spawns must clean up fully.

**Pawns/Spaceships/APSM5HullSweepComponent.cpp** (clean; Codex-authored)
- Collect the shape `Word0` values into a `TSet<uint32>` and add each once, cached in `FAPSM5HullSweepTree`. CVar `aps.M5.SweepUniqueIgnore`, default **1**.
- Today every shape of another actor (station, terrain, pad, ship) scans a 13.5k-entry `IgnoreActors` list linearly. That is a plausible share of the 9-17 ms take-off moves (inferred).
- Risk: very low, the result is identical. The engine dedupes the same way (WorldCollision.cpp:513-533).
- Later: rebuild the tree from `AggGeom.ConvexElems` and sweep the leaves with `FGenericPhysicsInterface::GeomSweepMulti`. The exact docking sweep then works with the hull body out of the scene.

**Pawns/Spaceships/Spaceship.cpp / Spaceship.h** (clean)
1. **Wire `aps.Ship.HullRestoreOnce`** (declared, default 1, unused).
   - While the body is out, set profile, responses, enabled state and non-simulating at BodyInstance level, then call `RecreatePhysicsState` once.
   - Skip the rebuild on EndPlay or teardown.
   - Otherwise delete the CVar so it stops promising behaviour that does not exist.
   - Risk: low. Removes the extra filter passes only; the ~300 ms build stays until item 4.
2. **Guard the generated-hull early return.**
   - Treat any hull whose `AggGeom.GetElementCount() > aps.Ship.DetailedHullShapes` as detailed in `SetFlightCollisionOptimization` (:2960) and `RebuildSimpleHullCollision` (:2205), whatever `bGenerateSimpleHullCollision` says.
   - No new CVar.
   - Risk: low. Pack_1 hulls (2-25 shapes) are unaffected; this protects V5 installs and `aps.ExpShip.Spawn`.
3. **Collision mode driven by motion, not the seat.** CVar `aps.Ship.CollisionByMotion`, default **0** (possession as today), plus `aps.Ship.MovingHoldSeconds` 2.0.
   - State: `enum class EAPSShipCollisionMode { Authored, Flight, WalkInFlight }`, `TSet<TWeakObjectPtr<ACharacter>> WalkersAboard`, `SinceMovedSeconds`.
   - `UpdateCollisionMode()` is called from Tick, UnPossessed (replacing the unconditional restore at :5391), the fleet hooks, BoardShip/LeaveShip and the thaw.
   - "Moving" means any of: |v| > 1 m/s, autopilot engaged, autopilot rotated this frame, fleet Departing/Transit, engine in a band. Held for 2 s.
   - **Flight**: proxy boxes in, hull body out.
   - **WalkInFlight**: proxies ignore Pawn/Camera/Visibility, plus the walk body (item 4). Until item 4 exists it falls back to the full hull, so nothing regresses.
   - **Authored**: stopped past the hold with a character within radius + 20 m, or a docking/landing approach (Hover band, or clearance < 50 m at < 20 m/s).
   - Risks:
     - Docking and landing must reach Authored in time.
     - The ~300 ms restore must happen only at rest, never on the stand-up frame at speed.
     - Ramp, interactions and gravity boarding must keep working.
     - Fall-through if proxies ignore Pawn and no walk body exists; the fallback covers it.
4. **Walk-body split.** New files `Pawns/Spaceships/APSHullWalkCollision.{h,cpp}`, CVar `aps.Ship.WalkBodySplit` default **0**.
   - `UAPSHullWalkCollisionComponent` returns a transient `UBodySetup`, cached per UStaticMesh, holding only the elements whose `GetCollisionEnabled() != PhysicsOnly`. Attached to the hull with identity transform and the hull's profile.
   - The hull SMC becomes visual-only. Every later collision setter on it must be guarded, because `Set*Collision*` calls `EnsurePhysicsStateCreated`.
   - Expected (inferred): walking, moves, shifts and restore on M02 go from 13,501 to ~900 shapes; restore around 20 ms.
   - Risks:
     - Must prove the `FKConvexElem` copies share the cooked `ChaosConvex`; otherwise there is recook and memory cost.
     - BP overrides and the ramp controller.
     - The M5 sweep needs the out-of-scene variant first.
     - Prototype it on M_P2_02 only.
5. **Gravity sphere at speed.** CVar `aps.Ship.SphereOverlapsAtSpeed` default **1** (today).
   - At 0, `SetGenerateOverlapEvents(false)` while a walker is aboard a moving ship; turned back on at rest or on LeaveShip.
   - Risk: the gravity source relies on cached overlaps (GravityDetectorComponent.cpp:283-296). The ridden ship is kept by its distance check, but a missed end-overlap on exit must be re-synced.

**Gameplay/Fleet/APSFleetCommand.cpp** (clean)
- Put a unit into Flight mode (hull body out, proxies in) when an order starts (Fly, Departing/Transit, around :1619). Return to Authored on Arrive or Return only when a character is near.
- A body that is out costs nothing on world shifts: no physics state, so the shift skips it.
- Gated by `aps.Ship.CollisionByMotion`, or a separate `aps.Fleet.HullOutOfScene` default 0 if it ships before item 3.
- Risk: medium-low. Fleet ships attach rather than physically dock. Ship-vs-ship contact stays on the proxies. Check that berth attach and detach do not re-create the body.

**Core/World/APSRealScale.cpp** (clean)
- After the thaw's `RegisterComponent` (:105-118), re-apply the ship's current collision mode, so a hull does not come back as a full body unless the mode is Authored.
- Inferred: today each thawed fleet M5 may cost about 300 ms; check the home-approach logs.
- Risk: low.

**Pawns/Characters/CustomGravityCharacter.cpp, GravityDetectorComponent.cpp** (clean; shared zone)
- BoardShip and LeaveShip (:1588-1652) notify the ship to add or remove the walker.
- Optional: skip the camera-boom probe against the ridden ship's PhysicsOnly shapes. This becomes moot after item 4.
- Risk: character code is shared, so hand off narrowly.

**Core/World/APSWorldOriginSubsystem.cpp** (**dirty: another workflow is editing it**)
- No edit proposed. The fleet cost comes from `DeferShipClearKm` (1,000,000 km) ending the still-ship deferral near any ship, so the world flows and teleports every fleet hull each frame. The fleet out-of-scene change removes that cost without touching this file.
- Use the existing `aps.WorldOrigin.ShiftCostReport` to measure it.

**Pawns/Spaceships/APSExperimentalShip.cpp**
- No edit; covered by Spaceship.cpp item 2.

---

## (4) Night report and test plan with pass criteria

**Before each run:**
- Re-read the tail of PLANET_EDITOR_WINDOW.md and the task list.
- Check RAM and GPU; ComfyUI plus UE rebooted the PC on 2026-09-28.
- One heavy run at a time, in an agreed window. Rio's editor stays open.
- Offscreen `-game` runs through `F:/ChatGPT/APOSFERA/work/flight/run_night_check.ps1`.
- The bench world defaults to 1 planet and 0 moons.
- Every case is run twice: CVars 0 (today) and CVars 1 (new), same seed and same phase.
- An offscreen PASS is not a visual PASS. Rio's own play test is the acceptance.

**Baselines on record (verified):**
- Seated at speed: 120 fps.
- Aboard at speed: 40-60 fps.
- Near the fleet: 90-100 fps.
- Stand-up restore: 299-305 ms.
- Per move: SetBodyTransform 1.3-1.6 ms.
- M5 sweep 0.6-1.2 ms; gravity-sphere UpdateOverlaps 0.65 ms.
- Take-off single moves: 9-17 ms.

| # | Test | Ships / setup | Record | PASS |
|---|---|---|---|---|
| T0 | Inventory cost (needs CollisionReport) | `aps.Ship.CollisionReport Catalog Csv=1`, `-NullRHI` | per class: shapes, PhysicsOnly, shell tris, moveMs, queryUs, sphereOverlapMs | counts match the section (1) tables (±1%); per-class cost curve established. Fallback before the report exists: ShipReport per piloted ship. |
| T1 | Each ship flying, seated | 30 s cruise + autopilot turn + a band change on M_P2_02, **M_P2_06**, M_P2_01, S_P1_24, M_P2_07, S_P1_03, S_P3_01, S_P1_01 (reference), XL_P1_14, M3 | APS.Perf avg/1% low, gt_top `Phys SetBodyTransform`, `M5SpatialHullSweep`, hitches | avg ≥ 0.95 × S_P1_01 fps (≈114+); SetBodyTransform ≤0.3 ms per frame; no hitch >50 ms; flags the seat-only proxy's real cost on M06 |
| T2 | Walk aboard | M_P2_02, M_P2_06, S_P1_24, S_P3_01; each **parked**, **cruise**, **autopilot turning** (`shipdrive exit=1`); then seat → stand → seat 5 times | fps, SceneQuery / UpdateOverlaps / SetBodyTransform ms, restore ms log line | cruise aboard ≥ 0.9 × seated fps (≥108 at 120); stand-up ≤16 ms with the split (today expect ~300 ms; M06 ~500); parked vs cruise separates query cost from move cost (prediction today: parked ~70-90 fps) |
| T3 | Near fleet ships | home shipyard with 2-3 parked fleet M5s: fly-by at 10 km and 1 km; one unit under orders (Departing/Transit); home approach with freeze/thaw | fps vs the same pose with no fleet (`aps.RealScale.DeferShipClearKm 0` as control), ShiftCostReport, `CollisionReport Live 30`, thaw hitch | ≥0.95 × no-fleet fps; ≤0.05 ms per fleet hull per frame; thaw hitch ≤16 ms per unit; a fleet ship ahead is still drawn where it is (Rio's 06.10 rule) |
| T4 | Docking and landing still work | M_P2_02 and S_P3_01: land on a pad, land on a planet, dock at the shipyard/HQ, ramp down/up, walk in from the ramp at rest, leave the ship, low-speed (<20 m/s) bump into a station | screenshots or frames at contact, mode log lines | no fall-through, no pass-through at <20 m/s, ramp and boarding prompt work, mode becomes Authored inside the Hover band / <50 m clearance before contact |
| T5 | Regression | Rio's 120 fps still-ship scene at hundreds of ly, stars streaming; save/load aboard; galaxy edge out and back | fps, crash or ensure, save round-trip | seated 120 unchanged (±3%); no new hitches; save/load aboard restores pose and collision |

**Night report contents (Russian, for Rio):**
- One table per test with today / new / delta, and the CVars used.
- Which ship is worst (expected: M_P2_06).
- The fleet composition the seed produced.
- What stays unverified: a visual PASS needs Rio; the M3's 491-convex component; trimesh-shell query cost under walking; whether convex copies share cooked data.
- The Codex handoff status: did rule 10 get adopted?
- Nothing is committed until Rio has played it, and only our own files go in.

**Open unknowns (inferred, not measured):**
- The linear per-shape scaling across ships.
- The real fleet mix per seed.
- The thaw hitch per fleet M5.
- How much of the 9-17 ms take-off comes from the duplicate ignore list.
- Shell trimesh cost for the 1.5 M-triangle NextM/S shells.
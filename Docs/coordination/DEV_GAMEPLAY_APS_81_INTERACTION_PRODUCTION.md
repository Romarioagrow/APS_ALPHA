# APS-81 — Gameplay Interaction & Production checkpoint ledger

Status: active Goal; source contract implemented, integration and heavy verification pending.

## Scope and ownership

Dev Gameplay Interaction & Production owns new code under:

- `Source/APS_ALPHA/Gameplay/Interaction/**`
- `Source/APS_ALPHA/Gameplay/Production/**`
- new focused APS-81 test files

Dev UI owns all prompt/panel visuals, localization, input glyphs, layout, focus and accessibility. Quest owns objective state, adapters and persistence. Surface owns terrain suitability, placement anchors and WorldScape. Civilization owns the runtime manifest, actor identity binding, starter materialization and shared save seams. Dev2 owns canonical astronomical identity/topology. Dev Ships owns ship classification, definitions, adapters and flight actors.

Shared files remain read-only until a named handoff. Current frozen seams include:

- `Pawns/Characters/CustomGravityCharacter.*`
- `Core/Controllers/GravityPlayerController.cpp`
- `Core/Saves/GameSave.h` and `SavedActorData.h`
- `Gameplay/Civilizations/**`
- `Core/World/APSPlanetSurfacePlacementResolver.*`
- `Generation/AstroGenerator.*`
- `Pawns/Spaceships/**`
- `Actors/Tech/SpaceHeadquarters.*` and `SpaceShipyard.*`
- existing UI panels and assets

## Audited baseline

- `F` is mapped to `Interact`, `CharacterAction` and vehicle `ReleaseControl` in legacy input.
- The production character currently performs one visibility trace and only recognizes `IVehicleControlling` actors.
- The interaction prompt is constructed directly inside `ACustomGravityCharacter`; it hard-codes `F TAKE CONTROL` and the actor object name.
- No generic actor interaction, crafting, construction, production queue or Quest-domain event contract existed before APS-81.
- `ASpaceShipyard` is an empty `ASpaceStation` subclass. `ASpaceHeadquarters` exposes a start point but no gameplay console.
- Existing HUD and civilization Widget Blueprints are inventory/evidence only; no APS-81 mutation is authorized.
- Save/load serializes `ABaseActor` state and mirrors Civilization `StableEntityId`, but it has no generic production-job snapshot yet.

## Canonical identity rules

- APS-81 consumes canonical IDs and never reimplements their hash/generation algorithms.
- Civilization actors use `UAPSCivilizationIdentityComponent::StableEntityId`, with owner/faction/parent/role metadata from the runtime manifest.
- Cluster/system actors use Dev2-owned stable records. Planet/moon identity is currently a deterministic string key, not a generic `FGuid`; APS-81 must not hash it locally.
- Production job, recipe execution and build-order IDs are generic persisted IDs outside the starter Civilization manifest.
- Missing authoritative actor IDs are permitted only in an explicit debug-only launcher path. Production actor-gated access rejects and logs them.

## Interaction contract

The first source checkpoint provides:

- transient query context: instigator, canonical subject ID, view origin/direction, range and explicit debug override;
- presentation-neutral prompt snapshot: stable prompt ID/revision, semantic context/kind, canonical target ID, localization token/arguments, distance/range, availability, typed actions, hold state, priority and expiry;
- actor interface for target identity, query and execute;
- tickless subsystem gates for focus, distance, canonical subject/target identity, expected revision and enabled action;
- execute-time re-query so stale UI cannot bypass range or revision validation.

Dev UI will later bind these snapshots through UI-owned presenters/adapters. Gameplay never provides brushes, colors, raw key glyphs, widget pointers or pre-styled text.

## Neutral production event contract

`FAPSProductionEvent` carries:

- unique `EventId` and lifecycle `CorrelationId`;
- semantic `Verb`;
- canonical `SubjectStableId` and `TargetStableId`;
- quantity;
- `Requested`, optional `Started`, then exactly one terminal `Succeeded`, `Failed` or `Cancelled` result;
- stable result/failure codes;
- non-authoritative context tags;
- monotonic sequence and telemetry-only world time.

The publisher rejects duplicate event IDs, missing request roots, payload drift across one correlation, repeated `Started`, and all events after the first terminal result. Quest subscribes to the native multicast, consumes terminal success by default and deduplicates by event/correlation ID. Quest never calls production executors or infers completion from UI.

## Civilization bootstrap seam

Civilization implemented the accepted native materialization lifecycle hook in `8e5dd33a`:

- restore only stages persisted state and does not broadcast;
- one actor-ready broadcast occurs after successful upsert;
- `IsMaterializationComplete()` is true inside that callback;
- current state is `Materialized` for new actors or `LoadedFromSave` for restored actors;
- late subscribers call the manifest getter and completion getter once.

APS-81 will enable headquarters/shipyard actor-gated services only for those two actor-ready terminal states.

## Surface placement seam

APS-81 consumes APS-78 without editing it:

1. Build one request from canonical body data and stable manifest/order seed.
2. Call `TryResolveCivilizationFootprint`.
3. If terrain resolved but not ready, retain the same placement key, ordinal and transforms.
4. Request idempotent anchors and bounded-poll the same request on the game thread.
5. Never reroll a candidate during polling.
6. On ready, Gameplay owns overlap/sweep, resources, materialization, persistence and events.
7. Release only owned anchors on success, cancel, timeout and world teardown.

Gameplay must not sample a second noise model, mutate player/collision anchor overrides, force-reset WorldScape or claim foliage clearance.

## Ship production identity seam

Current ship selection is class-path based and concrete ship instances have Civilization entity IDs; there is no stable ship-definition ID yet. The accepted forward contract from Dev Ships is:

- authoritative type: `FPrimaryAssetId ShipDefinitionId` with Primary Asset Type `Ship`;
- `ShipDefinitionSchemaVersion` for migration;
- optional non-authoritative `FSoftClassPath ResolvedClassSnapshot` for diagnostics/dual-read migration only;
- no mesh path or pawn class path as authoritative identity;
- Dev Ships catalog resolves the definition and verifies a non-abstract, non-deprecated, placeable `ASpaceship` class;
- mesh-only definitions go through a Dev Ships adapter; APS-81 never classifies meshes.

APS-81 owns job, owner, shipyard and spawn-pad IDs plus queue/spawn orchestration after catalog resolution.

## Checkpoints and gates

- `267ae334` — actor interaction and neutral production event source contracts.
- `4081cd3b` — execute-time focus/range/revision/action gates and correlation payload invariants.
- `6c4486fe` — focused interaction/production contract automation source.

No compile or automation PASS is claimed yet. The global heavy slot is single-owner; builds/tests wait for an explicit release and coordinated reservation. Existing Unreal/foreign processes must never be interrupted by APS-81.

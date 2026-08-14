# APS-80 — Quest & Onboarding architecture and ownership

- Status: approved architecture checkpoint; implementation is additive and in progress.
- Goal: `APS-80 — [EA] Quest & Onboarding Framework`.
- Jira: https://aposfera.atlassian.net/browse/APS-80
- Owner: Dev Quest & Onboarding.
- Date: 2026-08-14.

## Guardrails

Quest consumes committed gameplay facts. It does not execute interaction, production,
flight, generation, Surface or civilization operations and does not infer completion from
UI, actor names, transforms or polling.

Dev Quest exclusively owns these additive paths:

- `Docs/coordination/DEV_QUEST_ONBOARDING_APS80.md`;
- `Source/APS_ALPHA/Gameplay/Quests/**`;
- `Source/APS_ALPHA/Tests/APSQuest*`.

The following remain shared or owned by other streams and require a separate per-file
handoff before any edit:

- UI and prompt presentation: Dev UI, `UI/Quest/**`;
- civilization manifest/materialization and save/controller integration: Dev Civilization;
- WorldScape, landing context and Surface readiness: Dev Surface;
- generated world, system/body StableId and selection: Dev 2;
- interaction/build/craft/ship execution and emitted results: Dev Gameplay Interaction & Production;
- spaceship possession, navigation and flight state: the ship/flight owner;
- `APS_ALPHA.Build.cs` and module dependencies: shared.

No legacy authored map, generated model, actor hierarchy or existing UI is a Quest source
of truth. The accepted evolution-over-rewrite baseline remains intact.

## Runtime boundary

The framework is split into an isolated deterministic runtime and a thin
`UGameInstanceSubsystem` facade. Game-instance lifetime lets progress survive map travel;
world adapters bind and unbind owner-provided event sources without scanning actors.

Definitions are immutable `UPrimaryDataAsset` content:

- stable `QuestId` and positive `DefinitionVersion`;
- stable node IDs;
- an explicit entry node;
- directed success edges;
- event predicates, typed entity bindings, progress thresholds, prompts and rewards.

Runtime node state is explicit:

`Dormant -> Active -> Satisfied -> Completed`, with `Suspended` and `Failed` reserved for
diagnosable dependency or content failures. V1 rejects duplicate IDs, missing endpoints,
unreachable nodes and cycles. Recovery loops are policies, not hidden graph cycles.

## Identity contract

Quest never invents a GUID from an actor name. A typed entity reference preserves the
canonical identity domain:

- civilization/base/pad/selected ship: `FGuid` from the civilization manifest;
- star system: canonical system `FGuid`;
- planet/moon: canonical stable body key, for example `SYS0/S0/P0` plus moon suffix;
- landing context: deterministic Surface placement/landing-context key.

Named bindings let generated entities be attached to authored objectives at runtime.
Rebinding a name to a different canonical entity is rejected; recovery must explicitly
resume or restart rather than silently retargeting the player.

## Event contract

Quest receives normalized immutable events with schema version, stream ID, event ID,
correlation ID, monotonic sequence, verb, result, subject/target references, quantity and
stable failure code.

One gameplay request may emit `Requested`, `Started`, and exactly one terminal
`Succeeded`, `Failed` or `Cancelled` result under the same correlation ID. Objectives
advance on terminal success by default. Event and terminal-correlation ledgers provide
at-least-once deduplication; timestamps are telemetry only.

`GameplayTags` is not currently a module dependency, so the isolated V1 uses stable
`FName` verbs and failure codes. A future GameplayTag migration needs an explicit
`APS_ALPHA.Build.cs` handoff and save-schema migration.

## Rewards, prompts and persistence

Rewards are idempotent commands sent to an owner-provided sink. A deterministic
transaction ID is derived from quest instance, node and reward ID. The ledger records
requested/applied/failed state; Quest never mutates inventory, crafting or economy state
directly.

Quest publishes immutable prompt snapshots. Dev UI owns the future
`IQuestPromptPresenter`/Slate adapter. Prompt acknowledgement is persisted only when it
changes gameplay state. Toasts and purely explanatory hints are presentation state.

The isolated save payload is versioned and contains:

- quest definition and instance identity;
- node state and progress;
- typed named bindings;
- last accepted stream sequence;
- bounded event/correlation dedupe ledgers;
- reward transaction ledger;
- suspension/failure and recovery reason.

Integration into `UGameSave` happens only after the Civilization/save owner hands off the
exact shared files. An old save with no Quest payload starts from the first safe unresolved
checkpoint. Unsupported newer Quest data suspends only Quest progress and must not make
the generated world unloadable.

## Failure, recovery and debug

- A failed/cancelled domain action normally leaves the objective active and records a stable
  reason for a retry prompt.
- Missing identity, adapter or Surface readiness suspends progress; it never completes the
  objective or substitutes another entity.
- Recovery policies are explicit: retry, resume after same-ID rebind, restart current node,
  or non-shipping skip.
- Debug exposes definition validation, graph/state dump, event trace, binding inspection and
  deterministic injection/skip hooks.
- Injection, skip and replay cannot apply production rewards and are disabled in Shipping.

## First Steam Early Access route

1. Receive the materialized civilization manifest and bind the exact base, pad and selected ship.
2. Confirm the intended player character is controlled in the start context.
3. Inspect/use the base or station interaction target.
4. Complete one committed build/place action. A separately authored inspect/unlock branch is
   permitted only while the APS-81 production capability is unavailable.
5. Reach the landing pad and take control of the bound selected ship.
6. Start engines, take off and select a canonical planet or moon navigation target.
7. Commit arrival and landing context for that same stable body key.
8. Wait for Surface profile, LOD0 publication and spatial collision readiness; enter Surface.
9. Recover from one representative failed or cancelled action without duplicate progress or reward.
10. Exit Surface, return to orbit and restore canonical system selection; complete the route.

Save/load checkpoints cover pre-build, in-flight, landed and completed states.

## Dependency event seams

- Civilization: native materialization-state multicast plus immediate read-only manifest getter.
- Interaction/Production: versioned terminal domain-event envelope from APS-81/APS-99.
- Surface: landing committed, anchor ready, entered, exited and recovered events containing
  stable body key and deterministic landing context.
- Astronomy: canonical selection/arrival events and a read-only resolver for system/body identity.
- Flight: possession, engine/takeoff, navigation target and environment transition events.
- UI: immutable prompt snapshot presenter; no Quest state or save logic.

Adapters consume these seams after their owners publish them. Quest does not add actor-spawn
listeners or temporary polling fallbacks.

## Verification gates

Lightweight automation must cover definition validation, deterministic transition order,
duplicate/out-of-order events, named identity bindings, reward exactly-once behavior,
save export/restore, failure/recovery and non-shipping debug guards.

End-to-end evidence is deferred until all owner adapters exist and the single project-wide
heavy UE slot is explicitly free. The run must cover start -> build -> ship -> flight ->
planet/moon -> Surface -> system -> save/load without duplicate actors, generators,
materialization or rewards, and without visual/performance regression.

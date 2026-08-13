# ADR: Evolution over Rewrite

- Status: Accepted architectural guardrail; implementation migration remains on hold.
- Date: 2026-08-14
- Scope: Full-Scale Coherence / Visual Fidelity sprint.

## Decision

The current canonical generation pipeline evolves incrementally. The authored SinglePlay level and its serialized generator remain a frozen visual and integration reference. They are not the runtime source of truth for newly generated worlds, and they must not be mass-renamed, deleted, resaved, or automatically composed into current generation.

Historical formulas may be reused only after a diff-backed audit demonstrates their domain meaning and after they are introduced through an explicit adapter. The old pipeline is not copied wholesale.

## Ownership map

| Concern | Frozen legacy/reference owner | Current runtime owner | Rule |
|---|---|---|---|
| Authored start location | `/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation` | None; this is an explicit authored route | Open only when `bUseAuthoredSinglePlayWorld=true`; do not reinterpret as generated data. |
| Legacy astronomical integration | Placed `BP_AstroGenerator`, `bIntegrateStartPlanet`, `WSR_StartHomePlanet`, `InitLegacyAuthoredGenerationLevel` | Isolated runtime generator in `AGravityGameModeBase` | Never copy serialized actor pointers across map travel. |
| Live generator UI | Historical widgets/assets are reference only | `UGeneratedWorld`, `UWorldGenerationViewModel`, tagged `WorldGenerationPreview` generator | UI preview owns a transient generator and must not reuse placed gameplay generators. |
| Generated gameplay handoff | Not applicable | GameInstance-owned duplicate of `UGeneratedWorld` plus `USpawnParameters` | The menu snapshot is immutable across `OpenLevel`; gameplay spawns one isolated generator with legacy integration disabled. |
| Identity/topology | Authored placements are visual evidence only | stable IDs, generation seeds, canonical model records and parent IDs | No reroll or independent placement during scope changes. |
| Physical units | Useful historical km/AU semantics | current model radii/orbits and documented conversion constants | Preserve domain values; do not restore giant float transforms literally. |
| Preview coordinates | Historical actor scaling is reference evidence | bounded uniform preview root + scope presentation transforms | Presentation may compress coordinates, but must preserve identity, directions, ordering and declared topology. |
| Gameplay coordinates | Authored level keeps its serialized layout | generated local system uses current physical/full-scale deployment | Scaling changes remain read-only until the Full-Scale Spike decision. |
| Gas giant reference | authored/reference actor setup and `giant_icosphere_jupiter1to1_sub6` asset | current `APlanet::GasGiantVisualComponent` and future catalog adapter | Reuse the asset through the current generator/catalog; never hardcode a placed level actor. |

## Frozen reference set

Primary frozen reference:

- `/Game/APS/APS_ALPHA/Levels/Alpha/L_APS_SinglePlay_StartLocation`
- its placed `BP_AstroGenerator`, `WSR_StartHomePlanet`, headquarters/station/ship hierarchy and serialized visual dependencies
- `/Game/APS/APS_ALPHA/Assets/AI_Planet/giant_icosphere_jupiter1to1_sub6`

Secondary visual/reference maps (audit only unless separately authorized):

- `/Game/APS/APS_ALPHA/Levels/DEV_WCS_PLANETS`
- `/Game/APS/APS_ALPHA/Levels/DEV_WCS_PLANetSystem_PS1`
- `/Game/APS/APS_ALPHA/Levels/L_DEV_SCALE`
- `/Game/APS/APS_ALPHA/Levels/Dev/L_CatalogStars`
- `/Game/APS/APS_ALPHA/Levels/Dev/hims_params`
- equivalent older `/Game/APS/Levels/**` maps retained for historical A/B evidence

The list is deliberately path-based. No asset rename or metadata resave is required to enforce this ADR.

## Current source-of-truth path

1. `UGeneratedWorld` holds the editable canonical model and stable user choices.
2. `UWorldGenerationViewModel` creates/reuses only an explicitly tagged transient preview generator.
3. `CommitAndOpenLevel` flushes the selected-body buffer and duplicates the model into `UMainGameplayInstance`.
4. Generated routes explicitly set `bUseAuthoredSinglePlayWorld=false`.
5. `L_WorldGeneration` uses `AGravityGameModeBase`, which spawns one deferred runtime generator, disables `bAutoGeneration` and `bIntegrateStartPlanet`, clears `WSR_StartHomePlanet`, supplies the committed model, and generates once.
6. The authored SinglePlay action instead clears generated snapshots, sets `bUseAuthoredSinglePlayWorld=true`, and opens the frozen authored map directly.

## Adapter boundaries

Approved compatibility points:

- model/save adapter: physical km, AU, type/subtype, seed, stable ID and parent ID;
- coordinate adapter (future, decision pending): canonical double/hierarchical frame to scope-local preview transform to local gameplay frame;
- presentation adapter: angular readability, mesh/HISM LOD, corona/bloom and culling without mutating canonical radii or positions;
- asset adapter: current catalog/profile selects legacy-proven meshes/material parents without referencing placed actors;
- handoff adapter: immutable GameInstance snapshot to one isolated runtime generator.

Forbidden shortcuts:

- importing the entire historical generator path into current runtime;
- copying placed actor transforms as canonical positions;
- using compressed HISM positions for physical gameplay calculations when a materialized actor exists;
- resaving or renaming legacy maps/assets merely to mark them deprecated;
- creating a second independent generated model for GALAXY/CLUSTER/SYSTEM.

## Historical full-scale finding

The historical implementation correctly kept important domain semantics: star radius relative to the Sun, planet radius relative to Earth, orbit distances in AU, and one attached hierarchy. It also mixed those semantics directly into rendering transforms: star and planet mesh coefficients, AU-to-centimetre offsets, and a global `FullScaleValue=1e9` actor scale coexisted. Literal restoration would reintroduce oversized bounds, LWC/float precision risk, culling failures and material instability.

Therefore the recommended option remains an incremental observed coordinate adapter, not a rollback. No such migration is authorized until the read-only Spike produces numeric A/B evidence and a separate decision.

## Required tests and gates

Existing guards that must remain green:

- generated handoff uses exactly one generator/galaxy/cluster/system/star hierarchy;
- generated route flags prove `bUseAuthoredSinglePlayWorld=false`;
- menu models are duplicated into independent GameInstance snapshots;
- preview generator is tagged and has legacy integration disabled;
- stable ID/HISM instance/materialized system anchor parity in rendered preview;
- physical materialized `AStar` is the gameplay stellar-light target; compressed HISM is fallback only.

Required additions before the architecture Story can close:

- source/functional dependency test proving generated gameplay does not enter `InitLegacyAuthoredGenerationLevel`;
- lightweight map-open smoke proving the frozen authored SinglePlay route still opens and uses only serialized actors;
- canonical manifest diff before/after generated handoff (IDs, counts, radii, parent IDs, orbits and transforms);
- same-seed GALAXY→CLUSTER→SYSTEM→STAR→PLANET→SYSTEM round trip with normalized direction/distance and selected-ID checks;
- resource-safe rendered A/B: current SYSTEM baseline and neighboring scopes, plus FPS/frame-time/RAM record.

## Resource protocol

Only one Editor/build/commandlet/render process may own the UE slot. Before every heavy run, record UE/UBT/ShaderCompileWorker processes, RAM/commit, VRAM and disk space. Stop rather than overlap an existing Editor or sustained shader compilation. Store logs/screenshots after each green checkpoint; never mass-delete DDC or assets without explicit approval.

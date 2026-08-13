# Visual Fidelity / Full-Scale Coherence Sprint

Status: diagnostic baseline, 2026-08-14. Full-scale coordinate/scaling implementation is on hold until the read-only audit is reviewed and an architecture option is explicitly accepted.

## Sprint Definition of Done

No visual or performance regression is acceptable. A pass is not accepted when it improves one scope while degrading PLANET, STAR, SYSTEM, CLUSTER, GALAXY, gameplay, transition identity, or the established performance baseline.

Every pass must record the same camera, exposure, resolution, quality preset, hardware, scene seed and IDs before and after the change. Evidence includes affected and adjacent scopes, a round-trip transition, average FPS, maximum frame time, and 1% low when the measurement harness supports it.

The current user baseline is approximately 120 FPS in Astronomical Generation with a system of about six planets and several moons, including tab switching. This is a hard non-regression gate. The current test machine reported NVIDIA GeForce RTX 5080, D3D12 SM6, driver 595.79. Automated captures currently use an approximately 876-pixel-wide viewport; exact viewport dimensions must be written beside every future artifact.

No compromise is accepted silently. A quality/performance or cross-scope tradeoff requires explicit approval.

## Accepted Baselines — Freeze

- The current SYSTEM layout, orbital distances, proportional composition and planet-to-planet navigation are accepted.
- PLANET to STAR and STAR to SYSTEM transitions are accepted as the bottom-up coherence baseline.
- The current surface generator's macro terrain forms, type/subtype direction, stable seed behavior and overall generation flow are accepted.
- Stable entity IDs, seeds, hierarchy records, save/runtime links and gameplay handoff remain authoritative.
- Existing generation counts, probability distributions and canonical positions must not be changed by a visual-fidelity pass.
- The current gameplay handoff regression for logical system-envelope collision is retained: giant presentation/query envelopes must remain non-blocking.

## P0 Findings

### 1. Current physical data and presentation are mixed, but physical semantics still exist

Historical and current code retain the domain conversions:

- one light year = approximately 9.461e12 km;
- one astronomical unit is materialized as approximately 1.496e13 cm before hierarchy scaling;
- stellar radius uses 696,340 km per solar radius and the historical sphere-scale coefficient 813,684,224;
- planet radius uses the Earth-diameter scale coefficient 12,742,000;
- galaxy positions historically used `LightYearInKm / UnitInKm / AstroScaleCoefficient`.

The current system also retains stable catalog records and IDs (`FGalaxyCatalogStarRecord`, `FClusterStarSystemRecord`, `FStarSystemModel`). However, canonical physical coordinates and presentation/world transforms are not yet separated by one explicit coordinate service. Large `FVector` transforms, hierarchy root scaling, preview normalization, actor-relative relocation and visual compensation coexist in `AstroGenerator`.

This audit therefore rejects both a blind rollback and a second parallel generator.

### 2. Historical full-scale path: useful domain model, unsafe literal rendering strategy

Useful semantics to preserve:

- real radii, AU/light-year distances and orbital ordering;
- one generated hierarchy and one identity chain across GALAXY/CLUSTER/SYSTEM/gameplay;
- relative placement and deterministic seed/ID mapping;
- physical values remain independent of visual readability compensation.

Unsafe techniques not to restore literally:

- very large float actor transforms and bounds;
- root-scale toggles around 1e9–1e10 as the only precision solution;
- camera, culling and material behavior derived from giant world transforms;
- independent per-scope normalization without an explicit canonical-to-presentation adapter.

Those techniques caused precision, bounds, culling, collision and material/LOD instability despite preserving the intended scale ratios.

### 3. Main stellar visual regression is not caused only by lost physical scale

The first strong material transition is the programmatic stellar-master rebuild in commit `8cc34274` (`fix: unify astronomical preview presentation and complete material graph fidelity`). It converted the generated stellar masters to an Opaque/Unlit unified shader and encoded rim/corona energy primarily inside the sphere silhouette. The physical scale path still existed, but the old HDR-core plus broad bloom appearance no longer followed from the material and post-process chain.

The current tree has improved this architecture with a detailed opaque photosphere and a separate additive corona/point representation. The preview camera uses SOG bloom intensity 1.35 with threshold -1. Current corona presentation is separate from the photosphere, but its screen profile still requires rendered acceptance rather than scalar-contract acceptance.

Consequently the star regression is split into three independent causes:

1. physical-to-presentation angular size;
2. photosphere/corona HDR distribution and bloom;
3. scope-specific LOD/HISM representation.

None of these may be fixed by moving or regenerating canonical entities.

### 4. Secondary stars in SYSTEM use the same actor/material path but lose readability in sizing

All materialized stars use `AStar`, `ApplyModel`, the same physical radius scale, spectral material and corona component. The current SYSTEM presentation computes the largest presented star and then applies an approximately linear secondary ratio:

`SecondaryPresentedRadius = LargestPresentedRadius * (SecondaryPhysicalRadiusKm / LargestPhysicalRadiusKm)`

It then caps the result against orbit/envelope safety. A secondary with 0.1 of the physical radius therefore receives roughly 0.1 of the primary's screen radius and can become planet-like or point-like. This is not a missing material path and is not permission to make all stars equal.

Future acceptance requires a monotonic readability transform that preserves radius ordering and stellar class differences while keeping even the smallest materialized star recognizably stellar. It must not change positions, SYSTEM layout or orbital distances. No implementation is authorized until a same-camera multi-star baseline records physical radius, type, actor bound, presented radius and measured pixel radius for every star.

### 5. SYSTEM to CLUSTER must prove identity before any point-size retune

The observed regression is not merely that CLUSTER points are the wrong size. On SYSTEM to CLUSTER, stars can appear much larger/closer while the selected/home-system marker has no clearly visible corresponding object, creating the impression of a different map.

The diagnostic gate is a same-seed round trip:

`PLANET -> STAR -> SYSTEM -> CLUSTER -> SYSTEM`

For the selected system it must log and preserve:

- stable system ID and seed;
- cluster record index;
- HISM instance index or materialized actor mapping;
- cached base transform and presentation transform;
- selected marker world and screen anchors;
- normalized pairwise distances to deterministic neighboring records;
- materialize/dematerialize restoration.

`PreviewBackgroundVisualScale`, `PreviewBackgroundDistanceScale`, angular floors/caps, camera/FOV and framing are audited only after this identity mapping passes. Absence of the selected entity must not be hidden by increasing every point.

### 6. Planet fidelity regression contains separate geometry and material components

The historical full-scale WorldScape configuration used approximately `LodResolution=200` and `TriangleSize=75`. The current full-scale path uses approximately `LodResolution=96` and `TriangleSize=120`; the persistent selected PLANET presentation uses a related 10/96 configuration. This is roughly 4.3 times fewer grid vertices per patch and 1.6 times wider triangle spacing than the historical reference.

At the same time, the scaled orbital-preview path deliberately disables near/macro/meso color, detail normal and detail roughness strengths to avoid clipmap seams. That explains why some menu views look low-resolution even when the canonical physical terrain contains stronger macro/mid/micro material detail.

The accepted macro terrain generator is not to be rewritten. Geometry resolution and material richness are tested independently:

1. current 96 grid plus material-only detail;
2. profiled 128/160 grid candidates with the same material;
3. same-camera physical gameplay capture;
4. cold-build time, steady frame time, memory and transition hitch comparison.

Restoring 200 blindly is rejected because it risks the accepted 120 FPS baseline.

### 7. Atmosphere is a separate unaccepted P0/P1 visual path

The old implementation was not a sufficiently physical reference: it derived broad opacity/scale terms from radius but randomized scattering colors. The newer implementation is more deterministic and derives bounded Rayleigh/Mie/opacity/multi-scattering/airglow values from surface profile inputs. The preview path also suppresses duplicate shells and uses a local directional branch to avoid large-coordinate precision loss.

The remaining visual regression is real: pale flat fill, weak or inconsistent limb, insufficient terminator scattering and occasional bad color/contrast. The first major presentation transition appeared with the preview atmosphere fields and reduced sampling/light/opacity compensation around commit `8cc34274`; the later robust shell stabilization in `d35ef743` improved ownership and precision but did not by itself provide final visual fidelity.

No cosmetic independent preset is accepted. Atmosphere must deterministically resolve from canonical planet data: type/subtype, composition, pressure/density, temperature, humidity/cloud state, radius and gravity. Current data covers only part of that set, which is an explicit model gap.

Acceptance requires same-camera inside/outside and PLANET/gameplay captures for multiple planetary archetypes, including terminator, limb thickness, exposure, sun intensity, depth/clipping and WorldScape interaction. A contract-only pass is insufficient.

## Recommended Architecture Option — Pending Explicit Decision

### Option A: restore literal historical giant transforms

Rejected. It restores some scale ratios but also restores precision, bounds, culling, collision and material instability.

### Option B: retain ad-hoc per-scope normalization as the permanent architecture

Rejected as the long-term solution. It is low-cost but cannot reliably prove identity/coherence and encourages scope-specific drift.

### Option C: canonical double-precision hierarchical frames plus an explicit presentation adapter

Recommended, but not yet authorized for implementation.

- Canonical model: real km/AU/light-year values, stable IDs/seeds and parent-relative double-precision coordinates.
- Hierarchical frame: galaxy, cluster, system and local-body origins; no need to place all canonical magnitudes directly into one UE transform.
- Presentation adapter: deterministic canonical-to-local transform per scope, preserving direction, ordering, normalized pairwise distances and selected anchor.
- Visual LOD adapter: separate angular-size/readability compensation, never written back into physical radius, orbit or canonical position.
- Gameplay: SYSTEM is the bottom-up source frame; higher scopes are representations of the same records, not regenerated layouts.
- LWC/origin strategy: local frame/rebasing only where measured precision requires it; no broad root-scale toggle as domain state.

The migration must be incremental and keep the current SYSTEM baseline intact. The first implementation milestone would be an observation-only coordinate/identity report, not transform replacement.

## Ordered Sprint Map

### P0.0 — Evidence and immutable baseline

- Freeze current SYSTEM and transition screenshots.
- Record seed, stable IDs, radii, transforms, FOV/exposure, viewport and performance.
- Add all-scope round-trip identity telemetry without changing transforms.
- Record user baseline: ~120 FPS with ~6 planets and several moons.

### P0.1 — Secondary/multiple-star readability

- Same-camera multi-star baseline.
- One bounded monotonic presentation-only sizing change after review.
- Preserve physical ordering, positions, SYSTEM layout and all material paths.
- A/B STAR and SYSTEM plus adjacent PLANET and CLUSTER captures.

### P0.2 — HDR photosphere/corona

- Measure disc center, limb, exterior halo, clipping and bloom profile.
- Change one causal energy/profile layer at a time.
- Preserve HISM point path and accepted actor sizes unless separately approved.

### P0.3 — SYSTEM/CLUSTER identity coherence

- Prove selected marker and visible instance map to one canonical record.
- Compare normalized topology before any scale/cap retune.
- Then tune only presentation distance/angular-size adapters.

### P1.0 — Planet geometric and material fidelity

- Material-only high-frequency pass at current grid.
- Profiled grid-resolution experiment.
- Keep macro terrain, subtype diversity and water separation.
- Reject any variant that misses the accepted performance gate.

### P1.1 — Living worlds, water and atmosphere

- Physical gameplay captures are authoritative.
- Living-world land generation removes crater-like/nonliving cues where the canonical profile does not request them.
- Water remains an equipotential smooth shell with physically plausible optical response; nonliving liquids remain separate.
- Atmosphere uses canonical deterministic inputs and multi-scope rendered gates.

### P2 — Galaxy/Cluster/System stellar LOD and performance

- One identity-preserving catalog/actor representation chain.
- Angular-size and HDR readability without blobs or invisible sub-pixel points.
- No count, distribution, seed or canonical-position changes.
- Profile steady-state, cold materialization, memory, streaming and transition hitch.

## Required Artifact Set for Every Accepted Pass

- `Baseline_<seed>_<scope>_<resolution>.png`
- `After_<seed>_<scope>_<resolution>.png`
- exact camera transform, FOV, exposure and quality settings;
- entity ID/seed and physical/presentation metrics;
- affected and adjacent scope captures;
- round-trip identity log;
- average FPS, max frame time and 1% low where available;
- compiler and focused automation result;
- explicit note of any unresolved compromise.

## Current Evidence

- User SYSTEM and planet baselines: screenshots dated 2026-08-13/14 supplied in the active task.
- Historical stellar/planet references: 2023 Unreal screenshots and user-supplied 2026 reference photos in the active task.
- Current rendered preview logs include `CodexAstronomicalPreview_GasGiant_20260814.log` and related main-menu captures.
- Current gameplay handoff after non-blocking envelope fix: `CodexGameplayHandoff_Audit_Zones_20260813.log`, PASS.
- Current source checkpoint: `06179080`, branch `codex/fullscale-gameplay-sprint`, with a large intentional dirty visual-integration tree.

## Jira Mapping

Jira keys are pending because the Jira task is waiting for the Atlassian connector/plugin. Until keys arrive, this document is the local scope ledger. Once available, Jira sprint issues become the status/dependency/acceptance source; newly discovered regressions are recorded there before scope expansion.

## Hold Point

No full-scale coordinate/scaling architecture changes are authorized by this document. The next decision is whether to proceed with Option C as an incremental observation-first migration. Presentation-only visual work may continue only when it preserves canonical data, the accepted SYSTEM baseline, identity round trips and the performance Definition of Done above.


## 2026-08-14 Ledger Addendum

Architecture guardrail: see `Docs/ADR_EVOLUTION_OVER_REWRITE.md`. The authored SinglePlay level and legacy assets are frozen references; newly generated worlds continue through the canonical model snapshot and isolated runtime generator. No full-scale coordinate migration is authorized yet.

### Newly accepted baselines

- SYSTEM layout, orbital composition, planet-to-planet navigation and smooth scope transitions remain frozen.
- Menu/orbital performance: approximately 119.7-120.6 FPS, 8.3-8.4 ms, approximately 6.37 GB in the captured editor configuration.
- Surface gameplay provisional baseline: 89.2 FPS, 11.2 ms, approximately 2.53 GB; landing, gravity, character lighting and shadow work. Existing hills/relief geometry is preserved. The surface task is readability/material fidelity, not relief generation from zero.
- Ocean/living-water material and LAND COVERAGE response remain accepted baselines.
- Hypergiant type-to-size response is positive partial evidence, not final acceptance. Required fixed-camera matrix: Dwarf/Main Sequence/Giant/Hypergiant with physical radius, presented radius, corona/light bounds, orbit clearance, reverse-selection and gameplay handoff.

### Current P0 checkpoints

- OVERVIEW no longer presents the materialized home-star sphere; distant scopes use the exact stable-ID HISM proxy. Rendered identity assertions bind record stable ID, instance index and world anchor.
- Gameplay stellar lighting now prioritizes a materialized physical `AStar`. Compressed cluster HISM records are a fallback only. The rendered handoff gate requires the active target location and actor path to equal the primary star.
- SYSTEM presentation positions, orbit layout, star/planet data and cluster density were not modified by either checkpoint.

### Coordinated backlog hierarchy (keys pending)

- Full-Scale & Hierarchical Coherence
  - Spike: literal vs uniform interstellar compression vs piecewise/hybrid mapping (read-only until decision)
  - Bug: SYSTEM-to-CLUSTER stable identity/anchor and round-trip parity
  - Bug: generated gameplay handoff manifest and stellar lighting parity
  - Bug: Ring/Arc seam continuity; preserve accepted morphology/density
  - Architecture Story: Evolution over rewrite / legacy runtime isolation
- Stellar Visual Fidelity
  - secondary/multiple-star readable physical sizing
  - HDR photosphere/corona and HISM compact point profile without square/blob clipping
  - fixed-camera type/radius matrix and dense-overdraw gates
- Planet Surface Generation / Materials
  - selected-planet WorldScape anchor fidelity
  - Living World terrain polish (Enhancement; ocean frozen)
  - Relief and Feature Scale data-path/calibration
  - Desert Worlds Rework and Frozen Worlds Diversity Rework
  - Generator taxonomy/display labels without enum/asset migration
- Planet Surface Gameplay & Ground Fidelity
  - Surface relief readability/material fidelity (Dev Surface owner)
  - Biosphere-driven foliage and surface scatter: Dev 2 owns canonical biosphere/biome data; Dev Surface owns local realization
  - modern layered ground-generation design spike; no claim that a completed legacy ground generator existed
- Orbital Systems
  - Moon Orbit Distance editing, Roche/Hill/neighbor bounds, deterministic persistence
- Gas Giants Visual & Generation Rework
  - Bug: generated path does not reliably present the intended reference mesh/setup
  - Bug: single repetitive material
  - Bug: incorrect atmosphere/cloud-envelope scale
  - Story: seed-driven material profiles, bands/storms/subtypes and preview/gameplay parity
- UI/UX
  - separate Epic/sprint only; no mass redesign during P0 scaling/material work

### Resource gate

Do not start a build, commandlet or rendered automation while an Editor or ShaderCompileWorker owns the UE slot. Record RAM/commit, VRAM and disk before every heavy run. One heavy process at a time; preserve logs/screenshots after each green checkpoint.

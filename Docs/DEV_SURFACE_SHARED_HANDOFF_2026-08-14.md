# Dev Surface shared handoff — 2026-08-14

## Ownership

- Dev 2 owns canonical planet ID, seed, type/subtype, radius, atmosphere, biosphere and orbital biome representation.
- Dev Surface owns ground realization: local landscape fidelity, material, scatter/foliage, collision/walkability and anchor-local streaming.
- Shared contracts are model/seed identity, biome coverage, WorldScape coordinate mapping, save/handoff and performance/resource gates.
- Do not edit legacy authored maps as a generator source. No completed legacy ground generator is assumed to exist.

## Accepted ground baseline

- Landing, gravity, player movement, character lighting and cast shadow work.
- Existing macro hills/relief geometry is real and must remain the same seed/shape during material-readability work.
- Provisional captured performance: 89.2 FPS / 11.2 ms, approximately 2.53 GB. Record resolution/quality/hardware with the next formal capture.
- The task is Surface Relief Readability & Material Fidelity, not generating relief from zero.

## Bug: surface material crawls/clips while the player moves

User observation: ground appearance slides, crawls, clips, switches or oscillates relative to terrain/camera while walking/running.

Do not carry this into a new material. Separate these causes before visual rework:

1. camera-relative versus absolute world-position mapping and actor/root centre parameters;
2. LWC/origin rebasing and float precision at the active patch;
3. WorldScape LOD morphing, section/patch coordinate discontinuity and anchor updates;
4. triplanar/world-aligned projection scale/rotation consistency across components;
5. texture streaming/mip or VT residency transitions;
6. TSR/TAA shimmer versus true UV/material motion;
7. duplicate/coplanan terrain shells and z-fighting;
8. normals/tangents recomputation or stale dynamic material parameters following camera/anchor.

Required diagnostic sequence:

- same location: stationary, walk, sprint, turn, cross one patch/LOD boundary;
- capture Lit, Unlit and Wireframe; diagnostic no-temporal-upscaling run only for classification;
- temporary debug grid encodes stable world/root coordinates to distinguish geometry motion from projection motion;
- log active WorldScape anchor, root/patch transforms, LOD index, material interface and relevant parameter values;
- verify exactly one authoritative terrain renderer at the sampled pixel/position.

Acceptance:

- material coordinates remain locked to the surface under camera/player movement;
- no seam/pop/crawl at patch and LOD transitions;
- close/mid detail remains stable after texture residency settles;
- collision and rendered terrain agree;
- same planet seed/biome before and after;
- no performance regression from the provisional surface baseline.

## New capability: biosphere-driven foliage and scatter

Foliage is not an independent random decorator. It consumes the canonical biosphere/biome contract: habitability, atmosphere/composition, temperature, moisture, latitude, altitude, slope, subtype, water distance and seed. Orbital biome coverage and ground placement must share that contract.

Dev Surface may design a modern layered pipeline (macro planet, regional meso, anchor-local micro, biosphere, gameplay, streaming/LOD). Preserve the working hills/landing/gravity and incrementally vertical-slice one accepted Living/Ocean seed. Use deterministic streamed cells and data-driven soft-referenced presets; vegetation, rocks/debris and gameplay collision/nav budgets remain separate.

## Resource coordination

- Never overlap Editor, build, commandlet, shader compile or rendered automation with another heavy owner.
- Preflight processes, RAM/commit, VRAM and disk.
- Save logs, screenshots and small checkpoints after each green stage.
- Dev Surface must coordinate shared WorldScape files before editing them.

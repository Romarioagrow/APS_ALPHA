# APOSFERA UI/UX Epic — read-only audit and ownership map

Date: 2026-08-14  
Mode: documentation and mockups only  
Runtime implementation status: on hold pending user choice and an explicit ownership handoff from Dev 2

## Outcome

The current interface already has the right structural direction. The recommended pass is an evolution of the existing three-column astronomical workstation, not a replacement:

- preserve the live level viewport and its transparent root so UI work cannot change scene exposure;
- preserve the current 25 / 50 / 25 controls-preview-context flow;
- preserve Orbitron for brand and compact technical headings, while keeping a neutral Unreal UI face for generated names, descriptions and metadata;
- preserve the direct-position slider plus exact spin-box input;
- refine Live Model into scannable facts, strengthen Planet & Satellites hierarchy rows, and surface colonization readiness before the continuation action;
- reuse the same chrome tokens in Strategic Map, Civilization and the gameplay menu after ownership is agreed.

No `Source`, `Content` or `Config` file was edited for this audit. No Unreal, build, commandlet, shader or rendered automation process was started.

## Current UI inventory

| Surface | Current implementation | Observed flow | Audit disposition |
|---|---|---|---|
| Landing and path selection | `Source/APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.*` | Landing menu, path cards, world collection, settings/profile entry | Active Dev 2 work; preserve and do not edit |
| Astronomical generation workstation | `Source/APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.*` | Left scoped controls, center live preview and scope bar, right Current Scope / Live Model / hierarchy, footer commit action | Active Dev 2 work; use as the visual baseline |
| Generation model and handoff | `WorldGenerationViewModel.*`, `SpawnClassPicker.cpp` | Transient preview model, scope focus, selected body, generated gameplay handoff | Active Dev 2 work; UI changes must not alter model semantics |
| Strategic map | `Source/APS_ALPHA/UI/StrategicMap/SAPSStrategicMapPanel.*` | F10 overlay, hierarchy focus buttons, live world summary, mouse orbit/zoom | Clean runtime candidate for a later Dev UI-owned slice |
| Civilization shell | `Source/APS_ALPHA/UI/CivilizationMenu/*`, `Content/APS/APS_ALPHA/UI/CivilizationMenu/*` | Overview, Divisions and Division Details through a UMG switcher | Runtime files are clean but Content editing needs ownership and the UE slot |
| Gameplay / pause / settings | `Content/APS/APS_ALPHA/UI/MainMenu/WBP_PauseMenu.uasset`, `WBP_SettingsPanel.uasset`, `Content/APS/APS_APOSFERA_SPACETRIPS/MBLS/Widgets/General/APS_WB_GameMenu1.uasset` | Existing authored menu assets; exact Blueprint flow must be inspected in Editor later | Frozen reference until explicit Content ownership and a free UE slot |
| HUD | `Content/APS/APS_ALPHA/UI/HUD/*` | Status card, menu button, interaction HUD and current-status UI | Inventory only; no Content changes |
| Legacy generation widgets | `GenerationSlider.*`, `GenerationInput.*`, `AstroGenerationMenu.*`, `USelectWorldsMenu.*` plus GenerationMenu assets | Older UMG path and reusable bindings | Reference only; do not reconnect or rewrite during this pass |
| Typography assets | `Content/APS/APS_ALPHA/UI/Fonts/*` | Orbitron family plus several historical display faces | Orbitron remains the APOSFERA display face; body copy stays neutral |

## Shared-checkout ownership map

| File or area | Owner for this checkpoint | Rule |
|---|---|---|
| `Source/APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.cpp/.h` | Dev 2 / locked | Dirty with a large active patch; Dev UI must not edit |
| `Source/APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.cpp` | Dev 2 / locked | Dirty and coupled to active planet/habitability work; Dev UI must not edit |
| `WorldGenerationViewModel.cpp/.h`, `SpawnClassPicker.cpp` | Dev 2 / locked | Dirty model/handoff path; visual work must not enter it |
| `Content/**`, `Config/**`, other dirty `Source/**` | Existing owner / locked | No edits, resaves, moves or cleanup |
| `Source/APS_ALPHA/UI/StrategicMap/**` | Unassigned runtime candidate | Read-only until Dev 2 confirms no overlap and the user selects a direction |
| `Source/APS_ALPHA/UI/CivilizationMenu/**` | Unassigned runtime candidate | Read-only until a distinct slice is agreed |
| UI UMG assets | Unassigned Content candidate | Requires explicit ownership and a free Editor slot; never edit while another Editor is active |
| `Docs/UI_UX_APOSFERA/**` | Dev UI | Safe documentation, token and mockup ownership |
| Thread-scoped Codex visualization directory | Dev UI | Safe interactive comparison surface; not runtime code |

## Existing strengths to preserve

1. `SWorldGenerationPanel` deliberately keeps its root transparent. This prevents Slate chrome from washing the real astronomical viewport or hiding small galaxy/cluster points.
2. `Font()` loads Orbitron Bold/Medium for branding and compact technical labels, while `ReadableFont()` uses the neutral engine UI font for descriptions and generated content.
3. `SAPSGenerationRangeSlider` uses direct-position interaction and refreshes the bound Slate attribute immediately. Exact input remains available in the adjacent spin box.
4. The right panel already distinguishes Current Scope, Live Model and the Planet & Satellites hierarchy.
5. Hierarchy rows already encode indentation, body glyph, child count, selection and focus action.
6. Amber is already reserved for primary/selected emphasis while cyan describes navigation and live technical state.

## Main UX findings

### Palette and display calibration

- The same cyan/amber language is duplicated in Main Menu, Generation and Strategic Map rather than sourced from one UI style contract.
- Raw chrome values are extremely dark. This is cinematic, but it makes panel separation and secondary text depend heavily on monitor black level and room lighting.
- Gamma calibration must apply to UI chrome only. It must not change scene exposure, post-process settings, preview materials or the transparent center viewport.
- Three documented calibration profiles are proposed in `APOSFERA_UI_TOKENS.json`; `balanced_2_2` is the recommended default.

### Typography and spacing

- The current display/body split is correct and should become an explicit rule.
- Several metadata strings use sizes 8–10. The target floor is 11 for nonessential microcopy and 12 for persistent metadata at 1080p before application scaling.
- Body copy should not use wide tracking or all caps. Uppercase Orbitron stays with brand, section headings, short labels and command actions.
- Existing ad-hoc spacing values should converge on a 4 px base scale without changing the overall 25 / 50 / 25 composition.

### Live Model

- The current scope summary is accurate but paragraph-shaped. Stable, high-value fields should become 2–4 compact fact chips before the hierarchy: stable ID, habitability, radius/class and child count.
- A chip is display-only; it must never become a hidden filter or ambiguous button.
- Generated names and values use the readable body face and remain copyable/readable without relying on cyan alone.

### Planet & Satellites hierarchy

- Keep the existing tree, indentation and selected amber rail.
- Increase row target height to 44 px, retain two text lines only where details materially help, and keep child count as a neutral count with a visible label for assistive text.
- Selection must pair amber with a left rail and a textual `SELECTED` state. Hover, keyboard focus and selection are separate states.
- Parent/system rows stay in context; focusing a moon must never discard its planet/system relationship.

### Sliders and exact values

- Preserve slider plus spin box. The slider handles exploration; the spin box handles exact values and units.
- Provide a 32 px minimum slider hit region inside a 44 px control row, a visible 4 px track, a 20 px handle and keyboard/controller steps.
- Show label, current value and unit in one header line. Min/max may be secondary edge labels when the range is not obvious.
- Reset/default is allowed only for controls with a canonical default and must not be invented for seed/randomized values.

### Colonization readability

- The continuation path should expose readiness before committing: `READY`, `CONDITIONAL` or `BLOCKED`, always paired with text and an icon/shape rather than color alone.
- Four concise dimensions are enough for the first pass: atmosphere, temperature, gravity and water/resources. Detailed simulation remains in the Civilization screens.
- Blockers and mitigations must be sentences, not isolated red numbers. The primary action states the destination and selected body.

### Gameplay menu

- Evolve the existing Pause/Settings assets into the same APOSFERA shell; do not replace their gameplay bindings.
- Preserve a short primary path: Resume, Save, Settings, Return to Main Menu.
- Put run/world context beside the actions, not in a new dashboard. Destructive navigation needs a confirm step and must never share the primary amber treatment with Resume.

## Proposed implementation slices after selection

1. **Style contract:** add shared colors, typography and geometry in new non-model UI style files; migrate only a separately owned surface at a time.
2. **Generation polish:** after Dev 2 hands off MainMenu ownership, replace duplicated literals, refine fact chips/hierarchy rows and slider geometry without changing model delegates or preview behavior.
3. **Colonization handoff:** add a read-only readiness presentation fed by existing canonical habitability data; no new simulation rules in UI.
4. **Strategic/gameplay shell:** apply the same tokens to Strategic Map and the existing pause/settings flow in files not owned by Dev 2.
5. **Content pass:** only with an exclusive Editor slot, update UMG styles and navigation metadata; preserve Blueprint bindings and authored assets.

Each slice is independently reviewable and must keep the current workflow functional.

## User decisions requested before runtime work

1. Default calibration: `balanced_2_2` (recommended), `cinematic_2_4`, or `bright_2_0`.
2. Density: comfortable 44 px rows (recommended) or compact 38–40 px rows.
3. Colonization readiness: persistent compact block in the right context panel (recommended) or a confirmation step after Continue.
4. Gameplay menu: left action rail with contextual detail (recommended) or the current centered stack with token-only polish.

## Resource checkpoint

At audit time an existing `UnrealEditor` process was active with approximately 2.2 GB working set, plus a small MSBuild process. No heavy process was launched and no cache/editor state was touched.


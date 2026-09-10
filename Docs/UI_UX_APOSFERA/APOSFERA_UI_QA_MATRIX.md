# APOSFERA UI accessibility and QA matrix

Status: design acceptance gates; runtime execution deferred until ownership is assigned and the UE slot is free.

## Static checks completed in this audit

- Confirmed the generation root is intentionally transparent over the real level viewport.
- Confirmed the current display/body typography split.
- Confirmed slider and exact spin-box input are separate but bound to the same value.
- Confirmed hierarchy rows include depth, glyph, child count, focus action and selected state.
- Confirmed MainMenu runtime files are dirty and therefore owned by Dev 2 for this checkpoint.
- Confirmed an existing Unreal Editor process was active; no heavy validation was started.

## Recommended palette contrast sample

The following ratios use the proposed `balanced_2_2` sRGB values. They are design references and must be rechecked against the actual Slate render and opacity-composited scene.

| Foreground / opaque surface | Contrast ratio | Gate |
|---|---:|---|
| Primary text / panel | 16.59:1 | Pass AAA |
| Secondary text / panel | 9.64:1 | Pass AAA |
| Focus cyan / panel | 10.26:1 | Pass AAA |
| Action amber / panel | 8.57:1 | Pass AAA |
| Success / panel | 11.28:1 | Pass AAA |
| Quiet border / panel | 3.18:1 | Pass non-text UI boundary; do not use as text |

## Acceptance matrix

| Area | Scenario | Pass condition |
|---|---|---|
| Palette | Balanced 2.2, Cinematic 2.4, Bright 2.0 | Chrome remains legible; center viewport exposure and material appearance do not change |
| Transparency | Galaxy/cluster points behind UI | Transparent center/root never adds a wash or vignette over the preview |
| Text | 1280×720 through 3840×2160 with application scale | Persistent body/metadata meets the size floor; no overlap, clipping or false ellipsis |
| Localization | Long generated names and 30–50% expanded labels | Body face is used; critical names have tooltip/full text; action meaning remains visible |
| Keyboard | Tab/Shift+Tab, arrows, Enter/Space, Escape | Logical order follows left controls → preview scopes → right context → commit; focus never disappears |
| Controller | D-pad/stick navigation and face-button activation | Every interactive target is reachable; slider adjustment is stepped and reversible |
| Mouse | Sliders, hierarchy, preview orbit/zoom | Slider hit target is comfortable; hierarchy selection does not trigger accidental preview orbit |
| Focus | Hover, keyboard focus, selected and disabled states | States are visually distinct and selection does not rely on hover color |
| Color vision | Protan/deutan/tritan/grayscale review | Ready/warn/block, selected and primary action retain text/shape cues without hue |
| Motion | Preview transitions and animated chrome | Reduced-motion mode removes decorative motion; no looping UI animation is required to read state |
| Live Model | Missing, loading, valid and stale data | Explicit text state replaces blank chips; stable ID and units never masquerade as actions |
| Hierarchy | 0, 1, 10+ moons; long labels; disabled body | Parent relationship stays visible; count is labeled; selected row remains in view |
| Sliders | Min, max, fractional step, typed exact value | Handle, numeric value and model value agree in the same frame; units and limits are clear |
| Colonization | Ready, conditional and blocked | Status includes a word plus cause/mitigation; blocked cannot be confused with disabled loading |
| Gameplay menu | Resume, save, settings, return | Resume is the primary path; destructive navigation is separated and confirmed |
| Error handling | Preview unavailable or commit disabled | Reason is visible near the affected action; color alone is insufficient |
| Performance | Live hierarchy refresh and slider drag | No avoidable per-frame widget rebuild/allocation is introduced; focused profiling occurs only in an exclusive UE slot |

## Runtime validation sequence after implementation

1. Source-level focused compile/test for the separately owned UI slice.
2. Lightweight automation for widget construction, focus order, values and accessible state labels.
3. Exclusive Editor smoke at 1280×720 and 1920×1080 for all three calibration profiles.
4. 2560×1440, ultrawide and 4K application-scale checks.
5. Same-seed generation round trip to prove UI changes do not reroll or mutate canonical model data.
6. Captures of Generation, Colonization, Strategic Map and Gameplay menu with FPS/frame time/RAM recorded.

Do not run steps 1–6 while another Editor, build, commandlet or sustained ShaderCompileWorker owns the UE slot.


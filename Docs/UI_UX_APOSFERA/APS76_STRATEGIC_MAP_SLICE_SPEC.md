# APS-76 Strategic Map presentation slice

This note preserves the approved narrow integration boundary while the built-in
patch tool cannot read the existing panel source. It is an implementation aid,
not evidence of runtime completion.

## Ownership and baseline

- Approved file: `Source/APS_ALPHA/UI/StrategicMap/SAPSStrategicMapPanel.cpp`.
- `SWorldGenerationPanel.cpp` remains on HOLD.
- Baseline SHA-256: `8ab1e00b12233cd4972cc2306609ba568bd454f064137b15b36142bb014f0518`.
- Baseline git blob: `a1a729a50fdd146d6ea4b50f2543f025880562ee`.
- Baseline length: 178 source lines.

## Presentation mapping

1. Keep the root transparent so the astronomical viewport is never calibrated
   or dimmed by the UI display profile.
2. Use `FAPSUIStyle::GetRecommendedDisplayProfile()`, shared typography and the
   4 px spacing scale for header and side-panel chrome only.
3. Preserve the existing three-zone flow: hierarchy on the left, unobstructed
   viewport interaction in the center, canonical Live Model facts on the right.
4. Replace the six focus buttons with `SAPSUIHierarchyCard` instances using the
   same `EAstroPreviewFocus` values and the existing `Focus(uint8)` delegate.
5. Present home star, spectrum/temperature, home planet/radius, planet/moon
   counts and cluster star count through `SAPSUIFactChip` attributes read from
   the existing `UGeneratedWorld`. Do not cache or copy model state.
6. Present close/return through `SAPSUIActionButton`, bound to the existing
   `Close()` method. Keep both F10 and Escape behavior.
7. Add visible `READ-ONLY` wording. Do not expose generation controls or mutate
   the canonical model.

## Frozen behavioral region

The implementations of the following methods must remain unchanged in the
presentation commit:

- `OnKeyDown`
- `OnMouseButtonDown`
- `OnMouseButtonUp`
- `OnMouseCaptureLost`
- `OnMouseMove`
- `OnMouseWheel`
- `OnMouseButtonDoubleClick`
- `Close`
- `Focus`
- `FocusUnderCursor`
- `GetWorldSummary`

No controller, generator, camera, focus/orbit/zoom, return delegate, save,
Content or canonical-data file belongs in the slice.

## Verification gate

- Source diff contains only the approved panel cpp plus this UI-owned evidence.
- Frozen behavioral methods are identical before and after the presentation diff.
- Root/background source contains no full-viewport opaque or translucent fill.
- Existing hierarchy focus enum values remain one-to-one.
- Build and `APS.UI.*` automation run only in an explicitly reserved heavy slot.
- Visual PASS requires same-camera Strategic Map A/B plus keyboard focus, 1280 px
  narrow-layout and reduced-motion checks. Compile-only remains PARTIAL.

# APS-76 Dev UI checkpoint — reusable presentation components

Date: 2026-08-14
Parent: APS-8
Jira: APS-76 / In Progress

## Done

- Added `SAPSUIFactChip` for compact canonical Live Model label/value facts.
- Added `SAPSUIHierarchyCard` with depth, selected rail, two-line body metadata, child count and a 44 px minimum target.
- Added `SAPSUIReadinessRow` for Colonization/gameplay readiness with explicit status text plus semantic color.
- Kept all components presentation-only. They accept text/attributes/events and do not own ViewModel, camera, save, placement, spawn or civilization state.
- Refreshed the interactive direction mockup with Generation, Colonization and gameplay-menu screens plus three chrome calibration profiles.

## Checked

- Mockup static validation: PASS; 22,533 bytes, one local script, three mapped screens, seven mapped annotations, no network API or document wrapper.
- UE 5.7 local header audit covers the Slate widgets, button delegate, rounded brushes and style setters used by these components.
- Dev 2, Dev Surface and Dev Civilization each returned FREE for the two panel files; no runtime/shared binding scope was granted or requested.

## Result

`PARTIAL`: reusable runtime presentation pieces are ready. Existing panel integration is still deferred because the safe patch mechanism cannot read already-existing files under the current Windows ACL. No shell rewrite was used. Visual browser QA and same-camera A/B remain pending; no UE/build/render/shader process was started.

## Artifacts

- `Source/APS_ALPHA/UI/Style/APSUIComponents.h`
- `Source/APS_ALPHA/UI/Style/APSUIComponents.cpp`
- `aposfera-ui-direction-v2.html` in the Dev UI visualization workspace

# Accepted native-resolution point rendering — intermediate checkpoint

The user confirmed substantially rounder star points and reduced individual-point
shimmer in gameplay. A remaining scene/aggregate brightness increase after camera
motion stops is NOT resolved by this checkpoint. Menu rounding still needs its
own verification; the user reports menu brightness was already stable.

Saved change:

- Dedicated POINTS pass at native output resolution, instead of expanding its
  lower-resolution raster. Opaque scene TSR is not disabled or reconfigured.
- Actual raster clip coordinates interpolated from the vertex stage; optical
  point projection and scene-depth occlusion use the appropriate raster frame.
- Accepted colour/energy/halo formulas and original instance transforms remain.
  No new WPO, minimum-size filter, geometry padding or catalogue edits.
- VSync enabled in both game and editor at the user's explicit request.

The live POINTS material was saved through the editor, matched to its C++ recipe,
and compiled to356 pixel-shader instructions. Source and regression-test
compilation succeeded without linking/replacing the running editor DLL. The
updated native automation test has NOT yet been executed in a newly linked build.

Physical HISM corona and SUN material assets are not modified by this change.
The earlier spatial-filter experiment remains separately archived and inactive.

POINTS shader SHA256:
`d59f254c099794af0e6fab6750ba73405d95c2fa3fd8e087a2b8d2fcf4cb54c8`

User video: Unreal Engine 5 2026.09.12 - 03.47.55.03.mp4, about47seconds.
Video inspection must account for large changes of view content (a bright local
star enters/leaves view); whole-frame average brightness alone is not proof of
an exposure defect. Remaining goal requires matched camera-motion comparisons.

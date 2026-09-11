# Stellar rendering checkpoint — 12 September 2026

This is a recoverable intermediate result, NOT a completed flicker fix.

User observations after restart: the generation-map appearance is substantially
better (brightness changes during rotation seem absent); gameplay still flickers.
The same dedicated POINTS base is used, but menu uses GameplayPointProfile=0,
while gameplay's appearance adapter forces profile=1. These profiles differ in
both photometry and point reconstruction. The next comparison must isolate this
difference without changing catalogue coordinates, count, or physical scale.

This checkpoint preserves the saved round halo, separate late point pass with
explicit depth occlusion, full-scale gameplay point support, and existing
gameplay luminosity data. Orbit fixes are inherited from the parent commit.
Other editor/UI/gas-giant work is deliberately left outside this checkpoint.

The experimental spatial-filter implementation is archived as
`spatial_point_experiment.patch`. It compiled and linked, but its material was NOT
installed: an external commandlet save failed because the live editor held the
asset. The live Python update stopped at an API capability check before making
any changes. Do not apply its native-resolution config to the old point asset.
The working source may still contain this experiment; the checkpoint's active
source and point asset retain the preceding, matching recipe.

Controlled spatial experiments were promising (median adjacent displayed-flux
step around 0.28% vs 9.70% in the matched control), but one gameplay view is not
proof of a complete fix. Earlier same-pose endpoint metrics were insufficient.
The user-reported map/game discrepancy takes priority over further experiments.

Before the cold build, a new safety save `Lufoxusa Cluster 4813` was created through
the game's save API. It was read back and model fields and saved actor count were
verified. Existing saves were not overwritten. The editor was closed normally.
The user subsequently reopened it and is testing. No further close is scheduled.

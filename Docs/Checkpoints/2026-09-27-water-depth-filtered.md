# Filtered water-depth candidate (disabled)

Accepted recovery remains `67fff3b4f8fa57cbbd47f29e880f3b80197f096f`.
This follow-up to `97c02aa3` changes only the diagnostic material builder,
commandlet/probe opt-ins and two new diagnostic assets. It does not switch
production materials, modify WorldScape binaries, change geometry/geography,
saved worlds, palettes, exposure, stars or atmosphere defaults.

## Change

The original candidate blended away the accepted deep/Fresnel color term even
in deep water. The filtered version retains that baseline and adds only a
shallow contribution: `Legacy + (1-Legacy) * Strength * Shallow`.

The shallow exponential is analytically averaged across a one-dimensional
approximation of the pixel's depth range. Material-compiler DDX/DDY nodes supply
the range; derivatives are not evaluated inside a divergent custom-code branch.
Signed depth is retained across coastlines. A cancellation-safe expression is
used near zero. This is not an exact 2-D pixel-footprint integration, refraction
simulation or a solution to insufficient shoreline mesh resolution.

Defaults are strength .35 and half-depth 20 m. Invalid/missing depth payload
returns the accepted term; strength zero restores it. No new textures or noise
loops were added. Actual shader/gameplay cost is not yet benchmarked.

## Evidence and limits

Final build: `build-filtered-precision.log`, 4 actions, 8.34 seconds.
Bake: `bake-filtered-precision/bake.log`, 0 errors, 7 existing warnings;
`Saved=2 bound=0 filtered=1 halfDepthM=20 strength=0.35`.

Independent CPU float32 study (`verify_filtered_curve.py`) checked 572
depth/footprint/half-depth combinations against a double-precision integral.
Worst absolute error was 8.187309e-7; tested outputs were finite, bounded and
never suppressed the baseline. This is a finite numerical study, not exhaustive
shader proof. An earlier experimental bake with a cancellation-prone expression
was preserved under `Saved/WaterDepthFilteredPrecisionV1_20260927`, not installed
over accepted water.

Actual D3D12 SM6 PLANET runs used seed 1337, radius 6750 km, the same generated
globe and settled original/repeat/candidate/zero/invalid/return controls. Both
reports succeeded and owned processes exited with status 0. There are eight
888x500 captures per family; requested window size was not capture resolution.
Actual filtered material binding was logged. Geometry CRCs remained fixed and
material/UV1/ticking were restored.

Mean absolute RGB differences (0-255, ROI [284,90,603,415]):

| Comparison | Terrestrial | Ocean |
| --- | ---: | ---: |
| Original to depth, close | .39 | 1.11 |
| Original to zero strength | .14 | .22 |
| Original to invalid payload | .13 | .23 |
| Repeated original to returned original | .13 | .13 |

Visual inspection shows localized shallows. The previous broad Terrestrial
darkening was not reproduced in the inspected comparison; representative land
pixels stayed near the original and deep-water pixel (405,190) stayed
(35,60,108). Thin coastline outlines are still visible on Terrestrial; Ocean
has wider shallow bands. These are isolated candidate results, not user visual
acceptance, all-family coverage or proof that angular coasts are eliminated.

One-off physical depth sampling of 99846 vertices took 37.833 ms (Terrestrial)
and 39.146 ms (Ocean), not steady-state frame time. Forest/Oasis/Savanna, actual
WorldScape ocean LODs, surface transitions and preservation of ~120 FPS remain
unverified. Keep production selection unchanged until these are addressed.

## Reproduce / recover

Evidence workspace:
`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-visual`.
Runs: `render-filtered-terrestrial` (PID 30040), `render-filtered-ocean` (PID 12172).
Snapshots are under each run's
`Saved/Automation/PlanetRefinement/<family>/WaterDepth20260927` directory.

Creation flag: `-OnlyWaterDepthFilteredCandidate` (refuses existing packages).
Test flags: `-APSProbeWaterDepth -APSWaterDepthFiltered -APSPlanetProbeFamily=Ocean`
or `Terrestrial`, `-APSWaterDepthStrength=0.35 -APSWaterHalfDepthM=20`.
Automation: `APS.Rendered.PlanetRefinement.CausalLayers`.

New assets: `Content/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/WaterDepthFiltered20260927`.
SHA-256:

- Master: `9E8A67D68A92D855F08727869648350B7605FF7A6C7F76EE611ABF67ACDBBF63`
- MIC: `F1DA1314105AEC4B36731B178CEF5FAC2C1C58A9056E89C2709512C58BE46231`
- Terrestrial report: `BDF3B6522E9013C4D20D3889C89AE2DC3802F71605C30B250F1ECF920FDDE9DC`
- Ocean report: `0888AF7C48292B2C0842D538CB1A873ACA580AA50886A05A5E848209762BDF02`

Accepted shared master/MIC rechecked unchanged (SHA-1):
`91EA9BB6BAD5DE82BD29594B259DE4D94D52CCFC` /
`30ADFAAAF2EB11B5E4D30299FA886A67861E8CC8`.
Guarded source backups are `Saved/WaterDepthFiltered_20260927` and
`Saved/WaterDepthFilteredPrecisionSource_20260927`. The accepted external plugin
and editor binary snapshot remains `Saved/AcceptedVisualCheckpoint_20260927`.

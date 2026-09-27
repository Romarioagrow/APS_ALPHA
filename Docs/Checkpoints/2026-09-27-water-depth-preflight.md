# Water-depth preparation after the accepted visual checkpoint

Accepted visual recovery commit: `67fff3b4f8fa57cbbd47f29e880f3b80197f096f`.
Its external WorldScape/AtmoScape/project-binary backup remains documented in
`2026-09-27-worldscape-accepted.md`. This follow-up does not enable depth shading.

## Source changes

- Added `SampleHeightResolvedProfile`: the same terrain field with compile-time
  omission of material/climate/foliage outputs. Existing production entry points
  retain the full path. Only the new diagnostic test calls the height-only path.
- Added a saved-catalog comparison and CPU-cost test for Terrestrial, Ocean,
  Water, Forest, Oasis and Savanna (4096 directions per family).
- Fixed the LOD test's direct inclusion of an unguarded vendor header, without
  changing the installed WorldScape plugin.

No material assets, render bindings, startup defaults, seeds, saved worlds,
planet scales, continents or WorldScape geometry policies were edited. APS was
rebuilt. All 17 installed WorldScape binary-directory files still matched the
accepted manifest at the final check.

## Findings that govern further work

The compressed PLANET geometry deliberately omits physical microrelief. Reusing
its scaled terrain height as physical seabed produced 7.8–49.7 m RMS differences
and 61–133 wet/dry sign differences per 4096 samples across the six families.
Do not deploy the earlier private UV1 prototype unchanged; its preview-depth
assumption has been disproved. Keep its opt-in flag disabled in production.

The height-only sample reduced measured incremental CPU cost by approximately
18–26% in the initial comparison, but this is not an FPS improvement. A 96x96
equivalent sample batch still takes tens of milliseconds of extra CPU work,
excluding native patch geometry, volume queries and scheduling. No shader or
worker cache was installed. Safe immutable reuse of matching land/ocean samples
should be evaluated before adding more ocean-worker sampling.

Floating-point contract: the mathematical field is shared, but independently
optimized template specializations are not bit-identical. A strict-equality test
failed 1734 of 24576 full-scale comparisons; the original tolerant test hid this.
The final test reports the maximum error explicitly and enforces 1e-6 cm (10 nm),
well below mesh/depth-payload precision. It does not change the terrain to fit a
test and does not skip physical noise bands.

## Evidence location and limits

`C:/Users/Rio/Documents/ChatGPT/APOSFERA/work/planet_refinement_20260927/water-depth-sampling`

The README there records measured values, guarded source-install manifests,
backup receipts, build failures/fixes and reports. In particular:

- `probe-v1`: original mismatch/cost measurements.
- `probe-height-only`: initial five passing tests using tolerant comparisons.
- `probe-height-only-exact`: preserved failed bit-equality experiment.
- `probe-height-only-bounded`: final precision-bound and regression checks.

Final result: five successful tests, zero errors, three temporary-world cleanup
warnings (`World has no context`). Maximum measured height-only error was
`2.9103830456733704e-11` cm. Final report SHA256:
`F375D01FEBDD5B9EC5C4C5D58661170101FB2706E06F5951DE3002EE0EBF1BA5`.
Final APS build succeeded; the isolated test process exited itself. No user
editor was stopped, used, or relaunched.

No new visual or gameplay-FPS acceptance is claimed. The user's previously
accepted appearance and reported approximately 120 FPS remain the baseline.
The new sampler does not include WorldScape height/noise volumes or ocean-height
clamping; any eventual worker integration must preserve those separately.

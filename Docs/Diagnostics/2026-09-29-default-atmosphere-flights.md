# Ordinary-atmosphere terrain traversal, 29 September

## Actual run and scope

Claude explicitly released the window at 07:45. Both owned UE processes exited
normally. Runs under `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929`:

- `frozen-flight-default-atmo-v1`, PID25028, started 07:48:48: 134 route PNGs,
  67 native and 67 candidate samples. Two tests Success, one with warnings.
- `terrestrial-flight-default-atmo-v1`, PID30904, started 07:51:11: 131 route
  PNGs, 65 native and 66 candidate samples. Two tests Success, one with warnings.

Both use DLL SHA256
`052EBA41438D2447A10EB26D359F23B88A654CF33BBD5E7434D5B909BF27DDB9`.
Actual generated-world CDO atmosphere: height100km, opacity1, multi1, rayleigh8.
Actual screenshots1280x722; live WorldScape, no mesh freeze, unchanged light and
exposure. The observer travels100km ->100m ->100km over16s plus final hold per
variant. Frames sampled at4Hz, NOT every rendered frame. Native and diagnostic
V3 terrain are compared; water, seed, relief and foliage remain unchanged.

## What was checked

All sampled records retain10 presented terrain LOD components and report zero
incomplete payloads. The workers continue during motion; readiness was not used
to hide or defer captures. Minimum actual heights in metres:
Frozen100.382/100.279, Terrestrial100.004/100.034 (native/candidate).
All eight protected production Shared assets match their pre-run hashes.

Inspected16 original PNGs per run, covering100km, about52km,19km,5.5km,1km,
100m and intermediate/return frames. Frozen: pairs000/006/010/014/031 and
candidate011/012/013/019/050/066. Terrestrial: pairs000/010/014/019/031 and
candidate006/011/012/013/050/065. Files are in each run's
`Saved/Screenshots/Windows/APS_FieldsFlight_<14-or-1>_<Native-or-Candidate>_NNN.png`.

The new material preserves the near-ground appearance in these inspected views.
Frozen's dark rock detail emerges progressively through the inspected approach
frames. The old photosphere's circular shadow boundary is not visible here.
No newly disappearing terrain or rectangular coverage hole was observed in this
sample. This is limited evidence, not all-family or frame-complete LOD acceptance.

Terrestrial STILL shows coarse repetitive/directional texture at intermediate
altitudes in BOTH variants, and sharply cut blue water/land boundaries remain.
The far colour modification does not solve those remaining symptoms. Neither
the candidate nor these tests justify claiming that P0 is finished.

The independently timed routes do not reach identical heights on every frame
(for example Terrestrial frame010 is18.255/18.910km). Do not present pixel diffs
between those frames as an exact material-only measurement. ReadPixels overhead
and active generation invalidate a gameplay-FPS claim from these GPU counters.

## Next required work

Keep V3 diagnostic-only pending remaining family coverage and the unresolved
intermediate-scale pattern. Separately test lateral flight, body switch/resume,
fast travel and ordinary performance without frame readback. Claude reported
WorldScapeRoot spikes in `work/flight/runs/band-stellar-keep-1/trace.utrace`,
seconds28-43; that is an independent report to investigate, not a new measured
result of this pass. Do not alter ships/stars or accepted ground to hide it.

Both foliage switches remain off. No production material/catalog was replaced.
The overall goal remains ACTIVE; water, atmosphere, clouds and rendered foliage
are not accepted by these terrain checks.

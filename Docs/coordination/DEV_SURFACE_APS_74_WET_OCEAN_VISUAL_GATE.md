# APS-74 canonical wet-ocean visual gate correction

Date: 2026-08-14

Result: **PASS for canonical identity, geometry and ocean visibility; FAIL for
the rendered wet-ocean color gate**.

The seed-424242 `Water -> Oceanic` artifact remains the canonical
orbit-to-surface parity truth for body key, seed, radius, controls, profile,
terrain/water identities and full-scale WorldScape configuration. It must not
be replaced by the seed-73991 gallery fixtures.

However, the source automation log
`Saved/Logs/CodexWetOceanHandoff_CleanLivingWater_20260814.log` does not contain
an accepted visual PASS:

- Test: `APS.Rendered.Gameplay.GeneratedCivilizationWetOceanHandoff`.
- Automation result: `Fail` near log line 1293.
- Exact visual failure near line 1291:
  - lower rendered region remained grey/unsaturated;
  - mean RGB = `0.505537 / 0.477669 / 0.450832`;
  - blue dominance = `-0.05470`;
  - required minimum = `+0.01500`.

Evidence that remains valid from the same run:

- Water/Oceanic canonical identity and seed 424242.
- WorldScape ocean LODs = 9; collision terrain LODs = 9.
- Resolved water material path and MID slots.
- Ocean geometry separation matched the expected sampled depths.
- Ocean visibility was real: visible/hidden frame delta = `0.19543`, with
  different CRCs (`1824513944` vs `676223387`).
- Two stable open-water viewpoints and exact camera positions were captured.

Classification consequence:

- The artifact is an accepted **input/identity baseline**, not an accepted wet
  visual output.
- Ocean/Living and Land Coverage inputs remain protected from regression.
- The orbit-to-surface wet appearance gate remains open until a current-code
  run passes the color/saturation check at the same canonical seed.
- Surface will not modify shared water/material/profile files without an
  explicit ownership handoff.


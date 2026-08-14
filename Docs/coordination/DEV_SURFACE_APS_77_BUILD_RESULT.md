# APS-77 Surface material A/B build result

Date: 2026-08-14

Result: **PASS for source/build; runtime and visual A/B remain PENDING**.

- Toolchain: UE 5.4 with the installed `WorldScape_5.4` plugin, matching the
  project association and prior editor target receipt.
- Command target: `APS_ALPHAEditor Win64 Development`.
- UHT completed successfully.
- The subsystem's `Module.APS_ALPHA.10.cpp` unity unit compiled.
- `UnrealEditor-APS_ALPHA.lib` and `UnrealEditor-APS_ALPHA.dll` linked.
- Build completed with zero errors in 10.17 seconds. The only diagnostic was
  the existing non-preferred MSVC toolchain warning.
- A previous UE 5.7 attempt stopped before compilation because that engine
  installation does not contain WorldScape. It is classified as a toolchain
  mismatch, not an APS-77 source failure.
- The planned current-code baseline render did not launch because Dev Ships
  already owned the global heavy slot. PID is none; no screenshot, log, or
  report was overwritten.
- Compile-only evidence is not a visual PASS. Runtime cache/apply/restore and
  same-camera baseline/preset images remain required.

Source checkpoint: `f6e4fea2` (confirmed present on the coordinated remote).


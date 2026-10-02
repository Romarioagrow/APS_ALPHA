# Rejected mesh publication experiments

Not installed. Both trials were rolled back; installed Core remains58AC6497.
Evidence and limitations: Docs/Diagnostics/2026-09-30-mesh-publication-cost.md.

WorldScapeMeshComponent.diff and APSWorldScapeMeshPublicationTests.cpp record
the final private candidate, not production source. The diff uses absolute
before/private paths; inspect and deliberately rebase it before any future use.
Do not install this candidate just because eight private tests passed: its
flight performance was not accepted. Public headers and UObject ABI unchanged.

Completed rollback commands (do not run while any UE/compiler/profiler lives):

```powershell
& Tools/Diagnostics/WorldScapeMeshPublication/Install.ps1 -Rollback -BackupLabel mesh-publication-install-v1
& Tools/Diagnostics/WorldScapeMeshPublication/Install.ps1 -Rollback -BackupLabel mesh-publication-install-v2
```

Reproducing the private candidate needs native suite
Tools/Diagnostics/WorldScapeWaterDepthIntegration/RunNativePayloadTests.ps1,
then a fresh backup label and matching evidence label for Install.ps1. Its
default labels point at an existing transaction and intentionally refuse a new
install. Source/hash/ABI/process guards must pass; preserve all other changes.

Runner switches for an explicitly installed experimental build:
`-MeshUpdateTasks 1` / `4`, `-FastMeshCopy`, `-WaterFlight -Performance`.
Add `-CpuTrace` only for a separate attribution pass, not paired timings.
Normal runner calls without publication switches do not reference candidate CVars.

Next implementation should address worker-owned immutable render payload and
bounded lifetime, not keep tuning these two rejected inner-loop variants.

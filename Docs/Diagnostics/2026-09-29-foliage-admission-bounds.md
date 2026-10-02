# Foliage: aggregate per-root admission bounds (29 September)

Current 07:20: ALL EIGHT foliage policy tests, including `PrototypePaletteGates`,
passed on DLL A0F72033 in `volcanic-fields-v3-magma-family-v2` at 07:05-07:06.
No new policy code after that run. All runtime keys remain default off.
The independent asset bake failed safely before saving: vendor tree exceeded
the mesh triangle cap. Prepared builder-only reduction now needs a new link/bake;
it does not change these tested policy gates. No catalog/profile/save changes.

Status 06:25: built successfully at 06:11 (DLL 4572118D). ALL SEVEN foliage
automation tests passed in `volcanic-fields-v3-family-v1`: ActivationGates,
AggregateAdmissionBudget, CatalogDefaultsOff, FreshRootApplication,
PluginSectorEnvelope, TransientHISMBudget and WorldAdmissionBudget.
NOT visually/performance accepted: the prototype is still default OFF.
No asset, profile/save schema, ship or plugin file changed.

Transient copies now mask RF_Public and RF_Standalone DURING duplication,
including nested objects; RF_Transient is retained on the copied entry. Merely
adding RF_Transient to a duplicated public/standalone source did not remove
those ownership flags. The test verifies that the original flags are preserved
on authored inputs and absent on runtime copies. This is not a measured leak fix.

## World admission extension (05:53)

The policy now reserves at most TWO APS-owned foliage roots per UWorld, each
charged its entire per-root allowance (131,072 potential instances / 8,192
potential components per world). This is deliberately conservative, not a target
density, measured residency, byte limit or guarantee for manually-authored roots
that bypass the policy. A live root is charged even if currently distant/empty.

Reservations use weak UObject identities and are pruned only after actual object
reclamation, not on actor pending-kill or during the GC mark phase. No forced GC,
worker drain, root destruction, new tick/subsystem or streaming change was added.
The third candidate is refused BEFORE loading optional mesh/material dependencies.
The capacity is checked again after synchronous loading to cover re-entrant admission.
Empty/disabled/preview/dedicated-server roots consume no reservation. Reapplying
the policy to an admitted or populated root preserves its existing arrays.

Limitation: a denied fresh root remains without foliage until recreated. There
is intentionally no live collection retry; the plugin lacks a public safe drain.
Selection/priority during rapid planet changes and measured memory still require
prototype traversal tests. The existing cvar is a fresh-root opt-in, not an
immediate cancellation switch for already-running workers.

`WorldAdmissionBudget` automation covers the cap, denial before optional loading,
pending-kill retention, non-destructive reapplication, separate-world budgets and
authored-data preservation. It PASSED in the 06:12-06:14 UE run. Earlier syntax evidence:
`F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/foliage-world-admission-syntax-v1`.
The menu A/B launcher now accepts `-FoliageRegression`, allowing the complete
suite to run in the same short editor session as the material check.

## Verified plugin contract

WorldScape 5.4 `WorldScapeRoot_Helper.cpp::GetSurroundingFoliageSector` produces
27 neighbouring sector centres for a sphere. `WorldScapeRoot_Main.cpp::
FoliageHandleTick` retains/spawns sectors while the player is inside an inclusive
cube of half-width `Sector.Size * 4`. `WorldScapeHelper.cpp::IsPointInCube` uses
its scalar size as HALF-width, not side length. There can therefore be 9^3=729
unique retained lattice centres per type, not just the 27 generated per batch.
The game thread publishes sectors before retiring old sectors. The admission
envelope conservatively adds one 27-sector worker batch: 729+27=756 per type.

This relies on the audited plugin's one-worker/one-batch lifecycle, unique sector
identity, fixed per-type sector size on fresh roots and post-spawn retirement.
It is not a substitute for a runtime reservation/counter or for a memory bound:
mesh/texture dependencies, component internals, transient copies and GC can cost
more than a count of rendered instances suggests. The new sector test checks the
actual helper geometry and generation ring; future changes to the retention
call site still require an explicit audit.

## Source change

`FAPSWorldScapeFoliagePolicy::BuildBudgetedCollections` now divides an aggregate
per-root envelope across ALL allowed collections before allocating their types:

- at most 65,536 potential peak instances;
- at most 4,096 potential peak instanced-mesh components;
- cluster expansion counts toward instances, every retained cluster mesh counts
  toward components, including a unit that might emit no transforms;
- reserve a component slot for each subsequent type; clamp a cluster's mesh list
  to the remaining allocation;
- preserve ordered authored priorities, relative density among admitted types,
  biome gates and authored assets; keep the existing HISM/no-collision/no-shadow
  sanitization;
- at extremely low density, retain the first viable type instead of discarding
  the whole collection because there were more types than spawn slots.

These are conservative admission envelopes, not target densities. With the
largest allowed input, the previous per-sector ceilings permitted estimates of
774,144 instances and 36,288 components; the new instance quantization permits
at most 65,016 and the component allocation stays below 4,096. No speedup is
claimed from this arithmetic. Multiple roots still need a shared world-level
budget, plus runtime residency/queue/memory and traversal measurements before
foliage can be enabled for the accepted prototype.

## Verification and follow-up

Added automation tests:

- `APS.Gameplay.World.PlanetSurface.Foliage.AggregateAdmissionBudget` covers one
  and two collections, mesh-only/cluster-only/mixed palettes, oversized inputs,
  cluster expansion, authored-data preservation and low density.
- `APS.Gameplay.World.PlanetSurface.Foliage.PluginSectorEnvelope` checks the
  actual plugin's 27-sector ring and 729-centre inclusive cube.

Both policy and test translation units pass MSVC /Zs using the current UBT
response flags, one compiler at a time and the engine Source working directory.
First syntax attempt used the project working directory and failed include
resolution (`CoreMinimal.h`); corrected second attempt passed both files.
Evidence: `F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/foliage-admission-syntax-v2`.
Historical 05:14 check: no object/DLL was installed then. DLL hash at that time was
`736998CFEEF2E00BE1375673659E0AFE35C7D9FCA64B030999940ABC65F581A7`.

When the editor window is available: link the pending changes, run the full
`APS.Gameplay.World.PlanetSurface.Foliage` suite, resume the higher-priority
OrbitalFields family render matrix, and only then build the bounded content
prototype. This note does not close the foliage or visual-continuity epic.

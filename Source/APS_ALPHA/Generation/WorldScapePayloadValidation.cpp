#include "WorldScapePayloadValidation.h"

// WorldScapeLod.h in the 5.4 marketplace plugin has no include guard. Including
// it directly breaks Unreal unity builds when another source file has already
// reached it through WorldScapeRoot.h. The root header is guarded and still
// provides the complete UWorldScapeLod definition required below.
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "Async/ParallelFor.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace APSWorldScapePayloadValidation
{
	namespace
	{
		// 08.10 (tr062 traces): the readiness frame of every body scanned ~10 LODs x 3 payloads (48-64k vertices each)
		// for NaNs on the GameThread, ~8 ms per body. The arrays are worker-filled and untouched once the root has no
		// LOD in generation (this file's contract), so the read-only scans can run on the task workers too. Off by default
		// until its own A/B (it borrows the workers the LOD jobs and the render thread's tasks use).
		TAutoConsoleVariable<int32> CVarParallelScan(TEXT("aps.Surface.ParallelReadyScan"), 0,
			TEXT("08.10: the NaN scans of a WorldScape payload readiness check run in parallel chunks (same result). ")
			TEXT("0: one serial scan on the GameThread, as before."));
		constexpr int32 ParallelChunk = 8192;

		template <typename T, typename FBad>
		bool AnyBad(const TArray<T>& Values, FBad&& IsBad)
		{
			const int32 Num = Values.Num();
			if (Num < 2 * ParallelChunk || CVarParallelScan.GetValueOnAnyThread() == 0 || !IsInGameThread())
			{
				return Values.ContainsByPredicate(IsBad);
			}
			std::atomic<bool> bBad{false};
			const int32 Chunks = (Num + ParallelChunk - 1) / ParallelChunk;
			ParallelFor(Chunks, [&Values, &IsBad, &bBad, Num](const int32 Chunk)
			{
				const int32 End = FMath::Min(Num, (Chunk + 1) * ParallelChunk);
				for (int32 Index = Chunk * ParallelChunk; Index < End && !bBad.load(std::memory_order_relaxed); ++Index)
				{
					if (IsBad(Values[Index]))
					{
						bBad.store(true, std::memory_order_relaxed);
					}
				}
			});
			return bBad.load();
		}

		bool HasFiniteVectors(const TArray<FVector>& Values)
		{
			return Values.Num() > 0 && !AnyBad(Values, [](const FVector& Value) { return Value.ContainsNaN(); });
		}

		bool HasFiniteColors(const TArray<FLinearColor>& Values)
		{
			return Values.Num() > 0 && !AnyBad(Values,
				[](const FLinearColor& Value)
				{
					return !FMath::IsFinite(Value.R) || !FMath::IsFinite(Value.G)
						|| !FMath::IsFinite(Value.B) || !FMath::IsFinite(Value.A);
				});
		}
	}

	bool HasCompletePayload(const UWorldScapeLod* Lod, const bool bRequireProfileColor)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_HasCompletePayload);
		if (!IsValid(Lod) || !IsValid(Lod->Mesh) || Lod->Mesh->GetNumSections() < 3)
		{
			return false;
		}

		const bool bMainComplete = HasFiniteVectors(Lod->Vertices)
			&& Lod->Normals.Num() == Lod->Vertices.Num()
			&& Lod->UV.Num() == Lod->Vertices.Num()
			&& Lod->VertexColors.Num() == Lod->Vertices.Num()
			&& HasFiniteVectors(Lod->Normals) && HasFiniteColors(Lod->VertexColors)
			&& Lod->Triangles.Num() > 0;
		const bool bPatchAComplete = HasFiniteVectors(Lod->VerticesPA)
			&& Lod->VerticesNormalPA.Num() == Lod->VerticesPA.Num()
			&& Lod->UVPA.Num() == Lod->VerticesPA.Num()
			&& Lod->VerticesColorPA.Num() == Lod->VerticesPA.Num()
			&& HasFiniteVectors(Lod->VerticesNormalPA)
			&& HasFiniteColors(Lod->VerticesColorPA)
			&& Lod->TrianglesPatchA.Num() > 0;
		const bool bPatchBComplete = HasFiniteVectors(Lod->VerticesPB)
			&& Lod->VerticesNormalPB.Num() == Lod->VerticesPB.Num()
			&& Lod->UVPB.Num() == Lod->VerticesPB.Num()
			&& Lod->VerticesColorPB.Num() == Lod->VerticesPB.Num()
			&& HasFiniteVectors(Lod->VerticesNormalPB)
			&& HasFiniteColors(Lod->VerticesColorPB)
			&& Lod->TrianglesPatchB.Num() > 0;
		const bool bProfileColorReady = !bRequireProfileColor
			|| Lod->VertexColors.ContainsByPredicate(
				[](const FLinearColor& Color)
				{
					return !Color.Equals(FLinearColor::White, KINDA_SMALL_NUMBER);
				});

		return bMainComplete && bPatchAComplete && bPatchBComplete && bProfileColorReady;
	}

	bool HasCompleteCenteredPayload(const UWorldScapeLod* Lod,
		const FVector& DesiredSurfaceNormal, const bool bRequireProfileColor)
	{
		if (!IsValid(Lod) || DesiredSurfaceNormal.IsNearlyZero()
			|| Lod->SnappedAngle.ContainsNaN())
		{
			return false;
		}

		const FVector SnappedNormal = Lod->SnappedAngle.GetSafeNormal();
		return !SnappedNormal.IsNearlyZero()
			&& FVector::DotProduct(SnappedNormal, DesiredSurfaceNormal) >= 0.995
			&& HasCompletePayload(Lod, bRequireProfileColor);
	}

	bool HasExactCenteredPayloadSet(
		const TArray<UWorldScapeLod*>& Lods, const int32 ExpectedCount,
		const bool bExpectedWaterBody, const FVector& DesiredSurfaceNormal,
		const bool bRequireProfileColor)
	{
		if (ExpectedCount <= 0 || Lods.Num() != ExpectedCount
			|| DesiredSurfaceNormal.IsNearlyZero())
		{
			return false;
		}

		TBitArray<> SeenLodIds(false, ExpectedCount);
		TSet<const UWorldScapeLod*> SeenLods;
		TSet<const UWorldScapeMeshComponent*> SeenMeshes;
		for (const UWorldScapeLod* Lod : Lods)
		{
			if (!IsValid(Lod) || !IsValid(Lod->Mesh)
				|| Lod->WaterBody != bExpectedWaterBody
				|| Lod->Lod < 0 || Lod->Lod >= ExpectedCount
				|| SeenLodIds[Lod->Lod] || SeenLods.Contains(Lod)
				|| SeenMeshes.Contains(Lod->Mesh)
				|| !HasCompleteCenteredPayload(
					Lod, DesiredSurfaceNormal, bRequireProfileColor))
			{
				return false;
			}
			SeenLodIds[Lod->Lod] = true;
			SeenLods.Add(Lod);
			SeenMeshes.Add(Lod->Mesh);
		}
		// Count equality plus in-range uniqueness means every id is represented.
		return true;
	}
}

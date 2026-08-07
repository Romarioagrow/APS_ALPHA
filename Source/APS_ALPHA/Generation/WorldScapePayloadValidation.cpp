#include "WorldScapePayloadValidation.h"

// WorldScapeLod.h in the 5.4 marketplace plugin has no include guard. Including
// it directly breaks Unreal unity builds when another source file has already
// reached it through WorldScapeRoot.h. The root header is guarded and still
// provides the complete UWorldScapeLod definition required below.
#include "WorldScapeCore/Public/WorldScapeRoot.h"

namespace APSWorldScapePayloadValidation
{
	namespace
	{
		bool HasFiniteVectors(const TArray<FVector>& Values)
		{
			return Values.Num() > 0 && !Values.ContainsByPredicate(
				[](const FVector& Value) { return Value.ContainsNaN(); });
		}

		bool HasFiniteColors(const TArray<FLinearColor>& Values)
		{
			return Values.Num() > 0 && !Values.ContainsByPredicate(
				[](const FLinearColor& Value)
				{
					return !FMath::IsFinite(Value.R) || !FMath::IsFinite(Value.G)
						|| !FMath::IsFinite(Value.B) || !FMath::IsFinite(Value.A);
				});
		}
	}

	bool HasCompletePayload(const UWorldScapeLod* Lod, const bool bRequireProfileColor)
	{
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
}

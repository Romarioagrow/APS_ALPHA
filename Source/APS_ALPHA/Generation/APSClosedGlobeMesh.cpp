#include "APSClosedGlobeMesh.h"

#include "APSWorldScapePlanetNoise.h"
#include "APSPreviewCoastalMeshRefinement.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "Algo/AllOf.h"
#include "Async/ParallelFor.h"

namespace APSClosedGlobeMesh
{
	struct FCubeFace
	{
		FVector Normal;
		FVector AxisU;
		FVector AxisV;
	};

	bool BuildClosedCubeSphere(const FSamplingFrame& Frame,
		const FBuildOptions& Options, FMeshData& OutData)
	{
		const bool bBuildOcean = Options.bBuildOcean;
		const bool bWaterDepth = Options.bWaterDepth;
		const double PresentationScale = Frame.PresentationScale;
		const double NormalReliefExaggeration = Options.NormalReliefExaggeration;
		const int32 FaceResolution = Options.FaceResolution;
		const bool bRefineCoast = Options.bRefineCoast;
		const bool bPreviewNormals = Options.NormalPolicy == ENormalPolicy::PreviewReliefBlend;
		if (!FMath::IsFinite(Frame.Radius) || Frame.Radius <= UE_SMALL_NUMBER
			|| !FMath::IsFinite(Frame.OceanHeight)
			|| !FMath::IsFinite(Frame.NoiseScale) || !FMath::IsFinite(Frame.NoiseIntensity)
			|| !FMath::IsFinite(PresentationScale) || PresentationScale <= 0.0
			|| !FMath::IsFinite(NormalReliefExaggeration) || NormalReliefExaggeration <= 0.0
			|| FaceResolution < 16 || FaceResolution > MaximumFaceResolution
			|| (!bPreviewNormals && Options.NormalPolicy != ENormalPolicy::PhysicalTriangles)
			|| !OutData.TerrainVertices.IsEmpty() || !OutData.OceanVertices.IsEmpty()
			|| !OutData.Indices.IsEmpty() || !OutData.OceanIndices.IsEmpty()
			|| !OutData.Normals.IsEmpty() || !OutData.OceanNormals.IsEmpty()
			|| !OutData.OceanDepthUV1.IsEmpty() || !OutData.UV0.IsEmpty()
			|| !OutData.VertexColors.IsEmpty() || !OutData.OceanVertexColors.IsEmpty()
			|| !OutData.Tangents.IsEmpty())
		{
			return false;
		}

		static const FCubeFace Faces[] =
		{
			{ FVector( 1, 0, 0), FVector( 0, 1, 0), FVector(0, 0, 1) },
			{ FVector(-1, 0, 0), FVector( 0,-1, 0), FVector(0, 0, 1) },
			{ FVector( 0, 1, 0), FVector(-1, 0, 0), FVector(0, 0, 1) },
			{ FVector( 0,-1, 0), FVector( 1, 0, 0), FVector(0, 0, 1) },
			{ FVector( 0, 0, 1), FVector( 1, 0, 0), FVector(0, 1, 0) },
			{ FVector( 0, 0,-1), FVector(-1, 0, 0), FVector(0, 1, 0) }
		};

		const int32 VerticesPerFace = FMath::Square(FaceResolution + 1);
		const int32 TotalVertices = UE_ARRAY_COUNT(Faces) * VerticesPerFace;
		const int32 TotalIndices = UE_ARRAY_COUNT(Faces)
			* FaceResolution * FaceResolution * 6;
		OutData.TerrainVertices.Reserve(TotalVertices);
		if (bBuildOcean) OutData.OceanVertices.Reserve(TotalVertices);
		OutData.Indices.Reserve(TotalIndices);
		if (bBuildOcean) OutData.OceanIndices.Reserve(TotalIndices);
		OutData.Normals.Reserve(TotalVertices);
		if (bBuildOcean) OutData.OceanNormals.Reserve(TotalVertices);
		if (bBuildOcean) OutData.OceanDepthUV1.Reserve(TotalVertices);
		OutData.UV0.Reserve(TotalVertices);
		OutData.VertexColors.Reserve(TotalVertices);
		if (bBuildOcean) OutData.OceanVertexColors.Reserve(TotalVertices);
		OutData.Tangents.Reserve(TotalVertices);
		TArray<FVector> NormalReferenceVertices;
		NormalReferenceVertices.Reserve(TotalVertices);

		const double Radius = Frame.Radius;
		const double OceanRadius = Radius + Frame.OceanHeight;
		// Keep geometry physically faithful while exaggerating the same sampled height
		// only for its lighting normal. At orbital distance the real displacement is
		// sub-pixel; radial normals therefore made every detailed profile read as a
		// blurred smooth ball. This costs no additional resolver/noise evaluations.
		// The orbital globe is 1.2M cm across while the resolver's authored relief is
		// commonly only a few dozen centimetres after presentation scaling.  A 7x
		// reference surface still collapsed to an almost perfectly radial normal
		// (and made otherwise distinct terrain read as a smooth sphere).  Keep the
		// real vertices unchanged, but use enough bounded relief for stable orbital
		// lighting across the full preset range.
		constexpr double MaximumNormalReliefFraction = 0.015;
		// Profile resolution dominates the first selected 128x128 build (~100k
		// vertices). The six cube faces are independent, so sample them concurrently
		// while retaining the exact selected topology and resolved surface values. Each
		// worker owns its seeded noise state and receives a value snapshot of the
		// resolved profile, so no UObject is read or written off the game thread.
		TArray<FVector> SampleDirections;
		SampleDirections.SetNumUninitialized(TotalVertices);
		TArray<FNoiseData> SurfaceSamples;
		SurfaceSamples.SetNumUninitialized(TotalVertices);
		TArray<uint8> FaceSamplesValid;
		FaceSamplesValid.SetNumZeroed(UE_ARRAY_COUNT(Faces));
		FVector* DirectionData = SampleDirections.GetData();
		FNoiseData* SurfaceData = SurfaceSamples.GetData();
		uint8* FaceValidityData = FaceSamplesValid.GetData();
		const CustomNoise SeededNoise = Frame.SeededNoise;
		const FAPSResolvedPlanetSurfaceProfile SurfaceProfile = Frame.Profile;
		const bool bCoastalReliefCandidate = Frame.bCoastalReliefCandidate;
		const double NoiseScale = Frame.NoiseScale;
		const double NoiseIntensity = Frame.NoiseIntensity;
		ParallelFor(UE_ARRAY_COUNT(Faces), [Radius, FaceResolution,
			VerticesPerFace, DirectionData, SurfaceData, FaceValidityData,
			SeededNoise, &SurfaceProfile, NoiseScale, NoiseIntensity, bCoastalReliefCandidate](const int32 FaceIndex)
		{
			const FCubeFace& Face = Faces[FaceIndex];
			CustomNoise FaceNoise = SeededNoise;
			bool bFaceSamplesValid = true;
			for (int32 Y = 0; Y <= FaceResolution; ++Y)
			{
				const double V = -1.0 + 2.0 * static_cast<double>(Y) / FaceResolution;
				for (int32 X = 0; X <= FaceResolution; ++X)
				{
					const double U = -1.0 + 2.0 * static_cast<double>(X) / FaceResolution;
					const int32 SampleIndex = FaceIndex * VerticesPerFace
						+ Y * (FaceResolution + 1) + X;
					const FVector Direction = (Face.Normal + Face.AxisU * U + Face.AxisV * V)
						.GetSafeNormal();
					DVector NoisePosition;
					const FNoiseData Surface =
						UAPSWorldScapePlanetNoise::SampleResolvedProfile(
						SurfaceProfile, FaceNoise, DVector(Direction * Radius),
						DVector(0.0, 0.0, 0.0),
						NoiseScale, NoiseIntensity, Radius, Direction.Z, NoisePosition, bCoastalReliefCandidate);
					DirectionData[SampleIndex] = Direction;
					SurfaceData[SampleIndex] = Surface;
					bFaceSamplesValid = bFaceSamplesValid
						&& FMath::IsFinite(Surface.Height)
						&& FMath::IsFinite(Surface.HeightNormalize)
						&& FMath::IsFinite(Surface.Temperature)
						&& FMath::IsFinite(Surface.Humidity)
						&& FMath::IsFinite(Surface.WaterMask);
				}
			}
			FaceValidityData[FaceIndex] = bFaceSamplesValid ? 1 : 0;
		});
		if (!Algo::AllOf(FaceSamplesValid,
			[](const uint8 bFaceValid) { return bFaceValid != 0; }))
		{
			return false;
		}

		// Both base vertices and optional coastal vertices use exactly the same
		// physical sample -> render channels mapping. No interpolated height data.
		const auto AppendSampleVertex = [&](const FVector& Direction, const FNoiseData& Surface)
		{
			// Match UAPSWorldScapePlanetNoise::Evaluate exactly: the native envelope
			// uses the actual noise intensity, not a separately rounded ocean shell.
			const double Height = APSWorldScapeSurfaceEnvelope::Height(Surface.Height,
				static_cast<double>(SurfaceProfile.OceanLevel) * NoiseIntensity,
				Frame.bApplyNativeLavaEnvelope);
			OutData.TerrainVertices.Add(Direction * (Radius + Height));
			double NormalReferenceHeight = Height;
			if (bPreviewNormals)
			{
				const double RawNormalReferenceHeight = Height * NormalReliefExaggeration;
				const double MaximumNormalRelief = Radius * MaximumNormalReliefFraction;
				NormalReferenceHeight = RawNormalReferenceHeight
					/ (1.0 + FMath::Abs(RawNormalReferenceHeight) / FMath::Max(MaximumNormalRelief, 1.0));
			}
			NormalReferenceVertices.Add(Direction * (Radius + NormalReferenceHeight));
			if (bBuildOcean)
			{
				OutData.OceanVertices.Add(Direction * OceanRadius);
				OutData.OceanNormals.Add(Direction);
				OutData.OceanDepthUV1.Add(bWaterDepth ? FVector2D(
					(Frame.OceanHeight - Surface.Height) / PresentationScale / 100000.0, 1.0)
					: FVector2D::ZeroVector);
			}
			OutData.Normals.Add(FVector::ZeroVector);
			OutData.UV0.Add(FVector2D(
				0.5 + FMath::Atan2(Direction.Y, Direction.X) / (2.0 * UE_PI),
				0.5 - FMath::Asin(Direction.Z) / UE_PI));
			// Native terrain uses Hole alpha (intact ground = 0). Menu terrain and
			// the separate closed ocean preserve the resolver's liquid WaterMask.
			const FLinearColor OceanColor(Surface.HeightNormalize, Surface.Temperature,
				Surface.Humidity, FMath::Clamp(Surface.WaterMask, 0.0f, 1.0f));
			FLinearColor TerrainColor = OceanColor;
			if (Options.bNativeTerrainHoleAlpha) TerrainColor.A = Surface.Hole ? 1.0f : 0.0f;
			OutData.VertexColors.Add(TerrainColor);
			if (bBuildOcean) OutData.OceanVertexColors.Add(OceanColor);
			FVector Tangent = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
			if (Tangent.IsNearlyZero())
				Tangent = FVector::CrossProduct(FVector::RightVector, Direction).GetSafeNormal();
			OutData.Tangents.Add(FProcMeshTangent(Tangent, false));
		};

		for (int32 FaceIndex = 0; FaceIndex < UE_ARRAY_COUNT(Faces); ++FaceIndex)
		{
			const FCubeFace& Face = Faces[FaceIndex];
			const int32 FaceStart = OutData.TerrainVertices.Num();
			for (int32 Y = 0; Y <= FaceResolution; ++Y)
			{
				for (int32 X = 0; X <= FaceResolution; ++X)
				{
					const int32 SampleIndex = FaceIndex * VerticesPerFace
						+ Y * (FaceResolution + 1) + X;
					AppendSampleVertex(SampleDirections[SampleIndex], SurfaceSamples[SampleIndex]);
				}
			}

			for (int32 Y = 0; Y < FaceResolution; ++Y)
			{
				for (int32 X = 0; X < FaceResolution; ++X)
				{
					const int32 A = FaceStart + Y * (FaceResolution + 1) + X;
					const int32 B = A + 1;
					const int32 C = A + FaceResolution + 1;
					const int32 D = C + 1;
					// UProceduralMeshComponent renders the clockwise face as the front face.
					// Keep the supplied vertex normals outward, but reverse the
					// geometric cross-product winding so the globe is front-facing from
					// outside rather than being culled as an inside-out shell.
					OutData.Indices.Add(A);
					OutData.Indices.Add(D);
					OutData.Indices.Add(B);
					OutData.Indices.Add(A);
					OutData.Indices.Add(C);
					OutData.Indices.Add(D);

					if (bBuildOcean)
					{
						const int32 LocalA = Y * (FaceResolution + 1) + X;
						const int32 LocalB = LocalA + 1;
						const int32 LocalC = LocalA + FaceResolution + 1;
						const int32 LocalD = LocalC + 1;
						// The closed shell uses the authoritative smooth WaterMask carried in
						// vertex alpha. Keeping complete topology avoids triangle-sized coast
						// steps and pinholes; the shared liquid material applies that mask only
						// in orbital presentation, never on the physical WorldScape ocean.
						OutData.OceanIndices.Add(A);
						OutData.OceanIndices.Add(D);
						OutData.OceanIndices.Add(B);
						OutData.OceanIndices.Add(A);
						OutData.OceanIndices.Add(C);
						OutData.OceanIndices.Add(D);
					}
				}
			}
		}

		if (bRefineCoast && bBuildOcean)
		{
			APSPreviewCoastalMeshRefinement::FStats Stats;
			const double StartSeconds = FPlatformTime::Seconds();
			if (!APSPreviewCoastalMeshRefinement::Refine(SampleDirections, SurfaceSamples,
				OutData.Indices, SurfaceProfile, SeededNoise, Radius, Frame.OceanHeight,
				NoiseScale, NoiseIntensity, bCoastalReliefCandidate, Stats))
				return false; // Never publish a partial topology or invalid sample.
			for (int32 Index = TotalVertices; Index < SampleDirections.Num(); ++Index)
				AppendSampleVertex(SampleDirections[Index], SurfaceSamples[Index]);
			OutData.OceanIndices = OutData.Indices;
			UE_LOG(LogTemp, Display, TEXT("[APS.PreviewCoast] candidate=1 passes=%d addedVertices=%d triangles=%d budgetLimited=%d cpuMs=%.3f geographyChanged=0"),
				Stats.Passes, Stats.AddedVertices, OutData.Indices.Num() / 3, int32(Stats.bBudgetLimited),
				(FPlatformTime::Seconds() - StartSeconds) * 1000.0);
		}
		const int32 MeshVertexCount = SampleDirections.Num();

		// Accumulate area-weighted normals from the selected reference surface.
		// The render indices intentionally use ProceduralMesh's clockwise front face,
		// so orient the mathematical cross product back toward the radial direction.
		TArray<FVector> AccumulatedNormals;
		AccumulatedNormals.Init(FVector::ZeroVector, MeshVertexCount);
		for (int32 Triangle = 0; Triangle + 2 < OutData.Indices.Num(); Triangle += 3)
		{
			const int32 I0 = OutData.Indices[Triangle];
			const int32 I1 = OutData.Indices[Triangle + 1];
			const int32 I2 = OutData.Indices[Triangle + 2];
			const FVector& P0 = NormalReferenceVertices[I0];
			const FVector& P1 = NormalReferenceVertices[I1];
			const FVector& P2 = NormalReferenceVertices[I2];
			FVector WeightedNormal = FVector::CrossProduct(P1 - P0, P2 - P0);
			const FVector TriangleDirection = (P0 + P1 + P2).GetSafeNormal();
			if (FVector::DotProduct(WeightedNormal, TriangleDirection) < 0.0)
			{
				WeightedNormal *= -1.0;
			}
			if (!WeightedNormal.ContainsNaN() && !WeightedNormal.IsNearlyZero())
			{
				AccumulatedNormals[I0] += WeightedNormal;
				AccumulatedNormals[I1] += WeightedNormal;
				AccumulatedNormals[I2] += WeightedNormal;
			}
		}

		// Cube faces duplicate their edge/corner vertices. Weld the accumulated normal
		// by quantized radial direction so lighting remains continuous across all six
		// seams without changing the deliberately independent topology or UVs.
		constexpr double NormalWeldQuantization = 1048576.0;
		const auto MakeNormalWeldKey = [NormalWeldQuantization](const FVector& Position)
		{
			const FVector Direction = Position.GetSafeNormal();
			return FIntVector(
				FMath::RoundToInt(Direction.X * NormalWeldQuantization),
				FMath::RoundToInt(Direction.Y * NormalWeldQuantization),
				FMath::RoundToInt(Direction.Z * NormalWeldQuantization));
		};
		TMap<FIntVector, FVector> WeldedNormals;
		WeldedNormals.Reserve(MeshVertexCount);
		for (int32 VertexIndex = 0; VertexIndex < MeshVertexCount; ++VertexIndex)
		{
			WeldedNormals.FindOrAdd(MakeNormalWeldKey(
				NormalReferenceVertices[VertexIndex])) += AccumulatedNormals[VertexIndex];
		}
		constexpr float DetailedNormalWeight = 0.42f;
		for (int32 VertexIndex = 0; VertexIndex < MeshVertexCount; ++VertexIndex)
		{
			const FVector RadialNormal = OutData.TerrainVertices[VertexIndex].GetSafeNormal();
			FVector DetailedNormal = WeldedNormals.FindRef(
				MakeNormalWeldKey(NormalReferenceVertices[VertexIndex]));
			if (!DetailedNormal.Normalize())
			{
				DetailedNormal = RadialNormal;
			}
			if (FVector::DotProduct(DetailedNormal, RadialNormal) < 0.0)
			{
				DetailedNormal *= -1.0;
			}
			FVector FinalNormal = bPreviewNormals
				? DetailedNormal * DetailedNormalWeight + RadialNormal * (1.0f - DetailedNormalWeight)
				: DetailedNormal;
			if (!FinalNormal.Normalize())
			{
				FinalNormal = RadialNormal;
			}
			OutData.Normals[VertexIndex] = FinalNormal;
		}

		return OutData.TerrainVertices.Num() == MeshVertexCount
			&& OutData.Indices.Num() >= TotalIndices && OutData.Indices.Num() % 3 == 0
			&& OutData.Normals.Num() == MeshVertexCount
			&& OutData.UV0.Num() == MeshVertexCount
			&& OutData.VertexColors.Num() == MeshVertexCount
			&& OutData.Tangents.Num() == MeshVertexCount
			&& (!bRefineCoast || MeshVertexCount <= TotalVertices + APSPreviewCoastalMeshRefinement::MaximumExtraVertices)
			&& (bRefineCoast || (MeshVertexCount == TotalVertices && OutData.Indices.Num() == TotalIndices))
			&& (!bBuildOcean || (OutData.OceanVertices.Num() == MeshVertexCount
				&& OutData.OceanNormals.Num() == MeshVertexCount
				&& OutData.OceanDepthUV1.Num() == MeshVertexCount
				&& OutData.OceanVertexColors.Num() == MeshVertexCount
				&& OutData.OceanIndices.Num() == OutData.Indices.Num()));
	}
}

#include "AstroGenerator.h"
#include <Kismet/GameplayStatics.h>
#include "APSWorldScapePlanetNoise.h"
#include "PlanetarySurfaceGenerator.h"
#include "WorldScapePayloadValidation.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "PlanetaryAtmosphere.h"
#include "PlanetGenerator.h"
#include "APS_ALPHA/Actors/Astro/AstroAnchor.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/GalaxyModel.h"
#include "APS_ALPHA/Core/Structs/MoonGenerationModel.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarSystemGenerationModel.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include <unordered_map>
#include <functional>
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/GameModes/MainMenuGameModeBase.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "LocalVertexFactory.h"
#include "Misc/ScopeExit.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace APSPreviewGlobe
{
	// A 48x48 face stays bounded while the selected planet family is warmed one body
	// per tick, and keeps coastlines/mountain silhouettes from becoming faceted blobs.
	constexpr int32 FaceResolution = 48;
	constexpr const TCHAR* TerrainMaterialPath =
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalTerrain.M_APS_OrbitalTerrain");
	constexpr const TCHAR* LiquidMaterialPath =
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_OrbitalLiquid.M_APS_OrbitalLiquid");

	const TCHAR* GetLiquidMaterialPath(const EAPSPlanetLiquidType LiquidType)
	{
		switch (LiquidType)
		{
		case EAPSPlanetLiquidType::Water:
			return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Water.MI_APS_OrbitalLiquid_Water");
		case EAPSPlanetLiquidType::Ammonia:
			return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Ammonia.MI_APS_OrbitalLiquid_Ammonia");
		case EAPSPlanetLiquidType::Lava:
			return TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/MI_APS_OrbitalLiquid_Lava.MI_APS_OrbitalLiquid_Lava");
		default:
			return nullptr;
		}
	}

	UMaterialInstanceDynamic* CreateTerrainMaterial(
		UObject* Outer, UMaterialInterface* BaseMaterial,
		const FAPSResolvedPlanetSurfaceProfile& Profile)
	{
		if (!IsValid(BaseMaterial) || BaseMaterial->GetBlendMode() != BLEND_Opaque)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
			BaseMaterial, Outer);
		if (!IsValid(Material))
		{
			return nullptr;
		}

		UAPSPlanetSurfaceProfileResolver::ApplyMaterialParameters(Material, Profile);
		Material->SetScalarParameterValue(TEXT("Roughness"),
			FMath::Clamp(Profile.Roughness, 0.0f, 1.0f));
		Material->SetScalarParameterValue(TEXT("Metallic"),
			FMath::Clamp(Profile.Metallic, 0.0f, 1.0f));
		Material->SetScalarParameterValue(TEXT("Specular"),
			FMath::Lerp(0.24f, 0.72f, FMath::Clamp(Profile.Metallic, 0.0f, 1.0f)));
		Material->SetScalarParameterValue(TEXT("ClimateBlend"),
			FMath::Clamp(0.08f + Profile.ClimatePatchStrength * 0.17f,
				0.08f, 0.25f));
		return Material;
	}

	UMaterialInstanceDynamic* CreateLiquidMaterial(
		UObject* Outer, UMaterialInterface* BaseMaterial)
	{
		// The closed liquid shell must reveal terrain below it.  Keep this isolated
		// from WorldScape's near-field SingleLayerWater material, but require the
		// orbital asset's ordinary translucent blend contract.
		return IsValid(BaseMaterial)
			&& BaseMaterial->GetBlendMode() == BLEND_Translucent
			&& BaseMaterial->GetShadingModels().HasShadingModel(MSM_DefaultLit)
			? UMaterialInstanceDynamic::Create(BaseMaterial, Outer) : nullptr;
	}

	struct FMeshData
	{
		TArray<FVector> TerrainVertices;
		TArray<FVector> OceanVertices;
		TArray<int32> Indices;
		TArray<int32> OceanIndices;
		TArray<FVector> Normals;
		TArray<FVector> OceanNormals;
		TArray<FVector2D> UV0;
		TArray<FLinearColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;
	};

	struct FCubeFace
	{
		FVector Normal;
		FVector AxisU;
		FVector AxisV;
	};

	bool BuildClosedCubeSphere(UAPSWorldScapePlanetNoise* Noise,
		AWorldScapeRoot* ProfileRoot, const bool bBuildOcean,
		const double NormalReliefExaggeration, FMeshData& OutData)
	{
		if (!IsValid(Noise) || !IsValid(ProfileRoot)
			|| !FMath::IsFinite(ProfileRoot->PlanetScale)
			|| ProfileRoot->PlanetScale <= UE_SMALL_NUMBER
			|| !FMath::IsFinite(NormalReliefExaggeration)
			|| NormalReliefExaggeration <= 0.0)
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
		OutData.UV0.Reserve(TotalVertices);
		OutData.VertexColors.Reserve(TotalVertices);
		OutData.Tangents.Reserve(TotalVertices);
		TArray<FVector> NormalReferenceVertices;
		NormalReferenceVertices.Reserve(TotalVertices);

		const double Radius = ProfileRoot->PlanetScale;
		const double OceanRadius = Radius + ProfileRoot->OceanHeight;
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
		constexpr double MaximumNormalReliefFraction = 0.05;
		CustomNoise& SeededNoise = ProfileRoot->PlanetNoise;
		for (const FCubeFace& Face : Faces)
		{
			const int32 FaceStart = OutData.TerrainVertices.Num();
			TArray<float, TInlineAllocator<(FaceResolution + 1) * (FaceResolution + 1)>> FaceWaterMasks;
			if (bBuildOcean) FaceWaterMasks.Reserve(VerticesPerFace);
			for (int32 Y = 0; Y <= FaceResolution; ++Y)
			{
				const double V = -1.0 + 2.0 * static_cast<double>(Y) / FaceResolution;
				for (int32 X = 0; X <= FaceResolution; ++X)
				{
					const double U = -1.0 + 2.0 * static_cast<double>(X) / FaceResolution;
					const FVector Direction = (Face.Normal + Face.AxisU * U + Face.AxisV * V)
						.GetSafeNormal();
					DVector NoisePosition;
					const FNoiseData Surface = Noise->SampleResolved(
						SeededNoise, DVector(Direction * Radius), DVector(0.0, 0.0, 0.0),
						ProfileRoot->NoiseScale, ProfileRoot->NoiseIntensity, Radius,
						Direction.Z, NoisePosition);
					if (!FMath::IsFinite(Surface.Height)
						|| !FMath::IsFinite(Surface.HeightNormalize)
						|| !FMath::IsFinite(Surface.Temperature)
						|| !FMath::IsFinite(Surface.Humidity)
						|| !FMath::IsFinite(Surface.WaterMask))
					{
						return false;
					}

					OutData.TerrainVertices.Add(Direction * (Radius + Surface.Height));
					const double NormalReferenceHeight = FMath::Clamp(
						Surface.Height * NormalReliefExaggeration,
						-Radius * MaximumNormalReliefFraction,
						Radius * MaximumNormalReliefFraction);
					NormalReferenceVertices.Add(Direction * (Radius + NormalReferenceHeight));
					if (bBuildOcean)
					{
						OutData.OceanVertices.Add(Direction * OceanRadius);
						OutData.OceanNormals.Add(Direction);
						FaceWaterMasks.Add(FMath::Clamp(Surface.WaterMask, 0.0f, 1.0f));
					}
					OutData.Normals.Add(FVector::ZeroVector);
					OutData.UV0.Add(FVector2D(
						0.5 + FMath::Atan2(Direction.Y, Direction.X) / (2.0 * UE_PI),
						0.5 - FMath::Asin(Direction.Z) / UE_PI));
					// Alpha carries the resolver noise's authoritative liquid mask. Preview
					// water/lava and magmatic emissive consume the same mask, keeping orbital
					// coastlines consistent with the full-scale WorldScape profile.
					OutData.VertexColors.Add(FLinearColor(
						Surface.HeightNormalize, Surface.Temperature,
						Surface.Humidity, FMath::Clamp(Surface.WaterMask, 0.0f, 1.0f)));
					FVector Tangent = FVector::CrossProduct(FVector::UpVector, Direction)
						.GetSafeNormal();
					if (Tangent.IsNearlyZero())
					{
						Tangent = FVector::CrossProduct(FVector::RightVector, Direction)
							.GetSafeNormal();
					}
					OutData.Tangents.Add(FProcMeshTangent(Tangent, false));
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
						const auto AddOceanTriangle = [&OutData, &FaceWaterMasks](
							const int32 Global0, const int32 Global1, const int32 Global2,
							const int32 Local0, const int32 Local1, const int32 Local2)
						{
							// Centroid classification gives a clean, closed orbital ocean without
							// placing a translucent shell over dry land.  The terrain remains a
							// complete sphere beneath it, so shoreline cracks cannot reveal space.
							const float MeanWaterMask = (FaceWaterMasks[Local0]
								+ FaceWaterMasks[Local1] + FaceWaterMasks[Local2]) / 3.0f;
							if (MeanWaterMask > 0.01f)
							{
								OutData.OceanIndices.Add(Global0);
								OutData.OceanIndices.Add(Global1);
								OutData.OceanIndices.Add(Global2);
							}
						};
						AddOceanTriangle(A, D, B, LocalA, LocalD, LocalB);
						AddOceanTriangle(A, C, D, LocalA, LocalC, LocalD);
					}
				}
			}
		}

		// Accumulate area-weighted normals from the exaggerated reference surface.
		// The render indices intentionally use ProceduralMesh's clockwise front face,
		// so orient the mathematical cross product back toward the radial direction.
		TArray<FVector> AccumulatedNormals;
		AccumulatedNormals.Init(FVector::ZeroVector, TotalVertices);
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
		WeldedNormals.Reserve(TotalVertices);
		for (int32 VertexIndex = 0; VertexIndex < TotalVertices; ++VertexIndex)
		{
			WeldedNormals.FindOrAdd(MakeNormalWeldKey(
				NormalReferenceVertices[VertexIndex])) += AccumulatedNormals[VertexIndex];
		}
		constexpr float DetailedNormalWeight = 0.82f;
		for (int32 VertexIndex = 0; VertexIndex < TotalVertices; ++VertexIndex)
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
			FVector FinalNormal = DetailedNormal * DetailedNormalWeight
				+ RadialNormal * (1.0f - DetailedNormalWeight);
			if (!FinalNormal.Normalize())
			{
				FinalNormal = RadialNormal;
			}
			OutData.Normals[VertexIndex] = FinalNormal;
		}

		return OutData.TerrainVertices.Num() == TotalVertices
			&& OutData.Indices.Num() == TotalIndices
			&& OutData.Normals.Num() == TotalVertices
			&& (!bBuildOcean || (OutData.OceanVertices.Num() == TotalVertices
				&& OutData.OceanNormals.Num() == TotalVertices
				&& OutData.OceanIndices.Num() % 3 == 0));
	}
}

AAstroGenerator::AAstroGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	GenerationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GenerationRoot"));
	SetRootComponent(GenerationRoot);

	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(GenerationRoot);
	PreviewCamera->SetAbsolute(true, true, true);
	PreviewCamera->SetFieldOfView(55.0f);

	// Scope guides are real world-space shells. Keeping them as two stable
	// components avoids the camera-facing Slate circles that used to drift away
	// from off-centre star meshes and multiply into several apparent boundaries.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GuideSphereAsset(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GuideMaterialAsset(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	const auto CreatePreviewGuideShell = [this](const FName Name, const int32 SortPriority)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(GenerationRoot);
		Component->SetAbsolute(true, true, true);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
		Component->SetTranslucentSortPriority(SortPriority);
		Component->SetVisibility(false, true);
		Component->SetHiddenInGame(true, true);
		return Component;
	};
	PreviewStarInfluenceShell = CreatePreviewGuideShell(
		TEXT("PreviewStarInfluenceShell"), -8);
	PreviewSystemBoundaryShell = CreatePreviewGuideShell(
		TEXT("PreviewSystemBoundaryShell"), -12);
	if (GuideSphereAsset.Succeeded())
	{
		PreviewStarInfluenceShell->SetStaticMesh(GuideSphereAsset.Object);
		PreviewSystemBoundaryShell->SetStaticMesh(GuideSphereAsset.Object);
	}
	if (GuideMaterialAsset.Succeeded())
	{
		PreviewStarInfluenceShell->SetMaterial(0, GuideMaterialAsset.Object);
		PreviewSystemBoundaryShell->SetMaterial(0, GuideMaterialAsset.Object);
	}

	const auto CreatePreviewMesh = [this](const FName Name)
	{
		UProceduralMeshComponent* Component =
			CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		Component->SetupAttachment(GenerationRoot);
		Component->SetAbsolute(true, true, true);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->bUseAsyncCooking = false;
		Component->bUseComplexAsSimpleCollision = false;
		// Orbital materials own their bounded N.L presentation; shadow-map passes add
		// cost without contributing to the closed globe image.
		Component->SetCastShadow(false);
		Component->SetVisibility(false, true);
		Component->SetHiddenInGame(true, true);
		return Component;
	};
	PreviewTerrainProxyA = CreatePreviewMesh(TEXT("PreviewTerrainProxyA"));
	PreviewTerrainProxyB = CreatePreviewMesh(TEXT("PreviewTerrainProxyB"));
	PreviewOceanProxyA = CreatePreviewMesh(TEXT("PreviewOceanProxyA"));
	PreviewOceanProxyB = CreatePreviewMesh(TEXT("PreviewOceanProxyB"));
}

bool AAstroGenerator::ArePreviewMaterialAssetsWarmed() const
{
	return IsValid(PreviewTerrainBaseMaterial)
		&& IsValid(PreviewLiquidBaseMaterial)
		&& IsValid(PreviewWaterBaseMaterial)
		&& IsValid(PreviewAmmoniaBaseMaterial)
		&& IsValid(PreviewLavaBaseMaterial);
}

bool AAstroGenerator::WarmPreviewMaterialAssets()
{
	const auto LoadIfMissing = [](TObjectPtr<UMaterialInterface>& Asset,
		const TCHAR* ObjectPath)
	{
		if (!IsValid(Asset))
		{
			Asset = LoadObject<UMaterialInterface>(nullptr, ObjectPath);
		}
	};
	LoadIfMissing(PreviewTerrainBaseMaterial, APSPreviewGlobe::TerrainMaterialPath);
	LoadIfMissing(PreviewLiquidBaseMaterial, APSPreviewGlobe::LiquidMaterialPath);
	LoadIfMissing(PreviewWaterBaseMaterial,
		APSPreviewGlobe::GetLiquidMaterialPath(EAPSPlanetLiquidType::Water));
	LoadIfMissing(PreviewAmmoniaBaseMaterial,
		APSPreviewGlobe::GetLiquidMaterialPath(EAPSPlanetLiquidType::Ammonia));
	LoadIfMissing(PreviewLavaBaseMaterial,
		APSPreviewGlobe::GetLiquidMaterialPath(EAPSPlanetLiquidType::Lava));

	const bool bWarmed = ArePreviewMaterialAssetsWarmed();
	if (bWarmed && !bPreviewMaterialPSOPrecacheRequested && IsInGameThread())
	{
		// ProceduralMeshComponent does not expose material-specific collection in 5.4,
		// so submit its actual local-vertex-factory permutations directly. The engine
		// schedules these requests asynchronously; this path never waits or flushes.
		FPSOPrecacheParams PrecacheParams;
		if (IsValid(PreviewTerrainProxyA))
		{
			PreviewTerrainProxyA->SetupPrecachePSOParams(PrecacheParams);
		}
		PrecacheParams.bStaticLighting = false;
		PrecacheParams.bCastShadow = false;
		PrecacheParams.SetMobility(EComponentMobility::Movable);
		for (UMaterialInterface* Material : {
			PreviewTerrainBaseMaterial.Get(), PreviewLiquidBaseMaterial.Get(),
			PreviewWaterBaseMaterial.Get(), PreviewAmmoniaBaseMaterial.Get(),
			PreviewLavaBaseMaterial.Get() })
		{
			Material->PrecachePSOs(&FLocalVertexFactory::StaticType, PrecacheParams);
		}
		bPreviewMaterialPSOPrecacheRequested = true;
	}

	UE_CLOG(!bWarmed, LogTemp, Error,
		TEXT("[APS.WorldGeneration] Orbital preview material warmup incomplete terrain=%s liquid=%s water=%s ammonia=%s lava=%s"),
		*GetNameSafe(PreviewTerrainBaseMaterial), *GetNameSafe(PreviewLiquidBaseMaterial),
		*GetNameSafe(PreviewWaterBaseMaterial), *GetNameSafe(PreviewAmmoniaBaseMaterial),
		*GetNameSafe(PreviewLavaBaseMaterial));
	return bWarmed;
}

void AAstroGenerator::BeginPlay()
{
	Super::BeginPlay();

	// The menu owns one explicitly tagged, budgeted preview generator.  Legacy
	// generator actors serialized in the menu map must stay dormant; otherwise
	// they allocate a second 25k galaxy + 40k cluster before Slate even appears.
	// Gameplay and authored SinglePlay use a different GameMode and are untouched.
	if (bAutoGeneration && GetWorld()
		&& GetWorld()->GetAuthGameMode<AMainMenuGameModeBase>()
		&& !ActorHasTag(TEXT("WorldGenerationPreview")))
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldGeneration] Dormant legacy generator in MainMenu: %s"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== AAstroGenerator::BeginPlay START ==="));
	UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration: %s"), bAutoGeneration ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("StartPlanetNumber: %d"), StartPlanetNumber);

	if (bAutoGeneration)
	{
		UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration is true, initializing generators"));
		InitAstroGenerators();

		if (bIntegrateStartPlanet && WSR_StartHomePlanet)
		{
			// This is the legacy, authored SinglePlay contract. The placed
			// generator first builds its configured astronomical background and
			// home system, then integrates the serialized WorldScape/HQ hierarchy.
			// Menu previews use a separate tagged generator and never enter here.
			UE_LOG(LogTemp, Warning,
				TEXT("Single Play integration: running isolated legacy authored generation path"));
			InitLegacyAuthoredGenerationLevel();
			GenerateStarSystemAndIntegratePlanet();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Using normal astronomical generation path"));
			InitGenerationLevel();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("bAutoGeneration is false, skipping generation"));
	}

	UE_LOG(LogTemp, Warning, TEXT("=== AAstroGenerator::BeginPlay END ==="));
}

void AAstroGenerator::InitLegacyAuthoredGenerationLevel()
{
	// Keep the serialized SinglePlay generator equivalent to its pre-menu
	// sequence. Galaxy and StarCluster intentionally remain separate levels;
	// the modern preview is free to compose them on its own actor instance.
	bool bGeneratedHomeSystemAsPrimaryLevel = false;
	const bool bIntegrationWillGenerateHomeSystem = bIntegrateStartPlanet
		&& IsValid(WSR_StartHomePlanet);

	switch (AstroGenerationLevel)
	{
	case EAstroGenerationLevel::GalaxiesCluster:
		GenerateGalaxiesCluster();
		break;
	case EAstroGenerationLevel::Galaxy:
		GenerateGalaxy();
		break;
	case EAstroGenerationLevel::StarCluster:
		GenerateStarCluster();
		break;
	case EAstroGenerationLevel::StarSystem:
		if (!bIntegrationWillGenerateHomeSystem)
		{
			GenerateHomeStarSystem();
			bGeneratedHomeSystemAsPrimaryLevel = true;
		}
		break;
	case EAstroGenerationLevel::PlanetSystem:
		if (!bIntegrationWillGenerateHomeSystem)
		{
			GeneratePlanetSystem();
			bGeneratedHomeSystemAsPrimaryLevel = true;
		}
		break;
	case EAstroGenerationLevel::SinglePlanet:
		if (!bIntegrationWillGenerateHomeSystem)
		{
			GenerateSinglePlanet();
			bGeneratedHomeSystemAsPrimaryLevel = true;
		}
		break;
	default:
		GenerateRandomWorld();
		break;
	}

	if (bGenerateHomeSystem && !bGeneratedHomeSystemAsPrimaryLevel
		&& !bIntegrationWillGenerateHomeSystem)
	{
		GenerateHomeStarSystem();
	}
	if (bIntegrationWillGenerateHomeSystem)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldGeneration] Authored integration reserved exactly one home-system build"));
	}
}

void AAstroGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePreviewWorldScape();
	AWorldScapeRoot* PreviewSurface = PersistentPreviewWorldScapeRoot.Get();
	const bool bSurfaceJobsRunning = IsValid(PreviewSurface)
		&& PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
	const bool bSurfaceAwaitingReady = bPreviewSurfaceUpdatePending
		&& ActivePreviewWorldScapeBody.IsValid()
		&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(
			ActivePreviewWorldScapeBody->PlanetType)
		&& !ActivePreviewWorldScapeBody->bWorldScapeSurfaceReady;
	// A selected body is persistent state, not pending work. Keeping this actor alive
	// merely because PLANET remained selected cost a full game-thread tick forever.
	// A force-regeneration can spend one frame with no queued worker yet, so continue
	// polling until the new resolved payload has actually replaced the fallback.
	const bool bKeepSurfaceTick = bPreviewSurfaceUpdatePending || bSurfaceJobsRunning
		|| bSurfaceAwaitingReady;
	if (!bPreviewCameraTransitionActive || !PreviewCamera)
	{
		SetActorTickEnabled(bKeepSurfaceTick);
		return;
	}

	PreviewCameraTransitionElapsed += FMath::Max(DeltaSeconds, 0.0f);
	const float Alpha = FMath::Clamp(
		PreviewCameraTransitionElapsed / FMath::Max(PreviewCameraTransitionDuration, 0.01f), 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	FTransform BlendedTransform;
	BlendedTransform.Blend(PreviewCameraStartTransform, PreviewCameraTargetTransform, SmoothAlpha);
	PreviewCamera->SetWorldTransform(BlendedTransform);

	if (Alpha >= 1.0f)
	{
		bPreviewCameraTransitionActive = false;
		SetActorTickEnabled(bKeepSurfaceTick);
	}
}

void AAstroGenerator::GenerateWorldByModel()
{
	if (IsValid(GeneratedWorldModel))
	{
		PreviewGenerationSeed = FMath::Max(1, GeneratedWorldModel->GenerationSeed);
		FMath::RandInit(PreviewGenerationSeed);
	}
	InitAstroGenerators();

	ApplyWorldModel();

	ApplySpawnParameters();

	InitGenerationLevel();
	// Body overrides were already injected into planet/moon models before actor
	// creation. This second idempotent pass restores per-body atmosphere controls
	// that are not represented in the legacy generation structs.
	ApplyPreviewBodyEditOverrides(GeneratedWorldModel);
}

bool AAstroGenerator::RegeneratePreview(
	UGeneratedWorld* InGeneratedWorld, EAstroPreviewFocus RequestedFocus)
{
	if (!IsValid(InGeneratedWorld) || !GetWorld())
	{
		return false;
	}
	// The initial GALAXY/SYSTEM build is the astronomical page's warm-up window.
	// Keep all orbital materials resident now so a later PLANET/body click performs
	// only MID creation and mesh commit, never a synchronous package load.
	WarmPreviewMaterialAssets();
	PreviewFocus = RequestedFocus;
	PreviewGenerationSeed = FMath::Max(1, InGeneratedWorld->GenerationSeed);

	// Actor pointers cannot survive a structural preview rebuild. Preserve the
	// selected hierarchy address instead: generated star/planet/moon arrays are
	// deterministic for a given preview seed, so this resolves Star B/C and its
	// Planet N to the corresponding newly spawned actor after ClearGeneratedPreview.
	enum class EPreservedPreviewSelection : uint8 { None, Star, Planet, Moon };
	EPreservedPreviewSelection PreservedSelection = EPreservedPreviewSelection::None;
	int32 PreservedStarIndex = INDEX_NONE;
	int32 PreservedPlanetIndex = INDEX_NONE;
	int32 PreservedMoonIndex = INDEX_NONE;
	if (AActor* PreviousSelection = SelectedPreviewBodyActor.Get())
	{
		const TArray<AStar*>* PreviousStars = IsValid(GeneratedHomeStarSystem)
			? &GeneratedHomeStarSystem->GetStars() : nullptr;
		if (AStar* PreviousStar = Cast<AStar>(PreviousSelection))
		{
			PreservedSelection = EPreservedPreviewSelection::Star;
			PreservedStarIndex = PreviousStars
				? PreviousStars->IndexOfByKey(PreviousStar) : (PreviousStar == HomeStar ? 0 : INDEX_NONE);
		}
		else
		{
			APlanet* PreviousPlanet = Cast<APlanet>(PreviousSelection);
			if (AMoon* PreviousMoon = Cast<AMoon>(PreviousSelection))
			{
				PreviousPlanet = PreviousMoon->ParentPlanet;
				PreservedSelection = EPreservedPreviewSelection::Moon;
				PreservedMoonIndex = IsValid(PreviousPlanet)
					? PreviousPlanet->Moons.IndexOfByKey(PreviousMoon) : INDEX_NONE;
			}
			else if (IsValid(PreviousPlanet))
			{
				PreservedSelection = EPreservedPreviewSelection::Planet;
			}

			if (IsValid(PreviousPlanet))
			{
				AStar* PreviousParentStar = PreviousPlanet->ParentStar;
				PreservedStarIndex = PreviousStars && IsValid(PreviousParentStar)
					? PreviousStars->IndexOfByKey(PreviousParentStar)
					: (PreviousParentStar == HomeStar || PreviousPlanet == HomePlanet ? 0 : INDEX_NONE);
				if (IsValid(PreviousParentStar) && IsValid(PreviousParentStar->PlanetarySystem))
				{
					PreservedPlanetIndex = PreviousParentStar->PlanetarySystem
						->PlanetsActorsList.IndexOfByKey(PreviousPlanet);
				}
				else if (PreviousPlanet == HomePlanet)
				{
					PreservedPlanetIndex = 0;
				}
			}
		}
	}

	// Preserve the user's orbit direction and zoom as a ratio of the focused
	// object's radius. The regenerated actor can move and change size, so keeping
	// the old absolute camera transform is what caused the apparent camera jumps.
	FVector PreservedViewDirection = FVector(-1.0, -1.0, 0.45).GetSafeNormal();
	double PreservedDistanceRatio = 0.0;
	if (IsValid(PreviewCamera))
	{
		const FBox OldFocusBounds = GetPreviewFocusBounds(PreviewFocus);
		if (OldFocusBounds.IsValid)
		{
			const FVector OldOffset = PreviewCamera->GetComponentLocation() - OldFocusBounds.GetCenter();
			const double OldRadius = FMath::Max(
				static_cast<double>(OldFocusBounds.GetExtent().GetMax()), 500.0);
			if (!OldOffset.IsNearlyZero() && FMath::IsFinite(OldOffset.Size()))
			{
				PreservedViewDirection = OldOffset.GetSafeNormal();
				PreservedDistanceRatio = FMath::Clamp(OldOffset.Size() / OldRadius, 1.1, 30.0);
			}
		}
	}

	ClearGeneratedPreview();
	SetActorScale3D(FVector::OneVector);
	SetGeneratedWorld(InGeneratedWorld);

	InitAstroGenerators();
	ApplyWorldModel();

	const bool bSavedStarterLocation = bSpawnStarterLocation;
	const bool bSavedStarterPlanet = bSpawnStarterPlanet;
	const bool bSavedCharacterSpawn = bCharacterSpawn;
	bSpawnStarterLocation = false;
	bSpawnStarterPlanet = false;
	bCharacterSpawn = false;
	bIsPreviewGeneration = true;
	// Existing Blueprint CDOs can retain the old 10k/3k default even after the C++
	// default changes. Enforce the UI-only cap at runtime; committed gameplay
	// generation never enters this path.
	PreviewMaxInstances = FMath::Clamp(PreviewMaxInstances, 100, 1800);

	// A live editor must be deterministic: changing planet radius must not also
	// shuffle every star and orbit. Gameplay generation remains random because
	// this seed is applied exclusively inside the preview path.
	FMath::RandInit(PreviewGenerationSeed);

	InitGenerationLevel();
	// The hierarchy above is disposable. Restore edits by deterministic body path
	// before preview normalization, camera framing, or WorldScape presentation reads
	// the newly spawned actor/model values.
	ApplyPreviewBodyEditOverrides(InGeneratedWorld);

	// Full-scale generation deliberately places cluster instances and their materialized
	// star systems at astronomical coordinates. Keep one uniform transform for the whole
	// disposable hierarchy so every focus remains the same location at another zoom.
	// Bounds must be built from the logical catalog envelopes: component bounds are not
	// reliable until the HISM render trees finish registration, and using them here left
	// GALAXY thousands of times larger than the supposedly normalized preview.
	// The persisted model still has bGenerateFullScaledWorld and the gameplay level builds
	// the real full-scale hierarchy from it after Continue.
	constexpr double MaxSafePreviewRadius = 5.0e10; // Proven-safe rendered menu envelope.
	const auto GetLogicalPreviewBounds = [this]()
	{
		FBox LogicalBounds(EForceInit::ForceInit);
		for (const EAstroPreviewFocus LogicalFocus : {
			EAstroPreviewFocus::Galaxy,
			EAstroPreviewFocus::StarCluster,
			EAstroPreviewFocus::HomeSystem})
		{
			// A missing explicit focus falls back to the overview actor tree inside
			// GetPreviewFocusBounds.  That is useful for camera recovery, but it must
			// not participate in normalization: at STAR_SYSTEM level the unavailable
			// GALAXY/CLUSTER queries otherwise re-added the physical hypergiant mesh
			// and made the global root scale depend on stellar class.
			if (!IsPreviewFocusAvailable(LogicalFocus))
			{
				continue;
			}
			const FBox FocusBounds = GetPreviewFocusBounds(LogicalFocus);
			if (FocusBounds.IsValid)
			{
				LogicalBounds += FocusBounds;
			}
		}
		if (!LogicalBounds.IsValid)
		{
			LogicalBounds = GetPreviewFocusBounds(EAstroPreviewFocus::Overview);
		}
		return LogicalBounds;
	};
	const FBox FullScalePreviewBounds = GetLogicalPreviewBounds();
	if (FullScalePreviewBounds.IsValid)
	{
		const FVector PreviewCenter = FullScalePreviewBounds.GetCenter();
		const double PreviewRadius = FullScalePreviewBounds.GetExtent().GetMax();
		if (!PreviewCenter.ContainsNaN() && FMath::IsFinite(PreviewRadius)
			&& PreviewRadius > MaxSafePreviewRadius)
		{
			const double PreviewScaleFactor = MaxSafePreviewRadius / PreviewRadius;
			SetActorScale3D(GetActorScale3D() * PreviewScaleFactor);
			// The selected HISM instance is already zero-scaled at this point. Keep its
			// cached world presentation radius in the same normalized preview space as
			// the materialized hierarchy which replaced it.
			if (FMath::IsFinite(MaterializedHomeProxyWorldRadius)
				&& MaterializedHomeProxyWorldRadius > 0.0)
			{
				MaterializedHomeProxyWorldRadius *= PreviewScaleFactor;
			}
			const FBox NormalizedBounds = GetLogicalPreviewBounds();
			const double NormalizedRadius = NormalizedBounds.IsValid
				? NormalizedBounds.GetExtent().GetMax() : 0.0;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.WorldGeneration] Normalized live preview radius %.3e -> %.3e cm (factor %.3e)"),
				PreviewRadius, NormalizedRadius, PreviewScaleFactor);
			if (!NormalizedBounds.IsValid || !FMath::IsFinite(NormalizedRadius)
				|| NormalizedRadius > MaxSafePreviewRadius * 1.01)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.WorldGeneration] Rejected unsafe live preview after normalization"));
				ClearGeneratedPreview();
			}
		}
		else if (PreviewCenter.ContainsNaN() || !FMath::IsFinite(PreviewRadius))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Rejected non-finite live preview bounds before camera focus"));
			ClearGeneratedPreview();
		}
	}

	AActor* RestoredSelection = nullptr;
	AStar* RestoredStar = nullptr;
	if (IsValid(GeneratedHomeStarSystem))
	{
		const TArray<AStar*>& NewStars = GeneratedHomeStarSystem->GetStars();
		if (NewStars.IsValidIndex(PreservedStarIndex))
		{
			RestoredStar = NewStars[PreservedStarIndex];
		}
		else if (PreservedStarIndex == 0)
		{
			RestoredStar = HomeStar;
		}
	}
	if (PreservedSelection == EPreservedPreviewSelection::Star)
	{
		RestoredSelection = RestoredStar;
	}
	else if (PreservedSelection == EPreservedPreviewSelection::Planet
		|| PreservedSelection == EPreservedPreviewSelection::Moon)
	{
		APlanet* RestoredPlanet = nullptr;
		if (IsValid(RestoredStar) && IsValid(RestoredStar->PlanetarySystem)
			&& RestoredStar->PlanetarySystem->PlanetsActorsList.IsValidIndex(PreservedPlanetIndex))
		{
			RestoredPlanet = RestoredStar->PlanetarySystem->PlanetsActorsList[PreservedPlanetIndex];
		}
		else if (PreservedStarIndex == 0 && PreservedPlanetIndex == 0)
		{
			RestoredPlanet = HomePlanet;
		}
		if (PreservedSelection == EPreservedPreviewSelection::Moon
			&& IsValid(RestoredPlanet) && RestoredPlanet->Moons.IsValidIndex(PreservedMoonIndex))
		{
			RestoredSelection = RestoredPlanet->Moons[PreservedMoonIndex];
		}
		else
		{
			RestoredSelection = RestoredPlanet;
		}
	}
	SelectedPreviewBodyActor = IsValid(RestoredSelection)
		? RestoredSelection
		: RequestedFocus == EAstroPreviewFocus::HomeStar
			? Cast<AActor>(HomeStar)
			: RequestedFocus == EAstroPreviewFocus::HomePlanet ? Cast<AActor>(HomePlanet) : nullptr;

	// This actor remains the dedicated transient preview generator after the build.
	// Clearing the flag here made every local surface/atmosphere edit reject its
	// fast path and fall back to a full galaxy/system rebuild.
	bIsPreviewGeneration = true;
	bSpawnStarterLocation = bSavedStarterLocation;
	bSpawnStarterPlanet = bSavedStarterPlanet;
	bCharacterSpawn = bSavedCharacterSpawn;
	ApplyPreviewFocusPresentation(PreviewFocus);
	// Parameter edits may deliberately preserve the current camera instead of
	// calling FocusPreviewTarget again. Re-apply the selected surface to the newly
	// rebuilt hierarchy so PLANET never falls back to a featureless sphere after
	// changing type, radius or atmosphere.
	SetPreviewWorldScapeBody(PreviewFocus == EAstroPreviewFocus::HomePlanet
		? Cast<APlanetaryBody>(SelectedPreviewBodyActor.Get()) : nullptr);
	if (PreservedDistanceRatio > 0.0 && IsValid(PreviewCamera))
	{
		const FBox NewFocusBounds = GetPreviewFocusBounds(PreviewFocus);
		if (NewFocusBounds.IsValid)
		{
			const FVector NewCenter = NewFocusBounds.GetCenter();
			const double NewRadius = FMath::Max(
				static_cast<double>(NewFocusBounds.GetExtent().GetMax()), 500.0);
			const double NewDistance = NewRadius * PreservedDistanceRatio;
			const FVector NewLocation = NewCenter + PreservedViewDirection * NewDistance;
			PreviewOrbitCenter = NewCenter;
			PreviewOrbitDistance = NewDistance;
			PreviewCameraStartTransform = PreviewCamera->GetComponentTransform();
			PreviewCameraTargetTransform = FTransform(
				(NewCenter - NewLocation).Rotation(), NewLocation);
			PreviewCameraTransitionElapsed = 0.0f;
			PreviewCameraTransitionDuration = 0.28f;
			bPreviewCameraTransitionActive = true;
			SetActorTickEnabled(true);
		}
	}

	// The view model applies the requested focus after generation. Starting a
	// transition here as well used to restart the same blend twice and produced
	// a visible camera kick whenever a live parameter changed.
	return IsValid(GeneratedStarCluster) || IsValid(GeneratedGalaxy) || IsValid(GeneratedHomeStarSystem);
}

void AAstroGenerator::ClearGeneratedPreview()
{
	SetPreviewWorldScapeBody(nullptr);
	SetPreviewGuideShellVisible(PreviewStarInfluenceShell, false);
	SetPreviewGuideShellVisible(PreviewSystemBoundaryShell, false);
	ClearPreviewGlobeProxyCache();
	ResetPreviewBackgroundContextCache();
	PreviewBodyPresentationCenters.Reset();
	PreviewBodyPresentationRadii.Reset();
	SelectedPreviewClusterSystemIndex = INDEX_NONE;
	SelectedPreviewBodyActor.Reset();
	TArray<AActor*> GeneratedRoots;
	GeneratedRoots.AddUnique(GeneratedStarCluster);
	GeneratedRoots.AddUnique(GeneratedGalaxy);
	GeneratedRoots.AddUnique(GeneratedHomeStarSystem);
	GeneratedRoots.AddUnique(GeneratedWorld);

	for (AActor* GeneratedRoot : GeneratedRoots)
	{
		if (IsValid(GeneratedRoot) && GeneratedRoot != this)
		{
			DestroyActorTree(GeneratedRoot);
		}
	}

	GeneratedStarCluster = nullptr;
	GeneratedGalaxy = nullptr;
	GeneratedHomeStarSystem = nullptr;
	GeneratedStarSystems.Reset();
	GeneratedWorld = nullptr;
	HomePlanetarySystem = nullptr;
	HomeStar = nullptr;
	HomePlanet = nullptr;
	MaterializedHomeProxyWorldRadius = 0.0;
	HomeStarMeshBaseRelativeTransform = FTransform::Identity;
	bHasHomeStarMeshBaseRelativeTransform = false;
	StarIndexModelMap.Reset();
}

void AAstroGenerator::FocusPreviewCamera(APlayerController* PlayerController)
{
	FocusPreviewTarget(PreviewFocus, PlayerController);
}

bool AAstroGenerator::TryGetPreviewClusterSystemSphere(
	int32 InstanceIndex, FVector& OutCenter, double& OutRadius) const
{
	if (!IsValid(GeneratedStarCluster) || !IsValid(GeneratedStarCluster->StarMeshInstances))
	{
		return false;
	}
	const FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystem(InstanceIndex);
	FTransform WorldTransform;
	if (!Record || !GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(
		InstanceIndex, WorldTransform, true)
		|| WorldTransform.GetScale3D().GetAbsMax() <= UE_SMALL_NUMBER)
	{
		return false;
	}

	double ProxyRadius = 500.0;
	if (const UStaticMesh* StarMesh = GeneratedStarCluster->StarMeshInstances->GetStaticMesh())
	{
		ProxyRadius = FMath::Max(static_cast<double>(StarMesh->GetBounds().SphereRadius)
			* WorldTransform.GetScale3D().GetAbsMax(), ProxyRadius);
	}
	OutCenter = GeneratedStarCluster->GetPotentialSystemWorldLocation(*Record);
	OutRadius = FMath::Clamp(ProxyRadius * 10.0, 500.0, 2.0e6);
	return !OutCenter.ContainsNaN() && FMath::IsFinite(OutRadius);
}

bool AAstroGenerator::GetPreviewSystemPresentationSphere(
	FVector& OutCenter, double& OutRadius, double* OutPositionScale) const
{
	if (!IsValid(GeneratedHomeStarSystem))
	{
		return false;
	}

	// The gameplay safe zone is allowed to react to the physical stellar model.
	// The menu frame is not: a Hypergiant must not turn the complete system into a
	// one-pixel dot. Derive the preview envelope from actual barycentric star/planet
	// positions and add a small visual clearance beyond the last orbit. No actor,
	// orbit or generated radius is modified by this presentation-only calculation.
	OutCenter = GeneratedHomeStarSystem->GetActorLocation();
	double OuterOrbitDistance = 0.0;
	for (const AStar* Star : GeneratedHomeStarSystem->GetStars())
	{
		if (!IsValid(Star))
		{
			continue;
		}
		const double StarDistance = FVector::Distance(OutCenter, Star->GetActorLocation());
		if (FMath::IsFinite(StarDistance))
		{
			OuterOrbitDistance = FMath::Max(OuterOrbitDistance, StarDistance);
		}
		if (!IsValid(Star->PlanetarySystem))
		{
			continue;
		}
		for (const APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
		{
			if (!IsValid(Planet))
			{
				continue;
			}
			const double PlanetDistance = FVector::Distance(
				OutCenter, Planet->GetActorLocation());
			if (FMath::IsFinite(PlanetDistance))
			{
				OuterOrbitDistance = FMath::Max(OuterOrbitDistance, PlanetDistance);
			}
		}
	}

	double RawPresentationRadius = 0.0;
	if (OuterOrbitDistance > UE_SMALL_NUMBER)
	{
		// Ten percent is enough to separate the boundary from the outer orbit while
		// keeping the system legible and stable across stellar-class extremes.
		RawPresentationRadius = FMath::Max(OuterOrbitDistance * 1.10, 500.0);
	}
	else
	{
		double PhysicalRadius = IsValid(GeneratedHomeStarSystem->StarSystemZone)
			? GeneratedHomeStarSystem->StarSystemZone->GetScaledSphereRadius() : 0.0;
		if (!FMath::IsFinite(PhysicalRadius) || PhysicalRadius <= UE_SMALL_NUMBER)
		{
			PhysicalRadius = GeneratedHomeStarSystem->StarSystemRadius
				* GeneratedHomeStarSystem->GetActorTransform().GetScale3D().GetAbsMax();
		}
		if (!FMath::IsFinite(PhysicalRadius) || PhysicalRadius <= UE_SMALL_NUMBER)
		{
			PhysicalRadius = 8.0e6;
		}
		// Empty/single-body systems have no orbit envelope. Frame them against the
		// fixed 2.5e6-cm STAR presentation instead of the unbounded physical class.
		RawPresentationRadius = FMath::Clamp(PhysicalRadius, 6.0e6, 1.2e7);
	}

	// Preserve a reference across rebuilds that changed only stellar/spectral class.
	// Layout edits and explicit regeneration produce a new signature and are free to
	// establish a genuinely different preview size. Extreme stellar classes instead
	// receive a bounded mesh/orbit presentation scale while gameplay data stays exact.
	uint32 LayoutSignature = GetTypeHash(PreviewGenerationSeed);
	if (IsValid(GeneratedWorldModel))
	{
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(GeneratedWorldModel->GenerationSeed));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(static_cast<uint8>(GeneratedWorldModel->AstroGenerationLevel)));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(GeneratedWorldModel->bGenerateFullScaledWorld));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(static_cast<uint8>(GeneratedWorldModel->StarType)));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(static_cast<uint8>(GeneratedWorldModel->PlanetarySystemType)));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(static_cast<uint8>(GeneratedWorldModel->OrbitDistributionType)));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(GeneratedWorldModel->PlanetsAmount));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(GeneratedWorldModel->MoonsAmount));
		LayoutSignature = HashCombineFast(LayoutSignature,
			GetTypeHash(GeneratedWorldModel->PlanetRadius));
		// Parent galaxy/cluster presentation settings never change the child system
		// layout.  Including them here made a STAR-only rebuild invalidate this cache
		// after ApplyWorldModel refreshed unrelated parent defaults, so giant classes
		// briefly established a new reference and collapsed SYSTEM framing.
	}
	// This cache must be covariant with the one uniform preview-root transform.
	// Full-scale generation first builds at astronomical scale and RegeneratePreview
	// then shrinks the complete attached hierarchy into a renderer-safe envelope.
	// Storing the reference in world centimetres caused the first bounds query to
	// cache the pre-normalized radius; the validation query then clamped against that
	// stale value and rejected an otherwise uniformly scaled hierarchy.  Root-space
	// storage also survives subsequent rebuilds, which temporarily restore root scale
	// to one before generating the same layout again.
	const double PreviewRootWorldScale = FMath::Max(
		GetActorTransform().GetScale3D().GetAbsMax(), UE_DOUBLE_SMALL_NUMBER);
	const double RawPresentationRadiusInRootSpace =
		RawPresentationRadius / PreviewRootWorldScale;
	if (PreviewSystemLayoutSignature != LayoutSignature
		|| !FMath::IsFinite(PreviewSystemReferenceRadiusInRootSpace)
		|| PreviewSystemReferenceRadiusInRootSpace <= UE_SMALL_NUMBER)
	{
		PreviewSystemLayoutSignature = LayoutSignature;
		PreviewSystemReferenceRadiusInRootSpace = RawPresentationRadiusInRootSpace;
	}
	const double ReferenceWorldRadius =
		PreviewSystemReferenceRadiusInRootSpace * PreviewRootWorldScale;
	const double MinimumStableRadius = ReferenceWorldRadius * 0.72;
	const double MaximumStableRadius = ReferenceWorldRadius * 1.38;
	OutRadius = FMath::Clamp(
		RawPresentationRadius, MinimumStableRadius, MaximumStableRadius);
	if (OutPositionScale)
	{
		*OutPositionScale = OutRadius
			/ FMath::Max(RawPresentationRadius, UE_DOUBLE_SMALL_NUMBER);
	}
	return !OutCenter.ContainsNaN() && FMath::IsFinite(OutRadius)
		&& OutRadius > UE_SMALL_NUMBER;
}

FBox AAstroGenerator::GetPreviewFocusBounds(EAstroPreviewFocus Focus) const
{
	FBox Bounds(EForceInit::ForceInit);
	if (Focus == EAstroPreviewFocus::HomeSystem
		&& SelectedPreviewClusterSystemIndex != INDEX_NONE)
	{
		FVector ProxyCenter;
		double ProxyRadius = 0.0;
		if (TryGetPreviewClusterSystemSphere(
			SelectedPreviewClusterSystemIndex, ProxyCenter, ProxyRadius))
		{
			return FBox(ProxyCenter - FVector(ProxyRadius), ProxyCenter + FVector(ProxyRadius));
		}
	}
	const auto AddVisibleActorComponents = [&Bounds](const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}
		TInlineComponentArray<UPrimitiveComponent*> Components;
		Actor->GetComponents(Components);
		for (const UPrimitiveComponent* Component : Components)
		{
			if (IsValid(Component) && Component->IsRegistered() && Component->IsVisible()
				&& !Component->bHiddenInGame && Component->Bounds.SphereRadius > UE_SMALL_NUMBER)
			{
				Bounds += Component->Bounds.GetBox();
			}
		}
	};
	const auto AddVisibleActorTree = [&AddVisibleActorComponents](const AActor* RootActor)
	{
		TArray<const AActor*> Pending;
		TSet<const AActor*> Visited;
		if (IsValid(RootActor)) Pending.Add(RootActor);
		while (Pending.Num() > 0)
		{
			const AActor* Actor = Pending.Pop(EAllowShrinking::No);
			if (!IsValid(Actor) || Visited.Contains(Actor))
			{
				continue;
			}
			Visited.Add(Actor);
			AddVisibleActorComponents(Actor);
			TArray<AActor*> Children;
			// This loop owns the recursion. Asking GetAttachedActors for the complete
			// descendant set at every node revisits the same planet/moon hierarchy many
			// times and makes focus-bound queries scale quadratically.
			Actor->GetAttachedActors(Children, true, false);
			for (const AActor* Child : Children) if (IsValid(Child)) Pending.Add(Child);
		}
	};
	const AActor* FocusActor = nullptr;
	switch (Focus)
	{
	case EAstroPreviewFocus::StarCluster: FocusActor = GeneratedStarCluster; break;
	case EAstroPreviewFocus::Galaxy: FocusActor = GeneratedGalaxy; break;
	case EAstroPreviewFocus::HomeSystem: FocusActor = GeneratedHomeStarSystem; break;
	case EAstroPreviewFocus::HomeStar:
		FocusActor = SelectedPreviewBodyActor.IsValid() && SelectedPreviewBodyActor->IsA<AStar>()
			? SelectedPreviewBodyActor.Get() : HomeStar;
		break;
	case EAstroPreviewFocus::HomePlanet:
		FocusActor = SelectedPreviewBodyActor.IsValid() && SelectedPreviewBodyActor->IsA<APlanetaryBody>()
			? SelectedPreviewBodyActor.Get() : HomePlanet;
		break;
	case EAstroPreviewFocus::Overview:
	default: break;
	}

	if (Focus == EAstroPreviewFocus::Galaxy && IsValid(GeneratedGalaxy)
		&& !GeneratedGalaxy->StarCatalog.CatalogHalfExtent.IsNearlyZero())
	{
		// Use the logical catalog extent, not the extrema of the current HISM LOD.
		// Otherwise changing the render budget visibly changes the galaxy boundary.
		const FVector Extent = GeneratedGalaxy->StarCatalog.CatalogHalfExtent;
		return FBox(-Extent, Extent).TransformBy(GeneratedGalaxy->GetActorTransform());
	}
	if (Focus == EAstroPreviewFocus::StarCluster && IsValid(GeneratedStarCluster))
	{
		if (!GeneratedStarCluster->ClusterBounds.IsNearlyZero())
		{
			// Camera and boundary use the logical, symmetric formation envelope. HISM
			// extrema depend on the render budget and on the hidden materialized-home
			// instance, which previously shifted Ring/Arc framing between regenerations.
			// ClusterBounds is authored in generator units and star positions use * 100.
			const FVector HalfExtent = GeneratedStarCluster->ClusterBounds.GetAbs() * 50.0;
			const FTransform ClusterTransform = IsValid(GeneratedStarCluster->StarMeshInstances)
				? GeneratedStarCluster->StarMeshInstances->GetComponentTransform()
				: GeneratedStarCluster->GetActorTransform();
			return FBox(-HalfExtent, HalfExtent).TransformBy(ClusterTransform);
		}
		// Legacy assets without logical bounds still get a useful visible fallback.
		AddVisibleActorComponents(GeneratedStarCluster);
		return Bounds;
	}
	if (Focus == EAstroPreviewFocus::HomeSystem && IsValid(GeneratedHomeStarSystem))
	{
		FVector Center;
		double Radius = 0.0;
		if (GetPreviewSystemPresentationSphere(Center, Radius))
		{
			return FBox(Center - FVector(Radius), Center + FVector(Radius));
		}
	}
	if (IsValid(FocusActor))
	{
		// STAR and PLANET are object-level inspections. Their system/orbits/moons
		// are attached children, so recursively including them made these buttons
		// frame almost exactly the same bounds as SYSTEM.
		if (Focus == EAstroPreviewFocus::HomeStar)
		{
			// AStar also owns PlanetarySystemZone. It is a valid primitive whose bounds
			// can be several orbital systems wide, but it is not the visible star. Using
			// generic actor bounds here was the reason STAR sometimes flew kilometres
			// away or framed the entire system. Only the rendered stellar mesh owns this
			// scope and the orange influence guide.
			const AStar* FocusStar = Cast<AStar>(FocusActor);
			if (IsValid(FocusStar) && IsValid(FocusStar->StarMesh)
				&& FocusStar->StarMesh->IsRegistered()
				&& FocusStar->StarMesh->Bounds.SphereRadius > UE_SMALL_NUMBER)
			{
				// The guide must be centred on what is actually rendered. Several Blueprint
				// star meshes have a non-zero relative pivot, so the actor origin can sit below
				// the luminous sphere even though it remains the orbital barycentre.
				FocusStar->StarMesh->UpdateBounds();
				const FVector Center = FocusStar->StarMesh->Bounds.Origin;
				const double Radius = FocusStar->StarMesh->Bounds.SphereRadius;
				Bounds = FBox(Center - FVector(Radius), Center + FVector(Radius));
			}
			if (!Bounds.IsValid)
			{
				constexpr double FallbackVisibleStarRadius = 2.5e6;
				const FVector Center = FocusActor->GetActorLocation();
				Bounds = FBox(Center - FVector(FallbackVisibleStarRadius),
					Center + FVector(FallbackVisibleStarRadius));
			}
			return Bounds;
		}
		if (Focus == EAstroPreviewFocus::HomePlanet)
		{
			// Collision/gravity zones are intentionally much larger than the globe and
			// must never affect PLANET camera distance. Both the fallback mesh and the
			// persistent WorldScape proxy are normalized to this presentation radius.
			constexpr double PlanetPresentationRadius = 1.2e6;
			constexpr double AtmosphereFramePadding = 1.12;
			FVector Center = FocusActor->GetActorLocation();
			GetPreviewPresentationLocation(FocusActor, Center);
			double Radius = PlanetPresentationRadius * AtmosphereFramePadding;
			APlanet* FocusPlanet = Cast<APlanet>(const_cast<AActor*>(FocusActor));
			if (const AMoon* FocusMoon = Cast<AMoon>(FocusActor))
			{
				FocusPlanet = FocusMoon->ParentPlanet;
			}
			const auto IncludePresentedBody = [this, &Center, &Radius](const APlanetaryBody* Body)
			{
				if (!IsValid(Body)) return;
				const TWeakObjectPtr<AActor> Key(const_cast<APlanetaryBody*>(Body));
				const FVector* BodyCenter = PreviewBodyPresentationCenters.Find(Key);
				const double* BodyRadius = PreviewBodyPresentationRadii.Find(Key);
				if (BodyCenter && BodyRadius && FMath::IsFinite(*BodyRadius))
				{
					Radius = FMath::Max(Radius,
						FVector::Distance(Center, *BodyCenter) + *BodyRadius * 1.05);
				}
			};
			IncludePresentedBody(FocusPlanet);
			if (IsValid(FocusPlanet))
			{
				for (const AMoon* Moon : FocusPlanet->Moons)
				{
					IncludePresentedBody(Moon);
				}
			}
			return FBox(Center - FVector(Radius), Center + FVector(Radius));
		}

		AddVisibleActorTree(FocusActor);
		return Bounds;
	}

	const AActor* PreviewRoots[] = {GeneratedStarCluster, GeneratedGalaxy, GeneratedHomeStarSystem, GeneratedWorld};
	for (const AActor* PreviewRoot : PreviewRoots)
	{
		if (IsValid(PreviewRoot))
		{
			AddVisibleActorTree(PreviewRoot);
		}
	}
	return Bounds;
}

bool AAstroGenerator::GetPreviewFocusSphere(EAstroPreviewFocus Focus, FVector& OutCenter, double& OutRadius) const
{
	const FBox Bounds = GetPreviewFocusBounds(Focus);
	if (!Bounds.IsValid)
	{
		return false;
	}
	OutCenter = Bounds.GetCenter();
	OutRadius = FMath::Max(static_cast<double>(Bounds.GetExtent().GetMax()), 500.0);
	return !OutCenter.ContainsNaN() && FMath::IsFinite(OutRadius);
}

bool AAstroGenerator::GetPreviewGuideShellState(EAstroPreviewFocus GuideFocus,
	FVector& OutCenter, double& OutRadius, bool& bOutVisible) const
{
	UStaticMeshComponent* Shell = GuideFocus == EAstroPreviewFocus::HomeStar
		? PreviewStarInfluenceShell
		: GuideFocus == EAstroPreviewFocus::HomeSystem
			? PreviewSystemBoundaryShell : nullptr;
	if (!IsValid(Shell) || !IsValid(Shell->GetStaticMesh()))
	{
		bOutVisible = false;
		return false;
	}
	Shell->UpdateBounds();
	OutCenter = Shell->Bounds.Origin;
	OutRadius = Shell->Bounds.SphereRadius;
	bOutVisible = Shell->IsVisible() && !Shell->bHiddenInGame;
	return !OutCenter.ContainsNaN() && FMath::IsFinite(OutRadius)
		&& OutRadius > UE_SMALL_NUMBER;
}

bool AAstroGenerator::GetPreviewPresentationLocation(
	const AActor* Actor, FVector& OutLocation) const
{
	if (!IsValid(Actor))
	{
		return false;
	}
	const TWeakObjectPtr<AActor> Key(const_cast<AActor*>(Actor));
	if (const FVector* PresentationCenter = PreviewBodyPresentationCenters.Find(Key))
	{
		OutLocation = *PresentationCenter;
		return !OutLocation.ContainsNaN();
	}
	OutLocation = Actor->GetActorLocation();
	return !OutLocation.ContainsNaN();
}

void AAstroGenerator::StartPreviewCameraTransition(const FVector& Center, double Radius,
	APlayerController* PlayerController)
{
	if (!PreviewCamera || !GetWorld())
	{
		return;
	}
	if (Center.ContainsNaN() || !FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Ignored invalid preview camera target center=%s radius=%.3e"),
			*Center.ToString(), Radius);
		return;
	}

	const double HalfFovRadians = FMath::DegreesToRadians(PreviewCamera->FieldOfView * 0.5);
	// Slate side panels occupy roughly half the viewport. Frame against the central
	// preview safe area, otherwise outer planets and labels are technically on-screen
	// but hidden under the controls and appear to be missing.
	constexpr double PreviewHorizontalFraction = 0.50;
	constexpr double SafeFramePadding = 1.10;
	const double EffectiveHalfFovTangent = FMath::Max(
		FMath::Tan(HalfFovRadians) * PreviewHorizontalFraction, 0.05);
	const double Distance = FMath::Max(
		Radius * SafeFramePadding / EffectiveHalfFovTangent, Radius * 1.35);
	// Preserve the user's current viewing direction while changing hierarchy scale.
	// Resetting to a hard-coded vector made every SYSTEM/CLUSTER/PLANET switch jerk
	// sideways and, for an already-built WorldScape planet, exposed an ungenerated
	// hemisphere. PreviewOrbitCenter is the authoritative centre of the current view,
	// so the camera offset remains meaningful even while a previous blend is active.
	FVector CameraOffsetDirection =
		(PreviewCamera->GetComponentLocation() - PreviewOrbitCenter).GetSafeNormal();
	if (CameraOffsetDirection.IsNearlyZero() || CameraOffsetDirection.ContainsNaN())
	{
		CameraOffsetDirection = FVector(1.0, 1.0, -0.45).GetSafeNormal();
	}
	const FVector CameraLocation = Center + CameraOffsetDirection * Distance;
	if (CameraLocation.ContainsNaN() || !FMath::IsFinite(Distance))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Ignored non-finite preview camera transform"));
		return;
	}

	PreviewOrbitCenter = Center;
	PreviewOrbitDistance = Distance;
	PreviewCameraStartTransform = PreviewCamera->GetComponentTransform();
	PreviewCameraTargetTransform = FTransform((Center - CameraLocation).Rotation(), CameraLocation);
	PreviewCameraTransitionElapsed = 0.0f;
	PreviewCameraTransitionDuration = 0.55f;
	bPreviewCameraTransitionActive = true;
	SetActorTickEnabled(true);
	PreviewCamera->SetActive(true);

	UE_LOG(LogTemp, Log,
		TEXT("[APS.Preview.Camera] focus=%s center=%s radius=%.3e distance=%.3e"),
		*UEnum::GetValueAsString(PreviewFocus), *Center.ToString(), Radius, Distance);

	APlayerController* ResolvedController = PlayerController ? PlayerController : GetWorld()->GetFirstPlayerController();
	if (ResolvedController && ResolvedController->GetViewTarget() != this)
	{
		ResolvedController->SetViewTargetWithBlend(this, 0.35f, VTBlend_Cubic);
	}
}

void AAstroGenerator::FocusPreviewTarget(EAstroPreviewFocus NewFocus, APlayerController* PlayerController)
{
	// Explicit hierarchy buttons always return to the generated home hierarchy.
	// A concrete HISM system selection is retained only while it is the active
	// double-click target.
	SelectedPreviewClusterSystemIndex = INDEX_NONE;
	PreviewFocus = NewFocus;
	SelectedPreviewBodyActor = NewFocus == EAstroPreviewFocus::HomeStar
		? Cast<AActor>(HomeStar)
		: NewFocus == EAstroPreviewFocus::HomePlanet ? Cast<AActor>(HomePlanet) : nullptr;
	if (NewFocus == EAstroPreviewFocus::HomePlanet && !IsValid(HomePlanet))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] PLANET focus rejected: generated HomePlanet target is null"));
	}
	ApplyPreviewFocusPresentation(NewFocus);
	SetPreviewWorldScapeBody(NewFocus == EAstroPreviewFocus::HomePlanet
		? Cast<APlanetaryBody>(HomePlanet) : nullptr);
	FBox Bounds = GetPreviewFocusBounds(NewFocus);
	if (!Bounds.IsValid)
	{
		Bounds = FBox(GetActorLocation() - FVector(500.0), GetActorLocation() + FVector(500.0));
	}
	StartPreviewCameraTransition(
		Bounds.GetCenter(), FMath::Max(static_cast<double>(Bounds.GetExtent().GetMax()), 500.0), PlayerController);
}

void AAstroGenerator::ResetPreviewBackgroundContextCache()
{
	PreviewGalaxyContextOwner.Reset();
	PreviewClusterContextOwner.Reset();
	PreviewGalaxyBaseInstanceScales.Reset();
	PreviewClusterBaseInstanceScales.Reset();
	PreviewGalaxyBaseInstanceEmissions.Reset();
	PreviewClusterBaseInstanceEmissions.Reset();
	bPreviewBackgroundCullStateInitialized = false;
	bPreviewBackgroundCullApplied = false;
	PreviewBackgroundContextFocus = EAstroPreviewFocus::Overview;
	PreviewBackgroundVisualScale = 1.0;
	PreviewBackgroundCullCenter = FVector::ZeroVector;
	PreviewBackgroundCullRadius = 0.0;
	PreviewBackgroundDetailCenter = FVector::ZeroVector;
	PreviewBackgroundEmissionCap = TNumericLimits<float>::Max();
}

void AAstroGenerator::ApplyPreviewBackgroundContext(EAstroPreviewFocus NewFocus)
{
	UHierarchicalInstancedStaticMeshComponent* GalaxyHism =
		IsValid(GeneratedGalaxy) ? GeneratedGalaxy->StarMeshInstances : nullptr;
	UHierarchicalInstancedStaticMeshComponent* ClusterHism =
		IsValid(GeneratedStarCluster) ? GeneratedStarCluster->StarMeshInstances : nullptr;

	const auto CaptureBaseScales = [](
		UHierarchicalInstancedStaticMeshComponent* Component,
		TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>& CachedOwner,
		TArray<FVector>& CachedScales, TArray<float>& CachedEmissions)
	{
		const int32 InstanceCount = IsValid(Component) ? Component->GetInstanceCount() : 0;
		if (CachedOwner.Get() == Component && CachedScales.Num() == InstanceCount
			&& CachedEmissions.Num() == InstanceCount)
		{
			return false;
		}

		CachedOwner = Component;
		CachedScales.Reset(InstanceCount);
		CachedScales.SetNumZeroed(InstanceCount);
		CachedEmissions.Reset(InstanceCount);
		CachedEmissions.SetNumZeroed(InstanceCount);
		for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; ++InstanceIndex)
		{
			FTransform LocalTransform;
			if (Component->GetInstanceTransform(InstanceIndex, LocalTransform, false))
			{
				CachedScales[InstanceIndex] = LocalTransform.GetScale3D();
			}
			const int32 CustomDataOffset = InstanceIndex * Component->NumCustomDataFloats + 3;
			CachedEmissions[InstanceIndex] = Component->NumCustomDataFloats > 3
				&& Component->PerInstanceSMCustomData.IsValidIndex(CustomDataOffset)
				? Component->PerInstanceSMCustomData[CustomDataOffset] : 0.0f;
		}
		return true;
	};

	const bool bCacheChanged =
		CaptureBaseScales(GalaxyHism, PreviewGalaxyContextOwner,
			PreviewGalaxyBaseInstanceScales, PreviewGalaxyBaseInstanceEmissions)
		| CaptureBaseScales(ClusterHism, PreviewClusterContextOwner,
			PreviewClusterBaseInstanceScales, PreviewClusterBaseInstanceEmissions);
	const bool bDeepMaterializedScope = bIsPreviewGeneration
		&& SelectedPreviewClusterSystemIndex == INDEX_NONE
		&& (NewFocus == EAstroPreviewFocus::HomeSystem
			|| NewFocus == EAstroPreviewFocus::HomeStar
			|| NewFocus == EAstroPreviewFocus::HomePlanet);
	// A hierarchy transition is a camera move through one world, not a presentation
	// rebuild. Keep catalogue positions, instance count and base scales invariant across
	// GALAXY -> CLUSTER -> SYSTEM -> STAR -> PLANET. Detail scopes may derive a smaller
	// mesh-only LOD scale below so a far-view proxy cannot become a giant near-view disc;
	// the immutable base scale is restored whenever the camera returns to a parent scope.
	constexpr double ContextVisualScale = 1.0;

	FVector SafeCenter = FVector::ZeroVector;
	double SafeRadius = 0.0;
	const bool bApplyCull = bDeepMaterializedScope
		&& GetPreviewFocusSphere(EAstroPreviewFocus::HomeSystem, SafeCenter, SafeRadius)
		&& FMath::IsFinite(SafeRadius) && SafeRadius > UE_SMALL_NUMBER;
	FVector DetailFocusCenter = SafeCenter;
	if (bDeepMaterializedScope && SelectedPreviewBodyActor.IsValid())
	{
		GetPreviewPresentationLocation(SelectedPreviewBodyActor.Get(), DetailFocusCenter);
	}
	if (DetailFocusCenter.ContainsNaN())
	{
		DetailFocusCenter = SafeCenter;
	}
	// Galaxy/cluster proxies are authored to remain readable from their parent
	// scopes. At a planet-sized camera distance those same world radii become huge
	// glowing discs (and can visually cover the materialized system even when their
	// centres are correctly outside its safe zone). Keep every catalogue address
	// untouched, but cap the mesh-only proxy radius to a small angular footprint for
	// the current detail focus before applying the safe-zone cull.
	double MaxProxyAngularRadiusRadians = 0.0;
	float MaxProxyEmission = TNumericLimits<float>::Max();
	switch (NewFocus)
	{
	case EAstroPreviewFocus::HomeSystem:
		MaxProxyAngularRadiusRadians = FMath::DegreesToRadians(0.10);
		MaxProxyEmission = 24.0f;
		break;
	case EAstroPreviewFocus::HomeStar:
		MaxProxyAngularRadiusRadians = FMath::DegreesToRadians(0.065);
		MaxProxyEmission = 12.0f;
		break;
	case EAstroPreviewFocus::HomePlanet:
		MaxProxyAngularRadiusRadians = FMath::DegreesToRadians(0.050);
		MaxProxyEmission = 6.0f;
		break;
	default:
		break;
	}
	const bool bSameCull = bPreviewBackgroundCullStateInitialized
		&& bPreviewBackgroundCullApplied == bApplyCull
		&& PreviewBackgroundContextFocus == NewFocus
		&& FMath::IsNearlyEqual(PreviewBackgroundVisualScale, ContextVisualScale, 1.0e-6)
		&& PreviewBackgroundDetailCenter.Equals(DetailFocusCenter, 1.0)
		&& FMath::IsNearlyEqual(PreviewBackgroundEmissionCap, MaxProxyEmission)
		&& (!bApplyCull || (PreviewBackgroundCullCenter.Equals(SafeCenter, 1.0)
			&& FMath::IsNearlyEqual(PreviewBackgroundCullRadius, SafeRadius, 1.0)));
	if (!bCacheChanged && bSameCull)
	{
		return;
	}

	const auto ApplyComponentCull = [&](UHierarchicalInstancedStaticMeshComponent* Component,
		const TArray<FVector>& BaseScales, const TArray<float>& BaseEmissions)
	{
		if (!IsValid(Component) || BaseScales.Num() != Component->GetInstanceCount()
			|| BaseEmissions.Num() != Component->GetInstanceCount())
		{
			return;
		}

		const UStaticMesh* ProxyMesh = Component->GetStaticMesh();
		const double MeshRadius = IsValid(ProxyMesh)
			? FMath::Max(static_cast<double>(ProxyMesh->GetBounds().SphereRadius), 1.0) : 1.0;
		const FVector ComponentWorldScale =
			Component->GetComponentTransform().GetScale3D().GetAbs();
		bool bAnyRenderDataChanged = false;
		for (int32 InstanceIndex = 0; InstanceIndex < BaseScales.Num(); ++InstanceIndex)
		{
			FTransform LocalTransform;
			if (!Component->GetInstanceTransform(InstanceIndex, LocalTransform, false))
			{
				continue;
			}

			FVector DesiredScale = BaseScales[InstanceIndex] * ContextVisualScale;
			const float DesiredEmission = bDeepMaterializedScope
				? FMath::Min(BaseEmissions[InstanceIndex], MaxProxyEmission)
				: BaseEmissions[InstanceIndex];
			if (bApplyCull && !DesiredScale.IsNearlyZero())
			{
				const FVector WorldLocation = Component->GetComponentTransform().TransformPosition(
					LocalTransform.GetLocation());
				if (!WorldLocation.ContainsNaN())
				{
					// The current instance transform may already be zero-scaled by the
					// previous cull pass. Radius must therefore come from the immutable
					// captured scale composed with the HISM component's world scale.
					const FVector BaseWorldScale = DesiredScale.GetAbs() * ComponentWorldScale;
					double ProxyWorldRadius = MeshRadius
						* BaseWorldScale.GetAbsMax();
					const double DetailDistance = FVector::Distance(
						WorldLocation, DetailFocusCenter);
					if (MaxProxyAngularRadiusRadians > 0.0
						&& FMath::IsFinite(DetailDistance) && DetailDistance > UE_SMALL_NUMBER
						&& FMath::IsFinite(ProxyWorldRadius) && ProxyWorldRadius > UE_SMALL_NUMBER)
					{
						const double AngularRadiusLimit = DetailDistance
							* FMath::Tan(MaxProxyAngularRadiusRadians);
						if (FMath::IsFinite(AngularRadiusLimit) && AngularRadiusLimit > UE_SMALL_NUMBER
							&& ProxyWorldRadius > AngularRadiusLimit)
						{
							const double AngularScale = AngularRadiusLimit / ProxyWorldRadius;
							DesiredScale *= AngularScale;
							ProxyWorldRadius = AngularRadiusLimit;
						}
					}
					const double SurfaceDistance = FVector::Distance(
						WorldLocation, SafeCenter) - ProxyWorldRadius;
					if (SurfaceDistance <= SafeRadius * 1.10)
					{
						DesiredScale = FVector::ZeroVector;
					}
				}
			}

			if (!LocalTransform.GetScale3D().Equals(DesiredScale, UE_KINDA_SMALL_NUMBER))
			{
				LocalTransform.SetScale3D(DesiredScale);
				Component->UpdateInstanceTransform(
					InstanceIndex, LocalTransform, false, false, true);
				bAnyRenderDataChanged = true;
			}
			const int32 CustomDataOffset = InstanceIndex * Component->NumCustomDataFloats + 3;
			const float CurrentEmission = Component->NumCustomDataFloats > 3
				&& Component->PerInstanceSMCustomData.IsValidIndex(CustomDataOffset)
				? Component->PerInstanceSMCustomData[CustomDataOffset] : 0.0f;
			if (Component->NumCustomDataFloats > 3
				&& !FMath::IsNearlyEqual(CurrentEmission, DesiredEmission, 1.0e-4f))
			{
				Component->SetCustomDataValue(
					InstanceIndex, 3, DesiredEmission, false);
				bAnyRenderDataChanged = true;
			}
		}
		if (bAnyRenderDataChanged)
		{
			Component->BuildTreeIfOutdated(true, true);
			Component->MarkRenderStateDirty();
		}
	};

	ApplyComponentCull(GalaxyHism, PreviewGalaxyBaseInstanceScales,
		PreviewGalaxyBaseInstanceEmissions);
	ApplyComponentCull(ClusterHism, PreviewClusterBaseInstanceScales,
		PreviewClusterBaseInstanceEmissions);
	bPreviewBackgroundCullStateInitialized = true;
	bPreviewBackgroundCullApplied = bApplyCull;
	PreviewBackgroundContextFocus = NewFocus;
	PreviewBackgroundVisualScale = ContextVisualScale;
	PreviewBackgroundCullCenter = SafeCenter;
	PreviewBackgroundCullRadius = SafeRadius;
	PreviewBackgroundDetailCenter = DetailFocusCenter;
	PreviewBackgroundEmissionCap = MaxProxyEmission;
	UE_LOG(LogTemp, Verbose,
		TEXT("[APS.Preview.Context] scope=%s galaxy=%d cluster=%d cull=%d radius=%.3e visualScale=%.3f angularCapDeg=%.3f emissionCap=%.2f"),
		*UEnum::GetValueAsString(NewFocus), IsValid(GalaxyHism) ? GalaxyHism->GetInstanceCount() : 0,
		IsValid(ClusterHism) ? ClusterHism->GetInstanceCount() : 0,
		bApplyCull ? 1 : 0, SafeRadius, ContextVisualScale,
		FMath::RadiansToDegrees(MaxProxyAngularRadiusRadians), MaxProxyEmission);
}

void AAstroGenerator::SetPreviewGuideShellVisible(
	UStaticMeshComponent* Shell, const bool bVisible)
{
	if (!IsValid(Shell))
	{
		return;
	}
	Shell->SetVisibility(bVisible, true);
	Shell->SetHiddenInGame(!bVisible, true);
	if (bVisible)
	{
		Shell->MarkRenderStateDirty();
	}
}

void AAstroGenerator::UpdatePreviewGuideShells(EAstroPreviewFocus NewFocus)
{
	SetPreviewGuideShellVisible(PreviewStarInfluenceShell, false);
	SetPreviewGuideShellVisible(PreviewSystemBoundaryShell, false);
	if (SelectedPreviewClusterSystemIndex != INDEX_NONE
		|| (NewFocus != EAstroPreviewFocus::HomeSystem
			&& NewFocus != EAstroPreviewFocus::HomeStar))
	{
		return;
	}

	const auto EnsureShellMaterial = [this](UStaticMeshComponent* Shell,
		UMaterialInstanceDynamic*& Material, const FLinearColor& Color)
	{
		if (!IsValid(Shell))
		{
			return;
		}
		if (!IsValid(Material))
		{
			UMaterialInterface* ParentMaterial = Shell->GetMaterial(0);
			if (IsValid(ParentMaterial))
			{
				Material = UMaterialInstanceDynamic::Create(ParentMaterial, this);
				if (IsValid(Material))
				{
					Shell->SetMaterial(0, Material);
				}
			}
		}
		if (IsValid(Material))
		{
			// The engine debug material routes the Color alpha into opacity. Also set
			// the conventional scalar name for compatible project overrides.
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetScalarParameterValue(TEXT("Opacity"), Color.A);
		}
	};
	EnsureShellMaterial(PreviewStarInfluenceShell, PreviewStarInfluenceMaterial,
		FLinearColor(1.0f, 0.34f, 0.035f, 0.055f));
	EnsureShellMaterial(PreviewSystemBoundaryShell, PreviewSystemBoundaryMaterial,
		FLinearColor(1.0f, 0.055f, 0.025f, 0.030f));

	const auto PresentShell = [this](UStaticMeshComponent* Shell,
		const FVector& Center, const double Radius)
	{
		if (!IsValid(Shell) || !IsValid(Shell->GetStaticMesh())
			|| Center.ContainsNaN() || !FMath::IsFinite(Radius)
			|| Radius <= UE_SMALL_NUMBER)
		{
			SetPreviewGuideShellVisible(Shell, false);
			return;
		}
		const double MeshRadius = Shell->GetStaticMesh()->GetBounds().SphereRadius;
		if (!FMath::IsFinite(MeshRadius) || MeshRadius <= UE_SMALL_NUMBER)
		{
			SetPreviewGuideShellVisible(Shell, false);
			return;
		}
		Shell->SetAbsolute(true, true, true);
		Shell->SetWorldLocation(Center);
		Shell->SetWorldRotation(FRotator::ZeroRotator);
		Shell->SetWorldScale3D(FVector(Radius / MeshRadius));
		Shell->UpdateBounds();
		// Be exact even if an engine/project sphere asset is later replaced with an
		// off-centre mesh. The shell and its semantic target must share one address.
		const FVector CenterDelta = Center - Shell->Bounds.Origin;
		if (!CenterDelta.ContainsNaN() && !CenterDelta.IsNearlyZero(0.01))
		{
			Shell->SetWorldLocation(Shell->GetComponentLocation() + CenterDelta);
			Shell->UpdateBounds();
		}
		SetPreviewGuideShellVisible(Shell, true);
	};

	AStar* SelectedStar = Cast<AStar>(SelectedPreviewBodyActor.Get());
	SelectedStar = IsValid(SelectedStar) ? SelectedStar : HomeStar;
	if (IsValid(SelectedStar) && IsValid(SelectedStar->StarMesh)
		&& SelectedStar->StarMesh->IsVisible())
	{
		SelectedStar->StarMesh->UpdateBounds();
		const double InfluenceRadius = SelectedStar->StarMesh->Bounds.SphereRadius * 1.50;
		PresentShell(PreviewStarInfluenceShell,
			SelectedStar->StarMesh->Bounds.Origin, InfluenceRadius);
	}

	if (NewFocus == EAstroPreviewFocus::HomeSystem)
	{
		FVector SystemCenter;
		double SystemRadius = 0.0;
		if (GetPreviewSystemPresentationSphere(SystemCenter, SystemRadius))
		{
			PresentShell(PreviewSystemBoundaryShell, SystemCenter, SystemRadius);
		}
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("[APS.Preview.Guides] focus=%s star=%s system=%s"),
		*UEnum::GetValueAsString(NewFocus),
		IsValid(PreviewStarInfluenceShell) && PreviewStarInfluenceShell->IsVisible()
			? TEXT("visible") : TEXT("hidden"),
		IsValid(PreviewSystemBoundaryShell) && PreviewSystemBoundaryShell->IsVisible()
			? TEXT("visible") : TEXT("hidden"));
}

void AAstroGenerator::ApplyPreviewFocusPresentation(EAstroPreviewFocus NewFocus)
{
	if (!bIsPreviewGeneration && !IsValid(GeneratedHomeStarSystem))
	{
		return;
	}
	// Presentation centres are rebuilt transactionally for every focus. Their mesh
	// offsets are never allowed to become model/actor transforms.
	PreviewBodyPresentationCenters.Reset();
	PreviewBodyPresentationRadii.Reset();
	SetPreviewGuideShellVisible(PreviewStarInfluenceShell, false);
	SetPreviewGuideShellVisible(PreviewSystemBoundaryShell, false);
	// Blueprint component defaults can override the C++ constructor flags. These
	// collision/safe-zone primitives are data only; the two dedicated translucent
	// preview shells below own the visible STAR/SYSTEM boundaries.
	if (IsValid(GeneratedHomeStarSystem) && IsValid(GeneratedHomeStarSystem->StarSystemZone))
	{
		GeneratedHomeStarSystem->StarSystemZone->SetVisibility(false, true);
		GeneratedHomeStarSystem->StarSystemZone->SetHiddenInGame(true, true);
	}

	// Every hierarchy level keeps the same world coordinates and the same parent
	// HISM context. Focus changes are genuine camera zooms, not unrelated scenes.
	// Intrusive proxy stars inside the materialized system's safe envelope are
	// presentation-culled below without deleting/reindexing model records.
	if (IsValid(GeneratedGalaxy) && IsValid(GeneratedGalaxy->StarMeshInstances))
	{
		GeneratedGalaxy->SetActorHiddenInGame(false);
		// The HISM is also the Blueprint root of the nested preview hierarchy. Never
		// propagate its presentation visibility into attached system data components:
		// doing so re-enabled StarSystemZone after it had just been hidden.
		GeneratedGalaxy->StarMeshInstances->SetVisibility(true, false);
		GeneratedGalaxy->StarMeshInstances->SetHiddenInGame(false, false);
	}
	if (IsValid(GeneratedStarCluster) && IsValid(GeneratedStarCluster->StarMeshInstances))
	{
		GeneratedStarCluster->SetActorHiddenInGame(false);
		GeneratedStarCluster->StarMeshInstances->SetVisibility(true, false);
		GeneratedStarCluster->StarMeshInstances->SetHiddenInGame(false, false);
	}

	// Physical radii and orbital positions remain untouched in the generated model.
	// Only mesh-relative scale is adjusted for the disposable menu preview so a
	// realistic star does not collapse into a one-pixel point in the SYSTEM view.
	TArray<AActor*> Pending;
	TArray<AStar*> Stars;
	TArray<APlanetaryBody*> Bodies;
	TSet<AActor*> Visited;
	if (IsValid(GeneratedHomeStarSystem))
	{
		Pending.Add(GeneratedHomeStarSystem);
	}
	while (Pending.Num() > 0)
	{
		AActor* Actor = Pending.Pop(EAllowShrinking::No);
		if (!IsValid(Actor) || Visited.Contains(Actor))
		{
			continue;
		}
		Visited.Add(Actor);
		// The generated preview hierarchy is static between explicit UI mutations.
		// Dozens of hidden planets/moons previously kept ticking their AtmoScape
		// actors and rewriting five dynamic materials every frame.
		if (bIsPreviewGeneration && IsValid(Actor))
		{
			Actor->SetActorTickEnabled(false);
		}
		if (AStar* Star = Cast<AStar>(Actor))
		{
			Stars.Add(Star);
			Star->SetActorHiddenInGame(false);
			if (IsValid(Star->PlanetarySystemZone))
			{
				Star->PlanetarySystemZone->SetVisibility(false, true);
				Star->PlanetarySystemZone->SetHiddenInGame(true, true);
			}
			if (IsValid(Star->StarMesh))
			{
				// Focus changes are stateful. PLANET and STAR can hide meshes below,
				// therefore every presentation pass must first restore the complete
				// generated hierarchy before applying the new focus filter.
				Star->StarMesh->SetVisibility(true, true);
				Star->StarMesh->SetHiddenInGame(false, true);
				if (Star == HomeStar)
				{
					if (!bHasHomeStarMeshBaseRelativeTransform)
					{
						HomeStarMeshBaseRelativeTransform = Star->StarMesh->GetRelativeTransform();
						bHasHomeStarMeshBaseRelativeTransform = true;
					}
					else
					{
						// Galaxy/cluster temporarily place this component on the hidden HISM
						// address. Restore the authored star-local transform before every
						// deeper presentation pass.
						Star->StarMesh->SetRelativeTransform(HomeStarMeshBaseRelativeTransform);
					}
				}
				else
				{
					Star->StarMesh->SetRelativeScale3D(FVector::OneVector);
				}
				Star->StarMesh->UpdateBounds();
			}
		}
		else if (APlanetaryBody* Body = Cast<APlanetaryBody>(Actor))
		{
			Bodies.Add(Body);
			Body->SetActorHiddenInGame(false);
			TInlineComponentArray<UStaticMeshComponent*> Meshes;
			Body->GetComponents(Meshes);
			for (UStaticMeshComponent* Mesh : Meshes)
			{
				if (IsValid(Mesh))
				{
					Mesh->SetVisibility(true, true);
					Mesh->SetRelativeScale3D(FVector::OneVector);
					Mesh->UpdateBounds();
					// PLANET may have moved this mesh to a readability-only satellite
					// centre. Restore the semantic actor centre before applying the next
					// scope so SYSTEM/STAR always use the physical hierarchy again.
					const FVector CenterDelta = Body->GetActorLocation() - Mesh->Bounds.Origin;
					if (!CenterDelta.ContainsNaN() && !CenterDelta.IsNearlyZero(0.01))
					{
						Mesh->SetWorldLocation(Mesh->GetComponentLocation() + CenterDelta);
						Mesh->UpdateBounds();
					}
				}
			}
			if (IsValid(Body->PlanetaryEnvironmentGenerator)
				&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
			{
				AAtmoScape* BodyAtmosphere =
					Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
				BodyAtmosphere->SetActorHiddenInGame(true);
				BodyAtmosphere->SetActorTickEnabled(false);
			}
		}
		TArray<AActor*> AttachedChildren;
		// Recursion is owned by Pending. Requesting all descendants here caused the
		// same actors to be processed once per ancestor on every focus transition.
		Actor->GetAttachedActors(AttachedChildren, true, false);
		Pending.Append(AttachedChildren);
	}

	const auto SetBodyVisible = [](APlanetaryBody* Body, bool bVisible,
		bool bShowSelectedAtmosphere = false)
	{
		if (!IsValid(Body)) return;
		TInlineComponentArray<UStaticMeshComponent*> Meshes;
		Body->GetComponents(Meshes);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			if (IsValid(Mesh)) Mesh->SetVisibility(bVisible, true);
		}
		if (IsValid(Body->PlanetaryEnvironmentGenerator)
			&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
		{
			AAtmoScape* BodyAtmosphere =
				Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
			BodyAtmosphere->SetActorHiddenInGame(!bShowSelectedAtmosphere);
			BodyAtmosphere->SetActorTickEnabled(false);
		}
	};
	if (NewFocus == EAstroPreviewFocus::HomeSystem
		&& SelectedPreviewClusterSystemIndex != INDEX_NONE)
	{
		// A lightweight cluster record has no materialized body hierarchy yet. Keep
		// the cluster point field at its exact world location and hide the unrelated
		// authored home system instead of compositing both scopes.
		for (AStar* Star : Stars)
		{
			if (IsValid(Star->StarMesh)) Star->StarMesh->SetVisibility(false, true);
		}
		for (APlanetaryBody* Body : Bodies) SetBodyVisible(Body, false);
		if (IsValid(GeneratedHomeStarSystem)
			&& IsValid(GeneratedHomeStarSystem->StarSystemZone))
		{
			GeneratedHomeStarSystem->StarSystemZone->SetVisibility(false, true);
			GeneratedHomeStarSystem->StarSystemZone->SetHiddenInGame(true, true);
		}
		return;
	}

	const bool bUseDistantHomeSystemProxy =
		(NewFocus == EAstroPreviewFocus::Galaxy
			|| NewFocus == EAstroPreviewFocus::StarCluster);
	if (bUseDistantHomeSystemProxy)
	{
		// The home cluster HISM sample is the distant barycentric representation of
		// the materialized system. Never move a real star component onto that address:
		// in a binary/triple hierarchy the primary actor legitimately lives away from
		// the barycentre, and restoring it at SYSTEM caused a visible teleport while
		// its actor-backed label stayed elsewhere.
		for (AStar* Star : Stars)
		{
			if (IsValid(Star->StarMesh))
			{
				Star->StarMesh->SetVisibility(false, true);
			}
		}
		for (APlanetaryBody* Body : Bodies) SetBodyVisible(Body, false);
	}
	else if (NewFocus == EAstroPreviewFocus::HomeStar)
	{
		AStar* SelectedStar = Cast<AStar>(SelectedPreviewBodyActor.Get());
		SelectedStar = IsValid(SelectedStar) ? SelectedStar : HomeStar;
		for (AStar* Star : Stars)
		{
			if (IsValid(Star->StarMesh)) Star->StarMesh->SetVisibility(Star == SelectedStar, true);
		}
		for (APlanetaryBody* Body : Bodies) SetBodyVisible(Body, false);
	}
	else if (NewFocus == EAstroPreviewFocus::HomePlanet)
	{
		APlanetaryBody* SelectedBody = Cast<APlanetaryBody>(SelectedPreviewBodyActor.Get());
		SelectedBody = IsValid(SelectedBody) ? SelectedBody : HomePlanet;
		APlanet* FocusPlanet = Cast<APlanet>(SelectedBody);
		if (AMoon* FocusMoon = Cast<AMoon>(SelectedBody)) FocusPlanet = FocusMoon->ParentPlanet;
		// A planet focus changes only the camera orbit target and which planetary
		// family is editable. The complete generated system remains present at the
		// exact same actor coordinates as SYSTEM; PLANET only changes presentation
		// scale and the camera target. This also keeps sibling planets/moons available
		// as context instead of turning PLANET into an unrelated isolated scene.
		for (APlanetaryBody* Body : Bodies)
		{
			SetBodyVisible(Body, true, Body == SelectedBody);
		}
	}

	const auto ScaleMeshToWorldRadius = [](UStaticMeshComponent* Mesh,
		double DesiredWorldRadius, const FVector& DesiredWorldCenter)
	{
		if (!IsValid(Mesh) || !FMath::IsFinite(DesiredWorldRadius)
			|| DesiredWorldRadius <= 0.0 || DesiredWorldCenter.ContainsNaN())
		{
			return;
		}
		Mesh->UpdateBounds();
		const double CurrentRadius = Mesh->Bounds.SphereRadius;
		if (FMath::IsFinite(CurrentRadius) && CurrentRadius > UE_SMALL_NUMBER)
		{
			// Resolve to the requested presentation radius in both directions.  The old
			// lower bound of 1.0 made focus changes enlarge meshes permanently: returning
			// from STAR/PLANET to SYSTEM could never shrink them again, so bounds, camera
			// distance and WorldScape presentation scale all drifted apart.
			const double Factor = FMath::Clamp(
				DesiredWorldRadius / CurrentRadius, 1.0e-12, 1.0e12);
			Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D() * Factor);
			Mesh->UpdateBounds();
			// Blueprint star meshes are not guaranteed to have a centred asset pivot.
			// Scaling around that pivot moves the luminous bounds while the semantic
			// actor/orbit/label centre remains fixed. Re-centre the rendered bounds after
			// every presentation scale so guide, marker and star share one world address.
			const FVector CenterDelta = DesiredWorldCenter - Mesh->Bounds.Origin;
			if (!CenterDelta.ContainsNaN() && !CenterDelta.IsNearlyZero(0.01))
			{
				Mesh->SetWorldLocation(Mesh->GetComponentLocation() + CenterDelta);
				Mesh->UpdateBounds();
			}
		}
	};

	if (NewFocus == EAstroPreviewFocus::HomeSystem && Stars.Num() > 0)
	{
		FVector SystemCenter = GeneratedHomeStarSystem->GetActorLocation();
		double SystemRadius = 500.0;
		double SystemPositionScale = 1.0;
		GetPreviewSystemPresentationSphere(
			SystemCenter, SystemRadius, &SystemPositionScale);
		SystemPositionScale = FMath::Clamp(SystemPositionScale, 1.0e-9, 1.0e9);
		const auto SystemPresentationCenter = [SystemCenter, SystemPositionScale](
			const AActor* Actor)
		{
			return IsValid(Actor)
				? SystemCenter + (Actor->GetActorLocation() - SystemCenter) * SystemPositionScale
				: SystemCenter;
		};

		// Only rendered components move. Actor/model/orbit transforms remain the exact
		// generated full-scale hierarchy consumed by gameplay. This bounded transform
		// is what prevents stellar-class extremes from blowing apart the menu frame.
		for (APlanetaryBody* Body : Bodies)
		{
			if (!IsValid(Body))
			{
				continue;
			}
			const FVector DesiredCenter = SystemPresentationCenter(Body);
			const FVector PresentationDelta = DesiredCenter - Body->GetActorLocation();
			TInlineComponentArray<UStaticMeshComponent*> Meshes;
			Body->GetComponents(Meshes);
			double PresentedRadius = 0.0;
			for (UStaticMeshComponent* Mesh : Meshes)
			{
				if (!IsValid(Mesh)) continue;
				Mesh->SetWorldLocation(Mesh->GetComponentLocation() + PresentationDelta);
				Mesh->UpdateBounds();
				PresentedRadius = FMath::Max(
					PresentedRadius, static_cast<double>(Mesh->Bounds.SphereRadius));
			}
			const TWeakObjectPtr<AActor> Key(Body);
			PreviewBodyPresentationCenters.Add(Key, DesiredCenter);
			if (PresentedRadius > UE_SMALL_NUMBER)
			{
				PreviewBodyPresentationRadii.Add(Key, PresentedRadius);
			}
		}
		double LargestPhysicalRadiusKm = 1.0;
		for (const AStar* Star : Stars)
		{
			if (IsValid(Star))
			{
				LargestPhysicalRadiusKm = FMath::Max(
					LargestPhysicalRadiusKm, static_cast<double>(FMath::Max(Star->StarRadiusKM, 1)));
			}
		}
		const double LargestPresentationRadius = FMath::Clamp(
			SystemRadius * 0.085, 500.0, FMath::Max(SystemRadius * 0.11, 500.0));
		for (AStar* Star : Stars)
		{
			if (!IsValid(Star) || !IsValid(Star->StarMesh))
			{
				continue;
			}

			// Preserve the generated Double/Triple hierarchy instead of flattening all
			// components to the same screen-space diameter.
			const double RelativePhysicalRadius = static_cast<double>(
				FMath::Max(Star->StarRadiusKM, 1)) / LargestPhysicalRadiusKm;
			double DesiredRadius = LargestPresentationRadius * RelativePhysicalRadius;
			const FVector DesiredStarCenter = SystemPresentationCenter(Star);

			// A presentation mesh may never consume its own safe envelope or reach the
			// centre of the innermost planet. A 0.45 radius factor leaves at least ten
			// percent of the limiting diameter clear around the star.
			double LimitingRadius = SystemRadius;
			if (IsValid(Star->PlanetarySystemZone))
			{
				const double StellarSafeRadius =
					Star->PlanetarySystemZone->GetScaledSphereRadius();
				if (FMath::IsFinite(StellarSafeRadius) && StellarSafeRadius > UE_SMALL_NUMBER)
				{
					LimitingRadius = FMath::Min(
						LimitingRadius, StellarSafeRadius * SystemPositionScale);
				}
			}
			if (IsValid(Star->PlanetarySystem))
			{
				for (const APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
				{
					if (!IsValid(Planet)) continue;
					const double OrbitRadius = FVector::Distance(
						DesiredStarCenter, SystemPresentationCenter(Planet));
					if (FMath::IsFinite(OrbitRadius) && OrbitRadius > UE_SMALL_NUMBER)
					{
						LimitingRadius = FMath::Min(LimitingRadius, OrbitRadius);
					}
				}
			}
			const double SystemEdgeClearance = SystemRadius - FVector::Distance(
				SystemCenter, DesiredStarCenter);
			if (FMath::IsFinite(SystemEdgeClearance) && SystemEdgeClearance > UE_SMALL_NUMBER)
			{
				LimitingRadius = FMath::Min(LimitingRadius, SystemEdgeClearance);
			}
			DesiredRadius = FMath::Min(
				DesiredRadius, FMath::Max(LimitingRadius * 0.45, UE_DOUBLE_SMALL_NUMBER));
			ScaleMeshToWorldRadius(
				Star->StarMesh, DesiredRadius, DesiredStarCenter);
			const TWeakObjectPtr<AActor> Key(Star);
			PreviewBodyPresentationCenters.Add(Key, DesiredStarCenter);
			PreviewBodyPresentationRadii.Add(Key, DesiredRadius);
		}
		UE_LOG(LogTemp, Verbose,
			TEXT("[APS.Preview.SystemClamp] center=%s radius=%.3e positionScale=%.6f reference=%.3e"),
			*SystemCenter.ToString(), SystemRadius, SystemPositionScale,
			PreviewSystemReferenceRadiusInRootSpace
				* GetActorTransform().GetScale3D().GetAbsMax());
	}
	else if (NewFocus == EAstroPreviewFocus::HomeStar)
	{
		AStar* SelectedStar = Cast<AStar>(SelectedPreviewBodyActor.Get());
		SelectedStar = IsValid(SelectedStar) ? SelectedStar : HomeStar;
		if (IsValid(SelectedStar))
		{
			ScaleMeshToWorldRadius(
				SelectedStar->StarMesh, 2.5e6, SelectedStar->GetActorLocation());
		}
	}
	else if (NewFocus == EAstroPreviewFocus::HomePlanet)
	{
		APlanetaryBody* SelectedBody = Cast<APlanetaryBody>(SelectedPreviewBodyActor.Get());
		SelectedBody = IsValid(SelectedBody) ? SelectedBody : HomePlanet;
		if (IsValid(SelectedBody))
		{
			constexpr double PlanetPresentationRadius = 1.2e6;
			APlanet* FocusPlanet = Cast<APlanet>(SelectedBody);
			if (AMoon* FocusMoon = Cast<AMoon>(SelectedBody))
			{
				FocusPlanet = FocusMoon->ParentPlanet;
			}
			const auto PresentBody = [this, &ScaleMeshToWorldRadius](
				APlanetaryBody* Body, const double Radius, const FVector& Center)
			{
				if (!IsValid(Body) || Center.ContainsNaN() || !FMath::IsFinite(Radius)) return;
				TInlineComponentArray<UStaticMeshComponent*> Meshes;
				Body->GetComponents(Meshes);
				if (Meshes.Num() == 0) return;
				ScaleMeshToWorldRadius(Meshes[0], Radius, Center);
				const TWeakObjectPtr<AActor> Key(Body);
				PreviewBodyPresentationCenters.Add(Key, Center);
				PreviewBodyPresentationRadii.Add(Key, Radius);
			};

			// The actor hierarchy is full-scale data and stays invariant across every
			// scope. Its normalized moon offsets, however, are only hundreds of cm after
			// fitting a galaxy and would sit *inside* the 1.2e6 cm PLANET globe. Spread the
			// rendered satellite family over a bounded presentation ring while leaving the
			// actor/model/orbit transforms untouched.
			if (IsValid(FocusPlanet))
			{
				const auto GeneratedOrbitDistance = [FocusPlanet](const AMoon* Moon)
				{
					return IsValid(Moon)
						? FVector::Distance(Moon->GetActorLocation(), FocusPlanet->GetActorLocation())
						: 0.0;
				};
				double MinimumOrbit = TNumericLimits<double>::Max();
				double MaximumOrbit = 0.0;
				for (const AMoon* Moon : FocusPlanet->Moons)
				{
					if (!IsValid(Moon)) continue;
					const double OrbitValue = FMath::Max(GeneratedOrbitDistance(Moon), 0.0);
					if (FMath::IsFinite(OrbitValue))
					{
						MinimumOrbit = FMath::Min(MinimumOrbit, OrbitValue);
						MaximumOrbit = FMath::Max(MaximumOrbit, OrbitValue);
					}
				}
				if (!FMath::IsFinite(MinimumOrbit)) MinimumOrbit = 0.0;
				const auto PresentationOrbitDistance = [MinimumOrbit, MaximumOrbit,
					PlanetPresentationRadius, GeneratedOrbitDistance](const AMoon* Moon)
				{
					const double OrbitValue = IsValid(Moon)
						? FMath::Max(GeneratedOrbitDistance(Moon), 0.0) : MinimumOrbit;
					const double Alpha = MaximumOrbit > MinimumOrbit + UE_DOUBLE_SMALL_NUMBER
						? FMath::Clamp((OrbitValue - MinimumOrbit) / (MaximumOrbit - MinimumOrbit), 0.0, 1.0)
						: 0.5;
					return PlanetPresentationRadius * FMath::Lerp(1.65, 1.95, Alpha);
				};
				const auto MoonDirection = [FocusPlanet](const AMoon* Moon, const int32 MoonIndex)
				{
					FVector Direction = IsValid(Moon)
						? (Moon->GetActorLocation() - FocusPlanet->GetActorLocation()).GetSafeNormal()
						: FVector::ZeroVector;
					if (Direction.IsNearlyZero())
					{
						const double Angle = UE_TWO_PI * (0.173 + 0.381966 * MoonIndex);
						Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle),
							0.18 * FMath::Sin(Angle * 0.7)).GetSafeNormal();
					}
					return Direction;
				};

				FVector PresentedPlanetCenter = FocusPlanet->GetActorLocation();
				if (const AMoon* SelectedMoon = Cast<AMoon>(SelectedBody))
				{
					const int32 SelectedMoonIndex = FMath::Max(
						FocusPlanet->Moons.IndexOfByKey(const_cast<AMoon*>(SelectedMoon)), 0);
					PresentedPlanetCenter = SelectedMoon->GetActorLocation()
						- MoonDirection(SelectedMoon, SelectedMoonIndex)
						* PresentationOrbitDistance(SelectedMoon);
					PresentBody(FocusPlanet, 2.4e5, PresentedPlanetCenter);
				}
				else
				{
					PresentBody(FocusPlanet, PlanetPresentationRadius, PresentedPlanetCenter);
				}

				const double ParentRadiusKm = FMath::Max(
					FocusPlanet->RadiusKM, static_cast<double>(FocusPlanet->PlanetRadiusKM));
				for (int32 MoonIndex = 0; MoonIndex < FocusPlanet->Moons.Num(); ++MoonIndex)
				{
					AMoon* Moon = FocusPlanet->Moons[MoonIndex];
					if (!IsValid(Moon)) continue;
					const double MoonRadiusKm = FMath::Max(
						Moon->RadiusKM, static_cast<double>(Moon->PlanetRadiusKM));
					const double DesiredMoonRadius = Moon == SelectedBody
						? PlanetPresentationRadius
						: FMath::Clamp(PlanetPresentationRadius * MoonRadiusKm
							/ FMath::Max(ParentRadiusKm, 1.0), 3.6e4, 1.8e5);
					const FVector MoonCenter = Moon == SelectedBody
						? SelectedBody->GetActorLocation()
						: PresentedPlanetCenter + MoonDirection(Moon, MoonIndex)
							* PresentationOrbitDistance(Moon);
					PresentBody(Moon, DesiredMoonRadius, MoonCenter);
				}
			}
			else
			{
				PresentBody(SelectedBody, PlanetPresentationRadius,
					SelectedBody->GetActorLocation());
			}
			AStar* ContextParentStar = IsValid(FocusPlanet) ? FocusPlanet->ParentStar : HomeStar;
			double LargestStarRadiusKm = 1.0;
			for (const AStar* Star : Stars)
			{
				if (IsValid(Star))
				{
					LargestStarRadiusKm = FMath::Max(
						LargestStarRadiusKm, static_cast<double>(FMath::Max(Star->StarRadiusKM, 1)));
				}
			}
			double LargestPresentedStarRadius = 0.0;
			double ParentStarAngularRadiusDegrees = 0.0;
			int32 PresentedStarCount = 0;
			for (AStar* Star : Stars)
			{
				if (!IsValid(Star) || !IsValid(Star->StarMesh))
				{
					continue;
				}
				const double RelativePhysicalRadius = static_cast<double>(
					FMath::Max(Star->StarRadiusKM, 1)) / LargestStarRadiusKm;
				const bool bIsContextParent = Star == ContextParentStar;
				const double StarDistance = FVector::Distance(
					Star->GetActorLocation(), SelectedBody->GetActorLocation());
				// A fixed fraction of the normalized planet radius made the parent star a
				// 0.1-degree point at ordinary generated orbit distances. Size the mesh-only
				// presentation by distance instead, so it remains a recognizable sun while its
				// actor, barycentre and all orbital data stay at the exact SYSTEM address.
				const double TargetAngularRadiusRadians = FMath::DegreesToRadians(
					bIsContextParent ? 0.80 : 0.26);
				const double HierarchyScale = bIsContextParent
					? 1.0 : FMath::Sqrt(FMath::Clamp(RelativePhysicalRadius, 0.04, 1.0));
				// The distance-derived angular target is authoritative in PLANET.  The old
				// 0.35-planet-radius floor overrode the requested 0.8 degree sun in ordinary
				// systems (2.12 degrees in the rendered smoke), and its emissive halo then
				// read as a clipped/full-screen blob.  Keep only a small legibility floor;
				// actor positions and physical stellar radii remain untouched.
				const double MinimumRadius = PlanetPresentationRadius
					* (bIsContextParent ? 0.08 : 0.04) * HierarchyScale;
				const double MaximumRadius = FMath::Max(
					MinimumRadius, StarDistance * (bIsContextParent ? 0.025 : 0.015));
				const double DesiredRadius = FMath::Clamp(
					StarDistance * FMath::Tan(TargetAngularRadiusRadians) * HierarchyScale,
					MinimumRadius, MaximumRadius);
				ScaleMeshToWorldRadius(
					Star->StarMesh, DesiredRadius, Star->GetActorLocation());
				if (bIsContextParent && StarDistance > UE_SMALL_NUMBER)
				{
					ParentStarAngularRadiusDegrees = FMath::RadiansToDegrees(
						FMath::Atan2(DesiredRadius, StarDistance));
				}
				LargestPresentedStarRadius = FMath::Max(
					LargestPresentedStarRadius, DesiredRadius);
				++PresentedStarCount;
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Preview.PlanetContext] parent=%s stars=%d maxPresentedRadius=%.0f parentAngularRadiusDeg=%.3f clusterHism=angular-capped/safe-zone-culled"),
				*GetNameSafe(ContextParentStar), PresentedStarCount, LargestPresentedStarRadius,
				ParentStarAngularRadiusDegrees);
			if (IsValid(SelectedBody->PlanetaryEnvironmentGenerator)
				&& IsValid(SelectedBody->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
			{
				AAtmoScape* Atmosphere =
					SelectedBody->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
				const double PhysicalRadiusKm = FMath::Max(
					SelectedBody->RadiusKM, static_cast<double>(SelectedBody->PlanetRadiusKM));
				Atmosphere->bKeepRelativeScale = false;
				Atmosphere->LightSource = ContextParentStar;
				if (bIsPreviewGeneration)
				{
					// The menu uses only the outside-space shell. Half-resolution raymarch
					// sampling removes several milliseconds from the live editor while the
					// committed gameplay atmosphere retains its authored 32/16/32 quality.
					Atmosphere->CameraSamplesCount = FMath::Min(Atmosphere->CameraSamplesCount, 8);
					Atmosphere->LightSamplesCount = FMath::Min(Atmosphere->LightSamplesCount, 4);
					Atmosphere->StepsNumAO = FMath::Min(Atmosphere->StepsNumAO, 8.0f);
				}
				Atmosphere->AtmosphereHeight = FMath::Max(Atmosphere->AtmosphereHeight, 0.0f);
				Atmosphere->AtmosphereOpacity = FMath::Max(Atmosphere->AtmosphereOpacity, 0.0f);
				Atmosphere->MultiScatering = FMath::Max(Atmosphere->MultiScatering, 0.01f);
				Atmosphere->PresentationPlanetRadiusCm = 0.0f;
				Atmosphere->PresentationAtmosphereRadiusCm = 0.0f;
				Atmosphere->PresentationLightIntensity = bIsPreviewGeneration ? 0.65f : 1.0f;
				Atmosphere->PresentationOpacityScale = bIsPreviewGeneration ? 0.12f : 1.0f;
				// Material vector parameters cannot preserve an absolute stellar position
				// across UE5's LWC tile transform. The menu directional light is already
				// synchronized to this same parent star, so use AtmoScape's direction branch
				// for rendering and restore the semantic parent pointer afterwards.
				AActor* SemanticLightSource = Atmosphere->LightSource;
				if (bIsPreviewGeneration)
				{
					Atmosphere->LightSource = nullptr;
				}
				Atmosphere->UpdateScale();
				UStaticMeshComponent* SpaceAtmosphereMesh = nullptr;
				TInlineComponentArray<UStaticMeshComponent*> AtmosphereMeshes;
				Atmosphere->GetComponents(AtmosphereMeshes);
				for (UStaticMeshComponent* AtmosphereMesh : AtmosphereMeshes)
				{
					if (!IsValid(AtmosphereMesh)) continue;
					const FString ComponentName = AtmosphereMesh->GetName();
					const bool bIsSpaceShell = ComponentName.Contains(TEXT("SpacePlanetaryAtmoMesh"));
					if (bIsSpaceShell) SpaceAtmosphereMesh = AtmosphereMesh;
				}
				if (IsValid(SpaceAtmosphereMesh))
				{
					SpaceAtmosphereMesh->UpdateBounds();
					const double CurrentRadius = SpaceAtmosphereMesh->Bounds.SphereRadius;
					if (FMath::IsFinite(CurrentRadius) && CurrentRadius > UE_SMALL_NUMBER)
					{
						const double HeightRatio = FMath::Clamp(
							Atmosphere->AtmosphereHeight / FMath::Max(PhysicalRadiusKm, 1.0), 0.06, 0.12);
						const double Factor = FMath::Clamp(
							(1.2e6 * (1.0 + HeightRatio)) / CurrentRadius, 1.0e-6, 1.0e6);
						Atmosphere->SetActorScale3D(Atmosphere->GetActorScale3D() * Factor);
						Atmosphere->PresentationPlanetRadiusCm = 1.2e6f;
						Atmosphere->PresentationAtmosphereRadiusCm =
							static_cast<float>(1.2e6 * (1.0 + HeightRatio));
						// Actor scale changes the shell geometry, while these overrides change the
						// ray-march coordinate system. Refresh after both are coherent.
						Atmosphere->UpdateScale();
						UE_LOG(LogTemp, Verbose,
							TEXT("[APS.Preview.Atmosphere] body=%s heightKm=%.1f physicalRadiusKm=%.1f presentationRatio=%.4f relativeScale=%s"),
							*GetNameSafe(SelectedBody), Atmosphere->AtmosphereHeight, PhysicalRadiusKm,
							HeightRatio, Atmosphere->bKeepRelativeScale ? TEXT("true") : TEXT("false"));
					}
				}
				if (bIsPreviewGeneration)
				{
					Atmosphere->LightSource = SemanticLightSource;
				}
				// Restore actor visibility before component overrides. Keep only the
				// proper outside ray-march shell; the opaque air-glow mesh washes the globe.
				Atmosphere->SetActorHiddenInGame(false);
				for (UStaticMeshComponent* AtmosphereMesh : AtmosphereMeshes)
				{
					if (!IsValid(AtmosphereMesh)) continue;
					const FString ComponentName = AtmosphereMesh->GetName();
					const bool bIsSpaceShell = ComponentName.Contains(TEXT("SpacePlanetaryAtmoMesh"));
					const bool bShowComponent = bIsSpaceShell;
					AtmosphereMesh->SetHiddenInGame(!bShowComponent, true);
					AtmosphereMesh->SetVisibility(bShowComponent, true);
					if (bShowComponent)
					{
						AtmosphereMesh->Activate(true);
						AtmosphereMesh->MarkRenderStateDirty();
					}
				}
				// Disable the plugin's per-frame inside/outside switch in menu preview so
				// the explicitly selected thin space shell and air-glow remain stable.
				Atmosphere->SetActorTickEnabled(false);
				// Atmosphere is part of the planet preview, not a by-product of terrain
				// readiness. Keep the corrected outer shell visible over the temporary
				// globe as well, so entry/loading never looks like an airless black ball.
			}
		}
	}

	// Presentation centres are complete only after the selected body and satellites
	// have been laid out. Apply the immutable galaxy/cluster proxy LOD now so its
	// angular cap is measured from the exact same focus centre used by tests/camera.
	UpdatePreviewGuideShells(NewFocus);
	ApplyPreviewBackgroundContext(NewFocus);
	// Closed orbital globes belong to bodies, not to the current selection. Keep every
	// already materialized body proxy aligned with the presentation-only planet/moon
	// transform, and expose the complete retained set whenever PLANET is active.
	SyncPreviewGlobeProxyTransforms();
	SetPreviewGlobeProxyVisible(NewFocus == EAstroPreviewFocus::HomePlanet);

	// HISM/Blueprint roots may restore descendant visibility while the hierarchy is
	// rebuilt. Native safe-zone spheres are data only; dedicated preview shells own
	// the two centred guides. Reassert this last so no solid duplicate leaks through.
	if (IsValid(GeneratedHomeStarSystem) && IsValid(GeneratedHomeStarSystem->StarSystemZone))
	{
		GeneratedHomeStarSystem->StarSystemZone->SetVisibility(false, true);
		GeneratedHomeStarSystem->StarSystemZone->SetHiddenInGame(true, true);
	}
	if (IsValid(GeneratedHomeStarSystem))
	{
		for (AStar* Star : GeneratedHomeStarSystem->GetStars())
		{
			if (IsValid(Star) && IsValid(Star->PlanetarySystemZone))
			{
				Star->PlanetarySystemZone->SetVisibility(false, true);
				Star->PlanetarySystemZone->SetHiddenInGame(true, true);
			}
		}
	}
}

void AAstroGenerator::SetPreviewBodyBackingSphereVisible(
	APlanetaryBody* Body, const bool bVisible)
{
	if (!IsValid(Body)) return;
	UStaticMeshComponent* SphereMesh = Cast<UStaticMeshComponent>(
		Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!IsValid(SphereMesh)) return;

	// The authored globe is an atomic loading fallback only. Presentation sizing is
	// owned by ApplyPreviewFocusPresentation; changing it here made a cached parent
	// jump back to selected-body size whenever one of its moons was focused.
	SphereMesh->SetHiddenInGame(!bVisible, false);
	SphereMesh->SetVisibility(bVisible, false);
}

UProceduralMeshComponent* AAstroGenerator::CreatePreviewGlobeMeshComponent(
	const FName BaseName)
{
	if (!IsValid(GenerationRoot))
	{
		return nullptr;
	}
	const FName UniqueName = MakeUniqueObjectName(
		this, UProceduralMeshComponent::StaticClass(), BaseName);
	UProceduralMeshComponent* Component = NewObject<UProceduralMeshComponent>(
		this, UniqueName, RF_Transient);
	if (!IsValid(Component))
	{
		return nullptr;
	}
	Component->SetupAttachment(GenerationRoot);
	Component->SetAbsolute(true, true, true);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(ECR_Ignore);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->bUseAsyncCooking = false;
	Component->bUseComplexAsSimpleCollision = false;
	Component->SetCastShadow(false);
	Component->SetVisibility(false, false);
	Component->SetHiddenInGame(true, false);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

FAPSPreviewGlobeProxyState* AAstroGenerator::FindPreviewGlobeProxyState(
	const APlanetaryBody* Body)
{
	const FString StableKey = GetPreviewBodyStableKey(Body);
	return StableKey.IsEmpty() ? nullptr : PreviewGlobeProxyStates.Find(StableKey);
}

const FAPSPreviewGlobeProxyState* AAstroGenerator::FindPreviewGlobeProxyState(
	const APlanetaryBody* Body) const
{
	const FString StableKey = GetPreviewBodyStableKey(Body);
	return StableKey.IsEmpty() ? nullptr : PreviewGlobeProxyStates.Find(StableKey);
}

FAPSPreviewGlobeProxyState* AAstroGenerator::FindOrAddPreviewGlobeProxyState(
	APlanetaryBody* Body)
{
	if (!IsValid(Body))
	{
		return nullptr;
	}
	const FString StableKey = GetPreviewBodyStableKey(Body);
	if (StableKey.IsEmpty())
	{
		return nullptr;
	}
	if (FAPSPreviewGlobeProxyState* Existing = PreviewGlobeProxyStates.Find(StableKey))
	{
		Existing->Body = Body;
		return Existing;
	}

	FAPSPreviewGlobeProxyState NewState;
	NewState.Body = Body;
	if (PreviewGlobeProxyStates.Num() == 0)
	{
		NewState.TerrainA = PreviewTerrainProxyA;
		NewState.TerrainB = PreviewTerrainProxyB;
		NewState.OceanA = PreviewOceanProxyA;
		NewState.OceanB = PreviewOceanProxyB;
		NewState.bUsesDefaultBuffers = true;
	}
	else
	{
		NewState.TerrainA = CreatePreviewGlobeMeshComponent(TEXT("PreviewBodyTerrainA"));
		NewState.TerrainB = CreatePreviewGlobeMeshComponent(TEXT("PreviewBodyTerrainB"));
		NewState.OceanA = CreatePreviewGlobeMeshComponent(TEXT("PreviewBodyOceanA"));
		NewState.OceanB = CreatePreviewGlobeMeshComponent(TEXT("PreviewBodyOceanB"));
	}
	if (!NewState.TerrainA.IsValid() || !NewState.TerrainB.IsValid()
		|| !NewState.OceanA.IsValid() || !NewState.OceanB.IsValid())
	{
		for (const TWeakObjectPtr<UProceduralMeshComponent>& Component : {
			NewState.TerrainA, NewState.TerrainB, NewState.OceanA, NewState.OceanB })
		{
			if (Component.IsValid() && !NewState.bUsesDefaultBuffers)
			{
				Component->DestroyComponent();
			}
		}
		return nullptr;
	}
	return &PreviewGlobeProxyStates.Add(StableKey, MoveTemp(NewState));
}

UProceduralMeshComponent* AAstroGenerator::GetPreviewTerrainProxyForBody(
	const APlanetaryBody* Body) const
{
	const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Body);
	return State && State->ActiveBuffer == 0 ? State->TerrainA.Get()
		: State && State->ActiveBuffer == 1 ? State->TerrainB.Get() : nullptr;
}

UProceduralMeshComponent* AAstroGenerator::GetPreviewOceanProxyForBody(
	const APlanetaryBody* Body) const
{
	const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Body);
	return State && State->bHasOcean && State->ActiveBuffer == 0 ? State->OceanA.Get()
		: State && State->bHasOcean && State->ActiveBuffer == 1 ? State->OceanB.Get() : nullptr;
}

int32 AAstroGenerator::GetRetainedPreviewGlobeCount() const
{
	int32 Count = 0;
	for (const TPair<FString, FAPSPreviewGlobeProxyState>& Pair : PreviewGlobeProxyStates)
	{
		if (Pair.Value.Body.IsValid() && Pair.Value.ActiveBuffer != INDEX_NONE
			&& (Pair.Value.TerrainA.IsValid() || Pair.Value.TerrainB.IsValid()))
		{
			++Count;
		}
	}
	return Count;
}

bool AAstroGenerator::IsPreviewGlobeFamilyWarmQueueDrained() const
{
	return PendingPreviewGlobeBodies.IsEmpty()
		&& !PreviewSurfaceBuildBody.IsValid()
		&& !bPreviewSurfaceUpdatePending;
}

UProceduralMeshComponent* AAstroGenerator::GetActivePreviewTerrainProxy() const
{
	return GetPreviewTerrainProxyForBody(ActivePreviewWorldScapeBody.Get());
}

UProceduralMeshComponent* AAstroGenerator::GetActivePreviewOceanProxy() const
{
	return GetPreviewOceanProxyForBody(ActivePreviewWorldScapeBody.Get());
}

void AAstroGenerator::SetPreviewGlobeProxyVisible(const bool bVisible)
{
	APlanet* VisibleFamilyPlanet = Cast<APlanet>(ActivePreviewWorldScapeBody.Get());
	if (AMoon* FocusMoon = Cast<AMoon>(ActivePreviewWorldScapeBody.Get()))
	{
		VisibleFamilyPlanet = FocusMoon->ParentPlanet;
	}
	for (TPair<FString, FAPSPreviewGlobeProxyState>& Pair : PreviewGlobeProxyStates)
	{
		FAPSPreviewGlobeProxyState& State = Pair.Value;
		APlanetaryBody* Body = State.Body.Get();
		UProceduralMeshComponent* ActiveTerrain = State.ActiveBuffer == 0
			? State.TerrainA.Get() : State.ActiveBuffer == 1 ? State.TerrainB.Get() : nullptr;
		UProceduralMeshComponent* ActiveOcean = State.ActiveBuffer == 0
			? State.OceanA.Get() : State.ActiveBuffer == 1 ? State.OceanB.Get() : nullptr;
		const AMoon* BodyMoon = Cast<AMoon>(Body);
		const bool bBelongsToVisibleFamily = IsValid(VisibleFamilyPlanet)
			&& (Body == VisibleFamilyPlanet
				|| (BodyMoon && BodyMoon->ParentPlanet == VisibleFamilyPlanet));
		const bool bShowState = bVisible && bBelongsToVisibleFamily
			&& IsValid(Body) && IsValid(ActiveTerrain);
		for (UProceduralMeshComponent* Terrain : {
			State.TerrainA.Get(), State.TerrainB.Get() })
		{
			if (!IsValid(Terrain)) continue;
			const bool bShow = bShowState && Terrain == ActiveTerrain;
			Terrain->SetHiddenInGame(!bShow, false);
			Terrain->SetVisibility(bShow, false);
		}
		for (UProceduralMeshComponent* Ocean : {
			State.OceanA.Get(), State.OceanB.Get() })
		{
			if (!IsValid(Ocean)) continue;
			const bool bShow = bShowState && State.bHasOcean && Ocean == ActiveOcean;
			Ocean->SetHiddenInGame(!bShow, false);
			Ocean->SetVisibility(bShow, false);
		}
		if (bShowState)
		{
			// The selected body owns the atomic authored-sphere fallback and hides it
			// once its closed globe commits.  Sibling moons remain real visible members
			// of the PLANET family (rather than disappearing as soon as their retained
			// surface warms).  Inset their authored sphere slightly beneath the closed
			// terrain so it remains a valid fallback/actor visual without z-fighting.
			const bool bKeepMoonBacking = BodyMoon
				&& Body != ActivePreviewWorldScapeBody.Get();
			SetPreviewBodyBackingSphereVisible(Body, bKeepMoonBacking);
			if (bKeepMoonBacking)
			{
				UStaticMeshComponent* BackingSphere = Cast<UStaticMeshComponent>(
					Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
				const TWeakObjectPtr<AActor> PresentationKey(Body);
				const double* PresentedRadius = PreviewBodyPresentationRadii.Find(PresentationKey);
				const FVector* PresentedCenter = PreviewBodyPresentationCenters.Find(PresentationKey);
				if (IsValid(BackingSphere) && PresentedRadius
					&& FMath::IsFinite(*PresentedRadius) && *PresentedRadius > UE_SMALL_NUMBER)
				{
					BackingSphere->UpdateBounds();
					const double CurrentRadius = BackingSphere->Bounds.SphereRadius;
					if (FMath::IsFinite(CurrentRadius) && CurrentRadius > UE_SMALL_NUMBER)
					{
						const double InsetFactor = FMath::Clamp(
							(*PresentedRadius * 0.985) / CurrentRadius, 1.0e-6, 1.0e6);
						BackingSphere->SetRelativeScale3D(
							BackingSphere->GetRelativeScale3D() * InsetFactor);
						BackingSphere->UpdateBounds();
						if (PresentedCenter && !PresentedCenter->ContainsNaN())
						{
							const FVector CenterDelta = *PresentedCenter - BackingSphere->Bounds.Origin;
							if (!CenterDelta.ContainsNaN() && !CenterDelta.IsNearlyZero(0.01))
							{
								BackingSphere->SetWorldLocation(
									BackingSphere->GetComponentLocation() + CenterDelta);
								BackingSphere->UpdateBounds();
							}
						}
					}
				}
			}
		}
	}
}

void AAstroGenerator::SyncPreviewGlobeProxyTransforms()
{
	for (TPair<FString, FAPSPreviewGlobeProxyState>& Pair : PreviewGlobeProxyStates)
	{
		FAPSPreviewGlobeProxyState& State = Pair.Value;
		APlanetaryBody* Body = State.Body.Get();
		UProceduralMeshComponent* Terrain = State.ActiveBuffer == 0
			? State.TerrainA.Get() : State.ActiveBuffer == 1 ? State.TerrainB.Get() : nullptr;
		UProceduralMeshComponent* Ocean = State.ActiveBuffer == 0
			? State.OceanA.Get() : State.ActiveBuffer == 1 ? State.OceanB.Get() : nullptr;
		if (!IsValid(Body) || !IsValid(Terrain))
		{
			continue;
		}

		UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(
			Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		if (!IsValid(BodyMesh))
		{
			continue;
		}
		BodyMesh->UpdateBounds();
		FVector TargetCenter = BodyMesh->Bounds.Origin;
		double TargetRadius = BodyMesh->Bounds.SphereRadius;
		const TWeakObjectPtr<AActor> PresentationKey(Body);
		if (const FVector* PresentedCenter = PreviewBodyPresentationCenters.Find(PresentationKey))
		{
			TargetCenter = *PresentedCenter;
		}
		if (const double* PresentedRadius = PreviewBodyPresentationRadii.Find(PresentationKey))
		{
			TargetRadius = *PresentedRadius;
		}
		if (TargetCenter.ContainsNaN() || !FMath::IsFinite(TargetRadius)
			|| TargetRadius <= UE_SMALL_NUMBER)
		{
			continue;
		}

		Terrain->UpdateBounds();
		const double CurrentRadius = Terrain->Bounds.SphereRadius;
		FVector TargetScale = Terrain->GetComponentScale();
		if (FMath::IsFinite(CurrentRadius) && CurrentRadius > UE_SMALL_NUMBER)
		{
			TargetScale *= FMath::Clamp(TargetRadius / CurrentRadius, 1.0e-12, 1.0e12);
		}
		const FTransform ProxyTransform(Body->GetActorQuat(), TargetCenter, TargetScale);
		Terrain->SetWorldTransform(ProxyTransform, false, nullptr, ETeleportType::TeleportPhysics);
		Terrain->UpdateBounds();
		if (IsValid(Ocean))
		{
			Ocean->SetWorldTransform(ProxyTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Ocean->UpdateBounds();
		}
	}
}

void AAstroGenerator::UpdateActivePreviewGlobeCompatibilityState()
{
	APlanetaryBody* ActiveBody = ActivePreviewWorldScapeBody.Get();
	const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(ActiveBody);
	if (!State || State->ActiveBuffer == INDEX_NONE)
	{
		PreviewGlobeProxyBody.Reset();
		ActivePreviewGlobeBuffer = INDEX_NONE;
		PreviewGlobeProfileSignature = 0;
		PreviewGlobeVertexCount = 0;
		PreviewGlobeIndexCount = 0;
		bPreviewGlobeHasOcean = false;
		return;
	}
	PreviewGlobeProxyBody = ActiveBody;
	ActivePreviewGlobeBuffer = State->ActiveBuffer;
	PreviewGlobeProfileSignature = State->ProfileSignature;
	PreviewGlobeVertexCount = State->VertexCount;
	PreviewGlobeIndexCount = State->IndexCount;
	bPreviewGlobeHasOcean = State->bHasOcean;
}

void AAstroGenerator::InvalidatePreviewGlobeProxy(APlanetaryBody* Body)
{
	FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Body);
	if (!State)
	{
		return;
	}
	for (UProceduralMeshComponent* Component : {
		State->TerrainA.Get(), State->TerrainB.Get(), State->OceanA.Get(), State->OceanB.Get() })
	{
		if (!IsValid(Component)) continue;
		Component->SetVisibility(false, false);
		Component->SetHiddenInGame(true, false);
	}
	State->ActiveBuffer = INDEX_NONE;
	State->ProfileSignature = 0;
	State->VertexCount = 0;
	State->IndexCount = 0;
	State->bHasOcean = false;
	if (Body == ActivePreviewWorldScapeBody.Get())
	{
		UpdateActivePreviewGlobeCompatibilityState();
	}
}

void AAstroGenerator::ClearPreviewGlobeProxyCache()
{
	TSet<UProceduralMeshComponent*> DestroyedComponents;
	for (TPair<FString, FAPSPreviewGlobeProxyState>& Pair : PreviewGlobeProxyStates)
	{
		FAPSPreviewGlobeProxyState& State = Pair.Value;
		for (UProceduralMeshComponent* Component : {
			State.TerrainA.Get(), State.TerrainB.Get(), State.OceanA.Get(), State.OceanB.Get() })
		{
			if (!IsValid(Component) || DestroyedComponents.Contains(Component)) continue;
			DestroyedComponents.Add(Component);
			Component->SetVisibility(false, false);
			Component->SetHiddenInGame(true, false);
			Component->ClearAllMeshSections();
			Component->SetMaterial(0, nullptr);
			if (!State.bUsesDefaultBuffers)
			{
				Component->DestroyComponent();
			}
		}
	}
	for (UProceduralMeshComponent* DefaultComponent : {
		PreviewTerrainProxyA, PreviewTerrainProxyB, PreviewOceanProxyA, PreviewOceanProxyB })
	{
		if (!IsValid(DefaultComponent) || DestroyedComponents.Contains(DefaultComponent)) continue;
		DefaultComponent->SetVisibility(false, false);
		DefaultComponent->SetHiddenInGame(true, false);
		DefaultComponent->ClearAllMeshSections();
		DefaultComponent->SetMaterial(0, nullptr);
	}
	PreviewGlobeProxyStates.Reset();
	PendingPreviewGlobeBodies.Reset();
	PreviewSurfaceBuildBody.Reset();
	PreviewGlobeProxyBody.Reset();
	ActivePreviewGlobeBuffer = INDEX_NONE;
	PreviewGlobeProfileSignature = 0;
	PreviewGlobeVertexCount = 0;
	PreviewGlobeIndexCount = 0;
	bPreviewGlobeHasOcean = false;
	PreviewTerrainMaterialA = nullptr;
	PreviewTerrainMaterialB = nullptr;
	PreviewOceanMaterialA = nullptr;
	PreviewOceanMaterialB = nullptr;
}

void AAstroGenerator::QueuePreviewGlobeFamily(APlanetaryBody* Body)
{
	if (!IsValid(Body))
	{
		return;
	}
	APlanet* FocusPlanet = Cast<APlanet>(Body);
	if (AMoon* FocusMoon = Cast<AMoon>(Body))
	{
		FocusPlanet = FocusMoon->ParentPlanet;
	}
	const auto QueueMissingBody = [this](APlanetaryBody* Candidate)
	{
		if (!IsValid(Candidate)
			|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Candidate->PlanetType))
		{
			return;
		}
		const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Candidate);
		if (State && State->ActiveBuffer != INDEX_NONE)
		{
			return;
		}
		PendingPreviewGlobeBodies.AddUnique(Candidate);
	};
	QueueMissingBody(Body);
	if (IsValid(FocusPlanet))
	{
		QueueMissingBody(FocusPlanet);
		for (AMoon* Moon : FocusPlanet->Moons)
		{
			QueueMissingBody(Moon);
		}
	}
}

bool AAstroGenerator::BeginNextQueuedPreviewGlobeBuild()
{
	while (PendingPreviewGlobeBodies.Num() > 0)
	{
		TWeakObjectPtr<APlanetaryBody> CandidatePtr = PendingPreviewGlobeBodies[0];
		PendingPreviewGlobeBodies.RemoveAt(0, 1, EAllowShrinking::No);
		APlanetaryBody* Candidate = CandidatePtr.Get();
		if (!IsValid(Candidate)
			|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Candidate->PlanetType))
		{
			continue;
		}
		const FAPSPreviewGlobeProxyState* State = FindPreviewGlobeProxyState(Candidate);
		if (State && State->ActiveBuffer != INDEX_NONE)
		{
			continue;
		}
		PreviewSurfaceBuildBody = Candidate;
		bPreviewSurfaceUpdatePending = true;
		Candidate->bStreamWorldScapeSurface = false;
		Candidate->bWorldScapeSurfaceReady = false;
		return true;
	}
	PreviewSurfaceBuildBody.Reset();
	bPreviewSurfaceUpdatePending = false;
	return false;
}

bool AAstroGenerator::BuildPreviewGlobeProxy(APlanetaryBody* Body,
	APlanetarySurfaceGenerator* SurfaceGenerator, AWorldScapeRoot* ProfileRoot)
{
	if (!IsValid(Body) || !IsValid(SurfaceGenerator) || !IsValid(ProfileRoot)
		|| !IsValid(SurfaceGenerator->ResolvedNoiseInstance)
		|| !IsValid(SurfaceGenerator->ResolvedTerrainMaterialInstance)
		|| SurfaceGenerator->AppliedSurfaceProfileSignature == 0)
	{
		return false;
	}

	const bool bHasOcean = ProfileRoot->bOcean
		&& IsValid(SurfaceGenerator->ResolvedOceanMaterialInstance);
	// The menu preserves the body's full physical radius in its data while rendering
	// a uniformly compressed WorldScape root. Geometry keeps that exact compressed
	// height, but lighting normals compensate the same presentation scale so orbital
	// relief does not collapse into a smooth blurred sphere. This is deterministic
	// per body and naturally becomes 1x for a full-scale root.
	const double NormalReliefExaggeration = FMath::Clamp(
		1.0 / FMath::Max(Body->WorldScapePresentationScale, 1.0e-9), 1.0, 1024.0);
	APSPreviewGlobe::FMeshData MeshData;
	if (!APSPreviewGlobe::BuildClosedCubeSphere(
		SurfaceGenerator->ResolvedNoiseInstance, ProfileRoot, bHasOcean,
		NormalReliefExaggeration, MeshData))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Closed preview globe sampling failed body=%s signature=%u"),
			*GetNameSafe(Body), SurfaceGenerator->AppliedSurfaceProfileSignature);
		return false;
	}

	FAPSPreviewGlobeProxyState* State = FindOrAddPreviewGlobeProxyState(Body);
	if (!State)
	{
		return false;
	}
	const int32 NewBuffer = State->ActiveBuffer == 0 ? 1 : 0;
	UProceduralMeshComponent* NewTerrain = NewBuffer == 0
		? State->TerrainA.Get() : State->TerrainB.Get();
	UProceduralMeshComponent* NewOcean = NewBuffer == 0
		? State->OceanA.Get() : State->OceanB.Get();
	if (!IsValid(NewTerrain) || !IsValid(NewOcean))
	{
		return false;
	}
	UMaterialInstanceDynamic* NewTerrainMaterial =
		APSPreviewGlobe::CreateTerrainMaterial(
			NewTerrain, PreviewTerrainBaseMaterial,
			SurfaceGenerator->ResolvedSurfaceProfile);
	const bool bRenderOcean = bHasOcean && MeshData.OceanIndices.Num() >= 3;
	UMaterialInstanceDynamic* NewOceanMaterial = bRenderOcean
		? APSPreviewGlobe::CreateLiquidMaterial(NewOcean,
			SurfaceGenerator->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Water
				? PreviewWaterBaseMaterial.Get()
				: SurfaceGenerator->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Ammonia
					? PreviewAmmoniaBaseMaterial.Get()
					: SurfaceGenerator->ResolvedSurfaceProfile.LiquidType == EAPSPlanetLiquidType::Lava
						? PreviewLavaBaseMaterial.Get() : nullptr)
		: nullptr;
	if (!IsValid(NewTerrainMaterial) || (bRenderOcean && !IsValid(NewOceanMaterial)))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Warmed orbital material unavailable at commit body=%s terrain=%s liquid=%d ocean=%s"),
			*GetNameSafe(Body), *GetNameSafe(NewTerrainMaterial),
			static_cast<int32>(SurfaceGenerator->ResolvedSurfaceProfile.LiquidType),
			*GetNameSafe(NewOceanMaterial));
		return false;
	}

	NewTerrain->SetVisibility(false, false);
	NewTerrain->SetHiddenInGame(true, false);
	NewOcean->SetVisibility(false, false);
	NewOcean->SetHiddenInGame(true, false);
	NewTerrain->ClearAllMeshSections();
	NewOcean->ClearAllMeshSections();
	NewTerrain->CreateMeshSection_LinearColor(0, MeshData.TerrainVertices,
		MeshData.Indices, MeshData.Normals, MeshData.UV0, MeshData.VertexColors,
		MeshData.Tangents, false);
	NewTerrain->SetMaterial(0, NewTerrainMaterial);
	if (bRenderOcean)
	{
		NewOcean->CreateMeshSection_LinearColor(0, MeshData.OceanVertices,
			MeshData.OceanIndices, MeshData.OceanNormals, MeshData.UV0, MeshData.VertexColors,
			MeshData.Tangents, false);
		NewOcean->SetMaterial(0, NewOceanMaterial);
	}
	else
	{
		NewOcean->SetMaterial(0, nullptr);
	}

	UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(
		Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!IsValid(BodyMesh)
		|| NewTerrain->GetProcMeshSection(0) == nullptr
		|| NewTerrain->GetMaterial(0) != NewTerrainMaterial
		|| (bRenderOcean && (NewOcean->GetProcMeshSection(0) == nullptr
			|| NewOcean->GetMaterial(0) != NewOceanMaterial)))
	{
		NewTerrain->ClearAllMeshSections();
		NewOcean->ClearAllMeshSections();
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Closed preview globe commit validation failed body=%s terrain=%s ocean=%s"),
			*GetNameSafe(Body), *GetNameSafe(NewTerrain->GetMaterial(0)),
			*GetNameSafe(NewOcean->GetMaterial(0)));
		return false;
	}

	BodyMesh->UpdateBounds();
	const FTransform WorldTransform(Body->GetActorQuat(), BodyMesh->Bounds.Origin,
		FVector::OneVector);
	NewTerrain->SetWorldTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	NewOcean->SetWorldTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	NewTerrain->UpdateBounds();
	if (bRenderOcean) NewOcean->UpdateBounds();

	// Game-thread visibility changes are submitted as one render-state update: the
	// previous complete globe remains visible until the new geometry, MID and world
	// centre have all passed validation.
	State->ActiveBuffer = NewBuffer;
	if (NewBuffer == 0)
	{
		State->TerrainMaterialA = NewTerrainMaterial;
		State->OceanMaterialA = NewOceanMaterial;
	}
	else
	{
		State->TerrainMaterialB = NewTerrainMaterial;
		State->OceanMaterialB = NewOceanMaterial;
	}
	State->Body = Body;
	State->ProfileSignature = SurfaceGenerator->AppliedSurfaceProfileSignature;
	State->VertexCount = MeshData.TerrainVertices.Num();
	State->IndexCount = MeshData.Indices.Num();
	State->bHasOcean = bRenderOcean;
	if (State->bUsesDefaultBuffers)
	{
		// Keep reflected references for the constructor-owned first cache entry. Dynamic
		// entries are retained by their registered components and assigned materials.
		if (NewBuffer == 0)
		{
			PreviewTerrainMaterialA = NewTerrainMaterial;
			PreviewOceanMaterialA = NewOceanMaterial;
		}
		else
		{
			PreviewTerrainMaterialB = NewTerrainMaterial;
			PreviewOceanMaterialB = NewOceanMaterial;
		}
	}
	if (Body == ActivePreviewWorldScapeBody.Get())
	{
		UpdateActivePreviewGlobeCompatibilityState();
	}
	SyncPreviewGlobeProxyTransforms();
	SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);

	UE_LOG(LogTemp, Display,
		TEXT("[APS.WorldGeneration] Closed preview globe ready body=%s signature=%u vertices=%d indices=%d previewTerrain=%s previewOcean=%s resolvedTerrain=%s resolvedOcean=%s"),
		*GetNameSafe(Body), State->ProfileSignature, State->VertexCount,
		State->IndexCount,
		*GetNameSafe(NewTerrainMaterial),
		bRenderOcean ? *GetNameSafe(NewOceanMaterial) : TEXT("none"),
		*GetNameSafe(SurfaceGenerator->ResolvedTerrainMaterialInstance),
		bHasOcean ? *GetNameSafe(SurfaceGenerator->ResolvedOceanMaterialInstance) : TEXT("none"));
	return true;
}

void AAstroGenerator::SetPreviewWorldScapeBody(APlanetaryBody* Body)
{
	AWorldScapeRoot* PreviewSurface = PersistentPreviewWorldScapeRoot.Get();
	if (!IsValid(Body))
	{
		SetPreviewGlobeProxyVisible(false);
		ActivePreviewWorldScapeBody.Reset();
		// A pending A edit is allowed to finish after the user opens STAR/SYSTEM. Its
		// retained proxy stays hidden in those scopes and is immediately current when
		// PLANET is reopened; selection never requires a STAR -> PLANET repair pass.
		if (!PreviewSurfaceBuildBody.IsValid())
		{
			BeginNextQueuedPreviewGlobeBuild();
		}
		bPreviewCameraOrbitDragging = false;
		bPreviewSurfaceViewDirty = false;
		bPreviewSurfaceViewRefreshInFlight = false;
		bPreviewSurfaceLiveRefresh = false;
		bPreviewSurfaceRootInitializationPending = false;
		PendingPreviewSurfaceViewPosition = FVector::ZeroVector;
		UpdateActivePreviewGlobeCompatibilityState();
		if (IsValid(PreviewSurface))
		{
			PreviewSurface->SetActorHiddenInGame(true);
			PreviewSurface->SetActorEnableCollision(false);
			// Never let the WorldScape actor tick drain this queue. Its tick executes
			// UpdatePosition before CheckForLodGeneration and can enqueue a new batch in
			// the frame after the old one completes. Keep the producer frozen; this
			// generator polls only CheckForLodGeneration below.
			const bool bDrainWorkers = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
			PreviewSurface->bGenerateWorldScape = bDrainWorkers;
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
			SetActorTickEnabled(bPreviewSurfaceUpdatePending || bDrainWorkers
				|| bPreviewCameraTransitionActive);
		}
		else
		{
			SetActorTickEnabled(bPreviewSurfaceUpdatePending || bPreviewCameraTransitionActive);
		}
		return;
	}

	const bool bSelectionChanged = ActivePreviewWorldScapeBody.Get() != Body;
	ActivePreviewWorldScapeBody = Body;
	bPreviewCameraOrbitDragging = false;
	bPreviewSurfaceViewDirty = false;
	bPreviewSurfaceViewRefreshInFlight = false;
	bPreviewSurfaceLiveRefresh = false;
	bPreviewSurfaceRootInitializationPending = false;
	PendingPreviewSurfaceViewPosition = IsValid(PreviewCamera)
		? PreviewCamera->GetComponentLocation() : FVector::ZeroVector;
	Body->bStreamWorldScapeSurface = false; // the persistent proxy owns menu terrain
	QueuePreviewGlobeFamily(Body);
	const FAPSPreviewGlobeProxyState* CachedState = FindPreviewGlobeProxyState(Body);
	const bool bHasCurrentProxy = CachedState && CachedState->ActiveBuffer != INDEX_NONE;
	Body->bWorldScapeSurfaceReady = bHasCurrentProxy;
	UpdateActivePreviewGlobeCompatibilityState();

	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		InvalidatePreviewGlobeProxy(Body);
		PendingPreviewGlobeBodies.RemoveAll(
			[Body](const TWeakObjectPtr<APlanetaryBody>& Candidate)
			{
				return Candidate.Get() == Body;
			});
		if (!PreviewSurfaceBuildBody.IsValid())
		{
			BeginNextQueuedPreviewGlobeBuild();
		}
		SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
		SetPreviewBodyBackingSphereVisible(Body, true);
		SetActorTickEnabled(bPreviewSurfaceUpdatePending || bPreviewCameraTransitionActive);
		return;
	}

	if (!bHasCurrentProxy)
	{
		// The selected body always preempts a not-yet-started family build. The old
		// candidate remains queued; no previously committed proxy is hidden or moved.
		if (APlanetaryBody* InterruptedBody = PreviewSurfaceBuildBody.Get();
			IsValid(InterruptedBody) && InterruptedBody != Body)
		{
			PendingPreviewGlobeBodies.AddUnique(InterruptedBody);
		}
		PendingPreviewGlobeBodies.RemoveAll(
			[Body](const TWeakObjectPtr<APlanetaryBody>& Candidate)
			{
				return Candidate.Get() == Body;
			});
		PreviewSurfaceBuildBody = Body;
		bPreviewSurfaceUpdatePending = true;
		Body->bWorldScapeSurfaceReady = false;
	}
	else if (!PreviewSurfaceBuildBody.IsValid())
	{
		BeginNextQueuedPreviewGlobeBuild();
	}

	SyncPreviewGlobeProxyTransforms();
	SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
	SetPreviewBodyBackingSphereVisible(Body, !bHasCurrentProxy);
	if (bSelectionChanged && bHasCurrentProxy)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[APS.WorldGeneration] Reused retained body globe body=%s key=%s signature=%u retained=%d"),
			*GetNameSafe(Body), *GetPreviewBodyStableKey(Body),
			CachedState->ProfileSignature, GetRetainedPreviewGlobeCount());
	}
	// Do not start WorldScape synchronously from the button handler.  Focus callers
	// establish the new camera target immediately after this function returns.  If
	// generation begins here it samples the previous SYSTEM/CLUSTER camera position,
	// builds the wrong hemisphere, then tears those LODs down again while zooming to
	// the planet.  Let the generator tick once after the target transform is ready.
	SetActorTickEnabled(bPreviewSurfaceUpdatePending || bPreviewCameraTransitionActive);
}

bool AAstroGenerator::RefreshPreviewPlanetAppearance(
	UGeneratedWorld* InGeneratedWorld, const bool bRegenerateSurface)
{
	if (!bIsPreviewGeneration || !IsValid(InGeneratedWorld))
	{
		return false;
	}

	APlanetaryBody* Body = ActivePreviewWorldScapeBody.Get();
	if (!IsValid(Body))
	{
		Body = Cast<APlanetaryBody>(SelectedPreviewBodyActor.Get());
	}
	if (!IsValid(Body))
	{
		Body = HomePlanet;
	}
	if (!IsValid(Body))
	{
		return false;
	}

	constexpr double EarthRadiusKm = 6371.0;
	const double RadiusKm = FMath::Clamp(InGeneratedWorld->PlanetRadius, 100.0, 20000.0);
	ApplyPlanetaryBodyRadius(*Body, RadiusKm);
	Body->PlanetType = InGeneratedWorld->PlanetType;
	Body->WorldScapeSeed = FMath::Clamp(InGeneratedWorld->PlanetSurfaceSeed, 0, 999983);
	Body->SurfaceFeatureScale = FMath::Clamp(InGeneratedWorld->SurfaceFeatureScale, 0.25, 4.0);
	Body->SurfaceReliefScale = FMath::Clamp(InGeneratedWorld->SurfaceReliefScale, 0.25, 2.5);
	Body->SurfaceLandCoverageScale = FMath::Clamp(InGeneratedWorld->SurfaceLandCoverageScale, 0.25, 2.0);
	Body->SurfaceMountainScale = FMath::Clamp(InGeneratedWorld->SurfaceMountainScale, 0.0, 2.0);
	Body->SurfaceCraterScale = FMath::Clamp(InGeneratedWorld->SurfaceCraterScale, 0.0, 2.0);
	Body->SurfaceRoughnessScale = FMath::Clamp(InGeneratedWorld->SurfaceRoughnessScale, 0.25, 2.0);
	Body->AtmosphereHeight = FMath::Max(0.0, InGeneratedWorld->AtmosphereHeight);
	if (APlanet* Planet = Cast<APlanet>(Body))
	{
		Planet->SetPlanetType(InGeneratedWorld->PlanetType);
		if (!Planet->PlanetData.PlanetModel.IsValid())
		{
			Planet->PlanetData.PlanetModel = MakeShared<FPlanetModel>();
		}
		FPlanetModel& Model = *Planet->PlanetData.PlanetModel;
		Model.PlanetType = Body->PlanetType;
		Model.Radius = static_cast<float>(RadiusKm / EarthRadiusKm);
		Model.RadiusKM = static_cast<float>(RadiusKm);
		Model.SurfaceSeed = Body->WorldScapeSeed;
		Model.SurfaceFeatureScale = Body->SurfaceFeatureScale;
		Model.SurfaceReliefScale = Body->SurfaceReliefScale;
		Model.SurfaceLandCoverageScale = Body->SurfaceLandCoverageScale;
		Model.SurfaceMountainScale = Body->SurfaceMountainScale;
		Model.SurfaceCraterScale = Body->SurfaceCraterScale;
		Model.SurfaceRoughnessScale = Body->SurfaceRoughnessScale;
		Model.AtmosphereHeight = Body->AtmosphereHeight;
	}
	else if (AMoon* Moon = Cast<AMoon>(Body))
	{
		if (!Moon->GenerationModel.IsValid())
		{
			Moon->GenerationModel = MakeShared<FMoonModel>();
		}
		FMoonModel& Model = *Moon->GenerationModel;
		Model.PlanetType = Body->PlanetType;
		Model.Radius = static_cast<float>(RadiusKm / EarthRadiusKm);
		Model.RadiusKM = static_cast<float>(RadiusKm);
		Model.SurfaceSeed = Body->WorldScapeSeed;
		Model.SurfaceFeatureScale = Body->SurfaceFeatureScale;
		Model.SurfaceReliefScale = Body->SurfaceReliefScale;
		Model.SurfaceLandCoverageScale = Body->SurfaceLandCoverageScale;
		Model.SurfaceMountainScale = Body->SurfaceMountainScale;
		Model.SurfaceCraterScale = Body->SurfaceCraterScale;
		Model.SurfaceRoughnessScale = Body->SurfaceRoughnessScale;
		Model.MoonAtmosphereHeight = Body->AtmosphereHeight;
	}

	if (IsValid(Body->PlanetaryEnvironmentGenerator)
		&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
	{
		AAtmoScape* Atmosphere = Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
		Atmosphere->bKeepRelativeScale = false;
		Atmosphere->PlanetRadius = FMath::Max(RadiusKm - 1.0, 0.5);
		Atmosphere->AtmosphereHeight = FMath::Max(0.0, InGeneratedWorld->AtmosphereHeight);
		Atmosphere->AtmosphereOpacity = FMath::Max(0.0, InGeneratedWorld->AtmosphereOpacity);
		Atmosphere->MultiScatering = FMath::Max(0.01, InGeneratedWorld->AtmosphereMultiScattering);
		Atmosphere->RayleighHeight = FMath::Max(0.0, InGeneratedWorld->AtmosphereRayleighScattering);
		Atmosphere->RayleighScattering = InGeneratedWorld->AtmosphereColor;
		Atmosphere->UpdateScale();
		Atmosphere->SetActorHiddenInGame(false);
	}
	// UpdateScale performs AtmoScape's own inside/outside visibility switch. Restore
	// the menu's stable space shell and normalized body scale without touching the camera.
	ApplyPreviewFocusPresentation(PreviewFocus);

	if (bRegenerateSurface)
	{
		// Preserve the last complete closed proxy until its replacement has been fully
		// sampled and assigned. Solid -> gas is the only transition that deliberately
		// returns to the body's authored astronomical material.
		ActivePreviewWorldScapeBody = Body;
		Body->bWorldScapeSurfaceReady = false;
		const FAPSPreviewGlobeProxyState* CurrentState = FindPreviewGlobeProxyState(Body);
		const bool bHasCurrentProxy = CurrentState
			&& CurrentState->ActiveBuffer != INDEX_NONE;
		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
		{
			InvalidatePreviewGlobeProxy(Body);
			if (APlanetaryBody* InterruptedBody = PreviewSurfaceBuildBody.Get();
				IsValid(InterruptedBody) && InterruptedBody != Body)
			{
				PendingPreviewGlobeBodies.AddUnique(InterruptedBody);
			}
			PreviewSurfaceBuildBody.Reset();
			PendingPreviewGlobeBodies.RemoveAll(
				[Body](const TWeakObjectPtr<APlanetaryBody>& Candidate)
				{
					return Candidate.Get() == Body;
				});
			BeginNextQueuedPreviewGlobeBuild();
			SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
			SetPreviewBodyBackingSphereVisible(Body, true);
			SetActorTickEnabled(bPreviewSurfaceUpdatePending || bPreviewCameraTransitionActive);
			return true;
		}
		if (APlanetaryBody* InterruptedBody = PreviewSurfaceBuildBody.Get();
			IsValid(InterruptedBody) && InterruptedBody != Body)
		{
			PendingPreviewGlobeBodies.AddUnique(InterruptedBody);
		}
		PendingPreviewGlobeBodies.RemoveAll(
			[Body](const TWeakObjectPtr<APlanetaryBody>& Candidate)
			{
				return Candidate.Get() == Body;
			});
		PreviewSurfaceBuildBody = Body;
		SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
		SetPreviewBodyBackingSphereVisible(Body, !bHasCurrentProxy);
		bPreviewSurfaceUpdatePending = true;
		bPreviewSurfaceViewDirty = false;
		bPreviewSurfaceViewRefreshInFlight = false;
		bPreviewSurfaceLiveRefresh = false;
		bPreviewSurfaceRootInitializationPending = false;
		if (AWorldScapeRoot* PreviewSurface = PersistentPreviewWorldScapeRoot.Get())
		{
			PreviewSurface->SetActorHiddenInGame(true);
			// Do not destroy or force-regenerate while WorldScape still owns worker
			// output. Keep UpdatePosition frozen; UpdatePreviewWorldScape polls only the
			// current worker batch and then applies the latest model atomically.
			const bool bDrainWorkers = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
			PreviewSurface->bGenerateWorldScape = bDrainWorkers;
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
		}
		SetActorTickEnabled(true);
	}
	return true;
}

bool AAstroGenerator::PreparePreviewForTravel()
{
	SetPreviewWorldScapeBody(nullptr);
	AWorldScapeRoot* PreviewSurface = PersistentPreviewWorldScapeRoot.Get();
	if (!IsValid(PreviewSurface))
	{
		return true;
	}
	// Also poll from the travel retry callback so teardown progress does not depend
	// on actor tick order. The root stays frozen and therefore cannot start another
	// camera-follow batch while the last result is being committed.
	UpdatePreviewWorldScape();
	return PreviewSurface->WorldScapeLodInGeneration.Num() == 0;
}

void AAstroGenerator::UpdatePreviewWorldScape()
{
	APlanetaryBody* Body = ActivePreviewWorldScapeBody.Get();
	AWorldScapeRoot* PreviewSurface = PersistentPreviewWorldScapeRoot.Get();
	APlanetarySurfaceGenerator* SurfaceGenerator = PersistentPreviewSurfaceGenerator.Get();
	if (bIsPreviewGeneration)
	{
		const auto FreezeProfileRoot = [](AWorldScapeRoot* Root)
		{
			if (!IsValid(Root)) return;
			Root->SetActorHiddenInGame(true);
			Root->SetActorEnableCollision(false);
			Root->bFreezeGeneration = true;
			Root->SetActorTickEnabled(false);
			Root->bGenerateWorldScape = Root->WorldScapeLodInGeneration.Num() > 0;
		};

		APlanetaryBody* FocusedBody = Body;
		if (!IsValid(FocusedBody))
		{
			SetPreviewGlobeProxyVisible(false);
		}

		if (!PreviewSurfaceBuildBody.IsValid())
		{
			BeginNextQueuedPreviewGlobeBuild();
		}
		Body = PreviewSurfaceBuildBody.Get();
		if (!IsValid(Body))
		{
			if (!IsValid(FocusedBody))
			{
				FreezeProfileRoot(PreviewSurface);
				if (IsValid(PreviewSurface)
					&& PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
				{
					// Compatibility drain for a PIE session that entered PLANET before this
					// proxy path was installed. No new preview worker is ever launched here.
					PreviewSurface->CheckForLodGeneration();
					FreezeProfileRoot(PreviewSurface);
				}
				return;
			}
			const FAPSPreviewGlobeProxyState* FocusedState =
				FindPreviewGlobeProxyState(FocusedBody);
			const bool bFocusedProxyReady = FocusedState
				&& FocusedState->ActiveBuffer != INDEX_NONE;
			FocusedBody->bWorldScapeSurfaceReady = bFocusedProxyReady;
			UpdateActivePreviewGlobeCompatibilityState();
			SyncPreviewGlobeProxyTransforms();
			SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
			SetPreviewBodyBackingSphereVisible(FocusedBody, !bFocusedProxyReady);
			FreezeProfileRoot(PreviewSurface);
			return;
		}

		const auto CompleteCurrentBuild = [this, FocusedBody, Body]()
		{
			PreviewSurfaceBuildBody.Reset();
			bPreviewSurfaceUpdatePending = false;
			const bool bStartedNextBuild = BeginNextQueuedPreviewGlobeBuild();
			// Family warm-up intentionally reuses one resolver. Once the last moon has
			// been sampled, restore the resolver/root payload to the editor target without
			// rebuilding its retained proxy; diagnostics and subsequent edits then observe
			// the selected body as authoritative.
			if (!bStartedNextBuild && IsValid(FocusedBody) && FocusedBody != Body
				&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(FocusedBody->PlanetType))
			{
				if (APlanetarySurfaceGenerator* Resolver = PersistentPreviewSurfaceGenerator.Get())
				{
					Resolver->PlanetaryBody = FocusedBody;
					Resolver->ApplySurfaceProfile(FocusedBody);
					++PreviewSurfaceProfileApplyCount;
				}
				if (AWorldScapeRoot* Root = PersistentPreviewWorldScapeRoot.Get())
				{
					Root->SetActorHiddenInGame(true);
					Root->SetActorEnableCollision(false);
					Root->bFreezeGeneration = true;
					Root->bGenerateWorldScape = false;
					Root->SetActorTickEnabled(false);
				}
			}
			UpdateActivePreviewGlobeCompatibilityState();
			SyncPreviewGlobeProxyTransforms();
			SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
			const FAPSPreviewGlobeProxyState* FocusedState =
				FindPreviewGlobeProxyState(FocusedBody);
			SetPreviewBodyBackingSphereVisible(FocusedBody,
				!FocusedState || FocusedState->ActiveBuffer == INDEX_NONE);
			SetActorTickEnabled(bPreviewSurfaceUpdatePending || bPreviewCameraTransitionActive);
		};

		if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
		{
			Body->bWorldScapeSurfaceReady = false;
			InvalidatePreviewGlobeProxy(Body);
			SetPreviewBodyBackingSphereVisible(Body, true);
			FreezeProfileRoot(PreviewSurface);
			CompleteCurrentBuild();
			return;
		}

		if (!IsValid(SurfaceGenerator))
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.ObjectFlags |= RF_Transient;
			SpawnInfo.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SurfaceGenerator = GetWorld()->SpawnActor<APlanetarySurfaceGenerator>(
				APlanetarySurfaceGenerator::StaticClass(), GetActorTransform(), SpawnInfo);
			if (!IsValid(SurfaceGenerator))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.WorldGeneration] Failed to create preview profile resolver"));
				CompleteCurrentBuild();
				return;
			}
			SurfaceGenerator->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			SurfaceGenerator->SetActorHiddenInGame(true);
			SurfaceGenerator->SetActorEnableCollision(false);
			SurfaceGenerator->SetActorTickEnabled(false);
			PersistentPreviewSurfaceGenerator = SurfaceGenerator;
		}

		if (!IsValid(PreviewSurface))
		{
			if (!SurfaceGenerator->CreateRuntimeWorldScapeRoot(Body))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.WorldGeneration] Failed to create inert preview profile root for %s"),
					*GetNameSafe(Body));
				CompleteCurrentBuild();
				return;
			}
			PreviewSurface = SurfaceGenerator->WorldScapeRootInstance;
			if (!IsValid(PreviewSurface))
			{
				CompleteCurrentBuild();
				return;
			}
			PreviewSurface->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			PreviewSurface->SetOwner(this);
			SurfaceGenerator->bOwnsWorldScapeRootInstance = false;
			PersistentPreviewWorldScapeRoot = PreviewSurface;
		}

		FreezeProfileRoot(PreviewSurface);
		if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
		{
			PreviewSurface->CheckForLodGeneration();
			FreezeProfileRoot(PreviewSurface);
			return;
		}

		UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(
			Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		if (!IsValid(BodyMesh))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Preview body %s has no authored sphere"),
				*GetNameSafe(Body));
			CompleteCurrentBuild();
			return;
		}
		BodyMesh->UpdateBounds();
		const double VisualRadiusCm = BodyMesh->Bounds.SphereRadius;
		const double PhysicalRadiusCm = FMath::Max(
			Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM)) * 100000.0;
		Body->WorldScapePresentationScale = FMath::Clamp(
			VisualRadiusCm / FMath::Max(PhysicalRadiusCm, 1.0), 1.0e-9, 1.0);

		SurfaceGenerator->PlanetaryBody = Body;
		const uint32 PreviousProfileSignature =
			SurfaceGenerator->AppliedSurfaceProfileSignature;
		SurfaceGenerator->ApplySurfaceProfile(Body);
		++PreviewSurfaceProfileApplyCount;
		FreezeProfileRoot(PreviewSurface);
		PreviewSurface->SetActorLocation(BodyMesh->Bounds.Origin);
		PreviewSurface->SetActorRotation(Body->GetActorRotation());
		PreviewSurface->SetActorScale3D(FVector::OneVector);
		UE_LOG(LogTemp, Display,
			TEXT("[APS.WorldGeneration] Preview globe profile apply #%d body=%s old=%u new=%u seed=%d"),
			PreviewSurfaceProfileApplyCount, *GetNameSafe(Body), PreviousProfileSignature,
			SurfaceGenerator->AppliedSurfaceProfileSignature, Body->WorldScapeSeed);

		if (!SurfaceGenerator->IsSurfaceProfileCurrent(Body))
		{
			Body->bWorldScapeSurfaceReady = false;
			const FAPSPreviewGlobeProxyState* OldState = FindPreviewGlobeProxyState(Body);
			const bool bHasOldProxy = OldState && OldState->ActiveBuffer != INDEX_NONE;
			SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
			SetPreviewBodyBackingSphereVisible(Body, !bHasOldProxy);
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Resolved preview profile is incomplete body=%s"),
				*GetNameSafe(Body));
			CompleteCurrentBuild();
			return;
		}

		const bool bCommitted = BuildPreviewGlobeProxy(
			Body, SurfaceGenerator, PreviewSurface);
		Body->bWorldScapeSurfaceReady = bCommitted;
		if (!bCommitted)
		{
			const FAPSPreviewGlobeProxyState* OldState = FindPreviewGlobeProxyState(Body);
			const bool bHasOldProxy = OldState && OldState->ActiveBuffer != INDEX_NONE;
			SetPreviewGlobeProxyVisible(PreviewFocus == EAstroPreviewFocus::HomePlanet);
			SetPreviewBodyBackingSphereVisible(Body, !bHasOldProxy);
		}
		if (Body == FocusedBody && IsValid(Body->PlanetaryEnvironmentGenerator)
			&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
		{
			Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere->SetActorHiddenInGame(false);
		}
		CompleteCurrentBuild();
		return;
	}
	if (!IsValid(Body))
	{
		if (IsValid(PreviewSurface))
		{
			PreviewSurface->SetActorHiddenInGame(true);
			PreviewSurface->SetActorEnableCollision(false);
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
			if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
			{
				PreviewSurface->bGenerateWorldScape = true;
				PreviewSurface->CheckForLodGeneration();
			}
			if (PreviewSurface->WorldScapeLodInGeneration.Num() == 0)
			{
				PreviewSurface->bGenerateWorldScape = false;
			}
		}
		return;
	}

	const bool bSolidSurface = UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType);
	if (!bSolidSurface)
	{
		bPreviewSurfaceUpdatePending = false;
		Body->bWorldScapeSurfaceReady = false;
		SetPreviewBodyBackingSphereVisible(Body, true);
		bool bDrainWorkers = false;
		if (IsValid(PreviewSurface))
		{
			PreviewSurface->SetActorHiddenInGame(true);
			PreviewSurface->SetActorEnableCollision(false);
			bDrainWorkers = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
			PreviewSurface->bGenerateWorldScape = bDrainWorkers;
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
			if (bDrainWorkers)
			{
				PreviewSurface->CheckForLodGeneration();
				bDrainWorkers = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
				PreviewSurface->bGenerateWorldScape = bDrainWorkers;
			}
		}
		if (!bDrainWorkers)
		{
			ActivePreviewWorldScapeBody.Reset();
		}
		if (IsValid(Body->PlanetaryEnvironmentGenerator)
			&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
		{
			Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere->SetActorHiddenInGame(false);
		}
		return;
	}

	if (!IsValid(SurfaceGenerator))
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		SpawnInfo.ObjectFlags |= RF_Transient;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SurfaceGenerator = GetWorld()->SpawnActor<APlanetarySurfaceGenerator>(
			APlanetarySurfaceGenerator::StaticClass(), GetActorTransform(), SpawnInfo);
		if (!IsValid(SurfaceGenerator))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Failed to create persistent preview surface generator"));
			return;
		}
		SurfaceGenerator->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		SurfaceGenerator->SetActorHiddenInGame(true);
		SurfaceGenerator->SetActorEnableCollision(false);
		SurfaceGenerator->SetActorTickEnabled(false);
		PersistentPreviewSurfaceGenerator = SurfaceGenerator;
	}

	if (!IsValid(PreviewSurface))
	{
		if (!SurfaceGenerator->CreateRuntimeWorldScapeRoot(Body))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.WorldGeneration] Failed to create preview WorldScape root for %s"),
				*GetNameSafe(Body));
			return;
		}
		PreviewSurface = SurfaceGenerator->WorldScapeRootInstance;
		if (!IsValid(PreviewSurface)) return;
		PreviewSurface->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		PreviewSurface->SetOwner(this);
		SurfaceGenerator->bOwnsWorldScapeRootInstance = false;
		PersistentPreviewWorldScapeRoot = PreviewSurface;
	}

	if (IsValid(PreviewCamera))
	{
		PreviewSurface->bOverridePlayerPosition = true;
		// Generate for the final camera direction while the visible camera is still
		// blending.  This makes the fallback sphere hand-off atomic and avoids a
		// second expensive LOD pass at the end of every focus transition.
		PreviewSurface->OverridedPlayerPosition = bPreviewCameraTransitionActive
			? PreviewCameraTargetTransform.GetLocation()
			: PreviewCamera->GetComponentLocation();
	}

	if (bPreviewCameraOrbitDragging)
	{
		// WorldScape already commits a moved LOD hierarchy as a complete worker
		// batch. Keep that supported camera-follow path alive while RMB is held;
		// hiding it here was the visible "planet disappears while rotating" bug.
		PreviewSurface->SetActorHiddenInGame(false);
		SetPreviewBodyBackingSphereVisible(Body, false);
		PreviewSurface->bOverridePlayerPosition = true;
		PreviewSurface->OverridedPlayerPosition = PendingPreviewSurfaceViewPosition;
		PreviewSurface->bGenerateWorldScape = true;
		PreviewSurface->bFreezeGeneration = false;
		PreviewSurface->SetActorTickEnabled(true);
		return;
	}

	if (bPreviewSurfaceRootInitializationPending)
	{
		// The preceding frame gave a fully drained root one ordinary plugin tick. That
		// tick is WorldScape's supported synchronization point: it rebuilds changed base
		// LODs and enqueues their SetData jobs as one coherent batch. Freeze it before a
		// second UpdatePosition can run; AstroGenerator owns completion polling below.
		const bool bHasInitializedLods = PreviewSurface->WorldScapeLod.Num() > 0;
		const bool bHasInitializationWorkers =
			PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
		if (!bHasInitializedLods && !bHasInitializationWorkers)
		{
			PreviewSurface->bGenerateWorldScape = true;
			PreviewSurface->bFreezeGeneration = false;
			PreviewSurface->SetActorTickEnabled(true);
			return;
		}

		bPreviewSurfaceRootInitializationPending = false;
		PreviewSurface->bFreezeGeneration = true;
		PreviewSurface->SetActorTickEnabled(false);
		bPreviewSurfaceLiveRefresh = false;
		bPreviewSurfaceViewRefreshInFlight = bHasInitializationWorkers;
	}

	if (bPreviewSurfaceViewRefreshInFlight)
	{
		if (bPreviewSurfaceLiveRefresh)
		{
			// Let WorldScape's normal tick own UpdatePosition and completion polling.
			// Its worker map is replaced only after the complete moved batch is ready,
			// so the previously committed globe remains drawable during the refresh.
			PreviewSurface->SetActorHiddenInGame(false);
			SetPreviewBodyBackingSphereVisible(Body, false);
			PreviewSurface->bGenerateWorldScape = true;
			PreviewSurface->bFreezeGeneration = false;
			PreviewSurface->SetActorTickEnabled(true);
			if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
			{
				return;
			}
		}
		else
		{
			// Profile/type changes still use the authored sphere until their new
			// payload is complete; only camera movement is allowed to retain terrain.
			PreviewSurface->SetActorHiddenInGame(true);
			SetPreviewBodyBackingSphereVisible(Body, true);
			if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
			{
				PreviewSurface->bGenerateWorldScape = true;
				PreviewSurface->bFreezeGeneration = true;
				PreviewSurface->SetActorTickEnabled(false);
				PreviewSurface->CheckForLodGeneration();
				if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
				{
					return;
				}
			}
		}
		bPreviewSurfaceViewRefreshInFlight = false;
		bPreviewSurfaceLiveRefresh = false;
	}

	if (bPreviewSurfaceViewDirty && !bPreviewSurfaceUpdatePending)
	{
		bPreviewSurfaceViewDirty = false;
		Body->bWorldScapeSurfaceReady = false;
		PreviewSurface->SetActorHiddenInGame(false);
		SetPreviewBodyBackingSphereVisible(Body, false);
		PreviewSurface->bOverridePlayerPosition = true;
		PreviewSurface->OverridedPlayerPosition = PendingPreviewSurfaceViewPosition;
		PreviewSurface->bGenerateWorldScape = true;
		PreviewSurface->bFreezeGeneration = false;
		PreviewSurface->SetActorTickEnabled(true);
		// Give the plugin one ordinary tick before inspecting its worker map. Calling
		// UpdatePosition manually here races the root's own tick and was the source of
		// torn camera-centred batches.
		bPreviewSurfaceLiveRefresh = true;
		bPreviewSurfaceViewRefreshInFlight = true;
		return;
	}
	if (Body->bWorldScapeSurfaceReady && !bPreviewSurfaceUpdatePending
		&& !bPreviewCameraTransitionActive && !bPreviewSurfaceViewDirty
		&& !bPreviewSurfaceViewRefreshInFlight && !bPreviewCameraOrbitDragging)
	{
		// The menu displays a complete orbital globe, so a ready root has no useful
		// camera-follow work left. Freeze both producer actors and preserve the committed
		// LODs; an explicit type/seed change wakes this generator and regenerates them.
		const bool bDrainWorkers = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
		PreviewSurface->bGenerateWorldScape = bDrainWorkers;
		PreviewSurface->bFreezeGeneration = true;
		PreviewSurface->SetActorTickEnabled(false);
		if (bDrainWorkers)
		{
			PreviewSurface->CheckForLodGeneration();
			PreviewSurface->bGenerateWorldScape =
				PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
		}
		PreviewSurface->SetActorHiddenInGame(false);
		SetPreviewBodyBackingSphereVisible(Body, false);
		if (IsValid(Body->PlanetaryEnvironmentGenerator)
			&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
		{
			Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere->SetActorHiddenInGame(false);
		}
		return;
	}

	if (bPreviewSurfaceUpdatePending)
	{
		PreviewSurface->SetActorHiddenInGame(true);
		if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
		{
			// Consume completed results directly without running WorldScape's actor tick.
			// Its UpdatePosition executes before CheckForLodGeneration and can otherwise
			// enqueue another old-profile batch before this type change is applied.
			PreviewSurface->bGenerateWorldScape = true;
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
			PreviewSurface->CheckForLodGeneration();
			if (PreviewSurface->WorldScapeLodInGeneration.Num() > 0)
			{
				return;
			}
		}
		PreviewSurface->bFreezeGeneration = true;

		UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(
			Body->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		if (!IsValid(BodyMesh)) return;
		BodyMesh->UpdateBounds();
		const double VisualRadiusCm = BodyMesh->Bounds.SphereRadius;
		const double PhysicalRadiusCm = FMath::Max(
			Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM)) * 100000.0;
		Body->WorldScapePresentationScale = FMath::Clamp(
			VisualRadiusCm / FMath::Max(PhysicalRadiusCm, 1.0), 1.0e-9, 1.0);

		SurfaceGenerator->PlanetaryBody = Body;
		const uint32 PreviousProfileSignature = SurfaceGenerator->AppliedSurfaceProfileSignature;
		SurfaceGenerator->ApplySurfaceProfile(Body);
		++PreviewSurfaceProfileApplyCount;
		UE_LOG(LogTemp, Display,
			TEXT("[APS.WorldGeneration] Preview surface profile apply #%d body=%s old=%u new=%u seed=%d feature=%.3f relief=%.3f land=%.3f mountain=%.3f crater=%.3f roughness=%.3f"),
			PreviewSurfaceProfileApplyCount, *GetNameSafe(Body), PreviousProfileSignature,
			SurfaceGenerator->AppliedSurfaceProfileSignature, Body->WorldScapeSeed,
			Body->SurfaceFeatureScale, Body->SurfaceReliefScale,
			Body->SurfaceLandCoverageScale, Body->SurfaceMountainScale,
			Body->SurfaceCraterScale, Body->SurfaceRoughnessScale);
		if (!SurfaceGenerator->IsSurfaceProfileCurrent(Body))
		{
			// Asset/profile resolution failed. Keep the deterministic fallback visible
			// and do not ask WorldScape to regenerate with an incomplete configuration.
			// This is a terminal fallback state for this attempt: retaining the active
			// body here left the generator ticking forever and could later expose stale
			// LOD arrays from the previous subtype as if they belonged to the new one.
			bPreviewSurfaceUpdatePending = false;
			PreviewSurface->bGenerateWorldScape = false;
			PreviewSurface->bFreezeGeneration = true;
			PreviewSurface->SetActorTickEnabled(false);
			PreviewSurface->SetActorHiddenInGame(true);
			SetPreviewBodyBackingSphereVisible(Body, true);
			if (IsValid(Body->PlanetaryEnvironmentGenerator)
				&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
			{
				Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere->SetActorHiddenInGame(false);
			}
			ActivePreviewWorldScapeBody.Reset();
			return;
		}
		// Index 7 was only ~2.76e6 cm across for a 2.4e6 cm preview globe and left
		// almost no tangent/horizon padding, producing a visibly torn silhouette.
		// Index 8 (~5.53e6 cm) is the smallest bounded ring that covers the full disc.
		PreviewSurface->MaxLod = 9;
		PreviewSurface->LodResolution = 48;
		PreviewSurface->TriangleSize = 450.0f;
		PreviewSurface->OceanMaxLod = 9;
		PreviewSurface->OceanLodResolution = 32;
		PreviewSurface->OceanTriangleSize = 650.0f;
		// This is a visual orbital preview: collision, foliage and volume sampling
		// cannot contribute to the image, but WorldScape enables them by default and
		// otherwise spends most of the entry hitch building invisible gameplay data.
		PreviewSurface->bGenerateCollision = false;
		PreviewSurface->bGenerateCollisionInEditor = false;
		PreviewSurface->bGenerateFoliages = false;
		PreviewSurface->bEnableVolumes = false;
		PreviewSurface->EnabledGrid = false;
		PreviewSurface->bGenerateTangents = false;
		// WorldScape's editor-only distance check ignores bOverridePlayerPosition and
		// can freeze a PIE preview against the editor viewport camera. Zero disables
		// only that automatic freeze; explicit focus changes still freeze the proxy.
		PreviewSurface->DistanceToFreezeGeneration = 0.0f;
		PreviewSurface->SetActorLocation(BodyMesh->Bounds.Origin);
		PreviewSurface->SetActorRotation(Body->GetActorRotation());
		PreviewSurface->SetActorScale3D(FVector::OneVector);
		PreviewSurface->SetActorEnableCollision(false);
		// At this point the worker map is proven empty by the drain guard above. Give
		// WorldScape exactly one normal tick so its own CheckForRegenerate ->
		// GenerateBaseMesh -> UpdatePosition ordering creates the new profile safely.
		// Never call WS_ForceRegenerate here: it can CleanComponents while an async
		// SetData completion is crossing threads in WorldScape 5.4.
		PreviewSurface->bGenerateWorldScape = true;
		PreviewSurface->bFreezeGeneration = false;
		PreviewSurface->SetActorTickEnabled(true);
		bPreviewSurfaceRootInitializationPending = true;
		bPreviewSurfaceUpdatePending = false;
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldGeneration] Persistent preview WorldScape body=%s scale=%.3e"),
			*GetNameSafe(Body), Body->WorldScapePresentationScale);
		// The root now owns its single initialization tick. Do not inspect/show the old
		// LOD payload from the previous subtype during this hand-off frame.
		return;
	}

	const bool bWorkersInFlight = PreviewSurface->WorldScapeLodInGeneration.Num() > 0;
	const FVector DesiredSurfaceNormal = PreviewSurface->WorldToECEF(
		PreviewSurface->OverridedPlayerPosition).ToFVector().GetSafeNormal();
	int32 ExistingTerrainLods = 0;
	int32 ReadyTerrainLods = 0;
	// GenerateBaseMesh creates three flat sections synchronously, before any noise
	// job has written the resolved planet profile. Count only LODs whose default
	// white vertex payload was replaced by a completed async SetData/UpdateMesh pass.
	// The worker map must be empty before reading those arrays.
	for (const UWorldScapeLod* Lod : PreviewSurface->WorldScapeLod)
	{
		if (!IsValid(Lod) || !IsValid(Lod->Mesh) || Lod->Mesh->GetNumSections() == 0)
		{
			continue;
		}
		++ExistingTerrainLods;
		if (!bWorkersInFlight)
		{
			const bool bHasGeneratedVertexData =
				APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
					Lod, DesiredSurfaceNormal, true);
			if (bHasGeneratedVertexData)
			{
				++ReadyTerrainLods;
			}
		}
	}
	const int32 RequiredTerrainLods = PreviewSurface->WorldScapeLod.Num();
	int32 ReadyOceanLods = 0;
	for (const UWorldScapeLod* Lod : PreviewSurface->WorldScapeLodOcean)
	{
		if (!APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
			Lod, DesiredSurfaceNormal, false))
		{
			continue;
		}
		++ReadyOceanLods;
	}
	const bool bOceanReady = !PreviewSurface->bOcean
		|| (PreviewSurface->WorldScapeLodOcean.Num() > 0
			&& ReadyOceanLods == PreviewSurface->WorldScapeLodOcean.Num());
	const bool bProfileCurrent = IsValid(SurfaceGenerator)
		&& SurfaceGenerator->IsSurfaceProfileCurrent(Body);
	const bool bSurfaceReady = !bPreviewSurfaceUpdatePending
		&& !bWorkersInFlight
		&& !bPreviewCameraTransitionActive
		&& bProfileCurrent
		&& RequiredTerrainLods >= PreviewSurface->MaxLod
		&& ExistingTerrainLods == RequiredTerrainLods
		&& ReadyTerrainLods == RequiredTerrainLods
		&& bOceanReady;
	const bool bWasSurfaceReady = Body->bWorldScapeSurfaceReady;
	Body->bWorldScapeSurfaceReady = bSurfaceReady;
	// Atomic hand-off: keep the fallback until the resolved-noise vertex payload is
	// committed on a bounded terrain set and the current worker batch is idle.
	PreviewSurface->SetActorHiddenInGame(!bSurfaceReady);
	if (bSurfaceReady && !bWasSurfaceReady)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldGeneration] Preview WorldScape ready body=%s terrainLods=%d"),
			*GetNameSafe(Body), ReadyTerrainLods);
	}
	if (bSurfaceReady && !bWorkersInFlight)
	{
		// Keep the finished mesh resident but stop WorldScape from evaluating the same
		// orbital camera and material state every frame.
		PreviewSurface->bGenerateWorldScape = false;
		PreviewSurface->bFreezeGeneration = true;
		PreviewSurface->SetActorTickEnabled(false);
	}

	SetPreviewBodyBackingSphereVisible(Body, !bSurfaceReady);
	if (IsValid(Body->PlanetaryEnvironmentGenerator)
		&& IsValid(Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
	{
		Body->PlanetaryEnvironmentGenerator->PlanetAtmosphere->SetActorHiddenInGame(false);
	}
}

void AAstroGenerator::GetPreviewBodyEntries(TArray<FAPSPreviewBodyEntry>& OutEntries) const
{
	OutEntries.Reset();
	if (PreviewFocus == EAstroPreviewFocus::Overview)
	{
		const auto AddHierarchyEntry = [&OutEntries](const TCHAR* Label, const TCHAR* Details,
			EAstroPreviewFocus Focus, int32 Depth)
		{
			FAPSPreviewBodyEntry Entry;
			Entry.Label = FText::FromString(Label);
			Entry.Details = FText::FromString(Details);
			Entry.Depth = Depth;
			Entry.PreviewFocusValue = static_cast<int32>(Focus);
			OutEntries.Add(MoveTemp(Entry));
		};
		if (IsValid(GeneratedGalaxy))
		{
			AddHierarchyEntry(TEXT("GALAXY"), TEXT("FULL-SCALE PARENT / BOUNDED LIVE SAMPLE"),
				EAstroPreviewFocus::Galaxy, 0);
		}
		if (IsValid(GeneratedStarCluster))
		{
			AddHierarchyEntry(TEXT("HOME STAR CLUSTER"), TEXT("SYSTEM RECORDS / HISM STAR FIELD"),
				EAstroPreviewFocus::StarCluster, 1);
		}
		if (IsValid(GeneratedHomeStarSystem))
		{
			AddHierarchyEntry(TEXT("HOME STAR SYSTEM"), TEXT("STARS / PLANETS / MOONS"),
				EAstroPreviewFocus::HomeSystem, 2);
		}
		return;
	}
	if (PreviewFocus == EAstroPreviewFocus::Galaxy)
	{
		if (IsValid(GeneratedStarCluster))
		{
			FAPSPreviewBodyEntry Entry;
			Entry.Label = FText::FromString(TEXT("HOME STAR CLUSTER"));
			Entry.Details = FText::FromString(FString::Printf(
				TEXT("%d MODELED SYSTEM RECORDS / %d LIVE SAMPLES / %s"),
				FMath::Max(GeneratedStarCluster->ModeledStarAmount,
					GeneratedStarCluster->PotentialStarSystems.Num()),
				GeneratedStarCluster->PotentialStarSystems.Num(),
				*UEnum::GetDisplayValueAsText(GeneratedStarCluster->ClusterType).ToString().ToUpper()));
			Entry.Depth = 0;
			Entry.PreviewFocusValue = static_cast<int32>(EAstroPreviewFocus::StarCluster);
			OutEntries.Add(MoveTemp(Entry));
		}
		return;
	}
	if (PreviewFocus == EAstroPreviewFocus::StarCluster
		&& IsValid(GeneratedStarCluster) && SelectedPreviewClusterSystemIndex == INDEX_NONE)
	{
		// A cluster contains actor-free system records. Expose a bounded, clickable
		// sample instead of misleadingly repeating the home system body list. The
		// materialized home record is pinned first and points at its exact HISM star.
		const auto AddClusterRecord = [this, &OutEntries](const FClusterStarSystemRecord& Record,
			bool bIsHome)
		{
			if (Record.InstanceIndex == INDEX_NONE)
			{
				return;
			}

			FAPSPreviewBodyEntry Entry;
			Entry.Actor = bIsHome ? GeneratedHomeStarSystem : nullptr;
			Entry.ExplicitWorldAnchor = GeneratedStarCluster->GetPotentialSystemWorldLocation(Record);
			Entry.bHasExplicitWorldAnchor = !Entry.ExplicitWorldAnchor.ContainsNaN();
			const FString ShortSystemId = Record.StableId.ToString(EGuidFormats::Short).ToUpper();
			Entry.Label = FText::FromString(bIsHome
				? FString::Printf(TEXT("HOME SYSTEM  /  %s"), *ShortSystemId)
				: FString::Printf(TEXT("SYSTEM %s"), *ShortSystemId));
			Entry.Details = FText::FromString(FString::Printf(
				TEXT("%d STAR%s  /  %d POTENTIAL PLANET%s%s"),
				Record.SystemModel.AmountOfStars,
				Record.SystemModel.AmountOfStars == 1 ? TEXT("") : TEXT("S"),
				Record.SystemModel.PotentialPlanetCount,
				Record.SystemModel.PotentialPlanetCount == 1 ? TEXT("") : TEXT("S"),
				bIsHome ? TEXT("  /  CURRENT") : TEXT("")));
			Entry.Depth = bIsHome ? 0 : -1;
			Entry.ClusterSystemInstanceIndex = Record.InstanceIndex;
			if (bIsHome)
			{
				// The home HISM proxy remains the distant barycentric glyph after
				// materialization; route this row to the real hierarchy rather than
				// focusing that lightweight point as an unrelated catalog system.
				Entry.PreviewFocusValue = static_cast<int32>(EAstroPreviewFocus::HomeSystem);
			}
			OutEntries.Add(MoveTemp(Entry));
		};

		if (const FClusterStarSystemRecord* HomeRecord = GeneratedStarCluster->FindPotentialSystem(
			PendingHomeClusterInstanceIndex))
		{
			AddClusterRecord(*HomeRecord, true);
		}
		const int32 VisibleSystemCount = FMath::Min(GeneratedStarCluster->PotentialStarSystems.Num(), 16);
		for (int32 RecordIndex = 0; RecordIndex < GeneratedStarCluster->PotentialStarSystems.Num()
			&& OutEntries.Num() < VisibleSystemCount; ++RecordIndex)
		{
			const FClusterStarSystemRecord& Record = GeneratedStarCluster->PotentialStarSystems[RecordIndex];
			if (Record.InstanceIndex != PendingHomeClusterInstanceIndex)
			{
				AddClusterRecord(Record, false);
			}
		}
		return;
	}
	if (IsValid(GeneratedStarCluster) && SelectedPreviewClusterSystemIndex != INDEX_NONE)
	{
		if (const FClusterStarSystemRecord* Record =
			GeneratedStarCluster->FindPotentialSystem(SelectedPreviewClusterSystemIndex))
		{
			FAPSPreviewBodyEntry SystemEntry;
			SystemEntry.Label = FText::FromString(FString::Printf(TEXT("SYSTEM %s"),
				*Record->StableId.ToString(EGuidFormats::Short).ToUpper()));
			SystemEntry.Details = FText::FromString(FString::Printf(
				TEXT("%d STAR%s  /  %d POTENTIAL PLANET%s  /  LIGHTWEIGHT FULL-SCALE RECORD"),
				Record->SystemModel.AmountOfStars,
				Record->SystemModel.AmountOfStars == 1 ? TEXT("") : TEXT("S"),
				Record->SystemModel.PotentialPlanetCount,
				Record->SystemModel.PotentialPlanetCount == 1 ? TEXT("") : TEXT("S")));
			SystemEntry.Depth = -1;
			OutEntries.Add(MoveTemp(SystemEntry));
			return;
		}
	}
	if (!IsValid(HomeStar))
	{
		return;
	}

	// Object-level scopes intentionally expose only the selected object family.
	// SYSTEM remains the complete hierarchy; STAR does not repeat every orbit;
	// PLANET shows the selected planet and its own satellites only.
	if (PreviewFocus == EAstroPreviewFocus::HomeStar)
	{
		const AStar* SelectedStar = Cast<AStar>(SelectedPreviewBodyActor.Get());
		if (!IsValid(SelectedStar)) SelectedStar = HomeStar;
		FAPSPreviewBodyEntry Entry;
		Entry.Actor = const_cast<AStar*>(SelectedStar);
		Entry.Label = SelectedStar->AstroName.IsNone()
			? FText::FromString(TEXT("HOME STAR")) : FText::FromName(SelectedStar->AstroName);
		Entry.Details = FText::FromString(FString::Printf(TEXT("%s"),
			SelectedStar->FullSpectralName.IsNone() ? TEXT("STELLAR PRIMARY")
				: *SelectedStar->FullSpectralName.ToString().ToUpper()));
		OutEntries.Add(MoveTemp(Entry));
		return;
	}

	if (PreviewFocus == EAstroPreviewFocus::HomePlanet)
	{
		APlanet* SelectedPlanet = Cast<APlanet>(SelectedPreviewBodyActor.Get());
		if (const AMoon* SelectedMoon = Cast<AMoon>(SelectedPreviewBodyActor.Get()))
		{
			SelectedPlanet = SelectedMoon->ParentPlanet;
		}
		if (!IsValid(SelectedPlanet)) SelectedPlanet = HomePlanet;
		if (!IsValid(SelectedPlanet)) return;

		const APlanetarySystem* ParentSystem = IsValid(SelectedPlanet->ParentStar)
			? SelectedPlanet->ParentStar->PlanetarySystem : HomeStar->PlanetarySystem;
		const int32 PlanetIndex = IsValid(ParentSystem)
			? FMath::Max(ParentSystem->PlanetsActorsList.IndexOfByKey(SelectedPlanet), 0) : 0;
		if (IsValid(SelectedPlanet->ParentStar))
		{
			FAPSPreviewBodyEntry StarEntry;
			StarEntry.Actor = SelectedPlanet->ParentStar;
			StarEntry.Label = SelectedPlanet->ParentStar->AstroName.IsNone()
				? FText::FromString(TEXT("PARENT STAR"))
				: FText::FromString(FString::Printf(TEXT("STAR  /  %s"),
					*SelectedPlanet->ParentStar->AstroName.ToString().ToUpper()));
			StarEntry.Details = FText::FromString(
				SelectedPlanet->ParentStar->FullSpectralName.IsNone()
					? TEXT("STELLAR PARENT")
					: SelectedPlanet->ParentStar->FullSpectralName.ToString().ToUpper());
			StarEntry.Depth = 0;
			OutEntries.Add(MoveTemp(StarEntry));
		}
		FAPSPreviewBodyEntry PlanetEntry;
		PlanetEntry.Actor = SelectedPlanet;
		PlanetEntry.Label = FText::FromString(SelectedPlanet->AstroName.IsNone()
			? FString::Printf(TEXT("P%02d"), PlanetIndex + 1)
			: FString::Printf(TEXT("P%02d  /  %s"), PlanetIndex + 1,
				*SelectedPlanet->AstroName.ToString().ToUpper()));
		PlanetEntry.Details = FText::FromString(FString::Printf(
			TEXT("%s  /  %d KM"),
			*UEnum::GetDisplayValueAsText(SelectedPlanet->PlanetType).ToString().ToUpper(),
			SelectedPlanet->PlanetRadiusKM));
		PlanetEntry.Depth = 1;
		OutEntries.Add(MoveTemp(PlanetEntry));

		for (int32 MoonIndex = 0; MoonIndex < SelectedPlanet->Moons.Num(); ++MoonIndex)
		{
			const AMoon* Moon = SelectedPlanet->Moons[MoonIndex];
			if (!IsValid(Moon)) continue;
			FAPSPreviewBodyEntry MoonEntry;
			MoonEntry.Actor = const_cast<AMoon*>(Moon);
			MoonEntry.Label = FText::FromString(Moon->AstroName.IsNone()
				? FString::Printf(TEXT("MOON %02d.%02d"), PlanetIndex + 1, MoonIndex + 1)
				: FString::Printf(TEXT("MOON %02d.%02d  /  %s"), PlanetIndex + 1,
					MoonIndex + 1, *Moon->AstroName.ToString().ToUpper()));
			MoonEntry.Details = FText::FromString(FString::Printf(
				TEXT("%s  /  %d KM"),
				*UEnum::GetDisplayValueAsText(Moon->PlanetType).ToString().ToUpper(), Moon->PlanetRadiusKM));
			MoonEntry.Depth = 2;
			OutEntries.Add(MoveTemp(MoonEntry));
		}
		return;
	}

	TArray<AStar*> SystemStars;
	if (IsValid(GeneratedHomeStarSystem))
	{
		for (AStar* GeneratedStar : GeneratedHomeStarSystem->GetStars())
		{
			SystemStars.AddUnique(GeneratedStar);
		}
	}
	if (SystemStars.IsEmpty())
	{
		SystemStars.Add(HomeStar);
	}

	for (int32 StarIndex = 0; StarIndex < SystemStars.Num(); ++StarIndex)
	{
		const AStar* SystemStar = SystemStars[StarIndex];
		if (!IsValid(SystemStar))
		{
			continue;
		}
		const TCHAR StarLetter = TCHAR('A' + StarIndex);
		FAPSPreviewBodyEntry StarEntry;
		StarEntry.Actor = const_cast<AStar*>(SystemStar);
		StarEntry.Label = FText::FromString(SystemStar->AstroName.IsNone()
			? FString::Printf(TEXT("STAR %c"), StarLetter)
			: FString::Printf(TEXT("STAR %c  /  %s"), StarLetter,
				*SystemStar->AstroName.ToString().ToUpper()));
		StarEntry.Details = FText::FromString(FString::Printf(TEXT("%s"),
			SystemStar->FullSpectralName.IsNone() ? TEXT("STELLAR PRIMARY")
				: *SystemStar->FullSpectralName.ToString().ToUpper()));
		StarEntry.Depth = 0;
		OutEntries.Add(MoveTemp(StarEntry));

		const APlanetarySystem* PlanetarySystem = SystemStar->PlanetarySystem;
		if (!IsValid(PlanetarySystem))
		{
			continue;
		}
		for (int32 PlanetIndex = 0; PlanetIndex < PlanetarySystem->PlanetsActorsList.Num(); ++PlanetIndex)
		{
			const APlanet* Planet = PlanetarySystem->PlanetsActorsList[PlanetIndex];
			if (!IsValid(Planet))
			{
				continue;
			}

			FAPSPreviewBodyEntry PlanetEntry;
			PlanetEntry.Actor = const_cast<APlanet*>(Planet);
			PlanetEntry.Label = FText::FromString(Planet->AstroName.IsNone()
				? FString::Printf(TEXT("P%02d"), PlanetIndex + 1)
				: FString::Printf(TEXT("P%02d  /  %s"), PlanetIndex + 1,
					*Planet->AstroName.ToString().ToUpper()));
			PlanetEntry.Details = FText::FromString(FString::Printf(
				TEXT("%s  /  %d KM"),
				*UEnum::GetDisplayValueAsText(Planet->PlanetType).ToString().ToUpper(),
				Planet->PlanetRadiusKM));
			PlanetEntry.Depth = 1;
			OutEntries.Add(MoveTemp(PlanetEntry));

			for (int32 MoonIndex = 0; MoonIndex < Planet->Moons.Num(); ++MoonIndex)
			{
				const AMoon* Moon = Planet->Moons[MoonIndex];
				if (!IsValid(Moon))
				{
					continue;
				}
				FAPSPreviewBodyEntry MoonEntry;
				MoonEntry.Actor = const_cast<AMoon*>(Moon);
				MoonEntry.Label = FText::FromString(Moon->AstroName.IsNone()
					? FString::Printf(TEXT("MOON %02d.%02d"), PlanetIndex + 1, MoonIndex + 1)
					: FString::Printf(TEXT("MOON %02d.%02d  /  %s"), PlanetIndex + 1,
						MoonIndex + 1, *Moon->AstroName.ToString().ToUpper()));
				MoonEntry.Details = FText::FromString(FString::Printf(
					TEXT("%s  /  %d KM"),
					*UEnum::GetDisplayValueAsText(Moon->PlanetType).ToString().ToUpper(),
					Moon->PlanetRadiusKM));
				MoonEntry.Depth = 2;
				OutEntries.Add(MoveTemp(MoonEntry));
			}
		}
	}
}

bool AAstroGenerator::FocusPreviewBodyActor(AActor* BodyActor, APlayerController* PlayerController)
{
	if (!IsValid(BodyActor) || !BodyActor->IsAttachedTo(this))
	{
		return false;
	}

	PreviewFocus = BodyActor->IsA<AStar>() ? EAstroPreviewFocus::HomeStar : EAstroPreviewFocus::HomePlanet;
	SelectedPreviewBodyActor = BodyActor;
	ApplyPreviewFocusPresentation(PreviewFocus);
	SetPreviewWorldScapeBody(Cast<APlanetaryBody>(BodyActor));
	FBox Bounds = GetPreviewFocusBounds(PreviewFocus);
	if (!Bounds.IsValid)
	{
		Bounds = FBox(BodyActor->GetActorLocation() - FVector(500.0),
			BodyActor->GetActorLocation() + FVector(500.0));
	}
	StartPreviewCameraTransition(Bounds.GetCenter(),
		FMath::Max(static_cast<double>(Bounds.GetExtent().GetMax()), 500.0), PlayerController);
	return true;
}

bool AAstroGenerator::FocusPreviewClusterSystem(int32 InstanceIndex, APlayerController* PlayerController)
{
	if (!IsValid(GeneratedStarCluster) || !IsValid(GeneratedStarCluster->StarMeshInstances))
	{
		return false;
	}

	const FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystem(InstanceIndex);
	if (!Record)
	{
		return false;
	}
	if (Record->bMaterialized && Record->MaterializedSystem.Get() == GeneratedHomeStarSystem)
	{
		// The visible HISM point is the distant address of the real home hierarchy.
		// Clicking it must zoom into that hierarchy, not open a lightweight proxy of
		// the same system on top of itself.
		FocusPreviewTarget(EAstroPreviewFocus::HomeSystem, PlayerController);
		return true;
	}

	FVector FocusCenter;
	double FocusRadius = 0.0;
	if (!TryGetPreviewClusterSystemSphere(InstanceIndex, FocusCenter, FocusRadius))
	{
		return false;
	}

	SelectedPreviewClusterSystemIndex = InstanceIndex;
	SelectedPreviewBodyActor.Reset();
	PreviewFocus = EAstroPreviewFocus::HomeSystem;
	ApplyPreviewFocusPresentation(PreviewFocus);
	SetPreviewWorldScapeBody(nullptr);
	StartPreviewCameraTransition(FocusCenter, FocusRadius, PlayerController);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldGeneration] Focused cluster system %s from hierarchy instance %d"),
		*Record->StableId.ToString(EGuidFormats::DigitsWithHyphensLower), InstanceIndex);
	return true;
}

int32 AAstroGenerator::GetPreviewGalaxyRenderedStarCount() const
{
	return IsValid(GeneratedGalaxy) && IsValid(GeneratedGalaxy->StarMeshInstances)
		? GeneratedGalaxy->StarMeshInstances->GetInstanceCount() : 0;
}

int64 AAstroGenerator::GetPreviewGalaxyModeledStarCount() const
{
	return IsValid(GeneratedGalaxy) ? GeneratedGalaxy->StarCatalog.ModeledStarCount : 0;
}

int32 AAstroGenerator::GetPreviewClusterRenderedStarCount() const
{
	return IsValid(GeneratedStarCluster) && IsValid(GeneratedStarCluster->StarMeshInstances)
		? GeneratedStarCluster->StarMeshInstances->GetInstanceCount() : 0;
}

int32 AAstroGenerator::GetPreviewClusterModeledSystemCount() const
{
	return IsValid(GeneratedStarCluster)
		? FMath::Max(GeneratedStarCluster->ModeledStarAmount,
			GeneratedStarCluster->PotentialStarSystems.Num())
		: 0;
}

bool AAstroGenerator::GetSelectedPreviewClusterSystemSummary(
	FString& OutStableId, int32& OutStarCount, int32& OutPotentialPlanetCount) const
{
	if (!IsValid(GeneratedStarCluster) || SelectedPreviewClusterSystemIndex == INDEX_NONE)
	{
		return false;
	}
	const FClusterStarSystemRecord* Record =
		GeneratedStarCluster->FindPotentialSystem(SelectedPreviewClusterSystemIndex);
	if (!Record)
	{
		return false;
	}
	OutStableId = Record->StableId.ToString(EGuidFormats::Short).ToUpper();
	OutStarCount = Record->SystemModel.AmountOfStars;
	OutPotentialPlanetCount = Record->SystemModel.PotentialPlanetCount;
	return true;
}

bool AAstroGenerator::IsPreviewFocusAvailable(const EAstroPreviewFocus Focus) const
{
	switch (Focus)
	{
	case EAstroPreviewFocus::Overview:
		return IsValid(GeneratedGalaxy) || IsValid(GeneratedStarCluster)
			|| IsValid(GeneratedHomeStarSystem) || IsValid(GeneratedWorld);
	case EAstroPreviewFocus::Galaxy:
		return IsValid(GeneratedGalaxy);
	case EAstroPreviewFocus::StarCluster:
		return IsValid(GeneratedStarCluster);
	case EAstroPreviewFocus::HomeSystem:
		return IsValid(GeneratedHomeStarSystem)
			|| (IsValid(GeneratedStarCluster) && SelectedPreviewClusterSystemIndex != INDEX_NONE
				&& GeneratedStarCluster->FindPotentialSystem(SelectedPreviewClusterSystemIndex) != nullptr);
	case EAstroPreviewFocus::HomeStar:
		return IsValid(HomeStar);
	case EAstroPreviewFocus::HomePlanet:
		return IsValid(HomePlanet);
	default:
		return false;
	}
}

bool AAstroGenerator::FocusPreviewClusterSystemAtScreenPosition(
	APlayerController* PlayerController, const FVector2D& ScreenPosition, float MaxPixelDistance)
{
	if (!IsValid(PlayerController) || !IsValid(GeneratedStarCluster)
		|| !IsValid(GeneratedStarCluster->StarMeshInstances)
		|| GeneratedStarCluster->PotentialStarSystems.IsEmpty())
	{
		return false;
	}

	const float SafePixelDistance = FMath::Clamp(MaxPixelDistance, 4.0f, 96.0f);
	const double MaxDistanceSquared = FMath::Square(static_cast<double>(SafePixelDistance));
	double BestScreenDistanceSquared = MaxDistanceSquared;
	double BestWorldDistanceSquared = TNumericLimits<double>::Max();
	int32 BestInstanceIndex = INDEX_NONE;

	const FVector CameraLocation = PlayerController->PlayerCameraManager
		? PlayerController->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	for (const FClusterStarSystemRecord& Record : GeneratedStarCluster->PotentialStarSystems)
	{
		if (Record.InstanceIndex == INDEX_NONE)
		{
			continue;
		}

		FTransform LocalTransform;
		if (!GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(
			Record.InstanceIndex, LocalTransform, false)
			|| LocalTransform.GetScale3D().GetAbsMax() <= UE_SMALL_NUMBER)
		{
			// Zero scale is reserved for the current deep-scope safe-envelope cull.
			continue;
		}

		const FVector WorldLocation = GeneratedStarCluster->GetPotentialSystemWorldLocation(Record);
		FVector2D ProjectedPosition;
		if (WorldLocation.ContainsNaN()
			|| !PlayerController->ProjectWorldLocationToScreen(WorldLocation, ProjectedPosition, true))
		{
			continue;
		}

		const double ScreenDistanceSquared = FVector2D::DistSquared(ScreenPosition, ProjectedPosition);
		if (ScreenDistanceSquared > MaxDistanceSquared)
		{
			continue;
		}
		const double WorldDistanceSquared = FVector::DistSquared(CameraLocation, WorldLocation);
		const bool bCloserToCursor = ScreenDistanceSquared + 0.25 < BestScreenDistanceSquared;
		const bool bSameScreenPointAndNearer = FMath::IsNearlyEqual(
			ScreenDistanceSquared, BestScreenDistanceSquared, 0.25) && WorldDistanceSquared < BestWorldDistanceSquared;
		if (!bCloserToCursor && !bSameScreenPointAndNearer)
		{
			continue;
		}

		BestScreenDistanceSquared = ScreenDistanceSquared;
		BestWorldDistanceSquared = WorldDistanceSquared;
		BestInstanceIndex = Record.InstanceIndex;
	}

	const FClusterStarSystemRecord* SelectedRecord =
		GeneratedStarCluster->FindPotentialSystem(BestInstanceIndex);
	if (!SelectedRecord)
	{
		return false;
	}
	if (SelectedRecord->bMaterialized
		&& SelectedRecord->MaterializedSystem.Get() == GeneratedHomeStarSystem)
	{
		FocusPreviewTarget(EAstroPreviewFocus::HomeSystem, PlayerController);
		return true;
	}

	FVector FocusCenter;
	double FocusRadius = 0.0;
	if (!TryGetPreviewClusterSystemSphere(BestInstanceIndex, FocusCenter, FocusRadius))
	{
		return false;
	}
	SelectedPreviewClusterSystemIndex = BestInstanceIndex;
	SelectedPreviewBodyActor.Reset();
	PreviewFocus = EAstroPreviewFocus::HomeSystem;
	ApplyPreviewFocusPresentation(PreviewFocus);
	SetPreviewWorldScapeBody(nullptr);
	StartPreviewCameraTransition(FocusCenter, FocusRadius, PlayerController);
	UE_LOG(LogTemp, Log,
		TEXT("[APS.WorldGeneration] Selected cluster system %s at HISM instance %d (%d potential planets)"),
		*SelectedRecord->StableId.ToString(EGuidFormats::DigitsWithHyphensLower), BestInstanceIndex,
		SelectedRecord->SystemModel.PotentialPlanetCount);
	return true;
}

void AAstroGenerator::BeginPreviewCameraOrbit()
{
	if (PreviewFocus != EAstroPreviewFocus::HomePlanet
		|| !ActivePreviewWorldScapeBody.IsValid()
		|| !IsValid(PreviewCamera)
		|| !ActivePreviewWorldScapeBody->bWorldScapeSurfaceReady
		|| !IsValid(GetActivePreviewTerrainProxy()))
	{
		bPreviewCameraOrbitDragging = false;
		return;
	}

	bPreviewCameraOrbitDragging = true;
	bPreviewSurfaceViewDirty = false;
	PendingPreviewSurfaceViewPosition = PreviewCamera->GetComponentLocation();
	SetPreviewGlobeProxyVisible(true);
}

void AAstroGenerator::EndPreviewCameraOrbit()
{
	if (!bPreviewCameraOrbitDragging)
	{
		return;
	}

	bPreviewCameraOrbitDragging = false;
	bPreviewSurfaceViewDirty = false;
	bPreviewSurfaceViewRefreshInFlight = false;
	bPreviewSurfaceLiveRefresh = false;
	SetPreviewGlobeProxyVisible(ActivePreviewWorldScapeBody.IsValid());
}

void AAstroGenerator::OrbitPreviewCamera(FVector2D ScreenDelta)
{
	if (!PreviewCamera || PreviewOrbitDistance <= UE_SMALL_NUMBER)
	{
		return;
	}

	bPreviewCameraTransitionActive = false;
	const FVector Offset = PreviewCamera->GetComponentLocation() - PreviewOrbitCenter;
	const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(-ScreenDelta.X * 0.18));
	const FQuat Pitch(PreviewCamera->GetRightVector(), FMath::DegreesToRadians(ScreenDelta.Y * 0.14));
	const FVector NewOffset = (Pitch * Yaw).RotateVector(Offset).GetSafeNormal() * PreviewOrbitDistance;
	const FVector NewLocation = PreviewOrbitCenter + NewOffset;
	PreviewCamera->SetWorldLocationAndRotation(NewLocation, (PreviewOrbitCenter - NewLocation).Rotation());

	if (bPreviewCameraOrbitDragging
		&& PreviewFocus == EAstroPreviewFocus::HomePlanet
		&& ActivePreviewWorldScapeBody.IsValid())
	{
		PendingPreviewSurfaceViewPosition = NewLocation;
		SetPreviewGlobeProxyVisible(true);
	}
}

void AAstroGenerator::ZoomPreviewCamera(float WheelDelta)
{
	if (!PreviewCamera || FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}

	bPreviewCameraTransitionActive = false;
	double MinDistance = 100.0;
	double MaxDistance = 1.0e18;
	FVector FocusCenter = PreviewOrbitCenter;
	double FocusRadius = 0.0;
	if (GetPreviewFocusSphere(PreviewFocus, FocusCenter, FocusRadius)
		&& FMath::IsFinite(FocusRadius) && FocusRadius > UE_SMALL_NUMBER)
	{
		// Match the ratio retained by RegeneratePreview. Allowing a wheel event to
		// reach 1e18 cm caused LWC precision loss and the next parameter edit snapped
		// the camera back from an otherwise unreachable distance.
		MaxDistance = FMath::Max(FocusRadius * 30.0, MinDistance * 1.1);
	}
	if (PreviewFocus == EAstroPreviewFocus::HomePlanet
		|| PreviewFocus == EAstroPreviewFocus::HomeStar)
	{
		if (FocusRadius > UE_SMALL_NUMBER)
		{
			// Keep the camera outside the selected physical body and its normalized
			// atmosphere shell. System/cluster scopes deliberately remain traversable.
			MinDistance = FMath::Max(MinDistance, FocusRadius * 1.2);
		}
	}
	MaxDistance = FMath::Max(MaxDistance, MinDistance);
	PreviewOrbitDistance = FMath::Clamp(
		PreviewOrbitDistance * FMath::Pow(0.82, static_cast<double>(WheelDelta)),
		MinDistance, MaxDistance);
	FVector ViewDirection = (PreviewCamera->GetComponentLocation() - PreviewOrbitCenter).GetSafeNormal();
	if (ViewDirection.IsNearlyZero())
	{
		ViewDirection = FVector(1.0, 1.0, -0.45).GetSafeNormal();
	}
	const FVector NewLocation = PreviewOrbitCenter + ViewDirection * PreviewOrbitDistance;
	PreviewCamera->SetWorldLocationAndRotation(NewLocation, (PreviewOrbitCenter - NewLocation).Rotation());

	if (PreviewFocus == EAstroPreviewFocus::HomePlanet
		&& ActivePreviewWorldScapeBody.IsValid())
	{
		// The closed globe has no altitude or camera-normal LOD bucket. Wheel zoom only
		// moves the camera; it never invalidates geometry or starts a worker.
		PendingPreviewSurfaceViewPosition = NewLocation;
		SetPreviewGlobeProxyVisible(true);
	}
}

void AAstroGenerator::AdvancePreviewGenerationSeed()
{
	// Deterministic LCG: explicit REGENERATE produces a new arrangement, while
	// ordinary control changes keep that arrangement stable.
	const uint32 CurrentSeed = static_cast<uint32>(PreviewGenerationSeed);
	PreviewGenerationSeed = FMath::Max(1,
		static_cast<int32>((CurrentSeed * 196314165u + 907633515u) & 0x7fffffffu));
	if (IsValid(GeneratedWorldModel))
	{
		GeneratedWorldModel->GenerationSeed = PreviewGenerationSeed;
	}
}

void AAstroGenerator::InitAstroGenerators()
{
	GalaxyGenerator = NewObject<UGalaxyGenerator>();
	StarClusterGenerator = NewObject<UStarClusterGenerator>();
	StarSystemGenerator = NewObject<UStarSystemGenerator>();
	PlanetarySystemGenerator = NewObject<UPlanetarySystemGenerator>();
	StarGenerator = NewObject<UStarGenerator>();
	PlanetGenerator = NewObject<UPlanetGenerator>();
	MoonGenerator = NewObject<UMoonGenerator>();

	if (GalaxyGenerator == nullptr
		|| StarClusterGenerator == nullptr
		|| StarSystemGenerator == nullptr
		|| PlanetarySystemGenerator == nullptr
		|| StarGenerator == nullptr
		|| PlanetGenerator == nullptr
		|| MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("All generators OK!"));
	}
}

void AAstroGenerator::ApplySpawnParameters()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			UMainGameplayInstance* GameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>();
			USpawnParameters* SpawnParams = GameplayInstance ? GameplayInstance->SpawnParameters : nullptr;
			if (GameplayInstance && GameplayInstance->NewGeneratedWorld)
			{
				// A committed route is explicit and must not depend on potentially stale
				// Blueprint CDO defaults. Civilization always builds its selected starter
				// hierarchy; Generate Space/Create Planet never inherit one accidentally.
				bSpawnStarterLocation = GameplayInstance->bSpawnGeneratedCivilization;
				bCharacterSpawn = GameplayInstance->bSpawnGeneratedCivilization;
			}

			if (SpawnParams)
			{
				// ���������� ����������
				CharSpawnPlace = SpawnParams->CharacterSpawnPlace;
				HomeSpaceStationOrbitHeight = SpawnParams->HomeStationOrbitHeight;
				BP_CharacterClass = SpawnParams->BP_CharacterClass;
				BP_HomeSpaceStation = SpawnParams->BP_HomeSpaceStation;
				BP_HomeSpaceship = SpawnParams->BP_HomeSpaceship;
				BP_HomeSpaceShipyard = SpawnParams->BP_HomeSpaceShipyard;
				BP_HomeSpaceHeadquarters = SpawnParams->BP_HomeSpaceHeadquarters;

				UE_LOG(LogTemp, Log, TEXT("Spawn parameters applied"));
			}
		}
	}
}

void AAstroGenerator::ApplyWorldModel()
{
	if (!GeneratedWorldModel)
	{
		UE_LOG(LogTemp, Error, TEXT("NewGeneratedWorld is nullptr"));
		return;
	}

	// Copy values from NewGeneratedWorld to AAstroGenerator
	bGenerateFullScaledWorld = GeneratedWorldModel->bGenerateFullScaledWorld;
	bGenerateHomeSystem = GeneratedWorldModel->bGenerateHomeSystem;
	bStartWithHomePlanet = GeneratedWorldModel->bStartWithHomePlanet;
	// The legacy generator exposed a second internal switch that was not copied
	// from the menu model. Make START WITH HOME PLANET authoritative for committed
	// gameplay as well as the live preview.
	bSpawnStarterPlanet = bStartWithHomePlanet;
	bRandomHomeSystem = GeneratedWorldModel->bRandomHomeSystem;
	bRandomHomeSystemType = GeneratedWorldModel->bRandomHomeSystemType;
	bRandomHomeStar = GeneratedWorldModel->bRandomHomeStar;
	bRandomStartPlanetNumber = GeneratedWorldModel->bRandomStartPlanetNumber;
	AstroGenerationLevel = GeneratedWorldModel->AstroGenerationLevel;
	GalaxyType = GeneratedWorldModel->GalaxyType;
	GalaxyGlass = GeneratedWorldModel->GalaxyClass;
	StarClusterSize = GeneratedWorldModel->StarClusterSize;
	StarClusterType = GeneratedWorldModel->StarClusterType;
	StarClusterPopulation = GeneratedWorldModel->StarClusterPopulation;
	StarClusterComposition = GeneratedWorldModel->StarClusterComposition;
	HomeSystemStarType = GeneratedWorldModel->StarType;
	HomeStarStellarType = GeneratedWorldModel->StellarType;
	HomeStarSpectralClass = GeneratedWorldModel->SpectralClass;
	HomeSystemPlanetaryType = GeneratedWorldModel->PlanetarySystemType;
	HomeSystemOrbitDistributionType = GeneratedWorldModel->OrbitDistributionType;
	HomeSystemPosition = GeneratedWorldModel->HomeSystemPosition;
	GalaxySize = GeneratedWorldModel->GalaxySize;
	GalaxyStarCount = GeneratedWorldModel->GalaxyStarCount;
	PlanetsAmount = GeneratedWorldModel->PlanetsAmount;
	StartPlanetNumber = GeneratedWorldModel->StartPlanetIndex;
	PlanetsAmount = FMath::Max(1, PlanetsAmount);
	StartPlanetNumber = FMath::Clamp(StartPlanetNumber, 1, PlanetsAmount);
	GalaxyStarDensity = GeneratedWorldModel->GalaxyStarDensity;
	// Actor pointers captured by the menu preview are world-local and become stale
	// across OpenLevel. Only the placed authored SinglePlay generator is allowed to
	// retain serialized hierarchy actors; generated runtime/preview paths select the
	// newly materialized start planet below in GenerateStarSystemByModel.
	HomePlanetarySystem = bIntegrateStartPlanet ? GeneratedWorldModel->HomePlanetarySystem : nullptr;
	HomePlanet = bIntegrateStartPlanet ? GeneratedWorldModel->HomePlanet : nullptr;

	// The model-driven menu is explicit: every visible selector must deterministically
	// affect the live scene. Blueprint defaults for the old Random buttons otherwise
	// override cluster/galaxy selectors and make the Slate controls appear broken.
	bGenerateRandomCluster = false;
	bGenerateRandomGalaxy = false;
}

void AAstroGenerator::GenerateStarCluster()
{
	if (BP_StarClusterClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BP_StarClusterClass is not set!"));
		return;
	}

	const TSharedPtr<FStarClusterModel> StarClusterModel = MakeShared<FStarClusterModel>();
	if (bGenerateRandomCluster)
	{
		StarClusterGenerator->GetRandomStarClusterModel(StarClusterModel);
	}
	else
	{
		StarClusterModel->StarClusterSize = StarClusterSize;
		StarClusterModel->StarClusterType = StarClusterType;
		StarClusterModel->StarClusterPopulation = StarClusterPopulation;
		StarClusterModel->StarClusterComposition = StarClusterComposition;
	}
	const EStarClusterType ClusterType = StarClusterModel->StarClusterType;
	AStarCluster* NewStarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
	if (!NewStarCluster)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn star cluster."));
		return;
	}
	// A generated cluster is part of the galaxy, not a second unrelated root.
	// Keep relative transforms here: the generator may already carry the full-scale
	// world transform after GenerateGalaxy(), and KeepWorld would silently cancel it
	// on the newly spawned cluster.
	AActor* ClusterParent = IsValid(GeneratedGalaxy)
		? static_cast<AActor*>(GeneratedGalaxy) : static_cast<AActor*>(this);
	NewStarCluster->AttachToActor(ClusterParent, FAttachmentTransformRules::KeepRelativeTransform);

	// Calculate Cluster Params
	NewStarCluster->ModeledStarAmount = StarClusterGenerator->GetStarsAmountByRange(
		StarClusterModel->StarClusterSize);
	NewStarCluster->StarAmount = NewStarCluster->ModeledStarAmount;
	if (bIsPreviewGeneration)
	{
		int32 FormationBudget = 1600;
		switch (StarClusterModel->StarClusterSize)
		{
		case EStarClusterSize::Tiny: FormationBudget = 450; break;
		case EStarClusterSize::Small: FormationBudget = 800; break;
		case EStarClusterSize::Medium: FormationBudget = 1100; break;
		case EStarClusterSize::Large: FormationBudget = 1400; break;
		case EStarClusterSize::Giant: FormationBudget = 1600; break;
		case EStarClusterSize::Unknown:
		default: FormationBudget = 1000; break;
		}
		NewStarCluster->StarAmount = FMath::Min(NewStarCluster->StarAmount,
			FMath::Min(PreviewMaxInstances, FormationBudget));
	}
	NewStarCluster->StarDensity = StarClusterGenerator->GetStarClusterDensityByRange();
	NewStarCluster->ClusterBounds = StarClusterGenerator->GetStarClusterBoundsByRange(ClusterType);
	NewStarCluster->ClusterType = ClusterType;
	NewStarCluster->StarClusterComposition = StarClusterModel->StarClusterComposition;
	NewStarCluster->StarClusterPopulation = StarClusterModel->StarClusterPopulation;
	NewStarCluster->StarClusterSize = StarClusterModel->StarClusterSize;
	NewStarCluster->StarMeshInstances->NumCustomDataFloats = 6;
	double PreviewClusterToGalaxyScale = 1.0;
	if (bIsPreviewGeneration && IsValid(GeneratedGalaxy)
		&& !GeneratedGalaxy->StarCatalog.CatalogHalfExtent.IsNearlyZero())
	{
		const double GalaxyRadius = GeneratedGalaxy->StarCatalog.CatalogHalfExtent.GetAbs().GetMax();
		const double ClusterHalfExtent = NewStarCluster->ClusterBounds.GetAbs().GetMax() * 50.0;
		if (GalaxyRadius > UE_DOUBLE_SMALL_NUMBER && ClusterHalfExtent > UE_DOUBLE_SMALL_NUMBER)
		{
			// A cluster is a compact central region of its parent galaxy, not a second
			// galaxy-sized point cloud. Scale only the disposable preview hierarchy;
			// system records and the materialized home system inherit the same transform.
			PreviewClusterToGalaxyScale = FMath::Clamp(
				GalaxyRadius * 0.16 / ClusterHalfExtent, 1.0e-9, 1.0);
			NewStarCluster->SetActorRelativeScale3D(FVector(PreviewClusterToGalaxyScale));
		}
	}
	if (NewStarCluster->GenerationSeed == 0)
	{
		NewStarCluster->GenerationSeed = FMath::RandRange(1, MAX_int32);
	}
	// The generated home cluster is the navigation anchor of the galaxy.  Keep it
	// at the galaxy origin so the surrounding catalog remains balanced in 360°
	// and every deeper level can retain an exact, stable world-space address.
	NewStarCluster->SetActorRelativeLocation(FVector::ZeroVector);
	NewStarCluster->CalculateAffectionRadius();
	NewStarCluster->PotentialStarSystems.Reserve(NewStarCluster->StarAmount);
	NewStarCluster->StarMeshInstances->PreAllocateInstancesMemory(NewStarCluster->StarAmount);
	const UStaticMesh* ClusterProxyMesh = NewStarCluster->StarMeshInstances->GetStaticMesh();
	const double ClusterProxyMeshRadius = IsValid(ClusterProxyMesh)
		? FMath::Max(static_cast<double>(ClusterProxyMesh->GetBounds().SphereRadius), 1.0) : 50.0;
	const double LogicalClusterHalfExtent = NewStarCluster->ClusterBounds.GetAbs().GetMax() * 50.0;
	const double SparseSampleCompensation = FMath::Clamp(
		FMath::Sqrt(1600.0 / FMath::Max(NewStarCluster->StarAmount, 1)), 0.90, 1.75);
	const double MinimumPreviewProxyRadius = bIsPreviewGeneration
		? LogicalClusterHalfExtent * 0.00155 * SparseSampleCompensation : 0.0;
	const double MinimumPreviewProxyScale = MinimumPreviewProxyRadius / ClusterProxyMeshRadius;
	FBox RenderedClusterBounds(EForceInit::ForceInit);

	UE_LOG(LogTemp, VeryVerbose, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
	UE_LOG(LogTemp, VeryVerbose, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
	UE_LOG(LogTemp, VeryVerbose, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
	UE_LOG(LogTemp, VeryVerbose, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

	for (int32 i = 0; i < NewStarCluster->StarAmount; ++i)
	{
		// Create a star model
		TSharedPtr<FStarModel> NewStarModel = MakeShared<FStarModel>();

		if (bGenerateRandomCluster)
		{
			StarGenerator->GenerateRandomStarModel(NewStarModel);
		}
		else
		{
			StarGenerator->GenerateStarModelByProbability(NewStarModel, StarClusterModel);
		}

		// Position the star in the cluster
		FVector StarPosition = StarClusterGenerator->CalculateStarPosition(i, NewStarCluster, NewStarModel);
		NewStarModel->Location = StarPosition;

		// Create a star instance and add it to the HISM component
		FTransform StarTransform(StarPosition);
		const double FarVisualRadius = FMath::Max(
			UStarGenerator::GetFarStarVisualRadius(NewStarModel->Radius), MinimumPreviewProxyScale);
		StarTransform.SetScale3D(FVector(FarVisualRadius));
		RenderedClusterBounds += StarPosition;
		// CalculateStarPosition returns cluster-local coordinates. Supplying them as
		// world-space after the cluster has been nested inside a galaxy displaces the
		// entire render sample away from its logical bounds and camera target.
		const int32 StarInstIndex = NewStarCluster->StarMeshInstances->AddInstance(StarTransform, false);
		const FLinearColor ColorValue = StarGenerator->GetStarColor(NewStarModel->SpectralClass,
		                                                            NewStarModel->SpectralSubclass);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R, false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G, false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B, false);

		const double StarEmission = UStarGenerator::GetFarStarVisualEmission(NewStarModel->Radius,
			StarGenerator->CalculateEmission(NewStarModel->Luminosity * 25));
		NewStarCluster->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission, false);

		FStarSystemModel PotentialSystemModel;
		const int32 SystemSeed = static_cast<int32>(HashCombine(
			GetTypeHash(NewStarCluster->GenerationSeed), GetTypeHash(StarInstIndex)) & 0x7fffffffu);
		StarSystemGenerator->GeneratePotentialStarSystemModel(
			PotentialSystemModel, *NewStarModel, FMath::Max(SystemSeed, 1));

		FTransform LocalInstanceTransform;
		NewStarCluster->StarMeshInstances->GetInstanceTransform(
			StarInstIndex, LocalInstanceTransform, false);
		NewStarCluster->RegisterPotentialSystem(StarInstIndex, LocalInstanceTransform.GetLocation(),
			*NewStarModel, PotentialSystemModel);

		// Stable per-star surface seed and system occupancy are ready for the unlit HISM material.
		FRandomStream VisualStream(PotentialSystemModel.GenerationSeed);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(
			StarInstIndex, 4, VisualStream.FRand(), false);
		NewStarCluster->StarMeshInstances->SetCustomDataValue(
			StarInstIndex, 5, PotentialSystemModel.PotentialPlanetCount / 12.0f, false);

	}
	NewStarCluster->FinalizeGeneratedInstances();
	const FVector RenderedClusterCenter = RenderedClusterBounds.IsValid
		? RenderedClusterBounds.GetCenter() : FVector::ZeroVector;
	const FVector RenderedClusterExtent = RenderedClusterBounds.IsValid
		? RenderedClusterBounds.GetExtent() : FVector::ZeroVector;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.ClusterPreview] type=%d modeled=%d rendered=%d logicalHalfExtent=%.3e galaxyScale=%.3e "
			"sampleCenter=%s sampleExtent=%s minProxyRadius=%.3e minProxyScale=%.3e preview=%d"),
		static_cast<int32>(ClusterType), NewStarCluster->ModeledStarAmount,
		NewStarCluster->StarAmount, LogicalClusterHalfExtent, PreviewClusterToGalaxyScale,
		*RenderedClusterCenter.ToCompactString(), *RenderedClusterExtent.ToCompactString(),
		MinimumPreviewProxyRadius, MinimumPreviewProxyScale, bIsPreviewGeneration ? 1 : 0);

	GeneratedStarCluster = NewStarCluster;

	if (bGenerateFullScaledWorld)
	{
		SetActorScale3D(FVector(FullScaleValue, FullScaleValue, FullScaleValue));
	}
}

AStarSystem* AAstroGenerator::MaterializeClusterStarSystem(int32 InstanceIndex)
{
	if (!GeneratedStarCluster || !GeneratedStarCluster->StarMeshInstances || !StarGenerator
		|| !StarSystemGenerator || !BP_StarSystemClass || !BP_StarClass)
	{
		return nullptr;
	}

	FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystemMutable(InstanceIndex);
	if (!Record)
	{
		return nullptr;
	}
	if (Record->MaterializedSystem.IsValid())
	{
		return Record->MaterializedSystem.Get();
	}

	FTransform WorldTransform;
	if (!GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(InstanceIndex, WorldTransform, true))
	{
		return nullptr;
	}
	WorldTransform.SetScale3D(FVector::OneVector);

	UWorld* World = GetWorld();
	AStarSystem* StarSystem = World
		? World->SpawnActor<AStarSystem>(BP_StarSystemClass, WorldTransform) : nullptr;
	if (!StarSystem)
	{
		return nullptr;
	}
	StarSystem->AttachToActor(GeneratedStarCluster, FAttachmentTransformRules::KeepWorldTransform);
	StarSystemGenerator->ApplyModel(StarSystem, MakeShared<FStarSystemModel>(Record->SystemModel));

	AStar* Star = World->SpawnActor<AStar>(BP_StarClass, WorldTransform);
	if (!Star)
	{
		StarSystem->Destroy();
		return nullptr;
	}
	const TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>(Record->PrimaryStarModel);
	StarGenerator->ApplyModel(Star, StarModel);
	Star->SetActorLocation(WorldTransform.GetLocation());
	Star->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
	Star->StarRadiusKM = StarModel->Radius * 696340;
	Star->FullSpectralName = Star->GenerateFullSpectralName();
	Star->AstroName = FName(*FString::Printf(TEXT("STAR-%s"),
		*Record->StableId.ToString(EGuidFormats::Short)));
	StarGenerator->ApplySpectralMaterial(Star, StarModel);
	StarSystem->MainStar = Star;
	StarSystem->AddNewStar(Star);
	Star->AttachToActor(StarSystem, FAttachmentTransformRules::KeepWorldTransform);

	FTransform HiddenTransform;
	if (GeneratedStarCluster->StarMeshInstances->GetInstanceTransform(
		InstanceIndex, HiddenTransform, false))
	{
		HiddenTransform.SetScale3D(FVector::ZeroVector);
		GeneratedStarCluster->StarMeshInstances->UpdateInstanceTransform(
			InstanceIndex, HiddenTransform, false, true, true);
		GeneratedStarCluster->StarMeshInstances->BuildTreeIfOutdated(true, true);
	}
	Record->bMaterialized = true;
	Record->MaterializedSystem = StarSystem;

	UE_LOG(LogTemp, Log, TEXT("[APS.Cluster] Materialized system %s at instance %d (%d potential planets)"),
		*Record->StableId.ToString(EGuidFormats::DigitsWithHyphensLower), InstanceIndex,
		Record->SystemModel.PotentialPlanetCount);
	return StarSystem;
}

bool AAstroGenerator::DematerializeClusterStarSystem(int32 InstanceIndex)
{
	if (!GeneratedStarCluster || !GeneratedStarCluster->StarMeshInstances)
	{
		return false;
	}
	FClusterStarSystemRecord* Record = GeneratedStarCluster->FindPotentialSystemMutable(InstanceIndex);
	if (!Record)
	{
		return false;
	}
	if (Record->MaterializedSystem.IsValid())
	{
		DestroyActorTree(Record->MaterializedSystem.Get());
	}

	const FTransform RestoredTransform(
		FQuat::Identity, Record->ClusterLocalLocation, FVector(Record->PrimaryStarModel.Radius));
	GeneratedStarCluster->StarMeshInstances->UpdateInstanceTransform(
		InstanceIndex, RestoredTransform, false, true, true);
	GeneratedStarCluster->StarMeshInstances->BuildTreeIfOutdated(true, true);
	Record->bMaterialized = false;
	Record->MaterializedSystem.Reset();
	return true;
}

bool AAstroGenerator::AddGeneratedWorldModelData()
{
	if (!GeneratedWorldModel)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot store generated world data: GeneratedWorldModel is null"));
		return false;
	}
	if (!HomePlanet)
	{
		UE_LOG(LogTemp, Error, TEXT("HomePlanet is null!"));
		return false;
	}
	if (!HomeStar)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot store generated world data: HomeStar is null"));
		return false;
	}
	if (!GeneratedHomeStarSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot store generated world data: home star system is null"));
		return false;
	}

	// �������� ���������� ������
	if (!HomePlanet->PlanetData.PlanetModel.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HomePlanet's PlanetModel is invalid!"));
		return false;
	}
	
	// Keep the modeled population independent from the representative HISM
	// sample. A generated galaxy can contain millions of stars in save/model
	// data while rendering only a bounded subset during gameplay.
	if (GeneratedGalaxy)
	{
		GeneratedWorldModel->StarsAmount = FMath::Max(1, GeneratedWorldModel->GalaxyStarCount);
	}
	else if (GeneratedStarCluster)
	{
		GeneratedWorldModel->StarsAmount = FMath::Max(
			GeneratedStarCluster->ModeledStarAmount, GeneratedStarCluster->StarAmount);
	}
	else
	{
		switch (GeneratedWorldModel->StarType)
		{
		case EStarType::DoubleStar: GeneratedWorldModel->StarsAmount = 2; break;
		case EStarType::TripleStar: GeneratedWorldModel->StarsAmount = 3; break;
		case EStarType::MultipleStar: GeneratedWorldModel->StarsAmount = 4; break;
		case EStarType::SingleStar:
		default: GeneratedWorldModel->StarsAmount = 1; break;
		}
	}
	GeneratedWorldModel->HomeStarName = HomeStar->AstroName;
	GeneratedWorldModel->FullSpectralName = HomeStar->FullSpectralName;
	GeneratedWorldModel->HomeStarMass = HomeStar->Mass;
	GeneratedWorldModel->HomeStarRadius = HomeStar->Radius;
	GeneratedWorldModel->HomeStarTemperature = HomeStar->SurfaceTemperature;
	GeneratedWorldModel->HomePlanetName = HomePlanet->AstroName;
	GeneratedWorldModel->StarSystemRadius = GeneratedHomeStarSystem->StarSystemRadius;

	HomePlanet->FillPlanetData();
	GeneratedWorldModel->InhabitedPlanets.Add(HomePlanet->PlanetData);
	return true;
}

void AAstroGenerator::GenerateHomeStarSystem()
{
	/*
	 * Create UGeneratedWorld* GeneratedWorldModel
	 * Model with random params for random generation
	 * Model from GenerationMenu for custom generation
	 */

	if (bAutoGeneration)
	{
		SetAutoStarSystemModel();
	}
	else if (bRandomHomeSystem)
	{
		GenerateRandomStarSystemModel();
	}

	GenerateStarSystemByModel();

	if (GeneratedHomeStarSystem
		&& GeneratedHomeStarSystem->MainStar
		&& GeneratedHomeStarSystem->MainStar->PlanetarySystem)
	{
		TArray<TSharedPtr<FPlanetData>> PlanetDataMap = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetsList;
		UE_LOG(LogTemp, Warning, TEXT("Planet List - "));
		UE_LOG(LogTemp, Warning, TEXT("Planet Amount: %d"), PlanetDataMap.Num());
		ShowPlanetsList(PlanetDataMap);

		if (bSpawnStarterPlanet && GeneratedHomeStarSystem)
		{
			// A procedural home planet is already materialized in the selected orbit by
			// GenerateStarSystemByModel.  The legacy path spawned a second standalone
			// planet, broadcast ClearChildren on the first and left the system arrays
			// pointing at the destroyed actor.  Commit and populate the existing actor
			// instead; authored SinglePlay continues through its isolated integration
			// branch below.
			if (!bIntegrateStartPlanet)
			{
				const int32 StartOrbitIndex = StartPlanetNumber - 1;
				const TArray<APlanetOrbit*>& PlanetOrbits =
					GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList;
				if (!PlanetOrbits.IsValidIndex(StartOrbitIndex)
					|| !PlanetDataMap.IsValidIndex(StartOrbitIndex)
					|| !PlanetDataMap[StartOrbitIndex].IsValid()
					|| !PlanetDataMap[StartOrbitIndex]->PlanetModel.IsValid()
					|| !IsValid(PlanetOrbits[StartOrbitIndex])
					|| !IsValid(PlanetOrbits[StartOrbitIndex]->Planet)
					|| !IsValid(HomePlanet)
					|| PlanetOrbits[StartOrbitIndex]->Planet != HomePlanet)
				{
					UE_LOG(LogTemp, Error,
						TEXT("Cannot commit procedural home planet at orbit %d"), StartPlanetNumber);
					return;
				}

				HomePlanet = PlanetOrbits[StartOrbitIndex]->Planet;
				const TSharedPtr<FPlanetModel> HomePlanetModel =
					PlanetDataMap[StartOrbitIndex]->PlanetModel;
				HomePlanet->PlanetData.PlanetModel = HomePlanetModel;
				if (!AddGeneratedWorldModelData())
				{
					UE_LOG(LogTemp, Error,
						TEXT("Aborting generated-world handoff because its runtime model is incomplete"));
					return;
				}
				if (bSpawnStarterLocation)
				{
					if (!SpawnStartInteractiveActors(HomePlanetModel))
					{
						UE_LOG(LogTemp, Error,
							TEXT("Aborting world save because starter hierarchy creation failed"));
						return;
					}
				}
				else
				{
					UE_LOG(LogTemp, Log,
						TEXT("[APS.WorldGeneration] Astronomical world committed without civilization starter actors"));
				}
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					if (AGravityPlayerController* MainController = Cast<AGravityPlayerController>(PC))
					{
						MainController->SaveNewWorld(
							GeneratedWorldModel->AstroGenerationLevel, GeneratedWorldModel);
					}
				}
				return;
			}

			// Check if we should integrate StartPlanet instead of generating new one
			if (bIntegrateStartPlanet && WSR_StartHomePlanet)
			{
				UE_LOG(LogTemp, Warning, TEXT("Integrating StartPlanet into generated star system"));
				IntegrateStartPlanetIntoSystem();
			}
			else
			{
				// Original planet generation logic
				const TSharedPtr<FPlanetModel> HomePlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(
					GeneratedWorldModel);
				HomePlanet = PlanetGenerator->GeneratePlanet(HomePlanetModel, BP_PlanetClass, GetWorld());
				
				// Validate HomePlanet was created successfully
				if (!HomePlanet)
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to generate HomePlanet!"));
					return;
				}
				// Runtime generation must not inherit an old Blueprint CDO opt-out.
				// The authored SinglePlay planet disables streaming explicitly in its
				// integration path; every newly generated solid body participates.
				HomePlanet->bStreamWorldScapeSurface = true;
				HomePlanet->bGenerateByDefault = false;
				
				// InitEnvironment below owns the single AtmoScape actor. Creating a second
				// atmosphere here left overlapping shells with different scaling state.
				// Detailed terrain is created lazily by the WorldScape streaming
				// subsystem. Generating it here left an unconditional root at startup.
				
				HomePlanet->AstroName = AGravityPlayerController::GenerateUniqueName("Planet");
			}

			const TArray<APlanetOrbit*>& PlanetOrbits =
				GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList;
			const int32 StartOrbitIndex = StartPlanetNumber - 1;
			if (!PlanetOrbits.IsValidIndex(StartOrbitIndex))
			{
				UE_LOG(LogTemp, Error,
					TEXT("Cannot place home planet: start index %d is outside %d generated orbits"),
					StartPlanetNumber, PlanetOrbits.Num());
				return;
			}
			APlanetOrbit* NewHomePlanetOrbit = PlanetOrbits[StartOrbitIndex];
			if (NewHomePlanetOrbit && NewHomePlanetOrbit->Planet)
			{
				NewHomePlanetOrbit->TriggerClearChildren();

				// Attach a planet from the editor to the orbit actor
				if (HomePlanet)
				{
					HomePlanet->AttachToActor(NewHomePlanetOrbit,
					                          FAttachmentTransformRules::KeepRelativeTransform);

					// Get the local position of the current planet and set the local position for your planet based on the stored offset
					const FVector OldPlanetLocalPosition = NewHomePlanetOrbit->Planet->GetActorLocation() -
						NewHomePlanetOrbit->GetActorLocation();
					HomePlanet->SetActorLocation(
						NewHomePlanetOrbit->GetActorLocation() + OldPlanetLocalPosition);

					// Create HomePlanetModel for operations
					const TSharedPtr<FPlanetModel> HomePlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(GeneratedWorldModel);
					PlanetarySystemGenerator->GeneratePlanetMoonsList(PlanetGenerator, MoonGenerator,
					                                                  HomePlanetModel, HomePlanetModel->Radius,
					                                                  GeneratedWorldModel->MoonsAmount);

					if (!AddGeneratedWorldModelData())
					{
						UE_LOG(LogTemp, Error,
							TEXT("Aborting generated-world handoff because its runtime model is incomplete"));
						return;
					}
					
					SpawnPlanetMoons(HomePlanetModel);

					if (bSpawnStarterLocation)
					{
						if (!SpawnStartInteractiveActors(HomePlanetModel))
						{
							UE_LOG(LogTemp, Error,
								TEXT("Aborting world save because starter hierarchy creation failed"));
							return;
						}
					}
					else
					{
						UE_LOG(LogTemp, Log,
							TEXT("[APS.WorldGeneration] Astronomical world committed without civilization starter actors"));
					}

					if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
					{
						if (AGravityPlayerController* MainController = Cast<AGravityPlayerController>(PC))
						{
							MainController->
								SaveNewWorld(GeneratedWorldModel->AstroGenerationLevel, GeneratedWorldModel);
						}
					}
				}
			}
		}
	}
}

void AAstroGenerator::GenerateStarSystemByModel()
{
	UE_LOG(LogTemp, VeryVerbose, TEXT("=== GenerateStarSystemByModel START ==="));
	
	if (CheckGeneratorsFails()) 
	{
		UE_LOG(LogTemp, Error, TEXT("CheckGeneratorsFails() returned true - generators failed!"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("Generate Star System!"));

		FTransform HomeSystemTransform;
		FVector HomeSystemSpawnLocation;
		ComputeHomeSystemPosition(HomeSystemTransform, HomeSystemSpawnLocation);

		/*RandomPosition:
		get random index from hism array indexes
		get index model
		from hism index to star model map get random pair*/

		// Create a new star system
		TSharedPtr<FStarSystemModel> StarSystemModel;
		int AmountOfStars;
		ComputeStarAmount(StarSystemModel, AmountOfStars);

		FClusterStarSystemRecord* HomeClusterRecord = PendingHomeCluster.IsValid()
			? PendingHomeCluster->FindPotentialSystemMutable(PendingHomeClusterInstanceIndex)
			: nullptr;
		if (HomeClusterRecord)
		{
			StarSystemModel = MakeShared<FStarSystemModel>(HomeClusterRecord->SystemModel);
			StarSystemModel->Location = HomeSystemSpawnLocation;
			if (bRandomHomeSystem)
			{
				AmountOfStars = FMath::Max(1, StarSystemModel->AmountOfStars);
			}
			else
			{
				// The selected Single/Double/Triple/Multiple value owns the home
				// system.  A lightweight cluster record supplies identity/location,
				// but must never silently replace the user's requested star count.
				StarSystemModel->StarSystemType = HomeSystemStarType;
				StarSystemModel->AmountOfStars = AmountOfStars;
			}
		}

		AStarSystem* NewStarSystem = World->SpawnActor<AStarSystem>(BP_StarSystemClass, HomeSystemTransform);
		if (!NewStarSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("NewStarSystem failed!"));
			return;
		}
		// Preserve the generated data hierarchy in the actor tree as well. KeepWorld
		// retains the absolute centimetre scale of the materialized star system while
		// its parent cluster can remain under the full-scale galaxy transform.
		AActor* SystemParent = PendingHomeCluster.IsValid()
			? static_cast<AActor*>(PendingHomeCluster.Get()) : static_cast<AActor*>(this);
		NewStarSystem->AttachToActor(SystemParent, FAttachmentTransformRules::KeepWorldTransform);
		NewStarSystem->SetActorLocation(HomeSystemSpawnLocation);
		StarSystemGenerator->ApplyModel(NewStarSystem, StarSystemModel);

		FVector LastStarLocation = HomeSystemSpawnLocation;
		// The authored SinglePlay world intentionally keeps its legacy placement.
		// Preview and newly committed procedural worlds must be able to inspect binary
		// and multiple systems without stacking every stellar/planetary hierarchy at
		// one transform. Keep every complete child tree until its physical envelope is
		// known, then distribute only non-authored systems safely around the barycentre.
		TArray<AStar*> StellarHierarchyStars;
		TArray<double> StellarEnvelopeRadiiCm;
		for (int StarNumber = 0; StarNumber < AmountOfStars; StarNumber++)
		{
			TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

			if (HomeClusterRecord && StarNumber == 0 && bRandomHomeStar)
			{
				*StarModel = HomeClusterRecord->PrimaryStarModel;
				StarModel->Location = HomeSystemSpawnLocation;
			}
			else if (bRandomHomeStar)
			{
				StarGenerator->GenerateRandomStarModel(StarModel);
			}
			else
			{
				StarModel->StellarType = HomeStarStellarType;
				StarModel->SpectralClass = HomeStarSpectralClass;
				StarGenerator->GenerateStarModel(StarModel);
			}

			if (HomeClusterRecord && StarNumber == 0 && StarModel.IsValid())
			{
				// The materialized home system and its distant CLUSTER glyph are one
				// astronomical object.  A user-selected star used to replace only the
				// actor model, leaving the catalogue point with its old random (often
				// white) spectrum.  Commit the selected primary model back to the stable
				// record and update the existing HISM payload in place.
				SynchronizeHomeClusterRecord(
					*HomeClusterRecord, *StarModel, *StarSystemModel);

				if (UHierarchicalInstancedStaticMeshComponent* HomeClusterHism =
					PendingHomeCluster->StarMeshInstances)
				{
					const FLinearColor HomeColor = StarGenerator->GetStarColor(
						HomeClusterRecord->PrimaryStarModel.SpectralClass,
						HomeClusterRecord->PrimaryStarModel.SpectralSubclass);
					HomeClusterHism->SetCustomDataValue(
						PendingHomeClusterInstanceIndex, 0, HomeColor.R, false);
					HomeClusterHism->SetCustomDataValue(
						PendingHomeClusterInstanceIndex, 1, HomeColor.G, false);
					HomeClusterHism->SetCustomDataValue(
						PendingHomeClusterInstanceIndex, 2, HomeColor.B, false);
					const double HomeEmission = UStarGenerator::GetFarStarVisualEmission(
						HomeClusterRecord->PrimaryStarModel.Radius,
						StarGenerator->CalculateEmission(
							HomeClusterRecord->PrimaryStarModel.Luminosity * 25.0));
					HomeClusterHism->SetCustomDataValue(
						PendingHomeClusterInstanceIndex, 3, HomeEmission, true);
				}
			}

			PlanetarySystemModel = MakeShared<FPlanetarySystemModel>();

			if (bRandomHomeSystemType)
			{
				PlanetarySystemGenerator->GeneratePlanetarySystemModelByStar(
					PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
			}
			else
			{
				PlanetarySystemModel->AmountOfPlanets = PlanetsAmount;
				PlanetarySystemModel->PlanetarySystemType = HomeSystemPlanetaryType;
				PlanetarySystemModel->OrbitDistributionType = HomeSystemOrbitDistributionType;
				PlanetarySystemGenerator->GenerateCustomPlanetarySystemModel(
					PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);

				// The menu preview must be driven by the user's model. The legacy
				// generator only consumed PlanetType/Radius/Moons in the separate
				// starter-planet path, which is deliberately disabled for previews.
				// Replace the selected home-planet model in-place so changing a Slate
				// control has an immediate, visible result without spawning gameplay
				// infrastructure or WorldScape terrain in the menu level.
				if ((bIsPreviewGeneration || !bIntegrateStartPlanet)
					&& StarNumber == 0 && GeneratedWorldModel
					&& PlanetarySystemModel->PlanetsList.Num() > 0)
				{
					const int32 PreviewHomeIndex = FMath::Clamp(
						StartPlanetNumber - 1, 0, PlanetarySystemModel->PlanetsList.Num() - 1);
					TSharedPtr<FPlanetData>& HomeData = PlanetarySystemModel->PlanetsList[PreviewHomeIndex];
					if (!HomeData.IsValid())
					{
						HomeData = MakeShared<FPlanetData>();
					}
					const double ExistingOrbitRadius = HomeData->OrbitRadius;
					HomeData->PlanetModel = PlanetGenerator->CreatePlanetModelFromGeneratedWorld(GeneratedWorldModel);
					HomeData->PlanetOrder = PreviewHomeIndex + 1;
					HomeData->OrbitRadius = ExistingOrbitRadius;
					if (HomeData->PlanetModel.IsValid())
					{
						HomeData->PlanetModel->OrbitDistance = ExistingOrbitRadius;
						PlanetarySystemGenerator->GeneratePlanetMoonsList(
							PlanetGenerator, MoonGenerator, HomeData->PlanetModel,
							HomeData->PlanetModel->Radius, GeneratedWorldModel->MoonsAmount);
					}
				}
			}

			AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass);
			APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);

			if (StarNumber == 0)
			{
				NewStarSystem->MainStar = NewStar;
			}

			if (!NewStar || !NewPlanetarySystem)
			{
				UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
				return;
			}
			if (StarNumber == 0)
			{
				HomePlanetarySystem = NewPlanetarySystem;
			}

			// Set Star full-scale
			// TODO: PlanetarySystemGenerator->ConnectStar()
			StarGenerator->ApplyModel(NewStar, StarModel);
			PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetarySystemModel);
			const FVector SystemCenter = NewStarSystem->GetActorLocation();
			NewStar->SetActorLocation(SystemCenter);
			NewPlanetarySystem->SetActorLocation(SystemCenter);
			NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
			NewStar->StarRadiusKM = StarModel->Radius * 696340;
			NewStar->SetPlanetarySystem(NewPlanetarySystem);
			NewPlanetarySystem->SetStar(NewStar);
			NewStarSystem->AddNewStar(NewStar);
			NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
			StarGenerator->ApplySpectralMaterial(NewStar, StarModel);
			NewStar->FullSpectralName = NewStar->GenerateFullSpectralName();
			NewPlanetarySystem->SetStarFullSpectralName(NewStar->FullSpectralName);
			const FString SpectralIdentity = NewStar->FullSpectralName.IsNone()
				? TEXT("Star") : NewStar->FullSpectralName.ToString();
			NewStar->AstroName = AGravityPlayerController::GenerateUniqueName(SpectralIdentity);
			if (StarNumber == 0)
			{
				HomeStar = NewStar;
			}

			// Generate planets for each star
			FVector LastPlanetLocation{0};
			double PreviousSafeOrbitRadiusCm = 0.0;
			double PreviousPlanetEnvelopeCm = 0.0;
			int32 PlanetIndex = 0;
			for (const TSharedPtr<FPlanetData> FPlanetData : PlanetarySystemModel->PlanetsList)
			{
				if (!FPlanetData.IsValid())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.WorldGeneration] Skipping invalid planet data at index %d"), PlanetIndex);
					++PlanetIndex;
					continue;
				}
				APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(
					BP_PlanetOrbitClass, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
				if (!NewPlanetOrbit)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.WorldGeneration] Failed to spawn planet orbit at index %d"), PlanetIndex);
					++PlanetIndex;
					continue;
				}
				NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);

				// Planet Model and generation
				TSharedPtr<FPlanetModel> PlanetModel = FPlanetData->PlanetModel;
				if (!PlanetModel.IsValid())
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.WorldGeneration] Skipping invalid planet model at index %d"), PlanetIndex);
					NewPlanetOrbit->Destroy();
					++PlanetIndex;
					continue;
				}
				APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);
				if (!NewPlanet)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[APS.WorldGeneration] Failed to spawn planet at index %d"), PlanetIndex);
					NewPlanetOrbit->Destroy();
					++PlanetIndex;
					continue;
				}
				NewPlanet->bStreamWorldScapeSurface = true;
				NewPlanet->bGenerateByDefault = false;

				PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
				NewPlanet->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Planet"));
				NewStar->AddPlanet(NewPlanet);
				NewPlanet->SetParentStar(NewStar);

				// Set planet full-scale
				NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
				constexpr double AuToCentimetres = 14960000000000.0;
				double OrbitRadiusCm = PlanetModel->OrbitDistance * AuToCentimetres;
				if (bIsPreviewGeneration || !bIntegrateStartPlanet)
				{
					// A selected/generated stellar type can be a dwarf or a hypergiant.
					// Push every procedural orbit beyond the photosphere and preserve a
					// deterministic gap from its predecessor. The authored SinglePlay
					// hierarchy keeps its deliberately hand-placed legacy distances.
					const double PlanetRadiusCm = FMath::Max(
						PlanetModel->Radius * 6371.0 * 100000.0, 100000.0);
					const double PlanetEnvelopeCm = PlanetRadiusCm * 2.5;
					const double StellarClearanceCm = FMath::Max(
						NewStar->StarRadiusKM * 100000.0 * 1.35 + PlanetEnvelopeCm,
						NewStar->StarRadiusKM * 100000.0 + PlanetEnvelopeCm * 2.0);
					OrbitRadiusCm = FMath::Max(OrbitRadiusCm, StellarClearanceCm);
					if (PreviousSafeOrbitRadiusCm > 0.0)
					{
						const double InterOrbitGapCm = FMath::Max(
							AuToCentimetres * 0.01, NewStar->StarRadiusKM * 100000.0 * 0.08);
						OrbitRadiusCm = FMath::Max(OrbitRadiusCm,
							PreviousSafeOrbitRadiusCm + PreviousPlanetEnvelopeCm
							+ PlanetEnvelopeCm + InterOrbitGapCm);
					}
					PreviousSafeOrbitRadiusCm = OrbitRadiusCm;
					PreviousPlanetEnvelopeCm = PlanetEnvelopeCm;
					PlanetModel->OrbitDistance = OrbitRadiusCm / AuToCentimetres;
					FPlanetData->OrbitRadius = PlanetModel->OrbitDistance;
				}
				const FVector OrbitOffset(OrbitRadiusCm, 0, 0);
				const FVector NewLocation = NewPlanetOrbit->GetActorLocation()
					+ NewPlanetOrbit->GetActorQuat().RotateVector(OrbitOffset);
				NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
				NewPlanet->SetActorLocation(NewLocation);
				NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);
				NewPlanetOrbit->Planet = NewPlanet;
				// Commit the orbit only together with its successfully materialized body.
				// Adding it before model/actor validation left destroyed pointers in the
				// array and made the system report more orbit rings than planets.
				NewPlanetarySystem->PlanetOrbitsList.Add(NewPlanetOrbit);
				// The authored SinglePlay integration already owns HomePlanet. Do not
				// replace that legacy reference with the generated orbit placeholder:
				// IntegrateStartPlanetIntoSystem destroys the placeholder tree after
				// attaching the authored headquarters to HomePlanet. Overwriting this
				// pointer therefore destroyed the headquarters and every attached
				// station at startup.
				// A menu preview is always a generated hierarchy, even if the Blueprint
				// class inherited the authored SinglePlay default bIntegrateStartPlanet.
				// Without this explicit preview branch the planet existed in the list but
				// HomePlanet stayed null, so PLANET had no camera/WorldScape target.
				if ((bIsPreviewGeneration || !bIntegrateStartPlanet)
					&& StarNumber == 0 && PlanetIndex == FMath::Clamp(
					StartPlanetNumber - 1, 0, PlanetarySystemModel->PlanetsList.Num() - 1))
				{
					HomePlanet = NewPlanet;
				}

				// Generate Moons
				double DiameterOfLastMoon = 0;
				FVector LastMoonLocation;
				for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
				{
					if (!MoonData.IsValid() || !MoonData->MoonModel.IsValid())
					{
						continue;
					}
					const double KM_TO_UE_UNIT_SCALE = 100000;
					APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
						BP_PlanetOrbitClass, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
					if (!NewMoonOrbit)
					{
						continue;
					}
					NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);

					FVector MoonLocation = NewPlanet->GetActorLocation();
					AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
					if (!NewMoon)
					{
						NewMoonOrbit->Destroy();
						continue;
					}
					NewMoon->bStreamWorldScapeSurface = true;
					NewMoon->bGenerateByDefault = false;
					NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
					NewPlanet->AddMoon(NewMoon);
					NewMoon->SetParentPlanet(NewPlanet);
					NewPlanet->MoonOrbitsList.Add(NewMoonOrbit);

					MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
					MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

					// Set moon full-scale
					double MoonRadius = MoonData->MoonModel->Radius;
					NewMoon->RadiusKM = MoonRadius * 6371;
					NewMoon->PlanetRadiusKM = FMath::Max(1, FMath::RoundToInt(NewMoon->RadiusKM));
					NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));

					FVector Origin, BoxExtent;
					double SphereRadius;
					NewMoon->GetActorBounds(false, Origin, BoxExtent);
					SphereRadius = BoxExtent.GetMax();
					NewMoon->AffectionRadiusKM = SphereRadius / 100000.0;

					NewMoon->AddActorLocalOffset(FVector(
						0, ((PlanetModel->RadiusKM + (MoonData->OrbitRadius * PlanetModel->RadiusKM)) *
							KM_TO_UE_UNIT_SCALE) * 1, 0));
					NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);

					DiameterOfLastMoon = MoonRadius * 2;
					LastMoonLocation = NewMoon->GetActorLocation();

					NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewMoon->RadiusKM, NewMoon);
				}

				if (DiameterOfLastMoon == 0)
				{
					NewPlanet->PlanetaryZone->SetSphereRadius(100);
					NewPlanet->AffectionRadiusKM = 100 * NewPlanet->GetActorScale3D().X / 100000;
				}
				else
				{
					FVector PlanetLocation = NewPlanet->GetActorLocation();
					FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
					double SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
					SphereRadius /= NewPlanet->GetActorScale3D().X;
					SphereRadius *= 1.5;
					NewPlanet->PlanetaryZone->SetSphereRadius(SphereRadius);
					NewPlanet->AffectionRadiusKM = SphereRadius * NewPlanet->GetActorScale3D().X / 100000;
				}
				LastPlanetLocation = NewPlanet->GetActorLocation();
				LastStarLocation = LastPlanetLocation * 1.1;

				NewPlanet->OrbitHeight = (NewPlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
					NewPlanet->RadiusKM;

				NewPlanet->PlanetaryEnvironmentGenerator->InitEnviroment(NewPlanet, World);
				if (AAtmoScape* Atmosphere =
					NewPlanet->PlanetaryEnvironmentGenerator->PlanetAtmosphere)
				{
					// AtmoScape otherwise falls back to its zeroed generic sunlight input.
					// Bind every generated planet to its actual parent star so the rim remains
					// visible in both menu preview and committed full-scale gameplay.
					Atmosphere->LightSource = NewPlanet->ParentStar;
					Atmosphere->UpdateScale();
				}
				if (NewPlanet == HomePlanet && GeneratedWorldModel)
				{
					// Apply the same atmosphere values edited in Slate to the live
					// preview and to the generated gameplay home world.
					if (AAtmoScape* Atmosphere = NewPlanet->PlanetaryEnvironmentGenerator->PlanetAtmosphere)
					{
						Atmosphere->bKeepRelativeScale = false;
						Atmosphere->AtmosphereHeight = GeneratedWorldModel->AtmosphereHeight;
						Atmosphere->AtmosphereOpacity = GeneratedWorldModel->AtmosphereOpacity;
						Atmosphere->MultiScatering = GeneratedWorldModel->AtmosphereMultiScattering;
						Atmosphere->RayleighHeight = GeneratedWorldModel->AtmosphereRayleighScattering;
						Atmosphere->RayleighScattering = GeneratedWorldModel->AtmosphereColor;
						Atmosphere->UpdateScale();
					}
				}
				++PlanetIndex;
			}

			// Place Orbits
			if (bOrbitRotationCheck)
			{
				UE_LOG(LogTemp, Warning, TEXT("NewPlanetarySystem->PlanetsActorsList: %d"),
				       NewPlanetarySystem->PlanetsActorsList.Num());
				for (int i = 1; i < NewPlanetarySystem->PlanetsActorsList.Num(); i++)
				{
					APlanet* CurrentPlanet = NewPlanetarySystem->PlanetsActorsList[i];
					APlanet* PreviousPlanet = NewPlanetarySystem->PlanetsActorsList[i - 1];

					double CurrentPlanetLocation = CurrentPlanet->GetActorLocation().X;
					double PreviousPlanetLocation = PreviousPlanet->GetActorLocation().X;
					double SumOfAffectionZones = (CurrentPlanet->AffectionRadiusKM + PreviousPlanet->AffectionRadiusKM)
						* 1000000; // Converting to the same unit as locations
					double DistanceBetweenPlanets = CurrentPlanetLocation - PreviousPlanetLocation;

					if (DistanceBetweenPlanets <= SumOfAffectionZones)
					{
						double OrbitCoeff = CurrentPlanet->AffectionRadiusKM * 100000;
						OrbitCoeff *= 1.25;
						switch (NewPlanetarySystem->OrbitDistributionType)
						{
						case EOrbitDistributionType::Uniform:
							{
								OrbitCoeff = NewStar->RadiusKM / 2 * 100000 * (CurrentPlanet->Radius);
								break;
							}
						case EOrbitDistributionType::Chaotic:
							{
								OrbitCoeff = OrbitCoeff * FMath::RandRange(1, 10) * 2;
								break;
							}
						default:
							break;
						}

						// Shift it by the AffectionRadiusKM value of the current planet
						double NewLocationX = (PreviousPlanetLocation + PreviousPlanet->AffectionRadiusKM * 100000) + ((
							CurrentPlanet->AffectionRadiusKM * 100000) + OrbitCoeff);

						FVector NewLocation = CurrentPlanet->GetActorLocation();
						NewLocation.X = NewLocationX;
						CurrentPlanet->SetActorLocation(NewLocation);
						LastPlanetLocation = NewLocation;

						FVector OldLocation = CurrentPlanet->GetActorLocation();
						double OldLocationX = OldLocation.X;
						CurrentPlanet->SetActorLocation(NewLocation);

						UE_LOG(LogTemp, Warning,
						       TEXT("Planet %d: Moved from %f to %f, Distance: %f, SumOfAffectionZones: %f"), i,
						       OldLocationX, NewLocationX, DistanceBetweenPlanets, SumOfAffectionZones);
					}

					// Check Moons Orbits
					UE_LOG(LogTemp, Warning, TEXT("CurrentPlanet->Moons.Num(): %d"), CurrentPlanet->Moons.Num());
					for (int j = 1; j < CurrentPlanet->Moons.Num(); j++)
					{
						AMoon* CurrentMoon = CurrentPlanet->Moons[j];
						AMoon* PreviousMoon = CurrentPlanet->Moons[j - 1];

						double CurrentMoonLocation = CurrentMoon->GetActorLocation().Y;
						double PreviousMoonLocation = PreviousMoon->GetActorLocation().Y;
						double MoonSumOfAffectionZones = (CurrentMoon->AffectionRadiusKM + PreviousMoon->
							AffectionRadiusKM) * 100000; // Converting to the same unit as locations
						double DistanceBetweenMoons = CurrentMoonLocation - PreviousMoonLocation;

						if (DistanceBetweenMoons <= MoonSumOfAffectionZones)
						{
							double OrbitCoeff = CurrentMoon->AffectionRadiusKM * 100000;
							OrbitCoeff *= 1.1;
							double NewLocationY = (PreviousMoonLocation + PreviousMoon->AffectionRadiusKM * 100000)
								+ ((CurrentMoon->AffectionRadiusKM * 100000) + OrbitCoeff);
							CurrentMoon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
							FVector NewLocation = CurrentMoon->GetActorLocation();
							NewLocation.Y = NewLocationY;
							CurrentMoon->SetActorLocation(NewLocation);
							CurrentMoon->
								AttachToActor(CurrentPlanet, FAttachmentTransformRules::KeepWorldTransform);
							UE_LOG(LogTemp, Warning,
							       TEXT("Moon %d: Moved from %f to %f, Distance: %f, SumOfAffectionZones: %f"), j,
							       PreviousMoonLocation, CurrentMoonLocation, DistanceBetweenMoons,
							       MoonSumOfAffectionZones);
						}
					}
				}
			}

			if (bNeedOrbitRotation)
			{
				RotatePlanetOrbits(NewPlanetarySystem);
			}

			// StarAffectionZoneRadius and StarSystemRadius are logical world-centimetre
			// envelopes. USphereComponent::SetSphereRadius expects an unscaled local
			// radius, so mixing the two made distant systems tiny and nearby ones huge.
			double StellarEnvelopeCm = FMath::Max(
				NewStar->StarRadiusKM * 100000.0 * 1.35, 500.0);
			for (const APlanet* Planet : NewPlanetarySystem->PlanetsActorsList)
			{
				if (!IsValid(Planet))
				{
					continue;
				}
				const double PlanetOuterEdgeCm = FVector::Distance(
					NewStar->GetActorLocation(), Planet->GetActorLocation())
					+ FMath::Max(Planet->PlanetRadiusKM * 100000.0 * 2.5, 100000.0);
				StellarEnvelopeCm = FMath::Max(StellarEnvelopeCm, PlanetOuterEdgeCm);
			}
			const double StellarSafeRadiusCm = StellarEnvelopeCm * 1.08;
			const double StarWorldScale = FMath::Max(
				NewStar->GetActorTransform().GetScale3D().GetAbsMax(), UE_DOUBLE_SMALL_NUMBER);
			NewStar->PlanetarySystemZone->SetSphereRadius(StellarSafeRadiusCm / StarWorldScale);
			NewStar->StarAffectionZoneRadius = StellarSafeRadiusCm;
			NewStar->CalculateAffectionRadius();
			NewStarSystem->StarSystemRadius = FMath::Max(
				NewStarSystem->StarSystemRadius, StellarSafeRadiusCm);
			StellarHierarchyStars.Add(NewStar);
			StellarEnvelopeRadiiCm.Add(StellarSafeRadiusCm);
			LastStarLocation = NewStar->GetActorLocation() + FVector(StellarSafeRadiusCm, 0.0, 0.0);
		}

		const bool bCanSeparateStellarHierarchies = bIsPreviewGeneration || !bIntegrateStartPlanet;
		if (bCanSeparateStellarHierarchies && StellarHierarchyStars.Num() > 1
			&& StellarHierarchyStars.Num() == StellarEnvelopeRadiiCm.Num())
		{
			const int32 StellarHierarchyCount = StellarHierarchyStars.Num();
			double LargestEnvelopeCm = 0.0;
			for (const double EnvelopeCm : StellarEnvelopeRadiiCm)
			{
				LargestEnvelopeCm = FMath::Max(LargestEnvelopeCm, EnvelopeCm);
			}
			const double AngularClearance = FMath::Max(
				FMath::Sin(UE_PI / static_cast<double>(StellarHierarchyCount)), 0.25);
			const double StellarRingRadiusCm = LargestEnvelopeCm * 1.18 / AngularClearance;
			for (int32 StellarIndex = 0; StellarIndex < StellarHierarchyCount; ++StellarIndex)
			{
				AStar* StellarRoot = StellarHierarchyStars[StellarIndex];
				if (!IsValid(StellarRoot))
				{
					continue;
				}
				const double Angle = UE_TWO_PI * static_cast<double>(StellarIndex)
					/ static_cast<double>(StellarHierarchyCount);
				StellarRoot->SetActorLocation(NewStarSystem->GetActorLocation() + FVector(
					FMath::Cos(Angle) * StellarRingRadiusCm,
					FMath::Sin(Angle) * StellarRingRadiusCm, 0.0));
			}
			LastStarLocation = NewStarSystem->GetActorLocation()
				+ FVector(StellarRingRadiusCm + LargestEnvelopeCm, 0.0, 0.0);
			UE_LOG(LogTemp, Log,
				TEXT("[APS.WorldGeneration] Separated %d stellar hierarchies; ring %.3e cm envelope %.3e cm"),
				StellarHierarchyCount, StellarRingRadiusCm, LargestEnvelopeCm);
		}
		else if (!StellarEnvelopeRadiiCm.IsEmpty())
		{
			double LargestAuthoredEnvelopeCm = 0.0;
			for (const double EnvelopeCm : StellarEnvelopeRadiiCm)
			{
				LargestAuthoredEnvelopeCm = FMath::Max(LargestAuthoredEnvelopeCm, EnvelopeCm);
			}
			LastStarLocation = NewStarSystem->GetActorLocation()
				+ FVector(LargestAuthoredEnvelopeCm, 0.0, 0.0);
		}

		double StarSystemSphereRadius = FVector::Dist(NewStarSystem->GetActorLocation(), LastStarLocation);
		StarSystemSphereRadius = FMath::Max(StarSystemSphereRadius * 1.08, 500.0);
		const double SystemWorldScale = FMath::Max(
			NewStarSystem->GetActorTransform().GetScale3D().GetAbsMax(), UE_DOUBLE_SMALL_NUMBER);
		NewStarSystem->StarSystemZone->SetSphereRadius(StarSystemSphereRadius / SystemWorldScale);
		NewStarSystem->StarSystemRadius = StarSystemSphereRadius;
		GeneratedHomeStarSystem = NewStarSystem;
		// Keep the materialized-system registry synchronized with the actor hierarchy.
		// Cluster catalogue entries remain lightweight records; this array contains
		// only systems that currently exist as actors (the home system for now).
		GeneratedStarSystems.Reset();
		GeneratedStarSystems.Add(NewStarSystem);
		UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem set to: %s"), *GeneratedHomeStarSystem->GetName());
		NewStarSystem->CalculateAffectionRadius();

		if (HomeClusterRecord && PendingHomeCluster.IsValid())
		{
			int32 MaterializedPlanetCount = 0;
			for (const AStar* MaterializedStar : NewStarSystem->GetStars())
			{
				if (IsValid(MaterializedStar) && IsValid(MaterializedStar->PlanetarySystem))
				{
					MaterializedPlanetCount +=
						MaterializedStar->PlanetarySystem->PlanetsActorsList.Num();
				}
			}
			// Once the lightweight catalogue point has a real hierarchy, its summary
			// must describe that hierarchy rather than the earlier probability estimate.
			// This keeps CLUSTER labels/occupancy coherent after PlanetsAmount or
			// Single/Double/Triple changes without rebuilding the surrounding catalogue.
			HomeClusterRecord->SystemModel.AmountOfStars = NewStarSystem->GetStars().Num();
			HomeClusterRecord->SystemModel.PotentialPlanetCount = MaterializedPlanetCount;
			HomeClusterRecord->SystemModel.bHasPlanetarySystem = MaterializedPlanetCount > 0;
			HomeClusterRecord->bMaterialized = true;
			HomeClusterRecord->MaterializedSystem = NewStarSystem;
			// Keep the lightweight HISM point at this catalog address. It is the stable
			// barycentric representation used by GALAXY/CLUSTER while the real stellar
			// hierarchy stays hidden. Deep scopes cull this point together with every
			// other proxy inside the materialized system safe zone, so no duplicate is
			// visible and no real StarMesh needs to be teleported between presentations.
			if (UHierarchicalInstancedStaticMeshComponent* Hism = PendingHomeCluster->StarMeshInstances)
			{
				if (Hism->NumCustomDataFloats > 5)
				{
					Hism->SetCustomDataValue(PendingHomeClusterInstanceIndex, 5,
						MaterializedPlanetCount / 12.0f, true);
				}
				FTransform ProxyWorldTransform;
				if (Hism->GetInstanceTransform(
					PendingHomeClusterInstanceIndex, ProxyWorldTransform, true))
				{
					if (const UStaticMesh* ProxyMesh = Hism->GetStaticMesh())
					{
						MaterializedHomeProxyWorldRadius =
							static_cast<double>(ProxyMesh->GetBounds().SphereRadius)
							* ProxyWorldTransform.GetScale3D().GetAbsMax();
						if (!FMath::IsFinite(MaterializedHomeProxyWorldRadius)
							|| MaterializedHomeProxyWorldRadius <= UE_SMALL_NUMBER)
						{
							MaterializedHomeProxyWorldRadius = 0.0;
						}
					}
				}
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Cluster] Home system %s materialized from HISM instance %d"),
				*HomeClusterRecord->StableId.ToString(EGuidFormats::DigitsWithHyphensLower),
				PendingHomeClusterInstanceIndex);
		}

		if (NewStarSystem->StarSystemRadius == 0)
		{
			if (NewStarSystem->MainStar && NewStarSystem->MainStar->PlanetarySystemZone)
			{
				double SphereRadius = NewStarSystem->MainStar->PlanetarySystemZone->GetScaledSphereRadius() * 1.25;
				NewStarSystem->StarSystemRadius = SphereRadius;
				const double FallbackSystemScale = FMath::Max(
					NewStarSystem->GetActorTransform().GetScale3D().GetAbsMax(), UE_DOUBLE_SMALL_NUMBER);
				NewStarSystem->StarSystemZone->SetSphereRadius(SphereRadius / FallbackSystemScale);
			}
			else
			{
				if (!NewStarSystem->MainStar)
				{
					UE_LOG(LogTemp, Error, TEXT("NewStarSystem->MainStar is null"));
				}
				else if (!NewStarSystem->MainStar->PlanetarySystemZone)
				{
					UE_LOG(LogTemp, Error, TEXT("NewStarSystem->MainStar->PlanetarySystemZone is null"));
				}
			}
		}

		// Never RemoveInstance from the galaxy/cluster HISM here. Both components are
		// intentionally non-colliding, and removing an instance would invalidate the
		// stable InstanceIndex -> system-record identity. The menu hides visual
		// intruders by reversible zero-scale presentation culling; persistent spatial
		// separation belongs to the deterministic catalogue-generation pass.
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Falied to get World!"));
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("=== GenerateStarSystemByModel END ==="));
}

void AAstroGenerator::SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld)
{
	this->GeneratedWorldModel = InGeneratedWorld;
	if (IsValid(InGeneratedWorld))
	{
		PreviewGenerationSeed = FMath::Max(1, InGeneratedWorld->GenerationSeed);
	}
}

void AAstroGenerator::DisplayNewGeneratedWorld()
{
	this->GeneratedWorldModel->PrintAllValues();
}

void AAstroGenerator::InitGenerationLevel()
{
	bool bGeneratedHomeSystemAsPrimaryLevel = false;

	switch (AstroGenerationLevel)
	{
	case EAstroGenerationLevel::GalaxiesCluster:
		GenerateGalaxiesCluster();
		break;
	case EAstroGenerationLevel::Galaxy:
	case EAstroGenerationLevel::StarCluster:
		// The generation menu navigates one persistent hierarchy. Galaxy and
		// StarCluster used to be mutually exclusive switch branches, so one of the
		// corresponding focus buttons always framed a fallback root. Build both
		// lightweight HISM layers and materialize the home system below them.
		GenerateGalaxy();
		GenerateStarCluster();
		break;
	case EAstroGenerationLevel::StarSystem:
		GenerateHomeStarSystem();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	case EAstroGenerationLevel::PlanetSystem:
		GeneratePlanetSystem();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	case EAstroGenerationLevel::SinglePlanet:
		GenerateSinglePlanet();
		bGeneratedHomeSystemAsPrimaryLevel = true;
		break;
	default:
		GenerateRandomWorld();
		break;
	}

	if (bGenerateHomeSystem && !bGeneratedHomeSystemAsPrimaryLevel)
	{
		GenerateHomeStarSystem();
	}
}

void AAstroGenerator::GenerateGalaxy()
{
	if (!BP_GalaxyClass)
	{
		UE_LOG(LogTemp, Error, TEXT("BP_GalaxyClass is not set!"));
		return;
	}

	TSharedPtr<FGalaxyModel> GalaxyModel = MakeShared<FGalaxyModel>();

	if (bGenerateRandomGalaxy)
	{
		GalaxyGenerator->GenerateRandomGalaxyModel(GalaxyModel);
	}
	else
	{
		GalaxyModel->GalaxyClass = GalaxyGlass;
		GalaxyModel->GalaxyType = GalaxyType;
		GalaxyModel->StarsCount = GalaxyStarCount;
		GalaxyModel->StarsDensity = GalaxyStarDensity;
		GalaxyModel->GalaxySize = GalaxySize;
	}

	const int32 ModeledStarCount = FMath::Max(1, GalaxyModel->StarsCount);
	const int32 InstanceBudget = bIsPreviewGeneration
		? FMath::Min(FMath::Max(100, PreviewMaxInstances), 1800)
		: FMath::Max(1000, RuntimeMaxGalaxyInstances);
	const int32 RenderedStarCount = FMath::Min(ModeledStarCount, InstanceBudget);
	if (RenderedStarCount < ModeledStarCount)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[APS.WorldGeneration] Galaxy uses %d rendered HISM samples for %d catalog stars (%s)"),
			RenderedStarCount,
			ModeledStarCount,
			bIsPreviewGeneration ? TEXT("preview") : TEXT("gameplay"));
	}

	UWorld* World = GetWorld();

	if (World)
	{
		AGalaxy* NewGalaxy = World->SpawnActor<AGalaxy>(BP_GalaxyClass);
		if (!IsValid(NewGalaxy) || !IsValid(NewGalaxy->StarMeshInstances))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn a usable galaxy actor"));
			return;
		}
		NewGalaxy->GalaxyType = GalaxyModel->GalaxyType;
		NewGalaxy->GalaxyGlass = GalaxyModel->GalaxyClass;
		const int32 GalaxySeed = HashCombine(GetTypeHash(GalaxySize),
			HashCombine(GetTypeHash(GalaxyStarCount), GetTypeHash(static_cast<uint8>(GalaxyType))));
		GalaxyGenerator->GenerateGalaxyOctreeStars(
			StarGenerator, NewGalaxy, GalaxyModel, RenderedStarCount, GalaxySeed,
			bIsPreviewGeneration);
		NewGalaxy->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		GeneratedGalaxy = NewGalaxy;

		if (bGenerateFullScaledWorld)
		{
			this->SetActorScale3D(FVector(FullScaleValue));
		}
	}
}


void AAstroGenerator::ShowPlanetsList(TArray<TSharedPtr<FPlanetData>> PlanetDataMap)
{
	for (const TSharedPtr<FPlanetData>& PlanetDataPtr : PlanetDataMap)
	{
		if (PlanetDataPtr.IsValid())
		{
			FPlanetData PlanetData = *(PlanetDataPtr.Get());
			UE_LOG(LogTemp, Warning, TEXT("    Planet Order: %d"), PlanetData.PlanetOrder);
			UE_LOG(LogTemp, Warning, TEXT("     Orbit Radius: %f"), PlanetData.OrbitRadius);

			// �������� ������ �������
			TSharedPtr<FPlanetModel> PlanetModel = PlanetData.PlanetModel;

			// ����� ������ ������ �������
			if (PlanetModel.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("     Planet Type: %s"),
				       *UEnum::GetValueAsString(PlanetModel->PlanetType));
				UE_LOG(LogTemp, Warning, TEXT("     Planet Zone: %s"),
				       *UEnum::GetValueAsString(PlanetModel->PlanetZone));
				UE_LOG(LogTemp, Warning, TEXT("     Temperature: %d"), PlanetModel->Temperature);
				UE_LOG(LogTemp, Warning, TEXT("     Planet Density: %f"), PlanetModel->PlanetDensity);
				UE_LOG(LogTemp, Warning, TEXT("     Planet Gravity Strength: %f"),
				       PlanetModel->PlanetGravityStrength);
				UE_LOG(LogTemp, Warning, TEXT("     Amount of Moons: %d"), PlanetModel->AmountOfMoons);

				// ����� ���������� � ���������
				TArray<TSharedPtr<FMoonData>> MoonsList = PlanetModel->MoonsList;
				for (int32 i = 0; i < MoonsList.Num(); i++)
				{
					if (MoonsList[i].IsValid())
					{
						FMoonData MoonData = *(MoonsList[i].Get());

						// ������� ������ ��������
						UE_LOG(LogTemp, Warning, TEXT("         Moon Order: %d"), MoonData.MoonOrder);
						UE_LOG(LogTemp, Warning, TEXT("             Moon Orbit Radius: %f"), MoonData.OrbitRadius);

						// �������� ������ Moon
						TSharedPtr<FMoonModel> MoonModel = MoonData.MoonModel;

						// ����� ������ ������ Moon
						if (MoonModel.IsValid())
						{
							UE_LOG(LogTemp, Warning, TEXT("             Moon Type: %s"),
							       *UEnum::GetValueAsString(MoonModel->Type));
							UE_LOG(LogTemp, Warning, TEXT("             Moon Density: %f"), MoonModel->MoonDensity);
							UE_LOG(LogTemp, Warning, TEXT("             Moon Gravity: %f"), MoonModel->MoonGravity);
						}
					}
				}
			}
		}
	}
}

void AAstroGenerator::SpawnMoons(UWorld* World, APlanet* Planet, const int32 NumberOfMoons)
{
	if (!Planet || NumberOfMoons <= 0 || !BP_MoonClass || !BP_PlanetOrbitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid parameters for spawning moons."));
		return;
	}

	const double KM_TO_CM = 100000.0;
	const double SCALE_FACTOR = 1.0 / 50.0;

	// Planet radius in UE units
	const double PlanetRadiusUE = Planet->PlanetRadiusKM * KM_TO_CM * SCALE_FACTOR;
	const double MinOrbitDistanceUE = PlanetRadiusUE * 1.2;
	const double MaxOrbitDistanceUE = PlanetRadiusUE * 10;

	// Moon generation
	for (int32 i = 0; i < NumberOfMoons; ++i)
	{
		// Create an orbit for the moon
		if (APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
			BP_PlanetOrbitClass, Planet->GetActorLocation(),
			FRotator::ZeroRotator))
		{
			NewMoonOrbit->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);
			Planet->MoonOrbitsList.Add(NewMoonOrbit);

			// Spawn the moon
			FVector MoonLocation = Planet->GetActorLocation();
			if (AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator))
			{
				NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
				Planet->AddMoon(NewMoon);
				NewMoon->SetParentPlanet(Planet);

				// Set the scale of the moon
				const double MoonRadiusKm = FMath::RandRange(100.0, 1000.0); // Moon radius in kilometers
				NewMoon->RadiusKM = MoonRadiusKm;
				NewMoon->PlanetRadiusKM = FMath::Max(1, FMath::RoundToInt(MoonRadiusKm));
				const double MoonRadiusInCm = MoonRadiusKm * KM_TO_CM * SCALE_FACTOR;
				NewMoon->SetActorScale3D(FVector(MoonRadiusInCm));

				// Set the orbital distance
				const double OrbitDistanceUE = FMath::RandRange(MinOrbitDistanceUE, MaxOrbitDistanceUE);
				const FVector OffsetLocation = FVector(0, OrbitDistanceUE, 0);
				// Orbit of the moon at a distance from the planet
				NewMoon->AddActorLocalOffset(OffsetLocation);
				NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
				NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(World, NewMoon->RadiusKM, NewMoon);
			}
		}
	}
}

void AAstroGenerator::DestroyActorTree(AActor* Root)
{
	if (!IsValid(Root)) return;

	// 1) Собираем всех прямых "детей": как прикреплённых, так и из ChildActorComponent
	TArray<AActor*> Children;
	Root->GetAttachedActors(Children); // прямые прикреплённые

	TArray<UChildActorComponent*> ChildComps;
	Root->GetComponents<UChildActorComponent>(ChildComps);
	for (UChildActorComponent* CAC : ChildComps)
	{
		if (AActor* Child = CAC ? CAC->GetChildActor() : nullptr)
		{
			Children.AddUnique(Child);
		}
	}

	// 2) Рекурсивно удаляем "листья"
	for (AActor* Child : Children)
	{
		DestroyActorTree(Child);
	}

	// 3) Отцепляем и удаляем сам корень
	Root->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Root->Destroy();
}

void AAstroGenerator::SpawnPlanetMoons(const TSharedPtr<FPlanetModel>& PlanetModel)
{
	if (!PlanetModel.IsValid() || !IsValid(HomePlanet) || !GetWorld())
	{
		return;
	}
	double DiameterOfLastMoon = 0;
	FVector LastMoonLocation;
	for (const TSharedPtr<FMoonData> MoonData : PlanetModel->MoonsList)
	{
		if (!MoonData.IsValid() || !MoonData->MoonModel.IsValid())
		{
			continue;
		}
		const double KM_TO_UE_UNIT_SCALE = 100000;
		APlanetOrbit* NewMoonOrbit = GetWorld()->SpawnActor<APlanetOrbit>(
			BP_PlanetOrbitClass, HomePlanet->GetActorLocation(), FRotator::ZeroRotator);
		if (!NewMoonOrbit)
		{
			continue;
		}
		NewMoonOrbit->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);

		FVector MoonLocation = HomePlanet->GetActorLocation();
		AMoon* NewMoon = GetWorld()->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
		if (!NewMoon)
		{
			NewMoonOrbit->Destroy();
			continue;
		}
		NewMoon->bStreamWorldScapeSurface = true;
		NewMoon->bGenerateByDefault = false;
		NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
		HomePlanet->AddMoon(NewMoon);
		NewMoon->SetParentPlanet(HomePlanet);
		HomePlanet->MoonOrbitsList.Add(NewMoonOrbit);

		MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
		MoonGenerator->ConnectMoonWithPlanet(NewMoon, HomePlanet);

		// set moon full-scale
	double MoonRadius = MoonData->MoonModel->Radius;
	NewMoon->RadiusKM = MoonRadius * 6371;
	NewMoon->PlanetRadiusKM = FMath::Max(1, FMath::RoundToInt(NewMoon->RadiusKM));
	NewMoon->SetActorScale3D(FVector(MoonRadius * 12742000));

		FVector Origin, BoxExtent;
		double SphereRadius;
		NewMoon->GetActorBounds(false, Origin, BoxExtent);
		SphereRadius = BoxExtent.GetMax();
		NewMoon->AffectionRadiusKM = SphereRadius / 100000.0;

		NewMoon->AddActorLocalOffset(FVector(
			0, ((PlanetModel->RadiusKM + (MoonData->OrbitRadius * PlanetModel->RadiusKM)) *
				KM_TO_UE_UNIT_SCALE) * 1, 0));
		NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);

		DiameterOfLastMoon = MoonRadius * 2;
		LastMoonLocation = NewMoon->GetActorLocation();

		SetMoonRotation(NewMoonOrbit);

		// Atmosphere metadata is independent from the streamed surface. Do not
		// allocate a WorldScape root here; the nearest-body streamer owns it.
		NewMoon->PlanetaryEnvironmentGenerator->InitAtmoScape(GetWorld(), NewMoon->RadiusKM, NewMoon);
	}

	if (DiameterOfLastMoon == 0)
	{
		HomePlanet->PlanetaryZone->SetSphereRadius(100);
		HomePlanet->AffectionRadiusKM = 100 * HomePlanet->GetActorScale3D().X / 100000;
	}
	else
	{
		FVector PlanetLocation = HomePlanet->GetActorLocation();
		FVector LastMoonOuterEdgeLocation = LastMoonLocation + FVector(0, DiameterOfLastMoon * 6371, 0);
		double SphereRadius = FVector::Dist(PlanetLocation, LastMoonOuterEdgeLocation);
		SphereRadius /= HomePlanet->GetActorScale3D().X;
		SphereRadius *= 1.5;
		HomePlanet->PlanetaryZone->SetSphereRadius(SphereRadius);
		HomePlanet->AffectionRadiusKM = SphereRadius * HomePlanet->GetActorScale3D().X / 100000;
	}

	HomePlanet->OrbitHeight = (HomePlanet->GravityCollisionZone->GetScaledSphereRadius() / 100000) -
		HomePlanet->RadiusKM;

	HomePlanet->PlanetaryEnvironmentGenerator->InitEnviroment(HomePlanet, GetWorld());
	if (GeneratedWorldModel
		&& IsValid(HomePlanet->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
	{
		// The integrated/authored-planet route must consume the same UI atmosphere
		// contract as a newly generated home planet. InitAtmoScape intentionally
		// supplies generic defaults, so apply the selected full-scale values last.
		AAtmoScape* Atmosphere = HomePlanet->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
		Atmosphere->bKeepRelativeScale = false;
		Atmosphere->LightSource = IsValid(HomePlanet->ParentStar)
			? HomePlanet->ParentStar : HomeStar;
		Atmosphere->PlanetRadius = FMath::Max(HomePlanet->RadiusKM - 1.0, 0.5);
		Atmosphere->AtmosphereHeight = GeneratedWorldModel->AtmosphereHeight;
		Atmosphere->AtmosphereOpacity = GeneratedWorldModel->AtmosphereOpacity;
		Atmosphere->MultiScatering = GeneratedWorldModel->AtmosphereMultiScattering;
		Atmosphere->RayleighHeight = GeneratedWorldModel->AtmosphereRayleighScattering;
		Atmosphere->RayleighScattering = GeneratedWorldModel->AtmosphereColor;
		Atmosphere->UpdateScale();
		Atmosphere->SetActorHiddenInGame(false);
	}
}

bool AAstroGenerator::ResolveSpawnLocation(const ASpaceship* NewHomeSpaceship, FVector& CharSpawnLocation)
{
	CharSpawnLocation = {0, 0, 0};
	const auto ResolveSurfaceLocation = [](const APlanetaryBody* Body, const FVector& PreferredOutward,
		FVector& OutLocation)
	{
		if (!IsValid(Body))
		{
			return false;
		}
		const FVector Outward = PreferredOutward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER,
			Body->GetActorUpVector());
		// Two metres keeps the capsule above the placeholder/full-scale surface;
		// custom gravity performs the final grounding once play resumes.
		OutLocation = Body->GetActorLocation() + Outward * (Body->GetWorldScapeBodyRadiusCm() + 200.0);
		return true;
	};
	const auto GetFirstHomeMoon = [this]() -> AMoon*
	{
		if (!IsValid(HomePlanet))
		{
			return nullptr;
		}
		for (AMoon* Moon : HomePlanet->Moons)
		{
			if (IsValid(Moon))
			{
				return Moon;
			}
		}
		return nullptr;
	};
	switch (CharSpawnPlace)
	{
	case ECharSpawnPlace::PlanetOrbit:
		if (IsValid(HomeSpaceStation) && IsValid(HomeSpaceStation->SpawnPoint))
		{
			CharSpawnLocation = HomeSpaceStation->SpawnPoint->GetComponentLocation();
			return true;
		}
		break;
	case ECharSpawnPlace::PlanetSurface:
		if (IsValid(HomePlanet))
		{
			const FVector PreferredOutward = IsValid(HomeSpaceHeadquarters)
				? HomeSpaceHeadquarters->GetActorLocation() - HomePlanet->GetActorLocation()
				: HomePlanet->GetActorUpVector();
			return ResolveSurfaceLocation(HomePlanet, PreferredOutward, CharSpawnLocation);
		}
		break;
	case ECharSpawnPlace::MoonOrbit:
		if (const AMoon* Moon = GetFirstHomeMoon())
		{
			const FVector Outward = (Moon->GetActorLocation() - HomePlanet->GetActorLocation())
				.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Moon->GetActorUpVector());
			const double OrbitClearanceCm = FMath::Max(200000.0, Moon->GetWorldScapeBodyRadiusCm() * 0.10);
			CharSpawnLocation = Moon->GetActorLocation()
				+ Outward * (Moon->GetWorldScapeBodyRadiusCm() + OrbitClearanceCm);
			return true;
		}
		break;
	case ECharSpawnPlace::MoonSurface:
		if (const AMoon* Moon = GetFirstHomeMoon())
		{
			const FVector Outward = Moon->GetActorLocation() - HomePlanet->GetActorLocation();
			return ResolveSurfaceLocation(Moon, Outward, CharSpawnLocation);
		}
		break;
	case ECharSpawnPlace::SpaceShip:
		if (IsValid(NewHomeSpaceship))
		{
			CharSpawnLocation = NewHomeSpaceship->GetActorLocation();
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

bool AAstroGenerator::SpawnStartInteractiveActors(TSharedPtr<FPlanetModel> StartPlanetModel)
{
	/*
	 * SpawnModel
	 * BP_HomeSpaceHeadquarters
	 * BP_HomeSpaceStation
	 * BP_HomeSpaceship
	 * BP_CharacterClass
	 */

	UWorld* World = GetWorld();
	if (!World || !StartPlanetModel.IsValid() || !IsValid(HomePlanet) || !PlanetGenerator)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization] Cannot spawn starter hierarchy: World=%s PlanetModel=%s HomePlanet=%s PlanetGenerator=%s"),
			World ? TEXT("OK") : TEXT("NULL"), StartPlanetModel.IsValid() ? TEXT("OK") : TEXT("NULL"),
			IsValid(HomePlanet) ? TEXT("OK") : TEXT("NULL"), PlanetGenerator ? TEXT("OK") : TEXT("NULL"));
		return false;
	}
	if (bStarterHierarchySpawned || IsValid(HomeSpaceHeadquarters) || IsValid(HomeSpaceStation)
		|| IsValid(HomeSpaceShipyard) || IsValid(HomeSpaceship))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization] Duplicate starter spawn rejected: committed=%s HQ=%s Station=%s Shipyard=%s Ship=%s"),
			bStarterHierarchySpawned ? TEXT("true") : TEXT("false"),
			*GetNameSafe(HomeSpaceHeadquarters), *GetNameSafe(HomeSpaceStation),
			*GetNameSafe(HomeSpaceShipyard), *GetNameSafe(HomeSpaceship));
		return false;
	}
	if (!BP_HomeSpaceHeadquarters || !BP_HomeSpaceStation || !BP_HomeSpaceShipyard
		|| !BP_HomeSpaceship || !BP_CharacterClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization] Starter class missing: HQ=%s Station=%s Shipyard=%s Ship=%s Character=%s"),
			BP_HomeSpaceHeadquarters ? TEXT("OK") : TEXT("NULL"),
			BP_HomeSpaceStation ? TEXT("OK") : TEXT("NULL"),
			BP_HomeSpaceShipyard ? TEXT("OK") : TEXT("NULL"), BP_HomeSpaceship ? TEXT("OK") : TEXT("NULL"),
			BP_CharacterClass ? TEXT("OK") : TEXT("NULL"));
		return false;
	}

	APawn* PreviousPlayerCharacter = UGameplayStatics::GetPlayerPawn(World, 0);
	const FTransform PreviousPlayerTransform = IsValid(PreviousPlayerCharacter)
		? PreviousPlayerCharacter->GetActorTransform() : FTransform::Identity;
	const FTransform GeneratorTransformBeforeStarterSpawn = GetActorTransform();
	APawn* SpawnedSelectedPlayerCharacter = nullptr;
	bool bStarterHierarchyCommitted = false;
	ON_SCOPE_EXIT
	{
		if (bStarterHierarchyCommitted)
		{
			return;
		}

		// Starter creation is transactional. A missing SpawnPoint, failed class
		// spawn or failed possession must not leave a half-built hierarchy that
		// blocks the next attempt as a duplicate.
		if (IsValid(SpawnedSelectedPlayerCharacter))
		{
			if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
				PlayerController && PlayerController->GetPawn() == SpawnedSelectedPlayerCharacter
				&& IsValid(PreviousPlayerCharacter))
			{
				PlayerController->Possess(PreviousPlayerCharacter);
			}
			SpawnedSelectedPlayerCharacter->Destroy();
		}
		// The selected GameMode pawn can already be the requested class. In that
		// common path it is reused and moved together with the generated hierarchy;
		// restore both transforms if any later attachment/possession validation fails.
		SetActorTransform(GeneratorTransformBeforeStarterSpawn, false, nullptr,
			ETeleportType::TeleportPhysics);
		if (IsValid(PreviousPlayerCharacter))
		{
			PreviousPlayerCharacter->SetActorTransform(PreviousPlayerTransform, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
		if (IsValid(HomeSpaceship)) HomeSpaceship->Destroy();
		if (IsValid(HomeSpaceShipyard)) HomeSpaceShipyard->Destroy();
		if (IsValid(HomeSpaceStation)) HomeSpaceStation->Destroy();
		if (IsValid(HomeSpaceHeadquarters)) HomeSpaceHeadquarters->Destroy();
		HomeSpaceship = nullptr;
		HomeSpaceShipyard = nullptr;
		HomeSpaceStation = nullptr;
		HomeSpaceHeadquarters = nullptr;
		UE_LOG(LogTemp, Error, TEXT("[APS.Civilization] Partial starter hierarchy rolled back"));
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = HomePlanet;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// CalculateOrbitHeight preserves the input unit. Use the generated body's
	// physical centimetre radius directly; the old 250000 cm "Earth radius"
	// constant placed the complete station hierarchy deep inside a full-scale world.
	const double PlanetRadiusCm = HomePlanet->GetWorldScapeBodyRadiusCm();
	const double StationOrbitRadiusCm = PlanetGenerator->CalculateOrbitHeight(
		HomeSpaceStationOrbitHeight, PlanetRadiusCm);
	FVector PlanetPosition = HomePlanet->GetActorLocation();

	HomeSpaceHeadquarters = World->SpawnActor<ASpaceHeadquarters>(
		BP_HomeSpaceHeadquarters, PlanetPosition, FRotator::ZeroRotator, SpawnParams);
	if (!HomeSpaceHeadquarters)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to spawn the selected home headquarters class"));
		return false;
	}
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMainGameplayInstance* GameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			HomeSpaceHeadquarters->Civilization = GameplayInstance->CurrentCivilization;
		}
	}
	HomeSpaceHeadquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
	HomeSpaceHeadquarters->SetActorRelativeRotation(FRotator(0, 0, 0));

	const double SpawnOffset = FMath::Max(StationOrbitRadiusCm, PlanetRadiusCm * 1.01);
	FVector Offset = FVector(0, SpawnOffset, 0);
	HomeSpaceHeadquarters->AddActorWorldOffset(Offset);

	FVector HomeSpaceHeadquartersLocation = HomeSpaceHeadquarters->GetActorLocation();
	FRotator HomeSpaceHeadquartersRotation = HomeSpaceHeadquarters->GetActorRotation();
	HomeSpaceStation = World->SpawnActor<ASpaceStation>(
		BP_HomeSpaceStation, HomeSpaceHeadquartersLocation,
		HomeSpaceHeadquartersRotation, SpawnParams);
	if (!IsValid(HomeSpaceStation))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Civilization] Unable to spawn the selected home station class"));
		return false;
	}
	HomeSpaceStation->AttachToActor(HomeSpaceHeadquarters,
	                                FAttachmentTransformRules::KeepWorldTransform);
	//double HomeStationOffset = HomeSpaceStation->GravityCollisionZone->GetScaledSphereRadius() * 2;
	double HomeStationOffset = 50000;
	HomeSpaceStation->AddActorLocalOffset(FVector(0, HomeStationOffset, 0));
	HomeSpaceStation->CalculateAffectionRadius();

	// Spawn HomeShipyard
	HomeSpaceShipyard = World->SpawnActor<ASpaceShipyard>(BP_HomeSpaceShipyard,
	                                                      HomeSpaceHeadquartersLocation,
	                                                      HomeSpaceHeadquartersRotation, SpawnParams);
	if (!IsValid(HomeSpaceShipyard))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Civilization] Unable to spawn the selected home shipyard class"));
		return false;
	}
	/*double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GravityCollisionZone->
	                                                            GetScaledSphereRadius() * 2;*/
	//double HomeSpaceShipyardLocationOffset = HomeSpaceShipyard->GetActorLocation();
	//(X=6000.000000,Y=-15360.000001,Z=-500.000000)
	HomeSpaceShipyard->AddActorLocalOffset(FVector(6000, -15360, -500));
	HomeSpaceShipyard->AttachToActor(HomeSpaceHeadquarters,
	                                 FAttachmentTransformRules::KeepWorldTransform);
	HomeSpaceShipyard->CalculateAffectionRadius();

	//Spawn HomeSpaceship
	FActorSpawnParameters SpaceshipSpawnParams;
	SpaceshipSpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (!IsValid(HomeSpaceShipyard->SpawnPoint))
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.Civilization] Home shipyard has no valid SpawnPoint"));
		return false;
	}
	FVector HomeSpaceshipLocation = HomeSpaceShipyard->SpawnPoint->GetComponentLocation();

	HomeSpaceshipLocation.Z += 1000;
	ASpaceship* NewHomeSpaceship = World->SpawnActor<ASpaceship>(
		BP_HomeSpaceship, HomeSpaceshipLocation, HomeSpaceShipyard->GetActorRotation(),
		SpaceshipSpawnParams);
	HomeSpaceship = NewHomeSpaceship;

	if (!IsValid(NewHomeSpaceship) || !IsValid(GeneratedHomeStarSystem))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.Civilization] Cannot complete starter hierarchy: Ship=%s HomeSystem=%s"),
			IsValid(NewHomeSpaceship) ? TEXT("OK") : TEXT("NULL"),
			IsValid(GeneratedHomeStarSystem) ? TEXT("OK") : TEXT("NULL"));
		return false;
	}

	if (NewHomeSpaceship && GeneratedHomeStarSystem)
	{
		NewHomeSpaceship->AttachToActor(HomeSpaceShipyard,
		                                FAttachmentTransformRules::KeepWorldTransform);
		NewHomeSpaceship->OffsetSystem = GeneratedHomeStarSystem;

		if (NewHomeSpaceship->OnboardComputer)
		{
			NewHomeSpaceship->OnboardComputer->OffsetSystem = GeneratedHomeStarSystem;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Onboard Computer is nullptr!"));
		}
	}
	APawn* PlayerCharacter = PreviousPlayerCharacter;
	const bool bExistingPawnMatchesSelection = IsValid(PlayerCharacter)
		&& BP_CharacterClass
		&& PlayerCharacter->IsA(BP_CharacterClass);
	if (!bExistingPawnMatchesSelection && BP_CharacterClass)
	{
		const FVector InitialSpawnLocation = HomeSpaceHeadquarters->GetStartPointPosition();
		APawn* SelectedPlayerCharacter = World->SpawnActor<APawn>(BP_CharacterClass, InitialSpawnLocation,
			HomeSpaceShipyard->GetActorRotation(), SpawnParams);
		if (IsValid(SelectedPlayerCharacter))
		{
			SpawnedSelectedPlayerCharacter = SelectedPlayerCharacter;
			PlayerCharacter = SelectedPlayerCharacter;
			if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
			{
				PlayerController->Possess(SelectedPlayerCharacter);
			}
		}
	}
	if (IsValid(PlayerCharacter))
	{
		// Relocate spawned world to 000
		FVector PlayerLocation = HomeSpaceHeadquarters->GetActorLocation();
		FVector GeneratorLocation = this->GetActorLocation();
		FVector NewGeneratorLocation = GeneratorLocation - PlayerLocation;
		this->SetActorLocation(NewGeneratorLocation, false);

		// Resolve after relocating the generated hierarchy. Resolving before the
		// move produced a stale position and the old code then overwrote every
		// START LOCATION choice with the Headquarters start point.
		FVector SpawnLocation;
		if (!ResolveSpawnLocation(NewHomeSpaceship, SpawnLocation))
		{
			SpawnLocation = HomeSpaceHeadquarters->GetStartPointPosition();
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Civilization] Start location %d has no runtime anchor; using Headquarters start point"),
				static_cast<int32>(CharSpawnPlace));
		}
		const bool bTeleportSuccess = PlayerCharacter->SetActorLocation(SpawnLocation, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.Civilization] Character spawn location=%s success=%s"),
			*SpawnLocation.ToString(), bTeleportSuccess ? TEXT("true") : TEXT("false"));
		// Compose a readable first gameplay frame from the real generated hierarchy.
		// The shipyard rotation is an asset-local construction rotation; using it for
		// the pawn made the third-person camera look tangentially into empty space.
		// In orbit, looking through the station towards the home planet keeps the
		// station foreground, WorldScape terrain and parent-star illumination in one
		// physically coherent view. Surface starts retain a tangential forward vector
		// so they never point the camera into the ground.
		FVector CameraUp = HomeSpaceStation->GetActorUpVector().GetSafeNormal(
			UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
		FVector InitialViewDirection = HomePlanet->GetActorLocation() - SpawnLocation;
		if (CharSpawnPlace == ECharSpawnPlace::PlanetSurface)
		{
			CameraUp = (SpawnLocation - HomePlanet->GetActorLocation()).GetSafeNormal(
				UE_DOUBLE_SMALL_NUMBER, HomePlanet->GetActorUpVector());
			InitialViewDirection = HomeSpaceHeadquarters->GetActorLocation() - SpawnLocation;
			InitialViewDirection = FVector::VectorPlaneProject(InitialViewDirection, CameraUp);
		}
		else if (CharSpawnPlace == ECharSpawnPlace::MoonSurface)
		{
			const AMoon* FirstHomeMoon = nullptr;
			for (const AMoon* CandidateMoon : HomePlanet->Moons)
			{
				if (IsValid(CandidateMoon))
				{
					FirstHomeMoon = CandidateMoon;
					break;
				}
			}
			if (const AMoon* Moon = FirstHomeMoon)
			{
				CameraUp = (SpawnLocation - Moon->GetActorLocation()).GetSafeNormal(
					UE_DOUBLE_SMALL_NUMBER, Moon->GetActorUpVector());
				InitialViewDirection = FVector::VectorPlaneProject(
					HomePlanet->GetActorLocation() - SpawnLocation, CameraUp);
			}
		}
		else if (CharSpawnPlace == ECharSpawnPlace::SpaceShip && IsValid(NewHomeSpaceship))
		{
			CameraUp = NewHomeSpaceship->GetActorUpVector().GetSafeNormal(
				UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
			InitialViewDirection = NewHomeSpaceship->GetActorForwardVector();
		}
		if (InitialViewDirection.IsNearlyZero())
		{
			InitialViewDirection = HomeSpaceStation->GetActorForwardVector();
		}
		const FRotator InitialViewRotation = FRotationMatrix::MakeFromXZ(
			InitialViewDirection.GetSafeNormal(), CameraUp).Rotator();
		PlayerCharacter->SetActorRotation(InitialViewRotation, ETeleportType::TeleportPhysics);
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			// Selected pawn Blueprints may opt into control-rotation on their spring
			// arm even though the native pawn does not. Set both contracts so the
			// committed class opens on the same generated subject either way.
			PlayerController->SetControlRotation(InitialViewRotation);
			PlayerController->SetViewTarget(PlayerCharacter);
		}
		UE_LOG(LogTemp, Log,
			TEXT("[APS.Civilization] Initial gameplay view direction=%s up=%s rotation=%s targetPlanet=%s"),
			*InitialViewDirection.GetSafeNormal().ToCompactString(), *CameraUp.ToCompactString(),
			*InitialViewRotation.ToCompactString(), *GetNameSafe(HomePlanet));
		const UMainGameplayInstance* GameplayInstance = World->GetGameInstance()
			? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
		const bool bSelectedCharacterPossessed =
			UGameplayStatics::GetPlayerPawn(World, 0) == PlayerCharacter;
		const bool bHierarchyValid =
			HomeSpaceHeadquarters->GetAttachParentActor() == HomePlanet
			&& HomeSpaceStation->GetAttachParentActor() == HomeSpaceHeadquarters
			&& HomeSpaceShipyard->GetAttachParentActor() == HomeSpaceHeadquarters
			&& HomeSpaceship->GetAttachParentActor() == HomeSpaceShipyard
			&& HomeSpaceHeadquarters->IsA(BP_HomeSpaceHeadquarters)
			&& HomeSpaceStation->IsA(BP_HomeSpaceStation)
			&& HomeSpaceShipyard->IsA(BP_HomeSpaceShipyard)
			&& HomeSpaceship->IsA(BP_HomeSpaceship)
			&& PlayerCharacter->IsA(BP_CharacterClass)
			&& bTeleportSuccess
			&& bSelectedCharacterPossessed
			&& (!GameplayInstance || HomeSpaceHeadquarters->Civilization == GameplayInstance->CurrentCivilization);
		if (!bHierarchyValid)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Civilization] Starter hierarchy validation failed: HQParent=%s StationParent=%s ShipyardParent=%s ShipParent=%s CharacterClass=%s Possessed=%s Civilization=%s"),
				*GetNameSafe(HomeSpaceHeadquarters->GetAttachParentActor()),
				*GetNameSafe(HomeSpaceStation->GetAttachParentActor()),
				*GetNameSafe(HomeSpaceShipyard->GetAttachParentActor()),
				*GetNameSafe(HomeSpaceship->GetAttachParentActor()),
				PlayerCharacter->IsA(BP_CharacterClass) ? TEXT("OK") : TEXT("MISMATCH"),
				bSelectedCharacterPossessed ? TEXT("OK") : TEXT("NO"),
				(!GameplayInstance || HomeSpaceHeadquarters->Civilization == GameplayInstance->CurrentCivilization)
					? TEXT("OK") : TEXT("MISMATCH"));
		}
		else
		{
			bStarterHierarchyCommitted = true;
			bStarterHierarchySpawned = true;
			if (IsValid(PreviousPlayerCharacter) && PreviousPlayerCharacter != PlayerCharacter)
			{
				PreviousPlayerCharacter->Destroy();
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Civilization] Starter hierarchy ready exactly once: Planet=%s HQ=%s Station=%s Shipyard=%s Ship=%s Character=%s"),
				*GetNameSafe(HomePlanet), *GetNameSafe(HomeSpaceHeadquarters), *GetNameSafe(HomeSpaceStation),
				*GetNameSafe(HomeSpaceShipyard), *GetNameSafe(HomeSpaceship), *GetNameSafe(PlayerCharacter));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter is null!"));
	}
	return bStarterHierarchyCommitted;
}

void AAstroGenerator::SetMoonRotation(APlanetOrbit* NewMoonOrbit)
{
	NewMoonOrbit->SetActorRelativeRotation(FRotator(FMath::RandRange(-360.0, 360.0),
	                                                FMath::RandRange(-360.0, 360.0),
	                                                FMath::RandRange(-360.0, 360.0)));
}

bool AAstroGenerator::CheckGeneratorsFails()
{
	if (StarGenerator == nullptr || PlanetGenerator == nullptr || MoonGenerator == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the generators is null!"));
		return true;
	}

	if (!BP_PlanetarySystemClass || !BP_StarSystemClass || !BP_StarClass || !BP_PlanetClass || !BP_MoonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("One of the blueprint classes is not assigned in the editor!"));
		return true;
	}
	return false;
}

FVector AAstroGenerator::GetHomeSystemSpawnLocationForGalaxy(TArray<AActor*> AttachedActors, int32 RandomIndex)
{
	FVector HomeSystemSpawnLocation{0};
	if (AGalaxy* GalaxyActor = Cast<AGalaxy>(AttachedActors[RandomIndex]))
	{
		if (UHierarchicalInstancedStaticMeshComponent* HismComponent = GalaxyActor->StarMeshInstances; HismComponent &&
			HismComponent->GetInstanceCount() > 0)
		{
			int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
			FTransform InstanceTransform;
			HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
			HomeSystemSpawnLocation = InstanceTransform.GetLocation();
		}
	}
	return HomeSystemSpawnLocation;
}

FVector AAstroGenerator::GetHomeSystemSpawnLocationForStarCluster(TArray<AActor*> AttachedActors, int32 RandomIndex)
{
	FVector HomeSystemSpawnLocation{0};
	if (AStarCluster* StarClusterActor = Cast<AStarCluster>(AttachedActors[RandomIndex]))
	{
		if (UHierarchicalInstancedStaticMeshComponent* HismComponent = StarClusterActor->StarMeshInstances;
			HismComponent && HismComponent->GetInstanceCount() > 0)
		{
			int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
			FTransform InstanceTransform;
			HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
			HomeSystemSpawnLocation = InstanceTransform.GetLocation();
		}
	}
	return HomeSystemSpawnLocation;
}

FVector AAstroGenerator::DetermineHomeSystemSpawnLocation()
{
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);

	std::unordered_map<EHomeSystemPosition, std::function<FVector()>> HomeSystemPositionMap = {
		{EHomeSystemPosition::WorldCenter, []() { return FVector(0, 0, 0); }},
		{
			EHomeSystemPosition::RandomPosition, []()
			{
				double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
				double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
				return FVector(RandomX, RandomY, RandomZ);
			}
		},
		{
			EHomeSystemPosition::DirectPosition, [this, AttachedActors]()
			{
				if (AttachedActors.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, AttachedActors.Num() - 1);
					std::unordered_map<EAstroGenerationLevel, std::function<FVector()>> GenerationLevelMap = {
						{
							EAstroGenerationLevel::Galaxy,
							[this, AttachedActors, RandomIndex]()
							{
								return GetHomeSystemSpawnLocationForGalaxy(AttachedActors, RandomIndex);
							}
						},
						{
							EAstroGenerationLevel::StarCluster,
							[this, AttachedActors, RandomIndex]()
							{
								return GetHomeSystemSpawnLocationForStarCluster(AttachedActors, RandomIndex);
							}
						}
					};

					return (GenerationLevelMap.find(AstroGenerationLevel) != GenerationLevelMap.end())
						       ? GenerationLevelMap[AstroGenerationLevel]()
						       : FVector(0, 0, 0);
				}
				return FVector(0, 0, 0);
			}
		}
	};

	return (HomeSystemPositionMap.find(HomeSystemPosition) != HomeSystemPositionMap.end())
		       ? HomeSystemPositionMap[HomeSystemPosition]()
		       : FVector(0, 0, 0);
}

void AAstroGenerator::GenerateStarSystem(AStarSystem* NewStarSystem, TSharedPtr<FStarSystemModel> StarSystemModel)
{
	UWorld* World = GetWorld();

	std::unordered_map<EStarType, std::function<void()>> StarTypeMap = {
		{EStarType::SingleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 1; }},
		{EStarType::DoubleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 2; }},
		{EStarType::TripleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = 3; }},
		{EStarType::MultipleStar, [&StarSystemModel]() { StarSystemModel->AmountOfStars = FMath::RandRange(4, 6); }}
	};

	/*StarTypeMap.find(HomeSystemStarType) != StarTypeMap.end()
		? StarTypeMap[HomeSystemStarType]()
		: StarSystemModel->AmountOfStars = 1;*/

	if (auto it = StarTypeMap.find(HomeSystemStarType); it != StarTypeMap.end())
	{
		it->second(); // вызываем колбэк
	}
	else
	{
		StarSystemModel->AmountOfStars = 1; // дефолт
	}
	

	//FVector LastStarLocation{0};
	for (int StarNumber = 0; StarNumber < StarSystemModel->AmountOfStars; StarNumber++)
	{
		const TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();

		if (bRandomHomeStar)
		{
			StarGenerator->GenerateRandomStarModel(StarModel);
		}
		else
		{
			StarModel->StellarType = HomeStarStellarType;
			StarModel->SpectralClass = HomeStarSpectralClass;
			StarGenerator->GenerateStarModel(StarModel);
		}

		PlanetarySystemModel = MakeShared<FPlanetarySystemModel>();

		if (bRandomHomeSystemType)
		{
			PlanetarySystemGenerator->GeneratePlanetarySystemModelByStar(
				PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
		}
		else
		{
			PlanetarySystemModel->AmountOfPlanets = PlanetsAmount;
			PlanetarySystemModel->PlanetarySystemType = HomeSystemPlanetaryType;
			PlanetarySystemModel->OrbitDistributionType = HomeSystemOrbitDistributionType;
			PlanetarySystemGenerator->GenerateCustomPlanetarySystemModel(
				PlanetarySystemModel, StarModel, PlanetGenerator, MoonGenerator);
		}

		const int32 RetainedModelEdits = ApplyPreviewBodyEditOverridesToModels(
			GeneratedWorldModel, StarNumber, *PlanetarySystemModel);
		if (RetainedModelEdits > 0)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[APS.PreviewBodyEdit] Applied %d retained edits to star %d models before spawn"),
				RetainedModelEdits, StarNumber);
		}

		AStar* NewStar = World->SpawnActor<AStar>(BP_StarClass);
		APlanetarySystem* NewPlanetarySystem = World->SpawnActor<APlanetarySystem>(BP_PlanetarySystemClass);

		if (StarNumber == 0)
		{
			NewStarSystem->MainStar = NewStar;
		}

		if (!NewStar || !NewPlanetarySystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("Star Falied!"));
			return;
		}

		StarGenerator->ApplyModel(NewStar, StarModel);
		PlanetarySystemGenerator->ApplyModel(NewPlanetarySystem, PlanetarySystemModel);
		const FVector SystemCenter = NewStarSystem->GetActorLocation();
		NewStar->SetActorLocation(SystemCenter);
		NewPlanetarySystem->SetActorLocation(SystemCenter);
		NewStar->SetActorScale3D(FVector(StarModel->Radius * 813684224.0));
		NewStar->StarRadiusKM = StarModel->Radius * 696340;
		NewStar->SetPlanetarySystem(NewPlanetarySystem);
		NewPlanetarySystem->SetStar(NewStar);
		NewStarSystem->AddNewStar(NewStar);
		NewStar->AttachToActor(NewStarSystem, FAttachmentTransformRules::KeepWorldTransform);
		NewPlanetarySystem->AttachToActor(NewStar, FAttachmentTransformRules::KeepWorldTransform);
		NewStar->FullSpectralName = NewStar->GenerateFullSpectralName();
		NewPlanetarySystem->SetStarFullSpectralName(NewStar->FullSpectralName);
		const FString SpectralIdentity = NewStar->FullSpectralName.IsNone()
			? TEXT("Star") : NewStar->FullSpectralName.ToString();
		NewStar->AstroName = AGravityPlayerController::GenerateUniqueName(SpectralIdentity);

		StarGenerator->ApplySpectralMaterial(NewStar, StarModel);

		//FVector LastPlanetLocation{0};
		for (const TSharedPtr<FPlanetData>& PlanetData : PlanetarySystemModel->PlanetsList)
		{
			APlanetOrbit* NewPlanetOrbit = World->SpawnActor<APlanetOrbit>(
				BP_PlanetOrbitClass, NewPlanetarySystem->GetActorLocation(), FRotator::ZeroRotator);
			NewPlanetOrbit->AttachToActor(NewPlanetarySystem, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->PlanetOrbitsList.Add(NewPlanetOrbit);

			TSharedPtr<FPlanetModel> PlanetModel = PlanetData->PlanetModel;
			APlanet* NewPlanet = World->SpawnActor<APlanet>(BP_PlanetClass);

			PlanetGenerator->ApplyModel(NewPlanet, PlanetModel);
			NewPlanet->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Planet"));
			NewStar->AddPlanet(NewPlanet);
			NewPlanet->SetParentStar(NewStar);

			NewPlanet->SetActorScale3D(FVector(PlanetModel->Radius * 12742000));
			const FVector OrbitOffset(PlanetModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
			const FVector NewLocation = NewPlanetOrbit->GetActorLocation()
				+ NewPlanetOrbit->GetActorQuat().RotateVector(OrbitOffset);
			NewPlanet->PlanetRadiusKM = PlanetModel->Radius * 6371;
			NewPlanet->SetActorLocation(NewLocation);

			NewPlanet->AttachToActor(NewPlanetOrbit, FAttachmentTransformRules::KeepWorldTransform);
			NewPlanetarySystem->PlanetsActorsList.Add(NewPlanet);

			NewPlanetOrbit->Planet = NewPlanet;

			/*const double KM_TO_UE_UNIT_SCALE = 100000;
			double DiameterOfLastMoon = 0;
			FVector LastMoonLocation;*/

			for (const TSharedPtr<FMoonData>& MoonData : PlanetModel->MoonsList)
			{
				APlanetOrbit* NewMoonOrbit = World->SpawnActor<APlanetOrbit>(
					BP_PlanetOrbitClass, NewPlanet->GetActorLocation(), FRotator::ZeroRotator);
				NewMoonOrbit->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepWorldTransform);
				NewPlanet->MoonOrbitsList.Add(NewMoonOrbit);

				FVector MoonLocation = NewPlanet->GetActorLocation();
				AMoon* NewMoon = World->SpawnActor<AMoon>(BP_MoonClass, MoonLocation, FRotator::ZeroRotator);
				NewMoon->AstroName = AGravityPlayerController::GenerateUniqueName(TEXT("Moon"));
				NewPlanet->AddMoon(NewMoon);
				NewMoon->SetParentPlanet(NewPlanet);

				MoonGenerator->ApplyModel(NewMoon, MoonData->MoonModel);
				MoonGenerator->ConnectMoonWithPlanet(NewMoon, NewPlanet);

				NewMoon->SetActorScale3D(FVector(MoonData->MoonModel->Radius * 12742000));
				const FVector MoonOrbitOffset(
					MoonData->MoonModel->OrbitDistance * 149600000000000 / 1000, 0, 0);
				NewMoon->SetActorLocation(NewMoonOrbit->GetActorLocation()
					+ NewMoonOrbit->GetActorQuat().RotateVector(MoonOrbitOffset));

				NewMoon->AttachToActor(NewMoonOrbit, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}

void AAstroGenerator::SynchronizeHomeClusterRecord(
	FClusterStarSystemRecord& Record, const FStarModel& PrimaryStar,
	const FStarSystemModel& MaterializedSystem)
{
	// StableId, instance address and ClusterLocalLocation identify the already
	// rendered HISM point.  The actor model carries a world-space location, so copy
	// all generated data first and then restore those catalogue invariants.
	const FGuid StableId = Record.StableId;
	const int32 InstanceIndex = Record.InstanceIndex;
	const FVector ClusterLocalLocation = Record.ClusterLocalLocation;

	Record.PrimaryStarModel = PrimaryStar;
	Record.PrimaryStarModel.Location = ClusterLocalLocation;
	Record.SystemModel = MaterializedSystem;
	Record.SystemModel.StableId = StableId;
	Record.SystemModel.Location = ClusterLocalLocation;
	Record.StableId = StableId;
	Record.InstanceIndex = InstanceIndex;
	Record.ClusterLocalLocation = ClusterLocalLocation;
}

void AAstroGenerator::RotatePlanetOrbits(APlanetarySystem* NewPlanetarySystem)
{
	for (APlanetOrbit* PlanetOrbit : NewPlanetarySystem->PlanetOrbitsList)
	{
		const float RandomZRotation = FMath::RandRange(-360.0f, 360.0f);
		const float RandomYRotation = FMath::RandRange(-15.0f, 15.0f);
		const FRotator NewRotation = FRotator(RandomYRotation, RandomZRotation, 0);
		PlanetOrbit->AddActorLocalRotation(NewRotation);

		TArray<AActor*> AttachedActors;
		PlanetOrbit->GetAttachedActors(AttachedActors);

		for (AActor* AttachedActor : AttachedActors)
		{
			if (APlanet* Planet = Cast<APlanet>(AttachedActor))
			{
				// Iterate through the list of moons for this planet.
				for (APlanetOrbit* MoonOrbit : Planet->MoonOrbitsList)
				{
					const float RandomXRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const float RandomYRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const float RandomZRotationMoon = FMath::RandRange(-360.0f, 360.0f);
					const FRotator NewRotationMoon = FRotator(RandomXRotationMoon, RandomYRotationMoon,
					                                          RandomZRotationMoon);
					MoonOrbit->AddActorLocalRotation(NewRotationMoon);
				}
			}
		}
	}
}

void AAstroGenerator::ComputeHomeSystemPosition(FTransform& HomeSystemTransform, FVector& HomeSystemSpawnLocation)
{
	PendingHomeCluster.Reset();
	PendingHomeClusterInstanceIndex = INDEX_NONE;
	HomeSystemSpawnLocation = {0, 0, 0};
	switch (HomeSystemPosition)
	{
	case EHomeSystemPosition::WorldCenter:
		HomeSystemSpawnLocation = FVector(0, 0, 0);
		break;
	case EHomeSystemPosition::RandomPosition:
		{
			// Set HomeSystemSpawnLocation depending on your definition of center, middle, and end
			const double RandomX = FMath::RandRange(-1000000000000, 1000000000000);
			const double RandomY = FMath::RandRange(-1000000000000, 1000000000000);
			const double RandomZ = FMath::RandRange(-1000000000000, 1000000000000);
			HomeSystemSpawnLocation = FVector(RandomX, RandomY, RandomZ);
		}
		break;
	case EHomeSystemPosition::DirectPosition:
		{
			// Prefer the selected system record inside the generated cluster. The old
			// enum-based branch selected a raw galaxy point for Galaxy mode even when a
			// real cluster existed, breaking Galaxy -> Cluster -> System continuity.
			AStarCluster* StarClusterActor = GeneratedStarCluster;
			if (IsValid(StarClusterActor))
			{
				if (const UHierarchicalInstancedStaticMeshComponent* HismComponent =
					StarClusterActor->StarMeshInstances;
					HismComponent && HismComponent->GetInstanceCount() > 0)
				{
					const int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
					if (const FClusterStarSystemRecord* Record =
						StarClusterActor->FindPotentialSystem(RandomInstanceIndex))
					{
						HomeSystemSpawnLocation = StarClusterActor->GetPotentialSystemWorldLocation(*Record);
						PendingHomeCluster = StarClusterActor;
						PendingHomeClusterInstanceIndex = RandomInstanceIndex;
					}
					else
					{
						FTransform InstanceTransform;
						HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true);
						HomeSystemSpawnLocation = InstanceTransform.GetLocation();
					}
				}
			}
			else if (IsValid(GeneratedGalaxy) && IsValid(GeneratedGalaxy->StarMeshInstances))
			{
				UHierarchicalInstancedStaticMeshComponent* HismComponent = GeneratedGalaxy->StarMeshInstances;
				if (HismComponent->GetInstanceCount() > 0)
				{
					const int32 RandomInstanceIndex = FMath::RandRange(0, HismComponent->GetInstanceCount() - 1);
					FTransform InstanceTransform;
					if (HismComponent->GetInstanceTransform(RandomInstanceIndex, InstanceTransform, true))
					{
						HomeSystemSpawnLocation = InstanceTransform.GetLocation();
					}
				}
			}
		}
		break;
	default:
		break;
	}
	HomeSystemTransform.SetLocation(HomeSystemSpawnLocation);
	HomeSystemTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
}

void AAstroGenerator::ComputeStarAmount(TSharedPtr<FStarSystemModel>& StarSystemModel, int& AmountOfStars)
{
	StarSystemModel = MakeShared<FStarSystemModel>();
	AmountOfStars = {0};
	switch (HomeSystemStarType)
	{
	case EStarType::SingleStar:
		AmountOfStars = 1;
		break;
	case EStarType::DoubleStar:
		AmountOfStars = 2;
		break;
	case EStarType::TripleStar:
		AmountOfStars = 3;
		break;
	case EStarType::MultipleStar:
		AmountOfStars = FMath::RandRange(4, 6);
		break;
	default:
		AmountOfStars = 1;
		break;
	}
	StarSystemModel->AmountOfStars = AmountOfStars;
}

TMap<EStarClusterType, TPair<int, int>> ClusterStarAmount =
{
	{EStarClusterType::OpenCluster, {500, 5000}},
	{EStarClusterType::GlobularCluster, {5000, 25000}},
	{EStarClusterType::Supercluster, {25000, 50000}},
	{EStarClusterType::Nebula, {10000, 20000}},
	{EStarClusterType::ElongatedStream, {3000, 15000}},
	{EStarClusterType::RingArc, {2000, 12000}},
	{EStarClusterType::Hourglass, {5000, 20000}},
	{EStarClusterType::Unknown, {0, 0}}
};

int AAstroGenerator::GetRandomValueFromStarAmountRange(const EStarClusterType ClusterType)
{
	if (ClusterStarAmount.Contains(ClusterType))
	{
		const TPair<int, int> Range = ClusterStarAmount[ClusterType];
		return FMath::RandRange(Range.Key, Range.Value);
	}
	UE_LOG(LogTemp, Warning, TEXT("Cluster type not found in map, or range is invalid"));
	return 0;
}

void AAstroGenerator::Test_GenerateFullscaled()
{
	// ������ ��������� �����
	const double MinStarRadius = 0.1f;
	const double MaxStarRadius = 10.0f;
	const int StarCount = 100;

	// ������� �����-����� ��� ������ ����
	AAstroAnchor* AstroAnchor = GetWorld()->SpawnActor<AAstroAnchor>(BP_AstroAnchorClass);

	// ������� ����� �������� ����� � ����������� ��� � �����
	AStarCluster* StarCluster = GetWorld()->SpawnActor<AStarCluster>(BP_StarClusterClass);
	StarCluster->AttachToActor(AstroAnchor, FAttachmentTransformRules::KeepRelativeTransform);

	// ������� ������� �����
	for (int i = 0; i < StarCount; ++i)
	{
		// ���������� ��������� ���������� � ������ ��� ������
		FVector StarPosition = FMath::VRand() * FMath::FRand() * 5.0f;
		double StarRadius = FMath::RandRange(MinStarRadius, MaxStarRadius);

		// ������� �������������� ��� ������
		FTransform StarTransform;
		StarTransform.SetLocation(StarPosition * 10000);
		StarTransform.SetScale3D(FVector(StarRadius));

		// ��������� ������� ������ � HISM
		StarCluster->StarMeshInstances->AddInstance(StarTransform);
	}

	// ���������� �����-����� � ����� �������� � ������ ����������
	GeneratedWorld = AstroAnchor;
	GeneratedStarCluster = StarCluster;
}

void AAstroGenerator::GeneratePlanetSystem()
{
	GenerateStarSystemByModel();
}

void AAstroGenerator::GenerateSinglePlanet()
{
	const int32 SavedPlanetsAmount = PlanetsAmount;
	const int32 SavedStartPlanetNumber = StartPlanetNumber;
	PlanetsAmount = 1;
	StartPlanetNumber = 1;
	GenerateStarSystemByModel();
	PlanetsAmount = SavedPlanetsAmount;
	StartPlanetNumber = SavedStartPlanetNumber;
}

void AAstroGenerator::GenerateRandomWorld()
{
}

void AAstroGenerator::GenerateGalaxiesCluster()
{
	// GalaxiesCluster used to be an empty legacy branch. Treat it as the same
	// navigable parent hierarchy as the modern Galaxy/StarCluster preview so old
	// save/config values cannot produce a completely empty astronomical scene.
	GenerateGalaxy();
	GenerateStarCluster();
}

void AAstroGenerator::IntegrateStartPlanetIntoSystem()
{
	UE_LOG(LogTemp, Warning, TEXT("=== IntegrateStartPlanetIntoSystem START ==="));
	
	if (!WSR_StartHomePlanet || !HomePlanet || !GeneratedHomeStarSystem || !BP_Headquarters)
	{
		UE_LOG(LogTemp, Error, TEXT("IntegrateStartPlanetIntoSystem: Required references are null!"));
		UE_LOG(LogTemp, Error, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("HomePlanet: %s"), HomePlanet ? TEXT("Valid") : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("GeneratedHomeStarSystem: %s"), GeneratedHomeStarSystem ? TEXT("Valid") : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("BP_Headquarters: %s"), BP_Headquarters ? TEXT("Valid") : TEXT("NULL"));
		return;
	}

	if (!GeneratedHomeStarSystem->MainStar
		|| !GeneratedHomeStarSystem->MainStar->PlanetarySystem
		|| !GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.IsValidIndex(StartPlanetNumber - 1))
	{
		UE_LOG(LogTemp, Error,
			TEXT("IntegrateStartPlanetIntoSystem: StartPlanetNumber %d has no valid generated orbit"),
			StartPlanetNumber);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), *WSR_StartHomePlanet->GetName());
	UE_LOG(LogTemp, Warning, TEXT("BP_Headquarters: %s"), *BP_Headquarters->GetName());
	UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem: %s"), *GeneratedHomeStarSystem->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Using HomePlanet: %s"), *HomePlanet->GetName());

	// Single Play integrates an editor-authored start world. Its WorldScapeRoot is
	// already part of the level and must not be treated as another procedural body.
	// Otherwise the runtime streamer creates a second terrain surface around the
	// generated hierarchy planet as soon as play begins. Random-generation mode
	// does not enter this integration path and keeps normal nearest-body streaming.
	HomePlanet->bStreamWorldScapeSurface = false;
	HomePlanet->bGenerateByDefault = false;
	
	// Находим нужную орбиту и перемещаем домашнюю планету (иерархия от станции)
	APlanetOrbit* TargetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList[StartPlanetNumber - 1];
	const FVector& TargetPlanetPosition = TargetOrbit->Planet->GetActorLocation();
	BP_Headquarters->SetActorLocation(TargetPlanetPosition, false, nullptr, ETeleportType::ResetPhysics);

	// Прикрепляем систему к актору планеты
	GeneratedHomeStarSystem->AttachToActor(BP_Headquarters, FAttachmentTransformRules::KeepWorldTransform);
	BP_Headquarters->SetActorLocation(FVector(0, 0, 0));
	
	// Возвращаем иерархию к стандарту
	GeneratedHomeStarSystem->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	BP_Headquarters->AttachToActor(HomePlanet, FAttachmentTransformRules::KeepWorldTransform);
	HomePlanet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepWorldTransform);

	// Удаляем дефолтные акторы планеты
	DestroyActorTree(TargetOrbit->Planet);

	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Pawn->SetActorLocation(BP_Headquarters->GetStartPointPosition());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("=== IntegrateStartPlanetIntoSystem END ==="));
}

void AAstroGenerator::GenerateStarSystemAndIntegratePlanet()
{
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemAndIntegratePlanet START ==="));
	UE_LOG(LogTemp, Warning, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("StartPlanetNumber: %d"), StartPlanetNumber);
	
	// Check if we should integrate StartPlanet
	if (!bIntegrateStartPlanet || !WSR_StartHomePlanet)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateStarSystemAndIntegratePlanet: bIntegrateStartPlanet is false or StartHomePlanet is null!"));
		UE_LOG(LogTemp, Error, TEXT("bIntegrateStartPlanet: %s"), bIntegrateStartPlanet ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("StartHomePlanet: %s"), WSR_StartHomePlanet ? TEXT("Valid") : TEXT("NULL"));
		return;
	}
	
	// Generate the star system first
	UE_LOG(LogTemp, Warning, TEXT("Generating star system..."));
	APlanet* const AuthoredHomePlanet = HomePlanet;
	GenerateStarSystemByModel();
	// The SinglePlay map serializes its authored home-planet actor. Preview and
	// procedural generation are allowed to select a generated HomePlanet, but
	// this integration path must retain the serialized actor unconditionally.
	HomePlanet = AuthoredHomePlanet;
	UE_LOG(LogTemp, Warning, TEXT("Restored authored HomePlanet after generation: %s"),
		*GetNameSafe(HomePlanet));

	// Check if star system was generated successfully
	if (!GeneratedHomeStarSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateStarSystemAndIntegratePlanet: Failed to generate star system!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Star system generated successfully!"));
	UE_LOG(LogTemp, Warning, TEXT("GeneratedHomeStarSystem: %s"), *GeneratedHomeStarSystem->GetName());
	UE_LOG(LogTemp, Warning, TEXT("MainStar: %s"), GeneratedHomeStarSystem->MainStar ? TEXT("Valid") : TEXT("NULL"));
	
	if (GeneratedHomeStarSystem->MainStar && GeneratedHomeStarSystem->MainStar->PlanetarySystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlanetarySystem: %s"), *GeneratedHomeStarSystem->MainStar->PlanetarySystem->GetName());
		UE_LOG(LogTemp, Warning, TEXT("Available orbits: %d"), GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num());
	}
	
	// Now integrate our planet
	UE_LOG(LogTemp, Warning, TEXT("Integrating StartPlanet into generated star system"));
	IntegrateStartPlanetIntoSystem();
	
	UE_LOG(LogTemp, Warning, TEXT("=== GenerateStarSystemAndIntegratePlanet END ==="));
}

void AAstroGenerator::SetAutoStarSystemModel()
{
}

void AAstroGenerator::GenerateRandomStarSystemModel()
{
}




//TargetOrbit->Planet->Destroy();
	
	//TargetOrbit->Planet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepWorldTransform);

	
	
	/// !!!!! не прикрепляется BP_Headquarters->AttachToActor(TargetOrbit->Planet, FAttachmentTransformRules::KeepWorldTransform);
	


	/*if (AGravityCharacterPawn* PlayerPawn = Cast<AGravityCharacterPawn>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Teleporting player to StartPoint (0,0,0)"));
		PlayerPawn->SetActorLocation(FVector(1000, 1000, 1000));
	}*/
	

	/*
	// Get the target orbit for the start planet
	if (StartPlanetNumber < 1 || StartPlanetNumber > GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid StartPlanetNumber %d! Available orbits: %d"), 
			StartPlanetNumber, GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList.Num());
		return;
	}

	APlanetOrbit* TargetOrbit = GeneratedHomeStarSystem->MainStar->PlanetarySystem->PlanetOrbitsList[StartPlanetNumber - 1];
	if (!TargetOrbit)
	{
		UE_LOG(LogTemp, Error, TEXT("Target orbit is null!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Target orbit: %s"), *TargetOrbit->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Target orbit position: %s"), *TargetOrbit->GetActorLocation().ToString());

	// Clear existing planet from orbit if any
	if (TargetOrbit->Planet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Clearing existing planet from orbit"));
		TargetOrbit->TriggerClearChildren();
	}

	// Set HomePlanet mobility to Movable if needed
	if (HomePlanet->GetRootComponent()->Mobility == EComponentMobility::Static)
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting HomePlanet mobility to Movable"));
		HomePlanet->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	}

	// Move HomePlanet to orbit position
	FVector OrbitPosition = TargetOrbit->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("Moving HomePlanet to orbit position: %s"), *OrbitPosition.ToString());
	
	HomePlanet->SetActorLocation(OrbitPosition, false, nullptr, ETeleportType::TeleportPhysics);

	// Attach HomePlanet to the orbit
	UE_LOG(LogTemp, Warning, TEXT("Attaching HomePlanet to orbit"));
	bool bAttachResult = HomePlanet->AttachToActor(TargetOrbit, FAttachmentTransformRules::KeepRelativeTransform);
	UE_LOG(LogTemp, Warning, TEXT("Attach result: %s"), bAttachResult ? TEXT("SUCCESS") : TEXT("FAILED"));

	// Set the planet reference in the orbit
	TargetOrbit->Planet = HomePlanet;
	if (TargetOrbit->Planet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Planet reference set in orbit"));
	}

	// Get StartPoint position from Headquarters
	FVector StartPointPosition = BP_Headquarters->GetStartPointPosition();
	UE_LOG(LogTemp, Warning, TEXT("StartPoint position: %s"), *StartPointPosition.ToString());

	// Get StarSystem center position
	FVector StarSystemPosition = GeneratedHomeStarSystem->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("StarSystem position: %s"), *StarSystemPosition.ToString());

	// Calculate offset to move StartPoint to (0,0,0)
	FVector OffsetToCenter = FVector::ZeroVector - StartPointPosition;
	UE_LOG(LogTemp, Warning, TEXT("Offset to center StartPoint: %s"), *OffsetToCenter.ToString());

	// Move the entire StarSystem by the offset (this will move all planets and orbits)
	FVector NewStarSystemPosition = StarSystemPosition + OffsetToCenter;
	UE_LOG(LogTemp, Warning, TEXT("Moving StarSystem from %s to %s"), *StarSystemPosition.ToString(), *NewStarSystemPosition.ToString());
	
	GeneratedHomeStarSystem->SetActorLocation(NewStarSystemPosition);
	
	// Also move the BP_Headquarters by the same offset to keep StartPoint at (0,0,0)
	FVector HeadquartersCurrentPosition = BP_Headquarters->GetActorLocation();
	FVector NewHeadquartersPosition = HeadquartersCurrentPosition + OffsetToCenter;
	UE_LOG(LogTemp, Warning, TEXT("Moving BP_Headquarters from %s to %s"), *HeadquartersCurrentPosition.ToString(), *NewHeadquartersPosition.ToString());
	
	BP_Headquarters->SetActorLocation(NewHeadquartersPosition);

	// Teleport player to StartPoint (which is now at 0,0,0)
	AGravityCharacterPawn* PlayerPawn = Cast<AGravityCharacterPawn>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Teleporting player to StartPoint (0,0,0)"));
		PlayerPawn->SetActorLocation(FVector::ZeroVector);
		
		// Verify final positions
		FVector FinalPlayerPosition = PlayerPawn->GetActorLocation();
		FVector FinalStarSystemPosition = GeneratedHomeStarSystem->GetActorLocation();
		FVector FinalStartPointPosition = BP_Headquarters->GetStartPointPosition();
		FVector FinalHeadquartersPosition = BP_Headquarters->GetActorLocation();
		FVector FinalPlanetPosition = HomePlanet->GetActorLocation();
		
		UE_LOG(LogTemp, Warning, TEXT("Final player position: %s"), *FinalPlayerPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final StarSystem position: %s"), *FinalStarSystemPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final StartPoint position: %s"), *FinalStartPointPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final Headquarters position: %s"), *FinalHeadquartersPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Final Planet position: %s"), *FinalPlanetPosition.ToString());
		
		// Check if StartPoint is actually at (0,0,0)
		FVector StartPointDistance = FinalStartPointPosition - FVector::ZeroVector;
		float StartPointDistanceMagnitude = StartPointDistance.Size();
		UE_LOG(LogTemp, Warning, TEXT("StartPoint distance from origin: %f"), StartPointDistanceMagnitude);
		
		// Check if planet is attached to orbit
		AActor* PlanetParent = HomePlanet->GetAttachParentActor();
		UE_LOG(LogTemp, Warning, TEXT("Planet attach parent: %s"), PlanetParent ? *PlanetParent->GetName() : TEXT("NULL"));
		
		UE_LOG(LogTemp, Warning, TEXT("Integration completed successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerPawn not found!"));
	}*/

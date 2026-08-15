#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/GameModes/GravityGameModeBase.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Rendering/APSStellarVisualSubsystem.h"
#include "APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "PlanetaryAtmosphere.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

namespace APSGeneratedGameplayHandoffSmokeTests
{
	constexpr double WholeTestTimeoutSeconds = 150.0;
	constexpr double ScreenshotTimeoutSeconds = 10.0;
	constexpr double PhysicalSurfaceTimeoutSeconds = 30.0;
	constexpr double CleanupTimeoutSeconds = 20.0;
	constexpr double MinimumNonBlackPixelRatio = 0.005;
	constexpr double MinimumBrightnessVariance = 4.0;
	constexpr double MinimumSubjectMeanBrightness = 5.0;
	constexpr double MinimumSubjectNonBlackPixelRatio = 0.08;
	constexpr double MinimumSubjectBrightnessVariance = 24.0;
	// The nine default WorldScape collision tiles cover roughly 80-90 metres around
	// the pawn. Require real displaced geometry, but keep this local-patch guard below
	// the last measured 11.33 m cooked range. The wider 100 m / 250 m / 1 km probes
	// below remain the perceptual-relief contract and catch broadly flat profiles.
	constexpr double MinimumCollisionReliefVariationCm = 500.0;
	// These are geometric height-field deltas around the pawn's real production
	// landing point, not material contrast thresholds.
	constexpr double MinimumNaturalRange10mCm = 300.0;
	constexpr double MinimumProofRange100mCm = 1500.0;
	constexpr double MinimumProofRange250mCm = 2500.0;
	constexpr double MinimumProofRange1kmCm = 2000.0;
	// A single central trace can be satisfied by a flat fallback collider while the
	// analytic WorldScape height field remains displaced.  The gameplay handoff must
	// therefore prove the current cooked CollisionLods over the same local footprint.
	constexpr double MinimumLocalCollisionRange5mCm = 5.0;
	constexpr double MinimumLocalCollisionRange15mCm = 20.0;
	constexpr double MinimumLocalCollisionRange30mCm = 50.0;
	constexpr double MinimumLocalCollisionNoiseFraction = 0.45;
	constexpr int32 MinimumLocalCollisionSamplesPerRing = 8;
	constexpr double MaximumLocalCollisionNoiseDeltaCm = 250.0;
	constexpr double MaximumProofSlope = 0.25;
	constexpr double MaximumNaturalHeightfieldCollisionDeltaCm = 200.0;
	constexpr int32 PhysicalProofDirectionCount = 12;
	constexpr double MaximumSettledSpeedCmPerSecond = 75.0;
	constexpr double MaximumFootClearanceCm = 50.0;
	constexpr double MaximumManualObserverLagCm = 1.0;
	constexpr double MinimumGroundMeanBrightness = 30.0;
	constexpr double MinimumGroundP10Brightness = 30.0;
	constexpr double MinimumGroundNonBlackPixelRatio = 0.15;
	constexpr double MinimumGroundBrightnessVariance = 12.0;
	constexpr double MinimumGroundBrightnessSpread = 8.0;
	constexpr double MinimumGroundMeanSpatialDelta = 0.35;
	// Keep the physical-surface readability light strong enough to survive the
	// fixed-exposure floor, but bounded so it cannot become a second sun.
	constexpr float MinimumGameplaySurfaceFillIntensity = 2.10f;
	constexpr float MaximumGameplaySurfaceFillIntensity = 2.30f;
	constexpr double MinimumGameplaySurfaceFillIncidence = 0.65;
	constexpr double MaximumGameplaySurfaceFillIncidence = 0.80;
	constexpr double MinimumVisibleRenderLodReliefVariationCm = 100.0;
	constexpr double MaximumVisibleRenderNoiseDeltaCm = 250.0;
	constexpr double MaximumVisibleRenderAnchorErrorCm = 2.0;
	constexpr int32 RequiredNaturalSettleFrames = 8;
	constexpr int32 WetOceanRenderViewCount = 2;
	constexpr int32 WetOceanDirectionSampleCount = 192;
	constexpr int32 RequiredWetOceanStableFrames = 6;
	constexpr double MinimumWetOceanCameraClearanceCm = 5000.0;
	constexpr double MaximumWetOceanCameraClearanceCm = 50000.0;
	// WorldScape stops generating collision once round(log2(distance / HeightAnchor))
	// reaches one. Keep the observer comfortably below that sqrt(2) boundary.
	constexpr double WetOceanCollisionCookHeightAnchorFraction = 1.2;
	constexpr double MinimumWetOceanCameraMovementCm = 100000.0;
	// A 15-degree separation is wider than twice the centred-payload tolerance
	// (dot >= 0.995, about 5.73 degrees), so one stale snapped normal cannot pass
	// both rendered observer positions.
	constexpr double MaximumWetOceanViewDirectionDot = 0.9659258262890683;

	constexpr int32 ProjectionEvidenceWarmupFrames = 3;
	constexpr int32 ProjectionEvidenceRequiredFrames = 32;
	constexpr double ProjectionEvidencePhaseTimeoutSeconds = 20.0;
	constexpr double ProjectionEvidenceViewPositionToleranceCm = 0.01;
	constexpr double ProjectionEvidenceViewAngleToleranceDegrees = 0.01;
	constexpr double ProjectionEvidenceProxyPositionToleranceCm = 0.01;
	constexpr double ProjectionEvidenceProxyScaleTolerance = 1.0e-6;
	constexpr double ProjectionEvidenceProxyRotationToleranceDegrees = 1.0e-6;
	constexpr double ProjectionEvidenceRelativeVectorToleranceCm = 0.02;
	constexpr double ProjectionEvidenceStationaryNdcTolerance = 0.0015;
	constexpr double ProjectionEvidenceMovingClosureToleranceCm = 0.05;
	constexpr double ProjectionEvidenceMovementPerFrameCm = 100.0;

	static TAutoConsoleVariable<int32> CVarAPSTestsFullScaleProjectionEvidence(
		TEXT("aps.Tests.FullScaleProjectionEvidence"),
		0,
		TEXT("Opt-in 32-frame stationary and controlled-movement canonical projection evidence."),
		ECVF_Default);

	struct FVisibleWorldScapeRenderLodProof
	{
		const UWorldScapeLod* Lod0{nullptr};
		int32 VertexCount{0};
		double RelativeAnchorErrorCm{TNumericLimits<double>::Max()};
		double WorldAnchorErrorCm{TNumericLimits<double>::Max()};
		double ObserverCenterOffsetCm{TNumericLimits<double>::Max()};
		double MaximumCenterOffsetCm{0.0};
		double ReliefVariationCm{0.0};
		double MaximumNoiseDeltaCm{TNumericLimits<double>::Max()};
		double ClosestObserverSurfaceDistanceCm{TNumericLimits<double>::Max()};
		double ClosestObserverHeightCm{TNumericLimits<double>::Max()};
	};

	bool BuildVisibleWorldScapeRenderLod0Proof(AWorldScapeRoot* Root,
		const FVector& ObserverWorldPosition, FVisibleWorldScapeRenderLodProof& OutProof,
		FString& OutFailure)
	{
		OutProof = FVisibleWorldScapeRenderLodProof{};
		OutFailure.Reset();
		if (!IsValid(Root) || ObserverWorldPosition.ContainsNaN())
		{
			OutFailure = TEXT("root or WorldScape observer is invalid");
			return false;
		}
		if (Root->WorldScapeLodInGeneration.Num() != 0)
		{
			OutFailure = FString::Printf(TEXT("render workers remain in flight count=%d"),
				Root->WorldScapeLodInGeneration.Num());
			return false;
		}

		for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
		{
			if (!IsValid(Lod) || Lod->WaterBody || Lod->Lod != 0)
			{
				continue;
			}
			if (OutProof.Lod0)
			{
				OutFailure = TEXT("terrain render hierarchy contains duplicate LOD0 components");
				return false;
			}
			OutProof.Lod0 = Lod;
		}

		const FVector ObserverEcef = Root->WorldToECEF(ObserverWorldPosition).ToFVector();
		const FVector ObserverEcefNormal = ObserverEcef.GetSafeNormal();
		const FVector SurfaceCenter = Root->GetActorLocation();
		const FVector ObserverWorldNormal = (ObserverWorldPosition - SurfaceCenter)
			.GetSafeNormal();
		const UWorldScapeLod* Lod0 = OutProof.Lod0;
		if (!IsValid(Lod0) || ObserverEcefNormal.IsNearlyZero()
			|| ObserverWorldNormal.IsNearlyZero())
		{
			OutFailure = TEXT("unique terrain render LOD0 or observer normal is unavailable");
			return false;
		}
		if (!APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
			Lod0, ObserverEcefNormal, true))
		{
			OutFailure = TEXT("terrain render LOD0 payload is incomplete or snapped to a stale observer");
			return false;
		}
		if (!IsValid(Lod0->Mesh) || !Lod0->Mesh->IsRegistered()
			|| !Lod0->Mesh->IsVisible() || Lod0->Mesh->bHiddenInGame
			|| Lod0->Mesh->GetNumSections() < 1
			|| !Lod0->Mesh->IsMeshSectionVisible(0)
			|| Root->IsHidden() || Lod0->Mesh->GetAttachParent() != Root->TransformKeeper
			|| !Root->GetActorScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER)
			|| !Lod0->Mesh->GetRelativeScale3D().Equals(
				FVector::OneVector, KINDA_SMALL_NUMBER))
		{
			OutFailure = TEXT("terrain render LOD0 is not an effectively visible unit-scale TransformKeeper child");
			return false;
		}

		const FVector AnchorEcef = Lod0->RelativePosition.ToFVector();
		const FVector ExpectedAnchorWorld = Root->ECEFToWorld(AnchorEcef).ToFVector();
		OutProof.RelativeAnchorErrorCm = FVector::Distance(
			Lod0->Mesh->GetRelativeLocation(), AnchorEcef);
		OutProof.WorldAnchorErrorCm = FVector::Distance(
			Lod0->Mesh->GetComponentLocation(), ExpectedAnchorWorld);
		if (!FMath::IsFinite(OutProof.RelativeAnchorErrorCm)
			|| !FMath::IsFinite(OutProof.WorldAnchorErrorCm)
			|| OutProof.RelativeAnchorErrorCm > MaximumVisibleRenderAnchorErrorCm
			|| OutProof.WorldAnchorErrorCm > MaximumVisibleRenderAnchorErrorCm)
		{
			OutFailure = FString::Printf(
				TEXT("terrain render LOD0 mesh transform is stale relativeError=%.3fcm worldError=%.3fcm"),
				OutProof.RelativeAnchorErrorCm, OutProof.WorldAnchorErrorCm);
			return false;
		}

		if (!FMath::IsFinite(Lod0->LodSize) || Lod0->LodSize <= 0.0)
		{
			OutFailure = TEXT("terrain render LOD0 has no finite generated footprint");
			return false;
		}
		const FVector ObserverShellEcef = ObserverEcefNormal * Root->PlanetScaleCode;
		OutProof.ObserverCenterOffsetCm = FVector::Distance(ObserverShellEcef, AnchorEcef);
		OutProof.MaximumCenterOffsetCm = FMath::Max(
			Lod0->LodSize * 0.75, static_cast<double>(Root->TriangleSize) * 4.0);
		if (!FMath::IsFinite(OutProof.ObserverCenterOffsetCm)
			|| OutProof.ObserverCenterOffsetCm > OutProof.MaximumCenterOffsetCm)
		{
			OutFailure = FString::Printf(
				TEXT("terrain render LOD0 does not cover its current observer offset=%.3fcm maximum=%.3fcm"),
				OutProof.ObserverCenterOffsetCm, OutProof.MaximumCenterOffsetCm);
			return false;
		}

		const FTransform MeshTransform = Lod0->Mesh->GetComponentTransform();
		const FWorldScapeMeshSection* VisibleRenderSection =
			Lod0->Mesh->GetProcMeshSection(0);
		if (!VisibleRenderSection
			|| VisibleRenderSection->PlanetVertexBuffer.Num() < 3
			|| VisibleRenderSection->PlanetIndexBuffer.Num() < 3
			|| VisibleRenderSection->PlanetVertexBuffer.Num() != Lod0->Vertices.Num()
			|| VisibleRenderSection->PlanetIndexBuffer.Num() != Lod0->Triangles.Num())
		{
			OutFailure = FString::Printf(
				TEXT("terrain render LOD0 scene section is missing, empty, or stale sceneVertices=%d payloadVertices=%d sceneIndices=%d payloadIndices=%d"),
				VisibleRenderSection
					? VisibleRenderSection->PlanetVertexBuffer.Num() : 0,
				Lod0->Vertices.Num(), VisibleRenderSection
					? VisibleRenderSection->PlanetIndexBuffer.Num() : 0,
				Lod0->Triangles.Num());
			return false;
		}
		double MinimumRenderHeightCm = TNumericLimits<double>::Max();
		double MaximumRenderHeightCm = -TNumericLimits<double>::Max();
		double MaximumNoiseDeltaCm = 0.0;
		const int32 NoiseSampleStride = FMath::Max(
			1, VisibleRenderSection->PlanetVertexBuffer.Num() / 64);
		for (int32 VertexIndex = 0;
			VertexIndex < VisibleRenderSection->PlanetVertexBuffer.Num(); ++VertexIndex)
		{
			const FVector WorldVertex = MeshTransform.TransformPosition(
				VisibleRenderSection->PlanetVertexBuffer[VertexIndex].Position);
			const FVector VertexFromCenter = WorldVertex - SurfaceCenter;
			const double RadialHeightCm = VertexFromCenter.Size() - Root->PlanetScaleCode;
			if (WorldVertex.ContainsNaN() || !FMath::IsFinite(RadialHeightCm))
			{
				OutFailure = TEXT("terrain render LOD0 contains a non-finite transformed vertex");
				return false;
			}
			MinimumRenderHeightCm = FMath::Min(MinimumRenderHeightCm, RadialHeightCm);
			MaximumRenderHeightCm = FMath::Max(MaximumRenderHeightCm, RadialHeightCm);
			++OutProof.VertexCount;

			const FVector VertexNormal = VertexFromCenter.GetSafeNormal();
			const double ObserverSurfaceDistanceCm = FVector::Distance(
				VertexNormal * Root->PlanetScaleCode,
				ObserverWorldNormal * Root->PlanetScaleCode);
			if (ObserverSurfaceDistanceCm < OutProof.ClosestObserverSurfaceDistanceCm)
			{
				OutProof.ClosestObserverSurfaceDistanceCm = ObserverSurfaceDistanceCm;
				OutProof.ClosestObserverHeightCm = RadialHeightCm;
			}

			if (VertexIndex % NoiseSampleStride == 0
				|| VertexIndex == VisibleRenderSection->PlanetVertexBuffer.Num() - 1)
			{
				const double ExpectedHeightCm = Root->GetGroundHeight(WorldVertex, false);
				if (!FMath::IsFinite(ExpectedHeightCm))
				{
					OutFailure = TEXT("terrain render LOD0 analytic height sample is non-finite");
					return false;
				}
				MaximumNoiseDeltaCm = FMath::Max(MaximumNoiseDeltaCm,
					FMath::Abs(RadialHeightCm - ExpectedHeightCm));
			}
		}

		OutProof.ReliefVariationCm = MaximumRenderHeightCm - MinimumRenderHeightCm;
		OutProof.MaximumNoiseDeltaCm = MaximumNoiseDeltaCm;
		if (OutProof.VertexCount < 3
			|| !FMath::IsFinite(OutProof.ReliefVariationCm)
			|| OutProof.ReliefVariationCm < MinimumVisibleRenderLodReliefVariationCm)
		{
			OutFailure = FString::Printf(
				TEXT("terrain render LOD0 is geometrically flat vertices=%d relief=%.3fcm minimum=%.3fcm"),
				OutProof.VertexCount, OutProof.ReliefVariationCm,
				MinimumVisibleRenderLodReliefVariationCm);
			return false;
		}
		if (!FMath::IsFinite(OutProof.MaximumNoiseDeltaCm)
			|| OutProof.MaximumNoiseDeltaCm > MaximumVisibleRenderNoiseDeltaCm)
		{
			OutFailure = FString::Printf(
				TEXT("terrain render LOD0 geometry diverges from WorldScape noise maxDelta=%.3fcm maximum=%.3fcm"),
				OutProof.MaximumNoiseDeltaCm, MaximumVisibleRenderNoiseDeltaCm);
			return false;
		}
		return true;
	}

	UCapsuleComponent* FindPawnCapsule(const APawn* Pawn)
	{
		return IsValid(Pawn) ? Pawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	}

	UCameraComponent* FindPawnCamera(const APawn* Pawn)
	{
		return IsValid(Pawn) ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;
	}

	USpringArmComponent* FindPawnSpringArm(const APawn* Pawn)
	{
		return IsValid(Pawn) ? Pawn->FindComponentByClass<USpringArmComponent>() : nullptr;
	}

	bool ReadGravityContract(const APawn* Pawn,
		EGravityType& OutType, FVector& OutDirection, AActor*& OutTarget)
	{
		if (!IsValid(Pawn))
		{
			return false;
		}

		if (const ACustomGravityCharacter* CustomCharacter =
			Cast<ACustomGravityCharacter>(Pawn))
		{
			OutType = CustomCharacter->CurrentGravityType;
			OutDirection = CustomCharacter->GetCurrentGravityDirection();
			OutTarget = CustomCharacter->GravityTarget;
			return !OutDirection.ContainsNaN();
		}

		const FProperty* TypeProperty = FindFProperty<FProperty>(
			Pawn->GetClass(), TEXT("CurrentGravityType"));
		const FStructProperty* DirectionProperty = FindFProperty<FStructProperty>(
			Pawn->GetClass(), TEXT("GravityDirection"));
		const FObjectPropertyBase* TargetProperty = FindFProperty<FObjectPropertyBase>(
			Pawn->GetClass(), TEXT("GravityTargetActor"));
		if (!TypeProperty || !DirectionProperty || !TargetProperty
			|| DirectionProperty->Struct != TBaseStructure<FVector>::Get())
		{
			return false;
		}

		int64 TypeValue = 0;
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(TypeProperty))
		{
			const void* Value = EnumProperty->ContainerPtrToValuePtr<void>(Pawn);
			TypeValue = EnumProperty->GetUnderlyingProperty()
				->GetSignedIntPropertyValue(Value);
		}
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(TypeProperty))
		{
			TypeValue = ByteProperty->GetPropertyValue_InContainer(Pawn);
		}
		else
		{
			return false;
		}

		OutType = static_cast<EGravityType>(TypeValue);
		OutDirection = *DirectionProperty->ContainerPtrToValuePtr<FVector>(Pawn);
		OutTarget = Cast<AActor>(TargetProperty->GetObjectPropertyValue_InContainer(Pawn));
		return !OutDirection.ContainsNaN();
	}

	template <typename TActorType>
	TArray<TActorType*> FindActors(UWorld* World)
	{
		TArray<TActorType*> Result;
		if (!World)
		{
			return Result;
		}
		for (TActorIterator<TActorType> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Result.Add(*It);
			}
		}
		return Result;
	}

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (AAstroGenerator* Generator : FindActors<AAstroGenerator>(World))
		{
			if (Generator->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return Generator;
			}
		}
		return nullptr;
	}

	APlanetarySurfaceGenerator* FindPreviewSurfaceGenerator(
		UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (APlanetarySurfaceGenerator* Generator
			: FindActors<APlanetarySurfaceGenerator>(World))
		{
			if (Generator->GetOwner() == PreviewGenerator)
			{
				return Generator;
			}
		}
		return nullptr;
	}

	bool IsFiniteTransform(const FTransform& Transform)
	{
		const FVector Location = Transform.GetLocation();
		const FVector Scale = Transform.GetScale3D();
		const FQuat Rotation = Transform.GetRotation();
		return !Transform.ContainsNaN()
			&& FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y)
			&& FMath::IsFinite(Location.Z) && FMath::IsFinite(Scale.X)
			&& FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
			&& FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y)
			&& FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W);
	}

	int32 CountVisibleRegisteredRenderComponents(const AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsHidden())
		{
			return 0;
		}

		int32 Result = 0;
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents(PrimitiveComponents);
		for (const UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!IsValid(Component) || !Component->IsRegistered()
				|| !Component->IsVisible() || Component->bHiddenInGame)
			{
				continue;
			}
			if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component))
			{
				Result += StaticMesh->GetStaticMesh() ? 1 : 0;
			}
			else if (const USkeletalMeshComponent* SkeletalMesh =
				Cast<USkeletalMeshComponent>(Component))
			{
				Result += SkeletalMesh->GetSkeletalMeshAsset() ? 1 : 0;
			}
		}
		return Result;
	}

	int32 CountPresentedStaticMeshes(const AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsHidden())
		{
			return 0;
		}

		int32 Result = 0;
		TInlineComponentArray<UStaticMeshComponent*> Components;
		Actor->GetComponents(Components);
		for (const UStaticMeshComponent* Component : Components)
		{
			if (IsValid(Component) && Component->IsRegistered()
				&& Component->IsVisible() && !Component->bHiddenInGame)
			{
				++Result;
			}
		}
		return Result;
	}

	int32 CountCollidableStaticMeshes(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return 0;
		}

		int32 Result = 0;
		TInlineComponentArray<UStaticMeshComponent*> Components;
		Actor->GetComponents(Components);
		for (const UStaticMeshComponent* Component : Components)
		{
			if (IsValid(Component) && Component->IsRegistered()
				&& Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				++Result;
			}
		}
		return Result;
	}

	int32 CountPresentedProceduralMeshes(const AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsHidden())
		{
			return 0;
		}

		int32 Result = 0;
		TInlineComponentArray<UProceduralMeshComponent*> Components;
		Actor->GetComponents(Components);
		for (const UProceduralMeshComponent* Component : Components)
		{
			if (IsValid(Component) && Component->IsRegistered()
				&& Component->IsVisible() && !Component->bHiddenInGame)
			{
				++Result;
			}
		}
		return Result;
	}

	bool IsEffectivelyPresented(const UPrimitiveComponent* Component)
	{
		const AActor* Owner = IsValid(Component) ? Component->GetOwner() : nullptr;
		return IsValid(Component) && Component->IsRegistered()
			&& Component->IsVisible() && !Component->bHiddenInGame
			&& (!IsValid(Owner) || !Owner->IsHidden());
	}

	bool IgnoresEveryCollisionChannel(const UPrimitiveComponent* Component)
	{
		if (!IsValid(Component))
		{
			return false;
		}
		for (int32 ChannelIndex = 0;
			ChannelIndex < static_cast<int32>(ECC_MAX); ++ChannelIndex)
		{
			if (Component->GetCollisionResponseToChannel(
				static_cast<ECollisionChannel>(ChannelIndex)) != ECR_Ignore)
			{
				return false;
			}
		}
		return true;
	}

	bool IsPreviewOceanProxy(const UProceduralMeshComponent* Component)
	{
		if (!IsValid(Component))
		{
			return false;
		}
		const FString ComponentName = Component->GetName();
		return ComponentName.StartsWith(TEXT("PreviewOcean"), ESearchCase::CaseSensitive)
			|| ComponentName.StartsWith(
				TEXT("PreviewBodyOcean"), ESearchCase::CaseSensitive);
	}

	class FGeneratedCivilizationHandoffCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FGeneratedCivilizationHandoffCommand(FAutomationTestBase* InTest,
			EPlanetType InExpectedPlanetType = EPlanetType::Frozen,
			bool bInValidateWetOceanContract = false)
			: Test(InTest)
			, ExpectedPlanetType(InExpectedPlanetType)
			, bValidateWetOceanContract(bInValidateWetOceanContract)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (TestStartSeconds <= 0.0)
			{
				TestStartSeconds = Now;
				StepStartSeconds = Now;
			}
			if (Step != EStep::Cleanup && Now - TestStartSeconds > WholeTestTimeoutSeconds)
			{
				return Fail(FString::Printf(
					TEXT("150-second timeout at step %d"), static_cast<int32>(Step)));
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			switch (Step)
			{
			case EStep::OpenGenerator:
				return UpdateOpenGenerator(World, Now);
			case EStep::WaitForPreview:
				return UpdateWaitForPreview(World, Now);
			case EStep::WaitForGameplayTravel:
				return UpdateWaitForGameplayTravel(World, Now);
			case EStep::ValidateGameplayHierarchy:
				return UpdateValidateGameplayHierarchy(World, Now);
			case EStep::ProjectionEvidenceStationaryHold:
				return UpdateProjectionEvidenceStationaryHold(World, Now);
			case EStep::ProjectionEvidenceControlledMove:
				return UpdateProjectionEvidenceControlledMove(World, Now);
			case EStep::WaitForGameplaySurface:
				return UpdateWaitForGameplaySurface(World, Now);
			case EStep::WaitForScreenshot:
				return UpdateWaitForScreenshot(World, Now);
			case EStep::WaitForWetOceanScreenshots:
				return UpdateWaitForWetOceanScreenshots(World, Now);
			case EStep::ValidateManualApproachObserver:
				return UpdateValidateManualApproachObserver(World, Now);
			case EStep::WaitForPhysicalSurface:
				return UpdateWaitForPhysicalSurface(World, Now);
			case EStep::WaitForPhysicalSurfaceScreenshot:
				return UpdateWaitForPhysicalSurfaceScreenshot(World, Now);
			case EStep::Cleanup:
				return UpdateCleanup(World, Now);
			default:
				return true;
			}
		}

	private:
		enum class EStep : uint8
		{
			OpenGenerator,
			WaitForPreview,
			WaitForGameplayTravel,
			ValidateGameplayHierarchy,
			ProjectionEvidenceStationaryHold,
			ProjectionEvidenceControlledMove,
			WaitForGameplaySurface,
			WaitForScreenshot,
			WaitForWetOceanScreenshots,
			ValidateManualApproachObserver,
			WaitForPhysicalSurface,
			WaitForPhysicalSurfaceScreenshot,
			Cleanup
		};

		struct FProjectionEvidenceRenderedSample
		{
			uint64 RenderFrameCounter{0u};
			double RenderTime{0.0};
			FVector ViewLocation{FVector::ZeroVector};
			FQuat ViewRotation{FQuat::Identity};
			FVector GalaxyLocalPosition{FVector::ZeroVector};
			FQuat GalaxyLocalRotation{FQuat::Identity};
			FVector GalaxyLocalScale{FVector::OneVector};
			FVector GalaxyWorldPosition{FVector::ZeroVector};
			FVector ClusterLocalPosition{FVector::ZeroVector};
			FQuat ClusterLocalRotation{FQuat::Identity};
			FVector ClusterLocalScale{FVector::OneVector};
			FVector ClusterWorldPosition{FVector::ZeroVector};
			FVector2D ClusterNdc{FVector2D::ZeroVector};
			bool bClusterNdcValid{false};
			double GalaxyProjectionErrorCm{TNumericLimits<double>::Max()};
			double ClusterProjectionErrorCm{TNumericLimits<double>::Max()};
			double GalaxyMaxMatrixMagnitudeCm{TNumericLimits<double>::Max()};
			double ClusterMaxMatrixMagnitudeCm{TNumericLimits<double>::Max()};
			uint64 ProxyBuildSerial{0u};
			uint64 InstanceUploadCount{0u};
			uint64 TransformMutationSerial{0u};
		};

		bool Fail(const FString& Message)
		{
			if (bProjectionEvidencePhaseStarted
				&& (Step == EStep::ProjectionEvidenceStationaryHold
					|| Step == EStep::ProjectionEvidenceControlledMove))
			{
				const TCHAR* Phase = Step == EStep::ProjectionEvidenceStationaryHold
					? TEXT("Stationary") : TEXT("Moving");
				const TCHAR* Status = ProjectionEvidenceAcceptedFrameCount > 0
					? TEXT("FAIL") : TEXT("NOT_COVERED");
				UE_LOG(LogTemp, Error,
					TEXT("[APS.P0.ProjectionPhase] source=directRuntime phase=%s event=End status=%s samples=%d reason=%s"),
					Phase, Status, ProjectionEvidenceAcceptedFrameCount, *Message);
				bProjectionEvidencePhaseStarted = false;
			}
			RestoreProjectionEvidenceCamera();
			if (PendingFailure.IsEmpty())
			{
				PendingFailure = Message;
				UE_LOG(LogTemp, Error,
					TEXT("[APS.Handoff.Smoke] FAIL %s; beginning safe WorldScape drain"),
					*Message);
			}
			Step = EStep::Cleanup;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool UpdateOpenGenerator(UWorld* World, double Now)
		{
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			UGeneratedWorld* Model = ViewModel ? ViewModel->GetGeneratedWorld() : nullptr;
			if (!World || !Controller || !ViewModel || !Model)
			{
				return false;
			}

			MenuWorld = World;
			EditableGeneratedWorldAddress = Model;
			Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
			Model->bGenerateFullScaledWorld = true;
			Model->bGenerateHomeSystem = true;
			Model->bStartWithHomePlanet = true;
			Model->bRandomHomeSystem = false;
			Model->bRandomHomeSystemType = false;
			Model->bRandomHomeStar = false;
			Model->bRandomStartPlanetNumber = false;
			Model->GalaxyType = EGalaxyType::Spiral;
			Model->GalaxyClass = EGalaxyClass::Sc;
			Model->GalaxySize = 64;
			Model->GalaxyStarCount = 2048;
			Model->GalaxyStarDensity = 12.0;
			Model->StarClusterSize = EStarClusterSize::Tiny;
			Model->StarClusterType = EStarClusterType::OpenCluster;
			Model->StarClusterPopulation = EStarClusterPopulation::Dwarfs;
			Model->StarClusterComposition = EStarClusterComposition::AllSpectral;
			Model->StarType = EStarType::SingleStar;
			Model->StellarType = EStellarType::MainSequence;
			Model->SpectralClass = ESpectralClass::G;
			Model->HomeSystemPosition = EHomeSystemPosition::DirectPosition;
			Model->PlanetarySystemType = EPlanetarySystemType::SinglePlanetSystem;
			Model->OrbitDistributionType = EOrbitDistributionType::Uniform;
			Model->PlanetsAmount = 1;
			Model->MoonsAmount = 0;
			Model->StartPlanetIndex = 1;
			// The default smoke exercises the standard Cryogenic path. The dedicated
			// wet smoke reuses this production handoff with the Water resolver profile.
			Model->PlanetType = ExpectedPlanetType;
			Model->PlanetRadius = 6371.0;
			Model->PlanetSurfaceSeed = 424242;
			Model->SurfaceFeatureScale = 1.0;
			Model->SurfaceReliefScale = 1.0;
			Model->SurfaceLandCoverageScale = 1.0;
			Model->SurfaceMountainScale = 1.0;
			Model->SurfaceCraterScale = 1.0;
			Model->SurfaceRoughnessScale = 1.0;
			Model->AtmosphereHeight = 140.0;
			Model->AtmosphereOpacity = 12.0;
			Model->AtmosphereMultiScattering = 1.0;
			Model->AtmosphereRayleighScattering = 8.0;

			if (!Controller->OpenAstronomicalGenerationForAutomation(
				EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization))
			{
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Civilization generator opened map=%s seed=%d type=%d level=StarCluster fullScale=true wetOceanContract=%d"),
				*World->GetMapName(), Model->PlanetSurfaceSeed,
				static_cast<int32>(ExpectedPlanetType),
				bValidateWetOceanContract ? 1 : 0);
			Step = EStep::WaitForPreview;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForPreview(UWorld* World, double Now)
		{
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			if (!World || !Controller || !ViewModel || !ViewModel->bPreviewReady)
			{
				return false;
			}

			PreviewGenerator = FindPreviewGenerator(World);
			APlanet* PreviewPlanet = PreviewGenerator.IsValid()
				? Cast<APlanet>(PreviewGenerator->GetActivePreviewWorldScapeBody()) : nullptr;
			// The menu owns one persistent streaming generator outside the generated
			// planet actor so subtype changes can swap profiles without rebuilding the
			// hierarchy. The planet's gameplay generator is intentionally not the
			// authoritative preview root.
			APlanetarySurfaceGenerator* PreviewSurface = FindPreviewSurfaceGenerator(
				World, PreviewGenerator.Get());
			if (!PreviewGenerator.IsValid() || !IsValid(PreviewPlanet)
				|| !PreviewPlanet->bWorldScapeSurfaceReady || !IsValid(PreviewSurface)
				|| !PreviewSurface->IsSurfaceProfileCurrent(PreviewPlanet))
			{
				return false;
			}
			if (!Cast<UAPSWorldScapePlanetNoise>(PreviewSurface->ResolvedNoiseInstance))
			{
				return Fail(TEXT("menu preview became ready through a legacy/non-APS noise generator"));
			}

			USpawnParameters* Spawn = ViewModel->SpawnParameters;
			if (!Spawn || !Spawn->BP_CharacterClass || !Spawn->BP_HomeSpaceship
				|| !Spawn->BP_HomeSpaceStation || !Spawn->BP_HomeSpaceHeadquarters
				|| !Spawn->BP_HomeSpaceShipyard)
			{
				return Fail(TEXT("Civilization route did not initialize all five selected spawn classes"));
			}
			UClass* ProductionCharacterClass = LoadClass<ACustomGravityCharacter>(nullptr,
				TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C"));
			if (!ProductionCharacterClass
				|| !ProductionCharacterClass->IsChildOf(ACustomGravityCharacter::StaticClass()))
			{
				return Fail(TEXT("production BP_CustomGravityCharacter class is unavailable"));
			}
			// The menu default still points at the deprecated AGravityCharacterPawn.
			// Exercise the same ACustomGravityCharacter Blueprint used by the real
			// generated-civilization flow instead of silently validating another pawn.
			ViewModel->SetSpawnClass(
				EAPSStartAssetSlot::Character, ProductionCharacterClass);
			if (Spawn->BP_CharacterClass.Get() != ProductionCharacterClass)
			{
				return Fail(TEXT("menu did not accept BP_CustomGravityCharacter selection"));
			}
			SelectedPawnClass = Spawn->BP_CharacterClass.Get();
			SelectedPawnClassPath = SelectedPawnClass.IsValid()
				? SelectedPawnClass->GetPathName() : FString();
			if (!SelectedPawnClass.IsValid() || SelectedPawnClassPath.IsEmpty())
			{
				return Fail(TEXT("selected pawn class is invalid before commit"));
			}
			Spawn->CivilizationName = TEXT("APS HANDOFF SMOKE CIVILIZATION");
			// Exercise the production gameplay contract directly. The pawn must be
			// placed by AAstroGenerator::ResolveSpawnLocation and settle on the
			// authoritative WorldScape collision without any test-side relocation.
			Spawn->CharacterSpawnPlace = ECharSpawnPlace::PlanetSurface;
			Spawn->HomeStationOrbitHeight = EOrbitHeight::LowOrbit;
			EditableSpawnParametersAddress = Spawn;
			PreviewProfileSignature = PreviewSurface->AppliedSurfaceProfileSignature;
			const FAPSCanonicalStellarProjectionDescriptor& PreviewProjection =
				PreviewGenerator->GetCanonicalStellarProjectionDescriptor();
			FAPSCanonicalStellarProxyRecord PreviewHomeRecord;
			FAPSCanonicalStellarProxyRecord PreviewGalaxyRecord;
			if (!PreviewProjection.bFinalized
				|| !PreviewProjection.CanonicalDatasetHash
				|| !PreviewProjection.ContextHash
				|| !PreviewProjection.MaterializedHomeStableId.IsValid()
				|| !PreviewGenerator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					PreviewProjection.MaterializedHomeStableId, PreviewHomeRecord)
				|| !PreviewGenerator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::Galaxy, 0, PreviewGalaxyRecord))
			{
				return Fail(TEXT("menu preview has no finalized canonical home projection snapshot"));
			}
			bHasPreviewProjectionSnapshot = true;
			PreviewProjectionVersion = PreviewProjection.ProjectionVersion;
			PreviewProjectionContextHash = PreviewProjection.ContextHash;
			PreviewCanonicalDatasetHash = PreviewProjection.CanonicalDatasetHash;
			PreviewCanonicalDatasetVersion = PreviewProjection.CanonicalDatasetVersion;
			PreviewCanonicalDatasetInputHash = PreviewProjection.CanonicalDatasetInputHash;
			PreviewCanonicalDatasetBuildSerial = PreviewProjection.CanonicalDatasetBuildSerial;
			PreviewCanonicalDatasetRecordCount = PreviewProjection.CanonicalDatasetRecordCount;
			PreviewHomeStableId = PreviewHomeRecord.StableId;
			PreviewHomeCanonicalPosition = PreviewHomeRecord.CanonicalPositionUnits;
			PreviewHomeExpectedProxyPosition = PreviewHomeRecord.ExpectedBaseProxyPositionCm;
			PreviewHomeCanonicalRadiusSolar = PreviewHomeRecord.CanonicalPhysicalRadiusSolar;
			PreviewGalaxyStableId = PreviewGalaxyRecord.StableId;
			PreviewGalaxyCanonicalPosition = PreviewGalaxyRecord.CanonicalPositionUnits;
			PreviewGalaxyExpectedProxyPosition =
				PreviewGalaxyRecord.ExpectedBaseProxyPositionCm;
			PreviewGalaxyCanonicalRadiusSolar =
				PreviewGalaxyRecord.CanonicalPhysicalRadiusSolar;
			bool bHasPreviewClusterSentinel = false;
			for (int32 InstanceIndex = 0;
				InstanceIndex < PreviewProjection.ClusterRenderedCount; ++InstanceIndex)
			{
				FAPSCanonicalStellarProxyRecord Candidate;
				if (PreviewGenerator->GetCanonicalStellarProxyRecord(
						EAPSCanonicalStellarProxyLayer::StarCluster, InstanceIndex, Candidate)
					&& Candidate.StableId != PreviewHomeStableId)
				{
					PreviewClusterStableId = Candidate.StableId;
					PreviewClusterCanonicalPosition = Candidate.CanonicalPositionUnits;
					PreviewClusterExpectedProxyPosition = Candidate.ExpectedBaseProxyPositionCm;
					PreviewClusterCanonicalRadiusSolar = Candidate.CanonicalPhysicalRadiusSolar;
					bHasPreviewClusterSentinel = true;
					break;
				}
			}
			if (!bHasPreviewClusterSentinel)
			{
				return Fail(TEXT("menu preview has no non-home canonical cluster sentinel"));
			}
			if (PreviewProjection.CanonicalDatasetRecordCount
				> PreviewProjection.ClusterRenderedCount)
			{
				FAPSCanonicalClusterSystemRecord DatasetOnlyRecord;
				bHasPreviewDatasetOnlyRecord = PreviewGenerator->GetCanonicalClusterDatasetRecord(
					PreviewProjection.ClusterRenderedCount, DatasetOnlyRecord);
				if (!bHasPreviewDatasetOnlyRecord)
				{
					return Fail(TEXT("menu preview cannot resolve finalized record outside its HISM LOD"));
				}
				PreviewDatasetOnlyStableId = DatasetOnlyRecord.StableId;
				PreviewDatasetOnlyCanonicalPosition = DatasetOnlyRecord.ClusterLocalLocation;
				PreviewDatasetOnlyCanonicalRadiusSolar = DatasetOnlyRecord.PrimaryStarModel.Radius;
				PreviewDatasetOnlyMinOrbit = DatasetOnlyRecord.PrimaryStarModel.MinOrbit;
				PreviewDatasetOnlyMaxOrbit = DatasetOnlyRecord.PrimaryStarModel.MaxOrbit;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Preview ready in %.2fs pawn=%s profile=%u projection=%u/%u dataset=%u home=%s; committing L_WorldGeneration"),
				Now - StepStartSeconds, *SelectedPawnClassPath, PreviewProfileSignature,
				PreviewProjectionVersion, PreviewProjectionContextHash,
				PreviewCanonicalDatasetHash,
				*PreviewHomeStableId.ToString(EGuidFormats::Digits));
			ViewModel->CommitAndOpenLevel(TEXT("L_WorldGeneration"));
			Step = EStep::WaitForGameplayTravel;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForGameplayTravel(UWorld* World, double Now)
		{
			if (!World || World == MenuWorld.Get()
				|| !World->GetMapName().Contains(TEXT("L_WorldGeneration")))
			{
				return false;
			}
			if (!Cast<AGravityGameModeBase>(World->GetAuthGameMode()))
			{
				return false;
			}

			GameplayWorld = World;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Travel complete in %.2fs map=%s gameMode=%s"),
				Now - StepStartSeconds, *World->GetMapName(),
				*GetNameSafe(World->GetAuthGameMode()));
			Step = EStep::ValidateGameplayHierarchy;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateValidateGameplayHierarchy(UWorld* World, double Now)
		{
			if (!World || World != GameplayWorld.Get())
			{
				return false;
			}

			const TArray<AAstroGenerator*> Generators = FindActors<AAstroGenerator>(World);
			const TArray<AGalaxy*> Galaxies = FindActors<AGalaxy>(World);
			const TArray<AStarCluster*> Clusters = FindActors<AStarCluster>(World);
			const TArray<AStarSystem*> StarSystems = FindActors<AStarSystem>(World);
			const TArray<AStar*> Stars = FindActors<AStar>(World);
			const TArray<APlanetarySystem*> PlanetarySystems = FindActors<APlanetarySystem>(World);
			const TArray<APlanetOrbit*> PlanetOrbits = FindActors<APlanetOrbit>(World);
			const TArray<APlanet*> Planets = FindActors<APlanet>(World);
			const TArray<ASpaceHeadquarters*> Headquarters = FindActors<ASpaceHeadquarters>(World);
			const TArray<ASpaceShipyard*> Shipyards = FindActors<ASpaceShipyard>(World);
			const TArray<ASpaceship*> Spaceships = FindActors<ASpaceship>(World);
			TArray<ASpaceStation*> PlainStations;
			for (ASpaceStation* Station : FindActors<ASpaceStation>(World))
			{
				if (!Cast<ASpaceHeadquarters>(Station) && !Cast<ASpaceShipyard>(Station))
				{
					PlainStations.Add(Station);
				}
			}
			UMainGameplayInstance* GameplayState = World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
			APlayerController* PlayerController = World->GetFirstPlayerController();
			APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
			if (!GameplayState || !GameplayState->NewGeneratedWorld || !GameplayState->SpawnParameters
				|| !GameplayState->CurrentCivilization || !PlayerController || !PlayerPawn
				|| Generators.Num() == 0 || Galaxies.Num() == 0 || Clusters.Num() == 0
				|| StarSystems.Num() == 0 || Stars.Num() == 0 || PlanetarySystems.Num() == 0
				|| PlanetOrbits.Num() == 0 || Planets.Num() == 0 || Headquarters.Num() == 0
				|| PlainStations.Num() == 0 || Shipyards.Num() == 0 || Spaceships.Num() == 0)
			{
				return false;
			}

			if (Generators.Num() != 1 || Galaxies.Num() != 1 || Clusters.Num() != 1
				|| StarSystems.Num() != 1 || Stars.Num() != 1 || PlanetarySystems.Num() != 1
				|| PlanetOrbits.Num() != 1 || Planets.Num() != 1 || Headquarters.Num() != 1
				|| PlainStations.Num() != 1 || Shipyards.Num() != 1)
			{
				return Fail(FString::Printf(
					TEXT("duplicate/incomplete materialization generators=%d galaxy=%d cluster=%d systems=%d stars=%d planetarySystems=%d orbits=%d planets=%d HQ=%d stations=%d yards=%d ships=%d"),
					Generators.Num(), Galaxies.Num(), Clusters.Num(), StarSystems.Num(), Stars.Num(),
					PlanetarySystems.Num(), PlanetOrbits.Num(), Planets.Num(), Headquarters.Num(),
					PlainStations.Num(), Shipyards.Num(), Spaceships.Num()));
			}

			AGravityGameModeBase* GravityMode = Cast<AGravityGameModeBase>(World->GetAuthGameMode());
			if (!GravityMode)
			{
				return Fail(TEXT("L_WorldGeneration is not using GravityGameMode"));
			}
			if (GameplayState->bIsLoadingMode || GameplayState->bUseAuthoredSinglePlayWorld
				|| !GameplayState->bSpawnGeneratedCivilization || !GameplayState->SaveSlotName.IsEmpty())
			{
				return Fail(TEXT("GameInstance route flags do not describe a fresh generated civilization"));
			}
			if (GameplayState->NewGeneratedWorld == EditableGeneratedWorldAddress
				|| GameplayState->SpawnParameters == EditableSpawnParametersAddress)
			{
				return Fail(TEXT("menu-owned models were not frozen into independent GameInstance snapshots"));
			}
			const UGeneratedWorld* Snapshot = GameplayState->NewGeneratedWorld;
			if (Snapshot->AstroGenerationLevel != EAstroGenerationLevel::StarCluster
				|| !Snapshot->bGenerateFullScaledWorld || !Snapshot->bGenerateHomeSystem
				|| !Snapshot->bStartWithHomePlanet || Snapshot->StarType != EStarType::SingleStar
				|| Snapshot->PlanetsAmount != 1 || Snapshot->MoonsAmount != 0
				|| Snapshot->StartPlanetIndex != 1 || Snapshot->PlanetType != ExpectedPlanetType
				|| Snapshot->PlanetSurfaceSeed != 424242)
			{
				return Fail(TEXT("committed generated-world snapshot differs from deterministic menu model"));
			}
			if (!GameplayState->SpawnParameters
				|| GameplayState->SpawnParameters->CharacterSpawnPlace
					!= ECharSpawnPlace::PlanetSurface)
			{
				return Fail(TEXT("committed spawn snapshot did not preserve PlanetSurface"));
			}
			UClass* GameModePawnClass = GravityMode->GetDefaultPawnClassForController(PlayerController);
			if (!GameplayState->SpawnParameters->BP_CharacterClass || !GameModePawnClass
				|| GameplayState->SpawnParameters->BP_CharacterClass.Get()->GetPathName()
					!= SelectedPawnClassPath
				|| PlayerPawn->GetClass()->GetPathName() != SelectedPawnClassPath
				|| GameModePawnClass->GetPathName() != SelectedPawnClassPath)
			{
				return Fail(FString::Printf(
					TEXT("selected pawn handoff mismatch selected=%s snapshot=%s possessed=%s"),
					*SelectedPawnClassPath,
					*GetNameSafe(GameplayState->SpawnParameters->BP_CharacterClass.Get()),
					*GetNameSafe(PlayerPawn->GetClass())));
			}
			AGravityPlayerController* GravityController =
				Cast<AGravityPlayerController>(PlayerController);
			ACustomGravityCharacter* GravityPawn =
				Cast<ACustomGravityCharacter>(PlayerPawn);
			if (!GravityController || !GravityPawn)
			{
				return Fail(FString::Printf(
					TEXT("generated handoff controller/pawn types mismatch controller=%s pawn=%s"),
					*GetNameSafe(PlayerController ? PlayerController->GetClass() : nullptr),
					*GetNameSafe(PlayerPawn ? PlayerPawn->GetClass() : nullptr)));
			}
			UCameraComponent* PawnCamera = FindPawnCamera(GravityPawn);
			USpringArmComponent* PawnSpringArm = FindPawnSpringArm(GravityPawn);
			UCapsuleComponent* PawnCapsuleForCamera = FindPawnCapsule(GravityPawn);
			if (GravityController->GetViewTarget() != PlayerPawn
				|| !IsValid(GravityController->PlayerCameraManager)
				|| !IsValid(PawnCamera)
				|| !IsValid(PawnSpringArm)
				|| !IsValid(PawnCapsuleForCamera)
				|| !PawnCamera->IsRegistered()
				|| !PawnCamera->IsActive()
				|| !IsFiniteTransform(PawnCamera->GetComponentTransform()))
			{
				return Fail(FString::Printf(
					TEXT("possessed pawn camera contract mismatch viewTarget=%s pawn=%s manager=%s camera=%s registered=%d active=%d"),
					*GetNameSafe(GravityController->GetViewTarget()), *GetNameSafe(PlayerPawn),
					*GetNameSafe(GravityController->PlayerCameraManager),
					*GetNameSafe(PawnCamera),
					IsValid(PawnCamera) && PawnCamera->IsRegistered() ? 1 : 0,
					IsValid(PawnCamera) && PawnCamera->IsActive() ? 1 : 0));
			}
			const double CameraCapsuleHalfHeightCm =
				PawnCapsuleForCamera->GetScaledCapsuleHalfHeight();
			const double MaximumThirdPersonDistanceCm = FMath::Max(
				800.0, CameraCapsuleHalfHeightCm * 8.0);
			const double PawnCameraDistanceCm = FVector::Distance(
				PawnCamera->GetComponentLocation(), GravityPawn->GetActorLocation());
			const double ManagedCameraDistanceCm = FVector::Distance(
				GravityController->PlayerCameraManager->GetCameraLocation(),
				GravityPawn->GetActorLocation());
			const double CameraGravityUpAlignment = FVector::DotProduct(
				PawnCamera->GetUpVector().GetSafeNormal(),
				GravityPawn->GetGravityUpVector().GetSafeNormal());
			if (PawnCameraDistanceCm > MaximumThirdPersonDistanceCm
				|| ManagedCameraDistanceCm > MaximumThirdPersonDistanceCm
				|| PawnSpringArm->TargetArmLength > MaximumThirdPersonDistanceCm
				|| PawnSpringArm->CameraLagMaxDistance > MaximumThirdPersonDistanceCm
				|| !PawnSpringArm->GetRelativeScale3D().Equals(FVector::OneVector, 0.001)
				|| !PawnCamera->GetRelativeScale3D().Equals(FVector::OneVector, 0.001)
				|| CameraGravityUpAlignment < 0.95)
			{
				return Fail(FString::Printf(
					TEXT("BP_CustomGravityCharacter camera escaped character scale pawnDistance=%.2fcm managerDistance=%.2fcm maximum=%.2fcm arm=%.2fcm maxLag=%.2fcm boomScale=%s cameraScale=%s gravityUpDot=%.5f"),
					PawnCameraDistanceCm, ManagedCameraDistanceCm,
					MaximumThirdPersonDistanceCm, PawnSpringArm->TargetArmLength,
					PawnSpringArm->CameraLagMaxDistance,
					*PawnSpringArm->GetRelativeScale3D().ToCompactString(),
					*PawnCamera->GetRelativeScale3D().ToCompactString(),
					CameraGravityUpAlignment));
			}

			AAstroGenerator* Generator = Generators[0];
			AGalaxy* Galaxy = Galaxies[0];
			AStarCluster* Cluster = Clusters[0];
			AStarSystem* StarSystem = StarSystems[0];
			AStar* Star = Stars[0];
			APlanetarySystem* PlanetarySystem = PlanetarySystems[0];
			APlanetOrbit* Orbit = PlanetOrbits[0];
			APlanet* Planet = Planets[0];
			ASpaceHeadquarters* HeadquartersActor = Headquarters[0];
			ASpaceStation* StationActor = PlainStations[0];
			ASpaceShipyard* ShipyardActor = Shipyards[0];
			ASpaceship* SpaceshipActor = nullptr;
			int32 MatchingHomeShipCount = 0;
			for (ASpaceship* Candidate : Spaceships)
			{
				if (Candidate != PlayerPawn
					&& Candidate->GetAttachParentActor() == ShipyardActor
					&& Candidate->IsA(GameplayState->SpawnParameters->BP_HomeSpaceship))
				{
					SpaceshipActor = Candidate;
					++MatchingHomeShipCount;
				}
			}
			if (!IsValid(SpaceshipActor) || MatchingHomeShipCount != 1)
			{
				return Fail(FString::Printf(
					TEXT("expected exactly one generated home spaceship attached to shipyard, found %d (all ASpaceship actors=%d)"),
					MatchingHomeShipCount, Spaceships.Num()));
			}
			const FClusterStarSystemRecord* MaterializedHomeRecord = nullptr;
			int32 MatchingHomeRecordCount = 0;
			for (const FClusterStarSystemRecord& CandidateRecord : Cluster->PotentialStarSystems)
			{
				if (CandidateRecord.StableId == StarSystem->StableSystemId)
				{
					MaterializedHomeRecord = &CandidateRecord;
					++MatchingHomeRecordCount;
				}
			}

			FTransform HomeProxyLocalTransform;
			const bool bHasHomeProxyTransform = MaterializedHomeRecord
				&& IsValid(Cluster->StarMeshInstances)
				&& MaterializedHomeRecord->InstanceIndex >= 0
				&& MaterializedHomeRecord->InstanceIndex
					< Cluster->StarMeshInstances->GetInstanceCount()
				&& Cluster->StarMeshInstances->GetInstanceTransform(
					MaterializedHomeRecord->InstanceIndex, HomeProxyLocalTransform, false);
			FAPSCanonicalStellarProxyRecord HomeProjectionRecord;
			const bool bHasHomeProjectionRecord = MaterializedHomeRecord
				&& Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					MaterializedHomeRecord->InstanceIndex, HomeProjectionRecord);
			const bool bCanonicalHomeRecord = bHasHomeProjectionRecord
				&& MaterializedHomeRecord
				&& MaterializedHomeRecord->bMaterialized
				&& MaterializedHomeRecord->MaterializedSystem.Get() == StarSystem
				&& MaterializedHomeRecord->SystemModel.StableId == StarSystem->StableSystemId
				&& Cluster->MakeStableSystemId(MaterializedHomeRecord->InstanceIndex)
					== StarSystem->StableSystemId
				&& MaterializedHomeRecord->SystemModel.Location.Equals(
					MaterializedHomeRecord->ClusterLocalLocation, 0.01)
				&& bHasHomeProxyTransform
				&& HomeProxyLocalTransform.GetLocation().Equals(
					HomeProjectionRecord.ExpectedBaseProxyPositionCm, 0.01)
				&& HomeProjectionRecord.StableId == MaterializedHomeRecord->StableId
				&& HomeProjectionRecord.CanonicalPositionUnits.Equals(
					MaterializedHomeRecord->ClusterLocalLocation, 0.01)
				&& HomeProjectionRecord.bSuppressedMaterializedHome
				&& HomeProjectionRecord.ProjectionErrorCm <=
					APSCanonicalStellarProjection::ProjectionToleranceCm
				&& HomeProxyLocalTransform.GetScale3D() == FVector::ZeroVector;
			if (MatchingHomeRecordCount != 1 || !bCanonicalHomeRecord)
			{
				return Fail(FString::Printf(
					TEXT("detached gameplay home system does not retain one canonical suppressed cluster proxy: records=%d record=%d materialized=%d actor=%d stable=%d address=%d proxy=%d proxyScale=%s"),
					MatchingHomeRecordCount, MaterializedHomeRecord ? 1 : 0,
					MaterializedHomeRecord && MaterializedHomeRecord->bMaterialized ? 1 : 0,
					MaterializedHomeRecord
						&& MaterializedHomeRecord->MaterializedSystem.Get() == StarSystem ? 1 : 0,
					MaterializedHomeRecord
						&& MaterializedHomeRecord->SystemModel.StableId == StarSystem->StableSystemId ? 1 : 0,
					MaterializedHomeRecord
						&& MaterializedHomeRecord->SystemModel.Location.Equals(
							MaterializedHomeRecord->ClusterLocalLocation, 0.01) ? 1 : 0,
					bHasHomeProxyTransform ? 1 : 0,
					bHasHomeProxyTransform
						? *HomeProxyLocalTransform.GetScale3D().ToCompactString() : TEXT("None")));
			}

			const FAPSCanonicalStellarProjectionDescriptor& Projection =
				Generator->GetCanonicalStellarProjectionDescriptor();
			FAPSCanonicalStellarProxyRecord GalaxySentinel;
			const bool bHasGalaxySentinel = PreviewGalaxyStableId.IsValid()
				&& Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::Galaxy,
					PreviewGalaxyStableId, GalaxySentinel);
			FAPSCanonicalStellarProxyRecord ClusterSentinel;
			const bool bHasClusterSentinel = PreviewClusterStableId.IsValid()
				&& Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					PreviewClusterStableId, ClusterSentinel);
			FAPSCanonicalClusterSystemAddress HomeAddress;
			FAPSCanonicalClusterSystemAddress ClusterAddress;
			const FClusterStarSystemRecord* ClusterAddressRecord = bHasClusterSentinel
				? Cluster->FindPotentialSystem(ClusterSentinel.InstanceIndex) : nullptr;
			const bool bHasCanonicalAddresses = ClusterAddressRecord
				&& Generator->ResolveCanonicalClusterSystemAddress(
					PreviewHomeStableId, HomeAddress)
				&& Generator->ResolveCanonicalClusterSystemAddress(
					PreviewClusterStableId, ClusterAddress);
			FAPSCanonicalClusterSystemRecord DatasetOnlyRecord;
			const bool bHasGameplayDatasetOnlyRecord = !bHasPreviewDatasetOnlyRecord
				|| Generator->GetCanonicalClusterDatasetRecord(
					PreviewDatasetOnlyStableId, DatasetOnlyRecord);
			const bool bUnitProjectionRoots = Generator->GetActorScale3D().Equals(
				FVector::OneVector, 1.0e-6)
				&& Galaxy->GetActorScale3D().Equals(FVector::OneVector, 1.0e-6)
				&& Cluster->GetActorScale3D().Equals(FVector::OneVector, 1.0e-6)
				&& Galaxy->StarMeshInstances->GetComponentScale().Equals(
					FVector::OneVector, 1.0e-6)
				&& Cluster->StarMeshInstances->GetComponentScale().Equals(
					FVector::OneVector, 1.0e-6);
			const uint64 ExpectedUploadCount =
				(IsValid(Galaxy->StarMeshInstances) ? 1u : 0u)
				+ (IsValid(Cluster->StarMeshInstances) ? 1u : 0u);
			const bool bPreviewGameplayProjectionParity = bHasPreviewProjectionSnapshot
				&& Projection.ProjectionVersion == PreviewProjectionVersion
				&& Projection.ContextHash == PreviewProjectionContextHash
				&& Projection.CanonicalDatasetHash == PreviewCanonicalDatasetHash
				&& Projection.CanonicalDatasetVersion == PreviewCanonicalDatasetVersion
				&& Projection.CanonicalDatasetInputHash == PreviewCanonicalDatasetInputHash
				&& Projection.CanonicalDatasetBuildSerial == PreviewCanonicalDatasetBuildSerial
				&& Projection.CanonicalDatasetRecordCount == PreviewCanonicalDatasetRecordCount
				&& Projection.bConsumedFinalizedDataset
				&& bHasGameplayDatasetOnlyRecord
				&& (!bHasPreviewDatasetOnlyRecord
					|| (DatasetOnlyRecord.StableId == PreviewDatasetOnlyStableId
						&& DatasetOnlyRecord.ClusterLocalLocation.Equals(
							PreviewDatasetOnlyCanonicalPosition, 0.01)
						&& FMath::IsNearlyEqual(
							DatasetOnlyRecord.PrimaryStarModel.Radius,
							PreviewDatasetOnlyCanonicalRadiusSolar, 1.0e-9)
						&& FMath::IsNearlyEqual(
							DatasetOnlyRecord.PrimaryStarModel.MinOrbit,
							PreviewDatasetOnlyMinOrbit, 1.0e-9)
						&& FMath::IsNearlyEqual(
							DatasetOnlyRecord.PrimaryStarModel.MaxOrbit,
							PreviewDatasetOnlyMaxOrbit, 1.0e-9)))
				&& HomeProjectionRecord.StableId == PreviewHomeStableId
				&& HomeProjectionRecord.CanonicalPositionUnits.Equals(
					PreviewHomeCanonicalPosition, 0.01)
				&& HomeProjectionRecord.ExpectedBaseProxyPositionCm.Equals(
					PreviewHomeExpectedProxyPosition, 0.01)
				&& FMath::IsNearlyEqual(HomeProjectionRecord.CanonicalPhysicalRadiusSolar,
					PreviewHomeCanonicalRadiusSolar, 1.0e-9)
				&& GalaxySentinel.StableId == PreviewGalaxyStableId
				&& GalaxySentinel.CanonicalPositionUnits.Equals(
					PreviewGalaxyCanonicalPosition, 0.01)
				&& GalaxySentinel.ExpectedBaseProxyPositionCm.Equals(
					PreviewGalaxyExpectedProxyPosition, 0.01)
				&& FMath::IsNearlyEqual(GalaxySentinel.CanonicalPhysicalRadiusSolar,
					PreviewGalaxyCanonicalRadiusSolar, 1.0e-9)
				&& ClusterSentinel.StableId == PreviewClusterStableId
				&& ClusterSentinel.CanonicalPositionUnits.Equals(
					PreviewClusterCanonicalPosition, 0.01)
				&& ClusterSentinel.ExpectedBaseProxyPositionCm.Equals(
					PreviewClusterExpectedProxyPosition, 0.01)
				&& FMath::IsNearlyEqual(ClusterSentinel.CanonicalPhysicalRadiusSolar,
					PreviewClusterCanonicalRadiusSolar, 1.0e-9);
			const bool bProjectionContract = Projection.bFinalized
				&& Projection.bProjectionValid
				&& Projection.ProjectionVersion
					== FAPSCanonicalStellarProjectionFrame::CurrentVersion
				&& Projection.ContextHash != 0u
				&& Projection.CanonicalDatasetHash != 0u
				&& Projection.CanonicalIdentityHash == Projection.CanonicalDatasetHash
				&& Projection.CanonicalDatasetVersion
					== FAPSCanonicalStellarDataset::CurrentVersion
				&& Projection.CanonicalDatasetRecordCount == Projection.ClusterModeledCount
				&& Projection.RenderedMappingHash != 0u
				&& Projection.InstanceUploadCount == ExpectedUploadCount
				&& Projection.bMappingsComplete
				&& Projection.bUnitRoots
				&& Projection.bBoundsValid
				&& Projection.Galaxy.bEnabled
				&& Projection.StarCluster.bEnabled
				&& Projection.Galaxy.PositionScale
					== Projection.StarCluster.PositionScale
				&& FMath::IsNearlyEqual(
					Projection.StarCluster.LayerToRootPositionScale,
					Projection.ClusterToGalaxyPositionScale, 1.0e-12)
				&& FMath::IsNearlyEqual(
					Projection.Galaxy.VisualRadiusFloorFraction,
					APSCanonicalStellarProjection::GalaxyImpostorFloorFraction, 1.0e-12)
				&& FMath::IsNearlyEqual(
					Projection.StarCluster.VisualRadiusFloorFraction,
					APSCanonicalStellarProjection::ClusterImpostorFloorFraction, 1.0e-12)
				&& FMath::IsNearlyEqual(
					Projection.Galaxy.VisualRadiusCeilingFraction,
					APSCanonicalStellarProjection::ImpostorCeilingFraction, 1.0e-12)
				&& Projection.Galaxy.MaxProxyCoordinateCm
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
				&& Projection.StarCluster.MaxProxyCoordinateCm
					<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
				&& Projection.MaxObservedMatrixMagnitudeCm
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
				&& Projection.GalaxyMaxObservedMatrixMagnitudeCm
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
				&& Projection.ClusterMaxObservedMatrixMagnitudeCm
					<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
				&& Projection.MaterializedHomeStableId == StarSystem->StableSystemId
				&& Projection.MaterializedHomeInstanceIndex
					== MaterializedHomeRecord->InstanceIndex
				&& Projection.bMaterializedHomeProxySuppressed
				&& bHasCanonicalAddresses
				&& HomeAddress.bMaterializedHome
				&& FMath::IsNearlyZero(HomeAddress.CanonicalDistanceFromHomeCm, 0.01)
				&& !ClusterAddress.bMaterializedHome
				&& ClusterAddress.CanonicalDistanceFromHomeCm > 0.01
				&& FMath::IsNearlyEqual(
					ClusterAddress.CanonicalDistanceFromHomeCm,
					ClusterAddress.CanonicalDeltaFromHomeCm.Size(), 0.01)
				&& ClusterAddress.ImmutableProxyWorldLocationCm.Equals(
					Cluster->GetPotentialSystemWorldLocation(*ClusterAddressRecord), 0.01)
				&& bPreviewGameplayProjectionParity
				&& bUnitProjectionRoots
				&& bHasGalaxySentinel
				&& GalaxySentinel.StableId.IsValid()
				&& GalaxySentinel.ProjectionErrorCm
					<= APSCanonicalStellarProjection::ProjectionToleranceCm
				&& APSCanonicalStellarProjection::MaxAbsComponent(
					GalaxySentinel.ActualProxyPositionCm)
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
				&& bHasClusterSentinel
				&& ClusterSentinel.StableId == PreviewClusterStableId
				&& ClusterSentinel.CanonicalPositionUnits.Equals(
					PreviewClusterCanonicalPosition, 0.01)
				&& !ClusterSentinel.bSuppressedMaterializedHome
				&& ClusterSentinel.ActualProxyScale > 0.0
				&& ClusterSentinel.ProjectionErrorCm
					<= APSCanonicalStellarProjection::ProjectionToleranceCm
				&& APSCanonicalStellarProjection::MaxAbsComponent(
					ClusterSentinel.ActualProxyPositionCm)
					<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm;
			if (!bProjectionContract)
			{
				return Fail(FString::Printf(
					TEXT("canonical stellar projection contract failed: finalized=%d valid=%d version=%u context=%u dataset=%u datasetVersion=%u input=%u datasetBuild=%llu records=%d consumed=%d mapping=%u uploads=%llu expectedUploads=%llu mutations=%llu parity=%d roots=%d matrix=%.6e galaxyMatrix=%.6e clusterMatrix=%.6e galaxy=%d gError=%.6f gPos=%.6e cluster=%d cError=%.6f cPos=%.6e cScale=%.6e home=%s/%d suppressed=%d"),
					Projection.bFinalized ? 1 : 0, Projection.bProjectionValid ? 1 : 0,
					Projection.ProjectionVersion,
					Projection.ContextHash, Projection.CanonicalDatasetHash,
					Projection.CanonicalDatasetVersion,
					Projection.CanonicalDatasetInputHash,
					Projection.CanonicalDatasetBuildSerial,
					Projection.CanonicalDatasetRecordCount,
					Projection.bConsumedFinalizedDataset ? 1 : 0,
					Projection.RenderedMappingHash,
					Projection.InstanceUploadCount, ExpectedUploadCount,
					Projection.TransformMutationSerial,
					bPreviewGameplayProjectionParity ? 1 : 0,
					bUnitProjectionRoots ? 1 : 0, Projection.MaxObservedMatrixMagnitudeCm,
					Projection.GalaxyMaxObservedMatrixMagnitudeCm,
					Projection.ClusterMaxObservedMatrixMagnitudeCm,
					bHasGalaxySentinel ? 1 : 0, GalaxySentinel.ProjectionErrorCm,
					APSCanonicalStellarProjection::MaxAbsComponent(
						GalaxySentinel.ActualProxyPositionCm),
					bHasClusterSentinel ? 1 : 0, ClusterSentinel.ProjectionErrorCm,
					APSCanonicalStellarProjection::MaxAbsComponent(
						ClusterSentinel.ActualProxyPositionCm), ClusterSentinel.ActualProxyScale,
					*Projection.MaterializedHomeStableId.ToString(EGuidFormats::Digits),
					Projection.MaterializedHomeInstanceIndex,
					Projection.bMaterializedHomeProxySuppressed ? 1 : 0));
			}
			if (Galaxy->GetAttachParentActor() != Generator
				|| Cluster->GetAttachParentActor() != Galaxy
				|| StarSystem->GetAttachParentActor() != nullptr
				|| Star->GetAttachParentActor() != StarSystem
				|| PlanetarySystem->GetAttachParentActor() != Star
				|| Orbit->GetAttachParentActor() != PlanetarySystem
				|| Planet->GetAttachParentActor() != Orbit)
			{
				return Fail(FString::Printf(
					TEXT("gameplay catalog/local-bubble hierarchy mismatch: galaxyParent=%s clusterParent=%s systemParent=%s starParent=%s planetaryParent=%s orbitParent=%s planetParent=%s"),
					*GetNameSafe(Galaxy->GetAttachParentActor()),
					*GetNameSafe(Cluster->GetAttachParentActor()),
					*GetNameSafe(StarSystem->GetAttachParentActor()),
					*GetNameSafe(Star->GetAttachParentActor()),
					*GetNameSafe(PlanetarySystem->GetAttachParentActor()),
					*GetNameSafe(Orbit->GetAttachParentActor()),
					*GetNameSafe(Planet->GetAttachParentActor())));
			}
			if (!StarSystem->bMaterializedFromCluster || !StarSystem->StableSystemId.IsValid()
				|| StarSystem->GetStars().Num() != 1 || StarSystem->GetStars()[0] != Star
				|| StarSystem->MainStar != Star || Star->PlanetarySystem != PlanetarySystem
				|| PlanetarySystem->PlanetOrbitsList.Num() != 1
				|| PlanetarySystem->PlanetsActorsList.Num() != 1
				|| PlanetarySystem->PlanetOrbitsList[0] != Orbit
				|| PlanetarySystem->PlanetsActorsList[0] != Planet || Orbit->Planet != Planet
				|| Generator->GeneratedStarSystems.Num() != 1
				|| Generator->GeneratedStarSystems[0] != StarSystem
				|| Generator->HomeStar != Star || Generator->HomePlanetarySystem != PlanetarySystem
				|| Generator->HomePlanet != Planet)
			{
				return Fail(FString::Printf(
					TEXT("single-star/single-planet materialized records are inconsistent: materialized=%d stable=%d stars=%d main=%d starPS=%d orbits=%d planets=%d orbit0=%d planet0=%d orbitPlanet=%d registry=%d registry0=%d homeStar=%d homePS=%d homePlanet=%d"),
					StarSystem->bMaterializedFromCluster ? 1 : 0,
					StarSystem->StableSystemId.IsValid() ? 1 : 0,
					StarSystem->GetStars().Num(), StarSystem->MainStar == Star ? 1 : 0,
					Star->PlanetarySystem == PlanetarySystem ? 1 : 0,
					PlanetarySystem->PlanetOrbitsList.Num(), PlanetarySystem->PlanetsActorsList.Num(),
					PlanetarySystem->PlanetOrbitsList.Num() == 1 && PlanetarySystem->PlanetOrbitsList[0] == Orbit ? 1 : 0,
					PlanetarySystem->PlanetsActorsList.Num() == 1 && PlanetarySystem->PlanetsActorsList[0] == Planet ? 1 : 0,
					Orbit->Planet == Planet ? 1 : 0,
					Generator->GeneratedStarSystems.Num(),
					Generator->GeneratedStarSystems.Num() == 1 && Generator->GeneratedStarSystems[0] == StarSystem ? 1 : 0,
					Generator->HomeStar == Star ? 1 : 0,
					Generator->HomePlanetarySystem == PlanetarySystem ? 1 : 0,
					Generator->HomePlanet == Planet ? 1 : 0));
			}
			if (HeadquartersActor->GetAttachParentActor() != Planet
				|| StationActor->GetAttachParentActor() != HeadquartersActor
				|| ShipyardActor->GetAttachParentActor() != HeadquartersActor
				|| SpaceshipActor->GetAttachParentActor() != ShipyardActor
				|| HeadquartersActor->Civilization != GameplayState->CurrentCivilization
				|| !HeadquartersActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceHeadquarters)
				|| !StationActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceStation)
				|| !ShipyardActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceShipyard)
				|| !SpaceshipActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceship))
			{
				return Fail(TEXT("starter HQ/station/shipyard/spaceship classes or attachments mismatch"));
			}
			const bool bFinitePawnTransform = IsFiniteTransform(PlayerPawn->GetActorTransform());
			const bool bFiniteStationTransform = IsFiniteTransform(StationActor->GetActorTransform());
			const bool bValidSpawnPoint = IsValid(StationActor->SpawnPoint)
				&& StationActor->SpawnPoint->IsRegistered()
				&& IsFiniteTransform(StationActor->SpawnPoint->GetComponentTransform());
			const FVector SpawnPointLocation = bValidSpawnPoint
				? StationActor->SpawnPoint->GetComponentLocation() : FVector::ZeroVector;
			const double PawnToSpawnPointCm = bValidSpawnPoint && bFinitePawnTransform
				? FVector::Distance(PlayerPawn->GetActorLocation(), SpawnPointLocation)
				: TNumericLimits<double>::Max();
			const int32 CharacterRenderComponentCount =
				CountVisibleRegisteredRenderComponents(GravityPawn);
			const int32 StationRenderComponentCount =
				CountVisibleRegisteredRenderComponents(StationActor);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] controller=%s pawn=%s viewTarget=%s pawnLocation=%s stationLocation=%s spawnPoint=%s pawnToSpawn=%.2fcm cameraLocation=%s cameraRotation=%s cameraDistance=%.2fcm managerDistance=%.2fcm arm=%.2fcm maxLag=%.2fcm gravityUpDot=%.5f characterRenderers=%d stationRenderers=%d"),
				*GetNameSafe(GravityController->GetClass()), *GetNameSafe(PlayerPawn),
				*GetNameSafe(GravityController->GetViewTarget()),
				*PlayerPawn->GetActorLocation().ToCompactString(),
				*StationActor->GetActorLocation().ToCompactString(),
				*SpawnPointLocation.ToCompactString(), PawnToSpawnPointCm,
				*GravityController->PlayerCameraManager->GetCameraLocation().ToCompactString(),
				*GravityController->PlayerCameraManager->GetCameraRotation().ToCompactString(),
				PawnCameraDistanceCm, ManagedCameraDistanceCm,
				PawnSpringArm->TargetArmLength, PawnSpringArm->CameraLagMaxDistance,
				CameraGravityUpAlignment,
				CharacterRenderComponentCount, StationRenderComponentCount);
			if (!bFinitePawnTransform || !bFiniteStationTransform || !bValidSpawnPoint
				|| Generator->CharSpawnPlace != ECharSpawnPlace::PlanetSurface)
			{
				return Fail(FString::Printf(
					TEXT("starter spawn contract mismatch finitePawn=%d finiteStation=%d validStationSpawnPoint=%d pawnToStation=%.2fcm charSpawnPlace=%d"),
					bFinitePawnTransform ? 1 : 0, bFiniteStationTransform ? 1 : 0,
					bValidSpawnPoint ? 1 : 0, PawnToSpawnPointCm,
					static_cast<int32>(Generator->CharSpawnPlace)));
			}
			if (CharacterRenderComponentCount == 0 || StationRenderComponentCount == 0)
			{
				return Fail(FString::Printf(
					TEXT("starter character/station has no visible registered render asset character=%d station=%d"),
					CharacterRenderComponentCount, StationRenderComponentCount));
			}
			if (!IsValid(StationActor->GravityCollisionZone)
				|| StationActor->GravityCollisionZone->IsVisible()
				|| !StationActor->GravityCollisionZone->bHiddenInGame
				|| !IsValid(ShipyardActor->GravityCollisionZone)
				|| ShipyardActor->GravityCollisionZone->IsVisible()
				|| !ShipyardActor->GravityCollisionZone->bHiddenInGame)
			{
				return Fail(TEXT("station gravity collision volume is rendering in gameplay"));
			}

			Star->StarMesh->UpdateBounds();
			const bool bPhotospherePresented = IsEffectivelyPresented(Star->StarMesh)
				&& IsValid(Star->StarMesh->GetStaticMesh())
				&& IsValid(Star->StarMesh->GetMaterial(0));
			const bool bPlanetaryEnvelopeNonBlocking = IsValid(Star->PlanetarySystemZone)
				&& Star->PlanetarySystemZone->GetCollisionEnabled()
					== ECollisionEnabled::NoCollision
				&& !Star->PlanetarySystemZone->GetGenerateOverlapEvents()
				&& IgnoresEveryCollisionChannel(Star->PlanetarySystemZone);
			const bool bSystemEnvelopeNonBlocking = IsValid(StarSystem->StarSystemZone)
				&& StarSystem->StarSystemZone->GetCollisionEnabled()
					== ECollisionEnabled::NoCollision
				&& !StarSystem->StarSystemZone->GetGenerateOverlapEvents()
				&& IgnoresEveryCollisionChannel(StarSystem->StarSystemZone);
			const UAPSStellarVisualSubsystem* StellarVisuals =
				World->GetSubsystem<UAPSStellarVisualSubsystem>();
			FVector ActiveTargetLocation = FVector::ZeroVector;
			FString ActiveTargetIdentity;
			const bool bPhysicalStellarTarget = IsValid(StellarVisuals)
				&& StellarVisuals->GetActiveStellarTarget(
					ActiveTargetLocation, ActiveTargetIdentity)
				&& ActiveTargetLocation.Equals(Star->GetActorLocation(), 1.0)
				&& ActiveTargetIdentity == Star->GetPathName();
			if (!bPhysicalStellarTarget && Now - StepStartSeconds < 2.0)
			{
				return false;
			}
			const UDirectionalLightComponent* ActiveStellarKey = nullptr;
			for (TActorIterator<ADirectionalLight> It(World); It; ++It)
			{
				const ADirectionalLight* Candidate = *It;
				const UDirectionalLightComponent* Component = Candidate
					? Cast<UDirectionalLightComponent>(Candidate->GetLightComponent()) : nullptr;
				if (IsValid(Candidate) && IsValid(Component) && Component->IsVisible()
					&& Component->Intensity > 0.0f
					&& !Candidate->ActorHasTag(TEXT("APSPreviewFillLight"))
					&& !Candidate->ActorHasTag(TEXT("APSGameplaySurfaceFillLight")))
				{
					ActiveStellarKey = Component;
					break;
				}
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Stellar] star=%s location=%s target=%s targetIdentity=%s physicalTarget=%d photosphere=%d radius=%.3ecm key=%s keyIntensity=%.2f planetaryZoneCollision=%d systemZoneCollision=%d"),
				*GetNameSafe(Star), *Star->GetActorLocation().ToCompactString(),
				*ActiveTargetLocation.ToCompactString(), *ActiveTargetIdentity,
				bPhysicalStellarTarget ? 1 : 0,
				bPhotospherePresented ? 1 : 0, Star->StarMesh->Bounds.SphereRadius,
				*GetNameSafe(ActiveStellarKey), ActiveStellarKey ? ActiveStellarKey->Intensity : 0.0f,
				IsValid(Star->PlanetarySystemZone)
					? static_cast<int32>(Star->PlanetarySystemZone->GetCollisionEnabled()) : -1,
				IsValid(StarSystem->StarSystemZone)
					? static_cast<int32>(StarSystem->StarSystemZone->GetCollisionEnabled()) : -1);
			if (!bPhysicalStellarTarget || !bPhotospherePresented || !ActiveStellarKey
				|| !bPlanetaryEnvelopeNonBlocking || !bSystemEnvelopeNonBlocking)
			{
				return Fail(FString::Printf(
					TEXT("generated stellar gameplay contract failed physicalTarget=%d photosphere=%d directionalKey=%d planetaryEnvelopeNonBlocking=%d systemEnvelopeNonBlocking=%d"),
					bPhysicalStellarTarget ? 1 : 0, bPhotospherePresented ? 1 : 0,
					ActiveStellarKey ? 1 : 0, bPlanetaryEnvelopeNonBlocking ? 1 : 0,
					bSystemEnvelopeNonBlocking ? 1 : 0));
			}

			RuntimeGenerator = Generator;
			RuntimeHomePlanet = Planet;
			RuntimeGravityPawn = GravityPawn;
			RuntimeStation = StationActor;
			GeneratedSaveSlotName = GravityController->CurrentSaveSlotName;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Gameplay hierarchy ready in %.2fs system=%s planet=%s pawn=%s saveSlot=%s starterAttachments=OK"),
				Now - StepStartSeconds, *StarSystem->StableSystemId.ToString(),
				*GetNameSafe(Planet), *SelectedPawnClassPath, *GeneratedSaveSlotName);
			if (CVarAPSTestsFullScaleProjectionEvidence.GetValueOnGameThread() > 0)
			{
				FString EvidenceFailure;
				if (!InitializeProjectionEvidence(World, Generator, Galaxy, Cluster,
					PlayerController, GalaxySentinel, ClusterSentinel, EvidenceFailure))
				{
					return Fail(EvidenceFailure);
				}
				Step = EStep::ProjectionEvidenceStationaryHold;
			}
			else
			{
				Step = EStep::WaitForGameplaySurface;
			}
			StepStartSeconds = Now;
			return false;
		}

		static bool ProjectionFramesMatch(
			const FAPSCanonicalStellarProjectionFrame& A,
			const FAPSCanonicalStellarProjectionFrame& B)
		{
			return A.ProjectionVersion == B.ProjectionVersion
				&& A.ContextHash == B.ContextHash
				&& A.LayerOriginCanonicalUnits == B.LayerOriginCanonicalUnits
				&& A.LayerToRootPositionScale == B.LayerToRootPositionScale
				&& A.CanonicalAnchorCm == B.CanonicalAnchorCm
				&& A.RenderAnchorCm == B.RenderAnchorCm
				&& A.CanonicalCmPerUnit == B.CanonicalCmPerUnit
				&& A.PositionScale == B.PositionScale
				&& A.CanonicalHalfExtentUnits == B.CanonicalHalfExtentUnits
				&& A.ProxyHalfExtentCm == B.ProxyHalfExtentCm
				&& A.MaxProxyCoordinateCm == B.MaxProxyCoordinateCm
				&& A.RadiusPolicy == B.RadiusPolicy
				&& A.VisualRadiusFloorFraction == B.VisualRadiusFloorFraction
				&& A.VisualRadiusCeilingFraction == B.VisualRadiusCeilingFraction
				&& A.VisualRadiusClassExponent == B.VisualRadiusClassExponent
				&& A.VisualRadiusMinClassScale == B.VisualRadiusMinClassScale
				&& A.VisualRadiusMaxClassScale == B.VisualRadiusMaxClassScale
				&& A.bEnabled == B.bEnabled;
		}

		bool ProjectionDescriptorMatchesBaseline(
			const FAPSCanonicalStellarProjectionDescriptor& Current) const
		{
			const FAPSCanonicalStellarProjectionDescriptor& Baseline =
				ProjectionEvidenceBaselineDescriptor;
			return Current.ProjectionVersion == Baseline.ProjectionVersion
				&& Current.ContextHash == Baseline.ContextHash
				&& Current.CanonicalDatasetHash == Baseline.CanonicalDatasetHash
				&& Current.CanonicalDatasetVersion == Baseline.CanonicalDatasetVersion
				&& Current.CanonicalDatasetInputHash == Baseline.CanonicalDatasetInputHash
				&& Current.CanonicalDatasetBuildSerial == Baseline.CanonicalDatasetBuildSerial
				&& Current.CanonicalDatasetRecordCount == Baseline.CanonicalDatasetRecordCount
				&& Current.CanonicalIdentityHash == Baseline.CanonicalIdentityHash
				&& Current.RenderedMappingHash == Baseline.RenderedMappingHash
				&& ProjectionFramesMatch(Current.Galaxy, Baseline.Galaxy)
				&& ProjectionFramesMatch(Current.StarCluster, Baseline.StarCluster)
				&& Current.ProxyBuildSerial == Baseline.ProxyBuildSerial
				&& Current.InstanceUploadCount == Baseline.InstanceUploadCount
				&& Current.TransformMutationSerial == Baseline.TransformMutationSerial
				&& Current.GalaxyModeledCount == Baseline.GalaxyModeledCount
				&& Current.GalaxyRenderedCount == Baseline.GalaxyRenderedCount
				&& Current.ClusterModeledCount == Baseline.ClusterModeledCount
				&& Current.ClusterRenderedCount == Baseline.ClusterRenderedCount
				&& Current.MaterializedHomeStableId == Baseline.MaterializedHomeStableId
				&& Current.MaterializedHomeInstanceIndex
					== Baseline.MaterializedHomeInstanceIndex
				&& Current.MaxObservedMatrixMagnitudeCm
					== Baseline.MaxObservedMatrixMagnitudeCm
				&& Current.GalaxyMaxObservedMatrixMagnitudeCm
					== Baseline.GalaxyMaxObservedMatrixMagnitudeCm
				&& Current.ClusterMaxObservedMatrixMagnitudeCm
					== Baseline.ClusterMaxObservedMatrixMagnitudeCm
				&& Current.ClusterToGalaxyPositionScale
					== Baseline.ClusterToGalaxyPositionScale
				&& Current.ViewDistanceScale == Baseline.ViewDistanceScale
				&& Current.GalaxyViewVisualScale == Baseline.GalaxyViewVisualScale
				&& Current.ClusterViewVisualScale == Baseline.ClusterViewVisualScale
				&& Current.ViewAnchorWorldCm == Baseline.ViewAnchorWorldCm
				&& Current.bMaterializedHomeProxySuppressed
					== Baseline.bMaterializedHomeProxySuppressed
				&& Current.bConsumedFinalizedDataset == Baseline.bConsumedFinalizedDataset
				&& Current.bMappingsComplete == Baseline.bMappingsComplete
				&& Current.bUnitRoots == Baseline.bUnitRoots
				&& Current.bBoundsValid == Baseline.bBoundsValid
				&& Current.bProjectionValid == Baseline.bProjectionValid
				&& Current.bFinalized == Baseline.bFinalized;
		}

		static bool ProjectProjectionEvidenceNdc(const FWorldCachedViewInfo& ViewInfo,
			const FVector& WorldPosition, FVector2D& OutNdc)
		{
			const FVector4 Clip = ViewInfo.ViewProjectionMatrix.TransformFVector4(
				FVector4(WorldPosition, 1.0));
			if (!FMath::IsFinite(Clip.X) || !FMath::IsFinite(Clip.Y)
				|| !FMath::IsFinite(Clip.W) || Clip.W <= UE_SMALL_NUMBER)
			{
				return false;
			}
			OutNdc = FVector2D(Clip.X / Clip.W, Clip.Y / Clip.W);
			return FMath::IsFinite(OutNdc.X) && FMath::IsFinite(OutNdc.Y);
		}

		bool CaptureProjectionEvidenceDatasetBaseline(
			AAstroGenerator* Generator, FString& OutFailure)
		{
			OutFailure.Reset();
			const int32 RecordCount = ProjectionEvidenceBaselineDescriptor.
				CanonicalDatasetRecordCount;
			if (!IsValid(Generator) || RecordCount <= 0)
			{
				OutFailure = TEXT("projection evidence has no finalized canonical records");
				return false;
			}
			ProjectionEvidenceDatasetStableIds.SetNum(RecordCount);
			ProjectionEvidenceDatasetCanonicalPositions.SetNum(RecordCount);
			ProjectionEvidenceDatasetCanonicalRadii.SetNum(RecordCount);
			ProjectionEvidenceDatasetMinOrbits.SetNum(RecordCount);
			ProjectionEvidenceDatasetMaxOrbits.SetNum(RecordCount);
			for (int32 RecordIndex = 0; RecordIndex < RecordCount; ++RecordIndex)
			{
				FAPSCanonicalClusterSystemRecord Record;
				if (!Generator->GetCanonicalClusterDatasetRecord(RecordIndex, Record)
					|| Record.CanonicalIndex != RecordIndex
					|| !Record.StableId.IsValid()
					|| Record.SystemModel.StableId != Record.StableId
					|| !Record.PrimaryStarModel.Location.Equals(
						Record.ClusterLocalLocation, 0.001)
					|| !Record.SystemModel.Location.Equals(
						Record.ClusterLocalLocation, 0.001))
				{
					OutFailure = FString::Printf(
						TEXT("projection evidence could not snapshot canonical record %d/%d"),
						RecordIndex, RecordCount);
					return false;
				}
				ProjectionEvidenceDatasetStableIds[RecordIndex] = Record.StableId;
				ProjectionEvidenceDatasetCanonicalPositions[RecordIndex] =
					Record.ClusterLocalLocation;
				ProjectionEvidenceDatasetCanonicalRadii[RecordIndex] =
					Record.PrimaryStarModel.Radius;
				ProjectionEvidenceDatasetMinOrbits[RecordIndex] =
					Record.PrimaryStarModel.MinOrbit;
				ProjectionEvidenceDatasetMaxOrbits[RecordIndex] =
					Record.PrimaryStarModel.MaxOrbit;
			}
			return true;
		}

		bool ValidateProjectionEvidenceDataset(
			AAstroGenerator* Generator, FString& OutFailure) const
		{
			OutFailure.Reset();
			const int32 RecordCount = ProjectionEvidenceDatasetStableIds.Num();
			if (!IsValid(Generator) || RecordCount <= 0
				|| ProjectionEvidenceDatasetCanonicalPositions.Num() != RecordCount
				|| ProjectionEvidenceDatasetCanonicalRadii.Num() != RecordCount
				|| ProjectionEvidenceDatasetMinOrbits.Num() != RecordCount
				|| ProjectionEvidenceDatasetMaxOrbits.Num() != RecordCount)
			{
				OutFailure = TEXT("projection evidence canonical baseline is incomplete");
				return false;
			}
			for (int32 RecordIndex = 0; RecordIndex < RecordCount; ++RecordIndex)
			{
				FAPSCanonicalClusterSystemRecord Record;
				if (!Generator->GetCanonicalClusterDatasetRecord(RecordIndex, Record)
					|| Record.CanonicalIndex != RecordIndex
					|| Record.StableId != ProjectionEvidenceDatasetStableIds[RecordIndex]
					|| Record.SystemModel.StableId != Record.StableId
					|| Record.ClusterLocalLocation
						!= ProjectionEvidenceDatasetCanonicalPositions[RecordIndex]
					|| Record.PrimaryStarModel.Radius
						!= ProjectionEvidenceDatasetCanonicalRadii[RecordIndex]
					|| Record.PrimaryStarModel.MinOrbit
						!= ProjectionEvidenceDatasetMinOrbits[RecordIndex]
					|| Record.PrimaryStarModel.MaxOrbit
						!= ProjectionEvidenceDatasetMaxOrbits[RecordIndex])
				{
					OutFailure = FString::Printf(
						TEXT("canonical dataset/order/address changed at record %d/%d"),
						RecordIndex, RecordCount);
					return false;
				}
			}
			return true;
		}

		static bool ProxyRecordsMatchIdentity(
			const FAPSCanonicalStellarProxyRecord& Current,
			const FAPSCanonicalStellarProxyRecord& Baseline)
		{
			return Current.Layer == Baseline.Layer
				&& Current.StableId == Baseline.StableId
				&& Current.CanonicalIndex == Baseline.CanonicalIndex
				&& Current.InstanceIndex == Baseline.InstanceIndex
				&& Current.CanonicalPositionUnits == Baseline.CanonicalPositionUnits
				&& Current.CanonicalPhysicalRadiusSolar
					== Baseline.CanonicalPhysicalRadiusSolar
				&& Current.ExpectedPhysicalProxyRadiusCm
					== Baseline.ExpectedPhysicalProxyRadiusCm
				&& Current.AppliedVisualProxyRadiusCm
					== Baseline.AppliedVisualProxyRadiusCm
				&& Current.ExpectedBaseProxyPositionCm
					== Baseline.ExpectedBaseProxyPositionCm
				&& Current.ExpectedBaseProxyScale == Baseline.ExpectedBaseProxyScale
				&& Current.ActualProxyPositionCm.Equals(
					Baseline.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& FMath::IsNearlyEqual(Current.ActualProxyScale,
					Baseline.ActualProxyScale,
					ProjectionEvidenceProxyScaleTolerance)
				&& Current.bSuppressedMaterializedHome
					== Baseline.bSuppressedMaterializedHome
				&& Current.bSuppressedByView == Baseline.bSuppressedByView;
		}

		static bool CanonicalAddressesMatch(
			const FAPSCanonicalClusterSystemAddress& Current,
			const FAPSCanonicalClusterSystemAddress& Baseline)
		{
			return Current.ProjectionVersion == Baseline.ProjectionVersion
				&& Current.ContextHash == Baseline.ContextHash
				&& Current.CanonicalDatasetHash == Baseline.CanonicalDatasetHash
				&& Current.StableId == Baseline.StableId
				&& Current.CanonicalIndex == Baseline.CanonicalIndex
				&& Current.InstanceIndex == Baseline.InstanceIndex
				&& Current.CanonicalRootPositionCm == Baseline.CanonicalRootPositionCm
				&& Current.CanonicalDeltaFromHomeCm == Baseline.CanonicalDeltaFromHomeCm
				&& Current.CanonicalDistanceFromHomeCm
					== Baseline.CanonicalDistanceFromHomeCm
				&& Current.ImmutableProxyWorldLocationCm
					== Baseline.ImmutableProxyWorldLocationCm
				&& Current.CanonicalRadiusSolar == Baseline.CanonicalRadiusSolar
				&& Current.bMaterializedHome == Baseline.bMaterializedHome;
		}

		bool ValidateProjectionEvidenceInvariants(UWorld* World,
			const FWorldCachedViewInfo& ViewInfo, const FVector& ViewLocation,
			const FQuat& ViewRotation, FProjectionEvidenceRenderedSample& OutSample,
			FString& OutFailure) const
		{
			OutFailure.Reset();
			AAstroGenerator* Generator = ProjectionEvidenceGenerator.Get();
			AGalaxy* Galaxy = ProjectionEvidenceGalaxy.Get();
			AStarCluster* Cluster = ProjectionEvidenceCluster.Get();
			UHierarchicalInstancedStaticMeshComponent* GalaxyComponent =
				ProjectionEvidenceGalaxyComponent.Get();
			UHierarchicalInstancedStaticMeshComponent* ClusterComponent =
				ProjectionEvidenceClusterComponent.Get();
			if (!World || World != GameplayWorld.Get() || !IsValid(Generator)
				|| !IsValid(Galaxy) || !IsValid(Cluster)
				|| !IsValid(GalaxyComponent) || !IsValid(ClusterComponent)
				|| !bProjectionEvidenceBaselineCaptured)
			{
				OutFailure = TEXT("projection evidence lost its gameplay generator/catalogue actors");
				return false;
			}

			const FAPSCanonicalStellarProjectionDescriptor Current =
				Generator->GetCanonicalStellarProjectionDescriptor();
			if (!ProjectionDescriptorMatchesBaseline(Current)
				|| !Current.bFinalized || !Current.bProjectionValid
				|| !Current.bConsumedFinalizedDataset || !Current.bMappingsComplete
				|| !Current.bUnitRoots || !Current.bBoundsValid
				|| Current.ProjectionVersion
					!= FAPSCanonicalStellarProjectionFrame::CurrentVersion
				|| Current.CanonicalDatasetHash == 0u
				|| Current.CanonicalDatasetHash != Current.CanonicalIdentityHash
				|| Current.RenderedMappingHash == 0u
				|| Current.ProxyBuildSerial == 0u
				|| Current.InstanceUploadCount == 0u
				|| GalaxyComponent->GetInstanceCount() != Current.GalaxyRenderedCount
				|| ClusterComponent->GetInstanceCount() != Current.ClusterRenderedCount
				|| Current.Galaxy.MaxProxyCoordinateCm
					> APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
				|| Current.StarCluster.MaxProxyCoordinateCm
					> APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
				|| Current.GalaxyMaxObservedMatrixMagnitudeCm
					> APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm
				|| Current.ClusterMaxObservedMatrixMagnitudeCm
					> APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm
				|| !Current.bMaterializedHomeProxySuppressed)
			{
				OutFailure = FString::Printf(
					TEXT("projection descriptor/counters changed or became invalid build=%llu/%llu uploads=%llu/%llu mutations=%llu/%llu galaxyInstances=%d/%d clusterInstances=%d/%d dataset=%u mapping=%u"),
					Current.ProxyBuildSerial,
					ProjectionEvidenceBaselineDescriptor.ProxyBuildSerial,
					Current.InstanceUploadCount,
					ProjectionEvidenceBaselineDescriptor.InstanceUploadCount,
					Current.TransformMutationSerial,
					ProjectionEvidenceBaselineDescriptor.TransformMutationSerial,
					GalaxyComponent->GetInstanceCount(), Current.GalaxyRenderedCount,
					ClusterComponent->GetInstanceCount(), Current.ClusterRenderedCount,
					Current.CanonicalDatasetHash, Current.RenderedMappingHash);
				return false;
			}
			FAPSCanonicalStellarProxyRecord GalaxyRecord;
			FAPSCanonicalStellarProxyRecord ClusterRecord;
			FAPSCanonicalStellarProxyRecord HomeRecord;
			if (!Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::Galaxy,
					ProjectionEvidenceGalaxyStableId, GalaxyRecord)
				|| !Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					ProjectionEvidenceClusterStableId, ClusterRecord)
				|| !Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					ProjectionEvidenceHomeStableId, HomeRecord)
				|| !ProxyRecordsMatchIdentity(
					GalaxyRecord, ProjectionEvidenceGalaxyBaselineRecord)
				|| !ProxyRecordsMatchIdentity(
					ClusterRecord, ProjectionEvidenceClusterBaselineRecord)
				|| !ProxyRecordsMatchIdentity(
					HomeRecord, ProjectionEvidenceHomeBaselineRecord))
			{
				OutFailure = TEXT("projection sentinel StableId/index/canonical mapping changed");
				return false;
			}

			FAPSCanonicalClusterSystemAddress HomeAddress;
			FAPSCanonicalClusterSystemAddress ClusterAddress;
			if (!Generator->ResolveCanonicalClusterSystemAddress(
					ProjectionEvidenceHomeStableId, HomeAddress)
				|| !Generator->ResolveCanonicalClusterSystemAddress(
					ProjectionEvidenceClusterStableId, ClusterAddress)
				|| !CanonicalAddressesMatch(
					HomeAddress, ProjectionEvidenceHomeBaselineAddress)
				|| !CanonicalAddressesMatch(
					ClusterAddress, ProjectionEvidenceClusterBaselineAddress))
			{
				OutFailure = TEXT("projection canonical address/home-relative distance changed");
				return false;
			}

			FTransform GalaxyLocalTransform;
			FTransform GalaxyWorldTransform;
			FTransform ClusterLocalTransform;
			FTransform ClusterWorldTransform;
			FTransform HomeLocalTransform;
			FTransform HomeWorldTransform;
			if (!GalaxyComponent->GetInstanceTransform(
					GalaxyRecord.InstanceIndex, GalaxyLocalTransform, false)
				|| !GalaxyComponent->GetInstanceTransform(
					GalaxyRecord.InstanceIndex, GalaxyWorldTransform, true)
				|| !ClusterComponent->GetInstanceTransform(
					ClusterRecord.InstanceIndex, ClusterLocalTransform, false)
				|| !ClusterComponent->GetInstanceTransform(
					ClusterRecord.InstanceIndex, ClusterWorldTransform, true)
				|| !ClusterComponent->GetInstanceTransform(
					HomeRecord.InstanceIndex, HomeLocalTransform, false)
				|| !ClusterComponent->GetInstanceTransform(
					HomeRecord.InstanceIndex, HomeWorldTransform, true)
				|| !IsFiniteTransform(GalaxyLocalTransform)
				|| !IsFiniteTransform(GalaxyWorldTransform)
				|| !IsFiniteTransform(ClusterLocalTransform)
				|| !IsFiniteTransform(ClusterWorldTransform)
				|| !IsFiniteTransform(HomeLocalTransform)
				|| !IsFiniteTransform(HomeWorldTransform))
			{
				OutFailure = TEXT("projection evidence could not read finite local/world HISM transforms");
				return false;
			}

			const double GalaxyMatrixMagnitude =
				APSCanonicalStellarProjection::TransformMatrixMagnitude(GalaxyWorldTransform);
			const double ClusterMatrixMagnitude =
				APSCanonicalStellarProjection::TransformMatrixMagnitude(ClusterWorldTransform);
			const FVector GalaxyExpectedWorldPosition = GalaxyComponent
				->GetComponentTransform().TransformPosition(
					GalaxyRecord.ExpectedBaseProxyPositionCm);
			const FVector ClusterExpectedWorldPosition = ClusterComponent
				->GetComponentTransform().TransformPosition(
					ClusterRecord.ExpectedBaseProxyPositionCm);
			const double GalaxyWorldProjectionError = FVector::Distance(
				GalaxyWorldTransform.GetLocation(), GalaxyExpectedWorldPosition);
			const double ClusterWorldProjectionError = FVector::Distance(
				ClusterWorldTransform.GetLocation(), ClusterExpectedWorldPosition);
			const bool bUnitRoots = Generator->GetActorScale3D().Equals(
				FVector::OneVector, 1.0e-6)
				&& Galaxy->GetActorScale3D().Equals(FVector::OneVector, 1.0e-6)
				&& Cluster->GetActorScale3D().Equals(FVector::OneVector, 1.0e-6)
				&& GalaxyComponent->GetComponentScale().Equals(FVector::OneVector, 1.0e-6)
				&& ClusterComponent->GetComponentScale().Equals(FVector::OneVector, 1.0e-6);
			const bool bProxyContract = bUnitRoots
				&& !GalaxyRecord.bSuppressedMaterializedHome
				&& !ClusterRecord.bSuppressedMaterializedHome
				&& !GalaxyRecord.bSuppressedByView
				&& !ClusterRecord.bSuppressedByView
				&& GalaxyRecord.ActualProxyScale > 0.0
				&& ClusterRecord.ActualProxyScale > 0.0
				&& FMath::IsNearlyEqual(GalaxyRecord.ActualProxyScale,
					GalaxyRecord.ExpectedBaseProxyScale,
					ProjectionEvidenceProxyScaleTolerance)
				&& FMath::IsNearlyEqual(ClusterRecord.ActualProxyScale,
					ClusterRecord.ExpectedBaseProxyScale,
					ProjectionEvidenceProxyScaleTolerance)
				&& GalaxyRecord.ProjectionErrorCm
					<= ProjectionEvidenceProxyPositionToleranceCm
				&& ClusterRecord.ProjectionErrorCm
					<= ProjectionEvidenceProxyPositionToleranceCm
				&& GalaxyLocalTransform.GetLocation().Equals(
					GalaxyRecord.ExpectedBaseProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& ClusterLocalTransform.GetLocation().Equals(
					ClusterRecord.ExpectedBaseProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& GalaxyLocalTransform.GetLocation().Equals(
					GalaxyRecord.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& ClusterLocalTransform.GetLocation().Equals(
					ClusterRecord.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& GalaxyLocalTransform.GetLocation().Equals(
					ProjectionEvidenceGalaxyBaselineRecord.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& ClusterLocalTransform.GetLocation().Equals(
					ProjectionEvidenceClusterBaselineRecord.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& GalaxyLocalTransform.GetScale3D().Equals(
					FVector(GalaxyRecord.ActualProxyScale),
					ProjectionEvidenceProxyScaleTolerance)
				&& ClusterLocalTransform.GetScale3D().Equals(
					FVector(ClusterRecord.ActualProxyScale),
					ProjectionEvidenceProxyScaleTolerance)
				&& GalaxyLocalTransform.GetScale3D().Equals(
					ProjectionEvidenceGalaxyBaselineLocalScale,
					ProjectionEvidenceProxyScaleTolerance)
				&& ClusterLocalTransform.GetScale3D().Equals(
					ProjectionEvidenceClusterBaselineLocalScale,
					ProjectionEvidenceProxyScaleTolerance)
				&& FMath::RadiansToDegrees(
					ProjectionEvidenceGalaxyBaselineLocalRotation.AngularDistance(
						GalaxyLocalTransform.GetRotation()))
					<= ProjectionEvidenceProxyRotationToleranceDegrees
				&& FMath::RadiansToDegrees(
					ProjectionEvidenceClusterBaselineLocalRotation.AngularDistance(
						ClusterLocalTransform.GetRotation()))
					<= ProjectionEvidenceProxyRotationToleranceDegrees
				&& GalaxyWorldProjectionError
					<= ProjectionEvidenceProxyPositionToleranceCm
				&& ClusterWorldProjectionError
					<= ProjectionEvidenceProxyPositionToleranceCm
				&& HomeRecord.bSuppressedMaterializedHome
				&& HomeRecord.ProjectionErrorCm
					<= ProjectionEvidenceProxyPositionToleranceCm
				&& HomeLocalTransform.GetLocation().Equals(
					HomeRecord.ExpectedBaseProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& HomeLocalTransform.GetLocation().Equals(
					ProjectionEvidenceHomeBaselineRecord.ActualProxyPositionCm,
					ProjectionEvidenceProxyPositionToleranceCm)
				&& HomeRecord.ActualProxyScale == 0.0
				&& HomeLocalTransform.GetScale3D() == FVector::ZeroVector
				&& GalaxyMatrixMagnitude
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm
				&& ClusterMatrixMagnitude
					<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm
				&& GalaxyRecord.MaxMatrixMagnitudeCm
					<= APSCanonicalStellarProjection::GalaxyMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm
				&& ClusterRecord.MaxMatrixMagnitudeCm
					<= APSCanonicalStellarProjection::ClusterMaxProxyCoordinateCm
						+ ProjectionEvidenceProxyPositionToleranceCm;
			if (!bProxyContract)
			{
				OutFailure = FString::Printf(
					TEXT("projection local/world proxy contract changed roots=%d gError=%.6f cError=%.6f gMatrix=%.6e cMatrix=%.6e gScale=%.9g/%.9g cScale=%.9g/%.9g gViewSuppressed=%d cViewSuppressed=%d homeSuppressed=%d homeScale=%s"),
					bUnitRoots ? 1 : 0, GalaxyRecord.ProjectionErrorCm,
					ClusterRecord.ProjectionErrorCm, GalaxyMatrixMagnitude,
					ClusterMatrixMagnitude, GalaxyRecord.ActualProxyScale,
					GalaxyRecord.ExpectedBaseProxyScale,
					ClusterRecord.ActualProxyScale,
					ClusterRecord.ExpectedBaseProxyScale,
					GalaxyRecord.bSuppressedByView ? 1 : 0,
					ClusterRecord.bSuppressedByView ? 1 : 0,
					HomeRecord.bSuppressedMaterializedHome ? 1 : 0,
					*HomeLocalTransform.GetScale3D().ToCompactString());
				return false;
			}

			OutSample.ViewLocation = ViewLocation;
			OutSample.ViewRotation = ViewRotation;
			OutSample.GalaxyLocalPosition = GalaxyLocalTransform.GetLocation();
			OutSample.GalaxyLocalRotation = GalaxyLocalTransform.GetRotation();
			OutSample.GalaxyLocalRotation.Normalize();
			OutSample.GalaxyLocalScale = GalaxyLocalTransform.GetScale3D();
			OutSample.GalaxyWorldPosition = GalaxyWorldTransform.GetLocation();
			OutSample.ClusterLocalPosition = ClusterLocalTransform.GetLocation();
			OutSample.ClusterLocalRotation = ClusterLocalTransform.GetRotation();
			OutSample.ClusterLocalRotation.Normalize();
			OutSample.ClusterLocalScale = ClusterLocalTransform.GetScale3D();
			OutSample.ClusterWorldPosition = ClusterWorldTransform.GetLocation();
			OutSample.bClusterNdcValid = ProjectProjectionEvidenceNdc(
				ViewInfo, OutSample.ClusterWorldPosition, OutSample.ClusterNdc);
			OutSample.GalaxyProjectionErrorCm = FMath::Max(
				GalaxyRecord.ProjectionErrorCm, GalaxyWorldProjectionError);
			OutSample.ClusterProjectionErrorCm = FMath::Max(
				ClusterRecord.ProjectionErrorCm, ClusterWorldProjectionError);
			OutSample.GalaxyMaxMatrixMagnitudeCm = GalaxyMatrixMagnitude;
			OutSample.ClusterMaxMatrixMagnitudeCm = ClusterMatrixMagnitude;
			OutSample.ProxyBuildSerial = Current.ProxyBuildSerial;
			OutSample.InstanceUploadCount = Current.InstanceUploadCount;
			OutSample.TransformMutationSerial = Current.TransformMutationSerial;
			return true;
		}

		bool TryReadFreshProjectionEvidenceView(UWorld* World,
			bool& bOutFresh, FVector& OutViewLocation, FQuat& OutViewRotation,
			FWorldCachedViewInfo& OutViewInfo, FString& OutFailure)
		{
			bOutFresh = false;
			OutFailure.Reset();
			if (!World || World != GameplayWorld.Get())
			{
				OutFailure = TEXT("projection evidence lost its gameplay world");
				return false;
			}
			if (World->LastRenderTime
				<= ProjectionEvidenceLastAcceptedRenderTime + UE_DOUBLE_SMALL_NUMBER
				|| GFrameCounter == ProjectionEvidenceLastAcceptedFrameCounter)
			{
				return true;
			}
			if (World->ViewLocationsRenderedLastFrame.Num() != 1
				|| World->CachedViewInfoRenderedLastFrame.Num() != 1)
			{
				OutFailure = FString::Printf(
					TEXT("projection evidence requires exactly one rendered view locations=%d cachedViews=%d"),
					World->ViewLocationsRenderedLastFrame.Num(),
					World->CachedViewInfoRenderedLastFrame.Num());
				return false;
			}
			OutViewLocation = World->ViewLocationsRenderedLastFrame[0];
			OutViewInfo = World->CachedViewInfoRenderedLastFrame[0];
			OutViewRotation = OutViewInfo.ViewToWorld.ToQuat();
			OutViewRotation.Normalize();
			if (OutViewLocation.ContainsNaN() || OutViewRotation.ContainsNaN())
			{
				OutFailure = TEXT("projection evidence rendered view contains non-finite values");
				return false;
			}
			ProjectionEvidenceLastAcceptedRenderTime = World->LastRenderTime;
			ProjectionEvidenceLastAcceptedFrameCounter = GFrameCounter;
			bOutFresh = true;
			return true;
		}

		bool CaptureProjectionEvidenceBaseline(UWorld* World,
			const FWorldCachedViewInfo& ViewInfo, const FVector& ViewLocation,
			const FQuat& ViewRotation, FProjectionEvidenceRenderedSample& OutSample,
			FString& OutFailure)
		{
			AAstroGenerator* Generator = ProjectionEvidenceGenerator.Get();
			if (!IsValid(Generator))
			{
				OutFailure = TEXT("projection evidence generator is invalid at phase begin");
				return false;
			}
			const FAPSCanonicalStellarProjectionDescriptor CurrentDescriptor =
				Generator->GetCanonicalStellarProjectionDescriptor();
			if (!ProjectionDescriptorMatchesBaseline(CurrentDescriptor))
			{
				OutFailure = TEXT("projection descriptor/counters changed between hierarchy-ready and stationary begin");
				return false;
			}
			ProjectionEvidenceHomeStableId =
				ProjectionEvidenceBaselineDescriptor.MaterializedHomeStableId;
			if (!ProjectionEvidenceHomeStableId.IsValid()
				|| !ValidateProjectionEvidenceDataset(Generator, OutFailure))
			{
				if (OutFailure.IsEmpty())
				{
					OutFailure = TEXT("projection evidence could not capture authoritative sentinel baselines");
				}
				return false;
			}

			ProjectionEvidenceBaselineRenderedViewRotation = ViewRotation;
			ProjectionEvidenceBaselineRenderedViewRotation.Normalize();
			bProjectionEvidenceBaselineCaptured = true;
			if (!ValidateProjectionEvidenceInvariants(World, ViewInfo, ViewLocation,
				ViewRotation, OutSample, OutFailure))
			{
				return false;
			}
			if (!OutSample.bClusterNdcValid
				|| FMath::Abs(OutSample.ClusterNdc.X) > 1.0
				|| FMath::Abs(OutSample.ClusterNdc.Y) > 1.0)
			{
				OutFailure = FString::Printf(
					TEXT("deterministic cluster sentinel is not on-screen at stationary begin valid=%d ndc=(%.9f,%.9f)"),
					OutSample.bClusterNdcValid ? 1 : 0,
					OutSample.ClusterNdc.X, OutSample.ClusterNdc.Y);
				return false;
			}
			return true;
		}

		bool InitializeProjectionEvidence(UWorld* World, AAstroGenerator* Generator,
			AGalaxy* Galaxy, AStarCluster* Cluster, APlayerController* PlayerController,
			const FAPSCanonicalStellarProxyRecord& GalaxySentinel,
			const FAPSCanonicalStellarProxyRecord& ClusterSentinel,
			FString& OutFailure)
		{
			OutFailure.Reset();
			if (!World || !IsValid(Generator) || !IsValid(Galaxy) || !IsValid(Cluster)
				|| !IsValid(PlayerController) || !IsValid(Galaxy->StarMeshInstances)
				|| !IsValid(Cluster->StarMeshInstances)
				|| !GalaxySentinel.StableId.IsValid()
				|| !ClusterSentinel.StableId.IsValid())
			{
				OutFailure = TEXT("projection evidence cannot initialize its canonical actors/sentinels");
				return false;
			}
			ProjectionEvidenceGenerator = Generator;
			ProjectionEvidenceGalaxy = Galaxy;
			ProjectionEvidenceCluster = Cluster;
			ProjectionEvidenceGalaxyComponent = Galaxy->StarMeshInstances;
			ProjectionEvidenceClusterComponent = Cluster->StarMeshInstances;
			ProjectionEvidenceGalaxyStableId = GalaxySentinel.StableId;
			ProjectionEvidenceClusterStableId = ClusterSentinel.StableId;
			ProjectionEvidenceOriginalViewTarget = PlayerController->GetViewTarget();
			ProjectionEvidenceBaselineDescriptor =
				Generator->GetCanonicalStellarProjectionDescriptor();
			ProjectionEvidenceHomeStableId =
				ProjectionEvidenceBaselineDescriptor.MaterializedHomeStableId;
			if (!ProjectionEvidenceBaselineDescriptor.bFinalized
				|| !ProjectionEvidenceBaselineDescriptor.bProjectionValid
				|| !ProjectionEvidenceBaselineDescriptor.bConsumedFinalizedDataset
				|| !ProjectionEvidenceBaselineDescriptor.bMappingsComplete
				|| !ProjectionEvidenceBaselineDescriptor.bUnitRoots
				|| !ProjectionEvidenceBaselineDescriptor.bBoundsValid
				|| ProjectionEvidenceBaselineDescriptor.ProxyBuildSerial == 0u
				|| ProjectionEvidenceBaselineDescriptor.InstanceUploadCount == 0u
				|| !ProjectionEvidenceHomeStableId.IsValid())
			{
				OutFailure = TEXT("projection evidence hierarchy-ready descriptor is incomplete");
				return false;
			}
			ProjectionEvidenceGalaxyBaselineRecord = GalaxySentinel;
			ProjectionEvidenceClusterBaselineRecord = ClusterSentinel;
			ProjectionEvidenceDatasetStableIds.Reset();
			ProjectionEvidenceDatasetCanonicalPositions.Reset();
			ProjectionEvidenceDatasetCanonicalRadii.Reset();
			ProjectionEvidenceDatasetMinOrbits.Reset();
			ProjectionEvidenceDatasetMaxOrbits.Reset();
			if (!Generator->GetCanonicalStellarProxyRecord(
					EAPSCanonicalStellarProxyLayer::StarCluster,
					ProjectionEvidenceHomeStableId,
					ProjectionEvidenceHomeBaselineRecord)
				|| !Generator->ResolveCanonicalClusterSystemAddress(
					ProjectionEvidenceHomeStableId,
					ProjectionEvidenceHomeBaselineAddress)
				|| !Generator->ResolveCanonicalClusterSystemAddress(
					ProjectionEvidenceClusterStableId,
					ProjectionEvidenceClusterBaselineAddress)
				|| !CaptureProjectionEvidenceDatasetBaseline(Generator, OutFailure))
			{
				if (OutFailure.IsEmpty())
				{
					OutFailure = TEXT("projection evidence could not capture hierarchy-ready identity/address baselines");
				}
				return false;
			}

			FTransform GalaxyLocalTransform;
			FTransform ClusterLocalTransform;
			if (!Galaxy->StarMeshInstances->GetInstanceTransform(
					ProjectionEvidenceGalaxyBaselineRecord.InstanceIndex,
					GalaxyLocalTransform, false)
				|| !Cluster->StarMeshInstances->GetInstanceTransform(
					ProjectionEvidenceClusterBaselineRecord.InstanceIndex,
					ClusterLocalTransform, false)
				|| !IsFiniteTransform(GalaxyLocalTransform)
				|| !IsFiniteTransform(ClusterLocalTransform))
			{
				OutFailure = TEXT("projection evidence could not capture hierarchy-ready local transforms");
				return false;
			}
			ProjectionEvidenceGalaxyBaselineLocalRotation =
				GalaxyLocalTransform.GetRotation();
			ProjectionEvidenceGalaxyBaselineLocalRotation.Normalize();
			ProjectionEvidenceClusterBaselineLocalRotation =
				ClusterLocalTransform.GetRotation();
			ProjectionEvidenceClusterBaselineLocalRotation.Normalize();
			ProjectionEvidenceGalaxyBaselineLocalScale =
				GalaxyLocalTransform.GetScale3D();
			ProjectionEvidenceClusterBaselineLocalScale =
				ClusterLocalTransform.GetScale3D();

			const FVector SentinelWorldLocation = Cluster->StarMeshInstances
				->GetComponentTransform().TransformPosition(
					ClusterSentinel.ActualProxyPositionCm);
			const double CameraDistanceCm = FMath::Clamp(
				ClusterSentinel.AppliedVisualProxyRadiusCm * 8.0,
				250000.0, 2500000.0);
			const FVector CameraOffsetDirection = FVector(1.0, 0.37, 0.19).GetSafeNormal();
			const FVector CameraLocation = SentinelWorldLocation
				- CameraOffsetDirection * CameraDistanceCm;
			const FRotator CameraRotation =
				(SentinelWorldLocation - CameraLocation).Rotation();
			if (SentinelWorldLocation.ContainsNaN() || CameraLocation.ContainsNaN())
			{
				OutFailure = TEXT("projection evidence resolved a non-finite deterministic camera");
				return false;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACameraActor* TestCamera = World->SpawnActor<ACameraActor>(
				ACameraActor::StaticClass(), CameraLocation, CameraRotation,
				SpawnParameters);
			if (!IsValid(TestCamera))
			{
				OutFailure = TEXT("projection evidence could not spawn its deterministic camera");
				return false;
			}
			TestCamera->SetActorEnableCollision(false);
			TestCamera->SetActorTickEnabled(false);
			if (UCameraComponent* CameraComponent = TestCamera->GetCameraComponent())
			{
				CameraComponent->SetFieldOfView(70.0f);
			}
			ProjectionEvidenceCamera = TestCamera;
			ProjectionEvidenceExpectedCameraLocation = CameraLocation;
			PlayerController->SetViewTarget(TestCamera);

			ProjectionEvidenceWarmupFramesRemaining = ProjectionEvidenceWarmupFrames;
			ProjectionEvidenceAcceptedFrameCount = 0;
			ProjectionEvidenceLastAcceptedRenderTime = World->LastRenderTime;
			ProjectionEvidenceLastAcceptedFrameCounter = GFrameCounter;
			bProjectionEvidencePhaseStarted = false;
			bProjectionEvidenceBaselineCaptured = false;
			return true;
		}

		void RestoreProjectionEvidenceCamera()
		{
			ACameraActor* TestCamera = ProjectionEvidenceCamera.Get();
			UWorld* World = GameplayWorld.Get();
			APlayerController* PlayerController = World
				? World->GetFirstPlayerController() : nullptr;
			if (IsValid(PlayerController) && PlayerController->GetViewTarget() == TestCamera)
			{
				AActor* RestoreTarget = ProjectionEvidenceOriginalViewTarget.Get();
				if (!IsValid(RestoreTarget))
				{
					RestoreTarget = RuntimeGravityPawn.Get();
				}
				if (IsValid(RestoreTarget))
				{
					PlayerController->SetViewTarget(RestoreTarget);
				}
			}
			if (IsValid(TestCamera) && IsValid(World)
				&& TestCamera->GetWorld() == World)
			{
				World->DestroyActor(TestCamera);
			}
			ProjectionEvidenceCamera.Reset();
			ProjectionEvidenceOriginalViewTarget.Reset();
		}

		bool UpdateProjectionEvidenceStationaryHold(UWorld* World, double Now)
		{
			if (Now - StepStartSeconds > ProjectionEvidencePhaseTimeoutSeconds)
			{
				return Fail(FString::Printf(
					TEXT("projection stationary evidence incomplete samples=%d/%d"),
					ProjectionEvidenceAcceptedFrameCount,
					ProjectionEvidenceRequiredFrames));
			}
			bool bFresh = false;
			FVector ViewLocation = FVector::ZeroVector;
			FQuat ViewRotation = FQuat::Identity;
			FWorldCachedViewInfo ViewInfo;
			FString Failure;
			if (!TryReadFreshProjectionEvidenceView(World, bFresh, ViewLocation,
				ViewRotation, ViewInfo, Failure))
			{
				return Fail(Failure);
			}
			if (!bFresh)
			{
				return false;
			}
			const FAPSCanonicalStellarProjectionDescriptor WarmupDescriptor =
				ProjectionEvidenceGenerator.IsValid()
					? ProjectionEvidenceGenerator->GetCanonicalStellarProjectionDescriptor()
					: FAPSCanonicalStellarProjectionDescriptor{};
			if (!ProjectionDescriptorMatchesBaseline(WarmupDescriptor))
			{
				return Fail(TEXT("projection descriptor/counters changed after hierarchy-ready during camera warmup"));
			}

			const double ExpectedPositionError = FVector::Distance(
				ViewLocation, ProjectionEvidenceExpectedCameraLocation);
			if (ExpectedPositionError > ProjectionEvidenceViewPositionToleranceCm)
			{
				if (ProjectionEvidenceWarmupFramesRemaining > 0)
				{
					return false;
				}
				return Fail(FString::Printf(
					TEXT("stationary evidence rendered view does not match deterministic camera positionError=%.6fcm"),
					ExpectedPositionError));
			}
			if (ProjectionEvidenceWarmupFramesRemaining > 0)
			{
				--ProjectionEvidenceWarmupFramesRemaining;
				return false;
			}

			FProjectionEvidenceRenderedSample Sample;
			Sample.RenderFrameCounter = GFrameCounter;
			Sample.RenderTime = World->LastRenderTime;
			if (!bProjectionEvidencePhaseStarted)
			{
				if (!CaptureProjectionEvidenceBaseline(World, ViewInfo, ViewLocation,
					ViewRotation, Sample, Failure))
				{
					return Fail(Failure);
				}
				ProjectionEvidencePreviousSample = Sample;
				ProjectionEvidenceAcceptedFrameCount = 0;
				bProjectionEvidencePhaseStarted = true;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.P0.ProjectionPhase] source=directRuntime phase=Stationary event=Begin requiredSamples=%d warmup=%d version=%u context=%u dataset=%u datasetVersion=%u input=%u datasetBuild=%llu records=%d mapping=%u home=%s galaxy=%s cluster=%s proxyBuild=%llu uploads=%llu mutations=%llu"),
					ProjectionEvidenceRequiredFrames, ProjectionEvidenceWarmupFrames,
					ProjectionEvidenceBaselineDescriptor.ProjectionVersion,
					ProjectionEvidenceBaselineDescriptor.ContextHash,
					ProjectionEvidenceBaselineDescriptor.CanonicalDatasetHash,
					ProjectionEvidenceBaselineDescriptor.CanonicalDatasetVersion,
					ProjectionEvidenceBaselineDescriptor.CanonicalDatasetInputHash,
					ProjectionEvidenceBaselineDescriptor.CanonicalDatasetBuildSerial,
					ProjectionEvidenceBaselineDescriptor.CanonicalDatasetRecordCount,
					ProjectionEvidenceBaselineDescriptor.RenderedMappingHash,
					*ProjectionEvidenceHomeStableId.ToString(EGuidFormats::Digits),
					*ProjectionEvidenceGalaxyStableId.ToString(EGuidFormats::Digits),
					*ProjectionEvidenceClusterStableId.ToString(EGuidFormats::Digits),
					ProjectionEvidenceBaselineDescriptor.ProxyBuildSerial,
					ProjectionEvidenceBaselineDescriptor.InstanceUploadCount,
					ProjectionEvidenceBaselineDescriptor.TransformMutationSerial);
				return false;
			}
			if (!ValidateProjectionEvidenceInvariants(World, ViewInfo, ViewLocation,
				ViewRotation, Sample, Failure))
			{
				return Fail(Failure);
			}

			const FProjectionEvidenceRenderedSample& Previous =
				ProjectionEvidencePreviousSample;
			const double ViewPositionDelta = FVector::Distance(
				Previous.ViewLocation, Sample.ViewLocation);
			const double ViewAngularDelta = FMath::RadiansToDegrees(
				Previous.ViewRotation.AngularDistance(Sample.ViewRotation));
			const double BaselineViewAngularDelta = FMath::RadiansToDegrees(
				ProjectionEvidenceBaselineRenderedViewRotation.AngularDistance(
					Sample.ViewRotation));
			const bool bConsecutiveRenderedFrame = Sample.RenderFrameCounter
				== Previous.RenderFrameCounter + 1u;
			const double GalaxyLocalDelta = FVector::Distance(
				Previous.GalaxyLocalPosition, Sample.GalaxyLocalPosition);
			const double GalaxyLocalScaleDelta = FVector::Distance(
				Previous.GalaxyLocalScale, Sample.GalaxyLocalScale);
			const double GalaxyLocalRotationDelta = FMath::RadiansToDegrees(
				Previous.GalaxyLocalRotation.AngularDistance(
					Sample.GalaxyLocalRotation));
			const double GalaxyWorldDelta = FVector::Distance(
				Previous.GalaxyWorldPosition, Sample.GalaxyWorldPosition);
			const double ClusterLocalDelta = FVector::Distance(
				Previous.ClusterLocalPosition, Sample.ClusterLocalPosition);
			const double ClusterLocalScaleDelta = FVector::Distance(
				Previous.ClusterLocalScale, Sample.ClusterLocalScale);
			const double ClusterLocalRotationDelta = FMath::RadiansToDegrees(
				Previous.ClusterLocalRotation.AngularDistance(
					Sample.ClusterLocalRotation));
			const double ClusterWorldDelta = FVector::Distance(
				Previous.ClusterWorldPosition, Sample.ClusterWorldPosition);
			const double GalaxyRelativeDelta = FVector::Distance(
				Previous.GalaxyWorldPosition - Previous.ViewLocation,
				Sample.GalaxyWorldPosition - Sample.ViewLocation);
			const double ClusterRelativeDelta = FVector::Distance(
				Previous.ClusterWorldPosition - Previous.ViewLocation,
				Sample.ClusterWorldPosition - Sample.ViewLocation);
			const double NdcDelta = Previous.bClusterNdcValid && Sample.bClusterNdcValid
				? FVector2D::Distance(Previous.ClusterNdc, Sample.ClusterNdc)
				: TNumericLimits<double>::Max();
			if (!bConsecutiveRenderedFrame
				|| ViewPositionDelta > ProjectionEvidenceViewPositionToleranceCm
				|| ViewAngularDelta > ProjectionEvidenceViewAngleToleranceDegrees
				|| BaselineViewAngularDelta
					> ProjectionEvidenceViewAngleToleranceDegrees
				|| GalaxyLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| GalaxyLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
				|| GalaxyLocalRotationDelta
					> ProjectionEvidenceProxyRotationToleranceDegrees
				|| GalaxyWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| ClusterLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| ClusterLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
				|| ClusterLocalRotationDelta
					> ProjectionEvidenceProxyRotationToleranceDegrees
				|| ClusterWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| GalaxyRelativeDelta > ProjectionEvidenceRelativeVectorToleranceCm
				|| ClusterRelativeDelta > ProjectionEvidenceRelativeVectorToleranceCm
				|| NdcDelta > ProjectionEvidenceStationaryNdcTolerance
				|| !Sample.bClusterNdcValid
				|| FMath::Abs(Sample.ClusterNdc.X) > 1.0
				|| FMath::Abs(Sample.ClusterNdc.Y) > 1.0)
			{
				return Fail(FString::Printf(
					TEXT("stationary projection evidence drift consecutive=%d frame=%llu/%llu view=%.6fcm angle=%.9fdeg baselineAngle=%.9fdeg galaxyLocal=%.6fcm galaxyScale=%.9g galaxyRotation=%.9fdeg galaxyWorld=%.6fcm clusterLocal=%.6fcm clusterScale=%.9g clusterRotation=%.9fdeg clusterWorld=%.6fcm galaxyRelative=%.6fcm clusterRelative=%.6fcm ndc=%.9f valid=%d"),
					bConsecutiveRenderedFrame ? 1 : 0, Previous.RenderFrameCounter,
					Sample.RenderFrameCounter, ViewPositionDelta, ViewAngularDelta,
					BaselineViewAngularDelta, GalaxyLocalDelta,
					GalaxyLocalScaleDelta, GalaxyLocalRotationDelta,
					GalaxyWorldDelta, ClusterLocalDelta, ClusterLocalScaleDelta,
					ClusterLocalRotationDelta, ClusterWorldDelta,
					GalaxyRelativeDelta, ClusterRelativeDelta, NdcDelta,
					Sample.bClusterNdcValid ? 1 : 0));
			}

			++ProjectionEvidenceAcceptedFrameCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.P0.ProjectionSample] source=directRuntime phase=Stationary sample=%d/%d frame=%llu consecutive=1 renderTime=%.9f view=%s viewDelta=%.6fcm angleDelta=%.9fdeg baselineAngle=%.9fdeg galaxy=%s gLocalDelta=%.6fcm gScaleDelta=%.9g gRotationDelta=%.9fdeg gWorldDelta=%.6fcm gRelativeDelta=%.6fcm gError=%.6fcm gMatrix=%.6e cluster=%s cLocalDelta=%.6fcm cScaleDelta=%.9g cRotationDelta=%.9fdeg cWorldDelta=%.6fcm cRelativeDelta=%.6fcm cError=%.6fcm cMatrix=%.6e ndc=(%.9f,%.9f) ndcDelta=%.9f dataset=%u mapping=%u proxyBuild=%llu uploads=%llu mutations=%llu"),
				ProjectionEvidenceAcceptedFrameCount, ProjectionEvidenceRequiredFrames,
				Sample.RenderFrameCounter, Sample.RenderTime,
				*Sample.ViewLocation.ToCompactString(), ViewPositionDelta,
				ViewAngularDelta, BaselineViewAngularDelta,
				*ProjectionEvidenceGalaxyStableId.ToString(EGuidFormats::Digits),
				GalaxyLocalDelta, GalaxyLocalScaleDelta, GalaxyLocalRotationDelta,
				GalaxyWorldDelta, GalaxyRelativeDelta, Sample.GalaxyProjectionErrorCm,
				Sample.GalaxyMaxMatrixMagnitudeCm,
				*ProjectionEvidenceClusterStableId.ToString(EGuidFormats::Digits),
				ClusterLocalDelta, ClusterLocalScaleDelta, ClusterLocalRotationDelta,
				ClusterWorldDelta, ClusterRelativeDelta,
				Sample.ClusterProjectionErrorCm,
				Sample.ClusterMaxMatrixMagnitudeCm,
				Sample.ClusterNdc.X, Sample.ClusterNdc.Y, NdcDelta,
				ProjectionEvidenceBaselineDescriptor.CanonicalDatasetHash,
				ProjectionEvidenceBaselineDescriptor.RenderedMappingHash,
				Sample.ProxyBuildSerial, Sample.InstanceUploadCount,
				Sample.TransformMutationSerial);
			ProjectionEvidencePreviousSample = Sample;
			if (ProjectionEvidenceAcceptedFrameCount < ProjectionEvidenceRequiredFrames)
			{
				return false;
			}
			if (!ValidateProjectionEvidenceDataset(
				ProjectionEvidenceGenerator.Get(), Failure))
			{
				return Fail(Failure);
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.P0.ProjectionPhase] source=directRuntime phase=Stationary event=End status=PASS samples=%d proxyBuildDelta=0 uploadDelta=0 mutationDelta=0"),
				ProjectionEvidenceAcceptedFrameCount);
			bProjectionEvidencePhaseStarted = false;
			ACameraActor* TestCamera = ProjectionEvidenceCamera.Get();
			if (!IsValid(TestCamera))
			{
				return Fail(TEXT("projection evidence camera disappeared before controlled movement"));
			}
			ProjectionEvidenceMoveDelta = TestCamera->GetActorRightVector().GetSafeNormal()
				* ProjectionEvidenceMovementPerFrameCm;
			if (ProjectionEvidenceMoveDelta.IsNearlyZero())
			{
				return Fail(TEXT("projection evidence could not resolve controlled movement vector"));
			}
			ProjectionEvidenceAcceptedFrameCount = 0;
			ProjectionEvidenceExpectedCameraLocation = Sample.ViewLocation
				+ ProjectionEvidenceMoveDelta;
			Step = EStep::ProjectionEvidenceControlledMove;
			StepStartSeconds = Now;
			bProjectionEvidencePhaseStarted = true;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.P0.ProjectionPhase] source=directRuntime phase=Moving event=Begin requiredSamples=%d commandPerFrame=%s commandMagnitude=%.6fcm fixedRotation=1 ndcGate=disabled proxyBuild=%llu uploads=%llu mutations=%llu"),
				ProjectionEvidenceRequiredFrames,
				*ProjectionEvidenceMoveDelta.ToCompactString(),
				ProjectionEvidenceMoveDelta.Size(),
				ProjectionEvidenceBaselineDescriptor.ProxyBuildSerial,
				ProjectionEvidenceBaselineDescriptor.InstanceUploadCount,
				ProjectionEvidenceBaselineDescriptor.TransformMutationSerial);
			if (!TestCamera->SetActorLocation(ProjectionEvidenceExpectedCameraLocation,
				false, nullptr, ETeleportType::TeleportPhysics))
			{
				return Fail(TEXT("projection evidence could not command its first camera move"));
			}
			return false;
		}

		bool UpdateProjectionEvidenceControlledMove(UWorld* World, double Now)
		{
			if (Now - StepStartSeconds > ProjectionEvidencePhaseTimeoutSeconds)
			{
				return Fail(FString::Printf(
					TEXT("projection moving evidence incomplete samples=%d/%d"),
					ProjectionEvidenceAcceptedFrameCount,
					ProjectionEvidenceRequiredFrames));
			}
			bool bFresh = false;
			FVector ViewLocation = FVector::ZeroVector;
			FQuat ViewRotation = FQuat::Identity;
			FWorldCachedViewInfo ViewInfo;
			FString Failure;
			if (!TryReadFreshProjectionEvidenceView(World, bFresh, ViewLocation,
				ViewRotation, ViewInfo, Failure))
			{
				return Fail(Failure);
			}
			if (!bFresh)
			{
				return false;
			}

			FProjectionEvidenceRenderedSample Sample;
			Sample.RenderFrameCounter = GFrameCounter;
			Sample.RenderTime = World->LastRenderTime;
			if (!ValidateProjectionEvidenceInvariants(World, ViewInfo, ViewLocation,
				ViewRotation, Sample, Failure))
			{
				return Fail(Failure);
			}
			const FProjectionEvidenceRenderedSample& Previous =
				ProjectionEvidencePreviousSample;
			const bool bConsecutiveRenderedFrame = Sample.RenderFrameCounter
				== Previous.RenderFrameCounter + 1u;
			const FVector ViewDelta = Sample.ViewLocation - Previous.ViewLocation;
			const double CommandError = (ViewDelta - ProjectionEvidenceMoveDelta).Size();
			const double CameraLocationError = FVector::Distance(
				Sample.ViewLocation, ProjectionEvidenceExpectedCameraLocation);
			const double PreviousCameraLocationError = FVector::Distance(
				Sample.ViewLocation, Previous.ViewLocation);
			const double ViewAngularDelta = FMath::RadiansToDegrees(
				Previous.ViewRotation.AngularDistance(Sample.ViewRotation));
			const double BaselineViewAngularDelta = FMath::RadiansToDegrees(
				ProjectionEvidenceBaselineRenderedViewRotation.AngularDistance(
					Sample.ViewRotation));
			const double GalaxyLocalDelta = FVector::Distance(
				Previous.GalaxyLocalPosition, Sample.GalaxyLocalPosition);
			const double GalaxyLocalScaleDelta = FVector::Distance(
				Previous.GalaxyLocalScale, Sample.GalaxyLocalScale);
			const double GalaxyLocalRotationDelta = FMath::RadiansToDegrees(
				Previous.GalaxyLocalRotation.AngularDistance(
					Sample.GalaxyLocalRotation));
			const double GalaxyWorldDelta = FVector::Distance(
				Previous.GalaxyWorldPosition, Sample.GalaxyWorldPosition);
			const double ClusterLocalDelta = FVector::Distance(
				Previous.ClusterLocalPosition, Sample.ClusterLocalPosition);
			const double ClusterLocalScaleDelta = FVector::Distance(
				Previous.ClusterLocalScale, Sample.ClusterLocalScale);
			const double ClusterLocalRotationDelta = FMath::RadiansToDegrees(
				Previous.ClusterLocalRotation.AngularDistance(
					Sample.ClusterLocalRotation));
			const double ClusterWorldDelta = FVector::Distance(
				Previous.ClusterWorldPosition, Sample.ClusterWorldPosition);
			const FVector GalaxyClosure =
				(Sample.GalaxyWorldPosition - Sample.ViewLocation)
				- (Previous.GalaxyWorldPosition - Previous.ViewLocation)
				+ ViewDelta;
			const FVector ClusterClosure =
				(Sample.ClusterWorldPosition - Sample.ViewLocation)
				- (Previous.ClusterWorldPosition - Previous.ViewLocation)
				+ ViewDelta;
			if (!bConsecutiveRenderedFrame)
			{
				return Fail(FString::Printf(
					TEXT("moving projection evidence skipped a rendered frame previous=%llu current=%llu"),
					Previous.RenderFrameCounter, Sample.RenderFrameCounter));
			}
			if (ProjectionEvidenceAcceptedFrameCount == 0
				&& PreviousCameraLocationError
					<= ProjectionEvidenceViewPositionToleranceCm
				&& CameraLocationError > ProjectionEvidenceViewPositionToleranceCm)
			{
				if (ViewAngularDelta > ProjectionEvidenceViewAngleToleranceDegrees
					|| BaselineViewAngularDelta
						> ProjectionEvidenceViewAngleToleranceDegrees
					|| GalaxyLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
					|| GalaxyLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
					|| GalaxyLocalRotationDelta
						> ProjectionEvidenceProxyRotationToleranceDegrees
					|| GalaxyWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
					|| ClusterLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
					|| ClusterLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
					|| ClusterLocalRotationDelta
						> ProjectionEvidenceProxyRotationToleranceDegrees
					|| ClusterWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
					|| !Sample.bClusterNdcValid)
				{
					return Fail(FString::Printf(
						TEXT("moving projection command-pending frame changed scene viewAngle=%.9fdeg baselineAngle=%.9fdeg galaxyLocal=%.6fcm galaxyScale=%.9g galaxyRotation=%.9fdeg galaxyWorld=%.6fcm clusterLocal=%.6fcm clusterScale=%.9g clusterRotation=%.9fdeg clusterWorld=%.6fcm ndcValid=%d"),
						ViewAngularDelta, BaselineViewAngularDelta,
						GalaxyLocalDelta, GalaxyLocalScaleDelta,
						GalaxyLocalRotationDelta, GalaxyWorldDelta,
						ClusterLocalDelta, ClusterLocalScaleDelta,
						ClusterLocalRotationDelta, ClusterWorldDelta,
						Sample.bClusterNdcValid ? 1 : 0));
				}
				ProjectionEvidencePreviousSample = Sample;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.P0.ProjectionSample] source=directRuntime phase=Moving event=CommandPending frame=%llu consecutive=1 expected=%s actual=%s proxyBuild=%llu uploads=%llu mutations=%llu"),
					Sample.RenderFrameCounter,
					*ProjectionEvidenceExpectedCameraLocation.ToCompactString(),
					*Sample.ViewLocation.ToCompactString(), Sample.ProxyBuildSerial,
					Sample.InstanceUploadCount, Sample.TransformMutationSerial);
				return false;
			}
			if (ViewDelta.Size() <= 1.0
				|| CommandError > ProjectionEvidenceViewPositionToleranceCm
				|| CameraLocationError > ProjectionEvidenceViewPositionToleranceCm
				|| ViewAngularDelta > ProjectionEvidenceViewAngleToleranceDegrees
				|| BaselineViewAngularDelta
					> ProjectionEvidenceViewAngleToleranceDegrees
				|| GalaxyLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| GalaxyLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
				|| GalaxyLocalRotationDelta
					> ProjectionEvidenceProxyRotationToleranceDegrees
				|| GalaxyWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| ClusterLocalDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| ClusterLocalScaleDelta > ProjectionEvidenceProxyScaleTolerance
				|| ClusterLocalRotationDelta
					> ProjectionEvidenceProxyRotationToleranceDegrees
				|| ClusterWorldDelta > ProjectionEvidenceProxyPositionToleranceCm
				|| GalaxyClosure.Size()
					> ProjectionEvidenceMovingClosureToleranceCm
				|| ClusterClosure.Size()
					> ProjectionEvidenceMovingClosureToleranceCm
				|| !Sample.bClusterNdcValid)
			{
				return Fail(FString::Printf(
					TEXT("moving projection evidence failed viewDelta=%s commandError=%.6fcm cameraError=%.6fcm angle=%.9fdeg baselineAngle=%.9fdeg galaxyLocal=%.6fcm galaxyScale=%.9g galaxyRotation=%.9fdeg galaxyWorld=%.6fcm galaxyClosure=%.6fcm clusterLocal=%.6fcm clusterScale=%.9g clusterRotation=%.9fdeg clusterWorld=%.6fcm clusterClosure=%.6fcm ndcValid=%d"),
					*ViewDelta.ToCompactString(), CommandError, CameraLocationError,
					ViewAngularDelta, BaselineViewAngularDelta, GalaxyLocalDelta,
					GalaxyLocalScaleDelta, GalaxyLocalRotationDelta,
					GalaxyWorldDelta, GalaxyClosure.Size(), ClusterLocalDelta,
					ClusterLocalScaleDelta, ClusterLocalRotationDelta,
					ClusterWorldDelta, ClusterClosure.Size(),
					Sample.bClusterNdcValid ? 1 : 0));
			}

			++ProjectionEvidenceAcceptedFrameCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.P0.ProjectionSample] source=directRuntime phase=Moving sample=%d/%d frame=%llu consecutive=1 renderTime=%.9f view=%s viewDelta=%s command=%s commandError=%.6fcm cameraError=%.6fcm angleDelta=%.9fdeg baselineAngle=%.9fdeg galaxy=%s gLocalDelta=%.6fcm gScaleDelta=%.9g gRotationDelta=%.9fdeg gWorldDelta=%.6fcm gClosure=%.6fcm gError=%.6fcm cluster=%s cLocalDelta=%.6fcm cScaleDelta=%.9g cRotationDelta=%.9fdeg cWorldDelta=%.6fcm cClosure=%.6fcm cError=%.6fcm ndc=(%.9f,%.9f) ndcGate=disabled dataset=%u mapping=%u proxyBuild=%llu uploads=%llu mutations=%llu"),
				ProjectionEvidenceAcceptedFrameCount, ProjectionEvidenceRequiredFrames,
				Sample.RenderFrameCounter, Sample.RenderTime,
				*Sample.ViewLocation.ToCompactString(), *ViewDelta.ToCompactString(),
				*ProjectionEvidenceMoveDelta.ToCompactString(), CommandError,
				CameraLocationError, ViewAngularDelta, BaselineViewAngularDelta,
				*ProjectionEvidenceGalaxyStableId.ToString(EGuidFormats::Digits),
				GalaxyLocalDelta, GalaxyLocalScaleDelta, GalaxyLocalRotationDelta,
				GalaxyWorldDelta, GalaxyClosure.Size(),
				Sample.GalaxyProjectionErrorCm,
				*ProjectionEvidenceClusterStableId.ToString(EGuidFormats::Digits),
				ClusterLocalDelta, ClusterLocalScaleDelta, ClusterLocalRotationDelta,
				ClusterWorldDelta, ClusterClosure.Size(),
				Sample.ClusterProjectionErrorCm,
				Sample.ClusterNdc.X, Sample.ClusterNdc.Y,
				ProjectionEvidenceBaselineDescriptor.CanonicalDatasetHash,
				ProjectionEvidenceBaselineDescriptor.RenderedMappingHash,
				Sample.ProxyBuildSerial, Sample.InstanceUploadCount,
				Sample.TransformMutationSerial);
			ProjectionEvidencePreviousSample = Sample;
			if (ProjectionEvidenceAcceptedFrameCount < ProjectionEvidenceRequiredFrames)
			{
				ACameraActor* TestCamera = ProjectionEvidenceCamera.Get();
				if (!IsValid(TestCamera))
				{
					return Fail(TEXT("projection evidence camera disappeared during controlled movement"));
				}
				ProjectionEvidenceExpectedCameraLocation = Sample.ViewLocation
					+ ProjectionEvidenceMoveDelta;
				if (!TestCamera->SetActorLocation(ProjectionEvidenceExpectedCameraLocation,
					false, nullptr, ETeleportType::TeleportPhysics))
				{
					return Fail(TEXT("projection evidence could not command the next camera move"));
				}
				return false;
			}
			if (!ValidateProjectionEvidenceDataset(
				ProjectionEvidenceGenerator.Get(), Failure))
			{
				return Fail(Failure);
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.P0.ProjectionPhase] source=directRuntime phase=Moving event=End status=PASS samples=%d proxyBuildDelta=0 uploadDelta=0 mutationDelta=0 ndcGate=disabled"),
				ProjectionEvidenceAcceptedFrameCount);
			bProjectionEvidencePhaseStarted = false;
			RestoreProjectionEvidenceCamera();
			Step = EStep::WaitForGameplaySurface;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateWetOceanContract(UWorld* World, APlanet* Planet,
			APlanetarySurfaceGenerator* Surface, AWorldScapeRoot* Root,
			bool& bOutPending, FString& OutFailure)
		{
			bOutPending = false;
			OutFailure.Reset();
			if (!IsValid(World) || !IsValid(Planet) || !IsValid(Surface)
				|| !IsValid(Root))
			{
				OutFailure = TEXT("wet-ocean contract lost its gameplay surface actors");
				return false;
			}

			const TArray<AWorldScapeRoot*> Roots = FindActors<AWorldScapeRoot>(World);
			const TArray<APlanetarySurfaceGenerator*> SurfaceGenerators =
				FindActors<APlanetarySurfaceGenerator>(World);
			if (Roots.Num() != 1 || Roots[0] != Root || SurfaceGenerators.Num() != 1
				|| SurfaceGenerators[0] != Surface
				|| Surface->WorldScapeRootInstance != Root
				|| Root->GetOwner() != Planet || Root->GetAttachParentActor() != Planet)
			{
				OutFailure = FString::Printf(
					TEXT("wet gameplay does not own exactly one authoritative WorldScape pair roots=%d rootMatch=%d generators=%d generatorMatch=%d owner=%s parent=%s"),
					Roots.Num(), Roots.Num() == 1 && Roots[0] == Root ? 1 : 0,
					SurfaceGenerators.Num(),
					SurfaceGenerators.Num() == 1 && SurfaceGenerators[0] == Surface ? 1 : 0,
					*GetNameSafe(Root->GetOwner()),
					*GetNameSafe(Root->GetAttachParentActor()));
				return false;
			}

			float UnusedPhysicalOrbitalNormalBlend = -1.0f;
			const bool bHasUnusedPhysicalNormalBlend =
				IsValid(Surface->ResolvedOceanMaterialInstance)
				&& Surface->ResolvedOceanMaterialInstance->GetScalarParameterValue(
					FHashedMaterialParameterInfo(FName(TEXT("OrbitalNormalBlend"))),
					UnusedPhysicalOrbitalNormalBlend);
			float PhysicalWaveColorStrength = -1.0f;
			const bool bHasPhysicalWaveColor =
				IsValid(Surface->ResolvedOceanMaterialInstance)
				&& Surface->ResolvedOceanMaterialInstance->GetScalarParameterValue(
					FHashedMaterialParameterInfo(FName(TEXT("WaveColorStrength"))),
					PhysicalWaveColorStrength);
			float PhysicalWaveNormalStrength = -1.0f;
			const bool bHasPhysicalWaveNormal =
				IsValid(Surface->ResolvedOceanMaterialInstance)
				&& Surface->ResolvedOceanMaterialInstance->GetScalarParameterValue(
					FHashedMaterialParameterInfo(FName(TEXT("WaveNormalStrength"))),
					PhysicalWaveNormalStrength);
			float PhysicalSurfaceOpacity = -1.0f;
			const bool bHasPhysicalSurfaceOpacity =
				IsValid(Surface->ResolvedOceanMaterialInstance)
				&& Surface->ResolvedOceanMaterialInstance->GetScalarParameterValue(
					FHashedMaterialParameterInfo(FName(TEXT("WaterSurfaceOpacity"))),
					PhysicalSurfaceOpacity);
			UMaterial* ExpectedWorldScapeWaterMaster =
				LoadObject<UMaterial>(nullptr,
					TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/M_APS_WorldScapeLiquid.M_APS_WorldScapeLiquid"));
			if (Surface->ResolvedSurfaceProfile.PlanetType != EPlanetType::Water
				|| Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::Water
				|| !IsValid(Surface->ResolvedOceanMaterialInstance)
				|| !IsValid(ExpectedWorldScapeWaterMaster)
				|| Surface->ResolvedOceanMaterialInstance->GetBlendMode() != BLEND_Opaque
				|| !Surface->ResolvedOceanMaterialInstance->GetShadingModels()
					.HasShadingModel(MSM_SingleLayerWater)
				|| Surface->ResolvedOceanMaterialInstance->GetMaterial()
					!= ExpectedWorldScapeWaterMaster
				|| bHasUnusedPhysicalNormalBlend
				|| !bHasPhysicalWaveColor || PhysicalWaveColorStrength < 0.0f
				|| PhysicalWaveColorStrength > 0.02f
				|| !bHasPhysicalWaveNormal || PhysicalWaveNormalStrength < 0.0f
				|| PhysicalWaveNormalStrength > 0.04f
				|| !bHasPhysicalSurfaceOpacity || PhysicalSurfaceOpacity <= 0.0f
				|| PhysicalSurfaceOpacity > 0.45f
				|| !Root->bOcean
				|| Root->OceanMaterial.DefaultMaterial
					!= Surface->ResolvedOceanMaterialInstance)
			{
				OutFailure = FString::Printf(
					TEXT("Water handoff did not retain the project WorldScape SingleLayerWater ocean type=%d liquid=%d bOcean=%d blend=%d base=%s expectedBase=%s hasOrbitalBlend=%d waveColor=%f waveNormal=%f surfaceOpacity=%f resolvedMID=%s rootMID=%s"),
					static_cast<int32>(Surface->ResolvedSurfaceProfile.PlanetType),
					static_cast<int32>(Surface->ResolvedSurfaceProfile.LiquidType),
					Root->bOcean ? 1 : 0,
					IsValid(Surface->ResolvedOceanMaterialInstance)
						? static_cast<int32>(Surface->ResolvedOceanMaterialInstance->GetBlendMode())
						: INDEX_NONE,
					*GetNameSafe(IsValid(Surface->ResolvedOceanMaterialInstance)
						? Surface->ResolvedOceanMaterialInstance->GetMaterial() : nullptr),
					*GetNameSafe(ExpectedWorldScapeWaterMaster),
					bHasUnusedPhysicalNormalBlend ? 1 : 0,
					PhysicalWaveColorStrength,
					PhysicalWaveNormalStrength,
					PhysicalSurfaceOpacity,
					*GetNameSafe(Surface->ResolvedOceanMaterialInstance),
					*GetNameSafe(Root->OceanMaterial.DefaultMaterial));
				return false;
			}
			// WorldScape workers own the payload arrays until their whole batch commits.
			// Never inspect a partially-written LOD from the automation game thread.
			if (Root->WorldScapeLodInGeneration.Num() > 0)
			{
				bOutPending = true;
				OutFailure = FString::Printf(
					TEXT("wet WorldScape still has %d LOD workers in flight"),
					Root->WorldScapeLodInGeneration.Num());
				return false;
			}

			constexpr int32 ExpectedOceanLodCount = 9;
			if (Root->OceanMaxLod != ExpectedOceanLodCount
				|| Root->WorldScapeLodOcean.Num() > Root->OceanMaxLod)
			{
				OutFailure = FString::Printf(
					TEXT("wet WorldScape owns an invalid/duplicate ocean LOD set oceanLods=%d configured=%d expected=%d"),
					Root->WorldScapeLodOcean.Num(), Root->OceanMaxLod,
					ExpectedOceanLodCount);
				return false;
			}
			if (Root->WorldScapeLodOcean.Num() < Root->OceanMaxLod)
			{
				bOutPending = true;
				OutFailure = FString::Printf(
					TEXT("wet WorldScape ocean LOD set is still incomplete oceanLods=%d expected=%d"),
					Root->WorldScapeLodOcean.Num(), Root->OceanMaxLod);
				return false;
			}

			for (const UWorldScapeLod* TerrainLod : Root->WorldScapeLod)
			{
				if (!IsValid(TerrainLod) || TerrainLod->WaterBody)
				{
					OutFailure = TEXT("authoritative terrain array contains an invalid/ocean LOD");
					return false;
				}
			}

			const FVector RenderObserverWorldPosition = Root->bOverridePlayerPosition
				? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
			const FVector DesiredOceanNormal = Root->WorldToECEF(
				RenderObserverWorldPosition).ToFVector().GetSafeNormal();
			TBitArray<> SeenOceanLodIds(false, Root->OceanMaxLod);
			for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
			{
				if (!IsValid(OceanLod) || !OceanLod->WaterBody
					|| OceanLod->Lod < 0 || OceanLod->Lod >= Root->OceanMaxLod
					|| SeenOceanLodIds[OceanLod->Lod])
				{
					OutFailure = FString::Printf(
						TEXT("wet WorldScape ocean LOD ids are invalid/duplicated lod=%d expectedRange=[0,%d)"),
						IsValid(OceanLod) ? OceanLod->Lod : INDEX_NONE,
						Root->OceanMaxLod);
					return false;
				}
				SeenOceanLodIds[OceanLod->Lod] = true;
			}
			if (!APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(
				Root->WorldScapeLodOcean, Root->OceanMaxLod, true,
				DesiredOceanNormal, false))
			{
				bOutPending = true;
				OutFailure = FString::Printf(
					TEXT("authoritative ocean is not one exact centred contiguous LOD set oceanLods=%d expected=%d workers=%d"),
					Root->WorldScapeLodOcean.Num(), Root->OceanMaxLod,
					Root->WorldScapeLodInGeneration.Num());
				return false;
			}
			TSet<const UWorldScapeLod*> OceanLods;
			TSet<const UPrimitiveComponent*> OceanComponents;
			int32 ValidatedOceanMaterialSlotCount = 0;
			for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
			{
				if (!IsValid(OceanLod) || !OceanLod->WaterBody
					|| Root->WorldScapeLod.Contains(OceanLod)
					|| OceanLods.Contains(OceanLod) || !IsValid(OceanLod->Mesh))
				{
					OutFailure = TEXT("ocean geometry escaped or duplicated outside Root->WorldScapeLodOcean");
					return false;
				}
				OceanLods.Add(OceanLod);
				if (OceanComponents.Contains(OceanLod->Mesh)
					|| OceanLod->Mesh->GetAttachParent() != Root->TransformKeeper)
				{
					OutFailure = TEXT("ocean LOD does not own a unique authoritative TransformKeeper mesh");
					return false;
				}
				OceanComponents.Add(OceanLod->Mesh);
				if (OceanLod->Mesh->GetNumSections() > 3)
				{
					OutFailure = FString::Printf(
						TEXT("authoritative ocean LOD owns unexpected extra sections lod=%d sections=%d"),
						OceanLod->Lod, OceanLod->Mesh->GetNumSections());
					return false;
				}

				if (!APSWorldScapePayloadValidation::HasCompleteCenteredPayload(
					OceanLod, DesiredOceanNormal, false)
					|| !IsEffectivelyPresented(OceanLod->Mesh)
					|| OceanLod->Mesh->GetNumSections() != 3
					|| !OceanLod->Mesh->IsMeshSectionVisible(0)
					|| !OceanLod->Mesh->IsMeshSectionVisible(1)
					|| !OceanLod->Mesh->IsMeshSectionVisible(2))
				{
					bOutPending = true;
					OutFailure = FString::Printf(
						TEXT("authoritative ocean LOD is not yet centred/visible with exactly three sections lod=%d sections=%d mesh=%s"),
						OceanLod->Lod, IsValid(OceanLod->Mesh)
							? OceanLod->Mesh->GetNumSections() : INDEX_NONE,
						*GetNameSafe(OceanLod->Mesh));
					return false;
				}
				for (int32 SectionIndex = 0; SectionIndex < 3; ++SectionIndex)
				{
					UMaterialInterface* SectionMaterial =
						OceanLod->Mesh->GetMaterial(SectionIndex);
					if (SectionMaterial != Surface->ResolvedOceanMaterialInstance)
					{
						OutFailure = FString::Printf(
							TEXT("authoritative ocean LOD section lost the resolved Water MID lod=%d section=%d actual=%s expected=%s"),
							OceanLod->Lod, SectionIndex,
							*GetNameSafe(SectionMaterial),
							*GetNameSafe(Surface->ResolvedOceanMaterialInstance));
						return false;
					}
					++ValidatedOceanMaterialSlotCount;
				}
				if (OceanLod->Mesh->GetCollisionEnabled()
						!= ECollisionEnabled::NoCollision
					|| OceanLod->Mesh->GetGenerateOverlapEvents()
					|| !IgnoresEveryCollisionChannel(OceanLod->Mesh))
				{
					OutFailure = FString::Printf(
						TEXT("authoritative ocean LOD can affect gameplay collision lod=%d mesh=%s collision=%d overlaps=%d ignoreAll=%d"),
						OceanLod->Lod, *GetNameSafe(OceanLod->Mesh),
						static_cast<int32>(OceanLod->Mesh->GetCollisionEnabled()),
						OceanLod->Mesh->GetGenerateOverlapEvents() ? 1 : 0,
						IgnoresEveryCollisionChannel(OceanLod->Mesh) ? 1 : 0);
					return false;
				}
			}
			if (ValidatedOceanMaterialSlotCount != ExpectedOceanLodCount * 3)
			{
				OutFailure = FString::Printf(
					TEXT("wet WorldScape did not validate every ocean material slot validated=%d expected=%d"),
					ValidatedOceanMaterialSlotCount, ExpectedOceanLodCount * 3);
				return false;
			}

			int32 PreviewOceanProxyCount = 0;
			for (AAstroGenerator* Generator : FindActors<AAstroGenerator>(World))
			{
				TInlineComponentArray<UProceduralMeshComponent*> ProceduralMeshes;
				Generator->GetComponents(ProceduralMeshes);
				for (const UProceduralMeshComponent* Mesh : ProceduralMeshes)
				{
					if (!IsPreviewOceanProxy(Mesh))
					{
						continue;
					}
					++PreviewOceanProxyCount;
					if (IsEffectivelyPresented(Mesh)
						|| Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
						|| Mesh->GetGenerateOverlapEvents()
						|| !IgnoresEveryCollisionChannel(Mesh))
					{
						OutFailure = FString::Printf(
							TEXT("gameplay retained a visible/collidable PreviewOcean or PreviewBodyOcean proxy generator=%s mesh=%s visible=%d collision=%d overlaps=%d ignoreAll=%d"),
							*GetNameSafe(Generator), *GetNameSafe(Mesh),
							IsEffectivelyPresented(Mesh) ? 1 : 0,
							static_cast<int32>(Mesh->GetCollisionEnabled()),
							Mesh->GetGenerateOverlapEvents() ? 1 : 0,
							IgnoresEveryCollisionChannel(Mesh) ? 1 : 0);
						return false;
					}
				}
			}

			TSet<const UPrimitiveComponent*> TerrainCollisionComponents;
			for (const UWorldScapeLod* CollisionLod : Root->CollisionLods)
			{
				if (IsValid(CollisionLod) && IsValid(CollisionLod->Mesh)
					&& CollisionLod->Mesh->GetCollisionEnabled()
						!= ECollisionEnabled::NoCollision
					&& !CollisionLod->Vertices.IsEmpty()
					&& !CollisionLod->Triangles.IsEmpty())
				{
					TerrainCollisionComponents.Add(CollisionLod->Mesh);
				}
			}
			if (TerrainCollisionComponents.IsEmpty())
			{
				bOutPending = true;
				OutFailure = TEXT("wet WorldScape terrain CollisionLods are not cooked yet");
				return false;
			}

			const FVector SurfaceCenter = Root->GetActorLocation();
			const FVector TraceOutward = (RenderObserverWorldPosition - SurfaceCenter)
				.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Planet->GetActorUpVector());
			const double OceanRadiusCm = Root->PlanetScale
				+ static_cast<double>(Root->OceanHeight);
			if (TraceOutward.IsNearlyZero() || !FMath::IsFinite(OceanRadiusCm)
				|| OceanRadiusCm <= 0.0)
			{
				OutFailure = TEXT("wet WorldScape produced an invalid ocean trace frame");
				return false;
			}

			FVector TangentA = FVector::ZeroVector;
			FVector TangentB = FVector::ZeroVector;
			TraceOutward.FindBestAxisVectors(TangentA, TangentB);
			const double TangentScale = 1500.0 / Root->PlanetScale;
			const FVector TraceDirections[] = {
				TraceOutward,
				(TraceOutward + TangentA * TangentScale).GetSafeNormal(),
				(TraceOutward - TangentA * TangentScale).GetSafeNormal(),
				(TraceOutward + TangentB * TangentScale).GetSafeNormal(),
				(TraceOutward - TangentB * TangentScale).GetSafeNormal()};
			const ECollisionChannel TraceChannels[] = {ECC_Visibility, ECC_Pawn};
			const double TraceHalfSpanCm = FMath::Max(250000.0,
				FMath::Abs(static_cast<double>(Root->NoiseIntensity)) * 2.0
					+ 100000.0);
			FCollisionQueryParams TraceParams(
				SCENE_QUERY_STAT(APSGeneratedWetOceanHandoff), true,
				RuntimeGravityPawn.Get());
			// Starter infrastructure is a legitimate blocker but is not part of the
			// planetary surface contract. Keep body shells/proxies queryable so a stale
			// liquid or backing sphere still fails as a foreign blocking surface.
			for (ASpaceStation* Station : FindActors<ASpaceStation>(World))
			{
				TraceParams.AddIgnoredActor(Station);
			}
			for (ASpaceship* Spaceship : FindActors<ASpaceship>(World))
			{
				TraceParams.AddIgnoredActor(Spaceship);
			}
			int32 TerrainTraceCount = 0;
			for (const FVector& TraceDirection : TraceDirections)
			{
				const double GroundHeightCm = Root->GetGroundHeight(
					SurfaceCenter + TraceDirection * Root->PlanetScale, false);
				if (!FMath::IsFinite(GroundHeightCm))
				{
					OutFailure = TEXT("wet-ocean radial trace produced a non-finite terrain height");
					return false;
				}
				const double OuterHeightCm = FMath::Max(
					GroundHeightCm, static_cast<double>(Root->OceanHeight))
					+ TraceHalfSpanCm;
				const double InnerHeightCm = FMath::Min(
					GroundHeightCm, static_cast<double>(Root->OceanHeight))
					- TraceHalfSpanCm;
				const FVector TraceStart = SurfaceCenter + TraceDirection
					* (Root->PlanetScale + OuterHeightCm);
				const FVector TraceEnd = SurfaceCenter + TraceDirection
					* (Root->PlanetScale + InnerHeightCm);
				if (FVector::Distance(TraceStart, SurfaceCenter) <= OceanRadiusCm
					|| FVector::Distance(TraceEnd, SurfaceCenter) >= OceanRadiusCm)
				{
					OutFailure = TEXT("wet-ocean radial trace endpoints do not straddle the liquid shell");
					return false;
				}

				for (const ECollisionChannel TraceChannel : TraceChannels)
				{
					TArray<FHitResult> Hits;
					World->LineTraceMultiByChannel(Hits, TraceStart, TraceEnd,
						TraceChannel, TraceParams);
					bool bHitTerrain = false;
					for (const FHitResult& Hit : Hits)
					{
						if (!Hit.bBlockingHit)
						{
							continue;
						}
						const UPrimitiveComponent* HitComponent = Hit.GetComponent();
						if (OceanComponents.Contains(HitComponent))
						{
							OutFailure = FString::Printf(
								TEXT("radial trace was blocked by authoritative ocean mesh=%s channel=%d"),
								*GetNameSafe(HitComponent), static_cast<int32>(TraceChannel));
							return false;
						}
						if (!TerrainCollisionComponents.Contains(HitComponent))
						{
							OutFailure = FString::Printf(
								TEXT("wet-ocean radial trace hit a foreign blocking surface actor=%s component=%s channel=%d"),
								*GetNameSafe(Hit.GetActor()), *GetNameSafe(HitComponent),
								static_cast<int32>(TraceChannel));
							return false;
						}
						bHitTerrain = true;
						break;
					}
					if (bHitTerrain)
					{
						++TerrainTraceCount;
					}
				}
			}

			const int32 RequiredTraceCount = UE_ARRAY_COUNT(TraceDirections)
				* UE_ARRAY_COUNT(TraceChannels);
			if (TerrainTraceCount != RequiredTraceCount)
			{
				bOutPending = true;
				OutFailure = FString::Printf(
					TEXT("wet-ocean radial traces have not all reached authoritative terrain hits=%d required=%d collisionLods=%d"),
					TerrainTraceCount, RequiredTraceCount, Root->CollisionLods.Num());
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.WetOcean] Contract ready root=%s oceanLods=%d previewOceanProxies=%d terrainCollisionLods=%d passThroughTerrainTraces=%d liquid=Water base=M_APS_WorldScapeLiquid materialSlots=27xResolvedMID"),
				*GetNameSafe(Root), Root->WorldScapeLodOcean.Num(),
				PreviewOceanProxyCount, TerrainCollisionComponents.Num(),
				TerrainTraceCount);
			return true;
		}

		bool InitializeWetOceanRenderViews(AWorldScapeRoot* Root, FString& OutFailure)
		{
			OutFailure.Reset();
			if (!IsValid(Root) || !Root->bOcean || Root->OceanMaxLod <= 0)
			{
				OutFailure = TEXT("wet-ocean render proof has no authoritative ocean root");
				return false;
			}

			struct FWetDirectionCandidate
			{
				FVector Outward{FVector::ZeroVector};
				double WaterDepthCm{0.0};
			};
			TArray<FWetDirectionCandidate> Candidates;
			Candidates.Reserve(WetOceanDirectionSampleCount);
			const FVector SurfaceCenter = Root->GetActorLocation();
			const double OceanRadiusCm = Root->PlanetScale
				+ static_cast<double>(Root->OceanHeight);
			const double MinimumWaterDepthCm = FMath::Max(
				1.0, FMath::Abs(static_cast<double>(Root->NoiseIntensity)) * 0.0001);
			const double CollisionCookDistanceBudgetCm =
				static_cast<double>(Root->HeightAnchor)
					* WetOceanCollisionCookHeightAnchorFraction;
			if (!FMath::IsFinite(OceanRadiusCm) || OceanRadiusCm <= 0.0)
			{
				OutFailure = TEXT("wet-ocean render proof resolved an invalid ocean radius");
				return false;
			}
			if (!FMath::IsFinite(CollisionCookDistanceBudgetCm)
				|| CollisionCookDistanceBudgetCm <= MinimumWetOceanCameraClearanceCm)
			{
				OutFailure = TEXT("wet-ocean render proof resolved an invalid collision-cook altitude budget");
				return false;
			}

			constexpr double GoldenAngleRadians = 2.39996322972865332;
			const FQuat RootRotation = Root->GetActorQuat();
			for (int32 SampleIndex = 0;
				SampleIndex < WetOceanDirectionSampleCount; ++SampleIndex)
			{
				const double Z = 1.0 - 2.0
					* (static_cast<double>(SampleIndex) + 0.5)
					/ static_cast<double>(WetOceanDirectionSampleCount);
				const double Radius = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
				const double Angle = GoldenAngleRadians * SampleIndex;
				const FVector LocalDirection(
					Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
				const FVector WorldDirection = RootRotation.RotateVector(
					LocalDirection).GetSafeNormal();
				const double GroundHeightCm = Root->GetGroundHeight(
					SurfaceCenter + WorldDirection * Root->PlanetScale, false);
				const double WaterDepthCm = static_cast<double>(Root->OceanHeight)
					- GroundHeightCm;
				if (!WorldDirection.IsNearlyZero() && FMath::IsFinite(GroundHeightCm)
					&& FMath::IsFinite(WaterDepthCm)
					&& WaterDepthCm >= MinimumWaterDepthCm
					&& WaterDepthCm + MinimumWetOceanCameraClearanceCm
						<= CollisionCookDistanceBudgetCm)
				{
					FWetDirectionCandidate Candidate;
					Candidate.Outward = WorldDirection;
					Candidate.WaterDepthCm = WaterDepthCm;
					Candidates.Add(Candidate);
				}
			}
			Candidates.Sort([](const FWetDirectionCandidate& A,
				const FWetDirectionCandidate& B)
			{
				return A.WaterDepthCm > B.WaterDepthCm;
			});
			if (Candidates.Num() < WetOceanRenderViewCount)
			{
				OutFailure = FString::Printf(
					TEXT("wet-ocean render proof found only %d open-water directions inside the %.2fcm collision-cook altitude budget"),
					Candidates.Num(), CollisionCookDistanceBudgetCm);
				return false;
			}

			int32 SecondCandidateIndex = INDEX_NONE;
			for (int32 CandidateIndex = 1; CandidateIndex < Candidates.Num(); ++CandidateIndex)
			{
				const double DirectionDot = FVector::DotProduct(
					Candidates[0].Outward, Candidates[CandidateIndex].Outward);
				if (DirectionDot <= MaximumWetOceanViewDirectionDot && DirectionDot >= 0.5)
				{
					SecondCandidateIndex = CandidateIndex;
					break;
				}
			}
			if (SecondCandidateIndex == INDEX_NONE)
			{
				for (int32 CandidateIndex = 1;
					CandidateIndex < Candidates.Num(); ++CandidateIndex)
				{
					if (FVector::DotProduct(Candidates[0].Outward,
						Candidates[CandidateIndex].Outward)
						<= MaximumWetOceanViewDirectionDot)
					{
						SecondCandidateIndex = CandidateIndex;
						break;
					}
				}
			}
			if (SecondCandidateIndex == INDEX_NONE)
			{
				OutFailure = TEXT("wet-ocean render proof could not find two water directions separated by fifteen degrees");
				return false;
			}

			const FWetDirectionCandidate Selected[WetOceanRenderViewCount] = {
				Candidates[0], Candidates[SecondCandidateIndex]};
			for (int32 ViewIndex = 0; ViewIndex < WetOceanRenderViewCount; ++ViewIndex)
			{
				FVector TangentA = FVector::ZeroVector;
				FVector TangentB = FVector::ZeroVector;
				Selected[ViewIndex].Outward.FindBestAxisVectors(TangentA, TangentB);
				if (TangentA.IsNearlyZero())
				{
					OutFailure = TEXT("wet-ocean render proof could not build a tangent camera frame");
					return false;
				}
				const double AvailableClearanceCm = CollisionCookDistanceBudgetCm
					- Selected[ViewIndex].WaterDepthCm;
				if (!FMath::IsFinite(AvailableClearanceCm)
					|| AvailableClearanceCm < MinimumWetOceanCameraClearanceCm)
				{
					OutFailure = TEXT("wet-ocean render proof selected a view outside the collision-cook altitude budget");
					return false;
				}
				const double CaptureClearanceCm = FMath::Clamp(
					AvailableClearanceCm * 0.5,
					MinimumWetOceanCameraClearanceCm,
					MaximumWetOceanCameraClearanceCm);
				const FVector CaptureLocation = SurfaceCenter
					+ Selected[ViewIndex].Outward
						* (OceanRadiusCm + CaptureClearanceCm);
				const FRotator CaptureRotation = FRotationMatrix::MakeFromXZ(
					TangentA, Selected[ViewIndex].Outward).Rotator();
				WetOceanCaptureTransforms[ViewIndex] = FTransform(
					CaptureRotation, CaptureLocation, FVector::OneVector);
				WetOceanCaptureWaterDepthsCm[ViewIndex] =
					Selected[ViewIndex].WaterDepthCm;
				WetOceanCaptureClearancesCm[ViewIndex] = CaptureClearanceCm;
			}

			const double DirectionDot = FVector::DotProduct(
				Selected[0].Outward, Selected[1].Outward);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.WetOcean.Render] selected two open-water views depth=[%.2f,%.2f]cm clearance=[%.2f,%.2f]cm collisionCookBudget=%.2fcm directionDot=%.6f cameraTravel=%.2fkm"),
				WetOceanCaptureWaterDepthsCm[0], WetOceanCaptureWaterDepthsCm[1],
				WetOceanCaptureClearancesCm[0], WetOceanCaptureClearancesCm[1],
				CollisionCookDistanceBudgetCm, DirectionDot, FVector::Distance(
					WetOceanCaptureTransforms[0].GetLocation(),
					WetOceanCaptureTransforms[1].GetLocation()) / 100000.0);
			return true;
		}

		bool PinWetOceanRenderView(UWorld* World, FString& OutFailure)
		{
			OutFailure.Reset();
			APlanet* Planet = RuntimeHomePlanet.Get();
			APawn* GravityPawn = RuntimeGravityPawn.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface)
				? Surface->WorldScapeRootInstance : nullptr;
			UAPSPlanetEnvironmentStreamingSubsystem* StreamingSubsystem = World
				? World->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>() : nullptr;
			if (!World || World != GameplayWorld.Get() || !IsValid(Planet)
				|| !IsValid(GravityPawn) || !IsValid(Surface) || !IsValid(Root)
				|| !IsValid(StreamingSubsystem)
				|| WetOceanCaptureIndex < 0
				|| WetOceanCaptureIndex >= WetOceanRenderViewCount)
			{
				OutFailure = TEXT("wet-ocean render view lost its pawn/root/streaming subsystem");
				return false;
			}

			if (UCapsuleComponent* PawnCapsule = FindPawnCapsule(GravityPawn))
			{
				PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
			const FTransform& CaptureTransform =
				WetOceanCaptureTransforms[WetOceanCaptureIndex];
			if (!GravityPawn->SetActorLocationAndRotation(
				CaptureTransform.GetLocation(), CaptureTransform.Rotator(), false,
				nullptr, ETeleportType::TeleportPhysics))
			{
				OutFailure = TEXT("wet-ocean render camera teleport failed");
				return false;
			}
			if (USpringArmComponent* CameraSpringArm = FindPawnSpringArm(GravityPawn))
			{
				CameraSpringArm->bUsePawnControlRotation = false;
				CameraSpringArm->TargetArmLength = 500.0f;
				CameraSpringArm->SetRelativeLocation(FVector(0.0, 0.0, 55.0));
				CameraSpringArm->SetRelativeRotation(FRotator(-22.0, 0.0, 0.0));
			}
			if (UCameraComponent* PawnCamera = FindPawnCamera(GravityPawn))
			{
				PawnCamera->SetFieldOfView(72.0f);
			}
			if (APlayerController* PlayerController = World->GetFirstPlayerController())
			{
				PlayerController->SetControlRotation(CaptureTransform.Rotator());
			}

			StreamingSubsystem->Tick(0.0f);
			Root = Surface->WorldScapeRootInstance;
			const double ObserverDeltaCm = IsValid(Root)
				? FVector::Distance(Root->OverridedPlayerPosition,
					GravityPawn->GetActorLocation())
				: TNumericLimits<double>::Max();
			if (!IsValid(Root) || !Root->bOverridePlayerPosition
				|| !Root->CollisionDependantActor.Contains(GravityPawn)
				|| ObserverDeltaCm > MaximumManualObserverLagCm)
			{
				OutFailure = FString::Printf(
					TEXT("wet-ocean render observer did not follow capture view index=%d override=%d collisionBound=%d delta=%.3fcm"),
					WetOceanCaptureIndex,
					IsValid(Root) && Root->bOverridePlayerPosition ? 1 : 0,
					IsValid(Root) && Root->CollisionDependantActor.Contains(GravityPawn) ? 1 : 0,
					ObserverDeltaCm);
				return false;
			}
			return true;
		}

		bool BeginWetOceanRenderView(UWorld* World, const int32 ViewIndex,
			const double Now, FString& OutFailure)
		{
			if (ViewIndex < 0 || ViewIndex >= WetOceanRenderViewCount)
			{
				OutFailure = TEXT("wet-ocean render view index is out of range");
				return false;
			}
			WetOceanCaptureIndex = ViewIndex;
			bWetOceanCaptureContractReady = false;
			WetOceanStableFramesRemaining = 0;
			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				ViewIndex == 0
					? TEXT("Screenshots/Windows/APS_GeneratedCivilization_WetOcean_A.png")
					: TEXT("Screenshots/Windows/APS_GeneratedCivilization_WetOcean_B.png"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			StepStartSeconds = Now;
			return PinWetOceanRenderView(World, OutFailure);
		}

		bool UpdateWaitForWetOceanScreenshots(UWorld* World, double Now)
		{
			FString ViewFailure;
			if (!PinWetOceanRenderView(World, ViewFailure))
			{
				return Fail(ViewFailure);
			}

			APlanet* Planet = RuntimeHomePlanet.Get();
			APawn* GravityPawn = RuntimeGravityPawn.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface)
				? Surface->WorldScapeRootInstance : nullptr;
			if (!IsValid(Root))
			{
				return Fail(TEXT("wet-ocean rendered proof lost its WorldScape root"));
			}

			if (Root->WorldScapeLodInGeneration.Num() > 0)
			{
				if (bWetOceanCaptureContractReady)
				{
					return Fail(TEXT("wet-ocean LOD generation regressed during a settled rendered view"));
				}
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("wet-ocean view %d did not finish recentering in thirty seconds workers=%d"),
						WetOceanCaptureIndex, Root->WorldScapeLodInGeneration.Num()));
				}
				return false;
			}

			bool bWetContractPending = false;
			FString WetContractFailure;
			if (!ValidateWetOceanContract(World, Planet, Surface, Root,
				bWetContractPending, WetContractFailure))
			{
				if (bWetOceanCaptureContractReady)
				{
					return Fail(FString::Printf(
						TEXT("wet-ocean rendered view regressed after readiness: %s"),
						*WetContractFailure));
				}
				if (bWetContractPending
					&& Now - StepStartSeconds <= PhysicalSurfaceTimeoutSeconds)
				{
					return false;
				}
				return Fail(WetContractFailure.IsEmpty()
					? TEXT("wet-ocean rendered view failed without diagnostics")
					: WetContractFailure);
			}

			if (!bWetOceanCaptureContractReady)
			{
				bWetOceanCaptureContractReady = true;
				WetOceanStableFramesRemaining = RequiredWetOceanStableFrames;
				StepStartSeconds = Now;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.WetOcean.Render] view=%d exact ocean set centred; validating %d stable frames before %s"),
					WetOceanCaptureIndex, RequiredWetOceanStableFrames,
					*ScreenshotPath);
				return false;
			}
			if (WetOceanStableFramesRemaining > 0)
			{
				--WetOceanStableFramesRemaining;
				return false;
			}

			uint32 CapturedFrameCrc = 0;
			FString CaptureFailure;
			if (!CaptureGameplayViewport(
				World, CaptureFailure, false, false, &CapturedFrameCrc))
			{
				if (!CaptureFailure.IsEmpty())
				{
					return Fail(CaptureFailure);
				}
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("wet-ocean viewport pixels were unavailable for ten seconds"));
				}
				return false;
			}

			APlayerController* PlayerController = World->GetFirstPlayerController();
			APlayerCameraManager* CameraManager = IsValid(PlayerController)
				? PlayerController->PlayerCameraManager : nullptr;
			if (!IsValid(CameraManager))
			{
				return Fail(TEXT("wet-ocean rendered proof lost its camera manager"));
			}
			WetOceanCapturedCameraLocations[WetOceanCaptureIndex] =
				CameraManager->GetCameraLocation();
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.WetOcean.Render] captured view=%d crc=%u camera=%s depth=%.2fcm screenshot=%s"),
				WetOceanCaptureIndex, CapturedFrameCrc,
				*WetOceanCapturedCameraLocations[WetOceanCaptureIndex].ToCompactString(),
				WetOceanCaptureWaterDepthsCm[WetOceanCaptureIndex], *ScreenshotPath);

			if (WetOceanCaptureIndex == 0)
			{
				WetOceanFirstFrameCrc = CapturedFrameCrc;
				if (!BeginWetOceanRenderView(World, 1, Now, ViewFailure))
				{
					return Fail(ViewFailure);
				}
				return false;
			}

			const double CameraMovementCm = FVector::Distance(
				WetOceanCapturedCameraLocations[0], WetOceanCapturedCameraLocations[1]);
			if (CapturedFrameCrc == 0 || CapturedFrameCrc == WetOceanFirstFrameCrc
				|| !FMath::IsFinite(CameraMovementCm)
				|| CameraMovementCm < MinimumWetOceanCameraMovementCm)
			{
				return Fail(FString::Printf(
					TEXT("wet-ocean rendered views did not produce two distinct moved frames crcA=%u crcB=%u cameraMovement=%.2fcm minimum=%.2fcm"),
					WetOceanFirstFrameCrc, CapturedFrameCrc, CameraMovementCm,
					MinimumWetOceanCameraMovementCm));
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.WetOcean.Render] PASS views=2 cameraMovement=%.2fkm crcA=%u crcB=%u exactOceanLods=%d"),
				CameraMovementCm / 100000.0, WetOceanFirstFrameCrc,
				CapturedFrameCrc, Root->WorldScapeLodOcean.Num());
			Step = EStep::Cleanup;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForGameplaySurface(UWorld* World, double Now)
		{
			APlanet* Planet = RuntimeHomePlanet.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
			AAtmoScape* Atmosphere = IsValid(Surface) ? Surface->PlanetAtmosphere : nullptr;
			if (!World || World != GameplayWorld.Get() || !IsValid(Planet) || !IsValid(Surface)
				|| !IsValid(Root) || !IsValid(Atmosphere))
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(TEXT("gameplay WorldScape actors were unavailable for thirty seconds"));
				}
				return false;
			}

			const FAPSResolvedPlanetSurfaceProfile ExpectedProfile =
				UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Surface->SurfaceProfileCatalog);
			const uint32 ExpectedSignature =
				UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(ExpectedProfile);
			const uint32 AppliedSignature =
				UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Surface->ResolvedSurfaceProfile);
			const FVector RenderObserverWorldPosition = Root->bOverridePlayerPosition
				? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
			const FVector RenderObserverNormal = Root->WorldToECEF(
				RenderObserverWorldPosition).ToFVector().GetSafeNormal();
			const bool bCompleteTerrainPayload = Root->WorldScapeLod.Num() >= Root->MaxLod
				&& !Root->WorldScapeLod.ContainsByPredicate([](const UWorldScapeLod* Lod)
				{
					return !APSWorldScapePayloadValidation::HasCompletePayload(Lod, true);
				});
			const bool bCompleteOceanPayload = !Root->bOcean
				|| APSWorldScapePayloadValidation::HasExactCenteredPayloadSet(
					Root->WorldScapeLodOcean, Root->OceanMaxLod, true,
					RenderObserverNormal, false);
			const int32 PresentedBodyBackingMeshes = CountPresentedStaticMeshes(Planet);
			const int32 CollidableBodyBackingMeshes = CountCollidableStaticMeshes(Planet);
			const TArray<AAstroGenerator*> RuntimeGenerators = FindActors<AAstroGenerator>(World);
			const int32 PresentedRuntimePreviewMeshes = RuntimeGenerators.IsEmpty()
				? 0 : CountPresentedProceduralMeshes(RuntimeGenerators[0]);
			int32 AbsorptionShellCount = 0;
			int32 PresentedAbsorptionShellCount = 0;
			int32 OuterAirglowShellCount = 0;
			int32 PresentedOuterAirglowShellCount = 0;
			int32 HiddenOuterAirglowShellCount = 0;
			int32 SkylightShellCount = 0;
			int32 PresentedSkylightShellCount = 0;
			int32 HiddenSkylightShellCount = 0;
			int32 MainAtmosphereShellCount = 0;
			int32 PresentedMainAtmosphereShellCount = 0;
			int32 SpaceAtmosphereShellCount = 0;
			int32 HiddenSpaceAtmosphereShellCount = 0;
			TInlineComponentArray<UStaticMeshComponent*> AtmosphereMeshes;
			Atmosphere->GetComponents(AtmosphereMeshes);
			for (const UStaticMeshComponent* AtmosphereMesh : AtmosphereMeshes)
			{
				if (!IsValid(AtmosphereMesh))
				{
					continue;
				}
				const FString ComponentName = AtmosphereMesh->GetName();
				if (ComponentName.Contains(TEXT("PlanetaryAbsorptionMesh")))
				{
					++AbsorptionShellCount;
					if (IsEffectivelyPresented(AtmosphereMesh))
					{
						++PresentedAbsorptionShellCount;
					}
				}
				else if (ComponentName.Contains(TEXT("PlanetarOutterMesh")))
				{
					++OuterAirglowShellCount;
					if (IsEffectivelyPresented(AtmosphereMesh))
					{
						++PresentedOuterAirglowShellCount;
					}
					if (AtmosphereMesh->bHiddenInGame)
					{
						++HiddenOuterAirglowShellCount;
					}
				}
				else if (ComponentName.Contains(TEXT("PlanetarySkylightMesh")))
				{
					++SkylightShellCount;
					if (IsEffectivelyPresented(AtmosphereMesh))
					{
						++PresentedSkylightShellCount;
					}
					if (AtmosphereMesh->bHiddenInGame)
					{
						++HiddenSkylightShellCount;
					}
				}
				else if (ComponentName.Contains(TEXT("SpacePlanetaryAtmoMesh")))
				{
					++SpaceAtmosphereShellCount;
					if (AtmosphereMesh->bHiddenInGame)
					{
						++HiddenSpaceAtmosphereShellCount;
					}
				}
				else if (ComponentName.Contains(TEXT("PlanetaryAtmoMesh")))
				{
					++MainAtmosphereShellCount;
					if (IsEffectivelyPresented(AtmosphereMesh))
					{
						++PresentedMainAtmosphereShellCount;
					}
				}
			}
			if (bValidateWetOceanContract
				&& (OuterAirglowShellCount != 1
					|| PresentedOuterAirglowShellCount != 0
					|| HiddenOuterAirglowShellCount != 1
					|| SkylightShellCount != 1
					|| PresentedSkylightShellCount != 0
					|| HiddenSkylightShellCount != 1
					|| MainAtmosphereShellCount != 1
					|| PresentedMainAtmosphereShellCount != 1
					|| SpaceAtmosphereShellCount != 1
					|| HiddenSpaceAtmosphereShellCount != 0))
			{
				return Fail(FString::Printf(
					TEXT("wet-ocean ground atmosphere did not retain one physical scattering shell while suppressing coplanar cap passes and preserving the orbital shell outerShells=%d outerPresented=%d outerHidden=%d skylightShells=%d skylightPresented=%d skylightHidden=%d mainShells=%d mainPresented=%d spaceShells=%d spaceHidden=%d"),
					OuterAirglowShellCount, PresentedOuterAirglowShellCount,
					HiddenOuterAirglowShellCount, SkylightShellCount,
					PresentedSkylightShellCount, HiddenSkylightShellCount,
					MainAtmosphereShellCount, PresentedMainAtmosphereShellCount,
					SpaceAtmosphereShellCount, HiddenSpaceAtmosphereShellCount));
			}
			FVisibleWorldScapeRenderLodProof InitialRenderProof;
			FString InitialRenderFailure;
			const bool bInitialRenderLod0Ready = BuildVisibleWorldScapeRenderLod0Proof(
				Root, RenderObserverWorldPosition, InitialRenderProof, InitialRenderFailure);
			if (!Surface->IsSurfaceProfileCurrent(Planet)
				|| Surface->ResolvedSurfaceProfile.PlanetType != ExpectedPlanetType
				|| ExpectedSignature != AppliedSignature
				|| !Cast<UAPSWorldScapePlanetNoise>(Surface->ResolvedNoiseInstance)
				|| Root->WorldScapeNoise != Surface->ResolvedNoiseInstance
				|| !Surface->ResolvedTerrainMaterialInstance
				|| Root->TerrainMaterial.DefaultMaterial != Surface->ResolvedTerrainMaterialInstance
				|| Atmosphere->bKeepRelativeScale || Atmosphere->LightSource != Planet->ParentStar
				|| AbsorptionShellCount != 1 || PresentedAbsorptionShellCount != 0
				|| !FMath::IsNearlyEqual(Planet->WorldScapePresentationScale, 1.0)
				|| Planet->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
				|| PresentedRuntimePreviewMeshes != 0)
			{
				return Fail(FString::Printf(
					TEXT("gameplay surface bypassed resolver or is not the sole complete renderer/collider type=%d expectedSig=%u appliedSig=%u lods=%d workers=%d scale=%.9f backingMeshes=%d backingColliders=%d previewMeshes=%d absorptionShells=%d absorptionPresented=%d renderLod0=%d renderFailure=%s renderVertices=%d renderRelief=%.3fcm renderNoiseDelta=%.3fcm renderCenterOffset=%.3fcm"),
					static_cast<int32>(Surface->ResolvedSurfaceProfile.PlanetType), ExpectedSignature,
					AppliedSignature, Root->WorldScapeLod.Num(),
					Root->WorldScapeLodInGeneration.Num(), Planet->WorldScapePresentationScale,
					PresentedBodyBackingMeshes, CollidableBodyBackingMeshes,
					PresentedRuntimePreviewMeshes, AbsorptionShellCount,
					PresentedAbsorptionShellCount, bInitialRenderLod0Ready ? 1 : 0,
					*InitialRenderFailure, InitialRenderProof.VertexCount,
					InitialRenderProof.ReliefVariationCm,
					InitialRenderProof.MaximumNoiseDeltaCm,
					InitialRenderProof.ObserverCenterOffsetCm));
			}

			// Render and collision construction is asynchronous.  Workers in flight,
			// incomplete payloads, a hidden root and a not-yet-centred LOD0 are transient
			// states, not resolver failures.  The old immediate failure happened on the
			// first frame after travel and never gave the production finalizer a chance to
			// publish the pawn-centred surface it deliberately waits for.
			const bool bGameplaySurfaceReady = Planet->bWorldScapeSurfaceReady
				&& Root->WorldScapeLodInGeneration.Num() == 0
				&& bCompleteTerrainPayload && bCompleteOceanPayload
				&& !Root->IsHidden() && bInitialRenderLod0Ready
				&& PresentedBodyBackingMeshes == 0
				&& CollidableBodyBackingMeshes == 0;
			if (!bGameplaySurfaceReady)
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("gameplay WorldScape did not publish a pawn-centred render LOD0 in thirty seconds ready=%d lods=%d workers=%d hidden=%d renderLod0=%d renderFailure=%s"),
						Planet->bWorldScapeSurfaceReady ? 1 : 0,
						Root->WorldScapeLod.Num(), Root->WorldScapeLodInGeneration.Num(),
						Root->IsHidden() ? 1 : 0, bInitialRenderLod0Ready ? 1 : 0,
						*InitialRenderFailure));
				}
				return false;
			}

			if (bValidateWetOceanContract)
			{
				bool bWetContractPending = false;
				FString WetContractFailure;
				if (!ValidateWetOceanContract(World, Planet, Surface, Root,
					bWetContractPending, WetContractFailure))
				{
					if (bWetContractPending
						&& Now - StepStartSeconds <= PhysicalSurfaceTimeoutSeconds)
					{
						return false;
					}
					return Fail(WetContractFailure.IsEmpty()
						? TEXT("wet-ocean gameplay contract failed without diagnostics")
						: WetContractFailure);
				}

				FString WetRenderFailure;
				if (!InitializeWetOceanRenderViews(Root, WetRenderFailure)
					|| !BeginWetOceanRenderView(World, 0, Now, WetRenderFailure))
				{
					return Fail(WetRenderFailure.IsEmpty()
						? TEXT("wet-ocean rendered proof initialization failed")
						: WetRenderFailure);
				}
				Step = EStep::WaitForWetOceanScreenshots;
				return false;
			}

			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				TEXT("Screenshots/Windows/APS_GeneratedCivilization_Handoff.png"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Gameplay WorldScape ready in %.2fs resolverSig=%u lods=%d renderLod0Vertices=%d renderRelief=%.3fcm renderNoiseDelta=%.3fcm renderCenterOffset=%.3fcm; settling game viewport for %s"),
				Now - StepStartSeconds, AppliedSignature, Root->WorldScapeLod.Num(),
				InitialRenderProof.VertexCount, InitialRenderProof.ReliefVariationCm,
				InitialRenderProof.MaximumNoiseDeltaCm,
				InitialRenderProof.ObserverCenterOffsetCm, *ScreenshotPath);
			ScreenshotSettleFramesRemaining = 2;
			Step = EStep::WaitForScreenshot;
			StepStartSeconds = Now;
			return false;
		}

		bool CaptureGameplayViewport(UWorld* World, FString& OutFailure,
			bool bValidateStationSubject = true, bool bValidateGroundLowerRegion = false,
			uint32* OutFrameCrc = nullptr)
		{
			OutFailure.Reset();
			if (OutFrameCrc)
			{
				*OutFrameCrc = 0;
			}
			UGameViewportClient* GameViewportClient = AutomationCommon::GetAnyGameViewportClient();
			FViewport* GameViewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
			if (!World || !GameViewportClient || GameViewportClient->GetWorld() != World || !GameViewport)
			{
				return false;
			}

			const FIntPoint ViewportSize = GameViewport->GetSizeXY();
			if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
			{
				return false;
			}
			TArray<FColor> Pixels;
			if (!GameViewport->ReadPixels(Pixels)
				|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
			{
				return false;
			}
			const uint32 FrameCrc = FCrc::MemCrc32(Pixels.GetData(),
				static_cast<int32>(Pixels.Num() * sizeof(FColor)));
			if (OutFrameCrc)
			{
				*OutFrameCrc = FrameCrc;
			}

			TArray64<uint8> PngData;
			FImageUtils::PNGCompressImageArray(ViewportSize.X, ViewportSize.Y,
				TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
			if (PngData.IsEmpty() || !FFileHelper::SaveArrayToFile(PngData, *ScreenshotPath))
			{
				OutFailure = FString::Printf(
					TEXT("could not encode/write gameplay viewport screenshot %s"), *ScreenshotPath);
				return false;
			}

			double BrightnessSum = 0.0;
			double BrightnessSquaredSum = 0.0;
			int64 NonBlackPixelCount = 0;
			for (const FColor& Pixel : Pixels)
			{
				const double Brightness = static_cast<double>(
					FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
				BrightnessSum += Brightness;
				BrightnessSquaredSum += Brightness * Brightness;
				NonBlackPixelCount += Brightness > 8.0 ? 1 : 0;
			}
			const double PixelCount = static_cast<double>(Pixels.Num());
			const double MeanBrightness = BrightnessSum / PixelCount;
			const double BrightnessVariance = FMath::Max(
				0.0, BrightnessSquaredSum / PixelCount - MeanBrightness * MeanBrightness);
			const double NonBlackPixelRatio = static_cast<double>(NonBlackPixelCount) / PixelCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] captured GAME viewport=%dx%d meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f crc=%u screenshot=%s"),
				ViewportSize.X, ViewportSize.Y, MeanBrightness, BrightnessVariance,
				NonBlackPixelRatio, FrameCrc, *ScreenshotPath);
			if (NonBlackPixelRatio < MinimumNonBlackPixelRatio
				|| BrightnessVariance < MinimumBrightnessVariance)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay camera frame is blank/flat meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f (minimum ratio=%.5f variance=%.3f)"),
					MeanBrightness, BrightnessVariance, NonBlackPixelRatio,
					MinimumNonBlackPixelRatio, MinimumBrightnessVariance);
				return false;
			}
			if (bValidateGroundLowerRegion)
			{
				// The middle/lower gameplay region is terrain in the generated
				// PlanetSurface view. Ignore the outer panels and bottom HUD while
				// ensuring a technically displaced surface is also visibly lit.
				const int32 GroundMinX = FMath::FloorToInt(ViewportSize.X * 0.20);
				const int32 GroundMaxX = FMath::CeilToInt(ViewportSize.X * 0.80);
				const int32 GroundMinY = FMath::FloorToInt(ViewportSize.Y * 0.55);
				const int32 GroundMaxY = FMath::CeilToInt(ViewportSize.Y * 0.88);
				double GroundBrightnessSum = 0.0;
				double GroundBrightnessSquaredSum = 0.0;
				double GroundSpatialDeltaSum = 0.0;
				int64 GroundNonBlackPixelCount = 0;
				int64 GroundPixelCount = 0;
				int64 GroundSpatialDeltaCount = 0;
				TArray<uint8> GroundBrightnessValues;
				GroundBrightnessValues.Reserve(
					(GroundMaxX - GroundMinX) * (GroundMaxY - GroundMinY));
				for (int32 Y = GroundMinY; Y < GroundMaxY; ++Y)
				{
					for (int32 X = GroundMinX; X < GroundMaxX; ++X)
					{
						const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
						const uint8 BrightnessByte = FMath::Max3(
							Pixel.R, Pixel.G, Pixel.B);
						const double Brightness = static_cast<double>(BrightnessByte);
						GroundBrightnessSum += Brightness;
						GroundBrightnessSquaredSum += Brightness * Brightness;
						GroundNonBlackPixelCount += Brightness > 8.0 ? 1 : 0;
						GroundBrightnessValues.Add(BrightnessByte);
						if (X > GroundMinX)
						{
							const FColor& LeftPixel = Pixels[Y * ViewportSize.X + X - 1];
							const double LeftBrightness = static_cast<double>(FMath::Max3(
								LeftPixel.R, LeftPixel.G, LeftPixel.B));
							GroundSpatialDeltaSum += FMath::Abs(Brightness - LeftBrightness);
							++GroundSpatialDeltaCount;
						}
						if (Y > GroundMinY)
						{
							const FColor& UpperPixel = Pixels[(Y - 1) * ViewportSize.X + X];
							const double UpperBrightness = static_cast<double>(FMath::Max3(
								UpperPixel.R, UpperPixel.G, UpperPixel.B));
							GroundSpatialDeltaSum += FMath::Abs(Brightness - UpperBrightness);
							++GroundSpatialDeltaCount;
						}
						++GroundPixelCount;
					}
				}
				const double GroundMeanBrightness = GroundPixelCount > 0
					? GroundBrightnessSum / static_cast<double>(GroundPixelCount) : 0.0;
				const double GroundBrightnessVariance = GroundPixelCount > 0
					? FMath::Max(0.0,
						GroundBrightnessSquaredSum / static_cast<double>(GroundPixelCount)
						- GroundMeanBrightness * GroundMeanBrightness) : 0.0;
				const double GroundNonBlackPixelRatio = GroundPixelCount > 0
					? static_cast<double>(GroundNonBlackPixelCount)
						/ static_cast<double>(GroundPixelCount) : 0.0;
				const double GroundMeanSpatialDelta = GroundSpatialDeltaCount > 0
					? GroundSpatialDeltaSum / static_cast<double>(GroundSpatialDeltaCount) : 0.0;
				GroundBrightnessValues.Sort();
				const int32 GroundLastIndex = GroundBrightnessValues.Num() - 1;
				const double GroundP10 = GroundLastIndex >= 0
					? static_cast<double>(GroundBrightnessValues[
						FMath::FloorToInt(static_cast<double>(GroundLastIndex) * 0.10)]) : 0.0;
				const double GroundP90 = GroundLastIndex >= 0
					? static_cast<double>(GroundBrightnessValues[
						FMath::FloorToInt(static_cast<double>(GroundLastIndex) * 0.90)]) : 0.0;
				const double GroundBrightnessSpread = GroundP90 - GroundP10;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.PhysicalSurface] groundROI=(%d,%d)-(%d,%d) meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f p10=%.1f p90=%.1f spread=%.1f meanSpatialDelta=%.3f"),
					GroundMinX, GroundMinY, GroundMaxX, GroundMaxY,
					GroundMeanBrightness, GroundBrightnessVariance,
					GroundNonBlackPixelRatio, GroundP10, GroundP90,
					GroundBrightnessSpread, GroundMeanSpatialDelta);
				if (GroundMeanBrightness < MinimumGroundMeanBrightness
					|| GroundP10 < MinimumGroundP10Brightness
					|| GroundNonBlackPixelRatio < MinimumGroundNonBlackPixelRatio
					|| GroundBrightnessVariance < MinimumGroundBrightnessVariance
					|| GroundBrightnessSpread < MinimumGroundBrightnessSpread
					|| GroundMeanSpatialDelta < MinimumGroundMeanSpatialDelta)
				{
					OutFailure = FString::Printf(
						TEXT("settled WorldScape ground region is dark/flat mean=%.3f p10=%.1f variance=%.3f nonBlack=%.5f spread=%.1f spatialDelta=%.3f (minimum mean=%.3f p10=%.1f variance=%.3f ratio=%.3f spread=%.1f spatialDelta=%.3f)"),
						GroundMeanBrightness, GroundP10, GroundBrightnessVariance,
						GroundNonBlackPixelRatio, GroundBrightnessSpread,
						GroundMeanSpatialDelta, MinimumGroundMeanBrightness,
						MinimumGroundP10Brightness,
						MinimumGroundBrightnessVariance,
						MinimumGroundNonBlackPixelRatio,
						MinimumGroundBrightnessSpread,
						MinimumGroundMeanSpatialDelta);
					return false;
				}
			}
			if (!bValidateStationSubject)
			{
				return true;
			}

			APawn* GravityPawn = RuntimeGravityPawn.Get();
			ASpaceStation* Station = RuntimeStation.Get();
			APlayerController* PlayerController = World->GetFirstPlayerController();
			if (!IsValid(GravityPawn) || !IsValid(Station) || !IsValid(Station->SpawnPoint)
				|| !IsValid(PlayerController))
			{
				OutFailure = FString::Printf(
					TEXT("gameplay subject ROI has invalid contract pawn=%s station=%s spawnPoint=%s controller=%s"),
					*GetNameSafe(GravityPawn), *GetNameSafe(Station),
					*GetNameSafe(Station ? Station->SpawnPoint : nullptr),
					*GetNameSafe(PlayerController));
				return false;
			}

			FVector2D PawnScreenPosition = FVector2D::ZeroVector;
			FVector2D StationScreenPosition = FVector2D::ZeroVector;
			const bool bPawnProjected = PlayerController->ProjectWorldLocationToScreen(
				GravityPawn->GetActorLocation(), PawnScreenPosition, false);
			const bool bStationProjected = PlayerController->ProjectWorldLocationToScreen(
				Station->SpawnPoint->GetComponentLocation(), StationScreenPosition, false);
			if (!bPawnProjected || !bStationProjected
				|| !FMath::IsFinite(PawnScreenPosition.X) || !FMath::IsFinite(PawnScreenPosition.Y)
				|| !FMath::IsFinite(StationScreenPosition.X) || !FMath::IsFinite(StationScreenPosition.Y))
			{
				OutFailure = FString::Printf(
					TEXT("could not project gameplay subject pawnProjected=%d stationProjected=%d pawnScreen=%s stationScreen=%s"),
					bPawnProjected ? 1 : 0, bStationProjected ? 1 : 0,
					*PawnScreenPosition.ToString(), *StationScreenPosition.ToString());
				return false;
			}

			const FVector2D SubjectScreenPosition =
				(PawnScreenPosition + StationScreenPosition) * 0.5;
			if (SubjectScreenPosition.X < 0.0 || SubjectScreenPosition.Y < 0.0
				|| SubjectScreenPosition.X >= ViewportSize.X
				|| SubjectScreenPosition.Y >= ViewportSize.Y)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay pawn/station subject is outside viewport anchor=%s viewport=%dx%d"),
					*SubjectScreenPosition.ToString(), ViewportSize.X, ViewportSize.Y);
				return false;
			}
			const int32 SubjectHalfWidth = FMath::Clamp(
				FMath::RoundToInt(static_cast<double>(ViewportSize.X) * 0.10), 96, 240);
			const int32 SubjectHalfHeight = FMath::Clamp(
				FMath::RoundToInt(static_cast<double>(ViewportSize.Y) * 0.18), 96, 180);
			const int32 SubjectMinX = FMath::Clamp(
				FMath::FloorToInt(SubjectScreenPosition.X) - SubjectHalfWidth,
				0, ViewportSize.X - 1);
			const int32 SubjectMinY = FMath::Clamp(
				FMath::FloorToInt(SubjectScreenPosition.Y) - SubjectHalfHeight,
				0, ViewportSize.Y - 1);
			const int32 SubjectMaxX = FMath::Clamp(
				FMath::CeilToInt(SubjectScreenPosition.X) + SubjectHalfWidth,
				SubjectMinX + 1, ViewportSize.X);
			const int32 SubjectMaxY = FMath::Clamp(
				FMath::CeilToInt(SubjectScreenPosition.Y) + SubjectHalfHeight,
				SubjectMinY + 1, ViewportSize.Y);

			double SubjectBrightnessSum = 0.0;
			double SubjectBrightnessSquaredSum = 0.0;
			int64 SubjectNonBlackPixelCount = 0;
			int64 SubjectPixelCount = 0;
			for (int32 Y = SubjectMinY; Y < SubjectMaxY; ++Y)
			{
				for (int32 X = SubjectMinX; X < SubjectMaxX; ++X)
				{
					const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
					const double Brightness = static_cast<double>(
						FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
					SubjectBrightnessSum += Brightness;
					SubjectBrightnessSquaredSum += Brightness * Brightness;
					SubjectNonBlackPixelCount += Brightness > 8.0 ? 1 : 0;
					++SubjectPixelCount;
				}
			}
			const double SubjectMeanBrightness = SubjectBrightnessSum / SubjectPixelCount;
			const double SubjectBrightnessVariance = FMath::Max(
				0.0, SubjectBrightnessSquaredSum / SubjectPixelCount
					- SubjectMeanBrightness * SubjectMeanBrightness);
			const double SubjectNonBlackPixelRatio =
				static_cast<double>(SubjectNonBlackPixelCount) / SubjectPixelCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] subjectROI=(%d,%d)-(%d,%d) pawnScreen=%s stationSpawnScreen=%s meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f"),
				SubjectMinX, SubjectMinY, SubjectMaxX, SubjectMaxY,
				*PawnScreenPosition.ToString(), *StationScreenPosition.ToString(),
				SubjectMeanBrightness, SubjectBrightnessVariance,
				SubjectNonBlackPixelRatio);
			if (SubjectMeanBrightness < MinimumSubjectMeanBrightness
				|| SubjectNonBlackPixelRatio < MinimumSubjectNonBlackPixelRatio
				|| SubjectBrightnessVariance < MinimumSubjectBrightnessVariance)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay pawn/station ROI is unreadable meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f (minimum mean=%.3f ratio=%.3f variance=%.3f)"),
					SubjectMeanBrightness, SubjectBrightnessVariance,
					SubjectNonBlackPixelRatio, MinimumSubjectMeanBrightness,
					MinimumSubjectNonBlackPixelRatio,
					MinimumSubjectBrightnessVariance);
				return false;
			}
			return true;
		}

		bool UpdateWaitForScreenshot(UWorld* World, double Now)
		{
			if (!World || World != GameplayWorld.Get())
			{
				return false;
			}
			if (ScreenshotSettleFramesRemaining > 0)
			{
				--ScreenshotSettleFramesRemaining;
				return false;
			}

			FString CaptureFailure;
			if (!CaptureGameplayViewport(World, CaptureFailure, false, false))
			{
				if (!CaptureFailure.IsEmpty())
				{
					return Fail(CaptureFailure);
				}
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("game viewport pixels were unavailable for ten seconds"));
				}
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Non-blank game viewport screenshot written; probing natural physical WorldScape surface"));
			Step = EStep::WaitForPhysicalSurface;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateValidateManualApproachObserver(UWorld* World, double Now)
		{
			APlanet* Planet = RuntimeHomePlanet.Get();
			APawn* GravityPawn = RuntimeGravityPawn.Get();
			ASpaceStation* Station = RuntimeStation.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
			UAPSPlanetEnvironmentStreamingSubsystem* StreamingSubsystem = World
				? World->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>() : nullptr;
			if (!World || World != GameplayWorld.Get() || !IsValid(Planet)
				|| !IsValid(GravityPawn) || !IsValid(Station) || !IsValid(Station->SpawnPoint)
				|| !IsValid(Surface) || !IsValid(Root) || !IsValid(StreamingSubsystem))
			{
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("manual approach observer actors/root/subsystem were unavailable for ten seconds"));
				}
				return false;
			}

			UCapsuleComponent* PawnCapsule = FindPawnCapsule(GravityPawn);
			auto TeleportAndRefreshObserver = [&](const FVector& TargetLocation,
				const TCHAR* Stage, double& OutObserverDeltaCm, FString& OutFailure)
			{
				if (IsValid(PawnCapsule))
				{
					PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
				}
				if (!GravityPawn->SetActorLocation(TargetLocation, false, nullptr,
					ETeleportType::TeleportPhysics))
				{
					OutFailure = FString::Printf(TEXT("manual %s teleport failed"), Stage);
					return false;
				}

				// This zero-delta tick is a deterministic single-frame regression check.
				// It must update only the already-bound visual observer; the expensive
				// family/body selection remains governed by the subsystem's 0.5 s cadence.
				StreamingSubsystem->Tick(0.0f);
				Root = Surface->WorldScapeRootInstance;
				if (!IsValid(Root))
				{
					OutFailure = FString::Printf(
						TEXT("manual %s approach lost the active WorldScape root"), Stage);
					return false;
				}

				OutObserverDeltaCm = FVector::Distance(
					Root->OverridedPlayerPosition, GravityPawn->GetActorLocation());
				const bool bCollisionObserverBound =
					Root->CollisionDependantActor.Contains(GravityPawn);
				if (!Root->bOverridePlayerPosition || !bCollisionObserverBound
					|| OutObserverDeltaCm > MaximumManualObserverLagCm)
				{
					OutFailure = FString::Printf(
						TEXT("manual %s observer did not follow pawn in one tick override=%d collisionBound=%d delta=%.3fcm maximum=%.3fcm"),
						Stage, Root->bOverridePlayerPosition ? 1 : 0,
						bCollisionObserverBound ? 1 : 0, OutObserverDeltaCm,
						MaximumManualObserverLagCm);
					return false;
				}
				return true;
			};

			// Recreate the user's route without ResolveSpawnLocation and without the
			// later best-patch search: first visit the authored station/orbit point...
			const FVector OrbitLocation = Station->SpawnPoint->GetComponentLocation();
			double OrbitObserverDeltaCm = 0.0;
			FString ManualApproachFailure;
			if (!TeleportAndRefreshObserver(OrbitLocation, TEXT("station/orbit"),
				OrbitObserverDeltaCm, ManualApproachFailure))
			{
				return Fail(ManualApproachFailure);
			}

			// ...then move to an arbitrary near-surface direction. The small tangent
			// offset guarantees a real traversal while deliberately avoiding all terrain
			// height/resolver sampling used by the physical-relief proof that follows.
			FVector Outward = (OrbitLocation - Root->GetActorLocation()).GetSafeNormal();
			if (Outward.IsNearlyZero())
			{
				Outward = Planet->GetActorUpVector().GetSafeNormal();
			}
			if (Outward.IsNearlyZero())
			{
				Outward = FVector::UpVector;
			}
			FVector TangentA = FVector::ZeroVector;
			FVector TangentB = FVector::ZeroVector;
			Outward.FindBestAxisVectors(TangentA, TangentB);
			const FVector ApproachDirection =
				(Outward + TangentA * 0.015 + TangentB * 0.007).GetSafeNormal();
			const double ApproachClearanceCm = FMath::Max(
				1000000.0, Root->PlanetScale * 0.002);
			const FVector NearSurfaceLocation = Root->GetActorLocation()
				+ ApproachDirection * (Root->PlanetScale + ApproachClearanceCm);
			if (NearSurfaceLocation.ContainsNaN()
				|| FVector::Distance(OrbitLocation, NearSurfaceLocation) < 100000.0)
			{
				return Fail(TEXT("manual station-to-surface route did not produce a finite meaningful traversal"));
			}

			double SurfaceObserverDeltaCm = 0.0;
			if (!TeleportAndRefreshObserver(NearSurfaceLocation, TEXT("near-surface"),
				SurfaceObserverDeltaCm, ManualApproachFailure))
			{
				return Fail(ManualApproachFailure);
			}
			if (Planet->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
				|| Root->IsHidden())
			{
				return Fail(FString::Printf(
					TEXT("manual approach hid/deactivated WorldScape state=%d hidden=%d"),
					static_cast<int32>(Planet->GetWorldScapeStreamingState()),
					Root->IsHidden() ? 1 : 0));
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.ManualApproach] PASS route=station/orbit->near-surface resolver=not-called bestPatch=not-selected orbitDelta=%.3fcm surfaceDelta=%.3fcm traversal=%.2fkm collisionObserver=bound"),
				OrbitObserverDeltaCm, SurfaceObserverDeltaCm,
				FVector::Distance(OrbitLocation, NearSurfaceLocation) / 100000.0);
			bManualApproachObserverValidated = true;
			Step = EStep::WaitForPhysicalSurface;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForPhysicalSurface(UWorld* World, double Now)
		{
			AAstroGenerator* Generator = RuntimeGenerator.Get();
			APlanet* Planet = RuntimeHomePlanet.Get();
			APawn* GravityPawn = RuntimeGravityPawn.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
			UCapsuleComponent* PawnCapsule = FindPawnCapsule(GravityPawn);
			if (!World || World != GameplayWorld.Get() || !IsValid(Generator)
				|| !IsValid(Planet) || !IsValid(GravityPawn) || !IsValid(Surface)
				|| !IsValid(Root))
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(TEXT("physical surface actors/root were unavailable for 20 seconds"));
				}
				return false;
			}

			if (!bPhysicalSurfaceProbeInitialized)
			{
				// Authored body meshes and semantic influence/gravity volumes may provide
				// visuals and overlaps, but they must never be the smooth walkable shell.
				// WorldScape's displaced CollisionLods are the sole physical terrain.
				const int32 AuthoredBodyColliderCount = CountCollidableStaticMeshes(Planet);
				const bool bPlanetaryZoneBlocksPawn = !IsValid(Planet->PlanetaryZone)
					|| Planet->PlanetaryZone->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
				const bool bGravityZoneBlocksPawn = !IsValid(Planet->GravityCollisionZone)
					|| Planet->GravityCollisionZone->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
				if (AuthoredBodyColliderCount != 0 || bPlanetaryZoneBlocksPawn
					|| bGravityZoneBlocksPawn)
				{
					return Fail(FString::Printf(
						TEXT("authored planet shell/zone can still become smooth ground staticMeshColliders=%d planetaryZoneBlocksPawn=%d gravityZoneBlocksPawn=%d"),
						AuthoredBodyColliderCount, bPlanetaryZoneBlocksPawn ? 1 : 0,
						bGravityZoneBlocksPawn ? 1 : 0));
				}

				if (!Surface->IsSurfaceProfileCurrent(Planet)
					|| !Cast<UAPSWorldScapePlanetNoise>(Surface->ResolvedNoiseInstance)
					|| Root->WorldScapeNoise != Surface->ResolvedNoiseInstance
					|| !Root->bGenerateCollision || !Root->bGenerateCollisionForAllPlayer
#if WITH_EDITOR
					|| !Root->bGenerateCollisionInEditor || Root->bStaticCollisionInEditor
#endif
					)
				{
					return Fail(FString::Printf(
						TEXT("active WorldScape collision contract is invalid noise=%s rootNoise=%s runtime=%d allPlayers=%d"),
						*GetNameSafe(Surface->ResolvedNoiseInstance),
						*GetNameSafe(Root->WorldScapeNoise), Root->bGenerateCollision ? 1 : 0,
						Root->bGenerateCollisionForAllPlayer ? 1 : 0));
				}

				// Validate the real handoff before the visual-proof code below is allowed
				// to relocate the pawn. This specifically covers BP_CustomGravityCharacter:
				// production must resolve, bind planet gravity and ground its capsule on an
				// actual WorldScape CollisionLod without help from the automation test.
				if (!bNaturalSurfaceLandingValidated)
				{
					if (!IsValid(PawnCapsule))
					{
						return Fail(TEXT("BP_CustomGravityCharacter has no capsule component"));
					}

					TSet<const UPrimitiveComponent*> NaturalCollisionComponents;
					for (const UWorldScapeLod* CollisionLod : Root->CollisionLods)
					{
						if (IsValid(CollisionLod) && IsValid(CollisionLod->Mesh)
							&& CollisionLod->Mesh->GetCollisionEnabled()
								!= ECollisionEnabled::NoCollision
							&& !CollisionLod->Vertices.IsEmpty()
							&& !CollisionLod->Triangles.IsEmpty())
						{
							NaturalCollisionComponents.Add(CollisionLod->Mesh);
						}
					}
					if (NaturalCollisionComponents.IsEmpty())
					{
						if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
						{
							return Fail(TEXT("natural surface start produced no WorldScape collision near BP_CustomGravityCharacter"));
						}
						return false;
					}

					const FVector SurfaceCenter = Root->GetActorLocation();
					const FVector PawnOutward = (GravityPawn->GetActorLocation() - SurfaceCenter)
						.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Planet->GetActorUpVector());
					const double ExpectedHeightCm = Root->GetGroundHeight(
						SurfaceCenter + PawnOutward * Root->PlanetScale, false);
					const double TraceHalfSpanCm = FMath::Max(250000.0,
						FMath::Abs(static_cast<double>(Root->NoiseIntensity)) * 2.0
							+ 100000.0);
					const FVector TraceStart = SurfaceCenter + PawnOutward
						* (Root->PlanetScale + ExpectedHeightCm + TraceHalfSpanCm);
					const FVector TraceEnd = SurfaceCenter + PawnOutward
						* (Root->PlanetScale + ExpectedHeightCm - TraceHalfSpanCm);
					FCollisionQueryParams NaturalTraceParams(
						SCENE_QUERY_STAT(APSGeneratedNaturalSurfaceStart), true, GravityPawn);
					FHitResult NaturalTerrainHit;
					// CollisionLods are a moving WorldScape pool: the plugin can destroy and
					// replace their mesh components when its snapped player/camera cell changes.
					// A component-local query is not a reliable proof of the live Chaos scene
					// after such a replacement (and, in PIE, can keep returning false while a
					// world query already resolves the freshly published component). Query the
					// authoritative world scene, then require the returned component to belong
					// to the CollisionLods collected in this same frame. This remains strict:
					// authored shells, oceans and stale/replaced WorldScape components cannot
					// satisfy the handoff contract.
					TSet<const UPrimitiveComponent*> NaturalOceanComponents;
					for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
					{
						if (IsValid(OceanLod) && IsValid(OceanLod->Mesh))
						{
							if (OceanLod->Mesh->GetCollisionEnabled()
								!= ECollisionEnabled::NoCollision)
							{
								return Fail(TEXT("natural production landing exposed a collidable WorldScape ocean shell"));
							}
							NaturalOceanComponents.Add(OceanLod->Mesh);
						}
					}
					TArray<FHitResult> NaturalWorldHits;
					World->LineTraceMultiByChannel(NaturalWorldHits, TraceStart, TraceEnd,
						ECC_Visibility, NaturalTraceParams);
					for (const FHitResult& CandidateHit : NaturalWorldHits)
					{
						const UPrimitiveComponent* HitComponent = CandidateHit.GetComponent();
						if (CandidateHit.bBlockingHit
							&& !NaturalOceanComponents.Contains(HitComponent)
							&& NaturalCollisionComponents.Contains(HitComponent))
						{
							NaturalTerrainHit = CandidateHit;
							break;
						}
					}

					EGravityType GravityType = EGravityType::ZeroG;
					FVector GravityDirection = FVector::ZeroVector;
					AActor* GravityTarget = nullptr;
					const bool bGravityReady = ReadGravityContract(
						GravityPawn, GravityType, GravityDirection, GravityTarget)
						&& GravityType == EGravityType::OnPlanet
						&& GravityTarget == Planet
						&& FVector::DotProduct(GravityDirection.GetSafeNormal(), -PawnOutward) > 0.95;
					const double CapsuleHalfHeightCm =
						PawnCapsule->GetScaledCapsuleHalfHeight();
					const double FootClearanceCm = NaturalTerrainHit.bBlockingHit
						? FVector::Distance(GravityPawn->GetActorLocation(), SurfaceCenter)
							- FVector::Distance(NaturalTerrainHit.ImpactPoint, SurfaceCenter)
							- CapsuleHalfHeightCm
						: TNumericLimits<double>::Max();
					const double PawnSpeedCmPerSecond = GravityPawn->GetVelocity().Size();
					const bool bNaturallyGrounded = NaturalTerrainHit.bBlockingHit
						&& FMath::Abs(FootClearanceCm) <= MaximumFootClearanceCm
						&& PawnSpeedCmPerSecond <= MaximumSettledSpeedCmPerSecond
						&& bGravityReady;
					NaturalSurfaceSettleFrames = bNaturallyGrounded
						? NaturalSurfaceSettleFrames + 1 : 0;
					if (NaturalSurfaceSettleFrames < RequiredNaturalSettleFrames)
					{
						if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
						{
							return Fail(FString::Printf(
								TEXT("BP_CustomGravityCharacter did not naturally settle on production WorldScape terrain frames=%d hit=%d worldHits=%d currentTerrainComponents=%d footClearance=%.2fcm speed=%.2f gravityType=%d gravityTarget=%s"),
								NaturalSurfaceSettleFrames,
								NaturalTerrainHit.bBlockingHit ? 1 : 0,
								NaturalWorldHits.Num(), NaturalCollisionComponents.Num(),
								FootClearanceCm, PawnSpeedCmPerSecond,
								static_cast<int32>(GravityType),
								*GetNameSafe(GravityTarget)));
						}
						return false;
					}

					// Prove the untouched production landing itself is visibly displaced.
					// The later physical-proof phase deliberately teleports to a best patch;
					// it must not be able to hide a flat natural spawn. Sample the same
					// UAPSWorldScapePlanetNoise heightfield used by collision at 10/100/250 m,
					// and require both the published render LOD0 and cooked hit to agree
					// with that field before any test relocation is allowed.
					const double NaturalTraceHeightCm = FVector::Distance(
						NaturalTerrainHit.ImpactPoint, SurfaceCenter) - Root->PlanetScale;
					const double NaturalHeightfieldCollisionDeltaCm = FMath::Abs(
						NaturalTraceHeightCm - ExpectedHeightCm);
					FVisibleWorldScapeRenderLodProof NaturalRenderProof;
					FString NaturalRenderFailure;
					if (!BuildVisibleWorldScapeRenderLod0Proof(Root,
						GravityPawn->GetActorLocation(), NaturalRenderProof,
						NaturalRenderFailure))
					{
						return Fail(FString::Printf(
							TEXT("natural production landing has no matching visible WorldScape LOD0: %s"),
							*NaturalRenderFailure));
					}
					const double NaturalRenderCollisionDeltaCm = FMath::Abs(
						NaturalRenderProof.ClosestObserverHeightCm - NaturalTraceHeightCm);
					const double MaximumNaturalRenderSampleDistanceCm = FMath::Max(
						static_cast<double>(Root->TriangleSize) * 4.0, 500.0);
					if (!FMath::IsFinite(NaturalRenderCollisionDeltaCm)
						|| NaturalRenderCollisionDeltaCm
							> MaximumNaturalHeightfieldCollisionDeltaCm
						|| NaturalRenderProof.ClosestObserverSurfaceDistanceCm
							> MaximumNaturalRenderSampleDistanceCm)
					{
						return Fail(FString::Printf(
							TEXT("natural production landing render/noise/collision diverged renderCollisionDelta=%.2fcm nearestRenderSample=%.2fcm maximumSample=%.2fcm"),
							NaturalRenderCollisionDeltaCm,
							NaturalRenderProof.ClosestObserverSurfaceDistanceCm,
							MaximumNaturalRenderSampleDistanceCm));
					}
					const FVector NaturalReferenceAxis = FMath::Abs(PawnOutward.Z) < 0.82
						? FVector::UpVector : FVector::ForwardVector;
					const FVector NaturalTangentU = FVector::CrossProduct(
						NaturalReferenceAxis, PawnOutward).GetSafeNormal(
							UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
					const FVector NaturalTangentV = FVector::CrossProduct(
						PawnOutward, NaturalTangentU).GetSafeNormal(
							UE_DOUBLE_SMALL_NUMBER, FVector::RightVector);
					constexpr double NaturalProbeDistancesCm[3] = {
						1000.0, 10000.0, 25000.0};
					double NaturalMinHeightsCm[3] = {
						ExpectedHeightCm, ExpectedHeightCm, ExpectedHeightCm};
					double NaturalMaxHeightsCm[3] = {
						ExpectedHeightCm, ExpectedHeightCm, ExpectedHeightCm};
					double NaturalMaxSlope = 0.0;
					for (int32 DirectionIndex = 0;
						DirectionIndex < PhysicalProofDirectionCount; ++DirectionIndex)
					{
						const double Angle = UE_TWO_PI * static_cast<double>(DirectionIndex)
							/ static_cast<double>(PhysicalProofDirectionCount);
						const FVector Tangent = NaturalTangentU * FMath::Cos(Angle)
							+ NaturalTangentV * FMath::Sin(Angle);
						for (int32 DistanceIndex = 0; DistanceIndex < 3; ++DistanceIndex)
						{
							const double DistanceCm = NaturalProbeDistancesCm[DistanceIndex];
							const FVector SampleDirection = (PawnOutward
								+ Tangent * (DistanceCm / Root->PlanetScale)).GetSafeNormal();
							const double SampleHeightCm = Root->GetGroundHeight(
								SurfaceCenter + SampleDirection * Root->PlanetScale, false);
							if (!FMath::IsFinite(SampleHeightCm))
							{
								return Fail(TEXT("natural production landing returned a non-finite WorldScape height sample"));
							}
							NaturalMinHeightsCm[DistanceIndex] = FMath::Min(
								NaturalMinHeightsCm[DistanceIndex], SampleHeightCm);
							NaturalMaxHeightsCm[DistanceIndex] = FMath::Max(
								NaturalMaxHeightsCm[DistanceIndex], SampleHeightCm);
							NaturalMaxSlope = FMath::Max(NaturalMaxSlope,
								FMath::Abs(SampleHeightCm - ExpectedHeightCm) / DistanceCm);
						}
					}
					const double NaturalRange10mCm =
						NaturalMaxHeightsCm[0] - NaturalMinHeightsCm[0];
					const double NaturalRange100mCm =
						NaturalMaxHeightsCm[1] - NaturalMinHeightsCm[1];
					const double NaturalRange250mCm =
						NaturalMaxHeightsCm[2] - NaturalMinHeightsCm[2];
					if (NaturalHeightfieldCollisionDeltaCm
							> MaximumNaturalHeightfieldCollisionDeltaCm
						|| NaturalRange10mCm < MinimumNaturalRange10mCm
						|| NaturalRange100mCm < MinimumProofRange100mCm
						|| NaturalRange250mCm < MinimumProofRange250mCm
						|| NaturalMaxSlope > MaximumProofSlope)
					{
						return Fail(FString::Printf(
							TEXT("natural production landing is not the same readable WorldScape heightfield collisionDelta=%.2fcm range10m=%.2fcm range100m=%.2fcm range250m=%.2fcm maxSlope=%.5f"),
							NaturalHeightfieldCollisionDeltaCm, NaturalRange10mCm,
							NaturalRange100mCm, NaturalRange250mCm,
							NaturalMaxSlope));
					}

					// Preserve the exact production-selected patch before the independent
					// manual approach regression moves the pawn away from it.  The final
					// render/collision proof returns here; it never searches for or teleports
					// to a more favourable automation-only relief patch.
					PhysicalSurfaceProbeOutward = PawnOutward;
					PhysicalSurfaceSpawnLocation = GravityPawn->GetActorLocation();
					PhysicalSurfaceProofForward = FVector::VectorPlaneProject(
						GravityPawn->GetActorForwardVector(), PawnOutward).GetSafeNormal();
					if (PhysicalSurfaceProofForward.IsNearlyZero())
					{
						PhysicalSurfaceProofForward = NaturalTangentU;
					}
					bNaturalSurfaceLandingValidated = true;
					UE_LOG(LogTemp, Display,
						TEXT("[APS.Handoff.NaturalSurface] PASS pawn=%s body=%s terrainComponent=%s footClearance=%.2fcm speed=%.2f gravityType=%d frames=%d heightfieldDelta=%.2fcm renderCollisionDelta=%.2fcm relief=[%.2f,%.2f,%.2f]cm maxSlope=%.5f location=%s"),
						*GetNameSafe(GravityPawn), *GetNameSafe(Planet),
						*GetNameSafe(NaturalTerrainHit.GetComponent()), FootClearanceCm,
						PawnSpeedCmPerSecond, static_cast<int32>(GravityType),
						NaturalSurfaceSettleFrames, NaturalHeightfieldCollisionDeltaCm,
						NaturalRenderCollisionDeltaCm, NaturalRange10mCm,
						NaturalRange100mCm, NaturalRange250mCm, NaturalMaxSlope,
						*GravityPawn->GetActorLocation().ToCompactString());
				}

				// Preserve the production landing proof above before simulating the user's
				// separate station/orbit -> manual approach route. The manual phase returns
				// here, then the existing resolver/best-patch visual proof may relocate the pawn.
				if (!bManualApproachObserverValidated)
				{
					Step = EStep::ValidateManualApproachObserver;
					StepStartSeconds = Now;
					return false;
				}

				// The manual approach phase intentionally moved the pawn. Return to the
				// exact direction selected and validated by production, not to an
				// automation-only global best patch.
				if (PhysicalSurfaceProbeOutward.IsNearlyZero())
				{
					return Fail(TEXT("natural production landing direction was not preserved"));
				}
				const FVector SurfaceCenter = Root->GetActorLocation();
				PhysicalSurfaceProbeOutward = PhysicalSurfaceProbeOutward.GetSafeNormal();
				const double NaturalSelectedHeightCm = Root->GetGroundHeight(
					SurfaceCenter + PhysicalSurfaceProbeOutward * Root->PlanetScale, false);
				const double CapsuleHalfHeightCm = IsValid(PawnCapsule)
					? PawnCapsule->GetScaledCapsuleHalfHeight() : 0.0;
				const double ProductionPatchClearanceCm = FMath::Max(
					500.0, CapsuleHalfHeightCm + 100.0);
				if (!FMath::IsFinite(NaturalSelectedHeightCm))
				{
					return Fail(TEXT("preserved production landing returned a non-finite WorldScape height"));
				}
				PhysicalSurfaceSpawnLocation = SurfaceCenter + PhysicalSurfaceProbeOutward
					* (Root->PlanetScale + NaturalSelectedHeightCm
						+ ProductionPatchClearanceCm);
				if (IsValid(PawnCapsule))
				{
					PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
				}
				if (!GravityPawn->SetActorLocation(PhysicalSurfaceSpawnLocation, false,
					nullptr, ETeleportType::TeleportPhysics))
				{
					return Fail(TEXT("could not return gameplay pawn to natural production landing"));
				}
				bPhysicalSurfaceProbeInitialized = true;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.PhysicalSurface] restored natural production patch spawn=%s height=%.2fcm clearance=%.2fcm capsuleHalf=%.2fcm"),
					*PhysicalSurfaceSpawnLocation.ToCompactString(),
					NaturalSelectedHeightCm, ProductionPatchClearanceCm,
					CapsuleHalfHeightCm);
				return false;

#if 0 // Superseded: the smoke must never hide a flat production spawn with a best-patch teleport.
				const ECharSpawnPlace PreviousSpawnPlace = Generator->CharSpawnPlace;
				Generator->CharSpawnPlace = ECharSpawnPlace::PlanetSurface;
				FVector ResolvedSurfaceSpawnLocation = FVector::ZeroVector;
				const bool bResolvedSurfaceSpawn = Generator->ResolveSpawnLocation(
					nullptr, ResolvedSurfaceSpawnLocation);
				Generator->CharSpawnPlace = PreviousSpawnPlace;
				Surface = Planet->PlanetaryEnvironmentGenerator;
				Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
				if (!bResolvedSurfaceSpawn || !IsValid(Root)
					|| ResolvedSurfaceSpawnLocation.ContainsNaN())
				{
					return Fail(TEXT("PlanetSurface spawn could not resolve against the active WorldScape root"));
				}

				const FVector SurfaceCenter = Root->GetActorLocation();
				const FVector ResolvedSpawnOutward = (ResolvedSurfaceSpawnLocation - SurfaceCenter)
					.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Planet->GetActorUpVector());
				const double ResolvedHeightCm = Root->GetGroundHeight(
					SurfaceCenter + ResolvedSpawnOutward * Root->PlanetScale, false);
				const double ResolvedSpawnClearanceCm = FVector::Distance(
					ResolvedSurfaceSpawnLocation, SurfaceCenter)
					- Root->PlanetScale - ResolvedHeightCm;
				const double CapsuleHalfHeightCm = IsValid(PawnCapsule)
					? PawnCapsule->GetScaledCapsuleHalfHeight() : 0.0;
				if (!FMath::IsFinite(ResolvedHeightCm)
					|| !FMath::IsFinite(ResolvedSpawnClearanceCm)
					|| ResolvedSpawnClearanceCm < CapsuleHalfHeightCm + 25.0
					|| ResolvedSpawnClearanceCm > 1000.0)
				{
					return Fail(FString::Printf(
						TEXT("PlanetSurface spawn did not use WorldScape height plus capsule clearance height=%.2f clearance=%.2f capsuleHalf=%.2f"),
						ResolvedHeightCm, ResolvedSpawnClearanceCm, CapsuleHalfHeightCm));
				}

				// ResolveSpawnLocation proves the production handoff. For the visual proof,
				// deterministically choose a dry Cryogenic patch that contains measurable
				// near-, mid- and far-field geometry. This is still the exact same
				// UAPSWorldScapePlanetNoise queried by render and collision generation.
				TArray<FVector> CandidateDirections;
				CandidateDirections.Reserve(PhysicalProofCandidateCount + 1);
				CandidateDirections.Add(ResolvedSpawnOutward);
				const double GoldenAngle = UE_PI * (3.0 - FMath::Sqrt(5.0));
				for (int32 CandidateIndex = 0;
					CandidateIndex < PhysicalProofCandidateCount; ++CandidateIndex)
				{
					const double UnitZ = 1.0 - 2.0
						* (static_cast<double>(CandidateIndex) + 0.5)
						/ static_cast<double>(PhysicalProofCandidateCount);
					const double UnitRadius = FMath::Sqrt(FMath::Max(0.0, 1.0 - UnitZ * UnitZ));
					const double Azimuth = GoldenAngle * static_cast<double>(CandidateIndex);
					CandidateDirections.Add(FVector(
						UnitRadius * FMath::Cos(Azimuth),
						UnitRadius * FMath::Sin(Azimuth), UnitZ));
				}

				constexpr double ProofDistancesCm[3] = {10000.0, 25000.0, 100000.0};
				const double OceanHeightCm = static_cast<double>(
					Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity;
				double BestCandidateScore = -TNumericLimits<double>::Max();
				double BestCandidateHeightCm = 0.0;
				double BestCandidateRangesCm[3] = {0.0, 0.0, 0.0};
				double BestCandidateMaxSlope = 0.0;
				FVector BestCandidateOutward = FVector::ZeroVector;
				FVector BestCandidateForward = FVector::ZeroVector;
				TArray<double> DryCandidateRanges100mCm;
				DryCandidateRanges100mCm.Reserve(CandidateDirections.Num());
				for (const FVector& CandidateDirectionValue : CandidateDirections)
				{
					const FVector CandidateOutward = CandidateDirectionValue.GetSafeNormal();
					const double CandidateHeightCm = Root->GetGroundHeight(
						SurfaceCenter + CandidateOutward * Root->PlanetScale, false);
					if (!FMath::IsFinite(CandidateHeightCm)
						|| CandidateHeightCm - OceanHeightCm < 5000.0)
					{
						continue;
					}

					const FVector ReferenceAxis = FMath::Abs(CandidateOutward.Z) < 0.82
						? FVector::UpVector : FVector::ForwardVector;
					const FVector TangentU = FVector::CrossProduct(
						ReferenceAxis, CandidateOutward).GetSafeNormal();
					const FVector TangentV = FVector::CrossProduct(
						CandidateOutward, TangentU).GetSafeNormal();
					double MinHeightsCm[3] = {
						CandidateHeightCm, CandidateHeightCm, CandidateHeightCm};
					double MaxHeightsCm[3] = {
						CandidateHeightCm, CandidateHeightCm, CandidateHeightCm};
					double CandidateMaxSlope = 0.0;
					double BestDirectionScore = -TNumericLimits<double>::Max();
					FVector CandidateForward = TangentU;
					for (int32 DirectionIndex = 0;
						DirectionIndex < PhysicalProofDirectionCount; ++DirectionIndex)
					{
						const double Angle = UE_TWO_PI * static_cast<double>(DirectionIndex)
							/ static_cast<double>(PhysicalProofDirectionCount);
						const FVector Tangent = TangentU * FMath::Cos(Angle)
							+ TangentV * FMath::Sin(Angle);
						double DirectionScore = 0.0;
						for (int32 DistanceIndex = 0; DistanceIndex < 3; ++DistanceIndex)
						{
							const double DistanceCm = ProofDistancesCm[DistanceIndex];
							const FVector SampleDirection = (CandidateOutward
								+ Tangent * (DistanceCm / Root->PlanetScale)).GetSafeNormal();
							const double SampleHeightCm = Root->GetGroundHeight(
								SurfaceCenter + SampleDirection * Root->PlanetScale, false);
							if (!FMath::IsFinite(SampleHeightCm))
							{
								DirectionScore = -TNumericLimits<double>::Max();
								break;
							}
							MinHeightsCm[DistanceIndex] = FMath::Min(
								MinHeightsCm[DistanceIndex], SampleHeightCm);
							MaxHeightsCm[DistanceIndex] = FMath::Max(
								MaxHeightsCm[DistanceIndex], SampleHeightCm);
							const double HeightDeltaCm = FMath::Abs(
								SampleHeightCm - CandidateHeightCm);
							CandidateMaxSlope = FMath::Max(
								CandidateMaxSlope, HeightDeltaCm / DistanceCm);
							const double TargetRangeCm = DistanceIndex == 0
								? MinimumProofRange100mCm : (DistanceIndex == 1
									? MinimumProofRange250mCm : MinimumProofRange1kmCm);
							DirectionScore += HeightDeltaCm / TargetRangeCm;
						}
						if (DirectionScore > BestDirectionScore)
						{
							BestDirectionScore = DirectionScore;
							CandidateForward = Tangent;
						}
					}

					const double CandidateRangesCm[3] = {
						MaxHeightsCm[0] - MinHeightsCm[0],
						MaxHeightsCm[1] - MinHeightsCm[1],
						MaxHeightsCm[2] - MinHeightsCm[2]};
					const bool bDryNeighbourhood = MinHeightsCm[2] - OceanHeightCm >= 1000.0;
					if (!bDryNeighbourhood || CandidateMaxSlope > MaximumProofSlope)
					{
						continue;
					}
					DryCandidateRanges100mCm.Add(CandidateRangesCm[0]);
					const double CandidateScore =
						CandidateRangesCm[0] / MinimumProofRange100mCm
						+ CandidateRangesCm[1] / MinimumProofRange250mCm
						+ CandidateRangesCm[2] / MinimumProofRange1kmCm
						+ BestDirectionScore * 0.25;
					if (CandidateScore > BestCandidateScore)
					{
						BestCandidateScore = CandidateScore;
						BestCandidateHeightCm = CandidateHeightCm;
						BestCandidateRangesCm[0] = CandidateRangesCm[0];
						BestCandidateRangesCm[1] = CandidateRangesCm[1];
						BestCandidateRangesCm[2] = CandidateRangesCm[2];
						BestCandidateMaxSlope = CandidateMaxSlope;
						BestCandidateOutward = CandidateOutward;
						BestCandidateForward = CandidateForward;
					}
				}

				DryCandidateRanges100mCm.Sort();
				const int32 P10Index = DryCandidateRanges100mCm.IsEmpty() ? INDEX_NONE
					: FMath::Clamp(FMath::FloorToInt(
						static_cast<double>(DryCandidateRanges100mCm.Num() - 1) * 0.10),
						0, DryCandidateRanges100mCm.Num() - 1);
				const double DryPatchP10Range100mCm = P10Index == INDEX_NONE
					? 0.0 : DryCandidateRanges100mCm[P10Index];
				if (DryPatchP10Range100mCm < MinimumProofP10Range100mCm)
				{
					return Fail(FString::Printf(
						TEXT("standard Frozen profile is broadly flat at 100m p10=%.2fcm minimum=%.2fcm dryPatches=%d"),
						DryPatchP10Range100mCm, MinimumProofP10Range100mCm,
						DryCandidateRanges100mCm.Num()));
				}

				if (BestCandidateOutward.IsNearlyZero()
					|| BestCandidateRangesCm[0] < MinimumProofRange100mCm
					|| BestCandidateRangesCm[1] < MinimumProofRange250mCm
					|| BestCandidateRangesCm[2] < MinimumProofRange1kmCm)
				{
					return Fail(FString::Printf(
						TEXT("standard Frozen profile has no dry readable relief patch range100m=%.2fcm range250m=%.2fcm range1km=%.2fcm maxSlope=%.5f"),
						BestCandidateRangesCm[0], BestCandidateRangesCm[1],
						BestCandidateRangesCm[2], BestCandidateMaxSlope));
				}

				PhysicalSurfaceProbeOutward = BestCandidateOutward;
				PhysicalSurfaceProofForward = BestCandidateForward;
				const double ProofSpawnClearanceCm = FMath::Max(
					500.0, CapsuleHalfHeightCm + 100.0);
				PhysicalSurfaceSpawnLocation = SurfaceCenter + BestCandidateOutward
					* (Root->PlanetScale + BestCandidateHeightCm + ProofSpawnClearanceCm);

				if (IsValid(PawnCapsule))
				{
					PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
				}
				if (!GravityPawn->SetActorLocation(PhysicalSurfaceSpawnLocation, false,
					nullptr, ETeleportType::TeleportPhysics))
				{
					return Fail(TEXT("could not teleport gameplay pawn to resolved physical surface probe"));
				}
				bPhysicalSurfaceProbeInitialized = true;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.PhysicalSurface] resolverSpawn=%s resolverHeight=%.2fcm resolverClearance=%.2fcm proofSpawn=%s proofHeight=%.2fcm p10Range100m=%.2fcm dryPatches=%d ranges=[%.2f,%.2f,%.2f]cm maxSlope=%.5f capsuleHalf=%.2fcm"),
					*ResolvedSurfaceSpawnLocation.ToCompactString(), ResolvedHeightCm,
					ResolvedSpawnClearanceCm, *PhysicalSurfaceSpawnLocation.ToCompactString(),
					BestCandidateHeightCm, DryPatchP10Range100mCm,
					DryCandidateRanges100mCm.Num(), BestCandidateRangesCm[0],
					BestCandidateRangesCm[1], BestCandidateRangesCm[2],
					BestCandidateMaxSlope, CapsuleHalfHeightCm);
				return false;
#endif
			}

			// Keep the pawn at the selected landing point until WorldScape has cooked the
			// first local collision patch. This isolates collision generation from the
			// pawn's short gravity fall; the resulting trace still queries real physics.
			if (IsValid(PawnCapsule))
			{
				PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
			GravityPawn->SetActorLocation(PhysicalSurfaceSpawnLocation, false,
				nullptr, ETeleportType::TeleportPhysics);
			const FVector PhysicalRenderObserverWorldPosition = Root->bOverridePlayerPosition
				? Root->OverridedPlayerPosition : Root->PlayerWorldPos.ToFVector();
			FVisibleWorldScapeRenderLodProof PhysicalRenderProof;
			FString PhysicalRenderFailure;
			if (!BuildVisibleWorldScapeRenderLod0Proof(Root,
				PhysicalRenderObserverWorldPosition, PhysicalRenderProof,
				PhysicalRenderFailure))
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("visible WorldScape terrain LOD0 never represented the physical probe: %s vertices=%d relief=%.3fcm noiseDelta=%.3fcm centerOffset=%.3fcm"),
						*PhysicalRenderFailure, PhysicalRenderProof.VertexCount,
						PhysicalRenderProof.ReliefVariationCm,
						PhysicalRenderProof.MaximumNoiseDeltaCm,
						PhysicalRenderProof.ObserverCenterOffsetCm));
				}
				return false;
			}
			if (Root->CollisionLods.IsEmpty())
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("WorldScape produced no collision LOD near gameplay pawn in 20 seconds runtime=%d allPlayers=%d editor=%d"),
						Root->bGenerateCollision ? 1 : 0,
						Root->bGenerateCollisionForAllPlayer ? 1 : 0,
#if WITH_EDITOR
						Root->bGenerateCollisionInEditor ? 1 : 0));
#else
						0));
#endif
				}
				return false;
			}

			TSet<UPrimitiveComponent*> CollisionComponents;
			double MinCollisionHeightCm = TNumericLimits<double>::Max();
			double MaxCollisionHeightCm = -TNumericLimits<double>::Max();
			int32 CollisionVertexCount = 0;
			for (const UWorldScapeLod* CollisionLod : Root->CollisionLods)
			{
				if (!IsValid(CollisionLod) || !IsValid(CollisionLod->Mesh)
					|| CollisionLod->Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					|| CollisionLod->Vertices.IsEmpty() || CollisionLod->Triangles.IsEmpty())
				{
					continue;
				}
				CollisionComponents.Add(CollisionLod->Mesh);
				const FTransform MeshTransform = CollisionLod->Mesh->GetComponentTransform();
				for (const FVector& LocalVertex : CollisionLod->Vertices)
				{
					const double RadialHeightCm = FVector::Distance(
						MeshTransform.TransformPosition(LocalVertex), Root->GetActorLocation())
						- Root->PlanetScale;
					if (FMath::IsFinite(RadialHeightCm))
					{
						MinCollisionHeightCm = FMath::Min(MinCollisionHeightCm, RadialHeightCm);
						MaxCollisionHeightCm = FMath::Max(MaxCollisionHeightCm, RadialHeightCm);
						++CollisionVertexCount;
					}
				}
			}
			if (CollisionComponents.IsEmpty() || CollisionVertexCount < 3)
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(TEXT("WorldScape collision LODs exist but contain no cooked terrain geometry"));
				}
				return false;
			}

			for (const UWorldScapeLod* OceanLod : Root->WorldScapeLodOcean)
			{
				if (IsValid(OceanLod) && IsValid(OceanLod->Mesh)
					&& OceanLod->Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
				{
					return Fail(TEXT("WorldScape ocean presentation LOD became a gameplay collider"));
				}
			}

			const FVector SurfaceCenter = Root->GetActorLocation();
			const FVector ProbeOutward = PhysicalSurfaceProbeOutward.GetSafeNormal(
				UE_DOUBLE_SMALL_NUMBER, (PhysicalSurfaceSpawnLocation - SurfaceCenter)
					.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Planet->GetActorUpVector()));
			const double ExpectedHeightCm = Root->GetGroundHeight(
				SurfaceCenter + ProbeOutward * Root->PlanetScale, false);
			const FVector TraceStart = SurfaceCenter + ProbeOutward
				* (Root->PlanetScale + ExpectedHeightCm + 100000.0);
			const FVector TraceEnd = SurfaceCenter + ProbeOutward
				* (Root->PlanetScale + ExpectedHeightCm - 100000.0);
			FCollisionQueryParams TraceParams(
				SCENE_QUERY_STAT(APSGeneratedPhysicalWorldScapeSurface), true, GravityPawn);
			FHitResult SurfaceHit;
			if (!World->LineTraceSingleByChannel(
				SurfaceHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams)
				|| !CollisionComponents.Contains(SurfaceHit.GetComponent()))
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("ground trace did not hit WorldScape collision component hitActor=%s hitComponent=%s collisionLods=%d"),
						*GetNameSafe(SurfaceHit.GetActor()), *GetNameSafe(SurfaceHit.GetComponent()),
						Root->CollisionLods.Num()));
				}
				return false;
			}

			// Prove that the *current physical collision* around the production landing
			// point is the same displaced surface as WorldScape noise.  The old global
			// collision-vertex range plus one central trace could still pass when a broad
			// flat shell happened to coexist with displaced geometry elsewhere.
			const FVector LocalReferenceAxis = FMath::Abs(ProbeOutward.Z) < 0.82
				? FVector::UpVector : FVector::ForwardVector;
			const FVector LocalTangentU = FVector::CrossProduct(
				LocalReferenceAxis, ProbeOutward).GetSafeNormal(
					UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
			const FVector LocalTangentV = FVector::CrossProduct(
				ProbeOutward, LocalTangentU).GetSafeNormal(
					UE_DOUBLE_SMALL_NUMBER, FVector::RightVector);
			// The production collision patch is roughly 74 m wide.  Physical probes
			// must stay inside that patch; wider relief remains covered by the
			// independent analytic/render checks.
			constexpr double LocalProbeDistancesCm[3] = {
				500.0, 1500.0, 3000.0};
			constexpr double MinimumLocalRangesCm[3] = {
				MinimumLocalCollisionRange5mCm,
				MinimumLocalCollisionRange15mCm,
				MinimumLocalCollisionRange30mCm};
			const double CentralCollisionHeightCm = FVector::Distance(
				SurfaceHit.ImpactPoint, SurfaceCenter) - Root->PlanetScale;
			const double CentralNoiseHeightCm = Root->GetGroundHeight(
				SurfaceCenter + ProbeOutward * Root->PlanetScale, false);
			if (!FMath::IsFinite(CentralCollisionHeightCm)
				|| !FMath::IsFinite(CentralNoiseHeightCm)
				|| FMath::Abs(CentralCollisionHeightCm - CentralNoiseHeightCm)
					> MaximumLocalCollisionNoiseDeltaCm)
			{
				return Fail(FString::Printf(
					TEXT("central WorldScape collision diverges from noise collision=%.2fcm noise=%.2fcm maximum=%.2fcm"),
					CentralCollisionHeightCm, CentralNoiseHeightCm,
					MaximumLocalCollisionNoiseDeltaCm));
			}
			double LocalMinimumCollisionHeightsCm[3] = {
				CentralCollisionHeightCm, CentralCollisionHeightCm,
				CentralCollisionHeightCm};
			double LocalMaximumCollisionHeightsCm[3] = {
				CentralCollisionHeightCm, CentralCollisionHeightCm,
				CentralCollisionHeightCm};
			double LocalMinimumNoiseHeightsCm[3] = {
				CentralNoiseHeightCm, CentralNoiseHeightCm, CentralNoiseHeightCm};
			double LocalMaximumNoiseHeightsCm[3] = {
				CentralNoiseHeightCm, CentralNoiseHeightCm, CentralNoiseHeightCm};
			int32 LocalCollisionSampleCounts[3] = {0, 0, 0};
			double MaximumLocalNoiseDeltaCm = FMath::Abs(
				CentralCollisionHeightCm - CentralNoiseHeightCm);
			const double LocalTraceHalfSpanCm = FMath::Max(250000.0,
				FMath::Abs(static_cast<double>(Root->NoiseIntensity)) * 2.0
					+ 100000.0);
			FCollisionQueryParams LocalTraceParams(
				SCENE_QUERY_STAT(APSGeneratedLocalWorldScapeRelief), true, GravityPawn);
			for (int32 DirectionIndex = 0;
				DirectionIndex < PhysicalProofDirectionCount; ++DirectionIndex)
			{
				const double Angle = UE_TWO_PI * static_cast<double>(DirectionIndex)
					/ static_cast<double>(PhysicalProofDirectionCount);
				const FVector LocalTangent = LocalTangentU * FMath::Cos(Angle)
					+ LocalTangentV * FMath::Sin(Angle);
				for (int32 DistanceIndex = 0; DistanceIndex < 3; ++DistanceIndex)
				{
					const double DistanceCm = LocalProbeDistancesCm[DistanceIndex];
					const FVector SampleDirection = (ProbeOutward
						+ LocalTangent * (DistanceCm / Root->PlanetScale)).GetSafeNormal();
					const double SampleNoiseHeightCm = Root->GetGroundHeight(
						SurfaceCenter + SampleDirection * Root->PlanetScale, false);
					if (SampleDirection.IsNearlyZero()
						|| !FMath::IsFinite(SampleNoiseHeightCm))
					{
						return Fail(TEXT("local WorldScape collision proof produced a non-finite analytic sample"));
					}

					const FVector LocalTraceStart = SurfaceCenter + SampleDirection
						* (Root->PlanetScale + SampleNoiseHeightCm + LocalTraceHalfSpanCm);
					const FVector LocalTraceEnd = SurfaceCenter + SampleDirection
						* (Root->PlanetScale + SampleNoiseHeightCm - LocalTraceHalfSpanCm);
					FHitResult LocalTerrainHit;
					TArray<FHitResult> LocalWorldHits;
					World->LineTraceMultiByChannel(LocalWorldHits, LocalTraceStart,
						LocalTraceEnd, ECC_Visibility, LocalTraceParams);
					for (const FHitResult& CandidateHit : LocalWorldHits)
					{
						if (CandidateHit.bBlockingHit
							&& CollisionComponents.Contains(CandidateHit.GetComponent()))
						{
							LocalTerrainHit = CandidateHit;
							break;
						}
					}
					if (!LocalTerrainHit.bBlockingHit)
					{
						continue;
					}

					const double CollisionHeightCm = FVector::Distance(
						LocalTerrainHit.ImpactPoint, SurfaceCenter) - Root->PlanetScale;
					if (!FMath::IsFinite(CollisionHeightCm))
					{
						return Fail(TEXT("local WorldScape collision proof produced a non-finite collision height"));
					}
					LocalMinimumCollisionHeightsCm[DistanceIndex] = FMath::Min(
						LocalMinimumCollisionHeightsCm[DistanceIndex], CollisionHeightCm);
					LocalMaximumCollisionHeightsCm[DistanceIndex] = FMath::Max(
						LocalMaximumCollisionHeightsCm[DistanceIndex], CollisionHeightCm);
					LocalMinimumNoiseHeightsCm[DistanceIndex] = FMath::Min(
						LocalMinimumNoiseHeightsCm[DistanceIndex], SampleNoiseHeightCm);
					LocalMaximumNoiseHeightsCm[DistanceIndex] = FMath::Max(
						LocalMaximumNoiseHeightsCm[DistanceIndex], SampleNoiseHeightCm);
					++LocalCollisionSampleCounts[DistanceIndex];
					MaximumLocalNoiseDeltaCm = FMath::Max(MaximumLocalNoiseDeltaCm,
						FMath::Abs(CollisionHeightCm - SampleNoiseHeightCm));
				}
			}

			double LocalCollisionRangesCm[3] = {0.0, 0.0, 0.0};
			double LocalNoiseRangesCm[3] = {0.0, 0.0, 0.0};
			double RequiredLocalRangesCm[3] = {0.0, 0.0, 0.0};
			bool bLocalCollisionProofComplete =
				MaximumLocalNoiseDeltaCm <= MaximumLocalCollisionNoiseDeltaCm;
			for (int32 DistanceIndex = 0; DistanceIndex < 3; ++DistanceIndex)
			{
				LocalCollisionRangesCm[DistanceIndex] =
					LocalMaximumCollisionHeightsCm[DistanceIndex]
						- LocalMinimumCollisionHeightsCm[DistanceIndex];
				LocalNoiseRangesCm[DistanceIndex] =
					LocalMaximumNoiseHeightsCm[DistanceIndex]
						- LocalMinimumNoiseHeightsCm[DistanceIndex];
				RequiredLocalRangesCm[DistanceIndex] = FMath::Max(
					MinimumLocalRangesCm[DistanceIndex],
					LocalNoiseRangesCm[DistanceIndex]
						* MinimumLocalCollisionNoiseFraction);
				bLocalCollisionProofComplete = bLocalCollisionProofComplete
					&& LocalCollisionSampleCounts[DistanceIndex]
						>= MinimumLocalCollisionSamplesPerRing
					&& LocalCollisionRangesCm[DistanceIndex]
						>= RequiredLocalRangesCm[DistanceIndex];
			}
			if (!bLocalCollisionProofComplete)
			{
				if (Now - StepStartSeconds > PhysicalSurfaceTimeoutSeconds)
				{
					return Fail(FString::Printf(
						TEXT("local WorldScape collision is incomplete, flat, or diverges from noise samples=[%d,%d,%d]/%d collisionRanges=[%.2f,%.2f,%.2f]cm noiseRanges=[%.2f,%.2f,%.2f]cm required=[%.2f,%.2f,%.2f]cm maxNoiseDelta=%.2fcm maximum=%.2fcm"),
						LocalCollisionSampleCounts[0], LocalCollisionSampleCounts[1],
						LocalCollisionSampleCounts[2], MinimumLocalCollisionSamplesPerRing,
						LocalCollisionRangesCm[0], LocalCollisionRangesCm[1],
						LocalCollisionRangesCm[2], LocalNoiseRangesCm[0],
						LocalNoiseRangesCm[1], LocalNoiseRangesCm[2],
						RequiredLocalRangesCm[0], RequiredLocalRangesCm[1],
						RequiredLocalRangesCm[2],
						MaximumLocalNoiseDeltaCm, MaximumLocalCollisionNoiseDeltaCm));
				}
				return false;
			}

			const double CollisionReliefVariationCm =
				MaxCollisionHeightCm - MinCollisionHeightCm;
			if (CollisionReliefVariationCm < MinimumCollisionReliefVariationCm)
			{
				return Fail(FString::Printf(
					TEXT("WorldScape collision surface is effectively flat localVariation=%.3fcm vertices=%d minimum=%.3fcm"),
					CollisionReliefVariationCm, CollisionVertexCount,
					MinimumCollisionReliefVariationCm));
			}
			if (Planet->PlanetGravityStrength <= UE_SMALL_NUMBER)
			{
				return Fail(FString::Printf(
					TEXT("generated home planet has no positive gameplay gravity strength=%.6f"),
					Planet->PlanetGravityStrength));
			}

			const double TraceHeightCm = FVector::Distance(
				SurfaceHit.ImpactPoint, SurfaceCenter) - Root->PlanetScale;
			const double RenderCollisionHeightDeltaCm = FMath::Abs(
				PhysicalRenderProof.ClosestObserverHeightCm - TraceHeightCm);
			const double MaximumRenderSampleDistanceCm = FMath::Max(
				static_cast<double>(Root->TriangleSize) * 4.0, 500.0);
			if (FMath::Abs(TraceHeightCm - ExpectedHeightCm) > 200.0
				|| !FMath::IsFinite(RenderCollisionHeightDeltaCm)
				|| RenderCollisionHeightDeltaCm > 200.0
				|| PhysicalRenderProof.ClosestObserverSurfaceDistanceCm
					> MaximumRenderSampleDistanceCm)
			{
				return Fail(FString::Printf(
					TEXT("natural WorldScape render/noise/collision diverged expected=%.2fcm trace=%.2fcm noiseCollisionDelta=%.2fcm renderCollisionDelta=%.2fcm nearestRenderSample=%.2fcm maximumSample=%.2fcm renderRelief=%.2fcm renderNoiseDelta=%.2fcm"),
					ExpectedHeightCm, TraceHeightCm,
					FMath::Abs(TraceHeightCm - ExpectedHeightCm),
					RenderCollisionHeightDeltaCm,
					PhysicalRenderProof.ClosestObserverSurfaceDistanceCm,
					MaximumRenderSampleDistanceCm,
					PhysicalRenderProof.ReliefVariationCm,
					PhysicalRenderProof.MaximumNoiseDeltaCm));
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.PhysicalSurface] PASS source=natural-production-patch renderLod0Vertices=%d renderRelief=%.2fcm renderNoiseDelta=%.2fcm renderCenterOffset=%.2fcm renderClosestObserverVertex=%.2fcm renderCollisionDelta=%.2fcm collisionLods=%d traceActor=%s traceComponent=%s expectedHeight=%.2fcm traceHeight=%.2fcm collisionRelief=%.2fcm collisionVertices=%d localCollisionSamples=[%d,%d,%d] localCollisionRelief=[%.2f,%.2f,%.2f]cm localCollisionNoiseDelta=%.2fcm gravity=%.6f noise=%s"),
				PhysicalRenderProof.VertexCount, PhysicalRenderProof.ReliefVariationCm,
				PhysicalRenderProof.MaximumNoiseDeltaCm,
				PhysicalRenderProof.ObserverCenterOffsetCm,
				PhysicalRenderProof.ClosestObserverSurfaceDistanceCm,
				RenderCollisionHeightDeltaCm,
				Root->CollisionLods.Num(), *GetNameSafe(SurfaceHit.GetActor()),
				*GetNameSafe(SurfaceHit.GetComponent()), ExpectedHeightCm, TraceHeightCm,
				CollisionReliefVariationCm, CollisionVertexCount,
				LocalCollisionSampleCounts[0], LocalCollisionSampleCounts[1],
				LocalCollisionSampleCounts[2], LocalCollisionRangesCm[0],
				LocalCollisionRangesCm[1], LocalCollisionRangesCm[2],
				MaximumLocalNoiseDeltaCm, Planet->PlanetGravityStrength,
				*GetNameSafe(Root->WorldScapeNoise));

			// Put the capsule on the traced collision, not at the five-metre generation
			// clearance used to wake WorldScape. This makes the character a useful scale
			// reference instead of a visibly suspended zero-G subject.
			const double CapsuleHalfHeightCm = IsValid(PawnCapsule)
				? PawnCapsule->GetScaledCapsuleHalfHeight() : 0.0;
			PhysicalSurfaceSpawnLocation = SurfaceHit.ImpactPoint
				+ ProbeOutward * (CapsuleHalfHeightCm + 2.0);

			// Aim the third-person camera along the strongest coherent 100 m / 250 m /
			// 1 km profile. Selecting by absolute delta (rather than the old highest-only
			// sample) also captures downhill relief and produces a readable grazing view.
			const FVector ReferenceAxis = FMath::Abs(ProbeOutward.Z) < 0.82
				? FVector::UpVector : FVector::ForwardVector;
			const FVector TangentU = FVector::CrossProduct(ReferenceAxis, ProbeOutward)
				.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
			const FVector TangentV = FVector::CrossProduct(ProbeOutward, TangentU)
				.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::RightVector);
			constexpr double ViewDistancesCm[3] = {10000.0, 25000.0, 100000.0};
			FVector BestForward = PhysicalSurfaceProofForward.IsNearlyZero()
				? TangentU : PhysicalSurfaceProofForward;
			double BestViewScore = -TNumericLimits<double>::Max();
			double BestViewDeltasCm[3] = {0.0, 0.0, 0.0};
			double BestViewMaxSlope = 0.0;
			for (int32 DirectionIndex = 0;
				DirectionIndex < PhysicalProofDirectionCount; ++DirectionIndex)
			{
				const double Angle = UE_TWO_PI * static_cast<double>(DirectionIndex)
					/ static_cast<double>(PhysicalProofDirectionCount);
				const FVector Tangent = TangentU * FMath::Cos(Angle)
					+ TangentV * FMath::Sin(Angle);
				double DirectionDeltasCm[3] = {0.0, 0.0, 0.0};
				double DirectionScore = 0.0;
				double DirectionMaxSlope = 0.0;
				for (int32 DistanceIndex = 0; DistanceIndex < 3; ++DistanceIndex)
				{
					const double DistanceCm = ViewDistancesCm[DistanceIndex];
					const FVector SampleDirection = (ProbeOutward
						+ Tangent * (DistanceCm / Root->PlanetScale)).GetSafeNormal();
					const double SampleHeightCm = Root->GetGroundHeight(
						SurfaceCenter + SampleDirection * Root->PlanetScale, false);
					if (!FMath::IsFinite(SampleHeightCm))
					{
						DirectionScore = -TNumericLimits<double>::Max();
						break;
					}
					DirectionDeltasCm[DistanceIndex] = SampleHeightCm - ExpectedHeightCm;
					DirectionMaxSlope = FMath::Max(DirectionMaxSlope,
						FMath::Abs(DirectionDeltasCm[DistanceIndex]) / DistanceCm);
					const double TargetRangeCm = DistanceIndex == 0
						? MinimumProofRange100mCm : (DistanceIndex == 1
							? MinimumProofRange250mCm : MinimumProofRange1kmCm);
					DirectionScore += FMath::Abs(DirectionDeltasCm[DistanceIndex])
						/ (TargetRangeCm * 0.5);
				}
				if (DirectionMaxSlope <= MaximumProofSlope && DirectionScore > BestViewScore)
				{
					BestViewScore = DirectionScore;
					BestForward = Tangent;
					BestViewDeltasCm[0] = DirectionDeltasCm[0];
					BestViewDeltasCm[1] = DirectionDeltasCm[1];
					BestViewDeltasCm[2] = DirectionDeltasCm[2];
					BestViewMaxSlope = DirectionMaxSlope;
				}
			}
			// The landing patch already proves its full ring range at 100 m, 250 m and
			// 1 km. A single tangent is allowed to crest and descend before 1 km; forcing
			// one monotonic direction at all three radii rejected real rolling terrain.
			// For the screenshot, require the near field that is actually visible behind
			// the pawn, while retaining the measured 1 km delta in diagnostics.
			if (FMath::Abs(BestViewDeltasCm[0]) < MinimumProofRange100mCm * 0.5
				|| FMath::Abs(BestViewDeltasCm[1]) < MinimumProofRange250mCm * 0.5)
			{
				return Fail(FString::Printf(
					TEXT("selected Frozen proof direction has a visually flat near field delta100m=%.2fcm delta250m=%.2fcm delta1km=%.2fcm maxSlope=%.5f"),
					BestViewDeltasCm[0], BestViewDeltasCm[1], BestViewDeltasCm[2],
					BestViewMaxSlope));
			}
			PhysicalSurfaceViewRotation = FRotationMatrix::MakeFromXZ(
				BestForward, ProbeOutward).Rotator();
			GravityPawn->SetActorRotation(
				PhysicalSurfaceViewRotation, ETeleportType::TeleportPhysics);
			if (USpringArmComponent* CameraSpringArm = FindPawnSpringArm(GravityPawn))
			{
				CameraSpringArm->bUsePawnControlRotation = false;
				CameraSpringArm->TargetArmLength = 500.0f;
				CameraSpringArm->SetRelativeLocation(FVector(0.0, 0.0, 55.0));
				CameraSpringArm->SetRelativeRotation(FRotator(-8.0, 0.0, 0.0));
			}
			if (UCameraComponent* PawnCamera = FindPawnCamera(GravityPawn))
			{
				PawnCamera->SetFieldOfView(72.0f);
			}
			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				TEXT("Screenshots/Windows/APS_GeneratedCivilization_PhysicalSurface.png"));
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			ScreenshotSettleFramesRemaining = 16;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.PhysicalSurface] settling grounded proof screenshot=%s viewDelta=[%.2f,%.2f,%.2f]cm maxSlope=%.5f grounded=%s"),
				*ScreenshotPath, BestViewDeltasCm[0], BestViewDeltasCm[1],
				BestViewDeltasCm[2], BestViewMaxSlope,
				*PhysicalSurfaceSpawnLocation.ToCompactString());
			Step = EStep::WaitForPhysicalSurfaceScreenshot;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForPhysicalSurfaceScreenshot(UWorld* World, double Now)
		{
			APawn* GravityPawn = RuntimeGravityPawn.Get();
			if (!World || World != GameplayWorld.Get() || !IsValid(GravityPawn))
			{
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("physical-surface proof camera became unavailable"));
				}
				return false;
			}

			UCapsuleComponent* PawnCapsule = FindPawnCapsule(GravityPawn);
			if (IsValid(PawnCapsule))
			{
				PawnCapsule->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
			GravityPawn->SetActorLocationAndRotation(
				PhysicalSurfaceSpawnLocation, PhysicalSurfaceViewRotation, false,
				nullptr, ETeleportType::TeleportPhysics);
			if (ScreenshotSettleFramesRemaining > 0)
			{
				--ScreenshotSettleFramesRemaining;
				return false;
			}

			const UDirectionalLightComponent* ActiveSurfaceFillComponent = nullptr;
			for (TActorIterator<ADirectionalLight> It(World); It; ++It)
			{
				const ADirectionalLight* Candidate = *It;
				const UDirectionalLightComponent* CandidateComponent = Candidate
					? Cast<UDirectionalLightComponent>(Candidate->GetLightComponent()) : nullptr;
				if (IsValid(Candidate)
					&& Candidate->ActorHasTag(TEXT("APSGameplaySurfaceFillLight"))
					&& IsValid(CandidateComponent)
					&& CandidateComponent->IsVisible())
				{
					ActiveSurfaceFillComponent = CandidateComponent;
					break;
				}
			}
			if (!ActiveSurfaceFillComponent)
			{
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("physical WorldScape surface readability fill did not activate"));
				}
				return false;
			}
			const FLightingChannels& FillChannels = ActiveSurfaceFillComponent->LightingChannels;
			const APlanet* ActiveSurfaceBody = RuntimeHomePlanet.Get();
			const FVector SurfaceOutward = IsValid(ActiveSurfaceBody)
				? (GravityPawn->GetActorLocation() - ActiveSurfaceBody->GetActorLocation())
					.GetSafeNormal()
				: FVector::ZeroVector;
			const double FillIncidence = SurfaceOutward.IsNearlyZero()
				? -1.0
				: -FVector::DotProduct(
					ActiveSurfaceFillComponent->GetDirection(), SurfaceOutward);
			if (ActiveSurfaceFillComponent->Intensity < MinimumGameplaySurfaceFillIntensity
				|| ActiveSurfaceFillComponent->Intensity > MaximumGameplaySurfaceFillIntensity
				|| FillIncidence < MinimumGameplaySurfaceFillIncidence
				|| FillIncidence > MaximumGameplaySurfaceFillIncidence
				|| ActiveSurfaceFillComponent->CastShadows
				|| ActiveSurfaceFillComponent->IsUsedAsAtmosphereSunLight()
				|| !FMath::IsNearlyZero(
					ActiveSurfaceFillComponent->VolumetricScatteringIntensity, 0.001f)
				|| !FillChannels.bChannel0 || FillChannels.bChannel1 || FillChannels.bChannel2)
			{
				return Fail(FString::Printf(
					TEXT("physical WorldScape readability fill violated its bounded non-solar contract intensity=%.2f expected=[%.2f,%.2f] incidence=%.4f expected=[%.2f,%.2f] shadows=%d atmosphereSun=%d volumetric=%.3f channels=[%d,%d,%d]"),
					ActiveSurfaceFillComponent->Intensity,
					MinimumGameplaySurfaceFillIntensity,
					MaximumGameplaySurfaceFillIntensity,
					FillIncidence,
					MinimumGameplaySurfaceFillIncidence,
					MaximumGameplaySurfaceFillIncidence,
					ActiveSurfaceFillComponent->CastShadows ? 1 : 0,
					ActiveSurfaceFillComponent->IsUsedAsAtmosphereSunLight() ? 1 : 0,
					ActiveSurfaceFillComponent->VolumetricScatteringIntensity,
					FillChannels.bChannel0 ? 1 : 0,
					FillChannels.bChannel1 ? 1 : 0,
					FillChannels.bChannel2 ? 1 : 0));
			}

			FString CaptureFailure;
			if (!CaptureGameplayViewport(World, CaptureFailure, false, true))
			{
				if (!CaptureFailure.IsEmpty())
				{
					return Fail(CaptureFailure);
				}
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("physical-surface viewport pixels were unavailable for ten seconds"));
				}
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.PhysicalSurface] ground proof screenshot written %s"),
				*ScreenshotPath);
			Step = EStep::Cleanup;
			StepStartSeconds = Now;
			return false;
		}

		void BeginGameplayWorldScapeCleanup(UWorld* World)
		{
			if (GeneratedSaveSlotName.IsEmpty())
			{
				if (const AGravityPlayerController* GravityController =
					Cast<AGravityPlayerController>(World ? World->GetFirstPlayerController() : nullptr))
				{
					GeneratedSaveSlotName = GravityController->CurrentSaveSlotName;
				}
			}
			if (!GeneratedSaveSlotName.IsEmpty())
			{
				const bool bDeletedSave = UGameplayStatics::DeleteGameInSlot(GeneratedSaveSlotName, 0);
				const FString SidecarPath = FPaths::Combine(FPaths::ProjectSavedDir(),
					TEXT("SaveGames"), GeneratedSaveSlotName + TEXT(".apsmeta"));
				const bool bDeletedSidecar = !IFileManager::Get().FileExists(*SidecarPath)
					|| IFileManager::Get().Delete(*SidecarPath, false, true);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.Smoke] Removed generated smoke save slot=%s save=%s sidecar=%s"),
					*GeneratedSaveSlotName, bDeletedSave ? TEXT("deleted") : TEXT("missing"),
					bDeletedSidecar ? TEXT("deleted/missing") : TEXT("FAILED"));
			}
			for (APlanetaryBody* Body : FindActors<APlanetaryBody>(World))
			{
				Body->bStreamWorldScapeSurface = false;
				Body->bGenerateByDefault = false;
			}
			for (APlanetarySurfaceGenerator* Surface :
				FindActors<APlanetarySurfaceGenerator>(World))
			{
				Surface->UnloadWorldScapeRoot();
			}
			for (AWorldScapeRoot* Root : FindActors<AWorldScapeRoot>(World))
			{
				Root->bFreezeGeneration = true;
				Root->SetActorTickEnabled(false);
				Root->SetActorHiddenInGame(true);
				Root->SetActorEnableCollision(false);
			}
		}

		bool AreGameplayWorldScapeWorkersDrained(UWorld* World)
		{
			bool bDrained = true;
			for (AWorldScapeRoot* Root : FindActors<AWorldScapeRoot>(World))
			{
				Root->bFreezeGeneration = true;
				Root->SetActorTickEnabled(false);
				if (Root->WorldScapeLodInGeneration.Num() > 0)
				{
					Root->CheckForLodGeneration();
				}
				if (Root->WorldScapeLodInGeneration.Num() > 0)
				{
					bDrained = false;
				}
				else
				{
					Root->bGenerateWorldScape = false;
				}
			}
			for (APlanetarySurfaceGenerator* Surface :
				FindActors<APlanetarySurfaceGenerator>(World))
			{
				if (Surface->bPendingWorldScapeUnload)
				{
					bDrained = false;
				}
			}
			return bDrained;
		}

		bool UpdateCleanup(UWorld* World, double Now)
		{
			if (!bCleanupStarted)
			{
				bCleanupStarted = true;
				RestoreProjectionEvidenceCamera();
				if (World && World == GameplayWorld.Get())
				{
					BeginGameplayWorldScapeCleanup(World);
				}
			}

			bool bPreviewDrained = true;
			if (PreviewGenerator.IsValid())
			{
				bPreviewDrained = PreviewGenerator->PreparePreviewForTravel();
			}
			bool bGameplayDrained = true;
			if (World && World == GameplayWorld.Get())
			{
				bGameplayDrained = AreGameplayWorldScapeWorkersDrained(World);
			}
			if ((!bPreviewDrained || !bGameplayDrained)
				&& Now - StepStartSeconds <= CleanupTimeoutSeconds)
			{
				return false;
			}
			if (!bPreviewDrained || !bGameplayDrained)
			{
				if (PendingFailure.IsEmpty())
				{
					PendingFailure = TEXT("WorldScape workers did not drain within 20 seconds");
				}
			}

			if (!PendingFailure.IsEmpty())
			{
				Test->AddError(PendingFailure);
			}
			else
			{
				if (bValidateWetOceanContract)
				{
					UE_LOG(LogTemp, Display,
						TEXT("[APS.Handoff.WetOcean] PASS menu preview -> immutable Water handoff -> one authoritative WorldScape root -> exact 9x3 project-owned non-displacing SingleLayerWater ocean material slots -> collisionless/IgnoreAll liquid -> Visibility/Pawn traces reach terrain -> one ground scattering shell without coplanar cap passes plus preserved orbital shell -> two re-centred rendered observer positions -> two distinct screenshots -> hidden non-colliding preview ocean proxies -> safe worker drain"));
				}
				else
				{
					UE_LOG(LogTemp, Display,
						TEXT("[APS.Handoff.Smoke] PASS menu preview -> immutable handoff -> GravityGameMode -> exact hierarchy -> selected pawn -> starter attachments -> resolver WorldScape -> screenshot -> manual station/surface observer tracking -> physical terrain collision/relief -> safe worker drain"));
				}
			}
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		EPlanetType ExpectedPlanetType{EPlanetType::Frozen};
		bool bValidateWetOceanContract{false};
		EStep Step{EStep::OpenGenerator};
		double TestStartSeconds{0.0};
		double StepStartSeconds{0.0};
		TWeakObjectPtr<UWorld> MenuWorld;
		TWeakObjectPtr<UWorld> GameplayWorld;
		TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
		TWeakObjectPtr<AAstroGenerator> RuntimeGenerator;
		TWeakObjectPtr<APlanet> RuntimeHomePlanet;
		TWeakObjectPtr<APawn> RuntimeGravityPawn;
		TWeakObjectPtr<ASpaceStation> RuntimeStation;
		TWeakObjectPtr<AAstroGenerator> ProjectionEvidenceGenerator;
		TWeakObjectPtr<AGalaxy> ProjectionEvidenceGalaxy;
		TWeakObjectPtr<AStarCluster> ProjectionEvidenceCluster;
		TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>
			ProjectionEvidenceGalaxyComponent;
		TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>
			ProjectionEvidenceClusterComponent;
		TWeakObjectPtr<ACameraActor> ProjectionEvidenceCamera;
		TWeakObjectPtr<AActor> ProjectionEvidenceOriginalViewTarget;
		FAPSCanonicalStellarProjectionDescriptor ProjectionEvidenceBaselineDescriptor;
		FAPSCanonicalStellarProxyRecord ProjectionEvidenceGalaxyBaselineRecord;
		FAPSCanonicalStellarProxyRecord ProjectionEvidenceClusterBaselineRecord;
		FAPSCanonicalStellarProxyRecord ProjectionEvidenceHomeBaselineRecord;
		FAPSCanonicalClusterSystemAddress ProjectionEvidenceHomeBaselineAddress;
		FAPSCanonicalClusterSystemAddress ProjectionEvidenceClusterBaselineAddress;
		FProjectionEvidenceRenderedSample ProjectionEvidencePreviousSample;
		FQuat ProjectionEvidenceBaselineRenderedViewRotation{FQuat::Identity};
		FQuat ProjectionEvidenceGalaxyBaselineLocalRotation{FQuat::Identity};
		FQuat ProjectionEvidenceClusterBaselineLocalRotation{FQuat::Identity};
		FVector ProjectionEvidenceGalaxyBaselineLocalScale{FVector::OneVector};
		FVector ProjectionEvidenceClusterBaselineLocalScale{FVector::OneVector};
		TArray<FGuid> ProjectionEvidenceDatasetStableIds;
		TArray<FVector> ProjectionEvidenceDatasetCanonicalPositions;
		TArray<double> ProjectionEvidenceDatasetCanonicalRadii;
		TArray<double> ProjectionEvidenceDatasetMinOrbits;
		TArray<double> ProjectionEvidenceDatasetMaxOrbits;
		FGuid ProjectionEvidenceHomeStableId;
		FGuid ProjectionEvidenceGalaxyStableId;
		FGuid ProjectionEvidenceClusterStableId;
		FVector ProjectionEvidenceExpectedCameraLocation{FVector::ZeroVector};
		FVector ProjectionEvidenceMoveDelta{FVector::ZeroVector};
		double ProjectionEvidenceLastAcceptedRenderTime{0.0};
		uint64 ProjectionEvidenceLastAcceptedFrameCounter{0u};
		int32 ProjectionEvidenceWarmupFramesRemaining{0};
		int32 ProjectionEvidenceAcceptedFrameCount{0};
		bool bProjectionEvidencePhaseStarted{false};
		bool bProjectionEvidenceBaselineCaptured{false};
		TWeakObjectPtr<UClass> SelectedPawnClass;
		const UGeneratedWorld* EditableGeneratedWorldAddress{nullptr};
		const USpawnParameters* EditableSpawnParametersAddress{nullptr};
		FString SelectedPawnClassPath;
		FString GeneratedSaveSlotName;
		FString ScreenshotPath;
		FString PendingFailure;
		FTransform WetOceanCaptureTransforms[WetOceanRenderViewCount]{
			FTransform::Identity, FTransform::Identity};
		double WetOceanCaptureWaterDepthsCm[WetOceanRenderViewCount]{0.0, 0.0};
		double WetOceanCaptureClearancesCm[WetOceanRenderViewCount]{0.0, 0.0};
		FVector WetOceanCapturedCameraLocations[WetOceanRenderViewCount]{
			FVector::ZeroVector, FVector::ZeroVector};
		int32 WetOceanCaptureIndex{0};
		int32 WetOceanStableFramesRemaining{0};
		uint32 WetOceanFirstFrameCrc{0};
		bool bWetOceanCaptureContractReady{false};
		uint32 PreviewProfileSignature{0};
		bool bHasPreviewProjectionSnapshot{false};
		uint32 PreviewProjectionVersion{0};
		uint32 PreviewProjectionContextHash{0};
		uint32 PreviewCanonicalDatasetHash{0};
		uint32 PreviewCanonicalDatasetVersion{0};
		uint32 PreviewCanonicalDatasetInputHash{0};
		uint64 PreviewCanonicalDatasetBuildSerial{0};
		int32 PreviewCanonicalDatasetRecordCount{0};
		bool bHasPreviewDatasetOnlyRecord{false};
		FGuid PreviewDatasetOnlyStableId;
		FVector PreviewDatasetOnlyCanonicalPosition{FVector::ZeroVector};
		double PreviewDatasetOnlyCanonicalRadiusSolar{0.0};
		double PreviewDatasetOnlyMinOrbit{0.0};
		double PreviewDatasetOnlyMaxOrbit{0.0};
		FGuid PreviewHomeStableId;
		FVector PreviewHomeCanonicalPosition{FVector::ZeroVector};
		FVector PreviewHomeExpectedProxyPosition{FVector::ZeroVector};
		double PreviewHomeCanonicalRadiusSolar{0.0};
		FGuid PreviewGalaxyStableId;
		FVector PreviewGalaxyCanonicalPosition{FVector::ZeroVector};
		FVector PreviewGalaxyExpectedProxyPosition{FVector::ZeroVector};
		double PreviewGalaxyCanonicalRadiusSolar{0.0};
		FGuid PreviewClusterStableId;
		FVector PreviewClusterCanonicalPosition{FVector::ZeroVector};
		FVector PreviewClusterExpectedProxyPosition{FVector::ZeroVector};
		double PreviewClusterCanonicalRadiusSolar{0.0};
		FVector PhysicalSurfaceSpawnLocation{FVector::ZeroVector};
		FVector PhysicalSurfaceProbeOutward{FVector::ZeroVector};
		FVector PhysicalSurfaceProofForward{FVector::ZeroVector};
		FRotator PhysicalSurfaceViewRotation{FRotator::ZeroRotator};
		int32 ScreenshotSettleFramesRemaining{0};
		bool bPhysicalSurfaceProbeInitialized{false};
		bool bNaturalSurfaceLandingValidated{false};
		bool bManualApproachObserverValidated{false};
		int32 NaturalSurfaceSettleFrames{0};
		bool bCleanupStarted{false};
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGeneratedCivilizationHandoffSmokeTest,
	"APS.Rendered.Gameplay.GeneratedCivilizationHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGeneratedCivilizationHandoffSmokeTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("[APS.Handoff.Smoke] Could not open the current MainMenu map"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSGeneratedGameplayHandoffSmokeTests::FGeneratedCivilizationHandoffCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGeneratedCivilizationWetOceanHandoffSmokeTest,
	"APS.Rendered.Gameplay.GeneratedCivilizationWetOceanHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGeneratedCivilizationWetOceanHandoffSmokeTest::RunTest(
	const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("[APS.Handoff.WetOcean] Could not open the current MainMenu map"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSGeneratedGameplayHandoffSmokeTests::FGeneratedCivilizationHandoffCommand(
			this, EPlanetType::Water, true));
	return true;
}

#endif

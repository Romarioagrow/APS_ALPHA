#include "APSStellarVisualSubsystem.h"
#include "APSGameplayStarAppearance.h"
#include "APSPlanetSurfaceFill.h"
#include "APS_ALPHA/Core/World/APSPlaceholderGlobe.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"

#include "APS_ALPHA/Actors/Astro/APSBlackHoleVisual.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSStellarVisuals, Log, All);

namespace
{
	const FName PreviewFillLightTag(TEXT("APSPreviewFillLight"));
	const FName RuntimeStellarKeyLightTag(TEXT("APSRuntimeStellarKeyLight"));
	// PLANET uses a fixed-exposure camera and a camera-facing fill in addition to
	// the generated-star key. Keep their combined energy near one ordinary scene
	// directional light so bright terrain palettes retain colour and relief.
	constexpr float PreviewFillLightIntensity = 0.65f;
	constexpr float PreviewKeyLightIntensityScale = 0.25f;
	const FLinearColor PreviewFillLightColor(0.72f, 0.82f, 1.0f, 1.0f);
	const FName GameplayStationFillLightTag(TEXT("APSGameplayStationFillLight"));
	constexpr float GameplayStationFillLightIntensity = 28.0f;
	constexpr float GameplayStationFillMinimumAttenuationRadiusCm = 4500.0f;
	constexpr float GameplayStationFillMaximumAttenuationRadiusCm = 30000.0f;
	const FLinearColor GameplayStationFillLightColor(0.72f, 0.82f, 1.0f, 1.0f);
	const FName GameplaySurfaceFillLightTag(TEXT("APSGameplaySurfaceFillLight"));
	/** UAPSObjectLightingSubsystem's camera-aligned fill (APSObjectLighting::ObjectFillTag): never the star's key light. */
	const FName ObjectFillLightTag(TEXT("APSObjectFillLight"));
	// Keep the generated star as the dominant key. This fill only lifts the
	// fixed-exposure floor enough to retain readable normals on the night side.
	// The dry Frozen handoff still compressed the settled ground to a nine-level
	// p10/p90 range at 1.40 lux. A calibrated lift keeps the off-axis normal
	// response above display quantization while remaining below one quarter of the
	// generated star key (about 9.5 lux in the standard handoff).
	constexpr float GameplaySurfaceFillLightIntensity = 2.20f;
	const FLinearColor GameplaySurfaceFillLightColor(0.78f, 0.84f, 0.94f, 1.0f);
}

bool UAPSStellarVisualSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UAPSStellarVisualSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UAPSWorldOriginSubsystem>();
	APSWorldShiftEvents::BindPostShift(this, [this](UWorld* ShiftedWorld)
	{
		if (!ShiftedWorld || ShiftedWorld != GetWorld() || !bHasTargetStar) return;
		if (const auto* Origin = ShiftedWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			// Both barriers run after actors and the origin reach their final frame.
			// Resolve from the canonical snapshot, so duplicate notices cannot add
			// the shift twice. Selection and the existing light interpolation stay unchanged.
			TargetStarLocation = Origin->FromGenerationFrame(TargetStarGenerationLocation) + Origin->GetSkyOffset();
		}
	});
	// Rio 06.10 (still ship): the key light's star follows the sky every owed step and comes back with it on a pay (the
	// same snapshot: its true place, shown at the sky offset).
	UAPSWorldOriginSubsystem::OnSkyOffsetChanged().AddWeakLambda(this, [this](UWorld* ShiftedWorld, const FVector&)
	{
		if (!ShiftedWorld || ShiftedWorld != GetWorld() || !bHasTargetStar) return;
		if (const auto* Origin = ShiftedWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
		{
			TargetStarLocation = Origin->FromGenerationFrame(TargetStarGenerationLocation) + Origin->GetSkyOffset();
		}
	});
}

void UAPSStellarVisualSubsystem::Tick(float DeltaTime)
{
	UpdateGameplayStellarView();
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Observer = PlayerController ? PlayerController->GetPawn() : nullptr;
	FVector ObserverLocation = Observer ? Observer->GetActorLocation() : FVector::ZeroVector;
	bool bHasObserverLocation = IsValid(Observer);
	const APlanetaryBody* ActivePreviewBody = nullptr;

	// The menu camera is intentionally detached from the generated hierarchy.
	// Light the selected planet from its actual parent star instead of from the
	// menu pawn/camera, otherwise rotating the preview produces physically
	// inconsistent illumination. Gameplay worlds keep the existing pawn path.
	if (World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			AAstroGenerator* Generator = *It;
			if (!IsValid(Generator) || !Generator->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				continue;
			}
			if (const APlanetaryBody* PreviewBody = Generator->GetActivePreviewWorldScapeBody())
			{
				ActivePreviewBody = PreviewBody;
				ObserverLocation = PreviewBody->GetActorLocation();
				bHasObserverLocation = true;
			}
			break;
		}
	}

	FVector PreviewCameraLocation = FVector::ZeroVector;
	FRotator PreviewCameraRotation = FRotator::ZeroRotator;
	const bool bHasPreviewCameraLocation = IsValid(PlayerController);
	if (bHasPreviewCameraLocation)
	{
		PlayerController->GetPlayerViewPoint(PreviewCameraLocation, PreviewCameraRotation);
	}
	UpdatePreviewFillLight(
		ActivePreviewBody,
		PreviewCameraLocation,
		bHasPreviewCameraLocation,
		DeltaTime);
	UpdateGameplayStationFillLight(
		ActivePreviewBody ? nullptr : Observer,
		PreviewCameraLocation,
		bHasPreviewCameraLocation);

	if (!bHasObserverLocation)
	{
		UpdateGameplaySurfaceFillLight(nullptr, FVector::ZeroVector, false, DeltaTime);
		return;
	}

	if (!DirectionalLight.IsValid())
	{
		ResolveDirectionalLight();
	}
	SearchElapsed += DeltaTime;
	if (SearchElapsed >= 0.5f || !bHasTargetStar)
	{
		SearchElapsed = 0.0f;
		APSGameplayStarAppearance::Apply(World);
		ResolveNearestStar(ObserverLocation);
	}
	// Every frame: the stars' daylight fade follows a climb continuously (twice a second it stepped visibly, 30.09).
	if (!ActivePreviewBody && bHasPreviewCameraLocation)
	{
		UpdateGameplayDaylightStars(PreviewCameraLocation);
	}
	UpdateGameplaySurfaceFillLight(
		ActivePreviewBody ? nullptr : Observer,
		ObserverLocation,
		bHasObserverLocation,
		DeltaTime);

	ADirectionalLight* Light = DirectionalLight.Get();
	UDirectionalLightComponent* LightComponent = Light
		? Cast<UDirectionalLightComponent>(Light->GetLightComponent()) : nullptr;
	if (!LightComponent || !bHasTargetStar)
	{
		return;
	}

	const FVector LightRayDirection = (ObserverLocation - TargetStarLocation).GetSafeNormal();
	const AAstroGenerator* FullScaleGenerator = GameplayStellarGenerator.Get();
	// A committed generated world must not initially display the map's unrelated
	// sun and then fade/rotate to its own star over several seconds. The existing
	// gameplay adapter identifies the accepted full-scale hierarchy; additionally
	// exclude authored SinglePlay and every preview path. A weak per-generator
	// latch leaves later star changes, movement and rebases on the original smoothing.
	const bool bInitializeGeneratedLight = IsValid(FullScaleGenerator)
		&& !ActivePreviewBody
		&& FullScaleGenerator->bGenerateFullScaledWorld
		&& !FullScaleGenerator->bIntegrateStartPlanet
		&& !FullScaleGenerator->ActorHasTag(TEXT("WorldGenerationPreview"))
		&& !FullScaleGenerator->UsesContinuousPreviewFrame()
		&& FullScaleGenerator->GetCanonicalStellarProjectionDescriptor().bConsumedFinalizedDataset
		&& InitializedGameplayLightGenerator.Get() != FullScaleGenerator
		&& !LightRayDirection.IsNearlyZero();
	const float PreviousKeyIntensity = LightComponent->Intensity;
	const FRotator PreviousKeyRotation = Light->GetActorRotation();
	if (!LightRayDirection.IsNearlyZero())
	{
		const FRotator DesiredRotation = LightRayDirection.Rotation();
		const FRotator CurrentRotation = Light->GetActorRotation();
		FRotator UpdatedRotation = bInitializeGeneratedLight ? DesiredRotation
			: FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, 1.6f);
		if (UpdatedRotation.Equals(DesiredRotation, 0.05f))
		{
			UpdatedRotation = DesiredRotation;
		}
		// A movable light invalidates virtual shadow maps whenever a setter dirties
		// its render state. Stop writing once interpolation has converged.
		if (!CurrentRotation.Equals(UpdatedRotation, 0.001f))
		{
			Light->SetActorRotation(UpdatedRotation);
		}
	}
	FLinearColor UpdatedLightColor = bInitializeGeneratedLight ? TargetLightColor
		: FMath::CInterpTo(SmoothedLightColor, TargetLightColor, DeltaTime, 1.3f);
	if (UpdatedLightColor.Equals(TargetLightColor, 0.002f))
	{
		UpdatedLightColor = TargetLightColor;
	}
	SmoothedLightColor = UpdatedLightColor;
	const float EffectiveTargetLightIntensity = ActivePreviewBody
		? TargetLightIntensity * PreviewKeyLightIntensityScale
		: TargetLightIntensity;
	float UpdatedLightIntensity = bInitializeGeneratedLight ? EffectiveTargetLightIntensity
		: FMath::FInterpTo(SmoothedLightIntensity, EffectiveTargetLightIntensity, DeltaTime, 1.3f);
	if (FMath::IsNearlyEqual(
		UpdatedLightIntensity, EffectiveTargetLightIntensity, 0.002f))
	{
		UpdatedLightIntensity = EffectiveTargetLightIntensity;
	}
	SmoothedLightIntensity = UpdatedLightIntensity;
	if (!LightComponent->GetLightColor().Equals(SmoothedLightColor, 0.0005f))
	{
		LightComponent->SetLightColor(SmoothedLightColor);
	}
	if (!FMath::IsNearlyEqual(LightComponent->Intensity, SmoothedLightIntensity, 0.0005f))
	{
		LightComponent->SetIntensity(SmoothedLightIntensity);
	}
	if (!bCapturedOriginalTemperature)
	{
		bCapturedOriginalTemperature = true;
		bOriginalUseTemperature = LightComponent->bUseTemperature;
		OriginalTemperature = LightComponent->Temperature;
		SmoothedLightTemperature = TargetLightTemperature;
	}
	SmoothedLightTemperature = bInitializeGeneratedLight ? TargetLightTemperature
		: FMath::FInterpTo(SmoothedLightTemperature, TargetLightTemperature, DeltaTime, 1.3f);
	if (!LightComponent->bUseTemperature)
	{
		LightComponent->SetUseTemperature(true);
	}
	if (!FMath::IsNearlyEqual(LightComponent->Temperature, SmoothedLightTemperature, 1.0f))
	{
		LightComponent->SetTemperature(SmoothedLightTemperature);
	}
	if (bInitializeGeneratedLight)
	{
		InitializedGameplayLightGenerator = GameplayStellarGenerator;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("[APS.StellarLighting.Bootstrap] generator=%s star=%s key=%s oldIntensity=%.3f intensity=%.3f oldRotation=%s rotation=%s ray=%s"),
			*GetNameSafe(FullScaleGenerator), *ActiveStarIdentity, *GetNameSafe(Light),
			PreviousKeyIntensity, LightComponent->Intensity,
			*PreviousKeyRotation.ToCompactString(), *Light->GetActorRotation().ToCompactString(),
			*LightRayDirection.ToCompactString());
	}
}

TStatId UAPSStellarVisualSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSStellarVisualSubsystem, STATGROUP_Tickables);
}

bool UAPSStellarVisualSubsystem::GetActiveStellarTarget(
	FVector& OutTargetLocation, FString& OutTargetIdentity) const
{
	if (!bHasTargetStar)
	{
		return false;
	}
	OutTargetLocation = TargetStarLocation;
	OutTargetIdentity = ActiveStarIdentity;
	return true;
}

void UAPSStellarVisualSubsystem::Deinitialize()
{
	FCoreDelegates::PostWorldOriginOffset.RemoveAll(this);
	APSWorldShiftEvents::OnPostDoubleShift().RemoveAll(this);
	// Rio 06.10 (audit: hygiene): the sky-offset listener bound in Initialize (a weak lambda) goes with the subsystem.
	UAPSWorldOriginSubsystem::OnSkyOffsetChanged().RemoveAll(this);
	ResetGameplayStellarView();
	if (ADirectionalLight* FillLight = PreviewFillLight.Get())
	{
		FillLight->Destroy();
	}
	PreviewFillLight.Reset();
	if (APointLight* FillLight = GameplayStationFillLight.Get())
	{
		FillLight->Destroy();
	}
	GameplayStationFillLight.Reset();
	GameplayFillStation.Reset();
	if (ADirectionalLight* FillLight = GameplaySurfaceFillLight.Get())
	{
		FillLight->Destroy();
	}
	GameplaySurfaceFillLight.Reset();
	GameplayFillBody.Reset();

	if (ADirectionalLight* Light = DirectionalLight.Get(); bOwnsDirectionalLight && Light)
	{
		Light->Destroy();
	}
	else if (bCapturedOriginalLight && Light)
	{
		if (UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
		{
			Component->SetLightColor(OriginalLightColor);
			Component->SetIntensity(OriginalLightIntensity);
			Component->SetForwardShadingPriority(OriginalForwardShadingPriority);
			Component->SetMobility(OriginalMobility);
		}
		Light->SetActorRotation(OriginalLightRotation);
	}
	Super::Deinitialize();
}

void UAPSStellarVisualSubsystem::ResolveDirectionalLight()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		ADirectionalLight* Candidate = *It;
		// Rio 04.10 ("the light in space lags, darker, then black"): the object fill is a directional light too; taken
		// for the star's key, the two subsystems would turn and dim the same light against each other.
		if (IsValid(Candidate)
			&& (Candidate->ActorHasTag(PreviewFillLightTag)
				|| Candidate->ActorHasTag(GameplaySurfaceFillLightTag)
				|| Candidate->ActorHasTag(ObjectFillLightTag)))
		{
			continue;
		}
		UDirectionalLightComponent* Component = IsValid(Candidate)
			? Cast<UDirectionalLightComponent>(Candidate->GetLightComponent()) : nullptr;
		if (!Component)
		{
			continue;
		}
		DirectionalLight = Candidate;
		OriginalLightRotation = Candidate->GetActorRotation();
		OriginalLightColor = Component->GetLightColor();
		OriginalLightIntensity = Component->Intensity;
		OriginalForwardShadingPriority = Component->ForwardShadingPriority;
		OriginalMobility = Component->Mobility;
		SmoothedLightColor = OriginalLightColor;
		SmoothedLightIntensity = OriginalLightIntensity;
		Component->SetMobility(EComponentMobility::Movable);
		// Keep the authored star above ordinary scene directionals. PLANET's
		// camera-facing fill receives priority 2 only while it is visible so
		// SingleLayerWater has one deterministic forward light.
		Component->SetForwardShadingPriority(1);
		bCapturedOriginalLight = true;
		UE_LOG(LogAPSStellarVisuals, Log, TEXT("Using existing directional light %s"), *Candidate->GetName());
		break;
	}

	if (DirectionalLight.IsValid())
	{
		return;
	}

	// Generated gameplay levels are allowed to contain no authored sun. In that
	// case the stellar subsystem still knows the generated parent-star direction,
	// but previously had no light to drive, leaving physically displaced terrain
	// visually indistinguishable from a flat dark texture. Create one transient key
	// (not a proxy surface and not a camera fill) and let the normal nearest-star
	// path below orient and colour it on the same tick.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADirectionalLight* RuntimeKey = World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-35.0, -35.0, 0.0),
		SpawnParameters);
	UDirectionalLightComponent* RuntimeComponent = RuntimeKey
		? Cast<UDirectionalLightComponent>(RuntimeKey->GetLightComponent()) : nullptr;
	if (!RuntimeKey || !RuntimeComponent)
	{
		return;
	}
	RuntimeKey->Tags.AddUnique(RuntimeStellarKeyLightTag);
	RuntimeKey->SetActorEnableCollision(false);
	RuntimeComponent->SetMobility(EComponentMobility::Movable);
	RuntimeComponent->SetAtmosphereSunLight(true);
	RuntimeComponent->SetForwardShadingPriority(1);
	RuntimeComponent->SetLightColor(FLinearColor::White);
	RuntimeComponent->SetIntensity(10.0f);
	DirectionalLight = RuntimeKey;
	SmoothedLightColor = FLinearColor::White;
	SmoothedLightIntensity = 10.0f;
	bOwnsDirectionalLight = true;
	UE_LOG(LogAPSStellarVisuals, Log,
		TEXT("Created transient generated-star directional key %s"), *RuntimeKey->GetName());
}

void UAPSStellarVisualSubsystem::UpdatePreviewFillLight(
	const APlanetaryBody* PreviewBody,
	const FVector& PreviewCameraLocation,
	bool bHasPreviewCameraLocation,
	float DeltaTime)
{
	ADirectionalLight* FillLight = PreviewFillLight.Get();
	UDirectionalLightComponent* FillComponent = FillLight
		? Cast<UDirectionalLightComponent>(FillLight->GetLightComponent()) : nullptr;

	const bool bShouldIlluminatePreview = IsValid(PreviewBody) && bHasPreviewCameraLocation;
	if (!bShouldIlluminatePreview)
	{
		if (FillComponent && FillComponent->IsVisible())
		{
			FillComponent->SetVisibility(false);
		}
		return;
	}

	FVector PresentedBodyCenter = PreviewBody->GetActorLocation();
	bool bContinuousPreview = false;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (It->UsesContinuousPreviewFrame() && PreviewBody->IsAttachedTo(*It))
			{
				bContinuousPreview = true;
				It->GetPreviewPresentationLocation(PreviewBody, PresentedBodyCenter);
				break;
			}
		}
	}
	const FVector LightRayDirection =
		(PresentedBodyCenter - PreviewCameraLocation).GetSafeNormal();
	if (LightRayDirection.IsNearlyZero())
	{
		if (FillComponent && FillComponent->IsVisible())
		{
			FillComponent->SetVisibility(false);
		}
		return;
	}

	const FRotator DesiredRotation = LightRayDirection.Rotation();
	if (!FillComponent)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FillLight = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			PresentedBodyCenter,
			DesiredRotation,
			SpawnParameters);
		FillComponent = FillLight
			? Cast<UDirectionalLightComponent>(FillLight->GetLightComponent()) : nullptr;
		if (!FillLight || !FillComponent)
		{
			return;
		}

		FillLight->Tags.AddUnique(PreviewFillLightTag);
		FillLight->SetActorEnableCollision(false);
		FillComponent->SetMobility(EComponentMobility::Movable);
		FillComponent->SetCastShadows(false);
		// A camera-facing readability fill is not a second sun. Directional lights
		// default to Atmosphere Sun Light enabled, so opt out explicitly. Give the
		// fill the unique highest forward priority while PLANET is active: opaque
		// terrain receives both lights, whereas SingleLayerWater can consume only
		// one directional light and otherwise rendered the complete ocean mesh black.
		FillComponent->SetAtmosphereSunLight(false);
		FillComponent->SetForwardShadingPriority(2);
		FillComponent->SetVolumetricScatteringIntensity(0.0f);
		FillComponent->SetLightColor(PreviewFillLightColor);
		FillComponent->SetIntensity(PreviewFillLightIntensity);
		PreviewFillLight = FillLight;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("Created transient PLANET-preview fill light (intensity=%.2f)"),
			PreviewFillLightIntensity);
	}

	// The camera-facing inspection fill is not a physical emitter. A specular
	// contribution puts an artificial, camera-locked "star" at the globe's centre.
	// Actual stellar lights retain their specular reflections.
	FillComponent->SetSpecularScale(bContinuousPreview ? 0.0f : 1.0f);
	if (!FillComponent->IsVisible())
	{
		FillComponent->SetVisibility(true);
	}
	const FRotator CurrentRotation = FillLight->GetActorRotation();
	FRotator UpdatedRotation = FMath::RInterpTo(
		CurrentRotation, DesiredRotation, DeltaTime, 12.0f);
	if (UpdatedRotation.Equals(DesiredRotation, 0.05f))
	{
		UpdatedRotation = DesiredRotation;
	}
	if (!CurrentRotation.Equals(UpdatedRotation, 0.001f))
	{
		FillLight->SetActorRotation(UpdatedRotation);
	}
}

void UAPSStellarVisualSubsystem::UpdateGameplayStationFillLight(
	const APawn* CharacterPawn,
	const FVector& CameraLocation,
	bool bHasCameraLocation)
{
	APointLight* FillLight = GameplayStationFillLight.Get();
	UPointLightComponent* FillComponent = FillLight
		? Cast<UPointLightComponent>(FillLight->GetLightComponent()) : nullptr;

	ASpaceStation* ContainingStation = nullptr;
	double BestNormalizedDistanceSquared = TNumericLimits<double>::Max();
	UWorld* World = GetWorld();
	if (World && IsValid(CharacterPawn) && bHasCameraLocation)
	{
		const FVector CharacterLocation = CharacterPawn->GetActorLocation();
		for (TActorIterator<ASpaceStation> It(World); It; ++It)
		{
			ASpaceStation* Candidate = *It;
			const USphereComponent* GravityZone = IsValid(Candidate)
				? Candidate->GravityCollisionZone : nullptr;
			if (!IsValid(GravityZone) || !GravityZone->IsRegistered())
			{
				continue;
			}

			const double RadiusCm = GravityZone->GetScaledSphereRadius();
			if (RadiusCm <= UE_DOUBLE_SMALL_NUMBER)
			{
				continue;
			}
			const double DistanceSquared = FVector::DistSquared(
				CharacterLocation, GravityZone->GetComponentLocation());
			const double NormalizedDistanceSquared =
				DistanceSquared / (RadiusCm * RadiusCm);
			if (NormalizedDistanceSquared <= 1.0
				&& NormalizedDistanceSquared < BestNormalizedDistanceSquared)
			{
				ContainingStation = Candidate;
				BestNormalizedDistanceSquared = NormalizedDistanceSquared;
			}
		}
	}

	if (!ContainingStation)
	{
		if (FillComponent && FillComponent->IsVisible())
		{
			FillComponent->SetVisibility(false);
			UE_LOG(LogAPSStellarVisuals, Log,
				TEXT("Gameplay station fill disabled character=%s previousStation=%s"),
				*GetNameSafe(CharacterPawn), *GetNameSafe(GameplayFillStation.Get()));
		}
		GameplayFillStation.Reset();
		return;
	}

	if (!FillComponent)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FillLight = World->SpawnActor<APointLight>(
			APointLight::StaticClass(), CameraLocation, FRotator::ZeroRotator,
			SpawnParameters);
		FillComponent = FillLight
			? Cast<UPointLightComponent>(FillLight->GetLightComponent()) : nullptr;
		if (!FillLight || !FillComponent)
		{
			return;
		}

		FillLight->Tags.AddUnique(GameplayStationFillLightTag);
		FillLight->SetActorEnableCollision(false);
		FillComponent->SetMobility(EComponentMobility::Movable);
		FillComponent->SetUseInverseSquaredFalloff(false);
		FillComponent->SetLightFalloffExponent(2.0f);
		FillComponent->SetIntensity(GameplayStationFillLightIntensity);
		FillComponent->SetAttenuationRadius(GameplayStationFillMinimumAttenuationRadiusCm);
		FillComponent->SetLightColor(GameplayStationFillLightColor);
		FillComponent->SetInverseExposureBlend(1.0f);
		FillComponent->SetCastShadows(false);
		FillComponent->SetAffectTranslucentLighting(false);
		FillComponent->SetVolumetricScatteringIntensity(0.0f);
		FillComponent->SetLightingChannels(true, false, false);
		FillComponent->SetVisibility(false);
		GameplayStationFillLight = FillLight;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("Created transient gameplay station fill intensity=%.1f attenuationRange=%.0f..%.0fcm"),
			GameplayStationFillLightIntensity, GameplayStationFillMinimumAttenuationRadiusCm,
			GameplayStationFillMaximumAttenuationRadiusCm);
	}

	if (!FillLight->GetActorLocation().Equals(CameraLocation, 0.5))
	{
		FillLight->SetActorLocation(CameraLocation, false, nullptr,
			ETeleportType::TeleportPhysics);
	}
	// The authored station is much larger than a character-scale room and its
	// spawn point sits near the exterior docking geometry. A fixed 45 m fill lit
	// only the pawn while the station remained black. Cover the actual camera to
	// station span, bounded so this local readability light never becomes a sun.
	const float DesiredAttenuationRadius = FMath::Clamp(
		static_cast<float>(FVector::Distance(
			CameraLocation, ContainingStation->GetActorLocation()) * 1.35),
		GameplayStationFillMinimumAttenuationRadiusCm,
		GameplayStationFillMaximumAttenuationRadiusCm);
	if (!FMath::IsNearlyEqual(
		FillComponent->AttenuationRadius, DesiredAttenuationRadius, 1.0f))
	{
		FillComponent->SetAttenuationRadius(DesiredAttenuationRadius);
	}
	// Rio 2026-09-28: 28 blew station interiors out under the fixed exposure; live-tunable now.
	static const IConsoleVariable* StationFill = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Lighting.StationFill"));
	const float DesiredFillIntensity = StationFill ? FMath::Max(0.0f, StationFill->GetFloat()) : GameplayStationFillLightIntensity;
	if (!FMath::IsNearlyEqual(FillComponent->Intensity, DesiredFillIntensity, 0.01f))
	{
		FillComponent->SetIntensity(DesiredFillIntensity);
	}
	const bool bStationChanged = GameplayFillStation.Get() != ContainingStation;
	if (!FillComponent->IsVisible() || bStationChanged)
	{
		FillComponent->SetVisibility(true);
		GameplayFillStation = ContainingStation;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("Gameplay station fill enabled character=%s station=%s camera=%s normalizedDistance=%.4f attenuation=%.0fcm"),
			*GetNameSafe(CharacterPawn), *GetNameSafe(ContainingStation),
			*CameraLocation.ToCompactString(), BestNormalizedDistanceSquared,
			DesiredAttenuationRadius);
	}
}

void UAPSStellarVisualSubsystem::UpdateGameplaySurfaceFillLight(
	const APawn* CharacterPawn,
	const FVector& ObserverLocation,
	bool bHasObserverLocation,
	float DeltaTime)
{
	ADirectionalLight* FillLight = GameplaySurfaceFillLight.Get();
	UDirectionalLightComponent* FillComponent = FillLight
		? Cast<UDirectionalLightComponent>(FillLight->GetLightComponent()) : nullptr;

	APlanetaryBody* ClosestSurfaceBody = nullptr;
	double ClosestSurfaceAltitudeCm = TNumericLimits<double>::Max();
	UWorld* World = GetWorld();
	if (World && IsValid(CharacterPawn) && bHasObserverLocation && bHasTargetStar)
	{
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			APlanetaryBody* Candidate = *It;
			if (!IsValid(Candidate) || !APSPlanetSurfaceFill::IsEligible(
				Candidate->bWorldScapeSurfaceReady, APSPlaceholderGlobe::Handles(Candidate)))
			{
				continue;
			}
			const double BodyRadiusCm = Candidate->GetWorldScapeBodyRadiusCm();
			if (!FMath::IsFinite(BodyRadiusCm) || BodyRadiusCm <= UE_DOUBLE_SMALL_NUMBER)
			{
				continue;
			}
			const double RadialDistanceCm = FVector::Distance(
				ObserverLocation, Candidate->GetActorLocation());
			const double SurfaceAltitudeCm = FMath::Abs(RadialDistanceCm - BodyRadiusCm);
			if (SurfaceAltitudeCm <= APSPlanetSurfaceFill::MaximumAltitudeCm
				&& SurfaceAltitudeCm < ClosestSurfaceAltitudeCm)
			{
				ClosestSurfaceBody = Candidate;
				ClosestSurfaceAltitudeCm = SurfaceAltitudeCm;
			}
		}
	}

	if (!ClosestSurfaceBody)
	{
		if (FillComponent && FillComponent->IsVisible())
		{
			FillComponent->SetVisibility(false);
			UE_LOG(LogAPSStellarVisuals, Log,
				TEXT("Gameplay surface fill disabled previousBody=%s"),
				*GetNameSafe(GameplayFillBody.Get()));
		}
		GameplayFillBody.Reset();
		return;
	}

	const FVector SurfaceOutward = (ObserverLocation
		- ClosestSurfaceBody->GetActorLocation()).GetSafeNormal();
	if (SurfaceOutward.IsNearlyZero())
	{
		return;
	}
	const FVector StarRayDirection = (ObserverLocation - TargetStarLocation).GetSafeNormal();
	FVector StarAzimuth = FVector::VectorPlaneProject(
		StarRayDirection, SurfaceOutward).GetSafeNormal();
	if (StarAzimuth.IsNearlyZero())
	{
		const FVector ReferenceAxis = FMath::Abs(SurfaceOutward.Z) < 0.82
			? FVector::UpVector : FVector::ForwardVector;
		StarAzimuth = FVector::CrossProduct(ReferenceAxis, SurfaceOutward)
			.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::RightVector);
	}
	// Forty-five degree off-axis incidence exposes the real WorldScape vertex
	// normals instead of front-lighting the material into a flat colour. The low
	// intensity only lifts the fixed-exposure floor; the generated star remains the
	// dominant key and continues to own shadows and atmosphere direction.
	const FVector DesiredLightRayDirection =
		(-SurfaceOutward * 0.72 + StarAzimuth * 0.69).GetSafeNormal();
	const FRotator DesiredRotation = DesiredLightRayDirection.Rotation();
	// Keep the accepted near-surface fill; fade it to zero before the existing
	// visibility cutoff instead of switching 2.2 lux off in one orbital frame.
	const float DesiredSurfaceFillIntensity = GameplaySurfaceFillLightIntensity
		* APSPlanetSurfaceFill::Weight(ClosestSurfaceAltitudeCm);

	if (!FillComponent)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FillLight = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(), FVector::ZeroVector, DesiredRotation,
			SpawnParameters);
		FillComponent = FillLight
			? Cast<UDirectionalLightComponent>(FillLight->GetLightComponent()) : nullptr;
		if (!FillLight || !FillComponent)
		{
			return;
		}

		FillLight->Tags.AddUnique(GameplaySurfaceFillLightTag);
		FillLight->SetActorEnableCollision(false);
		FillComponent->SetMobility(EComponentMobility::Movable);
		// A secondary fill must not produce a second, conflicting terrain shadow.
		// Relief remains defined by the shadow-casting generated-star key.
		FillComponent->SetCastShadows(false);
		FillComponent->SetAtmosphereSunLight(false);
		FillComponent->SetForwardShadingPriority(0);
		FillComponent->SetVolumetricScatteringIntensity(0.0f);
		FillComponent->SetLightColor(GameplaySurfaceFillLightColor);
		FillComponent->SetIntensity(DesiredSurfaceFillIntensity);
		FillComponent->SetSpecularScale(0.0f);
		FillComponent->SetLightingChannels(true, false, false);
		GameplaySurfaceFillLight = FillLight;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("Created transient gameplay surface fill intensity=%.2f"),
			GameplaySurfaceFillLightIntensity);
	}

	if (!FillComponent->IsVisible())
	{
		FillComponent->SetVisibility(true);
	}
	// Keep hot-reloaded/editor worlds deterministic as well: an already-created
	// transient fill must adopt the current readability contract without forcing a
	// destroy/recreate cycle that would flash the terrain for one frame.
	// This diffuse readability fill is not another physical star. Its reflection
	// otherwise becomes a large, off-axis white disc on oceans and wet terrain.
	if (!FMath::IsNearlyZero(FillComponent->SpecularScale))
	{
		FillComponent->SetSpecularScale(0.0f);
	}
	if (!FillComponent->GetLightColor().Equals(GameplaySurfaceFillLightColor, 0.0005f))
	{
		FillComponent->SetLightColor(GameplaySurfaceFillLightColor);
	}
	if (!FMath::IsNearlyEqual(
		FillComponent->Intensity, DesiredSurfaceFillIntensity, 0.00001f))
	{
		FillComponent->SetIntensity(DesiredSurfaceFillIntensity);
	}
	const FRotator CurrentRotation = FillLight->GetActorRotation();
	FRotator UpdatedRotation = FMath::RInterpTo(
		CurrentRotation, DesiredRotation, DeltaTime, 8.0f);
	if (UpdatedRotation.Equals(DesiredRotation, 0.05f))
	{
		UpdatedRotation = DesiredRotation;
	}
	if (!CurrentRotation.Equals(UpdatedRotation, 0.001f))
	{
		FillLight->SetActorRotation(UpdatedRotation);
	}
	if (GameplayFillBody.Get() != ClosestSurfaceBody)
	{
		GameplayFillBody = ClosestSurfaceBody;
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("Gameplay surface fill enabled body=%s altitude=%.0fcm ray=%s"),
			*GetNameSafe(ClosestSurfaceBody), ClosestSurfaceAltitudeCm,
			*DesiredLightRayDirection.ToCompactString());
	}
}

void UAPSStellarVisualSubsystem::ResolveNearestStar(const FVector& ObserverLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	double BestDistanceSquared = TNumericLimits<double>::Max();
	FVector BestLocation = FVector::ZeroVector;
	FLinearColor BestColor = FLinearColor::White;
	int32 BestTemperature = 0;
	float BestLuminosity = 1.0f;
	FString BestIdentity;
	const AStar* BestMaterializedStar = nullptr;
	bool bHasMaterializedStar = false;
	const AStarSystem* PreviewSystem = nullptr;
	for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		if (It->UsesContinuousPreviewFrame()) { PreviewSystem = It->GetContinuousPreviewActiveSystem(); break; }

	// Rio 06.10 (still ship): a ship owing its travel stays put; a star actor lights it from where it is relative to the
	// ship truly, its sky place (a system riding with the sky where it is, any other at its place + the sky offset).
	const FVector SkyOffset = UAPSWorldOriginSubsystem::SkyOffsetOf(World);
	for (TActorIterator<AStar> It(World); It; ++It)
	{
		const AStar* Star = *It;
		if (!IsValid(Star) || (PreviewSystem && !Star->IsAttachedTo(PreviewSystem)))
		{
			continue;
		}
		bHasMaterializedStar = true;
		const FVector StarPlace = UAPSWorldOriginSubsystem::SkyPlace(*Star);
		const double DistanceSquared = FVector::DistSquared(ObserverLocation, StarPlace);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestLocation = StarPlace;
			BestColor = UStarGenerator::GetStarColor(Star->SpectralClass, Star->SpectralSubclass);
			BestTemperature = Star->SurfaceTemperature;
			BestLuminosity = Star->Luminosity;
			BestIdentity = Star->GetPathName();
			BestMaterializedStar = Star;
		}
	}

	// Cluster HISM records live in a compressed presentation frame. They are a
	// far-field fallback only and must never replace a materialized physical star
	// when driving gameplay lighting.
	for (TActorIterator<AStarCluster> ClusterIt(World); !bHasMaterializedStar && ClusterIt; ++ClusterIt)
	{
		const AStarCluster* Cluster = *ClusterIt;
		if (!IsValid(Cluster) || !Cluster->StarMeshInstances)
		{
			continue;
		}

		const FClusterStarSystemRecord* ClosestClusterRecord = nullptr;
		FVector ClosestProxyWorldLocation = FVector::ZeroVector;
		double ClosestDistanceSquared = TNumericLimits<double>::Max();
		for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
		{
			if (Record.bMaterialized || Record.InstanceIndex == INDEX_NONE)
			{
				continue;
			}
			// The record location is canonical cluster space, not component-local cm.
			// Resolve the immutable projected base instead of reading a mutable HISM
			// presentation transform or transforming canonical units as centimeters.
			const FVector ProxyWorldLocation = Cluster->GetPotentialSystemWorldLocation(Record);
			const double DistanceSquared = FVector::DistSquared(
				ObserverLocation, ProxyWorldLocation);
			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestProxyWorldLocation = ProxyWorldLocation;
				ClosestClusterRecord = &Record;
			}
		}
		if (ClosestClusterRecord)
		{
			if (ClosestDistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = ClosestDistanceSquared;
				BestLocation = ClosestProxyWorldLocation;
				BestColor = UStarGenerator::GetStarColor(
					ClosestClusterRecord->PrimaryStarModel.SpectralClass,
					ClosestClusterRecord->PrimaryStarModel.SpectralSubclass);
				BestLuminosity = ClosestClusterRecord->PrimaryStarModel.Luminosity;
				BestTemperature = ClosestClusterRecord->PrimaryStarModel.SurfaceTemperature;
				BestIdentity = ClosestClusterRecord->StableId.ToString(
					EGuidFormats::DigitsWithHyphensLower);
			}
		}
	}

	bHasTargetStar = BestDistanceSquared < TNumericLimits<double>::Max();
	if (!bHasTargetStar)
	{
		return;
	}
	TargetStarLocation = BestLocation;
	const auto* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
	TargetStarGenerationLocation = Origin ? Origin->ToGenerationFrame(BestLocation - SkyOffset) : BestLocation;
	const AAstroGenerator* GeneratedWorld = GameplayStellarGenerator.Get();
	const AStarSystem* GeneratedHomeSystem = IsValid(GeneratedWorld)
		? GeneratedWorld->GetPreviewHomeSystem() : nullptr;
	// The visible photosphere emits this directional key: casting its own sphere's
	// shadow blocks that key on the illuminated planet once its direction settles.
	// Only touch the selected physical emitter of a committed generated hierarchy;
	// authored SinglePlay, PLANET preview, other stars and terrain keep their flags.
	if (IsValid(GeneratedWorld) && GeneratedWorld->bGenerateFullScaledWorld
		&& !GeneratedWorld->bIntegrateStartPlanet
		&& !GeneratedWorld->ActorHasTag(TEXT("WorldGenerationPreview"))
		&& !GeneratedWorld->UsesContinuousPreviewFrame()
		&& GeneratedWorld->GetCanonicalStellarProjectionDescriptor().bConsumedFinalizedDataset
		&& IsValid(BestMaterializedStar)
		&& (BestMaterializedStar == GeneratedWorld->HomeStar
			|| (IsValid(GeneratedHomeSystem) && BestMaterializedStar->IsAttachedTo(GeneratedHomeSystem)))
		&& IsValid(BestMaterializedStar->StarMesh) && BestMaterializedStar->StarMesh->CastShadow)
	{
		BestMaterializedStar->StarMesh->SetCastShadow(false);
		UE_LOG(LogAPSStellarVisuals, Log,
			TEXT("[APS.StellarLighting.EmitterShadow] generator=%s activeStar=%s photosphere=%s castShadow=0; emitting sphere must not occlude its own directional key"),
			*GetNameSafe(GeneratedWorld), *BestIdentity, *GetPathNameSafe(BestMaterializedStar->StarMesh));
	}
	// Rio 2026-09-28: the key takes the generated star's own colour temperature. It moves from neutral
	// 6500 K towards the star in mired space by aps.Lighting.StarKelvinStrength, so an M dwarf reads warm
	// and an O/B star cool without drowning the scene; aps.Lighting.StarKelvin 0 restores the old tint.
	static const IConsoleVariable* StarKelvin = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Lighting.StarKelvin"));
	static const IConsoleVariable* StarKelvinStrength = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Lighting.StarKelvinStrength"));
	if (BestTemperature > 0 && (!StarKelvin || StarKelvin->GetInt() != 0))
	{
		const double Strength = StarKelvinStrength ? FMath::Clamp(StarKelvinStrength->GetFloat(), 0.0f, 1.0f) : 0.75;
		const double Mired = FMath::Lerp(1.0e6 / 6500.0, 1.0e6 / FMath::Max(BestTemperature, 1000), Strength);
		TargetLightTemperature = static_cast<float>(FMath::Clamp(1.0e6 / Mired, 1700.0, 12000.0));
		TargetLightColor = FLinearColor::White;
	}
	else
	{
		TargetLightTemperature = 6500.0f;
		// Keep spectral identity readable without tinting the whole scene into an accessibility problem.
		TargetLightColor = FMath::Lerp(FLinearColor::White, BestColor.GetClamped(), 0.38f);
	}
	TargetLightColor.A = 1.0f;
	TargetLightIntensity = 9.0f + FMath::Clamp(
		FMath::LogX(10.0f, FMath::Max(BestLuminosity, 0.0f) + 1.0f) * 1.8f, 0.0f, 9.0f);
	// Rio 03.10 (the black hole V2): a hole is no sun; its key is the accretion disc's dim warm glow.
	if (IsValid(BestMaterializedStar) && BestMaterializedStar->StellarClass == EStellarType::BlackHole
		&& APSBlackHoleVisual::IsEnabled())
	{
		float DiskIntensity = 3.0f;
		float DiskKelvin = 4300.0f;
		APSBlackHoleVisual::GetDiskKeyLight(DiskIntensity, DiskKelvin);
		TargetLightTemperature = DiskKelvin;
		TargetLightColor = FLinearColor::White;
		TargetLightIntensity = DiskIntensity;
	}

	if (BestIdentity != ActiveStarIdentity)
	{
		ActiveStarIdentity = BestIdentity;
		UE_LOG(LogAPSStellarVisuals, Log, TEXT("Active star=%s color=%s intensity=%.2f"),
			*ActiveStarIdentity, *TargetLightColor.ToString(), TargetLightIntensity);
	}
}

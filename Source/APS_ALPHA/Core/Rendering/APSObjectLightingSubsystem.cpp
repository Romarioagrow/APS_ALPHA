#include "APSObjectLightingSubsystem.h"
#include "APSStellarVisualSubsystem.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSObjectLighting, Log, All);

namespace APSObjectLighting
{
	TAutoConsoleVariable<float> CVarObjectFill(
		TEXT("aps.Lighting.ObjectFill"), 1.8f,
		TEXT("Camera-aligned fill (lux) on lighting channel 1 for ships, stations and pilots, so their shadow side stays readable. 0 disables."));
	TAutoConsoleVariable<float> CVarObjectFillBacklit(
		TEXT("aps.Lighting.ObjectFillBacklit"), 3.0f,
		TEXT("Rio 04.10: extra object fill while the camera faces the star (the hull's night side in view), as a multiple of the ")
		TEXT("plain fill: 3 makes it four times as bright looking straight at the star. 0 keeps it even."));
	TAutoConsoleVariable<float> CVarObjectFillInStation(
		TEXT("aps.Lighting.ObjectFillInStation"), 0.0f,
		TEXT("Object fill (lux) while the pawn is inside a station gravity volume; interiors have their own lamps."));
	TAutoConsoleVariable<float> CVarStationLightScale(
		TEXT("aps.Lighting.StationLightScale"), 0.3f,
		TEXT("Multiplier for the lamps authored in station, headquarters and shipyard Blueprints (fixed exposure blew interiors out)."));
	TAutoConsoleVariable<int32> CVarAutoExposure(
		TEXT("aps.Lighting.AutoExposure"), 0,
		TEXT("1 enables histogram auto exposure within aps.Lighting.AutoExposureMinEV..MaxEV; 0 keeps the project's fixed exposure."));
	TAutoConsoleVariable<float> CVarAutoExposureMinEV(
		TEXT("aps.Lighting.AutoExposureMinEV"), -1.0f, TEXT("Lowest EV100 the auto exposure may use (dark scenes)."));
	TAutoConsoleVariable<float> CVarAutoExposureMaxEV(
		TEXT("aps.Lighting.AutoExposureMaxEV"), 3.0f, TEXT("Highest EV100 the auto exposure may use (bright interiors)."));
	// Read by ASpaceship and UAPSStellarVisualSubsystem through the console manager.
	TAutoConsoleVariable<int32> CVarPilotFill(
		TEXT("aps.Lighting.PilotFill"), 0,
		TEXT("1 restores the private point fill of the piloted ship in deep space (it lit only that ship)."));
	TAutoConsoleVariable<int32> CVarStarKelvin(
		TEXT("aps.Lighting.StarKelvin"), 1,
		TEXT("1 colours the star key light by the generated star's surface temperature (Kelvin); 0 uses the old spectral tint."));
	TAutoConsoleVariable<float> CVarStarKelvinStrength(
		TEXT("aps.Lighting.StarKelvinStrength"), 0.75f,
		TEXT("0..1: how far the key light moves from neutral 6500 K towards the star's temperature (in mired)."));
	TAutoConsoleVariable<float> CVarStationFill(
		TEXT("aps.Lighting.StationFill"), 8.0f,
		TEXT("Camera-local readability fill inside stations (was 28)."));

	const FName ObjectFillTag(TEXT("APSObjectFillLight"));
	const FLinearColor ObjectFillColor(0.86f, 0.9f, 1.0f, 1.0f);

	TAutoConsoleVariable<float> CVarNightFill(
		TEXT("aps.Planet.NightFill"), 0.6f,
		TEXT("Rio 04.10 evening: lux of the faint light travelling towards the star that lights a world's night side seen ")
		TEXT("from orbit (above 60 km; below, the near-surface fill does it). The star key is about 9.5. 0 = black night side."));
	/** Its own name, and the surface fill's: the star key search skips it and the water takes it for its one fill. */
	const FName NightFillTag(TEXT("APSOrbitalNightFillLight"));
	const FName SurfaceFillTag(TEXT("APSGameplaySurfaceFillLight"));
	const FLinearColor NightFillColor(0.55f, 0.68f, 1.0f, 1.0f);
	/** The near-surface fill ends at 50 km (APSPlanetSurfaceFill); this one starts above 60 km, so the two never meet. */
	constexpr double NightFillFromAltitudeCm = 6000000.0;
	constexpr double NightFillFullAltitudeCm = 15000000.0;
	/** Far from the world (its centre this many radii away) the fill fades: its night side is small by then. */
	constexpr double NightFillFullRadii = 8.0;
	constexpr double NightFillEndRadii = 12.0;
}

bool UAPSObjectLightingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UAPSObjectLightingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSObjectLightingSubsystem, STATGROUP_Tickables);
}

void UAPSObjectLightingSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}
	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	bool bInsideStation = false;
	if (const APawn* Pawn = PlayerController->GetPawn())
	{
		for (TActorIterator<ASpaceStation> It(World); It && !bInsideStation; ++It)
		{
			const USphereComponent* Zone = IsValid(*It) ? It->GravityCollisionZone : nullptr;
			bInsideStation = IsValid(Zone) && Zone->IsRegistered()
				&& FVector::DistSquared(Pawn->GetActorLocation(), Zone->GetComponentLocation())
					<= FMath::Square(Zone->GetScaledSphereRadius());
		}
	}

	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.5f || OptedInActors.IsEmpty())
	{
		RefreshElapsed = 0.0f;
		RefreshObjects();
	}
	UpdateObjectFill(CameraLocation, CameraRotation, bInsideStation);
	UpdateNightFill(DeltaTime);
	ApplyStationLightScale();
	UpdateExposure();
}

void UAPSObjectLightingSubsystem::UpdateObjectFill(
	const FVector& CameraLocation, const FRotator& CameraRotation, bool bInsideStation)
{
	float Intensity = FMath::Max(0.0f, bInsideStation
		? APSObjectLighting::CVarObjectFillInStation.GetValueOnGameThread()
		: APSObjectLighting::CVarObjectFill.GetValueOnGameThread());
	// Rio 04.10 ("in a far system the ship turned black"): the key light comes from the star the ship flies into, so the
	// chase camera sees the hull's night side, lit by the plain fill alone. Facing the star, the fill grows.
	const UAPSStellarVisualSubsystem* Stellar = GetWorld() ? GetWorld()->GetSubsystem<UAPSStellarVisualSubsystem>() : nullptr;
	FVector StarLocation;
	FString StarIdentity;
	if (!bInsideStation && Intensity > 0.0f && Stellar && Stellar->GetActiveStellarTarget(StarLocation, StarIdentity))
	{
		const float Backlit = FMath::Clamp(static_cast<float>(FVector::DotProduct(CameraRotation.Vector(),
			(StarLocation - CameraLocation).GetSafeNormal())), 0.0f, 1.0f);
		Intensity *= 1.0f + FMath::Max(APSObjectLighting::CVarObjectFillBacklit.GetValueOnGameThread(), 0.0f) * Backlit;
	}
	ADirectionalLight* Fill = ObjectFillLight.Get();
	UDirectionalLightComponent* Component = Fill ? Cast<UDirectionalLightComponent>(Fill->GetLightComponent()) : nullptr;
	if (Intensity <= 0.0f)
	{
		if (Component && Component->IsVisible())
		{
			Component->SetVisibility(false);
		}
		return;
	}
	if (!Component)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Fill = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),
			CameraLocation, CameraRotation, SpawnParameters);
		Component = Fill ? Cast<UDirectionalLightComponent>(Fill->GetLightComponent()) : nullptr;
		if (!Component)
		{
			return;
		}
		Fill->Tags.AddUnique(APSObjectLighting::ObjectFillTag);
		Fill->SetActorEnableCollision(false);
		Component->SetMobility(EComponentMobility::Movable);
		// Only objects that opted into channel 1 receive it; planets and terrain keep channel 0 only.
		Component->SetLightingChannels(false, true, false);
		Component->SetAtmosphereSunLight(false);
		Component->SetCastShadows(false);
		Component->SetVolumetricScatteringIntensity(0.0f);
		Component->SetAffectTranslucentLighting(false);
		Component->SetSpecularScale(0.35f);
		Component->SetLightColor(APSObjectLighting::ObjectFillColor);
		ObjectFillLight = Fill;
		UE_LOG(LogAPSObjectLighting, Log, TEXT("[APS.Lighting] object fill created intensity=%.2f lux"), Intensity);
	}
	if (!Component->IsVisible())
	{
		Component->SetVisibility(true);
	}
	// Light travels along the view direction: whatever the camera looks at is lit from the front.
	if (!Fill->GetActorRotation().Equals(CameraRotation, 0.5f))
	{
		Fill->SetActorRotation(CameraRotation);
	}
	if (!FMath::IsNearlyEqual(Component->Intensity, Intensity, 0.001f))
	{
		Component->SetIntensity(Intensity);
	}
}

void UAPSObjectLightingSubsystem::UpdateNightFill(const float DeltaTime)
{
	UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const float Lux = FMath::Max(0.0f, APSObjectLighting::CVarNightFill.GetValueOnGameThread());
	float Wanted = 0.0f;
	bool bNearSurface = false;
	FVector RayDirection = FVector::ZeroVector;
	// Gameplay pawns only: the menu's planet preview has its own fill, and its water expects exactly one.
	if (World && Lux > 0.0f && Pawn && (Pawn->IsA<ACustomGravityCharacter>() || Pawn->IsA<APilotingVehicle>()))
	{
		const FVector Observer = Pawn->GetActorLocation();
		const APlanetaryBody* Nearest = nullptr;
		double NearestAltitudeCm = TNumericLimits<double>::Max();
		double NearestRadiusCm = 0.0;
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			const double RadiusCm = IsValid(*It) ? It->GetWorldScapeBodyRadiusCm() : 0.0;
			if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 1.0)
			{
				continue;
			}
			const double AltitudeCm = FVector::Distance(Observer, It->GetActorLocation()) - RadiusCm;
			bNearSurface |= AltitudeCm <= APSObjectLighting::NightFillFromAltitudeCm;
			if (AltitudeCm < NearestAltitudeCm)
			{
				Nearest = *It;
				NearestAltitudeCm = AltitudeCm;
				NearestRadiusCm = RadiusCm;
			}
		}
		const UAPSStellarVisualSubsystem* Stellar = World->GetSubsystem<UAPSStellarVisualSubsystem>();
		FVector StarLocation;
		FString StarIdentity;
		if (Nearest && !bNearSurface && Stellar && Stellar->GetActiveStellarTarget(StarLocation, StarIdentity))
		{
			const double In = FMath::SmoothStep(APSObjectLighting::NightFillFromAltitudeCm,
				APSObjectLighting::NightFillFullAltitudeCm, NearestAltitudeCm);
			const double CentreRadii = (NearestAltitudeCm + NearestRadiusCm) / NearestRadiusCm;
			const double Out = 1.0 - FMath::SmoothStep(APSObjectLighting::NightFillFullRadii,
				APSObjectLighting::NightFillEndRadii, CentreRadii);
			Wanted = static_cast<float>(Lux * In * Out);
			// Exactly against the star key's rays (UAPSStellarVisualSubsystem aims those from the star at the pawn): a
			// surface either faces the star and keeps its look, or faces away and receives only this.
			RayDirection = (StarLocation - Observer).GetSafeNormal();
		}
	}
	ADirectionalLight* Fill = NightFillLight.Get();
	UDirectionalLightComponent* Component = Fill ? Cast<UDirectionalLightComponent>(Fill->GetLightComponent()) : nullptr;
	// Near a surface the accepted surface fill lights the night; it and this one are never on together (the water's
	// lighting takes exactly one fill), so this one goes out at once there; at 60 km it has faded to nothing anyway.
	NightFillIntensity = bNearSurface || RayDirection.IsNearlyZero() ? 0.0f
		: FMath::FInterpTo(NightFillIntensity, Wanted, DeltaTime, 1.5f);
	if (NightFillIntensity <= 0.001f)
	{
		NightFillIntensity = 0.0f;
		if (Component && Component->IsVisible())
		{
			Component->SetVisibility(false);
		}
		return;
	}
	if (!Component)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Fill = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector,
			RayDirection.Rotation(), SpawnParameters);
		Component = Fill ? Cast<UDirectionalLightComponent>(Fill->GetLightComponent()) : nullptr;
		if (!Component)
		{
			return;
		}
		Fill->Tags.AddUnique(APSObjectLighting::NightFillTag);
		Fill->Tags.AddUnique(APSObjectLighting::SurfaceFillTag);
		Fill->SetActorEnableCollision(false);
		Component->SetMobility(EComponentMobility::Movable);
		// As the surface fill: no second shadow, no glint on the oceans, not a sun for the atmosphere.
		Component->SetCastShadows(false);
		Component->SetAtmosphereSunLight(false);
		Component->SetForwardShadingPriority(0);
		Component->SetVolumetricScatteringIntensity(0.0f);
		Component->SetSpecularScale(0.0f);
		Component->SetLightingChannels(true, false, false);
		Component->SetLightColor(APSObjectLighting::NightFillColor);
		Component->SetIntensity(NightFillIntensity);
		NightFillLight = Fill;
		UE_LOG(LogAPSObjectLighting, Log, TEXT("[APS.Lighting] night fill created (%.2f lux wanted)"), Wanted);
	}
	if (!Component->IsVisible())
	{
		Component->SetVisibility(true);
	}
	const FRotator Rotation = RayDirection.Rotation();
	if (!Fill->GetActorRotation().Equals(Rotation, 0.05f))
	{
		Fill->SetActorRotation(Rotation);
	}
	if (!FMath::IsNearlyEqual(Component->Intensity, NightFillIntensity, 0.001f))
	{
		Component->SetIntensity(NightFillIntensity);
	}
}

void UAPSObjectLightingSubsystem::RefreshObjects()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (auto It = OptedInActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	// Rio 04.10 ("ships are black from afar, light up close; lighting must not depend on the player"): every refresh
	// checks every mesh again, not once per actor. A ship's own setup (boarding, its class, parts added later) could
	// clear channel 1 after the first opt-in, leaving that ship without the fill for good.
	const auto OptIn = [this](AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}
		TArray<UMeshComponent*> Meshes;
		Actor->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (IsValid(Mesh) && !Mesh->LightingChannels.bChannel1)
			{
				Mesh->SetLightingChannels(Mesh->LightingChannels.bChannel0, true, Mesh->LightingChannels.bChannel2);
			}
		}
		OptedInActors.Add(Actor);
	};
	for (TActorIterator<ASpaceship> It(World); It; ++It)
	{
		OptIn(*It);
	}
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		OptIn(*It);
	}
	for (TActorIterator<ASpaceStation> It(World); It; ++It)
	{
		OptIn(*It);
		TArray<ULocalLightComponent*> Lamps;
		It->GetComponents<ULocalLightComponent>(Lamps);
		const float Scale = FMath::Max(0.0f, APSObjectLighting::CVarStationLightScale.GetValueOnGameThread());
		for (ULocalLightComponent* Lamp : Lamps)
		{
			if (IsValid(Lamp) && !StationLightBaseIntensity.Contains(Lamp))
			{
				StationLightBaseIntensity.Add(Lamp, Lamp->Intensity);
				Lamp->SetIntensity(Lamp->Intensity * Scale);
			}
		}
	}
}

void UAPSObjectLightingSubsystem::ApplyStationLightScale()
{
	const float Scale = FMath::Max(0.0f, APSObjectLighting::CVarStationLightScale.GetValueOnGameThread());
	if (FMath::IsNearlyEqual(Scale, AppliedStationLightScale))
	{
		return;
	}
	for (auto It = StationLightBaseIntensity.CreateIterator(); It; ++It)
	{
		if (ULocalLightComponent* Lamp = It->Key.Get())
		{
			Lamp->SetIntensity(It->Value * Scale);
		}
		else
		{
			It.RemoveCurrent();
		}
	}
	AppliedStationLightScale = Scale;
	UE_LOG(LogAPSObjectLighting, Log, TEXT("[APS.Lighting] station lamps scaled x%.2f (%d lamps)"),
		Scale, StationLightBaseIntensity.Num());
}

void UAPSObjectLightingSubsystem::UpdateExposure()
{
	const bool bAutoExposure = APSObjectLighting::CVarAutoExposure.GetValueOnGameThread() != 0;
	APostProcessVolume* Volume = ExposureVolume.Get();
	if (!bAutoExposure)
	{
		if (Volume && Volume->bEnabled)
		{
			Volume->bEnabled = false;
		}
		return;
	}
	if (!Volume)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		Volume = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(),
			FTransform::Identity, SpawnParameters);
		if (!Volume)
		{
			return;
		}
		Volume->bUnbound = true;
		Volume->Priority = 100.0f;
		ExposureVolume = Volume;
	}
	Volume->bEnabled = true;
	FPostProcessSettings& Settings = Volume->Settings;
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = AEM_Histogram;
	Settings.bOverride_AutoExposureMinBrightness = true;
	Settings.AutoExposureMinBrightness = APSObjectLighting::CVarAutoExposureMinEV.GetValueOnGameThread();
	Settings.bOverride_AutoExposureMaxBrightness = true;
	Settings.AutoExposureMaxBrightness = FMath::Max(Settings.AutoExposureMinBrightness,
		APSObjectLighting::CVarAutoExposureMaxEV.GetValueOnGameThread());
	Settings.bOverride_AutoExposureSpeedUp = true;
	Settings.AutoExposureSpeedUp = 2.0f;
	Settings.bOverride_AutoExposureSpeedDown = true;
	Settings.AutoExposureSpeedDown = 1.5f;
}

void UAPSObjectLightingSubsystem::Deinitialize()
{
	for (const TPair<TWeakObjectPtr<ULocalLightComponent>, float>& Entry : StationLightBaseIntensity)
	{
		if (ULocalLightComponent* Lamp = Entry.Key.Get())
		{
			Lamp->SetIntensity(Entry.Value);
		}
	}
	StationLightBaseIntensity.Reset();
	if (ADirectionalLight* Fill = ObjectFillLight.Get())
	{
		Fill->Destroy();
	}
	if (ADirectionalLight* NightFill = NightFillLight.Get())
	{
		NightFill->Destroy();
	}
	if (APostProcessVolume* Volume = ExposureVolume.Get())
	{
		Volume->Destroy();
	}
	Super::Deinitialize();
}

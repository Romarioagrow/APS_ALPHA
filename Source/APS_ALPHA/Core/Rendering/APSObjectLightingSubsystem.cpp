#include "APSObjectLightingSubsystem.h"

#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
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
	ApplyStationLightScale();
	UpdateExposure();
}

void UAPSObjectLightingSubsystem::UpdateObjectFill(
	const FVector& CameraLocation, const FRotator& CameraRotation, bool bInsideStation)
{
	const float Intensity = FMath::Max(0.0f, bInsideStation
		? APSObjectLighting::CVarObjectFillInStation.GetValueOnGameThread()
		: APSObjectLighting::CVarObjectFill.GetValueOnGameThread());
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
	const auto OptIn = [this](AActor* Actor)
	{
		if (!IsValid(Actor) || OptedInActors.Contains(Actor))
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
	if (APostProcessVolume* Volume = ExposureVolume.Get())
	{
		Volume->Destroy();
	}
	Super::Deinitialize();
}

#include "PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Planetary/PlanetAtmosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetBiosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetGeosphere.h"
#include "APS_ALPHA/Core/Enums/Planetary/BiodiversityIndex.h"
#include "APS_ALPHA/Core/Enums/Planetary/BiomassLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/CO2Level.h"
#include "APS_ALPHA/Core/Enums/Planetary/CrustThicknessLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/HumidityLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/MagneticFieldStrength.h"
#include "APS_ALPHA/Core/Enums/Planetary/OxigenLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/PressureLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/SeismicActivityLevel.h"
#include "APS_ALPHA/Core/Enums/Planetary/WindSpeed.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"

APlanetaryBody::APlanetaryBody()
{
	PlanetData.PlanetModel = MakeShared<FPlanetModel>();
	
	PlanetAtmosphere.OxygenLevel = EOxigenLevel::NoOxigen;
	PlanetAtmosphere.CO2Level = ECO2Level::NoCO2;
	PlanetAtmosphere.WindSpeedLevel = EWindSpeed::NoWind;
	PlanetAtmosphere.PressureLevel = EPressureLevel::NoPressure;
	PlanetAtmosphere.HumidityLevel = EHumidityLevel::NoHumidity;

	PlanetBiosphere.BiomassLevel = EBiomassLevel::NoBiomass;
	PlanetBiosphere.BiodiversityLevel = EBiodiversityIndex::NoBiodiversity;

	PlanetGeosphere.SeismicActivityLevel = ESeismicActivityLevel::Inactive;
	PlanetGeosphere.CrustThicknessLevel = ECrustThicknessLevel::VeryThin;
	PlanetGeosphere.MagneticFieldStrengthLevel = EMagneticFieldStrength::NoField;

	// Применение вычисленных значений
	PlanetAtmosphere.UpdateAtmosphereProperties();
	PlanetBiosphere.UpdateBiosphereProperties();
	PlanetGeosphere.UpdateGeosphereProperties();
}

void APlanetaryBody::BeginPlay()
{
	Super::BeginPlay();

	// The legacy environment API still uses this helper for planetary atmosphere
	// setup. It is attached below so it stays nested under its body in the Outliner;
	// it no longer allocates a WorldScape root until the body becomes Active.
	EnsurePlanetaryEnvironmentGenerator();
	if (bGenerateByDefault && !bStreamWorldScapeSurface)
	{
		SetWorldScapeStreamingActive(true);
	}
	
	PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
}

APlanetarySurfaceGenerator* APlanetaryBody::EnsurePlanetaryEnvironmentGenerator()
{
	if (IsValid(PlanetaryEnvironmentGenerator))
	{
		PlanetaryEnvironmentGenerator->PlanetaryBody = this;
		return PlanetaryEnvironmentGenerator;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PlanetaryEnvironmentGenerator = World->SpawnActor<APlanetarySurfaceGenerator>(
		APlanetarySurfaceGenerator::StaticClass(), GetActorTransform(), SpawnParameters);
	if (PlanetaryEnvironmentGenerator)
	{
		PlanetaryEnvironmentGenerator->PlanetaryBody = this;
		PlanetaryEnvironmentGenerator->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		PlanetaryEnvironmentGenerator->SetActorHiddenInGame(true);
		PlanetaryEnvironmentGenerator->SetActorEnableCollision(false);
		PlanetaryEnvironmentGenerator->SetActorTickEnabled(false);
	}
	return PlanetaryEnvironmentGenerator;
}

bool APlanetaryBody::EnsureWorldScapeSurface()
{
	if (!UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(PlanetType))
	{
		// A preview/runtime type switch may leave the former solid world's root
		// resident while its async jobs drain. This guard is body-generic because a
		// generated Gas moon also carries PlanetType::GasGiant.
		if (IsValid(PlanetaryEnvironmentGenerator))
		{
			PlanetaryEnvironmentGenerator->UnloadWorldScapeRoot();
		}
		return false;
	}

	APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator();
	if (!Generator || IsValid(Generator->WorldScapeRootInstance))
	{
		return Generator && IsValid(Generator->WorldScapeRootInstance);
	}

	return Generator->CreateRuntimeWorldScapeRoot(this);
}

void APlanetaryBody::SetWorldScapeStreamingActive(bool bActive)
{
	SetWorldScapeStreamingState(bActive
		? EWorldScapeSurfaceState::Active
		: EWorldScapeSurfaceState::Unloaded);
}

bool APlanetaryBody::IsWorldScapeStreamingActive() const
{
	const AWorldScapeRoot* Root = IsValid(PlanetaryEnvironmentGenerator)
		? PlanetaryEnvironmentGenerator->WorldScapeRootInstance : nullptr;
	return WorldScapeSurfaceState == EWorldScapeSurfaceState::Active
		&& IsValid(Root)
		&& (bWorldScapeSurfaceReady
			|| (Root->bGenerateWorldScape && !Root->bFreezeGeneration));
}

double APlanetaryBody::GetWorldScapeActivationRadiusCm() const
{
	const double BodyRadiusCm = GetWorldScapeBodyRadiusCm();
	// Existing Blueprint CDOs may still serialize the old near-field value. Keep
	// the native far preload guarantee even before those assets are resaved.
	const double EffectiveMultiplier = FMath::Max(WorldScapeActivationRadiusMultiplier, 96.0);
	return FMath::Max(BodyRadiusCm * EffectiveMultiplier, BodyRadiusCm * 1.25);
}

double APlanetaryBody::GetWorldScapeBodyRadiusCm() const
{
	double BodyRadiusCm = FMath::Max(RadiusKM, static_cast<double>(PlanetRadiusKM)) * 100000.0;
	if (BodyRadiusCm <= UE_DOUBLE_SMALL_NUMBER)
	{
		FVector Origin;
		FVector Extent;
		GetActorBounds(false, Origin, Extent);
		BodyRadiusCm = Extent.GetMax();
	}
	return FMath::Max(BodyRadiusCm, 100000.0);
}

double APlanetaryBody::GetWorldScapeDeactivationRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double EffectiveActivationMultiplier = FMath::Max(WorldScapeActivationRadiusMultiplier, 96.0);
	const double BodyRadiusCm = ActivationRadius / EffectiveActivationMultiplier;
	return FMath::Max(ActivationRadius * 1.25,
		BodyRadiusCm * FMath::Max(WorldScapeDeactivationRadiusMultiplier, 128.0));
}

void APlanetaryBody::FillPlanetData()
{
	// Заполняем параметры планеты
	//PlanetData.PlanetOrder = PlanetOrder;
	//PlanetData.OrbitRadius = OrbitRadius;
	PlanetData.PlanetRadiusKM = PlanetRadiusKM;
	PlanetData.Temperature = Temperature;
	PlanetData.PlanetDensity = PlanetDensity;
	PlanetData.PlanetGravityStrength = PlanetGravityStrength;

	// Заполняем данные окружения
	PlanetData.PlanetAtmosphere = PlanetAtmosphere;
	PlanetData.PlanetBiosphere = PlanetBiosphere;
	PlanetData.PlanetGeosphere = PlanetGeosphere;
	
	PlanetData.PlanetModelData = *(PlanetData.PlanetModel); //PlanetData.Duplicate();
	PlanetData.PlanetModelData.MoonsListData = PlanetData.PlanetModelData.GetMoonsData(); //*(PlanetData.PlanetModelData.MoonsList); //PlanetData.Duplicate();
	
}

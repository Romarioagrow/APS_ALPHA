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
		PlanetaryEnvironmentGenerator->SetActorHiddenInGame(true);
		PlanetaryEnvironmentGenerator->SetActorEnableCollision(false);
	}
	return PlanetaryEnvironmentGenerator;
}

bool APlanetaryBody::EnsureWorldScapeSurface()
{
	APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator();
	if (!Generator || IsValid(Generator->WorldScapeRootInstance))
	{
		return Generator && IsValid(Generator->WorldScapeRootInstance);
	}

	if (APlanet* Planet = Cast<APlanet>(this))
	{
		if (!Planet->IsNotGasGiant())
		{
			return false;
		}
		Generator->GenerateWorldscapeSurfaceByModel(GetWorld(), Planet);
	}
	else if (AMoon* Moon = Cast<AMoon>(this))
	{
		Generator->GenerateWorldscapeSurfaceByModel(GetWorld(), Moon);
	}
	return IsValid(Generator->WorldScapeRootInstance);
}

void APlanetaryBody::SetWorldScapeStreamingActive(bool bActive)
{
	if (!bStreamWorldScapeSurface && !bGenerateByDefault)
	{
		return;
	}

	APlanetarySurfaceGenerator* Generator = EnsurePlanetaryEnvironmentGenerator();
	if (!Generator)
	{
		return;
	}
	if (bActive)
	{
		if (EnsureWorldScapeSurface())
		{
			Generator->SpawnWorldScapeRoot();
			if (APlanet* Planet = Cast<APlanet>(this)) Planet->DisableSphereMesh();
			else if (AMoon* Moon = Cast<AMoon>(this)) Moon->DisableSphereMesh();
			bEnvironmentSpawned = true;
		}
	}
	else if (IsValid(Generator->WorldScapeRootInstance))
	{
		Generator->DestroyPlanetEnvironment();
		if (APlanet* Planet = Cast<APlanet>(this)) Planet->EnableSphereMesh();
		else if (AMoon* Moon = Cast<AMoon>(this)) Moon->EnableSphereMesh();
		bEnvironmentSpawned = false;
	}
}

bool APlanetaryBody::IsWorldScapeStreamingActive() const
{
	return IsValid(PlanetaryEnvironmentGenerator)
		&& IsValid(PlanetaryEnvironmentGenerator->WorldScapeRootInstance)
		&& PlanetaryEnvironmentGenerator->WorldScapeRootInstance->bGenerateWorldScape
		&& !PlanetaryEnvironmentGenerator->WorldScapeRootInstance->bFreezeGeneration
		&& !PlanetaryEnvironmentGenerator->WorldScapeRootInstance->IsHidden();
}

double APlanetaryBody::GetWorldScapeActivationRadiusCm() const
{
	double BodyRadiusCm = FMath::Max(RadiusKM, static_cast<double>(PlanetRadiusKM)) * 100000.0;
	if (BodyRadiusCm <= UE_DOUBLE_SMALL_NUMBER)
	{
		FVector Origin;
		FVector Extent;
		GetActorBounds(false, Origin, Extent);
		BodyRadiusCm = Extent.GetMax();
	}
	return FMath::Max(BodyRadiusCm * WorldScapeActivationRadiusMultiplier, BodyRadiusCm * 1.25);
}

double APlanetaryBody::GetWorldScapeDeactivationRadiusCm() const
{
	const double ActivationRadius = GetWorldScapeActivationRadiusCm();
	const double BodyRadiusCm = ActivationRadius / FMath::Max(WorldScapeActivationRadiusMultiplier, 1.0);
	return FMath::Max(ActivationRadius * 1.25, BodyRadiusCm * WorldScapeDeactivationRadiusMultiplier);
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

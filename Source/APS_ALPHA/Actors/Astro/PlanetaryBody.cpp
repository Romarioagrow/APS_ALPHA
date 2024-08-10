#include "PlanetaryBody.h"

#include "APS_ALPHA/Actors/Planetary/PlanetAtmosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetBiosphere.h"
#include "APS_ALPHA/Actors/Planetary/PlanetGeosphere.h"
#include "APS_ALPHA/Core/Enums/BiodiversityIndex.h"
#include "APS_ALPHA/Core/Enums/BiomassLevel.h"
#include "APS_ALPHA/Core/Enums/CO2Level.h"
#include "APS_ALPHA/Core/Enums/CrustThicknessLevel.h"
#include "APS_ALPHA/Core/Enums/HumidityLevel.h"
#include "APS_ALPHA/Core/Enums/MagneticFieldStrength.h"
#include "APS_ALPHA/Core/Enums/OxigenLevel.h"
#include "APS_ALPHA/Core/Enums/PressureLevel.h"
#include "APS_ALPHA/Core/Enums/SeismicActivityLevel.h"
#include "APS_ALPHA/Core/Enums/WindSpeed.h"
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
	
	PlanetaryEnvironmentGenerator = NewObject<APlanetarySurfaceGenerator>();
	if (PlanetaryEnvironmentGenerator)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("PlanetSurfaceGenerator has been created successfully."));

		if (bGenerateByDefault)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				PlanetaryEnvironmentGenerator->InitWorldScape(World);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("World Scape Initiated!"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("World IS NULL"));
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to create PlanetSurfaceGenerator."));
		UE_LOG(LogTemp, Warning, TEXT("Failed to create PlanetSurfaceGenerator."));
	}

	if (bGenerateByDefault && !bEnvironmentSpawned)
	{
		bEnvironmentSpawned = true;
	}
	
	PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
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
	///PlanetData.PlanetModelData.MoonsListData = PlanetData.PlanetModelData.GetMoonsData(); //*(PlanetData.PlanetModelData.MoonsList); //PlanetData.Duplicate();
	
}
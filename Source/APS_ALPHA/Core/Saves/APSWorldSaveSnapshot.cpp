#include "APSWorldSaveSnapshot.h"

#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace
{
	void ApplyLegacySummary(const FGeneratedWorldData& Data, UGeneratedWorld& Model,
		const FString& SlotName)
	{
		// Old saves did not retain a seed.  A slot-derived value cannot recreate the
		// historical random roll, but it does make every subsequent legacy load stable.
		Model.GenerationSeed = 1 + static_cast<int32>(
			GetTypeHash(SlotName) % static_cast<uint32>(MAX_int32 - 1));
		Model.PreviewDisplayNameOverrides = Data.PreviewDisplayNameOverrides;
		Model.bGenerateFullScaledWorld = Data.bGenerateFullScaledWorld;
		Model.bGenerateHomeSystem = Data.bGenerateHomeSystem;
		Model.bStartWithHomePlanet = Data.bStartWithHomePlanet;
		Model.bRandomHomeSystem = Data.bRandomHomeSystem;
		Model.bRandomHomeSystemType = Data.bRandomHomeSystemType;
		Model.bRandomHomeStar = Data.bRandomHomeStar;
		Model.bRandomStartPlanetNumber = Data.bRandomStartPlanetNumber;
		Model.AstroGenerationLevel = Data.AstroGenerationLevel;
		Model.GalaxyType = Data.GalaxyType;
		Model.GalaxyClass = Data.GalaxyClass;
		Model.StarClusterSize = Data.StarClusterSize;
		Model.StarClusterType = Data.StarClusterType;
		Model.StarClusterPopulation = Data.StarClusterPopulation;
		Model.StarClusterComposition = Data.StarClusterComposition;
		Model.StarType = Data.StarType;
		Model.StellarType = Data.StellarType;
		Model.SpectralClass = Data.SpectralClass;
		Model.HomeStarRadiusOverrideSolar = Data.HomeStarRadiusOverrideSolar;
		Model.PlanetarySystemType = Data.PlanetarySystemType;
		Model.OrbitDistributionType = Data.OrbitDistributionType;
		Model.HomeSystemPosition = Data.HomeSystemPosition;
		Model.PlanetType = Data.PlanetType;
		Model.PlanetHabitability = Data.PlanetHabitability;
		Model.GalaxySize = Data.GalaxySize;
		Model.GalaxyStarCount = Data.GalaxyStarCount;
		Model.PlanetsAmount = Data.PlanetsAmount;
		Model.MoonsAmount = Data.MoonsAmount;
		Model.StartPlanetIndex = Data.StartPlanetIndex;
		Model.GalaxyStarDensity = Data.GalaxyStarDensity;
		Model.PlanetRadius = Data.PlanetRadius;
		Model.PlanetSurfaceSeed = Data.PlanetSurfaceSeed;
		Model.SurfaceFeatureScale = Data.SurfaceFeatureScale;
		Model.SurfaceReliefScale = Data.SurfaceReliefScale;
		Model.SurfaceLandCoverageScale = Data.SurfaceLandCoverageScale;
		Model.SurfaceMountainScale = Data.SurfaceMountainScale;
		Model.SurfaceCraterScale = Data.SurfaceCraterScale;
		Model.SurfaceRoughnessScale = Data.SurfaceRoughnessScale;
		Model.AtmosphereHeight = Data.AtmosphereHeight;
		Model.AtmosphereOpacity = Data.AtmosphereOpacity;
		Model.AtmosphereMultiScattering = Data.AtmosphereMultiScattering;
		Model.AtmosphereRayleighScattering = Data.AtmosphereRayleighScattering;
		Model.AtmosphereColor = Data.AtmosphereColor;
		Model.StarsAmount = Data.StarsAmount;
		Model.HomeStarName = Data.HomeStarName;
		Model.HomePlanetName = Data.HomePlanetName;
		Model.FullSpectralName = Data.FullSpectralName;
		Model.HomeStarMass = Data.HomeStarMass;
		Model.HomeStarRadius = Data.HomeStarRadius;
		Model.HomeStarTemperature = Data.HomeStarTemperature;
		Model.StarSystemRadius = Data.StarSystemRadius;
	}
}

bool APSWorldSaveSnapshot::Capture(const UGeneratedWorld* WorldModel, TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	if (!IsValid(WorldModel))
	{
		return false;
	}

	FMemoryWriter Writer(OutBytes, true);
	FObjectAndNameAsStringProxyArchive Archive(Writer, false);
	Archive.ArNoDelta = true;
	const_cast<UGeneratedWorld*>(WorldModel)->Serialize(Archive);
	Archive.Close();
	Writer.Close();
	return !Writer.IsError() && !OutBytes.IsEmpty();
}

UGeneratedWorld* APSWorldSaveSnapshot::Restore(const UGameSave* Save, UObject* Outer,
	const FString& SlotName)
{
	if (!IsValid(Save) || !IsValid(Outer))
	{
		return nullptr;
	}

	UGeneratedWorld* Model = NewObject<UGeneratedWorld>(Outer);
	if (!IsValid(Model))
	{
		return nullptr;
	}

	bool bRestoredSnapshot = false;
	if (!Save->GeneratedWorldModelData.IsEmpty())
	{
		FMemoryReader Reader(Save->GeneratedWorldModelData, true);
		FObjectAndNameAsStringProxyArchive Archive(Reader, true);
		Archive.ArNoDelta = true;
		Model->Serialize(Archive);
		Archive.Close();
		bRestoredSnapshot = !Reader.IsError();
		Reader.Close();
	}

	if (!bRestoredSnapshot)
	{
		if (Save->GeneratedWorldsDataArray.IsEmpty())
		{
			return nullptr;
		}
		ApplyLegacySummary(Save->GeneratedWorldsDataArray[0], *Model, SlotName);
	}

	// Actor pointers belong to the world in which a model was captured.  Gameplay
	// materializes fresh actors from the actor-free data after OpenLevel.
	Model->HomePlanetarySystem = nullptr;
	Model->HomePlanet = nullptr;
	Model->InhabitedPlanets = Save->InhabitedPlanetsDataArray;
	Model->GenerationSeed = FMath::Max(Model->GenerationSeed, 1);
	return Model;
}

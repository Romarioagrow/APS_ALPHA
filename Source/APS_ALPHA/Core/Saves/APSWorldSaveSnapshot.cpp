#include "APSWorldSaveSnapshot.h"

#include "APS_ALPHA/Generation/APSBodyNames.h"

#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
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
		Model.CloudSettings = Data.CloudSettings.Sanitized();
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
	// A snapshot from before 02.10 has no NameStyle tag: it keeps the legacy names. Newer snapshots overwrite this.
	Model->NameStyle = APSBodyNames::LegacyStyle;
	if (!Save->GeneratedWorldModelData.IsEmpty())
	{
		FMemoryReader Reader(Save->GeneratedWorldModelData, true);
		FObjectAndNameAsStringProxyArchive Archive(Reader, true);
		Archive.ArNoDelta = true;
		Model->Serialize(Archive);
		Archive.Close();
		bRestoredSnapshot = !Reader.IsError();
		Reader.Close();
		if (!bRestoredSnapshot)
		{
			// Rio 06.10 (audit: saves): the legacy summary has no seed (a slot hash stands in) and none of the newer fields,
			// so substituting it for a present snapshot silently loaded another world under the save's name.
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Save] Slot %s: model snapshot (%d bytes, version %d) is unreadable; refusing legacy-summary substitution"),
				*SlotName, Save->GeneratedWorldModelData.Num(), Save->SaveFormatVersion);
			return nullptr;
		}
	}

	// Only a save without a snapshot (before the snapshot existed) falls back to the legacy summary.
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
	// Rio 03.10 (a save audit): every load and save added the home world once more (Mevelex had three for one).
	RemoveDuplicateInhabitedPlanets(Model->InhabitedPlanets);
	Model->GenerationSeed = FMath::Max(Model->GenerationSeed, 1);
	return Model;
}

bool APSWorldSaveSnapshot::IsSameInhabitedPlanet(const FPlanetData& A, const FPlanetData& B)
{
	return A.PlanetOrder == B.PlanetOrder && A.PlanetRadiusKM == B.PlanetRadiusKM
		&& FMath::IsNearlyEqual(A.OrbitRadius, B.OrbitRadius, 1.0e-6);
}

void APSWorldSaveSnapshot::RecordInhabitedPlanet(TArray<FPlanetData>& Planets, const FPlanetData& Planet)
{
	if (FPlanetData* Existing = Planets.FindByPredicate([&Planet](const FPlanetData& Entry)
		{
			return IsSameInhabitedPlanet(Entry, Planet);
		}))
	{
		*Existing = Planet;
		return;
	}
	Planets.Add(Planet);
}

void APSWorldSaveSnapshot::RemoveDuplicateInhabitedPlanets(TArray<FPlanetData>& Planets)
{
	for (int32 Index = Planets.Num() - 1; Index > 0; --Index)
	{
		for (int32 Earlier = 0; Earlier < Index; ++Earlier)
		{
			if (IsSameInhabitedPlanet(Planets[Earlier], Planets[Index]))
			{
				Planets.RemoveAt(Index);
				break;
			}
		}
	}
}

bool APSWorldSaveSnapshot::CaptureSpawnParameters(const USpawnParameters* Parameters,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	if (!IsValid(Parameters))
	{
		return false;
	}

	FMemoryWriter Writer(OutBytes, true);
	FObjectAndNameAsStringProxyArchive Archive(Writer, false);
	Archive.ArNoDelta = true;
	const_cast<USpawnParameters*>(Parameters)->Serialize(Archive);
	Archive.Close();
	Writer.Close();
	return !Writer.IsError() && !OutBytes.IsEmpty();
}

USpawnParameters* APSWorldSaveSnapshot::RestoreSpawnParameters(const UGameSave* Save,
	UObject* Outer)
{
	if (!IsValid(Save) || !IsValid(Outer) || Save->SpawnParametersData.IsEmpty())
	{
		return nullptr;
	}

	USpawnParameters* Parameters = NewObject<USpawnParameters>(Outer);
	if (!IsValid(Parameters))
	{
		return nullptr;
	}

	FMemoryReader Reader(Save->SpawnParametersData, true);
	FObjectAndNameAsStringProxyArchive Archive(Reader, true);
	Archive.ArNoDelta = true;
	Parameters->Serialize(Archive);
	Archive.Close();
	const bool bRestored = !Reader.IsError();
	Reader.Close();
	if (!bRestored)
	{
		return nullptr;
	}
	Parameters->SanitizeForGeneration();
	return Parameters;
}

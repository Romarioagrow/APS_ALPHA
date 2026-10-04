#include "APSWorldBrowserMetadata.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace APSWorldBrowserMetadataPrivate
{
	const TCHAR* const SidecarSection = TEXT("APSWorld");
	constexpr double AstronomicalUnitCm = 1.495978707e13;
	/** The browser draws a dozen orbits at most; a 120-planet edit still records its true count. */
	constexpr int32 MaxRecordedPlanets = 64;

	template <typename T>
	FString EnumLabel(T Value)
	{
		const UEnum* Enum = StaticEnum<T>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString() : TEXT("Unknown");
	}

	FString NameText(const FName Name)
	{
		return Name.IsNone() ? FString() : Name.ToString();
	}

	/** The generated physical radius when set (as the menu's body editor reads it), else the integer one. */
	int32 RadiusKmOf(const APlanet& Planet)
	{
		return FMath::IsFinite(Planet.RadiusKM) && Planet.RadiusKM > 0.0
			? FMath::RoundToInt(Planet.RadiusKM) : Planet.PlanetRadiusKM;
	}

	bool IsSameInhabitedEntry(const FPlanetData& A, const FPlanetData& B)
	{
		return A.PlanetOrder == B.PlanetOrder && A.PlanetRadiusKM == B.PlanetRadiusKM
			&& FMath::IsNearlyEqual(A.OrbitRadius, B.OrbitRadius, 1.0e-6);
	}

	/** Saves before 03.10 gained one more copy of the home world on every load and save (Mevelex: three for one). */
	int32 CountUniqueInhabited(const TArray<FPlanetData>& Entries)
	{
		int32 Count = 0;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			bool bRepeat = false;
			for (int32 Earlier = 0; Earlier < Index && !bRepeat; ++Earlier)
			{
				bRepeat = IsSameInhabitedEntry(Entries[Earlier], Entries[Index]);
			}
			Count += bRepeat ? 0 : 1;
		}
		return Count;
	}

	/** The save's inhabited list holds copies of the home world's FPlanetData: same orbit and radius. */
	bool WasRecordedInhabited(const UGameSave& Save, const APlanet& Planet)
	{
		const double Orbit = Planet.PlanetData.OrbitRadius;
		return Orbit > 0.0 && Save.InhabitedPlanetsDataArray.ContainsByPredicate([&Planet, Orbit](const FPlanetData& Entry)
		{
			return Entry.PlanetRadiusKM == Planet.PlanetRadiusKM && FMath::IsNearlyEqual(Entry.OrbitRadius, Orbit, 1.0e-6);
		});
	}

	/** The home planet of the hierarchy LiveWorld plays: the model's (set when the generator committed the world), else
	 * the live generator's. Preview generators never count. */
	const APlanet* FindHomePlanet(const UWorld* LiveWorld, const UGeneratedWorld* Model)
	{
		if (!LiveWorld)
		{
			return nullptr;
		}
		if (Model && IsValid(Model->HomePlanet) && Model->HomePlanet->GetWorld() == LiveWorld)
		{
			return Model->HomePlanet;
		}
		TArray<AActor*> Generators;
		UGameplayStatics::GetAllActorsOfClass(LiveWorld, AAstroGenerator::StaticClass(), Generators);
		for (AActor* Actor : Generators)
		{
			const AAstroGenerator* Generator = Cast<AAstroGenerator>(Actor);
			if (IsValid(Generator) && !Generator->ActorHasTag(TEXT("WorldGenerationPreview"))
				&& !Generator->UsesContinuousPreviewFrame() && IsValid(Generator->HomePlanet))
			{
				return Generator->HomePlanet;
			}
		}
		return nullptr;
	}

	struct FLiveHomeSystem
	{
		const APlanet* HomePlanet{nullptr};
		FString SystemType;
		int32 TotalPlanets{0};
		FAPSWorldSystemRecord Record;
	};

	/** Reads the home star system as it stands in the world: every star, each star's planets in orbit order. */
	bool DescribeLiveHomeSystem(const UGameSave& Save, const UWorld* LiveWorld, const UGeneratedWorld* Model,
		FLiveHomeSystem& Out)
	{
		const APlanet* HomePlanet = FindHomePlanet(LiveWorld, Model);
		const AStar* HomeStar = HomePlanet && IsValid(HomePlanet->ParentStar) ? HomePlanet->ParentStar : nullptr;
		if (!HomeStar)
		{
			return false;
		}
		TArray<const AStar*> Stars;
		Stars.Add(HomeStar);
		if (const AStarSystem* StarSystem = Cast<AStarSystem>(HomeStar->GetAttachParentActor()))
		{
			for (const AStar* Star : StarSystem->GetStars())
			{
				if (IsValid(Star) && Star != HomeStar)
				{
					Stars.Add(Star);
				}
			}
		}

		Out.HomePlanet = HomePlanet;
		Out.SystemType = IsValid(HomeStar->PlanetarySystem)
			? EnumLabel(HomeStar->PlanetarySystem->PlanetarySystemType) : FString();
		FAPSWorldSystemRecord& Record = Out.Record;
		Record.StarClassLabel = EnumLabel(HomeStar->SpectralClass);
		Record.StellarType = EnumLabel(HomeStar->StellarClass);
		Record.HomeStarName = NameText(HomeStar->AstroName);
		Record.HomePlanetName = NameText(HomePlanet->AstroName);
		Record.StarCount = Stars.Num();
		for (int32 StarIndex = 0; StarIndex < Stars.Num(); ++StarIndex)
		{
			const AStar* Star = Stars[StarIndex];
			TArray<const APlanet*> Members;
			if (IsValid(Star->PlanetarySystem))
			{
				for (const APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
				{
					if (IsValid(Planet))
					{
						Members.AddUnique(Planet);
					}
				}
			}
			if (StarIndex == 0)
			{
				Members.AddUnique(HomePlanet);
			}
			Members.StableSort([](const APlanet& A, const APlanet& B)
			{
				return A.PlanetData.OrbitRadius < B.PlanetData.OrbitRadius;
			});
			for (const APlanet* Planet : Members)
			{
				++Out.TotalPlanets;
				if (Record.Planets.Num() >= MaxRecordedPlanets)
				{
					continue;
				}
				FAPSWorldPlanetRecord& Entry = Record.Planets.AddDefaulted_GetRef();
				Entry.Type = EnumLabel(Planet->PlanetType);
				Entry.OrbitAu = Planet->PlanetData.OrbitRadius > 0.0 ? Planet->PlanetData.OrbitRadius
					: FVector::Dist(Planet->GetActorLocation(), Star->GetActorLocation()) / AstronomicalUnitCm;
				Entry.RadiusKm = RadiusKmOf(*Planet);
				for (const AMoon* Moon : Planet->Moons)
				{
					Entry.Moons += IsValid(Moon) ? 1 : 0;
				}
				Entry.Star = StarIndex;
				// The generator lists the home world as inhabited in every save; only a world that spawned its
				// civilization is inhabited for the browser (Rio 03.10: the INHABITED filter must split the worlds).
				Entry.bInhabited = Save.bHadGeneratedCivilization
					&& (Planet == HomePlanet || WasRecordedInhabited(Save, *Planet));
				if (Planet == HomePlanet)
				{
					Record.HomePlanetIndex = Record.Planets.Num() - 1;
				}
			}
		}
		return true;
	}

	/** "Type,OrbitAu,RadiusKm,Moons,Inhabited,Star;...": display names hold none of the separators. Rio 04.10 ("the world
	 * cards show one planet"): the entries used to be joined with '|', which the engine's ini reader takes as a line
	 * break, so only the first planet came back. ';' is a plain character to it. */
	FString FormatPlanets(const TArray<FAPSWorldPlanetRecord>& Planets)
	{
		TArray<FString> Entries;
		Entries.Reserve(Planets.Num());
		for (const FAPSWorldPlanetRecord& Planet : Planets)
		{
			const FString Type = Planet.Type.Replace(TEXT(","), TEXT(" ")).Replace(TEXT("|"), TEXT(" ")).Replace(TEXT(";"), TEXT(" "));
			Entries.Add(FString::Printf(TEXT("%s,%.6g,%d,%d,%d,%d"), *Type, Planet.OrbitAu, Planet.RadiusKm,
				Planet.Moons, Planet.bInhabited ? 1 : 0, Planet.Star));
		}
		return FString::Join(Entries, TEXT(";"));
	}
}

FString APSWorldBrowserMetadata::SidecarPath(const FString& SlotName)
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (SlotName + TEXT(".apsmeta"));
}

bool APSWorldBrowserMetadata::WriteForSave(const UGameSave* Save, const FGeneratedWorldData& WorldData,
	const UWorld* LiveWorld, const UGeneratedWorld* Model)
{
	using namespace APSWorldBrowserMetadataPrivate;
	if (!Save || Save->SaveSlotName.IsEmpty())
	{
		return false;
	}

	// The model's values are all version 1 had: the menu's editor buffer, not necessarily the world (Rio 03.10: 217 of
	// 227 sidecars said one frozen planet around a G star). The home system edit at least knows the planet count.
	FString SystemType = EnumLabel(WorldData.PlanetarySystemType);
	FString StarLabel = EnumLabel(WorldData.SpectralClass);
	FString HomeType = EnumLabel(WorldData.PlanetType);
	FString Habitability = EnumLabel(WorldData.PlanetHabitability);
	int32 HomeRadiusKm = FMath::RoundToInt(WorldData.PlanetRadius);
	int32 TotalPlanets = WorldData.PlanetsAmount;
	if (const FAPSPreviewSystemEditOverride* HomeSystemEdit = Model ? Model->FindPreviewSystemEditOverride(TEXT("SYS0")) : nullptr;
		HomeSystemEdit && HomeSystemEdit->TotalPlanets != INDEX_NONE)
	{
		TotalPlanets = HomeSystemEdit->TotalPlanets;
	}
	// The generator lists the home world in every save; a world without its civilization has nobody on it.
	int32 InhabitedPlanets = Save->bHadGeneratedCivilization ? CountUniqueInhabited(Save->InhabitedPlanetsDataArray) : 0;

	FLiveHomeSystem Live;
	const bool bLive = DescribeLiveHomeSystem(*Save, LiveWorld, Model, Live);
	if (bLive)
	{
		if (!Live.SystemType.IsEmpty())
		{
			SystemType = Live.SystemType;
		}
		StarLabel = Live.Record.StarClassLabel;
		HomeType = EnumLabel(Live.HomePlanet->PlanetType);
		Habitability = EnumLabel(Live.HomePlanet->PlanetHabitability);
		HomeRadiusKm = RadiusKmOf(*Live.HomePlanet);
		TotalPlanets = Live.TotalPlanets;
		InhabitedPlanets = 0;
		for (const FAPSWorldPlanetRecord& Planet : Live.Record.Planets)
		{
			InhabitedPlanets += Planet.bInhabited ? 1 : 0;
		}
	}

	FConfigFile Metadata;
	Metadata.SetInt64(SidecarSection, TEXT("Version"), bLive ? 2 : 1);
	Metadata.SetString(SidecarSection, TEXT("DisplayName"),
		*(Save->WorldName.IsEmpty() ? Save->SaveSlotName : Save->WorldName));
	Metadata.SetString(SidecarSection, TEXT("SystemType"), *SystemType);
	Metadata.SetString(SidecarSection, TEXT("StarType"), *StarLabel);
	Metadata.SetString(SidecarSection, TEXT("PlanetType"), *HomeType);
	Metadata.SetString(SidecarSection, TEXT("Habitability"), *Habitability);
	Metadata.SetString(SidecarSection, TEXT("Environment"),
		*FString::Printf(TEXT("%s / %d KM"), *HomeType, HomeRadiusKm));
	Metadata.SetInt64(SidecarSection, TEXT("TotalPlanets"), TotalPlanets);
	Metadata.SetInt64(SidecarSection, TEXT("InhabitedPlanets"), InhabitedPlanets);
	if (bLive)
	{
		const FAPSWorldSystemRecord& Record = Live.Record;
		Metadata.SetString(SidecarSection, TEXT("StellarType"), *Record.StellarType);
		Metadata.SetString(SidecarSection, TEXT("HomeStarName"), *Record.HomeStarName);
		Metadata.SetString(SidecarSection, TEXT("HomePlanetName"), *Record.HomePlanetName);
		Metadata.SetInt64(SidecarSection, TEXT("StarCount"), Record.StarCount);
		Metadata.SetInt64(SidecarSection, TEXT("HomePlanetIndex"), Record.HomePlanetIndex);
		Metadata.SetString(SidecarSection, TEXT("Planets"), *FormatPlanets(Record.Planets));
	}

	const FString MetadataPath = SidecarPath(Save->SaveSlotName);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(MetadataPath), true);
	if (!Metadata.Write(MetadataPath, false))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Save] Could not write metadata sidecar: %s"), *MetadataPath);
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Save] Sidecar slot=%s version=%d star=%s planets=%d inhabited=%d stars=%d"),
		*Save->SaveSlotName, bLive ? 2 : 1, *StarLabel, TotalPlanets, InhabitedPlanets,
		bLive ? Live.Record.StarCount : 0);
	return true;
}

bool APSWorldBrowserMetadata::ReadSidecar(const FString& Path, FConfigFile& OutMetadata)
{
	// Rio 04.10: the sidecars written before 04.10 join their planets with '|', where FConfigFile::Read would cut the line;
	// read as text with it turned into ';', so those worlds show all their planets without being saved again.
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		return false;
	}
	Text.ReplaceInline(TEXT("|"), TEXT(";"));
	OutMetadata.ProcessInputFileContents(Text, Path);
	return true;
}

bool APSWorldBrowserMetadata::ReadSystemRecord(const FConfigFile& Metadata, FAPSWorldSystemRecord& OutRecord)
{
	using namespace APSWorldBrowserMetadataPrivate;
	OutRecord = FAPSWorldSystemRecord();
	int64 Version = 0;
	if (!Metadata.GetInt64(SidecarSection, TEXT("Version"), Version) || Version < 2)
	{
		return false;
	}
	Metadata.GetString(SidecarSection, TEXT("StarType"), OutRecord.StarClassLabel);
	Metadata.GetString(SidecarSection, TEXT("StellarType"), OutRecord.StellarType);
	Metadata.GetString(SidecarSection, TEXT("HomeStarName"), OutRecord.HomeStarName);
	Metadata.GetString(SidecarSection, TEXT("HomePlanetName"), OutRecord.HomePlanetName);
	int64 Value = 0;
	if (Metadata.GetInt64(SidecarSection, TEXT("StarCount"), Value))
	{
		OutRecord.StarCount = static_cast<int32>(FMath::Clamp<int64>(Value, 1, 16));
	}
	FString List;
	Metadata.GetString(SidecarSection, TEXT("Planets"), List);
	TArray<FString> Entries;
	const TCHAR* const Separators[] = {TEXT(";"), TEXT("|")};
	List.ParseIntoArray(Entries, Separators, UE_ARRAY_COUNT(Separators), true);
	for (const FString& Entry : Entries)
	{
		TArray<FString> Fields;
		Entry.ParseIntoArray(Fields, TEXT(","), false);
		if (Fields.Num() < 2 || OutRecord.Planets.Num() >= MaxRecordedPlanets)
		{
			continue;
		}
		FAPSWorldPlanetRecord& Planet = OutRecord.Planets.AddDefaulted_GetRef();
		Planet.Type = Fields[0].TrimStartAndEnd();
		Planet.OrbitAu = FMath::Max(0.0, FCString::Atod(*Fields[1]));
		Planet.RadiusKm = Fields.IsValidIndex(2) ? FMath::Max(0, FCString::Atoi(*Fields[2])) : 0;
		Planet.Moons = Fields.IsValidIndex(3) ? FMath::Clamp(FCString::Atoi(*Fields[3]), 0, 999) : 0;
		Planet.bInhabited = Fields.IsValidIndex(4) && FCString::Atoi(*Fields[4]) != 0;
		Planet.Star = Fields.IsValidIndex(5) ? FMath::Clamp(FCString::Atoi(*Fields[5]), 0, 15) : 0;
	}
	if (Metadata.GetInt64(SidecarSection, TEXT("HomePlanetIndex"), Value) && Value >= 0
		&& Value < OutRecord.Planets.Num())
	{
		OutRecord.HomePlanetIndex = static_cast<int32>(Value);
	}
	return true;
}

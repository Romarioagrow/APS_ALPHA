#include "APSAuthoredWorlds.h"

#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/UI/MainMenu/APSWorldRoll.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UObjectIterator.h"

namespace APSAuthoredWorldsLocal
{
	TArray<APSAuthoredWorlds::FCard> GCards;
	bool bGScanned = false;

	/** Model fields never carried by a preset: transient preview actors and the sealed stellar dataset. */
	const TCHAR* const SkippedKeys[] = {
		TEXT("HomePlanetarySystem"), TEXT("HomePlanet"), TEXT("InhabitedPlanets"), TEXT("CanonicalStellarDataset")};

	TSharedPtr<FJsonObject> ReadJson(const FString& Path, FString* OutFailure)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			if (OutFailure) *OutFailure = FString::Printf(TEXT("cannot read %s"), *Path);
			return nullptr;
		}
		TSharedPtr<FJsonObject> Json;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
		{
			if (OutFailure) *OutFailure = FString::Printf(TEXT("malformed JSON in %s"), *Path);
			return nullptr;
		}
		return Json;
	}

	uint8 ParseLocks(const FJsonObject& Json)
	{
		double Number = 0.0;
		if (Json.TryGetNumberField(TEXT("locks"), Number))
		{
			return static_cast<uint8>(FMath::Clamp(static_cast<int32>(Number), 0, static_cast<int32>(APSWorldRules::LockAll)));
		}
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Json.TryGetArrayField(TEXT("locks"), Values) || !Values)
		{
			return 0;
		}
		uint8 Bits = 0;
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString Name;
			if (!Value.IsValid() || !Value->TryGetString(Name)) continue;
			Name = Name.ToLower();
			if (Name == TEXT("all")) Bits |= APSWorldRules::LockAll;
			else if (Name == TEXT("galaxy")) Bits |= APSWorldRules::LockGalaxy;
			else if (Name == TEXT("cluster")) Bits |= APSWorldRules::LockCluster;
			else if (Name == TEXT("system")) Bits |= APSWorldRules::LockHomeSystem;
			else if (Name == TEXT("star")) Bits |= APSWorldRules::LockStar;
			else if (Name == TEXT("body") || Name == TEXT("planet")) Bits |= APSWorldRules::LockBody;
		}
		return Bits;
	}

	uint8 ParseOrigin(const FString& Text)
	{
		const FString Name = Text.ToLower();
		if (Name == TEXT("ark")) return static_cast<uint8>(APSWorldRules::EOrigin::Ark);
		if (Name == TEXT("adrift")) return static_cast<uint8>(APSWorldRules::EOrigin::Adrift);
		if (Name == TEXT("ring") || Name == TEXT("undertherring") || Name == TEXT("under_the_ring")) return static_cast<uint8>(APSWorldRules::EOrigin::UnderTheRing);
		if (Name == TEXT("exodus")) return static_cast<uint8>(APSWorldRules::EOrigin::Exodus);
		return 0;
	}

	bool ParseCard(const FString& Directory, APSAuthoredWorlds::FCard& OutCard, int32& OutOrder)
	{
		const FString CardPath = Directory / TEXT("card.json");
		const TSharedPtr<FJsonObject> Json = ReadJson(CardPath, nullptr);
		if (!Json.IsValid())
		{
			return false;
		}
		OutCard.Directory = Directory;
		OutCard.Id = FPaths::GetCleanFilename(Directory);
		Json->TryGetStringField(TEXT("id"), OutCard.Id);
		Json->TryGetStringField(TEXT("name"), OutCard.Name);
		Json->TryGetStringField(TEXT("subtitle"), OutCard.Subtitle);
		Json->TryGetStringField(TEXT("lore"), OutCard.Lore);
		Json->TryGetStringField(TEXT("place"), OutCard.Place);
		Json->TryGetStringField(TEXT("readiness"), OutCard.Readiness);
		const TArray<TSharedPtr<FJsonValue>>* Tags = nullptr;
		if (Json->TryGetArrayField(TEXT("tags"), Tags) && Tags)
		{
			for (const TSharedPtr<FJsonValue>& Tag : *Tags)
			{
				FString Text;
				if (Tag.IsValid() && Tag->TryGetString(Text)) OutCard.Tags.Add(Text);
			}
		}
		OutCard.Locks = ParseLocks(*Json);
		FString Origin;
		if (Json->TryGetStringField(TEXT("origin"), Origin)) OutCard.RecommendedOrigin = ParseOrigin(Origin);
		FString Route;
		if (Json->TryGetStringField(TEXT("route"), Route)) OutCard.bSpaceRoute = Route.Equals(TEXT("space"), ESearchCase::IgnoreCase);
		FString Spawn;
		if (Json->TryGetStringField(TEXT("spawn"), Spawn))
		{
			const int64 Value = StaticEnum<ECharSpawnPlace>()->GetValueByNameString(Spawn);
			OutCard.SpawnPlace = Value == INDEX_NONE ? INDEX_NONE : static_cast<int32>(Value);
		}
		bool bPlayable = true;
		if (Json->TryGetBoolField(TEXT("playable"), bPlayable)) OutCard.bPlayable = bPlayable;
		double Order = 1000.0;
		Json->TryGetNumberField(TEXT("order"), Order);
		OutOrder = static_cast<int32>(Order);
		OutCard.bHasWorld = FPaths::FileExists(Directory / TEXT("world.json"));
		if (OutCard.Name.IsEmpty()) OutCard.Name = OutCard.Id.ToUpper();
		return true;
	}

	void Scan()
	{
		GCards.Reset();
		bGScanned = true;
		const FString Root = APSAuthoredWorlds::Root();
		TArray<FString> Directories;
		IFileManager::Get().FindFiles(Directories, *(Root / TEXT("*")), false, true);
		TArray<TPair<int32, APSAuthoredWorlds::FCard>> Found;
		for (const FString& Name : Directories)
		{
			APSAuthoredWorlds::FCard Card;
			int32 Order = 1000;
			if (ParseCard(Root / Name, Card, Order))
			{
				Found.Emplace(Order, MoveTemp(Card));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Worlds] %s: no readable card.json, skipped"), *Name);
			}
		}
		Found.Sort([](const TPair<int32, APSAuthoredWorlds::FCard>& A, const TPair<int32, APSAuthoredWorlds::FCard>& B)
		{
			return A.Key != B.Key ? A.Key < B.Key : A.Value.Id < B.Value.Id;
		});
		for (TPair<int32, APSAuthoredWorlds::FCard>& Pair : Found)
		{
			GCards.Add(MoveTemp(Pair.Value));
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Worlds] %d authored worlds under %s"), GCards.Num(), *Root);
	}

	/** The menu's editable model: not a class default, not the committed copy owned by the gameplay instance. */
	UGeneratedWorld* FindMenuModel()
	{
		UGeneratedWorld* Found = nullptr;
		for (TObjectIterator<UGeneratedWorld> It; It; ++It)
		{
			if (It->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || !IsValid(*It)) continue;
			if (It->GetTypedOuter<UMainGameplayInstance>()) continue;
			Found = *It;
		}
		return Found;
	}

	FAutoConsoleCommand GListCommand(TEXT("aps.Worlds.List"),
		TEXT("Lists the authored worlds (Content/APS/APS_ALPHA/Worlds/Authored) with their locks and readiness."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			for (const APSAuthoredWorlds::FCard& Card : APSAuthoredWorlds::Catalogue(true))
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Worlds] %-16s locks=%2d world=%d route=%s spawn=%d  %s — %s"), *Card.Id, Card.Locks,
					Card.bHasWorld ? 1 : 0, Card.bSpaceRoute ? TEXT("space") : TEXT("civ"), Card.SpawnPlace, *Card.Name, *Card.Readiness);
			}
		}));

	FAutoConsoleCommand GExportCommand(TEXT("aps.Worlds.ExportAuthored"),
		TEXT("aps.Worlds.ExportAuthored <id>: writes the menu's world model to Content/APS/APS_ALPHA/Worlds/Authored/<id>/world.json."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (Args.Num() < 1 || Args[0].IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Worlds] usage: aps.Worlds.ExportAuthored <id>"));
				return;
			}
			UGeneratedWorld* Model = FindMenuModel();
			if (!Model)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Worlds] no menu world model to export (open NEW WORLD first)"));
				return;
			}
			FString Path;
			if (APSAuthoredWorlds::Export(*Model, Args[0], Path))
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Worlds] exported %s (model %s) -> %s"), *Args[0], *Model->GetPathName(), *Path);
				APSAuthoredWorlds::Catalogue(true);
			}
		}));
}

namespace APSAuthoredWorlds
{
	FString Root()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("APS/APS_ALPHA/Worlds/Authored"));
	}

	const TArray<FCard>& Catalogue(const bool bRescan)
	{
		if (bRescan || !APSAuthoredWorldsLocal::bGScanned)
		{
			APSAuthoredWorldsLocal::Scan();
		}
		return APSAuthoredWorldsLocal::GCards;
	}

	const FCard* Find(const FString& Id)
	{
		return Catalogue().FindByPredicate([&Id](const FCard& Card) { return Card.Id.Equals(Id, ESearchCase::IgnoreCase); });
	}

	bool Apply(const FCard& Card, UGeneratedWorld& World, USpawnParameters* Spawn, FString* OutFailure)
	{
		const TSharedPtr<FJsonObject> Json = APSAuthoredWorldsLocal::ReadJson(Card.Directory / TEXT("world.json"), OutFailure);
		if (!Json.IsValid())
		{
			return false;
		}
		for (const TCHAR* Key : APSAuthoredWorldsLocal::SkippedKeys)
		{
			Json->RemoveField(Key);
		}
		FText Reason;
		if (!FJsonObjectConverter::JsonObjectToUStruct(Json.ToSharedRef(), UGeneratedWorld::StaticClass(), &World, 0, 0, false, &Reason))
		{
			if (OutFailure) *OutFailure = FString::Printf(TEXT("%s: %s"), *Card.Id, *Reason.ToString());
			return false;
		}
		World.AuthoredWorldId = Card.Id;
		World.AuthoredLocks = Card.Locks;
		if (Spawn && Card.SpawnPlace != INDEX_NONE)
		{
			Spawn->CharacterSpawnPlace = static_cast<ECharSpawnPlace>(Card.SpawnPlace);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Worlds] applied '%s': seed=%d locks=%d spawn=%d %s"), *Card.Id, World.GenerationSeed, Card.Locks,
			Card.SpawnPlace, *APSWorldRules::Describe(APSWorldRules::Of(&World)));
		return true;
	}

	bool Export(const UGeneratedWorld& World, const FString& Id, FString& OutPath)
	{
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		if (!FJsonObjectConverter::UStructToJsonObject(UGeneratedWorld::StaticClass(), &World, Json, 0, 0))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Worlds] export %s: the model did not convert"), *Id);
			return false;
		}
		for (const TCHAR* Key : APSAuthoredWorldsLocal::SkippedKeys)
		{
			Json->RemoveField(Key);
		}
		// The id and locks are the card's; a preset never pins itself to another preset.
		Json->RemoveField(TEXT("AuthoredWorldId"));
		Json->RemoveField(TEXT("AuthoredLocks"));
		FString Text;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
		if (!FJsonSerializer::Serialize(Json, Writer))
		{
			return false;
		}
		const FString Directory = Root() / Id;
		IFileManager::Get().MakeDirectory(*Directory, true);
		OutPath = Directory / TEXT("world.json");
		if (!FFileHelper::SaveStringToFile(Text, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			return false;
		}
		const FString CardPath = Directory / TEXT("card.json");
		if (!FPaths::FileExists(CardPath))
		{
			const FString Card = FString::Printf(TEXT("{\n  \"id\": \"%s\",\n  \"name\": \"%s\",\n  \"subtitle\": \"\",\n  \"lore\": \"\",\n")
				TEXT("  \"place\": \"\",\n  \"tags\": [],\n  \"locks\": [\"all\"],\n  \"origin\": \"ark\",\n  \"route\": \"civilization\",\n")
				TEXT("  \"spawn\": \"PlanetSurface\",\n  \"playable\": true,\n  \"readiness\": \"exported\",\n  \"order\": 500\n}\n"),
				*Id, *Id.ToUpper());
			FFileHelper::SaveStringToFile(Card, *CardPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}
		return true;
	}

	uint8 LockBitsOf(const APSWorldRoll::EScope Scope)
	{
		switch (Scope)
		{
		case APSWorldRoll::EScope::GalaxyOnly: return APSWorldRules::LockGalaxy;
		case APSWorldRoll::EScope::ClusterOnly: return APSWorldRules::LockCluster;
		case APSWorldRoll::EScope::StarOnly: return APSWorldRules::LockStar;
		case APSWorldRoll::EScope::BodyOnly:
		case APSWorldRoll::EScope::PlanetOnly: return APSWorldRules::LockBody;
		case APSWorldRoll::EScope::HomeSystemOnly:
		case APSWorldRoll::EScope::System:
		default: return APSWorldRules::LockHomeSystem | APSWorldRules::LockStar | APSWorldRules::LockBody;
		}
	}

	bool IsScopeLocked(const UGeneratedWorld& World, const APSWorldRoll::EScope Scope)
	{
		return !World.AuthoredWorldId.IsEmpty() && (World.AuthoredLocks & LockBitsOf(Scope)) != 0;
	}
}

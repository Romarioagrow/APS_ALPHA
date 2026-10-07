#include "APSCivilizationSave.h"

#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Quests/APSQuestSubsystem.h"
#include "APS_ALPHA/Pawns/Vehicles/PilotingVehicle.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace APSCivilizationSavePrivate
{
	constexpr uint32 Magic = 0x53564943; // "CIVS"
	/**
	 * 2: the ship the player was piloting, to seat the pilot in it again. 3: the quests (the onboarding). 4: the fleet's
	 * structures and slipways. 5: the expansion (star systems, infrastructure and stocks, missions, builders' types).
	 */
	constexpr int32 Version = 5;

	/** Rio 06.10 (audit: saves). A seated quit loaded as a character in space with no ship: the lifecycle autosave runs
	 * after UnPossess, so the piloted ship's key was always empty, and a seated pilot was taken back to the ship's berth. */
	TAutoConsoleVariable<int32> CVarRestorePilotedShip(
		TEXT("aps.Save.RestorePilotedShip"), 1,
		TEXT("1: a ship piloted at save time comes back with its pilot: the key of the ship last piloted is saved even ")
		TEXT("after UnPossess (the lifecycle autosave), and on load a ship more than 50 m from the saved pilot is moved so ")
		TEXT("its seat is where the pilot was saved. 0: as before (the key only while possessed, the pilot taken to the ship)."));

	/** Beyond this the ship stands somewhere else than where the pilot was saved (it was at its berth at load). */
	constexpr double SavedSeatMoveThresholdCm = 5000.0;

	/**
	 * The pilot was just snapped to the seat (PawnAfter, the seat's world transform) from where it was saved (PawnBefore).
	 * UE composes left to right (A * B applies A, then B): the seat's world transform is SeatInShip * Ship, so the rigid
	 * world move Move = PawnAfter^-1 * PawnBefore takes the seat onto PawnBefore (PawnAfter * Move == PawnBefore), and the
	 * ship takes the same move: Ship * Move. Move has unit scale, so the product is exact even for a scaled ship.
	 */
	void PlaceVehicleAtSavedSeat(APilotingVehicle& Vehicle, FTransform PawnBefore, FTransform PawnAfter)
	{
		PawnBefore.SetScale3D(FVector::OneVector);
		PawnAfter.SetScale3D(FVector::OneVector);
		if (FVector::Dist(PawnBefore.GetLocation(), PawnAfter.GetLocation()) <= SavedSeatMoveThresholdCm)
		{
			return;
		}
		const FTransform Stood = Vehicle.GetActorTransform();
		const FTransform Move = PawnAfter.Inverse() * PawnBefore;
		const FTransform Target = Stood * Move;
		// Boarding already detached it (APilotingVehicle::BeginVehicleControl); a berth must not carry it back.
		if (Vehicle.GetAttachParentActor())
		{
			Vehicle.DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		static const FName SurfaceParkedTag(TEXT("APS.Civilization.SurfaceParked"));
		Vehicle.Tags.Remove(SurfaceParkedTag);
		Vehicle.SetActorTransform(Target, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("[APS.Save] %s placed at the saved seat (%.1f km from where it stood)"),
			*Vehicle.GetName(), FVector::Dist(Stood.GetLocation(), Target.GetLocation()) / 100000.0);
	}

	UAPSQuestSubsystem* QuestOf(const UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UAPSQuestSubsystem>() : nullptr;
	}

	struct FModuleRecord
	{
		FString ModuleId;
		FGuid StableId;
		uint8 Site{0};
		FTransform RelativeToAnchor{FTransform::Identity};
		double FoundationDepthCm{0.0};
		double BoomLengthCm{0.0};

		friend FArchive& operator<<(FArchive& Ar, FModuleRecord& Record)
		{
			return Ar << Record.ModuleId << Record.StableId << Record.Site << Record.RelativeToAnchor
				<< Record.FoundationDepthCm << Record.BoomLengthCm;
		}
	};

	/** Modules of a load that wait for their site (the colony materializes after the actor overlay). */
	struct FPendingColony
	{
		TWeakObjectPtr<UWorld> World;
		TArray<FModuleRecord> Modules;
		double StartSeconds{0.0};
	};
	TArray<FPendingColony> PendingColonies;
	FTSTicker::FDelegateHandle PendingTicker;

	bool TickPendingColonies(float)
	{
		const double Now = FPlatformTime::Seconds();
		PendingColonies.RemoveAll([Now](FPendingColony& Pending)
		{
			UWorld* World = Pending.World.Get();
			UAPSColonyConstructionSubsystem* Construction = World ? World->GetSubsystem<UAPSColonyConstructionSubsystem>() : nullptr;
			if (!Construction || Now - Pending.StartSeconds > 600.0)
			{
				if (!Pending.Modules.IsEmpty())
				{
					UE_LOG(LogTemp, Warning, TEXT("[APS.Save] %d saved colony module(s) not restored: no site in time"),
						Pending.Modules.Num());
				}
				return true;
			}
			Pending.Modules.RemoveAll([Construction](const FModuleRecord& Record)
			{
				return Construction->RestoreModule(FName(*Record.ModuleId), Record.StableId,
					static_cast<EAPSSpawnSite>(Record.Site), Record.RelativeToAnchor, Record.FoundationDepthCm,
					Record.BoomLengthCm) != nullptr;
			});
			return Pending.Modules.IsEmpty();
		});
		if (PendingColonies.IsEmpty())
		{
			PendingTicker.Reset();
			return false;
		}
		return true;
	}

	/** A game saved in the pilot's seat: the pilot sits down in that ship again once both are back. */
	struct FPendingSeat
	{
		TWeakObjectPtr<UWorld> World;
		FString VehicleKey;
		double StartSeconds{0.0};
	};
	TArray<FPendingSeat> PendingSeats;
	FTSTicker::FDelegateHandle SeatTicker;

	bool TickPendingSeats(float)
	{
		const double Now = FPlatformTime::Seconds();
		PendingSeats.RemoveAll([Now](const FPendingSeat& Pending)
		{
			UWorld* World = Pending.World.Get();
			APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = Player ? Player->GetPawn() : nullptr;
			if (!World || Now - Pending.StartSeconds > 30.0)
			{
				// Not seated in time: the pilot must not stay inside the hull it was saved in.
				if (World && IsValid(Pawn) && !Pawn->IsA<APilotingVehicle>())
				{
					FVector Location = Pawn->GetActorLocation();
					if (World->FindTeleportSpot(Pawn, Location, Pawn->GetActorRotation()))
					{
						Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
					}
				}
				UE_LOG(LogTemp, Warning, TEXT("[APS.Save] the pilot was not seated again: %s not back in time"),
					*Pending.VehicleKey);
				return true;
			}
			if (!IsValid(Pawn) || Pawn->IsA<APilotingVehicle>())
			{
				return false;
			}
			for (TActorIterator<APilotingVehicle> It(World); It; ++It)
			{
				if (!IsValid(*It) || It->HasPilot() || FAPSFleetCommand::KeyOf(*It) != Pending.VehicleKey)
				{
					continue;
				}
				// Rio 06.10 (audit: saves, aps.Save.RestorePilotedShip): where the pilot was saved, before the seat snaps it.
				const FTransform PawnBefore = Pawn->GetActorTransform();
				if (!It->RequestVehicleControl(Pawn))
				{
					continue;
				}
				UE_LOG(LogTemp, Log, TEXT("[APS.Save] the pilot sits in %s again"), *It->GetName());
				if (CVarRestorePilotedShip.GetValueOnGameThread() != 0)
				{
					PlaceVehicleAtSavedSeat(**It, PawnBefore, Pawn->GetActorTransform());
				}
				return true;
			}
			return false;
		});
		if (PendingSeats.IsEmpty())
		{
			SeatTicker.Reset();
			return false;
		}
		return true;
	}
}

bool APSCivilizationSave::RestorePilotedShipEnabled()
{
	return APSCivilizationSavePrivate::CVarRestorePilotedShip.GetValueOnGameThread() != 0;
}

void APSCivilizationSave::Capture(UWorld* World, TArray<uint8>& OutBytes, const FString& FallbackPilotedVehicleKey)
{
	using namespace APSCivilizationSavePrivate;
	OutBytes.Reset();
	if (!World)
	{
		return;
	}
	FMemoryWriter Ar(OutBytes, true);
	uint32 Header = Magic;
	int32 SavedVersion = Version;
	Ar << Header << SavedVersion;

	TArray<FModuleRecord> Modules;
	if (const UAPSColonyConstructionSubsystem* Construction = World->GetSubsystem<UAPSColonyConstructionSubsystem>())
	{
		for (const EAPSSpawnSite Site : {EAPSSpawnSite::Surface, EAPSSpawnSite::Orbit})
		{
			const AActor* Anchor = Construction->GetSiteAnchor(Site);
			TArray<AAPSColonyModule*> Built;
			Construction->GetBuiltModules(Site, Built);
			for (const AAPSColonyModule* Module : Built)
			{
				if (!IsValid(Module) || !IsValid(Anchor))
				{
					continue;
				}
				FModuleRecord& Record = Modules.AddDefaulted_GetRef();
				Record.ModuleId = Module->GetModuleId().ToString();
				Record.StableId = Module->GetStableId();
				Record.Site = static_cast<uint8>(Site);
				Record.RelativeToAnchor = Module->GetActorTransform().GetRelativeTransform(Anchor->GetActorTransform());
				Record.FoundationDepthCm = Module->GetFoundationDepthCm();
				Record.BoomLengthCm = Module->GetBoomLengthCm();
			}
		}
	}
	Ar << Modules;

	FAPSFleetSaveData Fleet;
	if (const FAPSFleetCommand* Command = APSFleetFind(World))
	{
		Command->CaptureSave(Fleet);
	}
	Ar << Fleet;

	int32 EntryCount = 0;
	const UAPSCivilizationJournalSubsystem* Journal = World->GetSubsystem<UAPSCivilizationJournalSubsystem>();
	if (Journal)
	{
		EntryCount = Journal->GetEntries().Num();
	}
	Ar << EntryCount;
	for (int32 Index = 0; Index < EntryCount; ++Index)
	{
		const FAPSCivilizationJournalEntry& Entry = Journal->GetEntries()[Index];
		FString Category = Entry.Category.ToString();
		FString Text = Entry.Text.ToString();
		double Seconds = Entry.WorldSeconds;
		Ar << Category << Text << Seconds;
	}

	// Seated in a ship: the controller saves the pilot as the player's pawn, this says which ship to sit in again.
	FString PilotedVehicleKey;
	if (const APlayerController* Player = World->GetFirstPlayerController())
	{
		if (const APilotingVehicle* Vehicle = Cast<APilotingVehicle>(Player->GetPawn()); Vehicle && Vehicle->HasPilot())
		{
			PilotedVehicleKey = FAPSFleetCommand::KeyOf(Vehicle);
		}
	}
	// Rio 06.10 (audit: saves, aps.Save.RestorePilotedShip): after UnPossess (the lifecycle autosave) the controller's
	// cached key says which ship the pilot sat in.
	if (PilotedVehicleKey.IsEmpty() && !FallbackPilotedVehicleKey.IsEmpty() && RestorePilotedShipEnabled())
	{
		PilotedVehicleKey = FallbackPilotedVehicleKey;
	}
	Ar << PilotedVehicleKey;

	// The quests' progress (the onboarding restarted from its first step after every load, audit A02).
	TArray<uint8> QuestBytes;
	int32 QuestCount = 0;
	if (UAPSQuestSubsystem* Quest = QuestOf(World))
	{
		FAPSQuestSaveData QuestData = Quest->ExportQuestSaveData();
		QuestCount = QuestData.Instances.Num();
		FMemoryWriter QuestWriter(QuestBytes, true);
		FObjectAndNameAsStringProxyArchive QuestArchive(QuestWriter, false);
		FAPSQuestSaveData::StaticStruct()->SerializeItem(QuestArchive, &QuestData, nullptr);
	}
	Ar << QuestBytes;
	// Version 4: the stations, shipyards and HQs the fleet built and the slipways' queues (Rio, 01.10).
	FAPSFleetSaveData::SerializeExtras(Ar, Fleet);
	// Version 5: the expansion (Rio, 02.10), as one blob with versions of its own inside.
	TArray<uint8> ExpansionBytes;
	{
		FMemoryWriter Writer(ExpansionBytes, true);
		FAPSStarSystemsSaveData Stars;
		if (const FAPSStarSystems* Systems = APSStarSystemsFind(World)) Systems->CaptureSave(Stars);
		FAPSInfrastructureSaveData Infrastructure;
		if (const FAPSInfrastructure* Runtime = APSInfrastructureFind(World)) Runtime->CaptureSave(Infrastructure);
		FAPSMissionSaveData Missions;
		if (const FAPSMissionBoard* Board = APSMissionsFind(World)) Board->CaptureSave(Missions);
		TArray<FString> Builders, BuilderTypes;
		for (const TPair<FString, FString>& Building : Fleet.UnitStructureTypes)
		{
			Builders.Add(Building.Key);
			BuilderTypes.Add(Building.Value);
		}
		Writer << Stars << Infrastructure << Missions << Builders << BuilderTypes;
	}
	Ar << ExpansionBytes;
	UE_LOG(LogTemp, Log, TEXT("[APS.Save] civilization state: %d modules, %d units, %d surveys, %d outposts, %d structures, %d slipway jobs, %d journal entries, %d quests, %d bytes%s%s"),
		Modules.Num(), Fleet.Units.Num(), Fleet.Surveys.Num(), Fleet.Outposts.Num(), Fleet.Structures.Num(),
		Fleet.ShipyardJobs.Num(), EntryCount, QuestCount, OutBytes.Num(),
		PilotedVehicleKey.IsEmpty() ? TEXT("") : TEXT(", piloting "), *PilotedVehicleKey);
}

void APSCivilizationSave::Restore(UWorld* World, const TArray<uint8>& Bytes)
{
	using namespace APSCivilizationSavePrivate;
	if (!World || Bytes.Num() < 8)
	{
		return;
	}
	FMemoryReader Ar(Bytes, true);
	uint32 Header = 0;
	int32 SavedVersion = 0;
	Ar << Header << SavedVersion;
	if (Header != Magic || SavedVersion < 1 || SavedVersion > Version)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Save] civilization state skipped: header %08x version %d"), Header, SavedVersion);
		return;
	}
	TArray<FModuleRecord> Modules;
	FAPSFleetSaveData Fleet;
	int32 EntryCount = 0;
	Ar << Modules;
	Ar << Fleet;
	Ar << EntryCount;
	TArray<FAPSCivilizationJournalEntry> Entries;
	for (int32 Index = 0; Index < EntryCount && !Ar.IsError(); ++Index)
	{
		FString Category;
		FString Text;
		double Seconds = 0.0;
		Ar << Category << Text << Seconds;
		FAPSCivilizationJournalEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Category = FName(*Category);
		Entry.Text = FText::FromString(Text);
		Entry.WorldSeconds = Seconds;
	}
	FString PilotedVehicleKey;
	if (SavedVersion >= 2)
	{
		Ar << PilotedVehicleKey;
	}
	TArray<uint8> QuestBytes;
	if (SavedVersion >= 3)
	{
		Ar << QuestBytes;
	}
	if (SavedVersion >= 4)
	{
		FAPSFleetSaveData::SerializeExtras(Ar, Fleet);
	}
	TArray<uint8> ExpansionBytes;
	if (SavedVersion >= 5)
	{
		Ar << ExpansionBytes;
	}
	if (Ar.IsError())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Save] civilization state is damaged; nothing restored from it"));
		return;
	}
	if (UAPSCivilizationJournalSubsystem* Journal = World->GetSubsystem<UAPSCivilizationJournalSubsystem>())
	{
		Journal->RestoreEntries(MoveTemp(Entries));
	}
	if (!ExpansionBytes.IsEmpty())
	{
		FMemoryReader Reader(ExpansionBytes, true);
		FAPSStarSystemsSaveData Stars;
		FAPSInfrastructureSaveData Infrastructure;
		FAPSMissionSaveData Missions;
		TArray<FString> Builders, BuilderTypes;
		Reader << Stars << Infrastructure << Missions << Builders << BuilderTypes;
		if (Reader.IsError())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Save] the expansion block is damaged; star systems, infrastructure and missions start over"));
		}
		else
		{
			if (FAPSStarSystems* Systems = APSStarSystemsFind(World)) Systems->RestoreSave(MoveTemp(Stars));
			if (FAPSInfrastructure* Runtime = APSInfrastructureFind(World)) Runtime->RestoreSave(MoveTemp(Infrastructure));
			if (FAPSMissionBoard* Board = APSMissionsFind(World)) Board->RestoreSave(MoveTemp(Missions));
			for (int32 Index = 0; Index < Builders.Num() && Index < BuilderTypes.Num(); ++Index)
			{
				Fleet.UnitStructureTypes.Emplace(Builders[Index], BuilderTypes[Index]);
			}
		}
	}
	const int32 UnitCount = Fleet.Units.Num();
	if (FAPSFleetCommand* Command = APSFleetFind(World))
	{
		Command->SetPendingRestore(MoveTemp(Fleet));
	}
	if (!Modules.IsEmpty())
	{
		FPendingColony& Pending = PendingColonies.AddDefaulted_GetRef();
		Pending.World = World;
		Pending.Modules = MoveTemp(Modules);
		Pending.StartSeconds = FPlatformTime::Seconds();
		if (!PendingTicker.IsValid())
		{
			PendingTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickPendingColonies), 0.5f);
		}
	}
	// Before the world's quest adapter starts the onboarding again for this manifest: it then finds the saved instance
	// and continues it (a matching start is a no-op).
	if (!QuestBytes.IsEmpty())
	{
		FAPSQuestSaveData QuestData;
		FMemoryReader QuestReader(QuestBytes, true);
		FObjectAndNameAsStringProxyArchive QuestArchive(QuestReader, true);
		FAPSQuestSaveData::StaticStruct()->SerializeItem(QuestArchive, &QuestData, nullptr);
		FString Reason;
		UAPSQuestSubsystem* Quest = QuestOf(World);
		if (QuestReader.IsError() || !Quest || !Quest->RestoreQuestSaveData(QuestData, Reason))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Save] quests not restored: %s"),
				Reason.IsEmpty() ? TEXT("unreadable") : *Reason);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Save] quests restored: %d"), QuestData.Instances.Num());
		}
	}
	if (!PilotedVehicleKey.IsEmpty())
	{
		FPendingSeat& Seat = PendingSeats.AddDefaulted_GetRef();
		Seat.World = World;
		Seat.VehicleKey = PilotedVehicleKey;
		Seat.StartSeconds = FPlatformTime::Seconds();
		if (!SeatTicker.IsValid())
		{
			SeatTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickPendingSeats), 0.25f);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Save] civilization state restoring: %d modules, %d units, %d journal entries%s%s"),
		PendingColonies.IsEmpty() ? 0 : PendingColonies.Last().Modules.Num(), UnitCount, EntryCount,
		PilotedVehicleKey.IsEmpty() ? TEXT("") : TEXT(", pilot back to "), *PilotedVehicleKey);
}

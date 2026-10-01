#include "APSCivilizationSave.h"

#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Vehicles/PilotingVehicle.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace APSCivilizationSavePrivate
{
	constexpr uint32 Magic = 0x53564943; // "CIVS"
	/** 2: the ship the player was piloting, to seat the pilot in it again. */
	constexpr int32 Version = 2;

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
				if (IsValid(*It) && !It->HasPilot() && FAPSFleetCommand::KeyOf(*It) == Pending.VehicleKey
					&& It->RequestVehicleControl(Pawn))
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Save] the pilot sits in %s again"), *It->GetName());
					return true;
				}
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

void APSCivilizationSave::Capture(UWorld* World, TArray<uint8>& OutBytes)
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
	Ar << PilotedVehicleKey;
	UE_LOG(LogTemp, Log, TEXT("[APS.Save] civilization state: %d modules, %d units, %d surveys, %d outposts, %d journal entries, %d bytes%s%s"),
		Modules.Num(), Fleet.Units.Num(), Fleet.Surveys.Num(), Fleet.Outposts.Num(), EntryCount, OutBytes.Num(),
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
	if (Ar.IsError())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Save] civilization state is damaged; nothing restored from it"));
		return;
	}
	if (UAPSCivilizationJournalSubsystem* Journal = World->GetSubsystem<UAPSCivilizationJournalSubsystem>())
	{
		Journal->RestoreEntries(MoveTemp(Entries));
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

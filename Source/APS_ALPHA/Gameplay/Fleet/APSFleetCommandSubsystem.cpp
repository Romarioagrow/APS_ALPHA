#include "APSFleetCommandSubsystem.h"

#include "APSFleetCommand.h"
#include "APS_ALPHA/Core/Diagnostics/APSMemoryProbe.h"
#include "Engine/World.h"

bool UAPSFleetCommandSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAPSFleetCommandSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Fleet = MakeShared<FAPSFleetCommand>(GetWorld());
	APSFleetRegister(GetWorld(), Fleet.Get());
	// Memory at each world's start and end (and once a minute in between): play sessions show where commit grows.
	APSMemoryProbe::Log(GetWorld(), TEXT("world start"));
}

void UAPSFleetCommandSubsystem::Deinitialize()
{
	APSMemoryProbe::Log(GetWorld(), TEXT("world end"));
	APSFleetRegister(GetWorld(), nullptr);
	Fleet.Reset();
	Super::Deinitialize();
}

void UAPSFleetCommandSubsystem::Tick(const float DeltaTime)
{
	if (Fleet.IsValid())
	{
		Fleet->Tick(DeltaTime);
	}
	APSMemoryProbe::Tick(GetWorld());
}

TStatId UAPSFleetCommandSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSFleetCommandSubsystem, STATGROUP_Tickables);
}

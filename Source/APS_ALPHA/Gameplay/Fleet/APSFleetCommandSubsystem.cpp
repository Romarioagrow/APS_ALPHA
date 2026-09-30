#include "APSFleetCommandSubsystem.h"

#include "APSFleetCommand.h"
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
}

void UAPSFleetCommandSubsystem::Deinitialize()
{
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
}

TStatId UAPSFleetCommandSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSFleetCommandSubsystem, STATGROUP_Tickables);
}

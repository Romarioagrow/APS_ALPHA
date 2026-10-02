#include "APSFleetCommandSubsystem.h"

#include "APSFleetCommand.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Core/Diagnostics/APSMemoryProbe.h"
#include "APS_ALPHA/Core/Diagnostics/APSPerfProbe.h"
#include "APS_ALPHA/Core/Diagnostics/APSSurfaceDiag.h"
#include "APS_ALPHA/UI/Colony/APSMissionTracker.h"
#include "APS_ALPHA/Gameplay/Vehicles/APSGroundVehicles.h"
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
	Stars = MakeShared<FAPSStarSystems>(GetWorld());
	APSStarSystemsRegister(GetWorld(), Stars.Get());
	Infrastructure = MakeShared<FAPSInfrastructure>(GetWorld());
	APSInfrastructureRegister(GetWorld(), Infrastructure.Get());
	Missions = MakeShared<FAPSMissionBoard>(GetWorld());
	APSMissionsRegister(GetWorld(), Missions.Get());
	// Memory at each world's start and end (and once a minute in between): play sessions show where commit grows.
	APSMemoryProbe::Log(GetWorld(), TEXT("world start"));
}

void UAPSFleetCommandSubsystem::Deinitialize()
{
	APSMemoryProbe::Log(GetWorld(), TEXT("world end"));
	APSMissionTracker::Remove(GetWorld());
	APSMissionsRegister(GetWorld(), nullptr);
	APSInfrastructureRegister(GetWorld(), nullptr);
	APSStarSystemsRegister(GetWorld(), nullptr);
	APSFleetRegister(GetWorld(), nullptr);
	Missions.Reset();
	Infrastructure.Reset();
	Stars.Reset();
	Fleet.Reset();
	Super::Deinitialize();
}

void UAPSFleetCommandSubsystem::Tick(const float DeltaTime)
{
	if (Stars.IsValid())
	{
		Stars->Tick(DeltaTime);
	}
	if (Infrastructure.IsValid())
	{
		Infrastructure->Tick(DeltaTime);
	}
	if (Missions.IsValid())
	{
		Missions->Tick(DeltaTime);
	}
	if (Fleet.IsValid())
	{
		Fleet->Tick(DeltaTime);
	}
	APSMemoryProbe::Tick(GetWorld());
	APSPerfProbe::Tick(GetWorld(), DeltaTime);
	APSGroundVehicles::Tick(GetWorld(), DeltaTime);
	APSSurfaceDiag::Tick(GetWorld(), DeltaTime);
	APSMissionTracker::Tick(GetWorld());
}

TStatId UAPSFleetCommandSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSFleetCommandSubsystem, STATGROUP_Tickables);
}

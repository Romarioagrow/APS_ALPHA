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
	// Rio 05.10 afternoon (flight FPS): this tick grows from 0.03 to 0.45 ms a frame at drive speed; each part is traced.
	if (Stars.IsValid())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_Stars);
		Stars->Tick(DeltaTime);
	}
	if (Infrastructure.IsValid())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_Infrastructure);
		Infrastructure->Tick(DeltaTime);
	}
	if (Missions.IsValid())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_Missions);
		Missions->Tick(DeltaTime);
	}
	if (Fleet.IsValid())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_Fleet);
		Fleet->Tick(DeltaTime);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_Probes);
		APSMemoryProbe::Tick(GetWorld());
		APSPerfProbe::Tick(GetWorld(), DeltaTime);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_GroundVehicles);
		APSGroundVehicles::Tick(GetWorld(), DeltaTime);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Fleet_SurfaceDiagAndTracker);
		APSSurfaceDiag::Tick(GetWorld(), DeltaTime);
		APSMissionTracker::Tick(GetWorld());
	}
}

TStatId UAPSFleetCommandSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSFleetCommandSubsystem, STATGROUP_Tickables);
}

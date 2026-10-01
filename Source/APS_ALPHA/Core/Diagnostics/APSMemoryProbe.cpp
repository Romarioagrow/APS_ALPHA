#include "APSMemoryProbe.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "UObject/UObjectArray.h"

namespace APSMemoryProbePrivate
{
	TAutoConsoleVariable<float> CVarSeconds(TEXT("aps.Mem.LogSeconds"), 60.0f,
		TEXT("Seconds between [APS.Mem] lines in a game world (process commit and working set, system commit left, ")
		TEXT("UObjects, actors, WorldScape roots); 0 turns them off."));

	double NextSeconds = 0.0;

	double GiB(const uint64 Bytes)
	{
		return static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0);
	}
}

void APSMemoryProbe::Log(UWorld* World, const TCHAR* Reason)
{
	using namespace APSMemoryProbePrivate;
	if (!World)
	{
		return;
	}
	const FPlatformMemoryStats Stats = FPlatformMemory::GetStats();
	int32 Actors = 0;
	int32 Roots = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		++Actors;
		Roots += It->GetClass()->GetName().Contains(TEXT("WorldScapeRoot")) ? 1 : 0;
	}
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Mem] %s commit %.2f GiB (peak %.2f), resident %.2f GiB (peak %.2f) | system commit left %.2f GiB, ")
		TEXT("physical free %.2f GiB | UObjects %d, actors %d, WorldScape roots %d | t=%.0f s"),
		Reason, GiB(Stats.UsedVirtual), GiB(Stats.PeakUsedVirtual), GiB(Stats.UsedPhysical), GiB(Stats.PeakUsedPhysical),
		GiB(Stats.AvailableVirtual), GiB(Stats.AvailablePhysical), GUObjectArray.GetObjectArrayNumMinusAvailable(), Actors,
		Roots, World->GetTimeSeconds());
}

void APSMemoryProbe::Tick(UWorld* World)
{
	using namespace APSMemoryProbePrivate;
	const float Interval = CVarSeconds.GetValueOnGameThread();
	const double Now = FPlatformTime::Seconds();
	if (Interval <= 0.0f || Now < NextSeconds)
	{
		return;
	}
	NextSeconds = Now + Interval;
	Log(World, TEXT("periodic"));
}

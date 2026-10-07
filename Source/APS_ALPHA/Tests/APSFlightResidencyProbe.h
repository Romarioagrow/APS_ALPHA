#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "WorldScapeRoot.h"
#include "EngineUtils.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"

namespace APSFlightResidencyProbe
{
    inline bool Requested() { return FParse::Param(FCommandLine::Get(),TEXT("APSProbeFlightResidency")); }
    inline bool Performance() { return Requested() && FParse::Param(FCommandLine::Get(),TEXT("APSProbeFlightResidencyPerf")); }
    inline bool Disabled() { return FParse::Param(FCommandLine::Get(),TEXT("APSProbeFlightResidencyOff")); }
    class FProbe
    {
        struct FFrame { double T, Dt, Game, Render, GPU; bool Transit; };
        TArray<FFrame> Frames;
        double NextSnapshot=0;
    public:
        void Tick(AWorldScapeRoot* Root, double T)
        {
            Frames.Add({T,FApp::GetDeltaTime()*1000.,FPlatformTime::ToMilliseconds(GGameThreadTime),
                FPlatformTime::ToMilliseconds(GRenderThreadTime),FPlatformTime::ToMilliseconds(GGPUFrameTime),
                Root->ActorHasTag(TEXT("APS.Surface.Transit"))});
            if(T<NextSnapshot) return;
            NextSnapshot=T+1.;
            int32 Roots=0, RegisteredMeshes=0, Workers=0;
            uint64 Vertices=0, PublishedBytes=0;
            // Published GT sections only. Never inspect the worker-owned Lod arrays.
            for(TActorIterator<AWorldScapeRoot> It(Root->GetWorld());It;++It)
            {
                auto* R=*It;
                if(R->GetOwner()!=Root->GetOwner()) continue;
                ++Roots; Workers+=R->WorldScapeLodInGeneration.Num();
                for(const auto* Lods:{&R->WorldScapeLod,&R->WorldScapeLodOcean,&R->CollisionLods})
                for(const auto* Lod:*Lods)
                {
                    auto* Mesh=IsValid(Lod)?Lod->Mesh:nullptr;
                    if(!IsValid(Mesh) || !Mesh->IsRegistered())continue;
                    ++RegisteredMeshes;
                    for(int32 S=0;S<Mesh->GetNumSections();++S)
                        if(const auto* Section=Mesh->GetProcMeshSection(S))
                        {
                            Vertices+=Section->PlanetVertexBuffer.Num();
                            PublishedBytes+=Section->PlanetVertexBuffer.GetAllocatedSize()+Section->PlanetIndexBuffer.GetAllocatedSize();
                        }
                }
            }
            UE_LOG(LogTemp,Display,TEXT("[APS.FlightResidency.Memory] t=%.3f roots=%d registeredMeshes=%d vertices=%llu publishedBytes=%llu workers=%d processRAMMiB=%.1f transit=%d collision=%d collisionLods=%d anchors=%d"),
                T,Roots,RegisteredMeshes,Vertices,PublishedBytes,Workers,FPlatformMemory::GetStats().UsedPhysical/1048576.,Root->ActorHasTag(TEXT("APS.Surface.Transit")),Root->bGenerateCollision,Root->CollisionLods.Num(),Root->CollisionDependantActor.Num());
        }
        bool Finish()
        {
            FString CSV=TEXT("t,dt_ms,game_ms,render_ms,gpu_ms,transit\n");
            for(const auto& F:Frames) CSV+=FString::Printf(TEXT("%.6f,%.6f,%.6f,%.6f,%.6f,%d\n"),F.T,F.Dt,F.Game,F.Render,F.GPU,F.Transit);
            const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Diagnostics"));
            IFileManager::Get().MakeDirectory(*Directory,true);
            const bool Saved=FFileHelper::SaveStringToFile(CSV,*(Directory/TEXT("FlightResidency.csv")));
            // Outside the measurement. Native DestroyComponent releases the old GPU
            // proxy in render order; do not flush on every runtime transition.
            FlushRenderingCommands();
            return Saved && Frames.Num()>100;
        }
    };
}
#endif

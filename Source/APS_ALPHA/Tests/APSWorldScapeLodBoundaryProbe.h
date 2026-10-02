#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "WorldScapeRoot.h"

namespace APSWorldScapeLodBoundaryProbe
{
    inline bool Requested(){return FParse::Param(FCommandLine::Get(),TEXT("APSProbeFlightLodBoundary"));}
    // Live route: worker staging arrays are not legal to inspect until the
    // complete batch has drained. Check the GT-published sections instead.
    // This proves section presence and sampled integrity, not every vertex.
    inline bool HasPublishedSections(const UWorldScapeLod* Lod)
    {
        if(!IsValid(Lod) || !IsValid(Lod->Mesh) || Lod->Mesh->GetNumSections()!=3)return false;
        for(int32 Index=0;Index<3;++Index)
        {
            const auto* Section=Lod->Mesh->GetProcMeshSection(Index);
            if(!Section || Section->PlanetVertexBuffer.IsEmpty() || Section->PlanetIndexBuffer.Num()<3)return false;
            for(int32 I:{0,Section->PlanetVertexBuffer.Num()/2,Section->PlanetVertexBuffer.Num()-1})
            {
                const auto& V=Section->PlanetVertexBuffer[I];
                if(V.Position.ContainsNaN() || V.Normal.ContainsNaN())return false;
            }
            for(int32 I:{0,Section->PlanetIndexBuffer.Num()/2,Section->PlanetIndexBuffer.Num()-1})
                if(!Section->PlanetVertexBuffer.IsValidIndex(Section->PlanetIndexBuffer[I]))return false;
        }
        return true;
    }
    inline double HeightKm(double Elapsed)
    {
        // Hold2s, descend8s, hold2s, ascend8s, hold2s. Cross only16<->32.
        double T=Elapsed<=10 ? FMath::Clamp((Elapsed-2.)/8.,0.,1.)
            : Elapsed<=12 ? 1. : 1.-FMath::Clamp((Elapsed-12.)/8.,0.,1.);
        T=T*T*(3.-2.*T);
        return 2.36-.21*T;
    }
    // Only game-thread published mesh data; never inspect worker staging arrays,
    // LodSize, RelativePosition or MaterialLod while workers can mutate them.
    inline bool PublishedSpacing(AWorldScapeRoot* Root,double& StepCm,uint32& Signature)
    {
        if(!Root || Root->WorldScapeLod.IsEmpty())return false;
        const auto* Lod=Root->WorldScapeLod[0];
        if(!IsValid(Lod) || !IsValid(Lod->Mesh))return false;
        const auto* S=Lod->Mesh->GetProcMeshSection(0);
        if(!S || S->PlanetIndexBuffer.Num()<96)return false;
        const FTransform Transform=Lod->Mesh->GetComponentTransform();
        const FVector Center=Transform.InverseTransformPosition(Root->GetActorLocation());
        TArray<double> Edges;Signature=GetTypeHash(Transform.GetLocation());
        const int32 Limit=FMath::Min(384,S->PlanetIndexBuffer.Num());
        for(int32 I=0;I+2<Limit;I+=3)
            for(int32 E=0;E<3;++E)
            {
                const uint32 A=S->PlanetIndexBuffer[I+E],B=S->PlanetIndexBuffer[I+(E+1)%3];
                if(!S->PlanetVertexBuffer.IsValidIndex(A) || !S->PlanetVertexBuffer.IsValidIndex(B))return false;
                const auto& VA=S->PlanetVertexBuffer[A];const auto& VB=S->PlanetVertexBuffer[B];
                const FVector NA=(VA.Position-Center).GetSafeNormal(),NB=(VB.Position-Center).GetSafeNormal();
                const double Length=(NA-NB).Size()*Root->PlanetScaleCode;
                if(!FMath::IsFinite(Length) || NA.IsNearlyZero() || NB.IsNearlyZero())return false;
                if(Length>1.e-3)Edges.Add(Length);
                Signature=HashCombine(Signature,HashCombine(GetTypeHash(VA.Position),GetTypeHash(VA.Normal)));
            }
        if(Edges.IsEmpty())return false;
        Edges.Sort();StepCm=Edges[Edges.Num()/2];return true;
    }
}
#endif

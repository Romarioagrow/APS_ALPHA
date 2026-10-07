#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "WorldScapeMeshComponent.h"

// A causal, normal-only probe. Reconstruct the SAME tessellation on a perfect
// sphere and remove only its curvature error. Do not replace terrain slopes
// by radial normals, resample height, move vertices or modify indices/tangents.
namespace APSMeshCurvatureProbe
{
    inline bool Build(const FWorldScapeMeshSection& Section, const FVector& LocalCenter,
        bool bUnitFaceWeights, TArray<FVector>& Out, double& MaxDegrees, double& MeanDegrees)
    {
        const int32 Count=Section.PlanetVertexBuffer.Num();
        if(!Count || Section.PlanetIndexBuffer.Num()%3)return false;
        TArray<FVector> Radial, Reference;
        Radial.Reserve(Count);Reference.SetNumZeroed(Count);Out.Reset(Count);
        for(const auto& V:Section.PlanetVertexBuffer)
        {
            const FVector N=(V.Position-LocalCenter).GetSafeNormal();
            if(N.ContainsNaN() || N.IsNearlyZero())return false;
            Radial.Add(N);
        }
        // Large common radius avoids tiny cross products; normalization removes it.
        constexpr double Radius=1.e8;
        for(int32 I=0;I<Section.PlanetIndexBuffer.Num();I+=3)
        {
            const uint32 A=Section.PlanetIndexBuffer[I],B=Section.PlanetIndexBuffer[I+1],C=Section.PlanetIndexBuffer[I+2];
            if(A>=uint32(Count) || B>=uint32(Count) || C>=uint32(Count))return false;
            FVector N=FVector::CrossProduct((Radial[C]-Radial[A])*Radius,(Radial[B]-Radial[A])*Radius);
            if(bUnitFaceWeights)N=N.GetSafeNormal();
            Reference[A]+=N;Reference[B]+=N;Reference[C]+=N;
        }
        MaxDegrees=0;MeanDegrees=0;int32 Corrected=0;
        for(int32 I=0;I<Count;++I)
        {
            const FVector Ref=Reference[I].GetSafeNormal();
            const FVector Native=Section.PlanetVertexBuffer[I].Normal;
            // Isolated/degenerate vertices keep their exact source normal.
            if(Ref.IsNearlyZero()){Out.Add(Native);continue;}
            if(Ref.ContainsNaN() || FVector::DotProduct(Ref,Radial[I])<=0)return false;
            const double Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Ref,Radial[I]),-1.,1.)));
            MaxDegrees=FMath::Max(MaxDegrees,Angle);MeanDegrees+=Angle;++Corrected;
            const FVector N=FQuat::FindBetweenNormals(Ref,Radial[I]).RotateVector(Native);
            if(N.ContainsNaN())return false;
            Out.Add(N);
        }
        if(Corrected)MeanDegrees/=Corrected;
        return Out.Num()==Count;
    }
}
#endif

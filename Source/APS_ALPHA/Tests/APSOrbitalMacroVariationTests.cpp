#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Planetary/APSOrbitalMacroVariation.h"
#include "APS_ALPHA/Core/Planetary/APSSharedTerrainMaterial.h"
#include "Misc/ScopeExit.h"
#include "APSMeshCurvatureProbe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSMeshCurvatureProbeTest,
    "APS.Contracts.PlanetSurface.MeshCurvatureProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSMeshCurvatureProbeTest::RunTest(const FString& Parameters)
{
    FWorldScapeMeshSection S;
    const FVector Center(10000,-20000,30000);
    for(const FVector V:{FVector(1,0,0),FVector(1,0,.1),FVector(1,.1,0)})
    {auto& Vertex=S.PlanetVertexBuffer.AddDefaulted_GetRef();Vertex.Position=Center+V.GetSafeNormal()*1.e6;}
    S.PlanetIndexBuffer={0,1,2};
    const FVector Ref=FVector::CrossProduct(S.PlanetVertexBuffer[2].Position-S.PlanetVertexBuffer[0].Position,
        S.PlanetVertexBuffer[1].Position-S.PlanetVertexBuffer[0].Position).GetSafeNormal();
    for(auto& V:S.PlanetVertexBuffer)V.Normal=Ref;
    for(bool Unit:{false,true})
    {
        TArray<FVector> Out;double Max=0,Mean=0;
        if(!TestTrue(TEXT("Valid outward sphere"),APSMeshCurvatureProbe::Build(S,Center,Unit,Out,Max,Mean)))return false;
        for(int32 I=0;I<Out.Num();++I)
            TestTrue(TEXT("Pure sphere becomes radial, not faceted"),Out[I].Equals((S.PlanetVertexBuffer[I].Position-Center).GetSafeNormal(),1.e-9));
        const FVector Sloped=FQuat(FVector::CrossProduct(Ref,FVector::UpVector).GetSafeNormal(),.2).RotateVector(Ref);
        for(auto& V:S.PlanetVertexBuffer)V.Normal=Sloped;
        if(!TestTrue(TEXT("Sloped surface"),APSMeshCurvatureProbe::Build(S,Center,Unit,Out,Max,Mean)))return false;
        for(int32 I=0;I<Out.Num();++I)
            TestTrue(TEXT("Relief angle preserved"),FMath::Abs(FVector::DotProduct(Out[I],(S.PlanetVertexBuffer[I].Position-Center).GetSafeNormal())-FVector::DotProduct(Sloped,Ref))<1.e-9);
        for(auto& V:S.PlanetVertexBuffer)V.Normal=Ref;
    }
    S.PlanetIndexBuffer.Add(99);
    TArray<FVector> Out;double Max=0,Mean=0;
    TestFalse(TEXT("Invalid index buffer refused"),APSMeshCurvatureProbe::Build(S,Center,false,Out,Max,Mean));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSOrbitalMacroFamilyGateTest,
    "APS.Contracts.PlanetSurface.OrbitalMacroFamilyGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSOrbitalMacroFamilyGateTest::RunTest(const FString& Parameters)
{
    for (uint8 Value = 0; Value <= APSPlanetTypes::LastValue; ++Value)
    {
        const auto Type = static_cast<EPlanetType>(Value);
        const bool Expected = Type == EPlanetType::Terrestrial || Type == EPlanetType::Oasis;
        TestEqual(FString::Printf(TEXT("Only rendered families opt in: %u"),Value),
            APSOrbitalMacroVariation::Allows(Type),Expected);
    }
    TestFalse(TEXT("Unknown future family keeps the native material"),
        APSOrbitalMacroVariation::Allows(static_cast<EPlanetType>(255)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSTerrainContinuityGateTest,
    "APS.Contracts.PlanetSurface.TerrainContinuityGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSTerrainContinuityGateTest::RunTest(const FString& Parameters)
{
    auto* Switch=IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Surface.TerrainContinuity"));
    if(!TestNotNull(TEXT("Registered compatibility marker"),Switch))return false;
    TestTrue(TEXT("Interactive rollback is forbidden"), Switch->TestFlags(ECVF_ReadOnly));
    const int32 Previous=Switch->GetInt();
    const uint32 PreviousPriority=Switch->GetFlags() & ECVF_SetByMask;
    ON_SCOPE_EXIT{
        Switch->Set(Previous,ECVF_SetByConsole);
        Switch->ClearFlags(ECVF_SetByMask);
        Switch->SetFlags(static_cast<EConsoleVariableFlags>(PreviousPriority));
    };
    for(int32 Enable:{0,1})
    {
        // Direct C++ writes simulate an old config/launcher. Even a stale zero
        // cannot change routing; restore its value/priority before leaving.
        Switch->Set(Enable,ECVF_SetByConsole);
        TestEqual(TEXT("Test switch actually changed"),Switch->GetInt(),Enable);
        for(int32 Value=0;Value<=255;++Value)
        {
            FAPSResolvedPlanetSurfaceProfile P;
            P.PlanetType=static_cast<EPlanetType>(Value);
            P.Archetype=UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(P.PlanetType);
            // Independent saved-ID boundary: 3 magmatic, 3 giants and Unknown
            // stay outside ContinuousTerra; legacy Exoplanet=29 is resolvable.
            const bool Expected=Value<=34 && Value!=3 && Value!=4 && Value!=5 && Value!=6
                && Value!=12 && Value!=19 && Value!=30;
            TestTrue(TEXT("Stale CVar cannot disable the original terrain"), APSTerrainContinuityMaterial::Enabled());
            TestEqual(FString::Printf(TEXT("Original parent despite stale setting%d type%d"),Enable,Value),
                FString(APSSharedTerrainMaterial::TemplatePath(P)),
                FString(Expected?APSTerrainContinuityMaterial::TemplatePath:APSSharedTerrainMaterial::TemplatePath(P.Archetype)));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSTerrainContinuityAssetTest,
    "APS.Contracts.PlanetSurface.TerrainContinuityAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSTerrainContinuityAssetTest::RunTest(const FString& Parameters)
{
    auto* Template=LoadObject<UMaterialInstance>(nullptr,APSTerrainContinuityMaterial::TemplatePath);
    auto* Original=LoadObject<UMaterialInstance>(nullptr,APSSharedTerrainMaterial::TemplatePath(EAPSPlanetSurfaceArchetype::Rocky));
    if(!TestNotNull(TEXT("Saved release template"),Template) || !TestNotNull(TEXT("Protected original"),Original))return false;
    TestTrue(TEXT("Recognized exact release stack"),APSSharedTerrainMaterial::IsSharedStack(Template));
    if(!TestNotNull(TEXT("Release master"),Template->GetMaterial()))return false;
    TestEqual(TEXT("Correct saved hierarchy"),Template->GetMaterial()->GetPathName(),FString(APSTerrainContinuityMaterial::MasterPath));
    auto* Root=NewObject<USceneComponent>();
    auto* New=UMaterialInstanceDynamic::Create(Template,Root);
    auto* Old=UMaterialInstanceDynamic::Create(Original,Root);
    if(!TestNotNull(TEXT("Release MID"),New) || !TestNotNull(TEXT("Original MID"),Old))return false;
    for(EPlanetType Type:{EPlanetType::Terrestrial,EPlanetType::Frozen})
    {
        FAPSResolvedPlanetSurfaceProfile P;
        P.PlanetType=Type;P.Archetype=UAPSPlanetSurfaceProfileResolver::GetArchetypeForType(Type);
        P.LiquidType=Type==EPlanetType::Terrestrial?EAPSPlanetLiquidType::Water:EAPSPlanetLiquidType::None;
        APSNativeTerrainMaterial::ApplyPalette(New,P);APSNativeTerrainMaterial::ApplyPalette(Old,P);
        TArray<FMaterialParameterInfo> Infos;TArray<FGuid> Ids;
        Old->GetAllVectorParameterInfo(Infos,Ids);
        for(const auto& Info:Infos)
        {
            FLinearColor A,B;
            TestTrue(*Info.Name.ToString(),Old->GetVectorParameterValue(Info,A) && New->GetVectorParameterValue(Info,B) && A==B);
        }
        Infos.Reset();Ids.Reset();Old->GetAllScalarParameterInfo(Infos,Ids);
        for(const auto& Info:Infos)
        {
            if(Info.Name==TEXT("APS_OrbitalMacroMode"))continue;
            float A=0,B=0;
            TestTrue(*Info.Name.ToString(),Old->GetScalarParameterValue(Info,A) && New->GetScalarParameterValue(Info,B) && A==B);
        }
        Infos.Reset();Ids.Reset();Old->GetAllTextureParameterInfo(Infos,Ids);
        for(const auto& Info:Infos)
        {
            UTexture* A=nullptr;UTexture* B=nullptr;
            TestTrue(*Info.Name.ToString(),Old->GetTextureParameterValue(Info,A) && New->GetTextureParameterValue(Info,B) && A==B);
        }
        float ColorMode=-1,NormalMode=-1;
        TestTrue(TEXT("Original colour field is retained at every distance"),
            New->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_OrbitalMacroMode")),ColorMode) && ColorMode==0.0f);
        TestTrue(TEXT("Accepted normal-hex detail remains enabled"),
            New->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_NormalMacroWarpMode")),NormalMode) && NormalMode==1.0f);
    }
    Root->SetWorldLocation(FVector(3.e13,-2.e12,9.e11));
    Root->SetWorldRotation(FRotator(17,31,-5));
    for(double Scale:{1.0,1.e-9})
    {
        TestTrue(TEXT("Release physical binding"),APSSharedTerrainMaterial::WriteFrame(New,Root,Scale));
        TestTrue(TEXT("Original physical binding"),APSSharedTerrainMaterial::WriteFrame(Old,Root,Scale));
        for(const TCHAR* Name:{TEXT("APS_SharedPlanetCenter"),TEXT("APS_SharedInverseScale"),TEXT("APS_SharedAxisX"),TEXT("APS_SharedAxisY"),TEXT("APS_SharedAxisZ")})
        {
            FVector4 A,B;TestTrue(Name,New->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name),A)
                && Old->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name),B) && A==B);
        }
    }
    AddInfo(TEXT("Asset/palette/frame contracts only; rendered gameplay/menu acceptance is separate."));
    return true;
}
#endif

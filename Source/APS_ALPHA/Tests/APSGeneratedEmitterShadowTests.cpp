#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSGeneratedEmitterShadowTest,
    "APS.Gameplay.Generation.EmissivePhotosphereShadowPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGeneratedEmitterShadowTest::RunTest(const FString& Parameters)
{
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false)
        .RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,false,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("World"),World)) return false;
    AStar* Star=World->SpawnActor<AStar>();
    UStarGenerator* Generator=NewObject<UStarGenerator>();
    if (!Star || !Star->StarMesh || !Generator)
    { AddError(TEXT("Fixture allocation failed")); World->DestroyWorld(false); return false; }
    auto Model=MakeShared<FStarModel>();
    Model->StellarType=EStellarType::MainSequence;
    Model->SpectralClass=ESpectralClass::G;
    Model->SurfaceTemperature=5778; Model->Luminosity=1; Model->Radius=1; Model->Mass=1;
    Generator->ApplySpectralMaterial(Star,Model);
    TestFalse(TEXT("Generated emitter is not an occluder"),bool(Star->StarMesh->CastShadow));
    auto* OriginalMID=Star->StarDynamicMaterial;
    const FTransform Transform=Star->StarMesh->GetComponentTransform();
    const auto Mesh=Star->StarMesh->GetStaticMesh();
    const auto* Corona=Star->CoronaMesh->GetMaterial(0);
    const float LightIntensity=Star->StellarLight->Intensity;
    const float Emission=OriginalMID ? OriginalMID->K2_GetScalarParameterValue(TEXT("Multiplier")) : 0;
    Star->StarMesh->SetCastShadow(true); // Simulate a retained Blueprint override.
    Generator->ApplySpectralMaterial(Star,Model);
    TestFalse(TEXT("Replay repairs stale caster flag"),bool(Star->StarMesh->CastShadow));
    TestTrue(TEXT("Material instance retained"),OriginalMID && Star->StarDynamicMaterial==OriginalMID);
    TestTrue(TEXT("Mesh and transform retained"),Star->StarMesh->GetStaticMesh()==Mesh
        && Star->StarMesh->GetComponentTransform().Equals(Transform));
    TestTrue(TEXT("Corona retained"),Star->CoronaMesh->GetMaterial(0)==Corona);
    TestEqual(TEXT("Emitter light power unchanged"),Star->StellarLight->Intensity,LightIntensity);
    if (OriginalMID) TestEqual(TEXT("Surface emission unchanged"),OriginalMID->K2_GetScalarParameterValue(TEXT("Multiplier")),Emission);
    Model->StellarType=EStellarType::BlackHole;
    Star->StarMesh->SetCastShadow(true);
    Generator->ApplySpectralMaterial(Star,Model);
    TestTrue(TEXT("Non-emitting black-hole caster policy retained"),bool(Star->StarMesh->CastShadow));
    World->DestroyWorld(false);
    return true;
}
#endif

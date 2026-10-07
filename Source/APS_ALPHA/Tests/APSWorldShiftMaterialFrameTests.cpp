#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/World/APSWorldShiftEvents.h"
#include "APS_ALPHA/Core/Planetary/APSCoastalWaterMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedAmmoniaMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace APSWorldShiftMaterialFrameTests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;
        FTestWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
                false, ERHIFeatureLevel::Num, &Values);
        }
        ~FTestWorld() { if (World) World->DestroyWorld(false); }
        FTestWorld(const FTestWorld&) = delete;
        FTestWorld& operator=(const FTestWorld&) = delete;
    };

    // Remove only this test's subscriptions before its captured state/world dies.
    struct FBindings
    {
        USceneComponent* Frame;
        UMaterialInstanceDynamic* Material;
        FDelegateHandle CounterHandle;
        int32 TransformUpdates = 0;
        FBindings(USceneComponent* InFrame, UMaterialInstanceDynamic* InMaterial)
            : Frame(InFrame), Material(InMaterial) {}
        ~FBindings()
        {
            Frame->TransformUpdated.Remove(CounterHandle);
            Frame->TransformUpdated.RemoveAll(Material);
            FCoreDelegates::PostWorldOriginOffset.RemoveAll(Material);
            APSWorldShiftEvents::OnPostDoubleShift().RemoveAll(Material);
        }
        void CountTransforms()
        {
            CounterHandle = Frame->TransformUpdated.AddLambda(
                [this](USceneComponent*, EUpdateTransformFlags, ETeleportType) { ++TransformUpdates; });
        }
    };

    template<typename TValue> struct TParameter
    {
        FMaterialParameterInfo Info;
        TValue Value;
    };

    struct FUniformSnapshot
    {
        TArray<TParameter<float>> Scalars;
        TArray<TParameter<FLinearColor>> Vectors;
        TArray<TParameter<FVector4>> DoubleVectors;
        TArray<TParameter<UTexture*>> Textures;
        UMaterialInterface* Parent = nullptr;

        bool Capture(UMaterialInstanceDynamic* M, FName CenterName)
        {
            Parent = M->Parent.Get();
            TArray<FMaterialParameterInfo> Infos;
            TArray<FGuid> Ids;
            bool bValid = true;
            M->GetAllScalarParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                float Value = 0;
                bValid &= M->GetScalarParameterValue(Info, Value);
                Scalars.Add({Info, Value});
            }
            Infos.Reset(); Ids.Reset(); M->GetAllVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                FLinearColor Value = FLinearColor::Black;
                bValid &= M->GetVectorParameterValue(Info, Value);
                Vectors.Add({Info, Value});
            }
            Infos.Reset(); Ids.Reset(); M->GetAllDoubleVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                if (Info.Name == CenterName) continue;
                FVector4 Value(0, 0, 0, 0);
                bValid &= M->GetDoubleVectorParameterValue(Info, Value);
                DoubleVectors.Add({Info, Value});
            }
            Infos.Reset(); Ids.Reset(); M->GetAllTextureParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                UTexture* Value = nullptr;
                bValid &= M->GetTextureParameterValue(Info, Value);
                Textures.Add({Info, Value});
            }
            return bValid;
        }

        void Check(FAutomationTestBase& Test, UMaterialInstanceDynamic* M, const FString& Label) const
        {
            Test.TestTrue(Label + TEXT(" parent unchanged"), M->Parent.Get() == Parent);
            for (const auto& P : Scalars)
            {
                float Value = 0;
                Test.TestTrue(Label + TEXT(" scalar ") + P.Info.Name.ToString(),
                    M->GetScalarParameterValue(P.Info, Value) && Value == P.Value);
            }
            for (const auto& P : Vectors)
            {
                FLinearColor Value;
                Test.TestTrue(Label + TEXT(" palette/detail vector ") + P.Info.Name.ToString(),
                    M->GetVectorParameterValue(P.Info, Value) && Value == P.Value);
            }
            for (const auto& P : DoubleVectors)
            {
                FVector4 Value;
                Test.TestTrue(Label + TEXT(" double vector ") + P.Info.Name.ToString(),
                    M->GetDoubleVectorParameterValue(P.Info, Value) && Value == P.Value);
            }
            for (const auto& P : Textures)
            {
                UTexture* Value = nullptr;
                Test.TestTrue(Label + TEXT(" texture ") + P.Info.Name.ToString(),
                    M->GetTextureParameterValue(P.Info, Value) && Value == P.Value);
            }
        }
    };

    enum class EBinding { Native, Shared, AmmoniaWater, Lava, Coastal };
    struct FFixture { const TCHAR* Name; const TCHAR* Path; EBinding Binding; };

    bool Bind(const FFixture& Fixture, UMaterialInstanceDynamic* M, USceneComponent* Frame, double Scale)
    {
        switch (Fixture.Binding)
        {
        case EBinding::Native:
            APSNativeTerrainMaterial::BindNewInstanceCenter(M, Frame);
            return Frame->TransformUpdated.IsBoundToObject(M);
        case EBinding::Shared: return APSSharedTerrainMaterial::BindNewInstanceFrame(M, Frame, Scale);
        case EBinding::AmmoniaWater: return APSSharedAmmoniaMaterial::BindFrame(M, Frame, Scale);
        case EBinding::Lava: return APSSharedLavaMaterial::BindFrame(M, Frame, Scale);
        case EBinding::Coastal: return APSCoastalWaterMaterial::BindFrame(M, Frame, Scale);
        }
        return false;
    }

    bool HasCenter(UMaterialInstanceDynamic* M, FName Name, const FVector& Center)
    {
        FVector4 Value;
        return M->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Name), Value)
            && Value.Equals(FVector4(Center.X, Center.Y, Center.Z, 0.0), 1.0e-6);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldShiftMaterialFramesTest,
    "APS.World.Origin.MaterialFrames.DoubleShift",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldShiftMaterialFramesTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldShiftMaterialFrameTests;
    FTestWorld Owned, Unrelated;
    if (!TestNotNull(TEXT("Owned transient world"), Owned.World)
        || !TestNotNull(TEXT("Unrelated transient world"), Unrelated.World)) return false;
    const FFixture Fixtures[] = {
        {TEXT("NativeTerra"), TEXT("/Game/Ressources/Materials/WorldScapeMaterials/MaterialInstances/MI_Terra.MI_Terra"), EBinding::Native},
        {TEXT("SharedTerra"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Shared/MI_APS_SharedTerra.MI_APS_SharedTerra"), EBinding::Shared},
        {TEXT("ContinuousTerra"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/ContinuityV1/MI_APS_ContinuousTerra.MI_APS_ContinuousTerra"), EBinding::Shared},
        {TEXT("UnifiedLava"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/UnifiedLava/MI_APS_UnifiedLavaSurface.MI_APS_UnifiedLavaSurface"), EBinding::Shared},
        {TEXT("Ammonia"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedAmmonia.MI_APS_SharedAmmonia"), EBinding::AmmoniaWater},
        {TEXT("Water"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedWater.MI_APS_SharedWater"), EBinding::AmmoniaWater},
        {TEXT("LavaLiquid"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/SharedLiquid/MI_APS_SharedLava.MI_APS_SharedLava"), EBinding::Lava},
        {TEXT("CoastalWater"), TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/WaterV1/MI_APS_CoastalWater.MI_APS_CoastalWater"), EBinding::Coastal}
    };
    const FVector Initial(100000000.125, -200000000.25, 300000000.5);
    // Fractional centimetres and a component beyond int32: no FIntVector truncation.
    const FVector Offset(-3500000000.125, 2750000000.5, -1250000000.25);
    constexpr double PresentationScale = .25;
    for (const FFixture& Fixture : Fixtures)
    {
        const FString Label(Fixture.Name);
        TStrongObjectPtr<UMaterialInstance> Parent(LoadObject<UMaterialInstance>(nullptr, Fixture.Path));
        if (!TestNotNull(Label + TEXT(" saved parent"), Parent.Get())) continue;
        const bool bWasDirty = Parent->GetOutermost()->IsDirty();
        TStrongObjectPtr<UMaterialInstanceDynamic> M(UMaterialInstanceDynamic::Create(Parent.Get(), GetTransientPackage()));
        AStaticMeshActor* Actor = Owned.World->SpawnActor<AStaticMeshActor>(Initial, FRotator(17, 31, -9));
        if (!TestTrue(Label + TEXT(" transient MID and actor"), M.IsValid() && IsValid(Actor))) continue;
        USceneComponent* Frame = Actor->GetRootComponent();
        Frame->SetMobility(EComponentMobility::Movable);
        Frame->SetWorldScale3D(FVector(2.0));
        FBindings Bindings(Frame, M.Get());
        if (!TestTrue(Label + TEXT(" exact adapter bound"), Bind(Fixture, M.Get(), Frame, PresentationScale))) continue;
        const FName CenterName(Fixture.Binding == EBinding::Native ? TEXT("PlanetPosistion") : TEXT("APS_SharedPlanetCenter"));
        TestTrue(Label + TEXT(" initial center"), HasCenter(M.Get(), CenterName, Initial));
        TestTrue(Label + TEXT(" legacy event subscribed"), FCoreDelegates::PostWorldOriginOffset.IsBoundToObject(M.Get()));
        TestTrue(Label + TEXT(" double event subscribed"), APSWorldShiftEvents::OnPostDoubleShift().IsBoundToObject(M.Get()));
        FUniformSnapshot Before;
        TestTrue(Label + TEXT(" all baseline uniforms readable"), Before.Capture(M.Get(), CenterName));
        if (Fixture.Binding != EBinding::Native)
        {
            FVector4 InverseScale;
            TestTrue(Label + TEXT(" component and presentation scale composed"),
                M->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(TEXT("APS_SharedInverseScale")), InverseScale)
                && InverseScale == FVector4(2.0, 0.0, 0.0, 0.0));
        }
        // The public delegates expose no invocation count. Equal allocation size
        // plus per-object membership checks catch extra weak subscriptions here.
        const SIZE_T TransformBytes = Frame->TransformUpdated.GetAllocatedSize();
        const SIZE_T LegacyBytes = FCoreDelegates::PostWorldOriginOffset.GetAllocatedSize();
        const SIZE_T DoubleBytes = APSWorldShiftEvents::OnPostDoubleShift().GetAllocatedSize();
        for (int32 Repeat = 0; Repeat < 3; ++Repeat)
            TestTrue(Label + TEXT(" repeat bind accepted"), Bind(Fixture, M.Get(), Frame, PresentationScale));
        TestTrue(Label + TEXT(" transform binding idempotent"), TransformBytes == Frame->TransformUpdated.GetAllocatedSize());
        TestTrue(Label + TEXT(" legacy binding idempotent"), LegacyBytes == FCoreDelegates::PostWorldOriginOffset.GetAllocatedSize());
        TestTrue(Label + TEXT(" double binding idempotent"), DoubleBytes == APSWorldShiftEvents::OnPostDoubleShift().GetAllocatedSize());
        Before.Check(*this, M.Get(), Label + TEXT(" after rebind"));
        Bindings.CountTransforms();
        Frame->ApplyWorldOffset(Offset, true);
        TestEqual(Label + TEXT(" ApplyWorldOffset bypasses TransformUpdated"), Bindings.TransformUpdates, 0);
        TestTrue(Label + TEXT(" root physically shifted"), Frame->GetComponentLocation().Equals(Initial + Offset, 1.0e-6));
        TestTrue(Label + TEXT(" center stale until post-shift barrier"), HasCenter(M.Get(), CenterName, Initial));
        APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Unrelated.World, Offset);
        TestTrue(Label + TEXT(" unrelated world cannot refresh this MID"), HasCenter(M.Get(), CenterName, Initial));
        APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Owned.World, Offset);
        TestTrue(Label + TEXT(" post-shift center reads final root"), HasCenter(M.Get(), CenterName, Frame->GetComponentLocation()));
        Before.Check(*this, M.Get(), Label + TEXT(" shifted"));
        APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Owned.World, Offset);
        TestTrue(Label + TEXT(" repeated notification does not double-add offset"), HasCenter(M.Get(), CenterName, Frame->GetComponentLocation()));
        Frame->ApplyWorldOffset(-Offset, true);
        APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Owned.World, -Offset);
        TestEqual(Label + TEXT(" reverse shift also bypasses TransformUpdated"), Bindings.TransformUpdates, 0);
        TestTrue(Label + TEXT(" opposite shift restores root"), Frame->GetComponentLocation().Equals(Initial, 1.0e-6));
        TestTrue(Label + TEXT(" opposite shift restores material center"), HasCenter(M.Get(), CenterName, Initial));
        Before.Check(*this, M.Get(), Label + TEXT(" restored"));
        TestEqual(Label + TEXT(" saved parent dirty state unchanged"), Parent->GetOutermost()->IsDirty(), bWasDirty);
    }
    AddInfo(TEXT("CPU MID/frame contracts only; no shader/render acceptance, cloud component or factory readiness coverage. Saved parents are read-only. Legacy global origin event is not broadcast: vendor listeners may ignore its world argument."));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSWorldShiftWeakLifetimeTest,
    "APS.World.Origin.MaterialFrames.WeakLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSWorldShiftWeakLifetimeTest::RunTest(const FString& Parameters)
{
    using namespace APSWorldShiftMaterialFrameTests;
    FTestWorld Owned, Unrelated;
    if (!TestNotNull(TEXT("Owned transient world"), Owned.World)
        || !TestNotNull(TEXT("Unrelated transient world"), Unrelated.World)) return false;
    // UObject itself is abstract in UE5.4. This concrete, unregistered receiver
    // has no world/render state; only its weak UObject lifetime is under test.
    TStrongObjectPtr<USceneComponent> Lifetime(
        NewObject<USceneComponent>(GetTransientPackage(), NAME_None, RF_Transient));
    if (!TestNotNull(TEXT("Concrete transient receiver"), Lifetime.Get())) return false;
    const TSharedRef<int32> Calls = MakeShared<int32>(0);
    APSWorldShiftEvents::BindPostShift(Lifetime.Get(), [ExpectedWorld = Owned.World, Calls](UWorld* World)
    {
        if (World == ExpectedWorld) ++*Calls;
    });
    TestTrue(TEXT("Helper subscribes legacy event"), FCoreDelegates::PostWorldOriginOffset.IsBoundToObject(Lifetime.Get()));
    TestTrue(TEXT("Helper subscribes double event"), APSWorldShiftEvents::OnPostDoubleShift().IsBoundToObject(Lifetime.Get()));
    APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Unrelated.World, FVector(1.25, 2.5, 3.75));
    TestEqual(TEXT("Caller world guard filters unrelated event"), *Calls, 0);
    APSWorldShiftEvents::OnPostDoubleShift().Broadcast(Owned.World, FVector(1.25, 2.5, 3.75));
    TestEqual(TEXT("Live lifetime receives own world event"), *Calls, 1);
    // Copy the registered weak callback before removing it from global delegates.
    // Expire only this transient UObject, with no global GC or stale test closure.
    auto ExpiredProbe = APSWorldShiftEvents::OnPostDoubleShift();
    FCoreDelegates::PostWorldOriginOffset.RemoveAll(Lifetime.Get());
    APSWorldShiftEvents::OnPostDoubleShift().RemoveAll(Lifetime.Get());
    Lifetime->MarkAsGarbage();
    TestFalse(TEXT("Transient lifetime is invalid"), IsValid(Lifetime.Get()));
    ExpiredProbe.Broadcast(Owned.World, FVector(1.25, 2.5, 3.75));
    TestEqual(TEXT("Expired weak callback does not execute"), *Calls, 1);
    return true;
}

#endif

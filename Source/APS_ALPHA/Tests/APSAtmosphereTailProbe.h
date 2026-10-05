#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "APSAtmosphereTailAssets.h"
#include "APSFrozenDescentProbe.h"
#include "PlanetaryAtmosphere.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

// Explicit boundary/ground-route experiment only. Both ordinary atmosphere passes keep
// their geometry and optical inputs; only the copied shader's upper-density tail
// differs. Never adds a production selector or changes a source material asset.
namespace APSAtmosphereTailProbe
{
    inline bool Parse(const TCHAR* CommandLine, int32& Mode, FString& Error,
        bool* bGroundRequested = nullptr)
    {
        Mode = -1; Error.Reset();
        if (bGroundRequested) *bGroundRequested = false;
        bool bGround = false;
        const TCHAR* Cursor = CommandLine ? CommandLine : TEXT("");
        FString Token;
        while (FParse::Token(Cursor, Token, false))
        {
            const bool bGroundFlag = Token.Equals(TEXT("-APSProbeFrozenAtmosphereGround"), ESearchCase::IgnoreCase);
            const bool bGroundValue = Token.StartsWith(TEXT("-APSProbeFrozenAtmosphereGround="), ESearchCase::IgnoreCase);
            if (bGroundFlag || bGroundValue)
            {
                if (bGround || bGroundValue)
                { Error = TEXT("Atmosphere ground flag must occur once and have no value"); return false; }
                bGround = true;
                continue;
            }
            if (!Token.Equals(TEXT("-APSProbeAtmosphereTail"), ESearchCase::IgnoreCase)
                && !Token.StartsWith(TEXT("-APSProbeAtmosphereTail="), ESearchCase::IgnoreCase)) continue;
            if (Mode != -1 || (!Token.Equals(TEXT("-APSProbeAtmosphereTail=0"), ESearchCase::IgnoreCase)
                && !Token.Equals(TEXT("-APSProbeAtmosphereTail=1"), ESearchCase::IgnoreCase)))
            { Error = TEXT("Atmosphere tail requires exactly one =0 (control) or =1 (candidate)"); return false; }
            Mode = Token.EndsWith(TEXT("1")) ? 1 : 0;
        }
        // Ground route may now observe the installed production parent directly;
        // absence of a tail selector must never install the diagnostic lease.
        if (Mode >= 0 || bGround)
        {
            bool bBoundary = false;
            if (!APSFrozenDescentProbe::ParseAtmosphereBoundaryCommandLine(CommandLine, bBoundary, Error)) return false;
            if (!bBoundary || FParse::Param(CommandLine, TEXT("APSProbeContinuousWarpPixel")))
            { Error = TEXT("Atmosphere tail requires the separate default-atmosphere boundary route, without warp-pixel"); return false; }
        }
        if (bGroundRequested) *bGroundRequested = bGround;
        return true;
    }

    enum class EReadiness : uint8 { Ready, Pending, Failed };
    // The plugin intentionally exposes no public setters for these private
    // UPROPERTY fields. This diagnostic checks their exact reflected contract;
    // it never changes plugin access or assumes a native member offset.
    struct FActorFields
    {
        FObjectProperty* InsideMaterial = nullptr;
        FObjectProperty* OutsideMaterial = nullptr;
        FObjectProperty* InsideComponent = nullptr;
        FObjectProperty* OutsideComponent = nullptr;

        bool Resolve(AAtmoScape* Atmo, FString& Error)
        {
            if (!IsValid(Atmo)) { Error = TEXT("Atmosphere tail actor is invalid"); return false; }
            const auto Find = [&](const TCHAR* Name, UClass* Type, FObjectProperty*& Out)
            {
                Out = FindFProperty<FObjectProperty>(AAtmoScape::StaticClass(), Name);
                if (!Out || Out->GetOwnerClass() != AAtmoScape::StaticClass()
                    || Out->GetFName() != FName(Name) || Out->PropertyClass != Type || Out->ArrayDim != 1)
                { Error = FString(TEXT("Atmosphere tail reflected field contract changed: ")) + Name; return false; }
                return true;
            };
            return Find(TEXT("AtmosphereMaterial"), UMaterialInstanceDynamic::StaticClass(), InsideMaterial)
                && Find(TEXT("SpaceAtmosphereMaterial"), UMaterialInstanceDynamic::StaticClass(), OutsideMaterial)
                && Find(TEXT("PlanetaryAtmoMesh"), UStaticMeshComponent::StaticClass(), InsideComponent)
                && Find(TEXT("SpacePlanetaryAtmoMesh"), UStaticMeshComponent::StaticClass(), OutsideComponent);
        }
        UMaterialInstanceDynamic* ReadInside(AAtmoScape* Atmo) const
        { return Cast<UMaterialInstanceDynamic>(InsideMaterial->GetObjectPropertyValue_InContainer(Atmo)); }
        UMaterialInstanceDynamic* ReadOutside(AAtmoScape* Atmo) const
        { return Cast<UMaterialInstanceDynamic>(OutsideMaterial->GetObjectPropertyValue_InContainer(Atmo)); }
        UStaticMeshComponent* ReadInsideComponent(AAtmoScape* Atmo) const
        { return Cast<UStaticMeshComponent>(InsideComponent->GetObjectPropertyValue_InContainer(Atmo)); }
        UStaticMeshComponent* ReadOutsideComponent(AAtmoScape* Atmo) const
        { return Cast<UStaticMeshComponent>(OutsideComponent->GetObjectPropertyValue_InContainer(Atmo)); }
    };

    class FLease
    {
        TStrongObjectPtr<UMaterial> Master{nullptr};
        TStrongObjectPtr<UMaterialInstanceDynamic> OriginalInside{nullptr}, OriginalOutside{nullptr};
        TStrongObjectPtr<UMaterialInstanceDynamic> Inside{nullptr}, Outside{nullptr};
        TWeakObjectPtr<AAtmoScape> Atmosphere;
        TWeakObjectPtr<UStaticMeshComponent> InsideComponent, OutsideComponent;
        TWeakObjectPtr<UStaticMesh> InsideMesh, OutsideMesh;
        int32 SelectedMode = -1;
        bool bCompileRequested = false;
        static bool SameUniforms(UMaterialInstanceDynamic* Source, UMaterialInstanceDynamic* Copy, FString& Error)
        {
            TArray<FMaterialParameterInfo> Infos; TArray<FGuid> Ids;
            Source->GetAllScalarParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                float A = 0, B = 0;
                if (!Source->GetScalarParameterValue(Info, A) || !Copy->GetScalarParameterValue(Info, B) || A != B)
                { Error = TEXT("Atmosphere tail changed scalar: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Source->GetAllVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                FLinearColor A, B;
                if (!Source->GetVectorParameterValue(Info, A) || !Copy->GetVectorParameterValue(Info, B) || A != B)
                { Error = TEXT("Atmosphere tail changed vector: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Source->GetAllDoubleVectorParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                FVector4 A, B;
                if (!Source->GetDoubleVectorParameterValue(Info, A) || !Copy->GetDoubleVectorParameterValue(Info, B) || A != B)
                { Error = TEXT("Atmosphere tail changed double-vector: ") + Info.Name.ToString(); return false; }
            }
            Infos.Reset(); Ids.Reset(); Source->GetAllTextureParameterInfo(Infos, Ids);
            for (const auto& Info : Infos)
            {
                UTexture* A = nullptr; UTexture* B = nullptr;
                if (!Source->GetTextureParameterValue(Info, A) || !Copy->GetTextureParameterValue(Info, B) || A != B)
                { Error = TEXT("Atmosphere tail changed texture: ") + Info.Name.ToString(); return false; }
            }
            return true;
        }
    public:
        ~FLease() { Restore(); }
        bool Active() const { return Atmosphere.IsValid() && Inside.IsValid() && Outside.IsValid(); }
        EReadiness Prepare(UWorld* World, FString& Error)
        {
            if (!IsValid(World) || !Parse(FCommandLine::Get(), SelectedMode, Error) || SelectedMode < 0)
            { if (Error.IsEmpty()) Error = TEXT("Atmosphere tail requires explicit mode and world"); return EReadiness::Failed; }
            if (!Master.IsValid())
            {
                if (!APSAtmosphereTailAssets::VerifySources(Error)) return EReadiness::Failed;
                Master.Reset(LoadObject<UMaterial>(nullptr, APSAtmosphereTailAssets::MasterPath));
            }
            auto* Resource = Master.IsValid() ? Master->GetMaterialResource(World->GetFeatureLevel()) : nullptr;
            if (!Resource || Resource->GetCompileErrors().Num())
            { Error = TEXT("Atmosphere tail material missing or shader errors on cold load"); return EReadiness::Failed; }
            if (!Resource->IsGameThreadShaderMapComplete())
            {
#if WITH_EDITOR
                if (!bCompileRequested) Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::ForceLocal);
#endif
                bCompileRequested = true; Error = TEXT("Atmosphere tail shader pending"); return EReadiness::Pending;
            }
            const auto* Map = Resource->GetGameThreadShaderMap();
            if (!Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            { Error = TEXT("Atmosphere tail shader lacks LocalVF"); return EReadiness::Failed; }
            Error.Reset(); return EReadiness::Ready;
        }

        bool Begin(AAtmoScape* Atmo, FString& Error)
        {
            if (Active() || !IsValid(Atmo) || !Master.IsValid() || SelectedMode < 0)
            { Error = TEXT("Atmosphere tail lease not prepared or already active"); return false; }
            FActorFields Fields;
            if (!Fields.Resolve(Atmo, Error)) return false;
            auto* NativeInside = Fields.ReadInside(Atmo);
            auto* NativeOutside = Fields.ReadOutside(Atmo);
            auto* NativeInsideComponent = Fields.ReadInsideComponent(Atmo);
            auto* NativeOutsideComponent = Fields.ReadOutsideComponent(Atmo);
            if (!IsValid(NativeInsideComponent) || !IsValid(NativeOutsideComponent)
                || NativeInsideComponent == NativeOutsideComponent
                || NativeInsideComponent->GetOwner() != Atmo || NativeOutsideComponent->GetOwner() != Atmo
                || !IsValid(NativeInsideComponent->GetStaticMesh()) || !IsValid(NativeOutsideComponent->GetStaticMesh())
                || !IsValid(NativeInside) || !IsValid(NativeOutside)
                || NativeInsideComponent->GetMaterial(0) != NativeInside
                || NativeOutsideComponent->GetMaterial(0) != NativeOutside
                || !IsValid(NativeInside->GetMaterial()) || !IsValid(NativeOutside->GetMaterial())
                || NativeInside->GetMaterial()->GetPathName() != APSAtmosphereTailAssets::SourceMasterPath
                || NativeOutside->GetMaterial()->GetPathName() != APSAtmosphereTailAssets::SourceMasterPath)
            { Error = TEXT("Atmosphere tail requires two ordinary source-bound atmosphere passes"); return false; }
            OriginalInside.Reset(NativeInside); OriginalOutside.Reset(NativeOutside);
            Inside.Reset(UMaterialInstanceDynamic::Create(Master.Get(), Atmo));
            Outside.Reset(UMaterialInstanceDynamic::Create(Master.Get(), Atmo));
            if (!Inside.IsValid() || !Outside.IsValid())
            { Error = TEXT("Atmosphere tail MID allocation failed"); Restore(); return false; }
            Inside->CopyMaterialUniformParameters(OriginalInside.Get());
            Outside->CopyMaterialUniformParameters(OriginalOutside.Get());
            if (!SameUniforms(OriginalInside.Get(), Inside.Get(), Error)
                || !SameUniforms(OriginalOutside.Get(), Outside.Get(), Error)) { Restore(); return false; }
            Inside->SetScalarParameterValue(APSAtmosphereTailAssets::TailParameter, SelectedMode);
            Outside->SetScalarParameterValue(APSAtmosphereTailAssets::TailParameter, SelectedMode);
            Atmosphere = Atmo;
            InsideComponent = NativeInsideComponent; OutsideComponent = NativeOutsideComponent;
            InsideMesh = NativeInsideComponent->GetStaticMesh(); OutsideMesh = NativeOutsideComponent->GetStaticMesh();
            // Actor fields must match the mesh MIDs so its normal UpdateScale
            // continues writing camera/sun/physical-frame inputs on every tick.
            Fields.InsideMaterial->SetObjectPropertyValue_InContainer(Atmo, Inside.Get());
            Fields.OutsideMaterial->SetObjectPropertyValue_InContainer(Atmo, Outside.Get());
            NativeInsideComponent->SetMaterial(0, Inside.Get());
            NativeOutsideComponent->SetMaterial(0, Outside.Get());
            if (!Validate(Atmo, Error)) { Restore(); return false; }
            UE_LOG(LogTemp, Display, TEXT("[APS.AtmosphereTail.Probe] BEGIN mode=%d master=%s two native meshes; optical inputs copied, lower80%% local density retained; diagnostic only"),
                SelectedMode, APSAtmosphereTailAssets::MasterPath);
            return true;
        }

        bool Validate(AAtmoScape* Atmo, FString& Error) const
        {
            if (!Active() || Atmo != Atmosphere.Get())
            { Error = TEXT("Atmosphere tail lease actor changed"); return false; }
            FActorFields Fields;
            if (!Fields.Resolve(Atmo, Error)) return false;
            auto* BoundInside = Fields.ReadInsideComponent(Atmo);
            auto* BoundOutside = Fields.ReadOutsideComponent(Atmo);
            if (!IsValid(BoundInside) || !IsValid(BoundOutside)
                || BoundInside != InsideComponent.Get() || BoundOutside != OutsideComponent.Get()
                || BoundInside->GetOwner() != Atmo || BoundOutside->GetOwner() != Atmo
                || Fields.ReadInside(Atmo) != Inside.Get() || Fields.ReadOutside(Atmo) != Outside.Get()
                || BoundInside->GetMaterial(0) != Inside.Get() || BoundOutside->GetMaterial(0) != Outside.Get()
                || !InsideMesh.IsValid() || !OutsideMesh.IsValid()
                || BoundInside->GetStaticMesh() != InsideMesh.Get() || BoundOutside->GetStaticMesh() != OutsideMesh.Get())
            { Error = TEXT("Atmosphere tail lease actor/material/mesh changed"); return false; }
            for (auto* MID : {Inside.Get(), Outside.Get()})
            {
                float Value = -1, EarthRadius = 0, AtmosRadius = 0;
                if (MID->Parent != Master.Get()
                    || !MID->GetScalarParameterValue(FMaterialParameterInfo(APSAtmosphereTailAssets::TailParameter), Value)
                    || Value != SelectedMode
                    || !MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("EarthRadius")), EarthRadius)
                    || !MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("AtmosRadius")), AtmosRadius)
                    || !FMath::IsFinite(EarthRadius) || !FMath::IsFinite(AtmosRadius) || AtmosRadius <= EarthRadius || EarthRadius <= 0)
                { Error = TEXT("Atmosphere tail mode/radius contract changed"); return false; }
            }
            Error.Reset(); return true;
        }

        void Restore()
        {
            if (auto* Atmo = Atmosphere.Get())
            {
                FActorFields Fields; FString Error;
                if (Fields.Resolve(Atmo, Error))
                {
                    if (Inside.IsValid() && OriginalInside.IsValid() && Fields.ReadInside(Atmo) == Inside.Get())
                        Fields.InsideMaterial->SetObjectPropertyValue_InContainer(Atmo, OriginalInside.Get());
                    if (Outside.IsValid() && OriginalOutside.IsValid() && Fields.ReadOutside(Atmo) == Outside.Get())
                        Fields.OutsideMaterial->SetObjectPropertyValue_InContainer(Atmo, OriginalOutside.Get());
                    auto* BoundInside = InsideComponent.Get(); auto* BoundOutside = OutsideComponent.Get();
                    if (IsValid(BoundInside) && BoundInside->GetOwner() == Atmo && OriginalInside.IsValid()
                        && Inside.IsValid() && BoundInside->GetMaterial(0) == Inside.Get())
                        BoundInside->SetMaterial(0, OriginalInside.Get());
                    if (IsValid(BoundOutside) && BoundOutside->GetOwner() == Atmo && OriginalOutside.IsValid()
                        && Outside.IsValid() && BoundOutside->GetMaterial(0) == Outside.Get())
                        BoundOutside->SetMaterial(0, OriginalOutside.Get());
                    if (IsValid(BoundInside) && IsValid(BoundOutside)
                        && Fields.ReadInsideComponent(Atmo) == BoundInside && Fields.ReadOutsideComponent(Atmo) == BoundOutside
                        && OriginalInside.IsValid() && OriginalOutside.IsValid()
                        && Fields.ReadInside(Atmo) == OriginalInside.Get() && Fields.ReadOutside(Atmo) == OriginalOutside.Get())
                        Atmo->UpdateScale();
                }
                else UE_LOG(LogTemp, Error, TEXT("[APS.AtmosphereTail.Probe] Restore refused: %s"), *Error);
            }
            Atmosphere.Reset(); Inside.Reset(); Outside.Reset(); OriginalInside.Reset(); OriginalOutside.Reset();
            InsideComponent.Reset(); OutsideComponent.Reset();
            InsideMesh.Reset(); OutsideMesh.Reset(); Master.Reset(); SelectedMode = -1; bCompileRequested = false;
        }
    };
}
#endif

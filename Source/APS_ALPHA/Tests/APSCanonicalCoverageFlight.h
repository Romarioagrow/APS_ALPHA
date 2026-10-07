#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "APSCanonicalCoverageTexture.h"
#include "APSFrozenDescentProbe.h"
#include "APS_ALPHA/Generation/APSNativeGlobeSnapshot.h"
#include "APS_ALPHA/Core/Planetary/APSTerrainContinuityMaterial.h"
#include "Async/Async.h"
#include "Camera/CameraTypes.h"
#include "Engine/Texture2D.h"
#include "EngineGlobals.h"
#include "LocalVertexFactory.h"
#include "MaterialShared.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "RenderCommandFence.h"
#include "UObject/StrongObjectPtr.h"
#include <atomic>

// Opt-in, unattended Lidim fixture only. Prepares before the EXISTING descent
// clock starts, then permits ordinary movement/LOD publication. No pose setters,
// alternate MID/parent, freeze, asset save or production/cache ownership.
namespace APSCanonicalCoverageFlight
{
    constexpr double PreparationSeconds = 120.0;
    constexpr int32 Slots = APSCanonicalCoverageLayout::MaximumLevels;
    constexpr int32 VectorCount = 3 + Slots;
    constexpr int32 NormalTextureCount = 7;
    constexpr int64 MaximumBytesPerLevel = 11184808; // 1024^2 RGBA16F, full chain.
    enum class EResult : uint8 { Pending, Ready, Failed };

    inline bool Requested()
    {
        const TCHAR* Cursor = FCommandLine::Get(); FString Token;
        while (FParse::Token(Cursor, Token, false))
            if (Token.Equals(TEXT("-APSProbeCanonicalCoverageFlight"), ESearchCase::IgnoreCase)
                || Token.StartsWith(TEXT("-APSProbeCanonicalCoverageFlight="), ESearchCase::IgnoreCase)
                // Invalid orphan diagnostic flags must reach Guard, not silently run a control.
                || Token.Equals(TEXT("-APSProbeCanonicalDenseHeight"), ESearchCase::IgnoreCase)
                || Token.StartsWith(TEXT("-APSProbeCanonicalDenseHeight="), ESearchCase::IgnoreCase)
                || Token.Equals(TEXT("-APSProbeCanonicalFlatNormals"), ESearchCase::IgnoreCase)
                || Token.StartsWith(TEXT("-APSProbeCanonicalFlatNormals="), ESearchCase::IgnoreCase)) return true;
        return false;
    }

    class FLease
    {
        struct FBuildResult
        {
            TArray<APSCanonicalFilteredHeight::FData> Levels;
            APSCanonicalFilteredHeight::FComparison Comparison, QuadratureComparison;
            double TotalWallSeconds = 0, ExtraFiveSeconds = 0, ExtraDenseSeconds = 0, ExtraQuadratureSeconds = 0;
            int64 ExtraFiveCalls = 0, ExtraDenseCalls = 0, ExtraQuadratureCalls = 0;
            int32 DenseOversample = 0, ComparisonLevel = 1;
            FString Error; bool Valid = false;
        };
        TFuture<FBuildResult> Future;
        TSharedPtr<std::atomic_bool, ESPMode::ThreadSafe> Cancellation;
        int32 DenseOversample = 0;
        TStrongObjectPtr<UMaterialInstanceDynamic> Material{nullptr};
        TStrongObjectPtr<UTexture> OldTextures[Slots];
        TStrongObjectPtr<UTexture2D> Textures[Slots];
        TStrongObjectPtr<UTexture2D> FlatNormal{nullptr};
        TStrongObjectPtr<UTexture> OldNormalTextures[NormalTextureCount];
        TWeakObjectPtr<APlanetarySurfaceGenerator> Surface;
        TWeakObjectPtr<AWorldScapeRoot> Root;
        TWeakObjectPtr<UMaterialInterface> Parent;
        APSCanonicalCoverageLayout::FLayout Layout;
        FLinearColor OldVectors[VectorCount], BoundVectors[VectorCount];
        float OldAvailable = 0, BoundAvailable = 0, NormalStart = 0, NormalEnd = 0;
        double NoiseScale = 0, NoiseIntensity = 0, BeginTime = 0, LastNow = 0, LastProgress = -10, EnableTime = 0;
        uint64 EnableFrame = 0;
        uint32 ProfileIdentity = 0;
        int32 Stage = 0; // 0 CPU,1 upload fence,2 disabled-bind fence,3 enabled fence.
        bool bStarted = false, bBound = false, bReady = false, bReleased = false, bReleaseOK = true;
        bool bFlatNormals = false, bNormalsBound = false;
        FRenderCommandFence Fence;
        FString Failure, ReleaseError;

        static FName TextureName(int32 I) { return FName(*FString::Printf(TEXT("APS_CanonicalCoverageTexture%d"), I)); }
        static FName AvailableName() { return TEXT("APS_CanonicalCoverageAvailable"); }
        static FName NormalTextureName(int32 I)
        {
            static const FName Names[] = {TEXT("SlopeNormal"), TEXT("SlopeNormal3"), TEXT("MacroSlopeNormal"),
                TEXT("Ground1N (T2d)"), TEXT("Ground2N(T2d)"), TEXT("Ground3"), TEXT("PlanetaryNormal")};
            static_assert(UE_ARRAY_COUNT(Names) == NormalTextureCount);
            return Names[I];
        }
        static FName VectorName(int32 I)
        {
            if (I >= 3) return FName(*FString::Printf(TEXT("APS_CanonicalCoverageMetrics%d"), I - 3));
            static const FName Names[] = {TEXT("APS_CanonicalCoverageCenter"), TEXT("APS_CanonicalCoverageU"), TEXT("APS_CanonicalCoverageV")};
            return Names[I];
        }
        UTexture2D* BoundTexture(int32 I) const { return Textures[FMath::Min(I, Layout.Count - 1)].Get(); }
        EResult Fail(FString Why, FString& Error)
        {
            Failure = MoveTemp(Why); FString Cleanup;
            if (!Release(Cleanup)) Failure += TEXT("; cleanup: ") + Cleanup;
            Error = Failure; return EResult::Failed;
        }
        static bool Guard(FString& Error, int32& Dense, bool& FlatNormals)
        {
            if (!WITH_EDITOR || !FApp::IsUnattended() || !IsInGameThread() || IsRunningCommandlet())
            { Error = TEXT("Coverage flight requires unattended editor gameplay automation on GT"); return false; }
            int32 Count = 0, DenseCount = 0, FlatCount = 0; Dense = 0; FlatNormals = false;
            bool bFlatConflict = false;
            const TCHAR* Cursor = FCommandLine::Get(); FString Token;
            while (FParse::Token(Cursor, Token, false))
            {
                if (Token.Equals(TEXT("-APSProbeCanonicalCoverageFlight"), ESearchCase::IgnoreCase)) ++Count;
                else if (Token.StartsWith(TEXT("-APSProbeCanonicalCoverageFlight="), ESearchCase::IgnoreCase))
                { Error = TEXT("Coverage flag takes no value"); return false; }
                if (Token.Equals(TEXT("-APSProbeCanonicalFlatNormals"), ESearchCase::IgnoreCase)) ++FlatCount;
                else if (Token.StartsWith(TEXT("-APSProbeCanonicalFlatNormals="), ESearchCase::IgnoreCase))
                { Error = TEXT("CanonicalFlatNormals takes no value"); return false; }
                for (const TCHAR* Other : {TEXT("-APSProbeCanonicalSlopeAB"), TEXT("-APSProbeNormalSources"),
                    TEXT("-APSProbeMeshCurvature"), TEXT("-APSProbeTerrainBuffers"), TEXT("-APSProbeMacroAB"),
                    TEXT("-APSProbeFarNormalAB"), TEXT("-APSProbeNormalHex"), TEXT("-APSProbeNormalMacroWarp"),
                    TEXT("-APSProbeOrbitalFieldsAB"), TEXT("-APSProbeAtmosphereFlight"),
                    TEXT("-APSProbeShadowFlight"), TEXT("-APSProbeStarShadowFlight")})
                    if (Token.Equals(Other, ESearchCase::IgnoreCase) || Token.StartsWith(FString(Other) + TEXT("="), ESearchCase::IgnoreCase))
                        bFlatConflict = true;
                if (Token.Equals(TEXT("-APSProbeCanonicalDenseHeight"), ESearchCase::IgnoreCase))
                { Error = TEXT("DenseHeight requires exactly =2 or =4"); return false; }
                if (Token.StartsWith(TEXT("-APSProbeCanonicalDenseHeight="), ESearchCase::IgnoreCase))
                {
                    ++DenseCount;
                    if (Token.Equals(TEXT("-APSProbeCanonicalDenseHeight=2"), ESearchCase::IgnoreCase)) Dense = 2;
                    else if (Token.Equals(TEXT("-APSProbeCanonicalDenseHeight=4"), ESearchCase::IgnoreCase)) Dense = 4;
                    else { Error = TEXT("DenseHeight accepts only2 or4"); return false; }
                }
                for (const TCHAR* Other : {TEXT("-APSProbeCanonicalChartAB"), TEXT("-APSProbeContinuousWarpPixel"),
                    TEXT("-APSProbeFrozenAtmosphereBoundary"), TEXT("-APSProbeFrozenAtmosphereGround"), TEXT("-APSProbeAtmosphereTail")})
                    if (Token.Equals(Other, ESearchCase::IgnoreCase) || Token.StartsWith(FString(Other) + TEXT("="), ESearchCase::IgnoreCase))
                    { Error = TEXT("Coverage flight cannot mix chart/warp/atmosphere diagnostic modes"); return false; }
            }
            if (DenseCount > 1 || (DenseCount && (!FParse::Param(FCommandLine::Get(), TEXT("APSProbeCanonicalCoverageFar"))
                || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeFlightDaylight"))
                || FParse::Param(FCommandLine::Get(), TEXT("APSProbeCanonicalSlopeAB")))))
            { Error = TEXT("DenseHeight requires one opt-in, standalone CoverageFar and Daylight; no slope comparison"); return false; }
            if (FlatCount > 1 || (FlatCount && (DenseCount || bFlatConflict
                || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeCanonicalCoverageFar"))
                || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeFlightDaylight")))))
            { Error = TEXT("CanonicalFlatNormals requires one opt-in, standalone CoverageFar and Daylight; no dense/slope/other normal isolation"); return false; }
            APSFrozenDescentProbe::FReference Reference; bool bDescent = false;
            if (Count != 1 || !FParse::Param(FCommandLine::Get(), TEXT("APSProbeOrbitalFieldsFlight"))
                || !FParse::Param(FCommandLine::Get(), TEXT("APSProbePublishedTerrainFlight"))
                || !APSFrozenDescentProbe::ParseCommandLine(FCommandLine::Get(), Reference, bDescent, Error)
                || !bDescent || Reference.Name != TEXT("Lidim"))
            { if (Error.IsEmpty()) Error = TEXT("Coverage needs exactly one opt-in and the existing published Lidim descent fixture"); return false; }
            FlatNormals = FlatCount == 1;
            return true;
        }
        bool Check(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, FString& Error) const
        {
            auto* R = Root.Get(); auto* M = Material.Get();
            if (!IsInGameThread() || !IsValid(S) || S != Surface.Get() || !IsValid(R) || !IsValid(M)
                || !IsValid(S->PlanetaryBody) || S->WorldScapeRootInstance != R
                || S->GetWorld() != R->GetWorld() || S->ResolvedTerrainMaterialInstance != M
                || R->TerrainMaterial.DefaultMaterial != M || M->Parent.Get() != Parent.Get()
                || !S->PlanetaryBody->bWorldScapeSurfaceReady || R->bFreezeGeneration || !R->IsActorTickEnabled()
                || R->Seed != 257455 || double(R->PlanetScale) != Layout.RadiusCm
                || double(R->NoiseScale) != NoiseScale || double(R->NoiseIntensity) != NoiseIntensity
                || APSNativeGlobeSnapshot::Identity(S->PlanetaryBody, S->SurfaceProfileCatalog) != ProfileIdentity
                || !APSNativeGlobeSnapshot::Ready(S->PlanetaryBody, S, Error))
            { if (Error.IsEmpty()) Error = TEXT("Coverage lost original body/root/noise/profile/MID identity"); return false; }
            const FTransform Frame = R->GetActorTransform();
            const FVector Local = Frame.InverseTransformPosition(View.Location);
            if (View.Location.ContainsNaN() || View.Rotation.ContainsNaN() || Local.ContainsNaN() || Local.IsNearlyZero()
                || !FMath::IsFinite(View.FOV) || View.FOV <= 0 || View.FOV >= 180
                || !FMath::IsFinite(View.AspectRatio) || View.AspectRatio <= 0)
            { Error = TEXT("Coverage camera/frame became invalid"); return false; }
            const FVector C(Layout.Frame.Center.X, Layout.Frame.Center.Y, Layout.Frame.Center.Z);
            if (FVector::DotProduct(Local.GetSafeNormal(), C) < 0.0)
            { Error = TEXT("Coverage immutable front anchor no longer covers observer hemisphere"); return false; }
            float Start = 0, End = 0;
            if (!M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), Start)
                || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), End)
                || Start != NormalStart || End != NormalEnd)
            { Error = TEXT("Coverage existing near/far normal policy changed"); return false; }
            const auto* Resource = M->GetMaterialResource(R->GetWorld()->GetFeatureLevel());
            const auto* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
            if (!Resource || !Resource->IsGameThreadShaderMapComplete() || !Map
                || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
            { Error = TEXT("Coverage actual MID shader is not complete with LocalVF"); return false; }
#if WITH_EDITOR
            if (!Resource->GetCompileErrors().IsEmpty())
            { Error = TEXT("Coverage actual MID shader has compile errors"); return false; }
#endif
            // Published GT section material identities only. Workers, geometry
            // signatures and completeness may change normally during the route.
            if (R->WorldScapeLod.Num() > 32) { Error = TEXT("Coverage LOD binding audit bound exceeded"); return false; }
            for (const auto* Lod : R->WorldScapeLod)
            {
                auto* Mesh = IsValid(Lod) ? Lod->Mesh : nullptr;
                if (!IsValid(Mesh) || !Mesh->IsVisible() || Mesh->bHiddenInGame) continue;
                if (Mesh->GetNumSections() > 3) { Error = TEXT("Coverage unexpected terrain section count"); return false; }
                for (int32 I = 0; I < Mesh->GetNumSections(); ++I)
                {
                    const auto* Section = Mesh->GetProcMeshSection(I);
                    if (Section && Section->bSectionVisible && !Section->PlanetVertexBuffer.IsEmpty()
                        && Mesh->GetMaterial(I) != M)
                    { Error = TEXT("Coverage presented terrain section switched material"); return false; }
                }
            }
            if (bBound)
            {
                float Available = -1;
                if (!M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), Available) || Available != BoundAvailable)
                { Error = TEXT("Coverage owned availability was overwritten"); return false; }
                for (int32 I = 0; I < Slots; ++I)
                {
                    UTexture* Value = nullptr; auto* Expected = BoundTexture(I);
                    if (!IsValid(Expected) || !Expected->GetResource() || !Expected->GetResource()->IsInitialized()
                        || !M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)), Value) || Value != Expected)
                    { Error = TEXT("Coverage owned texture/resource changed"); return false; }
                }
                for (int32 I = 0; I < VectorCount; ++I)
                {
                    FLinearColor Value;
                    if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), Value) || Value != BoundVectors[I])
                    { Error = TEXT("Coverage owned chart frame/metrics changed"); return false; }
                }
            }
            if (bFlatNormals)
                for (int32 I = 0; I < NormalTextureCount; ++I)
                {
                    UTexture* Value = nullptr;
                    UTexture* Expected = bNormalsBound ? FlatNormal.Get() : OldNormalTextures[I].Get();
                    if (!IsValid(Expected) || !M->GetTextureParameterValue(FMaterialParameterInfo(NormalTextureName(I)), Value)
                        || Value != Expected)
                    { Error = TEXT("CanonicalFlatNormals owned texture changed: ") + NormalTextureName(I).ToString(); return false; }
                }
            return true;
        }
        bool Begin(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, double Now, FString& Error)
        {
            if (!Guard(Error, DenseOversample, bFlatNormals) || !IsValid(S) || !IsValid(S->PlanetaryBody))
            { if (Error.IsEmpty()) Error = TEXT("Coverage missing fixture surface"); return false; }
            APSClosedGlobeMesh::FSamplingFrame Frame; APSClosedGlobeMesh::FBuildOptions Options;
            if (!APSNativeGlobeSnapshot::Capture(S->PlanetaryBody, S, Frame, Options, ProfileIdentity, Error)) return false;
            auto* R = S->WorldScapeRootInstance; auto* M = S->ResolvedTerrainMaterialInstance;
            if (Frame.Profile.PlanetType != EPlanetType::Frozen || R->Seed != 257455
                || !FMath::IsNearlyEqual(Frame.Radius, double(float(1280.896 * 100000.0)), .01)
                || !IsValid(M) || !M->GetMaterial() || M->GetMaterial()->GetPathName() != APSTerrainContinuityMaterial::MasterPath
                || !M->Parent || M->Parent->GetPathName() != APSTerrainContinuityMaterial::TemplatePath)
            { Error = TEXT("Coverage is restricted to Lidim and the original canonical master/MIC"); return false; }
            Root = R; Surface = S; Parent = M->Parent; Material.Reset(M);
            NoiseScale = Frame.NoiseScale; NoiseIntensity = Frame.NoiseIntensity;
            const FVector Local = R->GetActorTransform().InverseTransformPosition(View.Location);
            const FQuat Rotation = R->GetActorQuat().Inverse() * View.Rotation.Quaternion();
            const double HeightKm = (Local.Size() - Frame.Radius) / 100000.0;
            const FVector C = Local.GetSafeNormal();
            const FVector U = FVector::VectorPlaneProject(Rotation.GetRightVector(), C).GetSafeNormal();
            const FVector V = FVector::CrossProduct(C, U);
            APSCanonicalCoverageLayout::FFrame Anchor{{C.X,C.Y,C.Z},{U.X,U.Y,U.Z},{V.X,V.Y,V.Z}};
            std::string LayoutError;
            if (!FMath::IsFinite(HeightKm) || HeightKm < 20 || HeightKm > 40
                || !APSCanonicalCoverageLayout::Build(Frame.Radius, Anchor, Layout, LayoutError) || Layout.Count < 2)
            { Error = TEXT("Coverage needs initial30.5km route hold and valid immutable radial anchor: ") + FString(UTF8_TO_TCHAR(LayoutError.c_str())); return false; }
            if (!M->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()), OldAvailable) || OldAvailable != 0
                || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalStartCm")), NormalStart)
                || !M->GetScalarParameterValue(FMaterialParameterInfo(TEXT("APS_FarNormalEndCm")), NormalEnd)
                || !FMath::IsFinite(NormalStart) || !FMath::IsFinite(NormalEnd) || NormalStart <= 0 || NormalEnd <= NormalStart)
            { Error = TEXT("Coverage requires installed default-disabled parameters and existing normal policy"); return false; }
            if (bFlatNormals)
            {
                FlatNormal.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineMaterials/DefaultNormal.DefaultNormal")));
                if (!FlatNormal.IsValid() || FlatNormal->SRGB || FlatNormal->CompressionSettings != TC_Normalmap)
                { Error = TEXT("CanonicalFlatNormals requires the existing linear engine normal-map texture"); return false; }
                for (int32 I = 0; I < NormalTextureCount; ++I)
                {
                    UTexture* Previous = nullptr;
                    if (!M->GetTextureParameterValue(FMaterialParameterInfo(NormalTextureName(I)), Previous) || !IsValid(Previous))
                    { Error = TEXT("CanonicalFlatNormals missing original texture: ") + NormalTextureName(I).ToString(); return false; }
                    OldNormalTextures[I].Reset(Previous);
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.FlatNormals] original parameter=%s texture=%s"),
                        *NormalTextureName(I).ToString(), *Previous->GetPathName());
                }
                for (const TCHAR* Name : {TEXT("Ground2TexSize"), TEXT("SlopeTextureSize1"), TEXT("SlopeTextureSize2"),
                    TEXT("SlopeTextureSize3"), TEXT("APS_NormalMacroWarpMode")})
                {
                    float Value = 0;
                    if (M->GetScalarParameterValue(FMaterialParameterInfo(Name), Value))
                        UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.FlatNormals] original scalar=%s value=%.9g"),Name,Value);
                }
            }
            for (int32 I = 0; I < Slots; ++I)
            {
                UTexture* Previous = nullptr;
                if (!M->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)), Previous) || !IsValid(Previous))
                { Error = TEXT("Coverage texture parameter missing"); return false; }
                OldTextures[I].Reset(Previous);
            }
            for (int32 I = 0; I < VectorCount; ++I)
                if (!M->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)), OldVectors[I]))
                { Error = TEXT("Coverage vector parameter missing"); return false; }
            BoundVectors[0] = FLinearColor(C.X,C.Y,C.Z,float(Layout.Count));
            BoundVectors[1] = FLinearColor(U.X,U.Y,U.Z,0); BoundVectors[2] = FLinearColor(V.X,V.Y,V.Z,0);
            for (int32 I = 0; I < Slots; ++I)
            {
                const double Width = Layout.Levels[FMath::Min(I,Layout.Count-1)].WidthCm;
                BoundVectors[3+I] = FLinearColor(2.0 * Frame.Radius / Width, .5f / Layout.Resolution, Width, 0);
            }
            if (!Check(S, View, Error)) return false;
            const auto CapturedLayout = Layout; const uint32 Signature = ProfileIdentity; const int32 Dense = DenseOversample;
            Cancellation = MakeShared<std::atomic_bool, ESPMode::ThreadSafe>(false);
            const auto Cancel = Cancellation;
            bStarted = true; BeginTime = LastNow = Now;
            Future = Async(EAsyncExecution::ThreadPool, [Frame, CapturedLayout, Signature, Dense, Cancel]()
            {
                FBuildResult Result; Result.DenseOversample = Dense; const double Started = FPlatformTime::Seconds();
                APSCanonicalFilteredHeight::FOptions Settings; Settings.InputSignature = Signature;
                Settings.Cancellation = Cancel.Get(); Settings.Quadrature = APSCanonicalFilteredHeight::EQuadrature::Three;
                if (Dense)
                { Settings.HeightFilter = APSCanonicalFilteredHeight::EHeightFilter::SharedLattice; Settings.Oversample = Dense; }
                if (!APSCanonicalFilteredHeight::BuildCascade(Frame,CapturedLayout,Settings,Result.Levels,Result.Error)) return Result;
                if (!Dense)
                {
                    // Original control: unchanged level1,3-vs-5 quadrature measurement.
                    Settings.Quadrature = APSCanonicalFilteredHeight::EQuadrature::Five;
                    APSCanonicalFilteredHeight::FData Extra;
                    if (!APSCanonicalFilteredHeight::BuildLevel(Frame,CapturedLayout,1,Settings,Extra,Result.Error)
                        || !APSCanonicalFilteredHeight::CompareSameLattice(Result.Levels[1],Extra,Result.Comparison,Result.Error)) return Result;
                    Result.ExtraFiveCalls = Extra.Metadata.ActualHeightSamples;
                    Result.ExtraFiveSeconds = Extra.Metadata.PreparationWallSeconds;
                }
                else
                {
                    Result.ComparisonLevel = CapturedLayout.Count - 1;
                    const auto& Selected = Result.Levels[Result.ComparisonLevel];
                    APSCanonicalFilteredHeight::FData OtherDense, OldQuadrature;
                    Settings.Oversample = Dense == 2 ? 4 : 2;
                    if (!APSCanonicalFilteredHeight::BuildLevel(Frame,CapturedLayout,Result.ComparisonLevel,Settings,OtherDense,Result.Error)) return Result;
                    const auto& S2 = Dense == 2 ? Selected : OtherDense;
                    const auto& S4 = Dense == 4 ? Selected : OtherDense;
                    if (Cancel->load(std::memory_order_relaxed)
                        || !APSCanonicalFilteredHeight::CompareSameLattice(S2,S4,Result.Comparison,Result.Error)) return Result;
                    Result.ExtraDenseCalls = OtherDense.Metadata.ActualHeightSamples;
                    Result.ExtraDenseSeconds = OtherDense.Metadata.PreparationWallSeconds;
                    Settings.HeightFilter = APSCanonicalFilteredHeight::EHeightFilter::Quadrature;
                    Settings.Oversample = 4; // Default option; legacy metadata must still report Oversample=0.
                    Settings.Quadrature = APSCanonicalFilteredHeight::EQuadrature::Three;
                    if (!APSCanonicalFilteredHeight::BuildLevel(Frame,CapturedLayout,Result.ComparisonLevel,Settings,OldQuadrature,Result.Error)
                        || Cancel->load(std::memory_order_relaxed)
                        || !APSCanonicalFilteredHeight::CompareSameLattice(OldQuadrature,S4,Result.QuadratureComparison,Result.Error)) return Result;
                    Result.ExtraQuadratureCalls = OldQuadrature.Metadata.ActualHeightSamples;
                    Result.ExtraQuadratureSeconds = OldQuadrature.Metadata.PreparationWallSeconds;
                }
                Result.TotalWallSeconds = FPlatformTime::Seconds() - Started;
                Result.Valid = !Cancel->load(std::memory_order_relaxed); return Result;
            });
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] PREPARE levels=%d N=1024 centreSpacing=15m terminalWidth=4R cap=120s C=%s U=%s V=%s initialHeightKm=%.6f sameMID=%s; fixture, not saved camera"),
                Layout.Count,*C.ToString(),*U.ToString(),*V.ToString(),HeightKm,*M->GetPathName());
            if (DenseOversample) UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.DenseHeight] PREPARE S=%d comparisons=coarsest:S2vsS4,Q3vsS4 cap=120s; same layout/heightfield/material/light, no automatic quality acceptance"),DenseOversample);
            return true;
        }

    public:
        ~FLease() { Release(); }
        bool Started() const { return bStarted; }
        bool Ready() const { return bReady && !bReleased; }
        const FString& GetError() const { return Failure; }
        bool Validate(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, FString& Error) const
        {
            Error.Reset();
            if (!Ready()) { Error = TEXT("Coverage flight has no ready active lease"); return false; }
            return Check(S, View, Error);
        }
        EResult Prepare(APlanetarySurfaceGenerator* S, const FMinimalViewInfo& View, double Now, FString& Error)
        {
            Error.Reset();
            if (bReleased || !Failure.IsEmpty()) { Error = Failure.IsEmpty() ? TEXT("Coverage lease was released") : Failure; return EResult::Failed; }
            if (!FMath::IsFinite(Now) || (bStarted && Now < LastNow)) return Fail(TEXT("Coverage preparation clock invalid"), Error);
            if (!bStarted && !Begin(S, View, Now, Error)) return Fail(Error, Error);
            LastNow = Now;
            if (!Check(S, View, Error)) return Fail(Error, Error);
            if (bReady) return EResult::Ready;
            if (Now - BeginTime >= PreparationSeconds) return Fail(TEXT("Coverage preparation exceeded120s; no retry/publication"), Error);
            if (Stage == 0)
            {
                if (!Future.IsValid()) return Fail(TEXT("Coverage CPU future missing"), Error);
                if (!Future.IsReady())
                {
                    if (Now - LastProgress >= 5) { LastProgress = Now; UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] preparing elapsed=%.3fs"),Now-BeginTime); }
                    return EResult::Pending;
                }
                const auto& Result = Future.Get();
                if (!Result.Valid || Result.Levels.Num() != Layout.Count || Result.DenseOversample != DenseOversample) return Fail(Result.Error.IsEmpty() ? TEXT("Coverage CPU result incomplete") : Result.Error, Error);
                const auto& C = Result.Comparison;
                if (C.Samples != int64(Layout.Resolution)*Layout.Resolution || !FMath::IsFinite(C.MeanDegrees)
                    || !FMath::IsFinite(C.RmsDegrees) || !FMath::IsFinite(C.P95Degrees) || !FMath::IsFinite(C.MaximumDegrees))
                    return Fail(TEXT("Coverage one-shot comparison readback invalid"), Error);
                if (DenseOversample)
                {
                    const auto& Q = Result.QuadratureComparison;
                    if (Result.ComparisonLevel != Layout.Count - 1 || Q.Samples != C.Samples
                        || !FMath::IsFinite(Q.MeanDegrees) || !FMath::IsFinite(Q.RmsDegrees)
                        || !FMath::IsFinite(Q.P95Degrees) || !FMath::IsFinite(Q.MaximumDegrees)
                        || Result.ExtraDenseCalls <= 0 || Result.ExtraQuadratureCalls <= 0
                        || !FMath::IsFinite(Result.ExtraDenseSeconds) || Result.ExtraDenseSeconds < 0
                        || !FMath::IsFinite(Result.ExtraQuadratureSeconds) || Result.ExtraQuadratureSeconds < 0)
                        return Fail(TEXT("DenseHeight coarsest comparisons/provenance invalid"),Error);
                }
                int64 TotalBytes = 0, TotalCalls = 0;
                for (int32 I = 0; I < Layout.Count; ++I)
                {
                    const auto& Data = Result.Levels[I];
                    const bool FilterMatches = DenseOversample
                        ? Data.Metadata.HeightFilter == APSCanonicalFilteredHeight::EHeightFilter::SharedLattice
                            && Data.Metadata.Oversample == DenseOversample && Data.Metadata.QuadratureOrder == 0
                        : Data.Metadata.HeightFilter == APSCanonicalFilteredHeight::EHeightFilter::Quadrature
                            && Data.Metadata.Oversample == 0 && Data.Metadata.QuadratureOrder == 3;
                    if (Data.Metadata.Level != I || Data.Metadata.InputSignature != ProfileIdentity
                        || !FilterMatches || Data.ByteCount <= 0 || Data.ByteCount > MaximumBytesPerLevel)
                        return Fail(TEXT("Coverage level identity/upload budget changed"), Error);
                    TotalBytes += Data.ByteCount; TotalCalls += Data.Metadata.ActualHeightSamples;
                    Textures[I].Reset(APSCanonicalCoverageTexture::CreateTransient(Data, Error));
                    if (!Textures[I].IsValid()) return Fail(Error, Error);
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] LEVEL%d widthCm=%.9g centreStepCm=%.9g CPU=%.3fs heightCalls=%lld bytes=%lld"),
                        I,Data.Metadata.WidthCm,Data.Metadata.CenterTexelSpacingCm,Data.Metadata.PreparationWallSeconds,Data.Metadata.ActualHeightSamples,Data.ByteCount);
                }
                if (TotalBytes > Slots * MaximumBytesPerLevel) return Fail(TEXT("Coverage total texture budget exceeded"), Error);
                if (!DenseOversample)
                {
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] CPU_READY levels=%d seconds=%.3f cascadeCalls=%lld extraFiveCalls=%lld extraFiveSeconds=%.3f bytes=%lld; convergence level1 n=%lld meanDeg=%.6f rmsDeg=%.6f p95Deg=%.6f maxDeg=%.6f, evidence only not bandlimited/game-performance/visualPASS"),
                        Layout.Count,Result.TotalWallSeconds,TotalCalls,Result.ExtraFiveCalls,Result.ExtraFiveSeconds,TotalBytes,
                        C.Samples,C.MeanDegrees,C.RmsDegrees,C.P95Degrees,C.MaximumDegrees);
                }
                else
                {
                    const auto& Q = Result.QuadratureComparison;
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.DenseHeight] CPU_READY S=%d levels=%d seconds=%.3f cascadeCalls=%lld extraDenseCalls=%lld extraDenseSeconds=%.3f extraQ3Calls=%lld extraQ3Seconds=%.3f bytes=%lld comparisonLevel=%d n=%lld; S2vsS4 meanDeg=%.6f rmsDeg=%.6f p95Deg=%.6f maxDeg=%.6f; Q3vsS4 meanDeg=%.6f rmsDeg=%.6f p95Deg=%.6f maxDeg=%.6f; numerical evidence only, not converged/bandlimited/performance/visualPASS"),
                        DenseOversample,Layout.Count,Result.TotalWallSeconds,TotalCalls,Result.ExtraDenseCalls,Result.ExtraDenseSeconds,
                        Result.ExtraQuadratureCalls,Result.ExtraQuadratureSeconds,TotalBytes,Result.ComparisonLevel,C.Samples,
                        C.MeanDegrees,C.RmsDegrees,C.P95Degrees,C.MaximumDegrees,Q.MeanDegrees,Q.RmsDegrees,Q.P95Degrees,Q.MaximumDegrees);
                }
                Future.Reset(); Stage = 1; Fence.BeginFence(); return EResult::Pending;
            }
            if (!Fence.IsFenceComplete()) return EResult::Pending;
            if (Stage == 1)
            {
                for (int32 I = 0; I < Layout.Count; ++I)
                    if (!Textures[I].IsValid() || !Textures[I]->GetResource() || !Textures[I]->GetResource()->IsInitialized())
                        return Fail(TEXT("Coverage texture missing after upload fence"), Error);
                bBound = true; BoundAvailable = 0;
                Material->SetScalarParameterValue(AvailableName(), 0);
                for (int32 I = 0; I < Slots; ++I) Material->SetTextureParameterValue(TextureName(I), BoundTexture(I));
                for (int32 I = 0; I < VectorCount; ++I) Material->SetVectorParameterValue(VectorName(I), BoundVectors[I]);
                Stage = 2; Fence.BeginFence(); return EResult::Pending;
            }
            if (Stage == 2)
            {
                if (bFlatNormals && (!FlatNormal.IsValid() || !FlatNormal->GetResource() || !FlatNormal->GetResource()->IsInitialized()))
                    return Fail(TEXT("CanonicalFlatNormals engine texture resource is not ready"), Error);
                BoundAvailable = 1; Material->SetScalarParameterValue(AvailableName(), 1);
                if (bFlatNormals)
                {
                    bNormalsBound = true;
                    for (int32 I = 0; I < NormalTextureCount; ++I)
                        Material->SetTextureParameterValue(NormalTextureName(I), FlatNormal.Get());
                    UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.FlatNormals] BIND textures=7 sameMID=%s canonical=1; texture-source isolation only, no visual acceptance"),*Material->GetPathName());
                }
                EnableTime = Now; EnableFrame = GFrameCounter; Stage = 3; Fence.BeginFence(); return EResult::Pending;
            }
            if (Stage != 3) return Fail(TEXT("Coverage preparation state invalid"), Error);
            if (Now - EnableTime < .25 || GFrameCounter - EnableFrame < 2) return EResult::Pending;
            bReady = true;
            UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] READY levels=%d immutableAnchor=1 liveLOD=1 sameMID=%s nearOriginalBelowCm=%.9g; route may now start, no visual acceptance"),
                Layout.Count,*Material->GetPathName(),NormalStart);
            return EResult::Ready;
        }
        bool Release(FString& Error)
        {
            if (bReleased) { Error = ReleaseError; return bReleaseOK; }
            Error.Reset();
            if (!IsInGameThread()) { Error = TEXT("Coverage release must run on GT"); return false; }
            bReleased = true; bReady = false;
            if (Cancellation) Cancellation->store(true,std::memory_order_relaxed);
            if (bNormalsBound && Material.IsValid())
            {
                int32 Conflicts = 0;
                for (int32 I = 0; I < NormalTextureCount; ++I)
                {
                    UTexture* Current = nullptr;
                    if (Material->GetTextureParameterValue(FMaterialParameterInfo(NormalTextureName(I)), Current) && Current == FlatNormal.Get())
                        Material->SetTextureParameterValue(NormalTextureName(I), OldNormalTextures[I].Get());
                    else ++Conflicts;
                }
                if (Conflicts) { Error = FString::Printf(TEXT("CanonicalFlatNormals release preserved%d externally changed textures; "),Conflicts); bReleaseOK = false; }
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage.FlatNormals] RELEASE restored=%d conflicts=%d; only owned texture values, no broad clear"),NormalTextureCount-Conflicts,Conflicts);
            }
            if (bBound && Material.IsValid())
            {
                int32 Conflicts = 0; float Current = -1;
                if (Material->GetScalarParameterValue(FMaterialParameterInfo(AvailableName()),Current) && Current == BoundAvailable)
                    Material->SetScalarParameterValue(AvailableName(),OldAvailable);
                else ++Conflicts;
                for (int32 I = 0; I < Slots; ++I)
                {
                    UTexture* CurrentTexture = nullptr;
                    if (Material->GetTextureParameterValue(FMaterialParameterInfo(TextureName(I)),CurrentTexture) && CurrentTexture == BoundTexture(I))
                        Material->SetTextureParameterValue(TextureName(I),OldTextures[I].Get());
                    else ++Conflicts;
                }
                for (int32 I = 0; I < VectorCount; ++I)
                {
                    FLinearColor CurrentVector;
                    if (Material->GetVectorParameterValue(FMaterialParameterInfo(VectorName(I)),CurrentVector) && CurrentVector == BoundVectors[I])
                        Material->SetVectorParameterValue(VectorName(I),OldVectors[I]);
                    else ++Conflicts;
                }
                if (Conflicts) { Error += FString::Printf(TEXT("Coverage release preserved%d externally changed owned-name values"),Conflicts); bReleaseOK = false; }
                UE_LOG(LogTemp,Display,TEXT("[APS.CanonicalCoverage] RELEASE restoredOnlyOwnedEffectiveValues=22 conflicts=%d; no broad clear, equivalent explicit overrides may remain"),Conflicts);
            }
            Future.Reset(); Cancellation.Reset(); // Nonblocking; worker retains its monotonic cancellation token, no UObject/this.
            for (int32 I = 0; I < Slots; ++I) { Textures[I].Reset(); OldTextures[I].Reset(); }
            for (int32 I = 0; I < NormalTextureCount; ++I) OldNormalTextures[I].Reset();
            FlatNormal.Reset();
            Material.Reset(); ReleaseError = Error; return bReleaseOK;
        }
        void Release()
        {
            FString Error;
            if (!Release(Error)) UE_LOG(LogTemp,Error,TEXT("[APS.CanonicalCoverage] cleanup failed: %s"),*Error);
        }
    };
}
#endif

#pragma once

#include "APS_ALPHA/Core/Planetary/APSSharedWaterMaterial.h"
#include "APS_ALPHA/Core/Planetary/APSSharedLavaMaterial.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Read-only assertions on production-assigned instances, never a candidate factory.
namespace APSProductionSharedLiquidAssertions
{
inline bool Validate(UMaterialInstanceDynamic* Material, EAPSPlanetLiquidType Type,
    USceneComponent* Frame, double PresentationScale, float ExpectedContext,
    FString& Evidence, FString& Error)
{
    Evidence.Reset(); Error.Reset();
    const TCHAR* ExpectedParent = Type == EAPSPlanetLiquidType::Water ? APSSharedWaterMaterial::TemplatePath()
        : Type == EAPSPlanetLiquidType::Ammonia ? APSSharedAmmoniaMaterial::TemplatePath()
        : Type == EAPSPlanetLiquidType::Lava ? APSSharedLavaMaterial::TemplatePath() : nullptr;
    auto Fail = [&Error](const TCHAR* Why) { Error = Why; return false; };
    if (!ExpectedParent || !IsValid(Material) || !IsValid(Material->Parent.Get()) || !IsValid(Frame)
        || Material->Parent->GetPathName() != ExpectedParent
        || !(Type == EAPSPlanetLiquidType::Lava ? APSSharedLavaMaterial::IsSharedStack(Material)
            : APSSharedAmmoniaMaterial::IsSharedStack(Material)) || Material->GetBlendMode() != BLEND_Masked)
        return Fail(TEXT("actual production MID has the wrong exact family parent/master/blend"));
    if (Type == EAPSPlanetLiquidType::Lava)
    {
        float SavedBrightness = -1.0f, ActualBrightness = -1.0f;
        FLinearColor SavedEmission, ActualEmission;
        if (!Material->Parent->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), SavedBrightness)
            || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Brightness")), ActualBrightness)
            || !FMath::IsFinite(ActualBrightness) || SavedBrightness != ActualBrightness
            || !Material->Parent->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), SavedEmission)
            || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("EmissiveColor")), ActualEmission)
            || SavedEmission != ActualEmission)
            return Fail(TEXT("production Lava changed the saved emission/brightness authority"));
    }
    float Context = -1.0f;
    if (!Material->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("APS_UsePresentationWaterMask")), Context)
        || Context != ExpectedContext) return Fail(TEXT("production liquid context differs from this draw path"));
    UWorld* World = Frame->GetWorld();
    FMaterialResource* Resource = World ? Material->GetMaterialResource(World->GetFeatureLevel()) : nullptr;
    const FMaterialShaderMap* Map = Resource ? Resource->GetGameThreadShaderMap() : nullptr;
    if (!Resource || !Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num()
        || !Map || !Map->GetMeshShaderMap(&FLocalVertexFactory::StaticType))
        return Fail(TEXT("production material is not complete error-free LocalVF"));
    const FVector Center = Frame->GetComponentLocation();
    const FVector Scale = Frame->GetComponentScale();
    const FQuat Rotation = Frame->GetComponentQuat().GetNormalized();
    const double UniformScale = Scale.GetAbsMax();
    const double EffectiveScale = UniformScale * PresentationScale;
    if (Center.ContainsNaN() || Scale.ContainsNaN() || Rotation.ContainsNaN()
        || Scale.X <= 0.0 || Scale.Y <= 0.0 || Scale.Z <= 0.0
        || UniformScale - Scale.GetAbsMin() > UniformScale * 1.e-5
        || !FMath::IsFinite(EffectiveScale) || EffectiveScale <= 0.0)
        return Fail(TEXT("actual production frame has invalid scale/transform"));
    const TCHAR* Names[] = {TEXT("APS_SharedPlanetCenter"), TEXT("APS_SharedInverseScale"),
        TEXT("APS_SharedAxisX"), TEXT("APS_SharedAxisY"), TEXT("APS_SharedAxisZ")};
    const FVector ExpectedXYZ[] = {Center, FVector(1.0 / EffectiveScale, 0, 0),
        Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ()};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Names); ++I)
    {
        FVector4 Value;
        const double Tolerance = I == 0 ? 0.01 : I == 1 ? FMath::Max(1.e-12, ExpectedXYZ[I].X * 1.e-9) : 1.e-9;
        if (!Material->GetDoubleVectorParameterValue(FHashedMaterialParameterInfo(Names[I]), Value)
            || !FMath::IsFinite(Value.X) || !FMath::IsFinite(Value.Y) || !FMath::IsFinite(Value.Z) || !FMath::IsFinite(Value.W)
            || !FVector(Value.X, Value.Y, Value.Z).Equals(ExpectedXYZ[I], Tolerance) || FMath::Abs(Value.W) > 1.e-12)
        {
            Error = FString::Printf(TEXT("production frame parameter %s is missing/stale"), Names[I]);
            return false;
        }
    }
    // Opt-in evidence that the precise UV shader receives the composed rows,
    // not the identity defaults. Verify high and residual terms separately.
    if (Type == EAPSPlanetLiquidType::Lava
        && FParse::Param(FCommandLine::Get(), TEXT("APSVerifyLavaDetailPrecision")))
    {
        const TCHAR* HighNames[] = {TEXT("APS_SharedDetailRowXHigh"), TEXT("APS_SharedDetailRowYHigh"), TEXT("APS_SharedDetailRowZHigh")};
        const TCHAR* LowNames[] = {TEXT("APS_SharedDetailRowXLow"), TEXT("APS_SharedDetailRowYLow"), TEXT("APS_SharedDetailRowZLow")};
        const FVector Axes[] = {Rotation.GetAxisX(), Rotation.GetAxisY(), Rotation.GetAxisZ()};
        for (int32 I = 0; I < 3; ++I)
        {
            const FVector Row = Axes[I] * (1.0 / EffectiveScale);
            const FLinearColor H(float(Row.X), float(Row.Y), float(Row.Z), 0.0f);
            const FLinearColor L(float(Row.X-double(H.R)), float(Row.Y-double(H.G)), float(Row.Z-double(H.B)), 0.0f);
            FLinearColor ActualH, ActualL;
            if (!Material->GetVectorParameterValue(FHashedMaterialParameterInfo(HighNames[I]), ActualH)
                || !Material->GetVectorParameterValue(FHashedMaterialParameterInfo(LowNames[I]), ActualL)
                || ActualH != H || ActualL != L)
                return Fail(TEXT("precise lava UV row/residual does not match actual production frame"));
        }
    }
    Evidence = FString::Printf(TEXT("material=%s parent=%s base=%s context=%.0f center=%s inverseScale=%.17g axesMatchActual=1 complete=1 localVF=1"),
        *Material->GetPathName(), *Material->Parent->GetPathName(), *Material->GetMaterial()->GetPathName(),
        Context, *Center.ToCompactString(), 1.0 / EffectiveScale);
    if (Type == EAPSPlanetLiquidType::Lava && FParse::Param(FCommandLine::Get(), TEXT("APSVerifyLavaDetailPrecision")))
        Evidence += TEXT(" preciseDetailRowsMatchActual=1");
    return true;
}
}

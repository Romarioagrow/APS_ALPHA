#pragma once

#if WITH_EDITOR
#include "APSPlanetSurfaceDetailMaterial.h"
#include "Materials/MaterialExpressionTransformPosition.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"

/** Texture-only presentation for one closed PLANET component, never WorldScape rings. */
namespace APSOrbitalTerrainDetail
{
    struct FResult
    {
        UMaterialExpression* Color = nullptr;
        UMaterialExpression* Normal = nullptr;
    };

    inline FResult Build(UMaterial* M, UMaterialExpression* Palette, UMaterialExpression* BaseNormal)
    {
        using namespace APSPlanetSurfaceDetail;
        const FTextures Rock = TexturesFor(EAPSPlanetSurfaceArchetype::Rocky);
        UTexture2D* Albedo = LoadObject<UTexture2D>(nullptr, Rock.Albedo);
        UTexture2D* Normal = LoadObject<UTexture2D>(nullptr, Rock.Normal);
        if (!Albedo || !Normal) return {};

        // TransformPosition preserves the component's LWC transform before the
        // normalized float domain. No actor/bounds centre, camera phase, spherical
        // UV seam, metre-scale tiling or streamed-patch coordinates are involved.
        auto* WorldPosition = Node<UMaterialExpressionWorldPosition>(M);
        WorldPosition->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
        auto* LocalPosition = Node<UMaterialExpressionTransformPosition>(M);
        LocalPosition->TransformSourceType = TRANSFORMPOSSOURCE_World;
        LocalPosition->TransformType = TRANSFORMPOSSOURCE_Local;
        LocalPosition->Input.Connect(0, WorldPosition);
        auto* Direction = Node<UMaterialExpressionNormalize>(M);
        Direction->VectorInput.Connect(0, LocalPosition);
        auto* Seed = Node<UMaterialExpressionVectorParameter>(M);
        Seed->ParameterName = TEXT("OrbitalSeedOffset");
        Seed->DefaultValue = FLinearColor(3.1f, 7.7f, 11.3f, 0);
        auto* Scale = Scalar(M, TEXT("OrbitalTextureFrequency"), 96.0f);
        auto* RockMix = Scalar(M, TEXT("OrbitalRockMix"), 0.45f);
        auto* NormalStrength = Scalar(M, TEXT("OrbitalTextureNormalStrength"), 0.30f);
        auto* ColorStrength = Scalar(M, TEXT("OrbitalTextureColorStrength"), 0.75f);
        auto* Detail = Node<UMaterialExpressionCustom>(M);
        Detail->Description = TEXT("APS orbital native textures: two angular bands, 12 samples");
        Detail->OutputType = CMOT_Float4;
        Detail->Inputs.Reset();
        auto Input = [Detail](const FString& Name, UMaterialExpression* Source)
        {
            FCustomInput I; I.InputName = *Name; I.Input.Connect(0, Source); Detail->Inputs.Add(I);
        };
        Input(TEXT("N"), Direction);
        Input(TEXT("RockMix"), RockMix);
        Input(TEXT("NormalStrength"), NormalStrength);
        Input(TEXT("ColorStrength"), ColorStrength);
        for (int32 Band = 0; Band < 2; ++Band)
        {
            auto* Frequency = Node<UMaterialExpressionMultiply>(M);
            Frequency->A.Connect(0, Scale);
            Frequency->ConstB = Band == 0 ? 1.0f : 0.1873f;
            auto* Position = Node<UMaterialExpressionMultiply>(M);
            Position->A.Connect(0, Direction); Position->B.Connect(0, Frequency);
            auto* Seeded = Node<UMaterialExpressionAdd>(M);
            Seeded->A.Connect(0, Position); Seeded->B.Connect(0, Seed);
            for (int32 Axis = 0; Axis < 3; ++Axis)
            {
                auto* UV = Mask(M, Seeded, Axis);
                for (int32 Kind = 0; Kind < 2; ++Kind)
                {
                    const bool bNormal = Kind == 0;
                    auto* Sample = Node<UMaterialExpressionTextureSampleParameter2D>(M);
                    Sample->ParameterName = Band == 0
                        ? (bNormal ? TEXT("OrbitalPrimaryNormal") : TEXT("OrbitalPrimaryAlbedo"))
                        : (bNormal ? TEXT("OrbitalRockNormal") : TEXT("OrbitalRockAlbedo"));
                    Sample->Group = TEXT("APS Orbital Texture Detail");
                    Sample->Texture = bNormal ? Normal : Albedo;
                    Sample->SamplerType = bNormal ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
                    Sample->SamplerSource = SSM_Wrap_WorldGroupSettings;
                    Sample->AutomaticViewMipBias = false;
                    // Continuous angular UVs + hardware wrapping: implicit mip
                    // derivatives remain valid across repeats and cube-face edges.
                    Sample->Coordinates.Connect(0, UV);
                    Input(FString::Printf(TEXT("%s%d%d"), bNormal ? TEXT("B") : TEXT("C"), Band, Axis), Sample);
                }
            }
        }
        Detail->Code = TEXT(R"HLSL(
float3 n = normalize(N);
float3 w = pow(abs(n), 4.0);
w /= max(w.x + w.y + w.z, 1e-6);
float mixValue = saturate(RockMix);
float2 gx = lerp(B00.xy / max(B00.z, 0.35), B10.xy / max(B10.z, 0.35), mixValue);
float2 gy = lerp(B01.xy / max(B01.z, 0.35), B11.xy / max(B11.z, 0.35), mixValue);
float2 gz = lerp(B02.xy / max(B02.z, 0.35), B12.xy / max(B12.z, 0.35), mixValue);
float3 gradient = float3(0, gx.x, gx.y) * w.x * sign(n.x)
                + float3(gy.x, 0, gy.y) * w.y * sign(n.y)
                + float3(gz.x, gz.y, 0) * w.z * sign(n.z);
gradient -= n * dot(gradient, n);
float3 primary = C00 * w.x + C01 * w.y + C02 * w.z;
float3 rock = C10 * w.x + C11 * w.y + C12 * w.z;
float luminance = dot(lerp(primary, rock, mixValue), float3(0.2126, 0.7152, 0.0722));
// Native texture value modulates our resolved palette; texture hue never replaces it.
float colorFactor = lerp(1.0, clamp(0.65 + luminance, 0.65, 1.35), saturate(ColorStrength));
return float4(gradient * saturate(NormalStrength), colorFactor);
)HLSL");
        auto* Gradient = Node<UMaterialExpressionComponentMask>(M);
        Gradient->Input.Connect(0, Detail); Gradient->R = Gradient->G = Gradient->B = true; Gradient->A = false;
        auto* ColorFactor = Node<UMaterialExpressionComponentMask>(M);
        ColorFactor->Input.Connect(0, Detail); ColorFactor->R = ColorFactor->G = ColorFactor->B = false; ColorFactor->A = true;
        auto* Color = Node<UMaterialExpressionMultiply>(M);
        Color->A.Connect(0, Palette); Color->B.Connect(0, ColorFactor);
        auto* WorldGradient = Node<UMaterialExpressionTransform>(M);
        WorldGradient->TransformSourceType = TRANSFORMSOURCE_Local;
        WorldGradient->TransformType = TRANSFORM_World;
        WorldGradient->Input.Connect(0, Gradient);
        auto* ResultNormal = Node<UMaterialExpressionCustom>(M);
        ResultNormal->Description = TEXT("APS orbital scale-independent world normal");
        ResultNormal->OutputType = CMOT_Float3;
        ResultNormal->Inputs.Reset();
        for (const auto& Pair : { TPair<const TCHAR*, UMaterialExpression*>(TEXT("LocalGradient"), Gradient),
            {TEXT("WorldGradient"), WorldGradient}, {TEXT("BaseNormal"), BaseNormal} })
        {
            FCustomInput I; I.InputName = Pair.Key; I.Input.Connect(0, Pair.Value); ResultNormal->Inputs.Add(I);
        }
        ResultNormal->Code = TEXT(R"HLSL(
float3 n = normalize(BaseNormal);
float3 g = WorldGradient * (length(LocalGradient) / max(length(WorldGradient), 1e-6));
g -= n * dot(g, n);
return normalize(n + g);
)HLSL");
        return {Color, ResultNormal};
    }
}
#endif

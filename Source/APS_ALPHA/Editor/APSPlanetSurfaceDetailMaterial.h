#pragma once

#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDDX.h"
#include "Materials/MaterialExpressionDDY.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

/** Material detail only: no terrain displacement, patch UVs or camera-dependent coordinates. */
namespace APSPlanetSurfaceDetail
{
	struct FLayer
	{
		UMaterialExpression* ColorMultiplier = nullptr;
		UMaterialExpression* NormalPerturbation = nullptr;
	};

	struct FTextures
	{
		const TCHAR* Albedo;
		const TCHAR* Normal;
		float SizeCm;
	};

	inline FTextures TexturesFor(EAPSPlanetSurfaceArchetype Archetype)
	{
		switch (Archetype)
		{
		case EAPSPlanetSurfaceArchetype::Cryogenic:
			return {TEXT("/WorldScape/Ressources/Textures/Snow/T_Snow_Albedo"),
				TEXT("/WorldScape/Ressources/Textures/Snow/T_Snow_N"), 180.0f};
		case EAPSPlanetSurfaceArchetype::Desert:
			return {TEXT("/WorldScape/Ressources/Textures/Sand/T_Sable_D"),
				TEXT("/WorldScape/Ressources/Textures/Sand/T_Sable_N"), 240.0f};
		default:
			return {TEXT("/WorldScape/Ressources/Textures/Rock/T_Desert_Albedo1"),
				TEXT("/WorldScape/Ressources/Textures/Rock/T_Desert_Normal"), 200.0f};
		}
	}

	template<class T> T* Node(UMaterial* Material)
	{
		return CastChecked<T>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, T::StaticClass(), 4100, 0));
	}

	inline UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value)
	{
		auto* P = Node<UMaterialExpressionScalarParameter>(M);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		P->Group = TEXT("APS Surface Texture Detail");
		return P;
	}

	inline UMaterialExpressionComponentMask* Mask(UMaterial* M, UMaterialExpression* Input, int32 Axis)
	{
		auto* P = Node<UMaterialExpressionComponentMask>(M);
		P->Input.Connect(0, Input);
		// X projection = YZ, Y projection = XZ, Z projection = XY.
		P->R = Axis != 0; P->G = Axis != 1; P->B = Axis != 2; P->A = false;
		return P;
	}

	inline FLayer Build(UMaterial* M, UMaterialExpression* RootRelativePosition,
		UMaterialExpression* Seed, UMaterialExpression* GeometricNormal,
		UMaterialExpression* ShadingNormal, UMaterialExpression* Fade,
		UMaterialExpression* OrbitalBlend)
	{
		const FTextures Textures = TexturesFor(EAPSPlanetSurfaceArchetype::Rocky);
		UTexture2D* Albedo = LoadObject<UTexture2D>(nullptr, Textures.Albedo);
		UTexture2D* Normal = LoadObject<UTexture2D>(nullptr, Textures.Normal);
		if (!Albedo || !Normal) return {};
		auto* Size = Scalar(M, TEXT("SurfaceDetailSizeCm"), Textures.SizeCm);
		auto* Strength = Scalar(M, TEXT("SurfaceDetailNormalStrength"), 0.38f);
		auto* ColorStrength = Scalar(M, TEXT("SurfaceDetailColorStrength"), 0.38f);
		auto* Detail = Node<UMaterialExpressionCustom>(M);
		Detail->Description = TEXT("APS planet-anchored triplanar texture detail");
		Detail->OutputType = CMOT_Float4;
		Detail->Inputs.Reset();
		auto Input = [Detail](const TCHAR* Name, UMaterialExpression* Source)
		{
			FCustomInput I; I.InputName = Name; I.Input.Connect(0, Source); Detail->Inputs.Add(I);
		};
		Input(TEXT("N"), GeometricNormal);
		Input(TEXT("ShadeN"), ShadingNormal);
		Input(TEXT("Fade"), Fade);
		Input(TEXT("OrbitalBlend"), OrbitalBlend);
		Input(TEXT("Strength"), Strength);
		Input(TEXT("ColorStrength"), ColorStrength);
		for (int32 Band = 0; Band < 2; ++Band)
		{
			auto* BandSize = Node<UMaterialExpressionMultiply>(M);
			BandSize->A.Connect(0, Size);
			BandSize->ConstB = Band == 0 ? 1.0f : 5.371f;
			auto* Position = Node<UMaterialExpressionDivide>(M);
			Position->A.Connect(0, RootRelativePosition);
			Position->B.Connect(0, BandSize);
			auto* Seeded = Node<UMaterialExpressionAdd>(M);
			Seeded->A.Connect(0, Position); Seeded->B.Connect(0, Seed);
			// Frac runs BEFORE float sampler coordinates, preserving sub-metre phase on
			// LWC planet coordinates. Derivatives must bypass Frac, or every wrap seam
			// spuriously selects a coarse mip and draws a moving blurred grid.
			auto* Wrapped = Node<UMaterialExpressionFrac>(M);
			Wrapped->Input.Connect(0, Seeded);
			auto* Dx = Node<UMaterialExpressionDDX>(M); Dx->Value.Connect(0, Position);
			auto* Dy = Node<UMaterialExpressionDDY>(M); Dy->Value.Connect(0, Position);
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				auto* UV = Mask(M, Wrapped, Axis);
				auto* UVdx = Mask(M, Dx, Axis);
				auto* UVdy = Mask(M, Dy, Axis);
				for (int32 Kind = 0; Kind < (Band == 0 ? 2 : 1); ++Kind)
				{
					auto* Sample = Node<UMaterialExpressionTextureSampleParameter2D>(M);
					const bool bNormal = Kind == 0;
					Sample->ParameterName = bNormal ? TEXT("SurfaceDetailNormal") : TEXT("SurfaceDetailAlbedo");
					Sample->Group = TEXT("APS Surface Texture Detail");
					Sample->Texture = bNormal ? Normal : Albedo;
					Sample->SamplerType = bNormal ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
					Sample->SamplerSource = SSM_Wrap_WorldGroupSettings;
					Sample->AutomaticViewMipBias = false;
					Sample->MipValueMode = TMVM_Derivative;
					Sample->Coordinates.Connect(0, UV);
					Sample->CoordinatesDX.Connect(0, UVdx);
					Sample->CoordinatesDY.Connect(0, UVdy);
					Input(*FString::Printf(TEXT("%s%d%d"), bNormal ? TEXT("B") : TEXT("C"), Band, Axis), Sample);
				}
			}
		}
		Detail->Code = TEXT(R"HLSL(
float3 n = normalize(N);
float3 w = pow(abs(n), 4.0);
w /= max(w.x + w.y + w.z, 1e-6);
float2 gx = B00.xy / max(B00.z, 0.25) * 0.75 + B10.xy / max(B10.z, 0.25) * 0.25;
float2 gy = B01.xy / max(B01.z, 0.25) * 0.75 + B11.xy / max(B11.z, 0.25) * 0.25;
float2 gz = B02.xy / max(B02.z, 0.25) * 0.75 + B12.xy / max(B12.z, 0.25) * 0.25;
float3 gradient = float3(0, gx.x, gx.y) * w.x * sign(n.x)
                + float3(gy.x, 0, gy.y) * w.y * sign(n.y)
                + float3(gz.x, gz.y, 0) * w.z * sign(n.z);
float3 sn = normalize(ShadeN);
gradient -= sn * dot(gradient, sn);
float visible = saturate(Fade) * (1.0 - saturate(OrbitalBlend));
float3 color = C00 * w.x + C01 * w.y + C02 * w.z;
// Modulate the authored planetary palette, never replace it with the texture hue.
float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
float colorFactor = lerp(1.0, clamp(0.65 + luminance, 0.65, 1.35), saturate(ColorStrength) * visible);
return float4(gradient * saturate(Strength) * visible, colorFactor);
)HLSL");
		auto* Color = Node<UMaterialExpressionComponentMask>(M);
		Color->Input.Connect(0, Detail); Color->R = Color->G = Color->B = false; Color->A = true;
		auto* Gradient = Node<UMaterialExpressionComponentMask>(M);
		Gradient->Input.Connect(0, Detail); Gradient->R = Gradient->G = Gradient->B = true; Gradient->A = false;
		return {Color, Gradient};
	}
}
#endif

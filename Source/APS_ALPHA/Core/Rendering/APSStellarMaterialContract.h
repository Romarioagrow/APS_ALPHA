#pragma once

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace APSStellarMaterialContract
{
	inline constexpr const TCHAR* ActorBaseObjectPath =
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_SUN.M_SpectralStarMat_SUN");
	inline constexpr const TCHAR* HismBaseObjectPath =
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM.M_SpectralStarMat_HISM");
	inline constexpr const TCHAR* WorldGridBaseObjectPath =
		TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");

	inline UMaterial* LoadCanonicalBase(const TCHAR* ObjectPath)
	{
		return LoadObject<UMaterial>(nullptr, ObjectPath);
	}

	inline UMaterial* GetBaseMaterial(UMaterialInterface* Material)
	{
		return IsValid(Material) ? Material->GetBaseMaterial() : nullptr;
	}

	inline bool HasExactBase(UMaterialInterface* Material, const TCHAR* ExpectedObjectPath)
	{
		const UMaterial* BaseMaterial = GetBaseMaterial(Material);
		return IsValid(BaseMaterial) && BaseMaterial->GetPathName() == ExpectedObjectPath;
	}

	inline bool UsesWorldGrid(UMaterialInterface* Material)
	{
		return HasExactBase(Material, WorldGridBaseObjectPath);
	}
}

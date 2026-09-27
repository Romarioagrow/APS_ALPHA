#pragma once

#include "CoreMinimal.h"

/**
 * Shared runtime/asset contract for the landing-screen nebula master.
 *
 * The first eight per-instance custom-data channels deliberately stay stable so
 * future menu compositions and generated-space vistas can reuse the material
 * without cloning its shader graph.
 */
namespace APSMainMenuNebulaMaterial
{
	inline constexpr const TCHAR* ObjectPath =
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_APS_MainMenuNebula_SoftCloud.M_APS_MainMenuNebula_SoftCloud");
	inline constexpr const TCHAR* PackagePath =
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro");
	inline constexpr const TCHAR* AssetName = TEXT("M_APS_MainMenuNebula_SoftCloud");

	constexpr int32 CustomDataFloatCount = 8;
	constexpr int32 TintR = 0;
	constexpr int32 TintG = 1;
	constexpr int32 TintB = 2;
	constexpr int32 Opacity = 3;
	constexpr int32 Seed = 4;
	constexpr int32 PatternScale = 5;
	constexpr int32 Warmth = 6;
	constexpr int32 Detail = 7;
}

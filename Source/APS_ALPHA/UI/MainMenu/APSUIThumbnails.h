#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"

/** Baked editor-style thumbnails of the civilization start Blueprints, shown by the main menu. */
namespace APSUIThumbnails
{
	inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/UI/Thumbnails");

	/** Texture object path for a Blueprint package. The hash keeps duplicate asset names apart. */
	inline FString TexturePathForBlueprintPackage(const FString& BlueprintPackageName)
	{
		const FString AssetName = FPackageName::GetShortName(BlueprintPackageName);
		const FString TextureName = FString::Printf(TEXT("T_Thumb_%s_%08x"),
			*AssetName, FCrc::StrCrc32(*BlueprintPackageName));
		return FString::Printf(TEXT("%s/%s.%s"), Folder, *TextureName, *TextureName);
	}
}

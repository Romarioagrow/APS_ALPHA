#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"

class UClass;
class UTexture2D;
struct FSlateBrush;

/** Baked editor-style thumbnails of the civilization start Blueprints, shown by the main menu. */
namespace APSUIThumbnails
{
	inline constexpr const TCHAR* Folder = TEXT("/Game/APS/APS_ALPHA/UI/Thumbnails");

	/** UAPSStartAssetFilter: which start Blueprints the menu hides. Lives next to the thumbnails. */
	inline constexpr const TCHAR* StartAssetFilterPath =
		TEXT("/Game/APS/APS_ALPHA/UI/Thumbnails/DA_StartAssetFilter.DA_StartAssetFilter");

	/** Texture object path for a Blueprint package. The hash keeps duplicate asset names apart. */
	inline FString TexturePathForBlueprintPackage(const FString& BlueprintPackageName)
	{
		const FString AssetName = FPackageName::GetShortName(BlueprintPackageName);
		const FString TextureName = FString::Printf(TEXT("T_Thumb_%s_%08x"),
			*AssetName, FCrc::StrCrc32(*BlueprintPackageName));
		return FString::Printf(TEXT("%s/%s.%s"), Folder, *TextureName, *TextureName);
	}

	/**
	 * The baked thumbnail of a Blueprint actor class as a Slate brush (Rio 02.10: object pages and the map list show the
	 * real look of a station, a headquarters or a ship). Loaded once and kept for the session; null for native classes and
	 * Blueprints without a thumbnail.
	 */
	APS_ALPHA_API const FSlateBrush* FindBrush(const UClass* ActorClass);

	/**
	 * Rio 05.10 ("all the ship icons are too small"): points a brush at a baked thumbnail, cropped to what it shows. The
	 * icons are squares around the object with a margin, so a long hull is a thin band across the middle; the brush's UV
	 * region keeps the pixels with alpha plus a thin margin, and its image size follows, so a ScaleToFit box fills with
	 * the object itself. Reads the texture's source pixels, which uncooked runs keep (the editor, -game from it); a cooked
	 * build shows the whole square as before. The crop is worked out once per texture.
	 */
	APS_ALPHA_API void InitBrush(FSlateBrush& Brush, UTexture2D* Texture);
}

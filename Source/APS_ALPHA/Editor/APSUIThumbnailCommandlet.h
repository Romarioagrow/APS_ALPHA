#pragma once

#include "Commandlets/Commandlet.h"
#include "APSUIThumbnailCommandlet.generated.h"

/**
 * Renders editor-style thumbnails of the civilization start Blueprints (pilot, ships, stations,
 * headquarters, shipyards) into UI textures in APSUIThumbnails::Folder.
 * Needs a renderer: run with -AllowCommandletRendering.
 * Options: -Size=256, -Only=<package substring>, -KeepBackground (editor sky and floor, no cut-out),
 * -PngDir=<dir> (also write review PNGs), -SkipExisting (only Blueprints without a thumbnail).
 */
UCLASS()
class APS_ALPHA_API UAPSUIThumbnailCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAPSUIThumbnailCommandlet();
	virtual int32 Main(const FString& Params) override;
};

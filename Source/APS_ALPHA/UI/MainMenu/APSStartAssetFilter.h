#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "APSStartAssetFilter.generated.h"

/**
 * Start Blueprints the civilization menu leaves out, on top of its basic rule (a start asset must
 * reference a mesh). ExcludedClasses is curated by hand; ClassesWithoutVisuals is rewritten by the
 * thumbnail bake (-run=APSUIThumbnail) for Blueprints whose preview shows no geometry.
 */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSStartAssetFilter : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Hand-picked Blueprints that are not start assets (props, buildings, placeholders). */
	UPROPERTY(EditAnywhere, Category = "Start Menu")
	TArray<TSoftClassPtr<AActor>> ExcludedClasses;

	/** Written by the thumbnail bake: Blueprints whose preview renders nothing. */
	UPROPERTY(VisibleAnywhere, Category = "Start Menu")
	TArray<TSoftClassPtr<AActor>> ClassesWithoutVisuals;

	bool Hides(const FSoftObjectPath& ClassPath) const;
};

#pragma once

#include "CoreMinimal.h"

namespace APSStellarPointRasterRecipe
{
	// Preview and gameplay retain different photometry, but use the same circular
	// raster footprint. Do not apply this to the separate, accepted corona master.
	inline bool EnableForAllPointProfiles(FString& Code)
	{
		FString Updated = Code;
		const auto Replace = [&Updated](const TCHAR* From, const TCHAR* To, int32 Expected)
		{
			return Updated.ReplaceInline(From, To, ESearchCase::CaseSensitive) == Expected;
		};
		if (!Replace(TEXT("float gameplayProfile = saturate(GameplayPointProfile) * (1.0 - shellMode);"),
			TEXT("float gameplayProfile = saturate(GameplayPointProfile) * (1.0 - shellMode);\nfloat rasterProfile = 1.0 - shellMode;"), 1)
			|| !Replace(TEXT("if (gameplayProfile > 0.0"), TEXT("if (rasterProfile > 0.0"), 2)
			|| !Replace(TEXT("dot(pointQ, pointQ), gameplayProfile"), TEXT("dot(pointQ, pointQ), rasterProfile"), 1)
			|| !Replace(TEXT("roundEnvelope * projectionValid, gameplayProfile"), TEXT("roundEnvelope * projectionValid, rasterProfile"), 1)
			|| !Replace(TEXT("filteredLobes.x * edgeFade, gameplayProfile"), TEXT("filteredLobes.x * edgeFade, rasterProfile"), 1)
			|| !Replace(TEXT("filteredLobes.y * edgeFade, gameplayProfile"), TEXT("filteredLobes.y * edgeFade, rasterProfile"), 1))
		{
			return false;
		}
		Code = MoveTemp(Updated);
		return true;
	}
}

#include "APSFixStarHISMMaterialCommandlet.h"

#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSStarMaterialFix, Log, All);

UAPSFixStarHISMMaterialCommandlet::UAPSFixStarHISMMaterialCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSFixStarHISMMaterialCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const TArray<FString> PackagePaths{
		TEXT("/Game/APS/APS_ALPHA/Assets/Materials/Astro/M_SpectralStarMat_HISM"),
		TEXT("/Game/APS/Materials/Astro/M_SpectralStarMat_HISM")
	};

	int32 ChangedCount = 0;
	int32 AlreadyStableCount = 0;
	int32 ErrorCount = 0;
	for (const FString& PackagePath : PackagePaths)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("Could not load %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		if (!Material->DitheredLODTransition && !Material->DitherOpacityMask)
		{
			UE_LOG(LogAPSStarMaterialFix, Display, TEXT("Already stable: %s"), *ObjectPath);
			++AlreadyStableCount;
			continue;
		}

		Material->Modify();
		Material->DitheredLODTransition = false;
		Material->DitherOpacityMask = false;
		Material->PostEditChange();

		UPackage* Package = Material->GetOutermost();
		Package->SetDirtyFlag(true);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
		{
			UE_LOG(LogAPSStarMaterialFix, Error, TEXT("Could not save %s"), *Filename);
			++ErrorCount;
			continue;
		}

		UE_LOG(LogAPSStarMaterialFix, Display,
			TEXT("Disabled Dithered LOD Transition and Dither Opacity Mask: %s"), *ObjectPath);
		++ChangedCount;
	}

	UE_LOG(LogAPSStarMaterialFix, Display,
		TEXT("Complete: changed=%d already-stable=%d errors=%d targeted=%d"),
		ChangedCount, AlreadyStableCount, ErrorCount, PackagePaths.Num());
	return ErrorCount == 0 ? 0 : 1;
#else
	UE_LOG(LogAPSStarMaterialFix, Error, TEXT("This maintenance commandlet requires an Editor build."));
	return 1;
#endif
}

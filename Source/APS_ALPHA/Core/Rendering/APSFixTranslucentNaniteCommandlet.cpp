#include "APSFixTranslucentNaniteCommandlet.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSRenderFix, Log, All);

UAPSFixTranslucentNaniteCommandlet::UAPSFixTranslucentNaniteCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSFixTranslucentNaniteCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// Exact copies referenced by the current project. UE 5.4 supports only opaque and
	// masked Nanite sections, so these glass-bearing meshes must use their fallback proxy.
	const TArray<FString> PackagePaths{
		TEXT("/Game/APS/APS_ALPHA/Assets/Meshes/SM_MERGED_StaticMeshActor_2"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Meshes/SM_MERGED_StaticMeshActor_124"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Meshes/SM_MERGED_StaticMeshActor_388"),
		TEXT("/Game/APS/APS_ALPHA/Assets/Meshes/SM_MERGED_STATION_RING"),
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_2"),
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_124"),
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_388"),
		TEXT("/Game/APS/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_STATION_RING"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_BP_SplineElevator_Vert36_2"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_2"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_124"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_337"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_StaticMeshActor_388"),
		TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_STATION_RING"),
		TEXT("/Game/missiontominerva/Geometries/KB3D_MTM_BldgLgTerraformer_A_StairsB")
	};

	int32 ChangedCount = 0;
	int32 AlreadyDisabledCount = 0;
	int32 ErrorCount = 0;

	for (const FString& PackagePath : PackagePaths)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!Mesh)
		{
			UE_LOG(LogAPSRenderFix, Error, TEXT("Could not load %s"), *ObjectPath);
			++ErrorCount;
			continue;
		}

		bool bHasUnsupportedBlend = false;
		for (const FStaticMaterial& StaticMaterial : Mesh->GetStaticMaterials())
		{
			const UMaterialInterface* Material = StaticMaterial.MaterialInterface;
			if (Material && Material->GetBlendMode() != BLEND_Opaque && Material->GetBlendMode() != BLEND_Masked)
			{
				bHasUnsupportedBlend = true;
				break;
			}
		}

		if (!bHasUnsupportedBlend)
		{
			UE_LOG(LogAPSRenderFix, Display,
				TEXT("No translucent static slot on %s; disabling Nanite because gameplay components apply a translucent override"),
				*ObjectPath);
		}

		if (!Mesh->NaniteSettings.bEnabled)
		{
			UE_LOG(LogAPSRenderFix, Display, TEXT("Already fixed: %s"), *ObjectPath);
			++AlreadyDisabledCount;
			continue;
		}

		Mesh->Modify();
		Mesh->NaniteSettings.bEnabled = false;
		FProperty* ChangedProperty = FindFProperty<FProperty>(
			UStaticMesh::StaticClass(), GET_MEMBER_NAME_CHECKED(UStaticMesh, NaniteSettings));
		FPropertyChangedEvent ChangedEvent(ChangedProperty);
		Mesh->PostEditChangeProperty(ChangedEvent);

		UPackage* Package = Mesh->GetOutermost();
		Package->SetDirtyFlag(true);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs))
		{
			UE_LOG(LogAPSRenderFix, Error, TEXT("Could not save %s"), *Filename);
			++ErrorCount;
			continue;
		}

		UE_LOG(LogAPSRenderFix, Display, TEXT("Disabled Nanite and saved: %s"), *ObjectPath);
		++ChangedCount;
	}

	UE_LOG(LogAPSRenderFix, Display,
		TEXT("Complete: changed=%d already-disabled=%d errors=%d targeted=%d"),
		ChangedCount, AlreadyDisabledCount, ErrorCount, PackagePaths.Num());
	return ErrorCount == 0 ? 0 : 1;
#else
	UE_LOG(LogAPSRenderFix, Error, TEXT("This maintenance commandlet requires an Editor build."));
	return 1;
#endif
}

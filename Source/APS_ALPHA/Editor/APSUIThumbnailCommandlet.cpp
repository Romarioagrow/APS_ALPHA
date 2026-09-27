#include "APSUIThumbnailCommandlet.h"

#if WITH_EDITOR
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UnrealEdGlobals.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAPSUIThumbnails, Log, All);

UAPSUIThumbnailCommandlet::UAPSUIThumbnailCommandlet()
{
	// The engine copies this into GIsClient; without it the renderer hands every world a null
	// scene and thumbnails come back as the canvas clear colour (black).
	IsClient = true;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

#if WITH_EDITOR
namespace
{
	int32 ColorDistance(const uint8* A, const uint8* B)
	{
		return FMath::Max3(FMath::Abs(A[0] - B[0]), FMath::Abs(A[1] - B[1]), FMath::Abs(A[2] - B[2]));
	}

	/** True when the render has no content at all (a single flat colour). */
	bool IsBlankImage(const TArray<uint8>& Pixels)
	{
		for (int32 Index = 4; Index < Pixels.Num(); Index += 4)
		{
			if (ColorDistance(&Pixels[Index], &Pixels[0]) > 2)
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * Makes the backdrop transparent. The thumbnail scene renders without its sky sphere and floor, so
	 * the backdrop is black (0-1 after tonemapping). Background is the near-black region connected to
	 * the image border; the strict level keeps dark hull parts that touch it. Silhouette pixels are hull
	 * colour blended over black, so they take alpha from their brightness and are un-premultiplied;
	 * cleared pixels take the neighbouring hull colour so bilinear scaling in Slate leaves no dark
	 * fringe. Returns the background fraction and leaves the image untouched when nothing would remain.
	 */
	float CutOutBackground(TArray<uint8>& Pixels, int32 Size)
	{
		constexpr int32 BlackLevel = 3;
		constexpr int32 OpaqueLevel = 40;
		const int32 Count = Size * Size;
		const TArray<uint8> Source = Pixels;
		const auto Brightness = [&Source](int32 Pixel)
		{
			return FMath::Max3<int32>(Source[Pixel * 4], Source[Pixel * 4 + 1], Source[Pixel * 4 + 2]);
		};
		TArray<bool> Background;
		Background.Init(false, Count);
		TArray<int32> Queue;
		Queue.Reserve(Count);
		const auto Visit = [&](int32 Pixel)
		{
			if (!Background[Pixel] && Brightness(Pixel) <= BlackLevel)
			{
				Background[Pixel] = true;
				Queue.Add(Pixel);
			}
		};
		for (int32 Step = 0; Step < Size; ++Step)
		{
			Visit(Step);
			Visit((Size - 1) * Size + Step);
			Visit(Step * Size);
			Visit(Step * Size + Size - 1);
		}
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const int32 Pixel = Queue[Head];
			const int32 X = Pixel % Size;
			const int32 Y = Pixel / Size;
			if (X > 0) Visit(Pixel - 1);
			if (X + 1 < Size) Visit(Pixel + 1);
			if (Y > 0) Visit(Pixel - Size);
			if (Y + 1 < Size) Visit(Pixel + Size);
		}
		const float BackgroundFraction = static_cast<float>(Queue.Num()) / Count;
		if (BackgroundFraction > 0.995f)
		{
			return BackgroundFraction;
		}

		const auto IsBackground = [&Background, Size](int32 X, int32 Y)
		{
			return X >= 0 && Y >= 0 && X < Size && Y < Size && Background[Y * Size + X];
		};
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const int32 Pixel = Y * Size + X;
				uint8* Out = &Pixels[Pixel * 4];
				if (Background[Pixel])
				{
					int32 Sum[3] = {0, 0, 0};
					int32 Samples = 0;
					for (int32 DY = -1; DY <= 1; ++DY)
					{
						for (int32 DX = -1; DX <= 1; ++DX)
						{
							const int32 NX = X + DX;
							const int32 NY = Y + DY;
							if (NX < 0 || NY < 0 || NX >= Size || NY >= Size || Background[NY * Size + NX])
							{
								continue;
							}
							const uint8* Hull = &Source[(NY * Size + NX) * 4];
							Sum[0] += Hull[0];
							Sum[1] += Hull[1];
							Sum[2] += Hull[2];
							++Samples;
						}
					}
					for (int32 Channel = 0; Channel < 3; ++Channel)
					{
						Out[Channel] = Samples > 0 ? static_cast<uint8>(Sum[Channel] / Samples) : 0;
					}
					Out[3] = 0;
				}
				else if (IsBackground(X - 1, Y) || IsBackground(X + 1, Y) || IsBackground(X, Y - 1) || IsBackground(X, Y + 1))
				{
					const int32 Alpha = FMath::Clamp(Brightness(Pixel) * 255 / OpaqueLevel, 1, 255);
					for (int32 Channel = 0; Channel < 3; ++Channel)
					{
						Out[Channel] = static_cast<uint8>(FMath::Min(255, Source[Pixel * 4 + Channel] * 255 / Alpha));
					}
					Out[3] = static_cast<uint8>(Alpha);
				}
			}
		}
		return BackgroundFraction;
	}

	/** The largest visible static or skeletal mesh asset among a Blueprint's default components. */
	UObject* FindLargestMeshAsset(const UBlueprint* Blueprint)
	{
		const AActor* Defaults = Blueprint->GeneratedClass
			? Blueprint->GeneratedClass->GetDefaultObject<AActor>() : nullptr;
		if (!Defaults)
		{
			return nullptr;
		}
		UObject* Best = nullptr;
		double BestRadius = 0.0;
		const auto Consider = [&Best, &BestRadius](const UActorComponent* Component)
		{
			const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
			if (!Primitive || !Primitive->IsVisible() || Primitive->bHiddenInGame)
			{
				return;
			}
			UObject* Mesh = nullptr;
			double Radius = 0.0;
			if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Primitive))
			{
				if (UStaticMesh* Asset = StaticMeshComponent->GetStaticMesh())
				{
					Mesh = Asset;
					Radius = Asset->GetBounds().SphereRadius;
				}
			}
			else if (const USkinnedMeshComponent* SkinnedComponent = Cast<USkinnedMeshComponent>(Primitive))
			{
				if (USkinnedAsset* Asset = SkinnedComponent->GetSkinnedAsset())
				{
					Mesh = Asset;
					Radius = Asset->GetBounds().SphereRadius;
				}
			}
			Radius *= Primitive->GetRelativeScale3D().GetAbsMax();
			if (Mesh && Radius > BestRadius)
			{
				Best = Mesh;
				BestRadius = Radius;
			}
		};
		for (const UActorComponent* Component : Defaults->GetComponents())
		{
			Consider(Component);
		}
		if (Blueprint->SimpleConstructionScript)
		{
			for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				Consider(Node ? Node->ComponentTemplate : nullptr);
			}
		}
		return Best;
	}

	void SaveReviewPng(const FString& Directory, const FString& Name, TArray<uint8>& Pixels, int32 Size)
	{
		if (!Directory.IsEmpty())
		{
			FImageUtils::SaveImageByExtension(*FPaths::Combine(Directory, Name + TEXT(".png")),
				FImageView(Pixels.GetData(), Size, Size, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB));
		}
	}
}
#endif

int32 UAPSUIThumbnailCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	int32 Size = 256;
	FParse::Value(*Params, TEXT("Size="), Size);
	Size = FMath::Clamp(Size, 64, 1024);
	FString Only;
	FParse::Value(*Params, TEXT("Only="), Only);
	FString PngDir;
	FParse::Value(*Params, TEXT("PngDir="), PngDir);
	const bool bKeepBackground = FParse::Param(*Params, TEXT("KeepBackground"));
	const bool bSkipExisting = FParse::Param(*Params, TEXT("SkipExisting"));

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);

	// Same set the civilization menu lists: ships, stations, headquarters, shipyards, the pilot.
	TSet<FTopLevelAssetPath> WantedClasses;
	for (const UClass* BaseClass : {ASpaceship::StaticClass(), ASpaceStation::StaticClass(),
		ASpaceHeadquarters::StaticClass(), ASpaceShipyard::StaticClass()})
	{
		TSet<FTopLevelAssetPath> Derived;
		Registry.GetDerivedClassNames({BaseClass->GetClassPathName()}, TSet<FTopLevelAssetPath>(), Derived);
		WantedClasses.Append(Derived);
	}
	WantedClasses.Add(FTopLevelAssetPath(
		TEXT("/Game/APS/APS_ALPHA/Blueprints/BP_CustomGravityCharacter.BP_CustomGravityCharacter_C")));

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/APS"));
	Filter.PackagePaths.Add(TEXT("/Game/APS_PREA"));
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);

	// ThumbnailTools and the thumbnail preview scenes reach the thumbnail manager through GUnrealEd,
	// which commandlets never create (they run a plain UEditorEngine), so every render is silently
	// skipped. GetThumbnailManager() reads no engine state, so the class default object is enough;
	// it is installed only around the render calls.
	UUnrealEdEngine* ThumbnailEngine = GUnrealEd ? GUnrealEd : GetMutableDefault<UUnrealEdEngine>();
	if (!bKeepBackground)
	{
		// Without the sky sphere and floor plane the object renders over black and cuts out cleanly,
		// with no floor shadow. Lighting (three directional lights, ambient cubemap) is unchanged.
		UThumbnailManager& ThumbnailManager = UThumbnailManager::Get();
		ThumbnailManager.EditorSkySphere = nullptr;
		ThumbnailManager.EditorPlane = nullptr;
	}

	// A Blueprint that no longer renders must not keep an older (possibly black) texture: the menu
	// falls back to its vector glyph when no thumbnail exists.
	const auto RemoveStaleThumbnail = [](const FAssetData& Asset)
	{
		const FString StalePackage = FPackageName::ObjectPathToPackageName(
			APSUIThumbnails::TexturePathForBlueprintPackage(Asset.PackageName.ToString()));
		FString StaleFile;
		if (FPackageName::DoesPackageExist(StalePackage, &StaleFile)
			&& IFileManager::Get().Delete(*StaleFile, false, true, true))
		{
			UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] removed stale %s"), *StaleFile);
		}
	};

	int32 Rendered = 0;
	int32 Failed = 0;
	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassExportPath;
		if (!Asset.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassExportPath))
		{
			continue;
		}
		const FTopLevelAssetPath GeneratedClassPath(
			FPackageName::ExportTextPathToObjectPath(GeneratedClassExportPath));
		// The menu hides abstract and deprecated classes, so they need no thumbnail.
		const uint32 ClassFlags = Asset.GetTagValueRef<uint32>(FBlueprintTags::ClassFlags);
		if (!WantedClasses.Contains(GeneratedClassPath)
			|| (ClassFlags & (CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_NotPlaceable)) != 0
			|| (!Only.IsEmpty() && !Asset.PackageName.ToString().Contains(Only))
			|| (bSkipExisting && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(
				APSUIThumbnails::TexturePathForBlueprintPackage(Asset.PackageName.ToString())))))
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint)
		{
			++Failed;
			UE_LOG(LogAPSUIThumbnails, Warning, TEXT("[APS.UIThumbnails] cannot load %s"), *Asset.GetObjectPathString());
			continue;
		}
		// Render with real materials, not the default fallback used while shaders still compile.
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}

		TArray<uint8> Pixels;
		const auto RenderInto = [&Pixels, Size, ThumbnailEngine](UObject* Object)
		{
			FObjectThumbnail Thumbnail;
			{
				TGuardValue<UUnrealEdEngine*> ThumbnailHost(GUnrealEd, ThumbnailEngine);
				ThumbnailTools::RenderThumbnail(Object, Size, Size,
					ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, nullptr, &Thumbnail);
			}
			Pixels = Thumbnail.GetUncompressedImageData();
			return Pixels.Num() == Size * Size * 4 && !IsBlankImage(Pixels);
		};
		bool bDrawn = RenderInto(Blueprint);
		FString DrawnFrom = TEXT("blueprint");
		if (!bDrawn)
		{
			// The preview world never ticks here, so some hulls (Nanite, skeletal) come out blank.
			// Retry with Nanite fallback meshes, then with the Blueprint's largest mesh asset.
			IConsoleVariable* NaniteVariable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite"));
			const int32 PreviousNanite = NaniteVariable ? NaniteVariable->GetInt() : 1;
			if (NaniteVariable)
			{
				NaniteVariable->Set(0, ECVF_SetByCode);
				FlushRenderingCommands();
			}
			bDrawn = RenderInto(Blueprint);
			DrawnFrom = TEXT("blueprint, Nanite fallback");
			if (!bDrawn)
			{
				if (UObject* MeshAsset = FindLargestMeshAsset(Blueprint))
				{
					bDrawn = RenderInto(MeshAsset) || RenderInto(MeshAsset);
					DrawnFrom = FString::Printf(TEXT("mesh %s"), *MeshAsset->GetName());
				}
			}
			if (NaniteVariable)
			{
				NaniteVariable->Set(PreviousNanite, ECVF_SetByCode);
				FlushRenderingCommands();
			}
		}
		if (!bDrawn)
		{
			++Failed;
			UE_LOG(LogAPSUIThumbnails, Warning, TEXT("[APS.UIThumbnails] blank or missing image for %s (%d bytes)"),
				*Asset.GetObjectPathString(), Pixels.Num());
			RemoveStaleThumbnail(Asset);
			continue;
		}
		for (int32 Index = 3; Index < Pixels.Num(); Index += 4)
		{
			Pixels[Index] = 255;
		}

		const FString ObjectPath = APSUIThumbnails::TexturePathForBlueprintPackage(Asset.PackageName.ToString());
		const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
		const FString TextureName = FPackageName::GetShortName(PackageName);
		SaveReviewPng(PngDir, TextureName + TEXT("_raw"), Pixels, Size);
		const float BackgroundFraction = bKeepBackground ? 0.0f : CutOutBackground(Pixels, Size);
		if (BackgroundFraction > 0.995f)
		{
			++Failed;
			UE_LOG(LogAPSUIThumbnails, Warning, TEXT("[APS.UIThumbnails] nothing visible for %s"),
				*Asset.GetObjectPathString());
			RemoveStaleThumbnail(Asset);
			continue;
		}
		SaveReviewPng(PngDir, TextureName, Pixels, Size);

		UPackage* Package = FPackageName::DoesPackageExist(PackageName)
			? LoadPackage(nullptr, *PackageName, LOAD_None)
			: CreatePackage(*PackageName);
		if (!Package)
		{
			++Failed;
			continue;
		}
		UTexture2D* Texture = FindObject<UTexture2D>(Package, *TextureName);
		if (!Texture)
		{
			Texture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(Texture);
		}
		Texture->Source.Init(Size, Size, 1, 1, TSF_BGRA8, Pixels.GetData());
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->SRGB = true;
		Texture->NeverStream = true;
		Texture->PostEditChange();
		Package->MarkPackageDirty();

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackageName, FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(Package, Texture, *Filename, SaveArgs))
		{
			++Rendered;
			UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] %s -> %s background=%.0f%% from=%s"),
				*Asset.PackageName.ToString(), *ObjectPath, BackgroundFraction * 100.0f, *DrawnFrom);
		}
		else
		{
			++Failed;
			UE_LOG(LogAPSUIThumbnails, Warning, TEXT("[APS.UIThumbnails] save failed %s"), *Filename);
		}
		// Drop this Blueprint's meshes and textures before loading the next one; one process holds
		// every start Blueprint otherwise.
		Texture->ClearFlags(RF_Standalone);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] rendered=%d failed=%d size=%d"), Rendered, Failed, Size);
	return Failed == 0 ? 0 : 1;
#else
	UE_LOG(LogAPSUIThumbnails, Error, TEXT("[APS.UIThumbnails] requires an editor build"));
	return 1;
#endif
}

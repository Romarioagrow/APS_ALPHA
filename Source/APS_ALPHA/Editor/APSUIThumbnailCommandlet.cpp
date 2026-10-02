#include "APSUIThumbnailCommandlet.h"

#if WITH_EDITOR
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSStartAssetFilter.h"
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
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
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

	/**
	 * Uses the render's own alpha channel as the matte. It carries coverage only when the process runs
	 * with r.PostProcessing.PropagateAlpha=2; dark hull parts then stay opaque instead of being keyed
	 * out with the black backdrop. The corners are backdrop, which orients the channel (UE may store it
	 * inverted). Edge colours were blended over black, so they are un-premultiplied. Returns false when
	 * the channel holds no coverage.
	 */
	bool ApplySceneAlpha(TArray<uint8>& Pixels, int32 Size)
	{
		const int32 Corners[4] = {0, Size - 1, (Size - 1) * Size, Size * Size - 1};
		int32 CornerAlpha = 0;
		for (const int32 Pixel : Corners)
		{
			CornerAlpha += Pixels[Pixel * 4 + 3];
		}
		const bool bInverted = CornerAlpha > 4 * 128;
		int32 MinAlpha = 255;
		int32 MaxAlpha = 0;
		for (int32 Index = 3; Index < Pixels.Num(); Index += 4)
		{
			const int32 Alpha = bInverted ? 255 - Pixels[Index] : Pixels[Index];
			Pixels[Index] = static_cast<uint8>(Alpha);
			MinAlpha = FMath::Min(MinAlpha, Alpha);
			MaxAlpha = FMath::Max(MaxAlpha, Alpha);
		}
		if (MaxAlpha - MinAlpha < 16)
		{
			return false;
		}
		for (int32 Index = 0; Index < Pixels.Num(); Index += 4)
		{
			const int32 Alpha = Pixels[Index + 3];
			if (Alpha > 0 && Alpha < 255)
			{
				for (int32 Channel = 0; Channel < 3; ++Channel)
				{
					Pixels[Index + Channel] = static_cast<uint8>(FMath::Min(255, Pixels[Index + Channel] * 255 / Alpha));
				}
			}
		}
		return true;
	}

	/**
	 * Frames the visible object: crops a square around the pixels with alpha (plus a small margin) and
	 * resamples it to OutSize with 4x4 bilinear taps in premultiplied space. Long, thin hulls that the
	 * bounds-sphere camera leaves small fill the icon this way; magnification is capped at 2x. Zoom above 1 frames
	 * tighter (Rio 02.10: the S P3 icon "a bit bigger, add 50% zoom"); a long hull's ends may then leave the icon.
	 */
	bool CropToContent(const TArray<uint8>& Source, int32 SourceSize, TArray<uint8>& Out, int32 OutSize, double Zoom = 1.0)
	{
		int32 MinX = SourceSize;
		int32 MinY = SourceSize;
		int32 MaxX = -1;
		int32 MaxY = -1;
		for (int32 Y = 0; Y < SourceSize; ++Y)
		{
			for (int32 X = 0; X < SourceSize; ++X)
			{
				if (Source[(Y * SourceSize + X) * 4 + 3] > 8)
				{
					MinX = FMath::Min(MinX, X);
					MaxX = FMath::Max(MaxX, X);
					MinY = FMath::Min(MinY, Y);
					MaxY = FMath::Max(MaxY, Y);
				}
			}
		}
		if (MaxX < 0)
		{
			return false;
		}
		const double CenterX = (MinX + MaxX + 1) * 0.5;
		const double CenterY = (MinY + MaxY + 1) * 0.5;
		const double Side = FMath::Clamp(FMath::Max(MaxX - MinX + 1, MaxY - MinY + 1) * 1.12 / FMath::Max(Zoom, 0.25),
			OutSize * 0.5, static_cast<double>(SourceSize));
		const double Left = FMath::Clamp(CenterX - Side * 0.5, 0.0, SourceSize - Side);
		const double Top = FMath::Clamp(CenterY - Side * 0.5, 0.0, SourceSize - Side);
		const double Scale = Side / OutSize;
		constexpr int32 Taps = 4;
		Out.SetNumZeroed(OutSize * OutSize * 4);
		for (int32 Y = 0; Y < OutSize; ++Y)
		{
			for (int32 X = 0; X < OutSize; ++X)
			{
				double Sum[4] = {0.0, 0.0, 0.0, 0.0};
				for (int32 TapY = 0; TapY < Taps; ++TapY)
				{
					for (int32 TapX = 0; TapX < Taps; ++TapX)
					{
						const double SourceX = Left + (X + (TapX + 0.5) / Taps) * Scale - 0.5;
						const double SourceY = Top + (Y + (TapY + 0.5) / Taps) * Scale - 0.5;
						const int32 X0 = FMath::Clamp(FMath::FloorToInt(SourceX), 0, SourceSize - 1);
						const int32 Y0 = FMath::Clamp(FMath::FloorToInt(SourceY), 0, SourceSize - 1);
						const int32 X1 = FMath::Min(X0 + 1, SourceSize - 1);
						const int32 Y1 = FMath::Min(Y0 + 1, SourceSize - 1);
						const double FracX = FMath::Clamp(SourceX - X0, 0.0, 1.0);
						const double FracY = FMath::Clamp(SourceY - Y0, 0.0, 1.0);
						const int32 Xs[4] = {X0, X1, X0, X1};
						const int32 Ys[4] = {Y0, Y0, Y1, Y1};
						const double Weights[4] = {(1.0 - FracX) * (1.0 - FracY), FracX * (1.0 - FracY),
							(1.0 - FracX) * FracY, FracX * FracY};
						for (int32 Corner = 0; Corner < 4; ++Corner)
						{
							const uint8* Pixel = &Source[(Ys[Corner] * SourceSize + Xs[Corner]) * 4];
							const double Alpha = Pixel[3] / 255.0 * Weights[Corner];
							Sum[0] += Pixel[0] * Alpha;
							Sum[1] += Pixel[1] * Alpha;
							Sum[2] += Pixel[2] * Alpha;
							Sum[3] += Alpha;
						}
					}
				}
				uint8* Target = &Out[(Y * OutSize + X) * 4];
				Target[3] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Sum[3] / (Taps * Taps) * 255.0), 0, 255));
				if (Sum[3] > UE_DOUBLE_SMALL_NUMBER)
				{
					for (int32 Channel = 0; Channel < 3; ++Channel)
					{
						Target[Channel] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Sum[Channel] / Sum[3]), 0, 255));
					}
				}
			}
		}
		return true;
	}

	/** Gives fully transparent pixels their neighbours' colour so bilinear scaling in Slate leaves no dark fringe. */
	void BleedIntoTransparent(TArray<uint8>& Pixels, int32 Size)
	{
		const TArray<uint8> Source = Pixels;
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				uint8* Target = &Pixels[(Y * Size + X) * 4];
				if (Target[3] != 0)
				{
					continue;
				}
				int32 Sum[3] = {0, 0, 0};
				int32 Samples = 0;
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					for (int32 DX = -1; DX <= 1; ++DX)
					{
						const int32 NX = X + DX;
						const int32 NY = Y + DY;
						const uint8* Neighbour = NX >= 0 && NY >= 0 && NX < Size && NY < Size
							? &Source[(NY * Size + NX) * 4] : nullptr;
						if (Neighbour && Neighbour[3] != 0)
						{
							Sum[0] += Neighbour[0];
							Sum[1] += Neighbour[1];
							Sum[2] += Neighbour[2];
							++Samples;
						}
					}
				}
				if (Samples > 0)
				{
					Target[0] = static_cast<uint8>(Sum[0] / Samples);
					Target[1] = static_cast<uint8>(Sum[1] / Samples);
					Target[2] = static_cast<uint8>(Sum[2] / Samples);
				}
			}
		}
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
	// -Zoom=1.5 frames the object 1.5 times tighter (with -Only, for one Blueprint's icon).
	double Zoom = 1.0;
	FParse::Value(*Params, TEXT("Zoom="), Zoom);
	FString PngDir;
	FParse::Value(*Params, TEXT("PngDir="), PngDir);
	const bool bKeepBackground = FParse::Param(*Params, TEXT("KeepBackground"));
	const bool bSkipExisting = FParse::Param(*Params, TEXT("SkipExisting"));
	// Preview scenes and render resources pile up in one process (it never ticks a frame) until VRAM
	// runs out, so large bakes run as several processes: -Max limits the Blueprints per process and
	// -Progress remembers the ones already handled.
	int32 MaxBlueprints = 0;
	FParse::Value(*Params, TEXT("Max="), MaxBlueprints);
	FString ProgressFile;
	FParse::Value(*Params, TEXT("Progress="), ProgressFile);
	TSet<FString> AlreadyProcessed;
	if (!ProgressFile.IsEmpty())
	{
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *ProgressFile);
		AlreadyProcessed.Append(Lines);
	}
	int32 Attempted = 0;
	int32 Remaining = 0;
	// Render larger than the icon, then crop to the object and downsample.
	const int32 RenderSize = FMath::Min(Size * 2, 2048);
	// With r.PostProcessing.PropagateAlpha=2 (pass -ini:Engine:[/Script/Engine.RendererSettings]:
	// r.PostProcessing.PropagateAlpha=2) the render carries coverage; otherwise the black backdrop is keyed.
	const IConsoleVariable* PropagateAlpha = IConsoleManager::Get().FindConsoleVariable(TEXT("r.PostProcessing.PropagateAlpha"));
	const bool bSceneAlpha = !bKeepBackground && PropagateAlpha && PropagateAlpha->GetInt() == 2;
	UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] matte=%s render=%d icon=%d"),
		bKeepBackground ? TEXT("none") : bSceneAlpha ? TEXT("scene alpha") : TEXT("black key"), RenderSize, Size);

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
	TSet<FSoftObjectPath> ProcessedClasses;
	TArray<FSoftObjectPath> ClassesWithoutVisuals;
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

		const FString BlueprintPackage = Asset.PackageName.ToString();
		if (AlreadyProcessed.Contains(BlueprintPackage))
		{
			continue;
		}
		if (MaxBlueprints > 0 && Attempted >= MaxBlueprints)
		{
			++Remaining;
			continue;
		}
		++Attempted;
		ON_SCOPE_EXIT
		{
			if (!ProgressFile.IsEmpty())
			{
				FFileHelper::SaveStringToFile(BlueprintPackage + LINE_TERMINATOR, *ProgressFile,
					FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
			}
		};
		const FSoftObjectPath ClassPath(GeneratedClassPath.ToString());
		ProcessedClasses.Add(ClassPath);
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
		const auto RenderInto = [&Pixels, RenderSize, ThumbnailEngine](UObject* Object)
		{
			FObjectThumbnail Thumbnail;
			{
				TGuardValue<UUnrealEdEngine*> ThumbnailHost(GUnrealEd, ThumbnailEngine);
				ThumbnailTools::RenderThumbnail(Object, RenderSize, RenderSize,
					ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, nullptr, &Thumbnail);
			}
			Pixels = Thumbnail.GetUncompressedImageData();
			return Pixels.Num() == RenderSize * RenderSize * 4 && !IsBlankImage(Pixels);
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
			ClassesWithoutVisuals.Add(ClassPath);
			continue;
		}

		const FString ObjectPath = APSUIThumbnails::TexturePathForBlueprintPackage(Asset.PackageName.ToString());
		const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
		const FString TextureName = FPackageName::GetShortName(PackageName);
		SaveReviewPng(PngDir, TextureName + TEXT("_raw"), Pixels, RenderSize);
		bool bMatted = bSceneAlpha && ApplySceneAlpha(Pixels, RenderSize);
		float BackgroundFraction = 0.0f;
		if (!bMatted)
		{
			for (int32 Index = 3; Index < Pixels.Num(); Index += 4)
			{
				Pixels[Index] = 255;
			}
			BackgroundFraction = bKeepBackground ? 0.0f : CutOutBackground(Pixels, RenderSize);
		}
		TArray<uint8> Icon;
		if (BackgroundFraction > 0.995f || !CropToContent(Pixels, RenderSize, Icon, Size, Zoom))
		{
			++Failed;
			UE_LOG(LogAPSUIThumbnails, Warning, TEXT("[APS.UIThumbnails] nothing visible for %s"),
				*Asset.GetObjectPathString());
			RemoveStaleThumbnail(Asset);
			ClassesWithoutVisuals.Add(ClassPath);
			continue;
		}
		BleedIntoTransparent(Icon, Size);
		Pixels = MoveTemp(Icon);
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
			UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] %s -> %s matte=%s from=%s"),
				*Asset.PackageName.ToString(), *ObjectPath, bMatted ? TEXT("alpha") : TEXT("key"), *DrawnFrom);
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

	// The menu hides Blueprints that render nothing. Entries for Blueprints this run did not process
	// (-Only, -SkipExisting) are kept; the hand-curated ExcludedClasses list is never touched.
	{
		const FString FilterPackageName = FPackageName::ObjectPathToPackageName(FString(APSUIThumbnails::StartAssetFilterPath));
		const FString FilterName = FPackageName::GetShortName(FilterPackageName);
		UPackage* FilterPackage = FPackageName::DoesPackageExist(FilterPackageName)
			? LoadPackage(nullptr, *FilterPackageName, LOAD_None)
			: CreatePackage(*FilterPackageName);
		UAPSStartAssetFilter* StartAssetFilter = FilterPackage ? FindObject<UAPSStartAssetFilter>(FilterPackage, *FilterName) : nullptr;
		if (FilterPackage && !StartAssetFilter)
		{
			StartAssetFilter = NewObject<UAPSStartAssetFilter>(FilterPackage, *FilterName, RF_Public | RF_Standalone | RF_Transactional);
			FAssetRegistryModule::AssetCreated(StartAssetFilter);
		}
		if (StartAssetFilter)
		{
			TArray<TSoftClassPtr<AActor>> Kept;
			for (const TSoftClassPtr<AActor>& Entry : StartAssetFilter->ClassesWithoutVisuals)
			{
				if (!ProcessedClasses.Contains(Entry.ToSoftObjectPath()))
				{
					Kept.Add(Entry);
				}
			}
			for (const FSoftObjectPath& ClassPath : ClassesWithoutVisuals)
			{
				Kept.AddUnique(TSoftClassPtr<AActor>(ClassPath));
			}
			StartAssetFilter->ClassesWithoutVisuals = MoveTemp(Kept);
			FilterPackage->MarkPackageDirty();
			FSavePackageArgs FilterSaveArgs;
			FilterSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			FilterSaveArgs.SaveFlags = SAVE_NoError;
			const bool bFilterSaved = UPackage::SavePackage(FilterPackage, StartAssetFilter,
				*FPackageName::LongPackageNameToFilename(FilterPackageName, FPackageName::GetAssetPackageExtension()),
				FilterSaveArgs);
			UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] filter %s: %d without visuals, %d excluded by hand, saved=%s"),
				*FilterPackageName, StartAssetFilter->ClassesWithoutVisuals.Num(), StartAssetFilter->ExcludedClasses.Num(),
				bFilterSaved ? TEXT("true") : TEXT("false"));
		}
	}

	UE_LOG(LogAPSUIThumbnails, Display, TEXT("[APS.UIThumbnails] rendered=%d failed=%d size=%d remaining=%d"),
		Rendered, Failed, Size, Remaining);
	return Failed == 0 ? 0 : 1;
#else
	UE_LOG(LogAPSUIThumbnails, Error, TEXT("[APS.UIThumbnails] requires an editor build"));
	return 1;
#endif
}

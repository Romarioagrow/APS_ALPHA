#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/PlayerCameraManager.h"
#include "APS_ALPHA/UI/MainMenu/SAPSChamferedOverlay.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace APSMainMenuHeroGalaxyTests
{
	constexpr double TimeoutSeconds = 180.0;
	/** Rio 06.10: the hand-made composition has 9,200 points; the catalogue galaxy (aps.Menu.HeroGalaxyV2 1) its own. */
	int32 ExpectedInstanceCount()
	{
		const IConsoleVariable* CatalogueGalaxy = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Menu.HeroGalaxyV2"));
		const IConsoleVariable* CatalogueStars = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Menu.HeroGalaxyStars"));
		return CatalogueGalaxy && CatalogueGalaxy->GetInt() != 0 && CatalogueStars
			? FMath::Clamp(CatalogueStars->GetInt(), 5000, 200000) : 9200;
	}
	constexpr int32 ExpectedDeepSpaceInstanceCount = 2780;
	constexpr int32 ExpectedNebulaInstanceCount = 0;

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return *It;
			}
		}
		return nullptr;
	}

	class FHeroGalaxySwitchCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FHeroGalaxySwitchCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (StartSeconds <= 0.0)
			{
				StartSeconds = Now;
			}
			if (Now - StartSeconds > TimeoutSeconds)
			{
				Test->AddError(TEXT("Main-menu hero galaxy switch timed out"));
				return true;
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			AAstroGenerator* Generator = World ? FindPreviewGenerator(World) : nullptr;
			if (!Controller || !ViewModel || !Generator)
			{
				return false;
			}

			if (Phase == 0)
			{
				UGameViewportClient* Client = AutomationCommon::GetAnyGameViewportClient();
				if (!Client || !Client->Viewport || !Client->GetWindow().IsValid()) return false;
				if (!bCapturedStandardViewport)
				{
					if (!Generator->IsMainMenuHeroGalaxyActive()) return false;
					Phase = 1;
					PhaseStartSeconds = Now;
					return false;
				}
				if (!bRequestedWideViewport)
				{
					Client->GetWindow()->Resize(FVector2D(1920, 760));
					bRequestedWideViewport = true;
					return false;
				}
				if (Client->Viewport->GetSizeXY().X < 1800) return false;
				Generator->ReleaseMainMenuHeroGalaxy();
				Generator->GenerateMainMenuHeroGalaxy(Controller);
				if (!Generator->IsMainMenuHeroGalaxyActive())
				{
					return false;
				}
				Test->TestEqual(TEXT("Landing hero uses its fixed HISM budget"),
					Generator->GetMainMenuHeroGalaxyInstanceCount(), ExpectedInstanceCount());
				Test->TestEqual(TEXT("Landing deep-space layer uses its fixed HISM budget"),
					Generator->GetMainMenuDeepSpaceInstanceCount(), ExpectedDeepSpaceInstanceCount);
				Test->TestEqual(TEXT("Landing contains no nebula carriers"),
					Generator->GetMainMenuNebulaInstanceCount(), ExpectedNebulaInstanceCount);
				Test->TestEqual(TEXT("Landing camera is owned by the preview generator"),
					Controller->GetViewTarget(), static_cast<AActor*>(Generator));
				Phase = 1;
				PhaseStartSeconds = Now;
				return false;
			}

			if (Phase == 1)
			{
				// Let exposure, shader loading and any delayed level effects settle.
				if (Now - PhaseStartSeconds < 20.0) return false;
				UGameViewportClient* GameViewportClient =
					AutomationCommon::GetAnyGameViewportClient();
				FViewport* Viewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
				if (!Viewport || GameViewportClient->GetWorld() != World) return false;
				const FIntPoint ViewportSize = Viewport->GetSizeXY();
				TArray<FColor> Pixels;
				if (ViewportSize.X <= 0 || ViewportSize.Y <= 0
					|| !Viewport->ReadPixels(Pixels)
					|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
				{
					Test->AddError(TEXT("Landing hero game viewport could not be captured"));
					return true;
				}
				int32 BrightHeroPixels = 0;
				const FMinimalViewInfo& ActualView = Controller->PlayerCameraManager->GetCameraCacheView();
				FVector2D ProjectedCenter;
				Controller->ProjectWorldLocationToScreen(Generator->GetActorLocation(), ProjectedCenter);
				UE_LOG(LogTemp, Display, TEXT("[APS.MenuRenderQA] camera=%s rotation=%s FOV=%.3f center=%s"),
					*ActualView.Location.ToCompactString(), *ActualView.Rotation.ToCompactString(), ActualView.FOV,
					*ProjectedCenter.ToString());
				TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Layers;
				Generator->GetComponents(Layers);
				for (const auto* Layer : Layers)
				{
					UE_LOG(LogTemp, Display, TEXT("[APS.MenuRenderQA] %s visible=%d hidden=%d built=%d render=%d bounds=%s extent=%s transform=%s"),
						*Layer->GetName(), Layer->IsVisible(), Layer->bHiddenInGame,
						Layer->NumBuiltRenderInstances, Layer->InstanceCountToRender,
						*Layer->Bounds.Origin.ToCompactString(), *Layer->Bounds.BoxExtent.ToCompactString(),
						*Layer->GetComponentTransform().ToHumanReadableString());
					if (Layer->GetInstanceCount() > 0)
					{
						FTransform First;
						Layer->GetInstanceTransform(0, First, true);
						FVector2D Screen;
						Controller->ProjectWorldLocationToScreen(First.GetLocation(), Screen);
						FString Data;
						for (int32 I = 0; I < FMath::Min(6, Layer->PerInstanceSMCustomData.Num()); ++I)
							Data += FString::Printf(TEXT(" %.3f"), Layer->PerInstanceSMCustomData[I]);
						UE_LOG(LogTemp, Display, TEXT("[APS.MenuRenderQA] %s first=%s screen=%s custom=%s material=%s"),
							*Layer->GetName(), *First.ToHumanReadableString(), *Screen.ToString(), *Data,
							*GetPathNameSafe(Layer->GetMaterial(0)));
					}
				}
				for (int32 Y = ViewportSize.Y / 4; Y < ViewportSize.Y * 3 / 4; ++Y)
				for (int32 X = ViewportSize.X / 3; X < ViewportSize.X * 2 / 3; ++X)
				{
					const FColor& P = Pixels[Y * ViewportSize.X + X];
					BrightHeroPixels += static_cast<int32>(P.R) + P.G + P.B > 330 ? 1 : 0;
				}
				UE_LOG(LogTemp, Display, TEXT("[APS.MenuRenderQA] brightHeroPixels=%d"), BrightHeroPixels);
				Test->TestTrue(TEXT("The rebuilt galaxy is visible in the actual rendered frame"), BrightHeroPixels > 500);
				ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
					TEXT("Screenshots/Windows/APS_MainMenu_HeroCloud.png"));
				IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
				TArray64<uint8> PngData;
				FImageUtils::PNGCompressImageArray(ViewportSize.X, ViewportSize.Y,
					TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
				if (PngData.IsEmpty()
					|| !FFileHelper::SaveArrayToFile(PngData, *ScreenshotPath))
				{
					Test->AddError(TEXT("Landing hero game viewport screenshot was not written"));
					return true;
				}
				UE_LOG(LogTemp, Display,
					TEXT("[APS.MainMenuHero] Captured game viewport screenshot=%s"),
					*ScreenshotPath);
				SaveMenuFrame(Controller, bCapturedStandardViewport
					? TEXT("APS_MainMenu_NoFog.png") : TEXT("APS_MainMenu_NoFog_16x9.png"));
				const FPostProcessSettings& PP = Controller->PlayerCameraManager->GetCameraCacheView().PostProcessSettings;
				Test->TestEqual(TEXT("Desktop diaphragm DOF is disabled"), PP.DepthOfFieldFocalDistance, 0.0f);
				Test->TestEqual(TEXT("Distance depth blur is disabled"), PP.DepthOfFieldDepthBlurRadius, 0.0f);
				if (!bCapturedStandardViewport)
				{
					bCapturedStandardViewport = true;
					Phase = 0;
					return false;
				}
				if (!Controller->OpenAstronomicalGenerationForAutomation(
					EAstroPreviewFocus::StarCluster, EAPSGenerationRoute::Space))
				{
					Test->AddError(TEXT("Could not open Astronomical Generation from landing"));
					return true;
				}
				Phase = 2;
				PhaseStartSeconds = Now;
				return false;
			}

			if (!ViewModel->bPreviewReady)
			{
				return false;
			}
			if (Now - PhaseStartSeconds < 8.0) return false;
			SaveMenuFrame(Controller, TEXT("APS_Astronomical_ClippedCorners.png"));
			Test->TestFalse(TEXT("Astronomical Generation releases the decorative HISM"),
				Generator->IsMainMenuHeroGalaxyActive());
			Test->TestEqual(TEXT("Released decorative HISM owns no instances"),
				Generator->GetMainMenuHeroGalaxyInstanceCount(), 0);
			Test->TestEqual(TEXT("Released deep-space HISM owns no instances"),
				Generator->GetMainMenuDeepSpaceInstanceCount(), 0);
			Test->TestEqual(TEXT("Released nebula HISM owns no instances"),
				Generator->GetMainMenuNebulaInstanceCount(), 0);
			Test->TestTrue(TEXT("Astronomical Generation resumes the normal preview contract"),
				Generator->IsPreviewFocusAvailable(EAstroPreviewFocus::StarCluster));
			return true;
		}

	private:
		void SaveMenuFrame(AMainMenuController* Controller, const TCHAR* Filename)
		{
			const TSharedPtr<SAPSMainMenuRoot> Root = Controller->GetSlateMenuRootForAutomation();
			TArray<FColor> Pixels;
			FIntVector Size = FIntVector::ZeroValue;
			if (!Root.IsValid() || !FSlateApplication::Get().TakeScreenshot(Root.ToSharedRef(), Pixels, Size))
			{
				Test->AddError(TEXT("Could not capture the complete menu including Slate"));
				return;
			}
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y,
				TArrayView64<const FColor>(Pixels.GetData(), static_cast<int64>(Size.X) * Size.Y), Png);
			Test->TestTrue(TEXT("Full menu screenshot saved"), FFileHelper::SaveArrayToFile(Png,
				*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots/Windows"), Filename)));
		}

		FAutomationTestBase* Test{nullptr};
		double StartSeconds{0.0};
		double PhaseStartSeconds{0.0};
		int32 Phase{0};
		bool bRequestedWideViewport{false};
		bool bCapturedStandardViewport{false};
		FString ScreenshotPath;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSMainMenuHeroGalaxySwitchTest,
	"APS.Rendered.MainMenu.HeroGalaxySwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSMainMenuHeroGalaxySwitchTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for hero galaxy smoke"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSMainMenuHeroGalaxyTests::FHeroGalaxySwitchCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSChamferChildClipTest,
	"APS.Rendered.UI.ChamferClipsChildren",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSChamferChildClipTest::RunTest(const FString& Parameters)
{
	// Deliberately use rectangular children and a child that requests independent
	// clipping: all four cut-outs must still reveal the contrasting parent.
	FWidgetRenderer Renderer(true);
	for (const float Scale : {1.0f, 1.5f, 2.0f})
	{
		const FVector2D DrawSize = FVector2D(256, 144) * Scale;
		const TSharedRef<SWidget> Widget = SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(1, 0, 1, 1)).Padding(16.0f)
			[
				SNew(SAPSChamferedOverlay) + SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0, 1, 0, 1))
					.Clipping(EWidgetClipping::ClipToBoundsWithoutIntersecting)
				]
			];
		UTextureRenderTarget2D* Target = FWidgetRenderer::CreateTargetFor(DrawSize, TF_Nearest, true);
		Renderer.DrawWidget(Target, Widget, Scale, DrawSize, 0.0f);
		TArray<FColor> Pixels;
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels))
		{
			AddError(TEXT("Could not read chamfer regression render"));
			return false;
		}
		int32 EscapedPixels = 0;
		const int32 Width = static_cast<int32>(DrawSize.X);
		const int32 Height = static_cast<int32>(DrawSize.Y);
		const int32 Margin = FMath::RoundToInt(16.0f * Scale);
		for (int32 Corner = 0; Corner < 4; ++Corner)
		for (int32 Y = 1; Y < FMath::FloorToInt(9.0f * Scale); ++Y)
		for (int32 X = 1; X + Y < FMath::FloorToInt(10.0f * Scale); ++X)
		{
			const int32 PX = (Corner & 1) ? Width - Margin - 1 - X : Margin + X;
			const int32 PY = (Corner & 2) ? Height - Margin - 1 - Y : Margin + Y;
			const FColor& Color = Pixels[PY * Width + PX];
			EscapedPixels += Color.G > 20 || Color.R < 220 || Color.B < 220 ? 1 : 0;
		}
		TestEqual(*FString::Printf(TEXT("No rectangular child leaks through any corner at scale %.1f"), Scale), EscapedPixels, 0);
		TestTrue(TEXT("Panel interior is still painted"), Pixels[(Height / 2) * Width + Width / 2].G > 220);
	}
	return true;
}

#endif

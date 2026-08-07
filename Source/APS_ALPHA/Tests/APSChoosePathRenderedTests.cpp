#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Layout/SlateRect.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace APSChoosePathRenderedTests
{
	constexpr double StartupTimeoutSeconds = 20.0;
	constexpr double ScreenshotTimeoutSeconds = 12.0;
	constexpr double PageSettleSeconds = 0.75;
	constexpr double InteractionSettleSeconds = 0.20;
	constexpr int32 ExpectedCardCount = 6;

	struct FCardPixelMetrics
	{
		double MeanLuminance{0.0};
		double Variance{0.0};
		double DarkRatio{0.0};
		double BrightRatio{0.0};
		double ColoredDetailRatio{0.0};
		double RailBrightRatio{0.0};
	};

	bool ReadSlateWidget(const TSharedRef<SWidget>& Widget,
		TArray<FColor>& OutPixels, FIntPoint& OutSize)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return false;
		}

		FIntVector SlateSize = FIntVector::ZeroValue;
		if (!FSlateApplication::Get().TakeScreenshot(Widget, OutPixels, SlateSize))
		{
			return false;
		}

		OutSize = FIntPoint(SlateSize.X, SlateSize.Y);
		const int32 RequiredPixels = OutSize.X * OutSize.Y;
		if (OutSize.X <= 0 || OutSize.Y <= 0 || OutPixels.Num() < RequiredPixels)
		{
			return false;
		}
		// Some renderers reserve a larger readback buffer. Pixel addressing below is
		// deliberately limited to the widget rectangle reported by Slate.
		OutPixels.SetNum(RequiredPixels, EAllowShrinking::No);
		return true;
	}

	FIntRect ToPixelRect(const FSlateRect& NormalizedRect, const FIntPoint& ViewportSize)
	{
		return FIntRect(
			FMath::Clamp(FMath::FloorToInt(NormalizedRect.Left * ViewportSize.X), 0, ViewportSize.X - 1),
			FMath::Clamp(FMath::FloorToInt(NormalizedRect.Top * ViewportSize.Y), 0, ViewportSize.Y - 1),
			FMath::Clamp(FMath::CeilToInt(NormalizedRect.Right * ViewportSize.X), 1, ViewportSize.X),
			FMath::Clamp(FMath::CeilToInt(NormalizedRect.Bottom * ViewportSize.Y), 1, ViewportSize.Y));
	}

	double ChangedPixelRatio(const TArray<FColor>& Before, const TArray<FColor>& After,
		const FIntPoint& ViewportSize, const FSlateRect& NormalizedRect)
	{
		if (Before.Num() != After.Num() || Before.IsEmpty())
		{
			return 0.0;
		}
		const FIntRect Rect = ToPixelRect(NormalizedRect, ViewportSize);
		int64 Samples = 0;
		int64 Changed = 0;
		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; Y += 2)
		{
			for (int32 X = Rect.Min.X; X < Rect.Max.X; X += 2)
			{
				const int32 PixelIndex = Y * ViewportSize.X + X;
				const FColor& A = Before[PixelIndex];
				const FColor& B = After[PixelIndex];
				const int32 Difference = FMath::Abs(static_cast<int32>(A.R) - B.R)
					+ FMath::Abs(static_cast<int32>(A.G) - B.G)
					+ FMath::Abs(static_cast<int32>(A.B) - B.B);
				Changed += Difference >= 8 ? 1 : 0;
				++Samples;
			}
		}
		return Samples > 0 ? static_cast<double>(Changed) / Samples : 0.0;
	}

	FCardPixelMetrics MeasureCard(const TArray<FColor>& Pixels,
		const FIntPoint& ViewportSize, const FSlateRect& NormalizedRect)
	{
		FCardPixelMetrics Metrics;
		const FIntRect Rect = ToPixelRect(NormalizedRect, ViewportSize);
		if (Rect.Width() < 8 || Rect.Height() < 8)
		{
			return Metrics;
		}

		const int32 RailStartY = Rect.Min.Y + FMath::FloorToInt(Rect.Height() * 0.64f);
		int64 Samples = 0;
		int64 DarkPixels = 0;
		int64 BrightPixels = 0;
		int64 ColoredPixels = 0;
		int64 RailSamples = 0;
		int64 RailBrightPixels = 0;
		double Sum = 0.0;
		double SumSquares = 0.0;
		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
		{
			for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
			{
				const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
				const double Luminance = (54.0 * Pixel.R + 183.0 * Pixel.G + 19.0 * Pixel.B) / 256.0;
				Sum += Luminance;
				SumSquares += Luminance * Luminance;
				DarkPixels += Luminance < 38.0 ? 1 : 0;
				BrightPixels += Luminance > 96.0 ? 1 : 0;
				const uint8 MinimumChannel = FMath::Min3(Pixel.R, Pixel.G, Pixel.B);
				const uint8 MaximumChannel = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
				ColoredPixels += MaximumChannel > 35 && MaximumChannel - MinimumChannel > 16 ? 1 : 0;
				if (Y >= RailStartY)
				{
					++RailSamples;
					RailBrightPixels += Luminance > 105.0 ? 1 : 0;
				}
				++Samples;
			}
		}
		if (Samples > 0)
		{
			Metrics.MeanLuminance = Sum / Samples;
			Metrics.Variance = FMath::Max(0.0, SumSquares / Samples
				- Metrics.MeanLuminance * Metrics.MeanLuminance);
			Metrics.DarkRatio = static_cast<double>(DarkPixels) / Samples;
			Metrics.BrightRatio = static_cast<double>(BrightPixels) / Samples;
			Metrics.ColoredDetailRatio = static_cast<double>(ColoredPixels) / Samples;
		}
		Metrics.RailBrightRatio = RailSamples > 0
			? static_cast<double>(RailBrightPixels) / RailSamples : 0.0;
		return Metrics;
	}

	class FChoosePathRenderedCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FChoosePathRenderedCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (StartSeconds <= 0.0)
			{
				StartSeconds = Now;
				StepStartSeconds = Now;
			}
			if (Now - StartSeconds > StartupTimeoutSeconds + ScreenshotTimeoutSeconds)
			{
				Test->AddError(TEXT("Choose Path rendered test timed out"));
				return true;
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			TSharedPtr<SAPSMainMenuRoot> Root = Controller
				? Controller->GetSlateMenuRootForAutomation() : nullptr;

			switch (Step)
			{
			case 0:
				return OpenPage(Root, Now);
			case 1:
				return ValidateStructureAndFocus(Root, Now);
			case 2:
				return ValidateFocusAndHover(Root, Now);
			case 3:
				return ValidateHoverAndClearInteractions(Root, Now);
			case 4:
				return ValidateNeutralStateAndRequestScreenshot(Root, Now);
			case 5:
				return ValidateScreenshot(Root, Now);
			default:
				return true;
			}
		}

	private:
		bool OpenPage(const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!Root.IsValid())
			{
				if (Now - StepStartSeconds < StartupTimeoutSeconds) return false;
				Test->AddError(TEXT("Slate MainMenu root was not installed"));
				return true;
			}
			Root->OpenChoosePathForAutomation();
			Step = 1;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateStructureAndFocus(const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!Root.IsValid())
			{
				Test->AddError(TEXT("Choose Path root disappeared"));
				return true;
			}
			if (Now - StepStartSeconds < PageSettleSeconds) return false;

			int32 CardCount = 0;
			int32 ProceduralVisualCount = 0;
			int32 StaticTextureCount = 0;
			Root->GetChoosePathDiagnosticsForAutomation(
				CardCount, ProceduralVisualCount, StaticTextureCount);
			Test->TestEqual(TEXT("Choose Path owns six route cards"), CardCount, ExpectedCardCount);
			Test->TestEqual(TEXT("Every route card owns a procedural visual"),
				ProceduralVisualCount, ExpectedCardCount);
			Test->TestEqual(TEXT("Route cards reference no static texture resources"),
				StaticTextureCount, 0);

			CardRects.Reset();
			for (int32 CardIndex = 0; CardIndex < ExpectedCardCount; ++CardIndex)
			{
				FSlateRect CardRect;
				const bool bHasRect = Root->GetChoosePathCardNormalizedRectForAutomation(
					CardIndex, CardRect);
				Test->TestTrue(*FString::Printf(TEXT("Card %d has laid-out geometry"), CardIndex), bHasRect);
				if (!bHasRect) continue;
				Test->TestTrue(*FString::Printf(TEXT("Card %d is inside the viewport"), CardIndex),
					CardRect.Left >= -0.01f && CardRect.Top >= -0.01f
					&& CardRect.Right <= 1.01f && CardRect.Bottom <= 1.01f);
				Test->TestTrue(*FString::Printf(TEXT("Card %d has readable area"), CardIndex),
					CardRect.Right - CardRect.Left >= 0.10f
					&& CardRect.Bottom - CardRect.Top >= 0.18f);
				CardRects.Add(CardRect);
			}
			if (CardRects.Num() != ExpectedCardCount
				|| !ReadSlateWidget(Root.ToSharedRef(), BaselinePixels, ViewportSize))
			{
				Test->AddError(TEXT("Choose Path baseline Slate frame could not be captured"));
				return true;
			}

			Test->TestTrue(TEXT("Primary route accepts keyboard focus"),
				Root->FocusChoosePathCardForAutomation(0));
			Step = 2;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateFocusAndHover(const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!Root.IsValid())
			{
				Test->AddError(TEXT("Choose Path root disappeared during focus"));
				return true;
			}
			if (Now - StepStartSeconds < InteractionSettleSeconds) return false;

			bool bHovered = false;
			bool bFocused = false;
			Test->TestTrue(TEXT("Primary route interaction state is readable"),
				Root->GetChoosePathCardInteractionForAutomation(0, bHovered, bFocused));
			Test->TestTrue(TEXT("Primary route keeps actual Slate keyboard focus"), bFocused);

			TArray<FColor> FocusedPixels;
			FIntPoint FocusedSize;
			if (ReadSlateWidget(Root.ToSharedRef(), FocusedPixels, FocusedSize)
				&& FocusedSize == ViewportSize)
			{
				const double FocusDelta = ChangedPixelRatio(
					BaselinePixels, FocusedPixels, ViewportSize, CardRects[0]);
				Test->TestTrue(TEXT("Keyboard focus produces a visible card response"),
					FocusDelta >= 0.001);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.ChoosePath] focusDelta=%.5f"), FocusDelta);
			}
			else
			{
				Test->AddError(TEXT("Focused Choose Path frame could not be read"));
			}

			Test->TestTrue(TEXT("Secondary route accepts synthetic pointer hover"),
				Root->HoverChoosePathCardForAutomation(1));
			Step = 3;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateHoverAndClearInteractions(const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!Root.IsValid())
			{
				Test->AddError(TEXT("Choose Path root disappeared during hover"));
				return true;
			}
			if (Now - StepStartSeconds < InteractionSettleSeconds) return false;

			bool bHovered = false;
			bool bFocused = false;
			Root->GetChoosePathCardInteractionForAutomation(1, bHovered, bFocused);
			if (!bHovered && Now - StepStartSeconds < 2.0)
			{
				Root->HoverChoosePathCardForAutomation(1);
				return false;
			}
			Test->TestTrue(TEXT("Secondary route receives actual Slate hover"), bHovered);

			TArray<FColor> HoveredPixels;
			FIntPoint HoveredSize;
			if (ReadSlateWidget(Root.ToSharedRef(), HoveredPixels, HoveredSize)
				&& HoveredSize == ViewportSize)
			{
				const double HoverDelta = ChangedPixelRatio(
					BaselinePixels, HoveredPixels, ViewportSize, CardRects[1]);
				Test->TestTrue(TEXT("Pointer hover produces a visible card response"),
					HoverDelta >= 0.001);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.ChoosePath] hoverDelta=%.5f"), HoverDelta);
			}
			else
			{
				Test->AddError(TEXT("Hovered Choose Path frame could not be read"));
			}

			Test->TestTrue(TEXT("Choose Path interactions can be returned to neutral"),
				Root->ClearChoosePathCardInteractionsForAutomation());
			Step = 4;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateNeutralStateAndRequestScreenshot(
			const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!Root.IsValid())
			{
				Test->AddError(TEXT("Choose Path root disappeared while clearing interactions"));
				return true;
			}
			if (Now - StepStartSeconds < InteractionSettleSeconds) return false;

			bool bAnyCardActive = false;
			for (int32 CardIndex = 0; CardIndex < ExpectedCardCount; ++CardIndex)
			{
				bool bHovered = false;
				bool bFocused = false;
				if (!Root->GetChoosePathCardInteractionForAutomation(
					CardIndex, bHovered, bFocused))
				{
					Test->AddError(*FString::Printf(
						TEXT("Card %d interaction state disappeared before screenshot"), CardIndex));
					continue;
				}
				bAnyCardActive |= bHovered || bFocused;
			}
			if (bAnyCardActive && Now - StepStartSeconds < 2.0)
			{
				Root->ClearChoosePathCardInteractionsForAutomation();
				return false;
			}
			Test->TestFalse(TEXT("Final Choose Path frame has no focused or hovered card"),
				bAnyCardActive);

			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				TEXT("Screenshots/Windows/APS_ChoosePath_Procedural.png"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false, false);
			Step = 5;
			StepStartSeconds = Now;
			return false;
		}

		bool ValidateScreenshot(const TSharedPtr<SAPSMainMenuRoot>& Root, double Now)
		{
			if (!IFileManager::Get().FileExists(*ScreenshotPath))
			{
				if (Now - StepStartSeconds < ScreenshotTimeoutSeconds) return false;
				Test->AddError(TEXT("Choose Path screenshot was not written"));
				return true;
			}
			if (!Root.IsValid())
			{
				Test->AddError(TEXT("Choose Path root disappeared before screenshot validation"));
				return true;
			}

			TArray<FColor> Pixels;
			FIntPoint Size;
			if (!ReadSlateWidget(Root.ToSharedRef(), Pixels, Size) || Size != ViewportSize)
			{
				if (Now - StepStartSeconds < ScreenshotTimeoutSeconds) return false;
				Test->AddError(TEXT("Choose Path screenshot Slate pixels could not be read"));
				return true;
			}

			for (int32 CardIndex = 0; CardIndex < CardRects.Num(); ++CardIndex)
			{
				const FCardPixelMetrics Metrics = MeasureCard(Pixels, Size, CardRects[CardIndex]);
				Test->TestTrue(*FString::Printf(TEXT("Card %d keeps a dark readable plate"), CardIndex),
					Metrics.MeanLuminance >= 2.0 && Metrics.MeanLuminance <= 110.0
					&& Metrics.DarkRatio >= 0.25);
				Test->TestTrue(*FString::Printf(TEXT("Card %d contains visible procedural detail"), CardIndex),
					Metrics.Variance >= 12.0 && Metrics.ColoredDetailRatio >= 0.001);
				Test->TestTrue(*FString::Printf(TEXT("Card %d contains high-contrast labels"), CardIndex),
					Metrics.BrightRatio >= 0.0005 && Metrics.RailBrightRatio >= 0.0005);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.ChoosePath] card=%d mean=%.2f variance=%.2f dark=%.3f bright=%.4f color=%.4f railBright=%.4f"),
					CardIndex, Metrics.MeanLuminance, Metrics.Variance, Metrics.DarkRatio,
					Metrics.BrightRatio, Metrics.ColoredDetailRatio, Metrics.RailBrightRatio);
			}

			int32 CardCount = 0;
			int32 ProceduralVisualCount = 0;
			int32 StaticTextureCount = 0;
			Root->GetChoosePathDiagnosticsForAutomation(
				CardCount, ProceduralVisualCount, StaticTextureCount);
			Test->TestEqual(TEXT("Screenshot still contains all procedural cards"),
				ProceduralVisualCount, ExpectedCardCount);
			Test->TestEqual(TEXT("Screenshot cards still contain no static textures"),
				StaticTextureCount, 0);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.ChoosePath] rendered contract passed screenshot=%s"), *ScreenshotPath);
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		int32 Step{0};
		double StartSeconds{0.0};
		double StepStartSeconds{0.0};
		FIntPoint ViewportSize{FIntPoint::ZeroValue};
		TArray<FSlateRect> CardRects;
		TArray<FColor> BaselinePixels;
		FString ScreenshotPath;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSChoosePathRenderedTest,
	"APS.Rendered.MainMenu.ChoosePathProcedural",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSChoosePathRenderedTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for Choose Path rendered test"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSChoosePathRenderedTests::FChoosePathRenderedCommand(this));
	return true;
}

#endif

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/UI/Style/APSUIStyle.h"

namespace APSUIContrastTests
{
	double ContrastRatio(const FLinearColor& Foreground, const FLinearColor& Background)
	{
		const double ForegroundLuminance = Foreground.GetLuminance();
		const double BackgroundLuminance = Background.GetLuminance();
		return (FMath::Max(ForegroundLuminance, BackgroundLuminance) + 0.05)
			/ (FMath::Min(ForegroundLuminance, BackgroundLuminance) + 0.05);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSUIContrastContractTest,
	"APS.UI.Style.ContrastContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUIContrastContractTest::RunTest(const FString& Parameters)
{
	using namespace APSUIContrastTests;
	const TArray<EAPSUIDisplayProfile> Profiles{
		EAPSUIDisplayProfile::Balanced22,
		EAPSUIDisplayProfile::Cinematic24,
		EAPSUIDisplayProfile::Bright20
	};

	for (int32 Index = 0; Index < Profiles.Num(); ++Index)
	{
		const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(Profiles[Index]);
		const FString Prefix = FString::Printf(TEXT("Profile %d"), Index);
		TestTrue(Prefix + TEXT(" primary text meets 4.5:1"),
			ContrastRatio(Palette.TextPrimary, Palette.Panel) >= 4.5);
		TestTrue(Prefix + TEXT(" secondary text meets 4.5:1"),
			ContrastRatio(Palette.TextSecondary, Palette.Panel) >= 4.5);
		TestTrue(Prefix + TEXT(" success text meets 4.5:1"),
			ContrastRatio(Palette.Success, Palette.Panel) >= 4.5);
		TestTrue(Prefix + TEXT(" warning text meets 4.5:1"),
			ContrastRatio(Palette.Warning, Palette.Panel) >= 4.5);
		TestTrue(Prefix + TEXT(" danger text meets 4.5:1"),
			ContrastRatio(Palette.Danger, Palette.Panel) >= 4.5);
		TestTrue(Prefix + TEXT(" focus accent meets 3:1"),
			ContrastRatio(Palette.FocusCyan, Palette.Panel) >= 3.0);
		TestTrue(Prefix + TEXT(" primary action accent meets 3:1"),
			ContrastRatio(Palette.ActionAmber, Palette.Panel) >= 3.0);
	}

	return true;
}

#endif

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/UI/Style/APSUIStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSUIStyleSemanticContractTest,
	"APS.UI.Style.SemanticContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSUIStyleSemanticContractTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Balanced 2.2 remains the recommended UI chrome profile"),
		FAPSUIStyle::GetRecommendedDisplayProfile() == EAPSUIDisplayProfile::Balanced22);

	const FAPSUIColorPalette Balanced = FAPSUIStyle::GetPalette(EAPSUIDisplayProfile::Balanced22);
	const FAPSUIColorPalette Cinematic = FAPSUIStyle::GetPalette(EAPSUIDisplayProfile::Cinematic24);
	const FAPSUIColorPalette Bright = FAPSUIStyle::GetPalette(EAPSUIDisplayProfile::Bright20);
	const TArray<FAPSUIColorPalette> Profiles{Balanced, Cinematic, Bright};

	for (int32 Index = 0; Index < Profiles.Num(); ++Index)
	{
		const FAPSUIColorPalette& Palette = Profiles[Index];
		TestTrue(FString::Printf(TEXT("Profile %d keeps the astronomical root transparent"), Index),
			Palette.RootTransparent.A <= KINDA_SMALL_NUMBER);
		TestTrue(FString::Printf(TEXT("Profile %d keeps primary text brighter than its panel"), Index),
			Palette.TextPrimary.GetLuminance() > Palette.Panel.GetLuminance());
		TestTrue(FString::Printf(TEXT("Profile %d exposes a visible focus accent"), Index),
			Palette.FocusCyan.A > 0.9f && Palette.FocusCyan.GetLuminance() > Palette.BorderQuiet.GetLuminance());
		TestTrue(FString::Printf(TEXT("Profile %d separates focus and primary action colors"), Index),
			!Palette.FocusCyan.Equals(Palette.ActionAmber));
	}

	TestTrue(TEXT("Calibration profiles remain visually distinct"),
		!Balanced.Panel.Equals(Cinematic.Panel) && !Balanced.Panel.Equals(Bright.Panel));

	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	TestEqual(TEXT("Primary control target is 44 px"), Layout.ControlMinHeight, 44.0f);
	TestEqual(TEXT("Hierarchy target is 44 px"), Layout.HierarchyRowMinHeight, 44.0f);
	TestTrue(TEXT("Slider hit target contains the visual thumb"),
		Layout.SliderHitHeight >= Layout.SliderHandleHoverSize.Y);
	TestTrue(TEXT("Hovered slider thumb grows without changing the model value"),
		Layout.SliderHandleHoverSize.X > Layout.SliderHandleSize.X);
	TestEqual(TEXT("Spacing scale starts at 4 px"), Layout.Space1, 4.0f);
	TestEqual(TEXT("Slider visual track is 4 px"), Layout.SliderTrackHeight, 4.0f);

	const FAPSUIMotionMetrics Motion = FAPSUIStyle::Motion(false);
	const FAPSUIMotionMetrics ReducedMotion = FAPSUIStyle::Motion(true);
	TestTrue(TEXT("Default transitions remain short and non-zero"),
		Motion.HoverSeconds > 0.0f && Motion.PageTransitionSeconds > Motion.HoverSeconds);
	TestEqual(TEXT("Reduced motion removes hover timing"), ReducedMotion.HoverSeconds, 0.0f);
	TestEqual(TEXT("Reduced motion removes page timing"), ReducedMotion.PageTransitionSeconds, 0.0f);
	TestEqual(TEXT("Reduced motion removes focus timing"), ReducedMotion.FocusTransitionSeconds, 0.0f);

	const FSliderStyle SliderStyle = FAPSUIStyle::MakeSliderStyle(Balanced);
	TestEqual(TEXT("Runtime slider style uses the tokenized track thickness"),
		SliderStyle.BarThickness, Layout.SliderTrackHeight);

	return true;
}

#endif

#include "APSUIParameterSlider.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SAPSUIParameterSlider::Construct(const FArguments& InArgs)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(InArgs._Profile);
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	HintText = InArgs._HintText;
	ValueBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.Control, Layout.ControlRadius, Palette.BorderQuiet, Layout.QuietBorder);
	SliderStyle = MakeShared<FSliderStyle>(FAPSUIStyle::MakeSliderStyle(Palette));

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(InArgs._Label)
				.Font(FAPSUIStyle::BodyFont(TEXT("Bold"), 10))
				.ColorAndOpacity(Palette.TextPrimary)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Layout.Space3, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(ValueBrush.Get())
				.Padding(FMargin(Layout.Space2, Layout.Space1))
				[
					SNew(STextBlock)
					.Text(InArgs._ValueText)
					.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 9))
					.ColorAndOpacity(Palette.FocusCyan)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(HintText)
			.AutoWrapText(true)
			.Visibility_Lambda([this]()
			{
				return HintText.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
			})
			.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9))
			.ColorAndOpacity(Palette.TextSecondary)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space2, 0.0f, 0.0f)
		[
			SNew(SBox)
			.HeightOverride(Layout.SliderHitHeight)
			.VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Style(SliderStyle.Get())
				.Value(InArgs._Value)
				.MinValue(InArgs._MinValue)
				.MaxValue(InArgs._MaxValue)
				.StepSize(InArgs._StepSize)
				.IsEnabled(InArgs._IsEnabled)
				.IsFocusable(true)
				.OnValueChanged(InArgs._OnValueChanged)
				.OnMouseCaptureBegin(InArgs._OnMouseCaptureBegin)
				.OnMouseCaptureEnd(InArgs._OnMouseCaptureEnd)
			]
		]
	];
}

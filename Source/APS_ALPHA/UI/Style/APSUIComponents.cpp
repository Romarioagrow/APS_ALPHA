#include "APSUIComponents.h"

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SAPSUIFactChip::Construct(const FArguments& InArgs)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(InArgs._Profile);
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	const FLinearColor Accent = InArgs._Accent.A > 0.0f ? InArgs._Accent : Palette.FocusCyan;
	BackgroundBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.PanelRaised, Layout.ControlRadius, Accent, Layout.QuietBorder);

	ChildSlot
	[
		SNew(SBox).MinDesiredHeight(Layout.ControlMinHeight)
		[
			SNew(SBorder).BorderImage(BackgroundBrush.Get()).Padding(FMargin(Layout.Space3, Layout.Space2))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(InArgs._Label)
					.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9)).ColorAndOpacity(Palette.TextSecondary)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, Layout.Space1, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(InArgs._Value).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					.Font(FAPSUIStyle::BodyFont(TEXT("Bold"), 11)).ColorAndOpacity(Palette.TextPrimary)
				]
			]
		]
	];
}

void SAPSUIHierarchyCard::Construct(const FArguments& InArgs)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(InArgs._Profile);
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	const int32 Depth = FMath::Clamp(InArgs._Depth, 0, 4);
	IsSelected = InArgs._IsSelected;
	CountLabel = InArgs._CountLabel;
	ButtonStyle = MakeShared<FButtonStyle>(FAPSUIStyle::MakeHierarchyButtonStyle(Palette));
	GlyphBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.Control, Layout.HierarchyRadius, Palette.BorderQuiet, Layout.QuietBorder);
	CountBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.Control, Layout.HierarchyRadius, Palette.BorderQuiet, Layout.QuietBorder);

	ChildSlot
	[
		SNew(SBox).MinDesiredHeight(Layout.HierarchyRowMinHeight)
		[
			SNew(SButton)
			.ButtonStyle(ButtonStyle.Get())
			.ContentPadding(FMargin(Layout.Space2 + Depth * Layout.Space3, Layout.Space2))
			.IsEnabled(InArgs._IsEnabled)
			.OnClicked(InArgs._OnClicked)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
				[
					SNew(SBox).WidthOverride(Layout.SelectedRail)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([this, Palette]()
						{
							return IsSelected.Get() ? Palette.ActionAmber : FLinearColor::Transparent;
						})
					]
				]
				+ SOverlay::Slot().Padding(Layout.Space2, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(28.0f).HeightOverride(28.0f)
						[
							SNew(SBorder).BorderImage(GlyphBrush.Get()).Padding(0.0f)
							[
								SNew(STextBlock).Text(InArgs._Glyph).Justification(ETextJustify::Center)
								.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 8)).ColorAndOpacity(Palette.FocusCyan)
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(Layout.Space2, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(InArgs._Label).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.Font(FAPSUIStyle::BodyFont(TEXT("Bold"), 10)).ColorAndOpacity(Palette.TextPrimary)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(InArgs._Details).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9)).ColorAndOpacity(Palette.TextSecondary)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).MinDesiredWidth(28.0f).HeightOverride(24.0f)
						.Visibility_Lambda([this]()
						{
							return CountLabel.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
						})
						[
							SNew(SBorder).BorderImage(CountBrush.Get()).Padding(FMargin(Layout.Space2, 0.0f))
							[
								SNew(STextBlock).Text(CountLabel).Justification(ETextJustify::Center)
								.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 8)).ColorAndOpacity(Palette.FocusCyan)
							]
						]
					]
				]
			]
		]
	];
}

void SAPSUIReadinessRow::Construct(const FArguments& InArgs)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette(InArgs._Profile);
	const FAPSUILayoutMetrics& Layout = FAPSUIStyle::Metrics();
	const FLinearColor ToneColor = [Tone = InArgs._Tone, &Palette]()
	{
		switch (Tone)
		{
		case EAPSUIStatusTone::Success: return Palette.Success;
		case EAPSUIStatusTone::Warning: return Palette.Warning;
		case EAPSUIStatusTone::Danger: return Palette.Danger;
		default: return Palette.FocusCyan;
		}
	}();
	RowBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.HierarchyRow, Layout.HierarchyRadius, Palette.BorderQuiet, Layout.QuietBorder);
	StatusBrush = MakeShared<FSlateRoundedBoxBrush>(
		Palette.Control, Layout.ControlRadius, ToneColor, Layout.QuietBorder);

	ChildSlot
	[
		SNew(SBox).MinDesiredHeight(Layout.ControlMinHeight)
		[
			SNew(SBorder).BorderImage(RowBrush.Get()).Padding(FMargin(Layout.Space3, Layout.Space2))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(InArgs._Label)
						.Font(FAPSUIStyle::BodyFont(TEXT("Bold"), 10)).ColorAndOpacity(Palette.TextPrimary)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(InArgs._Details).AutoWrapText(true)
						.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9)).ColorAndOpacity(Palette.TextSecondary)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Layout.Space3, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder).BorderImage(StatusBrush.Get()).Padding(FMargin(Layout.Space3, Layout.Space1))
					[
						SNew(STextBlock).Text(InArgs._Status)
						.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 8)).ColorAndOpacity(ToneColor)
					]
				]
			]
		]
	];
}

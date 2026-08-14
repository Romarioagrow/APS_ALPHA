#include "APSUIDisplayProfileSelector.h"

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSUIDisplayProfileSelector"

void SAPSUIDisplayProfileSelector::Construct(const FArguments& InArgs)
{
	CurrentProfile = InArgs._CurrentProfile;
	OnProfileChanged = InArgs._OnProfileChanged;
	Palette = FAPSUIStyle::GetPalette(EAPSUIDisplayProfile::Balanced22);
	Layout = FAPSUIStyle::Metrics();
	ButtonStyle = MakeShared<FButtonStyle>(FAPSUIStyle::MakeSecondaryButtonStyle(Palette));

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, Layout.Space2, 0.0f)
		[
			MakeProfileButton(
				EAPSUIDisplayProfile::Balanced22,
				LOCTEXT("BalancedLabel", "BALANCED 2.2"),
				LOCTEXT("BalancedDescription", "Recommended UI chrome"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, Layout.Space2, 0.0f)
		[
			MakeProfileButton(
				EAPSUIDisplayProfile::Cinematic24,
				LOCTEXT("CinematicLabel", "CINEMATIC 2.4"),
				LOCTEXT("CinematicDescription", "Darker room / richer chrome"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			MakeProfileButton(
				EAPSUIDisplayProfile::Bright20,
				LOCTEXT("BrightLabel", "BRIGHT 2.0"),
				LOCTEXT("BrightDescription", "Bright room / lifted chrome"))
		]
	];
}

TSharedRef<SWidget> SAPSUIDisplayProfileSelector::MakeProfileButton(
	const EAPSUIDisplayProfile Profile,
	const FText& Label,
	const FText& Description)
{
	return SNew(SBox).MinDesiredHeight(Layout.ControlMinHeight)
	[
		SNew(SButton)
		.ButtonStyle(ButtonStyle.Get())
		.ContentPadding(FMargin(Layout.Space3, Layout.Space2))
		.OnClicked(this, &SAPSUIDisplayProfileSelector::SelectProfile, Profile)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this, Profile]()
				{
					return CurrentProfile.Get() == Profile
						? Palette.ActionAmber.CopyWithNewOpacity(0.14f)
						: FLinearColor::Transparent;
				})
			]
			+ SOverlay::Slot().Padding(Layout.Space2, Layout.Space1)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Label)
					.Font(FAPSUIStyle::DisplayFont(TEXT("Bold"), 9))
					.ColorAndOpacity_Lambda([this, Profile]()
					{
						return CurrentProfile.Get() == Profile ? Palette.ActionAmber : Palette.TextPrimary;
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Description).AutoWrapText(true)
					.Font(FAPSUIStyle::BodyFont(TEXT("Regular"), 9))
					.ColorAndOpacity(Palette.TextSecondary)
				]
			]
		]
	];
}

FReply SAPSUIDisplayProfileSelector::SelectProfile(const EAPSUIDisplayProfile Profile)
{
	OnProfileChanged.ExecuteIfBound(Profile);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

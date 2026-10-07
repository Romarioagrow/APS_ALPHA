#include "SAPSWalkerHud.h"

#include "APSHudKit.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSWalkerHud"

namespace APSWalkerHudPrivate
{
	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	TAttribute<FSlateColor> Colour(TFunction<FLinearColor()> Pick)
	{
		return TAttribute<FSlateColor>::CreateLambda([Pick]() { return FSlateColor(Pick()); });
	}

	FString Sentence(const FString& Text)
	{
		FString Result = Text.ToLower();
		if (Result.Len() > 0)
		{
			Result[0] = FChar::ToUpper(Result[0]);
		}
		return Result;
	}
}

SAPSWalkerHud::FStatus SAPSWalkerHud::ReadStatus() const
{
	FStatus Status;
	TArray<FString> Parts;
	StatusText.Get().ToString().ParseIntoArray(Parts, TEXT("|"), true);
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		const FString Part = Parts[Index].TrimStartAndEnd();
		if (Index == 0)
		{
			FString Left;
			FString Right;
			if (Part.Split(TEXT(" / "), &Left, &Right))
			{
				Status.GravityLabel = Left;
				Status.Gravity = Right;
			}
			else
			{
				Status.GravityLabel = TEXT("GRAVITY");
				Status.Gravity = Part;
			}
		}
		else if (Part.Equals(TEXT("BOOST")))
		{
			Status.bBoost = true;
		}
		else if (Part.StartsWith(TEXT("PACE ")))
		{
			// "PACE 1 WALK": the key number is on the hint line, the bar keeps the word.
			FString Pace = Part.Mid(5).TrimStart();
			if (Pace.Len() > 2 && FChar::IsDigit(Pace[0]) && Pace[1] == TEXT(' '))
			{
				Pace = Pace.Mid(2);
			}
			Status.Pace = Pace;
		}
		else if (Status.Speed.IsEmpty())
		{
			Status.Speed = Part;
		}
	}
	return Status;
}

void SAPSWalkerHud::Construct(const FArguments& InArgs)
{
	using namespace APSWalkerHudPrivate;
	StatusText = InArgs._StatusText;
	BuildText = InArgs._BuildText;
	Alert = InArgs._Alert;

	const auto ColumnLabel = [](const TAttribute<FText>& Text)
	{
		return APSHud::Label(Text, Colour([]() { return P().TextQuiet; }));
	};

	const TSharedRef<SWidget> Bar = APSHud::Card(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).MinDesiredWidth(150.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[ColumnLabel(TAttribute<FText>::CreateLambda([this]() { return FText::FromString(ReadStatus().GravityLabel); }))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(APSHud::ValueFont(14))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(Alert.Get(false) ? P().Action : P().Text); })
					.Text_Lambda([this]() { return FText::FromString(ReadStatus().Gravity); })
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14.0f, 0.0f)[APSHud::Rule(true)]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).MinDesiredWidth(110.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ColumnLabel(LOCTEXT("Speed", "SPEED"))]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(LOCTEXT("Boost", "BOOST")).Font(APSHud::LabelFont())
						.ColorAndOpacity(Colour([]() { return P().Action; }))
						.Visibility_Lambda([this]() { return ReadStatus().bBoost ? EVisibility::Visible : EVisibility::Collapsed; })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[APSHud::ValueWithUnit(TAttribute<FString>::CreateLambda([this]() { return ReadStatus().Speed; }), 16)]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14.0f, 0.0f)
		[
			SNew(SBox).Visibility_Lambda([this]() { return ReadStatus().Pace.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			[APSHud::Rule(true)]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return ReadStatus().Pace.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			+ SVerticalBox::Slot().AutoHeight()[ColumnLabel(LOCTEXT("Pace", "PACE"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(APSHud::ValueFont(14)).ColorAndOpacity(Colour([]() { return P().Highlight; }))
				.Text_Lambda([this]() { return FText::FromString(ReadStatus().Pace); })
			]
		],
		APSHud::EEdge::Centre, FMargin(18.0f, 11.0f, 18.0f, 12.0f));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 48.0f)
		[
			SNew(SVerticalBox)
			// Build mode (Rio 02.10): where it can start, and for a moment why it could not.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock).Font(APSHud::TextFont(12, TEXT("SemiBold"))).Justification(ETextJustify::Center)
				.ColorAndOpacity(Colour([]() { return P().ActionPeak; }))
				.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f)).ShadowOffset(FVector2D(1.0f, 1.0f))
				.Text(BuildText)
				.Visibility_Lambda([this]() { return BuildText.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Bar]
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(60.0f, 0.0f, 60.0f, 14.0f)
		[
			SNew(SAPSHudKeyHints).Text(InArgs._HintText)
		]
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

void SAPSTakeControlPrompt::Construct(const FArguments& InArgs)
{
	using namespace APSWalkerHudPrivate;
	const TAttribute<FText> Subject = InArgs._Subject;
	ChildSlot
	[
		APSHud::Card(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[APSChrome::KeyChip(LOCTEXT("TakeControlKey", "F"), P().Text)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("TakeControl", "TAKE CONTROL")).Font(APSHud::ValueFont(13))
				.ColorAndOpacity(Colour([]() { return P().Text; }))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Subject).Font(APSHud::TextFont(12)).ColorAndOpacity(Colour([]() { return P().TextSoft; }))
			],
			APSHud::EEdge::Centre, FMargin(14.0f, 9.0f))
	];
	SetVisibility(EVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE

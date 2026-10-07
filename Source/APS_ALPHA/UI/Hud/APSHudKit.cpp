#include "APSHudKit.h"

#include "APS_ALPHA/UI/MainMenu/SAPSChamferedOverlay.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace APSHudPrivate
{
	const FAPSUIThemePalette& P()
	{
		return APSUITheme::Palette();
	}

	bool IsKeyToken(const FString& Token)
	{
		if (Token.IsEmpty() || Token.EndsWith(TEXT(":")))
		{
			return false;
		}
		if (Token.Len() == 1 || Token.Contains(TEXT("/")) || (Token.Contains(TEXT("-")) && Token.Len() <= 5))
		{
			return true;
		}
		static const TCHAR* Named[] = {TEXT("SHIFT"), TEXT("CTRL"), TEXT("ALT"), TEXT("SPACE"), TEXT("TAB"), TEXT("ESC"),
			TEXT("ENTER"), TEXT("WASD"), TEXT("MOUSE"), TEXT("LMB"), TEXT("RMB"), TEXT("WHEEL")};
		for (const TCHAR* Name : Named)
		{
			if (Token.Equals(Name, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		// F1..F12.
		return Token.Len() <= 3 && Token[0] == TEXT('F') && FChar::IsDigit(Token[1]);
	}

	FString SentenceCase(const FString& Text)
	{
		FString Result = Text.ToLower();
		if (Result.Len() > 0)
		{
			Result[0] = FChar::ToUpper(Result[0]);
		}
		return Result;
	}
}

TSharedRef<SWidget> APSHud::Card(const TSharedRef<SWidget>& Content, const EEdge Edge, const FMargin& Padding)
{
	using namespace APSHudPrivate;
	const EHorizontalAlignment EdgeAlign = Edge == EEdge::Left ? HAlign_Left : Edge == EEdge::Right ? HAlign_Right : HAlign_Center;
	return SNew(SAPSChamferedOverlay)
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedSurface).Brush(FAppStyle::GetBrush("WhiteBrush")).ChamferTop(true).ChamferBottom(true)
			.Tint_Lambda([]() { return P().Panel.CopyWithNewOpacity(0.84f); })
		]
		+ SOverlay::Slot().Padding(Padding)
		[
			Content
		]
		+ SOverlay::Slot().HAlign(EdgeAlign).VAlign(VAlign_Top).Padding(FMargin(14.0f, 0.0f))
		[
			SNew(SBox).WidthOverride(Edge == EEdge::Centre ? 110.0f : 84.0f).HeightOverride(2.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([]() { return FSlateColor(P().Action); })
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SAPSChamferedFrame).Thickness(1.0f).Color_Lambda([]() { return P().Frame; })
		];
}

FSlateFontInfo APSHud::LabelFont(const int32 Size)
{
	FSlateFontInfo Font = APSUITheme::DisplayFont("Bold", Size);
	Font.LetterSpacing = 160;
	return Font;
}

FSlateFontInfo APSHud::ValueFont(const int32 Size)
{
	return APSUITheme::DisplayFont("Bold", Size);
}

FSlateFontInfo APSHud::TextFont(const int32 Size, const FName Typeface)
{
	return APSUITheme::BodyFont(Typeface, Size);
}

TSharedRef<SWidget> APSHud::Label(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Colour)
{
	return SNew(STextBlock).Text(Text).Font(LabelFont()).ColorAndOpacity(Colour);
}

TSharedRef<SWidget> APSHud::ValueWithUnit(const TAttribute<FString>& Formatted, const int32 Size, const float ValueWidth)
{
	using namespace APSHudPrivate;
	const bool bFixed = ValueWidth > 0.0f;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			// Rio 06.10 (audit: an oversized value ran over the divider in a fixed box): a minimum width, so values that fit
			// sit right-justified as before and a longer one widens the box.
			SNew(SBox).MinDesiredWidth(bFixed ? FOptionalSize(ValueWidth) : FOptionalSize())
			[
				SNew(STextBlock).Font(ValueFont(Size)).ColorAndOpacity_Lambda([]() { return FSlateColor(P().Text); })
				.Justification(bFixed ? ETextJustify::Right : ETextJustify::Left)
				.Text_Lambda([Formatted]()
				{
					FString Value;
					FString Unit;
					SplitValueUnit(Formatted.Get(), Value, Unit);
					return FText::FromString(Value);
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(5.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock).Font(TextFont(FMath::Max(10, FMath::RoundToInt(Size * 0.6f))))
			.ColorAndOpacity_Lambda([]() { return FSlateColor(P().TextQuiet); })
			.Text_Lambda([Formatted]()
			{
				FString Value;
				FString Unit;
				SplitValueUnit(Formatted.Get(), Value, Unit);
				return FText::FromString(Unit);
			})
		];
}

TSharedRef<SWidget> APSHud::Rule(const bool bVertical)
{
	return SNew(SBox).WidthOverride(bVertical ? 1.0f : FOptionalSize()).HeightOverride(bVertical ? FOptionalSize() : 1.0f)
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([]() { return FSlateColor(APSUITheme::Fade(APSHudPrivate::P().Frame, 0.75f)); })
	];
}

void APSHud::SplitValueUnit(const FString& Formatted, FString& OutValue, FString& OutUnit)
{
	const FString Trimmed = Formatted.TrimStartAndEnd();
	int32 Space = INDEX_NONE;
	if (Trimmed.FindLastChar(TEXT(' '), Space) && Space > 0)
	{
		OutValue = Trimmed.Left(Space).TrimEnd();
		OutUnit = Trimmed.Mid(Space + 1);
		return;
	}
	OutValue = Trimmed;
	OutUnit.Reset();
}

TArray<APSHud::FKeyHint> APSHud::ParseKeyHints(const FString& Line)
{
	using namespace APSHudPrivate;
	TArray<FKeyHint> Hints;
	TArray<FString> Groups;
	Line.Replace(TEXT("\n"), TEXT("|")).ParseIntoArray(Groups, TEXT("|"), true);
	for (const FString& Group : Groups)
	{
		TArray<FString> Items;
		Group.ParseIntoArray(Items, TEXT("   "), true);
		for (const FString& RawItem : Items)
		{
			const FString Item = RawItem.TrimStartAndEnd();
			if (Item.IsEmpty())
			{
				continue;
			}
			TArray<FString> Words;
			Item.ParseIntoArray(Words, TEXT(" "), true);
			int32 KeyCount = 0;
			while (KeyCount < Words.Num() - 1 && IsKeyToken(Words[KeyCount]))
			{
				++KeyCount;
			}
			FKeyHint& Hint = Hints.AddDefaulted_GetRef();
			Hint.Keys = FString::Join(TArrayView<const FString>(Words.GetData(), KeyCount), TEXT(" "));
			Hint.Label = SentenceCase(FString::Join(TArrayView<const FString>(Words.GetData() + KeyCount, Words.Num() - KeyCount), TEXT(" ")));
		}
	}
	return Hints;
}

void SAPSHudKeyHints::Construct(const FArguments& InArgs)
{
	Text = InArgs._Text;
	ChildSlot
	[
		SAssignNew(Row, SWrapBox)
		.UseAllottedSize(true)
		.HAlign(HAlign_Center)
		.InnerSlotPadding(FVector2D(18.0f, 6.0f))
	];
	SetVisibility(EVisibility::HitTestInvisible);
	Rebuild(Text.Get().ToString());
	RegisterActiveTimer(0.2f, FWidgetActiveTimerDelegate::CreateSP(this, &SAPSHudKeyHints::Refresh));
}

EActiveTimerReturnType SAPSHudKeyHints::Refresh(double, float)
{
	const FString Line = Text.Get().ToString();
	if (Line != Shown)
	{
		Rebuild(Line);
	}
	return EActiveTimerReturnType::Continue;
}

void SAPSHudKeyHints::Rebuild(const FString& Line)
{
	using namespace APSHudPrivate;
	Shown = Line;
	if (!Row.IsValid())
	{
		return;
	}
	Row->ClearChildren();
	for (const APSHud::FKeyHint& Hint : APSHud::ParseKeyHints(Line))
	{
		const TSharedRef<SHorizontalBox> Item = SNew(SHorizontalBox);
		if (!Hint.Keys.IsEmpty())
		{
			Item->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
			[
				APSChrome::KeyChip(FText::FromString(Hint.Keys), P().Text)
			];
		}
		Item->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(Hint.Label)).Font(APSHud::TextFont(12, Hint.Keys.IsEmpty() ? FName(TEXT("SemiBold")) : FName(TEXT("Regular"))))
			.ColorAndOpacity(Hint.Keys.IsEmpty() ? P().Highlight : P().TextSoft)
			.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f)).ShadowOffset(FVector2D(1.0f, 1.0f))
		];
		Row->AddSlot()[Item];
	}
}

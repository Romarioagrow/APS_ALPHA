#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SWrapBox;

/**
 * Rio 06.10 HUD (mockup "Flight HUD v4"): one instrument family for every in-game panel. Dark glass cards with a quiet
 * frame and a short action-coloured edge, Chakra Petch labels and numbers, Exo 2 text, everything from the interface
 * theme (APSUITheme), so the HUD follows SETTINGS / INTERFACE like the menu.
 */
namespace APSHud
{
	enum class EEdge : uint8
	{
		Left,
		Centre,
		Right
	};

	/** A HUD card: chamfered glass, quiet frame, a short action-coloured edge on top at Edge. */
	APS_ALPHA_API TSharedRef<SWidget> Card(const TSharedRef<SWidget>& Content, EEdge Edge,
		const FMargin& Padding = FMargin(14.0f, 11.0f, 14.0f, 12.0f));

	/** Small spaced capitals: a section's name (TASKS, NAVIGATION, SPEED). */
	APS_ALPHA_API FSlateFontInfo LabelFont(int32 Size = 9);
	/** Numbers and short values (Chakra Petch). */
	APS_ALPHA_API FSlateFontInfo ValueFont(int32 Size);
	/** Running text (Exo 2). */
	APS_ALPHA_API FSlateFontInfo TextFont(int32 Size, FName Typeface = TEXT("Regular"));

	/** A label in small spaced capitals. */
	APS_ALPHA_API TSharedRef<SWidget> Label(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& Colour);
	/** A value with its unit after it ("9.0314" "ly"), both read from one formatted string. With a ValueWidth the number
	 * is right-aligned in a box that wide, so the unit stays put while the digits change (Rio 06.10: "the m/s jumps"). */
	APS_ALPHA_API TSharedRef<SWidget> ValueWithUnit(const TAttribute<FString>& Formatted, int32 Size, float ValueWidth = 0.0f);
	/** A one-pixel rule in the frame colour. */
	APS_ALPHA_API TSharedRef<SWidget> Rule(bool bVertical = false);

	/** "9.0314 ly" -> "9.0314" and "ly"; a string without a space is all value. */
	APS_ALPHA_API void SplitValueUnit(const FString& Formatted, FString& OutValue, FString& OutUnit);

	/** One key hint: the keys ("W/S", "N M T V Y") and what they do; a status word has no keys. */
	struct FKeyHint
	{
		FString Keys;
		FString Label;
	};

	/** Splits a hint line ("W/S THRUST   SHIFT BOOST   |   Z AUTOPILOT") into key hints: groups by '|' or a line
	 * break, hints by three spaces, keys are the leading key-like words. Labels come back in sentence case. */
	APS_ALPHA_API TArray<FKeyHint> ParseKeyHints(const FString& Line);
}

/** A centred row of key chips and their labels, rebuilt only when the hint line changes. */
class APS_ALPHA_API SAPSHudKeyHints final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSHudKeyHints) {}
		SLATE_ATTRIBUTE(FText, Text)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EActiveTimerReturnType Refresh(double CurrentTime, float DeltaTime);
	void Rebuild(const FString& Line);

	TAttribute<FText> Text;
	FString Shown;
	TSharedPtr<SWrapBox> Row;
};

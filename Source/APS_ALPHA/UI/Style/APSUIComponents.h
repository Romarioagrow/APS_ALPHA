#pragma once

#include "CoreMinimal.h"
#include "APSUIStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"

enum class EAPSUIStatusTone : uint8
{
	Neutral,
	Success,
	Warning,
	Danger
};

/** Compact two-line fact chip for canonical Live Model values. */
class APS_ALPHA_API SAPSUIFactChip final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIFactChip)
		: _Profile(EAPSUIDisplayProfile::Balanced22)
		, _Accent(FLinearColor::Transparent)
	{}
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, Value)
		SLATE_ARGUMENT(EAPSUIDisplayProfile, Profile)
		SLATE_ARGUMENT(FLinearColor, Accent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<FSlateRoundedBoxBrush> BackgroundBrush;
};

/** 44 px hierarchy target with indentation, details and a selected rail. */
class APS_ALPHA_API SAPSUIHierarchyCard final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIHierarchyCard)
		: _Depth(0)
		, _IsSelected(false)
		, _IsEnabled(true)
		, _Profile(EAPSUIDisplayProfile::Balanced22)
	{}
		SLATE_ATTRIBUTE(FText, Glyph)
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, Details)
		SLATE_ATTRIBUTE(FText, CountLabel)
		SLATE_ARGUMENT(int32, Depth)
		SLATE_ATTRIBUTE(bool, IsSelected)
		SLATE_ATTRIBUTE(bool, IsEnabled)
		SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_ARGUMENT(EAPSUIDisplayProfile, Profile)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TAttribute<bool> IsSelected;
	TAttribute<FText> CountLabel;
	TSharedPtr<FButtonStyle> ButtonStyle;
	TSharedPtr<FSlateRoundedBoxBrush> GlyphBrush;
	TSharedPtr<FSlateRoundedBoxBrush> CountBrush;
};

/** Readable readiness row with a text status in addition to semantic color. */
class APS_ALPHA_API SAPSUIReadinessRow final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIReadinessRow)
		: _Tone(EAPSUIStatusTone::Neutral)
		, _Profile(EAPSUIDisplayProfile::Balanced22)
	{}
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, Details)
		SLATE_ATTRIBUTE(FText, Status)
		SLATE_ARGUMENT(EAPSUIStatusTone, Tone)
		SLATE_ARGUMENT(EAPSUIDisplayProfile, Profile)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<FSlateRoundedBoxBrush> RowBrush;
	TSharedPtr<FSlateRoundedBoxBrush> StatusBrush;
};

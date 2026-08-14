#pragma once

#include "CoreMinimal.h"
#include "APSUIStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"

enum class EAPSUIActionTone : uint8
{
	Secondary,
	Primary,
	Danger
};

/** Accessible gameplay/menu action row. It owns presentation, never navigation state. */
class APS_ALPHA_API SAPSUIActionButton final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIActionButton)
		: _Tone(EAPSUIActionTone::Secondary)
		, _IsEnabled(true)
		, _Profile(EAPSUIDisplayProfile::Balanced22)
	{}
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, Description)
		SLATE_ATTRIBUTE(FText, Shortcut)
		SLATE_ARGUMENT(EAPSUIActionTone, Tone)
		SLATE_ATTRIBUTE(bool, IsEnabled)
		SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_ARGUMENT(EAPSUIDisplayProfile, Profile)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TAttribute<FText> Description;
	TAttribute<FText> Shortcut;
	TSharedPtr<FButtonStyle> ButtonStyle;
	TSharedPtr<FSlateRoundedBoxBrush> ShortcutBrush;
};

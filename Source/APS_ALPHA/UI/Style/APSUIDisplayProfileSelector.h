#pragma once

#include "CoreMinimal.h"
#include "APSUIStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FOnAPSUIDisplayProfileChanged, EAPSUIDisplayProfile)

/** Controlled selector for UI-chrome calibration. It never changes scene rendering. */
class APS_ALPHA_API SAPSUIDisplayProfileSelector final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIDisplayProfileSelector)
		: _CurrentProfile(EAPSUIDisplayProfile::Balanced22)
	{}
		SLATE_ATTRIBUTE(EAPSUIDisplayProfile, CurrentProfile)
		SLATE_EVENT(FOnAPSUIDisplayProfileChanged, OnProfileChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> MakeProfileButton(
		EAPSUIDisplayProfile Profile,
		const FText& Label,
		const FText& Description);
	FReply SelectProfile(EAPSUIDisplayProfile Profile);

	TAttribute<EAPSUIDisplayProfile> CurrentProfile;
	FOnAPSUIDisplayProfileChanged OnProfileChanged;
	FAPSUIColorPalette Palette;
	FAPSUILayoutMetrics Layout;
	TSharedPtr<FButtonStyle> ButtonStyle;
};

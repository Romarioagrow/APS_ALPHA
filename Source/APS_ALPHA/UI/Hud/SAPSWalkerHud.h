#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Rio 06.10 HUD on foot, the ship HUD's family: a compact bar at the bottom centre (gravity, speed, pace), the key
 * hints under it, a build-mode line when it has something to say, and the take-control prompt above the bar.
 */
class SAPSWalkerHud final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSWalkerHud) {}
		/** The traversal status line: "GRAVITY / PLANET | 4.2 m/s | BOOST | PACE 1 WALK". */
		SLATE_ATTRIBUTE(FText, StatusText)
		SLATE_ATTRIBUTE(FText, HintText)
		SLATE_ATTRIBUTE(FText, BuildText)
		/** Manual zero-G: the gravity value in the action colour. */
		SLATE_ATTRIBUTE(bool, Alert)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FStatus
	{
		FString GravityLabel;
		FString Gravity;
		FString Speed;
		FString Pace;
		bool bBoost{false};
	};
	FStatus ReadStatus() const;

	TAttribute<FText> StatusText;
	TAttribute<FText> BuildText;
	TAttribute<bool> Alert;
};

/** Rio 06.10: the walker's take-control prompt as a HUD card: the F key, TAKE CONTROL and what it is. */
class SAPSTakeControlPrompt final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSTakeControlPrompt) {}
		SLATE_ATTRIBUTE(FText, Subject)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

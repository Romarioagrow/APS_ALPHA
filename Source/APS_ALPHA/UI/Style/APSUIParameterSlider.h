#pragma once

#include "CoreMinimal.h"
#include "APSUIStyle.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Controlled parameter slider for APOSFERA panels.
 *
 * The consumer owns the value and formatting. This widget only supplies the
 * large pointer target, semantic chrome and an exact textual readout, so it can
 * wrap existing generation delegates without duplicating model state.
 */
class APS_ALPHA_API SAPSUIParameterSlider final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSUIParameterSlider)
		: _Value(0.0f)
		, _MinValue(0.0f)
		, _MaxValue(1.0f)
		, _StepSize(0.01f)
		, _IsEnabled(true)
		, _Profile(EAPSUIDisplayProfile::Balanced22)
	{}
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, ValueText)
		SLATE_ATTRIBUTE(FText, HintText)
		SLATE_ATTRIBUTE(float, Value)
		SLATE_ARGUMENT(float, MinValue)
		SLATE_ARGUMENT(float, MaxValue)
		SLATE_ARGUMENT(float, StepSize)
		SLATE_ATTRIBUTE(bool, IsEnabled)
		SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureBegin)
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureEnd)
		SLATE_ARGUMENT(EAPSUIDisplayProfile, Profile)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TAttribute<FText> HintText;
	TSharedPtr<FSlateRoundedBoxBrush> ValueBrush;
	TSharedPtr<FSliderStyle> SliderStyle;
};

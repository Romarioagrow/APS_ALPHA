#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateTypes.h"

/**
 * APS-76 display calibration affects opaque UI chrome only. The astronomical
 * viewport stays transparent and must never inherit scene exposure, renderer
 * gamma, post-process, or material changes from this profile.
 */
enum class EAPSUIDisplayProfile : uint8
{
	Balanced22,
	Cinematic24,
	Bright20
};

struct FAPSUIColorPalette
{
	FLinearColor RootTransparent;
	FLinearColor Scrim;
	FLinearColor Panel;
	FLinearColor PanelRaised;
	FLinearColor Control;
	FLinearColor HierarchyRow;
	FLinearColor HierarchyHover;
	FLinearColor BorderQuiet;
	FLinearColor FocusCyan;
	FLinearColor ActionAmber;
	FLinearColor TextPrimary;
	FLinearColor TextSecondary;
	FLinearColor Success;
	FLinearColor Warning;
	FLinearColor Danger;
};

struct FAPSUILayoutMetrics
{
	float Space1{4.0f};
	float Space2{8.0f};
	float Space3{12.0f};
	float Space4{16.0f};
	float Space5{24.0f};
	float Space6{32.0f};

	float PanelRadius{10.0f};
	float ControlRadius{6.0f};
	float HierarchyRadius{5.0f};
	float QuietBorder{1.0f};
	float FocusBorder{2.0f};
	float SelectedRail{3.0f};

	float ControlMinHeight{44.0f};
	float HierarchyRowMinHeight{44.0f};
	float SliderHitHeight{32.0f};
	float SliderTrackHeight{4.0f};
	FVector2D SliderHandleSize{20.0f, 20.0f};
	FVector2D SliderHandleHoverSize{24.0f, 24.0f};
	float IconButtonTarget{40.0f};
};

struct FAPSUIMotionMetrics
{
	float HoverSeconds{0.12f};
	float PageTransitionSeconds{0.18f};
	float FocusTransitionSeconds{0.10f};
};

/**
 * Shared APOSFERA chrome contract. It intentionally contains no model, save,
 * placement, spawn, or viewport state.
 */
class APS_ALPHA_API FAPSUIStyle final
{
public:
	static EAPSUIDisplayProfile GetRecommendedDisplayProfile();
	static FAPSUIColorPalette GetPalette(EAPSUIDisplayProfile Profile = EAPSUIDisplayProfile::Balanced22);
	static const FAPSUILayoutMetrics& Metrics();
	static FAPSUIMotionMetrics Motion(bool bReduceMotion = false);

	static FSlateFontInfo DisplayFont(const FName Typeface, int32 Size);
	static FSlateFontInfo BodyFont(const FName Typeface, int32 Size);

	static FSlateRoundedBoxBrush MakePanelBrush(const FAPSUIColorPalette& Palette);
	static FSlateRoundedBoxBrush MakeRaisedPanelBrush(const FAPSUIColorPalette& Palette);
	static FSlateRoundedBoxBrush MakeControlBrush(const FAPSUIColorPalette& Palette);
	static FButtonStyle MakeSecondaryButtonStyle(const FAPSUIColorPalette& Palette);
	static FButtonStyle MakePrimaryButtonStyle(const FAPSUIColorPalette& Palette);
	static FButtonStyle MakeHierarchyButtonStyle(const FAPSUIColorPalette& Palette);
	static FSliderStyle MakeSliderStyle(const FAPSUIColorPalette& Palette);

private:
	static FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255);
};


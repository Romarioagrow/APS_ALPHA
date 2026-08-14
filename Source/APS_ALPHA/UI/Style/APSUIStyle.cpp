#include "APSUIStyle.h"

#include "Engine/Font.h"
#include "Styling/CoreStyle.h"

namespace APSUIStylePrivate
{
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> DisplayMediumFont;

	UFont* LoadDisplayFont(const FName Typeface)
	{
		TWeakObjectPtr<UFont>& FontSlot = Typeface == TEXT("Bold") ? DisplayFont : DisplayMediumFont;
		if (!FontSlot.IsValid())
		{
			const TCHAR* FontPath = Typeface == TEXT("Bold")
				? TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font")
				: TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font");
			FontSlot = LoadObject<UFont>(nullptr, FontPath);
		}
		return FontSlot.Get();
	}

	FSlateRoundedBoxBrush FillBrush(const FLinearColor& Fill, const float Radius, const FVector2D ImageSize = FVector2D::ZeroVector)
	{
		FSlateRoundedBoxBrush Brush(Fill, Radius);
		if (!ImageSize.IsZero())
		{
			Brush.ImageSize = ImageSize;
		}
		return Brush;
	}

	FLinearColor Disabled(const FLinearColor& Color)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Color.A * 0.45f);
	}
}

EAPSUIDisplayProfile FAPSUIStyle::GetRecommendedDisplayProfile()
{
	return EAPSUIDisplayProfile::Balanced22;
}

FAPSUIColorPalette FAPSUIStyle::GetPalette(const EAPSUIDisplayProfile Profile)
{
	FAPSUIColorPalette Palette;
	Palette.RootTransparent = FLinearColor::Transparent;

	switch (Profile)
	{
	case EAPSUIDisplayProfile::Cinematic24:
		Palette.Scrim = SRGB(1, 5, 10, 214);
		Palette.Panel = SRGB(3, 18, 28, 242);
		Palette.PanelRaised = SRGB(6, 27, 39, 250);
		Palette.Control = SRGB(4, 23, 32, 247);
		Palette.HierarchyRow = SRGB(6, 28, 39, 250);
		Palette.HierarchyHover = SRGB(9, 45, 60);
		Palette.BorderQuiet = SRGB(36, 93, 112);
		Palette.FocusCyan = SRGB(42, 207, 255);
		Palette.ActionAmber = SRGB(255, 151, 34);
		Palette.TextPrimary = SRGB(233, 247, 250);
		Palette.TextSecondary = SRGB(160, 189, 200);
		Palette.Success = SRGB(85, 221, 169);
		Palette.Warning = SRGB(255, 188, 80);
		Palette.Danger = SRGB(255, 97, 119);
		break;

	case EAPSUIDisplayProfile::Bright20:
		Palette.Scrim = SRGB(7, 23, 32, 184);
		Palette.Panel = SRGB(16, 42, 54, 245);
		Palette.PanelRaised = SRGB(21, 55, 68, 250);
		Palette.Control = SRGB(16, 47, 59, 250);
		Palette.HierarchyRow = SRGB(18, 51, 63, 250);
		Palette.HierarchyHover = SRGB(25, 73, 90);
		Palette.BorderQuiet = SRGB(74, 143, 163);
		Palette.FocusCyan = SRGB(88, 221, 255);
		Palette.ActionAmber = SRGB(255, 172, 69);
		Palette.TextPrimary = SRGB(245, 251, 253);
		Palette.TextSecondary = SRGB(186, 208, 216);
		Palette.Success = SRGB(114, 233, 189);
		Palette.Warning = SRGB(255, 208, 113);
		Palette.Danger = SRGB(255, 120, 138);
		break;

	case EAPSUIDisplayProfile::Balanced22:
	default:
		Palette.Scrim = SRGB(2, 8, 13, 199);
		Palette.Panel = SRGB(6, 25, 35, 240);
		Palette.PanelRaised = SRGB(10, 37, 50, 250);
		Palette.Control = SRGB(7, 30, 41, 247);
		Palette.HierarchyRow = SRGB(8, 35, 47, 250);
		Palette.HierarchyHover = SRGB(12, 52, 68);
		Palette.BorderQuiet = SRGB(45, 111, 132);
		Palette.FocusCyan = SRGB(53, 212, 255);
		Palette.ActionAmber = SRGB(255, 156, 42);
		Palette.TextPrimary = SRGB(237, 248, 252);
		Palette.TextSecondary = SRGB(165, 195, 205);
		Palette.Success = SRGB(94, 228, 177);
		Palette.Warning = SRGB(255, 193, 90);
		Palette.Danger = SRGB(255, 101, 122);
		break;
	}

	return Palette;
}

const FAPSUILayoutMetrics& FAPSUIStyle::Metrics()
{
	static const FAPSUILayoutMetrics Value;
	return Value;
}

FAPSUIMotionMetrics FAPSUIStyle::Motion(const bool bReduceMotion)
{
	if (bReduceMotion)
	{
		return FAPSUIMotionMetrics{0.0f, 0.0f, 0.0f};
	}
	return FAPSUIMotionMetrics{};
}

FSlateFontInfo FAPSUIStyle::DisplayFont(const FName Typeface, const int32 Size)
{
	if (UFont* FontObject = APSUIStylePrivate::LoadDisplayFont(Typeface))
	{
		return FSlateFontInfo(FontObject, Size, Typeface);
	}
	return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
}

FSlateFontInfo FAPSUIStyle::BodyFont(const FName Typeface, const int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
}

FSlateRoundedBoxBrush FAPSUIStyle::MakePanelBrush(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	return FSlateRoundedBoxBrush(Palette.Panel, Layout.PanelRadius, Palette.BorderQuiet, Layout.QuietBorder);
}

FSlateRoundedBoxBrush FAPSUIStyle::MakeRaisedPanelBrush(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	return FSlateRoundedBoxBrush(Palette.PanelRaised, Layout.PanelRadius, Palette.BorderQuiet, Layout.QuietBorder);
}

FSlateRoundedBoxBrush FAPSUIStyle::MakeControlBrush(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	return FSlateRoundedBoxBrush(Palette.Control, Layout.ControlRadius, Palette.BorderQuiet, Layout.QuietBorder);
}

FButtonStyle FAPSUIStyle::MakeSecondaryButtonStyle(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	return FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Palette.Control, Layout.ControlRadius, Palette.BorderQuiet, Layout.QuietBorder))
		.SetHovered(FSlateRoundedBoxBrush(Palette.HierarchyHover, Layout.ControlRadius, Palette.FocusCyan, 1.5f))
		.SetPressed(FSlateRoundedBoxBrush(Palette.HierarchyHover, Layout.ControlRadius, Palette.FocusCyan, Layout.FocusBorder))
		.SetDisabled(FSlateRoundedBoxBrush(APSUIStylePrivate::Disabled(Palette.Control), Layout.ControlRadius,
			APSUIStylePrivate::Disabled(Palette.BorderQuiet), Layout.QuietBorder));
}

FButtonStyle FAPSUIStyle::MakePrimaryButtonStyle(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	const FLinearColor NormalFill(Palette.ActionAmber.R * 0.30f, Palette.ActionAmber.G * 0.18f,
		Palette.ActionAmber.B * 0.08f, 0.98f);
	const FLinearColor HoverFill(Palette.ActionAmber.R * 0.48f, Palette.ActionAmber.G * 0.28f,
		Palette.ActionAmber.B * 0.10f, 1.0f);
	return FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(NormalFill, Layout.ControlRadius, Palette.ActionAmber, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(HoverFill, Layout.ControlRadius, Palette.Warning, Layout.FocusBorder))
		.SetPressed(FSlateRoundedBoxBrush(Palette.ActionAmber, Layout.ControlRadius, Palette.Warning, Layout.FocusBorder))
		.SetDisabled(FSlateRoundedBoxBrush(APSUIStylePrivate::Disabled(NormalFill), Layout.ControlRadius,
			APSUIStylePrivate::Disabled(Palette.ActionAmber), Layout.QuietBorder));
}

FButtonStyle FAPSUIStyle::MakeHierarchyButtonStyle(const FAPSUIColorPalette& Palette)
{
	const FAPSUILayoutMetrics& Layout = Metrics();
	return FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Palette.HierarchyRow, Layout.HierarchyRadius, Palette.BorderQuiet, Layout.QuietBorder))
		.SetHovered(FSlateRoundedBoxBrush(Palette.HierarchyHover, Layout.HierarchyRadius, Palette.FocusCyan, 1.25f))
		.SetPressed(FSlateRoundedBoxBrush(Palette.HierarchyHover, Layout.HierarchyRadius, Palette.FocusCyan, 1.5f))
		.SetDisabled(FSlateRoundedBoxBrush(APSUIStylePrivate::Disabled(Palette.HierarchyRow), Layout.HierarchyRadius,
			APSUIStylePrivate::Disabled(Palette.BorderQuiet), Layout.QuietBorder));
}

FSliderStyle FAPSUIStyle::MakeSliderStyle(const FAPSUIColorPalette& Palette)
{
	using namespace APSUIStylePrivate;
	const FAPSUILayoutMetrics& Layout = Metrics();
	const FVector2D BarSize(1.0f, Layout.SliderTrackHeight);

	return FSliderStyle()
		.SetNormalBarImage(FillBrush(Palette.BorderQuiet, Layout.SliderTrackHeight * 0.5f, BarSize))
		.SetHoveredBarImage(FillBrush(Palette.FocusCyan, Layout.SliderTrackHeight * 0.5f, BarSize))
		.SetDisabledBarImage(FillBrush(Disabled(Palette.BorderQuiet), Layout.SliderTrackHeight * 0.5f, BarSize))
		.SetNormalThumbImage(FillBrush(Palette.FocusCyan, Layout.SliderHandleSize.X * 0.5f, Layout.SliderHandleSize))
		.SetHoveredThumbImage(FillBrush(Palette.TextPrimary, Layout.SliderHandleHoverSize.X * 0.5f, Layout.SliderHandleHoverSize))
		.SetDisabledThumbImage(FillBrush(Disabled(Palette.FocusCyan), Layout.SliderHandleSize.X * 0.5f, Layout.SliderHandleSize))
		.SetBarThickness(Layout.SliderTrackHeight);
}

FLinearColor FAPSUIStyle::SRGB(const uint8 R, const uint8 G, const uint8 B, const uint8 A)
{
	return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
}


#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

/**
 * Rio 06.10: switchable interface themes (SETTINGS / INTERFACE / THEME). Every screen reads its chrome colours from
 * the active theme's palette and its type from APSUITheme::DisplayFont / BodyFont, so one switch restyles the menu,
 * generation, the colony terminal and the HUD. Content colours (stars, planets, departments, fleet divisions, charts)
 * are not themed.
 */
enum class EAPSUITheme : uint8
{
	/** Black glass, white type, one racing red for what needs action (the default). */
	Obsidian,
	/** Black glass, white and greys only: emphasis by brightness. */
	Carbon,
	/** Cool graphite with a pale ice-blue for data. */
	Titanium,
	/** The cyan and amber look the game had until 06.10. */
	Classic,
	Count
};

/** The chrome palette. Names follow the roles the Classic colours had, so a call site keeps its meaning. */
struct FAPSUIThemePalette
{
	/** Card fill (Classic: deep teal, 242 alpha). */
	FLinearColor Panel;
	/** Secondary card and list fill. */
	FLinearColor PanelSoft;
	/** Raised control on a panel. */
	FLinearColor Raised;
	/** Full-screen dimming behind modal panels. */
	FLinearColor Scrim;
	/** Quiet frames and rules (Classic: CyanDim). */
	FLinearColor Frame;
	/** Focus, hover, glyphs and live values (Classic: Cyan). */
	FLinearColor Highlight;
	/** The one action colour: primary buttons, the selection, warnings to act on (Classic: Amber). */
	FLinearColor Action;
	/** Action on hover. */
	FLinearColor ActionBright;
	/** Action on hover of an already lit control. */
	FLinearColor ActionPeak;
	/** Text and key letters on a solid Action fill (CONTINUE, the NEW WORLD action). */
	FLinearColor OnAction;
	FLinearColor OnActionCode;
	/** Dark fill behind an action-framed card (Classic: the dark amber of AmberPanelBrush). */
	FLinearColor ActionFill;
	/** Dark fill behind a highlight-framed badge (Classic: CyanBadgeBrush). */
	FLinearColor HighlightFill;
	/** Recessed wells: inputs, metric tiles. */
	FLinearColor InsetFill;
	FLinearColor InsetFrame;
	/** Primary text. */
	FLinearColor Text;
	/** Readable secondary text (Classic: Muted 170,194,202). */
	FLinearColor TextSoft;
	/** Footnotes, still legible. */
	FLinearColor TextQuiet;
	/** Captions and disabled labels (Classic: the menu's dim Muted 88,114,122). */
	FLinearColor TextDim;
	/** Good state. */
	FLinearColor Success;
	/** Destructive action or failure. */
	FLinearColor Danger;
	/** Brighter Danger for hover. */
	FLinearColor DangerBright;
};

namespace APSUITheme
{
	APS_ALPHA_API EAPSUITheme Current();
	/** Switches the theme, saves it to GameUserSettings.ini and broadcasts OnChanged (screens rebuild on it). */
	APS_ALPHA_API void SetCurrent(EAPSUITheme Theme);
	APS_ALPHA_API const FAPSUIThemePalette& Palette();
	APS_ALPHA_API const FAPSUIThemePalette& PaletteOf(EAPSUITheme Theme);
	APS_ALPHA_API FText DisplayName(EAPSUITheme Theme);
	APS_ALPHA_API FText Description(EAPSUITheme Theme);
	/** Raised after every switch. Widgets built once (HUD, terminal) compare Revision() and rebuild themselves. */
	APS_ALPHA_API FSimpleMulticastDelegate& OnChanged();
	APS_ALPHA_API uint32 Revision();

	/** Rio 06.10 type: Chakra Petch for titles, labels and numbers; Exo 2 for text. aps.UI.LegacyFonts 1 returns
	 * the old Orbitron and Roboto. Typefaces: Regular, Medium, SemiBold, Bold (Light and Italic map to Regular). */
	APS_ALPHA_API FSlateFontInfo DisplayFont(FName Typeface, int32 Size);
	APS_ALPHA_API FSlateFontInfo BodyFont(FName Typeface, int32 Size);
	APS_ALPHA_API bool UsesLegacyFonts();
	/** SETTINGS / INTERFACE / TYPE: the old Orbitron and Roboto (true) or Chakra Petch and Exo 2; saved like the
	 * theme and broadcast through OnChanged. */
	APS_ALPHA_API void SetLegacyFonts(bool bLegacy);
	APS_ALPHA_API bool IsDisplayFont(const FSlateFontInfo& Font);
	APS_ALPHA_API bool IsBodyFont(const FSlateFontInfo& Font);

	/** sRGB bytes to a linear colour, the conversion every palette in the UI uses. */
	APS_ALPHA_API FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255);
	/** The same colour with its alpha scaled. */
	APS_ALPHA_API FLinearColor Fade(const FLinearColor& Color, float Alpha);

	/** A Classic chrome tone written inline (a dark teal fill, a teal frame, a pale cyan) in the active theme: the
	 * same luminance and alpha in the hue of the theme's panels. Classic returns it unchanged. */
	APS_ALPHA_API FLinearColor Retint(const FLinearColor& ClassicTone);
	/** The same in the hue of the theme's action colour (Classic amber tones: a selected toggle's tint). */
	APS_ALPHA_API FLinearColor RetintAction(const FLinearColor& ClassicTone);
	/** The same in the hue of the theme's highlight (Classic cyan tones: a switched-on toggle). */
	APS_ALPHA_API FLinearColor RetintHighlight(const FLinearColor& ClassicTone);
}

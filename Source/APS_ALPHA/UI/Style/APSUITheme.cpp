#include "APSUITheme.h"

#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "APSUITheme"

namespace APSUIThemePrivate
{
	const TCHAR* ConfigSection = TEXT("/Script/APS_ALPHA.APSUITheme");
	const TCHAR* ConfigKey = TEXT("Theme");

	const TCHAR* ThemeKey(const EAPSUITheme Theme)
	{
		switch (Theme)
		{
		case EAPSUITheme::Carbon: return TEXT("Carbon");
		case EAPSUITheme::Titanium: return TEXT("Titanium");
		case EAPSUITheme::Classic: return TEXT("Classic");
		case EAPSUITheme::Obsidian:
		default: return TEXT("Obsidian");
		}
	}

	bool ParseTheme(const FString& Text, EAPSUITheme& OutTheme)
	{
		for (int32 Index = 0; Index < static_cast<int32>(EAPSUITheme::Count); ++Index)
		{
			const EAPSUITheme Candidate = static_cast<EAPSUITheme>(Index);
			if (Text.Equals(ThemeKey(Candidate), ESearchCase::IgnoreCase) || Text == FString::FromInt(Index))
			{
				OutTheme = Candidate;
				return true;
			}
		}
		return false;
	}

	EAPSUITheme ActiveTheme = EAPSUITheme::Obsidian;
	bool bLoaded = false;
	uint32 ThemeRevision = 1;

	FSimpleMulticastDelegate& ChangedDelegate()
	{
		static FSimpleMulticastDelegate Delegate;
		return Delegate;
	}

	void LoadTypePreference();

	void EnsureLoaded()
	{
		if (bLoaded || !GConfig)
		{
			return;
		}
		bLoaded = true;
		FString Saved;
		EAPSUITheme Parsed;
		if (GConfig->GetString(ConfigSection, ConfigKey, Saved, GGameUserSettingsIni) && ParseTheme(Saved, Parsed))
		{
			ActiveTheme = Parsed;
		}
		LoadTypePreference();
	}

	FAPSUIThemePalette MakeClassic()
	{
		using APSUITheme::SRGB;
		FAPSUIThemePalette P;
		P.Panel = SRGB(8, 32, 42, 242);
		P.PanelSoft = SRGB(6, 19, 26, 232);
		P.Raised = SRGB(12, 41, 52, 251);
		P.Scrim = SRGB(2, 7, 11, 150);
		P.Frame = SRGB(27, 83, 96, 178);
		P.Highlight = SRGB(67, 214, 236);
		P.Action = SRGB(242, 181, 29);
		P.ActionBright = SRGB(255, 208, 82);
		P.ActionPeak = SRGB(255, 226, 140);
		P.OnAction = SRGB(4, 18, 26);
		P.OnActionCode = SRGB(90, 61, 0);
		P.ActionFill = FLinearColor(0.11f, 0.045f, 0.002f, 0.96f);
		P.HighlightFill = FLinearColor(0.01f, 0.07f, 0.10f, 0.98f);
		P.InsetFill = FLinearColor(0.001f, 0.012f, 0.022f, 0.96f);
		P.InsetFrame = FLinearColor(0.04f, 0.22f, 0.31f, 1.0f);
		P.Text = SRGB(234, 246, 248);
		P.TextSoft = SRGB(170, 194, 202);
		P.TextQuiet = SRGB(124, 150, 158);
		P.TextDim = SRGB(88, 114, 122);
		P.Success = SRGB(100, 214, 166);
		P.Danger = SRGB(222, 108, 92);
		P.DangerBright = SRGB(255, 142, 120);
		return P;
	}

	FAPSUIThemePalette MakeObsidian()
	{
		using APSUITheme::SRGB;
		FAPSUIThemePalette P;
		P.Panel = SRGB(10, 10, 11, 242);
		P.PanelSoft = SRGB(7, 7, 8, 232);
		P.Raised = SRGB(22, 22, 24, 251);
		P.Scrim = SRGB(3, 3, 4, 150);
		P.Frame = SRGB(66, 67, 71, 190);
		P.Highlight = SRGB(228, 229, 231);
		// Rio 06.10: racing red, a touch calmer than #E10600 ("too bright, keep the hue").
		P.Action = SRGB(196, 22, 12);
		P.ActionBright = SRGB(220, 34, 20);
		P.ActionPeak = SRGB(236, 64, 48);
		P.OnAction = SRGB(255, 255, 255);
		P.OnActionCode = SRGB(243, 196, 190);
		P.ActionFill = FLinearColor(0.05f, 0.003f, 0.002f, 0.96f);
		P.HighlightFill = FLinearColor(0.012f, 0.012f, 0.013f, 0.98f);
		P.InsetFill = FLinearColor(0.002f, 0.002f, 0.0025f, 0.96f);
		P.InsetFrame = FLinearColor(0.05f, 0.05f, 0.055f, 1.0f);
		P.Text = SRGB(242, 242, 243);
		P.TextSoft = SRGB(181, 183, 186);
		P.TextQuiet = SRGB(141, 144, 148);
		P.TextDim = SRGB(104, 107, 111);
		P.Success = SRGB(140, 200, 160);
		P.Danger = SRGB(232, 104, 88);
		P.DangerBright = SRGB(255, 140, 120);
		return P;
	}

	FAPSUIThemePalette MakeCarbon()
	{
		using APSUITheme::SRGB;
		FAPSUIThemePalette P;
		P.Panel = SRGB(11, 12, 14, 242);
		P.PanelSoft = SRGB(8, 9, 10, 232);
		P.Raised = SRGB(24, 25, 28, 251);
		P.Scrim = SRGB(3, 3, 4, 150);
		P.Frame = SRGB(74, 76, 80, 190);
		P.Highlight = SRGB(232, 233, 235);
		P.Action = SRGB(244, 245, 246);
		P.ActionBright = SRGB(255, 255, 255);
		P.ActionPeak = SRGB(255, 255, 255);
		P.OnAction = SRGB(11, 12, 14);
		P.OnActionCode = SRGB(92, 95, 100);
		P.ActionFill = FLinearColor(0.03f, 0.031f, 0.034f, 0.96f);
		P.HighlightFill = FLinearColor(0.012f, 0.013f, 0.015f, 0.98f);
		P.InsetFill = FLinearColor(0.002f, 0.0022f, 0.0026f, 0.96f);
		P.InsetFrame = FLinearColor(0.05f, 0.052f, 0.057f, 1.0f);
		P.Text = SRGB(244, 245, 246);
		P.TextSoft = SRGB(184, 188, 193);
		P.TextQuiet = SRGB(142, 147, 153);
		P.TextDim = SRGB(106, 110, 116);
		P.Success = SRGB(160, 206, 176);
		P.Danger = SRGB(226, 112, 98);
		P.DangerBright = SRGB(255, 146, 128);
		return P;
	}

	FAPSUIThemePalette MakeTitanium()
	{
		using APSUITheme::SRGB;
		FAPSUIThemePalette P;
		P.Panel = SRGB(18, 21, 25, 242);
		P.PanelSoft = SRGB(14, 16, 19, 232);
		P.Raised = SRGB(28, 33, 39, 251);
		P.Scrim = SRGB(4, 5, 7, 150);
		P.Frame = SRGB(64, 76, 90, 190);
		P.Highlight = SRGB(169, 214, 229);
		P.Action = SRGB(221, 239, 246);
		P.ActionBright = SRGB(240, 250, 255);
		P.ActionPeak = SRGB(250, 253, 255);
		P.OnAction = SRGB(17, 22, 27);
		P.OnActionCode = SRGB(76, 94, 108);
		P.ActionFill = FLinearColor(0.02f, 0.03f, 0.04f, 0.96f);
		P.HighlightFill = FLinearColor(0.008f, 0.014f, 0.02f, 0.98f);
		P.InsetFill = FLinearColor(0.003f, 0.004f, 0.006f, 0.96f);
		P.InsetFrame = FLinearColor(0.04f, 0.055f, 0.07f, 1.0f);
		P.Text = SRGB(238, 242, 245);
		P.TextSoft = SRGB(182, 192, 201);
		P.TextQuiet = SRGB(141, 152, 163);
		P.TextDim = SRGB(104, 114, 125);
		P.Success = SRGB(140, 210, 180);
		P.Danger = SRGB(226, 112, 98);
		P.DangerBright = SRGB(255, 146, 128);
		return P;
	}

	// ---- Type ----

	TAutoConsoleVariable<int32> CVarLegacyFonts(TEXT("aps.UI.LegacyFonts"), 0,
		TEXT("1 = the old Orbitron and Roboto instead of Chakra Petch and Exo 2 (A/B; screens rebuild when reopened)."));
	const TCHAR* LegacyFontsKey = TEXT("LegacyFonts");

	void LoadTypePreference()
	{
		bool bLegacy = false;
		if (GConfig && GConfig->GetBool(ConfigSection, LegacyFontsKey, bLegacy, GGameUserSettingsIni) && bLegacy)
		{
			CVarLegacyFonts->Set(1, ECVF_SetByCode);
		}
	}

	FString FontFile(const TCHAR* Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Slate/Fonts") / Name);
	}

	/** Every typeface name the UI asks for, mapped onto the weights shipped in Content/Slate/Fonts. */
	void AppendWeights(FTypeface& Typeface, const TCHAR* Regular, const TCHAR* Medium, const TCHAR* SemiBold,
		const TCHAR* Bold)
	{
		const auto Add = [&Typeface](const TCHAR* Name, const TCHAR* File)
		{
			Typeface.AppendFont(Name, FontFile(File), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		};
		Add(TEXT("Regular"), Regular);
		Add(TEXT("Medium"), Medium);
		Add(TEXT("SemiBold"), SemiBold);
		Add(TEXT("Bold"), Bold);
		Add(TEXT("Light"), Regular);
		Add(TEXT("Italic"), Regular);
		Add(TEXT("BoldItalic"), Bold);
		Add(TEXT("Black"), Bold);
	}

	bool FontFilesPresent()
	{
		static const bool bPresent = []()
		{
			IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
			const TCHAR* Names[] = {TEXT("ChakraPetch-Medium.ttf"), TEXT("ChakraPetch-SemiBold.ttf"),
				TEXT("ChakraPetch-Bold.ttf"), TEXT("Exo2-Regular.ttf"), TEXT("Exo2-Medium.ttf"),
				TEXT("Exo2-SemiBold.ttf"), TEXT("Exo2-Bold.ttf")};
			for (const TCHAR* Name : Names)
			{
				if (!Files.FileExists(*FontFile(Name)))
				{
					UE_LOG(LogTemp, Warning, TEXT("[APS.UI] Font file missing: %s; the UI keeps Orbitron and Roboto"),
						*FontFile(Name));
					return false;
				}
			}
			return true;
		}();
		return bPresent;
	}

	// Held for the whole session (never destroyed): text widgets and Slate's font cache keep the pointer.
	const TSharedPtr<const FCompositeFont>& BodyComposite()
	{
		static TSharedPtr<const FCompositeFont>* Font = []()
		{
			TSharedRef<FStandaloneCompositeFont> Made = MakeShared<FStandaloneCompositeFont>();
			AppendWeights(Made->DefaultTypeface, TEXT("Exo2-Regular.ttf"), TEXT("Exo2-Medium.ttf"),
				TEXT("Exo2-SemiBold.ttf"), TEXT("Exo2-Bold.ttf"));
			return new TSharedPtr<const FCompositeFont>(Made);
		}();
		return *Font;
	}

	const TSharedPtr<const FCompositeFont>& DisplayComposite()
	{
		static TSharedPtr<const FCompositeFont>* Font = []()
		{
			TSharedRef<FStandaloneCompositeFont> Made = MakeShared<FStandaloneCompositeFont>();
			// Chakra Petch has no Regular in the set (Medium reads as the regular display weight) and no Cyrillic:
			// those letters come from Exo 2 at the same weight.
			AppendWeights(Made->DefaultTypeface, TEXT("ChakraPetch-Medium.ttf"), TEXT("ChakraPetch-Medium.ttf"),
				TEXT("ChakraPetch-SemiBold.ttf"), TEXT("ChakraPetch-Bold.ttf"));
			AppendWeights(Made->FallbackTypeface.Typeface, TEXT("Exo2-Medium.ttf"), TEXT("Exo2-Medium.ttf"),
				TEXT("Exo2-SemiBold.ttf"), TEXT("Exo2-Bold.ttf"));
			return new TSharedPtr<const FCompositeFont>(Made);
		}();
		return *Font;
	}

	TWeakObjectPtr<UFont> LegacyBold;
	TWeakObjectPtr<UFont> LegacyMedium;

	UFont* LegacyDisplayFont(const FName Typeface)
	{
		TWeakObjectPtr<UFont>& Slot = Typeface == TEXT("Bold") ? LegacyBold : LegacyMedium;
		if (!Slot.IsValid())
		{
			const TCHAR* Path = Typeface == TEXT("Bold")
				? TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font")
				: TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font");
			// Rooted for the session: an STextBlock keeps FSlateFontInfo::FontObject as a plain pointer that GC never
			// sees, so a font held only by the menu controller was collected after the level change and the next
			// re-shape of the objective overlay's text read freed memory (a4-trace-2, 02.10).
			UFont* Font = LoadObject<UFont>(nullptr, Path);
			if (Font)
			{
				Font->AddToRoot();
			}
			Slot = Font;
		}
		return Slot.Get();
	}

	FName BodyTypeface(const FName Typeface)
	{
		return Typeface.IsNone() ? FName(TEXT("Regular")) : Typeface;
	}
}

FLinearColor APSUITheme::SRGB(const uint8 R, const uint8 G, const uint8 B, const uint8 A)
{
	return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
}

FLinearColor APSUITheme::Fade(const FLinearColor& Color, const float Alpha)
{
	return FLinearColor(Color.R, Color.G, Color.B, Color.A * Alpha);
}

namespace APSUIThemePrivate
{
	FLinearColor InHue(const FLinearColor& Tone, const FLinearColor& Hue)
	{
		const float HueLuminance = FMath::Max(Hue.GetLuminance(), 1.0e-5f);
		const float Scale = Tone.GetLuminance() / HueLuminance;
		return FLinearColor(Hue.R * Scale, Hue.G * Scale, Hue.B * Scale, Tone.A);
	}
}

FLinearColor APSUITheme::Retint(const FLinearColor& ClassicTone)
{
	return Current() == EAPSUITheme::Classic ? ClassicTone : APSUIThemePrivate::InHue(ClassicTone, Palette().Panel);
}

FLinearColor APSUITheme::RetintAction(const FLinearColor& ClassicTone)
{
	return Current() == EAPSUITheme::Classic ? ClassicTone : APSUIThemePrivate::InHue(ClassicTone, Palette().Action);
}

FLinearColor APSUITheme::RetintHighlight(const FLinearColor& ClassicTone)
{
	return Current() == EAPSUITheme::Classic ? ClassicTone : APSUIThemePrivate::InHue(ClassicTone, Palette().Highlight);
}

EAPSUITheme APSUITheme::Current()
{
	APSUIThemePrivate::EnsureLoaded();
	return APSUIThemePrivate::ActiveTheme;
}

void APSUITheme::SetCurrent(const EAPSUITheme Theme)
{
	using namespace APSUIThemePrivate;
	if (Theme >= EAPSUITheme::Count)
	{
		return;
	}
	EnsureLoaded();
	if (GConfig)
	{
		GConfig->SetString(ConfigSection, ConfigKey, ThemeKey(Theme), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	if (Theme == ActiveTheme)
	{
		return;
	}
	ActiveTheme = Theme;
	++ThemeRevision;
	UE_LOG(LogTemp, Log, TEXT("[APS.UI] Theme %s (revision %u)"), ThemeKey(Theme), ThemeRevision);
	ChangedDelegate().Broadcast();
}

const FAPSUIThemePalette& APSUITheme::PaletteOf(const EAPSUITheme Theme)
{
	using namespace APSUIThemePrivate;
	static const FAPSUIThemePalette Palettes[] = {MakeObsidian(), MakeCarbon(), MakeTitanium(), MakeClassic()};
	static_assert(UE_ARRAY_COUNT(Palettes) == static_cast<int32>(EAPSUITheme::Count), "one palette per theme");
	const int32 Index = FMath::Clamp(static_cast<int32>(Theme), 0, static_cast<int32>(EAPSUITheme::Count) - 1);
	return Palettes[Index];
}

const FAPSUIThemePalette& APSUITheme::Palette()
{
	return PaletteOf(Current());
}

FText APSUITheme::DisplayName(const EAPSUITheme Theme)
{
	switch (Theme)
	{
	case EAPSUITheme::Carbon: return LOCTEXT("Carbon", "CARBON");
	case EAPSUITheme::Titanium: return LOCTEXT("Titanium", "TITANIUM");
	case EAPSUITheme::Classic: return LOCTEXT("Classic", "CLASSIC");
	case EAPSUITheme::Obsidian:
	default: return LOCTEXT("Obsidian", "OBSIDIAN");
	}
}

FText APSUITheme::Description(const EAPSUITheme Theme)
{
	switch (Theme)
	{
	case EAPSUITheme::Carbon: return LOCTEXT("CarbonNote", "Black glass, white and greys only.");
	case EAPSUITheme::Titanium: return LOCTEXT("TitaniumNote", "Cool graphite with pale ice-blue data.");
	case EAPSUITheme::Classic: return LOCTEXT("ClassicNote", "The original cyan and amber.");
	case EAPSUITheme::Obsidian:
	default: return LOCTEXT("ObsidianNote", "Black glass, white type, one racing red.");
	}
}

FSimpleMulticastDelegate& APSUITheme::OnChanged()
{
	return APSUIThemePrivate::ChangedDelegate();
}

uint32 APSUITheme::Revision()
{
	return APSUIThemePrivate::ThemeRevision;
}

bool APSUITheme::UsesLegacyFonts()
{
	APSUIThemePrivate::EnsureLoaded();
	return APSUIThemePrivate::CVarLegacyFonts.GetValueOnGameThread() != 0 || !APSUIThemePrivate::FontFilesPresent();
}

void APSUITheme::SetLegacyFonts(const bool bLegacy)
{
	using namespace APSUIThemePrivate;
	EnsureLoaded();
	if (GConfig)
	{
		GConfig->SetBool(ConfigSection, LegacyFontsKey, bLegacy, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	if ((CVarLegacyFonts.GetValueOnGameThread() != 0) == bLegacy)
	{
		return;
	}
	CVarLegacyFonts->Set(bLegacy ? 1 : 0, ECVF_SetByCode);
	++ThemeRevision;
	UE_LOG(LogTemp, Log, TEXT("[APS.UI] Type %s (revision %u)"), bLegacy ? TEXT("Orbitron+Roboto") : TEXT("ChakraPetch+Exo2"),
		ThemeRevision);
	ChangedDelegate().Broadcast();
}

FSlateFontInfo APSUITheme::DisplayFont(const FName Typeface, const int32 Size)
{
	using namespace APSUIThemePrivate;
	if (UsesLegacyFonts())
	{
		if (UFont* FontObject = LegacyDisplayFont(Typeface))
		{
			return FSlateFontInfo(FontObject, Size, Typeface);
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}
	return FSlateFontInfo(DisplayComposite(), Size, Typeface.IsNone() ? FName(TEXT("Bold")) : Typeface);
}

FSlateFontInfo APSUITheme::BodyFont(const FName Typeface, const int32 Size)
{
	using namespace APSUIThemePrivate;
	if (UsesLegacyFonts())
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}
	return FSlateFontInfo(BodyComposite(), Size, BodyTypeface(Typeface));
}

bool APSUITheme::IsDisplayFont(const FSlateFontInfo& Font)
{
	if (Font.CompositeFont.IsValid())
	{
		return Font.CompositeFont == APSUIThemePrivate::DisplayComposite();
	}
	const UObject* FontObject = Font.FontObject;
	return FontObject && FontObject->GetName().Contains(TEXT("Orbitron"));
}

bool APSUITheme::IsBodyFont(const FSlateFontInfo& Font)
{
	return Font.CompositeFont.IsValid() && Font.CompositeFont == APSUIThemePrivate::BodyComposite();
}

namespace APSUIThemePrivate
{
	FAutoConsoleCommand ThemeCommand(TEXT("aps.UI.Theme"),
		TEXT("aps.UI.Theme <Obsidian|Carbon|Titanium|Classic|0-3>: switches the interface theme (saved like the setting)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			EAPSUITheme Theme;
			if (Args.Num() > 0 && ParseTheme(Args[0], Theme))
			{
				APSUITheme::SetCurrent(Theme);
			}
			UE_LOG(LogTemp, Display, TEXT("[APS.UI] Theme is %s"), ThemeKey(APSUITheme::Current()));
		}));
}

#undef LOCTEXT_NAMESPACE

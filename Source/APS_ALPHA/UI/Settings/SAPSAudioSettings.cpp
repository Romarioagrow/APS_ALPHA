#include "SAPSAudioSettings.h"

#include "APS_ALPHA/Core/Audio/APSAudioPreferences.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/UI/Style/APSUIStyle.h"
#include "Engine/World.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSAudioSettings"

void SAPSAudioSettings::Construct(const FArguments& Args)
{
	World = Args._World;
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette();
	SliderStyle = FAPSUIStyle::MakeSliderStyle(Palette);
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	if (!Args._CreditsOnly)
	{
		if (Args._ShowTitle)
		{
			Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
			[SNew(STextBlock).Text(LOCTEXT("Audio", "AUDIO"))
				.Font(FAPSUIStyle::DisplayFont("Bold", 20)).ColorAndOpacity(Palette.TextPrimary)];
		}
		Rows->AddSlot().AutoHeight()[MakeRow(EAPSAudioChannel::Master, LOCTEXT("Master", "MASTER"))];
		Rows->AddSlot().AutoHeight()[MakeRow(EAPSAudioChannel::Music, LOCTEXT("Music", "MUSIC"))];
		Rows->AddSlot().AutoHeight()[MakeRow(EAPSAudioChannel::Ambience, LOCTEXT("Ambience", "AMBIENCE"))];
		Rows->AddSlot().AutoHeight()[MakeRow(EAPSAudioChannel::Effects, LOCTEXT("Effects", "SOUND EFFECTS"))];
		Rows->AddSlot().AutoHeight()[MakeRow(EAPSAudioChannel::Interface, LOCTEXT("Interface", "INTERFACE"))];
	}
	Rows->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
		[SNew(STextBlock).Text(LOCTEXT("Credits", "Footsteps Mini Sound Pack — Mechanics Mechanics · CC BY 4.0"))
			.ToolTipText(LOCTEXT("CreditDetails", "Source: fab.com/listings/baf07baa-d485-4d37-bf73-dab6a50ed4bb\nLicense: creativecommons.org/licenses/by/4.0/\nVolume and playback pitch adjusted."))
			.Font(FAPSUIStyle::BodyFont("Regular", 11)).ColorAndOpacity(Palette.TextSecondary)];
	Rows->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
		[SNew(STextBlock).Text(LOCTEXT("MenuMusicCredits", "Menu music — Scott Buckley · CC BY 4.0"))
			.ToolTipText(LOCTEXT("MenuMusicCreditDetails", "A Kind Of Hope; Tears in Rain; Celestial\nMusic by Scott Buckley — released under CC-BY 4.0.\nSource: www.scottbuckley.com.au/library/\nLicense: creativecommons.org/licenses/by/4.0/\nPlayback level and transitions adjusted."))
			.Font(FAPSUIStyle::BodyFont("Regular", 11)).ColorAndOpacity(Palette.TextSecondary)];
	Rows->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
		[SNew(STextBlock).Text(LOCTEXT("GameplayMusicCredits", "Gameplay music — Stellardrone · CC BY"))
			.ToolTipText(LOCTEXT("GameplayMusicCreditDetails", "Light Years (2013) — Stellardrone / Energostatic Records\nAirglow; Comet Halley; Ultra Deep Field; Light Years; In Time; Cepheid; Red Giant; Messier 45\nSource: stellardrone.bandcamp.com/album/light-years\nCC BY 4.0: creativecommons.org/licenses/by/4.0/\nOriginal CC BY 3.0 notice: creativecommons.org/licenses/by/3.0/\nLevel, EQ, padding and transitions adjusted."))
			.Font(FAPSUIStyle::BodyFont("Regular", 11)).ColorAndOpacity(Palette.TextSecondary)];
	ChildSlot[Rows];
}

TSharedRef<SWidget> SAPSAudioSettings::MakeRow(EAPSAudioChannel Channel, const FText& Label)
{
	const FAPSUIColorPalette Palette = FAPSUIStyle::GetPalette();
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 4.f, 20.f, 4.f)
		[SNew(SBox).WidthOverride(200.f)
			[SNew(STextBlock).Text(Label).Font(FAPSUIStyle::DisplayFont("Bold", 14)).ColorAndOpacity(Palette.TextPrimary)]]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[SNew(SBox).HeightOverride(32.f)
			[SNew(SSlider).Style(&SliderStyle).StepSize(0.01f)
				.Value_Lambda([Channel]() { return GetDefault<UAPSAudioPreferences>()->Get(Channel); })
				.OnValueChanged_Lambda([WeakWorld = World, Channel](float Value)
				{
					if (UWorld* CurrentWorld = WeakWorld.Get())
					{
						if (UAPSAudioSubsystem* Audio = CurrentWorld->GetSubsystem<UAPSAudioSubsystem>())
						{
							Audio->SetVolume(Channel, Value);
							return;
						}
					}
					GetMutableDefault<UAPSAudioPreferences>()->Set(Channel, Value);
				})
				.OnMouseCaptureEnd_Lambda([this]() { Save(); })
				.OnControllerCaptureEnd_Lambda([this]() { Save(); })]]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)
		[SNew(SBox).WidthOverride(45.f)
			[SNew(STextBlock).Justification(ETextJustify::Right)
				.Font(FAPSUIStyle::DisplayFont("Bold", 14)).ColorAndOpacity(Palette.TextSecondary)
				.Text_Lambda([Channel]()
				{
					return FText::FromString(FString::Printf(TEXT("%d%%"),
						FMath::RoundToInt(GetDefault<UAPSAudioPreferences>()->Get(Channel) * 100.f)));
				})]];
}

void SAPSAudioSettings::Save() const
{
	GetMutableDefault<UAPSAudioPreferences>()->SaveConfig();
}

SAPSAudioSettings::~SAPSAudioSettings()
{
	Save();
}

#undef LOCTEXT_NAMESPACE

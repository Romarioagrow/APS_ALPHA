#include "SAPSMainMenuRoot.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Saves/GameSave.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.h"
#include "APS_ALPHA/UI/MainMenu/SpawnClassPicker.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSMainMenuRoot"

namespace APSMenu
{
	const FLinearColor Background(0.002f, 0.009f, 0.017f, 0.22f);
	const FLinearColor Panel(0.004f, 0.025f, 0.041f, 0.94f);
	const FLinearColor PanelSoft(0.012f, 0.055f, 0.083f, 0.88f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.05f, 0.30f, 0.42f, 1.0f);
	const FLinearColor Amber(1.0f, 0.55f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.48f, 0.62f, 0.70f, 1.0f);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	const FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush PanelSoftBrush(PanelSoft, 9.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush AmberPanelBrush(FLinearColor(0.11f, 0.045f, 0.002f, 0.96f), 9.0f, Amber, 1.4f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		if (UFont* FontObject = (Typeface == TEXT("Bold") ? DisplayFont.Get() : BodyFont.Get()))
		{
			return FSlateFontInfo(FontObject, Size, Typeface);
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FText SaveSizeText(int64 Bytes)
	{
		const double MB = static_cast<double>(Bytes) / (1024.0 * 1024.0);
		return FText::FromString(MB >= 1.0
			? FString::Printf(TEXT("%.1f MB"), MB)
			: FString::Printf(TEXT("%.0f KB"), static_cast<double>(Bytes) / 1024.0));
	}

	template <typename T>
	FString EnumLabel(T Value)
	{
		const UEnum* Enum = StaticEnum<T>();
		return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Value)).ToString() : TEXT("UNKNOWN");
	}
}

SAPSMainMenuRoot::SAPSMainMenuRoot()
{
	PrimaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.18f, 0.08f, 0.005f, 0.94f), 8.0f, APSMenu::Amber, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.42f, 0.18f, 0.01f, 0.98f), 8.0f, FLinearColor(1.0f, 0.76f, 0.18f), 2.0f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.62f, 0.26f, 0.01f, 1.0f), 8.0f, APSMenu::Amber, 2.0f))
		.SetNormalPadding(FMargin(2.0f)).SetPressedPadding(FMargin(2.0f, 3.0f, 2.0f, 1.0f));
	SecondaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.01f, 0.04f, 0.065f, 0.92f), 7.0f, APSMenu::CyanDim, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.10f, 0.15f, 0.96f), 7.0f, APSMenu::Cyan, 1.5f))
		.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.16f, 0.22f, 1.0f), 7.0f, APSMenu::Cyan, 1.5f));
	CardButtonStyle = SecondaryButtonStyle;
	DisabledCardButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.025f, 0.035f, 0.82f), 8.0f, FLinearColor(0.20f, 0.25f, 0.28f), 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.025f, 0.035f, 0.82f), 8.0f, FLinearColor(0.20f, 0.25f, 0.28f), 1.0f));
	ScrollBarStyle = FAppStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar");
}

void SAPSMainMenuRoot::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	ViewModel = InArgs._ViewModel;
	LoadVisualResources();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(APSMenu::Background)
		.Padding(0.0f)
		[
			SAssignNew(ContentHost, SBox)
		]
	];
	Navigate(EAPSMenuPage::Landing);
}

void SAPSMainMenuRoot::Navigate(EAPSMenuPage NewPage)
{
	if (CurrentPage == EAPSMenuPage::ExistingWorlds && NewPage != EAPSMenuPage::ExistingWorlds)
	{
		if (AMainMenuController* PC = Controller.Get())
		{
			PC->CancelWorldMetadataLoad();
		}
	}
	PreviousPage = CurrentPage;
	CurrentPage = NewPage;
	if (!ContentHost.IsValid()) return;

	switch (CurrentPage)
	{
	case EAPSMenuPage::Landing: ContentHost->SetContent(BuildLandingPage()); break;
	case EAPSMenuPage::ChoosePath: ContentHost->SetContent(BuildChoosePathPage()); break;
	case EAPSMenuPage::ExistingWorlds: ContentHost->SetContent(BuildExistingWorldsPage()); break;
	case EAPSMenuPage::AstronomicalGeneration:
		ContentHost->SetContent(
			SNew(SWorldGenerationPanel)
			.ViewModel(ViewModel)
			.OnBack(FSimpleDelegate::CreateSP(this, &SAPSMainMenuRoot::Navigate, EAPSMenuPage::ChoosePath))
			.OnContinue(FSimpleDelegate::CreateSP(this, &SAPSMainMenuRoot::Navigate, EAPSMenuPage::Civilization)));
		break;
	case EAPSMenuPage::Civilization: ContentHost->SetContent(BuildCivilizationPage()); break;
	}
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildHeader(const FText& SectionTitle, bool bShowBack)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(180.0f).HeightOverride(54.0f)
			[
				SNew(SButton)
				.Visibility(bShowBack ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(&SecondaryButtonStyle)
				.OnClicked(this, &SAPSMainMenuRoot::Back)
				.ContentPadding(FMargin(22.0f, 12.0f))
				[
					SNew(STextBlock).Text(LOCTEXT("Back", "<  BACK")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("Brand", "A P O S F E R A")).Font(APSMenu::Font("Bold", 54)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("SubBrand", "S P A C E T R I P S   G E N E R A T I O N")).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f)
				[SNew(STextBlock).Text(SectionTitle).Font(APSMenu::Font("Bold", 17)).ColorAndOpacity(APSMenu::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(92.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Cyan)]]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(180.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
				[SNew(SButton).ButtonStyle(&SecondaryButtonStyle)[SNew(STextBlock).Text(LOCTEXT("Profile", "PROFILE")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
				[SNew(SButton).ButtonStyle(&SecondaryButtonStyle)[SNew(STextBlock).Text(LOCTEXT("SettingsShort", "SETTINGS")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 9)).ColorAndOpacity(APSMenu::White)]]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildLandingPage()
{
	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(150.0f, 40.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 80.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("LandingTitle", "APOSFERA SPACETRIPS")).Font(APSMenu::Font("Bold", 54)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[
				SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::OpenChoosePath)
				.ContentPadding(FMargin(26.0f, 14.0f))
				[SNew(STextBlock).Text(LOCTEXT("SingleGame", "SINGLE GAME")).Font(APSMenu::Font("Bold", 20)).ColorAndOpacity(APSMenu::White)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[SNew(STextBlock).Text(LOCTEXT("Multiplayer", "MULTIPLAYER GAME")).Font(APSMenu::Font("Regular", 18)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[SNew(STextBlock).Text(LOCTEXT("ProfileDisabled", "PROFILE")).Font(APSMenu::Font("Regular", 18)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 28.0f, 0.0f, 6.0f)
			[SNew(STextBlock).Text(LOCTEXT("Settings", "SETTINGS")).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::White)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[
				SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
				.OnClicked(this, &SAPSMainMenuRoot::QuitGame)
				[SNew(STextBlock).Text(LOCTEXT("Quit", "QUIT")).Font(APSMenu::Font("Regular", 14)).ColorAndOpacity(APSMenu::White)]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildPathCard(const FText& Title, const FText& Description,
	const FSlateBrush* Image, const FLinearColor& Accent, FSimpleDelegate Action, bool bLarge, bool bEnabled)
{
	return SNew(SButton)
		.IsEnabled(bEnabled)
		.ButtonStyle(!bEnabled ? &DisabledCardButtonStyle : (bLarge ? &PrimaryButtonStyle : &CardButtonStyle))
		.OnClicked_Lambda([Action]() mutable { Action.ExecuteIfBound(); return FReply::Handled(); })
		.ContentPadding(0.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SScaleBox).Stretch(bLarge ? EStretch::ScaleToFitY : EStretch::ScaleToFitX).StretchDirection(EStretchDirection::Both)
					[
						SNew(SImage).Image(Image).ColorAndOpacity(bEnabled ? FLinearColor::White : FLinearColor(0.18f, 0.22f, 0.25f, 0.42f))
					]
				]
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.001f, 0.010f, 0.018f, 0.94f))
				.Padding(FMargin(bLarge ? 30.0f : 22.0f, bLarge ? 24.0f : 16.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", bLarge ? 28 : 17)).ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(Description).AutoWrapText(true).Font(APSMenu::Font("Regular", bLarge ? 15 : 12)).ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bLarge ? 18.0f : 0.0f, 0.0f, 0.0f)
					[
						SNew(SBorder).Visibility(bLarge ? EVisibility::Visible : EVisibility::Collapsed)
						.BorderImage(&APSMenu::AmberPanelBrush).Padding(FMargin(18.0f, 11.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[SNew(STextBlock).Text(LOCTEXT("LaunchGame", "LAUNCH GAME")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::White)]
							+ SHorizontalBox::Slot().AutoWidth()
							[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(APSMenu::Font("Bold", 18)).ColorAndOpacity(APSMenu::Amber)]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildChoosePathPage()
{
	TSharedRef<SWidget> Foreground = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 20.0f, 30.0f, 10.0f)[BuildHeader(LOCTEXT("ChoosePath", "CHOOSE YOUR PATH"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(58.0f, 18.0f, 58.0f, 46.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.31f).Padding(7.0f)
			[
				BuildPathCard(LOCTEXT("StartGame", "START SINGLE GAME"),
					LOCTEXT("StartGameDesc", "Begin a new journey through the live full-scale world."), &SystemImage,
					APSMenu::Amber, FSimpleDelegate::CreateLambda([this]() { StartSingleGame(); }), true)
			]
			+ SHorizontalBox::Slot().FillWidth(0.69f).Padding(7.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.5f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f)
					[BuildPathCard(LOCTEXT("VisitWorld", "VISIT EXISTING WORLD"), LOCTEXT("VisitDesc", "Explore worlds you have already created."), &PlanetImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenExistingWorlds(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f)
					[BuildPathCard(LOCTEXT("GenCiv", "GENERATE CIVILIZATION"), LOCTEXT("GenCivDesc", "Create a civilization and shape its astronomical home."), &CivilizationImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomeSystem); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f)
					[BuildPathCard(LOCTEXT("GenSpace", "GENERATE SPACE"), LOCTEXT("GenSpaceDesc", "Procedurally generate stellar systems and clusters."), &GalaxyImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::StarCluster); }))]
				]
				+ SVerticalBox::Slot().FillHeight(0.5f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f)
					[BuildPathCard(LOCTEXT("CreatePlanet", "CREATE PLANET"), LOCTEXT("PlanetDesc", "Design a planet with atmosphere, terrain and moons."), &PlanetImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::HomePlanet); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f)
					[BuildPathCard(LOCTEXT("Story", "STORY MODE"), LOCTEXT("StoryDesc", "Unravel the deeper story of Aposfera.  COMING SOON"), &GalaxyImage, APSMenu::Muted, FSimpleDelegate(), false, false)]
				]
			]
		];

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.22f, 0.34f, 0.44f, 0.32f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.012f, 0.024f, 0.44f))]
		+ SOverlay::Slot()[Foreground];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildExistingWorldsPage()
{
	LoadExistingWorlds();
	ExistingWorldPage = 0;

	const auto NavRow = [this](const FText& Label, EAPSWorldCollection Collection, TFunction<int32()> CountGetter)
	{
		return SNew(SButton)
			.ButtonStyle(&SecondaryButtonStyle)
			.ButtonColorAndOpacity_Lambda([this, Collection]()
			{
				return WorldCollection == Collection
					? FLinearColor(0.72f, 0.32f, 0.03f, 1.0f) : FLinearColor::White;
			})
			.ContentPadding(FMargin(14.0f, 11.0f))
			.OnClicked(this, &SAPSMainMenuRoot::SetWorldCollection, Collection)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity_Lambda([this, Collection](){ return WorldCollection == Collection ? APSMenu::Amber : APSMenu::White; })]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(STextBlock).Text_Lambda([CountGetter](){ return FText::AsNumber(CountGetter()); }).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Muted)]
			];
	};

	const auto FilterRow = [this](const FText& Label, EAPSWorldFilterKind Kind)
	{
		return SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(2.0f, 5.0f)).OnClicked(this, &SAPSMainMenuRoot::CycleWorldFilter, Kind)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([this, Kind](){ return GetWorldFilterLabel(Kind); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]
			];
	};

	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(30.0f, 18.0f, 30.0f, 8.0f)[BuildHeader(LOCTEXT("VisitExisting", "VISIT EXISTING WORLD"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(24.0f, 8.0f, 24.0f, 24.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.17f).Padding(5.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(14.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[NavRow(LOCTEXT("AllWorlds", "ALL WORLDS"), EAPSWorldCollection::All, [this](){ return ExistingWorlds.Num(); })]
					+ SVerticalBox::Slot().AutoHeight()[NavRow(LOCTEXT("MyWorlds", "MY WORLDS"), EAPSWorldCollection::MyWorlds, [this](){ return ExistingWorlds.Num(); })]
					+ SVerticalBox::Slot().AutoHeight()[NavRow(LOCTEXT("Favorites", "FAVORITES"), EAPSWorldCollection::Favorites, [this](){ int32 Count=0; for(const TSharedPtr<FAPSExistingWorldEntry>& E:ExistingWorlds){ if(E.IsValid()&&E->bFavorite){++Count;} } return Count; })]
					+ SVerticalBox::Slot().AutoHeight()[NavRow(LOCTEXT("Recent", "RECENT"), EAPSWorldCollection::Recent, [this](){ return FMath::Min(6, ExistingWorlds.Num()); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 20.0f, 4.0f, 10.0f)[SNew(STextBlock).Text(LOCTEXT("Filters", "FILTERS")).Font(APSMenu::Font("Bold", 11)).ColorAndOpacity(APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("StarTypeFilter", "STAR TYPE"), EAPSWorldFilterKind::StarType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("WorldTypeFilter", "WORLD TYPE"), EAPSWorldFilterKind::WorldType)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("InhabitedFilter", "INHABITED"), EAPSWorldFilterKind::Inhabited)]
					+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 5.0f)[FilterRow(LOCTEXT("EnvironmentFilter", "ENVIRONMENT"), EAPSWorldFilterKind::Environment)]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					+ SVerticalBox::Slot().AutoHeight()[SNew(SButton).IsEnabled(false).ToolTipText(LOCTEXT("ImportSaveHint", "Place .sav files in Saved/SaveGames; the browser discovers them without blocking.")).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(15.0f, 12.0f))[SNew(STextBlock).Text(LOCTEXT("ImportSave", "IMPORT SAVE")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Muted)]]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSearchBox).HintText(LOCTEXT("SearchWorlds", "Search worlds...")).OnTextChanged(this, &SAPSMainMenuRoot::OnWorldSearchChanged)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleWorldSort)[SNew(STextBlock).Text_Lambda([this](){ return GetWorldSortLabel(); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ToggleWorldView)[SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bCompactWorldList ? TEXT("LIST") : TEXT("GRID")); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(10.0f)
					[SAssignNew(ExistingWorldGridHost, SBox)]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ChangeExistingWorldPage, -1)[SNew(STextBlock).Text(LOCTEXT("PreviousPage", "<<  PREVIOUS")).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 7.0f)[SNew(STextBlock).Text_Lambda([this](){ return FText::AsNumber(ExistingWorldPage + 1); }).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::White)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ChangeExistingWorldPage, 1)[SNew(STextBlock).Text(LOCTEXT("NextPage", "NEXT  >>")).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.32f).Padding(5.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(16.0f)
				[SAssignNew(ExistingWorldDetailsHost, SBox)]
			]
		];

	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	BeginExistingWorldMetadataLoad();
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFill)[SNew(SImage).Image(&BackgroundImage).ColorAndOpacity(FLinearColor(0.16f, 0.26f, 0.35f, 0.24f))]]
		+ SOverlay::Slot()
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.0f, 0.010f, 0.020f, 0.52f))]
		+ SOverlay::Slot()[Page];
}

void SAPSMainMenuRoot::LoadExistingWorlds()
{
	TMap<FString, TSharedPtr<FAPSExistingWorldEntry>> CachedEntries;
	const FString PreviouslySelectedSlot = SelectedWorld.IsValid() ? SelectedWorld->SaveFileName : FString();
	for (const TSharedPtr<FAPSExistingWorldEntry>& ExistingEntry : ExistingWorlds)
	{
		if (ExistingEntry.IsValid())
		{
			CachedEntries.Add(ExistingEntry->SaveFileName, ExistingEntry);
		}
	}
	ExistingWorlds.Reset();
	TArray<FString> SaveFiles;
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager::Get().FindFiles(SaveFiles, *SaveDirectory, TEXT("*.sav"));
	for (const FString& SaveFile : SaveFiles)
	{
		const FString SlotName = FPaths::GetBaseFilename(SaveFile);
		const FFileStatData Stat = IFileManager::Get().GetStatData(*(SaveDirectory / SaveFile));
		TSharedPtr<FAPSExistingWorldEntry> Entry = CachedEntries.FindRef(SlotName);
		const bool bCacheIsCurrent = Entry.IsValid()
			&& Entry->FileTimestamp == Stat.ModificationTime.ToUnixTimestamp()
			&& Entry->FileSizeBytes == Stat.FileSize;
		if (!Entry.IsValid())
		{
			Entry = MakeShared<FAPSExistingWorldEntry>();
		}
		Entry->SaveFileName = SlotName;
		if (!bCacheIsCurrent)
		{
			Entry->DisplayName = SlotName;
			Entry->SystemType = TEXT("FULL-SCALE STAR SYSTEM");
			Entry->StarType = TEXT("GENERATED STAR");
			Entry->PlanetType = TEXT("PERSISTENT WORLD");
			Entry->Environment = TEXT("Legacy world - details load on launch");
			Entry->TotalPlanets = 0;
			Entry->InhabitedPlanets = 0;
			Entry->bMetadataLoaded = false;
		}
		Entry->FileTimestamp = Stat.ModificationTime.ToUnixTimestamp();
		Entry->FileSizeBytes = Stat.FileSize;
		ExistingWorlds.Add(Entry);
	}
	ExistingWorlds.Sort([](const auto& A, const auto& B) { return A->FileTimestamp > B->FileTimestamp; });
	const TSharedPtr<FAPSExistingWorldEntry>* RestoredSelection = ExistingWorlds.FindByPredicate(
		[&PreviouslySelectedSlot](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
		{
			return Entry.IsValid() && Entry->SaveFileName == PreviouslySelectedSlot;
		});
	SelectedWorld = RestoredSelection ? *RestoredSelection : (ExistingWorlds.Num() > 0 ? ExistingWorlds[0] : nullptr);
}

void SAPSMainMenuRoot::BeginExistingWorldMetadataLoad()
{
	TArray<FString> LightweightSlots;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		// Old saves can exceed 100 MB and deserialize on the game thread even when
		// their file read is asynchronous. Keep the browser instant; large legacy
		// worlds remain launchable and receive metadata after a future lightweight save.
		if (Entry.IsValid() && !Entry->bMetadataLoaded
			&& Entry->FileSizeBytes <= 1ll * 1024ll * 1024ll)
		{
			LightweightSlots.Add(Entry->SaveFileName);
		}
	}
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->LoadWorldMetadataAsync(LightweightSlots);
	}
}

void SAPSMainMenuRoot::ApplyExistingWorldMetadata(const FString& SlotName, const UGameSave* Save)
{
	if (!Save) return;
	const TSharedPtr<FAPSExistingWorldEntry>* Found = ExistingWorlds.FindByPredicate(
		[&SlotName](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
		{
			return Entry.IsValid() && Entry->SaveFileName.Equals(SlotName, ESearchCase::IgnoreCase);
		});
	if (!Found || !Found->IsValid()) return;

	FAPSExistingWorldEntry& Entry = *Found->Get();
	Entry.DisplayName = Save->WorldName.IsEmpty() ? Entry.SaveFileName : Save->WorldName;
	Entry.InhabitedPlanets = Save->InhabitedPlanetsDataArray.Num();
	Entry.bMetadataLoaded = true;
	Entry.SystemType = TEXT("GENERATED WORLD");
	if (Save->GeneratedWorldsDataArray.Num() > 0)
	{
		const FGeneratedWorldData& Data = Save->GeneratedWorldsDataArray[0];
		Entry.SystemType = APSMenu::EnumLabel(Data.PlanetarySystemType);
		Entry.StarType = APSMenu::EnumLabel(Data.SpectralClass);
		Entry.PlanetType = APSMenu::EnumLabel(Data.PlanetType);
		Entry.Environment = FString::Printf(TEXT("%s / %.0f KM"), *Entry.PlanetType, Data.PlanetRadius);
		Entry.TotalPlanets = Data.PlanetsAmount;
	}
	RebuildExistingWorldGrid();
	if (SelectedWorld == *Found) RebuildExistingWorldDetails();
}

void SAPSMainMenuRoot::RebuildExistingWorldGrid()
{
	if (!ExistingWorldGridHost) return;
	TArray<TSharedPtr<FAPSExistingWorldEntry>> Filtered;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		if (Entry.IsValid()
			&& (WorldSearch.IsEmpty() || Entry->DisplayName.Contains(WorldSearch, ESearchCase::IgnoreCase))
			&& PassesExistingWorldFilters(*Entry))
		{
			Filtered.Add(Entry);
		}
	}
	if (WorldCollection == EAPSWorldCollection::Recent && Filtered.Num() > 6)
	{
		Filtered.SetNum(6, EAllowShrinking::No);
	}
	if (WorldSortMode == EAPSWorldSortMode::Name)
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->DisplayName < B->DisplayName; });
	}
	else if (WorldSortMode == EAPSWorldSortMode::SaveSize)
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->FileSizeBytes > B->FileSizeBytes; });
	}
	else
	{
		Filtered.Sort([](const auto& A, const auto& B){ return A->FileTimestamp > B->FileTimestamp; });
	}
	constexpr int32 ItemsPerPage = 6;
	const int32 MaxPage = FMath::Max(0, FMath::DivideAndRoundUp(Filtered.Num(), ItemsPerPage) - 1);
	ExistingWorldPage = FMath::Clamp(ExistingWorldPage, 0, MaxPage);

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	const int32 FirstIndex = ExistingWorldPage * ItemsPerPage;
	const int32 LastIndex = FMath::Min(FirstIndex + ItemsPerPage, Filtered.Num());
	for (int32 SourceIndex = FirstIndex; SourceIndex < LastIndex; ++SourceIndex)
	{
		const int32 VisibleIndex = SourceIndex - FirstIndex;
		const TSharedPtr<FAPSExistingWorldEntry>& Entry = Filtered[SourceIndex];
		const uint32 StableImageIndex = GetTypeHash(Entry->SaveFileName) % 3u;
		const FSlateBrush* Image = StableImageIndex == 0 ? &PlanetImage : (StableImageIndex == 1 ? &GalaxyImage : &SystemImage);
		const bool bSelected = SelectedWorld == Entry;
		const FString LastPlayed = FDateTime::FromUnixTimestamp(Entry->FileTimestamp).ToString(TEXT("%Y-%m-%d  %H:%M"));
		const int32 ColumnCount = bCompactWorldList ? 1 : 2;
		Grid->AddSlot(VisibleIndex % ColumnCount, VisibleIndex / ColumnCount)
		[
			SNew(SButton).ButtonStyle(bSelected ? &PrimaryButtonStyle : &CardButtonStyle).OnClicked(this, &SAPSMainMenuRoot::SelectExistingWorld, Entry).ContentPadding(0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox).HeightOverride(bCompactWorldList ? 125.0f : 205.0f).Clipping(EWidgetClipping::ClipToBounds)
					[SNew(SScaleBox).Stretch(EStretch::ScaleToFitX)[SNew(SImage).Image(Image)]]
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10.0f)
				[
					SNew(SButton).ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.OnClicked(this, &SAPSMainMenuRoot::ToggleWorldFavorite, Entry).ContentPadding(2.0f)
					[SNew(STextBlock).Text(LOCTEXT("FavoriteStar", "*")).Font(APSMenu::Font("Bold", 21)).ColorAndOpacity(Entry->bFavorite ? APSMenu::Amber : APSMenu::White)]
				]
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.002f, 0.01f, 0.02f, 0.94f)).Padding(12.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Entry->DisplayName)).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::White)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)[SNew(STextBlock).Text(FText::FromString(Entry->SystemType)).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Cyan)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("LAST PLAYED  %s"), *LastPlayed))).Font(APSMenu::Font("Regular", 9)).ColorAndOpacity(APSMenu::Muted)]
					]
				]
			]
		];
	}
	ExistingWorldGridHost->SetContent(Grid);
}

void SAPSMainMenuRoot::RebuildExistingWorldDetails()
{
	if (!ExistingWorldDetailsHost) return;
	if (!SelectedWorld)
	{
		ExistingWorldDetailsHost->SetContent(SNew(STextBlock).Text(LOCTEXT("NoWorlds", "NO SAVED WORLDS FOUND")).ColorAndOpacity(APSMenu::Muted));
		return;
	}

	FString DetailsText = FString::Printf(TEXT("SYSTEM TYPE                 %s\n\nSTAR TYPE                       %s\n\nTOTAL PLANETS               %d\n\nINHABITED PLANETS       %d\n\nENVIRONMENT                %s\n\nSAVE SIZE                       %s"),
		*SelectedWorld->SystemType, *SelectedWorld->StarType, SelectedWorld->TotalPlanets,
		SelectedWorld->InhabitedPlanets, *SelectedWorld->Environment,
		*APSMenu::SaveSizeText(SelectedWorld->FileSizeBytes).ToString());
	if (bShowTechnicalWorldDetails)
	{
		DetailsText += FString::Printf(TEXT("\n\nSAVE SLOT                       %s\n\nMETADATA                       %s\n\nLAST PLAYED                 %s"),
			*SelectedWorld->SaveFileName,
			SelectedWorld->bMetadataLoaded ? TEXT("READY") : TEXT("DEFERRED UNTIL LAUNCH"),
			*FDateTime::FromUnixTimestamp(SelectedWorld->FileTimestamp).ToString(TEXT("%Y-%m-%d  %H:%M")));
	}
	const uint32 StableImageIndex = GetTypeHash(SelectedWorld->SaveFileName) % 3u;
	const FSlateBrush* DetailsImage = StableImageIndex == 0
		? &PlanetImage : (StableImageIndex == 1 ? &GalaxyImage : &SystemImage);

	ExistingWorldDetailsHost->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(0.38f)
		[
			SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
			[SNew(SScaleBox).Stretch(EStretch::ScaleToFitX)[SNew(SImage).Image(DetailsImage)]]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 6.0f)[SNew(STextBlock).Text(FText::FromString(SelectedWorld->DisplayName)).Font(APSMenu::Font("Bold", 24)).ColorAndOpacity(APSMenu::White)]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(SelectedWorld->SystemType)).ColorAndOpacity(APSMenu::Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f)
		[
			SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(APSMenu::Muted)
			.Text(FText::FromString(DetailsText))
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f)
		[
			SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ContinueExistingWorld).ContentPadding(FMargin(20.0f, 14.0f))
			[SNew(STextBlock).Text(LOCTEXT("ContinueWorld", "CONTINUE  >")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 17)).ColorAndOpacity(APSMenu::White)]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ToggleWorldDetails).ContentPadding(FMargin(12.0f, 9.0f))[SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bShowTechnicalWorldDetails ? TEXT("HIDE DETAILS") : TEXT("WORLD DETAILS")); }).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::Cyan)]]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[SNew(SButton).IsEnabled(false).ButtonStyle(&SecondaryButtonStyle).ContentPadding(FMargin(12.0f, 9.0f))[SNew(STextBlock).Text(LOCTEXT("DeleteWorld", "DELETE WORLD")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(FLinearColor(0.88f, 0.19f, 0.14f, 1.0f))]]
		]);
}

FReply SAPSMainMenuRoot::ChangeExistingWorldPage(int32 Delta)
{
	ExistingWorldPage = FMath::Max(0, ExistingWorldPage + Delta);
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

bool SAPSMainMenuRoot::PassesExistingWorldFilters(const FAPSExistingWorldEntry& Entry) const
{
	if (WorldCollection == EAPSWorldCollection::Favorites && !Entry.bFavorite)
	{
		return false;
	}

	const int32 StarFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::StarType);
	if (StarFilter != 0)
	{
		const FString Star = Entry.StarType.ToUpper();
		const bool bKnown = Entry.bMetadataLoaded;
		const bool bHot = Star == TEXT("O") || Star == TEXT("B") || Star == TEXT("A");
		const bool bSolar = Star == TEXT("F") || Star == TEXT("G") || Star == TEXT("K");
		const bool bCool = Star == TEXT("M") || Star == TEXT("L") || Star == TEXT("T") || Star == TEXT("Y");
		if ((StarFilter == 1 && !bHot) || (StarFilter == 2 && !bSolar)
			|| (StarFilter == 3 && !bCool) || (StarFilter == 4 && bKnown))
		{
			return false;
		}
	}

	const int32 WorldTypeFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::WorldType);
	if (WorldTypeFilter != 0)
	{
		const FString Type = Entry.SystemType.ToUpper();
		const bool bMulti = Type.Contains(TEXT("MULTI"));
		const bool bSingle = Type.Contains(TEXT("SINGLE"));
		if ((WorldTypeFilter == 1 && !bMulti) || (WorldTypeFilter == 2 && !bSingle)
			|| (WorldTypeFilter == 3 && (bMulti || bSingle)))
		{
			return false;
		}
	}

	const int32 InhabitedFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::Inhabited);
	if ((InhabitedFilter == 1 && Entry.InhabitedPlanets <= 0)
		|| (InhabitedFilter == 2 && Entry.InhabitedPlanets > 0))
	{
		return false;
	}

	const int32 EnvironmentFilter = WorldFilterIndices.FindRef(EAPSWorldFilterKind::Environment);
	if (EnvironmentFilter != 0)
	{
		const FString EnvironmentValue = (Entry.Environment + TEXT(" ") + Entry.PlanetType).ToUpper();
		const bool bRocky = EnvironmentValue.Contains(TEXT("ROCK")) || EnvironmentValue.Contains(TEXT("EARTH"));
		const bool bGas = EnvironmentValue.Contains(TEXT("GAS"));
		const bool bIce = EnvironmentValue.Contains(TEXT("ICE")) || EnvironmentValue.Contains(TEXT("FROZEN"));
		if ((EnvironmentFilter == 1 && !bRocky) || (EnvironmentFilter == 2 && !bGas)
			|| (EnvironmentFilter == 3 && !bIce)
			|| (EnvironmentFilter == 4 && (bRocky || bGas || bIce)))
		{
			return false;
		}
	}
	return true;
}

FText SAPSMainMenuRoot::GetWorldCollectionLabel(EAPSWorldCollection Collection) const
{
	switch (Collection)
	{
	case EAPSWorldCollection::MyWorlds: return LOCTEXT("MyWorldsValue", "MY WORLDS");
	case EAPSWorldCollection::Favorites: return LOCTEXT("FavoritesValue", "FAVORITES");
	case EAPSWorldCollection::Recent: return LOCTEXT("RecentValue", "RECENT");
	default: return LOCTEXT("AllWorldsValue", "ALL WORLDS");
	}
}

FText SAPSMainMenuRoot::GetWorldSortLabel() const
{
	switch (WorldSortMode)
	{
	case EAPSWorldSortMode::Name: return LOCTEXT("SortName", "SORT: NAME  v");
	case EAPSWorldSortMode::SaveSize: return LOCTEXT("SortSize", "SORT: SAVE SIZE  v");
	default: return LOCTEXT("SortRecent", "SORT: LAST PLAYED  v");
	}
}

FText SAPSMainMenuRoot::GetWorldFilterLabel(EAPSWorldFilterKind Kind) const
{
	static const TCHAR* StarLabels[] = {TEXT("ANY  v"), TEXT("HOT O/B/A  v"), TEXT("SOLAR F/G/K  v"), TEXT("COOL M/L/T/Y  v"), TEXT("UNKNOWN  v")};
	static const TCHAR* TypeLabels[] = {TEXT("ANY  v"), TEXT("MULTI PLANET  v"), TEXT("SINGLE STAR  v"), TEXT("OTHER  v")};
	static const TCHAR* InhabitedLabels[] = {TEXT("ANY  v"), TEXT("INHABITED  v"), TEXT("UNINHABITED  v")};
	static const TCHAR* EnvironmentLabels[] = {TEXT("ANY  v"), TEXT("ROCKY  v"), TEXT("GAS  v"), TEXT("ICE  v"), TEXT("OTHER  v")};
	const int32 Index = WorldFilterIndices.FindRef(Kind);
	switch (Kind)
	{
	case EAPSWorldFilterKind::StarType: return FText::FromString(StarLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(StarLabels) - 1)]);
	case EAPSWorldFilterKind::WorldType: return FText::FromString(TypeLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(TypeLabels) - 1)]);
	case EAPSWorldFilterKind::Inhabited: return FText::FromString(InhabitedLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(InhabitedLabels) - 1)]);
	default: return FText::FromString(EnvironmentLabels[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(EnvironmentLabels) - 1)]);
	}
}

FReply SAPSMainMenuRoot::SetWorldCollection(EAPSWorldCollection Collection)
{
	WorldCollection = Collection;
	ExistingWorldPage = 0;
	if (!SelectedWorld.IsValid() || !PassesExistingWorldFilters(*SelectedWorld))
	{
		const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
			[this](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
			{
				return Entry.IsValid() && PassesExistingWorldFilters(*Entry);
			});
		SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
	}
	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CycleWorldSort()
{
	WorldSortMode = static_cast<EAPSWorldSortMode>((static_cast<uint8>(WorldSortMode) + 1) % 3);
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldView()
{
	bCompactWorldList = !bCompactWorldList;
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::CycleWorldFilter(EAPSWorldFilterKind Kind)
{
	static const int32 Counts[] = {5, 4, 3, 5};
	int32& Index = WorldFilterIndices.FindOrAdd(Kind);
	Index = (Index + 1) % Counts[static_cast<uint8>(Kind)];
	ExistingWorldPage = 0;
	if (!SelectedWorld.IsValid() || !PassesExistingWorldFilters(*SelectedWorld))
	{
		const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
			[this](const TSharedPtr<FAPSExistingWorldEntry>& Entry)
			{
				return Entry.IsValid() && PassesExistingWorldFilters(*Entry);
			});
		SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
	}
	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldFavorite(TSharedPtr<FAPSExistingWorldEntry> Entry)
{
	if (Entry.IsValid())
	{
		Entry->bFavorite = !Entry->bFavorite;
		if (WorldCollection == EAPSWorldCollection::Favorites && !Entry->bFavorite && SelectedWorld == Entry)
		{
			const TSharedPtr<FAPSExistingWorldEntry>* FirstMatch = ExistingWorlds.FindByPredicate(
				[this](const TSharedPtr<FAPSExistingWorldEntry>& Candidate)
				{
					return Candidate.IsValid() && PassesExistingWorldFilters(*Candidate);
				});
			SelectedWorld = FirstMatch ? *FirstMatch : nullptr;
			RebuildExistingWorldDetails();
		}
		RebuildExistingWorldGrid();
	}
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::ToggleWorldDetails()
{
	bShowTechnicalWorldDetails = !bShowTechnicalWorldDetails;
	RebuildExistingWorldDetails();
	return FReply::Handled();
}

void SAPSMainMenuRoot::LoadSpawnClassOptions()
{
	if (SpawnClassOptions.Num() > 0) return;
	UClass* PickerClass = LoadClass<USpawnClassPicker>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/GenerationMenu/WBP_CivilizationMenu_UI.WBP_CivilizationMenu_UI_C"));
	if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(PickerClass);
	const USpawnClassPicker* Picker = PickerClass ? PickerClass->GetDefaultObject<USpawnClassPicker>() : nullptr;
	SpawnClassOptions.Add(EAPSStartAssetSlot::Character, Picker ? Picker->CharacterClasses : TArray<TSubclassOf<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Spaceship, Picker ? Picker->SpaceshipClasses : TArray<TSubclassOf<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::SpaceStation, Picker ? Picker->SpaceStationClasses : TArray<TSubclassOf<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Headquarters, Picker ? Picker->SpaceHeadquartersClasses : TArray<TSubclassOf<AActor>>());
	SpawnClassOptions.Add(EAPSStartAssetSlot::Shipyard, Picker ? Picker->ShipyardClasses : TArray<TSubclassOf<AActor>>());
	for (auto& Pair : SpawnClassOptions)
	{
		UClass* CurrentClass = nullptr;
		if (ViewModel.IsValid() && ViewModel->SpawnParameters)
		{
			switch (Pair.Key)
			{
			case EAPSStartAssetSlot::Character: CurrentClass = ViewModel->SpawnParameters->BP_CharacterClass; break;
			case EAPSStartAssetSlot::Spaceship: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceship; break;
			case EAPSStartAssetSlot::SpaceStation: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceStation; break;
			case EAPSStartAssetSlot::Headquarters: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceHeadquarters; break;
			case EAPSStartAssetSlot::Shipyard: CurrentClass = ViewModel->SpawnParameters->BP_HomeSpaceShipyard; break;
			}
		}
		// Slate remains functional even if the legacy picker Blueprint is absent or
		// has an empty array: generator defaults are valid runtime selections and do
		// not require the user to reconnect anything in UMG.
		if (CurrentClass && CurrentClass->IsChildOf(AActor::StaticClass()))
		{
			Pair.Value.AddUnique(CurrentClass);
		}
		int32 InitialIndex = Pair.Value.IndexOfByPredicate([CurrentClass](const TSubclassOf<AActor>& Candidate)
		{
			return Candidate.Get() == CurrentClass;
		});
		if (InitialIndex == INDEX_NONE) InitialIndex = 0;
		SpawnClassIndices.Add(Pair.Key, InitialIndex);
		if (Pair.Value.IsValidIndex(InitialIndex) && ViewModel.IsValid())
		{
			ViewModel->SetSpawnClass(Pair.Key, Pair.Value[InitialIndex]);
		}
		RefreshSpawnClassBrush(Pair.Key);
	}
}

void SAPSMainMenuRoot::RefreshSpawnClassBrush(EAPSStartAssetSlot Slot)
{
	FSlateBrush& Brush = SpawnClassBrushes.FindOrAdd(Slot);
	Brush.SetResourceObject(nullptr);
	Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
	const TArray<TSubclassOf<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index) || !(*Options)[Index]) return;
	const AActor* DefaultActor = (*Options)[Index]->GetDefaultObject<AActor>();
	if (!DefaultActor || !DefaultActor->Implements<UItemInfoInterface>()) return;
	if (UTexture2D* Texture = IItemInfoInterface::Execute_GetAvatarPicture(DefaultActor))
	{
		if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(Texture);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(256.0f, 180.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
	}
}

const FSlateBrush* SAPSMainMenuRoot::GetSpawnClassBrush(EAPSStartAssetSlot Slot) const
{
	return SpawnClassBrushes.Find(Slot);
}

FText SAPSMainMenuRoot::GetSpawnClassName(EAPSStartAssetSlot Slot) const
{
	const TArray<TSubclassOf<AActor>>* Options = SpawnClassOptions.Find(Slot);
	const int32 Index = SpawnClassIndices.FindRef(Slot);
	if (!Options || !Options->IsValidIndex(Index) || !(*Options)[Index]) return LOCTEXT("Unavailable", "NOT CONFIGURED");
	const AActor* DefaultActor = (*Options)[Index]->GetDefaultObject<AActor>();
	if (DefaultActor && DefaultActor->Implements<UItemInfoInterface>())
	{
		return IItemInfoInterface::Execute_GetInGameName(DefaultActor);
	}
	return FText::FromString((*Options)[Index]->GetName().Replace(TEXT("BP_"), TEXT("")));
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildSpawnCard(EAPSStartAssetSlot Slot, const FText& Label)
{
	return SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(14.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f, 10.0f)
			[
				SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
				[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image_Lambda([this, Slot]() { return GetSpawnClassBrush(Slot); })]]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([this, Slot]() { return GetSpawnClassName(Slot); }).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::White).Justification(ETextJustify::Center)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, -1)[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Justification(ETextJustify::Center)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CycleSpawnClass, Slot, 1)[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Justification(ETextJustify::Center)]]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildCivilizationPage()
{
	LoadSpawnClassOptions();
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;

	const auto EnumControl = [this](const FText& Label, const UEnum* Enum,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter)
	{
		const auto Step = [Enum, Getter, Setter](int32 Direction)
		{
			if (!Enum) return FReply::Handled();
			const int32 Count = FMath::Max(1, Enum->NumEnums() - 1);
			Setter((Getter() + Direction + Count) % Count);
			return FReply::Handled();
		};
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked_Lambda([Step](){ return Step(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).ColorAndOpacity(APSMenu::Cyan)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([Enum, Getter](){ return Enum ? Enum->GetDisplayNameTextByValue(Getter()) : FText::FromString(TEXT("--")); }).Font(APSMenu::Font("Bold", 10)).ColorAndOpacity(APSMenu::White)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&SecondaryButtonStyle).OnClicked_Lambda([Step](){ return Step(1); })[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).ColorAndOpacity(APSMenu::Cyan)]]
			];
	};

	const auto NumberControl = [](const FText& Label, int32 MinValue, int32 MaxValue, int32 Delta,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SSpinBox<int32>).MinValue(MinValue).MaxValue(MaxValue).Delta(Delta)
				.Value_Lambda([Getter](){ return Getter(); })
				.OnValueChanged_Lambda([Setter](int32 Value){ Setter(Value); })
			];
	};

	const auto InfoPanel = [](const FText& Title, TAttribute<FText> Body)
	{
		return SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(16.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", 13)).ColorAndOpacity(APSMenu::Cyan)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(Body).AutoWrapText(true).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::White)]
		];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(28.0f, 18.0f, 28.0f, 6.0f)[BuildHeader(LOCTEXT("CivParameters", "CIVILIZATION PARAMETERS"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(34.0f, 10.0f, 34.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.74f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.56f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Character, LOCTEXT("Character", "CHARACTER"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Spaceship, LOCTEXT("Spaceship", "SPACESHIP"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::SpaceStation, LOCTEXT("Station", "SPACE STATION"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Headquarters, LOCTEXT("HQ", "HEADQUARTERS"))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)[BuildSpawnCard(EAPSStartAssetSlot::Shipyard, LOCTEXT("Shipyard", "SHIPYARD"))]
				]
				+ SVerticalBox::Slot().FillHeight(0.44f).Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("CivilizationStatus", "CIVILIZATION STATUS"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Government: %s\nEconomy: %s\nSociety: %s\nPopulation: %s"), *APSMenu::EnumLabel(P->GovernmentType), *APSMenu::EnumLabel(P->EconomicSystem), *APSMenu::EnumLabel(P->SocietyType), *FText::AsNumber(P->FoundingPopulation).ToString())) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("StarSystemInfo", "STAR SYSTEM INFO"), TAttribute<FText>::CreateLambda([VM](){ const UGeneratedWorld* W=VM.IsValid()?VM->GeneratedWorld.Get():nullptr; return W ? FText::FromString(FString::Printf(TEXT("System: %s\nStar: %s / %s\nPlanets: %d\nHome planet: %s / %.0f KM"), *APSMenu::EnumLabel(W->PlanetarySystemType), *APSMenu::EnumLabel(W->StellarType), *APSMenu::EnumLabel(W->SpectralClass), W->PlanetsAmount, *APSMenu::EnumLabel(W->PlanetType), W->PlanetRadius)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("Infrastructure", "INFRASTRUCTURE"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Star outposts: %d\nPlanet outposts: %d\nOrbital stations: %d\nGround settlements: %d\nStarting fleet: %d"), P->StarOutposts, P->PlanetOutposts, P->OrbitalOutposts, P->GroundOutposts, P->StartingFleetSize)) : FText::GetEmpty(); }))]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4.0f)
					[InfoPanel(LOCTEXT("Divisions", "DIVISIONS"), TAttribute<FText>::CreateLambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P ? FText::FromString(FString::Printf(TEXT("Exploration     Lv.%d\nIndustry          Lv.%d\nScience           Lv.%d\nCivil Affairs    Lv.%d\nMilitary           Lv.%d\nFleet                Lv.%d"), P->ExplorationDivisionLevel, P->IndustryDivisionLevel, P->ScienceDivisionLevel, P->CivilAffairsDivisionLevel, P->MilitaryDivisionLevel, P->FleetDivisionLevel)) : FText::GetEmpty(); }))]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.26f).Padding(5.0f)
			[
				SNew(SBorder).BorderImage(&APSMenu::PanelBrush).Padding(16.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SNew(STextBlock).Text(LOCTEXT("SpawnParameters", "CIVILIZATION & SPAWN")).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("CivilizationName", "CIVILIZATION NAME")).Font(APSMenu::Font("Regular", 10)).ColorAndOpacity(APSMenu::Muted)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)[SNew(SEditableTextBox).Text_Lambda([VM](){ const USpawnParameters* P=VM.IsValid()?VM->SpawnParameters.Get():nullptr; return P?FText::FromString(P->CivilizationName):FText::GetEmpty(); }).OnTextCommitted_Lambda([VM](const FText& T,ETextCommit::Type){ if(VM.IsValid()&&VM->SpawnParameters) VM->SpawnParameters->CivilizationName=T.ToString(); })]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Archetype", "ARCHETYPE"), StaticEnum<EAPSCivilizationArchetype>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CivilizationArchetype):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilizationArchetype=static_cast<EAPSCivilizationArchetype>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Government", "GOVERNMENT"), StaticEnum<EAPSGovernmentType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->GovernmentType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GovernmentType=static_cast<EAPSGovernmentType>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Economy", "ECONOMY"), StaticEnum<EAPSEconomicSystem>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->EconomicSystem):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->EconomicSystem=static_cast<EAPSEconomicSystem>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("Society", "SOCIETY"), StaticEnum<EAPSSocietyType>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->SocietyType):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->SocietyType=static_cast<EAPSSocietyType>(V);})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FoundingPopulation", "FOUNDING POPULATION"), 1, 100000000, 1000, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FoundingPopulation:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FoundingPopulation=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("StartingCredits", "STARTING CREDITS"), 0, 2000000000, 10000, [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(FMath::Min<int64>(VM->SpawnParameters->StartingCredits, MAX_int32)):0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingCredits=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("Technology", "TECHNOLOGY LEVEL"), 1, 10, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->TechnologyLevel:1;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->TechnologyLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("SpawnPlace", "START LOCATION"), StaticEnum<ECharSpawnPlace>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->CharacterSpawnPlace):0;}, [VM](int32 V){if(VM.IsValid())VM->SetCharacterSpawnPlace(V);})]
						+ SVerticalBox::Slot().AutoHeight()[EnumControl(LOCTEXT("OrbitHeight", "HOME ORBIT"), StaticEnum<EOrbitHeight>(), [VM](){return VM.IsValid()&&VM->SpawnParameters?static_cast<int32>(VM->SpawnParameters->HomeStationOrbitHeight):0;}, [VM](int32 V){if(VM.IsValid())VM->SetStationOrbitHeight(V);})]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)[SNew(STextBlock).Text(LOCTEXT("InfrastructureSetup", "STARTING INFRASTRUCTURE")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Cyan)]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FleetSize", "STARTING FLEET"), 0, 1000, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StartingFleetSize:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StartingFleetSize=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("StarOutposts", "STAR OUTPOSTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->StarOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->StarOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("PlanetOutposts", "PLANET OUTPOSTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->PlanetOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->PlanetOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("OrbitalOutposts", "ORBITAL STATIONS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->OrbitalOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->OrbitalOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("GroundOutposts", "GROUND SETTLEMENTS"), 0, 100, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->GroundOutposts:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->GroundOutposts=V;})]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 8.0f)[SNew(STextBlock).Text(LOCTEXT("DivisionSetup", "DIVISION LEVELS")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Cyan)]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("ExplorationDivision", "EXPLORATION"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ExplorationDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ExplorationDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("IndustryDivision", "INDUSTRY"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->IndustryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->IndustryDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("ScienceDivision", "SCIENCE / RESEARCH"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->ScienceDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->ScienceDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("CivilDivision", "CIVIL AFFAIRS"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->CivilAffairsDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->CivilAffairsDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("MilitaryDivision", "MILITARY"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->MilitaryDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->MilitaryDivisionLevel=V;})]
						+ SVerticalBox::Slot().AutoHeight()[NumberControl(LOCTEXT("FleetDivision", "FLEET COMMAND"), 0, 20, 1, [VM](){return VM.IsValid()&&VM->SpawnParameters?VM->SpawnParameters->FleetDivisionLevel:0;}, [VM](int32 V){if(VM.IsValid()&&VM->SpawnParameters)VM->SpawnParameters->FleetDivisionLevel=V;})]
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 24.0f)
		[
			SNew(SBox).WidthOverride(440.0f)
			[
				SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::CommitCivilization).ContentPadding(FMargin(24.0f, 16.0f))
				[SNew(STextBlock).Text(LOCTEXT("GenerateWorld", "GENERATE WORLD  >")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 19)).ColorAndOpacity(APSMenu::White)]
			]
		];
}

void SAPSMainMenuRoot::LoadVisualResources()
{
	APSMenu::DisplayFont = LoadObject<UFont>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font"));
	APSMenu::BodyFont = LoadObject<UFont>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font"));
	if (AMainMenuController* PC = Controller.Get())
	{
		PC->HoldSlateResource(APSMenu::DisplayFont.Get());
		PC->HoldSlateResource(APSMenu::BodyFont.Get());
	}

	auto Load = [this](FSlateBrush& Brush, const TCHAR* Path)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path);
		if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(Texture);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Texture
			? FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()))
			: FVector2D(640.0f, 420.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
	};
	Load(SystemImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/T_System.T_System"));
	Load(PlanetImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/planet_realistic.planet_realistic"));
	Load(GalaxyImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/spiral_galaxy.spiral_galaxy"));
	Load(ClusterImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/I_Cluster.I_Cluster"));
	Load(CivilizationImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/circular_galaxy.circular_galaxy"));
	Load(BackgroundImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/Screenshot_2023.Screenshot_2023"));
}

FReply SAPSMainMenuRoot::Back()
{
	if (CurrentPage == EAPSMenuPage::Landing) return FReply::Handled();
	EAPSMenuPage TargetPage = EAPSMenuPage::ChoosePath;
	if (CurrentPage == EAPSMenuPage::ChoosePath) TargetPage = EAPSMenuPage::Landing;
	else if (CurrentPage == EAPSMenuPage::Civilization) TargetPage = EAPSMenuPage::AstronomicalGeneration;
	Navigate(TargetPage);
	return FReply::Handled();
}

FReply SAPSMainMenuRoot::OpenChoosePath() { Navigate(EAPSMenuPage::ChoosePath); return FReply::Handled(); }
FReply SAPSMainMenuRoot::StartSingleGame() { if (AMainMenuController* PC = Controller.Get()) PC->LaunchSingleGame(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenExistingWorlds() { Navigate(EAPSMenuPage::ExistingWorlds); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenAstronomicalGeneration(EAstroPreviewFocus Focus) { if (ViewModel.IsValid()) { ViewModel->SetPreviewFocus(Focus); ViewModel->RequestPreview(); } Navigate(EAPSMenuPage::AstronomicalGeneration); return FReply::Handled(); }
FReply SAPSMainMenuRoot::OpenCivilization() { Navigate(EAPSMenuPage::Civilization); return FReply::Handled(); }
FReply SAPSMainMenuRoot::CommitCivilization() { if (ViewModel.IsValid()) ViewModel->CommitAndOpenLevel(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::ContinueExistingWorld() { if (SelectedWorld.IsValid()) if (AMainMenuController* PC = Controller.Get()) PC->LoadWorldSlot(SelectedWorld->SaveFileName); return FReply::Handled(); }
FReply SAPSMainMenuRoot::SelectExistingWorld(TSharedPtr<FAPSExistingWorldEntry> Entry) { SelectedWorld = Entry; RebuildExistingWorldDetails(); return FReply::Handled(); }
FReply SAPSMainMenuRoot::QuitGame() { if (AMainMenuController* PC = Controller.Get()) UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false); return FReply::Handled(); }

void SAPSMainMenuRoot::OnWorldSearchChanged(const FText& SearchText)
{
	WorldSearch = SearchText.ToString();
	ExistingWorldPage = 0;
	RebuildExistingWorldGrid();
}

FReply SAPSMainMenuRoot::CycleSpawnClass(EAPSStartAssetSlot Slot, int32 Direction)
{
	const TArray<TSubclassOf<AActor>>* Options = SpawnClassOptions.Find(Slot);
	if (!Options || Options->Num() == 0) return FReply::Handled();
	int32& Index = SpawnClassIndices.FindOrAdd(Slot);
	Index = (Index + Direction + Options->Num()) % Options->Num();
	if (ViewModel.IsValid()) ViewModel->SetSpawnClass(Slot, (*Options)[Index]);
	RefreshSpawnClassBrush(Slot);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

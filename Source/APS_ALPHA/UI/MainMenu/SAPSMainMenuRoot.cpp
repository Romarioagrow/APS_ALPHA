#include "SAPSMainMenuRoot.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Interfaces/ItemInfoInterface.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
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
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
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
	const FLinearColor Background(0.003f, 0.012f, 0.022f, 0.78f);
	const FLinearColor Panel(0.012f, 0.045f, 0.07f, 0.90f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.05f, 0.30f, 0.42f, 1.0f);
	const FLinearColor Amber(1.0f, 0.55f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.48f, 0.62f, 0.70f, 1.0f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
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
			SNew(SBox).WidthOverride(150.0f)
			[
				SNew(SButton)
				.Visibility(bShowBack ? EVisibility::Visible : EVisibility::Collapsed)
				.ButtonStyle(&SecondaryButtonStyle)
				.OnClicked(this, &SAPSMainMenuRoot::Back)
				.ContentPadding(FMargin(18.0f, 10.0f))
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
				SNew(STextBlock).Text(LOCTEXT("Brand", "A P O S F E R A")).Font(APSMenu::Font("Bold", 34)).ColorAndOpacity(APSMenu::White)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("SubBrand", "S P A C E T R I P S   G E N E R A T I O N")).Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(APSMenu::Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(SectionTitle).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(150.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("Profile", "PROFILE    SETTINGS")).Justification(ETextJustify::Right)
				.Font(APSMenu::Font("Regular", 11)).ColorAndOpacity(APSMenu::Muted)
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
		.ButtonStyle(bEnabled ? &CardButtonStyle : &DisabledCardButtonStyle)
		.OnClicked_Lambda([Action]() mutable { Action.ExecuteIfBound(); return FReply::Handled(); })
		.ContentPadding(0.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(Image).ColorAndOpacity(bEnabled ? FLinearColor::White : FLinearColor(0.25f, 0.25f, 0.25f, 0.55f))
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.002f, 0.012f, 0.02f, 0.92f)).Padding(FMargin(22.0f, 18.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(Title).Font(APSMenu::Font("Bold", bLarge ? 25 : 17)).ColorAndOpacity(bEnabled ? Accent : APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(Description).AutoWrapText(true).Font(APSMenu::Font("Regular", 12)).ColorAndOpacity(bEnabled ? APSMenu::White : APSMenu::Muted)]
				]
			]
		];
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildChoosePathPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(24.0f, 20.0f, 24.0f, 12.0f)[BuildHeader(LOCTEXT("ChoosePath", "CHOOSE YOUR PATH"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(48.0f, 16.0f, 48.0f, 42.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.36f).Padding(6.0f)
			[
				BuildPathCard(LOCTEXT("StartGame", "START SINGLE GAME"),
					LOCTEXT("StartGameDesc", "Begin a new journey through the live full-scale world."), &SystemImage,
					APSMenu::Amber, FSimpleDelegate::CreateLambda([this]() { StartSingleGame(); }), true)
			]
			+ SHorizontalBox::Slot().FillWidth(0.64f).Padding(6.0f)
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
					[BuildPathCard(LOCTEXT("GenSpace", "GENERATE SPACE"), LOCTEXT("GenSpaceDesc", "Procedurally generate stellar systems and clusters."), &ClusterImage, APSMenu::Cyan, FSimpleDelegate::CreateLambda([this]() { OpenAstronomicalGeneration(EAstroPreviewFocus::StarCluster); }))]
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
}

TSharedRef<SWidget> SAPSMainMenuRoot::BuildExistingWorldsPage()
{
	TSharedRef<SWidget> Page = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(24.0f, 20.0f, 24.0f, 10.0f)[BuildHeader(LOCTEXT("VisitExisting", "VISIT EXISTING WORLD"))]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(24.0f, 10.0f, 24.0f, 28.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.18f).Padding(5.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Panel).Padding(18.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f)[SNew(STextBlock).Text(LOCTEXT("AllWorlds", "ALL WORLDS")).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::Amber)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f)[SNew(STextBlock).Text(LOCTEXT("MyWorlds", "MY WORLDS")).ColorAndOpacity(APSMenu::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f)[SNew(STextBlock).Text(LOCTEXT("Favorites", "FAVORITES")).ColorAndOpacity(APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f)[SNew(STextBlock).Text(LOCTEXT("Recent", "RECENT")).ColorAndOpacity(APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 24.0f, 0.0f, 8.0f)[SNew(STextBlock).Text(LOCTEXT("Filters", "FILTERS")).Font(APSMenu::Font("Bold", 12)).ColorAndOpacity(APSMenu::Muted)]
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("FilterLabels", "STAR TYPE        ALL TYPES\n\nWORLD TYPE     ALL TYPES\n\nINHABITED       ANY\n\nENVIRONMENT   ANY")).ColorAndOpacity(APSMenu::Muted)]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(5.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SSearchBox).HintText(LOCTEXT("SearchWorlds", "Search worlds..."))
					.OnTextChanged(this, &SAPSMainMenuRoot::OnWorldSearchChanged)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Panel).Padding(10.0f)
					[SAssignNew(ExistingWorldGridHost, SBox)]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.32f).Padding(5.0f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Panel).Padding(16.0f)
				[SAssignNew(ExistingWorldDetailsHost, SBox)]
			]
		];

	LoadExistingWorlds();
	RebuildExistingWorldGrid();
	RebuildExistingWorldDetails();
	return Page;
}

void SAPSMainMenuRoot::LoadExistingWorlds()
{
	ExistingWorlds.Reset();
	TArray<FString> SaveFiles;
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager::Get().FindFiles(SaveFiles, *SaveDirectory, TEXT("*.sav"));
	for (const FString& SaveFile : SaveFiles)
	{
		UGameSave* Save = Cast<UGameSave>(UGameplayStatics::LoadGameFromSlot(FPaths::GetBaseFilename(SaveFile), 0));
		if (!Save) continue;

		TSharedPtr<FAPSExistingWorldEntry> Entry = MakeShared<FAPSExistingWorldEntry>();
		Entry->SaveFileName = FPaths::GetBaseFilename(SaveFile);
		Entry->DisplayName = Save->WorldName.IsEmpty() ? FPaths::GetBaseFilename(SaveFile) : Save->WorldName;
		Entry->InhabitedPlanets = Save->InhabitedPlanetsDataArray.Num();
		if (Save->GeneratedWorldsDataArray.Num() > 0)
		{
			const FGeneratedWorldData& Data = Save->GeneratedWorldsDataArray[0];
			Entry->SystemType = APSMenu::EnumLabel(Data.PlanetarySystemType);
			Entry->StarType = APSMenu::EnumLabel(Data.SpectralClass);
			Entry->PlanetType = APSMenu::EnumLabel(Data.PlanetType);
			Entry->Environment = FString::Printf(TEXT("%s / %.0f KM"), *Entry->PlanetType, Data.PlanetRadius);
			Entry->TotalPlanets = Data.PlanetsAmount;
		}
		Entry->FileTimestamp = IFileManager::Get().GetStatData(*(SaveDirectory / SaveFile)).ModificationTime.ToUnixTimestamp();
		ExistingWorlds.Add(Entry);
	}
	ExistingWorlds.Sort([](const auto& A, const auto& B) { return A->FileTimestamp > B->FileTimestamp; });
	SelectedWorld = ExistingWorlds.Num() > 0 ? ExistingWorlds[0] : nullptr;
}

void SAPSMainMenuRoot::RebuildExistingWorldGrid()
{
	if (!ExistingWorldGridHost) return;
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	int32 VisibleIndex = 0;
	for (const TSharedPtr<FAPSExistingWorldEntry>& Entry : ExistingWorlds)
	{
		if (!WorldSearch.IsEmpty() && !Entry->DisplayName.Contains(WorldSearch, ESearchCase::IgnoreCase)) continue;
		const FSlateBrush* Image = (VisibleIndex % 3 == 0) ? &PlanetImage : ((VisibleIndex % 3 == 1) ? &GalaxyImage : &SystemImage);
		Grid->AddSlot(VisibleIndex % 2, VisibleIndex / 2)
		[
			SNew(SButton).ButtonStyle(&CardButtonStyle).OnClicked(this, &SAPSMainMenuRoot::SelectExistingWorld, Entry).ContentPadding(0.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SBox).HeightOverride(190.0f)[SNew(SImage).Image(Image)]]
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.002f, 0.01f, 0.02f, 0.90f)).Padding(12.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Entry->DisplayName)).Font(APSMenu::Font("Bold", 16)).ColorAndOpacity(APSMenu::White)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Entry->SystemType)).ColorAndOpacity(APSMenu::Cyan)]
					]
				]
			]
		];
		++VisibleIndex;
	}
	ExistingWorldGridHost->SetContent(SNew(SScrollBox).Orientation(Orient_Vertical) + SScrollBox::Slot()[Grid]);
}

void SAPSMainMenuRoot::RebuildExistingWorldDetails()
{
	if (!ExistingWorldDetailsHost) return;
	if (!SelectedWorld)
	{
		ExistingWorldDetailsHost->SetContent(SNew(STextBlock).Text(LOCTEXT("NoWorlds", "NO SAVED WORLDS FOUND")).ColorAndOpacity(APSMenu::Muted));
		return;
	}

	ExistingWorldDetailsHost->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(0.42f)[SNew(SImage).Image(&PlanetImage)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 6.0f)[SNew(STextBlock).Text(FText::FromString(SelectedWorld->DisplayName)).Font(APSMenu::Font("Bold", 24)).ColorAndOpacity(APSMenu::White)]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(SelectedWorld->SystemType)).ColorAndOpacity(APSMenu::Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f)
		[
			SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(APSMenu::Muted)
			.Text(FText::FromString(FString::Printf(TEXT("STAR TYPE\n%s\n\nTOTAL PLANETS\n%d\n\nINHABITED PLANETS\n%d\n\nENVIRONMENT\n%s"),
				*SelectedWorld->StarType, SelectedWorld->TotalPlanets, SelectedWorld->InhabitedPlanets, *SelectedWorld->Environment)))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f)
		[
			SNew(SButton).ButtonStyle(&PrimaryButtonStyle).OnClicked(this, &SAPSMainMenuRoot::ContinueExistingWorld).ContentPadding(FMargin(20.0f, 14.0f))
			[SNew(STextBlock).Text(LOCTEXT("ContinueWorld", "CONTINUE  >")).Justification(ETextJustify::Center).Font(APSMenu::Font("Bold", 17)).ColorAndOpacity(APSMenu::White)]
		]);
}

void SAPSMainMenuRoot::LoadSpawnClassOptions()
{
	if (SpawnClassOptions.Num() > 0) return;
	UClass* PickerClass = LoadClass<USpawnClassPicker>(nullptr,
		TEXT("/Game/APS/APS_ALPHA/UI/GenerationMenu/WBP_CivilizationMenu_UI.WBP_CivilizationMenu_UI_C"));
	if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(PickerClass);
	const USpawnClassPicker* Picker = PickerClass ? PickerClass->GetDefaultObject<USpawnClassPicker>() : nullptr;
	if (!Picker) return;
	SpawnClassOptions.Add(EAPSStartAssetSlot::Character, Picker->CharacterClasses);
	SpawnClassOptions.Add(EAPSStartAssetSlot::Spaceship, Picker->SpaceshipClasses);
	SpawnClassOptions.Add(EAPSStartAssetSlot::SpaceStation, Picker->SpaceStationClasses);
	SpawnClassOptions.Add(EAPSStartAssetSlot::Headquarters, Picker->SpaceHeadquartersClasses);
	SpawnClassOptions.Add(EAPSStartAssetSlot::Shipyard, Picker->ShipyardClasses);
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
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Panel).Padding(14.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(Label).Font(APSMenu::Font("Bold", 15)).ColorAndOpacity(APSMenu::Cyan)]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f, 10.0f)
			[
				SNew(SImage).Image_Lambda([this, Slot]() { return GetSpawnClassBrush(Slot); })
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
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(24.0f, 20.0f, 24.0f, 10.0f)[BuildHeader(LOCTEXT("CivParameters", "CIVILIZATION PARAMETERS"))]
		+ SVerticalBox::Slot().FillHeight(0.56f).Padding(50.0f, 20.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f)[BuildSpawnCard(EAPSStartAssetSlot::Character, LOCTEXT("Character", "CHARACTER"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f)[BuildSpawnCard(EAPSStartAssetSlot::Spaceship, LOCTEXT("Spaceship", "SPACESHIP"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f)[BuildSpawnCard(EAPSStartAssetSlot::SpaceStation, LOCTEXT("Station", "SPACE STATION"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f)[BuildSpawnCard(EAPSStartAssetSlot::Headquarters, LOCTEXT("HQ", "HEADQUARTERS"))]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f)[BuildSpawnCard(EAPSStartAssetSlot::Shipyard, LOCTEXT("Shipyard", "SHIPYARD"))]
		]
		+ SVerticalBox::Slot().FillHeight(0.28f).Padding(220.0f, 10.0f)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(APSMenu::Panel).Padding(24.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("SpawnPlace", "START LOCATION")).ColorAndOpacity(APSMenu::Cyan)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)[SNew(SSpinBox<int32>).MinValue(0).MaxValue(4).Value_Lambda([this]() { return ViewModel.IsValid() && ViewModel->SpawnParameters ? static_cast<int32>(ViewModel->SpawnParameters->CharacterSpawnPlace) : 0; }).OnValueChanged_Lambda([this](int32 V) { if (ViewModel.IsValid()) ViewModel->SetCharacterSpawnPlace(V); })]
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { return ViewModel.IsValid() && ViewModel->SpawnParameters ? StaticEnum<ECharSpawnPlace>()->GetDisplayNameTextByValue(static_cast<int64>(ViewModel->SpawnParameters->CharacterSpawnPlace)) : FText::GetEmpty(); }).ColorAndOpacity(APSMenu::White)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(12.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("OrbitHeight", "HOME ORBIT")).ColorAndOpacity(APSMenu::Cyan)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)[SNew(SSpinBox<int32>).MinValue(0).MaxValue(4).Value_Lambda([this]() { return ViewModel.IsValid() && ViewModel->SpawnParameters ? static_cast<int32>(ViewModel->SpawnParameters->HomeStationOrbitHeight) : 1; }).OnValueChanged_Lambda([this](int32 V) { if (ViewModel.IsValid()) ViewModel->SetStationOrbitHeight(V); })]
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { return ViewModel.IsValid() && ViewModel->SpawnParameters ? StaticEnum<EOrbitHeight>()->GetDisplayNameTextByValue(static_cast<int64>(ViewModel->SpawnParameters->HomeStationOrbitHeight)) : FText::GetEmpty(); }).ColorAndOpacity(APSMenu::White)]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 12.0f, 0.0f, 34.0f)
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
	auto Load = [this](FSlateBrush& Brush, const TCHAR* Path)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path);
		if (AMainMenuController* PC = Controller.Get()) PC->HoldSlateResource(Texture);
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(640.0f, 420.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
	};
	Load(SystemImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/T_System.T_System"));
	Load(PlanetImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/planet_realistic.planet_realistic"));
	Load(GalaxyImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/spiral_galaxy.spiral_galaxy"));
	Load(ClusterImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/I_Cluster.I_Cluster"));
	Load(CivilizationImage, TEXT("/Game/APS/APS_ALPHA/UI/Images/circular_galaxy.circular_galaxy"));
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

#include "SAPSBuildPalette.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APS_ALPHA/Gameplay/Construction/APSConstructionMode.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSBuildPalette"

namespace APSBuildPaletteLocal
{
	using namespace APSChrome;

	FLinearColor ErrorColour()
	{
		return FLinearColor(1.0f, 0.45f, 0.35f, 1.0f);
	}

	EAPSChromeGlyph PropGlyph(const FName Id)
	{
		static const TMap<FName, EAPSChromeGlyph> Glyphs = {
			{TEXT("CargoCrate"), EAPSChromeGlyph::Collection},
			{TEXT("CargoContainer"), EAPSChromeGlyph::Collection},
			{TEXT("WaterTank"), EAPSChromeGlyph::Planet},
			{TEXT("LandingPad"), EAPSChromeGlyph::Shipyard},
			{TEXT("SolarPanel"), EAPSChromeGlyph::System},
			{TEXT("LightMast"), EAPSChromeGlyph::Favorite},
			{TEXT("NavBeacon"), EAPSChromeGlyph::Compass},
			{TEXT("Barrier"), EAPSChromeGlyph::Lock},
			{TEXT("Terminal"), EAPSChromeGlyph::Settings},
			{TEXT("SignalTower"), EAPSChromeGlyph::Station}};
		const EAPSChromeGlyph* Glyph = Glyphs.Find(Id);
		return Glyph ? *Glyph : EAPSChromeGlyph::Infrastructure;
	}

	/**
	 * The menu's chamfered card (the colony terminal's ChromeButton): the frame lights up on hover, amber when selected,
	 * a dim red when the runtime refuses it. Never takes keyboard focus, so the keys keep going to the game.
	 */
	TSharedRef<SWidget> CardButton(const TSharedRef<SWidget>& Content, const FOnClicked& OnClicked,
		const TAttribute<bool>& IsSelected, const bool bDimmed, const float HorizontalPadding = 10.0f)
	{
		const TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(0.0f)
			.IsFocusable(false)
			.OnClicked(OnClicked);
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SAPSChamferedOverlay)
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedSurface)
				.Brush(FAppStyle::GetBrush("WhiteBrush"))
				.Tint_Lambda([IsSelected, bDimmed]()
				{
					if (IsSelected.Get(false))
					{
						return APSUITheme::RetintAction(FLinearColor(0.11f, 0.08f, 0.02f, 0.97f));
					}
					return bDimmed ? APSUITheme::Retint(FLinearColor(0.03f, 0.045f, 0.055f, 0.94f)) : Panel();
				})
				.ChamferTop(true)
				.ChamferBottom(true)
			]
			+ SOverlay::Slot().Padding(FMargin(HorizontalPadding, 7.0f))
			[
				Content
			]
			+ SOverlay::Slot()
			[
				SNew(SAPSChamferedFrame)
				.Thickness(1.0f)
				.Color_Lambda([WeakButton, IsSelected, bDimmed]()
				{
					if (IsSelected.Get(false))
					{
						return Amber();
					}
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					if (Pinned && Pinned->IsHovered())
					{
						return Cyan();
					}
					return bDimmed ? FLinearColor(0.42f, 0.14f, 0.09f, 0.85f) : CyanDim();
				})
			]);
		return Button;
	}

	/** A label alone (the section tabs): centred both ways by its capitals, 14 each side (Rio 03.10). */
	TSharedRef<SWidget> CardButton(const TSharedRef<STextBlock>& Label, const FOnClicked& OnClicked,
		const TAttribute<bool>& IsSelected, const bool bDimmed)
	{
		Label->SetJustification(ETextJustify::Center);
		Label->SetRenderTransform(CapsCenterShift(Label->GetFont()));
		return CardButton(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[Label], OnClicked, IsSelected, bDimmed, 14.0f);
	}

	TSharedRef<SWidget> Hint(const FText& Key, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				KeyChip(Key, Cyan())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Label).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
			];
	}
}

void SAPSBuildPalette::Construct(const FArguments& InArgs)
{
	using namespace APSBuildPaletteLocal;
	Mode = InArgs._Mode;
	const TWeakPtr<FAPSConstructionMode> WeakMode = Mode;
	// The ship's orbital build mode: the wheel floats the ghost nearer or further, R tilts it.
	const TSharedPtr<FAPSConstructionMode> BuiltFor = WeakMode.Pin();
	const bool bOrbit = BuiltFor.IsValid() && BuiltFor->IsOrbit();

	const auto SectionTab = [WeakMode](const APSConstruction::ESection Section, const FText& Label)
	{
		return CardButton(
			SNew(STextBlock).Text(Label).Font(Font("Bold", 9)).ColorAndOpacity(White()),
			FOnClicked::CreateLambda([WeakMode, Section]()
			{
				if (const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin())
				{
					Pinned->SetSection(Section);
				}
				return FReply::Handled();
			}),
			TAttribute<bool>::CreateLambda([WeakMode, Section]()
			{
				const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
				return Pinned.IsValid() && Pinned->GetSection() == Section;
			}),
			false);
	};

	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		// Above the walking HUD's corner (bottom left) and the interaction prompt (bottom centre).
		.Padding(FMargin(24.0f, 0.0f, 24.0f, 104.0f))
		[
			SNew(SBox)
			.MaxDesiredWidth(1260.0f)
			[
				SAssignNew(Bar, SBorder)
				.BorderImage(FAppStyle::GetBrush("NoBorder"))
				.Padding(0.0f)
				.Visibility(EVisibility::Visible)
				// Clicks on the bar's background are the palette's, never a placement in the world behind it.
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
				[
					ChamferPanel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								IconBadge(EAPSChromeGlyph::Infrastructure, Amber(), 32.0f)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock).Text(LOCTEXT("Title", "BUILD MODE")).Font(Font("Bold", 13)).ColorAndOpacity(White())
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
									.Text_Lambda([WeakMode]()
									{
										const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
										return Pinned.IsValid()
											? FText::Format(Pinned->IsOrbit() ? LOCTEXT("WhereOrbit", "IN ORBIT OF {0}  /  WITHIN THE RING ROUND THE SHIP")
												: LOCTEXT("Where", "ON {0}  /  INSIDE THE RING"), Pinned->GetSiteName())
											: FText::GetEmpty();
									})
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.0f, 0.0f, 0.0f, 0.0f)
							[
								SectionTab(APSConstruction::ESection::Props, LOCTEXT("Props", "PROPS"))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
							[
								SectionTab(APSConstruction::ESection::Infrastructure, LOCTEXT("Infrastructure", "INFRASTRUCTURE"))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
							[
								KeyChip(LOCTEXT("SectionKey", "C"), Cyan())
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).HAlign(HAlign_Right)
							.Padding(14.0f, 0.0f, 14.0f, 0.0f)
							[
								SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(Muted())
								.Justification(ETextJustify::Right).AutoWrapText(true)
								.Text_Lambda([WeakMode]()
								{
									const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
									return Pinned.IsValid() && Pinned->GetSection() == APSConstruction::ESection::Infrastructure
										? Pinned->GetStocks() : FText::GetEmpty();
								})
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								// Rio 02.10 ("think how to combine the two build modes"): the strategic construction is the
								// terminal's INFRASTRUCTURE; this leaves build mode and opens it there.
								CardButton(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(LOCTEXT("Strategic", "STRATEGIC BUILD")).Font(Font("Bold", 9))
										.ColorAndOpacity(White())
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										KeyChip(LOCTEXT("StrategicKey", "TAB"), Cyan())
									],
									FOnClicked::CreateLambda([WeakMode]()
									{
										const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
										UWorld* ModeWorld = Pinned.IsValid() ? Pinned->GetWorld() : nullptr;
										if (!Pinned.IsValid() || !ModeWorld)
										{
											return FReply::Handled();
										}
										Pinned->RequestExit();
										// After build mode has handed the camera and the input back, so the terminal keeps
										// its cursor.
										const TWeakObjectPtr<UWorld> WeakWorld = ModeWorld;
										FTimerHandle Handle;
										ModeWorld->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakWorld]()
										{
											if (UAPSColonyTerminalSubsystem* Terminal = WeakWorld.IsValid()
												? WeakWorld->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr)
											{
												Terminal->OpenTerminalTab(2);
											}
										}), 0.1f, false);
										return FReply::Handled();
									}),
									false, false, 14.0f)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								CardButton(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(LOCTEXT("Exit", "EXIT")).Font(Font("Bold", 9)).ColorAndOpacity(White())
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										KeyChip(LOCTEXT("ExitKey", "B"), Cyan())
									],
									FOnClicked::CreateLambda([WeakMode]()
									{
										if (const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin())
										{
											Pinned->RequestExit();
										}
										return FReply::Handled();
									}),
									false, false, 14.0f)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Font(Font("Bold", 9)).AutoWrapText(true)
							.Text_Lambda([WeakMode]()
							{
								const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
								return Pinned.IsValid() ? Pinned->GetStatus() : FText::GetEmpty();
							})
							.ColorAndOpacity_Lambda([WeakMode]()
							{
								const TSharedPtr<FAPSConstructionMode> Pinned = WeakMode.Pin();
								return FSlateColor(Pinned.IsValid() && Pinned->IsStatusError() ? ErrorColour() : White());
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
						[
							SAssignNew(Cards, SWrapBox)
							.UseAllottedSize(true)
							.InnerSlotPadding(FVector2D(6.0f, 6.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
						[
							SNew(SWrapBox)
							.UseAllottedSize(true)
							.InnerSlotPadding(FVector2D(16.0f, 5.0f))
							+ SWrapBox::Slot()[Hint(LOCTEXT("PlaceKey", "LMB"), LOCTEXT("PlaceLabel", "PLACE"))]
							+ SWrapBox::Slot()[Hint(LOCTEXT("RotateKey", "Q / E"), bOrbit ? LOCTEXT("RotateLabelOrbit", "TURN")
								: LOCTEXT("RotateLabel", "ROTATE (WHEEL TOO)"))]
							+ SWrapBox::Slot()[Hint(bOrbit ? LOCTEXT("TiltKey", "R / SHIFT+R") : LOCTEXT("CancelKey", "RMB"),
								bOrbit ? LOCTEXT("TiltLabel", "TILT") : LOCTEXT("CancelLabel", "CANCEL"))]
							+ SWrapBox::Slot()[Hint(bOrbit ? LOCTEXT("DistanceKey", "WHEEL") : LOCTEXT("CameraKey", "RMB / MMB DRAG"),
								bOrbit ? LOCTEXT("DistanceLabel", "NEARER / FURTHER") : LOCTEXT("CameraLabel", "CAMERA"))]
							+ SWrapBox::Slot()[Hint(LOCTEXT("PickKey", "1-9, 0"), LOCTEXT("PickLabel", "PICK"))]
							+ SWrapBox::Slot()[Hint(LOCTEXT("RemoveKey", "X"), LOCTEXT("RemoveLabel", "REMOVE A PROP"))]
							+ SWrapBox::Slot()[Hint(LOCTEXT("TerminalKey", "TAB"), LOCTEXT("TerminalLabel", "TERMINAL: STRATEGIC BUILD"))]
							+ SWrapBox::Slot()[Hint(LOCTEXT("LeaveKey", "B / ESC"), LOCTEXT("LeaveLabel", "EXIT"))]
						],
						FMargin(16.0f, 12.0f), CyanDim())
				]
			]
		]
	];
	RebuildCards();
}

void SAPSBuildPalette::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const TSharedPtr<FAPSConstructionMode> Pinned = Mode.Pin();
	if (Pinned.IsValid() && (Pinned->GetEntriesRevision() != BuiltRevision
		|| static_cast<int32>(Pinned->GetSection()) != BuiltSection))
	{
		RebuildCards();
	}
}

bool SAPSBuildPalette::IsPointerOverBar() const
{
	return Bar.IsValid() && Bar->IsHovered();
}

void SAPSBuildPalette::RebuildCards()
{
	using namespace APSBuildPaletteLocal;
	const TSharedPtr<FAPSConstructionMode> Pinned = Mode.Pin();
	if (!Cards.IsValid() || !Pinned.IsValid())
	{
		return;
	}
	Cards->ClearChildren();
	const APSConstruction::ESection Section = Pinned->GetSection();
	BuiltSection = static_cast<int32>(Section);
	BuiltRevision = Pinned->GetEntriesRevision();
	const TArray<APSConstruction::FEntry>& Entries = Pinned->GetEntries(Section);
	if (Entries.IsEmpty())
	{
		Cards->AddSlot()
		[
			SNew(STextBlock).Font(Font("Regular", 11)).ColorAndOpacity(Muted())
			.Text(Section == APSConstruction::ESection::Infrastructure
				? LOCTEXT("NoStructures", "No structure of the catalogue is raised on this ground (TAB: the terminal for the rest).")
				: LOCTEXT("NoProps", "No props in the catalogue."))
		];
		return;
	}
	const TWeakPtr<FAPSConstructionMode> WeakMode = Mode;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const APSConstruction::FEntry& Entry = Entries[Index];
		const bool bStructure = Entry.Section == APSConstruction::ESection::Infrastructure;
		const bool bRefused = !Entry.Refusal.IsEmpty();
		const FText Key = Index < 9 ? FText::FromString(FString::FromInt(Index + 1))
			: Index == 9 ? FText::FromString(TEXT("0")) : FText::GetEmpty();
		const FLinearColor Accent = bStructure
			? APSInfrastructure::DepartmentColour(APSInfrastructure::Find(Entry.Id)
				? APSInfrastructure::Find(Entry.Id)->Department : APSInfrastructure::EDepartment::Industry)
			: Cyan();
		const FName Id = Entry.Id;
		const APSConstruction::ESection EntrySection = Entry.Section;
		const TSharedRef<SWidget> Content =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					IconBadge(bStructure ? EAPSChromeGlyph::Infrastructure : PropGlyph(Entry.Id),
						bRefused ? Muted() : Accent, 26.0f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Top)
				[
					SNew(SBox).Visibility(Key.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)
					[
						KeyChip(Key, Cyan())
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Entry.Name).Font(Font("Bold", 9)).AutoWrapText(true)
				.ColorAndOpacity(bRefused ? Muted() : White())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Entry.Detail).Font(Font("Regular", 9)).AutoWrapText(true)
				.ColorAndOpacity(bStructure ? Amber() : Muted())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Entry.Refusal).Font(Font("Regular", 9)).AutoWrapText(true)
				.ColorAndOpacity(ErrorColour())
				.Visibility(bRefused ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			];
		Cards->AddSlot()
		[
			SNew(SBox)
			// Ten prop cards fit one row from 1280 px wide; structure cards are wider for their cost and refusal.
			.WidthOverride(bStructure ? 176.0f : 112.0f)
			.MinDesiredHeight(bStructure ? 120.0f : 104.0f)
			[
				CardButton(Content,
					FOnClicked::CreateLambda([WeakMode, EntrySection, Id]()
					{
						if (const TSharedPtr<FAPSConstructionMode> Owner = WeakMode.Pin())
						{
							Owner->Select(EntrySection, Id);
						}
						return FReply::Handled();
					}),
					TAttribute<bool>::CreateLambda([WeakMode, EntrySection, Id]()
					{
						const TSharedPtr<FAPSConstructionMode> Owner = WeakMode.Pin();
						return Owner.IsValid() && Owner->GetSelectedId() == Id && Owner->GetSelectedSection() == EntrySection;
					}),
					bRefused)
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE

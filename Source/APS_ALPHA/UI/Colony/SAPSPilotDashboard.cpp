#include "SAPSPilotDashboard.h"

#include "APSDashboardKit.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncients.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncientsTypes.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyOnboardingSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSPilotDashboard"

namespace APSPilotDashboardPrivate
{
	using namespace APSChrome;
	using namespace APSDashboard;

	/** Shown at most in AROUND YOU: the nearest first. */
	constexpr int32 MaxNearby = 7;
	constexpr int32 MaxJournal = 3;
	/** Standard gravity, cm/s2. */
	constexpr double OneG = 980.665;
	const FLinearColor AncientColour(0.78f, 0.62f, 1.0f, 1.0f);

	FSlateFontInfo TitleFont() { return FCoreStyle::GetDefaultFontStyle("Bold", 13); }
	FSlateFontInfo BodyFont() { return FCoreStyle::GetDefaultFontStyle("Regular", 11); }

	FText DistanceText(const double Cm)
	{
		return FText::FromString(UShipNavigationComponent::FormatDistance(FMath::Max(Cm, 0.0)));
	}

	FText Designated(const AActor* Actor, const FText& Name)
	{
		const FString Designation = APSBodyDesignation::Of(Actor);
		return Designation.IsEmpty() ? Name : FText::FromString(Designation + TEXT("  ") + Name.ToString());
	}

	FText BodyTypeText(const APlanetaryBody* Body)
	{
		FString Type = UEnum::GetDisplayValueAsText(Body->PlanetType).ToString().ToUpper();
		Type.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Type);
	}

	/** The catalogue writes mission titles in capitals; read them in sentence case (as the HUD's mission card does). */
	FText SentenceCase(const FText& Text)
	{
		const FString Source = Text.ToString();
		if (Source.IsEmpty() || !Source.Equals(Text.ToUpper().ToString(), ESearchCase::CaseSensitive))
		{
			return Text;
		}
		const FString Lower = Text.ToLower().ToString();
		return FText::AsCultureInvariant(Lower.Left(1).ToUpper() + Lower.Mid(1));
	}

	/** The journal tab's category colours, so an event reads the same here. */
	FLinearColor JournalColour(const FName Category)
	{
		if (Category == TEXT("Objective")) return Amber();
		if (Category == TEXT("Fleet")) return Cyan();
		if (Category == TEXT("Colony")) return FLinearColor(0.36f, 1.0f, 0.58f);
		if (Category == TEXT("Build")) return FLinearColor(1.0f, 0.62f, 0.32f);
		if (Category == TEXT("Flight") || Category == TEXT("Navigation")) return FLinearColor(0.56f, 0.78f, 1.0f);
		if (Category == TEXT("Ancients")) return AncientColour;
		if (Category == TEXT("Start")) return White();
		return Muted();
	}

	FText Shortened(const FText& Text, const int32 MaxChars)
	{
		const FString Source = Text.ToString();
		return Source.Len() <= MaxChars ? Text : FText::FromString(Source.Left(MaxChars - 1).TrimEnd() + TEXT("…"));
	}

	FText Ago(const double Seconds)
	{
		if (Seconds < 60.0)
		{
			return LOCTEXT("AgoNow", "just now");
		}
		if (Seconds < 3600.0)
		{
			return FText::Format(LOCTEXT("AgoMinutes", "{0} min ago"), APSUINumber::Number(FMath::FloorToInt(Seconds / 60.0)));
		}
		return FText::Format(LOCTEXT("AgoHours", "{0} h ago"), APSUINumber::Number(FMath::FloorToInt(Seconds / 3600.0)));
	}

	/** A caption and a value under it, the value in the display face. */
	TSharedRef<SWidget> Fact(const FText& Label, const TAttribute<FText>& Value, const TAttribute<FSlateColor>& Colour,
		const int32 Size)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Caption(Label)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Text(Value).Font(Font("Bold", Size)).ColorAndOpacity(Colour)
			];
	}
}

void SAPSPilotDashboard::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	CourseShip = InArgs._CourseShip;
	ColonyActor = InArgs._ColonyActor;
	OnOpenTab = InArgs._OnOpenTab;
	ReadState();

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 20.0f)
		[
			BuildHero()
		]
		+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 16.0f)
		[
			BuildHeadlines()
		]
		+ SScrollBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.1f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				BuildWhere()
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
			[
				BuildAround()
			]
			+ SHorizontalBox::Slot().FillWidth(1.1f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				BuildNow()
			]
		]
	];
	RebuildNearby();
	RebuildJournal();
}

void SAPSPilotDashboard::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// The terminal's switcher ticks only the shown tab, so nothing is read while another tab is open.
	ReadAccumulator += InDeltaTime;
	if (ReadAccumulator >= 0.25f)
	{
		ReadAccumulator = 0.0f;
		ReadState();
		RebuildNearby();
		RebuildJournal();
	}
}

FReply SAPSPilotDashboard::OpenTab(const int32 Tab)
{
	OnOpenTab.ExecuteIfBound(Tab);
	return FReply::Handled();
}

void SAPSPilotDashboard::ReadState()
{
	using namespace APSPilotDashboardPrivate;
	FState Next;
	UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		Next.Title = LOCTEXT("NoPilot", "NO PILOT");
		Next.Environment = LOCTEXT("NoPilotLine", "No pilot in the world");
		State = MoveTemp(Next);
		return;
	}
	Next.bPawn = true;
	const FVector Location = Pawn->GetActorLocation();
	ASpaceship* Ship = Cast<ASpaceship>(Pawn);
	const ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pawn);
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);

	// Who and how.
	if (Ship)
	{
		const FAPSFleetUnit* Unit = Fleet ? Fleet->FindUnit(Ship) : nullptr;
		Next.Title = FText::Format(LOCTEXT("Piloting", "PILOTING {0}"),
			FText::FromString((Unit && !Unit->CallSign.IsEmpty() ? Unit->CallSign : Ship->GetName()).ToUpper()));
		Next.Environment = FText::FromString(Ship->GetFlightEnvironmentName().ToUpper());
		if (Ship->ActiveGravityAcceleration > 1.0)
		{
			Next.GravityCm = Ship->ActiveGravityAcceleration;
			Next.GravitySource = FText::FromString(Ship->GetGravitySourceName());
		}
		if (Ship->FlightModel)
		{
			Next.Mode = FText::FromString(Ship->FlightModel->GetBandSettings(Ship->FlightModel->GetFlightBand()).Name);
		}
	}
	else if (Character)
	{
		Next.Title = Character->bIsZeroG ? LOCTEXT("Weightless", "WEIGHTLESS") : LOCTEXT("OnFoot", "ON FOOT");
		Next.Environment = Character->CurrentGravityType == EGravityType::OnPlanet ? LOCTEXT("OnWorld", "ON A WORLD")
			: Character->CurrentGravityType == EGravityType::OnShip ? LOCTEXT("AboardShip", "ABOARD A SHIP")
			: Character->CurrentGravityType == EGravityType::OnStation ? LOCTEXT("OnStation", "ON A STATION")
			: LOCTEXT("ZeroG", "ZERO-G");
		const double GravityZ = Character->GetCharacterMovement() ? FMath::Abs(Character->GetCharacterMovement()->GetGravityZ()) : 0.0;
		if (!Character->bIsZeroG && GravityZ >= 1.0)
		{
			Next.GravityCm = GravityZ;
			Next.GravitySource = Character->GravityTarget ? FAPSFleetCommand::DisplayName(Character->GravityTarget) : FText::GetEmpty();
		}
		Next.Mode = LOCTEXT("Walking", "Walking");
	}
	else
	{
		Next.Title = FText::FromString(Pawn->GetName().ToUpper());
	}
	Next.SpeedCm = Pawn->GetVelocity().Size();

	// The nearest world: its surface distance is the altitude.
	const APlanetaryBody* Nearest = nullptr;
	double NearestSurface = TNumericLimits<double>::Max();
	if (LiveWorld)
	{
		for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			const double Surface = FVector::Dist(It->GetActorLocation(), Location) - It->GetWorldScapeBodyRadiusCm();
			if (Surface < NearestSurface)
			{
				NearestSurface = Surface;
				Nearest = *It;
			}
		}
	}
	if (Nearest)
	{
		Next.bBody = true;
		Next.BodyName = Designated(Nearest, FText::FromString(Nearest->AstroName.ToString().ToUpper()));
		Next.BodyColour = BodyColour(Nearest);
		Next.AltitudeCm = FMath::Max(NearestSurface, 0.0);
		const AMoon* Moon = Cast<AMoon>(Nearest);
		const APlanet* Planet = Moon ? Moon->ParentPlanet : Cast<APlanet>(Nearest);
		Next.Moons = Moon ? 0 : Planet ? Planet->Moons.Num() : 0;
		Next.BodyDetail = Moon && Planet
			? FText::Format(LOCTEXT("MoonDetail", "{0}  ·  radius {1} km  ·  moon of {2}"), BodyTypeText(Nearest),
				APSUINumber::Number(Nearest->PlanetRadiusKM), FText::FromString(Planet->AstroName.ToString().ToUpper()))
			: FText::Format(LOCTEXT("WorldDetail", "{0}  ·  radius {1} km"), BodyTypeText(Nearest),
				APSUINumber::Number(Nearest->PlanetRadiusKM));
		const double BodyRadius = Nearest->GetWorldScapeBodyRadiusCm();
		const FVector Up = (Location - Nearest->GetActorLocation()).GetSafeNormal();
		Next.bNear = BodyRadius > 0.0 && Next.AltitudeCm < BodyRadius;
		if (Next.bNear)
		{
			// The body-local direction, as the ancient sites and their quests write coordinates.
			Next.Coordinates = APSAncients::WhereText(Nearest->GetActorTransform().InverseTransformVectorNoScale(Up));
		}
		if (const AStar* Star = Planet ? Planet->ParentStar : nullptr)
		{
			Next.StarName = Designated(Star, FText::FromString(Star->AstroName.ToString().ToUpper()));
			Next.StarDetail = Star->SurfaceTemperature > 0
				? FText::Format(LOCTEXT("StarDetail", "Class {0}  ·  {1} K"), FText::FromName(Star->FullSpectralName),
					APSUINumber::Number(Star->SurfaceTemperature))
				: FText::Format(LOCTEXT("StarDetailClass", "Class {0}"), FText::FromName(Star->FullSpectralName));
			Next.StarColour = StarColour(Star);
			const FVector ToStar = Star->GetActorLocation() - Location;
			Next.StarDistanceCm = ToStar.Size();
			Next.SunElevation = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(Up, ToStar.GetSafeNormal()), -1.0, 1.0)));
		}
	}

	// The course of the piloted ship, or of the home ship while on foot.
	if (const ASpaceship* Coursing = CourseShip ? CourseShip() : nullptr; Coursing && Coursing->ShipNavigation)
	{
		if (const FShipNavigationContact* Contact = Coursing->ShipNavigation->GetSelectedContact(); Contact && Contact->Actor.IsValid())
		{
			const double CourseCm = FVector::Dist(Contact->Actor->GetActorLocation(), Location);
			Next.bCourse = true;
			Next.Course = Designated(Contact->Actor.Get(), FAPSFleetCommand::DisplayName(Contact->Actor.Get()));
			const double Seconds = Next.SpeedCm > 100.0 ? CourseCm / Next.SpeedCm : -1.0;
			const FText Eta = Seconds < 0.0 ? LOCTEXT("EtaStopped", "not moving")
				: Seconds < 120.0 ? FText::Format(LOCTEXT("EtaSeconds", "{0} s at this speed"), APSUINumber::Number(FMath::RoundToInt(Seconds)))
				: Seconds < 7200.0 ? FText::Format(LOCTEXT("EtaMinutes", "{0} min at this speed"), APSUINumber::Number(FMath::RoundToInt(Seconds / 60.0)))
				: LOCTEXT("EtaLong", "hours at this speed");
			Next.CourseDetail = FText::Format(LOCTEXT("CourseDetail", "{0}  ·  {1}"), DistanceText(CourseCm), Eta);
		}
	}
	if (!Next.bCourse)
	{
		Next.Course = LOCTEXT("NoCourse", "No course");
		Next.CourseDetail = LOCTEXT("NoCourseDetail", "Pick a target on the system map");
	}

	// What to do: the onboarding objective and the tracked mission.
	if (const UAPSColonyOnboardingSubsystem* Onboarding = LiveWorld ? LiveWorld->GetSubsystem<UAPSColonyOnboardingSubsystem>() : nullptr)
	{
		Onboarding->GetObjective(Next.ObjectiveTitle, Next.ObjectiveBody);
	}
	const FAPSMissionBoard* Board = LiveWorld ? APSMissionsFind(LiveWorld) : nullptr;
	if (const FAPSMission* Mission = Board ? Board->GetTracked() : nullptr; Mission && Mission->State == APSMissions::EState::Active)
	{
		Next.bMission = true;
		Next.MissionColour = APSInfrastructure::DepartmentColour(Mission->Department);
		Next.MissionLabel = FText::Format(LOCTEXT("MissionLabel", "{0} MISSION"), APSInfrastructure::DepartmentName(Mission->Department));
		Next.MissionTitle = SentenceCase(Mission->Title);
		Next.MissionStep = SentenceCase(APSMissions::ObjectiveName(Mission->Objective));
		Next.MissionSubject = Mission->SubjectName;
		Next.MissionCount = FText::Format(LOCTEXT("MissionCount", "{0} / {1}"), APSUINumber::Number(Mission->Progress),
			APSUINumber::Number(Mission->Count));
	}

	// Around: the colony, the fleet's ships and structures, the other worlds and the ancient sites already found.
	TSet<const AActor*> Seen;
	Seen.Add(Pawn);
	if (Nearest)
	{
		Seen.Add(Nearest);
	}
	const auto Add = [&Next, &Seen, &Location](const AActor* Actor, const EAPSChromeGlyph Glyph, const FLinearColor& Colour,
		const FText& Name, const FText& Detail)
	{
		if (!IsValid(Actor) || Seen.Contains(Actor))
		{
			return;
		}
		Seen.Add(Actor);
		FNearby& Entry = Next.Nearby.AddDefaulted_GetRef();
		Entry.Glyph = Glyph;
		Entry.Colour = Colour;
		Entry.Name = Name;
		Entry.Detail = Detail;
		Entry.DistanceCm = FVector::Dist(Actor->GetActorLocation(), Location);
	};
	if (ColonyActor)
	{
		Add(ColonyActor(0), EAPSChromeGlyph::Headquarters, Amber(), LOCTEXT("ColonyBase", "COLONY BASE"), LOCTEXT("ColonyBaseDetail", "The colony on the ground"));
		Add(ColonyActor(1), EAPSChromeGlyph::Infrastructure, Amber(), LOCTEXT("LandingPad", "LANDING PAD"), LOCTEXT("LandingPadDetail", "The colony's pad"));
	}
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			const ASpaceship* UnitShip = Unit.Ship.Get();
			if (!UnitShip)
			{
				continue;
			}
			const FText Detail = Unit.Order != APSFleet::EOrder::None
				? FText::Format(LOCTEXT("ShipBusy", "{0}  ·  {1}"), APSFleet::DivisionName(Unit.Division), APSFleet::OrderName(Unit.Order))
				: FText::Format(LOCTEXT("ShipIdle", "{0}  ·  standing by"), APSFleet::DivisionName(Unit.Division));
			Add(UnitShip, EAPSChromeGlyph::Ship, APSFleet::DivisionColour(Unit.Division),
				FText::FromString((Unit.CallSign.IsEmpty() ? UnitShip->GetName() : Unit.CallSign).ToUpper()), Detail);
		}
		for (const FAPSFleetStructure& Structure : Fleet->GetStructures())
		{
			const AActor* Actor = Structure.Actor.Get();
			if (!Actor)
			{
				continue;
			}
			const EAPSChromeGlyph Glyph = Structure.Kind == APSFleet::EStructure::Shipyard ? EAPSChromeGlyph::Shipyard
				: Structure.Kind == APSFleet::EStructure::Headquarters ? EAPSChromeGlyph::Headquarters : EAPSChromeGlyph::Station;
			const FText Kind = Structure.Kind == APSFleet::EStructure::Shipyard ? LOCTEXT("KindShipyard", "Shipyard")
				: Structure.Kind == APSFleet::EStructure::Headquarters ? LOCTEXT("KindHeadquarters", "Headquarters")
				: LOCTEXT("KindStation", "Station");
			Add(Actor, Glyph, Cyan(), FText::FromString(FAPSFleetCommand::DisplayName(Actor).ToString().ToUpper()), Kind);
		}
	}
	if (ColonyActor)
	{
		Add(ColonyActor(2), EAPSChromeGlyph::Ship, Amber(), LOCTEXT("HomeShip", "HOME SHIP"), LOCTEXT("HomeShipDetail", "The colony's ship"));
	}
	if (const FAPSAncients* Ancients = LiveWorld ? APSAncientsFind(LiveWorld) : nullptr)
	{
		for (const FAPSAncients::FSite& Site : Ancients->GetSites())
		{
			// Only sites whose quest began: the dashboard must not give away what the player has not found.
			FVector Centre;
			if (Site.Step < 0 || Site.Stage != FAPSAncients::FSite::EStage::Built || !Ancients->GetCentre(Site, Centre)
				|| !Site.Actor.IsValid())
			{
				continue;
			}
			if (Seen.Contains(Site.Actor.Get()))
			{
				continue;
			}
			Seen.Add(Site.Actor.Get());
			FNearby& Entry = Next.Nearby.AddDefaulted_GetRef();
			Entry.Glyph = EAPSChromeGlyph::Favorite;
			Entry.Colour = AncientColour;
			Entry.Name = APSAncients::KindName(Site.Spec.Kind).ToUpper();
			Entry.Detail = FText::Format(LOCTEXT("AncientDetail", "Ancient site  ·  {0}"), Site.BodyName);
			Entry.DistanceCm = FVector::Dist(Centre, Location);
		}
	}
	if (LiveWorld)
	{
		for (TActorIterator<APlanetaryBody> It(LiveWorld); It; ++It)
		{
			const APlanetaryBody* Body = *It;
			Add(Body, EAPSChromeGlyph::Planet, BodyColour(Body),
				Designated(Body, FText::FromString(Body->AstroName.ToString().ToUpper())),
				Body->IsA<AMoon>() ? FText::Format(LOCTEXT("MoonKind", "Moon  ·  {0}"), BodyTypeText(Body)) : BodyTypeText(Body));
		}
	}
	Next.Nearby.Sort([](const FNearby& A, const FNearby& B) { return A.DistanceCm < B.DistanceCm; });
	if (Next.Nearby.Num() > MaxNearby)
	{
		Next.Nearby.SetNum(MaxNearby);
	}

	// The latest events, newest first.
	if (const UAPSCivilizationJournalSubsystem* Journal = LiveWorld ? LiveWorld->GetSubsystem<UAPSCivilizationJournalSubsystem>() : nullptr)
	{
		const TArray<FAPSCivilizationJournalEntry>& Entries = Journal->GetEntries();
		const double Now = LiveWorld->GetTimeSeconds();
		for (int32 Index = Entries.Num() - 1; Index >= 0 && Next.Journal.Num() < MaxJournal; --Index)
		{
			FJournalLine& Line = Next.Journal.AddDefaulted_GetRef();
			Line.Colour = JournalColour(Entries[Index].Category);
			Line.Category = FText::Format(LOCTEXT("JournalCategory", "{0}  ·  {1}"),
				FText::FromString(Entries[Index].Category.ToString().ToUpper()), Ago(FMath::Max(0.0, Now - Entries[Index].WorldSeconds)));
			Line.Text = Shortened(Entries[Index].Text, 150);
		}
	}
	State = MoveTemp(Next);
}

void SAPSPilotDashboard::RebuildNearby()
{
	using namespace APSPilotDashboardPrivate;
	if (!NearbyBox.IsValid())
	{
		return;
	}
	FString Signature;
	for (const FNearby& Entry : State.Nearby)
	{
		Signature += Entry.Name.ToString() + TEXT("|") + Entry.Detail.ToString() + TEXT(";");
	}
	if (Signature == NearbySignature)
	{
		return;
	}
	NearbySignature = Signature;
	NearbyBox->ClearChildren();
	if (State.Nearby.IsEmpty())
	{
		NearbyBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NothingNear", "Nothing charted nearby.")).Font(BodyFont()).ColorAndOpacity(Muted())
		];
		return;
	}
	for (int32 Index = 0; Index < State.Nearby.Num(); ++Index)
	{
		const FNearby& Entry = State.Nearby[Index];
		NearbyBox->AddSlot().AutoHeight().Padding(0.0f, Index == 0 ? 0.0f : 10.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(24.0f).HeightOverride(24.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(Entry.Glyph).Color(Entry.Colour).StrokeWidth(1.5f)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(12.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Entry.Name).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(White())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Entry.Detail).Font(BodyFont()).ColorAndOpacity(Muted())
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(Cyan())
				.Text_Lambda([this, Index]()
				{
					return State.Nearby.IsValidIndex(Index) ? DistanceText(State.Nearby[Index].DistanceCm) : FText::GetEmpty();
				})
			]
		];
	}
}

void SAPSPilotDashboard::RebuildJournal()
{
	using namespace APSPilotDashboardPrivate;
	if (!JournalBox.IsValid())
	{
		return;
	}
	FString Signature;
	for (const FJournalLine& Line : State.Journal)
	{
		Signature += Line.Category.ToString() + Line.Text.ToString();
	}
	if (Signature == JournalSignature)
	{
		return;
	}
	JournalSignature = Signature;
	JournalBox->ClearChildren();
	if (State.Journal.IsEmpty())
	{
		JournalBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("JournalEmpty", "Nothing has happened yet.")).Font(BodyFont()).ColorAndOpacity(Muted())
		];
		return;
	}
	for (int32 Index = 0; Index < State.Journal.Num(); ++Index)
	{
		const FJournalLine& Line = State.Journal[Index];
		JournalBox->AddSlot().AutoHeight().Padding(0.0f, Index == 0 ? 0.0f : 9.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 3.0f, 10.0f, 0.0f)
			[
				Swatch(Line.Colour, 8.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(Line.Category).Font(CaptionFont()).ColorAndOpacity(Line.Colour)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(Line.Text).Font(BodyFont()).ColorAndOpacity(White()).AutoWrapText(true)
				]
			]
		];
	}
}

TSharedRef<SWidget> SAPSPilotDashboard::BuildHero()
{
	using namespace APSPilotDashboardPrivate;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			IconBadge(EAPSChromeGlyph::Pilot, Cyan(), 60.0f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Font(Font("Bold", 26)).ColorAndOpacity(White())
				.Text_Lambda([this]() { return State.Title; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(Font("Regular", 12)).ColorAndOpacity(Muted())
				.Text_Lambda([this]()
				{
					if (!State.bBody)
					{
						return State.Environment;
					}
					FText Line = FText::Format(LOCTEXT("HeroAt", "{0}  ·  {1}"), State.Environment, State.BodyName);
					if (!State.StarName.IsEmpty())
					{
						Line = FText::Format(LOCTEXT("HeroSystem", "{0}, {1} system"), Line, State.StarName);
					}
					return State.bNear && !State.Coordinates.IsEmpty()
						? FText::Format(LOCTEXT("HeroCoordinates", "{0}  ·  {1}"), Line, State.Coordinates) : Line;
				})
			]
		];
}

TSharedRef<SWidget> SAPSPilotDashboard::BuildHeadlines()
{
	using namespace APSPilotDashboardPrivate;
	const auto Under = [](TFunction<FText()> Text)
	{
		return SNew(STextBlock).Font(CaptionFont()).ColorAndOpacity(Muted())
			.Text_Lambda([Getter = MoveTemp(Text)]() { return Getter(); });
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::World, Cyan(), LOCTEXT("Altitude", "ALTITUDE"),
				TAttribute<FText>::CreateLambda([this]() { return State.bBody ? DistanceText(State.AltitudeCm) : LOCTEXT("NoAltitude", "-"); }),
				FText::GetEmpty(),
				Under([this]()
				{
					return State.bBody ? FText::Format(LOCTEXT("AltitudeOver", "OVER {0}"), State.BodyName) : FText::GetEmpty();
				}),
				LOCTEXT("AltitudeHint", "Height over the ground of the nearest world."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Ship, Cyan(), LOCTEXT("Speed", "SPEED"),
				TAttribute<FText>::CreateLambda([this]()
				{
					return State.SpeedCm >= 100000.0
						? FText::Format(LOCTEXT("SpeedKm", "{0} km/s"),
							APSUINumber::Number(State.SpeedCm / 100000.0, &FNumberFormattingOptions().SetMaximumFractionalDigits(1)))
						: FText::Format(LOCTEXT("SpeedM", "{0} m/s"), APSUINumber::Number(FMath::RoundToInt(State.SpeedCm / 100.0)));
				}),
				FText::GetEmpty(),
				Under([this]() { return State.Mode.ToUpper(); }),
				LOCTEXT("SpeedHint", "Speed and the flight mode (walking on foot)."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Space, Amber(), LOCTEXT("Gravity", "GRAVITY"),
				TAttribute<FText>::CreateLambda([this]()
				{
					return State.GravityCm > 0.0
						? APSUINumber::Number(State.GravityCm / OneG, &FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2))
						: LOCTEXT("NoGravity", "0");
				}),
				LOCTEXT("GravityUnit", "g"),
				Under([this]()
				{
					if (State.GravityCm <= 0.0)
					{
						return LOCTEXT("GravityNone", "WEIGHTLESS");
					}
					const FText Metric = FText::Format(LOCTEXT("GravityMetric", "{0} m/s²"),
						APSUINumber::Number(State.GravityCm / 100.0, &FNumberFormattingOptions().SetMaximumFractionalDigits(1)));
					return State.GravitySource.IsEmpty() ? Metric
						: FText::Format(LOCTEXT("GravityFrom", "{0}  ·  FROM {1}"), Metric, State.GravitySource.ToUpper());
				}),
				LOCTEXT("GravityHint", "The pull on the pilot, in Earth gravities, and what pulls."))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			Headline(EAPSChromeGlyph::Favorite, Amber(), LOCTEXT("Sky", "SKY"),
				TAttribute<FText>::CreateLambda([this]()
				{
					if (!State.bNear || State.StarName.IsEmpty())
					{
						return LOCTEXT("SkySpace", "SPACE");
					}
					return State.SunElevation > 6.0 ? LOCTEXT("SkyDay", "DAY")
						: State.SunElevation > -6.0 ? LOCTEXT("SkyTwilight", "TWILIGHT") : LOCTEXT("SkyNight", "NIGHT");
				}),
				FText::GetEmpty(),
				Under([this]()
				{
					if (State.StarName.IsEmpty())
					{
						return FText::GetEmpty();
					}
					if (!State.bNear)
					{
						return FText::Format(LOCTEXT("StarAway", "{0}  ·  {1} AWAY"), State.StarName, DistanceText(State.StarDistanceCm));
					}
					const int32 Degrees = FMath::RoundToInt(FMath::Abs(State.SunElevation));
					return State.SunElevation >= 0.0
						? FText::Format(LOCTEXT("SunUp", "STAR {0}° ABOVE THE HORIZON"), APSUINumber::Number(Degrees))
						: FText::Format(LOCTEXT("SunDown", "STAR {0}° BELOW THE HORIZON"), APSUINumber::Number(Degrees));
				}),
				LOCTEXT("SkyHint", "Day or night where the pilot stands: the star's height over the horizon."))
		];
}

TSharedRef<SWidget> SAPSPilotDashboard::BuildWhere()
{
	using namespace APSPilotDashboardPrivate;
	const auto Coloured = [this](FLinearColor FState::* Field)
	{
		return TAttribute<FSlateColor>::CreateLambda([this, Field]() { return FSlateColor(State.*Field); });
	};
	const TSharedRef<SWidget> Body =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(170.0f).HeightOverride(150.0f)
			[
				SNew(SAPSDashboardOrbit)
				.StarColour_Lambda([this]() { return State.StarColour; })
				.BodyColour_Lambda([this]() { return State.BodyColour; })
				.Moons_Lambda([this]() { return State.Moons; })
				.HasBody_Lambda([this]() { return State.bBody; })
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(18.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Fact(LOCTEXT("NearestWorld", "NEAREST WORLD"),
					TAttribute<FText>::CreateLambda([this]() { return State.bBody ? State.BodyName : LOCTEXT("NoWorld", "NONE"); }),
					Coloured(&FState::BodyColour), 16)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted()).AutoWrapText(true)
				.Text_Lambda([this]() { return State.BodyDetail; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				Fact(LOCTEXT("StarCaption", "STAR"), TAttribute<FText>::CreateLambda([this]() { return State.StarName; }),
					Coloured(&FState::StarColour), 13)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted())
				.Text_Lambda([this]() { return State.StarDetail; })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() { return State.bNear ? EVisibility::Visible : EVisibility::Collapsed; })
				[
					Fact(LOCTEXT("Coordinates", "COORDINATES"), TAttribute<FText>::CreateLambda([this]() { return State.Coordinates; }),
						Cyan(), 11)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				Fact(LOCTEXT("Environment", "ENVIRONMENT"), TAttribute<FText>::CreateLambda([this]() { return State.Environment; }),
					Amber(), 11)
			]
		];
	return Card(EAPSChromeGlyph::Planet, LOCTEXT("WhereTitle", "WHERE YOU ARE"),
		LOCTEXT("WhereSubtitle", "The nearest world and its star"), Body,
		LinkButton(LOCTEXT("OpenSurface", "SURFACE MAP"), FOnClicked::CreateSP(this, &SAPSPilotDashboard::OpenTab, 9)));
}

TSharedRef<SWidget> SAPSPilotDashboard::BuildAround()
{
	using namespace APSPilotDashboardPrivate;
	return Card(EAPSChromeGlyph::Compass, LOCTEXT("AroundTitle", "AROUND YOU"),
		LOCTEXT("AroundSubtitle", "Ships, stations, the colony and worlds, nearest first"),
		SAssignNew(NearbyBox, SVerticalBox),
		LinkButton(LOCTEXT("OpenMap", "SYSTEM MAP"), FOnClicked::CreateSP(this, &SAPSPilotDashboard::OpenTab, 1)));
}

TSharedRef<SWidget> SAPSPilotDashboard::BuildNow()
{
	using namespace APSPilotDashboardPrivate;
	const auto Collapsed = [](TFunction<bool()> bShown)
	{
		return TAttribute<EVisibility>::CreateLambda([Shown = MoveTemp(bShown)]()
		{
			return Shown() ? EVisibility::Visible : EVisibility::Collapsed;
		});
	};
	const TSharedRef<SWidget> Body =
		SNew(SVerticalBox)
		// The course.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.0f, 1.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(22.0f).HeightOverride(22.0f)
				[
					SNew(SAPSVectorGlyph).Glyph(EAPSChromeGlyph::Compass).Color(Amber()).StrokeWidth(1.5f)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Caption(LOCTEXT("CourseCaption", "COURSE"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(TitleFont()).AutoWrapText(true)
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(State.bCourse ? Amber() : Muted()); })
					.Text_Lambda([this]() { return State.Course; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted()).AutoWrapText(true)
					.Text_Lambda([this]() { return State.CourseDetail; })
				]
			]
		]
		// The onboarding objective.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			.Visibility(Collapsed([this]() { return !State.ObjectiveTitle.IsEmpty(); }))
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Amber())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("ObjectiveCaption", "OBJECTIVE")).Font(CaptionFont()).ColorAndOpacity(Amber())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(White()).AutoWrapText(true)
					.Text_Lambda([this]() { return State.ObjectiveTitle; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted()).AutoWrapText(true)
					.Text_Lambda([this]() { return State.ObjectiveBody; })
				]
			]
		]
		// The tracked mission.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			.Visibility(Collapsed([this]() { return State.bMission; }))
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor_Lambda([this]() { return FSlateColor(State.MissionColour); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock).Font(CaptionFont())
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(State.MissionColour); })
						.Text_Lambda([this]() { return State.MissionLabel; })
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Font(Font("Bold", 10)).ColorAndOpacity(White())
						.Text_Lambda([this]() { return State.MissionCount; })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(TitleFont()).ColorAndOpacity(White()).AutoWrapText(true)
					.Text_Lambda([this]() { return State.MissionTitle; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Font(BodyFont()).ColorAndOpacity(Muted()).AutoWrapText(true)
					.Text_Lambda([this]()
					{
						return State.MissionSubject.IsEmpty() ? State.MissionStep
							: FText::Format(LOCTEXT("MissionStep", "{0}  ·  {1}"), State.MissionStep, State.MissionSubject);
					})
				]
			]
		]
		// The latest events.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 8.0f)
		[
			Caption(LOCTEXT("LatestCaption", "LATEST EVENTS"))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(JournalBox, SVerticalBox)
		];
	return Card(EAPSChromeGlyph::Recent, LOCTEXT("NowTitle", "RIGHT NOW"),
		LOCTEXT("NowSubtitle", "Course, tasks and what just happened"), Body,
		LinkButton(LOCTEXT("OpenJournal", "JOURNAL"), FOnClicked::CreateSP(this, &SAPSPilotDashboard::OpenTab, 5)));
}

#undef LOCTEXT_NAMESPACE

#include "APSColonyConstructionSubsystem.h"

#include "APSColonyModule.h"
#include "APSColonyModuleCatalogue.h"
#include "APSColonyOnboardingSubsystem.h"
#include "APSColonyPlinth.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationRuntimeManifest.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionSubsystem.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "APSColonyConstruction"

DEFINE_LOG_CATEGORY_STATIC(LogAPSColonyConstruction, Log, All);

namespace APSColonyConstruction
{
	constexpr float PollIntervalSeconds = 0.25f;
	/** A finished job whose surface is not loaded tries again this often; it keeps its place in the queue. */
	constexpr double RetryIntervalSeconds = 2.0;
	constexpr int32 QueueCapacity = 6;
	/** Surface modules keep this gap to the base and each other; orbital ones leave more room for ships. */
	constexpr double SurfaceSpacingCm = 800.0;
	constexpr double OrbitSpacingCm = 1500.0;
	constexpr double SurfaceReachCm = 12000.0;
	constexpr double OrbitReachCm = 20000.0;
	constexpr double MaximumModuleSlope = 0.15;
	/** A structure whose underside stands higher than this above the ground gets a plinth, sunk this far in. */
	constexpr double MinimumPlinthGapCm = 40.0;
	constexpr double PlinthBuryCm = 60.0;
	/** The pad's stilts (Rio, 01.10: "a pad on stilts of a reasonable height, so that you can get onto it"). */
	constexpr double LegDiameterCm = 160.0;
	constexpr double MinimumLegDropCm = 30.0;
	constexpr double MaximumLegDropCm = 5000.0;
	/** Its ramp to the ground: about this steep, never shorter or longer than these runs. */
	constexpr double RampSlopeDegrees = 18.0;
	constexpr double RampMinimumRunCm = 1200.0;
	constexpr double RampMaximumRunCm = 6000.0;
	constexpr double RampWidthCm = 1800.0;
	constexpr double RampThicknessCm = 35.0;
	/** Test runs photograph what was ordered after this long even if a site never became ready. */
	constexpr double AutoBuildShotTimeoutSeconds = 180.0;
	const FName EnqueueAction(TEXT("APS.Production.Enqueue"));
	const FName JournalCategory(TEXT("Build"));

	TAutoConsoleVariable<float> CVarBuildTimeScale(TEXT("aps.Colony.BuildTimeScale"), 1.0f,
		TEXT("Scales colony module build times for test runs; read when the build definitions register."));
	TAutoConsoleVariable<FString> CVarAutoBuild(TEXT("aps.Colony.AutoBuild"), TEXT(""),
		TEXT("Module ids (separated by + | , or spaces) ordered once their site is ready, as from the colony terminal (test runs)."));
	TAutoConsoleVariable<int32> CVarAutoOnboarding(TEXT("aps.Colony.AutoOnboarding"), 0,
		TEXT("1: once the surface site is ready, count the colony terminal as opened at the base, before any auto-build ")
		TEXT("(test runs of the onboarding quest)."));
	TAutoConsoleVariable<int32> CVarModuleShots(TEXT("aps.Colony.ModuleShots"), 0,
		TEXT("1: once the auto-built modules stand, photograph each from a temporary camera to ")
		TEXT("Saved/Screenshots/ColonyModules, then capture the colony terminal (test runs)."));

	FText SiteName(const EAPSSpawnSite Site)
	{
		return Site == EAPSSpawnSite::Surface
			? LOCTEXT("SurfaceSite", "the surface base") : LOCTEXT("OrbitSite", "the headquarters");
	}

	FText OrderFailureText(const FName Code)
	{
		const FString Text = Code.ToString();
		if (Text == TEXT("APS.Production.QueueFull"))
		{
			return LOCTEXT("QueueFull", "The build queue at this site is full.");
		}
		if (Text == TEXT("APS.Production.StaleRevision"))
		{
			return LOCTEXT("StaleRevision", "The queue just changed; order again.");
		}
		if (Text == TEXT("APS.Production.PersistenceLoadInProgress"))
		{
			return LOCTEXT("LoadInProgress", "A saved game is still loading.");
		}
		return FText::Format(LOCTEXT("OrderRefused", "Refused: {0}"), FText::FromName(Code));
	}

	FText PlacementFailureText(const FName Code)
	{
		const FString Text = Code.ToString();
		if (Text == TEXT("APS.Spawn.TooSteep"))
		{
			return LOCTEXT("TooSteep", "no level ground near the site");
		}
		if (Text == TEXT("APS.Spawn.Flooded"))
		{
			return LOCTEXT("Flooded", "the ground near the site is under water");
		}
		if (Text == TEXT("APS.Spawn.Crowded"))
		{
			return LOCTEXT("Crowded", "no free room around the site");
		}
		return FText::FromName(Code);
	}

	/** Where the star is seen from a point on the body; zero when the body has no star (a moon, a rogue world). */
	FVector SunDirection(const FVector& From, const APlanetaryBody* Body)
	{
		const APlanet* Planet = Cast<APlanet>(Body);
		const AStar* Star = Planet ? Planet->ParentStar : nullptr;
		return IsValid(Star) ? (Star->GetActorLocation() - From).GetSafeNormal() : FVector::ZeroVector;
	}

	/**
	 * The landing pad on stilts: legs from the deck's underside down to the ground (the centre, 6 inner and 12 outer),
	 * and the access ramp from the deck edge on the base side (-X) down to the ground. DeckFootprint is the deck in the
	 * pad's own space. Returns the number of legs; OutRampRunCm and OutRampDegrees describe the ramp (zero when the
	 * ground below it is not loaded).
	 */
	int32 BuildPadSupports(UWorld& World, AAPSCivilizationLandingPad& Pad, APlanetaryBody& Body,
		const UAPSSpawnPlacementSubsystem& Spawner, const FBox& DeckFootprint, double& OutRampRunCm,
		double& OutRampDegrees)
	{
		OutRampRunCm = 0.0;
		OutRampDegrees = 0.0;
		const FTransform Transform = Pad.GetActorTransform();
		const FVector Up = Transform.GetUnitAxis(EAxis::Z);
		const FVector Scale = Transform.GetScale3D().GetAbs();
		const double DeckRadius = DeckFootprint.GetExtent().X;
		int32 Legs = 0;
		const auto Leg = [&](const double LocalX, const double LocalY)
		{
			const FVector Top = Transform.TransformPosition(FVector(LocalX, LocalY, DeckFootprint.Min.Z));
			const double Drop = Spawner.GroundDropBelow(&Body, Top);
			if (!(Drop > MinimumLegDropCm) || Drop > MaximumLegDropCm)
			{
				return;
			}
			const double Height = Drop + PlinthBuryCm;
			const FTransform LegTransform(Transform.GetRotation(), Top - Up * (Height * 0.5 + 1.0),
				FVector(LegDiameterCm * 0.01, LegDiameterCm * 0.01, Height * 0.01));
			AAPSColonyPlinth* Plinth = World.SpawnActorDeferred<AAPSColonyPlinth>(AAPSColonyPlinth::StaticClass(),
				LegTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Plinth)
			{
				return;
			}
			Plinth->Configure(true);
			Plinth->FinishSpawning(LegTransform);
			Plinth->AttachToActor(&Pad, FAttachmentTransformRules::KeepWorldTransform);
			++Legs;
		};
		Leg(0.0, 0.0);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / 6.0;
			Leg(0.55 * DeckRadius * FMath::Cos(Angle), 0.55 * DeckRadius * FMath::Sin(Angle));
		}
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const double Angle = UE_TWO_PI * (Index + 0.5) / 12.0;
			Leg(0.92 * DeckRadius * FMath::Cos(Angle), 0.92 * DeckRadius * FMath::Sin(Angle));
		}

		// The ramp: its upper edge on the deck edge at deck height, its foot on the ground where it lands.
		UStaticMeshComponent* Ramp = Pad.AccessRamp;
		if (!Ramp || Scale.X <= UE_SMALL_NUMBER || Scale.Y <= UE_SMALL_NUMBER || Scale.Z <= UE_SMALL_NUMBER)
		{
			return Legs;
		}
		const double DeckTop = DeckFootprint.Max.Z;
		const double EdgeX = -DeckRadius;
		double Run = RampMinimumRunCm;
		double Rise = 0.0;
		for (int32 Pass = 0; Pass < 3; ++Pass)
		{
			const FVector Foot = Transform.TransformPosition(FVector(EdgeX - Run / Scale.X, 0.0, DeckTop));
			const double Drop = Spawner.GroundDropBelow(&Body, Foot);
			if (Drop <= -TNumericLimits<double>::Max())
			{
				return Legs;
			}
			Rise = FMath::Max(Drop, 0.0);
			Run = FMath::Clamp(Rise / FMath::Tan(FMath::DegreesToRadians(RampSlopeDegrees)),
				RampMinimumRunCm, RampMaximumRunCm);
		}
		const double Degrees = FMath::RadiansToDegrees(FMath::Atan2(Rise, Run));
		const double Length = FMath::Sqrt(Run * Run + Rise * Rise);
		Ramp->SetRelativeLocationAndRotation(
			FVector(EdgeX - Run * 0.5 / Scale.X, 0.0, (DeckTop - (Rise + RampThicknessCm) * 0.5) / Scale.Z),
			FRotator(Degrees, 0.0, 0.0));
		Ramp->SetRelativeScale3D(FVector(Length / (100.0 * Scale.X), RampWidthCm / (100.0 * Scale.Y),
			RampThicknessCm / (100.0 * Scale.Z)));
		OutRampRunCm = Run;
		OutRampDegrees = Degrees;
		return Legs;
	}
}

bool UAPSColonyConstructionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAPSColonyConstructionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAPSColonyConstructionSubsystem, STATGROUP_Tickables);
}

void UAPSColonyConstructionSubsystem::Deinitialize()
{
	if (ShotCamera.IsValid())
	{
		ShotCamera->Destroy();
	}
	BuiltModules.Reset();
	ShotModules.Reset();
	TestShotsFinished.Clear();
	Super::Deinitialize();
}

void UAPSColonyConstructionSubsystem::Tick(const float DeltaTime)
{
	TickModuleShots();
	PollAccumulator += DeltaTime;
	if (PollAccumulator < APSColonyConstruction::PollIntervalSeconds)
	{
		return;
	}
	PollAccumulator = 0.0f;
	UpdateSites();
	GroundColonyStructures();
	MaterializeFinishedJobs();
	TickTestAutomation();
}

UAPSProductionSubsystem* UAPSColonyConstructionSubsystem::GetProduction() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UAPSProductionSubsystem>() : nullptr;
}

APawn* UAPSColonyConstructionSubsystem::GetPlayerPawn() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

bool UAPSColonyConstructionSubsystem::IsSiteReady(const EAPSSpawnSite Site) const
{
	const FSite& Entry = Sites[static_cast<int32>(Site)];
	return Entry.bRegistered && Entry.Anchor.IsValid();
}

AActor* UAPSColonyConstructionSubsystem::GetSiteAnchor(const EAPSSpawnSite Site) const
{
	return Sites[static_cast<int32>(Site)].Anchor.Get();
}

void UAPSColonyConstructionSubsystem::EnsureDefinitions(UAPSProductionSubsystem& Production)
{
	if (bDefinitionsRegistered)
	{
		return;
	}
	const double TimeScale = FMath::Max(0.0f, APSColonyConstruction::CVarBuildTimeScale.GetValueOnGameThread());
	for (const FAPSColonyModuleSpec& Spec : FAPSColonyModuleCatalogue::Get())
	{
		FString Failure;
		if (!Production.RegisterDefinition(FAPSColonyModuleCatalogue::MakeDefinition(Spec, TimeScale), Failure))
		{
			UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] module %s not registered: %s"),
				*Spec.Id.ToString(), *Failure);
		}
	}
	bDefinitionsRegistered = true;
}

bool UAPSColonyConstructionSubsystem::RegisterSite(const EAPSSpawnSite Kind, AActor* Anchor, APlanetaryBody* Body,
	const FGuid& InCivilizationId)
{
	UAPSProductionSubsystem* Production = GetProduction();
	if (!Production || !IsValid(Anchor) || !InCivilizationId.IsValid()
		|| (Kind == EAPSSpawnSite::Surface && !IsValid(Body)))
	{
		return false;
	}
	CivilizationId = InCivilizationId;
	const FString Civilization = CivilizationId.ToString(EGuidFormats::DigitsWithHyphens);
	// The pilot has no identity of its own yet: a stable one per civilization stands in as the order's subject.
	PilotStableId = MakePilotStableId(CivilizationId);
	EnsureDefinitions(*Production);

	FAPSProductionContextRegistration Registration;
	Registration.ContextStableId = FAPSCivilizationRuntimeManifestFactory::MakeStableId(TEXT("APS.Colony.Site"),
		Civilization + (Kind == EAPSSpawnSite::Surface ? TEXT(":Surface") : TEXT(":Orbit")));
	Registration.OwnerStableId = CivilizationId;
	Registration.Domain = EAPSProductionDomain::Building;
	Registration.AccessMode = EAPSProductionAccessMode::ActorGated;
	Registration.ContextActor = Anchor;
	Registration.QueueCapacity = APSColonyConstruction::QueueCapacity;
	Registration.MaximumConcurrentJobs = 1;
	FString Failure;
	if (!Production->RegisterContext(Registration, Failure))
	{
		UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] %s site on %s not registered: %s"),
			Kind == EAPSSpawnSite::Surface ? TEXT("surface") : TEXT("orbit"), *GetNameSafe(Anchor), *Failure);
		return false;
	}
	FSite& Site = Sites[static_cast<int32>(Kind)];
	Site.Anchor = Anchor;
	Site.Body = Body;
	Site.ContextId = Registration.ContextStableId;
	Site.bRegistered = true;
	UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] %s site ready: %s%s"),
		Kind == EAPSSpawnSite::Surface ? TEXT("surface") : TEXT("orbit"), *GetNameSafe(Anchor),
		IsValid(Body) ? *FString::Printf(TEXT(" on %s"), *Body->GetName()) : TEXT(""));
	return true;
}

void UAPSColonyConstructionSubsystem::UpdateSites()
{
	UWorld* World = GetWorld();
	const UAPSCivilizationMaterializationSubsystem* Colony = World
		? World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>() : nullptr;
	if (!Colony || !Colony->GetRuntimeManifest().CivilizationId.IsValid())
	{
		return;
	}
	const FGuid ManifestCivilization = Colony->GetRuntimeManifest().CivilizationId;
	// Surface: the materialized colony base, on the body it is attached to.
	if (!IsSiteReady(EAPSSpawnSite::Surface) && Colony->IsMaterializationComplete())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const UAPSCivilizationIdentityComponent* Identity = IsValid(*It)
				&& It->ActorHasTag(TEXT("APS.Civilization.Materialized"))
				? It->FindComponentByClass<UAPSCivilizationIdentityComponent>() : nullptr;
			if (Identity && Identity->Role == EAPSCivilizationEntityRole::BaseModule
				&& Identity->OwnerCivilizationId == ManifestCivilization)
			{
				RegisterSite(EAPSSpawnSite::Surface, *It, Cast<APlanetaryBody>(It->GetAttachParentActor()),
					ManifestCivilization);
				break;
			}
		}
	}
	// Orbit: the home headquarters, the default base (Rio, 29.09).
	if (!IsSiteReady(EAPSSpawnSite::Orbit))
	{
		for (TActorIterator<ASpaceHeadquarters> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				RegisterSite(EAPSSpawnSite::Orbit, *It, nullptr, ManifestCivilization);
				break;
			}
		}
	}
}

void UAPSColonyConstructionSubsystem::GroundColonyStructures()
{
	using namespace APSColonyConstruction;
	const FSite& Site = Sites[static_cast<int32>(EAPSSpawnSite::Surface)];
	UWorld* World = GetWorld();
	APlanetaryBody* Body = Site.Body.Get();
	const UAPSSpawnPlacementSubsystem* Spawner = World ? World->GetSubsystem<UAPSSpawnPlacementSubsystem>() : nullptr;
	if (bColonyGrounded || !IsSiteReady(EAPSSpawnSite::Surface) || !Spawner || !IsValid(Body))
	{
		return;
	}
	// The colony's own structures on this body: the base and its landing pad.
	TArray<AActor*> Structures{Site.Anchor.Get()};
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const UAPSCivilizationIdentityComponent* Identity = IsValid(*It) && It->GetAttachParentActor() == Body
			&& It->ActorHasTag(TEXT("APS.Civilization.Materialized"))
			? It->FindComponentByClass<UAPSCivilizationIdentityComponent>() : nullptr;
		if (Identity && Identity->Role == EAPSCivilizationEntityRole::LandingPad)
		{
			Structures.Add(*It);
		}
	}
	struct FMeasured
	{
		AActor* Structure{nullptr};
		FBox Footprint{ForceInit};
		double Gap{0.0};
		bool bRound{false};
	};
	TArray<FMeasured> Measured;
	for (AActor* Structure : Structures)
	{
		FMeasured& Entry = Measured.AddDefaulted_GetRef();
		Entry.Structure = Structure;
		Entry.Gap = Spawner->MeasureGroundGap(Structure, Body, Entry.Footprint, Entry.bRound);
		if (!Entry.Footprint.IsValid)
		{
			// The surface is not loaded yet: measure everything again later, place nothing now.
			return;
		}
	}
	TMap<const AActor*, FString> Supports;
	for (const FMeasured& Entry : Measured)
	{
		if (AAPSCivilizationLandingPad* Pad = Cast<AAPSCivilizationLandingPad>(Entry.Structure))
		{
			// The pad stands on stilts with a ramp down to the ground, not on a solid plinth.
			double RampRun = 0.0;
			double RampDegrees = 0.0;
			const int32 Legs = BuildPadSupports(*World, *Pad, *Body, *Spawner, Entry.Footprint, RampRun, RampDegrees);
			Supports.Add(Pad, FString::Printf(TEXT(", %d stilts, ramp %.0f m at %.0f deg"), Legs, RampRun * 0.01,
				RampDegrees));
			continue;
		}
		if (Entry.Gap < MinimumPlinthGapCm)
		{
			continue;
		}
		const FTransform Transform = Entry.Structure->GetActorTransform();
		const FVector Scale = Transform.GetScale3D().GetAbs();
		const FVector Up = Transform.GetUnitAxis(EAxis::Z);
		const double Height = Entry.Gap + PlinthBuryCm;
		const FVector Top = Transform.TransformPosition(
			FVector(Entry.Footprint.GetCenter().X, Entry.Footprint.GetCenter().Y, Entry.Footprint.Min.Z));
		// Engine shapes are 100 cm and centred: the scale sizes the plinth, its top just under the underside.
		const FTransform PlinthTransform(Transform.GetRotation(), Top - Up * (Height * 0.5 + 1.0),
			FVector(Entry.Footprint.GetExtent().X * 0.02 * Scale.X, Entry.Footprint.GetExtent().Y * 0.02 * Scale.Y,
				Height * 0.01));
		AAPSColonyPlinth* Plinth = World->SpawnActorDeferred<AAPSColonyPlinth>(AAPSColonyPlinth::StaticClass(),
			PlinthTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Plinth)
		{
			continue;
		}
		Plinth->Configure(Entry.bRound);
		Plinth->FinishSpawning(PlinthTransform);
		Plinth->AttachToActor(Entry.Structure, FAttachmentTransformRules::KeepWorldTransform);
	}
	bColonyGrounded = true;
	for (const FMeasured& Entry : Measured)
	{
		// The footprint the gap was measured over (the largest visible part), to tell a steep site from a wrong part.
		const FVector Size = Entry.Footprint.GetSize() * Entry.Structure->GetActorTransform().GetScale3D().GetAbs() * 0.01;
		const FString* Support = Supports.Find(Entry.Structure);
		UE_LOG(LogAPSColonyConstruction, Log,
			TEXT("[APS.Colony.Build] %s: underside up to %.1f m above the ground over a %.0fx%.0f m %s footprint%s"),
			*GetNameSafe(Entry.Structure), Entry.Gap * 0.01, Size.X, Size.Y, Entry.bRound ? TEXT("round") : TEXT("square"),
			Support ? **Support : Entry.Gap >= MinimumPlinthGapCm ? TEXT(", plinth placed beneath")
				: TEXT(", stands on the ground"));
	}
}

bool UAPSColonyConstructionSubsystem::RequestBuild(const FName ModuleId, FText& OutFailure)
{
	return RequestBuild(ModuleId, GetPlayerPawn(), OutFailure);
}

bool UAPSColonyConstructionSubsystem::RequestBuild(const FName ModuleId, AActor* Instigator, FText& OutFailure)
{
	using namespace APSColonyConstruction;
	const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId);
	if (!Spec)
	{
		OutFailure = LOCTEXT("UnknownModule", "Unknown module.");
		return false;
	}
	FSite& Site = Sites[static_cast<int32>(Spec->Site)];
	UAPSProductionSubsystem* Production = GetProduction();
	AActor* Anchor = Site.Anchor.Get();
	if (!Production || !Site.bRegistered || !IsValid(Anchor))
	{
		OutFailure = Spec->Site == EAPSSpawnSite::Surface
			? LOCTEXT("NoBase", "The colony base is not founded yet.")
			: LOCTEXT("NoHeadquarters", "The headquarters is not in this world.");
		return false;
	}
	if (!IsValid(Instigator))
	{
		OutFailure = LOCTEXT("NoPilot", "No pilot to give the order.");
		return false;
	}
	FAPSProductionSnapshot Snapshot;
	FString Failure;
	if (!Production->QuerySnapshot(Site.ContextId, Anchor, EAPSProductionAccessMode::ActorGated, Snapshot, Failure))
	{
		OutFailure = OrderFailureText(FName(*Failure));
		return false;
	}
	FAPSProductionCommand Command;
	Command.CorrelationId = FGuid::NewGuid();
	Command.ActionId = EnqueueAction;
	Command.ExpectedRevision = Snapshot.Revision;
	Command.SubjectStableId = PilotStableId;
	Command.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	Command.ContextStableId = Site.ContextId;
	Command.DefinitionId = FAPSColonyModuleCatalogue::MakeDefinitionId(ModuleId);
	Command.Quantity = 1;
	Command.AccessMode = EAPSProductionAccessMode::ActorGated;
	Command.InstigatorActor = Instigator;
	Command.ContextActor = Anchor;
	const FAPSProductionCommandResult Result = Production->ExecuteCommand(Command);
	if (Result.Status != EAPSProductionCommandStatus::Accepted)
	{
		OutFailure = OrderFailureText(Result.FailureCode);
		UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] order for %s refused: %s"),
			*ModuleId.ToString(), *Result.FailureCode.ToString());
		return false;
	}
	UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] ordered %s at %s (job %s, %.1f s)"),
		*ModuleId.ToString(), *GetNameSafe(Anchor), *Result.JobId.ToString(),
		FAPSColonyModuleCatalogue::MakeDefinition(*Spec,
			FMath::Max(0.0f, CVarBuildTimeScale.GetValueOnGameThread())).DurationSeconds);
	UAPSCivilizationJournalSubsystem::Post(this, JournalCategory, FText::Format(
		LOCTEXT("Ordered", "Construction ordered: {0} at {1}."), Spec->Name, SiteName(Spec->Site)));
	return true;
}

bool UAPSColonyConstructionSubsystem::GetSiteSnapshot(const EAPSSpawnSite Site,
	FAPSProductionSnapshot& OutSnapshot) const
{
	const FSite& Entry = Sites[static_cast<int32>(Site)];
	const UAPSProductionSubsystem* Production = GetProduction();
	FString Failure;
	return Production && Entry.bRegistered && Entry.Anchor.IsValid()
		&& Production->QuerySnapshot(Entry.ContextId, Entry.Anchor.Get(), EAPSProductionAccessMode::ActorGated,
			OutSnapshot, Failure);
}

AAPSColonyModule* UAPSColonyConstructionSubsystem::RestoreModule(const FName ModuleId, const FGuid& StableId,
	const EAPSSpawnSite Site, const FTransform& RelativeToAnchor, const double FoundationDepthCm, const double BoomLengthCm)
{
	const int32 Index = static_cast<int32>(Site);
	UWorld* World = GetWorld();
	if (!World || Index < 0 || Index >= UE_ARRAY_COUNT(Sites) || !FAPSColonyModuleCatalogue::Find(ModuleId))
	{
		return nullptr;
	}
	FSite& Entry = Sites[Index];
	AActor* Anchor = Entry.Anchor.Get();
	if (!Entry.bRegistered || !IsValid(Anchor))
	{
		return nullptr;
	}
	// Loaded twice into one world: the module already stands.
	for (const TWeakObjectPtr<AAPSColonyModule>& Built : BuiltModules)
	{
		if (Built.IsValid() && Built->GetStableId() == StableId)
		{
			return Built.Get();
		}
	}
	const FTransform Transform = RelativeToAnchor * Anchor->GetActorTransform();
	AAPSColonyModule* Module = World->SpawnActorDeferred<AAPSColonyModule>(AAPSColonyModule::StaticClass(), Transform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Module)
	{
		return nullptr;
	}
	Module->Configure(ModuleId, StableId, CivilizationId, FoundationDepthCm, BoomLengthCm);
	Module->FinishSpawning(Transform);
	Module->BuildParts();
	// Attached as a build attaches it: to the body on the surface, to the headquarters in orbit.
	AActor* Parent = Site == EAPSSpawnSite::Surface ? static_cast<AActor*>(Entry.Body.Get()) : Anchor;
	if (IsValid(Parent))
	{
		Module->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
	}
	BuiltModules.Add(Module);
	UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] restored %s beside %s"), *ModuleId.ToString(),
		*GetNameSafe(Anchor));
	return Module;
}

void UAPSColonyConstructionSubsystem::GetBuiltModules(const EAPSSpawnSite Site,
	TArray<AAPSColonyModule*>& OutModules) const
{
	OutModules.Reset();
	for (const TWeakObjectPtr<AAPSColonyModule>& Built : BuiltModules)
	{
		const FAPSColonyModuleSpec* Spec = Built.IsValid()
			? FAPSColonyModuleCatalogue::Find(Built->GetModuleId()) : nullptr;
		if (Spec && Spec->Site == Site)
		{
			OutModules.Add(Built.Get());
		}
	}
}

FText UAPSColonyConstructionSubsystem::GetJobNote(const FGuid& JobId) const
{
	const FText* Note = JobNotes.Find(JobId);
	return Note ? *Note : FText::GetEmpty();
}

FGuid UAPSColonyConstructionSubsystem::MakePilotStableId(const FGuid& CivilizationId)
{
	return FAPSCivilizationRuntimeManifestFactory::MakeStableId(TEXT("APS.Civilization.Pilot"),
		CivilizationId.ToString(EGuidFormats::DigitsWithHyphens));
}

FText UAPSColonyConstructionSubsystem::DescribeFailure(const FName FailureCode)
{
	return APSColonyConstruction::PlacementFailureText(FailureCode);
}

void UAPSColonyConstructionSubsystem::MaterializeFinishedJobs()
{
	UAPSProductionSubsystem* Production = GetProduction();
	if (!Production)
	{
		return;
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sites); ++Index)
	{
		FSite& Site = Sites[Index];
		AActor* Anchor = Site.Anchor.Get();
		FAPSProductionSnapshot Snapshot;
		FString Failure;
		if (!Site.bRegistered || !IsValid(Anchor)
			|| !Production->QuerySnapshot(Site.ContextId, Anchor, EAPSProductionAccessMode::ActorGated, Snapshot,
				Failure))
		{
			continue;
		}
		for (const FAPSProductionJobSnapshot& Job : Snapshot.Jobs)
		{
			if (Job.State == EAPSProductionJobState::AwaitingMaterialization)
			{
				MaterializeJob(static_cast<EAPSSpawnSite>(Index), Site, *Production, Job);
			}
		}
	}
}

bool UAPSColonyConstructionSubsystem::MaterializeJob(const EAPSSpawnSite Kind, FSite& Site,
	UAPSProductionSubsystem& Production, const FAPSProductionJobSnapshot& Job)
{
	using namespace APSColonyConstruction;
	UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (const double* RetryAt = JobRetrySeconds.Find(Job.JobId); RetryAt && Now < *RetryAt)
	{
		return false;
	}
	const FName ModuleId = FAPSColonyModuleCatalogue::ModuleIdFromDefinition(Job.DefinitionId);
	const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId);
	const UAPSSpawnPlacementSubsystem* Spawner = World ? World->GetSubsystem<UAPSSpawnPlacementSubsystem>() : nullptr;
	FString Failure;
	if (!Spec || !Spawner)
	{
		Production.ResolveMaterialization(Site.ContextId, Job.JobId, FGuid(), FSoftClassPath(), false,
			TEXT("APS.Colony.UnknownModule"), Failure);
		return true;
	}

	FAPSSpawnRequest Request;
	Request.Site = Kind;
	Request.Anchor = Site.Anchor.Get();
	Request.Body = Site.Body.Get();
	Request.SizeCm = Spec->SizeCm;
	Request.SpacingCm = Kind == EAPSSpawnSite::Surface ? SurfaceSpacingCm : OrbitSpacingCm;
	Request.MaximumReachCm = Kind == EAPSSpawnSite::Surface ? SurfaceReachCm : OrbitReachCm;
	Request.MaximumSlope = MaximumModuleSlope;
	for (const TWeakObjectPtr<AAPSColonyModule>& Built : BuiltModules)
	{
		if (Built.IsValid())
		{
			Request.Obstacles.Add(Built.Get());
		}
	}
	if (Kind == EAPSSpawnSite::Surface && World)
	{
		// The pad, and a ship parked on it, stand on the same body; the walk from the base to the pad stays free.
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!IsValid(*It) || *It == Request.Anchor || !It->ActorHasTag(TEXT("APS.Civilization.Materialized"))
				|| It->GetAttachParentActor() != Request.Body)
			{
				continue;
			}
			Request.Obstacles.Add(*It);
			const UAPSCivilizationIdentityComponent* Identity =
				It->FindComponentByClass<UAPSCivilizationIdentityComponent>();
			if (Identity && Identity->Role == EAPSCivilizationEntityRole::LandingPad)
			{
				Request.RouteTargets.Add(*It);
			}
		}
	}

	FAPSSpawnPlacement Placement;
	if (!Spawner->ResolvePlacement(Request, Placement))
	{
		if (!Placement.bSiteReady)
		{
			// The surface is not loaded (the pilot is far from it): the crew waits and the job keeps its place.
			JobNotes.Add(Job.JobId, LOCTEXT("WaitingSurface", "Waiting for the surface to load"));
			JobRetrySeconds.Add(Job.JobId, Now + APSColonyConstruction::RetryIntervalSeconds);
			return false;
		}
		Production.ResolveMaterialization(Site.ContextId, Job.JobId, FGuid(), FSoftClassPath(), false,
			Placement.FailureCode, Failure);
		JobNotes.Remove(Job.JobId);
		JobRetrySeconds.Remove(Job.JobId);
		UAPSCivilizationJournalSubsystem::Post(this, JournalCategory, FText::Format(
			LOCTEXT("PlacementFailed", "Construction failed: {0} at {1}, {2}."), Spec->Name, SiteName(Kind),
			PlacementFailureText(Placement.FailureCode)));
		UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] %s not placed: %s after %d candidates"),
			*ModuleId.ToString(), *Placement.FailureCode.ToString(), Placement.CandidatesTried);
		return true;
	}
	if (Spec->bFaceSun)
	{
		// Solar arrays turn their front to the star; the footprint circle that was checked allows any heading.
		const FVector Up = Placement.Transform.GetUnitAxis(EAxis::Z);
		const FVector Facing = FVector::VectorPlaneProject(
			SunDirection(Placement.Transform.GetLocation(), Site.Body.Get()), Up).GetSafeNormal();
		if (!Facing.IsNearlyZero())
		{
			Placement.Transform.SetRotation(FRotationMatrix::MakeFromXZ(Facing, Up).ToQuat());
		}
	}

	const FGuid ModuleStableId = FAPSCivilizationRuntimeManifestFactory::MakeStableId(TEXT("APS.Colony.Module"),
		Job.JobId.ToString(EGuidFormats::DigitsWithHyphens));
	const double BoomLengthCm = Kind == EAPSSpawnSite::Orbit ? Placement.GapToAnchorCm : 0.0;
	const FGuid OwnerCivilization = CivilizationId;
	AAPSColonyModule* Module = Cast<AAPSColonyModule>(Spawner->SpawnAtPlacement(AAPSColonyModule::StaticClass(),
		Placement, [&](AActor* Actor)
		{
			if (AAPSColonyModule* NewModule = Cast<AAPSColonyModule>(Actor))
			{
				NewModule->Configure(ModuleId, ModuleStableId, OwnerCivilization, Placement.FoundationDepthCm,
					BoomLengthCm);
			}
		}));
	if (!IsValid(Module))
	{
		Production.ResolveMaterialization(Site.ContextId, Job.JobId, FGuid(), FSoftClassPath(), false,
			TEXT("APS.Colony.SpawnFailed"), Failure);
		return true;
	}
	// BeginPlay has assembled it in a playing world; a world that never began play (tests) assembles it here.
	Module->BuildParts();
	if (!Production.ResolveMaterialization(Site.ContextId, Job.JobId, ModuleStableId,
		FSoftClassPath(AAPSColonyModule::StaticClass()), true, NAME_None, Failure))
	{
		// Never two modules for one job: take this one down and try again later.
		UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] %s built but not committed: %s"),
			*ModuleId.ToString(), *Failure);
		Module->Destroy();
		JobRetrySeconds.Add(Job.JobId, Now + APSColonyConstruction::RetryIntervalSeconds);
		return false;
	}
	BuiltModules.Add(Module);
	JobNotes.Remove(Job.JobId);
	JobRetrySeconds.Remove(Job.JobId);
	UAPSCivilizationJournalSubsystem::Post(this, JournalCategory, FText::Format(
		LOCTEXT("Built", "Built: {0} beside {1}."), Spec->Name, SiteName(Kind)));
	UE_LOG(LogAPSColonyConstruction, Log,
		TEXT("[APS.Colony.Build] built %s beside %s: %d parts (%d without the pack), slope %.3f, foundation %.0f cm, boom %.0f cm"),
		*ModuleId.ToString(), *GetNameSafe(Site.Anchor.Get()), Module->GetPartCount(),
		Module->GetFallbackPartCount(), Placement.Slope, Placement.FoundationDepthCm, BoomLengthCm);
	return true;
}

void UAPSColonyConstructionSubsystem::TickTestAutomation()
{
	using namespace APSColonyConstruction;
	const FString AutoBuild = CVarAutoBuild.GetValueOnGameThread();
	UWorld* World = GetWorld();
	if (AutoBuild.IsEmpty() || !World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (AutoBuildStartSeconds < 0.0)
	{
		AutoBuildStartSeconds = Now;
	}
	if (CVarAutoOnboarding.GetValueOnGameThread() > 0 && !bAutoOnboardingDone)
	{
		// The onboarding asks for the base interaction before the first build: open it first, as a player would.
		UAPSColonyOnboardingSubsystem* Onboarding = World->GetSubsystem<UAPSColonyOnboardingSubsystem>();
		if (!IsSiteReady(EAPSSpawnSite::Surface) || !Onboarding)
		{
			return;
		}
		Onboarding->NotifyTerminalOpened();
		bAutoOnboardingDone = true;
		AutoBuildStartSeconds = Now + 3.0;
		return;
	}
	if (Now < AutoBuildStartSeconds)
	{
		return;
	}
	// Any of these separate the ids: -ExecCmds splits its own commands at commas.
	static const TCHAR* const Separators[] = {TEXT(","), TEXT("+"), TEXT("|"), TEXT(" ")};
	TArray<FString> Ids;
	AutoBuild.ParseIntoArray(Ids, Separators, UE_ARRAY_COUNT(Separators), true);
	for (FString& Id : Ids)
	{
		Id.TrimStartAndEndInline();
		const FName ModuleId(*Id);
		if (AutoBuildIssued.Contains(ModuleId))
		{
			continue;
		}
		const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId);
		if (!Spec)
		{
			AutoBuildIssued.Add(ModuleId);
			UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] auto-build: no module %s"), *Id);
			continue;
		}
		if (!IsSiteReady(Spec->Site) || !GetPlayerPawn())
		{
			continue;
		}
		FText Failure;
		// One order per module: a refusal is a result too.
		AutoBuildIssued.Add(ModuleId);
		if (RequestBuild(ModuleId, Failure))
		{
			++AutoBuildOrdered;
		}
		else
		{
			UE_LOG(LogAPSColonyConstruction, Warning, TEXT("[APS.Colony.Build] auto-build %s refused: %s"), *Id,
				*Failure.ToString());
		}
	}
	if (CVarModuleShots.GetValueOnGameThread() <= 0 || ShotIndex >= 0 || bShotsDone || AutoBuildOrdered == 0)
	{
		return;
	}
	bool bBusy = AutoBuildIssued.Num() < Ids.Num();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sites) && !bBusy; ++Index)
	{
		FAPSProductionSnapshot Snapshot;
		if (GetSiteSnapshot(static_cast<EAPSSpawnSite>(Index), Snapshot))
		{
			for (const FAPSProductionJobSnapshot& Job : Snapshot.Jobs)
			{
				bBusy |= Job.State == EAPSProductionJobState::Queued || Job.State == EAPSProductionJobState::InProgress
					|| Job.State == EAPSProductionJobState::AwaitingMaterialization;
			}
		}
	}
	if (bBusy && Now - AutoBuildStartSeconds < AutoBuildShotTimeoutSeconds)
	{
		return;
	}
	ShotModules.Reset();
	// Surface modules first: orbital shots take the camera away from the pilot.
	for (const EAPSSpawnSite Site : {EAPSSpawnSite::Surface, EAPSSpawnSite::Orbit})
	{
		TArray<AAPSColonyModule*> Modules;
		GetBuiltModules(Site, Modules);
		ShotModules.Append(Modules);
	}
	ShotIndex = 0;
	ShotStepSeconds = Now + 1.0;
	if (ACustomGravityCharacter* Pilot = Cast<ACustomGravityCharacter>(GetPlayerPawn());
		Pilot && !Pilot->IsSurfaceHandoffSuspended())
	{
		// Hold the pilot still while the camera is away from it.
		Pilot->SetSurfaceHandoffSuspended(true);
		ShotHeldPilot = Pilot;
	}
	UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] photographing %d module(s)%s"),
		ShotModules.Num(), bBusy ? TEXT(" (timed out waiting for the queue)") : TEXT(""));
}

void UAPSColonyConstructionSubsystem::TickModuleShots()
{
	if (ShotIndex < 0)
	{
		return;
	}
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller)
	{
		ShotIndex = -1;
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Now < ShotStepSeconds)
	{
		return;
	}
	const int32 ModuleIndex = ShotIndex / 2;
	AActor* Base = GetSiteAnchor(EAPSSpawnSite::Surface);
	AActor* Pad = nullptr;
	for (TActorIterator<AAPSCivilizationLandingPad> It(World); It && IsValid(Base); ++It)
	{
		if (IsValid(*It) && It->GetAttachParentActor() == Base->GetAttachParentActor())
		{
			Pad = *It;
			break;
		}
	}
	// After the modules, one overview of the surface colony from above, then the pad from low beside it (stilts, ramp).
	const bool bOverview = ModuleIndex == ShotModules.Num() && IsValid(Base);
	const bool bPadShot = ModuleIndex == ShotModules.Num() + 1 && IsValid(Pad);
	if (ModuleIndex >= ShotModules.Num() && !bOverview && !bPadShot)
	{
		// Done: back to the pilot's view, then the terminal's own captures.
		if (APawn* Pawn = Controller->GetPawn())
		{
			Controller->SetViewTarget(Pawn);
		}
		if (ShotCamera.IsValid())
		{
			ShotCamera->Destroy();
		}
		if (ACustomGravityCharacter* Pilot = ShotHeldPilot.Get())
		{
			Pilot->SetSurfaceHandoffSuspended(false);
		}
		ShotHeldPilot.Reset();
		ShotIndex = -1;
		bShotsDone = true;
		UE_LOG(LogAPSColonyConstruction, Log, TEXT("[APS.Colony.Build] module shots saved"));
		TestShotsFinished.Broadcast();
		return;
	}
	AActor* Subject = bOverview ? Base : bPadShot ? Pad : ShotModules[ModuleIndex].Get();
	if (!IsValid(Subject))
	{
		ShotIndex = (ModuleIndex + 1) * 2;
		return;
	}
	const AAPSColonyModule* Module = Cast<AAPSColonyModule>(Subject);
	if (ShotIndex % 2 == 0)
	{
		const FAPSColonyModuleSpec* Spec = Module ? FAPSColonyModuleCatalogue::Find(Module->GetModuleId()) : nullptr;
		const bool bOrbit = Spec && Spec->Site == EAPSSpawnSite::Orbit;
		const FVector Size = Spec ? Spec->SizeCm : FVector(1000.0);
		const FTransform Frame = Subject->GetActorTransform();
		const FVector Up = Frame.GetUnitAxis(EAxis::Z);
		const double PadReach = bPadShot ? Pad->GetComponentsBoundingBox(true).GetExtent().Size() * 1.1 : 0.0;
		const double Reach = bPadShot ? PadReach : bOverview ? 17000.0
			: Size.GetMax() * (bOrbit ? 2.5 : 2.0) + (bOrbit ? 3000.0 : 1200.0);
		const FVector Target = Frame.GetLocation() + Up * (bPadShot ? -150.0 : bOverview ? 800.0 : Size.Z * 0.4);
		// A module is seen from behind (its front faces the site) and above, the site beyond it; the colony from high up;
		// the pad from low on its ramp side, so its stilts and the ramp to the ground show.
		FVector Eye = Target + Frame.TransformVectorNoScale((bPadShot ? FVector(-0.75, 0.6, 0.12)
			: bOverview ? FVector(-0.55, 0.6, 0.6) : FVector(-0.75, 0.55, 0.35)).GetSafeNormal()) * Reach;
		if (bOrbit)
		{
			// A module bolted into a recess of the station would hide behind its arms: look from outside the station's box.
			if (const AActor* Station = GetSiteAnchor(EAPSSpawnSite::Orbit))
			{
				FVector StationCenter;
				FVector StationExtent;
				Station->GetActorBounds(false, StationCenter, StationExtent);
				const FVector Away = (Target - StationCenter).GetSafeNormal();
				Eye = StationCenter + Away * (StationExtent.Size() + Reach) + Up * (Reach * 0.4)
					+ Frame.GetUnitAxis(EAxis::Y) * (Reach * 0.4);
			}
		}
		if (!ShotCamera.IsValid())
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ShotCamera = World->SpawnActor<ACameraActor>(Eye, FRotator::ZeroRotator, Params);
		}
		if (ACameraActor* Camera = ShotCamera.Get())
		{
			Camera->SetActorLocationAndRotation(Eye, FRotationMatrix::MakeFromXZ(Target - Eye, Up).Rotator());
			Controller->SetViewTarget(Camera);
		}
		ShotStepSeconds = Now + 1.5;
	}
	else
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / TEXT("ColonyModules")
			/ FString::Printf(TEXT("%s_%s.png"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")),
				Module ? *Module->GetModuleId().ToString() : bPadShot ? TEXT("LandingPad") : TEXT("ColonyOverview")), false, false);
		ShotStepSeconds = Now + 0.6;
	}
	++ShotIndex;
}

#undef LOCTEXT_NAMESPACE

#include "APSAncients.h"

#include "APSAncientsGeometry.h"
#include "APSAncientsSites.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/TechInfrastructure.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSObjectActions.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Expansion/APSSystemMaterializer.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightBenchmark.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Misc/DelayedAutoRegister.h"
#include "ProceduralMeshComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"
#include "UObject/UnrealType.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

#define LOCTEXT_NAMESPACE "APSAncients"

namespace APSAncientsLocal
{
	using namespace APSAncients;

	TAutoConsoleVariable<int32> CVarEnable(TEXT("aps.Ancients.Enable"), 1,
		TEXT("1: the Builders' ancient sites are placed, built and run their quest chains. 0: the runtime stops (built sites stay)."));
	TAutoConsoleVariable<int32> CVarQuests(TEXT("aps.Ancients.Quests"), 1,
		TEXT("1: the ancient sites' quest chains run on the department mission board. 0: the sites stand without quests."));
	TAutoConsoleVariable<int32> CVarShadows(TEXT("aps.Ancients.Shadows"), 1,
		TEXT("1: the ancient sites' stone casts shadows (also into the far cascades). Applies to sites built afterwards (aps.Ancients.Rebuild)."));
	TAutoConsoleVariable<float> CVarGlow(TEXT("aps.Ancients.Glow"), 1.0f,
		TEXT("Brightness of the ancient sites' glyph seams and crowns; 0 builds them without glow. Applies to sites built afterwards."));
	TAutoConsoleVariable<float> CVarSignalDelay(TEXT("aps.Ancients.SignalDelay"), 20.0f,
		TEXT("Seconds after the quests open (a new game, or a load once applied) before the home monument's signal comes in."));
	// Rio 09.10 (0.6.4 playtest: REZANOGRAD's stone circle hung ~1 km over the moon's real ground). A root's height function
	// carries its body's seed and sphere radius only once the root built them (GenerateBaseMeshBatch: PlanetNoise.SetSeed,
	// PlanetScaleCode). H_CIRCLE was placed 4 s before the moon's first build (log 23:12:14 vs Surface ready 23:12:18) and
	// sampled the previous/default noise: centre -1571 m, and another place and -286 m on the next load of the same world.
	TAutoConsoleVariable<int32> CVarWaitForSurface(TEXT("aps.Ancients.WaitForSurface"), 1,
		TEXT("1: a surface site is chosen and built only once its body's WorldScape surface is ready, so the height function it ")
		TEXT("samples is the one the terrain was built from. 0: as soon as the surface profile is applied (0.6.4 and before)."));
	TAutoConsoleVariable<float> CVarReseatMetres(TEXT("aps.Ancients.ReseatMetres"), 25.0f,
		TEXT("A built surface site whose centre stands more than this many metres off its body's current (ready) surface is ")
		TEXT("rebuilt on it at the same place, logged with both heights. 0: never."));

	const FName SiteTag(TEXT("APS.Ancient.Site"));
	const FString SiteTagPrefix(TEXT("APS.Ancient.Site."));
	/** The civilization map lists actors with this tag as anomaly sites (SAPSCivilizationMap). */
	const FName AnomalyTag(TEXT("APS.Fleet.Anomaly"));
	const FLinearColor AncientColour(0.25f, 0.9f, 1.0f, 1.0f);

	constexpr float LogicSeconds = 0.25f;
	constexpr float LostScanSeconds = 5.0f;
	/** Candidate places tried per tick, and in all before the best one is taken. */
	constexpr int32 CandidatesPerTick = 6;
	constexpr int32 MaxCandidates = 48;
	/** Emissive strength of the glyph seams and of the crowns (EmissiveMeshMaterial "Color"). */
	constexpr float GlowStrength = 8.0f;
	constexpr float BeaconStrength = 60.0f;
	/** A step the player dropped comes back as an offer after this long. */
	constexpr double ReofferSeconds = 120.0;
	/** Start distances: the hull's chain, the circle's, a lost work's, a nearby site's (cm). */
	constexpr double HullNoticeCm = 150000000.0;
	constexpr double CircleNoticeCm = 5000000.0;
	constexpr double LostNoticeCm = 20000000.0;
	constexpr double RoadNoticeCm = 10000000.0;
	constexpr double KmToCm = 100000.0;

	/** Every game and PIE world's runtime (raw pointers: owned here, freed at the world's cleanup, never at exit). */
	TMap<const UWorld*, FAPSAncients*>& Registry()
	{
		static TMap<const UWorld*, FAPSAncients*>* Map = new TMap<const UWorld*, FAPSAncients*>();
		return *Map;
	}

	/** The root has built the body's surface: its height function now uses the seed and radius the terrain was built from. */
	bool IsSurfaceBuilt(const APlanetaryBody& Body, const AWorldScapeRoot& Root)
	{
		return Body.bWorldScapeSurfaceReady && Root.init && Root.PlanetScaleCode == static_cast<double>(Root.PlanetScale);
	}

	/** The body's WorldScape root when it carries this body's current surface; null otherwise. */
	AWorldScapeRoot* LoadedRoot(const APlanetaryBody* Body)
	{
		APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0f || !Surface->IsSurfaceProfileCurrent(Body))
		{
			return nullptr;
		}
		return CVarWaitForSurface.GetValueOnGameThread() == 0 || IsSurfaceBuilt(*Body, *Root) ? Root : nullptr;
	}

	/** How far (cm) a built surface site's centre stands above (+) or below (-) its body's built surface; 0 when unknown. */
	double OffSurfaceCm(const APlanetaryBody* Body, const AActor& Actor, const double NavHeightCm, double& OutGroundCm)
	{
		AWorldScapeRoot* Root = LoadedRoot(Body);
		if (!Root || !IsSurfaceBuilt(*Body, *Root))
		{
			return 0.0;
		}
		const FVector BodyCentre = Root->GetActorLocation();
		const FVector SiteCentre = Actor.GetActorLocation() - Actor.GetActorUpVector() * NavHeightCm;
		const FVector Direction = (SiteCentre - BodyCentre).GetSafeNormal();
		const double Scale = Root->PlanetScale;
		OutGroundCm = Root->GetGroundHeight(BodyCentre + Direction * Scale, false);
		return FMath::IsFinite(OutGroundCm) ? FVector::Dist(SiteCentre, BodyCentre) - (Scale + OutGroundCm) : 0.0;
	}

	/** The sea's radius from the body's centre (cm), or -1 for a world without liquid. */
	double SeaRadiusOf(const APlanetaryBody& Body, const AWorldScapeRoot& Root)
	{
		const APlanetarySurfaceGenerator* Surface = Body.PlanetaryEnvironmentGenerator;
		return IsValid(Surface) && Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			? static_cast<double>(Root.PlanetScale) + static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root.NoiseIntensity
			: -1.0;
	}

	double RadiusOf(const AActor* Actor)
	{
		if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor))
		{
			return Body->GetWorldScapeBodyRadiusCm();
		}
		if (const AStar* Star = Cast<AStar>(Actor))
		{
			return FMath::Max(static_cast<double>(Star->StarRadiusKM), 1.0) * KmToCm;
		}
		return 100000000.0;
	}

	/** Up at the site, its forward (the body's north turned by the yaw) and right, world space. */
	void SiteFrame(const AActor& Body, const FVector& LocalUp, const double Yaw, FVector& OutUp, FVector& OutForward, FVector& OutRight)
	{
		const FTransform& BodyTransform = Body.GetActorTransform();
		OutUp = BodyTransform.TransformVectorNoScale(LocalUp).GetSafeNormal();
		FVector North = FVector::VectorPlaneProject(BodyTransform.TransformVectorNoScale(FVector::UpVector), OutUp).GetSafeNormal();
		if (North.IsNearlyZero())
		{
			North = FVector::VectorPlaneProject(BodyTransform.TransformVectorNoScale(FVector::ForwardVector), OutUp).GetSafeNormal();
		}
		OutForward = FQuat(OutUp, Yaw).RotateVector(North).GetSafeNormal();
		OutRight = FVector::CrossProduct(OutUp, OutForward).GetSafeNormal();
	}

	/** The ground under the site's plane from WorldScape's own height function (the one its terrain is built from). */
	struct FWorldScapeGround final : FAPSAncientGround
	{
		AWorldScapeRoot* Root{nullptr};
		FVector BodyCentre{FVector::ZeroVector};
		FVector Origin{FVector::ZeroVector};
		FVector Forward{FVector::ForwardVector};
		FVector Right{FVector::RightVector};
		FVector Up{FVector::UpVector};

		virtual double Z(const double X, const double Y) const override
		{
			const FVector Direction = (Origin + Forward * X + Right * Y - BodyCentre).GetSafeNormal();
			const double Radius = static_cast<double>(Root->PlanetScale)
				+ Root->GetGroundHeight(BodyCentre + Direction * static_cast<double>(Root->PlanetScale), false);
			return FVector::DotProduct(BodyCentre + Direction * Radius - Origin, Up);
		}
	};

	/** Orbital sites stand on nothing. */
	struct FFlatGround final : FAPSAncientGround
	{
		virtual double Z(const double, const double) const override
		{
			return 0.0;
		}
	};

	/** How a candidate place fits: the ground's spread under the footprint and whether all of it stands above the sea. */
	struct FCandidateFit
	{
		double Slope{0.0};
		double Low{0.0};
		bool bDry{true};
	};

	FCandidateFit FitAt(AWorldScapeRoot& Root, const double DryRadius, const FVector& Up, const FVector& Forward, const FVector& Right,
		const double Footprint)
	{
		const FVector Centre = Root.GetActorLocation();
		const double Scale = Root.PlanetScale;
		const auto RadiusAt = [&Root, &Centre, Scale](const FVector& Direction)
		{
			return Scale + Root.GetGroundHeight(Centre + Direction * Scale, false);
		};
		const double CentreRadius = RadiusAt(Up);
		double Low = CentreRadius;
		double High = CentreRadius;
		const FVector Origin = Centre + Up * CentreRadius;
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / 8.0;
			const double Radius = RadiusAt((Origin + (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)) * Footprint - Centre).GetSafeNormal());
			Low = FMath::Min(Low, Radius);
			High = FMath::Max(High, Radius);
		}
		FCandidateFit Fit;
		Fit.Low = Low;
		Fit.Slope = (High - Low) / FMath::Max(2.0 * Footprint, 1.0);
		Fit.bDry = DryRadius <= 0.0 || Low > DryRadius;
		return Fit;
	}

	void SetInGameName(AActor* Actor, const FText& Name)
	{
		// The world actor keeps its display name protected; it is a reflected property (as the fleet's anomaly beacons do).
		if (FTextProperty* NameProperty = Actor ? FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")) : nullptr)
		{
			NameProperty->SetPropertyValue_InContainer(Actor, Name);
		}
	}

	UMaterialInterface* ShapeMaterial(AActor& Owner, const FLinearColor& Colour, const float Glow)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, Glow > 0.0f
			? TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")
			: TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Parent)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, &Owner);
		Material->SetVectorParameterValue(TEXT("Color"), Glow > 0.0f ? Colour * Glow : Colour);
		return Material;
	}

	/** One procedural mesh under the site's root, its origin at the site's centre (the root is the navigation point). */
	UProceduralMeshComponent* MakeMesh(AActor& Owner, USceneComponent& Root, const TCHAR* Name, const double Drop,
		const bool bCollision, const bool bShadow, const float CullCm)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(&Owner, Name);
		Mesh->SetupAttachment(&Root);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetRelativeLocation(FVector(0.0, 0.0, -Drop));
		Mesh->bUseAsyncCooking = true;
		Mesh->bUseComplexAsSimpleCollision = true;
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetGenerateOverlapEvents(false);
		if (bCollision)
		{
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		Mesh->SetCastShadow(bShadow);
		Mesh->bCastFarShadow = bShadow;
		if (CullCm > 0.0f)
		{
			Mesh->SetCullDistance(CullCm);
		}
		Mesh->RegisterComponent();
		Owner.AddInstanceComponent(Mesh);
		return Mesh;
	}

	void SetSection(UProceduralMeshComponent& Mesh, const int32 Section, const FAPSAncientMesh& Data, const bool bCollision,
		UMaterialInterface* Material)
	{
		if (Data.IsEmpty())
		{
			return;
		}
		Mesh.CreateMeshSection(Section, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, TArray<FColor>(), Data.Tangents, bCollision);
		if (Material)
		{
			Mesh.SetMaterial(Section, Material);
		}
	}

	const TCHAR* StageName(const FAPSAncients::FSite::EStage Stage)
	{
		switch (Stage)
		{
		case FAPSAncients::FSite::EStage::Waiting: return TEXT("waiting for its world's surface");
		case FAPSAncients::FSite::EStage::Resolving: return TEXT("finding its place");
		case FAPSAncients::FSite::EStage::Built: return TEXT("built");
		default: return TEXT("waiting for its system");
		}
	}

	void HandleWorldInit(UWorld* World, const UWorld::InitializationValues /*Values*/)
	{
		if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE) || Registry().Contains(World))
		{
			return;
		}
		Registry().Add(World, new FAPSAncients(World));
	}

	void HandleWorldCleanup(UWorld* World, const bool /*bSessionEnded*/, const bool /*bCleanupResources*/)
	{
		if (FAPSAncients* Ancients = Registry().FindRef(World))
		{
			Registry().Remove(World);
			delete Ancients;
		}
	}

	/**
	 * Registered once the engine has started (so no shared module or class had to change): a runtime for every game and
	 * PIE world, its object actions on every screen's object page.
	 */
	FDelayedAutoRegisterHelper GRegistration(EDelayedRegisterRunPhase::EndOfEngineInit, []()
	{
		FWorldDelegates::OnPostWorldInitialization.AddStatic(&HandleWorldInit);
		FWorldDelegates::OnWorldCleanup.AddStatic(&HandleWorldCleanup);
		APSObjectActions::RegisterProvider(TEXT("Ancients"), [](UWorld* InWorld, AActor* Object, TArray<FAPSObjectAction>& OutActions)
		{
			const FAPSAncients* Ancients = APSAncientsFind(InWorld);
			if (const FAPSAncients::FSite* Site = Ancients ? Ancients->FindByActor(Object) : nullptr)
			{
				Ancients->GatherActions(*Site, Object, OutActions);
			}
		});
		// A world that started before the engine finished (a -game run straight into a map) gets its runtime now.
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* Existing = Context.World();
				if (Existing && Existing->bIsWorldInitialized)
				{
					HandleWorldInit(Existing, UWorld::InitializationValues());
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] registered: a runtime for every game and PIE world"));
	});

	/** The console's world, else any world with a runtime (the console may run in the editor's world during PIE). */
	FAPSAncients* FindForConsole(const UWorld* InWorld)
	{
		if (FAPSAncients* Found = APSAncientsFind(InWorld))
		{
			return Found;
		}
		for (const TPair<const UWorld*, FAPSAncients*>& Pair : Registry())
		{
			if (Pair.Value)
			{
				return Pair.Value;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[APS.Ancients] no ancients runtime in this world (start a game or PIE)"));
		return nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs ListCommand(TEXT("aps.Ancients.List"),
		TEXT("Logs every ancient site: id, kind, size, world, place, build stage, distance and quest step."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* InWorld)
		{
			if (const FAPSAncients* Ancients = FindForConsole(InWorld))
			{
				Ancients->LogSites();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs TeleportCommand(TEXT("aps.Ancients.Teleport"),
		TEXT("Moves the pilot to an ancient site: aps.Ancients.Teleport <index|id>. In a ship: above it; on foot: beside it. A site not built yet: to its world (or its system's edge) first."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* InWorld)
		{
			FAPSAncients* Ancients = FindForConsole(InWorld);
			const int32 Index = Ancients && !Args.IsEmpty() ? Ancients->FindIndex(Args[0]) : INDEX_NONE;
			if (Index == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Ancients] teleport: name a site by index or id (aps.Ancients.List)"));
				return;
			}
			Ancients->Teleport(Index);
		}));

	FAutoConsoleCommandWithWorldAndArgs AdvanceCommand(TEXT("aps.Ancients.Advance"),
		TEXT("Tests: completes the active quest step of a site (starting its chain first), through the normal path: aps.Ancients.Advance <index|id>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* InWorld)
		{
			FAPSAncients* Ancients = FindForConsole(InWorld);
			const int32 Index = Ancients && !Args.IsEmpty() ? Ancients->FindIndex(Args[0]) : INDEX_NONE;
			if (Index != INDEX_NONE)
			{
				Ancients->Advance(Index);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs StartAllCommand(TEXT("aps.Ancients.StartAll"),
		TEXT("Tests: starts every ancient site's quest chain now, whatever its start condition."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* InWorld)
		{
			if (FAPSAncients* Ancients = FindForConsole(InWorld))
			{
				Ancients->StartAll();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs RebuildCommand(TEXT("aps.Ancients.Rebuild"),
		TEXT("Builds every ancient site again (after changing aps.Ancients.Shadows or aps.Ancients.Glow)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* InWorld)
		{
			if (FAPSAncients* Ancients = FindForConsole(InWorld))
			{
				Ancients->Rebuild();
			}
		}));
}

FAPSAncients* APSAncientsFind(const UWorld* World)
{
	return World ? APSAncientsLocal::Registry().FindRef(World) : nullptr;
}

FAPSAncients::FAPSAncients(UWorld* InWorld)
	: World(InWorld)
{
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] runtime started for %s"), *GetNameSafe(InWorld));
}

FAPSAncients::~FAPSAncients() = default;

TStatId FAPSAncients::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSAncients, STATGROUP_Tickables);
}

bool FAPSAncients::IsTickable() const
{
	return World.IsValid();
}

UWorld* FAPSAncients::GetTickableGameObjectWorld() const
{
	return World.Get();
}

double FAPSAncients::Now() const
{
	const UWorld* LiveWorld = World.Get();
	return LiveWorld ? LiveWorld->GetTimeSeconds() : 0.0;
}

APawn* FAPSAncients::Pilot() const
{
	const UWorld* LiveWorld = World.Get();
	const APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

void FAPSAncients::Tick(const float DeltaTime)
{
	using namespace APSAncientsLocal;
	TRACE_CPUPROFILER_EVENT_SCOPE(APSAncients_Tick);
	if (CVarEnable.GetValueOnGameThread() == 0)
	{
		return;
	}
	LogicClock += DeltaTime;
	if (LogicClock < LogicSeconds)
	{
		return;
	}
	const float Elapsed = LogicClock;
	LogicClock = 0.0f;
	AAstroGenerator* Gen = FindGenerator();
	if (!Gen)
	{
		return;
	}
	if (!bHomeBuilt)
	{
		BuildHomeSites(*Gen);
		if (!bHomeBuilt)
		{
			return;
		}
	}
	LostClock -= Elapsed;
	if (LostClock <= 0.0f)
	{
		LostClock = LostScanSeconds;
		ScanLostWorks(*Gen);
	}
	if (!bNearbyBuilt)
	{
		BuildNearbySites();
	}
	UpdateSites();
	UpdateQuests(Elapsed);
}

AAstroGenerator* FAPSAncients::FindGenerator()
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return nullptr;
	}
	if (AAstroGenerator* Known = Generator.Get(); Known && IsValid(Known->HomePlanet))
	{
		return Known;
	}
	// The authored single-play map stays exactly as it was saved: nothing is added to it.
	const UGameInstance* GameInstance = LiveWorld->GetGameInstance();
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (Gameplay && Gameplay->bUseAuthoredSinglePlayWorld)
	{
		return nullptr;
	}
	for (TActorIterator<AAstroGenerator> It(LiveWorld); It; ++It)
	{
		if (IsValid(*It) && !It->ActorHasTag(TEXT("WorldGenerationPreview")) && !It->UsesContinuousPreviewFrame()
			&& IsValid(It->HomePlanet))
		{
			Generator = *It;
			return *It;
		}
	}
	return nullptr;
}

FAPSAncients::FSite& FAPSAncients::AddSite(const APSAncients::FSiteSpec& Spec, AActor* Body)
{
	FSite& Site = Sites.AddDefaulted_GetRef();
	Site.Spec = Spec;
	Site.Body = Body;
	Site.Stage = Body ? FSite::EStage::Waiting : FSite::EStage::Unbound;
	Site.BodyName = Body ? APSAncientsSites::BodyName(Body) : FText::GetEmpty();
	// Measured from the seed alone, so the texts know its size before it is built.
	FAPSAncientShape Measure;
	APSAncientsGeometry::Build(Spec, nullptr, Measure);
	Site.Metrics = Measure.Metrics;
	if (Spec.SystemId.IsValid())
	{
		if (const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get()))
		{
			if (const FAPSStarSystemInfo* Info = Stars->Find(Spec.SystemId))
			{
				Site.SystemName = FText::FromString(Info->Name);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] site %s: %s %s x%.2f, chain %s, %s%s, seed %u"), *Spec.Id,
		APSAncients::KindLabel(Spec.Kind), *APSAncients::SizeName(Spec.Size).ToString(), Spec.Scale,
		*APSAncients::ChainName(Spec.Chain).ToString(), Body ? TEXT("on ") : TEXT("in system "),
		Body ? *Site.BodyName.ToString() : *Site.SystemName.ToString(), Spec.Seed);
	return Site;
}

void FAPSAncients::BuildHomeSites(AAstroGenerator& Gen)
{
	APlanet* Home = Gen.HomePlanet;
	if (!IsValid(Home) || Home->AstroName.IsNone())
	{
		return;
	}
	// Every roll hashes the home planet's catalogue name; the name and the moons settle with the generation, so wait
	// until the name has held for a moment.
	if (Home->AstroName != HomeNameSeen)
	{
		HomeNameSeen = Home->AstroName;
		HomeNameSince = Now();
		return;
	}
	if (Now() - HomeNameSince < 3.0)
	{
		return;
	}
	WorldSeed = APSAncientsSites::WorldSeedOf(World.Get(), &Gen);
	TArray<APSAncients::FSiteSpec> Specs;
	TMap<FString, TWeakObjectPtr<APlanetaryBody>> Bodies;
	APSAncientsSites::BuildHomeSpecs(WorldSeed, Home, Specs, Bodies);
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] start system of %s, world seed %d: %d guaranteed site(s)"),
		*Home->AstroName.ToString(), WorldSeed, Specs.Num());
	for (const APSAncients::FSiteSpec& Spec : Specs)
	{
		AActor* Body = Bodies.FindRef(Spec.Id).Get();
		AddSite(Spec, Body);
		// The worlds with a guaranteed site get no chance site besides.
		RolledBodies.Add(Spec.BodyKey);
	}
	bHomeBuilt = true;
}

void FAPSAncients::ScanLostWorks(AAstroGenerator& Gen)
{
	const APlanet* Home = Gen.HomePlanet;
	const AStar* Star = IsValid(Home) && IsValid(Home->ParentStar) ? Home->ParentStar : Gen.HomeStar;
	if (!IsValid(Star))
	{
		return;
	}
	TArray<APlanetaryBody*> Bodies;
	for (APlanet* Planet : Star->Planets)
	{
		if (!IsValid(Planet))
		{
			continue;
		}
		Bodies.Add(Planet);
		for (AMoon* Moon : Planet->Moons)
		{
			if (IsValid(Moon))
			{
				Bodies.Add(Moon);
			}
		}
	}
	// Each world is rolled from its own key the first time it is seen: the order worlds appear in does not matter.
	for (APlanetaryBody* Body : Bodies)
	{
		if (Body->AstroName.IsNone())
		{
			continue;
		}
		const FString Key = APSAncientsSites::KeyOf(Body);
		if (RolledBodies.Contains(Key))
		{
			continue;
		}
		RolledBodies.Add(Key);
		APSAncients::FSiteSpec Spec;
		if (APSAncientsSites::RollLostWorks(WorldSeed, Body, Spec))
		{
			AddSite(Spec, Body);
		}
	}
}

void FAPSAncients::BuildNearbySites()
{
	const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	if (!Stars || !Stars->IsReady() || !Stars->GetHome())
	{
		return;
	}
	TArray<APSAncients::FSiteSpec> Specs;
	APSAncientsSites::BuildNearbySpecs(*Stars, Specs);
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] nearby systems (cluster seed %d): %d site(s) among the nearest"),
		Stars->GetClusterSeed(), Specs.Num());
	for (const APSAncients::FSiteSpec& Spec : Specs)
	{
		AddSite(Spec, nullptr);
	}
	bNearbyBuilt = true;
	// Their saved missions (a load) get their rewards back too.
	bHydrated = false;
}

void FAPSAncients::UpdateSites()
{
	using namespace APSAncientsLocal;
	// One heavy piece of work a tick: a batch of candidate places or one build.
	bool bBudget = true;
	for (FSite& Site : Sites)
	{
		switch (Site.Stage)
		{
		case FSite::EStage::Unbound:
			BindNearby(Site);
			break;
		case FSite::EStage::Waiting:
		{
			AActor* Body = Site.Body.Get();
			if (!Body)
			{
				if (!Site.Spec.bHomeSystem)
				{
					Site.Stage = FSite::EStage::Unbound;
				}
				break;
			}
			if (!bBudget)
			{
				break;
			}
			if (Site.Spec.IsOrbital())
			{
				bBudget = false;
				BuildOrbital(Site);
				break;
			}
			if (LoadedRoot(Cast<APlanetaryBody>(Body)))
			{
				if (Site.bResolved)
				{
					bBudget = false;
					BuildSurface(Site);
				}
				else
				{
					Site.Stage = FSite::EStage::Resolving;
				}
			}
			break;
		}
		case FSite::EStage::Resolving:
			if (bBudget)
			{
				bBudget = !ResolveSome(Site);
			}
			break;
		case FSite::EStage::Built:
			if (!Site.Body.IsValid() || !Site.Actor.IsValid())
			{
				// Its world went (a nearby system released) or the actor was removed: it is built again when it stands.
				UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: %s; built again when its world stands"), *Site.Spec.Id,
					Site.Body.IsValid() ? TEXT("the site actor is gone") : TEXT("its world is gone"));
				DestroyActor(Site);
				if (!Site.Body.IsValid())
				{
					Site.bResolved = false;
					Site.NextCandidate = 0;
					Site.BestCandidate = INDEX_NONE;
					Site.BestScore = TNumericLimits<double>::Max();
					Site.SeenPlanets = INDEX_NONE;
				}
				Site.Stage = Site.Body.IsValid() || Site.Spec.bHomeSystem ? FSite::EStage::Waiting : FSite::EStage::Unbound;
			}
			else if (bBudget && !Site.Spec.IsOrbital() && CVarReseatMetres.GetValueOnGameThread() > 0.0f)
			{
				// A site built from a height function that was not the body's built one (or a surface rebuilt since) stands
				// off the ground the terrain and its collision have: it is rebuilt on that ground, at the same place.
				const APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get());
				double GroundCm = 0.0;
				const double OffCm = OffSurfaceCm(Body, *Site.Actor.Get(), Site.Metrics.NavHeightCm, GroundCm);
				if (FMath::Abs(OffCm) > CVarReseatMetres.GetValueOnGameThread() * 100.0)
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: re-seated on %s's built surface: centre %.0f m -> %.0f m from the base radius (was %.0f m off the ground)"),
						*Site.Spec.Id, *Site.BodyName.ToString(), (GroundCm + OffCm) / 100.0, GroundCm / 100.0, OffCm / 100.0);
					bBudget = false;
					BuildSurface(Site);
				}
			}
			break;
		default:
			break;
		}
	}
}

void FAPSAncients::BindNearby(FSite& Site)
{
	FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
	const FAPSSystemMaterializer* Materializer = Stars ? Stars->GetMaterializer() : nullptr;
	if (Site.Spec.bHomeSystem || !Materializer || Materializer->GetActiveIndex() != Site.Spec.SystemIndex)
	{
		Site.SeenPlanets = INDEX_NONE;
		return;
	}
	TArray<APlanet*> Planets;
	Materializer->GetPlanets(Planets);
	// The materializer spawns a planet (with its moons) a few times a second: the world is chosen once they all stand.
	if (Planets.Num() != Site.SeenPlanets)
	{
		Site.SeenPlanets = Planets.Num();
		Site.PlanetsSince = Now();
		return;
	}
	if (Planets.IsEmpty() || Now() - Site.PlanetsSince < 3.0)
	{
		return;
	}
	// A surface site takes a world with a surface (planets and their moons), a derelict any planet; by name, so the choice
	// is the same whatever order the worlds came in. A system without a surface world keeps a derelict instead.
	TArray<APlanetaryBody*> Eligible;
	for (APlanet* Planet : Planets)
	{
		if (Site.Spec.IsOrbital())
		{
			Eligible.Add(Planet);
			continue;
		}
		if (UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Planet->PlanetType))
		{
			Eligible.Add(Planet);
		}
		for (AMoon* Moon : Planet->Moons)
		{
			if (IsValid(Moon) && UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Moon->PlanetType))
			{
				Eligible.Add(Moon);
			}
		}
	}
	if (Eligible.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: no world with a surface in %s; the site is a derelict instead"),
			*Site.Spec.Id, *Site.SystemName.ToString());
		Site.Spec.Kind = APSAncients::EKind::Derelict;
		for (APlanet* Planet : Planets)
		{
			Eligible.Add(Planet);
		}
	}
	Eligible.Sort([](const APlanetaryBody& A, const APlanetaryBody& B) { return A.AstroName.LexicalLess(B.AstroName); });
	APlanetaryBody* Body = Eligible[static_cast<int32>(Site.Spec.Pick % static_cast<uint32>(Eligible.Num()))];
	Site.Body = Body;
	Site.Spec.BodyKey = APSAncientsSites::KeyOf(Body);
	Site.BodyName = APSAncientsSites::BodyName(Body);
	FAPSAncientShape Measure;
	APSAncientsGeometry::Build(Site.Spec, nullptr, Measure);
	Site.Metrics = Measure.Metrics;
	Site.Stage = FSite::EStage::Waiting;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: %s stands in %s on %s (%d eligible world(s))"), *Site.Spec.Id,
		APSAncients::KindLabel(Site.Spec.Kind), *Site.SystemName.ToString(), *Site.BodyName.ToString(), Eligible.Num());
}

bool FAPSAncients::ResolveSome(FSite& Site)
{
	using namespace APSAncientsLocal;
	APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get());
	AWorldScapeRoot* Root = LoadedRoot(Body);
	if (!Body || !Root)
	{
		Site.Stage = FSite::EStage::Waiting;
		return false;
	}
	const double Footprint = FMath::Max(Site.Metrics.FootprintCm, 1000.0);
	const double MaxSlope = APSAncientsGeometry::MaxSlope(Site.Spec.Kind);
	const double Sea = SeaRadiusOf(*Body, *Root);
	const double DryRadius = Sea > 0.0 ? Sea + (Site.Spec.Size == ESize::Small ? 300.0 : 2000.0) : -1.0;
	const double Yaw = APSAncientsSites::Yaw(Site.Spec.Seed);
	int32 Chosen = INDEX_NONE;
	for (int32 Batch = 0; Batch < CandidatesPerTick && Site.NextCandidate < MaxCandidates; ++Batch)
	{
		const int32 Index = Site.NextCandidate++;
		FVector Up;
		FVector Forward;
		FVector Right;
		SiteFrame(*Body, APSAncientsSites::CandidateDirection(Site.Spec.Seed, Index), Yaw, Up, Forward, Right);
		const FCandidateFit Fit = FitAt(*Root, DryRadius, Up, Forward, Right, Footprint);
		// The first place in the seeded order with dry, gentle ground wins; failing all, the least steep and wet one.
		const double Score = Fit.Slope + (Fit.bDry ? 0.0 : 10.0 + FMath::Max(DryRadius - Fit.Low, 0.0) / KmToCm);
		if (Score < Site.BestScore)
		{
			Site.BestScore = Score;
			Site.BestCandidate = Index;
		}
		if (Fit.bDry && Fit.Slope <= MaxSlope)
		{
			Chosen = Index;
			break;
		}
	}
	if (Chosen == INDEX_NONE && Site.NextCandidate < MaxCandidates)
	{
		return true;
	}
	if (Chosen == INDEX_NONE)
	{
		Chosen = Site.BestCandidate;
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: no candidate on %s is dry and gentle enough; the best of %d (score %.3f)"),
			*Site.Spec.Id, *Site.BodyName.ToString(), MaxCandidates, Site.BestScore);
	}
	Site.Candidate = Chosen;
	Site.LocalUp = APSAncientsSites::CandidateDirection(Site.Spec.Seed, Chosen);
	Site.bResolved = true;
	Site.Stage = FSite::EStage::Waiting;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: place on %s at %s (candidate %d of %d tried)"), *Site.Spec.Id,
		*Site.BodyName.ToString(), *APSAncients::WhereText(Site.LocalUp).ToString(), Chosen, Site.NextCandidate);
	return true;
}

bool FAPSAncients::BuildSurface(FSite& Site)
{
	using namespace APSAncientsLocal;
	TRACE_CPUPROFILER_EVENT_SCOPE(APSAncients_BuildSurface);
	APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get());
	AWorldScapeRoot* Root = LoadedRoot(Body);
	if (!Body || !Root)
	{
		Site.Stage = FSite::EStage::Waiting;
		return false;
	}
	FWorldScapeGround Ground;
	Ground.Root = Root;
	Ground.BodyCentre = Root->GetActorLocation();
	SiteFrame(*Body, Site.LocalUp, APSAncientsSites::Yaw(Site.Spec.Seed), Ground.Up, Ground.Forward, Ground.Right);
	const double Scale = Root->PlanetScale;
	const double CentreRadius = Scale + Root->GetGroundHeight(Ground.BodyCentre + Ground.Up * Scale, false);
	Ground.Origin = Ground.BodyCentre + Ground.Up * CentreRadius;
	FAPSAncientShape Shape;
	APSAncientsGeometry::Build(Site.Spec, &Ground, Shape);
	Site.Metrics = Shape.Metrics;
	AActor* Actor = SpawnSiteActor(Site, Shape, Ground.Origin, FRotationMatrix::MakeFromXZ(Ground.Forward, Ground.Up).ToQuat());
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Ancients] %s: the site actor did not spawn"), *Site.Spec.Id);
		return false;
	}
	Site.Stage = FSite::EStage::Built;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Ancients] built %s (%s) on %s at %s: %s, %d stone + %d glow triangles, centre %.0f m from the base radius, nav point %.1f km up"),
		*Site.Spec.Id, APSAncients::KindLabel(Site.Spec.Kind), *Site.BodyName.ToString(), *APSAncients::WhereText(Site.LocalUp).ToString(),
		*APSAncients::LengthText(Site.Metrics.SizeCm).ToString(), Shape.Stone.NumTriangles(),
		Shape.Glow.NumTriangles() + Shape.Beacon.NumTriangles(), (CentreRadius - Scale) / 100.0, Site.Metrics.NavHeightCm / KmToCm);
	return true;
}

bool FAPSAncients::BuildOrbital(FSite& Site)
{
	using namespace APSAncientsLocal;
	TRACE_CPUPROFILER_EVENT_SCOPE(APSAncients_BuildOrbital);
	AActor* Body = Site.Body.Get();
	if (!Body)
	{
		return false;
	}
	FVector LocalDirection;
	double Radii = 2.0;
	APSAncientsSites::OrbitOf(Site.Spec.Seed, LocalDirection, Radii);
	const FTransform& BodyTransform = Body->GetActorTransform();
	const double OrbitCm = RadiusOf(Body) * Radii;
	const FVector Centre = Body->GetActorLocation() + BodyTransform.TransformVectorNoScale(LocalDirection).GetSafeNormal() * OrbitCm;
	const FQuat Rotation = BodyTransform.GetRotation() * APSAncientsSites::OrbitalTurn(Site.Spec.Seed);
	const FFlatGround Flat;
	FAPSAncientShape Shape;
	APSAncientsGeometry::Build(Site.Spec, &Flat, Shape);
	Site.Metrics = Shape.Metrics;
	Site.LocalUp = LocalDirection;
	Site.bResolved = true;
	AActor* Actor = SpawnSiteActor(Site, Shape, Centre, Rotation);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Ancients] %s: the site actor did not spawn"), *Site.Spec.Id);
		return false;
	}
	Site.Stage = FSite::EStage::Built;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] built %s (%s) in orbit of %s: %s long, %.2f radii out over %s, %d stone + %d glow triangles"),
		*Site.Spec.Id, APSAncients::KindLabel(Site.Spec.Kind), *Site.BodyName.ToString(), *APSAncients::LengthText(Site.Metrics.SizeCm).ToString(),
		Radii, *APSAncients::WhereText(LocalDirection).ToString(), Shape.Stone.NumTriangles(), Shape.Glow.NumTriangles() + Shape.Beacon.NumTriangles());
	return true;
}

AActor* FAPSAncients::SpawnSiteActor(FSite& Site, const FAPSAncientShape& Shape, const FVector& Centre, const FQuat& Rotation)
{
	using namespace APSAncientsLocal;
	UWorld* LiveWorld = World.Get();
	AActor* Body = Site.Body.Get();
	if (!LiveWorld || !Body)
	{
		return nullptr;
	}
	DestroyActor(Site);
	// The actor stands at the navigation point above the centre: fleet slots round it (1.5 km out) stay above the ground.
	const double Drop = Shape.Metrics.NavHeightCm;
	const FVector NavPoint = Centre + Rotation.GetAxisZ() * Drop;
	FActorSpawnParameters Parameters;
	// A stable name: fleet orders aimed at the site find it again after a load (FAPSFleetCommand::KeyOf).
	Parameters.Name = FName(*(TEXT("APS_Ancient_") + Site.Spec.Id));
	Parameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.ObjectFlags |= RF_Transient;
	// A technological actor: ship navigation lists it as a contact (a course can be set to it) and fleet MOVE orders take it.
	ATechInfrastructure* Actor = LiveWorld->SpawnActor<ATechInfrastructure>(ATechInfrastructure::StaticClass(),
		FTransform(Rotation, NavPoint), Parameters);
	if (!Actor)
	{
		return nullptr;
	}
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("AncientRoot"));
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Actor->SetActorLocationAndRotation(NavPoint, Rotation, false, nullptr, ETeleportType::TeleportPhysics);

	const bool bShadows = CVarShadows.GetValueOnGameThread() != 0;
	const float Glow = FMath::Max(CVarGlow.GetValueOnGameThread(), 0.0f);
	// Monuments are drawn out to a few radii of their world (they are seen from orbit); a small site only nearby.
	const float Cull = Site.Spec.Size == ESize::Small ? 2000000.0f
		: static_cast<float>(RadiusOf(Body) * (Site.Spec.IsOrbital() ? 8.0 : 5.0));
	if (!Shape.Stone.IsEmpty())
	{
		UProceduralMeshComponent* StoneMesh = MakeMesh(*Actor, *Root, TEXT("AncientStone"), Drop, true, bShadows, Cull);
		SetSection(*StoneMesh, 0, Shape.Stone, true, ShapeMaterial(*Actor, Shape.StoneColour, 0.0f));
	}
	if (Glow > 0.0f && (!Shape.Glow.IsEmpty() || !Shape.Beacon.IsEmpty()))
	{
		UProceduralMeshComponent* GlowMesh = MakeMesh(*Actor, *Root, TEXT("AncientGlow"), Drop, false, false, Cull);
		SetSection(*GlowMesh, 0, Shape.Glow, false, ShapeMaterial(*Actor, Shape.GlowColour, GlowStrength * Glow));
		SetSection(*GlowMesh, 1, Shape.Beacon, false, ShapeMaterial(*Actor, Shape.GlowColour, BeaconStrength * Glow));
	}
	// Riding the body: its turn and every world-origin shift carry the site along.
	Actor->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
	Actor->SetActorTickEnabled(false);
	Actor->Tags.Add(SiteTag);
	Actor->Tags.Add(FName(*(SiteTagPrefix + Site.Spec.Id)));
	Actor->Tags.Add(AnomalyTag);
#if WITH_EDITOR
	Actor->SetActorLabel(FString::Printf(TEXT("Ancient %s %s"), *Site.Spec.Id, APSAncients::KindLabel(Site.Spec.Kind)));
#endif
	Site.Actor = Actor;
	NameSite(Site);
	return Actor;
}

void FAPSAncients::DestroyActor(FSite& Site)
{
	if (AActor* Actor = Site.Actor.Get())
	{
		Actor->Destroy();
	}
	Site.Actor.Reset();
}

void FAPSAncients::NameSite(const FSite& Site) const
{
	AActor* Actor = Site.Actor.Get();
	if (!Actor)
	{
		return;
	}
	// While the chains run a site is unknown until its first survey (a chart for a nearby one; the circle as its chain
	// starts); without the chains it is named at once.
	const int32 KnownFrom = Site.Spec.Chain == APSAncients::EChain::Road ? 2 : Site.Spec.Chain == APSAncients::EChain::Circle ? 0 : 1;
	const bool bKnown = !bQuestsOpen || (Site.bStarted && Site.Step >= KnownFrom);
	APSAncientsLocal::SetInGameName(Actor, bKnown
		? FText::Format(LOCTEXT("SiteName", "ANCIENT SITE: {0}"), APSAncients::KindName(Site.Spec.Kind))
		: LOCTEXT("UnknownSite", "UNKNOWN STRUCTURE"));
}

const FAPSAncients::FSite* FAPSAncients::FindByActor(const AActor* Actor) const
{
	if (!Actor || !Actor->ActorHasTag(APSAncientsLocal::SiteTag))
	{
		return nullptr;
	}
	return Sites.FindByPredicate([Actor](const FSite& Site) { return Site.Actor.Get() == Actor; });
}

bool FAPSAncients::GetCentre(const FSite& Site, FVector& OutCentre) const
{
	const AActor* Actor = Site.Actor.Get();
	if (!Actor || Site.Stage != FSite::EStage::Built)
	{
		return false;
	}
	OutCentre = Actor->GetActorLocation() - Actor->GetActorUpVector() * Site.Metrics.NavHeightCm;
	return true;
}

bool FAPSAncients::QuestsAllowed() const
{
	const UWorld* LiveWorld = World.Get();
	const UGameInstance* GameInstance = LiveWorld ? LiveWorld->GetGameInstance() : nullptr;
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	// A generated civilization's game, once a load (if any) has been applied: the board then holds what the save had.
	return APSAncientsLocal::CVarQuests.GetValueOnGameThread() != 0 && Gameplay && Gameplay->bSpawnGeneratedCivilization
		&& !Gameplay->bIsLoadingMode && APSMissionsFind(LiveWorld) != nullptr;
}

void FAPSAncients::Post(const FText& Text) const
{
	UAPSCivilizationJournalSubsystem::Post(World.Get(), TEXT("Ancients"), Text);
}

void FAPSAncients::UpdateQuests(const float DeltaSeconds)
{
	FAPSMissionBoard* Board = APSMissionsFind(World.Get());
	if (!Board || !QuestsAllowed())
	{
		return;
	}
	if (!bQuestsOpen)
	{
		bQuestsOpen = true;
		QuestsOpenedAt = Now();
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] quest chains open (%d site(s))"), Sites.Num());
	}
	if (!bHydrated)
	{
		bHydrated = true;
		HydrateRewards(*Board);
	}
	BoardView.Refresh(*Board);
	for (FSite& Site : Sites)
	{
		UpdateChain(Site, *Board, DeltaSeconds);
	}
}

void FAPSAncients::HydrateRewards(FAPSMissionBoard& Board)
{
	TMap<FName, const APSAncientsQuests::FStep*> Steps;
	for (const FSite& Site : Sites)
	{
		const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
		for (int32 Index = 0; Index < Chain.Steps.Num(); ++Index)
		{
			Steps.Add(APSAncientsQuests::TemplateOf(Site.Spec, Index), &Chain.Steps[Index]);
		}
	}
	// So the tracker shows them and the board pays them when the step completes.
	const bool bChanged = APSAncientsQuests::Edit(Board, [&Steps](FAPSMission& Mission)
	{
		const APSAncientsQuests::FStep* const* Found = Steps.Find(Mission.Template);
		if (!Found || !Mission.Reward.IsEmpty() || (*Found)->Reward.IsEmpty())
		{
			return false;
		}
		Mission.Reward = (*Found)->Reward;
		Mission.RewardLevels = (*Found)->Levels;
		Mission.Unlocks = (*Found)->Unlocks ? FName((*Found)->Unlocks) : NAME_None;
		return true;
	});
	if (bChanged)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] rewards of the saved ancient missions restored"));
	}
}

int32 FAPSAncients::CompletedSteps(const FSite& Site) const
{
	const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
	int32 Done = 0;
	while (Done < Chain.Steps.Num() && BoardView.Completed.Contains(APSAncientsQuests::TemplateOf(Site.Spec, Done)))
	{
		++Done;
	}
	return Done;
}

bool FAPSAncients::IsChainDone(const APSAncients::EChain Chain, const FString& SiteId) const
{
	for (const FSite& Site : Sites)
	{
		if (Site.Spec.Chain == Chain && (SiteId.IsEmpty() || Site.Spec.Id == SiteId))
		{
			return CompletedSteps(Site) >= APSAncientsQuests::ChainOf(Chain).Steps.Num();
		}
	}
	return false;
}

void FAPSAncients::UpdateChain(FSite& Site, FAPSMissionBoard& Board, const float DeltaSeconds)
{
	using namespace APSAncientsLocal;
	using namespace APSAncientsQuests;
	const FChain& Chain = ChainOf(Site.Spec.Chain);
	const int32 Done = CompletedSteps(Site);
	if (!Site.bSynced)
	{
		// The first look (after a load: what the save holds): steps done before are not announced again.
		Site.bSynced = true;
		Site.bStarted = Done > 0 || BoardView.Open.Contains(TemplateOf(Site.Spec, 0));
		Site.Step = Site.bStarted ? Done : -1;
		NameSite(Site);
	}
	else if (Site.bStarted && Done > Site.Step)
	{
		OnStepsCompleted(Site, FMath::Max(Site.Step, 0), Done);
		Site.Step = Done;
		Site.bOpenSeen = false;
		Site.bNotified = false;
		Site.bForced = false;
		Site.ReofferAt = 0.0;
		Site.ScienceSeconds = Site.ExpeditionSeconds = Site.LingerSeconds = Site.DockSeconds = 0.0f;
		NameSite(Site);
	}
	if (Done >= Chain.Steps.Num())
	{
		return;
	}
	const FName Template = TemplateOf(Site.Spec, Done);
	if (const FAPSMission* Open = BoardView.Open.Find(Template))
	{
		Site.bOpenSeen = true;
		Site.ReofferAt = 0.0;
		if (Open->State != APSMissions::EState::Active || Site.bNotified || !StepMet(Site, Chain.Steps[Done], DeltaSeconds))
		{
			return;
		}
		// A load leaves our missions' rewards empty (the board fills rewards from its own templates): fill them first.
		const FStep& Definition = Chain.Steps[Done];
		if (Open->Reward.IsEmpty() && !Definition.Reward.IsEmpty())
		{
			Edit(Board, [&Template, &Definition](FAPSMission& Mission)
			{
				if (Mission.Template != Template || !Mission.Reward.IsEmpty())
				{
					return false;
				}
				Mission.Reward = Definition.Reward;
				Mission.RewardLevels = Definition.Levels;
				Mission.Unlocks = Definition.Unlocks ? FName(Definition.Unlocks) : NAME_None;
				return true;
			});
		}
		Board.Notify(Open->Objective, Open->Subject, 1);
		Site.bNotified = true;
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: step %d/%d done (%s)%s"), *Site.Spec.Id, Done + 1, Chain.Steps.Num(),
			*Definition.Title.ToString(), Site.bForced ? TEXT(", forced from the console") : TEXT(""));
		return;
	}
	if (!Site.bStarted)
	{
		if (StartConditionMet(Site))
		{
			StartChain(Site, Board);
		}
		return;
	}
	if (Site.bOpenSeen)
	{
		// It was on the board and went without completing: the player dropped it. It is offered again after a while, each
		// time later (2, 4, 8 ... at most 32 minutes), so a chain the player does not want stops pressing.
		if (Site.ReofferAt <= 0.0)
		{
			const double Delay = ReofferSeconds * static_cast<double>(1 << FMath::Clamp(Site.Drops, 0, 4));
			Site.ReofferAt = Now() + Delay;
			++Site.Drops;
			UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: step %d dropped; offered again in %.0f s"), *Site.Spec.Id, Done + 1, Delay);
			return;
		}
		if (Now() < Site.ReofferAt)
		{
			return;
		}
		Site.ReofferAt = 0.0;
		PutStep(Site, Board, APSMissions::EState::Offered);
		return;
	}
	// The chain's next step: one just completed, or a load that came back between two steps.
	PutStep(Site, Board, APSMissions::EState::Active);
}

bool FAPSAncients::StartConditionMet(const FSite& Site) const
{
	using namespace APSAncientsLocal;
	using APSAncients::EChain;
	if (bStartAll || Site.bForced)
	{
		return true;
	}
	FVector Centre;
	const bool bBuilt = GetCentre(Site, Centre);
	const APawn* Pawn = Pilot();
	const double PilotDistance = bBuilt && Pawn ? FVector::Dist(Pawn->GetActorLocation(), Centre) : TNumericLimits<double>::Max();
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	const AAstroGenerator* Gen = Generator.Get();
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get());
	// The home planet is known from the start: its survey says nothing new.
	const bool bBodySurveyed = Fleet && Body && (!Gen || Body != Gen->HomePlanet) && Fleet->GetSurvey(Body) >= APSFleet::ESurvey::Surveyed;
	const auto EchoesStepDone = [this](const int32 Step)
	{
		for (const FSite& Other : Sites)
		{
			if (Other.Spec.Chain == EChain::Echoes)
			{
				return BoardView.Completed.Contains(APSAncientsQuests::TemplateOf(Other.Spec, Step));
			}
		}
		return false;
	};
	switch (Site.Spec.Chain)
	{
	case EChain::Echoes:
		// The monument's signal comes in a little after the game starts, once the site stands.
		return bBuilt && Now() - QuestsOpenedAt >= CVarSignalDelay.GetValueOnGameThread();
	case EChain::QuietHull:
		// The monument's survey (its glyphs point up), or the pilot passing near the hull.
		return EchoesStepDone(0) || PilotDistance < HullNoticeCm;
	case EChain::Circle:
		return EchoesStepDone(1) || bBodySurveyed || PilotDistance < CircleNoticeCm;
	case EChain::LostWorks:
		return bBodySurveyed || PilotDistance < LostNoticeCm;
	case EChain::Road:
	{
		if (PilotDistance < RoadNoticeCm)
		{
			return true;
		}
		// The charts lead from one site to the next: the first nearby site after the monument, each next one after the last.
		const FSite* Previous = nullptr;
		for (const FSite& Other : Sites)
		{
			if (&Other == &Site)
			{
				break;
			}
			if (Other.Spec.Chain == EChain::Road)
			{
				Previous = &Other;
			}
		}
		return Previous ? CompletedSteps(*Previous) >= APSAncientsQuests::ChainOf(EChain::Road).Steps.Num()
			: IsChainDone(EChain::Echoes);
	}
	default:
		return false;
	}
}

bool FAPSAncients::StepMet(FSite& Site, const APSAncientsQuests::FStep& Step, const float DeltaSeconds)
{
	using namespace APSAncientsLocal;
	using namespace APSAncientsQuests;
	if (Site.bForced)
	{
		return true;
	}
	const uint16 Wanted = Step.Conditions;
	const bool bOrbital = Site.Spec.IsOrbital();
	FVector Centre;
	const bool bBuilt = GetCentre(Site, Centre);
	const APawn* Pawn = Pilot();
	const bool bPilotOnFoot = Pawn && Pawn->IsA<ACustomGravityCharacter>();
	const double PilotDistance = bBuilt && Pawn ? FVector::Dist(Pawn->GetActorLocation(), Centre) : TNumericLimits<double>::Max();
	if ((Wanted & PilotNear) && PilotDistance <= Step.NearKm * KmToCm)
	{
		return true;
	}
	// The fleet at the site: ships holding there under a MOVE order to it (the site's own actor).
	bool bAnyHolding = false;
	bool bScienceHolding = false;
	bool bExplorationHolding = false;
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	if (Fleet && bBuilt)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			if (Unit.Order == APSFleet::EOrder::Move && Unit.Phase == APSFleet::EPhase::Holding && Unit.Target.Get() == Site.Actor.Get())
			{
				bAnyHolding = true;
				bScienceHolding |= Unit.Division == APSFleet::EDivision::Science;
				bExplorationHolding |= Unit.Division == APSFleet::EDivision::Exploration;
			}
		}
	}
	if ((Wanted & FleetHold) && bAnyHolding)
	{
		return true;
	}
	Site.ScienceSeconds = bScienceHolding ? Site.ScienceSeconds + DeltaSeconds : 0.0f;
	if ((Wanted & ScienceHold) && Site.ScienceSeconds >= Step.HoldSeconds)
	{
		return true;
	}
	Site.ExpeditionSeconds = bExplorationHolding ? Site.ExpeditionSeconds + DeltaSeconds : 0.0f;
	if ((Wanted & Expedition) && Site.ExpeditionSeconds >= Step.ExpeditionSeconds)
	{
		return true;
	}
	Site.LingerSeconds = PilotDistance <= Step.LingerKm * KmToCm ? Site.LingerSeconds + DeltaSeconds : 0.0f;
	if ((Wanted & PilotLinger) && Site.LingerSeconds >= Step.LingerSeconds)
	{
		return true;
	}
	if ((Wanted & OnFoot) && !bOrbital && bPilotOnFoot && PilotDistance <= Site.Metrics.OnFootCm)
	{
		return true;
	}
	Site.DockSeconds = Pawn && !bPilotOnFoot && PilotDistance <= Step.DockKm * KmToCm ? Site.DockSeconds + DeltaSeconds : 0.0f;
	if ((Wanted & Dock) && bOrbital && Site.DockSeconds >= Step.DockSeconds)
	{
		return true;
	}
	// What the fleet learned of the site's world (never the home planet: it is known from the start).
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get());
	const AAstroGenerator* Gen = Generator.Get();
	if (Fleet && Body && (!Gen || Body != Gen->HomePlanet))
	{
		const APSFleet::ESurvey Survey = Fleet->GetSurvey(Body);
		if (((Wanted & BodySurveyed) && Survey >= APSFleet::ESurvey::Surveyed) || ((Wanted & BodyStudied) && Survey >= APSFleet::ESurvey::Studied))
		{
			return true;
		}
	}
	if ((Wanted & SystemSurveyed) && Site.Spec.SystemId.IsValid())
	{
		const FAPSStarSystems* Stars = APSStarSystemsFind(World.Get());
		if (Stars && Stars->GetKnowledge(Site.Spec.SystemId) >= APSStars::EKnowledge::Surveyed)
		{
			return true;
		}
	}
	return false;
}

void FAPSAncients::StartChain(FSite& Site, FAPSMissionBoard& Board)
{
	const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
	Site.bStarted = true;
	Site.bForced = false;
	Site.Step = 0;
	// The Builders' chart marks a nearby site's system: scanned, its beacon on the maps and the navigation list.
	if (Site.Spec.Chain == APSAncients::EChain::Road && Site.Spec.SystemId.IsValid())
	{
		if (FAPSStarSystems* Stars = APSStarSystemsFind(World.Get()))
		{
			if (Stars->GetKnowledge(Site.Spec.SystemId) < APSStars::EKnowledge::Scanned)
			{
				Stars->Learn(Site.Spec.SystemId, APSStars::EKnowledge::Scanned, LOCTEXT("ChartHow", "read from the Builders' chart"));
			}
			Stars->GetAnchor(Site.Spec.SystemId);
		}
	}
	Post(FText::Format(Chain.Signal, ArgsFor(Site, nullptr)));
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: chain %s starts"), *Site.Spec.Id, *APSAncients::ChainName(Site.Spec.Chain).ToString());
	PutStep(Site, Board, APSMissions::EState::Active);
	NameSite(Site);
}

void FAPSAncients::PutStep(FSite& Site, FAPSMissionBoard& Board, const APSMissions::EState State)
{
	const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
	const int32 Index = FMath::Max(Site.Step, 0);
	if (!Chain.Steps.IsValidIndex(Index))
	{
		return;
	}
	Site.bOpenSeen = true;
	Site.bNotified = false;
	Site.ScienceSeconds = Site.ExpeditionSeconds = Site.LingerSeconds = Site.DockSeconds = 0.0f;
	const FName Template = APSAncientsQuests::TemplateOf(Site.Spec, Index);
	if (Board.GetMissions().ContainsByPredicate([&Template](const FAPSMission& Existing)
	{
		return Existing.Template == Template
			&& (Existing.State == APSMissions::EState::Offered || Existing.State == APSMissions::EState::Active);
	}))
	{
		return;
	}
	const FAPSMission Mission = APSAncientsQuests::MakeMission(Site.Spec, Index, ArgsFor(Site, &Chain.Steps[Index]),
		SubjectNameFor(Site, Index), Now(), State);
	APSAncientsQuests::Inject(Board, Mission, true);
	Post(FText::Format(Index > 0 && State == APSMissions::EState::Active
			? LOCTEXT("StepNext", "{0} next: {1}. {2}")
			: LOCTEXT("StepOpen", "{0} — {1}. {2}"),
		APSInfrastructure::DepartmentName(Chain.Department), Mission.Title, Mission.Brief));
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: step %d/%d on the board as %s (%s)"), *Site.Spec.Id, Index + 1, Chain.Steps.Num(),
		State == APSMissions::EState::Active ? TEXT("active") : TEXT("an offer"), *Mission.Template.ToString());
}

void FAPSAncients::OnStepsCompleted(FSite& Site, const int32 From, const int32 To)
{
	const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
	for (int32 Index = From; Index < To && Chain.Steps.IsValidIndex(Index); ++Index)
	{
		const APSAncientsQuests::FStep& Step = Chain.Steps[Index];
		const FText Done = FText::Format(Step.Done, ArgsFor(Site, &Step));
		if (!Done.IsEmpty())
		{
			Post(Done);
		}
	}
	if (To >= Chain.Steps.Num())
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: chain %s complete"), *Site.Spec.Id, *APSAncients::ChainName(Site.Spec.Chain).ToString());
	}
}

FFormatNamedArguments FAPSAncients::ArgsFor(const FSite& Site, const APSAncientsQuests::FStep* Step) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Site"), APSAncients::KindName(Site.Spec.Kind));
	Args.Add(TEXT("Body"), !Site.BodyName.IsEmpty() ? Site.BodyName
		: !Site.SystemName.IsEmpty() ? Site.SystemName : LOCTEXT("UnknownWorld", "an unknown world"));
	Args.Add(TEXT("Where"), Site.bResolved ? APSAncients::WhereText(Site.LocalUp) : LOCTEXT("WhereUnknown", "a place not yet pinned down"));
	Args.Add(TEXT("Size"), APSAncients::LengthText(Site.Metrics.SizeCm));
	Args.Add(TEXT("Description"), APSAncients::KindDescription(Site.Spec.Kind, Site.Metrics));
	Args.Add(TEXT("Story"), APSAncients::KindStory(Site.Spec.Kind));
	Args.Add(TEXT("System"), Site.SystemName);
	// The last step of a nearby chain names the next charted system, if any.
	FText Next = FText::GetEmpty();
	bool bAfter = false;
	for (const FSite& Other : Sites)
	{
		if (&Other == &Site)
		{
			bAfter = true;
			continue;
		}
		if (bAfter && Other.Spec.Chain == APSAncients::EChain::Road && Site.Spec.Chain == APSAncients::EChain::Road)
		{
			Next = FText::Format(LOCTEXT("NextChart", "Another of its glyphs circles {0}."), Other.SystemName);
			break;
		}
	}
	Args.Add(TEXT("Next"), Next);
	if (Step)
	{
		Args.Add(TEXT("NearKm"), FText::AsNumber(Step->NearKm));
		Args.Add(TEXT("LingerKm"), FText::AsNumber(Step->LingerKm));
		Args.Add(TEXT("LingerSeconds"), FText::AsNumber(Step->LingerSeconds));
		Args.Add(TEXT("HoldSeconds"), FText::AsNumber(Step->HoldSeconds));
		Args.Add(TEXT("ExpeditionSeconds"), FText::AsNumber(Step->ExpeditionSeconds));
		Args.Add(TEXT("DockKm"), FText::AsNumber(Step->DockKm));
		Args.Add(TEXT("DockSeconds"), FText::AsNumber(Step->DockSeconds));
		const FText Reach = FText::Format(Site.Spec.IsOrbital()
			? LOCTEXT("ReachOrbit", "Bring your ship within {DockKm} km of {Site} and hold for {DockSeconds} s")
			: LOCTEXT("ReachSurface", "Land and walk to {Site} on {Body}"), Args);
		Args.Add(TEXT("Reach"), Reach);
	}
	return Args;
}

FText FAPSAncients::SubjectNameFor(const FSite& Site, const int32 Step) const
{
	if (Site.Spec.Chain == APSAncients::EChain::Road && Step == 0)
	{
		return Site.SystemName;
	}
	const FText Where = !Site.BodyName.IsEmpty() ? Site.BodyName : Site.SystemName;
	return Where.IsEmpty() ? APSAncients::KindName(Site.Spec.Kind)
		: FText::Format(LOCTEXT("SubjectName", "{0}  /  {1}"), APSAncients::KindName(Site.Spec.Kind), Where);
}

void FAPSAncients::GatherActions(const FSite& Site, AActor* Object, TArray<FAPSObjectAction>& OutActions) const
{
	using namespace APSAncientsLocal;
	const FText Group = LOCTEXT("GroupAncients", "ANCIENTS");
	const int32 KnownFrom = Site.Spec.Chain == APSAncients::EChain::Road ? 2 : Site.Spec.Chain == APSAncients::EChain::Circle ? 0 : 1;
	const bool bKnown = !bQuestsOpen || (Site.bStarted && Site.Step >= KnownFrom);
	{
		FAPSObjectAction About;
		About.Id = TEXT("Ancients.About");
		About.Label = bKnown ? APSAncients::KindName(Site.Spec.Kind) : LOCTEXT("UnknownSite", "UNKNOWN STRUCTURE");
		About.Group = Group;
		About.Colour = AncientColour;
		const FText Description = bKnown
			? FText::Format(LOCTEXT("AboutKnown", "{0} on {1}: {2}"), APSAncients::KindName(Site.Spec.Kind), Site.BodyName,
				APSAncients::KindDescription(Site.Spec.Kind, Site.Metrics))
			: FText::Format(LOCTEXT("AboutUnknown", "Something built, {0} of it, older than any record. Fly close or send a ship to learn more."),
				APSAncients::LengthText(Site.Metrics.SizeCm));
		About.Detail = Description;
		About.Execute = [Description]() { return Description; };
		OutActions.Add(MoveTemp(About));
	}
	const FAPSFleetCommand* Fleet = APSFleetFind(World.Get());
	if (!Fleet || !Object)
	{
		return;
	}
	const TWeakObjectPtr<UWorld> WeakWorld = World;
	const auto AddOrder = [&](const FName Id, const FText& Label, const APSFleet::EDivision Division, const FText& What)
	{
		// The free (or holding) ship of the division nearest the site that may fly there.
		ASpaceship* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		FText Refusal;
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			ASpaceship* Ship = Unit.Ship.Get();
			if (!Ship || Unit.Division != Division || (Unit.Order != APSFleet::EOrder::None && Unit.Phase != APSFleet::EPhase::Holding))
			{
				continue;
			}
			if (Unit.Order == APSFleet::EOrder::Move && Unit.Target.Get() == Object)
			{
				Refusal = FText::Format(LOCTEXT("AlreadyThere", "{0} is there already."), FText::FromString(Unit.CallSign));
				continue;
			}
			const FText Why = Fleet->CheckOrder(Ship, APSFleet::EOrder::Move, Object);
			if (!Why.IsEmpty())
			{
				if (Refusal.IsEmpty())
				{
					Refusal = Why;
				}
				continue;
			}
			const double Distance = FVector::DistSquared(Ship->GetActorLocation(), Object->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Ship;
			}
		}
		FAPSObjectAction Action;
		Action.Id = Id;
		Action.Label = Label;
		Action.Group = Group;
		Action.Colour = AncientColour;
		Action.bEnabled = Best != nullptr;
		const FAPSFleetUnit* Chosen = Best ? Fleet->FindUnit(Best) : nullptr;
		Action.Detail = Chosen
			? FText::Format(LOCTEXT("OrderDetail", "{0} ({1}) flies there and holds over it: {2}"), FText::FromString(Chosen->CallSign),
				APSFleet::DivisionName(Division), What)
			: !Refusal.IsEmpty() ? Refusal : FText::Format(LOCTEXT("NoShip", "No free {0} ship."), APSFleet::DivisionName(Division));
		const TWeakObjectPtr<AActor> WeakObject = Object;
		const TWeakObjectPtr<ASpaceship> WeakShip = Best;
		Action.Execute = [WeakWorld, WeakObject, WeakShip]()
		{
			FAPSFleetCommand* Command = APSFleetFind(WeakWorld.Get());
			ASpaceship* Ship = WeakShip.Get();
			if (!Command || !Ship || !WeakObject.IsValid())
			{
				return LOCTEXT("OrderGone", "The ship or the site is gone.");
			}
			FText Refused;
			if (Command->IssueOrder({Ship}, APSFleet::EOrder::Move, WeakObject.Get(), Refused) <= 0)
			{
				return Refused;
			}
			const FAPSFleetUnit* Unit = Command->FindUnit(Ship);
			return FText::Format(LOCTEXT("OrderGiven", "{0}: on its way."), Unit ? FText::FromString(Unit->CallSign) : FText::GetEmpty());
		};
		OutActions.Add(MoveTemp(Action));
	};
	AddOrder(TEXT("Ancients.Study"), LOCTEXT("Study", "STUDY IT (SCIENCE SHIP)"), APSFleet::EDivision::Science,
		LOCTEXT("StudyWhat", "half a minute there reads the site."));
	AddOrder(TEXT("Ancients.Expedition"), LOCTEXT("Expedition", "SEND AN EXPEDITION (EXPLORATION SHIP)"), APSFleet::EDivision::Exploration,
		LOCTEXT("ExpeditionWhat", "after a minute its crew goes down."));
}

int32 FAPSAncients::FindIndex(const FString& Text) const
{
	if (Text.IsNumeric())
	{
		const int32 Index = FCString::Atoi(*Text);
		return Sites.IsValidIndex(Index) ? Index : INDEX_NONE;
	}
	return Sites.IndexOfByPredicate([&Text](const FSite& Site) { return Site.Spec.Id.Contains(Text, ESearchCase::IgnoreCase); });
}

void FAPSAncients::LogSites() const
{
	using namespace APSAncientsLocal;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %d site(s), world seed %d, quests %s"), Sites.Num(), WorldSeed,
		bQuestsOpen ? TEXT("open") : TEXT("not open"));
	const APawn* Pawn = Pilot();
	for (int32 Index = 0; Index < Sites.Num(); ++Index)
	{
		const FSite& Site = Sites[Index];
		FVector Centre;
		const bool bBuilt = GetCentre(Site, Centre);
		const APSAncientsQuests::FChain& Chain = APSAncientsQuests::ChainOf(Site.Spec.Chain);
		const FString Where = Site.bResolved ? APSAncients::WhereText(Site.LocalUp).ToString() : FString(TEXT("-"));
		const FString Distance = bBuilt && Pawn
			? FString::Printf(TEXT("%.1f km from the pilot"), FVector::Dist(Pawn->GetActorLocation(), Centre) / KmToCm) : FString(TEXT("-"));
		const FString Quest = !Site.bSynced ? FString(TEXT("not synced"))
			: !Site.bStarted ? FString(TEXT("not started"))
			: Site.Step >= Chain.Steps.Num() ? FString(TEXT("complete"))
			: FString::Printf(TEXT("step %d/%d %s"), Site.Step + 1, Chain.Steps.Num(), *Chain.Steps[FMath::Max(Site.Step, 0)].Title.ToString());
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] #%d %s %s %s x%.2f | %s%s | %s at %s, %s, %s | %s: %s"), Index, *Site.Spec.Id,
			APSAncients::KindLabel(Site.Spec.Kind), *APSAncients::SizeName(Site.Spec.Size).ToString(), Site.Spec.Scale,
			Site.BodyName.IsEmpty() ? TEXT("system ") : TEXT("on "),
			Site.BodyName.IsEmpty() ? *Site.SystemName.ToString() : *Site.BodyName.ToString(), StageName(Site.Stage), *Where,
			*APSAncients::LengthText(Site.Metrics.SizeCm).ToString(), *Distance, *APSAncients::ChainName(Site.Spec.Chain).ToString(), *Quest);
	}
}

bool FAPSAncients::Teleport(const int32 Index)
{
	using namespace APSAncientsLocal;
	UWorld* LiveWorld = World.Get();
	APlayerController* Controller = LiveWorld ? LiveWorld->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !Sites.IsValidIndex(Index))
	{
		return false;
	}
	// Rio 06.10 (still ship): the target is a world place; a ship owing its travel pays it first.
	if (UAPSWorldOriginSubsystem* Origin = LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
	{
		Origin->SettleDeferredTravel(TEXT("an ancients teleport"));
	}
	const FSite& Site = Sites[Index];
	ACustomGravityCharacter* Character = Cast<ACustomGravityCharacter>(Pawn);
	FVector Target;
	FVector LookAt;
	FVector Up = (Pawn->GetActorLocation() - FVector::ZeroVector).GetSafeNormal();
	FVector Centre;
	if (GetCentre(Site, Centre))
	{
		const AActor* Actor = Site.Actor.Get();
		Up = Actor->GetActorUpVector();
		const FVector Forward = Actor->GetActorForwardVector();
		if (Site.Spec.IsOrbital())
		{
			FVector Away = (Pawn->GetActorLocation() - Centre).GetSafeNormal();
			Away = Away.IsNearlyZero() ? Up : Away;
			Target = Centre + Away * (Site.Metrics.SizeCm * 1.2 + 300000.0);
			LookAt = Centre;
		}
		else if (Character)
		{
			// On foot: beside the site, a little above its ground so the character settles onto the terrain.
			Target = Centre + Forward * (Site.Metrics.OnFootCm * 1.4) + Up * 3000.0;
			if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Site.Body.Get()))
			{
				if (AWorldScapeRoot* Root = LoadedRoot(Body))
				{
					const FVector BodyCentre = Root->GetActorLocation();
					const FVector Direction = (Target - BodyCentre).GetSafeNormal();
					const double Scale = Root->PlanetScale;
					Target = BodyCentre + Direction * (Scale + Root->GetGroundHeight(BodyCentre + Direction * Scale, false) + 600.0);
					Up = Direction;
				}
			}
			LookAt = Centre + Up * FMath::Min(Site.Metrics.HeightCm * 0.5, 200000.0);
		}
		else
		{
			// In a ship: off the site's front and above it, looking at it.
			Target = Centre + Forward * (Site.Metrics.FootprintCm * 1.6 + 300000.0) + Up * (Site.Metrics.HeightCm + 200000.0);
			LookAt = Centre + Up * (Site.Metrics.HeightCm * 0.5);
		}
	}
	else if (const AActor* Body = Site.Body.Get())
	{
		// Not built yet: three radii out from its world, so the world's surface loads and the site finds its place.
		FVector Away = (Pawn->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal();
		Away = Away.IsNearlyZero() ? FVector::UpVector : Away;
		Target = Body->GetActorLocation() + Away * RadiusOf(Body) * 3.0;
		LookAt = Body->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] teleport: %s is not built yet; to %s first (teleport again once it is built)"),
			*Site.Spec.Id, *Site.BodyName.ToString());
	}
	else
	{
		// A nearby system that does not stand yet: to its edge, where the materializer builds it.
		FAPSStarSystems* Stars = APSStarSystemsFind(LiveWorld);
		FAPSSystemMaterializer* Materializer = Stars ? Stars->GetMaterializer() : nullptr;
		const bool bVisited = Materializer && Materializer->VisitForTest(*Stars, Site.Spec.SystemIndex, false);
		UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] teleport: %s's system %s %s"), *Site.Spec.Id, *Site.SystemName.ToString(),
			bVisited ? TEXT("- at its edge now; teleport again once its worlds stand") : TEXT("cannot be reached from here"));
		return bVisited;
	}
	const FVector Facing = (LookAt - Target).GetSafeNormal();
	if (ASpaceship* Ship = Cast<ASpaceship>(Pawn))
	{
		FAPSShipFlightBenchmark::SetKinematicVelocity(*Ship, FVector::ZeroVector);
		FVector ShipUp = FVector::VectorPlaneProject(Up, Facing).GetSafeNormal();
		ShipUp = ShipUp.IsNearlyZero() ? FVector::VectorPlaneProject(FVector::UpVector, Facing).GetSafeNormal() : ShipUp;
		Pawn->SetActorLocationAndRotation(Target, FAPSShipFlightBenchmark::GetRotationForFlightAxes(*Ship, Facing, ShipUp), false, nullptr,
			ETeleportType::TeleportPhysics);
	}
	else
	{
		if (Character && Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		const FVector Flat = FVector::VectorPlaneProject(Facing, Up).GetSafeNormal();
		Pawn->SetActorLocationAndRotation(Target, FRotationMatrix::MakeFromZX(Up, Flat.IsNearlyZero() ? Facing : Flat).ToQuat(), false,
			nullptr, ETeleportType::TeleportPhysics);
	}
	Controller->SetControlRotation(Facing.Rotation());
	// Keep the pilot near the origin as the save and spawn paths do (the engine's precision is best there).
	if (UAPSWorldOriginSubsystem* Origin = LiveWorld->GetSubsystem<UAPSWorldOriginSubsystem>())
	{
		Origin->RebaseOnto(Pawn->GetActorLocation(), TEXT("ancients teleport"));
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] teleport: %s to %s (%s)"), *Pawn->GetName(), *Site.Spec.Id,
		Character ? TEXT("on foot") : TEXT("aboard"));
	return true;
}

bool FAPSAncients::Advance(const int32 Index)
{
	if (!Sites.IsValidIndex(Index))
	{
		return false;
	}
	FSite& Site = Sites[Index];
	Site.bForced = true;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] %s: %s on the next tick (console)"), *Site.Spec.Id,
		Site.bStarted ? TEXT("the active step completes") : TEXT("the chain starts"));
	return true;
}

void FAPSAncients::StartAll()
{
	bStartAll = true;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] every chain starts now (console)"));
}

void FAPSAncients::Rebuild()
{
	int32 Count = 0;
	for (FSite& Site : Sites)
	{
		if (Site.Stage == FSite::EStage::Built)
		{
			DestroyActor(Site);
			Site.Stage = FSite::EStage::Waiting;
			++Count;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Ancients] rebuilding %d site(s)"), Count);
}

#undef LOCTEXT_NAMESPACE

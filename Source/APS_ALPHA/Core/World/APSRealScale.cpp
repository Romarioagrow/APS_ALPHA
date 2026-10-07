#include "APSRealScale.h"

#include "APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyPlinth.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipHullComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DelayedAutoRegister.h"

namespace APSRealScaleLocal
{
	constexpr double LightYearCm = 9.4607304725808e17;
	constexpr double ParsecCm = 3.0856775814913673e18;

	/** The game world's gameplay generator, found once it is final and kept while it lives. */
	struct FGeneratorCache
	{
		TWeakObjectPtr<const UWorld> World;
		TWeakObjectPtr<AAstroGenerator> Generator;
		double NextScanSeconds{0.0};
	};
	FGeneratorCache GGeneratorCache;

	/**
	 * Rio 05.10 (real scale, stage 2): the home system and a materialized one stay where they are when the pilot flies away,
	 * parsecs out. Their stations, headquarters, colony and other tech, and their parked ships, are hidden, without
	 * collision and (the tech) without actor tick while the pilot is far beyond them, and come back as the pilot returns;
	 * nothing is moved. Stars, planets and moons are never touched (their surfaces and air stream by themselves, and from
	 * afar a materialized star keeps its catalogue glyph while it is shown).
	 */
	// 0.5: beyond where a materialized system is let go (aps.RealScale.MaterializeLy x 1.5), so only a system the pilot
	// leaves faster than that (and the home) is ever frozen.
	TAutoConsoleVariable<float> CVarFreezeLy(
		TEXT("aps.RealScale.FreezeLy"), 0.5f,
		TEXT("Rio 05.10 (REAL SCALE, stage 2): a star system's stations, tech and parked ships are hidden, without collision ")
		TEXT("and without actor tick while the pilot is farther than this many light years beyond the system's own extent; ")
		TEXT("they come back as the pilot returns (stars, planets and moons are never touched). 0: never. Worlds without REAL ")
		TEXT("SCALE never freeze."));
	constexpr float FreezeIntervalSeconds = 0.5f;
	/** A frozen system comes back a fifth of the way inside the line it froze at (a pilot on the line does not flip it). */
	constexpr double ThawShare = 0.8;
	/** While the world flows past the pilot, a frozen system comes back only within this of one of its frozen actors (1e6 km). */
	constexpr double ThawNearCm = 1.0e11;

	/** What the freeze changed on one actor, and only that is given back. */
	struct FFrozenActor
	{
		bool bHid{false};
		bool bCollision{false};
		bool bTick{false};
		/** Rio 05.10 afternoon (flight FPS at speed): the primitives the freeze took out of the scene and physics. */
		TArray<TWeakObjectPtr<UPrimitiveComponent>> Unregistered;
	};

	struct FFreezeState
	{
		TWeakObjectPtr<UWorld> World;
		TMap<TWeakObjectPtr<AActor>, FFrozenActor> Actors;
		TSet<TWeakObjectPtr<AStarSystem>> Systems;
	};
	FFreezeState GFreeze;

	/**
	 * Stations, headquarters, colony and other tech, and what the tech is built of. Rio 05.10 afternoon (flight FPS): the
	 * colony's plinths (20 bodies at home, each teleported on every world shift; they tick nothing), the colony's
	 * headquarters (a child actor of the colony, 16 bodies) and its motor pool (8) freeze like the tech.
	 */
	bool IsFreezableTech(const AActor& Actor)
	{
		const auto IsTech = [](const AActor& Each)
		{
			return Each.IsA<ATechActor>() || Each.IsA<AAPSColonyPlinth>();
		};
		const AActor* Parent = Actor.GetParentActor();
		return IsTech(Actor) || (Parent && IsTech(*Parent)) || Actor.ActorHasTag(TEXT("APS.Colony.MotorPool"));
	}

	UWorld* FindGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void Restore(AActor& Actor, const FFrozenActor& State)
	{
		// Collision first: a primitive registered while its actor's collision is off would come back without a body.
		if (State.bCollision)
		{
			Actor.SetActorEnableCollision(true);
		}
		for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : State.Unregistered)
		{
			if (UPrimitiveComponent* Component = Primitive.Get(); Component && !Component->IsRegistered())
			{
				Component->RegisterComponent();
			}
		}
		if (State.bHid)
		{
			Actor.SetActorHiddenInGame(false);
		}
		if (State.bTick)
		{
			Actor.SetActorTickEnabled(true);
		}
	}

	void RestoreAll(const TCHAR* Reason)
	{
		int32 Count = 0;
		for (const TPair<TWeakObjectPtr<AActor>, FFrozenActor>& Entry : GFreeze.Actors)
		{
			if (AActor* Actor = Entry.Key.Get(); IsValid(Actor))
			{
				Restore(*Actor, Entry.Value);
				++Count;
			}
		}
		if (Count > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.RealScale] far systems: %d actors back (%s)"), Count, Reason);
		}
		GFreeze.Actors.Reset();
		GFreeze.Systems.Reset();
	}

	bool TickFreeze(float)
	{
		UWorld* World = FindGameWorld();
		if (GFreeze.World.Get() != World)
		{
			// Another world (a travel): the actors of the last one went with it.
			GFreeze = FFreezeState();
			GFreeze.World = World;
		}
		const double FreezeCm = static_cast<double>(CVarFreezeLy.GetValueOnGameThread()) * LightYearCm;
		if (!World || !(FreezeCm > 0.0) || !APSRealScale::IsActive(World))
		{
			if (!GFreeze.Actors.IsEmpty() || !GFreeze.Systems.IsEmpty())
			{
				RestoreAll(TEXT("freeze off"));
			}
			return true;
		}
		const APlayerController* Controller = World->GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			return true;
		}
		const FVector Pilot = Pawn->GetActorLocation();
		const UAPSWorldOriginSubsystem* Origin = World->GetSubsystem<UAPSWorldOriginSubsystem>();
		// The pilot's own chain (the ship flown or walked in, what carries it, who rides along) is never frozen.
		TSet<const AActor*> Exempt;
		for (const AActor* Link = Pawn; Link; Link = Link->GetAttachParentActor())
		{
			Exempt.Add(Link);
		}
		TArray<AActor*> Riders;
		Pawn->GetAttachedActors(Riders, true, true);
		for (const AActor* Rider : Riders)
		{
			Exempt.Add(Rider);
		}

		TSet<TWeakObjectPtr<AActor>> Wanted;
		TSet<TWeakObjectPtr<AStarSystem>> FrozenSystems;
		TArray<AActor*> Tree;
		for (TActorIterator<AStarSystem> It(World); It; ++It)
		{
			AStarSystem* System = *It;
			if (!IsValid(System))
			{
				continue;
			}
			const FVector Centre = System->GetActorLocation();
			// Rio 06.10 (still ship, the review): measured from where the pilot sees the system while a debt is owed.
			const FVector SeenCentre = UAPSWorldOriginSubsystem::SkyPlace(*System);
			System->GetAttachedActors(Tree, true, true);
			// The system's own extent: its bodies, stations and ships. A surface generator kept where it was spawned (the
			// world origin, by the pilot then) once made the extent the pilot's own distance (Rio's 05.10 run, "line 2.7 ly").
			double Extent = 0.0;
			for (const AActor* Member : Tree)
			{
				if (IsValid(Member) && (Member->IsA<AWorldActor>() || Member->IsA<ASpaceship>()))
				{
					Extent = FMath::Max(Extent, FVector::Dist(Member->GetActorLocation(), Centre));
				}
			}
			const double Distance = FVector::Dist(Pilot, SeenCentre);
			const bool bWasFrozen = GFreeze.Systems.Contains(System);
			const double Line = FreezeCm + Extent;
			// Rio 05.10 evening (flight FPS, the way home): thawed while the world still flowed past the ship, the base's
			// bodies and instanced meshes went with every per-frame shift for the last ~25 s of the approach (+3-4 ms a
			// frame). A frozen system the pilot returns to stays frozen until the flow stops or the pilot is near one of
			// its frozen actors (none of them shows from that far).
			bool bHoldFrozen = bWasFrozen && Origin && Origin->IsWorldFlowing();
			for (AActor* Member : Tree)
			{
				if (bHoldFrozen && IsValid(Member) && GFreeze.Actors.Contains(Member)
					&& FVector::DistSquared(Pilot, UAPSWorldOriginSubsystem::SkyPlace(*Member)) < FMath::Square(ThawNearCm))
				{
					bHoldFrozen = false;
				}
			}
			if ((!bHoldFrozen && Distance <= (bWasFrozen ? ThawShare * Line : Line)) || Exempt.Contains(System)
				|| Tree.ContainsByPredicate([&Exempt](const AActor* Member) { return Exempt.Contains(Member); }))
			{
				continue;
			}
			FrozenSystems.Add(System);
			Tree.Add(System);
			int32 NewlyFrozen = 0;
			for (AActor* Member : Tree)
			{
				// Tech (IsFreezableTech) and parked ships. Never a star, planet, moon or orbit (Rio saw moons "half loaded"
				// after a long flight, 05.10: their streaming and air keep their own state), and nothing of a plugin
				// (surfaces, atmospheres).
				const bool bTech = IsValid(Member) && IsFreezableTech(*Member);
				if (!IsValid(Member) || (!bTech && !Member->IsA<ASpaceship>()))
				{
					continue;
				}
				Wanted.Add(Member);
				if (GFreeze.Actors.Contains(Member))
				{
					continue;
				}
				FFrozenActor State;
				State.bHid = !Member->IsHidden();
				State.bCollision = Member->GetActorEnableCollision();
				// A ship's tick also flies fleet orders; only the tech stops ticking.
				State.bTick = bTech && Member->IsActorTickEnabled();
				if (State.bHid)
				{
					Member->SetActorHiddenInGame(true);
				}
				if (State.bCollision)
				{
					Member->SetActorEnableCollision(false);
				}
				// Rio 05.10 afternoon ("FPS at high speed is low": a trace at 1e10 c, ~1.4 ms a frame teleporting the far home
				// base's physics bodies on every world shift, more in the editor). Hidden and without collision, its bodies
				// still existed (an editor world keeps them for traces); its primitives leave the scene and physics until the
				// thaw registers them again, where the actor then is.
				Member->ForEachComponent<UPrimitiveComponent>(false, [&State](UPrimitiveComponent* Primitive)
				{
					// Rio 07.10 (aps.Ship.HullHold): a held hull body is inert and gets no transform sends, world shifts
					// included; the actor's collision switch filters it out. Unregistering it would destroy it and the thaw
					// would build it again in one synchronous InitBody, the freeze the hold exists to avoid.
					if (const UAPSShipHullComponent* Hull = Cast<UAPSShipHullComponent>(Primitive); Hull && Hull->IsBodyHeld())
					{
						return;
					}
					if (Primitive->IsRegistered() && Primitive->IsPhysicsStateCreated())
					{
						State.Unregistered.Add(Primitive);
						Primitive->UnregisterComponent();
					}
				});
				if (State.bTick)
				{
					Member->SetActorTickEnabled(false);
				}
				GFreeze.Actors.Add(Member, State);
				++NewlyFrozen;
			}
			if (!bWasFrozen || NewlyFrozen > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.RealScale] far system %s frozen: %d actors (%d new), pilot %.3f ly away, line %.3f ly"),
					*GetNameSafe(System), Tree.Num(), NewlyFrozen, Distance / LightYearCm, Line / LightYearCm);
			}
		}
		int32 Restored = 0;
		for (auto Entry = GFreeze.Actors.CreateIterator(); Entry; ++Entry)
		{
			if (Wanted.Contains(Entry.Key()))
			{
				continue;
			}
			if (AActor* Actor = Entry.Key().Get(); IsValid(Actor))
			{
				Restore(*Actor, Entry.Value());
				++Restored;
			}
			Entry.RemoveCurrent();
		}
		if (Restored > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.RealScale] far systems: %d actors back as the pilot returns (%d system(s) still frozen)"),
				Restored, FrozenSystems.Num());
		}
		GFreeze.Systems = MoveTemp(FrozenSystems);
		return true;
	}

	FDelayedAutoRegisterHelper GFreezeRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickFreeze), FreezeIntervalSeconds);
	});

	FAutoConsoleCommandWithWorld ReportCommand(
		TEXT("aps.RealScale.Report"),
		TEXT("Rio 05.10: logs whether this game world is REAL SCALE, its layout, the pilot's distance from home and the frozen far systems."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld)
		{
			UWorld* World = InWorld && InWorld->IsGameWorld() ? InWorld : FindGameWorld();
			const AAstroGenerator* Generator = APSRealScale::FindGameplayGenerator(World);
			double GalaxyCm = 0.0;
			double ClusterCm = 0.0;
			double NeighbourCm = 0.0;
			if (Generator)
			{
				Generator->GetRealScaleSummary(GalaxyCm, ClusterCm, NeighbourCm);
			}
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			const AActor* Home = Generator ? Generator->GetPreviewHomeSystem() : nullptr;
			const UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr;
			UE_LOG(LogTemp, Log,
				TEXT("[APS.RealScale] report: active=%d generator=%s | galaxy radius %.0f pc, cluster R90 %.1f pc, neighbours %.2f pc | ")
				TEXT("pilot %.4f ly from home, %.1f km from 0,0,0 | generation offset %.4f ly | %d far system(s) frozen, %d actors"),
				APSRealScale::IsActive(World) ? 1 : 0, *GetNameSafe(Generator), GalaxyCm / ParsecCm, ClusterCm / ParsecCm,
				NeighbourCm / ParsecCm,
				Pawn && Home ? FVector::Dist(Pawn->GetActorLocation(), Home->GetActorLocation()) / LightYearCm : -1.0,
				Pawn ? Pawn->GetActorLocation().Size() / 100000.0 : -1.0,
				Origin ? Origin->GetOriginOffset().Size() / LightYearCm : -1.0, GFreeze.Systems.Num(), GFreeze.Actors.Num());
		}));
}

AAstroGenerator* APSRealScale::FindGameplayGenerator(const UWorld* World)
{
	using namespace APSRealScaleLocal;
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}
	FGeneratorCache& Cache = GGeneratorCache;
	if (Cache.World.Get() != World)
	{
		Cache = FGeneratorCache();
		Cache.World = World;
	}
	if (AAstroGenerator* Known = Cache.Generator.Get(); IsValid(Known) && Known->GetWorld() == World)
	{
		return Known;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now < Cache.NextScanSeconds)
	{
		return nullptr;
	}
	Cache.NextScanSeconds = Now + 1.0;
	// The generator the stellar view draws in a game (UAPSStellarVisualSubsystem::UpdateGameplayStellarView), not the menu's.
	for (TActorIterator<AAstroGenerator> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WorldGenerationPreview")) && !It->UsesContinuousPreviewFrame()
			&& It->GetCanonicalStellarProjectionDescriptor().bFinalized && IsValid(It->GetPreviewHomeSystem()))
		{
			Cache.Generator = *It;
			return *It;
		}
	}
	return nullptr;
}

bool APSRealScale::IsActive(const UWorld* World)
{
	const AAstroGenerator* Generator = FindGameplayGenerator(World);
	return Generator && Generator->UsesRealScale();
}

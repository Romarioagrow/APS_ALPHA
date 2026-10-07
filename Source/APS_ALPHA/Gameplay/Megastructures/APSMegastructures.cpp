#include "APSMegastructures.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Actors/Tech/TechInfrastructure.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Crc.h"

#define LOCTEXT_NAMESPACE "APSMegastructures"

namespace APSMegastructuresLocal
{
	using namespace APSMegastructures;
	using APSInfrastructure::FType;

	TAutoConsoleVariable<int32> CVarShadows(TEXT("aps.Mega.Shadows"), 0,
		TEXT("1: the hubs, the elevator's tower and its counterweight cast shadows (the ring, the tether and the collectors never do). Applies to structures spawned afterwards."));
	TAutoConsoleVariable<int32> CVarCollision(TEXT("aps.Mega.Collision"), 1,
		TEXT("Megastructure collision: 0 none, 1 the elevator's tower blocks (its simple hull), 2 the hubs and the counterweight too. The ring, the tether and the collectors never block: their km-wide hulls would be invisible walls."));
	TAutoConsoleVariable<float> CVarCullScale(TEXT("aps.Mega.CullScale"), 1.0f,
		TEXT("Scales the megastructures' draw distances (1: hubs to ~2600 km, the elevator to 40 world radii, the ring to 40 of its radii)."));

	/** On every built megastructure and scaffold; scaffolds carry their own tag too. */
	const FName MegaTag(TEXT("APS.Mega"));
	const FName ScaffoldTag(TEXT("APS.Mega.Scaffold"));
	/** FAPSInfrastructure's surface tag: the elevator's root settles on the ground like every surface structure. */
	const FName SurfaceTag(TEXT("APS.Infra.Surface"));
	const FName ElevatorId(TEXT("SpaceElevator"));
	const FName SwarmId(TEXT("DysonSwarm"));

	/** Sizes of the authored meshes at their authored scale (the offline scan of 03.10), for scaffolds that load nothing. */
	constexpr double SpaceHubRadiusCm = 576000.0;
	constexpr double SpaceHubHeightCm = 566000.0;
	constexpr double GrandHubRadiusCm = 3360000.0;
	constexpr double GrandHubHeightCm = 5940000.0;
	constexpr double TowerWidthCm = 277000.0;
	constexpr double TowerHeightCm = 599000.0;

	double CullScale()
	{
		return FMath::Max(CVarCullScale.GetValueOnGameThread(), 0.05f);
	}

	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		static TSet<FString> Reported;
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh && !Reported.Contains(Path))
		{
			Reported.Add(Path);
			UE_LOG(LogTemp, Warning, TEXT("[APS.Mega] mesh missing: %s (an engine shape stands in)"), Path);
		}
		return Mesh;
	}

	UMaterialInterface* Tinted(AActor& Owner, const FLinearColor& Colour, const float Glow)
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

	FLinearColor Bronze() { return FLinearColor(0.42f, 0.28f, 0.15f, 1.0f); }
	FLinearColor Steel() { return FLinearColor(0.62f, 0.64f, 0.68f, 1.0f); }
	FLinearColor Lamp() { return FLinearColor(1.0f, 0.78f, 0.45f, 1.0f); }
	FLinearColor Collector() { return FLinearColor(0.95f, 0.66f, 0.22f, 1.0f); }

	struct FPartOptions
	{
		bool bCollision{false};
		bool bShadow{false};
		/** Draw distance, cm (before aps.Mega.CullScale); 0: always drawn. */
		double CullCm{0.0};
	};

	void Configure(UPrimitiveComponent& Part, const FPartOptions& Options)
	{
		Part.SetMobility(EComponentMobility::Movable);
		Part.SetCanEverAffectNavigation(false);
		Part.SetGenerateOverlapEvents(false);
		if (Options.bCollision)
		{
			Part.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			Part.SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part.SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		Part.SetCastShadow(Options.bShadow);
		Part.bCastFarShadow = Options.bShadow;
		// Kilometre-scaled meshes stay out of the distance fields and the indirect lighting: their cost, not their look.
		Part.bAffectDistanceFieldLighting = false;
		Part.bAffectDynamicIndirectLighting = false;
		Part.bReceivesDecals = false;
		if (Options.CullCm > 0.0)
		{
			Part.SetCullDistance(static_cast<float>(FMath::Min(Options.CullCm * CullScale(), 1.0e30)));
		}
	}

	UStaticMeshComponent* AddPart(AActor& Owner, USceneComponent& Root, const TCHAR* Name, UStaticMesh* Mesh,
		const FTransform& Relative, const FPartOptions& Options, UMaterialInterface* Material = nullptr)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(&Owner, Name);
		Part->SetupAttachment(&Root);
		Configure(*Part, Options);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeTransform(Relative);
		if (Material)
		{
			for (int32 Slot = 0; Slot < FMath::Max(Part->GetNumMaterials(), 1); ++Slot)
			{
				Part->SetMaterial(Slot, Material);
			}
		}
		Part->RegisterComponent();
		Owner.AddInstanceComponent(Part);
		return Part;
	}

	UInstancedStaticMeshComponent* AddInstances(AActor& Owner, USceneComponent& Root, const TCHAR* Name, UStaticMesh* Mesh,
		UMaterialInterface* Material, const FPartOptions& Options)
	{
		UInstancedStaticMeshComponent* Part = NewObject<UInstancedStaticMeshComponent>(&Owner, Name);
		Part->SetupAttachment(&Root);
		Configure(*Part, Options);
		Part->SetStaticMesh(Mesh);
		if (Material)
		{
			Part->SetMaterial(0, Material);
		}
		Part->RegisterComponent();
		Owner.AddInstanceComponent(Part);
		return Part;
	}

	void SetInGameName(AActor* Actor, const FText& Name)
	{
		// The world actor keeps its display name protected; it is a reflected property (as SpawnVisual sets it).
		if (FTextProperty* NameProperty = Actor ? FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")) : nullptr)
		{
			NameProperty->SetPropertyValue_InContainer(Actor, Name);
		}
	}

	/**
	 * A technological actor with a scene root at Root: ship navigation lists it, fleet MOVE orders take it. Riding the
	 * place: its turn and every world-origin shift carry it along. No tick.
	 */
	AActor* SpawnRoot(UWorld& World, AActor* Site, const FTransform& Root, const FString& ActorName, const FText& Name,
		const bool bTransient)
	{
		FActorSpawnParameters Parameters;
		if (!ActorName.IsEmpty())
		{
			Parameters.Name = FName(*ActorName);
			Parameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
		}
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (bTransient)
		{
			Parameters.ObjectFlags |= RF_Transient;
		}
		ATechInfrastructure* Actor = World.SpawnActor<ATechInfrastructure>(ATechInfrastructure::StaticClass(), Root, Parameters);
		if (!Actor)
		{
			return nullptr;
		}
		USceneComponent* RootComponent = NewObject<USceneComponent>(Actor, TEXT("MegaRoot"));
		RootComponent->SetMobility(EComponentMobility::Movable);
		Actor->SetRootComponent(RootComponent);
		RootComponent->RegisterComponent();
		Actor->SetActorTransform(Root, false, nullptr, ETeleportType::TeleportPhysics);
		if (Site)
		{
			Actor->AttachToActor(Site, FAttachmentTransformRules::KeepWorldTransform);
		}
		Actor->SetActorTickEnabled(false);
		Actor->Tags.AddUnique(MegaTag);
		SetInGameName(Actor, Name);
		return Actor;
	}

	/** Mean density, g/cm3: the body's own when the generator gave one, else by its kind. */
	double DensityOf(const APlanetaryBody& Body)
	{
		if (Body.PlanetDensity > 0.1 && Body.PlanetDensity < 30.0)
		{
			return Body.PlanetDensity;
		}
		if (const AMoon* Moon = Cast<AMoon>(&Body); Moon && Moon->MoonDensity > 0.1 && Moon->MoonDensity < 30.0)
		{
			return Moon->MoonDensity;
		}
		switch (Body.PlanetType)
		{
		case EPlanetType::GasGiant:
		case EPlanetType::HotGiant:
			return 1.3;
		case EPlanetType::IceGiant:
			return 1.6;
		case EPlanetType::Dwarf:
			return 2.0;
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Ammonia:
		case EPlanetType::Nordic:
		case EPlanetType::Tundra:
			return 2.4;
		case EPlanetType::Ocean:
		case EPlanetType::Water:
			return 3.5;
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
			return 7.5;
		default:
			return 5.5;
		}
	}

	/** The generator gives the worlds no spin: a day of the world's own, 18 to 36 hours, from its stable name. */
	double DayHoursOf(const AActor& Body)
	{
		const uint32 Hash = FCrc::StrCrc32(*FAPSFleetCommand::KeyOf(&Body));
		return 18.0 + 18.0 * static_cast<double>(Hash % 1000u) / 999.0;
	}

	/** Free room from the body's centre to the nearest neighbour's surface: its moons, or its planet and sister moons. */
	double ClearanceOf(const APlanetaryBody& Body)
	{
		const FVector Centre = Body.GetActorLocation();
		double Clearance = 0.0;
		const auto Consider = [&Clearance](const APlanetaryBody* Other, const double Distance)
		{
			if (!IsValid(Other))
			{
				return;
			}
			const double Room = Distance - FMath::Max(Other->GetWorldScapeBodyRadiusCm(), 0.0);
			if (Room > 0.0 && (Clearance <= 0.0 || Room < Clearance))
			{
				Clearance = Room;
			}
		};
		if (const APlanet* Planet = Cast<APlanet>(&Body))
		{
			for (const AMoon* Moon : Planet->Moons)
			{
				if (IsValid(Moon))
				{
					Consider(Moon, FVector::Dist(Moon->GetActorLocation(), Centre));
				}
			}
		}
		else if (const AMoon* Self = Cast<AMoon>(&Body); Self && IsValid(Self->ParentPlanet))
		{
			const APlanet* Parent = Self->ParentPlanet;
			const double Orbit = FVector::Dist(Parent->GetActorLocation(), Centre);
			Consider(Parent, Orbit);
			// A sister moon comes as close as the difference of the two orbits.
			for (const AMoon* Sister : Parent->Moons)
			{
				if (IsValid(Sister) && Sister != Self)
				{
					Consider(Sister, FMath::Abs(FVector::Dist(Parent->GetActorLocation(), Sister->GetActorLocation()) - Orbit));
				}
			}
		}
		return Clearance;
	}

	/** On this body: attached to it (through any parents), else within a tenth of its radius over the ground. */
	bool IsOn(const AActor* Actor, const APlanetaryBody& Body)
	{
		for (const AActor* Parent = Actor ? Actor->GetAttachParentActor() : nullptr; Parent; Parent = Parent->GetAttachParentActor())
		{
			if (Parent == &Body)
			{
				return true;
			}
		}
		return Actor && FVector::Dist(Actor->GetActorLocation(), Body.GetActorLocation()) < Body.GetWorldScapeBodyRadiusCm() * 1.1;
	}

	/**
	 * A meridian of the world's own that does not move while things are built: through the colony on this world, else
	 * under a station of the civilization over it, else at a longitude from the world's name. bOutColony: the colony's.
	 */
	FVector WorldMeridian(const APlanetaryBody& Body, bool& bOutColony)
	{
		bOutColony = false;
		const FVector Centre = Body.GetActorLocation();
		const FVector North = Body.GetActorUpVector();
		const FVector Fallback = Body.GetActorForwardVector();
		if (UWorld* World = Body.GetWorld())
		{
			for (TActorIterator<AColony> It(World); It; ++It)
			{
				if (IsValid(*It) && IsOn(*It, Body))
				{
					bOutColony = true;
					return EquatorDirection(North, It->GetActorLocation() - Centre, 0.0, Fallback);
				}
			}
			for (TActorIterator<ASpaceStation> It(World); It; ++It)
			{
				if (IsValid(*It) && FAPSFleetCommand::OrbitedBody(*It) == &Body)
				{
					return EquatorDirection(North, It->GetActorLocation() - Centre, 0.0, Fallback);
				}
			}
		}
		const uint32 Hash = FCrc::StrCrc32(*FAPSFleetCommand::KeyOf(&Body));
		return EquatorDirection(North, Fallback, static_cast<double>(Hash % 3600u) / 10.0, Fallback);
	}

	/**
	 * The elevator's meridian: an elevator standing here already, else 5 degrees east of the colony on this world (the
	 * tower never stands on it, yet rises over its horizon), else the world's own meridian.
	 */
	FVector ElevatorDirection(const APlanetaryBody& Body, const FAPSInfrastructure& Infrastructure)
	{
		const FVector North = Body.GetActorUpVector();
		if (const AActor* Elevator = Infrastructure.FindActorAt(&Body, ElevatorId))
		{
			return EquatorDirection(North, Elevator->GetActorLocation() - Body.GetActorLocation(), 0.0, Body.GetActorForwardVector());
		}
		bool bColony = false;
		const FVector Meridian = WorldMeridian(Body, bColony);
		return bColony ? FQuat(North, FMath::DegreesToRadians(5.0)).RotateVector(Meridian).GetSafeNormal() : Meridian;
	}

	double StarRadiusOf(const AActor& Site)
	{
		const AStar* Star = Cast<AStar>(&Site);
		return Star && Star->StarRadiusKM > 0 ? Star->StarRadiusKM * 100000.0 : 7.0e10;
	}

	/** A swarm ring's radius around its star: clear of the glare (the infrastructure's own rule), each ring a bit wider. */
	double SwarmRadius(const AActor& Site, const int32 Ring)
	{
		return FMath::Max(0.05 * APSStars::AstronomicalUnitCm, StarRadiusOf(Site) * 40.0) * (1.0 + 0.06 * Ring);
	}

	/** The k-th swarm ring's axis: tilted and turned a little more for each ring, so five of them make a shell. */
	FVector SwarmAxis(const FVector& Up, const int32 Ring)
	{
		const FVector Side = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal().IsNearlyZero()
			? FVector::RightVector : FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
		const FQuat Tilt(Side, FMath::DegreesToRadians(26.0 * Ring));
		const FQuat Turn(Up, FMath::DegreesToRadians(37.0 * Ring));
		return (Turn * Tilt).RotateVector(Up).GetSafeNormal();
	}

	int32 CountAt(const FAPSInfrastructure& Infrastructure, const AActor* Site, const FName Type)
	{
		return Infrastructure.CountAt(Site, Type);
	}

	/** Collectors (or scaffold segments) round a circle in the root's frame: boxes along the tangent, flat to the axis. */
	void CircleBoxes(const FVector& Centre, const FVector& Axis, const FVector& Start, const double Radius, const int32 Count,
		const FVector& SizeTangentRadialAxial, TArray<FTransform>& OutBoxes)
	{
		TArray<FVector> Points;
		RingPoints(Centre, Axis, Start, Radius, Count, Points);
		OutBoxes.Reset();
		const FVector Normal = Axis.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : Axis.GetSafeNormal();
		for (const FVector& Point : Points)
		{
			const FVector Radial = (Point - Centre).GetSafeNormal();
			// Radial x axis keeps (tangent, radial, axis) right-handed: a proper rotation.
			const FVector Tangent = FVector::CrossProduct(Radial, Normal).GetSafeNormal();
			// The engine cube is 100 cm a side, centred: X along the tangent, Y out from the centre, Z along the axis.
			const FMatrix Frame(Tangent, Radial, Normal, FVector::ZeroVector);
			OutBoxes.Emplace(Frame.ToQuat(), Point, SizeTangentRadialAxial / 100.0);
		}
	}

	FText UnderConstructionName(const FType& Type)
	{
		return FText::Format(LOCTEXT("UnderConstruction", "{0} (UNDER CONSTRUCTION)"), Type.Name);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Pure geometry

APSMegastructures::EKind APSMegastructures::KindOf(const APSInfrastructure::EVisual Visual)
{
	using APSInfrastructure::EVisual;
	switch (Visual)
	{
	case EVisual::SpaceHub: return EKind::SpaceHub;
	case EVisual::GrandHub: return EKind::GrandHub;
	case EVisual::SpaceElevator: return EKind::SpaceElevator;
	case EVisual::OrbitalRing: return EKind::OrbitalRing;
	case EVisual::DysonSwarm: return EKind::DysonSwarm;
	case EVisual::DysonSphere: return EKind::DysonSphere;
	default: return EKind::None;
	}
}

double APSMegastructures::StationaryOrbitRatio(const double DensityGramsPerCm3, const double DayHours)
{
	// r^3 = G M T^2 / 4 pi^2 with M = rho 4/3 pi R^3: (r/R)^3 = G rho T^2 / (3 pi). SI inside, the ratio out.
	const double Rho = FMath::Max(DensityGramsPerCm3, 0.05) * 1000.0;
	const double Seconds = FMath::Max(DayHours, 0.1) * 3600.0;
	return FMath::Pow(6.674e-11 * Rho * Seconds * Seconds / (3.0 * UE_DOUBLE_PI), 1.0 / 3.0);
}

APSMegastructures::FWorldLayout APSMegastructures::ComputeLayout(const double BodyRadiusCm, const double StationaryRatio,
	const double ClearanceCm)
{
	FWorldLayout Layout;
	const double Radius = FMath::Max(BodyRadiusCm, 100000.0);
	Layout.BodyRadiusCm = Radius;
	Layout.ClearanceCm = FMath::Max(ClearanceCm, 0.0);
	const double Unlimited = TNumericLimits<double>::Max();
	const double RingCap = Layout.ClearanceCm > 0.0 ? Layout.ClearanceCm * 0.8 : Unlimited;
	const double TopCap = Layout.ClearanceCm > 0.0 ? Layout.ClearanceCm * 0.75 : Unlimited;
	// The ring at the hand-made level's own proportion (1.884 radii round its moon), never under 1.3.
	Layout.RingRadiusCm = FMath::Min(Radius * Assets::RingRadiusRatio, RingCap);
	Layout.bRingFits = Layout.RingRadiusCm >= Radius * 1.3;
	// The counterweight at the stationary orbit, above the ring (the tether crosses it there).
	const double Floor = Layout.bRingFits ? Layout.RingRadiusCm * 1.2 : Radius * 1.6;
	const double Wanted = FMath::Max(Radius * FMath::Clamp(StationaryRatio, 2.2, 9.0), Floor);
	Layout.CounterweightRadiusCm = FMath::Min(Wanted, TopCap);
	Layout.bElevatorFits = Layout.CounterweightRadiusCm >= Floor;
	// Hubs 1.6 fleet slots out (the administration's high orbit), above the air and clear of the neighbours.
	Layout.HubRadiusCm = FMath::Min(APSFleet::SlotRadius(Radius) * 1.6, TopCap);
	Layout.bHubFits = Layout.HubRadiusCm >= Radius * 1.15 + 1000000.0;
	return Layout;
}

int32 APSMegastructures::StageOf(const float Progress, const int32 Steps)
{
	if (Steps <= 0 || Progress <= 0.0f)
	{
		return 0;
	}
	return FMath::Clamp(FMath::CeilToInt(Progress * static_cast<float>(Steps)), 0, Steps);
}

FQuat APSMegastructures::AxisRotation(const int32 MeshAxis, const FVector& WorldAxis, const FVector& Reference)
{
	const int32 A = FMath::Clamp(MeshAxis, 0, 2);
	FVector W0 = WorldAxis.GetSafeNormal();
	if (W0.IsNearlyZero())
	{
		W0 = FVector::UpVector;
	}
	FVector W1 = FVector::VectorPlaneProject(Reference, W0).GetSafeNormal();
	if (W1.IsNearlyZero())
	{
		W1 = FVector::VectorPlaneProject(FMath::Abs(W0.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector, W0).GetSafeNormal();
	}
	// (A, A+1, A+2) is a cyclic order of the axes, so W2 = W0 x W1 keeps it a proper rotation.
	const FVector W2 = FVector::CrossProduct(W0, W1);
	FMatrix Matrix = FMatrix::Identity;
	Matrix.SetAxis(A, W0);
	Matrix.SetAxis((A + 1) % 3, W1);
	Matrix.SetAxis((A + 2) % 3, W2);
	return FQuat(Matrix).GetNormalized();
}

int32 APSMegastructures::LongestAxis(const FVector& Extent)
{
	return Extent.X >= Extent.Y && Extent.X >= Extent.Z ? 0 : Extent.Y >= Extent.Z ? 1 : 2;
}

int32 APSMegastructures::ShortestAxis(const FVector& Extent)
{
	return Extent.X <= Extent.Y && Extent.X <= Extent.Z ? 0 : Extent.Y <= Extent.Z ? 1 : 2;
}

FTransform APSMegastructures::PlaceMesh(const FVector& Anchor, const FVector& Scale, const FQuat& Rotation, const FVector& Point)
{
	return FTransform(Rotation, Point - Rotation.RotateVector(Scale * Anchor), Scale);
}

FTransform APSMegastructures::FitRing(const FVector& Origin, const FVector& Extent, const FVector& Centre, const FVector& Axis,
	const FVector& Reference, const double RadiusCm, const double ThicknessRatio)
{
	const int32 Thin = ShortestAxis(Extent);
	const double MeshRadius = FMath::Max3(Extent[(Thin + 1) % 3], Extent[(Thin + 2) % 3], 1.0e-3);
	const double Plane = RadiusCm / MeshRadius;
	FVector Scale(Plane);
	Scale[Thin] = Plane * ThicknessRatio;
	return PlaceMesh(Origin, Scale, AxisRotation(Thin, Axis, Reference), Centre);
}

FTransform APSMegastructures::FitSpan(const FVector& Origin, const FVector& Extent, const int32 MeshAxis, const FVector& From,
	const FVector& To, const FVector& Reference, const double ThicknessCm)
{
	const int32 A = FMath::Clamp(MeshAxis, 0, 2);
	const FVector Span = To - From;
	const double Length = Span.Size();
	const double Across = ThicknessCm / FMath::Max(2.0 * FMath::Max(Extent[(A + 1) % 3], Extent[(A + 2) % 3]), 1.0e-3);
	FVector Scale(Across);
	Scale[A] = FMath::Max(Length, 1.0) / FMath::Max(2.0 * Extent[A], 1.0e-3);
	return PlaceMesh(Origin, Scale, AxisRotation(A, Length > 0.0 ? Span / Length : FVector::UpVector, Reference), (From + To) * 0.5);
}

FTransform APSMegastructures::FitStanding(const FVector& Origin, const FVector& Extent, const FVector& Point, const FVector& Up,
	const FVector& Reference, const double Scale)
{
	return PlaceMesh(FVector(Origin.X, Origin.Y, Origin.Z - Extent.Z), FVector(Scale), AxisRotation(2, Up, Reference), Point);
}

FTransform APSMegastructures::FitCentred(const FVector& Origin, const FVector& Extent, const FVector& Point, const FVector& Axis,
	const FVector& Reference, const double Scale)
{
	return PlaceMesh(Origin, FVector(Scale), AxisRotation(2, Axis, Reference), Point);
}

FVector APSMegastructures::EquatorDirection(const FVector& Up, const FVector& Reference, const double OffsetDegrees,
	const FVector& Fallback)
{
	FVector North = Up.GetSafeNormal();
	if (North.IsNearlyZero())
	{
		North = FVector::UpVector;
	}
	FVector Direction = FVector::VectorPlaneProject(Reference, North).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::VectorPlaneProject(Fallback, North).GetSafeNormal();
	}
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::VectorPlaneProject(FMath::Abs(North.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector, North).GetSafeNormal();
	}
	return FQuat(North, FMath::DegreesToRadians(OffsetDegrees)).RotateVector(Direction).GetSafeNormal();
}

void APSMegastructures::RingPoints(const FVector& Centre, const FVector& Axis, const FVector& Start, const double RadiusCm,
	const int32 Count, TArray<FVector>& OutPoints)
{
	OutPoints.Reset();
	if (Count <= 0)
	{
		return;
	}
	const FVector North = Axis.GetSafeNormal().IsNearlyZero() ? FVector::UpVector : Axis.GetSafeNormal();
	const FVector First = EquatorDirection(North, Start - Centre, 0.0, FVector::ForwardVector);
	OutPoints.Reserve(Count);
	// 0, +1, -1, +2, -2 ...: the ring grows both ways from its first segment.
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 Step = (Index + 1) / 2 * (Index % 2 == 1 ? 1 : -1);
		const double Angle = UE_DOUBLE_TWO_PI * Step / Count;
		OutPoints.Add(Centre + FQuat(North, Angle).RotateVector(First) * RadiusCm);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// In the world

bool APSMegastructures::LayoutAt(const AActor* Site, FWorldLayout& OutLayout)
{
	using namespace APSMegastructuresLocal;
	const APlanetaryBody* Body = Cast<APlanetaryBody>(Site);
	if (!Body)
	{
		return false;
	}
	OutLayout = ComputeLayout(Body->GetWorldScapeBodyRadiusCm(), StationaryOrbitRatio(DensityOf(*Body), DayHoursOf(*Body)),
		ClearanceOf(*Body));
	return true;
}

FText APSMegastructures::CheckRoom(const APSInfrastructure::FType& Type, const AActor* Site)
{
	const EKind Kind = KindOf(Type.Visual);
	FWorldLayout Layout;
	if (Kind == EKind::None || Kind == EKind::DysonSwarm || Kind == EKind::DysonSphere || !LayoutAt(Site, Layout))
	{
		return FText::GetEmpty();
	}
	if (Kind == EKind::OrbitalRing && !Layout.bRingFits)
	{
		return LOCTEXT("NoRingRoom", "No room for a ring: a neighbouring world circles too close.");
	}
	if (Kind == EKind::SpaceElevator && !Layout.bElevatorFits)
	{
		return LOCTEXT("NoElevatorRoom", "No room for a stationary orbit: a neighbouring world circles too close.");
	}
	if ((Kind == EKind::SpaceHub || Kind == EKind::GrandHub) && !Layout.bHubFits)
	{
		return LOCTEXT("NoHubRoom", "No room for a hub's high orbit: a neighbouring world circles too close.");
	}
	return FText::GetEmpty();
}

bool APSMegastructures::PlaceRoot(const APSInfrastructure::FType& Type, const AActor& Site, const FAPSInfrastructure& Infrastructure,
	const FVector* /*Near*/, FTransform& OutRoot)
{
	using namespace APSMegastructuresLocal;
	const EKind Kind = KindOf(Type.Visual);
	const FVector Centre = Site.GetActorLocation();
	const APlanetaryBody* Body = Cast<APlanetaryBody>(&Site);
	FWorldLayout Layout;
	switch (Kind)
	{
	case EKind::SpaceElevator:
	case EKind::OrbitalRing:
	{
		if (!Body || !LayoutAt(Body, Layout))
		{
			return false;
		}
		// The root on the elevator's meridian: on the ground for the elevator, where the tether crosses the ring for it.
		const FVector North = Body->GetActorUpVector();
		const FVector Direction = ElevatorDirection(*Body, Infrastructure);
		const double Radius = Kind == EKind::SpaceElevator ? Layout.BodyRadiusCm : Layout.RingRadiusCm;
		OutRoot = FTransform(FRotationMatrix::MakeFromZX(Direction, North).ToQuat(), Centre + Direction * Radius);
		return true;
	}
	case EKind::SpaceHub:
	case EKind::GrandHub:
	{
		if (!Body || !LayoutAt(Body, Layout))
		{
			return false;
		}
		// 60 degrees east of the world's own meridian (clear of the elevator 5 degrees east of the colony), each hub already
		// here turning the next one 40 degrees on, lifted 25 degrees off the equator (north for a hub, south for a grand
		// hub) and so clear of the ring and the tether.
		TArray<const FAPSBuiltStructure*> Here;
		Infrastructure.GetAt(&Site, Here);
		int32 Hubs = 0;
		for (const FAPSBuiltStructure* Built : Here)
		{
			const FType* Standing = APSInfrastructure::Find(Built->Type);
			Hubs += Standing && Standing->Category == APSInfrastructure::ECategory::Hub ? 1 : 0;
		}
		const FVector North = Body->GetActorUpVector();
		bool bColony = false;
		const FVector Flat = FQuat(North, FMath::DegreesToRadians(60.0 + 40.0 * Hubs)).RotateVector(WorldMeridian(*Body, bColony))
			.GetSafeNormal();
		const double Lift = FMath::DegreesToRadians(Kind == EKind::GrandHub ? -25.0 : 25.0);
		const FVector Direction = (Flat * FMath::Cos(Lift) + North * FMath::Sin(Lift)).GetSafeNormal();
		const double Cap = Layout.ClearanceCm > 0.0 ? Layout.ClearanceCm * 0.75 : TNumericLimits<double>::Max();
		const double Radius = FMath::Min(Layout.HubRadiusCm * (Kind == EKind::GrandHub ? 1.15 : 1.0), Cap);
		OutRoot = FTransform(FRotationMatrix::MakeFromZX(Direction, North).ToQuat(), Centre + Direction * Radius);
		return true;
	}
	case EKind::DysonSwarm:
	case EKind::DysonSphere:
	{
		// Around the star: the k-th swarm ring on its own tilt (k: the segments standing there already); the root on that
		// ring, where the map marks it, its Z the ring's axis and its X pointing away from the star.
		const int32 Ring = Kind == EKind::DysonSwarm ? CountAt(Infrastructure, &Site, SwarmId) : 0;
		const FVector Up = Site.GetActorUpVector();
		const FVector Axis = Kind == EKind::DysonSwarm ? SwarmAxis(Up, Ring) : Up;
		const double Radius = Kind == EKind::DysonSwarm ? SwarmRadius(Site, Ring) : SwarmRadius(Site, 5) * 1.25;
		const FVector Direction = EquatorDirection(Axis, Site.GetActorForwardVector(), 0.0, FVector::ForwardVector);
		OutRoot = FTransform(FRotationMatrix::MakeFromZX(Axis, Direction).ToQuat(), Centre + Direction * Radius);
		return true;
	}
	default:
		return false;
	}
}

bool APSMegastructures::WorkSlot(const APSInfrastructure::FType& Type, const AActor& Site, const FAPSInfrastructure& Infrastructure,
	FVector& OutSlot)
{
	using namespace APSMegastructuresLocal;
	FTransform Root;
	if (!PlaceRoot(Type, Site, Infrastructure, nullptr, Root))
	{
		return false;
	}
	const FVector Up = Root.GetRotation().GetAxisZ();
	const FVector North = Root.GetRotation().GetAxisX();
	FWorldLayout Layout;
	LayoutAt(&Site, Layout);
	switch (KindOf(Type.Visual))
	{
	case EKind::SpaceHub:
		// Over the disc, on the side away from the world (the spindle hangs toward it).
		OutSlot = Root.GetLocation() + Up * (SpaceHubRadiusCm * 1.3 + 300000.0);
		return true;
	case EKind::GrandHub:
		OutSlot = Root.GetLocation() + Up * (GrandHubHeightCm * 0.5 + 1000000.0);
		return true;
	case EKind::SpaceElevator:
		// Beside the tether at the fleet's own slot height, 20 km off the cable.
		OutSlot = Root.GetLocation() + Up * (APSFleet::SlotRadius(Layout.BodyRadiusCm) - Layout.BodyRadiusCm) + North * 2000000.0;
		return true;
	case EKind::OrbitalRing:
		// Just outside the ring where the tether crosses it: the first segments go up there.
		OutSlot = Root.GetLocation() + Up * (Layout.RingRadiusCm * 0.04 + 1000000.0);
		return true;
	case EKind::DysonSwarm:
	case EKind::DysonSphere:
		// Over the ring's plane at its root (the root's Z is the ring's axis).
		OutSlot = Root.GetLocation() + Up * FVector::Dist(Root.GetLocation(), Site.GetActorLocation()) * 0.05;
		return true;
	default:
		return false;
	}
}

void APSMegastructures::GetAssetPaths(const APSInfrastructure::FType& Type, TArray<FSoftObjectPath>& OutPaths)
{
	OutPaths.Reset();
	switch (KindOf(Type.Visual))
	{
	case EKind::SpaceHub:
		OutPaths.Emplace(Assets::SpaceHub);
		break;
	case EKind::GrandHub:
		OutPaths.Emplace(Assets::GrandHub);
		break;
	case EKind::SpaceElevator:
		OutPaths.Emplace(Assets::ElevatorTower);
		OutPaths.Emplace(Assets::ElevatorTether);
		OutPaths.Emplace(Assets::Counterweight);
		break;
	case EKind::OrbitalRing:
		OutPaths.Emplace(Assets::Ring);
		break;
	default:
		break;
	}
}

AActor* APSMegastructures::SpawnStructure(UWorld* World, const APSInfrastructure::FType& Type, AActor* Site, const FTransform& Root,
	const FString& ActorName, const FText& Name, const FAPSInfrastructure& Infrastructure)
{
	using namespace APSMegastructuresLocal;
	const EKind Kind = KindOf(Type.Visual);
	if (!World || !Site || Kind == EKind::None)
	{
		return nullptr;
	}
	const double StartSeconds = FPlatformTime::Seconds();
	AActor* Actor = SpawnRoot(*World, Site, Root, ActorName, Name, false);
	if (!Actor)
	{
		return nullptr;
	}
	USceneComponent& Frame = *Actor->GetRootComponent();
	const FTransform& RootTransform = Actor->GetActorTransform();
	const bool bShadows = CVarShadows.GetValueOnGameThread() != 0;
	const int32 Collision = CVarCollision.GetValueOnGameThread();
	UStaticMesh* Cube = LoadMesh(Assets::Cube);
	UStaticMesh* Cylinder = LoadMesh(Assets::Cylinder);
	UStaticMesh* Sphere = LoadMesh(Assets::Sphere);
	// In the root's frame: Z is the root's up (away from the world, or the swarm ring's axis), X its north.
	const FVector Up = FVector::UpVector;
	const FVector North = FVector::ForwardVector;
	FString Summary;
	switch (Kind)
	{
	case EKind::SpaceHub:
	case EKind::GrandHub:
	{
		const bool bGrand = Kind == EKind::GrandHub;
		UStaticMesh* Mesh = LoadMesh(bGrand ? Assets::GrandHub : Assets::SpaceHub);
		const double Scale = bGrand ? Assets::GrandHubScale : Assets::SpaceHubScale;
		const double Radius = bGrand ? GrandHubRadiusCm : SpaceHubRadiusCm;
		FPartOptions Options;
		Options.bCollision = Collision >= 2;
		Options.bShadow = bShadows;
		// Seen while it spans three pixels or more: about 230 of its own radii.
		Options.CullCm = Radius * 460.0;
		if (Mesh)
		{
			const FBoxSphereBounds Bounds = Mesh->GetBounds();
			// The disc hub's spindle hangs toward the world, as on the hand-made level; the grand hub stands upright.
			AddPart(*Actor, Frame, TEXT("Hub"), Mesh, FitCentred(Bounds.Origin, Bounds.BoxExtent, FVector::ZeroVector,
				bGrand ? Up : -Up, North, Scale), Options);
			Summary = FString::Printf(TEXT("%s x%.0f, %.1f km across"), *Mesh->GetName(), Scale,
				2.0 * FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y) * Scale / 100000.0);
		}
		else if (Cylinder)
		{
			// Stand-in: a disc and a spindle of the hub's size.
			AddPart(*Actor, Frame, TEXT("HubDisc"), Cylinder, FTransform(FQuat::Identity, FVector::ZeroVector,
				FVector(Radius / 50.0, Radius / 50.0, Radius / 600.0)), Options, Tinted(*Actor, Steel(), 0.0f));
			AddPart(*Actor, Frame, TEXT("HubSpindle"), Cylinder, FTransform(FQuat::Identity, -Up * Radius * 0.6,
				FVector(Radius / 900.0, Radius / 900.0, Radius / 90.0)), Options, Tinted(*Actor, Steel(), 0.0f));
			Summary = TEXT("stand-in disc");
		}
		break;
	}
	case EKind::SpaceElevator:
	{
		FWorldLayout Layout;
		LayoutAt(Site, Layout);
		const double R = Layout.BodyRadiusCm;
		// The root stands on the ground (it settles there like every surface structure); the counterweight's base is
		// this high over it.
		const double Top = FMath::Max(Layout.CounterweightRadiusCm - R, 1000000.0);
		FPartOptions TowerOptions;
		TowerOptions.bCollision = Collision >= 1;
		TowerOptions.bShadow = bShadows;
		TowerOptions.CullCm = 2.0e8;
		double TowerTop = TowerHeightCm;
		if (UStaticMesh* Tower = LoadMesh(Assets::ElevatorTower))
		{
			const FBoxSphereBounds Bounds = Tower->GetBounds();
			AddPart(*Actor, Frame, TEXT("ElevatorTower"), Tower, FitStanding(Bounds.Origin, Bounds.BoxExtent, FVector::ZeroVector,
				Up, North, Assets::ElevatorTowerScale), TowerOptions);
			TowerTop = 2.0 * Bounds.BoxExtent.Z * Assets::ElevatorTowerScale;
		}
		else if (Cylinder)
		{
			AddPart(*Actor, Frame, TEXT("ElevatorTower"), Cylinder, FTransform(FQuat::Identity, Up * TowerHeightCm * 0.5,
				FVector(TowerWidthCm / 100.0, TowerWidthCm / 100.0, TowerHeightCm / 100.0)), TowerOptions, Tinted(*Actor, Steel(), 0.0f));
		}
		FPartOptions WeightOptions;
		WeightOptions.bCollision = Collision >= 2;
		WeightOptions.bShadow = bShadows;
		WeightOptions.CullCm = R * 40.0;
		if (UStaticMesh* Weight = LoadMesh(Assets::Counterweight))
		{
			const FBoxSphereBounds Bounds = Weight->GetBounds();
			AddPart(*Actor, Frame, TEXT("Counterweight"), Weight, FitStanding(Bounds.Origin, Bounds.BoxExtent, Up * Top, Up, North,
				Assets::CounterweightScale), WeightOptions);
		}
		else if (Sphere)
		{
			AddPart(*Actor, Frame, TEXT("Counterweight"), Sphere, FTransform(FQuat::Identity, Up * (Top + 300000.0),
				FVector(6000.0)), WeightOptions, Tinted(*Actor, Steel(), 0.0f));
		}
		// The tether from the tower's collar to the counterweight: the authored spline elevator when it is a long thin mesh,
		// else a cable; lit beads along it and a bright one where the ring will cross.
		const double Thickness = FMath::Clamp(R * 0.0012, 20000.0, 300000.0);
		const FVector From = Up * TowerTop * 0.9;
		const FVector To = Up * Top;
		FPartOptions TetherOptions;
		TetherOptions.CullCm = R * 40.0;
		bool bAuthoredTether = false;
		if (UStaticMesh* Tether = LoadMesh(Assets::ElevatorTether))
		{
			const FBoxSphereBounds Bounds = Tether->GetBounds();
			FVector Sizes = Bounds.BoxExtent;
			const int32 Long = LongestAxis(Sizes);
			const double LongSize = Sizes[Long];
			Sizes[Long] = 0.0;
			if (LongSize >= 4.0 * Sizes.GetMax())
			{
				AddPart(*Actor, Frame, TEXT("Tether"), Tether, FitSpan(Bounds.Origin, Bounds.BoxExtent, Long, From, To, North, Thickness),
					TetherOptions);
				bAuthoredTether = true;
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[APS.Mega] %s is not a long thin mesh (%s): the tether is a cable"), *Tether->GetName(),
					*(Bounds.BoxExtent * 2.0).ToCompactString());
			}
		}
		if (!bAuthoredTether && Cylinder)
		{
			AddPart(*Actor, Frame, TEXT("Tether"), Cylinder, FitSpan(FVector::ZeroVector, FVector(50.0), 2, From, To, North, Thickness),
				TetherOptions, Tinted(*Actor, Steel(), 0.0f));
		}
		if (Sphere)
		{
			FPartOptions BeadOptions;
			BeadOptions.CullCm = R * 12.0;
			UInstancedStaticMeshComponent* Beads = AddInstances(*Actor, Frame, TEXT("TetherBeads"), Sphere, Tinted(*Actor, Lamp(), 6.0f),
				BeadOptions);
			const int32 Count = 16;
			for (int32 Index = 1; Index < Count; ++Index)
			{
				Beads->AddInstance(FTransform(FQuat::Identity, FMath::Lerp(From, To, static_cast<double>(Index) / Count),
					FVector(Thickness * 4.0 / 100.0)));
			}
			if (Layout.bRingFits)
			{
				Beads->AddInstance(FTransform(FQuat::Identity, Up * (Layout.RingRadiusCm - R), FVector(Thickness * 10.0 / 100.0)));
			}
		}
		Summary = FString::Printf(TEXT("tether %.0f km (%s), counterweight at %.2f radii, ring room %s"), (Top - TowerTop) / 100000.0,
			bAuthoredTether ? TEXT("authored") : TEXT("cable"), Layout.CounterweightRadiusCm / R, Layout.bRingFits ? TEXT("yes") : TEXT("no"));
		break;
	}
	case EKind::OrbitalRing:
	{
		FWorldLayout Layout;
		LayoutAt(Site, Layout);
		const FVector LocalCentre = RootTransform.InverseTransformPosition(Site->GetActorLocation());
		const FVector LocalAxis = RootTransform.InverseTransformVectorNoScale(Site->GetActorUpVector());
		FPartOptions Options;
		Options.CullCm = Layout.RingRadiusCm * 40.0;
		if (UStaticMesh* RingMesh = LoadMesh(Assets::Ring))
		{
			const FBoxSphereBounds Bounds = RingMesh->GetBounds();
			// Its hoop round the world's centre in the equatorial plane; the root (here) is where the tether crosses it.
			AddPart(*Actor, Frame, TEXT("Ring"), RingMesh, FitRing(Bounds.Origin, Bounds.BoxExtent, LocalCentre, LocalAxis,
				-LocalCentre, Layout.RingRadiusCm, Assets::RingThicknessRatio), Options);
		}
		else if (Cube)
		{
			UInstancedStaticMeshComponent* Segments = AddInstances(*Actor, Frame, TEXT("RingSegments"), Cube, Tinted(*Actor, Bronze(), 0.0f),
				Options);
			TArray<FTransform> Boxes;
			const double Chord = UE_DOUBLE_TWO_PI * Layout.RingRadiusCm / 96.0;
			CircleBoxes(LocalCentre, LocalAxis, FVector::ZeroVector, Layout.RingRadiusCm, 96,
				FVector(Chord * 1.02, Layout.RingRadiusCm * 0.02, Layout.RingRadiusCm * 0.02), Boxes);
			for (const FTransform& Box : Boxes)
			{
				Segments->AddInstance(Box);
			}
		}
		if (Sphere)
		{
			FPartOptions LampOptions;
			LampOptions.CullCm = Layout.RingRadiusCm * 12.0;
			AddPart(*Actor, Frame, TEXT("Junction"), Sphere, FTransform(FQuat::Identity, FVector::ZeroVector,
				FVector(Layout.RingRadiusCm * 0.008 / 100.0)), LampOptions, Tinted(*Actor, Lamp(), 6.0f));
		}
		Summary = FString::Printf(TEXT("ring radius %.0f km (%.2f world radii)"), Layout.RingRadiusCm / 100000.0,
			Layout.RingRadiusCm / FMath::Max(Layout.BodyRadiusCm, 1.0));
		break;
	}
	case EKind::DysonSwarm:
	case EKind::DysonSphere:
	{
		if (!Cube)
		{
			break;
		}
		const FVector LocalCentre = RootTransform.InverseTransformPosition(Site->GetActorLocation());
		FPartOptions Options;
		UInstancedStaticMeshComponent* Collectors = AddInstances(*Actor, Frame, TEXT("Collectors"), Cube, Tinted(*Actor, Collector(), 2.0f),
			Options);
		// Spawned before the runtime records it: the segments standing there are the ones before this one.
		const int32 Rings = Kind == EKind::DysonSphere ? 6 : 1;
		const int32 Ring = Kind == EKind::DysonSwarm ? Infrastructure.CountAt(Site, SwarmId) : 0;
		const double Radius = Kind == EKind::DysonSwarm ? SwarmRadius(*Site, Ring) : SwarmRadius(*Site, 5) * 1.25;
		for (int32 Index = 0; Index < Rings; ++Index)
		{
			// The swarm ring lies in the root's own plane (the root's Z is its axis); the sphere's six turn about the root's
			// X, the line through the star.
			const FVector Axis = Kind == EKind::DysonSwarm ? Up : FQuat(North, FMath::DegreesToRadians(30.0 * Index)).RotateVector(Up);
			const int32 Count = 36;
			const double Chord = UE_DOUBLE_TWO_PI * Radius / Count;
			TArray<FTransform> Boxes;
			CircleBoxes(LocalCentre, Axis, FVector::ZeroVector, Radius, Count, FVector(Chord * 0.55, Chord * 0.04, Chord * 0.35), Boxes);
			for (const FTransform& Box : Boxes)
			{
				Collectors->AddInstance(Box);
			}
		}
		Summary = FString::Printf(TEXT("%d collectors at %.3f AU"), Collectors->GetInstanceCount(), Radius / APSStars::AstronomicalUnitCm);
		break;
	}
	default:
		break;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Mega] %s spawned at %s: %s (%.0f ms)"), *Actor->GetName(), *FAPSFleetCommand::KeyOf(Site), *Summary,
		(FPlatformTime::Seconds() - StartSeconds) * 1000.0);
	return Actor;
}

// ---------------------------------------------------------------------------------------------------------------------
// Construction sites

struct FAPSMegastructureYard::FScaffold
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<AActor> Site;
	FName Type;
	APSMegastructures::EKind Kind{APSMegastructures::EKind::None};
	/** Segments (frame boxes, ring segments, collectors) in the order they appear. */
	TWeakObjectPtr<UInstancedStaticMeshComponent> Segments;
	TArray<FTransform> SegmentBoxes;
	/** A frame that grows along the root's Z: the hub's spindle, the elevator's tether. */
	TWeakObjectPtr<UStaticMeshComponent> Spine;
	FVector SpineFrom{FVector::ZeroVector};
	FVector SpineTo{FVector::ZeroVector};
	double SpineThickness{1000.0};
	/** The elevator's tower frame, rising from the ground. */
	TWeakObjectPtr<UStaticMeshComponent> Core;
	double CoreWidth{1000.0};
	double CoreHeight{1000.0};
	int32 Stage{-1};
	/** The finished look's meshes, streamed in while the ships work (so completion does not wait for the disk). */
	TSharedPtr<FStreamableHandle> Preload;
};

FAPSMegastructureYard::FAPSMegastructureYard(UWorld* InWorld)
	: World(InWorld)
{
}

// The scaffolds are transient actors of the world: they go with it (nothing is destroyed during its teardown here).
FAPSMegastructureYard::~FAPSMegastructureYard() = default;

void FAPSMegastructureYard::Destroy(FScaffold& Scaffold)
{
	if (AActor* Actor = Scaffold.Actor.Get())
	{
		Actor->Destroy();
	}
	Scaffold.Actor.Reset();
	Scaffold.Preload.Reset();
}

void FAPSMegastructureYard::Finish(const AActor* Site, const FName Type)
{
	const FString Key = FAPSFleetCommand::KeyOf(Site) + TEXT("|") + Type.ToString();
	if (TUniquePtr<FScaffold>* Found = Scaffolds.Find(Key))
	{
		if (*Found)
		{
			Destroy(**Found);
		}
		Scaffolds.Remove(Key);
	}
	Previews.Remove(Key);
}

void FAPSMegastructureYard::SetPreview(AActor* Site, const FName Type, const float Progress)
{
	const FString Key = FAPSFleetCommand::KeyOf(Site) + TEXT("|") + Type.ToString();
	if (Progress < 0.0f || !Site)
	{
		Previews.Remove(Key);
		return;
	}
	Previews.Add(Key, TPair<TWeakObjectPtr<AActor>, float>(Site, FMath::Clamp(Progress, 0.0f, 1.0f)));
	Clock = 1.0f;
}

bool FAPSMegastructureYard::Create(FScaffold& Scaffold, const APSInfrastructure::FType& Type, AActor& Site,
	const FAPSInfrastructure& Infrastructure)
{
	using namespace APSMegastructures;
	using namespace APSMegastructuresLocal;
	UWorld* LiveWorld = World.Get();
	// The same place Complete gives the finished one (PlaceRoot depends on the world alone, not on the builder).
	FTransform Root;
	if (!LiveWorld || !PlaceRoot(Type, Site, Infrastructure, nullptr, Root))
	{
		return false;
	}
	AActor* Actor = SpawnRoot(*LiveWorld, &Site, Root, FString(), UnderConstructionName(Type), true);
	if (!Actor)
	{
		return false;
	}
	Actor->Tags.AddUnique(ScaffoldTag);
	Scaffold.Actor = Actor;
	Scaffold.Site = &Site;
	Scaffold.Type = Type.Id;
	Scaffold.Kind = KindOf(Type.Visual);
	Scaffold.Stage = -1;
	USceneComponent& Frame = *Actor->GetRootComponent();
	const FTransform& RootTransform = Actor->GetActorTransform();
	UStaticMesh* Cube = LoadMesh(Assets::Cube);
	UStaticMesh* Cylinder = LoadMesh(Assets::Cylinder);
	UMaterialInterface* FrameMaterial = Tinted(*Actor, Bronze(), 0.0f);
	const FVector Up = FVector::UpVector;
	FPartOptions Options;
	FWorldLayout Layout;
	LayoutAt(&Site, Layout);
	switch (Scaffold.Kind)
	{
	case EKind::SpaceHub:
	case EKind::GrandHub:
	{
		const bool bGrand = Scaffold.Kind == EKind::GrandHub;
		const double Radius = bGrand ? GrandHubRadiusCm : SpaceHubRadiusCm;
		const double Height = bGrand ? GrandHubHeightCm : SpaceHubHeightCm;
		Options.CullCm = Radius * 460.0;
		// The rim's frame first, then the spindle (toward the world for the disc hub, upright for the grand hub).
		const double Chord = UE_DOUBLE_TWO_PI * Radius / 24.0;
		CircleBoxes(FVector::ZeroVector, Up, FVector::ForwardVector, Radius, 24, FVector(Chord * 0.8, Radius * 0.05, Radius * 0.05),
			Scaffold.SegmentBoxes);
		Scaffold.SpineFrom = (bGrand ? -Up : Up) * Height * 0.1;
		Scaffold.SpineTo = (bGrand ? Up : -Up) * Height * 0.9;
		Scaffold.SpineThickness = Radius * 0.08;
		break;
	}
	case EKind::SpaceElevator:
	{
		const double Top = FMath::Max(Layout.CounterweightRadiusCm - Layout.BodyRadiusCm, 1000000.0);
		Options.CullCm = Layout.BodyRadiusCm * 40.0;
		Scaffold.CoreWidth = TowerWidthCm * 0.6;
		Scaffold.CoreHeight = TowerHeightCm;
		Scaffold.SpineFrom = Up * TowerHeightCm * 0.9;
		Scaffold.SpineTo = Up * Top;
		Scaffold.SpineThickness = FMath::Clamp(Layout.BodyRadiusCm * 0.0012, 20000.0, 300000.0);
		// The counterweight's frame closes in the last tenth.
		const double Radius = 300000.0;
		const double Chord = UE_DOUBLE_TWO_PI * Radius / 12.0;
		CircleBoxes(Up * Top, Up, FVector::ForwardVector, Radius, 12, FVector(Chord * 0.8, Radius * 0.12, Radius * 0.12),
			Scaffold.SegmentBoxes);
		Actor->Tags.AddUnique(SurfaceTag);
		break;
	}
	case EKind::OrbitalRing:
	{
		Options.CullCm = Layout.RingRadiusCm * 40.0;
		const FVector LocalCentre = RootTransform.InverseTransformPosition(Site.GetActorLocation());
		const FVector LocalAxis = RootTransform.InverseTransformVectorNoScale(Site.GetActorUpVector());
		const double Chord = UE_DOUBLE_TWO_PI * Layout.RingRadiusCm / 48.0;
		// From the junction (the root) both ways round the world.
		CircleBoxes(LocalCentre, LocalAxis, FVector::ZeroVector, Layout.RingRadiusCm, 48,
			FVector(Chord * 0.82, Layout.RingRadiusCm * 0.02, Layout.RingRadiusCm * 0.02), Scaffold.SegmentBoxes);
		break;
	}
	case EKind::DysonSwarm:
	case EKind::DysonSphere:
	{
		const FVector LocalCentre = RootTransform.InverseTransformPosition(Site.GetActorLocation());
		const int32 Rings = Scaffold.Kind == EKind::DysonSphere ? 6 : 1;
		const int32 Ring = Infrastructure.CountAt(&Site, SwarmId);
		const double Radius = Scaffold.Kind == EKind::DysonSwarm ? SwarmRadius(Site, Ring) : SwarmRadius(Site, 5) * 1.25;
		for (int32 Index = 0; Index < Rings; ++Index)
		{
			const FVector Axis = Scaffold.Kind == EKind::DysonSwarm ? Up
				: FQuat(FVector::ForwardVector, FMath::DegreesToRadians(30.0 * Index)).RotateVector(Up);
			const double Chord = UE_DOUBLE_TWO_PI * Radius / 36.0;
			TArray<FTransform> Boxes;
			CircleBoxes(LocalCentre, Axis, FVector::ZeroVector, Radius, 36, FVector(Chord * 0.55, Chord * 0.04, Chord * 0.35), Boxes);
			Scaffold.SegmentBoxes.Append(Boxes);
		}
		FrameMaterial = Tinted(*Actor, Collector(), 0.6f);
		break;
	}
	default:
		break;
	}
	if (Cube && !Scaffold.SegmentBoxes.IsEmpty())
	{
		Scaffold.Segments = AddInstances(*Actor, Frame, TEXT("ScaffoldSegments"), Cube, FrameMaterial, Options);
	}
	if (Cylinder && !Scaffold.SpineFrom.Equals(Scaffold.SpineTo))
	{
		UStaticMeshComponent* Spine = AddPart(*Actor, Frame, TEXT("ScaffoldSpine"), Cylinder, FTransform::Identity, Options,
			Tinted(*Actor, Steel(), 0.0f));
		Spine->SetVisibility(false);
		Scaffold.Spine = Spine;
	}
	if (Cylinder && Scaffold.Kind == EKind::SpaceElevator)
	{
		FPartOptions CoreOptions = Options;
		CoreOptions.CullCm = 2.0e8;
		UStaticMeshComponent* Core = AddPart(*Actor, Frame, TEXT("ScaffoldTower"), Cylinder, FTransform::Identity, CoreOptions, FrameMaterial);
		Core->SetVisibility(false);
		Scaffold.Core = Core;
	}
	// The finished look streams in meanwhile.
	TArray<FSoftObjectPath> Paths;
	GetAssetPaths(Type, Paths);
	if (!Paths.IsEmpty() && UAssetManager::IsInitialized())
	{
		Scaffold.Preload = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate());
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Mega] scaffold of %s at %s: %d segments"), *Type.Id.ToString(), *FAPSFleetCommand::KeyOf(&Site),
		Scaffold.SegmentBoxes.Num());
	return true;
}

void FAPSMegastructureYard::Grow(FScaffold& Scaffold, const float Progress)
{
	using namespace APSMegastructures;
	const int32 Stage = StageOf(Progress, 96);
	if (Stage == Scaffold.Stage)
	{
		return;
	}
	Scaffold.Stage = Stage;
	const float Done = static_cast<float>(Stage) / 96.0f;
	const auto Phase = [Done](const float From, const float To)
	{
		return FMath::Clamp((Done - From) / FMath::Max(To - From, 0.001f), 0.0f, 1.0f);
	};
	// Which share of the segments, the spine and the tower stand at this stage.
	float SegmentShare = Done;
	float SpineShare = 0.0f;
	float CoreShare = 0.0f;
	switch (Scaffold.Kind)
	{
	case EKind::SpaceHub:
	case EKind::GrandHub:
		SegmentShare = Phase(0.0f, 0.7f);
		SpineShare = Phase(0.3f, 0.95f);
		break;
	case EKind::SpaceElevator:
		CoreShare = Phase(0.0f, 0.25f);
		SpineShare = Phase(0.25f, 0.9f);
		SegmentShare = Phase(0.9f, 1.0f);
		break;
	default:
		break;
	}
	if (UInstancedStaticMeshComponent* Segments = Scaffold.Segments.Get())
	{
		const int32 Wanted = FMath::Clamp(FMath::CeilToInt(SegmentShare * Scaffold.SegmentBoxes.Num()), 0, Scaffold.SegmentBoxes.Num());
		if (Wanted < Segments->GetInstanceCount())
		{
			Segments->ClearInstances();
		}
		for (int32 Index = Segments->GetInstanceCount(); Index < Wanted; ++Index)
		{
			Segments->AddInstance(Scaffold.SegmentBoxes[Index]);
		}
	}
	if (UStaticMeshComponent* Spine = Scaffold.Spine.Get())
	{
		Spine->SetVisibility(SpineShare > 0.0f);
		if (SpineShare > 0.0f)
		{
			const FVector To = FMath::Lerp(Scaffold.SpineFrom, Scaffold.SpineTo, static_cast<double>(SpineShare));
			Spine->SetRelativeTransform(FitSpan(FVector::ZeroVector, FVector(50.0), 2, Scaffold.SpineFrom, To, FVector::ForwardVector,
				Scaffold.SpineThickness));
		}
	}
	if (UStaticMeshComponent* Core = Scaffold.Core.Get())
	{
		Core->SetVisibility(CoreShare > 0.0f);
		if (CoreShare > 0.0f)
		{
			const double Height = Scaffold.CoreHeight * CoreShare;
			Core->SetRelativeTransform(FTransform(FQuat::Identity, FVector::UpVector * Height * 0.5,
				FVector(Scaffold.CoreWidth / 100.0, Scaffold.CoreWidth / 100.0, Height / 100.0)));
		}
	}
}

void FAPSMegastructureYard::Tick(const float DeltaSeconds, const FAPSInfrastructure& Infrastructure)
{
	Clock += DeltaSeconds;
	if (Clock < 0.5f)
	{
		return;
	}
	Clock = 0.0f;
	UWorld* LiveWorld = World.Get();
	const FAPSFleetCommand* Fleet = APSFleetFind(LiveWorld);
	// The work under way: every construction ship at work on a hub or megastructure, and the console's previews.
	TMap<FString, TPair<AActor*, TPair<FName, float>>> Work;
	if (Fleet)
	{
		for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
		{
			const APSInfrastructure::FType* Type = Unit.Order == APSFleet::EOrder::BuildStructure && Unit.Phase == APSFleet::EPhase::Working
				? APSInfrastructure::Find(Unit.StructureType) : nullptr;
			AActor* Site = Unit.Target.Get();
			if (!Type || !Site || !APSInfrastructure::IsMegaVisual(Type->Visual))
			{
				continue;
			}
			Work.Add(FAPSFleetCommand::KeyOf(Site) + TEXT("|") + Type->Id.ToString(),
				TPair<AActor*, TPair<FName, float>>(Site, TPair<FName, float>(Type->Id, Unit.Progress)));
		}
	}
	for (auto It = Previews.CreateIterator(); It; ++It)
	{
		AActor* Site = It.Value().Key.Get();
		FString Type;
		It.Key().Split(TEXT("|"), nullptr, &Type);
		if (!Site)
		{
			It.RemoveCurrent();
			continue;
		}
		Work.Add(It.Key(), TPair<AActor*, TPair<FName, float>>(Site, TPair<FName, float>(FName(*Type), It.Value().Value)));
	}
	for (auto It = Scaffolds.CreateIterator(); It; ++It)
	{
		if (!Work.Contains(It.Key()) || !It.Value() || !It.Value()->Actor.IsValid())
		{
			if (It.Value())
			{
				Destroy(*It.Value());
			}
			It.RemoveCurrent();
		}
	}
	for (const TPair<FString, TPair<AActor*, TPair<FName, float>>>& Entry : Work)
	{
		const APSInfrastructure::FType* Type = APSInfrastructure::Find(Entry.Value.Value.Key);
		AActor* Site = Entry.Value.Key;
		if (!Type || !Site)
		{
			continue;
		}
		TUniquePtr<FScaffold>& Scaffold = Scaffolds.FindOrAdd(Entry.Key);
		if (!Scaffold)
		{
			Scaffold = MakeUnique<FScaffold>();
			if (!Create(*Scaffold, *Type, *Site, Infrastructure))
			{
				Scaffolds.Remove(Entry.Key);
				continue;
			}
		}
		Grow(*Scaffold, Entry.Value.Value.Value);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Console

namespace APSMegastructuresLocal
{
	/** A place named on the console: a planet or moon by name or key (the colony's world when none), a star for the rest. */
	AActor* FindSite(UWorld* World, const APSInfrastructure::FType& Type, const FString& Wanted)
	{
		if (!World)
		{
			return nullptr;
		}
		if (Type.Placement == APSInfrastructure::EPlacement::StarSystem)
		{
			for (TActorIterator<AStar> It(World); It; ++It)
			{
				return *It;
			}
			return nullptr;
		}
		APlanetaryBody* First = nullptr;
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			APlanetaryBody* Body = *It;
			if (!IsValid(Body))
			{
				continue;
			}
			if (!Wanted.IsEmpty())
			{
				if (FAPSFleetCommand::KeyOf(Body).Contains(Wanted) || Body->AstroName.ToString().Contains(Wanted))
				{
					return Body;
				}
				continue;
			}
			First = First ? First : Body;
			for (TActorIterator<AColony> Colony(World); Colony; ++Colony)
			{
				if (IsValid(*Colony) && IsOn(*Colony, *Body))
				{
					return Body;
				}
			}
		}
		return First;
	}

	FAutoConsoleCommandWithWorldAndArgs ListCommand(TEXT("aps.Mega.List"),
		TEXT("Logs the hubs and megastructures standing, the scaffolds at work and every world's layout (radius, ring, stationary orbit, clearance)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
		{
			const FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
			if (!Infrastructure)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Mega] no infrastructure in this world"));
				return;
			}
			for (const FAPSBuiltStructure& Built : Infrastructure->GetStructures())
			{
				const APSInfrastructure::FType* Type = APSInfrastructure::Find(Built.Type);
				if (Type && APSInfrastructure::IsMegaVisual(Type->Visual))
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Mega] %s at %s: %s"), *Built.Type.ToString(), *Built.SiteKey,
						Built.Actor.IsValid() ? *Built.Actor->GetActorLocation().ToCompactString() : TEXT("(no actor)"));
				}
			}
			for (TActorIterator<APlanetaryBody> It(World); It; ++It)
			{
				FWorldLayout Layout;
				if (IsValid(*It) && LayoutAt(*It, Layout))
				{
					UE_LOG(LogTemp, Log, TEXT("[APS.Mega] %s: R %.0f km, ring %.0f km (%s), counterweight %.0f km = %.2f R (%s), hubs %.0f km, clearance %.0f km"),
						*FAPSFleetCommand::KeyOf(*It), Layout.BodyRadiusCm / 100000.0, Layout.RingRadiusCm / 100000.0,
						Layout.bRingFits ? TEXT("fits") : TEXT("no room"), Layout.CounterweightRadiusCm / 100000.0,
						Layout.CounterweightRadiusCm / Layout.BodyRadiusCm, Layout.bElevatorFits ? TEXT("fits") : TEXT("no room"),
						Layout.HubRadiusCm / 100000.0, Layout.ClearanceCm / 100000.0);
				}
			}
			const FAPSMegastructureYard* Yard = Infrastructure->GetMegastructureYard();
			UE_LOG(LogTemp, Log, TEXT("[APS.Mega] scaffolds at work: %d"), Yard ? Yard->GetScaffoldCount() : 0);
		}));

	FAutoConsoleCommandWithWorldAndArgs RaiseCommand(TEXT("aps.Mega.Raise"),
		TEXT("Tests: raises a hub or megastructure at once, without its needs or cost: aps.Mega.Raise <SpaceHub|GrandHub|SpaceElevator|OrbitalRing|DysonSwarm|DysonSphere> [world name]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
			const APSInfrastructure::FType* Type = Args.Num() > 0 ? APSInfrastructure::Find(FName(*Args[0])) : nullptr;
			AActor* Site = Type ? FindSite(World, *Type, Args.Num() > 1 ? Args[1] : FString()) : nullptr;
			if (!Infrastructure || !Type || !Site)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Mega] raise: name a type (and a world); nothing raised"));
				return;
			}
			const AActor* Built = Infrastructure->Complete(Type->Id, Site);
			UE_LOG(LogTemp, Log, TEXT("[APS.Mega] raise %s at %s: %s"), *Type->Id.ToString(), *FAPSFleetCommand::KeyOf(Site),
				Built ? *Built->GetName() : TEXT("failed"));
		}));

	FAutoConsoleCommandWithWorldAndArgs PreviewCommand(TEXT("aps.Mega.Preview"),
		TEXT("Tests: shows a construction scaffold at a progress: aps.Mega.Preview <type> <0..1|off> [world name]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
			FAPSMegastructureYard* Yard = Infrastructure ? Infrastructure->GetMegastructureYard() : nullptr;
			const APSInfrastructure::FType* Type = Args.Num() > 1 ? APSInfrastructure::Find(FName(*Args[0])) : nullptr;
			AActor* Site = Type ? FindSite(World, *Type, Args.Num() > 2 ? Args[2] : FString()) : nullptr;
			if (!Yard || !Type || !Site || !APSInfrastructure::IsMegaVisual(Type->Visual))
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Mega] preview: aps.Mega.Preview <type> <0..1|off> [world]"));
				return;
			}
			const float Progress = Args[1].Equals(TEXT("off"), ESearchCase::IgnoreCase) ? -1.0f : FCString::Atof(*Args[1]);
			Yard->SetPreview(Site, Type->Id, Progress);
			UE_LOG(LogTemp, Log, TEXT("[APS.Mega] preview %s at %s: %s"), *Type->Id.ToString(), *FAPSFleetCommand::KeyOf(Site),
				Progress < 0.0f ? TEXT("off") : *FString::SanitizeFloat(Progress));
		}));
}

#undef LOCTEXT_NAMESPACE

#include "APSCivilizationMaterializationSubsystem.h"

#include "APSCivilizationIdentityComponent.h"
#include "APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** The lowest point of an actor along Direction relative to Point, from its own oriented bounds. */
	double OrientedBottom(const AActor* Actor, const FVector& Point, const FVector& Direction)
	{
		// The same visible box the placement used (invisible volumes reach far below a ship's hull).
		FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Actor);
		if (!Local.IsValid)
		{
			Local = Actor->CalculateComponentsBoundingBoxInLocalSpace(true);
		}
		if (!Local.IsValid)
		{
			return FVector::DotProduct(Actor->GetActorLocation() - Point, Direction);
		}
		FVector Corners[8];
		Local.GetVertices(Corners);
		const FTransform Transform = Actor->GetActorTransform();
		double Bottom = TNumericLimits<double>::Max();
		for (const FVector& Corner : Corners)
		{
			Bottom = FMath::Min(Bottom, FVector::DotProduct(Transform.TransformPosition(Corner) - Point, Direction));
		}
		return Bottom;
	}

	/** Whether two actors' own boxes overlap across the ground: the separating axes of both footprints. */
	bool FootprintsOverlap(const AActor* A, const AActor* B)
	{
		const FBox LocalA = A->CalculateComponentsBoundingBoxInLocalSpace(true);
		const FBox LocalB = B->CalculateComponentsBoundingBoxInLocalSpace(true);
		if (!LocalA.IsValid || !LocalB.IsValid)
		{
			return false;
		}
		const auto Separated = [](const AActor* Frame, const FBox& FrameBox, const AActor* Other, const FBox& OtherBox)
		{
			FVector Corners[8];
			OtherBox.GetVertices(Corners);
			const FTransform FrameTransform = Frame->GetActorTransform();
			const FTransform OtherTransform = Other->GetActorTransform();
			FBox InFrame(ForceInit);
			for (const FVector& Corner : Corners)
			{
				InFrame += FrameTransform.InverseTransformPosition(OtherTransform.TransformPosition(Corner));
			}
			return InFrame.Min.X > FrameBox.Max.X || InFrame.Max.X < FrameBox.Min.X
				|| InFrame.Min.Y > FrameBox.Max.Y || InFrame.Max.Y < FrameBox.Min.Y;
		};
		return !Separated(A, LocalA, B, LocalB) && !Separated(B, LocalB, A, LocalA);
	}

	/**
	 * Whether the pad's deck (a circle) reaches into the base's visible box, across the ground in the base's frame. The
	 * pad's access ramp runs towards the base on purpose, so it does not count (01.10: whole-actor boxes blocked the
	 * colony once the pad stood on the ground instead of tens of metres above the base).
	 */
	bool DeckOverlapsBase(const AActor* Base, const AAPSCivilizationLandingPad* Pad)
	{
		const FBox BaseBox = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Base);
		const UStaticMeshComponent* Deck = Pad ? Pad->Deck.Get() : nullptr;
		if (!BaseBox.IsValid || !IsValid(Deck))
		{
			return false;
		}
		const FTransform BaseTransform = Base->GetActorTransform();
		const FVector BaseScale = BaseTransform.GetScale3D().GetAbs().ComponentMax(FVector(UE_SMALL_NUMBER));
		// The deck's own radius: its mesh in its own axes, not a world box that shrinks as the deck tilts.
		const FVector MeshExtent = Deck->GetStaticMesh() ? Deck->GetStaticMesh()->GetBounds().BoxExtent : FVector::ZeroVector;
		const FVector DeckScale = Deck->GetComponentScale().GetAbs();
		const double DeckRadius = FMath::Max(MeshExtent.X * DeckScale.X, MeshExtent.Y * DeckScale.Y);
		const FVector Center = BaseTransform.InverseTransformPosition(Deck->GetComponentLocation()) * BaseScale;
		const FVector BoxCenter = BaseBox.GetCenter() * BaseScale;
		const FVector BoxExtent = BaseBox.GetExtent() * BaseScale;
		const double DX = FMath::Max(FMath::Abs(Center.X - BoxCenter.X) - BoxExtent.X, 0.0);
		const double DY = FMath::Max(FMath::Abs(Center.Y - BoxCenter.Y) - BoxExtent.Y, 0.0);
		return FMath::Sqrt(DX * DX + DY * DY) < DeckRadius;
	}

	/** The body's own terrain and foliage collision: owned by the body through its WorldScape root. */
	bool IsOwnedBy(const AActor* Actor, const AActor* Owner)
	{
		for (const AActor* Current = Actor; IsValid(Current); Current = Current->GetOwner())
		{
			if (Current == Owner)
			{
				return true;
			}
		}
		return false;
	}
}

bool UAPSCivilizationMaterializationSubsystem::ValidateMaterializedStarterSet(
	APlanetaryBody* HomeBody,
	const FAPSCivilizationFootprintResult& Placement,
	FString& OutFailureReason) const
{
	auto Fail = [&OutFailureReason](const FString& Reason)
	{
		OutFailureReason = Reason;
		return false;
	};

	if (!IsValid(HomeBody) || !IsValid(MaterializedBase.Get())
		|| !IsValid(MaterializedPad.Get()) || !IsValid(MaterializedShip.Get()))
	{
		return Fail(TEXT("starter actor set is incomplete"));
	}
	if (!Placement.bReadyForMaterialization || !Placement.bTerrainResolved
		|| !Placement.bDry || !Placement.bSlopeValid || !Placement.bWalkableRoute
		|| !Placement.bLod0Ready || !Placement.bCollisionReady)
	{
		return Fail(TEXT("Surface placement readiness contract is incomplete"));
	}

	const FAPSCivilizationManifestEntity* BaseEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::BaseModule);
	const FAPSCivilizationManifestEntity* PadEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::LandingPad);
	const FAPSCivilizationManifestEntity* ShipEntity = RuntimeManifest.FindEntity(
		EAPSCivilizationEntityRole::SelectedShip);
	if (!BaseEntity || !PadEntity || !ShipEntity)
	{
		return Fail(TEXT("starter manifest role is missing"));
	}

	struct FBinding
	{
		AActor* Actor;
		const FAPSCivilizationManifestEntity* Entity;
	};
	const FBinding Bindings[] = {
		{MaterializedBase.Get(), BaseEntity},
		{MaterializedPad.Get(), PadEntity},
		{MaterializedShip.Get(), ShipEntity}
	};

	for (const FBinding& Binding : Bindings)
	{
		// A ship in service (docked at the headquarters or piloted) keeps its own place: only its identity is checked.
		const bool bPlaced = Binding.Actor != MaterializedShip.Get() || bShipParkedAtColony;
		UClass* ExpectedClass = Binding.Entity->ActorClass.ResolveClass();
		if (!ExpectedClass || Binding.Actor->GetClass() != ExpectedClass)
		{
			return Fail(FString::Printf(TEXT("starter class mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		if (Binding.Actor->GetFName() != Binding.Entity->MakeDeterministicActorName())
		{
			return Fail(FString::Printf(TEXT("deterministic actor name mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		const UAPSCivilizationIdentityComponent* Identity =
			Binding.Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>();
		if (!Identity || Identity->ManifestSchemaVersion != RuntimeManifest.SchemaVersion
			|| Identity->StableEntityId != Binding.Entity->StableId
			|| Identity->ParentStableId != Binding.Entity->ParentStableId
			|| Identity->OwnerCivilizationId != RuntimeManifest.CivilizationId
			|| Identity->FactionId != RuntimeManifest.FactionId
			|| Identity->Role != Binding.Entity->Role
			|| Identity->HomeBodyKey != RuntimeManifest.HomeBodyKey)
		{
			return Fail(FString::Printf(TEXT("stable identity mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		if (bPlaced && Binding.Actor->GetAttachParentActor() != HomeBody)
		{
			return Fail(FString::Printf(TEXT("home-body attachment mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		const FVector RadialUp = (Binding.Actor->GetActorLocation()
			- HomeBody->GetActorLocation()).GetSafeNormal();
		if (bPlaced && FVector::DotProduct(Binding.Actor->GetActorUpVector(), RadialUp) < 0.98)
		{
			return Fail(FString::Printf(TEXT("planet-relative orientation mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		const FTransform RelativeTransform = Binding.Actor->GetActorTransform()
			.GetRelativeTransform(HomeBody->GetActorTransform());
		if (bPlaced && (!Binding.Entity->bHasPersistedTransform
			|| !RelativeTransform.Equals(Binding.Entity->PlanetRelativeTransform, 0.5)))
		{
			return Fail(FString::Printf(TEXT("persisted transform mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}

		int32 StableIdMatches = 0;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (const UAPSCivilizationIdentityComponent* CandidateIdentity =
				It->FindComponentByClass<UAPSCivilizationIdentityComponent>();
				CandidateIdentity && CandidateIdentity->StableEntityId == Binding.Entity->StableId)
			{
				++StableIdMatches;
			}
		}
		if (StableIdMatches != 1)
		{
			return Fail(FString::Printf(TEXT("stable id %s resolves to %d actors"),
				*Binding.Entity->StableId.ToString(EGuidFormats::DigitsWithHyphens),
				StableIdMatches));
		}
	}

	if (MaterializedShip->GetClass() != RuntimeManifest.SelectedShipClass.ResolveClass()
		|| (bShipParkedAtColony && (MaterializedShip->bProvidesArtificialGravity
			|| !MaterializedShip->bApplyExternalGravity)))
	{
		return Fail(TEXT("selected ship or natural-gravity contract mismatch"));
	}

	const AAPSCivilizationLandingPad* NativePad = Cast<AAPSCivilizationLandingPad>(MaterializedPad.Get());
	if (NativePad ? DeckOverlapsBase(MaterializedBase.Get(), NativePad)
		: FootprintsOverlap(MaterializedBase.Get(), MaterializedPad.Get()))
	{
		return Fail(TEXT("base and landing pad bounds intersect"));
	}

	if (!bManifestRestoredFromSave)
	{
		auto BottomClearance = [](const AActor* Actor, const FTransform& Support)
		{
			return OrientedBottom(Actor, Support.GetLocation(), Support.GetUnitAxis(EAxis::Z));
		};
		if (BottomClearance(MaterializedBase.Get(), Placement.BaseTransform) < -1.0
			|| BottomClearance(MaterializedPad.Get(), Placement.PadTransform) < -1.0
			|| (bShipParkedAtColony && BottomClearance(MaterializedShip.Get(), Placement.PadTransform) < -1.0))
		{
			return Fail(TEXT("starter bounds penetrate the Surface support planes"));
		}
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(APSCivilizationActorClearance), false);
	QueryParams.AddIgnoredActor(HomeBody);
	for (const FBinding& Binding : Bindings)
	{
		QueryParams.AddIgnoredActor(Binding.Actor);
	}
	for (const FBinding& Binding : Bindings)
	{
		if (Binding.Actor == MaterializedShip.Get() && !bShipParkedAtColony)
		{
			continue;
		}
		// The actor's own box, as placed: a world-axis box of a tilted pad reaches far into the ground around it.
		FBox Local = UAPSSpawnPlacementSubsystem::VisualLocalBounds(Binding.Actor);
		if (!Local.IsValid)
		{
			Local = Binding.Actor->CalculateComponentsBoundingBoxInLocalSpace(true);
		}
		if (!Local.IsValid)
		{
			continue;
		}
		const FTransform Transform = Binding.Actor->GetActorTransform();
		const FVector Extent = Local.GetExtent() * Transform.GetScale3D().GetAbs();
		const FVector QueryExtent(
			FMath::Max(1.0, Extent.X - 2.0),
			FMath::Max(1.0, Extent.Y - 2.0),
			FMath::Max(1.0, Extent.Z - 2.0));
		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByChannel(Overlaps, Transform.TransformPosition(Local.GetCenter()),
			Transform.GetRotation(), ECC_Visibility, FCollisionShape::MakeBox(QueryExtent), QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			// The structures stand on the body's own ground: its terrain and foliage collision are not blockers.
			if (const AActor* Blocker = Overlap.GetActor();
				Overlap.bBlockingHit && IsValid(Blocker) && !IsOwnedBy(Blocker, HomeBody))
			{
				return Fail(FString::Printf(TEXT("role %d overlaps blocking actor %s"),
					static_cast<int32>(Binding.Entity->Role), *GetNameSafe(Blocker)));
			}
		}
	}

	OutFailureReason.Reset();
	return true;
}

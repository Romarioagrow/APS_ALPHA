#include "APSCivilizationMaterializationSubsystem.h"

#include "APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	double ProjectedBoundsExtent(const FVector& Extent, const FVector& Direction)
	{
		return FMath::Abs(Direction.X) * Extent.X
			+ FMath::Abs(Direction.Y) * Extent.Y
			+ FMath::Abs(Direction.Z) * Extent.Z;
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
		if (Binding.Actor->GetAttachParentActor() != HomeBody)
		{
			return Fail(FString::Printf(TEXT("home-body attachment mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		const FVector RadialUp = (Binding.Actor->GetActorLocation()
			- HomeBody->GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(Binding.Actor->GetActorUpVector(), RadialUp) < 0.98)
		{
			return Fail(FString::Printf(TEXT("planet-relative orientation mismatch for role %d"),
				static_cast<int32>(Binding.Entity->Role)));
		}
		const FTransform RelativeTransform = Binding.Actor->GetActorTransform()
			.GetRelativeTransform(HomeBody->GetActorTransform());
		if (!Binding.Entity->bHasPersistedTransform
			|| !RelativeTransform.Equals(Binding.Entity->PlanetRelativeTransform, 0.5))
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
		|| MaterializedShip->bProvidesArtificialGravity
		|| !MaterializedShip->bApplyExternalGravity)
	{
		return Fail(TEXT("selected ship or natural-gravity contract mismatch"));
	}

	const FBox BaseBounds = MaterializedBase->GetComponentsBoundingBox(true);
	const FBox PadBounds = MaterializedPad->GetComponentsBoundingBox(true);
	if (BaseBounds.IsValid && PadBounds.IsValid && BaseBounds.Intersect(PadBounds))
	{
		return Fail(TEXT("base and landing pad bounds intersect"));
	}

	if (!bManifestRestoredFromSave)
	{
		auto BottomClearance = [](const AActor* Actor, const FTransform& Support)
		{
			FVector Origin;
			FVector Extent;
			Actor->GetActorBounds(false, Origin, Extent);
			const FVector Outward = Support.GetUnitAxis(EAxis::Z);
			return FVector::DotProduct(Origin - Support.GetLocation(), Outward)
				- ProjectedBoundsExtent(Extent, Outward);
		};
		if (BottomClearance(MaterializedBase.Get(), Placement.BaseTransform) < -1.0
			|| BottomClearance(MaterializedPad.Get(), Placement.PadTransform) < -1.0
			|| BottomClearance(MaterializedShip.Get(), Placement.PadTransform) < -1.0)
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
		FVector Origin;
		FVector Extent;
		Binding.Actor->GetActorBounds(false, Origin, Extent);
		const FVector QueryExtent(
			FMath::Max(1.0, Extent.X - 2.0),
			FMath::Max(1.0, Extent.Y - 2.0),
			FMath::Max(1.0, Extent.Z - 2.0));
		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity,
			ECC_Visibility, FCollisionShape::MakeBox(QueryExtent), QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (const AActor* Blocker = Overlap.GetActor();
				Overlap.bBlockingHit && IsValid(Blocker))
			{
				return Fail(FString::Printf(TEXT("role %d overlaps blocking actor %s"),
					static_cast<int32>(Binding.Entity->Role), *GetNameSafe(Blocker)));
			}
		}
	}

	OutFailureReason.Reset();
	return true;
}

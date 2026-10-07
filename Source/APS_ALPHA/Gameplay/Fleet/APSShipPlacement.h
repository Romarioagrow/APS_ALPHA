#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

/**
 * Free spots for new ships (Rio, 02.10: "ship spawn, especially the larger ones, must not put them into each other").
 * Sizes come from the hulls themselves (mesh components only: gravity spheres and interaction boxes are volumes, not
 * hulls), so a large escort or a heavy shipyard build gets room by its own size, not by the home ship's. Neighbours are
 * tested per mesh component, so a long shipyard arm does not push ships as far as its whole bounding sphere would.
 *
 * Rio 04.10 ("of eleven M ships two stood somewhere far off and the rest I could not find; spawn them beside each other
 * in a line, by their size, always in the same place"): a new ship takes the first clear place on one line to the side
 * of the spawn point, its hull box kept a margin from every hull box near it, instead of a 3D grid stepped by the hull's
 * diagonal (one oversized part sent ships rows and layers away).
 */
namespace APSShipPlacement
{
	/**
	 * A ship's meshes that make its hull: a part far larger than the primary hull (a plume, a range marker, an interior
	 * shell scaled up) does not count, so it cannot push the line apart.
	 */
	template <typename FunctionType>
	void ForEachHullMesh(const AActor* Actor, FunctionType&& Function)
	{
		double Limit = TNumericLimits<double>::Max();
		if (const ASpaceship* Ship = Cast<ASpaceship>(Actor))
		{
			// The primary hull (ASpaceship::GetPrimaryHullComponent's choice: the static hull, else the skeletal one).
			const UPrimitiveComponent* Primary = Ship->SpaceshipHull && Ship->SpaceshipHull->GetStaticMesh()
				? static_cast<const UPrimitiveComponent*>(Ship->SpaceshipHull)
				: Ship->SkeletalSpaceshipHull && Ship->SkeletalSpaceshipHull->GetSkeletalMeshAsset()
					? static_cast<const UPrimitiveComponent*>(Ship->SkeletalSpaceshipHull) : nullptr;
			if (Primary && Primary->IsRegistered())
			{
				Limit = FMath::Max(static_cast<double>(Primary->Bounds.SphereRadius) * 3.0, 2000.0);
			}
		}
		Actor->ForEachComponent<UMeshComponent>(false, [&](const UMeshComponent* Mesh)
		{
			if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0 && Mesh->Bounds.SphereRadius <= Limit)
			{
				Function(Mesh);
			}
		});
	}

	/** The box around an actor's visible hull meshes; invalid when it has none. */
	inline FBox HullBox(const AActor* Actor)
	{
		FBox Box(ForceInit);
		ForEachHullMesh(Actor, [&Box](const UMeshComponent* Mesh) { Box += Mesh->Bounds.GetBox(); });
		return Box;
	}

	/** Mesh boxes of the ships and structures (stations, shipyards, headquarters, outposts) within Reach of Anchor. */
	inline void GatherObstacles(UWorld* World, const AActor* Ignore, const FVector& Anchor, const double Reach,
		TArray<FBox>& Out)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Actor = *It;
			if (!IsValid(Actor) || Actor == Ignore || !(Actor->IsA<ASpaceship>() || Actor->IsA<ATechActor>())) continue;
			ForEachHullMesh(Actor, [&](const UMeshComponent* Mesh)
			{
				if (FVector::Dist(Mesh->Bounds.Origin, Anchor) - Mesh->Bounds.SphereRadius < Reach)
				{
					Out.Add(Mesh->Bounds.GetBox());
				}
			});
		}
	}

	/**
	 * Moves Ship (already spawned) to the first place on a line from Anchor along Frame's right where its hull box keeps
	 * Margin from every nearby hull box: the anchor itself when that is clear (a ship from a save stays put), else
	 * beside the last ship of the line. The line is walked in steps of half the hull's width, so ships of any size stand
	 * side by side. Returns the location used.
	 */
	inline FVector PlaceClear(ASpaceship& Ship, const FVector& Anchor, const FQuat& Frame, const double Margin = 5000.0)
	{
		UWorld* World = Ship.GetWorld();
		const FBox Hull = HullBox(&Ship);
		if (!World || !Hull.IsValid)
		{
			Ship.SetActorLocation(Anchor, false, nullptr, ETeleportType::TeleportPhysics);
			return Anchor;
		}
		// Where the hull sits relative to the actor's origin, so the hull (not the pivot) lands on the spot.
		const FVector PivotToHull = Hull.GetCenter() - Ship.GetActorLocation();
		const FVector Extent = Hull.GetExtent();
		const FVector Right = Frame.GetRightVector();
		// The hull's half width along the line (its world box seen along Right).
		const double HalfWidth = FMath::Abs(Right.X) * Extent.X + FMath::Abs(Right.Y) * Extent.Y + FMath::Abs(Right.Z) * Extent.Z;
		const double Step = FMath::Max(HalfWidth * 0.5, 500.0);
		constexpr int32 MaxSteps = 1024;
		TArray<FBox> Obstacles;
		GatherObstacles(World, &Ship, Anchor, Step * MaxSteps + Extent.Size() * 2.0 + Margin, Obstacles);
		const auto Clear = [&](const FVector& Centre)
		{
			const FBox Candidate = FBox(Centre - Extent, Centre + Extent).ExpandBy(Margin);
			for (const FBox& Obstacle : Obstacles)
			{
				if (Candidate.Intersect(Obstacle)) return false;
			}
			return true;
		};
		FVector Centre = Anchor;
		for (int32 Index = 0; Index < MaxSteps; ++Index)
		{
			Centre = Anchor + Right * (Index * Step);
			if (Clear(Centre))
			{
				break;
			}
		}
		const FVector Location = Centre - PivotToHull;
		Ship.SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		return Location;
	}
}

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

/**
 * Free spots for new ships (Rio, 02.10: "ship spawn, especially the larger ones, must not put them into each other").
 * Sizes come from the hulls themselves (mesh components only: gravity spheres and interaction boxes are volumes, not
 * hulls), so a large escort or a heavy shipyard build gets room by its own size, not by the home ship's. Neighbours are
 * tested per mesh component, so a long shipyard arm does not push ships as far as its whole bounding sphere would.
 */
namespace APSShipPlacement
{
	/** The box around an actor's visible meshes (its hull); invalid when it has none. */
	inline FBox HullBox(const AActor* Actor)
	{
		FBox Box(ForceInit);
		Actor->ForEachComponent<UMeshComponent>(false, [&Box](const UMeshComponent* Mesh)
		{
			if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0)
			{
				Box += Mesh->Bounds.GetBox();
			}
		});
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
			Actor->ForEachComponent<UMeshComponent>(false, [&](const UMeshComponent* Mesh)
			{
				if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->Bounds.SphereRadius > 1.0
					&& FVector::Dist(Mesh->Bounds.Origin, Anchor) - Mesh->Bounds.SphereRadius < Reach)
				{
					Out.Add(Mesh->Bounds.GetBox());
				}
			});
		}
	}

	/**
	 * Moves Ship (already spawned) to the first spot of a formation grid around Anchor where its hull keeps Margin clear
	 * of every nearby hull: the anchor, then to the sides along Right, rows further back against Forward, and a layer up
	 * along Up when a plane is full. The step is the ship's own diameter plus the margin, so big ships spread wider.
	 * Returns the location used.
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
		const double Radius = Hull.GetExtent().Size();
		// Where the hull sits relative to the actor's origin, so the hull (not the pivot) lands on the spot.
		const FVector PivotToHull = Hull.GetCenter() - Ship.GetActorLocation();
		TArray<FBox> Obstacles;
		GatherObstacles(World, &Ship, Anchor, Radius * 60.0 + 3000000.0, Obstacles);
		const FVector Right = Frame.GetRightVector();
		const FVector Back = -Frame.GetForwardVector();
		const FVector Up = Frame.GetUpVector();
		const double Step = Radius * 2.0 + Margin;
		const double Clearance = FMath::Square(Radius + Margin);
		const auto Clear = [&](const FVector& Centre)
		{
			for (const FBox& Obstacle : Obstacles)
			{
				if (Obstacle.ComputeSquaredDistanceToPoint(Centre) < Clearance) return false;
			}
			return true;
		};
		static const int32 Sides[] = {0, 1, -1, 2, -2, 3, -3, 4, -4};
		for (int32 Layer = 0; Layer < 8; ++Layer)
		{
			for (int32 Row = 0; Row < 16; ++Row)
			{
				for (const int32 Side : Sides)
				{
					const FVector Centre = Anchor + Right * (Side * Step) + Back * (Row * Step) + Up * (Layer * Step);
					if (Clear(Centre))
					{
						const FVector Location = Centre - PivotToHull;
						Ship.SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
						return Location;
					}
				}
			}
		}
		// Crowded beyond the whole grid: above it.
		const FVector Location = Anchor + Up * (Step * 9.0) - PivotToHull;
		Ship.SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		return Location;
	}
}

#include "APSBodyDesignation.h"

#include "Moon.h"
#include "Planet.h"
#include "PlanetarySystem.h"
#include "Star.h"
#include "StarSystem.h"

namespace APSBodyDesignationPrivate
{
	FString StarLetter(const AStar* Star)
	{
		// Stars are attached to their system (AAstroGenerator); the home star of a lone system is A.
		const AStarSystem* System = IsValid(Star) ? Cast<AStarSystem>(Star->GetAttachParentActor()) : nullptr;
		const int32 Index = IsValid(System) ? System->GetStars().IndexOfByKey(Star) : INDEX_NONE;
		return FString::Chr(static_cast<TCHAR>(TEXT('A') + FMath::Clamp(Index == INDEX_NONE ? 0 : Index, 0, 25)));
	}
}

FString APSBodyDesignation::Of(const AActor* Actor)
{
	using namespace APSBodyDesignationPrivate;
	if (const AStar* Star = Cast<AStar>(Actor))
	{
		return StarLetter(Star);
	}
	if (const AMoon* Moon = Cast<AMoon>(Actor))
	{
		const APlanet* Planet = Moon->ParentPlanet;
		const FString PlanetDesignation = Of(Planet);
		const int32 Index = IsValid(Planet) ? Planet->Moons.IndexOfByKey(Moon) : INDEX_NONE;
		return PlanetDesignation.IsEmpty() || Index == INDEX_NONE ? FString()
			: FString::Printf(TEXT("%s.%02d"), *PlanetDesignation, Index + 1);
	}
	if (const APlanet* Planet = Cast<APlanet>(Actor))
	{
		const AStar* Star = Planet->ParentStar;
		const APlanetarySystem* Family = IsValid(Star) ? Star->PlanetarySystem : nullptr;
		const int32 Index = IsValid(Family) ? Family->PlanetsActorsList.IndexOfByKey(Planet) : INDEX_NONE;
		return Index == INDEX_NONE ? FString() : FString::Printf(TEXT("%s%d"), *StarLetter(Star), Index + 1);
	}
	return FString();
}

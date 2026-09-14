#include "AstroGenerator.h"

#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"

FString AAstroGenerator::GetPreviewObjectStableKey(const AActor* Actor) const
{
	if (!IsValid(Actor)) return FString();
	if (const APlanetaryBody* Body = Cast<APlanetaryBody>(Actor)) return GetPreviewBodyStableKey(Body);
	const AStar* Star = Cast<AStar>(Actor);
	const AStarSystem* System = Cast<AStarSystem>(Actor);
	if (Star) System = Cast<AStarSystem>(Star->GetAttachParentActor());
	if (!IsValid(System)) return Star && Star == HomeStar ? TEXT("SYS0/S0") : FString();
	if (System != GeneratedHomeStarSystem && !System->StableSystemId.IsValid()) return FString();
	const FString Prefix = System == GeneratedHomeStarSystem ? TEXT("SYS0")
		: TEXT("SYS-") + System->StableSystemId.ToString(EGuidFormats::Digits);
	if (!Star) return Prefix;
	const int32 Index = System->GetStars().IndexOfByKey(Star);
	return Index == INDEX_NONE ? FString() : FString::Printf(TEXT("%s/S%d"), *Prefix, Index);
}

bool AAstroGenerator::ApplyPreviewDisplayNameByKey(
	const UGeneratedWorld* InGeneratedWorld, const FString& StableKey, AActor* Actor)
{
	ACelestialBody* Body = Cast<ACelestialBody>(Actor);
	if (!IsValid(InGeneratedWorld) || !IsValid(Body)) return false;
	const FString* Name = InGeneratedWorld->FindPreviewDisplayNameOverride(StableKey);
	if (!Name || Name->IsEmpty()) return false;
	Body->AstroName = FName(**Name);
	return true;
}

bool AAstroGenerator::SetPreviewObjectDisplayName(
	UGeneratedWorld* InGeneratedWorld, AActor* Actor, const FString& Name)
{
	if (!IsValid(InGeneratedWorld) || !IsValid(Actor) || !Actor->IsA<ACelestialBody>()) return false;
	const FString Key = GetPreviewObjectStableKey(Actor);
	if (!InGeneratedWorld->SetPreviewDisplayNameOverride(Key, Name)) return false;
	if (!ApplyPreviewDisplayNameByKey(InGeneratedWorld, Key, Actor)) return false;
	if (Actor == HomeStar) InGeneratedWorld->HomeStarName = HomeStar->AstroName;
	if (Actor == HomePlanet) InGeneratedWorld->HomePlanetName = HomePlanet->AstroName;
	return true;
}

void AAstroGenerator::ApplyPreviewDisplayNames(UGeneratedWorld* InGeneratedWorld)
{
	if (!IsValid(InGeneratedWorld) || InGeneratedWorld->PreviewDisplayNameOverrides.IsEmpty()) return;
	TSet<AActor*> Visited;
	const auto Apply = [&](AActor* Actor)
	{
		if (!IsValid(Actor) || Visited.Contains(Actor)) return;
		Visited.Add(Actor);
		ApplyPreviewDisplayNameByKey(InGeneratedWorld, GetPreviewObjectStableKey(Actor), Actor);
	};
	const auto ApplyTree = [&](AActor* Root)
	{
		if (!IsValid(Root)) return;
		Apply(Root);
		TArray<AActor*> Children;
		Root->GetAttachedActors(Children, true, true);
		for (AActor* Child : Children) Apply(Child);
	};
	ApplyTree(GeneratedHomeStarSystem);
	for (const auto& Pair : ContinuousMaterializedSystems) ApplyTree(Pair.Value.Get());
	// Star/planet-only generation can omit the system actor entirely.
	ApplyTree(HomeStar);
	ApplyTree(HomePlanet);
	if (IsValid(HomeStar)) InGeneratedWorld->HomeStarName = HomeStar->AstroName;
	if (IsValid(HomePlanet)) InGeneratedWorld->HomePlanetName = HomePlanet->AstroName;
}

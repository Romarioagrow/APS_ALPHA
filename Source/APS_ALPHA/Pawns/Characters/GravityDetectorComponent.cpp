#include "GravityDetectorComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GravityCharacterPawn.h" // где объявлены SwitchGravityType, CurrentGravityType, UpdateGravityPhysicParams
#include "APS_ALPHA/Actors/Astro/OrbitalBody.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "GameFramework/Character.h"

UGravityDetectorComponent::UGravityDetectorComponent()
{
	
}

void UGravityDetectorComponent::RunGravityCheck(ACharacter* Self)
{
	if (!Self) return;

	{
		//FName TagToCheck = "GravitySource";
		TArray<AActor*> GravitySources;
		TArray<AWorldActor*> WorldNavigatableActors;
		UGameplayStatics::GetAllActorsOfClass(Self->GetWorld(), AWorldActor::StaticClass(), GravitySources);
		TMap<AWorldActor*, double> ActorDistances;

		for (AActor* Actor : GravitySources)
		{
			if (Actor && Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
			{
				AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
				WorldNavigatableActors.Add(WorldNavigatableActor);

				double Distance = (FVector::Distance(Self->GetActorLocation(),
				                                     WorldNavigatableActor->GetActorLocation()) / 100000.0) -
					WorldNavigatableActor->RadiusKM;
				ActorDistances.Add(WorldNavigatableActor, Distance);
			}
		}

		WorldNavigatableActors.Sort([&](const AWorldActor& A, const AWorldActor& B)
		{
			return ActorDistances[&A] < ActorDistances[&B];
		});

		if (WorldNavigatableActors.Num() > 0)
		{
			AWorldActor* ClosestActor = WorldNavigatableActors[0];

			FString DebugMessageClosest = FString::Printf(
				TEXT("Closest Actor: %s \nDistance to surface: %f km \nAffectionRadiusKM: %f"),
				*ClosestActor->GetFName().ToString(), ActorDistances[ClosestActor],
				ClosestActor->AffectionRadiusKM);
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, DebugMessageClosest);

			if (ActorDistances[ClosestActor] <= ClosestActor->AffectionRadiusKM)
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green,
					FString::Printf(TEXT("Affected Actor: %s"), *ClosestActor->GetFName().ToString()));
				SwitchGravityType(ClosestActor);
			}
			else
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("No Actor within AffectionRadiusKM"));
				CurrentGravityType = EGravityType::ZeroG;
				//UpdateGravityPhysicParams();
			}
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("No Closest Gravity Actor"));
			CurrentGravityType = EGravityType::ZeroG;
			//Self->UpdateGravityPhysicParams();
		}
	}
}

void UGravityDetectorComponent::SwitchGravityType(AActor* GravitySourceActor)
{
	GravityTargetActor = GravitySourceActor;

	//ClosestBody = NewClosest;
	OnClosestGravityBodyChanged.Broadcast(GravitySourceActor);

	if (GravitySourceActor->IsA(ASpaceStation::StaticClass()) || GravitySourceActor->IsA(
		ASpaceHeadquarters::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnStation;
		OnGravityPhysicsParamChanged.Broadcast();
	}
	else if (GravitySourceActor->IsA(AOrbitalBody::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnPlanet;
		OnGravityPhysicsParamChanged.Broadcast();
	}
	else if (GravitySourceActor->IsA(ASpaceship::StaticClass()))
	{
		CurrentGravityType = EGravityType::OnShip;
		ASpaceship* Spaceship = Cast<ASpaceship>(GravitySourceActor);
		if (Spaceship != nullptr)
		{
			CurrentSpaceship = Spaceship;
			OnGravityPhysicsParamChanged.Broadcast();
		}
	}
	// Remove ship
	if (CurrentGravityType != EGravityType::OnShip)
	{
		CurrentSpaceship = nullptr;
	}

	// switch gravity param
	//UpdateGravityPhysicParams();
}
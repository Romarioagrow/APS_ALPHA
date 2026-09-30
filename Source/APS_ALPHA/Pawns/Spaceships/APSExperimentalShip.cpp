#include "APSExperimentalShip.h"

#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace APSExperimentalShipLocal
{
	APlayerController* FindPlayerController(UWorld* World)
	{
		if (APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
			PlayerController && PlayerController->GetPawn())
		{
			return PlayerController;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType != EWorldType::PIE && Context.WorldType != EWorldType::Game)
				{
					continue;
				}
				if (APlayerController* PlayerController = Context.World() ? Context.World()->GetFirstPlayerController() : nullptr;
					PlayerController && PlayerController->GetPawn())
				{
					return PlayerController;
				}
			}
		}
		return nullptr;
	}

	/**
	 * aps.ExpShip.Spawn [StaticMeshPath]: an experimental ship with the hull, nose and class of the piloted ship
	 * (or the given mesh) appears beside it and the pilot moves over. The old ship stays parked.
	 */
	void SpawnExperimentalShip(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PlayerController = FindPlayerController(World);
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] no player pawn; start a game first."));
			return;
		}
		ASpaceship* Source = Cast<ASpaceship>(Pawn);
		UStaticMesh* Mesh = Args.Num() > 0
			? LoadObject<UStaticMesh>(nullptr, *Args[0])
			: (Source && Source->SpaceshipHull ? Source->SpaceshipHull->GetStaticMesh().Get() : nullptr);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.ExpShip] pilot a ship with a static hull or pass a mesh: aps.ExpShip.Spawn /Game/Path/Mesh.Mesh"));
			return;
		}
		APawn* Character = Source ? Source->Pilot.Get() : Pawn;
		if (!IsValid(Character))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] no pilot character to move over."));
			return;
		}

		FTransform Transform = Source ? Source->GetActorTransform() : Character->GetActorTransform();
		const double Scale = Transform.GetScale3D().GetAbsMax();
		const double NewRadius = Mesh->GetBounds().SphereRadius * FMath::Max(Scale, 0.01);
		const double SourceRadius = Source && Source->SpaceshipHull ? Source->SpaceshipHull->Bounds.SphereRadius : 300.0;
		const FVector Side = Source ? Source->GetActorRightVector() : Character->GetActorForwardVector();
		Transform.SetLocation(Transform.GetLocation() + Side * (SourceRadius + NewRadius) * 1.25);
		if (!Source)
		{
			Transform.SetScale3D(FVector::OneVector);
		}

		UWorld* ShipWorld = Pawn->GetWorld();
		AAPSExperimentalShip* Ship = ShipWorld->SpawnActorDeferred<AAPSExperimentalShip>(
			AAPSExperimentalShip::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Ship)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] spawn failed."));
			return;
		}
		Ship->SpaceshipHull->SetStaticMesh(Mesh);
		Ship->bGenerateSimpleHullCollision = true;
		if (Source && Source->SpaceshipHull && Source->SpaceshipHull->GetStaticMesh() == Mesh)
		{
			// Hull Blueprints override materials per slot; the mesh defaults would change the look.
			for (int32 Slot = 0; Slot < Source->SpaceshipHull->GetNumMaterials(); ++Slot)
			{
				Ship->SpaceshipHull->SetMaterial(Slot, Source->SpaceshipHull->GetMaterial(Slot));
			}
		}
		if (Source)
		{
			Ship->SizeClass = Source->SizeClass;
			Ship->bInferSizeClassFromHull = Source->bInferSizeClassFromHull;
			Ship->bUseAuthoredNoseDirection = Source->bUseAuthoredNoseDirection;
			Ship->bHasInterior = Source->bHasInterior;
			Ship->bProvidesArtificialGravity = Source->bProvidesArtificialGravity;
			if (Source->ForwardVector && Ship->ForwardVector)
			{
				Ship->ForwardVector->SetRelativeTransform(Source->ForwardVector->GetRelativeTransform());
			}
		}
		else
		{
			Ship->bInferSizeClassFromHull = true;
		}
		Ship->FinishSpawning(Transform);

		if (Source && !Source->RequestReleaseVehicleControl())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] could not leave %s; the new ship waits beside it."),
				*Source->GetName());
			return;
		}
		if (!Ship->RequestVehicleControl(Character))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] %s spawned but could not take the pilot."), *Ship->GetName());
			return;
		}
		if (!Ship->bEngineRunning)
		{
			Ship->SwitchEngines();
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.ExpShip] spawned %s hull=%s class=%s from=%s"), *Ship->GetName(),
			*Mesh->GetPathName(), *Ship->GetSizeClassName(), Source ? *Source->GetName() : TEXT("character"));
	}

	/** aps.ExpShip.Drive <Band 1-5> <Forward -1..1> [Boost 0|1] | aps.ExpShip.Drive off */
	void DriveBandShip(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PlayerController = FindPlayerController(World);
		ASpaceship* Ship = PlayerController ? Cast<ASpaceship>(PlayerController->GetPawn()) : nullptr;
		UAPSShipFlightModel* Model = Ship ? Ship->FlightModel : nullptr;
		if (!Model || !Model->IsBandFlightActive())
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.ExpShip] pilot a ship with the band model (aps.Ship.FlightModel 1) first."));
			return;
		}
		if (Args.IsEmpty() || Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			Model->SetDebugDrive(false, 0.0f, false);
			return;
		}
		const int32 Band = FMath::Clamp(FCString::Atoi(*Args[0]), 1, static_cast<int32>(EAPSFlightBand::Stellar) + 1);
		const float Forward = Args.Num() > 1 ? FMath::Clamp(FCString::Atof(*Args[1]), -1.0f, 1.0f) : 1.0f;
		const bool bBoost = Args.Num() > 2 && FCString::Atoi(*Args[2]) != 0;
		if (!Ship->bEngineRunning)
		{
			Ship->SwitchEngines();
		}
		Model->SetFlightBand(static_cast<EAPSFlightBand>(Band - 1));
		Model->SetDebugDrive(true, Forward, bBoost);
	}

	FAutoConsoleCommandWithWorldAndArgs DriveCommand(
		TEXT("aps.ExpShip.Drive"),
		TEXT("aps.ExpShip.Drive <Band 1-5> <Forward -1..1> [Boost 0|1] | off: test drive of the piloted band-model ship ")
		TEXT("without a keyboard."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DriveBandShip));

	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(
		TEXT("aps.ExpShip.Spawn"),
		TEXT("aps.ExpShip.Spawn [StaticMeshPath]: experimental band-flight ship with the piloted ship's hull (or the ")
		TEXT("given mesh) beside it; the pilot moves over."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnExperimentalShip));
}

AAPSExperimentalShip::AAPSExperimentalShip()
{
	// Generated hulls: fitted convex collision when the mesh has it, cheap boxes otherwise.
	bGenerateSimpleHullCollision = true;
	if (FlightModel)
	{
		FlightModel->bAlwaysUseBands = true;
	}
}

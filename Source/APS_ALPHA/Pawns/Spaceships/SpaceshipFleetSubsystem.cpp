#include "SpaceshipFleetSubsystem.h"

#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif

bool USpaceshipFleetSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	const bool bEditorConversionRequested = World
		&& World->WorldType == EWorldType::Editor
		&& FParse::Param(FCommandLine::Get(), TEXT("APSConvertShipMeshesAndSave"));
	return World && (World->WorldType == EWorldType::Game
		|| World->WorldType == EWorldType::PIE
		|| bEditorConversionRequested);
}

void USpaceshipFleetSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!bFleetBuilt)
	{
		BuildRuntimeShipFleet();
		bFleetBuilt = true;
	}
}

void USpaceshipFleetSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!bFleetBuilt && World && World->WorldType == EWorldType::Editor)
	{
		const int32 ConvertedCount = BuildRuntimeShipFleet();
		bFleetBuilt = true;
#if WITH_EDITOR
		const FString MapPackageName = World->GetOutermost()->GetName();
		const bool bSaved = ConvertedCount > 0
			&& UEditorLoadingAndSavingUtils::SaveMap(World, MapPackageName);
		if (bSaved)
		{
			UE_LOG(LogTemp, Display, TEXT("[APS.Ships] Persistent conversion saved=true map=%s converted=%d"),
				*MapPackageName, ConvertedCount);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Ships] Persistent conversion saved=false map=%s converted=%d"),
				*MapPackageName, ConvertedCount);
		}
		FPlatformMisc::RequestExit(false);
#endif
	}
}

TStatId USpaceshipFleetSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USpaceshipFleetSubsystem, STATGROUP_Tickables);
}

bool USpaceshipFleetSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return Super::IsTickable()
		&& !bFleetBuilt
		&& World
		&& World->WorldType == EWorldType::Editor;
}

int32 USpaceshipFleetSubsystem::BuildRuntimeShipFleet()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<AActor*> MeshActors;
	UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), MeshActors);
	int32 ConvertedCount = 0;
	for (AActor* Candidate : MeshActors)
	{
		AStaticMeshActor* SourceActor = Cast<AStaticMeshActor>(Candidate);
		UStaticMeshComponent* SourceMesh = SourceActor ? SourceActor->GetStaticMeshComponent() : nullptr;
		if (!SourceMesh || !ASpaceship::IsGeneratedShipMeshAsset(SourceMesh->GetStaticMesh()))
		{
			continue;
		}

		const FTransform SourceTransform = SourceActor->GetActorTransform();
		ASpaceship* Ship = World->SpawnActorDeferred<ASpaceship>(
			ASpaceship::StaticClass(), SourceTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Ship || !Ship->SpaceshipHull)
		{
			continue;
		}

		Ship->bInferSizeClassFromHull = true;
		Ship->bHasInterior = false;
		Ship->bAllowExteriorInteraction = true;
		Ship->bProvidesArtificialGravity = false;
		Ship->bGenerateSimpleHullCollision = true;
		Ship->Tags = SourceActor->Tags;
		Ship->Tags.AddUnique(TEXT("APS.GeneratedShip"));
		Ship->SpaceshipHull->SetStaticMesh(SourceMesh->GetStaticMesh());
		Ship->SpaceshipHull->SetCollisionProfileName(SourceMesh->GetCollisionProfileName());
		const ECollisionEnabled::Type SourceCollision = SourceMesh->GetCollisionEnabled();
		Ship->SpaceshipHull->SetCollisionEnabled(
			SourceCollision == ECollisionEnabled::NoCollision ? ECollisionEnabled::QueryOnly : SourceCollision);
		Ship->SpaceshipHull->SetCollisionResponseToChannels(SourceMesh->GetCollisionResponseToChannels());
		Ship->SpaceshipHull->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Ship->SpaceshipHull->SetGenerateOverlapEvents(SourceMesh->GetGenerateOverlapEvents());
		Ship->SpaceshipHull->SetCastShadow(SourceMesh->CastShadow);
		for (int32 MaterialIndex = 0; MaterialIndex < SourceMesh->GetNumMaterials(); ++MaterialIndex)
		{
			Ship->SpaceshipHull->SetMaterial(MaterialIndex, SourceMesh->GetMaterial(MaterialIndex));
		}

		UGameplayStatics::FinishSpawningActor(Ship, SourceTransform);
#if WITH_EDITOR
		if (World->WorldType == EWorldType::Editor)
		{
			Ship->SetActorLabel(SourceActor->GetActorLabel());
			Ship->SetFolderPath(SourceActor->GetFolderPath());
			Ship->SetFlags(RF_Transactional);
		}
#endif
		SourceActor->Destroy();
		++ConvertedCount;
	}

	TArray<AActor*> SkeletalMeshActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASkeletalMeshActor::StaticClass(), SkeletalMeshActors);
	for (AActor* Candidate : SkeletalMeshActors)
	{
		ASkeletalMeshActor* SourceActor = Cast<ASkeletalMeshActor>(Candidate);
		USkeletalMeshComponent* SourceMesh = SourceActor ? SourceActor->GetSkeletalMeshComponent() : nullptr;
		if (!SourceMesh || !ASpaceship::IsGeneratedShipSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset()))
		{
			continue;
		}

		const FTransform SourceTransform = SourceActor->GetActorTransform();
		ASpaceship* Ship = World->SpawnActorDeferred<ASpaceship>(
			ASpaceship::StaticClass(), SourceTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Ship || !Ship->SkeletalSpaceshipHull)
		{
			continue;
		}

		Ship->bInferSizeClassFromHull = true;
		Ship->bHasInterior = false;
		Ship->bAllowExteriorInteraction = true;
		Ship->bProvidesArtificialGravity = false;
		Ship->bGenerateSimpleHullCollision = true;
		Ship->Tags = SourceActor->Tags;
		Ship->Tags.AddUnique(TEXT("APS.GeneratedShip"));
		Ship->SkeletalSpaceshipHull->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());
		Ship->SkeletalSpaceshipHull->SetCollisionProfileName(SourceMesh->GetCollisionProfileName());
		Ship->SkeletalSpaceshipHull->SetCollisionEnabled(SourceMesh->GetCollisionEnabled());
		Ship->SkeletalSpaceshipHull->SetCollisionResponseToChannels(SourceMesh->GetCollisionResponseToChannels());
		Ship->SkeletalSpaceshipHull->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Ship->SkeletalSpaceshipHull->SetGenerateOverlapEvents(false);
		Ship->SkeletalSpaceshipHull->SetCastShadow(SourceMesh->CastShadow);
		for (int32 MaterialIndex = 0; MaterialIndex < SourceMesh->GetNumMaterials(); ++MaterialIndex)
		{
			Ship->SkeletalSpaceshipHull->SetMaterial(MaterialIndex, SourceMesh->GetMaterial(MaterialIndex));
		}

		UGameplayStatics::FinishSpawningActor(Ship, SourceTransform);
#if WITH_EDITOR
		if (World->WorldType == EWorldType::Editor)
		{
			Ship->SetActorLabel(SourceActor->GetActorLabel());
			Ship->SetFolderPath(SourceActor->GetFolderPath());
			Ship->SetFlags(RF_Transactional);
		}
#endif
		SourceActor->Destroy();
		++ConvertedCount;
	}
	if (ConvertedCount > 0 && World->WorldType == EWorldType::Editor)
	{
		World->MarkPackageDirty();
	}

	TArray<AActor*> ShipActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASpaceship::StaticClass(), ShipActors);
	if (FParse::Param(FCommandLine::Get(), TEXT("APSReportShipFleet")))
	{
		for (AActor* ShipActor : ShipActors)
		{
			ASpaceship* Ship = Cast<ASpaceship>(ShipActor);
			if (!Ship || !Ship->Tags.Contains(TEXT("APS.GeneratedShip")))
			{
				continue;
			}
			const UPrimitiveComponent* Hull = Ship->SpaceshipHull && Ship->SpaceshipHull->GetStaticMesh()
				? Cast<UPrimitiveComponent>(Ship->SpaceshipHull)
				: Cast<UPrimitiveComponent>(Ship->SkeletalSpaceshipHull);
			FString AssetPath = TEXT("None");
			if (Ship->SpaceshipHull && Ship->SpaceshipHull->GetStaticMesh())
			{
				AssetPath = Ship->SpaceshipHull->GetStaticMesh()->GetPathName();
			}
			else if (Ship->SkeletalSpaceshipHull && Ship->SkeletalSpaceshipHull->GetSkeletalMeshAsset())
			{
				AssetPath = Ship->SkeletalSpaceshipHull->GetSkeletalMeshAsset()->GetPathName();
			}
#if WITH_EDITOR
			const FString ActorLabel = Ship->GetActorLabel();
			const FString FolderPath = Ship->GetFolderPath().ToString();
#else
			const FString ActorLabel = Ship->GetName();
			const FString FolderPath = TEXT("");
#endif
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Ships.Inventory] actor=%s label=%s folder=%s asset=%s size=%s"),
				*Ship->GetName(), *ActorLabel, *FolderPath, *AssetPath,
				Hull ? *Hull->Bounds.BoxExtent.ToString() : TEXT("None"));
		}
	}
	const TCHAR* WorldTypeName = World->WorldType == EWorldType::Editor ? TEXT("Editor")
		: World->WorldType == EWorldType::PIE ? TEXT("PIE") : TEXT("Game");
	UE_LOG(LogTemp, Warning,
		TEXT("[APS.Ships] Fleet subsystem world=%s type=%s converted=%d total_ships=%d source=/Game/APS/APS_ALPHA/Assets/AI_Shpis"),
		*GetNameSafe(World), WorldTypeName, ConvertedCount, ShipActors.Num());
	return ConvertedCount;
}

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSStarRenderStabilitySubsystem.generated.h"

class AActor;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;

/** Keeps generated astronomical point meshes deterministic for temporal rendering. */
UCLASS()
class APS_ALPHA_API UAPSStarRenderStabilitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HandleActorSpawned(AActor* Actor);
	void StabilizeActor(AActor* Actor) const;
	static void StabilizeInstances(UHierarchicalInstancedStaticMeshComponent* Instances);
	static bool StabilizeTranslucentNaniteMesh(UStaticMeshComponent* Component);

	FDelegateHandle ActorSpawnedHandle;
};

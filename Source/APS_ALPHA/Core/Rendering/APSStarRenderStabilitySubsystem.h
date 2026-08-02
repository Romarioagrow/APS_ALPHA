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

	/**
	 * Applies the large-world/temporal rendering contract to a completed star instance set.
	 * Generators must call this after their final AddInstance because actor-spawn callbacks run
	 * before Blueprint construction and procedural instance population have finished.
	 */
	static void StabilizeInstances(UHierarchicalInstancedStaticMeshComponent* Instances);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HandleActorSpawned(AActor* Actor);
	void StabilizeActor(AActor* Actor) const;
	static bool StabilizeTranslucentNaniteMesh(UStaticMeshComponent* Component);

	FDelegateHandle ActorSpawnedHandle;
};

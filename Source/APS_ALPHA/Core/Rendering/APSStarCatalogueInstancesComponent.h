#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "APSStarCatalogueInstancesComponent.generated.h"

struct FRenderTransform;

/**
 * Rio 06.10 (73-87 fps after the star drive; audit: the catalogue's 25k + 36k instance transforms re-gathered on the game
 * thread on every sky move, 1.6-3.6 ms a frame): the galaxy's and the cluster's StarMeshInstances. A HISM in every respect
 * (the member type, the Blueprints' serialized mesh and custom-float count, every reader); only the change set's transform
 * gather, when it covers every instance, runs on the task workers (aps.Stars.CatalogueParallelGather 1, the default): the
 * engine's own expression for each instance in the engine's order, so the renderer receives the same bytes.
 */
UCLASS(ClassGroup = Rendering)
class APS_ALPHA_API UAPSStarCatalogueInstancesComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UAPSStarCatalogueInstancesComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

	/** True while it hangs under a gameplay AstroGenerator (never the menu preview's); set on the game thread at each move. */
	bool IsGameplaySkyCatalogue() const { return bGameplaySky; }

protected:
	virtual void BuildComponentInstanceData(ERHIFeatureLevel::Type FeatureLevel, FInstanceUpdateComponentDesc& OutData) override;

private:
	void CountVerify(const TArray<FRenderTransform>& EngineTransforms, const TArray<FRenderTransform>& ParallelTransforms);

	bool bGameplaySky = false;
	bool bLoggedParallel = false;
	int64 VerifyFlushes = 0;
	int64 VerifyMismatches = 0;
	double VerifyLogSeconds = 0.0;
};

namespace APSStarCatalogue
{
	/** Emergency compile-time switch, valid ONLY while no asset has been saved with this class; the normal rollback is
	 * aps.Stars.CatalogueParallelGather 0 (pure pass-through). After BP_Galaxy / BP_StarCluster / a map were saved with it,
	 * false (or deleting the class) also needs [CoreRedirects] +ClassRedirects=(OldName="/Script/APS_ALPHA.APSStarCatalogueInstancesComponent",
	 * NewName="/Script/Engine.HierarchicalInstancedStaticMeshComponent"), else LinkerLoad fails the import and the BP loses its mesh. */
	inline constexpr bool bSubclass = false; // Rio 06.10 night: measured (g1 A/B): bitwise identical, 0.4-0.6 ms of game thread saved, no fps gain; off until a real win
}

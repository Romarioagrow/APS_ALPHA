#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APSColonyModule.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FAPSColonyModulePart;

/**
 * A module the colony built (U1): its catalogue entry's parts, on a concrete plinth that reaches the ground (surface) or
 * on a boom bolted to the headquarters (orbit). Pack meshes are used where installed and engine shapes otherwise, so
 * a checkout without the SpaceColonies pack still builds every module.
 */
UCLASS(BlueprintType)
class APS_ALPHA_API AAPSColonyModule : public ATechActor
{
	GENERATED_BODY()

public:
	AAPSColonyModule();

	/** Before FinishSpawning: the catalogue entry, identity, and what the placement measured. */
	void Configure(FName InModuleId, const FGuid& InStableId, const FGuid& InOwnerCivilizationId,
		double InFoundationDepthCm, double InBoomLengthCm);

	UFUNCTION(BlueprintPure, Category="Colony")
	FName GetModuleId() const { return ModuleId; }

	const FGuid& GetStableId() const { return StableId; }
	const FGuid& GetOwnerCivilizationId() const { return OwnerCivilizationId; }
	double GetFoundationDepthCm() const { return FoundationDepthCm; }
	double GetBoomLengthCm() const { return BoomLengthCm; }
	int32 GetPartCount() const { return PartCount; }
	/** Parts whose pack mesh is not installed (they fell back to an engine shape or were left out). */
	int32 GetFallbackPartCount() const { return FallbackPartCount; }

	/** Assembles the parts once; BeginPlay calls it. */
	void BuildParts();

protected:
	virtual void BeginPlay() override;

private:
	void AddPart(const FAPSColonyModulePart& Part, const FVector& Lift);
	UStaticMeshComponent* AddMeshComponent(UStaticMesh* Mesh, const FTransform& RelativeTransform, bool bCollides);
	UMaterialInterface* ShapeMaterial(const FLinearColor& Color, float Glow);

	UPROPERTY(VisibleAnywhere, Category="Colony")
	TObjectPtr<USceneComponent> ModuleRoot;

	UPROPERTY(VisibleAnywhere, Category="Colony")
	FName ModuleId;

	UPROPERTY(VisibleAnywhere, Category="Colony")
	FGuid StableId;

	UPROPERTY(VisibleAnywhere, Category="Colony")
	FGuid OwnerCivilizationId;

	UPROPERTY(VisibleAnywhere, Category="Colony")
	double FoundationDepthCm{0.0};

	UPROPERTY(VisibleAnywhere, Category="Colony")
	double BoomLengthCm{0.0};

	/** One material instance per colour and glow. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInterface>> MaterialCache;

	int32 PartCount{0};
	int32 FallbackPartCount{0};
	bool bPartsBuilt{false};
};

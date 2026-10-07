#pragma once

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "Galaxy.generated.h"

/** Lightweight, reproducible data for one logical galaxy star. */
USTRUCT(BlueprintType)
struct FGalaxyCatalogStarRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	FGuid StableId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int64 CatalogIndex{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	FVector GalaxyLocalLocation{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int32 GenerationSeed{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	ESpectralClass SpectralClass{ESpectralClass::G};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int32 SpectralSubclass{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	bool bPotentialStarSystem{true}; float RadiusScale{1.0f}; // Rio 03.10: galaxy POPULATION size factor (not reflected; one line keeps UHT line numbers)
};

/**
 * The complete galaxy model is a deterministic indexed catalog, not a giant
 * array of actors. Any logical star can be reconstructed in O(1), while only
 * a bounded prefix of one deterministic full-cycle order is sent to HISM.
 */
USTRUCT(BlueprintType)
struct FGalaxyCatalogDescriptor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int32 GenerationSeed{271828};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int64 ModeledStarCount{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int32 RenderedSampleCount{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	int32 GalaxySize{250};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	double StarDensity{10.0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	EGalaxyType GalaxyType{EGalaxyType::Elliptical};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	EGalaxyClass GalaxyClass{EGalaxyClass::E0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	FVector CatalogHalfExtent{FVector::ZeroVector}; uint8 StarPopulation{0}; uint8 StarComposition{0}; // Rio 03.10: galaxy POPULATION/COMPOSITION (EStarCluster* values; not reflected, one line keeps UHT line numbers)

	FGuid MakeStableStarId(int64 CatalogIndex) const;
	bool ResolveStar(int64 CatalogIndex, FGalaxyCatalogStarRecord& OutRecord) const;
};

UCLASS()
class APS_ALPHA_API AGalaxy : public ACelestialSystem
{
	GENERATED_BODY()

public:
	AGalaxy();
	bool EnsureCanonicalStellarMaterial();

protected:
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(VisibleAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Stars")
	UHierarchicalInstancedStaticMeshComponent* StarMeshInstances;

	/** Full logical catalog; RenderedSampleCount is only its current visual LOD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Galaxy|Catalog")
	FGalaxyCatalogDescriptor StarCatalog;

	/** Immutable HISM instance -> canonical catalog mapping for the current render LOD. */
	TArray<int64> RenderedCatalogIndices;
	/** Immutable one-time projected transforms before any explicit view-only presentation pass. */
	TArray<FTransform> RenderedProxyBaseTransforms;
	FAPSCanonicalStellarProjectionFrame CanonicalProjectionFrame;

	UFUNCTION(BlueprintCallable, Category = "Galaxy|Catalog")
	bool GetCatalogStarRecord(int64 CatalogIndex, FGalaxyCatalogStarRecord& OutRecord) const;
	bool GetRenderedCatalogIndex(int32 InstanceIndex, int64& OutCatalogIndex) const;
	bool GetRenderedCatalogRecord(int32 InstanceIndex, FGalaxyCatalogStarRecord& OutRecord) const;
	bool GetRenderedProxyBaseTransform(int32 InstanceIndex, FTransform& OutTransform) const;

	/**
	 * Rio 03.10 (galaxy phase 3): GPU points + glow of the catalogue beyond the ISM prefix (APSGalaxyGpuStars.h,
	 * Plugins/APSStarRenderer). Built on a background task, only while aps.Stars.GpuPoints / aps.Stars.GalaxyGlow ask.
	 */
	void RebuildGpuStarLayer();
	void ReleaseGpuStarLayer();
	/** APSStarRenderer handles (0 = none) and the build state; owned by APSGalaxyGpuStars. */
	uint32 GpuPointSet{0};
	uint32 GpuGlowVolume{0};
	uint32 GpuStarLayerSerial{0};
	bool bGpuStarLayerActive{false};
	bool bGpuStarLayerBuilding{false};
};

namespace APSGalaxyCatalogBatch
{
	/**
	 * Rio 03.10 (up to 1M placed stars): resolves render ordinals [FirstOrdinal, FirstOrdinal +
	 * Count) of a nested catalogue order, in parallel for large ranges. Pure catalogue reads, so
	 * the records equal one-by-one ResolveStar calls. An unresolvable ordinal keeps
	 * CatalogIndex == INDEX_NONE. Returns the number of resolved records.
	 */
	APS_ALPHA_API int32 ResolveStars(const FGalaxyCatalogDescriptor& Catalog,
		const APSCanonicalStellarProjection::FNestedCatalogPermutation& Order,
		int32 FirstOrdinal, int32 Count, TArray<FGalaxyCatalogStarRecord>& OutRecords);
}

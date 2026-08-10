#pragma once

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CoreMinimal.h"
#include "CelestialSystem.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
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
	bool bPotentialStarSystem{true};
};

/**
 * The complete galaxy model is a deterministic indexed catalog, not a giant
 * array of actors. Any logical star can be reconstructed in O(1), while only
 * a bounded stratified sample is sent to HISM for rendering.
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
	FVector CatalogHalfExtent{FVector::ZeroVector};

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

	UFUNCTION(BlueprintCallable, Category = "Galaxy|Catalog")
	bool GetCatalogStarRecord(int64 CatalogIndex, FGalaxyCatalogStarRecord& OutRecord) const;
};

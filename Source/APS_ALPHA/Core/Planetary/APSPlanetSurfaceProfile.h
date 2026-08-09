#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APSPlanetSurfaceProfile.generated.h"

class APlanetaryBody;
class UMaterialInstance;
class UMaterialInstanceDynamic;

/** Broad surface generators. User-facing planet presets remain compatible and resolve into one of these families. */
UENUM(BlueprintType)
enum class EAPSPlanetSurfaceArchetype : uint8
{
	Rocky UMETA(DisplayName = "Rocky Worlds"),
	Temperate UMETA(DisplayName = "Temperate Worlds"),
	Oceanic UMETA(DisplayName = "Ocean Worlds"),
	Biosphere UMETA(DisplayName = "Living Worlds"),
	Desert UMETA(DisplayName = "Desert Worlds"),
	Cryogenic UMETA(DisplayName = "Frozen Worlds"),
	Magmatic UMETA(DisplayName = "Volcanic Worlds"),
	Metallic UMETA(DisplayName = "Metallic Worlds"),
	ExoticChemical UMETA(DisplayName = "Exotic Chemical Worlds")
};

UENUM(BlueprintType)
enum class EAPSPlanetSurfaceMaterialFamily : uint8
{
	Temperate,
	Barren,
	Magmatic
};

UENUM(BlueprintType)
enum class EAPSPlanetLiquidType : uint8
{
	None,
	Water,
	Lava,
	Ammonia
};

/** Modifiers are intentionally independent of planet type so physical simulation can add them to any solid world. */
UENUM(BlueprintType, meta = (Bitflags))
enum class EAPSPlanetSurfaceModifier : uint8
{
	None = 0,
	SuperEarth = 1,
	Dwarf = 2,
	Rogue = 3,
	HighMountain = 4,
	Pangea = 5,
	Archipelago = 6,
	Nordic = 7,
	Oasis = 8,
	Greenhouse = 9,
	ActiveGeology = 10,
	DenseAtmosphere = 11,
	RichBiosphere = 12,
	TidallyLocked = 13,
	AbyssalBasins = 14,
	ContinentalShelves = 15,
	ForestCanopy = 16,
	IceSheets = 17,
	GlacialRifts = 18,
	LavaSeas = 19,
	CalderaFields = 20,
	MetallicPlates = 21,
	CarbonRidges = 22,
	ChemicalBands = 23,
	SandDunes = 24,
	TundraBands = 25,
	OasisWetlands = 26,
	MesaFields = 27,
	AtmosphericErosion = 28,
	AlienTerrain = 29,
	ArchipelagoChains = 30,
	VolcanicFissures = 31
};

USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSPlanetSurfacePalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Coast = FLinearColor(0.18f, 0.14f, 0.09f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Lowland = FLinearColor(0.16f, 0.24f, 0.09f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor MidLowland = FLinearColor(0.22f, 0.32f, 0.12f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Highland = FLinearColor(0.32f, 0.26f, 0.17f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Dryland = FLinearColor(0.48f, 0.35f, 0.18f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Peak = FLinearColor(0.72f, 0.72f, 0.68f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Slope = FLinearColor(0.24f, 0.22f, 0.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Emissive = FLinearColor::Black;
};

/** Editable ranges for an archetype. A catalog asset may override the native defaults without changing C++. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSPlanetSurfaceArchetypeDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets")
	TSoftObjectPtr<UMaterialInstance> TerrainMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets")
	TSoftObjectPtr<UMaterialInstance> OceanMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAPSPlanetSurfaceMaterialFamily MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAPSPlanetLiquidType LiquidType = EAPSPlanetLiquidType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D NoiseScale = FVector2D(500.0, 850.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D NoiseIntensity = FVector2D(700000.0, 1200000.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D LandCoverage = FVector2D(0.75, 1.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D MountainStrength = FVector2D(0.25, 0.65);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D HillStrength = FVector2D(0.15, 0.45);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D CraterStrength = FVector2D(0.1, 0.6);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D BaseTemperature = FVector2D(0.25, 0.65);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D BaseHumidity = FVector2D(0.05, 0.4);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Roughness = FVector2D(0.6, 0.95);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Metallic = FVector2D(0.0, 0.08);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D EmissiveStrength = FVector2D(0.0, 0.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D BiomeContrast = FVector2D(0.7, 1.2);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FAPSPlanetSurfacePalette Palette;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float PaletteHueVariationDegrees = 8.0f;
};

/** Immutable per-body result consumed by WorldScape noise and material code. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSResolvedPlanetSurfaceProfile
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EPlanetType PlanetType = EPlanetType::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAPSPlanetSurfaceArchetype Archetype = EAPSPlanetSurfaceArchetype::Rocky;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (Bitmask, BitmaskEnum = "/Script/APS_ALPHA.EAPSPlanetSurfaceModifier"))
	int64 ModifierMask = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAPSPlanetSurfaceMaterialFamily MaterialFamily = EAPSPlanetSurfaceMaterialFamily::Barren;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EAPSPlanetLiquidType LiquidType = EAPSPlanetLiquidType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TerrainSeed = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BiomeSeed = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PaletteSeed = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NoiseScale = 650.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NoiseIntensity = 900000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LandCoverage = 0.85f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MountainStrength = 0.45f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HillStrength = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CraterStrength = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Temperature = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Humidity = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Biomass = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Biodiversity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SeismicActivity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CrustThickness = 0.35f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AtmosphericPressure = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Roughness = 0.85f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Metallic = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EmissiveStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BiomeContrast = 1.0f;

	/** Per-preset shape controls. They keep amplitudes bounded and do not change WorldScape LOD topology. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ContinentalFrequencyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RegionalFrequencyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DetailFrequencyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RidgeFrequencyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CellularFrequencyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TerrainPatternStrength = 0.0f;

	/** Coarse biome patch amplitude written into WorldScape temperature/humidity channels. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ClimatePatchStrength = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LatitudeClimateStrength = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float OceanLevel = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FAPSPlanetSurfacePalette Palette;

	bool HasModifier(EAPSPlanetSurfaceModifier Modifier) const
	{
		return Modifier != EAPSPlanetSurfaceModifier::None
			&& (ModifierMask & (int64{1} << static_cast<uint8>(Modifier))) != 0;
	}
};

/** Optional project asset. Missing entries fall back to deterministic native definitions. */
UCLASS(BlueprintType)
class APS_ALPHA_API UAPSPlanetSurfaceCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface")
	TMap<EAPSPlanetSurfaceArchetype, FAPSPlanetSurfaceArchetypeDefinition> Archetypes;
};

UCLASS()
class APS_ALPHA_API UAPSPlanetSurfaceProfileResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "APS|Planet Surface")
	static EAPSPlanetSurfaceArchetype GetArchetypeForType(EPlanetType PlanetType);

	UFUNCTION(BlueprintPure, Category = "APS|Planet Surface")
	static int64 GetPresetModifierMask(EPlanetType PlanetType);

	UFUNCTION(BlueprintPure, Category = "APS|Planet Surface")
	static bool SupportsWorldScape(EPlanetType PlanetType);

	UFUNCTION(BlueprintPure, Category = "APS|Planet Surface")
	static FAPSResolvedPlanetSurfaceProfile ResolveForBody(
		const APlanetaryBody* Body, const UAPSPlanetSurfaceCatalog* Catalog = nullptr);

	static FAPSPlanetSurfaceArchetypeDefinition GetNativeDefinition(EAPSPlanetSurfaceArchetype Archetype);
	/**
	 * Stable UV/warp scale for the inherited WorldScape terrain material graph.
	 *
	 * This is deliberately keyed only by the material-template family. Geometry
	 * feature scale belongs to the noise resolver and must not retile the ground
	 * material when the PLANET UI changes SurfaceFeatureScale.
	 */
	static float ResolveMaterialWarpScale(EAPSPlanetSurfaceMaterialFamily MaterialFamily);
	/**
	 * Subordinate climate tint for the canonical WorldScape terrain graph.
	 *
	 * Height and the authored family palette remain the primary orbital signal;
	 * temperature/humidity only add enough variation to keep biomes legible.
	 */
	static float ResolveMaterialClimateBlend(EAPSPlanetSurfaceArchetype Archetype);
	static void ApplyMaterialParameters(UMaterialInstanceDynamic* Material, const FAPSResolvedPlanetSurfaceProfile& Profile);
	static uint32 BuildProfileSignature(const FAPSResolvedPlanetSurfaceProfile& Profile);
};

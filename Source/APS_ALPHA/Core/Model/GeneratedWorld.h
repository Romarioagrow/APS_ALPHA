#pragma once

#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/APSCanonicalStellarDataset.h"
#include "APS_ALPHA/Core/Saves/GeneratedWorldData.h"
#include "GeneratedWorld.generated.h"

struct FPlanetData;
class APlanetarySystem;
class APlanet;

enum class EPlanetType : uint8;
enum class EHomeSystemPosition : uint8;
enum class EStarClusterComposition : uint8;
enum class EStarClusterPopulation : uint8;
enum class EStarClusterType : uint8;
enum class EStarClusterSize : uint8;
enum class EGalaxyClass : uint8;
enum class EGalaxyType : uint8;
enum class EOrbitDistributionType : uint8;
enum class EPlanetarySystemType : uint8;
enum class ESpectralClass : uint8;
enum class EStellarType : uint8;
enum class EStarType : uint8;
enum class EAstroGenerationLevel : uint8;

/**
 * User-authored body values retained while the disposable main-menu hierarchy is
 * rebuilt.  The map key is a deterministic star/planet/moon index path; actor
 * pointers and generated display names are deliberately excluded.
 */
USTRUCT()
struct FAPSPreviewBodyEditOverride
{
	GENERATED_BODY()

	UPROPERTY()
	EPlanetType PlanetType{EPlanetType::Frozen};

	UPROPERTY()
	double RadiusKm{6750.0};

	UPROPERTY()
	int32 SurfaceSeed{1337};

	UPROPERTY()
	double SurfaceFeatureScale{1.0};

	UPROPERTY()
	double SurfaceReliefScale{1.0};

	UPROPERTY()
	double SurfaceLandCoverageScale{1.0};

	UPROPERTY()
	double SurfaceMountainScale{1.0};

	UPROPERTY()
	double SurfaceCraterScale{1.0};

	UPROPERTY()
	double SurfaceRoughnessScale{1.0};

	UPROPERTY()
	double AtmosphereHeight{100.0};

	UPROPERTY()
	double AtmosphereOpacity{12.0};

	UPROPERTY()
	double AtmosphereMultiScattering{1.0};

	UPROPERTY()
	double AtmosphereRayleighScattering{8.0};

	UPROPERTY()
	FLinearColor AtmosphereColor{FLinearColor(3.8f, 13.5f, 33.0f, 0.0f)};
};

UCLASS()
class UGeneratedWorld : public UObject
{
	GENERATED_BODY()
	
public:
	UGeneratedWorld();
	
	void PrintAllValues() const;

	/** Store a body edit under its stable hierarchy path. */
	void SetPreviewBodyEditOverride(
		const FString& StableBodyKey, const FAPSPreviewBodyEditOverride& BodyOverride);
	const FAPSPreviewBodyEditOverride* FindPreviewBodyEditOverride(const FString& StableBodyKey) const;
	void ClearPreviewBodyEditOverrides();
	int32 GetPreviewBodyEditOverrideCount() const { return PreviewBodyEditOverrides.Num(); }

	UPROPERTY()
	TArray<FPlanetData> InhabitedPlanets;

	// Метод для получения данных о заселенных планетах
	const TArray<FPlanetData>& GetInhabitedPlanets() const { return InhabitedPlanets; }

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateFullScaledWorld{ true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateHomeSystem{ true };

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{ false };

	/** Deterministic seed shared by menu preview and the committed gameplay hierarchy. */
	UPROPERTY(EditAnywhere, Category = "Generation Params")
	int32 GenerationSeed{271828};

	/** Finalized stellar truth duplicated unchanged into the gameplay GameInstance. */
	UPROPERTY()
	FAPSCanonicalStellarDataset CanonicalStellarDataset;

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{ false };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType{ false };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar{ false };

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{ false };


	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{ EAstroGenerationLevel::StarCluster };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarType StarType{ EStarType::SingleStar };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType StellarType{ EStellarType::MainSequence };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass SpectralClass{ ESpectralClass::G };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType PlanetarySystemType{ EPlanetarySystemType::MultiPlanetSystem };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType OrbitDistributionType{ EOrbitDistributionType::Uniform };

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;
	
	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetType PlanetType{ EPlanetType::Frozen };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{ 250 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{ 100000000 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int MoonsAmount{ 0 };

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int StartPlanetIndex{ 0 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{ 10.0 };

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double PlanetRadius{ 6750.0 };

	/** Stable input for the per-planet resolver. Zero lets the body derive a seed. */
	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0"))
	int32 PlanetSurfaceSeed{ 1337 };

	/** Safe multipliers applied after the selected EPlanetType profile is resolved. */
	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.25", ClampMax = "4.0"))
	double SurfaceFeatureScale{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.25", ClampMax = "2.5"))
	double SurfaceReliefScale{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.25", ClampMax = "2.0"))
	double SurfaceLandCoverageScale{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	double SurfaceMountainScale{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	double SurfaceCraterScale{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "0.25", ClampMax = "2.0"))
	double SurfaceRoughnessScale{ 1.0 };
		
	
	UPROPERTY()
	APlanetarySystem* HomePlanetarySystem;
	
	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;

	
	// Свойства атмосферы
	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereHeight{ 100.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereOpacity{ 12.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereMultiScattering{ 1.0 };

	UPROPERTY(EditAnywhere, Category = "Atmosphere")
	double AtmosphereRayleighScattering{ 8.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FLinearColor AtmosphereColor {FLinearColor(3.8f, 13.5f, 33.0f, 0.0f)};

	/**
	 * Per-body editor state. It intentionally belongs to the transient generation
	 * model rather than preview actors, which are destroyed on every structural
	 * rebuild. Explicit REGENERATE clears the map before advancing the seed.
	 */
	UPROPERTY()
	TMap<FString, FAPSPreviewBodyEditOverride> PreviewBodyEditOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	int StarsAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName HomeStarName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName HomePlanetName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	FName FullSpectralName;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	int HomeStarTemperature;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	double HomeStarRadius;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star Cluster")
	double HomeStarMass;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Star System")
	double StarSystemRadius;

	FGeneratedWorldData SaveWorldData() const
	{
		FGeneratedWorldData WorldData;
		WorldData.bGenerateFullScaledWorld = bGenerateFullScaledWorld;
		WorldData.bGenerateHomeSystem = bGenerateHomeSystem;
		WorldData.bStartWithHomePlanet = bStartWithHomePlanet;
		WorldData.bRandomHomeSystem = bRandomHomeSystem;
		WorldData.bRandomHomeSystemType = bRandomHomeSystemType;
		WorldData.bRandomHomeStar = bRandomHomeStar;
		WorldData.bRandomStartPlanetNumber = bRandomStartPlanetNumber;
		WorldData.AstroGenerationLevel = AstroGenerationLevel;
		WorldData.GalaxyType = GalaxyType;
		WorldData.GalaxyClass = GalaxyClass;
		WorldData.StarClusterSize = StarClusterSize;
		WorldData.StarClusterType = StarClusterType;
		WorldData.StarClusterPopulation = StarClusterPopulation;
		WorldData.StarClusterComposition = StarClusterComposition;
		WorldData.StarType = StarType;
		WorldData.StellarType = StellarType;
		WorldData.SpectralClass = SpectralClass;
		WorldData.PlanetarySystemType = PlanetarySystemType;
		WorldData.OrbitDistributionType = OrbitDistributionType;
		WorldData.HomeSystemPosition = HomeSystemPosition;
		WorldData.PlanetType = PlanetType;
		WorldData.GalaxySize = GalaxySize;
		WorldData.GalaxyStarCount = GalaxyStarCount;
		WorldData.PlanetsAmount = PlanetsAmount;
		WorldData.MoonsAmount = MoonsAmount;
		WorldData.StartPlanetIndex = StartPlanetIndex;
		WorldData.GalaxyStarDensity = GalaxyStarDensity;
		WorldData.PlanetRadius = PlanetRadius;
		WorldData.PlanetSurfaceSeed = PlanetSurfaceSeed;
		WorldData.SurfaceFeatureScale = SurfaceFeatureScale;
		WorldData.SurfaceReliefScale = SurfaceReliefScale;
		WorldData.SurfaceLandCoverageScale = SurfaceLandCoverageScale;
		WorldData.SurfaceMountainScale = SurfaceMountainScale;
		WorldData.SurfaceCraterScale = SurfaceCraterScale;
		WorldData.SurfaceRoughnessScale = SurfaceRoughnessScale;
		WorldData.AtmosphereHeight = AtmosphereHeight;
		WorldData.AtmosphereOpacity = AtmosphereOpacity;
		WorldData.AtmosphereMultiScattering = AtmosphereMultiScattering;
		WorldData.AtmosphereRayleighScattering = AtmosphereRayleighScattering;
		WorldData.AtmosphereColor = AtmosphereColor;
		WorldData.StarsAmount = StarsAmount;
		WorldData.HomeStarName = HomeStarName;
		WorldData.HomePlanetName = HomePlanetName;
		WorldData.FullSpectralName = FullSpectralName;
		WorldData.HomeStarTemperature = HomeStarTemperature;
		WorldData.HomeStarRadius = HomeStarRadius;
		WorldData.HomeStarMass = HomeStarMass;
		WorldData.StarSystemRadius = StarSystemRadius;

		return WorldData;
	}
};

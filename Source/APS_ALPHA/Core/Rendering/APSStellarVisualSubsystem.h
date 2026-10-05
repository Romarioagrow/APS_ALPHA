#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "APSPreviewVisibility.h"
#include "APSGameplayNativeStars.h"
#include "APSStellarVisualSubsystem.generated.h"

class ADirectionalLight;
class APlanetaryBody;
class APointLight;
class APawn;
class ASpaceStation;
class AAstroGenerator;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

struct FAPSGameplayStellarPoint
{
	FVector CenterFromHomeCm{FVector::ZeroVector};
	double RadiusCm{0.0};
	int32 InstanceIndex{INDEX_NONE};
	FTransform ProjectedTransform{FTransform::Identity};
	bool bOccluded{true};
};

/** Per-point glyph sizing of one consumed catalogue source (UAPSStellarVisualSubsystem::UpdateGameplayStellarView). */
struct FAPSGameplayStellarResizePass
{
	/** Observer (from home) at the source's last pass, and how far it may travel from there before a point of the
	 * source leaves its size budget (the smallest PointSizeSlackCm found then). */
	FVector Observer{FVector::ZeroVector};
	double SlackCm{0.0};
	/** Nearest sized point of the source at that pass; less the travel since, a bound on its nearest point now. */
	double NearestCm{TNumericLimits<double>::Max()};
	/** FPlatformTime::Seconds of the pass. */
	double Seconds{-1.0e9};
};

/** Disposable optical view; the source catalog and its identity mapping stay immutable. */
struct FAPSGameplayStellarLayer
{
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Source;
	TWeakObjectPtr<UInstancedStaticMeshComponent> View;
	TArray<FAPSGameplayStellarPoint> Points;
	TArray<FTransform> Transforms;
	bool bSourceVisible{true};
	bool bSourceHidden{false};
};

/** Runtime-only bridge from the current generated star to playable global lighting. */
UCLASS()
class APS_ALPHA_API UAPSStellarVisualSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;
	/** Snapshot used by runtime parity tests and diagnostics; never mutates selection. */
	bool GetActiveStellarTarget(FVector& OutTargetLocation, FString& OutTargetIdentity) const;

private:
	void UpdateGameplayStellarView();
	void ResetGameplayStellarView();
	void ResetGameplayNativeStars();
	void BeginGameplayNativeStars(AAstroGenerator* Generator, bool bRefreshDemand,
		const FVector& Camera, double PixelTangent, const FQuat& ViewRotation,
		double TanHalfHorizontal, double TanHalfVertical);
	bool ObserveGameplayNativePoint(AAstroGenerator* Generator,
		const FAPSGameplayStellarKey& Key, const FTransform& BaseTransform,
		const FTransform& CurrentTransform, bool& bOwned);
	void CollectGameplayNativeDemand(const FAPSGameplayStellarKey& Key,
		const FTransform& BaseTransform, double PhysicalRadiusCm, double PixelRadius);
	void PresentGameplayNativeStars(AAstroGenerator* Generator);
	TArray<FAPSGameplayNativePair> GameplayNativePairs;
	TMap<FAPSGameplayStellarKey, int32> GameplayNativeOwners;
	TArray<FAPSGameplayNativeDemand> GameplayNativeDemand;
	// One scalar per rendered slot; procedural records are resolved once per build.
	TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, TArray<double>> GameplayNativePhysicalRadii;
	// Consumed catalogues: observer distance each rendered slot was last sized at. Travel refreshes only the slots
	// whose own size error reaches the pixel budget, not the whole catalogue.
	TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, TArray<double>> GameplayNativeSizedDistances;
	/** Observer and nearest catalogue distance at the last full demand walk and at the last candidate re-measure. */
	FVector GameplayNativeDemandObserver{FVector::ZeroVector};
	double GameplayNativeDemandClosestCm{0.0};
	FVector GameplayNativeDemandRefreshObserver{FVector::ZeroVector};
	double GameplayNativeDemandRefreshClosestCm{0.0};
	/** Per-point sizing of each consumed source: its own slack, passes PointResizeIntervalSeconds apart. */
	TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, FAPSGameplayStellarResizePass>
		GameplayNativeResizePasses;
	/** Points bright enough (with a 2x margin) to be native-star demand; a turn of the camera re-selects only these. */
	TMap<TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent>, TArray<int32>> GameplayNativeDemandCandidates;
	bool bGameplayNativeDemandCandidatesValid{false};
	FVector GameplayNativeCamera{FVector::ZeroVector};
	double GameplayNativePixelTangent{0.0};
	FQuat GameplayNativeViewRotation{FQuat::Identity};
	FQuat GameplayNativeDemandRotation{FQuat::Identity};
	double GameplayNativeTanHalfHorizontal{0.0};
	double GameplayNativeTanHalfVertical{0.0};
	int32 GameplayNativeLastPresentedCount{INDEX_NONE};
	int32 GameplayNativeLastDemandCount{INDEX_NONE};
	uint64 GameplayNativeMutationSerial{0};
	/** Rio 04.10: the generator's batch mutation serial the catalogue was last sized for, and single points still to
	 * re-size (a materialized or released system's proxy), kept until their source's tree build has landed. */
	uint64 GameplayNativeBatchSerial{0};
	TArray<FAPSGameplayStellarKey> GameplayPendingPointRefresh;
	uint64 GameplayNativeUnknownMutationSerial{0};
	uint32 GameplayNativeTopologyHash{0};
	uint64 GameplayNativeBindFrame{MAX_uint64};
	int32 GameplayNativeBindAttempts{0};
	int32 GameplayNativeOverflowResolved{0};
	bool bGameplayNativeInitialized{false};
	TWeakObjectPtr<AAstroGenerator> GameplayStellarGenerator;
	TArray<FAPSGameplayStellarLayer> GameplayStellarLayers;
	FVector LastStellarObserverFromHome{FVector::ZeroVector};
	double ClosestStellarPointCm{0.0};
	double ClosestStellarRenderDistanceCm{0.0};
	FVector LastStellarMaskObserver{FVector::ZeroVector};
	TArray<FAPSPreviewOccluder> LastStellarOccluders;
	double LastStellarPixelTangent{-1.0};
	uint64 GameplayStellarBuildSerial{0u};
	void ResolveDirectionalLight();
	void ResolveNearestStar(const FVector& ObserverLocation);
	/**
	 * Daylight deep in an atmosphere hides the catalogue and native stars; the own star is an actor and stays (Rio,
	 * 29.09/30.09: a day sky shows one sun). Evaluated with the key-star search; hysteresis keeps dusk steady.
	 */
	void UpdateGameplayDaylightStars(const FVector& CameraLocation);
	bool bGameplayDaylightStarsHidden{false};
	/** World time of the last hide/show of the resolved stars (they change at most every three seconds). */
	double GameplayDaylightHideChangeSeconds{-1.0e9};
	float GameplayDaylightFactor{0.0f};
	void UpdatePreviewFillLight(
		const APlanetaryBody* PreviewBody,
		const FVector& PreviewCameraLocation,
		bool bHasPreviewCameraLocation,
		float DeltaTime);
	void UpdateGameplayStationFillLight(
		const APawn* CharacterPawn,
		const FVector& CameraLocation,
		bool bHasCameraLocation);
	void UpdateGameplaySurfaceFillLight(
		const APawn* CharacterPawn,
		const FVector& ObserverLocation,
		bool bHasObserverLocation,
		float DeltaTime);

	TWeakObjectPtr<ADirectionalLight> DirectionalLight;
	/** One bootstrap per committed generated hierarchy; never reset for a star switch. */
	TWeakObjectPtr<AAstroGenerator> InitializedGameplayLightGenerator;
	/**
	 * Menu-only camera fill. It keeps the selected WorldScape material readable on
	 * the physical night side without moving or replacing the parent-star light.
	 */
	TWeakObjectPtr<ADirectionalLight> PreviewFillLight;
	/** Camera-local readability fill used only while a character is inside a station. */
	TWeakObjectPtr<APointLight> GameplayStationFillLight;
	TWeakObjectPtr<ASpaceStation> GameplayFillStation;
	/**
	 * Low-energy off-axis daylight fill used only while the gameplay pawn is close
	 * to an active physical WorldScape surface. It does not own geometry, materials
	 * or displacement; it only keeps the real mesh normals readable with the
	 * project's fixed exposure when the generated star is at a grazing angle.
	 */
	TWeakObjectPtr<ADirectionalLight> GameplaySurfaceFillLight;
	TWeakObjectPtr<APlanetaryBody> GameplayFillBody;
	FVector TargetStarLocation{FVector::ZeroVector};
	/** Same selected target in the persistent frame; world shifts must not rotate its lighting ray. */
	FVector TargetStarGenerationLocation{FVector::ZeroVector};
	FLinearColor TargetLightColor{FLinearColor::White};
	FLinearColor SmoothedLightColor{FLinearColor::White};
	/** Surface temperature of the target star (K) mapped for the key light; 6500 K is neutral. */
	float TargetLightTemperature{6500.0f};
	float SmoothedLightTemperature{6500.0f};
	bool bCapturedOriginalTemperature{false};
	bool bOriginalUseTemperature{false};
	float OriginalTemperature{6500.0f};
	float TargetLightIntensity{10.0f};
	float SmoothedLightIntensity{10.0f};
	float SearchElapsed{0.0f};
	bool bHasTargetStar{false};
	bool bCapturedOriginalLight{false};
	/** True only for the transient gameplay key created when the level has no authored sun. */
	bool bOwnsDirectionalLight{false};
	FRotator OriginalLightRotation{FRotator::ZeroRotator};
	FLinearColor OriginalLightColor{FLinearColor::White};
	float OriginalLightIntensity{10.0f};
	int32 OriginalForwardShadingPriority{0};
	TEnumAsByte<EComponentMobility::Type> OriginalMobility{EComponentMobility::Movable};
	FString ActiveStarIdentity;
};

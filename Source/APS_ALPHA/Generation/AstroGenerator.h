#pragma once

#include "StarSystemGenerator.h"
#include "StarClusterGenerator.h"
#include "GalaxyGenerator.h"
#include "PlanetaryProceduralGenerator.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "CoreMinimal.h"
#include "APS_ALPHA/Actors/BaseActor.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Rendering/APSCanonicalStellarProjection.h"
#include "APS_ALPHA/Core/Rendering/APSContinuousPreviewFrame.h"
#include "GameFramework/Actor.h"
#include "AstroGenerator.generated.h"

class USpawnParameters;
class UStarGenerator;
class UCameraComponent;
class USceneComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class APlayerController;
class APawn;
class AAstroAnchor;
class AStarCluster;
class AMoon;
class APlanetOrbit;
class UGeneratedWorld;
class APlanet;
class APlanetaryBody;
class APlanetarySurfaceGenerator;
class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class ASpaceShipyard;
class ASpaceship;
class ASpaceStation;
class ASpaceHeadquarters;
enum class ECharSpawnPlace : uint8;
enum class EHomeSystemPosition : uint8;
enum class EOrbitHeight : uint8;
struct FPlanetModel;
struct FPlanetData;
struct FClusterStarSystemRecord;
struct FAPSCanonicalStellarDataset;
struct FAPSCanonicalClusterSystemRecord;
struct FStarModel;
struct FStarSystemModel;

/** Existing physical generation recipe, resolved without spawning its actor tree. */
struct FAPSContinuousPreviewStarLayout
{
	TSharedPtr<FStarModel> StarModel;
	TSharedPtr<FPlanetarySystemModel> FamilyModel;
	FVector OffsetCm{FVector::ZeroVector};
	double EnvelopeCm{0.0};
	TArray<double> PlanetOrbitRadiiCm;
	TArray<double> MoonEnvelopesCm;
};

/** View-only copy of a canonical catalog address, never fed back into generation. */
struct FAPSContinuousPreviewPoint
{
	FVector CenterCm{FVector::ZeroVector};
	double RadiusCm{0.0};
	int32 SourceInstanceIndex{INDEX_NONE};
	FGuid StableId;
	/** Stable star slot within StableId; retained after its heavy actor is retired. */
	int32 SystemStarIndex{INDEX_NONE};
	TWeakObjectPtr<AStar> MaterializedStar;
	/** Retained appearance recipe, independent of the disposable physical hierarchy. */
	TSharedPtr<FStarModel> StarModel;
};

struct FAPSContinuousResolvedStarView
{
	TWeakObjectPtr<UStaticMeshComponent> Photosphere;
	TWeakObjectPtr<UStaticMeshComponent> Corona;
};

struct APS_ALPHA_API FAPSPreviewBodyEntry
{
	TWeakObjectPtr<AActor> Actor;
	/**
	 * Actor-free catalogue rows (and the materialized home row while CLUSTER is
	 * visible) can publish the exact HISM address used by their rendered glyph.
	 * Labels must never drift to an actor pivot that is hidden at this scope.
	 */
	FVector ExplicitWorldAnchor{FVector::ZeroVector};
	bool bHasExplicitWorldAnchor{false};
	FText Label;
	FText Details;
	int32 Depth{0};
	/** Actor-free HISM system used by the cluster browser. */
	int32 ClusterSystemInstanceIndex{INDEX_NONE};
	/** Optional hierarchy focus for actor roots such as Galaxy or Cluster. */
	int32 PreviewFocusValue{INDEX_NONE};
};

/**
 * One retained, double-buffered orbital globe owned by a stable hierarchy body.
 * These components are menu-only presentation data: gameplay WorldScape actors and
 * resolved materials remain owned by APlanetarySurfaceGenerator.
 */
struct FAPSPreviewGlobeProxyState
{
	TWeakObjectPtr<APlanetaryBody> Body;
	TWeakObjectPtr<UProceduralMeshComponent> TerrainA;
	TWeakObjectPtr<UProceduralMeshComponent> TerrainB;
	TWeakObjectPtr<UProceduralMeshComponent> OceanA;
	TWeakObjectPtr<UProceduralMeshComponent> OceanB;
	TWeakObjectPtr<UMaterialInstanceDynamic> TerrainMaterialA;
	TWeakObjectPtr<UMaterialInstanceDynamic> TerrainMaterialB;
	TWeakObjectPtr<UMaterialInstanceDynamic> OceanMaterialA;
	TWeakObjectPtr<UMaterialInstanceDynamic> OceanMaterialB;
	int32 ActiveBuffer{INDEX_NONE};
	uint32 ProfileSignature{0};
	/** Profile + topology quality. Unlike ProfileSignature this changes across LOD tiers. */
	uint32 MeshSignature{0};
	int32 FaceResolution{0};
	int32 VertexCount{0};
	int32 IndexCount{0};
	bool bHasOcean{false};
	bool bUsesDefaultBuffers{false};
};

UENUM(BlueprintType)
enum class EAstroPreviewFocus : uint8
{
	Overview,
	StarCluster,
	Galaxy,
	HomeSystem,
	HomeStar,
	HomePlanet
};

UCLASS()
class APS_ALPHA_API AAstroGenerator : public ABaseActor
{
	GENERATED_BODY()

public:
	AAstroGenerator();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Transactionally spawns and validates the selected starter hierarchy. */
	bool SpawnStartInteractiveActors(TSharedPtr<FPlanetModel> StartPlanetModel);

	UFUNCTION(BlueprintPure, Category = "World Generation|Civilization")
	int32 GetGeneratedStartingFleetSize() const { return GeneratedStartingFleet.Num(); }

	UFUNCTION(BlueprintPure, Category = "World Generation|Civilization")
	int32 GetGeneratedInfrastructureCount() const { return GeneratedCivilizationInfrastructure.Num(); }

	void ComputeStarAmount(TSharedPtr<FStarSystemModel>& StarSystemModel, int& AmountOfStars);

	TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel;

	void SpawnPlanetMoons(const TSharedPtr<FPlanetModel>& PlanetModel);

	bool ResolveSpawnLocation(const ASpaceship* NewHomeSpaceship, FVector& CharSpawnLocation);

	void SetGeneratedWorld(UGeneratedWorld* InGeneratedWorld);

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bAutoGeneration{false};

	UFUNCTION()
	void DisplayNewGeneratedWorld();

	UFUNCTION()
	void ApplyWorldModel();

	UFUNCTION()
	void GenerateWorldByModel();

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	bool RegeneratePreview(UGeneratedWorld* InGeneratedWorld, EAstroPreviewFocus RequestedFocus);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void FocusPreviewCamera(APlayerController* PlayerController = nullptr);
	/** Read-only access for rendered preview diagnostics and automation contracts. */
	const UCameraComponent* GetPreviewCameraComponent() const { return PreviewCamera; }
	bool UsesContinuousPreviewFrame() const;
	const FAPSContinuousPreviewFrame& GetContinuousPreviewFrame() const { return ContinuousPreviewFrame; }
	const FAPSContinuousPreviewOrbit& GetContinuousPreviewOrbit() const { return ContinuousPreviewOrbit; }
	/** Angular space available around the optical axis inside the actual menu panel. */
	void SetContinuousPreviewFramingTangent(double Tangent);
	bool GetContinuousPreviewClusterLocation(int32 InstanceIndex, FVector& OutLocation) const;
	bool ProjectContinuousPreviewWorldPosition(const FVector& PhysicalWorldPosition, FVector& OutLocation,
		const AActor* CoordinateOwner = nullptr) const;
	/** Resolve a local physical hierarchy into the same canonical observer frame as the catalogs. */
	FVector GetContinuousPreviewPhysicalPosition(const AActor* Actor) const;
	AStarSystem* GetContinuousPreviewActiveSystem() const;
	/** Home is always resident. In-flight/resolved bodies may temporarily exceed this target. */
	static constexpr int32 ContinuousPreviewSystemCacheLimit = 4;
	int32 GetContinuousPreviewResidentSystemCount() const { return ContinuousMaterializedSystems.Num(); }
	const TArray<FAPSContinuousPreviewPoint>& GetContinuousPreviewClusterPoints() const { return ContinuousClusterPoints; }
	UStaticMeshComponent* GetContinuousPreviewResolvedStarMesh(int32 PointIndex) const;
	int32 GetContinuousPreviewResolvedStarCount() const { return ContinuousResolvedStarViews.Num(); }
	/** Inactive render-only pairs; they own no catalog address, light or physical hierarchy. */
	static constexpr int32 ContinuousResolvedStarPoolLimit = 1024;
	static constexpr int32 ContinuousResolvedStarPreparePairLimit = 16;
	int32 GetContinuousPreviewResolvedStarPoolCount() const { return ContinuousResolvedStarPool.Num(); }
	int32 GetContinuousPreviewResolvedStarAllocationCount() const { return ContinuousResolvedStarAllocations; }
	int32 GetContinuousPreviewResolvedStarReuseCount() const { return ContinuousResolvedStarReuses; }
	int32 GetContinuousPreviewResolvedStarPreparationCount() const { return ContinuousResolvedStarPreparations; }
	bool IsContinuousResolvedStarPoolHidden() const;
	/** Current star (including a selected body's owner), addressed independently of its disposable actor. */
	bool GetPreviewStarEditContext(FString& OutAddress, FStarModel& OutModel) const;
	bool GetPreviewSystemEditContext(FString& OutAddress, FStarSystemModel& OutModel) const;
	int32 GetPreviewHomePlanetCount() const;
	AStarSystem* GetPreviewHomeSystem() const { return GeneratedHomeStarSystem; }

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void FocusPreviewTarget(EAstroPreviewFocus NewFocus, APlayerController* PlayerController = nullptr);

	/** Returns the materialized star-system hierarchy used by the live Slate browser. */
	void GetPreviewBodyEntries(TArray<FAPSPreviewBodyEntry>& OutEntries) const;

	/** Focuses the exact selected planet or moon instead of the generic home-body slot. */
	bool FocusPreviewBodyActor(AActor* BodyActor, APlayerController* PlayerController = nullptr);

	/**
	 * Picks a visible actor-free cluster system in screen space. Cluster stars deliberately
	 * have no collision, so a physics trace can never select them without this path.
	 */
	bool FocusPreviewClusterSystemAtScreenPosition(
		APlayerController* PlayerController, const FVector2D& ScreenPosition, float MaxPixelDistance = 28.0f);
	bool FocusPreviewClusterSystem(int32 InstanceIndex, APlayerController* PlayerController = nullptr);
	int32 GetPreviewGalaxyRenderedStarCount() const;
	int64 GetPreviewGalaxyModeledStarCount() const;
	int32 GetPreviewClusterRenderedStarCount() const;
	int32 GetPreviewClusterModeledSystemCount() const;
	/** Immutable canonical-to-proxy descriptor shared by menu, gameplay and diagnostics. */
	const FAPSCanonicalStellarProjectionDescriptor& GetCanonicalStellarProjectionDescriptor() const
	{
		return CanonicalStellarProjection;
	}
	/** Resolves one exact rendered instance back to its canonical record and base projection. */
	bool GetCanonicalStellarProxyRecord(
		EAPSCanonicalStellarProxyLayer Layer, int32 InstanceIndex,
		FAPSCanonicalStellarProxyRecord& OutRecord) const;
	/** StableId lookup is authoritative across different menu/gameplay render budgets. */
	bool GetCanonicalStellarProxyRecord(
		EAPSCanonicalStellarProxyLayer Layer, const FGuid& StableId,
		FAPSCanonicalStellarProxyRecord& OutRecord) const;
	/** Authoritative physical address/distance and immutable proxy location for one cluster system. */
	bool ResolveCanonicalClusterSystemAddress(
		int32 InstanceIndex, FAPSCanonicalClusterSystemAddress& OutAddress) const;
	bool ResolveCanonicalClusterSystemAddress(
		const FGuid& StableId, FAPSCanonicalClusterSystemAddress& OutAddress) const;
	/** Full finalized dataset lookup; independent of the current HISM LOD subset. */
	bool GetCanonicalClusterDatasetRecord(
		int32 CanonicalIndex, FAPSCanonicalClusterSystemRecord& OutRecord) const;
	bool GetCanonicalClusterDatasetRecord(
		const FGuid& StableId, FAPSCanonicalClusterSystemRecord& OutRecord) const;
	bool GetSelectedPreviewClusterSystemSummary(
		FString& OutStableId, int32& OutStarCount, int32& OutPotentialPlanetCount) const;
	/** True only when the requested hierarchy level exists in the current live preview. */
	bool IsPreviewFocusAvailable(EAstroPreviewFocus Focus) const;
	EAstroPreviewFocus GetCurrentPreviewFocus() const { return PreviewFocus; }
	bool HasSelectedPreviewClusterSystem() const
	{
		return SelectedPreviewClusterSystemIndex != INDEX_NONE;
	}
	/** Newly materialized counterpart of the selected body after a structural preview rebuild. */
	AActor* GetSelectedPreviewBodyActor() const { return SelectedPreviewBodyActor.Get(); }
	/** World-space sphere used by Slate to draw a stable scope boundary. */
	bool GetPreviewFocusSphere(EAstroPreviewFocus Focus, FVector& OutCenter, double& OutRadius) const;
	/**
	 * Rendered 3D guide shell diagnostics. HomeStar addresses the amber stellar
	 * influence shell; HomeSystem addresses the coral system boundary shell.
	 */
	bool GetPreviewGuideShellState(EAstroPreviewFocus GuideFocus, FVector& OutCenter,
		double& OutRadius, bool& bOutVisible) const;
	/**
	 * Preview-only rendered centre for a body. PLANET can spread a physically tiny
	 * satellite family for readability without mutating generated actor/model transforms.
	 */
	bool GetPreviewPresentationLocation(const AActor* Actor, FVector& OutLocation) const;
	/** Matching mesh-only radius used by PLANET presentation and atmosphere framing. */
	bool GetPreviewPresentationRadius(const AActor* Actor, double& OutRadius) const;
	/** Selected real body whose closed globe is presented; WorldScape remains a hidden resolver. */
	APlanetaryBody* GetActivePreviewWorldScapeBody() const { return ActivePreviewWorldScapeBody.Get(); }
	/** Number of resolved profiles committed by the persistent menu surface generator. */
	int32 GetPreviewSurfaceProfileApplyCount() const { return PreviewSurfaceProfileApplyCount; }
	/** Closed, camera-independent globe used only by the PLANET orbital preview. */
	UProceduralMeshComponent* GetActivePreviewTerrainProxy() const;
	UProceduralMeshComponent* GetActivePreviewOceanProxy() const;
	/** Retained proxy for a concrete body, whether or not it is the current editor target. */
	UProceduralMeshComponent* GetPreviewTerrainProxyForBody(const APlanetaryBody* Body) const;
	UProceduralMeshComponent* GetPreviewOceanProxyForBody(const APlanetaryBody* Body) const;
	int32 GetRetainedPreviewGlobeCount() const;
	uint32 GetPreviewGlobeProfileSignature() const { return PreviewGlobeProfileSignature; }
	uint32 GetPreviewGlobeMeshSignature() const { return PreviewGlobeMeshSignature; }
	int32 GetPreviewGlobeFaceResolution() const { return PreviewGlobeFaceResolution; }
	int32 GetPreviewGlobeFaceResolutionForBody(const APlanetaryBody* Body) const;
	int32 GetPreviewGlobeVertexCount() const { return PreviewGlobeVertexCount; }
	int32 GetPreviewGlobeIndexCount() const { return PreviewGlobeIndexCount; }
	/** Loads and retains every material used by the closed orbital globes. */
	bool WarmPreviewMaterialAssets();
	bool ArePreviewMaterialAssetsWarmed() const;
	/** True after the selected planet and every supported moon have left the one-body-per-tick warm queue. */
	bool IsPreviewGlobeFamilyWarmQueueDrained() const;
	/** Applies surface/atmosphere controls to the selected preview planet without rebuilding its galaxy hierarchy. */
	bool RefreshPreviewPlanetAppearance(UGeneratedWorld* InGeneratedWorld, bool bRegenerateSurface);
	/** Deterministic index path used to retain body edits across disposable hierarchy rebuilds. */
	FString GetPreviewBodyStableKey(const APlanetaryBody* Body) const;
	/** Snapshots the current editor buffer for one body immediately, before the debounced surface refresh. */
	bool SavePreviewBodyEditOverride(UGeneratedWorld* InGeneratedWorld, const APlanetaryBody* Body) const;
	bool SaveSelectedPreviewBodyEditOverride(UGeneratedWorld* InGeneratedWorld) const;
	/** Loads a previously retained body snapshot back into the shared UI editor buffer. */
	bool LoadPreviewBodyEditOverride(UGeneratedWorld* InGeneratedWorld, const APlanetaryBody* Body) const;
	/** Reapplies retained snapshots to newly materialized preview actors and their generation models. */
	int32 ApplyPreviewBodyEditOverrides(UGeneratedWorld* InGeneratedWorld);
	/** Narrow deterministic primitive used by the hierarchy pass and automation coverage. */
	static bool ApplyPreviewBodyEditOverrideByKey(
		const UGeneratedWorld* InGeneratedWorld, const FString& StableBodyKey, APlanetaryBody* Body);
	/** Applies retained body values to a generated model before its actors/surfaces spawn. */
	static int32 ApplyPreviewBodyEditOverridesToModels(
		const UGeneratedWorld* InGeneratedWorld, int32 StarIndex,
		FPlanetarySystemModel& PlanetarySystem, const FString& SystemPrefix = TEXT("SYS0"));
	/** Changes the physical body scale proportionally while keeping hierarchy normalization intact. */
	static void ApplyPlanetaryBodyRadius(APlanetaryBody& Body, double RadiusKm);
	/** Stable world-space preview radius derived only from this body's own physical radius. */
	static double CalculatePreviewBodyPresentationRadius(double RadiusKm);
	/**
	 * Monotonic SYSTEM-only readability mapping. The canonical primary and largest
	 * endpoints remain unchanged while compact luminous companions retain a bounded
	 * fraction of the primary's angular footprint. Physical radii are never mutated.
	 */
	static double CalculateSystemStarReadabilityRatio(
		double StarRadiusKm, double PrimaryRadiusKm, double LargestRadiusKm);
	/**
	 * Commits the materialized home system back into its actor-free cluster record
	 * without changing the record identity or canonical catalog anchor. Keeping this
	 * primitive deterministic lets the GALAXY/CLUSTER glyph and SYSTEM hierarchy
	 * share one model even when the home system is randomized.
	 */
	static void SynchronizeHomeClusterRecord(
		FClusterStarSystemRecord& Record, const FStarModel& PrimaryStar,
		const FStarSystemModel& MaterializedSystem);
	/** Hides the preview surface and succeeds only after its async WorldScape workers are drained. */
	bool PreparePreviewForTravel();

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void OrbitPreviewCamera(FVector2D ScreenDelta);
	/** Arms camera orbit around the current immutable focus centre. */
	void BeginPreviewCameraOrbit();
	void EndPreviewCameraOrbit();

	/** Moves only the preview camera; a PLANET zoom never advances the hidden resolver. */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void ZoomPreviewCamera(float WheelDelta);

#if WITH_DEV_AUTOMATION_TESTS
	/**
	 * Atomically replaces a completed PLANET focus with a body-only inspection frame.
	 * This is intentionally unavailable to production UI: rendered gallery tests use
	 * it to exclude moon-orbit bounds without changing the authored PLANET transition.
	 */
	bool ApplyPreviewInspectionFramingForAutomation(
		APlayerController* PlayerController, const FVector& BodyCenter,
		double BodyRadius, const FIntPoint& ViewportSize,
		double TargetRadiusFraction);
#endif

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void AdvancePreviewGenerationSeed();
	UGeneratedWorld* GetGeneratedWorldModel() const { return GeneratedWorldModel; }

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void ClearGeneratedPreview();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	USceneComponent* GenerationRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	UCameraComponent* PreviewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	bool bIsPreviewGeneration{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Preview", meta = (ClampMin = "100", ClampMax = "3000"))
	int32 PreviewMaxInstances{1800};

	/**
	 * The world model may describe millions of stars, but a committed gameplay
	 * world only needs a representative HISM sample. Keeping this independent
	 * from GalaxyStarCount preserves full-scale data without blocking travel or
	 * overflowing renderer instance indices.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Performance", meta = (ClampMin = "1000", ClampMax = "500000"))
	int32 RuntimeMaxGalaxyInstances{25000};

	/** Keeps live menu regeneration spatially stable while individual controls change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Generation|Preview")
	int32 PreviewGenerationSeed{271828};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	UGeneratedWorld* GeneratedWorldModel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation")
	USpawnParameters* SpawnParameters;

	void SetAutoStarSystemModel();

	void GenerateRandomStarSystemModel();

	void GenerateHomeStarSystem();

	void SetMoonRotation(APlanetOrbit* NewMoonOrbit);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Waits for the authoritative WorldScape terrain collision, then grounds the pawn on it. */
	void ScheduleSurfaceSpawnFinalization(APawn* Pawn, APlanetaryBody* Body,
		const FVector& SurfaceOutward, const FVector& ViewDirection);
	void TryFinalizeSurfaceSpawn(TWeakObjectPtr<APawn> WeakPawn,
		TWeakObjectPtr<APlanetaryBody> WeakBody, FVector SurfaceOutward,
		FVector ViewDirection, int32 AttemptIndex, uint64 FinalizationSerial);
	uint64 SurfaceSpawnFinalizationSerial{0};

	/** Surface patch selected by ResolveSpawnLocation and consumed by the gameplay handoff. */
	TWeakObjectPtr<APlanetaryBody> ResolvedSurfaceSpawnBody;
	FVector ResolvedSurfaceSpawnOutward{FVector::ZeroVector};
	FVector ResolvedSurfaceSpawnViewDirection{FVector::ZeroVector};

	FBox GetPreviewFocusBounds(EAstroPreviewFocus Focus) const;
	bool GetPreviewSystemPresentationSphere(FVector& OutCenter, double& OutRadius,
		double* OutPositionScale = nullptr) const;
	bool TryGetPreviewClusterSystemSphere(int32 InstanceIndex, FVector& OutCenter, double& OutRadius) const;
	void StartPreviewCameraTransition(const FVector& Center, double Radius, APlayerController* PlayerController);
	void ApplyPreviewFocusPresentation(EAstroPreviewFocus NewFocus);
	bool GetContinuousPreviewPhysicalFocus(EAstroPreviewFocus Focus, FVector& CenterCm, double& RadiusCm) const;
	void EnsureContinuousPreviewPresentation();
	void ClearContinuousPreviewPresentation();
	void ApplyContinuousPreviewFrame();
	void StartContinuousPreviewTransition(APlayerController* PlayerController, double DistanceRatio = 0.0);
	void RememberContinuousPreviewBody(AActor* Body);
	void FocusContinuousPreviewTarget(EAstroPreviewFocus NewFocus, APlayerController* PlayerController);
	AStarSystem* MaterializeContinuousPreviewSystem(int32 InstanceIndex);
	bool BuildContinuousPreviewSystemLayout(const FClusterStarSystemRecord& Record,
		UStarGenerator* Stars, UPlanetarySystemGenerator* Families,
		TArray<FAPSContinuousPreviewStarLayout>& OutStars, double& OutRadiusCm);
	void TrimContinuousPreviewSystemCache(double PixelTangent);
	void PresentContinuousResolvedStars(double PixelTangent);
	FAPSContinuousResolvedStarView AllocateContinuousResolvedStarPair(UStaticMeshComponent* Template,
		UMaterialInterface* SurfaceBase, UMaterialInterface* CoronaBase);
	void PrepareContinuousResolvedStarPool(double PixelTangent);
	AStarSystem* GetContinuousPreviewOwningSystem(const AActor* Actor) const;
	FVector GetContinuousPreviewSystemCenter(const AStarSystem* System) const;
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AStarSystem>> ContinuousMaterializedSystems;
	TArray<int32> ContinuousSystemRecency;
	TMap<FString, FQuat> ContinuousRetiredBodyRotations;
	FAPSContinuousPreviewFrame ContinuousPreviewFrame;
	FAPSContinuousPreviewOrbit ContinuousPreviewOrbit;
	FAPSContinuousPreviewOrbit ContinuousPreviewStartOrbit;
	FAPSContinuousPreviewOrbit ContinuousPreviewTargetOrbit;
	bool bContinuousPreviewInitialized{false};
	double ContinuousPreviewFramingTangent{0.0};
	bool bContinuousPreviewAutoFraming{true};
	TArray<TWeakObjectPtr<AActor>> ContinuousPreviewBodies;
	TArray<FAPSContinuousPreviewPoint> ContinuousGalaxyPoints;
	TArray<FAPSContinuousPreviewPoint> ContinuousClusterPoints;
	TMap<int32, FAPSContinuousResolvedStarView> ContinuousResolvedStarViews;
	TArray<FAPSContinuousResolvedStarView> ContinuousResolvedStarPool;
	int32 ContinuousResolvedStarAllocations{0};
	int32 ContinuousResolvedStarReuses{0};
	int32 ContinuousResolvedStarPreparations{0};
	int32 ContinuousResolvedForecastStep{0}, ContinuousResolvedForecastCapacity{0};
	uint64 ContinuousResolvedLastPreparationFrame{MAX_uint64};
	TWeakObjectPtr<APlanetaryBody> ContinuousSelectedPlanet;
	TWeakObjectPtr<AStar> ContinuousSelectedStar;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> ContinuousGalaxyView{nullptr};
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> ContinuousClusterView{nullptr};
	FQuat GetPreviewPlanetPresentationRotation(const APlanetaryBody* Body) const;
	FQuat GetPreviewWorldScapePresentationRotation(const APlanetaryBody* Body) const;
	void ReapplyPreviewPlanetPresentationRotation();
	void RotatePreviewPlanetPresentation(const FQuat& DeltaRotation);
	void UpdatePreviewGuideShells(EAstroPreviewFocus NewFocus);
	void SetPreviewGuideShellVisible(UProceduralMeshComponent* Shell, bool bVisible);
	void HideLegacyPreviewGuideShells();
	void ApplyPreviewBackgroundContext(EAstroPreviewFocus NewFocus);
	void ResetPreviewBackgroundContextCache();
	void SetPreviewBodyBackingSphereVisible(APlanetaryBody* Body, bool bVisible);
	void SetPreviewGlobeProxyVisible(bool bVisible);
	UProceduralMeshComponent* CreatePreviewGlobeMeshComponent(FName BaseName);
	FAPSPreviewGlobeProxyState* FindPreviewGlobeProxyState(const APlanetaryBody* Body);
	const FAPSPreviewGlobeProxyState* FindPreviewGlobeProxyState(const APlanetaryBody* Body) const;
	FAPSPreviewGlobeProxyState* FindOrAddPreviewGlobeProxyState(APlanetaryBody* Body);
	void SyncPreviewGlobeProxyTransforms();
	void UpdateActivePreviewGlobeCompatibilityState();
	bool StabilizePreviewAtmosphere(APlanetaryBody* Body);
	void InvalidatePreviewGlobeProxy(APlanetaryBody* Body);
	void ClearPreviewGlobeProxyCache();
	void QueuePreviewGlobeFamily(APlanetaryBody* Body);
	int32 GetDesiredPreviewGlobeFaceResolution(const APlanetaryBody* Body) const;
	bool BeginNextQueuedPreviewGlobeBuild();
	bool BuildPreviewGlobeProxy(APlanetaryBody* Body,
		APlanetarySurfaceGenerator* SurfaceGenerator, AWorldScapeRoot* ProfileRoot);
	void SetPreviewWorldScapeBody(APlanetaryBody* Body);
	void UpdatePreviewWorldScape();

	EAstroPreviewFocus PreviewFocus{EAstroPreviewFocus::HomePlanet};
	int32 SelectedPreviewClusterSystemIndex{INDEX_NONE};
	/** Exact body selected from the hierarchy; root buttons fall back to the authored home body. */
	TWeakObjectPtr<AActor> SelectedPreviewBodyActor;
	/** Mesh-only PLANET presentation centres; generated actor transforms stay authoritative. */
	TMap<TWeakObjectPtr<AActor>, FVector> PreviewBodyPresentationCenters;
	/** Matching rendered radii used to frame the selected planet-and-satellite family. */
	TMap<TWeakObjectPtr<AActor>, double> PreviewBodyPresentationRadii;
	/**
	 * Presentation-only orientation retained per selected solid body. The generated
	 * hierarchy, camera and committed WorldScape tangent root remain immutable;
	 * PLANET RMB rotates only the selected planet/moon context around its focus.
	 */
	TMap<TWeakObjectPtr<APlanetaryBody>, FQuat> PreviewPlanetPresentationRotations;
	FTransform PreviewCameraStartTransform;
	FTransform PreviewCameraTargetTransform;
	FVector PreviewOrbitCenter{FVector::ZeroVector};
	double PreviewOrbitDistance{1000.0};
	double PreviewOrbitYawDegrees{0.0};
	double PreviewOrbitPitchDegrees{0.0};
	float PreviewCameraTransitionElapsed{0.0f};
	float PreviewCameraTransitionDuration{0.55f};
	bool bPreviewCameraTransitionActive{false};
	TWeakObjectPtr<APlanetaryBody> ActivePreviewWorldScapeBody;
	/** Last fully validated pair currently presented in PLANET scope. */
	TWeakObjectPtr<APlanetarySurfaceGenerator> PersistentPreviewSurfaceGenerator;
	TWeakObjectPtr<AWorldScapeRoot> PersistentPreviewWorldScapeRoot;
	/**
	 * Hidden transaction pair used for a profile/type/seed edit. WorldScape 5.4
	 * commits individual LOD workers independently, so mutating the presented root
	 * exposes torn hemispheres. The previous complete pair stays visible until every
	 * terrain/ocean payload in this pair validates; both pointers then swap in one
	 * game-thread tick.
	 */
	TWeakObjectPtr<APlanetarySurfaceGenerator> StagingPreviewSurfaceGenerator;
	TWeakObjectPtr<AWorldScapeRoot> StagingPreviewWorldScapeRoot;
	bool bPreviewSurfaceSwapInFlight{false};
	UPROPERTY(VisibleAnywhere, Transient, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewTerrainProxyA{nullptr};
	UPROPERTY(VisibleAnywhere, Transient, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewTerrainProxyB{nullptr};
	UPROPERTY(VisibleAnywhere, Transient, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewOceanProxyA{nullptr};
	UPROPERTY(VisibleAnywhere, Transient, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewOceanProxyB{nullptr};
	/** Serialized compatibility component. Always hidden; retained so old maps load cleanly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	UStaticMeshComponent* PreviewStarInfluenceShell{nullptr};
	/** Serialized compatibility component. Always hidden; retained so old maps load cleanly. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	UStaticMeshComponent* PreviewSystemBoundaryShell{nullptr};
	/** Amber, line-only, mesh-centred stellar influence guide. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewStarInfluenceWireGuide{nullptr};
	/** Coral, line-only, barycentre-centred outer system boundary guide. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Generation|Preview")
	UProceduralMeshComponent* PreviewSystemBoundaryWireGuide{nullptr};
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewStarInfluenceMaterial{nullptr};
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewSystemBoundaryMaterial{nullptr};
	/** Strong page-lifetime references loaded before PLANET body-switch commits begin. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewTerrainBaseMaterial{nullptr};
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewLiquidBaseMaterial{nullptr};
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewWaterBaseMaterial{nullptr};
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewAmmoniaBaseMaterial{nullptr};
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewLavaBaseMaterial{nullptr};
	bool bPreviewMaterialPSOPrecacheRequested{false};
	/**
	 * Stable reference for the current non-stellar system layout, expressed in
	 * AstroGenerator-root space.  The disposable preview root is uniformly scaled
	 * when a full-scale hierarchy is normalized; keeping this value in world space
	 * made the cached SYSTEM envelope immune to that normalization.
	 */
	mutable uint32 PreviewSystemLayoutSignature{0};
	mutable double PreviewSystemReferenceRadiusInRootSpace{0.0};
	/**
	 * Orbital-preview-only MIDs.  The resolver's WorldScape materials remain on the
	 * hidden WorldScape root; they use masked/tangent-patch assumptions and must not
	 * be assigned to the closed procedural globe.
	 */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewTerrainMaterialA{nullptr};
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewTerrainMaterialB{nullptr};
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewOceanMaterialA{nullptr};
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PreviewOceanMaterialB{nullptr};
	TWeakObjectPtr<APlanetaryBody> PreviewGlobeProxyBody;
	/** Stable-keyed orbital globes survive body selection changes inside one preview hierarchy. */
	TMap<FString, FAPSPreviewGlobeProxyState> PreviewGlobeProxyStates;
	TArray<TWeakObjectPtr<APlanetaryBody>> PendingPreviewGlobeBodies;
	TWeakObjectPtr<APlanetaryBody> PreviewSurfaceBuildBody;
	int32 ActivePreviewGlobeBuffer{INDEX_NONE};
	uint32 PreviewGlobeProfileSignature{0};
	uint32 PreviewGlobeMeshSignature{0};
	int32 PreviewGlobeFaceResolution{0};
	int32 PreviewGlobeVertexCount{0};
	int32 PreviewGlobeIndexCount{0};
	bool bPreviewGlobeHasOcean{false};
	bool bPreviewSurfaceUpdatePending{false};
	bool bPreviewCameraOrbitDragging{false};
	bool bPreviewSurfaceViewDirty{false};
	bool bPreviewSurfaceViewRefreshInFlight{false};
	/** Camera moves retain the last complete mesh; profile changes keep the atomic fallback. */
	bool bPreviewSurfaceLiveRefresh{false};
	/** WorldScape owns exactly one normal initialization tick after a drained profile change. */
	bool bPreviewSurfaceRootInitializationPending{false};
	/** Frame that armed the root; prevents stale LODs from being accepted before its first real tick. */
	uint64 PreviewSurfaceInitArmedFrame{0};
	/** Test/diagnostic counter: a coalesced slider burst must resolve exactly one new profile. */
	int32 PreviewSurfaceProfileApplyCount{0};
	/**
	 * Last atmosphere state committed to the live PLANET presentation.  Camera-only
	 * interaction calls ApplyPreviewFocusPresentation repeatedly; retaining this key
	 * lets StabilizePreviewAtmosphere prove that the already-correct shell can stay
	 * visible instead of briefly restoring its full-scale transform every time.
	 */
	TWeakObjectPtr<APlanetaryBody> StabilizedPreviewAtmosphereBody;
	TWeakObjectPtr<AWorldScapeRoot> StabilizedPreviewAtmosphereRoot;
	uint32 StabilizedPreviewAtmosphereSignature{0};
	FVector PendingPreviewSurfaceViewPosition{FVector::ZeroVector};
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> PreviewGalaxyContextOwner;
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> PreviewClusterContextOwner;
	/** Immutable generated local addresses used by reversible presentation-only field scaling. */
	TArray<FVector> PreviewGalaxyBaseInstanceLocations;
	TArray<FVector> PreviewClusterBaseInstanceLocations;
	TArray<FVector> PreviewGalaxyBaseInstanceScales;
	TArray<FVector> PreviewClusterBaseInstanceScales;
	/** Original HISM emissive custom-data channel restored outside detail scopes. */
	TArray<float> PreviewGalaxyBaseInstanceEmissions;
	TArray<float> PreviewClusterBaseInstanceEmissions;
	bool bPreviewBackgroundCullStateInitialized{false};
	bool bPreviewBackgroundCullApplied{false};
	EAstroPreviewFocus PreviewBackgroundContextFocus{EAstroPreviewFocus::Overview};
	double PreviewBackgroundVisualScale{1.0};
	double PreviewBackgroundDistanceScale{1.0};
	FVector PreviewBackgroundCullCenter{FVector::ZeroVector};
	double PreviewBackgroundCullRadius{0.0};
	FVector PreviewBackgroundDetailCenter{FVector::ZeroVector};
	float PreviewBackgroundEmissionCap{TNumericLimits<float>::Max()};

	void Test_GenerateFullscaled();

	void InitLegacyAuthoredGenerationLevel();

	void InitGenerationLevel();
	bool IsCanonicalStellarProjectionEnabled() const;
	void BeginCanonicalStellarProjectionBuild();
	void FinalizeCanonicalStellarProjectionBuild();
	void NoteCanonicalStellarProxyUpload();
	void NoteCanonicalStellarProxyMutation();
	uint32 BuildCanonicalStellarProjectionContextHash() const;
	uint32 BuildCanonicalStellarDatasetInputHash() const;
	uint32 BuildCanonicalStellarManifestHash(const FAPSCanonicalStellarDataset& Dataset) const;
	bool ValidateCanonicalStellarDataset(
		const FAPSCanonicalStellarDataset& Dataset, uint32 ExpectedInputHash) const;
	uint32 BuildCanonicalStellarDatasetHash() const;
	void SynchronizeCanonicalStellarManifestHomeRecord(
		const FClusterStarSystemRecord& Record);
	bool ComposeCanonicalStellarProjection(const FVector& HomeClusterLocalUnits);
	bool RefreshCanonicalClusterProxy(int32 InstanceIndex);

	void GenerateGalaxiesCluster();

	void GenerateGalaxy();

	void GeneratePlanetSystem();

	void GenerateSinglePlanet();

	void GenerateRandomWorld();

	void InitAstroGenerators();
	
	void ApplySpawnParameters();

	bool CheckGeneratorsFails();

	void GenerateStarSystemByModel();

	void ShowPlanetsList(TArray<TSharedPtr<FPlanetData>> PlanetDataMap);

	void SpawnMoons(UWorld* World, APlanet* Planet, int32 NumberOfMoons);

	bool SpawnConfiguredCivilizationAssets(const USpawnParameters* Parameters);
	void DestroyConfiguredCivilizationAssets();

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AGalaxy* GeneratedGalaxy;
	FAPSCanonicalStellarProjectionDescriptor CanonicalStellarProjection;
	uint64 CanonicalStellarProjectionBuildCounter{0u};
	bool bCanonicalStellarProjectionComposed{false};
	bool bConsumedFinalizedCanonicalStellarDataset{false};
	bool bCanonicalStellarDatasetValidated{false};
	bool bCanonicalStellarDatasetRejected{false};

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AStarCluster* GeneratedStarCluster;

	/** HISM point selected as the home system; retained until successful actor materialization. */
	TWeakObjectPtr<AStarCluster> PendingHomeCluster;
	int32 PendingHomeClusterInstanceIndex{INDEX_NONE};
	/**
	 * Visible world-space radius of the home cluster HISM sample immediately before
	 * materialization hides it. Galaxy/cluster presentation reuses this exact radius
	 * on HomeStar, so the hierarchy transition resolves to one continuous point.
	 */
	double MaterializedHomeProxyWorldRadius{0.0};
	/** Authored mesh transform restored after HomeStar temporarily represents its HISM proxy. */
	FTransform HomeStarMeshBaseRelativeTransform{FTransform::Identity};
	bool bHasHomeStarMeshBaseRelativeTransform{false};

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AStarSystem* GeneratedHomeStarSystem;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceHeadquarters* HomeSpaceHeadquarters;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceStation* HomeSpaceStation;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceship* HomeSpaceship;

	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	ASpaceShipyard* HomeSpaceShipyard;

	/** Full requested fleet; index zero is HomeSpaceship. */
	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	TArray<ASpaceship*> GeneratedStartingFleet;

	/** Physical star/planet/orbital/ground nodes created from the civilization manifest. */
	UPROPERTY(VisibleAnywhere, Category = "Generated Tech Actros")
	TArray<AActor*> GeneratedCivilizationInfrastructure;

	/** Prevents a second generation callback from duplicating the committed civilization starter set. */
	UPROPERTY(Transient)
	bool bStarterHierarchySpawned{false};

	UPROPERTY(VisibleAnywhere, Category = "Generated Astro Actros")
	AActor* GeneratedWorld;

	double FullScaleValue{1000000000.0};

public:
	TMap<int32, TSharedPtr<FStarModel>> StarIndexModelMap;
	TMap<FString, TSharedPtr<FStarModel>> PreviewResolvedStarModels;

	static void DestroyActorTree(AActor* Root);
	
	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateFullScaledWorld{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bGenerateHomeSystem{false};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bStartWithHomePlanet{false};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bSpawnStarterLocation{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bSpawnStarterPlanet{true};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	bool bIntegrateStartPlanet{false};

	UPROPERTY(EditAnywhere, Category = "Generation Params")
	EAstroGenerationLevel AstroGenerationLevel{EAstroGenerationLevel::StarCluster};

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	AWorldScapeRoot* WSR_StartHomePlanet;

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	APlanet* HomePlanet;

	UPROPERTY(EditInstanceOnly, Category = "Generation Params")
	ASpaceHeadquarters* BP_Headquarters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APlanetarySystem* HomePlanetarySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AStar* HomeStar;

	// STAR SYSTEM GENERATOR
	UPROPERTY(VisibleAnywhere, Category = "Home System")
	double StarSystemDeadZone{1};

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	EHomeSystemPosition HomeSystemPosition;

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	bool bFullScaledHomeSystem{false};

	UPROPERTY(VisibleAnywhere, Category = "Home System")
	FVector HomeSystemRadius{0};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomHomeSystem{false};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeSystemType;

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	bool bRandomHomeStar;

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bNeedOrbitRotation{true};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bOrbitRotationCheck{true};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	int PlanetsAmount{1};

	UPROPERTY(EditAnywhere, Category = "Home System")
	bool bRandomStartPlanetNumber{true};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomStartPlanetNumber"))
	int StartPlanetNumber{1};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStarType HomeSystemStarType{EStarType::SingleStar};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EStellarType HomeStarStellarType{EStellarType::MainSequence};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	ESpectralClass HomeStarSpectralClass{ESpectralClass::G};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EPlanetarySystemType HomeSystemPlanetaryType{EPlanetarySystemType::MultiPlanetSystem};

	UPROPERTY(EditAnywhere, Category = "Home System", meta = (EditCondition = "!bRandomHomeSystem"))
	EOrbitDistributionType HomeSystemOrbitDistributionType{EOrbitDistributionType::Uniform};

	// STAR CLUSTER GENERATOR
	UPROPERTY(EditAnywhere, Category = "Galaxy Cluster")
	bool bGenerateRandomGalaxyCluster{false};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	bool bGenerateRandomGalaxy{false};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyType GalaxyType;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	EGalaxyClass GalaxyGlass;

	UPROPERTY(EditAnywhere, Category = "Star Cluster")
	bool bGenerateRandomCluster{false};

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterSize StarClusterSize;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterType StarClusterType;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterPopulation StarClusterPopulation;

	UPROPERTY(EditAnywhere, Category = "Star Cluster", meta = (EditCondition = "!bGenerateRandomCluster"))
	EStarClusterComposition StarClusterComposition;

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxySize{250};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	int GalaxyStarCount{100000000};

	UPROPERTY(EditAnywhere, Category = "Galaxy")
	double GalaxyStarDensity{10.0};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawn{true};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	bool bCharacterSpawnAtRandomPlanet{true};

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	ECharSpawnPlace CharSpawnPlace;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	EOrbitHeight HomeSpaceStationOrbitHeight;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<APawn> BP_CharacterClass;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceStation> BP_HomeSpaceStation;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceship> BP_HomeSpaceship;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceShipyard> BP_HomeSpaceShipyard;

	UPROPERTY(EditAnywhere, Category = "Player Spawn")
	TSubclassOf<ASpaceHeadquarters> BP_HomeSpaceHeadquarters;

	UPROPERTY()
	TArray<AStarSystem*> GeneratedStarSystems;

	UPROPERTY()
	UGalaxyGenerator* GalaxyGenerator;

	UPROPERTY()
	UStarClusterGenerator* StarClusterGenerator;

	UPROPERTY()
	UStarSystemGenerator* StarSystemGenerator;

	UPROPERTY()
	UPlanetarySystemGenerator* PlanetarySystemGenerator;

	UPROPERTY()
	UStarGenerator* StarGenerator;

	UPROPERTY()
	UPlanetGenerator* PlanetGenerator;

	UPROPERTY()
	UMoonGenerator* MoonGenerator;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AGalaxy> BP_GalaxyClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStarCluster> BP_StarClusterClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanetarySystem> BP_PlanetarySystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStarSystem> BP_StarSystemClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AStar> BP_StarClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanet> BP_PlanetClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<APlanetOrbit> BP_PlanetOrbitClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AMoon> BP_MoonClass;

	UPROPERTY(EditAnywhere, Category = "AstroObject BP")
	TSubclassOf<AAstroAnchor> BP_AstroAnchorClass;

	int GetRandomValueFromStarAmountRange(EStarClusterType ClusterType);

	void GenerateStarCluster();

	/** Promotes one cheap HISM system record into an actor hierarchy on demand. */
	UFUNCTION(BlueprintCallable, Category = "World Generation|Star Cluster")
	AStarSystem* MaterializeClusterStarSystem(int32 InstanceIndex);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Star Cluster")
	bool DematerializeClusterStarSystem(int32 InstanceIndex);
	
	bool AddGeneratedWorldModelData();

	UFUNCTION()
	FVector DetermineHomeSystemSpawnLocation();

	UFUNCTION()
	FVector GetHomeSystemSpawnLocationForGalaxy(TArray<AActor*> AttachedActors, int32 RandomIndex);

	UFUNCTION()
	FVector GetHomeSystemSpawnLocationForStarCluster(TArray<AActor*> AttachedActors, int32 RandomIndex);

	void GenerateStarSystem(AStarSystem* NewStarSystem, TSharedPtr<FStarSystemModel> StarSystemModel);
	
	void RotatePlanetOrbits(APlanetarySystem* NewPlanetarySystem);
	
	void ComputeHomeSystemPosition(FTransform& HomeSystemTransform, FVector& HomeSystemSpawnLocation);
	
	void IntegrateStartPlanetIntoSystem();
	
	void GenerateStarSystemAndIntegratePlanet();
};

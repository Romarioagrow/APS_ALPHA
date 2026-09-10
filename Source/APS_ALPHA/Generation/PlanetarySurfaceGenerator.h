#pragma once

#include "WorldScapeCore/Public/WorldScapeRoot.h"
#include "AtmoScape/Public/PlanetaryAtmosphere.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "PlanetarySurfaceGenerator.generated.h"

class APlanet;
class AMoon;
class APlanetaryBody;
class UAPSWorldScapePlanetNoise;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FAmbientParameters
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "100.0"))
	float Opacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "100.0"))
	float MultiScatering = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.1", UIMax = "80.0"))
	float RayleighHeight = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.01", UIMax = "15.0"))
	float MieHeight = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "-0.935", UIMax = "0.935"))
	float MiePhase = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (UIMin = "0.0", UIMax = "1.0"))
	float OzoneContribution = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FColor RayleighColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FColor MieColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	FColor AbsorptionColor = FColor::Blue;

	// Конструктор по умолчанию для инициализации всех свойств
	FAmbientParameters()
		: Opacity(1.0f)
		, MultiScatering(1.0f)
		, RayleighHeight(8.0f)
		, MieHeight(1.2f)
		, MiePhase(0.5f)
		, OzoneContribution(0.5f)
		, RayleighColor(FColor::Blue)
		, MieColor(FColor::Blue)
		, AbsorptionColor(FColor::Blue)
	{
	}
};

USTRUCT(BlueprintType)
struct FPlanetColorParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FColor MinColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FColor MaxColor;

	// Конструктор по умолчанию для инициализации всех свойств
	FPlanetColorParams()
		: MinColor(FColor::White)
		, MaxColor(FColor::White)
	{
	}
};

UCLASS()
class APS_ALPHA_API APlanetarySurfaceGenerator : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlanetarySurfaceGenerator();

	FAmbientParameters AmbientParams;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* MoonLikeNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* LavaWorldNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* SelenaeNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* SelenaeMetalNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* EarthLikeNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* EarthNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* TerraNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* IceWorldNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* TerraDesert;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Noises")
	UWorldScapeNoiseClass* TerraForestNoise;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstance* MI_Terra;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstance* MI_Selenae;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstance* MI_Magma;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstance* MI_Planetary_Ocean;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstance* MI_Lava_Ocean;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitEnviroment(APlanet* NewPlanet, UWorld* World);

	void InitAtmoScape(UWorld* World, double Radius, APlanetaryBody* PlanetaryBody);

public:
	UPROPERTY(VisibleAnywhere, Category = "World Scape")
	AWorldScapeRoot* WorldScapeRootInstance;

	/** True only for transient roots created by this generator. Editor-authored home worlds are never destroyed by streaming. */
	UPROPERTY(Transient)
	bool bOwnsWorldScapeRootInstance{false};

	UPROPERTY(Transient)
	bool bSurfaceProfileApplied{false};

	/**
	 * Signature of the body data used to configure the current runtime root.
	 * A preview body can stay alive while its type, radius, seed or presentation
	 * scale changes, so a plain boolean is not sufficient to detect a stale
	 * WorldScape preset.
	 */
	UPROPERTY(Transient)
	uint32 AppliedSurfaceProfileSignature{0};

	/** Resolved model-driven profile shared by preview and gameplay WorldScape roots. */
	UPROPERTY(Transient, VisibleAnywhere, Category = "World Scape")
	FAPSResolvedPlanetSurfaceProfile ResolvedSurfaceProfile;

	/** Unique immutable noise instance; shared mutable noise assets are unsafe with concurrently streamed planets. */
	UPROPERTY(Transient)
	UAPSWorldScapePlanetNoise* ResolvedNoiseInstance{nullptr};

	/** Per-body material instances keep seeded palettes isolated between planets. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ResolvedTerrainMaterialInstance{nullptr};

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ResolvedOceanMaterialInstance{nullptr};

	/** Optional authored catalog. Native definitions remain a safe fallback when the asset is unavailable. */
	UPROPERTY(Transient)
	UAPSPlanetSurfaceCatalog* SurfaceProfileCatalog{nullptr};

	/**
	 * WorldScape writes generated vertex arrays from background tasks. Destroying
	 * its root while those tasks are in UWorldScapeLod::SetData is an immediate
	 * access violation, so both owned runtime roots and borrowed/manual roots use a
	 * two-phase shutdown. Owned roots are destroyed after the drain; borrowed roots
	 * are only disabled and retained by their external owner.
	 */
	UPROPERTY(Transient)
	bool bPendingWorldScapeUnload{false};

	UPROPERTY(Transient)
	bool bDestroyWorldScapeRootAfterDrain{false};

	UPROPERTY(VisibleAnywhere, Category = "Atmo Scape")
	AAtmoScape* PlanetAtmosphere;

	UPROPERTY(VisibleAnywhere, Category = "World Scape")
	double RadiusKM;

	APlanetaryBody* PlanetaryBody;

	void InitWorldScape(UWorld* World);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, APlanet* NewPlanet);

	void GenerateWorldscapeSurfaceByModel(UWorld* World, AMoon* NewMoon);

	/** Creates a disabled, unit-scale runtime root so its profile can be applied before generation begins. */
	bool CreateRuntimeWorldScapeRoot(APlanetaryBody* Body);

	/**
	 * Replaces the current runtime root only after its complete WorldScape worker
	 * batch has drained. The previous root and its resolved profile references stay
	 * intact when destruction is rejected, so callers can safely retain their
	 * fallback instead of spawning a second producer beside a live orphan.
	 */
	bool ReplaceDrainedRuntimeWorldScapeRoot(APlanetaryBody* Body);

	/** Applies the deterministic terrain/ocean profile for the owning body before its first WorldScape tick. */
	void ApplySurfaceProfile(APlanetaryBody* Body);

	/**
	 * True while a live profile edit is waiting for the current WorldScape worker
	 * batch to finish. During this window the old profile and all of its UObject
	 * dependencies stay resident and the root producer remains frozen.
	 */
	bool IsSurfaceProfileApplyPending() const { return bPendingSurfaceProfileApply; }

	/** True only when the current root was configured from this body's latest editable data. */
	bool IsSurfaceProfileCurrent(const APlanetaryBody* Body) const;

	/**
	 * Rebuilds Water's runtime MID only after WorldScape has published one complete
	 * ocean batch. This keeps the first visible frame off the material resource that
	 * was created before the procedural section proxies existed. Other liquids keep
	 * their authored runtime instances unchanged.
	 */
	bool FinalizeStableWaterMaterial();

	void SpawnWorldScapeRoot();

	/** Keep assets and the configured root resident without spending generation time. */
	void PreloadWorldScapeRoot();

	/** Keep already generated chunks visible while another body in the same family updates. */
	void FreezeWorldScapeRoot();

	void DestroyPlanetEnvironment();

	/** Release transient runtime roots when their complete planetary family leaves the retention zone. */
	void UnloadWorldScapeRoot();

private:
	enum class EDeferredWorldScapeRootState : uint8
	{
		Preloaded,
		FrozenVisible,
		Active
	};

	uint32 BuildSurfaceProfileSignature(const APlanetaryBody* Body) const;
	void ApplySurfaceProfileNow(APlanetaryBody* Body);
	void QueueSurfaceProfileApply(APlanetaryBody* Body);
	void HoldWorldScapeRootForProfileDrain();
	void TryFinalizeSurfaceProfileApply();
	void CancelPendingSurfaceProfileApply();
	void TryFinalizeWorldScapeUnload();

	/** Latest body edit wins while one immutable WorldScape worker batch drains. */
	TWeakObjectPtr<APlanetaryBody> PendingSurfaceProfileBody;
	bool bPendingSurfaceProfileApply{false};
	EDeferredWorldScapeRootState DeferredWorldScapeRootState{
		EDeferredWorldScapeRootState::Preloaded};

	/** Root/profile pair whose stable Water MID has already been published. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AWorldScapeRoot> FinalizedWaterMaterialRoot;

	UPROPERTY(Transient)
	uint32 FinalizedWaterMaterialProfileSignature{0};
};

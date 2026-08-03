#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "MVVMViewModelBase.h"
#include "WorldGenerationViewModel.generated.h"

class AAstroGenerator;
class UGeneratedWorld;
class USpawnParameters;

UENUM(BlueprintType)
enum class EAPSGenerationRoute : uint8
{
	Civilization,
	Space,
	Planet
};

UENUM(BlueprintType)
enum class EAPSStartAssetSlot : uint8
{
	Character,
	Spaceship,
	SpaceStation,
	Headquarters,
	Shipyard
};

UCLASS(BlueprintType)
class APS_ALPHA_API UWorldGenerationViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize(UObject* InWorldContext, UGeneratedWorld* InGeneratedWorld);
	void Shutdown();

	UFUNCTION(BlueprintPure, Category = "World Generation")
	UGeneratedWorld* GetGeneratedWorld() const { return GeneratedWorld; }

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetEnumValue(const UEnum* EnumClass, int32 SelectedValue);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetGalaxySize(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetGalaxyStarCount(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetGalaxyStarDensity(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetPlanetRadius(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetMoonsAmount(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetPlanetsAmount(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void SetStartPlanetIndex(double Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void RequestPreview();

	void CancelPendingPreview();

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void RegeneratePreviewVariant();

	UFUNCTION(BlueprintCallable, Category = "World Generation|Preview")
	void SetPreviewFocus(EAstroPreviewFocus NewFocus);

	void SetGenerationRoute(EAPSGenerationRoute NewRoute);
	EAPSGenerationRoute GetGenerationRoute() const { return GenerationRoute; }

	void OrbitPreview(FVector2D ScreenDelta);
	void ZoomPreview(float WheelDelta);
	bool FocusPreviewUnderCursor();
	void GetPreviewBodyEntries(TArray<FAPSPreviewBodyEntry>& OutEntries) const;
	bool FocusPreviewBody(const TWeakObjectPtr<AActor>& BodyActor);
	AActor* GetSelectedPreviewBody() const { return SelectedPreviewBody.Get(); }

	UFUNCTION(BlueprintCallable, Category = "World Generation|Civilization")
	void SetSpawnClass(EAPSStartAssetSlot Slot, UClass* NewClass);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Civilization")
	void SetCharacterSpawnPlace(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Civilization")
	void SetStationOrbitHeight(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "World Generation")
	void CommitAndOpenLevel(FName LevelName = TEXT("L_WorldGeneration"));

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "World Generation")
	TObjectPtr<UGeneratedWorld> GeneratedWorld;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "World Generation|Preview")
	int32 PreviewRevision{0};

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "World Generation|Preview")
	bool bPreviewReady{false};

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "World Generation|Preview")
	FText PreviewStatus;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "World Generation|Civilization")
	TObjectPtr<USpawnParameters> SpawnParameters;

	EAstroPreviewFocus GetPreviewFocus() const { return PreviewFocus; }

private:
	void ExecutePreview();
	AAstroGenerator* FindOrCreatePreviewGenerator();
	void InitializeSpawnDefaultsFromGenerator(AAstroGenerator* Generator);
	void SetPreviewStatus(const FText& Status, bool bReady);

	TWeakObjectPtr<UObject> WorldContext;
	TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
	FTimerHandle PreviewTimerHandle;
	EAstroPreviewFocus PreviewFocus{EAstroPreviewFocus::Overview};
	EAPSGenerationRoute GenerationRoute{EAPSGenerationRoute::Civilization};
	TWeakObjectPtr<AActor> SelectedPreviewBody;
	bool bPreserveCameraOnNextPreview{false};
	bool bForceRefocusOnNextPreview{false};
};

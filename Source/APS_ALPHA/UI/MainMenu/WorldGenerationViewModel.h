#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WorldGenerationViewModel.generated.h"

class AAstroGenerator;
class UGeneratedWorld;

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

private:
	void ExecutePreview();
	AAstroGenerator* FindOrCreatePreviewGenerator();
	void SetPreviewStatus(const FText& Status, bool bReady);

	TWeakObjectPtr<UObject> WorldContext;
	TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
	FTimerHandle PreviewTimerHandle;
};

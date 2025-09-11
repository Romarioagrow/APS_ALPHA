#pragma once

#include "CoreMinimal.h"
#include "ObstacleTypes.generated.h"

// Енамы для системы препятствий
UENUM(BlueprintType)
enum class EObstacleType : uint8
{
	None		UMETA(DisplayName = "None"),
	Step		UMETA(DisplayName = "Step"),
	LowWall		UMETA(DisplayName = "Low Wall"),
	HighWall	UMETA(DisplayName = "High Wall"),
	Slope		UMETA(DisplayName = "Slope"),
	Stairs		UMETA(DisplayName = "Stairs")
};

// Структура для информации о препятствии
USTRUCT(BlueprintType)
struct APS_ALPHA_API FObstacleInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	EObstacleType Type = EObstacleType::None;

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	float Height = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	FVector Normal = FVector::UpVector;

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	float Distance = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Obstacle")
	FVector ImpactPoint = FVector::ZeroVector;
};

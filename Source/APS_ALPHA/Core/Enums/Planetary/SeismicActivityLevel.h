#pragma once

UENUM(BlueprintType)
enum class ESeismicActivityLevel : uint8
{
	Inactive       UMETA(DisplayName = "Inactive"),        // Нет активности
	Minor          UMETA(DisplayName = "Minor Activity"),  // Незначительная активность
	Moderate       UMETA(DisplayName = "Moderate Activity"), // Умеренная активность
	Active         UMETA(DisplayName = "Active"),          // Активная сейсмическая активность
	Intense        UMETA(DisplayName = "Intense Activity"), // Интенсивная активность
};
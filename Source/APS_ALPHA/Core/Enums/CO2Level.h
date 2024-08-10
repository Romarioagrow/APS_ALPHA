#pragma once

UENUM(BlueprintType)
enum class ECO2Level : uint8
{
	NoCO2  UMETA(DisplayName = "No CO2"),  // Доиндустриальный уровень
	PreIndustrial  UMETA(DisplayName = "Pre-Industrial"),  // Доиндустриальный уровень
	Modern         UMETA(DisplayName = "Modern"),          // Современный уровень
	Elevated       UMETA(DisplayName = "Elevated"),        // Повышенный уровень
	High           UMETA(DisplayName = "High"),            // Высокий уровень
	Extreme        UMETA(DisplayName = "Extreme"),         // Экстремально высокий уровень
};
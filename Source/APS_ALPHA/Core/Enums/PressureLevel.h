#pragma once

UENUM(BlueprintType)
enum class EPressureLevel : uint8
{
	NoPressure    UMETA(DisplayName = "No Pressure"),      // Низкое давление
	LowPressure    UMETA(DisplayName = "Low Pressure"),      // Низкое давление
	BelowAverage   UMETA(DisplayName = "Below Average"),     // Ниже среднего
	Average        UMETA(DisplayName = "Average"),           // Среднее давление
	AboveAverage   UMETA(DisplayName = "Above Average"),     // Выше среднего
	HighPressure   UMETA(DisplayName = "High Pressure"),     // Высокое давление
};
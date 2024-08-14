#pragma once

UENUM(BlueprintType)
enum class EOxigenLevel : uint8
{
	NoOxigen      UMETA(DisplayName = "No Oxigen"),           // Следовые количества
	Trace       UMETA(DisplayName = "Trace"),           // Следовые количества
	LowOxygen   UMETA(DisplayName = "Low Oxygen"),      // Низкий уровень кислорода
	EarthLike   UMETA(DisplayName = "Earth-Like"),      // Как на Земле (21%)
	HighOxygen  UMETA(DisplayName = "High Oxygen"),     // Высокий уровень кислорода
	Hyperoxic   UMETA(DisplayName = "Hyperoxic"),       // Гипероксическая атмосфера
};
#pragma once

UENUM(BlueprintType)
enum class EBiomassLevel : uint8
{
	NoBiomass		UMETA(DisplayName = "No Biomass"),          // Разреженная биомасса
	Sparse			UMETA(DisplayName = "Sparse"),          // Разреженная биомасса
	LowBiomass		UMETA(DisplayName = "Low Biomass"),     // Низкая биомасса
	ModerateBiomass UMETA(DisplayName = "Moderate Biomass"), // Умеренная биомасса
	HighBiomass		UMETA(DisplayName = "High Biomass"),    // Высокая биомасса
	Dense			UMETA(DisplayName = "Dense"),           // Плотная биомасса
};
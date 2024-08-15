#pragma once

UENUM(BlueprintType)
enum class EBiodiversityIndex : uint8
{
	NoBiodiversity		UMETA(DisplayName = "No Biodiversity"),    // Однородная экосистема
	Monoculture			UMETA(DisplayName = "Monoculture"),    // Однородная экосистема
	LowDiversity		UMETA(DisplayName = "Low Diversity"),  // Низкое биоразнообразие
	ModerateDiversity	UMETA(DisplayName = "Moderate Diversity"), // Умеренное биоразнообразие
	HighDiversity		UMETA(DisplayName = "High Diversity"), // Высокое биоразнообразие
	VeryDiverse			UMETA(DisplayName = "Very Diverse"),   // Очень высокое биоразнообразие
};

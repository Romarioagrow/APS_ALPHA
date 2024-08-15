#pragma once

UENUM(BlueprintType)
enum class ECrustThicknessLevel : uint8
{
	VeryThin       UMETA(DisplayName = "Very Thin"),        // Очень тонкая кора
	Thin           UMETA(DisplayName = "Thin"),             // Тонкая кора
	Medium         UMETA(DisplayName = "Medium Thickness"), // Средняя толщина коры
	Thick          UMETA(DisplayName = "Thick"),            // Толстая кора
	VeryThick      UMETA(DisplayName = "Very Thick"),       // Очень толстая кора
};


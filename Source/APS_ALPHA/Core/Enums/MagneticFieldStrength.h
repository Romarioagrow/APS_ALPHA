#pragma once

UENUM(BlueprintType)
enum class EMagneticFieldStrength : uint8
{
	NoField        UMETA(DisplayName = "No Magnetic Field"),  // Отсутствие магнитного поля
	WeakField      UMETA(DisplayName = "Weak Magnetic Field"), // Слабое магнитное поле
	ModerateField  UMETA(DisplayName = "Moderate Magnetic Field"), // Умеренное магнитное поле
	StrongField    UMETA(DisplayName = "Strong Magnetic Field"),  // Сильное магнитное поле
	IntenseField   UMETA(DisplayName = "Intense Magnetic Field"), // Интенсивное магнитное поле
};

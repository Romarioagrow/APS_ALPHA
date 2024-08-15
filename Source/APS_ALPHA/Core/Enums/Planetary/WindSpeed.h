#pragma once

UENUM(BlueprintType)
enum class EWindSpeed : uint8
{
	NoWind      UMETA(DisplayName = "No Wind"),           // Штиль
	Calm        UMETA(DisplayName = "Calm"),           // Штиль
	Breeze      UMETA(DisplayName = "Breeze"),         // Легкий бриз
	Gale        UMETA(DisplayName = "Gale"),           // Сильный ветер
	Storm       UMETA(DisplayName = "Storm"),          // Шторм
	Hurricane   UMETA(DisplayName = "Hurricane"),      // Ураган
};
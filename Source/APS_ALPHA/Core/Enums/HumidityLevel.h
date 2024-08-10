#pragma once

UENUM(BlueprintType)
enum class EHumidityLevel : uint8
{
	NoHumidity UMETA(DisplayName = "No Humidity"),     // Очень сухо, как в пустыне
	DesertDry UMETA(DisplayName = "Desert Dry"),     // Очень сухо, как в пустыне
	Arid      UMETA(DisplayName = "Arid"),           // Засушливый
	Moderate  UMETA(DisplayName = "Moderate"),       // Умеренная влажность
	Humid     UMETA(DisplayName = "Humid"),          // Влажно
	Tropical  UMETA(DisplayName = "Tropical"),       // Очень влажно, тропики
};
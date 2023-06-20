#pragma once

UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Terrestrial		UMETA(DisplayName = "Terrestrial"), // Землеподобные планеты, как Земля
	Rocky			UMETA(DisplayName = "Rocky"),
	Greenhouse		UMETA(DisplayName = "Greenhouse Planet"), // Парниковые планеты, подобные Венере// Скалистые планеты (состоят в основном из камня)
	Melted 			UMETA(DisplayName = "Melted Planet"), // Melted
	HotGiant		UMETA(DisplayName = "Hot Giant"), // Hot Газовые гиганты
	GasGiant		UMETA(DisplayName = "Gas Giant"), // Газовые гиганты, как Юпитер
	IceGiant		UMETA(DisplayName = "Ice Giant"), // Ледяные гиганты, как Нептун
	Dwarf			UMETA(DisplayName = "Dwarf Planet"), // Карликовые планеты, как Плутон
	Exoplanet		UMETA(DisplayName = "Exoplanet"), // Экзопланеты - планеты вокруг других звезд
	Rogue			UMETA(DisplayName = "Rogue Planet"), // Бродячие планеты - планеты, которые не привязаны к конкретной звезде
	Ocean			UMETA(DisplayName = "Ocean Planet"), // Океанические планеты - планеты, полностью покрытые океаном
	Water			UMETA(DisplayName = "Ocean Planet"), // WaterWorld планеты - планеты, покрытые океаном
	Desert			UMETA(DisplayName = "Desert Planet"), // Пустынные планеты, на которых нет воды
	Forest			UMETA(DisplayName = "Forest Planet"), // Лесные планеты с обильной растительностью
	Volcanic		UMETA(DisplayName = "Volcanic Planet"), // Вулканические планеты с активной вулканической деятельностью
	Ice				UMETA(DisplayName = "Ice Planet"), // Ледяные планеты, покрытые льдом
	Frozen			UMETA(DisplayName = "Frozen Planet"), // Frozen планеты, покрытые льдом
	Ammonia			UMETA(DisplayName = "Ammonia Planet"), // Планеты аммиака, на которых преобладают аммиачные соединения
	Iron			UMETA(DisplayName = "Iron Planet"), // Железные планеты, состоящие преимущественно из металлов
	Carbon			UMETA(DisplayName = "Carbon Planet"), // Углеродные планеты, где преобладает углерод
	SuperEarth		UMETA(DisplayName = "Super-Earth"), // Супер-Земли, которые значительно больше нашей планеты
	Lava			UMETA(DisplayName = "Lava Planet"), // Планеты, полностью или большей частью покрытые раскаленной лавой.
	Metallic		UMETA(DisplayName = "Metallic Planet"), // Планеты, состоящие преимущественно из металлов, но не обязательно железа.

	Unknown	UMETA(DisplayName = "Unknown") // Unknown
};
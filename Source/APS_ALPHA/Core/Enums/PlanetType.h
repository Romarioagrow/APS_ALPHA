#pragma once

UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Rocky = 0			UMETA(DisplayName = "Rocky Planet"),
	Terrestrial = 1		UMETA(DisplayName = "Terrestrial Planet"), // Землеподобные планеты, как Земля
	Greenhouse = 2		UMETA(DisplayName = "Greenhouse Planet"), // Парниковые планеты, подобные Венере// Скалистые планеты (состоят в основном из камня)
	Melted = 3 			UMETA(DisplayName = "Melted Planet"), // Melted
	HotGiant = 4		UMETA(DisplayName = "Hot Giant"), // Hot Газовые гиганты
	GasGiant = 5		UMETA(DisplayName = "Gas Giant"), // Газовые гиганты, как Юпитер
	IceGiant = 6		UMETA(DisplayName = "Ice Giant"), // Ледяные гиганты, как Нептун
	Dwarf = 7			UMETA(DisplayName = "Dwarf Planet"), // Карликовые планеты, как Плутон
	Ocean = 8			UMETA(DisplayName = "Ocean Planet"), // Океанические планеты - планеты, полностью покрытые океаном
	Water = 9			UMETA(DisplayName = "Ocean Planet"), // WaterWorld планеты - планеты, покрытые океаном
	Desert = 10			UMETA(DisplayName = "Desert Planet"), // Пустынные планеты, на которых нет воды
	Forest = 11			UMETA(DisplayName = "Forest Planet"), // Лесные планеты с обильной растительностью
	Volcanic = 12		UMETA(DisplayName = "Volcanic Planet"), // Вулканические планеты с активной вулканической деятельностью
	Ice = 13				UMETA(DisplayName = "Ice Planet"), // Ледяные планеты, покрытые льдом
	Frozen = 14			UMETA(DisplayName = "Frozen Planet"), // Frozen планеты, покрытые льдом
	Ammonia = 15			UMETA(DisplayName = "Ammonia Planet"), // Планеты аммиака, на которых преобладают аммиачные соединения
	Metal = 16			UMETA(DisplayName = "Metal Planet"), // Железные планеты, состоящие преимущественно из металлов
	Carbon = 17			UMETA(DisplayName = "Carbon Planet"), // Углеродные планеты, где преобладает углерод
	SuperEarth = 18		UMETA(DisplayName = "Super-Earth"), // Супер-Земли, которые значительно больше нашей планеты
	Lava = 19			UMETA(DisplayName = "Lava Planet"), // Планеты, полностью или большей частью покрытые раскаленной лавой.
	Metallic = 20		UMETA(DisplayName = "Metallic Planet"), // Планеты, состоящие преимущественно из металлов, но не обязательно железа.
	Nordic = 21			UMETA(DisplayName = "Nordic Planet"),
	Tundra = 22			UMETA(DisplayName = "Tundra Planet"),
	HighMountain = 23	UMETA(DisplayName = "High Mountain Planet"),
	Sand = 24			UMETA(DisplayName = "Sand Planet"),
	Oasis = 25			UMETA(DisplayName = "Oasis Planet"),
	Archipelago = 26 	UMETA(DisplayName = "Archipelago Planet"),
	Pangea = 27		 	UMETA(DisplayName = "Pangea Planet"),
	Rogue = 28			UMETA(DisplayName = "Rogue Planet"), // Бродячие планеты - планеты, которые не привязаны к конкретной звезде
	Exoplanet = 29		UMETA(DisplayName = "Exoplanet", Hidden), // Экзопланеты - планеты вокруг других звезд
	Unknown = 30	UMETA(DisplayName = "Unknown"), // Unknown: stable saved ID, not the final preset
	// Append-only additions. Keep legacy Exoplanet=29 resolvable for saved worlds.
	Basalt = 31 UMETA(DisplayName = "Basalt Planet"),
	Savanna = 32 UMETA(DisplayName = "Savanna Planet"),
	Sulfur = 33 UMETA(DisplayName = "Sulfur Planet"),
	Crystal = 34 UMETA(DisplayName = "Crystal Planet")
};

namespace APSPlanetTypes
{
	inline constexpr uint8 LegacyLastValue = 30;
	inline constexpr uint8 LastValue = 34;

	// UI creation policy only. Loading and resolving legacy Exoplanet remains valid.
	inline constexpr bool IsSelectable(EPlanetType Type)
	{
		return static_cast<uint8>(Type) <= LastValue
			&& Type != EPlanetType::Unknown && Type != EPlanetType::Exoplanet;
	}
}

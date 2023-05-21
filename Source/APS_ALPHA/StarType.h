#pragma once

UENUM(BlueprintType)
enum class EStellarClass : uint8
{
	HyperGiant		UMETA(DisplayName = "Hypergiant"), // Гипергиганты (экстремально большие звезды)
	SuperGiant		UMETA(DisplayName = "Super Giant"), // Супергиганты (наибольшие звезды)
	BrightGiant		UMETA(DisplayName = "Bright Giant"), // BrightGiant
	Giant			UMETA(DisplayName = "Giant"), // Гиганты (большие звезды)
	SubGiant		UMETA(DisplayName = "SubGiant"), // SubGiant 
	MainSequence	UMETA(DisplayName = "Main Sequence"), // Главная последовательность (как наше Солнце)
	SubDwarf		UMETA(DisplayName = "Subdwarf"), // Субкарлики (звезды с малой яркостью и размером)
	WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // Белые карлики (маленькие, горячие звезды)
	BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // Коричневые карлики (не достаточно массы для начала термоядерных реакций)
	Neutron			UMETA(DisplayName = "Neutron Star"), // Нейтронные звезды (остатки взрывов суперновых, очень плотные)
	Protostar		UMETA(DisplayName = "Protostar"), // Протозвезды (молодые звезды, которые еще не начали основную термоядерную реакцию)
	Pulsar			UMETA(DisplayName = "Pulsar"), // Пульсары (вращающиеся нейтронные звезды, которые излучают импульсы радиоволн)
	BlackHole		UMETA(DisplayName = "Black Hole"), // Черные дыры (объекты с такой сильной гравитацией, что даже свет не может уйти)
	Unknown			UMETA(DisplayName = "Black Hole") // Черные дыры (объекты с такой сильной гравитацией, что даже свет не может уйти)
};

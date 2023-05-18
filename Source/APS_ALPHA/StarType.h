UENUM(BlueprintType)
enum class EStarType : uint8
{
	MainSequence	UMETA(DisplayName = "Main Sequence"), // Главная последовательность (как наше Солнце)
	Giant			UMETA(DisplayName = "Giant"), // Гиганты (большие звезды)
	SuperGiant		UMETA(DisplayName = "Super Giant"), // Супергиганты (наибольшие звезды)
	WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // Белые карлики (маленькие, горячие звезды)
	Neutron			UMETA(DisplayName = "Neutron Star"), // Нейтронные звезды (остатки взрывов суперновых, очень плотные)
	BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // Коричневые карлики (не достаточно массы для начала термоядерных реакций)
	RedDwarf		UMETA(DisplayName = "Red Dwarf"), // Красные карлики (маленькие и холодные звезды)
	SubDwarf		UMETA(DisplayName = "Subdwarf"), // Субкарлики (звезды с малой яркостью и размером)
	HyperGiant		UMETA(DisplayName = "Hypergiant"), // Гипергиганты (экстремально большие звезды)
	Protostar		UMETA(DisplayName = "Protostar"), // Протозвезды (молодые звезды, которые еще не начали основную термоядерную реакцию)
	Pulsar			UMETA(DisplayName = "Pulsar"), // Пульсары (вращающиеся нейтронные звезды, которые излучают импульсы радиоволн)
	BlackHole		UMETA(DisplayName = "Black Hole") // Черные дыры (объекты с такой сильной гравитацией, что даже свет не может уйти)
};
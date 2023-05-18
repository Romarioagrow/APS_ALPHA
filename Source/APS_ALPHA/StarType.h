UENUM(BlueprintType)
enum class EStarType : uint8
{
	MainSequence	UMETA(DisplayName = "Main Sequence"), // ������� ������������������ (��� ���� ������)
	Giant			UMETA(DisplayName = "Giant"), // ������� (������� ������)
	SuperGiant		UMETA(DisplayName = "Super Giant"), // ������������ (���������� ������)
	WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // ����� ������� (���������, ������� ������)
	Neutron			UMETA(DisplayName = "Neutron Star"), // ���������� ������ (������� ������� ����������, ����� �������)
	BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // ���������� ������� (�� ���������� ����� ��� ������ ������������ �������)
	RedDwarf		UMETA(DisplayName = "Red Dwarf"), // ������� ������� (��������� � �������� ������)
	SubDwarf		UMETA(DisplayName = "Subdwarf"), // ���������� (������ � ����� �������� � ��������)
	HyperGiant		UMETA(DisplayName = "Hypergiant"), // ������������ (������������ ������� ������)
	Protostar		UMETA(DisplayName = "Protostar"), // ����������� (������� ������, ������� ��� �� ������ �������� ������������ �������)
	Pulsar			UMETA(DisplayName = "Pulsar"), // �������� (����������� ���������� ������, ������� �������� �������� ���������)
	BlackHole		UMETA(DisplayName = "Black Hole") // ������ ���� (������� � ����� ������� �����������, ��� ���� ���� �� ����� ����)
};
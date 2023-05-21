#pragma once

UENUM(BlueprintType)
enum class EStellarClass : uint8
{
	HyperGiant		UMETA(DisplayName = "Hypergiant"), // ������������ (������������ ������� ������)
	SuperGiant		UMETA(DisplayName = "Super Giant"), // ������������ (���������� ������)
	BrightGiant		UMETA(DisplayName = "Bright Giant"), // BrightGiant
	Giant			UMETA(DisplayName = "Giant"), // ������� (������� ������)
	SubGiant		UMETA(DisplayName = "SubGiant"), // SubGiant 
	MainSequence	UMETA(DisplayName = "Main Sequence"), // ������� ������������������ (��� ���� ������)
	SubDwarf		UMETA(DisplayName = "Subdwarf"), // ���������� (������ � ����� �������� � ��������)
	WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // ����� ������� (���������, ������� ������)
	BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // ���������� ������� (�� ���������� ����� ��� ������ ������������ �������)
	Neutron			UMETA(DisplayName = "Neutron Star"), // ���������� ������ (������� ������� ����������, ����� �������)
	Protostar		UMETA(DisplayName = "Protostar"), // ����������� (������� ������, ������� ��� �� ������ �������� ������������ �������)
	Pulsar			UMETA(DisplayName = "Pulsar"), // �������� (����������� ���������� ������, ������� �������� �������� ���������)
	BlackHole		UMETA(DisplayName = "Black Hole"), // ������ ���� (������� � ����� ������� �����������, ��� ���� ���� �� ����� ����)
	Unknown			UMETA(DisplayName = "Black Hole") // ������ ���� (������� � ����� ������� �����������, ��� ���� ���� �� ����� ����)
};

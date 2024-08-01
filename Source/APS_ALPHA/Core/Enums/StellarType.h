#pragma once

UENUM(BlueprintType)
enum class EStellarType : uint8
{
	MainSequence	UMETA(DisplayName = "Main Sequence"), // ������� ������������������ (��� ���� ������)
	SubDwarf		UMETA(DisplayName = "Subdwarf"), // ���������� (������ � ����� �������� � ��������)
	SubGiant		UMETA(DisplayName = "SubGiant"), // SubGiant 
	Giant			UMETA(DisplayName = "Giant"), // ������� (������� ������)
	BrightGiant		UMETA(DisplayName = "Bright Giant"), // BrightGiant
	SuperGiant		UMETA(DisplayName = "Super Giant"), // ������������ (���������� ������)
	HyperGiant		UMETA(DisplayName = "Hypergiant"), // ������������ (������������ ������� ������)
	WhiteDwarf		UMETA(DisplayName = "White Dwarf"), // ����� ������� (���������, ������� ������)
	BrownDwarf		UMETA(DisplayName = "Brown Dwarf"), // ���������� ������� (�� ���������� ����� ��� ������ ������������ �������)
	Neutron			UMETA(DisplayName = "Neutron Star"), // ���������� ������ (������� ������� ����������, ����� �������)
	Protostar		UMETA(DisplayName = "Protostar"), // ����������� (������� ������, ������� ��� �� ������ �������� ������������ �������)
	Pulsar			UMETA(DisplayName = "Pulsar"), // �������� (����������� ���������� ������, ������� �������� �������� ���������)
	BlackHole		UMETA(DisplayName = "Black Hole"), // ������ ���� (������� � ����� ������� �����������, ��� ���� ���� �� ����� ����)
	Unknown			UMETA(DisplayName = "Black Hole") // ������ ���� (������� � ����� ������� �����������, ��� ���� ���� �� ����� ����)
};

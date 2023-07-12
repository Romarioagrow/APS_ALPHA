#pragma once

UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Rocky			UMETA(DisplayName = "Rocky"),
	Terrestrial		UMETA(DisplayName = "Terrestrial"), // ������������� �������, ��� �����
	Greenhouse		UMETA(DisplayName = "Greenhouse Planet"), // ���������� �������, �������� ������// ��������� ������� (������� � �������� �� �����)
	Melted 			UMETA(DisplayName = "Melted Planet"), // Melted
	HotGiant		UMETA(DisplayName = "Hot Giant"), // Hot ������� �������
	GasGiant		UMETA(DisplayName = "Gas Giant"), // ������� �������, ��� ������
	IceGiant		UMETA(DisplayName = "Ice Giant"), // ������� �������, ��� ������
	Dwarf			UMETA(DisplayName = "Dwarf Planet"), // ���������� �������, ��� ������
	Ocean			UMETA(DisplayName = "Ocean Planet"), // ������������ ������� - �������, ��������� �������� �������
	Water			UMETA(DisplayName = "Ocean Planet"), // WaterWorld ������� - �������, �������� �������
	Desert			UMETA(DisplayName = "Desert Planet"), // ��������� �������, �� ������� ��� ����
	Forest			UMETA(DisplayName = "Forest Planet"), // ������ ������� � �������� ���������������
	Volcanic		UMETA(DisplayName = "Volcanic Planet"), // ������������� ������� � �������� ������������� �������������
	Ice				UMETA(DisplayName = "Ice Planet"), // ������� �������, �������� �����
	Frozen			UMETA(DisplayName = "Frozen Planet"), // Frozen �������, �������� �����
	Ammonia			UMETA(DisplayName = "Ammonia Planet"), // ������� �������, �� ������� ����������� ��������� ����������
	Metal			UMETA(DisplayName = "Metal Planet"), // �������� �������, ��������� ��������������� �� ��������
	Carbon			UMETA(DisplayName = "Carbon Planet"), // ���������� �������, ��� ����������� �������
	SuperEarth		UMETA(DisplayName = "Super-Earth"), // �����-�����, ������� ����������� ������ ����� �������
	Lava			UMETA(DisplayName = "Lava Planet"), // �������, ��������� ��� ������� ������ �������� ����������� �����.
	Metallic		UMETA(DisplayName = "Metallic Planet"), // �������, ��������� ��������������� �� ��������, �� �� ����������� ������.
	Nordic			UMETA(DisplayName = "Metallic Planet"), // 
	Tundra			UMETA(DisplayName = "Metallic Planet"), // 
	HighMountain	UMETA(DisplayName = "Metallic Planet"), // 
	Sand			UMETA(DisplayName = "Metallic Planet"), // 
	Oasis			UMETA(DisplayName = "Metallic Planet"), // 
	Archipelago 	UMETA(DisplayName = "Metallic Planet"), // 
	Pangea		 	UMETA(DisplayName = "Metallic Planet"), // 

	Rogue			UMETA(DisplayName = "Rogue Planet"), // �������� ������� - �������, ������� �� ��������� � ���������� ������
	Exoplanet		UMETA(DisplayName = "Exoplanet"), // ����������� - ������� ������ ������ �����
	Unknown	UMETA(DisplayName = "Unknown") // Unknown
};
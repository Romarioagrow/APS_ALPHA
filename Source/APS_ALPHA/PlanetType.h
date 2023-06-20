#pragma once

UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Terrestrial		UMETA(DisplayName = "Terrestrial"), // ������������� �������, ��� �����
	Rocky			UMETA(DisplayName = "Rocky"),
	Greenhouse		UMETA(DisplayName = "Greenhouse Planet"), // ���������� �������, �������� ������// ��������� ������� (������� � �������� �� �����)
	Melted 			UMETA(DisplayName = "Melted Planet"), // Melted
	HotGiant		UMETA(DisplayName = "Hot Giant"), // Hot ������� �������
	GasGiant		UMETA(DisplayName = "Gas Giant"), // ������� �������, ��� ������
	IceGiant		UMETA(DisplayName = "Ice Giant"), // ������� �������, ��� ������
	Dwarf			UMETA(DisplayName = "Dwarf Planet"), // ���������� �������, ��� ������
	Exoplanet		UMETA(DisplayName = "Exoplanet"), // ����������� - ������� ������ ������ �����
	Rogue			UMETA(DisplayName = "Rogue Planet"), // �������� ������� - �������, ������� �� ��������� � ���������� ������
	Ocean			UMETA(DisplayName = "Ocean Planet"), // ������������ ������� - �������, ��������� �������� �������
	Water			UMETA(DisplayName = "Ocean Planet"), // WaterWorld ������� - �������, �������� �������
	Desert			UMETA(DisplayName = "Desert Planet"), // ��������� �������, �� ������� ��� ����
	Forest			UMETA(DisplayName = "Forest Planet"), // ������ ������� � �������� ���������������
	Volcanic		UMETA(DisplayName = "Volcanic Planet"), // ������������� ������� � �������� ������������� �������������
	Ice				UMETA(DisplayName = "Ice Planet"), // ������� �������, �������� �����
	Frozen			UMETA(DisplayName = "Frozen Planet"), // Frozen �������, �������� �����
	Ammonia			UMETA(DisplayName = "Ammonia Planet"), // ������� �������, �� ������� ����������� ��������� ����������
	Iron			UMETA(DisplayName = "Iron Planet"), // �������� �������, ��������� ��������������� �� ��������
	Carbon			UMETA(DisplayName = "Carbon Planet"), // ���������� �������, ��� ����������� �������
	SuperEarth		UMETA(DisplayName = "Super-Earth"), // �����-�����, ������� ����������� ������ ����� �������
	Lava			UMETA(DisplayName = "Lava Planet"), // �������, ��������� ��� ������� ������ �������� ����������� �����.
	Metallic		UMETA(DisplayName = "Metallic Planet"), // �������, ��������� ��������������� �� ��������, �� �� ����������� ������.

	Unknown	UMETA(DisplayName = "Unknown") // Unknown
};
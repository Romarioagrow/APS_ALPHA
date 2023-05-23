#pragma once

UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Terrestrial UMETA(DisplayName = "Terrestrial"), // ������������� �������, ��� �����
	Rocky		UMETA(DisplayName = "Rocky"), // ��������� ������� (������� � �������� �� �����)
	GasGiant	UMETA(DisplayName = "Gas Giant"), // ������� �������, ��� ������
	IceGiant	UMETA(DisplayName = "Ice Giant"), // ������� �������, ��� ������
	Dwarf		UMETA(DisplayName = "Dwarf Planet"), // ���������� �������, ��� ������
	Exoplanet	UMETA(DisplayName = "Exoplanet"), // ����������� - ������� ������ ������ �����
	Rogue		UMETA(DisplayName = "Rogue Planet"), // �������� ������� - �������, ������� �� ��������� � ���������� ������
	Ocean		UMETA(DisplayName = "Ocean Planet"), // ������������ ������� - �������, ��������� �������� �������
	Desert		UMETA(DisplayName = "Desert Planet"), // ��������� �������, �� ������� ��� ����
	Forest		UMETA(DisplayName = "Forest Planet"), // ������ ������� � �������� ���������������
	Volcanic	UMETA(DisplayName = "Volcanic Planet"), // ������������� ������� � �������� ������������� �������������
	Ice			UMETA(DisplayName = "Ice Planet"), // ������� �������, �������� �����
	Ammonia		UMETA(DisplayName = "Ammonia Planet"), // ������� �������, �� ������� ����������� ��������� ����������
	Iron		UMETA(DisplayName = "Iron Planet"), // �������� �������, ��������� ��������������� �� ��������
	Carbon		UMETA(DisplayName = "Carbon Planet"), // ���������� �������, ��� ����������� �������
	SuperEarth	UMETA(DisplayName = "Super-Earth"), // �����-�����, ������� ����������� ������ ����� �������
	Unknown	UMETA(DisplayName = "Unknown") // Unknown
};
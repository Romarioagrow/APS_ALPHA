#pragma once
UENUM(BlueprintType)
enum class EMoonType : uint8
{
	Rocky				UMETA(DisplayName = "Rocky"), // ��������� ���� (������� � �������� �� �����)
	Icy					UMETA(DisplayName = "Icy"), // ������� ���� (������� � �������� �� ����)
	Iron				UMETA(DisplayName = "Iron"), // �������� ���� (������� � �������� �� ��������)
	Volcanic			UMETA(DisplayName = "Volcanic"), // ������������� ���� (�������� ������� � ������������� ����������)
	Gas					UMETA(DisplayName = "Gas"), // ������� ���� (����� ������������ ���������)
	Ocean				UMETA(DisplayName = "Ocean"), // ������������ ���� (����� ������ ��� ������������)
	Continental			UMETA(DisplayName = "Continental"), // ��������������� ���� (����� ���� � ������)
	Desert				UMETA(DisplayName = "Desert"), // ��������� ���� (���� ����, ���������� �������)
	TidallyLocked		UMETA(DisplayName = "Tidally Locked"), // �������� ��������������� ���� (���� ������� ������ �������� � �������)
	Peculiar			UMETA(DisplayName = "Peculiar"), // �������� ���� (��������� ��������, ������� �� �������� ��� ������ ���������)
	CapturedAsteroid	UMETA(DisplayName = "Captured Asteroid") // ����������� ��������� (���������, �������� �� ������ ������� � ������� ������)
};
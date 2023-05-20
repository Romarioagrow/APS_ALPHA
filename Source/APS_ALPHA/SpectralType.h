
#pragma once	
#include "CoreMinimal.h"
UENUM(BlueprintType)
enum class ESpectralType : uint8
{
	O				UMETA(DisplayName = "Ia0 Hypergiant"), // ������������
//	Iaa				UMETA(DisplayName = "Ia+ Brightest Supergiant"), // �������� ������������
	Ia				UMETA(DisplayName = "Ia Bright Supergiant"), // ����� ������������
	Iab				UMETA(DisplayName = "Iab Normal Supergiant"), // ���������� ������������
	Ib				UMETA(DisplayName = "Ib Less Luminous Supergiant"), // ������������ ������ ����������
	II				UMETA(DisplayName = "II Bright Giant"), // ����� �������
	III				UMETA(DisplayName = "III Giant"), // �������
	IV				UMETA(DisplayName = "IV Subgiant"), // ����������
	V				UMETA(DisplayName = "V Main Sequence"), // ������ ������� ������������������
	VI				UMETA(DisplayName = "VI Subdwarf"), // ����������
	VII				UMETA(DisplayName = "VII White Dwarf"), // ����� �������
};

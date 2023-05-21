
#pragma once	
#include "CoreMinimal.h"
UENUM(BlueprintType)
enum class ESpectralType : uint8
{
	O				UMETA(DisplayName = "0 Hypergiant"), // ������������
	Ia				UMETA(DisplayName = "Ia Luminous Supergiant"), // ����� ������������
	Iab				UMETA(DisplayName = "Iab Normal Supergiant"), // ���������� ������������
	Ib				UMETA(DisplayName = "Ib Supergiant"), // ������������ ������ ����������
	II				UMETA(DisplayName = "II Bright Giant"), // ����� �������
	III				UMETA(DisplayName = "III Giant"), // �������
	IV				UMETA(DisplayName = "IV Subgiant"), // ����������
	V				UMETA(DisplayName = "V Main Sequence"), // ������ ������� ������������������
	VI				UMETA(DisplayName = "VI Subdwarf"), // ����������
	VII				UMETA(DisplayName = "VII White Dwarf"), // ����� �������
	VIII			UMETA(DisplayName = "VIII Brown Dwarf"), // ����� �������
	Unknown			UMETA(DisplayName = "Unknown") // �����������

};

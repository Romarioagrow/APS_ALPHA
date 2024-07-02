
#pragma once	
#include "CoreMinimal.h"
UENUM(BlueprintType)
enum class ESpectralType : uint8
{
	O				UMETA(DisplayName = "0 Hypergiant"), // Гипергиганты
	Ia				UMETA(DisplayName = "Ia Luminous Supergiant"), // Яркие сверхгиганты
	Iab				UMETA(DisplayName = "Iab Normal Supergiant"), // Нормальные сверхгиганты
	Ib				UMETA(DisplayName = "Ib Supergiant"), // Сверхгиганты низкой светимости
	II				UMETA(DisplayName = "II Bright Giant"), // Яркие гиганты
	III				UMETA(DisplayName = "III Giant"), // Гиганты
	IV				UMETA(DisplayName = "IV Subgiant"), // Субгиганты
	V				UMETA(DisplayName = "V Main Sequence"), // Звезды главной последовательности
	VI				UMETA(DisplayName = "VI Subdwarf"), // Субкарлики
	VII				UMETA(DisplayName = "VII White Dwarf"), // Белые карлики
	VIII			UMETA(DisplayName = "VIII Brown Dwarf"), // Белые карлики
	Unknown			UMETA(DisplayName = "Unknown") // Неизвестный

};

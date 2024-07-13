#include "AstroGenerationMenu.h"

#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	
	//GS_GenerationSlider->SetEnumContent(EAstroGenerationLevel);
	//GS_MenuSlider->SetEnumContent(EGalaxyClass);
	//GS_GenSlider->SetEnumContent(EGravityState);
	
	//SetEnumContent

	//EnumSlider

	if (GS_GenerationSlider)
	{
		GS_GenerationSlider->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::Update_AstroGenerationLevel);
		GS_GenerationSlider->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::Update_AstroGenerationLevel);
		GS_GenerationSlider->SetEnumContent(StaticEnum<EAstroGenerationLevel>());
	}
}

void UAstroGenerationMenu::Update_AstroGenerationLevel(float InValue)
{
	if (GS_GenerationSlider && GS_GenerationSlider->EnumArray.Num() > 0)
	{
		int32 Index = FMath::RoundToInt(InValue * (GS_GenerationSlider->EnumArray.Num() - 1));
		if (GS_GenerationSlider->EnumArray.IsValidIndex(Index))
		{
			uint8 EnumValue = GS_GenerationSlider->EnumArray[Index];
			FString EnumName = StaticEnum<EAstroGenerationLevel>()->GetNameStringByValue(EnumValue);
			
			UE_LOG(LogTemp, Warning, TEXT("Current Enum Value: %s"), *EnumName);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Current Enum Value: %s"), *EnumName));
			}
		}
	}
}

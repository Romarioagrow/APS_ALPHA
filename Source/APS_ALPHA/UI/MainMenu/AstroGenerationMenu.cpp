#include "AstroGenerationMenu.h"

#include "GenerationSlider.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"

void UAstroGenerationMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (GS_GenerationLevel)
	{
		GS_GenerationLevel->OnGenerationSliderChanged.AddDynamic(this, &UAstroGenerationMenu::Update_AstroGenerationLevel);
		GS_GenerationLevel->SetEnumContent(StaticEnum<EAstroGenerationLevel>());
	}
}

void UAstroGenerationMenu::Update_AstroGenerationLevel(float InValue)
{
	if (GS_GenerationLevel && GS_GenerationLevel->EnumArray.Num() > 0)
	{
		if (GS_GenerationLevel->EnumArray.IsValidIndex(InValue))
		{
			uint8 EnumValue = GS_GenerationLevel->EnumArray[InValue];
			AstroGenerationLevel = static_cast<EAstroGenerationLevel>(EnumValue);

			if (GEngine)
			{
				FString AstroGenerationLevelName = StaticEnum<EAstroGenerationLevel>()->GetNameStringByValue(static_cast<uint8>(AstroGenerationLevel));
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Astro Generation Level: %s"), *AstroGenerationLevelName));

			}
		}
	}
}

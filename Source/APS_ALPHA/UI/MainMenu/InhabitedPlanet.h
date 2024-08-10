#pragma once

#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/UI/BaseWidget.h"
#include "InhabitedPlanet.generated.h"

UCLASS()
class UInhabitedPlanet : public UBaseWidget
{
	GENERATED_BODY()
	
public:
	void Setup(FPlanetData PlanetData);
	
};

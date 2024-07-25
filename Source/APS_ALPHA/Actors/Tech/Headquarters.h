#pragma once

#include "CoreMinimal.h"
#include "TechActor.h"
#include "Headquarters.generated.h"

class UCivilization;

UCLASS(ABSTRACT)
class APS_ALPHA_API AHeadquarters : public ATechActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UCivilization* Civilization;
};

#pragma once

#include "AstroObject.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstonomicalSystem.generated.h"

UINTERFACE(MinimalAPI)
class UAstonomicalSystem : public UAstroObject
{
	GENERATED_BODY()
};

class APS_ALPHA_API IAstonomicalSystem
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
};

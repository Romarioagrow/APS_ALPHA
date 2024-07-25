#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Interfaces/AstroLocation.h"
#include "UObject/Interface.h"
#include "TechObject.generated.h"

UINTERFACE(MinimalAPI)
class UTechObject : public UAstroLocation
{
	GENERATED_BODY()
};

class APS_ALPHA_API ITechObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
};

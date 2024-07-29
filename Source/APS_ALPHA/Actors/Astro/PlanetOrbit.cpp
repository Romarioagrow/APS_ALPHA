#include "PlanetOrbit.h"

void APlanetOrbit::TriggerClearChildren()
{
	OnClearChildren.Broadcast();
}

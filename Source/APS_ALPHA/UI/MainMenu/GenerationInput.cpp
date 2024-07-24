#include "GenerationInput.h"

#include "Components/SpinBox.h"

float UGenerationInput::GetCurrentValue()
{
	return SpinBox_Value->GetValue();
}

#pragma once

#include "CoreMinimal.h"
#include "Internationalization/Internationalization.h"

/**
 * Numbers of the game UI in the UI's language, English: "6,750" and "1.06" on any system locale. Under the Russian
 * locale FText::AsNumber groups thousands with a no-break space, which the Orbitron font lacks: a box was drawn inside
 * every large number (Rio, 02.10: "6[]750 ... it is everywhere").
 */
namespace APSUINumber
{
	inline FCulturePtr Culture()
	{
		// Looked up each time instead of held in a static: a culture outliving the ICU shutdown crashes on exit.
		return FInternationalization::Get().GetCulture(TEXT("en"));
	}

	template <typename T>
	FText Number(T Value, const FNumberFormattingOptions* const Options = nullptr)
	{
		return FText::AsNumber(Value, Options, Culture());
	}

	template <typename T>
	FText Percent(T Value, const FNumberFormattingOptions* const Options = nullptr)
	{
		return FText::AsPercent(Value, Options, Culture());
	}
}

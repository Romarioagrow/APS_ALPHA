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

	/** Rio 15.10 ("on the route millions of AU: write light years then; small ones stay in AU"): a distance of at least a
	 * tenth of a light year (6,324 AU) as "0.12 LY", "14.5 LY", "7,802 LY". False below that (the caller keeps its AU). */
	inline bool LightYears(const double Cm, FText& Out)
	{
		constexpr double LightYearCm = 9.4607304725808e17;
		if (!FMath::IsFinite(Cm) || Cm < 0.1 * LightYearCm)
		{
			return false;
		}
		const double Ly = Cm / LightYearCm;
		FNumberFormattingOptions Options;
		Options.SetMaximumFractionalDigits(Ly >= 100.0 ? 0 : Ly >= 10.0 ? 1 : 2);
		Out = FText::Format(NSLOCTEXT("APSUINumber", "LightYears", "{0} LY"), Number(Ly, &Options));
		return true;
	}
}

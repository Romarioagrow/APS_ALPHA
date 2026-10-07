#pragma once

#include "CoreMinimal.h"

/**
 * Counter-based SplitMix64 stream for catalogue generation (Rio 03.10, galaxy/cluster V2).
 * A star's stream is keyed by (seed, index, salt), so any record can be resolved alone,
 * in any order and on any thread, without touching the global FMath::Rand state.
 */
namespace APSHashStream
{
	inline uint64 Mix64(uint64 Value)
	{
		Value ^= Value >> 30;
		Value *= 0xbf58476d1ce4e5b9ull;
		Value ^= Value >> 27;
		Value *= 0x94d049bb133111ebull;
		Value ^= Value >> 31;
		return Value;
	}

	inline uint64 Key(const int32 Seed, const int64 Index, const uint64 Salt)
	{
		return Mix64((static_cast<uint64>(static_cast<uint32>(Seed)) << 32)
			^ static_cast<uint64>(Index) ^ Salt ^ 0x9e3779b97f4a7c15ull);
	}

	struct FStream
	{
		explicit FStream(const uint64 InState)
			: State(InState)
		{
		}

		uint64 Next()
		{
			State += 0x9e3779b97f4a7c15ull;
			return Mix64(State);
		}

		/** [0, 1) with 53 random bits. */
		double U()
		{
			return static_cast<double>(Next() >> 11) * (1.0 / 9007199254740992.0);
		}

		double Range(const double Min, const double Max)
		{
			return Min + (Max - Min) * U();
		}

		/** Integer in [0, Count). */
		int32 Index(const int32 Count)
		{
			return Count > 1 ? FMath::Min(static_cast<int32>(U() * Count), Count - 1) : 0;
		}

		/**
		 * Bounded bell in [-1.5, 1.5] (sigma 0.5): soft edges without unbounded tails.
		 * Every draw is its own statement: C++ leaves the order of calls inside one expression
		 * unspecified, and a catalogue must not depend on the compiler's choice.
		 */
		double Bell()
		{
			const double A = U();
			const double B = U();
			const double C = U();
			return A + B + C - 1.5;
		}

		/** Uniform direction from exactly two draws (no rejection loop). */
		FVector UnitVector()
		{
			const double Z = Range(-1.0, 1.0);
			const double Phi = U() * UE_DOUBLE_TWO_PI;
			const double Planar = FMath::Sqrt(FMath::Max(0.0, 1.0 - Z * Z));
			return FVector(FMath::Cos(Phi) * Planar, FMath::Sin(Phi) * Planar, Z);
		}

		uint64 State;
	};
}

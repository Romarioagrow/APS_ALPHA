#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"

/** Per-instance optical sampling, never a change to physical radius or catalog position. */
namespace APSStellarOpticalSupport
{
inline constexpr int32 CoreScaleIndex = 11; // Fields 7..10 reserved for explicit projection data.
inline constexpr int32 RayStrengthIndex = 12;
inline constexpr int32 RequiredStride = 13;
inline constexpr double CompactSupportPixels = 2.2;
inline constexpr double MaximumSupportPixels = 14.0;

struct FProfile
{
	double SupportPixels = CompactSupportPixels;
	float RayStrength = 0.0f;
};

/**
 * RayRule (A5, Rio 01.10: "some stars have rays, others not"): 0 the accepted stable rank (a share of the bright
 * stars sparkles, so the field is no uniform grid of crosses), 1 every star bright enough sparkles, 2 none does.
 */
inline FProfile Select(const TArray<float>& Data, const int32 Stride, const int32 Index, const int32 RayRule = 0)
{
	FProfile Result;
	const int64 Base = int64(Index) * Stride;
	if (Index < 0 || Stride < 6 || Base + Stride > Data.Num()) return Result;
	for (int32 Field = 0; Field < 6; ++Field)
		if (!FMath::IsFinite(Data[Base + Field])) return Result;
	const double Peak = FMath::Max3(Data[Base], Data[Base + 1], Data[Base + 2]);
	const double Emission = FMath::Max(double(Data[Base + 3]), 0.0);
	const double Gain = Stride > 6 ? double(Data[Base + 6]) : 1.0;
	if (Peak <= 0.025 || Emission <= 0.0 || !FMath::IsFinite(Gain) || Gain <= 0.0) return Result;
	const double Luminance = FMath::Max(0.0,
		(0.2126 * Data[Base] + 0.7152 * Data[Base + 1] + 0.0722 * Data[Base + 2]) / Peak);
	const double Activity = FMath::Clamp(FMath::Log2(1.0 + Emission) / 8.97
		+ FMath::Clamp(double(Data[Base + 5]), 0.0, 1.0) * 0.14, 0.0, 1.0);
	// Bright warm giants can sparkle too. Temperature influences apparent brightness,
	// not a binary spectral exclusion. A stable rank prevents a uniform field of crosses.
	const double Brightness = Activity * (0.60 + 0.40 * FMath::Sqrt(Luminance))
		* FMath::Clamp(Gain, 0.5, 1.2);
	const double Rank = FMath::Frac(FMath::Abs(double(Data[Base + 4])) * 17.713 + 0.37);
	const double RankThreshold = FMath::Lerp(0.96, 0.72, FMath::Clamp(Brightness * 2.0, 0.0, 1.0));
	if (Brightness < 0.055 || RayRule == 2 || (RayRule == 1 ? Brightness < 0.3 : Rank < RankThreshold)) return Result;
	const double Strength = FMath::Clamp((Brightness - 0.035) / 0.42, 0.0, 1.0);
	Result.SupportPixels = FMath::Lerp(8.0, MaximumSupportPixels, Strength);
	Result.RayStrength = float(FMath::Lerp(0.35, 1.0, Strength));
	return Result;
}

inline double CoreRadius(const double PhysicalRadius, const double PixelWorldRadius)
{
	return FMath::Max(PhysicalRadius, PixelWorldRadius * CompactSupportPixels);
}

inline double CarrierRadius(const double PhysicalRadius, const double PixelWorldRadius, const FProfile& Profile)
{
	return FMath::Max(CoreRadius(PhysicalRadius, PixelWorldRadius), PixelWorldRadius * Profile.SupportPixels);
}

inline float CoreScale(const double Core, const double Carrier)
{
	return FMath::IsFinite(Core) && FMath::IsFinite(Carrier) && Core > 0.0
		? float(FMath::Clamp(Carrier / Core, 1.0, MaximumSupportPixels / CompactSupportPixels)) : 1.0f;
}

inline float ResolvedRayStrength(const FProfile& Profile, const double PhysicalRadius, const double PixelWorldRadius)
{
	const double PixelRadius = PhysicalRadius / FMath::Max(PixelWorldRadius, 1.0e-20);
	const double Fade = FMath::Clamp((PixelRadius - 3.0) / 5.0, 0.0, 1.0);
	return Profile.RayStrength * float(1.0 - Fade * Fade * (3.0 - 2.0 * Fade));
}

// UE clears ALL channels when changing stride. Preserve every existing field;
// initialize only added fields, including neutral luminosity for six-field data.
inline bool EnsureLayout(UInstancedStaticMeshComponent* Component)
{
	if (!IsValid(Component)) return false;
	const int32 OldStride = Component->NumCustomDataFloats;
	const int32 Count = Component->GetInstanceCount();
	if (OldStride < 6 || int64(Count) * OldStride != Component->PerInstanceSMCustomData.Num()
		|| int64(Count) * RequiredStride > MAX_int32) return false;
	if (OldStride >= RequiredStride) return true;
	TArray<float> Repacked;
	Repacked.SetNumZeroed(Count * RequiredStride);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		float* Row = Repacked.GetData() + Index * RequiredStride;
		FMemory::Memcpy(Row, Component->PerInstanceSMCustomData.GetData() + Index * OldStride, OldStride * sizeof(float));
		if (OldStride <= 6) Row[6] = 1.0f;
		if (OldStride <= CoreScaleIndex) Row[CoreScaleIndex] = 1.0f;
	}
	Component->SetNumCustomDataFloats(RequiredStride);
	for (int32 Index = 0; Index < Count; ++Index)
		Component->SetCustomData(Index, TArrayView<const float>(Repacked.GetData() + Index * RequiredStride, RequiredStride), false);
	Component->MarkRenderStateDirty();
	return true;
}

inline bool Publish(UInstancedStaticMeshComponent* Component, const int32 Index, const float Scale, const float Strength)
{
	const int32 Address = Index * Component->NumCustomDataFloats;
	if (!Component->PerInstanceSMCustomData.IsValidIndex(Address + RayStrengthIndex)) return false;
	bool Changed = false;
	for (const TPair<int32, float>& Field : {TPair<int32, float>(CoreScaleIndex, Scale), TPair<int32, float>(RayStrengthIndex, Strength)})
	{
		if (!FMath::IsNearlyEqual(Component->PerInstanceSMCustomData[Address + Field.Key], Field.Value, 1.0e-5f))
		{
			Component->SetCustomDataValue(Index, Field.Key, Field.Value, false);
			Changed = true;
		}
	}
	return Changed;
}
}

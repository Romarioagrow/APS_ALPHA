#include "GeneratedWorld.h"

#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Generation/PlanetaryProceduralGenerator.h"

UGeneratedWorld::UGeneratedWorld(): GalaxyClass()
{
	bGenerateFullScaledWorld = true;
	bGenerateHomeSystem = true;
	bStartWithHomePlanet = true;
	bRandomHomeSystem = false;
	bRandomHomeSystemType = false;
	bRandomHomeStar = false;
	bRandomStartPlanetNumber = false;
	AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
	GalaxyType = EGalaxyType::Elliptical;
	GalaxyClass = EGalaxyClass::E0;
	StarClusterSize = EStarClusterSize::Giant;
	// Ring / Arc is spatially legible immediately and keeps the home-system point
	// easy to locate. This is the transient menu model default; authored SinglePlay
	// generators retain their serialized cluster preset.
	StarClusterType = EStarClusterType::RingArc;
	StarClusterPopulation = EStarClusterPopulation::Dwarfs;
	StarClusterComposition = EStarClusterComposition::Unknown;
	StarType = EStarType::SingleStar;
	StellarType = EStellarType::MainSequence;
	SpectralClass = ESpectralClass::G;
	PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
	OrbitDistributionType = EOrbitDistributionType::Uniform;
	HomeSystemPosition = EHomeSystemPosition::DirectPosition;
	GalaxySize = 250;
	// The generated galaxy owns a virtual deterministic catalog. One hundred
	// million logical stars therefore costs a fixed descriptor plus the bounded
	// HISM visual sample, rather than one UObject/record per star.
	GalaxyStarCount = 100000000;
	PlanetsAmount = 1;
	StartPlanetIndex = 1;
	GalaxyStarDensity = 10.0;
	HomePlanetarySystem = nullptr;
	HomePlanet = nullptr;
}

EPlanetHabitability UGeneratedWorld::ResolveHomePlanetHabitabilityForSave() const
{
	if (IsValid(HomePlanet))
	{
		if (HomePlanet->PlanetData.PlanetModel.IsValid())
		{
			return HomePlanet->PlanetData.PlanetModel->PlanetHabitability;
		}
		return HomePlanet->PlanetHabitability;
	}

	// Before a generated hierarchy is materialized, the editor buffer still
	// represents the authored home planet and remains the only available source.
	return PlanetHabitability;
}

void UGeneratedWorld::SetPreviewBodyEditOverride(
	const FString& StableBodyKey, const FAPSPreviewBodyEditOverride& BodyOverride)
{
	if (StableBodyKey.IsEmpty())
	{
		return;
	}
	PreviewBodyEditOverrides.Add(StableBodyKey, BodyOverride);
}

const FAPSPreviewBodyEditOverride* UGeneratedWorld::FindPreviewBodyEditOverride(
	const FString& StableBodyKey) const
{
	return StableBodyKey.IsEmpty() ? nullptr : PreviewBodyEditOverrides.Find(StableBodyKey);
}

void UGeneratedWorld::ClearPreviewBodyEditOverrides()
{
	PreviewBodyEditOverrides.Reset();
	PreviewPlanetOrbitEdits.Reset();
}

const FAPSPreviewPlanetOrbitEdit* UGeneratedWorld::FindPlanetOrbitEdit(const FString& Address) const
{
	return PreviewPlanetOrbitEdits.Find(Address);
}

void UGeneratedWorld::SetPlanetOrbitEdit(const FString& Address, const FAPSPreviewPlanetOrbitEdit& Edit)
{
	if (Address.IsEmpty() || !Address.Contains(TEXT("/P")) || Address.Contains(TEXT("/M"))) return;
	if ((Edit.bOverrideDistance && (!FMath::IsFinite(Edit.DistanceAu) || Edit.DistanceAu <= 0.0))
		|| (Edit.bOverrideInclination && !FMath::IsFinite(Edit.InclinationDegrees))) return;
	FAPSPreviewPlanetOrbitEdit Safe = Edit;
	Safe.InclinationDegrees = FMath::Clamp(Safe.InclinationDegrees, 0.0, 90.0);
	PreviewPlanetOrbitEdits.Add(Address, Safe);
}

void UGeneratedWorld::ResetPlanetOrbitEdit(const FString& Address)
{
	if (!PreviewPlanetOrbitEdits.Remove(Address)) return;
	int32 Slash = INDEX_NONE;
	if (Address.FindLastChar(TEXT('/'), Slash))
		if (auto* StarEdit = PreviewStarEditOverrides.Find(Address.Left(Slash)))
			StarEdit->bReplayPlanetOrbitLayout = false; // Old snapshot may contain the manual distance.
}

double UGeneratedWorld::MinimumPlanetOrbitAu(const double StellarRadiusSolar, const double PlanetRadiusKm)
{
	const double StarAu = (FMath::IsFinite(StellarRadiusSolar) ? FMath::Max(0.0, StellarRadiusSolar) : 0.0) * 0.00465047;
	const double PlanetAu = (FMath::IsFinite(PlanetRadiusKm) ? FMath::Max(0.0, PlanetRadiusKm) : 0.0) / 149597870.7;
	return FMath::Max3(0.001, StarAu * 1.35 + PlanetAu * 2.5, StarAu + PlanetAu * 5.0);
}

double UGeneratedWorld::MaximumPlanetOrbitAu(const double StellarRadiusSolar)
{
	// Manual edits are physical AU, never multiplied by stellar mass or preview scale.
	return FMath::Max(50.0, MinimumPlanetOrbitAu(StellarRadiusSolar, 0.0) + 20.0);
}

void UGeneratedWorld::ApplyPlanetOrbitEdits(FPlanetarySystemModel& Family, const FString& StarAddress,
	const double StellarRadiusSolar) const
{
	const double Maximum = MaximumPlanetOrbitAu(StellarRadiusSolar);
	for (int32 I = 0; I < Family.PlanetsList.Num(); ++I)
	{
		const auto& Data = Family.PlanetsList[I];
		if (!Data || !Data->PlanetModel) continue;
		const double RadiusKm = FMath::Max(static_cast<double>(Data->PlanetModel->RadiusKM), Data->PlanetModel->Radius * 6371.0);
		const double Minimum = MinimumPlanetOrbitAu(StellarRadiusSolar, RadiusKm);
		if (const auto* Edit = FindPlanetOrbitEdit(FString::Printf(TEXT("%s/P%d"), *StarAddress, I));
			Edit && Edit->bOverrideDistance && FMath::IsFinite(Edit->DistanceAu))
			Data->OrbitRadius = FMath::Clamp(Edit->DistanceAu, Minimum, FMath::Max(Minimum, Maximum));
		Data->OrbitRadius = FMath::Max(Minimum, Data->OrbitRadius);
		Data->PlanetModel->OrbitDistance = Data->OrbitRadius;
		Data->PlanetModelData = *Data->PlanetModel;
	}
	// Sort indices, not bodies: stable identity and manual ordering survive.
	UPlanetarySystemGenerator::EnforcePlanetSurfaceClearance(Family);
}

FRotator UGeneratedWorld::ResolvePlanetOrbitRotation(const FString& PlanetAddress, const FRotator& AutomaticRotation) const
{
	const auto* Edit = FindPlanetOrbitEdit(PlanetAddress);
	if (!Edit || !Edit->bOverrideInclination || !FMath::IsFinite(Edit->InclinationDegrees)) return AutomaticRotation;
	const double Tilt = FMath::DegreesToRadians(FMath::Clamp(Edit->InclinationDegrees, 0.0, 90.0));
	const FVector OldNormal = AutomaticRotation.Quaternion().GetAxisZ();
	const double Node = FMath::Atan2(OldNormal.Y, OldNormal.X);
	const FVector NewNormal(FMath::Sin(Tilt) * FMath::Cos(Node), FMath::Sin(Tilt) * FMath::Sin(Node), FMath::Cos(Tilt));
	return (FQuat::FindBetweenNormals(OldNormal, NewNormal) * AutomaticRotation.Quaternion()).Rotator();
}

bool UGeneratedWorld::SetPreviewDisplayNameOverride(const FString& StableKey, const FString& DisplayName)
{
	const FString CleanName = DisplayName.TrimStartAndEnd();
	if (StableKey.IsEmpty() || CleanName.IsEmpty() || CleanName.Len() > 128
		|| CleanName.Equals(TEXT("None"), ESearchCase::IgnoreCase)) return false;
	for (TCHAR Character : CleanName)
	{
		if (FChar::IsControl(Character)) return false;
	}
	PreviewDisplayNameOverrides.Add(StableKey, CleanName);
	return true;
}

const FString* UGeneratedWorld::FindPreviewDisplayNameOverride(const FString& StableKey) const
{
	return StableKey.IsEmpty() ? nullptr : PreviewDisplayNameOverrides.Find(StableKey);
}

void UGeneratedWorld::SetPreviewStarEditOverride(
	const FString& StableStarKey, const FAPSPreviewStarEditOverride& Edit)
{
	if (StableStarKey.IsEmpty()) return;
	FAPSPreviewStarEditOverride& Stored = PreviewStarEditOverrides.Add(StableStarKey, Edit);
	// Position belongs to the canonical system, never to an editor snapshot.
	Stored.Model.Location = Stored.AutomaticModel.Location = FVector::ZeroVector;
}

const FAPSPreviewStarEditOverride* UGeneratedWorld::FindPreviewStarEditOverride(const FString& StableStarKey) const
{
	return PreviewStarEditOverrides.Find(StableStarKey);
}

bool UGeneratedWorld::ApplyPreviewStarEditOverride(const FString& StableStarKey, FStarModel& Model) const
{
	const FAPSPreviewStarEditOverride* Edit = FindPreviewStarEditOverride(StableStarKey);
	if (!Edit) return false;
	const FVector CanonicalLocation = Model.Location;
	Model = Edit->Model;
	Model.Location = CanonicalLocation;
	return true;
}

bool FAPSPreviewStarEditOverride::TryGetPlanetOrbitRangeAu(double& OutMinimumAu, double& OutMaximumAu) const
{
	if (FMath::IsFinite(PlanetOrbitRangeMinAu) && FMath::IsFinite(PlanetOrbitRangeMaxAu)
		&& PlanetOrbitRangeMinAu > 0.0 && PlanetOrbitRangeMaxAu > PlanetOrbitRangeMinAu)
	{
		OutMinimumAu = PlanetOrbitRangeMinAu;
		OutMaximumAu = PlanetOrbitRangeMaxAu;
		return true;
	}
	if (PlanetOrbitRadiiAu.IsEmpty()) return false;
	double MinimumAu = PlanetOrbitRadiiAu[0];
	double MaximumAu = MinimumAu;
	for (const double OrbitAu : PlanetOrbitRadiiAu)
	{
		if (!FMath::IsFinite(OrbitAu) || OrbitAu <= 0.0) return false;
		MinimumAu = FMath::Min(MinimumAu, OrbitAu);
		MaximumAu = FMath::Max(MaximumAu, OrbitAu);
	}
	// Do not touch caller defaults on invalid snapshots. A single-planet range
	// is valid too; the spacing pass adds clearance if the family gains planets.
	// Repair old multi-planet snapshots compressed to one narrow belt by prior
	// resampling. Expand INWARD only: never recover a giant mass-derived maximum.
	OutMinimumAu = PlanetOrbitRadiiAu.Num() >= 3 && MaximumAu - MinimumAu < MaximumAu * 0.15
		? FMath::Max(0.001, MaximumAu * 0.04) : MinimumAu;
	OutMaximumAu = MaximumAu;
	return true;
}

void FAPSPreviewStarEditOverride::CapturePlanetOrbitLayout(const TArray<double>& RadiiAu,
	const EOrbitDistributionType Distribution, double MinimumAu, double MaximumAu)
{
	const bool bHasStableRange = FMath::IsFinite(PlanetOrbitRangeMinAu)
		&& FMath::IsFinite(PlanetOrbitRangeMaxAu) && PlanetOrbitRangeMinAu > 0.0
		&& PlanetOrbitRangeMaxAu > PlanetOrbitRangeMinAu;
	PlanetOrbitRadiiAu = RadiiAu;
	PlanetOrbitDistribution = Distribution;
	bReplayPlanetOrbitLayout = true;
	if (!bHasStableRange)
	{
		double CapturedMin = 0.0, CapturedMax = 0.0;
		if (!TryGetPlanetOrbitRangeAu(CapturedMin, CapturedMax)) return;
		// A valid model envelope is wider than its sampled points and must survive
		// subsequent Dense -> star edit -> Uniform transitions without contraction.
		if (!(FMath::IsFinite(MinimumAu) && FMath::IsFinite(MaximumAu)
			&& MinimumAu > 0.0 && MinimumAu <= CapturedMin && MaximumAu >= CapturedMax))
		{
			MinimumAu = CapturedMin;
			MaximumAu = CapturedMax;
		}
		PlanetOrbitRangeMinAu = MinimumAu;
		PlanetOrbitRangeMaxAu = FMath::Max(MaximumAu, MinimumAu + 0.25);
	}
}

bool FAPSPreviewStarEditOverride::ApplyToPlanetOrbits(FPlanetarySystemModel& Family,
	const bool bCompactOrbits, const double StellarRadiusSolar) const
{
	if (!bReplayPlanetOrbitLayout || PlanetOrbitRadiiAu.IsEmpty() || PlanetOrbitRadiiAu.Num() != Family.PlanetsList.Num()
		|| PlanetOrbitDistribution != Family.OrbitDistributionType) return false;
	// Validate before writing: an incomplete snapshot cannot half-rescale a family.
	for (int32 Index = 0; Index < PlanetOrbitRadiiAu.Num(); ++Index)
	{
		if (!FMath::IsFinite(PlanetOrbitRadiiAu[Index]) || PlanetOrbitRadiiAu[Index] <= 0.0
			|| !Family.PlanetsList[Index] || !Family.PlanetsList[Index]->PlanetModel) return false;
	}
	double OldMin = 0.0, OldMax = 0.0;
	const bool bMigrate = bCompactOrbits && !bCompactOrbitEnvelope && TryGetPlanetOrbitRangeAu(OldMin, OldMax);
	double NewMin = OldMin, NewMax = OldMax;
	if (bMigrate) UPlanetarySystemGenerator::CompactPlanetOrbitRange(NewMin, NewMax, StellarRadiusSolar);
	for (int32 Index = 0; Index < PlanetOrbitRadiiAu.Num(); ++Index)
	{
		FPlanetData& Data = *Family.PlanetsList[Index];
		Data.OrbitRadius = bMigrate ? FMath::Lerp(NewMin, NewMax,
			FMath::Clamp((PlanetOrbitRadiiAu[Index] - OldMin) / FMath::Max(OldMax - OldMin, 0.25), 0.0, 1.0)) : PlanetOrbitRadiiAu[Index];
		Data.PlanetModel->OrbitDistance = Data.OrbitRadius;
		Data.PlanetModelData = *Data.PlanetModel;
	}
	return true;
}

uint32 UGeneratedWorld::GetPreviewStarEditHash() const
{
	const auto ModelHash = [](const FStarModel& Model)
	{
		uint32 Hash = GetTypeHash(Model.Radius);
		Hash = HashCombine(Hash, GetTypeHash(Model.RadiusKM));
		Hash = HashCombine(Hash, GetTypeHash(Model.Mass));
		Hash = HashCombine(Hash, GetTypeHash(Model.Luminosity));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Model.StellarType)));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Model.SpectralClass)));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Model.SpectralType)));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Model.StarStellarClass)));
		Hash = HashCombine(Hash, GetTypeHash(Model.SurfaceTemperature));
		Hash = HashCombine(Hash, GetTypeHash(Model.SpectralSubclass));
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Model.Age));
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Model.FullSpectralName.ToString()));
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Model.FullSpectralClass.ToString()));
		Hash = HashCombine(Hash, GetTypeHash(Model.MinOrbit));
		return HashCombine(Hash, GetTypeHash(Model.MaxOrbit));
	};
	TArray<FString> Addresses;
	PreviewStarEditOverrides.GetKeys(Addresses);
	Addresses.Sort();
	uint32 Hash = 0;
	for (const FString& Address : Addresses)
	{
		const FAPSPreviewStarEditOverride& Edit = PreviewStarEditOverrides.FindChecked(Address);
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Address));
		Hash = HashCombine(Hash, ModelHash(Edit.Model));
		Hash = HashCombine(Hash, ModelHash(Edit.AutomaticModel));
		Hash = HashCombine(Hash, GetTypeHash(Edit.RadiusOverrideSolar));
		Hash = HashCombine(Hash, GetTypeHash(Edit.PlanetOrbitRangeMinAu));
		Hash = HashCombine(Hash, GetTypeHash(Edit.PlanetOrbitRangeMaxAu));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bReplayPlanetOrbitLayout));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bCompactOrbitEnvelope));
		if (!Edit.PlanetOrbitRadiiAu.IsEmpty())
		{
			Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Edit.PlanetOrbitDistribution)));
			Hash = HashCombine(Hash, GetTypeHash(Edit.PlanetOrbitRadiiAu.Num()));
			for (const double OrbitAu : Edit.PlanetOrbitRadiiAu)
				Hash = HashCombine(Hash, GetTypeHash(OrbitAu));
		}
	}
	return Hash;
}

void FAPSPreviewSystemEditOverride::ApplyToSystem(FStarSystemModel& Model) const
{
	if (StarCount != INDEX_NONE)
	{
		Model.AmountOfStars = FMath::Clamp(StarCount, 1, 6);
		Model.StarSystemType = StarType;
	}
	if (TotalPlanets != INDEX_NONE)
	{
		Model.PotentialPlanetCount = FMath::Clamp(TotalPlanets, 0, 120);
		Model.bHasPlanetarySystem = Model.PotentialPlanetCount > 0;
	}
}

void FAPSPreviewSystemEditOverride::ApplyToFamily(
	FPlanetarySystemModel& Model, const int32 StarIndex, const int32 ActualStarCount) const
{
	if (bOverridePlanetaryType) Model.PlanetarySystemType = PlanetaryType;
	if (bOverrideOrbitDistribution) Model.OrbitDistributionType = OrbitDistribution;
	if (TotalPlanets != INDEX_NONE)
	{
		const int32 Count = FMath::Clamp(ActualStarCount, 1, 6);
		const int32 Total = FMath::Clamp(TotalPlanets, 0, 120);
		Model.AmountOfPlanets = Total / Count + (StarIndex < Total % Count ? 1 : 0);
		// Counts are exact: a family with no assigned planet cannot be inflated to
		// one by the legacy SinglePlanet branch, nor can NoPlanet swallow an edit.
		if (Model.AmountOfPlanets == 0) Model.PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
		else if (Model.PlanetarySystemType == EPlanetarySystemType::NoPlanetSystem
			|| (Model.PlanetarySystemType == EPlanetarySystemType::SinglePlanetSystem && Model.AmountOfPlanets > 1))
			Model.PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
	}
}

void UGeneratedWorld::SetPreviewSystemEditOverride(const FString& Address, const FAPSPreviewSystemEditOverride& Edit)
{
	const FAPSPreviewSystemEditOverride* Previous = PreviewSystemEditOverrides.Find(Address);
	if (!Address.IsEmpty() && Edit.bOverrideOrbitDistribution && (!Previous
		|| !Previous->bOverrideOrbitDistribution || Previous->OrbitDistribution != Edit.OrbitDistribution))
	{
		// A deliberate recipe change must also resample on returning to the initial
		// recipe. Keep radius/stellar edits and the bounded envelope; drop only replay.
		for (auto& Entry : PreviewStarEditOverrides)
			if (Entry.Key.StartsWith(Address + TEXT("/S"))) Entry.Value.bReplayPlanetOrbitLayout = false;
	}
	if (!Address.IsEmpty()) PreviewSystemEditOverrides.Add(Address, Edit);
}

double UGeneratedWorld::GetSystemMaxOrbitInclinationDegrees(const FString& Address) const
{
	const FAPSPreviewSystemEditOverride* Edit = FindPreviewSystemEditOverride(Address);
	return Edit && Edit->bOverrideOrbitInclination && FMath::IsFinite(Edit->MaxOrbitInclinationDegrees)
		? FMath::Clamp(Edit->MaxOrbitInclinationDegrees, 0.0, 90.0) : 8.0;
}

const FAPSPreviewSystemEditOverride* UGeneratedWorld::FindPreviewSystemEditOverride(const FString& Address) const
{
	return PreviewSystemEditOverrides.Find(Address);
}

uint32 UGeneratedWorld::GetPreviewSystemEditHash() const
{
	TArray<FString> Addresses;
	PreviewSystemEditOverrides.GetKeys(Addresses);
	Addresses.Sort();
	uint32 Hash = 0;
	for (const FString& Address : Addresses)
	{
		const FAPSPreviewSystemEditOverride& Edit = PreviewSystemEditOverrides.FindChecked(Address);
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Address));
		Hash = HashCombine(Hash, GetTypeHash(Edit.StarCount));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Edit.StarType)));
		Hash = HashCombine(Hash, GetTypeHash(Edit.TotalPlanets));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bOverridePlanetaryType));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Edit.PlanetaryType)));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bOverrideOrbitDistribution));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Edit.OrbitDistribution)));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bOverrideOrbitInclination));
		Hash = HashCombine(Hash, GetTypeHash(Edit.MaxOrbitInclinationDegrees));
	}
	TArray<FString> PlanetAddresses;
	PreviewPlanetOrbitEdits.GetKeys(PlanetAddresses);
	PlanetAddresses.Sort();
	for (const FString& Address : PlanetAddresses)
	{
		const auto& Edit = PreviewPlanetOrbitEdits.FindChecked(Address);
		Hash = HashCombine(Hash, FCrc::StrCrc32(*Address));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bOverrideDistance));
		Hash = HashCombine(Hash, GetTypeHash(Edit.DistanceAu));
		Hash = HashCombine(Hash, GetTypeHash(Edit.bOverrideInclination));
		Hash = HashCombine(Hash, GetTypeHash(Edit.InclinationDegrees));
	}
	return Addresses.IsEmpty() && PlanetAddresses.IsEmpty() ? 0u : (Hash != 0u ? Hash : 1u);
}

int32 UGeneratedWorld::ResolveCanonicalSurfaceSeed(
	const int32 AuthoredSeed, const int32 WorldGenerationSeed, const FString& StableBodyKey)
{
	constexpr int32 MaximumSurfaceSeed = 999983;
	if (AuthoredSeed > 0)
	{
		return FMath::Clamp(AuthoredSeed, 1, MaximumSurfaceSeed);
	}

	uint32 StableHash = HashCombine(
		GetTypeHash(FMath::Max(WorldGenerationSeed, 1)), FCrc::StrCrc32(*StableBodyKey));
	// Reserve zero as the UI's explicit Auto sentinel.
	return 1 + static_cast<int32>(StableHash % static_cast<uint32>(MaximumSurfaceSeed));
}

void UGeneratedWorld::PrintAllValues() const
{
    for (TFieldIterator<FProperty> PropIt(GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();

        FString PropertyValue;
        if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
        {
            uint8 ByteValue = ByteProperty->GetPropertyValue_InContainer(this);
            PropertyValue = FString::Printf(TEXT("%d"), ByteValue);
        }
        else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            int64 EnumValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(this));
            PropertyValue = EnumProperty->GetEnum()->GetNameStringByValue(EnumValue);
        }
        else if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            bool BoolValue = BoolProperty->GetPropertyValue_InContainer(this);
            PropertyValue = BoolValue ? TEXT("true") : TEXT("false");
        }
        else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
        {
            int32 IntValue = IntProperty->GetPropertyValue_InContainer(this);
            PropertyValue = FString::Printf(TEXT("%d"), IntValue);
        }
        else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
        {
            float FloatValue = FloatProperty->GetPropertyValue_InContainer(this);
            PropertyValue = FString::Printf(TEXT("%f"), FloatValue);
        }
        else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
        {
            double DoubleValue = DoubleProperty->GetPropertyValue_InContainer(this);
            PropertyValue = FString::Printf(TEXT("%f"), DoubleValue);
        }
        else if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
        {
            FString StrValue = StrProperty->GetPropertyValue_InContainer(this);
            PropertyValue = StrValue;
        }
        else
        {
            PropertyValue = TEXT("Unsupported property type");
        }

        // Model dumps are useful for reproducing generation bugs, but they must not
        // cover the first gameplay frame. Keep the same information in the log.
        UE_LOG(LogTemp, Log, TEXT("[APS.WorldModel] %s=%s"), *PropertyName, *PropertyValue);
    }
}

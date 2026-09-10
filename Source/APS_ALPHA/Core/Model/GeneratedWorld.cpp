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
	if (!Address.IsEmpty()) PreviewSystemEditOverrides.Add(Address, Edit);
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
	}
	return Addresses.IsEmpty() ? 0u : (Hash != 0u ? Hash : 1u);
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

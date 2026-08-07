#include "GeneratedWorld.h"

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

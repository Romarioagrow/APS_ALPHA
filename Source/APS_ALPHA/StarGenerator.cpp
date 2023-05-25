// Fill out your copyright notice in the Description page of Project Settings.

#include "StarGenerator.h"
#include "StarType.h"
#include <random>

UStarGenerator::UStarGenerator()
{
}

void UStarGenerator::ApplyModel(AStar* NewStar, FStarGenerationModel StarModel) // NewStar, StarModel
{
    NewStar->SetLuminosity(StarModel.Luminosity);
    NewStar->SetSurfaceTemperature(StarModel.SurfaceTemperature);
    NewStar->SetStarType(StarModel.StellarClass);
    NewStar->SetStarSpectralClass(StarModel.SpectralClass);
    
    NewStar->SetMass(StarModel.Mass);
    NewStar->SetRadius(StarModel.Radius);
    NewStar->SetAge(StarModel.Age);

    NewStar->SetSpectralSubclass(StarModel.SpectralSubclass); 
    NewStar->SetStarSpectralType(StarModel.SpectralType);
    NewStar->SetFullSpectralClass(StarModel.FullSpectralClass);
    NewStar->SetFullSpectralName(StarModel.FullSpectralName);

}

FStarGenerationModel UStarGenerator::GenerateRandomStarModel()
{
    FStarGenerationModel StarModel;
    StarModel.StellarClass = GenerateStarClassByRandomWeights();

    //ќпределите ESpectralClass (спектральный класс) 
    StarModel.SpectralClass = ChooseSpectralClassByStellarClass(StarModel.StellarClass);

    if (StarModel.StellarClass == EStellarClass::MainSequence)
    {
        // Get the mass from the spectral class
        double Mass = RandomMass(StarModel.SpectralClass);

        // Calculate the radius and luminosity using mass-radius-luminosity relation
        double Radius = pow(Mass, 0.8);  // Radius  Mass^0.8
        double Luminosity = CalculateLuminosityByMass(Mass);
        double SurfaceTemperature = RandomFromRange(StarTypeTemperatureRanges[StarModel.SpectralClass]);

        // Generate a star model
        StarModel.Mass = Mass;
        StarModel.Radius = Radius;
        StarModel.Luminosity = Luminosity;
        StarModel.SurfaceTemperature = SurfaceTemperature;
        StarModel.Age = CalculateMainSequenceStarAge(Mass);
    }
	else 
	{
        if (!StarAttributeRanges.Contains(StarModel.StellarClass))
        {
            UE_LOG(LogTemp, Error, TEXT("Unknown stellar class: %d"), static_cast<int>(StarModel.SpectralClass));
            return StarModel;
        }
    
        FStarAttributeRanges& AttributeRanges = StarAttributeRanges[StarModel.StellarClass];
        StarModel.Mass = FMath::RandRange(AttributeRanges.Mass.Range.Get<0>(), AttributeRanges.Mass.Range.Get<1>());
        StarModel.Radius = FMath::RandRange(AttributeRanges.Radius.Range.Get<0>(), AttributeRanges.Radius.Range.Get<1>());
        StarModel.SurfaceTemperature = GenerateRandomTemperatureBySpectralClass(StarModel.SpectralClass);//CalculateSurfaceTemperature(double Luminosity, double Radius)
        StarModel.Luminosity = CalculateLuminosity(StarModel.Radius, StarModel.SurfaceTemperature);
        StarModel.Age = CalculateNonMainSequenceStarAge(StarModel.Mass);//FMath::RandRange(AttributeRanges.Age.Range.Get<0>(), AttributeRanges.Age.Range.Get<1>());
    }

    StarModel.SpectralSubclass = CalculateSpectralSubclass(StarModel.SurfaceTemperature, StarModel.SpectralClass);
    StarModel.SpectralType = CalculateSpectralType(StarModel.StellarClass, StarModel.Luminosity);
    StarModel.FullSpectralClass = GenerateFullSpectralClass(StarModel);
    StarModel.FullSpectralName = GenerateFullSpectralName(StarModel);

    return StarModel;
}

FString UStarGenerator::CalculateNonMainSequenceStarAge(double StarMass)
{
    // Coefficient is a tuning parameter to fit the function to real star age data.
    const double coefficient = 10.0; // This value may need to be adjusted

    // Age is proportional to mass^-2.5
    double AgeInBillionYears = coefficient * pow(StarMass, -2.5);

    FString FormattedAge{};

    if (AgeInBillionYears >= 1.0)
    {
        FormattedAge = FString::Printf(TEXT("%.2f bln years"), AgeInBillionYears);
    }
    else if (AgeInBillionYears >= 0.001)
    {
        AgeInBillionYears = AgeInBillionYears * 1000.0;
        FormattedAge = FString::Printf(TEXT("%.2f mln years"), AgeInBillionYears);
    }
    else
    {
        AgeInBillionYears = AgeInBillionYears * 1000000.0;
        FormattedAge = FString::Printf(TEXT("%.2f ths years"), AgeInBillionYears);
    }

    return FormattedAge;
}

FString UStarGenerator::CalculateMainSequenceStarAge(double Mass)
{
    // ѕримен€ем формулу T = M^(-2.5)
    double AgeInBillionYears = pow(Mass, -2.5);
    FString FormattedAge{};

    if (AgeInBillionYears >= 1.0)
    {
        FormattedAge = FString::Printf(TEXT("%.2f bln years"), AgeInBillionYears);
    }
    else if (AgeInBillionYears >= 0.001)
    {
        AgeInBillionYears = AgeInBillionYears * 1000.0;
        FormattedAge = FString::Printf(TEXT("%.2f mln years"), AgeInBillionYears);
    }
    else
    {
        AgeInBillionYears = AgeInBillionYears * 1000000.0;
        FormattedAge = FString::Printf(TEXT("%.2f ths years"), AgeInBillionYears);
    }

    return FormattedAge;
}

EStellarClass UStarGenerator::GenerateStarClassByRandomWeights()
{
    int TotalWeight = 0;
    for (auto const& Pair : StarTypeWeights) {
        TotalWeight += Pair.Value;
    }
    int RandomValue = FMath::RandRange(0, TotalWeight - 1);
    EStellarClass ChosenStellarClass{};
    for (auto const& pair : StarTypeWeights) {
        if (RandomValue < pair.Value) {
            ChosenStellarClass = pair.Key;
            break;
        }
        RandomValue -= pair.Value;
    }
    return ChosenStellarClass;
}

FString UStarGenerator::GetSpectralClassColor(ESpectralClass Class)
{
    if (SpectralClassColorMap.Contains(Class))
    {
        return SpectralClassColorMap[Class];
    }
    else
    {
        return TEXT("Unknown");
    }
}

FString UStarGenerator::GetSpectralTypeDescription(ESpectralType Type)
{
    if (SpectralTypeDescriptionMap.Contains(Type))
    {
        return SpectralTypeDescriptionMap[Type];
    }
    else
    {
        return TEXT("Unknown");
    }
}

FName UStarGenerator::GenerateFullSpectralClass(const FStarGenerationModel& StarModel)
{
    if (StarModel.SpectralClass == ESpectralClass::Unknown)
    {
        return FName("Unknown");
    }

    FString SpectralClassString = StaticEnum<ESpectralClass>()->GetValueAsString(StarModel.SpectralClass);
    SpectralClassString.RemoveFromStart("ESpectralClass::");  // Remove prefix

    FString SpectralTypeString = StaticEnum<ESpectralType>()->GetValueAsString(StarModel.SpectralType);
    SpectralTypeString.RemoveFromStart("ESpectralType::");  // Remove prefix

    return FName(*(SpectralClassString + FString::Printf(TEXT("%d"), StarModel.SpectralSubclass) + SpectralTypeString));
}

FName UStarGenerator::GenerateFullSpectralName(const FStarGenerationModel& StarModel)
{
    if (StarModel.SpectralClass == ESpectralClass::Unknown)
    {
		return FName("Unknown");
	}

    FString SpectralClassColor = SpectralClassColorMap[StarModel.SpectralClass];  
    FString SpectralTypeDescription = SpectralTypeDescriptionMap[StarModel.SpectralType];

    FName SpectralClassColorName(*SpectralClassColor);
    FName SpectralTypeDescriptionName(*SpectralTypeDescription);

    return FName(*(SpectralClassColorName.ToString() + " " + SpectralTypeDescriptionName.ToString()));
}

ESpectralType UStarGenerator::CalculateSpectralType(EStellarClass StellarType, double Luminosity)
{
    switch (StellarType)
    {
    case EStellarClass::HyperGiant:
        return ESpectralType::O;
    case EStellarClass::BrightGiant:
        return ESpectralType::II;
    case EStellarClass::Giant:
        return ESpectralType::III;
    case EStellarClass::SubGiant:
        return ESpectralType::IV;
    case EStellarClass::MainSequence:
        return ESpectralType::V;
    case EStellarClass::SubDwarf:
        return ESpectralType::VI;
    case EStellarClass::WhiteDwarf:
        return ESpectralType::VII;
    case EStellarClass::BrownDwarf:
        return ESpectralType::VIII;
    case EStellarClass::SuperGiant:
    {
        FStarAttributeRanges& AttributeRanges = StarAttributeRanges[StellarType];
        FLuminosityRange LuminosityRange = AttributeRanges.Luminosity;  // получаем диапазон светимости

        double LowerThird = (2.0 / 3.0) * LuminosityRange.Range.Key;
        double MiddleThird = (2.0 / 3.0) * LuminosityRange.Range.Value;

        if (Luminosity < LowerThird)
        {
            return ESpectralType::Ib;
        }
        else if (Luminosity < MiddleThird)
        {
            return ESpectralType::Iab;
        }
        else
        {
            return ESpectralType::Ia;
        }
    }
   // case EStellarClass::Unknown:
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unexpected StellarType value"));
        return ESpectralType::Unknown; // Return a default value
    }
}

int UStarGenerator::CalculateSpectralSubclass(double StarTemperature, ESpectralClass SpectralClass)
{
    if (SpectralClass == ESpectralClass::Unknown)
    {
		return 0;
	}

    const TTuple<double, double>& TempRange = StarTypeTemperatureRanges[SpectralClass];

    // Linear interpolation between the temperature range of the spectral class
    double TempRatio = (StarTemperature - TempRange.Get<0>()) / (TempRange.Get<1>() - TempRange.Get<0>());

    // Multiply by 10 to get subclass (0-9)
    int Subclass = 9 - FMath::RoundToInt(TempRatio * 10.0);

    // Ensure Subclass is within the valid range
    if (SpectralClass == ESpectralClass::O && Subclass < 2)
    {
        Subclass = 2;
    }
    else if (Subclass < 0)
    {
        Subclass = 0;
    }
    else if (Subclass > 9)
    {
        Subclass = 9;
    }

    return Subclass;
}

double UStarGenerator::CalculateLuminosityByMass(double Mass)
{
    double Luminosity;

    if (Mass < 0.43)
    {
        // Low mass stars (less than about 0.43 solar masses)
        Luminosity = pow(Mass, 2.3);
    }
    else if (Mass >= 0.43 && Mass < 2)
    {
        // Solar-type stars (0.43 to about 2 solar masses)
        Luminosity = pow(Mass, 4);
    }
    else if (Mass >= 2 && Mass < 20)
    {
        // Intermediate-mass stars (2 to about 20 solar masses)
        Luminosity = pow(Mass, 3.5);
    }
    else
    {
        // High mass stars (greater than about 20 solar masses)
        Luminosity = pow(Mass, 5.5);
    }

    if (Luminosity > 100000.0)
    {
        // Set a maximum luminosity
        Luminosity = FMath::FRandRange(100000.0, 200000.0);
    }

    return Luminosity;
}

double UStarGenerator::RandomFromRange(TTuple<double, double> Range)
{
    return FMath::RandRange(Range.Key, Range.Value);
}

double UStarGenerator::RandomMass(ESpectralClass SpectralClass)
{
    auto Range = MainSequenceMassRanges[SpectralClass];
    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
}

double UStarGenerator::RandomRadius(ESpectralClass SpectralClass)
{
    auto Range = MainSequenceRadiusRanges[SpectralClass];
    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
}

double UStarGenerator::CalculateLuminosity(double Radius, double SurfaceTemperature)
{
    const double StefanBoltzmannConstant = 5.67e-8;  // in W/(m^2 K^4)
    const double SolarRadius = 6.96e8;  // in meters
    const double SolarLuminosity = 3.828e26;  // in watts

    double Luminosity = 4.0 * PI * pow(Radius * SolarRadius, 2) * StefanBoltzmannConstant * pow(SurfaceTemperature, 4);

    // Convert to solar luminosity units
    Luminosity /= SolarLuminosity;

    // Check for anomalies
    if (Luminosity < 0.0001) 
    {   
        // Set a minimum luminosity
        Luminosity = FMath::FRandRange(0.00001, 0.000001);
    }  
    if (Luminosity > 100000.0) 
    { 
        // Set a maximum luminosity
        //Luminosity =  FMath::FRandRange(100000.0, 200000.0);
    }  
    return Luminosity;
}

double UStarGenerator::CalculateSurfaceTemperature(double Luminosity, double Radius)
{
    const double StefanBoltzmannConstant = 5.67e-8;
    const double SolarLuminosity = 3.828e26; // in Watts
    const double SolarRadius = 6.9634e8; // in meters

    // Convert solar units to SI units
    Luminosity *= SolarLuminosity;
    Radius *= SolarRadius;
    double SurfaceTemperature = pow(Luminosity / (4.0 * PI * Radius * Radius * StefanBoltzmannConstant), 0.25);

    return SurfaceTemperature;
}

/*
ƒл€ начала, давайте определим, какие спектральные классы будут соответствовать каждому классу звезды(EStellarClass).
ƒл€ этого мы можем использовать следующую таблицу :

EStellarClass	ESpectralClass
Dwarf Star	M, K, G(можно добавить и другие классы, если вы хотите иметь карликовые звезды различных спектральных классов)
Main Sequence Star	O, B, A, F, G, K, M
Giant Star	K, M
Supergiant Star	O, B, A, F, G, K, M

»спользу€ эту информацию, мы можем начать писать функцию, котора€ будет случайно выбирать спектральный класс дл€ заданного класса звезды.

EStellarClass	ESpectralClass
HyperGiant	    O, B, A, F, G, K
SuperGiant	    O, B, A, F, G, K, M
BrightGiant	    O, B, A, F, G, K, M
Giant	        O, B, A, F, G, K, M
SubGiant	    O, B, A, F, G, K
MainSequence	O, B, A, F, G, K, M
SubDwarf	    O, B, A, F, G, K
WhiteDwarf	    WD 
BrownDwarf	    L, T, Y 

*/

ESpectralClass UStarGenerator::ChooseSpectralClassByStellarClass(EStellarClass StellarClass)
{
    // ћассивы спектральных классов дл€ каждого стелл€рного класса
    // Spectral Classes O, B, A, F, G, K, M
    const TArray<ESpectralClass> Spectral_OM = { ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G, ESpectralClass::K, ESpectralClass::M };
    // Spectral Classes O, B, A, F, G, K
    const TArray<ESpectralClass> Spectral_OK = { ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G, ESpectralClass::K };
    // Spectral Classes L, T, Y
    const TArray<ESpectralClass> Spectral_LY = { ESpectralClass::L, ESpectralClass::T, ESpectralClass::Y };

    const TArray<int> Weights_OM = { 3, 5, 22, 30, 20, 10, 3 }; // Weights for O, B, A, F, G, K, M
    const TArray<int> Weights_OK = { 3, 13, 22, 30, 20, 12 }; // Weights for O, B, A, F, G, K
    const TArray<int> Weights_LY = { 20, 50, 30 }; // Weights for L, T, Y

    // ќпределение выборки и весов
    const TArray<ESpectralClass>* SpectralArray;
    const TArray<int>* WeightsArray;

    switch (StellarClass)
    {
    case EStellarClass::HyperGiant:
    case EStellarClass::SuperGiant:
    case EStellarClass::BrightGiant:
    case EStellarClass::Giant:
    case EStellarClass::MainSequence:
        SpectralArray = &Spectral_OM;
        WeightsArray = &Weights_OM;
        break;
    case EStellarClass::SubGiant:
    case EStellarClass::SubDwarf:
    case EStellarClass::WhiteDwarf:
        SpectralArray = &Spectral_OK;
        WeightsArray = &Weights_OK;
        break;
    case EStellarClass::BrownDwarf:
        SpectralArray = &Spectral_LY;
        WeightsArray = &Weights_LY;
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown StellarClass!"));
        return ESpectralClass::Unknown;  // ¬озвращает O класс по умолчанию или можно выбрать другой класс
    }

    // —оздание кумул€тивного массива весов
    TArray<int> CumulativeWeights;
    int TotalWeight = 0;
    for (int i = 0; i < WeightsArray->Num(); ++i)
    {
        TotalWeight += (*WeightsArray)[i];
        CumulativeWeights.Add(TotalWeight);
    }

    // √енераци€ случайного числа
    int RandWeight = FMath::RandRange(0, TotalWeight - 1);

    // Ќахождение соответствующего спектрального класса
    for (int i = 0; i < CumulativeWeights.Num(); ++i)
    {
        if (RandWeight < CumulativeWeights[i])
        {
            return (*SpectralArray)[i];
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to choose spectral class!"));
    return ESpectralClass::M;
}

ESpectralClass UStarGenerator::DetermineSpectralClassByTemperature(EStellarClass StarType, double Temperature)
{
    switch (StarType) {
    /*case EStarStellarClass::WhiteDwarf:
        return EStarSpectralClass::WD;*/
    case EStellarClass::Neutron:
        return ESpectralClass::NS;
    case EStellarClass::BlackHole:
        return ESpectralClass::BH;
    case EStellarClass::BrownDwarf:
        if (Temperature > 1300) return ESpectralClass::L;
        else if (Temperature > 700) return ESpectralClass::T;
        else return ESpectralClass::Y;
    default:  // ƒл€ всех остальных типов звезд мы провер€ем температуру
        if (Temperature > 30000) return ESpectralClass::O;
        else if (Temperature > 10000) return ESpectralClass::B;
        else if (Temperature > 7500) return ESpectralClass::A;
        else if (Temperature > 6000) return ESpectralClass::F;
        else if (Temperature > 5000) return ESpectralClass::G;
        else if (Temperature > 3500) return ESpectralClass::K;
        else return ESpectralClass::M;
    }
}

double UStarGenerator::GenerateRandomTemperatureBySpectralClass(ESpectralClass SpectralClass)
{
    TTuple<double, double> TemperatureRange = StarTypeTemperatureRanges[SpectralClass];
    return FMath::RandRange(TemperatureRange.Get<0>(), TemperatureRange.Get<1>());
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "MoonGenerator.h"
#include "Planet.h"

UMoonGenerator::UMoonGenerator()
{
}

FMoonGenerationModel UMoonGenerator::GenerateMoonModel()
{
	return FMoonGenerationModel();
}

FMoonGenerationModel UMoonGenerator::GenerateRandomMoonModel()
{
	
	return FMoonGenerationModel();
}

void UMoonGenerator::ApplyModel(AMoon* Moon, FMoonGenerationModel MoonGenerationModel)
{

    Moon->SetMoonType(MoonGenerationModel.Type);
    Moon->SetMass(MoonGenerationModel.Mass);
    Moon->SetRadius(MoonGenerationModel.Radius); 
    Moon->SetMoonDensity(MoonGenerationModel.MoonDensity); 
    Moon->SetMoonGravity(MoonGenerationModel.MoonGravity); 
    Moon->SetOrbitDistance(MoonGenerationModel.OrbitDistance);
    /*Moon->MoonEscapeVelocity = MoonGenerationModel.MoonEscapeVelocity;
    Moon->MoonOrbitalPeriod = MoonGenerationModel.MoonOrbitalPeriod;*/
    

}


void UMoonGenerator::ConnectMoonWithPlanet(AMoon* NewMoon, APlanet* NewPlanet)
{
    NewPlanet->AddMoon(NewMoon);
	NewMoon->AttachToActor(NewPlanet, FAttachmentTransformRules::KeepRelativeTransform);
	NewMoon->SetParentPlanet(NewPlanet);
}

double UMoonGenerator::CalculateGravitationalForce(double MassPlanet, double MassMoon, double Distance)
{
    const double G = 6.67430e-11; // Гравитационная постоянная в м^3 kg^-1 s^-2
    const double MassEarth = 5.972e24; // Масса Земли в килограммах

    // Преобразование масс из масс Земли в килограммы
    double MassPlanetKg = MassPlanet * MassEarth;
    double MassMoonKg = MassMoon * MassEarth;

    // Преобразование расстояния из километров в метры
    double DistanceM = Distance * 1000;

    // Вычисление силы притяжения по формуле Ньютона
    double Force = G * (MassPlanetKg * MassMoonKg) / pow(DistanceM, 2);

    return Force;
}

double UMoonGenerator::CalculateRandomMoonDensity(EMoonType MoonType)
{
    
    TPair<double, double> DensityRange = MoonDensityRanges[MoonType];
    
    /*if (densityRange. == nullptr)
    {
        return 1.1;
    }*/

    
    return FMath::RandRange(DensityRange.Key, DensityRange.Value);

}



EMoonType UMoonGenerator::GenerateMoonType(FPlanetGenerationModel PlanetModel)
{
    
    EPlanetaryZoneType PlanetZone = PlanetModel.PlanetZone;


    TMap<EMoonType, float> PlanetMoonTypeProbabilities;


    if (PlanetZone == EPlanetaryZoneType::HotZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Iron, 0.5},
			{EMoonType::Volcanic, 0.2},
			{EMoonType::Desert, 0.3},
		};
	}
    else if (PlanetZone == EPlanetaryZoneType::WarmZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Iron, 0.1},
			{EMoonType::Volcanic, 0.2},
			{EMoonType::Gas, 0.1},
			{EMoonType::Continental, 0.1},
			{EMoonType::Desert, 0.5},
			{EMoonType::TidallyLocked, 0.2},
			{EMoonType::Peculiar, 0.2}
		};
	}
    else if (PlanetZone == EPlanetaryZoneType::HabitableZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Iron, 0.1},
			{EMoonType::Volcanic, 0.2},
			{EMoonType::Gas, 0.1},
			{EMoonType::Continental, 0.3},
			{EMoonType::Desert, 0.5},
			{EMoonType::TidallyLocked, 0.3},
			{EMoonType::Peculiar, 0.3}
		};
	}
    else if (PlanetZone == EPlanetaryZoneType::ColdZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Icy, 0.3},
			{EMoonType::Gas, 0.1},
			{EMoonType::Continental, 0.1},
		};
    }
    else if (PlanetZone == EPlanetaryZoneType::IceZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Icy, 0.6},
			{EMoonType::Gas, 0.1},
		};
    }
    else if (PlanetZone == EPlanetaryZoneType::GasGiantsZone)
    {
        PlanetMoonTypeProbabilities = 
        {
			{EMoonType::Rocky, 0.5},
			{EMoonType::Icy, 0.6},
			{EMoonType::Gas, 0.3},
			{EMoonType::Continental, 0.1},
			{EMoonType::Volcanic, 0.2},
			{EMoonType::Iron, 0.2},
			//{EMoonType::CapturedAsteroid, 0.2},
		};
    }
    else
    {
        PlanetMoonTypeProbabilities =
        {
            {EMoonType::Rocky, 0.5},
            {EMoonType::Icy, 0.1},
            {EMoonType::Iron, 0.1},
        };
    }



    // Подсчитываем общий вес
    float TotalWeight = 0.0f;
    for (auto const& Pair : PlanetMoonTypeProbabilities) {
        TotalWeight += Pair.Value;
    }

    // Генерируем случайное число в диапазоне от 0 до TotalWeight
    float RandomValue = FMath::FRand() * TotalWeight;

    EMoonType ChosenMoonType{};
    for (auto const& Pair : PlanetMoonTypeProbabilities) {
        if (RandomValue < Pair.Value) {
            ChosenMoonType = Pair.Key;
            break;
        }
        RandomValue -= Pair.Value;
    }

    return ChosenMoonType;



    
    //// Маппинг типов планет и диапазонов вероятностей для типов лун
    //TMap<EPlanetType, TMap<EMoonType, FFloatRange>> PlanetMoonTypeProbabilities;

    ////// TODO: Заполните маппинг PlanetMoonTypeProbabilities на основе вашего знания о том, какие типы лун наиболее вероятно появляются у каждого типа планеты

    ////// Получаем диапазон вероятностей для данного типа планеты
    //TMap<EMoonType, FFloatRange> MoonTypeProbabilities = PlanetMoonTypeProbabilities[PlanetModel.PlanetType];

    ////// Генерируем случайное число от 0 до 1
    //float RandNum = FMath::FRand();

    ////// Ищем соответствующий тип луны
    //for (const auto& Entry : MoonTypeProbabilities)
    //{
    //    if (Entry.Value.Contains(RandNum))
    //    {
    //        return Entry.Key;
    //    }
    //}

    // Если ничего не найдено, возвращаем Rocky по умолчанию
    return EMoonType::Rocky;
}

double UMoonGenerator::CalculateMoonMass(EMoonType MoonType)
{





    //// Генерируем случайное число
    //float randomValue = FMath::FRand();

    //// Скалистые, железные и вулканические луны обычно имеют большую плотность
    //if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    //{
    //    return randomValue * 1e23; // масса в килограммах
    //}

    //// Ледяные и океанические луны обычно имеют меньшую плотность
    //else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    //{
    //    return randomValue * 1e22; // масса в килограммах
    //}

    //// Газовые луны и захваченные астероиды могут иметь различную массу
    //else
    //{
    //    return randomValue * 1e21; // масса в килограммах
    //}

    return 0;
}


double UMoonGenerator::CalculateRandomMoonMass()
{
    // Convert the range from a logarithmic scale to a linear scale
    double minMass = FMath::Pow(10, -4);
    double maxMass = FMath::Pow(10, -1.5);

    // Generate a random mass within the range
    double moonMass = FMath::RandRange(minMass, maxMass);

    return moonMass;
}


double UMoonGenerator::CalculateMoonRadius(double MoonDensity, double MoonMass)
{
    return FMath::Pow((3.0 * MoonMass) / (4.0 * PI * MoonDensity), 1.0 / 3.0);


    //return 0;



    // Генерируем случайное число
    //float randomValue = FMath::FRand();

    //// Скалистые, железные и вулканические луны обычно имеют большой радиус
    //if (MoonType == EMoonType::Rocky || MoonType == EMoonType::Iron || MoonType == EMoonType::Volcanic)
    //{
    //    return randomValue * 2000; // радиус в километрах
    //}

    //// Ледяные и океанические луны обычно имеют меньший радиус
    //else if (MoonType == EMoonType::Icy || MoonType == EMoonType::Ocean)
    //{
    //    return randomValue * 1500; // радиус в километрах
    //}

    //// Газовые луны и захваченные астероиды могут иметь различный радиус
    //else
    //{
    //    return randomValue * 1000; // радиус в километрах
    //}
}
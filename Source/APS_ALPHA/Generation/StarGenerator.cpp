#include "StarGenerator.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

UStarGenerator::UStarGenerator()
{
	FDateTime Time = FDateTime::UtcNow();
	RandomStream = FRandomStream(Time.ToUnixTimestamp());
}

void UStarGenerator::ApplySpectralMaterial(AStar* NewStar, TSharedPtr<FStarModel> StarModel)
{
	// Получить материал с меша звезды
	UMaterialInterface* Material = NewStar->StarMesh->GetMaterial(0);

	// Попытаться привести материал к динамическому экземпляру
	UMaterialInstanceDynamic* StarDynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);

	if (StarDynamicMaterial == nullptr)
	{
		// Если это не динамический материал, создайте новый динамический экземпляр
		StarDynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
	}

	// Установите скалярный параметр
	FName ParameterName1 = "Multiplier";
	float MultiplierValue = 500 * StarModel->Luminosity;

	// Установите векторный параметр
	FName ParameterName2 = "Color";
	FLinearColor ColorValue = GetStarColor(StarModel->SpectralClass, StarModel->SpectralSubclass);
	StarDynamicMaterial->SetVectorParameterValue(ParameterName2, ColorValue);

	// Примените динамический материал к вашему объекту
	NewStar->StarMesh->SetMaterial(0, StarDynamicMaterial);
}

/*TUniquePtr<FStarModel>*/
void UStarGenerator::GenerateStarModelByProbability(TSharedPtr<FStarModel> StarModel,
                                                    TSharedPtr<FStarClusterModel> FStarClusterModel)
{
	//FStarModel* StarModel = new FStarModel();
	//TUniquePtr<FStarModel> StarModel = MakeUnique<FStarModel>();

	EStellarType StellarType;
	if (StarClusterPopulationWeights.Contains(FStarClusterModel->StarClusterPopulation))
	{
		TMap<EStellarType, float> StellarTypeMap = StarClusterPopulationWeights[FStarClusterModel->
			StarClusterPopulation];
		StellarType = GenerateStellarTypeByRandomWeights(StellarTypeMap);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StarClusterPopulationWeights doesn't contain %s"),
		       *UEnum::GetValueAsString(FStarClusterModel->StarClusterPopulation));
		StellarType = EStellarType::Unknown;
	}
	StarModel->StellarType = StellarType; //GenerateStarClassByRandomWeights();

	ESpectralClass SpectralClass;
	if (StarClusterCompositionWeights.Contains(FStarClusterModel->StarClusterComposition))
	{
		TMap<ESpectralClass, int> SpectralClassMap = StarClusterCompositionWeights[FStarClusterModel->
			StarClusterComposition];
		SpectralClass = GenerateSpectralClassByProbability(SpectralClassMap);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StarClusterCompositionWeights doesn't contain %s"),
		       *UEnum::GetValueAsString(FStarClusterModel->StarClusterComposition));
		SpectralClass = ESpectralClass::Unknown;
	}
	StarModel->SpectralClass = SpectralClass;
	//GenerateSpectralClassByProbability()//ChooseSpectralClassByStellarClass(StarModel.StellarClass);

	//return 
	GenerateStarModel(StarModel);
}


// Закон Вина
double UStarGenerator::WienLaw(double temperature)
{
	const double b = 2.897771955e-3; // Константа Вина, м*К
	return b / temperature;
}

FLinearColor UStarGenerator::WavelengthToRGB(double wavelength)
{
	double r, g, b;

	if (wavelength >= 380.0 && wavelength < 440.0)
	{
		r = -(wavelength - 440.0) / (440.0 - 380.0);
		g = 0.0;
		b = 1.0;
	}
	else if (wavelength >= 440.0 && wavelength < 490.0)
	{
		r = 0.0;
		g = (wavelength - 440.0) / (490.0 - 440.0);
		b = 1.0;
	}
	else if (wavelength >= 490.0 && wavelength < 510.0)
	{
		r = 0.0;
		g = 1.0;
		b = -(wavelength - 510.0) / (510.0 - 490.0);
	}
	else if (wavelength >= 510.0 && wavelength < 580.0)
	{
		r = (wavelength - 510.0) / (580.0 - 510.0);
		g = 1.0;
		b = 0.0;
	}
	else if (wavelength >= 580.0 && wavelength < 645.0)
	{
		r = 1.0;
		g = -(wavelength - 645.0) / (645.0 - 580.0);
		b = 0.0;
	}
	else if (wavelength >= 645.0 && wavelength <= 780.0)
	{
		r = 1.0;
		g = 0.0;
		b = 0.0;
	}
	else
	{
		r = 0.0;
		g = 0.0;
		b = 0.0;
	}

	// Переводим волновую длину в нанометрах в цветовую температуру в Кельвинах
	double s = 1.0;
	if (wavelength > 700.0)
	{
		s = 0.3 + 0.7 * (780.0 - wavelength) / (780.0 - 700.0);
	}
	else if (wavelength < 420.0)
	{
		s = 0.3 + 0.7 * (wavelength - 380.0) / (420.0 - 380.0);
	}

	return FLinearColor(r * s, g * s, b * s, 1.0);
}

FLinearColor UStarGenerator::TemperatureToColor(double temperature)
{
	double wavelength = WienLaw(temperature);
	return WavelengthToRGB(wavelength);
}

FLinearColor UStarGenerator::GetStarColor(ESpectralClass SpectralClass, int Subclass)
{
	static const TMap<ESpectralClass, FLinearColor> BaseColors =
	{
		{ESpectralClass::O, FLinearColor(0.5, 0.5, 1)}, // Синий
		{ESpectralClass::B, FLinearColor(0.6, 0.6, 1)}, // Голубой
		{ESpectralClass::A, FLinearColor(1, 1, 1)}, // Белый
		{ESpectralClass::F, FLinearColor(1, 1, 0.8)}, // Желто-белый
		{ESpectralClass::G, FLinearColor(1, 1, 0.6)}, // Желтый
		{ESpectralClass::K, FLinearColor(1, 0.6, 0.3)}, // Оранжевый
		{ESpectralClass::M, FLinearColor(1, 0.3, 0.3)}, // Красный
		{ESpectralClass::L, FLinearColor(0.5, 0.3, 0.1)}, // Бурый (для Brown Dwarf)
		{ESpectralClass::T, FLinearColor(0.6, 0.3, 0.1)}, // Коричневый (для Tauri Dwarf)
		{ESpectralClass::Y, FLinearColor(0.6, 0.5, 0.1)}, // Темно-желтый (для Cool Brown Dwarf)
		{ESpectralClass::NS, FLinearColor(0.9, 0.9, 1)}, // Бело-голубой (для Neutron Star)
		{ESpectralClass::PS, FLinearColor(1, 0.9, 0.6)}, // Желтоватый (для Proto Star)
		{ESpectralClass::BH, FLinearColor(0, 0, 0)}, // Черный (для Black Hole)
		{ESpectralClass::Unknown, FLinearColor(0.5, 0.5, 0.5)} // Серый (для Unknown)
	};

	// Вычисляем коэффициент интерполяции
	float InterpFactor = static_cast<float>(Subclass) / 10.0f;

	// Получаем базовый цвет для данного и следующего спектрального класса
	FLinearColor BaseColor = BaseColors[SpectralClass]; /// CRASH PIE
	FLinearColor NextBaseColor;
	if (SpectralClass != ESpectralClass::Unknown) // Проверяем, что текущий класс не последний
	{
		NextBaseColor = BaseColors[static_cast<ESpectralClass>(static_cast<int>(SpectralClass) + 1)];
	}
	else
	{
		NextBaseColor = BaseColor; // Если текущий класс уже последний, то следующий цвет просто равен текущему
	}

	// Интерполируем между двумя цветами
	FLinearColor starColor = FLinearColor::LerpUsingHSV(BaseColor, NextBaseColor, InterpFactor);

	return starColor;
}

FLinearColor UStarGenerator::TemperatureToRGB(float temperature)
{
	float r, g, b;

	// Обычно температура ниже 1000K не учитывается, но вы можете настроить этот порог как вам удобнее.
	if (temperature < 1000)
		temperature = 1000;

	// Нормализовать температуру (диапазон 0-1)
	temperature /= 10000;

	if (temperature <= 0.66)
	{
		r = 1;
		g = temperature;
		g = 0.39008157876902 * pow(g, -0.93412075736856); // Корректировка гаммы для зеленого
	}
	else
	{
		r = 0.98866243976127 * pow(temperature - 0.66, -0.6841316279095123); // Корректировка гаммы для красного
		g = 1 - r;
	}

	if (temperature >= 0.5)
	{
		b = 1;
	}
	else if (temperature >= 0.25)
	{
		b = 0.94279106151537 * pow((0.5 - temperature) * 2, -0.70176690865074); // Корректировка гаммы для синего
	}
	else
	{
		b = 0;
	}

	return FLinearColor(r, g, b);

	/*double Red, Green, Blue;

	if (Temperature <= 1000.0)
	{
	    Red = 0.647;
	    Green = 0.498;
	    Blue = 1.000;
	}
	else if (Temperature <= 3500.0)
	{
	    Red = 1.000;
	    Green = 0.706 - (Temperature - 2000.0) / 1500.0 * 0.206;
	    Blue = 0.306;
	}
	else if (Temperature <= 6000.0)
	{
	    Red = 1.000 - (Temperature - 3500.0) / 2500.0 * 0.454;
	    Green = 0.706 - (Temperature - 3500.0) / 2500.0 * 0.706;
	    Blue = 0.000;
	}
	else if (Temperature <= 9000.0)
	{
	    Red = 0.546 - (Temperature - 6000.0) / 3000.0 * 0.546;
	    Green = 0.000;
	    Blue = 0.000;
	}
	else
	{
	    Red = 0.000;
	    Green = 0.000;
	    Blue = 0.000;
	}

	return FLinearColor(Red, Green, Blue);*/
}

void UStarGenerator::ApplyModel(AStar* NewStar, TSharedPtr<FStarModel> StarModel) // NewStar, StarModel
{
	NewStar->SetLuminosity(StarModel->Luminosity);
	NewStar->SetSurfaceTemperature(StarModel->SurfaceTemperature);
	NewStar->SetStarType(StarModel->StellarType);
	NewStar->SetStarSpectralClass(StarModel->SpectralClass);

	NewStar->SetMass(StarModel->Mass);
	NewStar->SetRadius(StarModel->Radius);
	NewStar->SetAge(StarModel->Age);

	NewStar->SetSpectralSubclass(StarModel->SpectralSubclass);
	NewStar->SetStarSpectralType(StarModel->SpectralType);
	NewStar->SetFullSpectralClass(StarModel->FullSpectralClass);
	NewStar->SetFullSpectralName(StarModel->FullSpectralName);
	NewStar->RadiusKM = StarModel->RadiusKM;
	NewStar->AffectionRadiusKM = StarModel->RadiusKM;
	NewStar->MinOrbit = StarModel->MinOrbit;
	NewStar->MaxOrbit = StarModel->MaxOrbit;
}

void UStarGenerator::GenerateStarModel(TSharedPtr<FStarModel> StarModel)
{
	if (StarModel->StellarType == EStellarType::MainSequence)
	{
		// Get the mass from the spectral class
		double Mass = RandomMass(StarModel->SpectralClass);

		// Calculate the radius and luminosity using mass-radius-luminosity relation
		double Radius = pow(Mass, 0.8); // Radius  Mass^0.8
		double Luminosity = CalculateLuminosityByMass(Mass);
		double SurfaceTemperature = RandomFromRange(StarTypeTemperatureRanges[StarModel->SpectralClass]);

		// Generate a star model
		StarModel->Mass = Mass;
		StarModel->Radius = Radius;
		StarModel->RadiusKM = Radius * 6.957e+5;
		StarModel->Luminosity = Luminosity;
		StarModel->SurfaceTemperature = SurfaceTemperature;
		StarModel->Age = CalculateMainSequenceStarAge(Mass);
	}
	else
	{
		if (!StarAttributeRanges.Contains(StarModel->StellarType))
		{
			UE_LOG(LogTemp, Error, TEXT("Unknown stellar class: %d"), static_cast<int>(StarModel->SpectralClass));
			return;
		}

		FStarAttributeRanges& AttributeRanges = StarAttributeRanges[StarModel->StellarType];
		StarModel->Mass = FMath::RandRange(AttributeRanges.Mass.Range.Get<0>(), AttributeRanges.Mass.Range.Get<1>());
		/// TODO: Refactor with Key/Val
		StarModel->Radius = FMath::RandRange(AttributeRanges.Radius.Range.Get<0>(),
		                                     AttributeRanges.Radius.Range.Get<1>());
		StarModel->SurfaceTemperature = GenerateRandomTemperatureBySpectralClass(StarModel->SpectralClass);
		StarModel->Luminosity = CalculateLuminosity(StarModel->Radius, StarModel->SurfaceTemperature);
		StarModel->Age = CalculateNonMainSequenceStarAge(StarModel->Mass);
	}

	StarModel->SpectralSubclass = CalculateSpectralSubclass(StarModel->SurfaceTemperature, StarModel->SpectralClass);
	StarModel->SpectralType = CalculateSpectralType(StarModel->StellarType, StarModel->Luminosity);
}


void UStarGenerator::GenerateRandomStarModel(TSharedPtr<FStarModel> StarModel)
{
	StarModel->StellarType = GenerateStellarTypeByRandomWeights();
	StarModel->SpectralClass = ChooseSpectralClassByStellarClass(StarModel->StellarType);
	GenerateStarModel(StarModel);
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
	// Применяем формулу T = M^(-2.5)
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

ESpectralClass UStarGenerator::GenerateSpectralClassByProbability(
	TMap<ESpectralClass, int> StarSpectralClassProbabilities)
{
	int TotalWeight = 0;
	for (auto const& Pair : StarSpectralClassProbabilities)
	{
		TotalWeight += Pair.Value;
	}
	int RandomValue = FMath::RandRange(0, TotalWeight - 1);
	ESpectralClass ChosenSpectralClass{};
	for (auto const& pair : StarSpectralClassProbabilities)
	{
		if (RandomValue < pair.Value)
		{
			ChosenSpectralClass = pair.Key;
			break;
		}
		RandomValue -= pair.Value;
	}
	return ChosenSpectralClass;
}

EStellarType UStarGenerator::GenerateStellarTypeByRandomWeights(TMap<EStellarType, float> StarTypeProbabilities)
{
	float TotalWeight = 0.f;
	for (auto const& Pair : StarTypeProbabilities)
	{
		TotalWeight += Pair.Value;
	}
	float RandomValue = FMath::RandRange(0.f, TotalWeight - 1.f);
	EStellarType ChosenStellarClass{};
	for (auto const& pair : StarTypeProbabilities)
	{
		if (RandomValue < pair.Value)
		{
			ChosenStellarClass = pair.Key;
			break;
		}
		RandomValue -= pair.Value;
	}
	return ChosenStellarClass;
}

EStellarType UStarGenerator::GenerateStellarTypeByRandomWeights()
{
	int TotalWeight = 0;
	for (auto const& Pair : DefaultStarTypeWeights)
	{
		TotalWeight += Pair.Value;
	}
	int RandomValue = FMath::RandRange(0, TotalWeight - 1);
	EStellarType ChosenStellarClass{};
	for (auto const& pair : DefaultStarTypeWeights)
	{
		if (RandomValue < pair.Value)
		{
			ChosenStellarClass = pair.Key;
			break;
		}
		RandomValue -= pair.Value;
	}
	return ChosenStellarClass;
}

double UStarGenerator::CalculateEmission(float starLuminosity)
{
	if (starLuminosity < MinStarLuminosity)
		return MinStarLuminosity;
	else if (starLuminosity > MaxStarLuminosity)
		return MaxStarLuminosity;
	else
		return starLuminosity;
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

FName UStarGenerator::GenerateFullSpectralClass(const TUniquePtr<FStarModel> StarModel)
{
	if (StarModel->SpectralClass == ESpectralClass::Unknown)
	{
		return FName("Unknown");
	}

	FString SpectralClassString = StaticEnum<ESpectralClass>()->GetValueAsString(StarModel->SpectralClass);
	SpectralClassString.RemoveFromStart("ESpectralClass::"); // Remove prefix

	FString SpectralTypeString = StaticEnum<ESpectralType>()->GetValueAsString(StarModel->SpectralType);
	SpectralTypeString.RemoveFromStart("ESpectralType::"); // Remove prefix

	return FName(
		*(SpectralClassString + FString::Printf(TEXT("%d"), StarModel->SpectralSubclass) + SpectralTypeString));
}

FName UStarGenerator::GenerateFullSpectralName(const TUniquePtr<FStarModel> StarModel)
{
	if (StarModel->SpectralClass == ESpectralClass::Unknown)
	{
		return FName("Unknown");
	}

	FString SpectralClassColor;
	if (SpectralClassColorMap.Contains(StarModel->SpectralClass))
	{
		SpectralClassColor = SpectralClassColorMap[StarModel->SpectralClass];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SpectralClassColorMap doesn't contain %s"),
		       *UEnum::GetValueAsString(StarModel->SpectralClass));
		SpectralClassColor = "UnknownSpectralClassColor";
		// Возвращаем значение по умолчанию или обрабатываем ошибку иначе
	}

	FString SpectralTypeDescription;
	if (SpectralTypeDescriptionMap.Contains(StarModel->SpectralType))
	{
		SpectralTypeDescription = SpectralTypeDescriptionMap[StarModel->SpectralType];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SpectralTypeDescriptionMap doesn't contain %s"),
		       *UEnum::GetValueAsString(StarModel->SpectralType));
		SpectralTypeDescription = "UnknownSpectralTypeDescription";
		// Возвращаем значение по умолчанию или обрабатываем ошибку иначе
	}

	FName SpectralClassColorName(*SpectralClassColor);
	FName SpectralTypeDescriptionName(*SpectralTypeDescription);

	return FName(*(SpectralClassColorName.ToString() + " " + SpectralTypeDescriptionName.ToString()));
}

ESpectralType UStarGenerator::CalculateSpectralType(EStellarType StellarType, double Luminosity)
{
	switch (StellarType)
	{
	case EStellarType::HyperGiant:
		return ESpectralType::O;
	case EStellarType::BrightGiant:
		return ESpectralType::II;
	case EStellarType::Giant:
		return ESpectralType::III;
	case EStellarType::SubGiant:
		return ESpectralType::IV;
	case EStellarType::MainSequence:
		return ESpectralType::V;
	case EStellarType::SubDwarf:
		return ESpectralType::VI;
	case EStellarType::WhiteDwarf:
		return ESpectralType::VII;
	case EStellarType::BrownDwarf:
		return ESpectralType::VIII;
	case EStellarType::SuperGiant:
		{
			FStarAttributeRanges& AttributeRanges = StarAttributeRanges[StellarType];
			FLuminosityRange LuminosityRange = AttributeRanges.Luminosity; // получаем диапазон светимости

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
	if (MainSequenceMassRanges.Contains(SpectralClass))
	{
		auto Range = MainSequenceMassRanges[SpectralClass];
		return RandomStream.FRandRange(Range.Get<0>(), Range.Get<1>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainSequenceMassRanges doesn't contain %s"),
		       *UEnum::GetValueAsString(SpectralClass));
		return 0; // Возвращаем некоторое значение по умолчанию или обрабатываем ошибку иначе
	}
	//if (MainSequenceMassRanges.Contains(SpectralClass))
	//{
	//    auto Range = MainSequenceMassRanges[SpectralClass];
	//    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
	//}
	//else
	//{
	//    UE_LOG(LogTemp, Warning, TEXT("MainSequenceMassRanges doesn't contain %s"), *UEnum::GetValueAsString(SpectralClass));
	//    return 0; // Возвращаем некоторое значение по умолчанию или обрабатываем ошибку иначе
	//}
}

double UStarGenerator::RandomRadius(ESpectralClass SpectralClass)
{
	auto Range = MainSequenceRadiusRanges[SpectralClass];
	return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
}

double UStarGenerator::CalculateLuminosity(double Radius, double SurfaceTemperature)
{
	const double StefanBoltzmannConstant = 5.67e-8; // in W/(m^2 K^4)
	const double SolarRadius = 6.96e8; // in meters
	const double SolarLuminosity = 3.828e26; // in watts
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
Для начала, давайте определим, какие спектральные классы будут соответствовать каждому классу звезды(EStellarClass).
Для этого мы можем использовать следующую таблицу :

EStellarClass	ESpectralClass
Dwarf Star	M, K, G(можно добавить и другие классы, если вы хотите иметь карликовые звезды различных спектральных классов)
Main Sequence Star	O, B, A, F, G, K, M
Giant Star	K, M
Supergiant Star	O, B, A, F, G, K, M

Используя эту информацию, мы можем начать писать функцию, которая будет случайно выбирать спектральный класс для заданного класса звезды.

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

ESpectralClass UStarGenerator::ChooseSpectralClassByStellarClass(EStellarType StellarClass)
{
	// Массивы спектральных классов для каждого стеллярного класса
	// Spectral Classes O, B, A, F, G, K, M
	const TArray<ESpectralClass> Spectral_OM = {
		ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G,
		ESpectralClass::K, ESpectralClass::M
	};
	// Spectral Classes O, B, A, F, G, K
	const TArray<ESpectralClass> Spectral_OK = {
		ESpectralClass::O, ESpectralClass::B, ESpectralClass::A, ESpectralClass::F, ESpectralClass::G, ESpectralClass::K
	};
	// Spectral Classes L, T, Y
	const TArray<ESpectralClass> Spectral_LY = {ESpectralClass::L, ESpectralClass::T, ESpectralClass::Y};

	const TArray<int> Weights_OM = {1, 2, 10, 20, 30, 40, 50}; // best for direct systems
	//const TArray<int> Weights_OM = { 1, 2, 6, 30, 76, 121, 764 }; // best for star clusters
	//const TArray<int> Weights_OM = { 3, 5, 22, 30, 20, 10, 30 }; // Weights for O, B, A, F, G, K, M
	const TArray<int> Weights_OK = {3, 13, 22, 30, 20, 12}; // Weights for O, B, A, F, G, K
	const TArray<int> Weights_LY = {20, 50, 30}; // Weights for L, T, Y

	// Определение выборки и весов
	const TArray<ESpectralClass>* SpectralArray;
	const TArray<int>* WeightsArray;

	switch (StellarClass)
	{
	case EStellarType::HyperGiant:
	case EStellarType::SuperGiant:
	case EStellarType::BrightGiant:
	case EStellarType::Giant:
	case EStellarType::MainSequence:
		SpectralArray = &Spectral_OM;
		WeightsArray = &Weights_OM;
		break;
	case EStellarType::SubGiant:
	case EStellarType::SubDwarf:
	case EStellarType::WhiteDwarf:
		SpectralArray = &Spectral_OK;
		WeightsArray = &Weights_OK;
		break;
	case EStellarType::BrownDwarf:
		SpectralArray = &Spectral_LY;
		WeightsArray = &Weights_LY;
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown StellarClass!"));
		return ESpectralClass::Unknown; // Возвращает O класс по умолчанию или можно выбрать другой класс
	}

	// Создание кумулятивного массива весов
	TArray<int> CumulativeWeights;
	int TotalWeight = 0;
	for (int i = 0; i < WeightsArray->Num(); ++i)
	{
		TotalWeight += (*WeightsArray)[i];
		CumulativeWeights.Add(TotalWeight);
	}

	// Генерация случайного числа
	int RandWeight = FMath::RandRange(0, TotalWeight - 1);

	// Нахождение соответствующего спектрального класса
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

ESpectralClass UStarGenerator::DetermineSpectralClassByTemperature(EStellarType StarType, double Temperature)
{
	switch (StarType)
	{
	/*case EStarStellarClass::WhiteDwarf:
	    return EStarSpectralClass::WD;*/
	case EStellarType::Neutron:
		return ESpectralClass::NS;
	case EStellarType::BlackHole:
		return ESpectralClass::BH;
	case EStellarType::BrownDwarf:
		if (Temperature > 1300) return ESpectralClass::L;
		else if (Temperature > 700) return ESpectralClass::T;
		else return ESpectralClass::Y;
	default: // Для всех остальных типов звезд мы проверяем температуру
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

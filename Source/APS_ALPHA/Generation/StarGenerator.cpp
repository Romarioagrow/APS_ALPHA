#include "StarGenerator.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

namespace APSStellarSurface
{
	struct FVisualPreset
	{
		float Archetype;
		float SurfaceVariation;
		float Granulation;
		float Spots;
		float Corona;
	};

	float TypeActivity(const EStellarType StellarType)
	{
		switch (StellarType)
		{
		case EStellarType::Protostar: return 1.00f;
		case EStellarType::HyperGiant: return 0.92f;
		case EStellarType::SuperGiant: return 0.84f;
		case EStellarType::BrightGiant: return 0.76f;
		case EStellarType::Giant: return 0.68f;
		case EStellarType::SubGiant: return 0.58f;
		case EStellarType::Pulsar: return 0.50f;
		case EStellarType::Neutron: return 0.42f;
		case EStellarType::WhiteDwarf: return 0.35f;
		case EStellarType::BrownDwarf: return 0.62f;
		case EStellarType::SubDwarf: return 0.44f;
		case EStellarType::MainSequence: return 0.52f;
		case EStellarType::BlackHole: return 0.00f;
		default: return 0.45f;
		}
	}

	FVisualPreset GetVisualPreset(const EStellarType StellarType)
	{
		switch (StellarType)
		{
		case EStellarType::SubGiant:
			return {1.0f, 0.58f, 0.58f, 0.44f, 0.14f};
		case EStellarType::Giant:
			return {1.0f, 0.70f, 0.64f, 0.48f, 0.16f};
		case EStellarType::BrightGiant:
			return {1.0f, 0.76f, 0.69f, 0.52f, 0.18f};
		case EStellarType::SuperGiant:
			return {1.0f, 0.82f, 0.74f, 0.56f, 0.20f};
		case EStellarType::HyperGiant:
			return {1.0f, 0.88f, 0.79f, 0.60f, 0.22f};
		case EStellarType::Protostar:
			return {2.0f, 0.68f, 0.40f, 0.18f, 0.24f};
		case EStellarType::WhiteDwarf:
			return {3.0f, 0.18f, 0.22f, 0.06f, 0.13f};
		case EStellarType::Neutron:
			return {3.0f, 0.10f, 0.12f, 0.02f, 0.18f};
		case EStellarType::Pulsar:
			return {4.0f, 0.08f, 0.10f, 0.00f, 0.24f};
		case EStellarType::SubDwarf:
			return {5.0f, 0.30f, 0.34f, 0.30f, 0.11f};
		case EStellarType::BrownDwarf:
			return {5.0f, 0.36f, 0.28f, 0.42f, 0.10f};
		case EStellarType::BlackHole:
			return {6.0f, 0.00f, 0.00f, 0.00f, 0.20f};
		case EStellarType::MainSequence:
		case EStellarType::Unknown:
		default:
			return {0.0f, 0.42f, 0.54f, 0.42f, 0.14f};
		}
	}

	ESpectralClass GetCompactObjectSpectralFallback(const EStellarType StellarType)
	{
		switch (StellarType)
		{
		case EStellarType::Neutron:
		case EStellarType::Pulsar:
			return ESpectralClass::NS;
		case EStellarType::Protostar:
			return ESpectralClass::PS;
		case EStellarType::BlackHole:
			return ESpectralClass::BH;
		default:
			return ESpectralClass::Unknown;
		}
	}
}

UStarGenerator::UStarGenerator()
{
	FDateTime Time = FDateTime::UtcNow();
	RandomStream = FRandomStream(Time.ToUnixTimestamp());
}

void UStarGenerator::SetGenerationSeed(const int32 Seed)
{
	// Hash-derived seeds can be negative. Collapsing them all to one aliases half
	// the canonical addresses; FRandomStream accepts every signed 32-bit seed.
	RandomStream.Initialize(Seed);
	bSeededGeneration = true;
}

double UStarGenerator::GetFarStarVisualRadius(const double PhysicalRadius)
{
	constexpr double MinimumStableVisualRadius = 1.0;
	return FMath::Max(FMath::IsFinite(PhysicalRadius) ? PhysicalRadius : 0.0,
		MinimumStableVisualRadius);
}

double UStarGenerator::GetFarStarVisualEmission(const double PhysicalRadius,
	const double PhysicalEmission, const double ActualVisualRadius)
{
	if (!FMath::IsFinite(PhysicalRadius) || !FMath::IsFinite(PhysicalEmission)
		|| PhysicalRadius <= 0.0 || PhysicalEmission <= 0.0)
	{
		return 0.0;
	}

	const double VisualRadius = FMath::IsFinite(ActualVisualRadius) && ActualVisualRadius > 0.0
		? FMath::Max(ActualVisualRadius, PhysicalRadius)
		: GetFarStarVisualRadius(PhysicalRadius);
	const double AreaCompensation = FMath::Square(
		FMath::Clamp(PhysicalRadius / VisualRadius, 0.0, 1.0));
	return PhysicalEmission * AreaCompensation;
}

void UStarGenerator::ApplySpectralMaterial(AStar* NewStar, TSharedPtr<FStarModel> StarModel)
{
	if (!NewStar || !NewStar->StarMesh || !StarModel)
	{
		return;
	}
	// Blueprint overrides and the mesh's WorldGrid fallback are not authoritative.
	// Reuse a MID only when AStar verifies its ultimate alpha-master base.
	UMaterialInstanceDynamic* StarDynamicMaterial =
		NewStar->EnsureCanonicalStellarMaterial();

	if (StarDynamicMaterial == nullptr)
	{
		return;
	}
	const FAPSStellarMaterialParameters Parameters = ApplySpectralMaterialParameters(StarDynamicMaterial, StarModel);
	NewStar->StarDynamicMaterial = StarDynamicMaterial;
	NewStar->StarMesh->SetMaterial(0, StarDynamicMaterial);
	NewStar->ConfigureStellarPresentation(
		Parameters.Color, Parameters.Emission, Parameters.SurfaceSeed, StarModel->StellarType);
}

FAPSStellarMaterialParameters UStarGenerator::ApplySpectralMaterialParameters(
	UMaterialInstanceDynamic* StarDynamicMaterial, TSharedPtr<FStarModel> StarModel)
{
	if (!IsValid(StarDynamicMaterial) || !StarModel.IsValid()) return {};

	// ���������� ��������� ��������
	FName ParameterName1 = "Multiplier";
	const float MultiplierValue = static_cast<float>(CalculateEmission(StarModel->Luminosity * 25.0f));
	StarDynamicMaterial->SetScalarParameterValue(ParameterName1, MultiplierValue);

	// ���������� ��������� ��������
	FName ParameterName2 = "Color";
	FLinearColor ColorValue = GetStarColor(StarModel->SpectralClass, StarModel->SpectralSubclass);
	StarDynamicMaterial->SetVectorParameterValue(ParameterName2, ColorValue);
	const float SurfaceSeed = FMath::Frac(FMath::Abs(
		StarModel->SurfaceTemperature * 0.000173f
		+ StarModel->Mass * 0.137f
		+ StarModel->Radius * 0.071f
		+ StarModel->Luminosity * 0.019f));
	const float TypeActivity = APSStellarSurface::TypeActivity(StarModel->StellarType);
	const APSStellarSurface::FVisualPreset VisualPreset =
		APSStellarSurface::GetVisualPreset(StarModel->StellarType);
	const float Temperature01 = FMath::Clamp(
		(static_cast<float>(StarModel->SurfaceTemperature) - 2200.0f) / 27800.0f,
		0.0f, 1.0f);
	const float Luminosity01 = FMath::Clamp(
		FMath::Log2(1.0f + FMath::Max(StarModel->Luminosity, 0.0f)) / 14.0f,
		0.0f, 1.0f);
	StarDynamicMaterial->SetScalarParameterValue(TEXT("SurfaceSeed"), SurfaceSeed);
	StarDynamicMaterial->SetScalarParameterValue(TEXT("StellarArchetype"),
		VisualPreset.Archetype);
	StarDynamicMaterial->SetScalarParameterValue(TEXT("SurfaceVariation"),
		FMath::Clamp(VisualPreset.SurfaceVariation
			+ (TypeActivity - 0.5f) * 0.04f, 0.0f, 1.0f));
	StarDynamicMaterial->SetScalarParameterValue(TEXT("GranulationStrength"),
		FMath::Clamp(VisualPreset.Granulation + Luminosity01 * 0.06f, 0.0f, 1.0f));
	// Cooler convection zones tend to read with stronger dark spots; very hot and
	// compact stars retain fine granulation without being covered by black patches.
	StarDynamicMaterial->SetScalarParameterValue(TEXT("SpotStrength"),
		FMath::Clamp(VisualPreset.Spots + (1.0f - Temperature01) * 0.08f,
			0.0f, 1.0f));
	StarDynamicMaterial->SetScalarParameterValue(TEXT("CoronaStrength"),
		FMath::Clamp(VisualPreset.Corona * FMath::Lerp(0.92f, 1.14f, Luminosity01),
			0.10f, 0.24f));

	// ��������� ������������ �������� � ������ �������
	// Keep AStar's public runtime handle synchronized even when this function had
	// to create the MID itself (for example before BeginPlay or after a material
	// reset).  Later stellar edits must never target a stale instance.
	return { ColorValue, MultiplierValue, SurfaceSeed };
}

/*TUniquePtr<FStarModel>*/
void UStarGenerator::GenerateStarModelByProbability(TSharedPtr<FStarModel> StarModel,
                                                    TSharedPtr<FStarClusterModel> FStarClusterModel)
{
	//FStarModel* StarModel = new FStarModel();
	//TUniquePtr<FStarModel> StarModel = MakeUnique<FStarModel>();

	// Unknown means "use a broad natural mix". It must not turn every
	// generated star into an invalid zero-data model.
	const EStarClusterPopulation PopulationPreset =
		StarClusterPopulationWeights.Contains(FStarClusterModel->StarClusterPopulation)
			? FStarClusterModel->StarClusterPopulation
			: EStarClusterPopulation::AllSequenses;
	const TMap<EStellarType, float>& StellarTypeMap = StarClusterPopulationWeights.FindChecked(PopulationPreset);
	const EStellarType StellarType = GenerateStellarTypeByRandomWeights(StellarTypeMap);
	StarModel->StellarType = StellarType; //GenerateStarClassByRandomWeights();

	const EStarClusterComposition CompositionPreset =
		StarClusterCompositionWeights.Contains(FStarClusterModel->StarClusterComposition)
			? FStarClusterModel->StarClusterComposition
			: EStarClusterComposition::AllSpectral;
	const TMap<ESpectralClass, int>& SpectralClassMap = StarClusterCompositionWeights.FindChecked(CompositionPreset);
	ESpectralClass SpectralClass = GenerateSpectralClassByProbability(SpectralClassMap);
	if (StellarType == EStellarType::MainSequence && !MainSequenceMassRanges.Contains(SpectralClass))
	{
		// L/T/Y are substellar classes; compact-object classes are not main
		// sequence stars. Keep a valid visible star instead of producing mass 0.
		SpectralClass = SpectralClass == ESpectralClass::L
			|| SpectralClass == ESpectralClass::T
			|| SpectralClass == ESpectralClass::Y
				? ESpectralClass::M
				: ESpectralClass::G;
	}
	StarModel->SpectralClass = SpectralClass;
	//GenerateSpectralClassByProbability()//ChooseSpectralClassByStellarClass(StarModel.StellarClass);

	//return 
	GenerateStarModel(StarModel);
}


// ����� ����
double UStarGenerator::WienLaw(double temperature)
{
	const double b = 2.897771955e-3; // ��������� ����, �*�
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

	// ��������� �������� ����� � ���������� � �������� ����������� � ���������
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
	const FLinearColor OColor(0.56f, 0.70f, 1.00f);
	const FLinearColor BColor(0.68f, 0.80f, 1.00f);
	const FLinearColor AColor(0.90f, 0.94f, 1.00f);
	const FLinearColor FColor(1.00f, 0.96f, 0.82f);
	const FLinearColor GColor(1.00f, 0.86f, 0.58f);
	const FLinearColor KColor(1.00f, 0.60f, 0.28f);
	const FLinearColor MColor(1.00f, 0.30f, 0.18f);

	FLinearColor HotColor;
	FLinearColor CoolColor;
	switch (SpectralClass)
	{
	case ESpectralClass::O: HotColor = OColor; CoolColor = BColor; break;
	case ESpectralClass::B: HotColor = BColor; CoolColor = AColor; break;
	case ESpectralClass::A: HotColor = AColor; CoolColor = FColor; break;
	case ESpectralClass::F: HotColor = FColor; CoolColor = GColor; break;
	case ESpectralClass::G: HotColor = GColor; CoolColor = KColor; break;
	case ESpectralClass::K: HotColor = KColor; CoolColor = MColor; break;
	case ESpectralClass::M: HotColor = MColor; CoolColor = FLinearColor(0.72f, 0.12f, 0.06f); break;
	case ESpectralClass::L: return FLinearColor(0.58f, 0.24f, 0.08f);
	case ESpectralClass::T: return FLinearColor(0.38f, 0.18f, 0.10f);
	case ESpectralClass::Y: return FLinearColor(0.30f, 0.24f, 0.16f);
	case ESpectralClass::NS: return FLinearColor(0.76f, 0.88f, 1.00f);
	case ESpectralClass::PS: return FLinearColor(1.00f, 0.68f, 0.34f);
	case ESpectralClass::BH: return FLinearColor(0.002f, 0.002f, 0.004f);
	default: return FLinearColor(0.55f, 0.58f, 0.62f);
	}

	// Subclass 0 is hottest and 9 is coolest. Linear RGB avoids HSV's magenta detour.
	const float StableInterpFactor = FMath::Clamp(Subclass, 0, 9) / 9.0f;
	return FMath::Lerp(HotColor, CoolColor, StableInterpFactor);

#if 0 // Replaced: enum-order HSV interpolation crossed into compact-object classes.
	static const TMap<ESpectralClass, FLinearColor> BaseColors =
	{
		{ESpectralClass::O, FLinearColor(0.5, 0.5, 1)}, // �����
		{ESpectralClass::B, FLinearColor(0.6, 0.6, 1)}, // �������
		{ESpectralClass::A, FLinearColor(1, 1, 1)}, // �����
		{ESpectralClass::F, FLinearColor(1, 1, 0.8)}, // �����-�����
		{ESpectralClass::G, FLinearColor(1, 1, 0.6)}, // ������
		{ESpectralClass::K, FLinearColor(1, 0.6, 0.3)}, // ���������
		{ESpectralClass::M, FLinearColor(1, 0.3, 0.3)}, // �������
		{ESpectralClass::L, FLinearColor(0.5, 0.3, 0.1)}, // ����� (��� Brown Dwarf)
		{ESpectralClass::T, FLinearColor(0.6, 0.3, 0.1)}, // ���������� (��� Tauri Dwarf)
		{ESpectralClass::Y, FLinearColor(0.6, 0.5, 0.1)}, // �����-������ (��� Cool Brown Dwarf)
		{ESpectralClass::NS, FLinearColor(0.9, 0.9, 1)}, // ����-������� (��� Neutron Star)
		{ESpectralClass::PS, FLinearColor(1, 0.9, 0.6)}, // ���������� (��� Proto Star)
		{ESpectralClass::BH, FLinearColor(0, 0, 0)}, // ������ (��� Black Hole)
		{ESpectralClass::Unknown, FLinearColor(0.5, 0.5, 0.5)} // ����� (��� Unknown)
	};

	// ��������� ����������� ������������
	float InterpFactor = static_cast<float>(Subclass) / 10.0f;

	// �������� ������� ���� ��� ������� � ���������� ������������� ������
	FLinearColor BaseColor = BaseColors[SpectralClass]; /// CRASH PIE
	FLinearColor NextBaseColor;
	if (SpectralClass != ESpectralClass::Unknown) // ���������, ��� ������� ����� �� ���������
	{
		NextBaseColor = BaseColors[static_cast<ESpectralClass>(static_cast<int>(SpectralClass) + 1)];
	}
	else
	{
		NextBaseColor = BaseColor; // ���� ������� ����� ��� ���������, �� ��������� ���� ������ ����� ��������
	}

	// ������������� ����� ����� �������
	FLinearColor starColor = FLinearColor::LerpUsingHSV(BaseColor, NextBaseColor, InterpFactor);

	return starColor;
#endif
}

FLinearColor UStarGenerator::TemperatureToRGB(float temperature)
{
	float r, g, b;

	// ������ ����������� ���� 1000K �� �����������, �� �� ������ ��������� ���� ����� ��� ��� �������.
	if (temperature < 1000)
		temperature = 1000;

	// ������������� ����������� (�������� 0-1)
	temperature /= 10000;

	if (temperature <= 0.66)
	{
		r = 1;
		g = temperature;
		g = 0.39008157876902 * pow(g, -0.93412075736856); // ������������� ����� ��� ��������
	}
	else
	{
		r = 0.98866243976127 * pow(temperature - 0.66, -0.6841316279095123); // ������������� ����� ��� ��������
		g = 1 - r;
	}

	if (temperature >= 0.5)
	{
		b = 1;
	}
	else if (temperature >= 0.25)
	{
		b = 0.94279106151537 * pow((0.5 - temperature) * 2, -0.70176690865074); // ������������� ����� ��� ������
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
	if (!StarModel)
	{
		return;
	}

	// Compact objects have dedicated spectral identities. Older generation paths
	// left these as Unknown, which selected a zero-temperature grey fallback and
	// made protostars, neutron stars, pulsars and black holes visually identical.
	// Preserve an explicit user-selected spectrum, but make an omitted one
	// deterministic and physically meaningful.
	const ESpectralClass CompactFallback =
		APSStellarSurface::GetCompactObjectSpectralFallback(StarModel->StellarType);
	if (CompactFallback != ESpectralClass::Unknown
		&& StarModel->SpectralClass != CompactFallback)
	{
		// Stellar type owns compact-object physics. A stale/default UI G class must
		// not turn a neutron star, pulsar, protostar or black hole into a yellow sun.
		StarModel->SpectralClass = CompactFallback;
	}
	else if (StarModel->StellarType == EStellarType::BrownDwarf
		&& StarModel->SpectralClass != ESpectralClass::L
		&& StarModel->SpectralClass != ESpectralClass::T
		&& StarModel->SpectralClass != ESpectralClass::Y)
	{
		StarModel->SpectralClass = ESpectralClass::L;
	}
	else if (StarModel->SpectralClass == ESpectralClass::Unknown)
	{
		StarModel->SpectralClass = CompactFallback != ESpectralClass::Unknown
			? CompactFallback : ChooseSpectralClassByStellarClass(StarModel->StellarType);
		if (StarModel->SpectralClass == ESpectralClass::Unknown)
		{
			StarModel->SpectralClass = ESpectralClass::G;
		}
	}

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
		StarModel->RadiusKM = Radius * SolarRadiusKm;
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
		StarModel->Mass = GenerationRandRange(AttributeRanges.Mass.Range.Get<0>(), AttributeRanges.Mass.Range.Get<1>());
		/// TODO: Refactor with Key/Val
		StarModel->Radius = GenerationRandRange(AttributeRanges.Radius.Range.Get<0>(),
		                                     AttributeRanges.Radius.Range.Get<1>());
		StarModel->SurfaceTemperature = GenerateRandomTemperatureBySpectralClass(StarModel->SpectralClass);
		StarModel->Luminosity = CalculateLuminosity(StarModel->Radius, StarModel->SurfaceTemperature);
		StarModel->Age = CalculateNonMainSequenceStarAge(StarModel->Mass);
	}
	// RadiusKM is consumed by preview scaling, focus bounds and lighting for every
	// stellar type. The legacy branch populated it only for MainSequence, collapsing
	// Giant/Protostar/compact classes to the same one-unit presentation fallback.
	if (FMath::IsFinite(StarModel->Radius) && StarModel->Radius > 0.0)
	{
		StarModel->RadiusKM = StarModel->Radius * SolarRadiusKm;
	}

	StarModel->SpectralSubclass = CalculateSpectralSubclass(StarModel->SurfaceTemperature, StarModel->SpectralClass);
	StarModel->SpectralType = CalculateSpectralType(StarModel->StellarType, StarModel->Luminosity);
}

bool UStarGenerator::ApplyRadiusOverrideSolar(
	FStarModel& StarModel, const double RadiusSolar)
{
	if (!FMath::IsFinite(RadiusSolar) || RadiusSolar <= 0.0)
	{
		return false;
	}

	// The limits span stellar-mass compact objects through the largest stable
	// hypergiant presentation while preventing invalid UI text from poisoning the
	// orbital model. This runs before planetary generation, so every safe radius and
	// orbit consumes the same authored physical star.
	StarModel.Radius = FMath::Clamp(RadiusSolar, 1.0e-5, 1000.0);
	StarModel.RadiusKM = StarModel.Radius * SolarRadiusKm;
	StarModel.Luminosity = CalculateLuminosity(
		StarModel.Radius, FMath::Max(StarModel.SurfaceTemperature, 0));
	StarModel.SpectralType = CalculateSpectralType(
		StarModel.StellarType, StarModel.Luminosity);
	return FMath::IsFinite(StarModel.RadiusKM)
		&& FMath::IsFinite(StarModel.Luminosity);
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
	// ��������� ������� T = M^(-2.5)
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
	int RandomValue = GenerationRandRange(0, TotalWeight - 1);
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
	float RandomValue = GenerationRandRange(0.f, TotalWeight - 1.f);
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
	int RandomValue = GenerationRandRange(0, TotalWeight - 1);
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
		// ���������� �������� �� ��������� ��� ������������ ������ �����
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
		// ���������� �������� �� ��������� ��� ������������ ������ �����
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
	case EStellarType::Neutron:
	case EStellarType::Protostar:
	case EStellarType::Pulsar:
	case EStellarType::BlackHole:
	case EStellarType::Unknown:
		return ESpectralType::Unknown;
	case EStellarType::SuperGiant:
		{
			FStarAttributeRanges& AttributeRanges = StarAttributeRanges[StellarType];
			FLuminosityRange LuminosityRange = AttributeRanges.Luminosity; // �������� �������� ����������

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
	default:
		ensureMsgf(false, TEXT("Invalid EStellarType value: %d"), static_cast<int32>(StellarType));
		return ESpectralType::Unknown;
	}
}

int UStarGenerator::CalculateSpectralSubclass(double StarTemperature, ESpectralClass SpectralClass)
{
	if (SpectralClass == ESpectralClass::Unknown)
	{
		return 0;
	}

	const TTuple<double, double>& TempRange = StarTypeTemperatureRanges[SpectralClass];
	const double TemperatureSpan = TempRange.Get<1>() - TempRange.Get<0>();
	if (!FMath::IsFinite(TemperatureSpan) || FMath::IsNearlyZero(TemperatureSpan))
	{
		return 0;
	}

	// Linear interpolation between the temperature range of the spectral class
	double TempRatio = (StarTemperature - TempRange.Get<0>()) / TemperatureSpan;

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
		Luminosity = GenerationRandRange(100000.0, 200000.0);
	}

	return Luminosity;
}

double UStarGenerator::RandomFromRange(TTuple<double, double> Range)
{
	return GenerationRandRange(Range.Key, Range.Value);
}

double UStarGenerator::RandomMass(ESpectralClass SpectralClass)
{
	if (const TTuple<double, double>* Range = MainSequenceMassRanges.Find(SpectralClass))
	{
		return RandomStream.FRandRange(Range->Get<0>(), Range->Get<1>());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainSequenceMassRanges doesn't contain %s"),
		       *UEnum::GetValueAsString(SpectralClass));
		return 0; // ���������� ��������� �������� �� ��������� ��� ������������ ������ �����
	}
	//if (MainSequenceMassRanges.Contains(SpectralClass))
	//{
	//    auto Range = MainSequenceMassRanges[SpectralClass];
	//    return FMath::FRandRange(Range.Get<0>(), Range.Get<1>());
	//}
	//else
	//{
	//    UE_LOG(LogTemp, Warning, TEXT("MainSequenceMassRanges doesn't contain %s"), *UEnum::GetValueAsString(SpectralClass));
	//    return 0; // ���������� ��������� �������� �� ��������� ��� ������������ ������ �����
	//}
}

double UStarGenerator::RandomRadius(ESpectralClass SpectralClass)
{
	auto Range = MainSequenceRadiusRanges[SpectralClass];
	return GenerationRandRange(Range.Get<0>(), Range.Get<1>());
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
		// Reapplying the same radius/temperature must be idempotent. A random floor
		// changed compact-object luminosity on every repeated property application.
		Luminosity = bSeededGeneration ? 0.000001 : FMath::FRandRange(0.00001, 0.000001);
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
��� ������, ������� ���������, ����� ������������ ������ ����� ��������������� ������� ������ ������(EStellarClass).
��� ����� �� ����� ������������ ��������� ������� :

EStellarClass	ESpectralClass
Dwarf Star	M, K, G(����� �������� � ������ ������, ���� �� ������ ����� ���������� ������ ��������� ������������ �������)
Main Sequence Star	O, B, A, F, G, K, M
Giant Star	K, M
Supergiant Star	O, B, A, F, G, K, M

��������� ��� ����������, �� ����� ������ ������ �������, ������� ����� �������� �������� ������������ ����� ��� ��������� ������ ������.

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
	// ������� ������������ ������� ��� ������� ����������� ������
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

	// ����������� ������� � �����
	const TArray<ESpectralClass>* SpectralArray;
	const TArray<int>* WeightsArray;

	switch (StellarClass)
	{
	case EStellarType::Neutron:
	case EStellarType::Pulsar:
		return ESpectralClass::NS;
	case EStellarType::Protostar:
		return ESpectralClass::PS;
	case EStellarType::BlackHole:
		return ESpectralClass::BH;
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
		return ESpectralClass::Unknown; // ���������� O ����� �� ��������� ��� ����� ������� ������ �����
	}

	// �������� ������������� ������� �����
	TArray<int> CumulativeWeights;
	int TotalWeight = 0;
	for (int i = 0; i < WeightsArray->Num(); ++i)
	{
		TotalWeight += (*WeightsArray)[i];
		CumulativeWeights.Add(TotalWeight);
	}

	// ��������� ���������� �����
	int RandWeight = GenerationRandRange(0, TotalWeight - 1);

	// ���������� ���������������� ������������� ������
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
	default: // ��� ���� ��������� ����� ����� �� ��������� �����������
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
	return GenerationRandRange(TemperatureRange.Get<0>(), TemperatureRange.Get<1>());
}

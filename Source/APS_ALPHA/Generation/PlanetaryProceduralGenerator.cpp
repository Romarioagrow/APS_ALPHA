#include "PlanetaryProceduralGenerator.h"
#include <cmath>
#include "MoonGenerator.h"
#include "PlanetGenerator.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetHabitability.h"
#include "APS_ALPHA/Core/Structs/PlanetarySystemGenerationModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"

namespace APSOrbitGeneration
{
	constexpr double SolarRadiusInAu = 0.00465047;
	constexpr double AstronomicalUnitKm = 149597870.7;

	double DistributionPacking(const EOrbitDistributionType DistributionType)
	{
		switch (DistributionType)
		{
		case EOrbitDistributionType::Uniform: return 0.72;
		case EOrbitDistributionType::Gaussian: return 0.58;
		case EOrbitDistributionType::Chaotic: return 0.46;
		case EOrbitDistributionType::InnerOuter: return 0.60;
		case EOrbitDistributionType::Dense: return 0.36;
		default: return 0.50;
		}
	}
}

void UPlanetarySystemGenerator::EnforcePlanetSurfaceClearance(
	FPlanetarySystemModel& PlanetarySystemModel)
{
	TArray<int32> OrderedPlanetIndices;
	for (int32 PlanetIndex = 0;
		PlanetIndex < PlanetarySystemModel.PlanetsList.Num(); ++PlanetIndex)
	{
		const TSharedPtr<FPlanetData>& PlanetData =
			PlanetarySystemModel.PlanetsList[PlanetIndex];
		if (PlanetData.IsValid() && PlanetData->PlanetModel.IsValid())
		{
			OrderedPlanetIndices.Add(PlanetIndex);
		}
	}
	OrderedPlanetIndices.Sort([&PlanetarySystemModel](const int32 LeftIndex, const int32 RightIndex)
	{
		const double LeftOrbit = PlanetarySystemModel.PlanetsList[LeftIndex]->OrbitRadius;
		const double RightOrbit = PlanetarySystemModel.PlanetsList[RightIndex]->OrbitRadius;
		return (FMath::IsFinite(LeftOrbit) ? LeftOrbit : 0.0)
			< (FMath::IsFinite(RightOrbit) ? RightOrbit : 0.0);
	});

	double PreviousOrbitAu = 0.0;
	double PreviousRadiusKm = 0.0;
	bool bHasPreviousPlanet = false;
	for (const int32 PlanetIndex : OrderedPlanetIndices)
	{
		TSharedPtr<FPlanetData>& PlanetData = PlanetarySystemModel.PlanetsList[PlanetIndex];
		FPlanetModel& PlanetModel = *PlanetData->PlanetModel;
		const double PlanetRadiusKm = FMath::Max3(
			FMath::Max(0.0, static_cast<double>(PlanetModel.RadiusKM)),
			FMath::Max(0.0, static_cast<double>(PlanetData->PlanetRadiusKM)),
			FMath::Max(0.0, static_cast<double>(PlanetModel.Radius) * 6371.0));
		double OrbitAu = FMath::IsFinite(PlanetData->OrbitRadius)
			? FMath::Max(PlanetData->OrbitRadius, 0.0) : PreviousOrbitAu;
		if (bHasPreviousPlanet)
		{
			const double CombinedRadiusKm = PreviousRadiusKm + PlanetRadiusKm;
			const double SurfaceClearanceKm = FMath::Max(
				1000.0, CombinedRadiusKm * 0.10);
			const double RequiredCenterGapAu =
				(CombinedRadiusKm + SurfaceClearanceKm)
				/ APSOrbitGeneration::AstronomicalUnitKm;
			OrbitAu = FMath::Max(OrbitAu, PreviousOrbitAu + RequiredCenterGapAu);
		}

		PlanetData->OrbitRadius = OrbitAu;
		PlanetModel.OrbitDistance = OrbitAu;
		PlanetData->PlanetModelData = PlanetModel;
		PlanetData->PlanetHabitability = PlanetModel.PlanetHabitability;
		PreviousOrbitAu = OrbitAu;
		PreviousRadiusKm = PlanetRadiusKm;
		bHasPreviousPlanet = true;
	}
}

void UPlanetarySystemGenerator::EnforceMinimumPlanetOrbitSpacing(
	TArray<double>& InOutOrbitRadii, const double MinOrbit, const double MaxOrbit,
	const double StellarRadiusSolar, const EOrbitDistributionType DistributionType)
{
	if (InOutOrbitRadii.IsEmpty())
	{
		return;
	}

	const double PhotosphereClearanceAu = FMath::Max(
		0.001, FMath::Max(0.0, StellarRadiusSolar)
			* APSOrbitGeneration::SolarRadiusInAu * 1.35);
	const double EffectiveMin = FMath::Max(
		FMath::IsFinite(MinOrbit) ? MinOrbit : 0.0, PhotosphereClearanceAu);
	const double RequestedSpan = FMath::Max(
		FMath::IsFinite(MaxOrbit - MinOrbit) ? MaxOrbit - MinOrbit : 0.0, 0.25);
	const double EffectiveMax = FMath::Max(
		FMath::IsFinite(MaxOrbit) ? MaxOrbit : EffectiveMin,
		EffectiveMin + RequestedSpan);

	for (double& OrbitRadius : InOutOrbitRadii)
	{
		if (!FMath::IsFinite(OrbitRadius))
		{
			OrbitRadius = EffectiveMin;
		}
		OrbitRadius = FMath::Clamp(OrbitRadius, EffectiveMin, EffectiveMax);
	}
	InOutOrbitRadii.Sort();
	if (InOutOrbitRadii.Num() == 1)
	{
		return;
	}

	const double AvailableSpan = EffectiveMax - EffectiveMin;
	const double CapacityGap = AvailableSpan / (InOutOrbitRadii.Num() - 1);
	const double CharacterGap = AvailableSpan / (InOutOrbitRadii.Num() + 1)
		* APSOrbitGeneration::DistributionPacking(DistributionType);
	const double MinimumGap = FMath::Min(
		CapacityGap, FMath::Max(CharacterGap, 0.01));

	InOutOrbitRadii[0] = FMath::Max(InOutOrbitRadii[0], EffectiveMin);
	for (int32 OrbitIndex = 1; OrbitIndex < InOutOrbitRadii.Num(); ++OrbitIndex)
	{
		InOutOrbitRadii[OrbitIndex] = FMath::Max(
			InOutOrbitRadii[OrbitIndex], InOutOrbitRadii[OrbitIndex - 1] + MinimumGap);
	}
	if (InOutOrbitRadii.Last() > EffectiveMax)
	{
		InOutOrbitRadii.Last() = EffectiveMax;
		for (int32 OrbitIndex = InOutOrbitRadii.Num() - 2; OrbitIndex >= 0; --OrbitIndex)
		{
			InOutOrbitRadii[OrbitIndex] = FMath::Min(
				InOutOrbitRadii[OrbitIndex], InOutOrbitRadii[OrbitIndex + 1] - MinimumGap);
		}
	}
}

void UPlanetarySystemGenerator::EnforceSafeMoonOrbitSpacing(
	FPlanetModel& PlanetModel)
{
	if (PlanetModel.MoonsList.IsEmpty())
	{
		return;
	}

	const double ParentRadius = FMath::Max(PlanetModel.Radius, 0.01);
	double PreviousCenterInParentRadii = 0.0;
	double PreviousMoonRadiusRatio = 0.0;
	for (int32 MoonIndex = 0; MoonIndex < PlanetModel.MoonsList.Num(); ++MoonIndex)
	{
		TSharedPtr<FMoonData>& MoonData = PlanetModel.MoonsList[MoonIndex];
		if (!MoonData.IsValid())
		{
			continue;
		}
		if (!MoonData->MoonModel.IsValid())
		{
			MoonData->MoonModel = MakeShared<FMoonModel>(MoonData->MoonModelData);
		}

		FMoonModel& MoonModel = *MoonData->MoonModel;
		const double MoonRadiusRatio = FMath::Clamp(
			FMath::Max(MoonModel.Radius, 0.001) / ParentRadius, 0.001, 2.0);
		const double RequestedAltitude = FMath::Max(
			FMath::IsFinite(MoonData->OrbitRadius) ? MoonData->OrbitRadius : 0.0,
			FMath::IsFinite(MoonModel.OrbitDistance) ? MoonModel.OrbitDistance : 0.0);
		double SafeCenter = FMath::Max(
			1.0 + RequestedAltitude,
			1.0 + MoonRadiusRatio + FMath::Max(0.45, MoonRadiusRatio * 0.75));
		if (PreviousCenterInParentRadii > 0.0)
		{
			const double InterMoonGap = FMath::Max(
				0.28, (PreviousMoonRadiusRatio + MoonRadiusRatio) * 0.50);
			SafeCenter = FMath::Max(SafeCenter,
				PreviousCenterInParentRadii + PreviousMoonRadiusRatio
				+ MoonRadiusRatio + InterMoonGap);
		}

		MoonData->MoonOrder = MoonIndex + 1;
		MoonData->OrbitRadius = SafeCenter - 1.0;
		MoonModel.OrbitDistance = MoonData->OrbitRadius;
		MoonData->MoonModelData = MoonModel;
		PreviousCenterInParentRadii = SafeCenter;
		PreviousMoonRadiusRatio = MoonRadiusRatio;
	}
	PlanetModel.MoonsListData = PlanetModel.GetMoonsData();
}

void UPlanetarySystemGenerator::ApplyModel(APlanetarySystem* NewPlanetarySystem,
                                           TSharedPtr<FPlanetarySystemModel> PlanetraySystemModel)
{
	NewPlanetarySystem->SetAmountOfPlanets(PlanetraySystemModel->AmountOfPlanets);
	NewPlanetarySystem->SetPlanetarySystemType(PlanetraySystemModel->PlanetarySystemType);
	NewPlanetarySystem->SetOrbitDistributionType(PlanetraySystemModel->OrbitDistributionType);
	NewPlanetarySystem->SetStarFullSpectralName(PlanetraySystemModel->FullSpectralName);
	NewPlanetarySystem->SetPlanetsList(PlanetraySystemModel->PlanetsList);
	NewPlanetarySystem->HotZoneRadius = PlanetraySystemModel->HotZoneRadius;
	NewPlanetarySystem->WarmZoneRadius = PlanetraySystemModel->WarmZoneRadius;
	NewPlanetarySystem->ColdZoneRadius = PlanetraySystemModel->ColdZoneRadius;
	NewPlanetarySystem->IceZoneRadius = PlanetraySystemModel->IceZoneRadius;
	NewPlanetarySystem->InnerPlanetZoneRadius = PlanetraySystemModel->InnerPlanetZoneRadius;
	NewPlanetarySystem->HabitableZoneRadius = PlanetraySystemModel->HabitableZoneRadius;
	NewPlanetarySystem->GasGiantsZoneRadius = PlanetraySystemModel->GasGiantsZoneRadius;

	NewPlanetarySystem->HotZoneOuter = PlanetraySystemModel->HotZoneOuter;
	NewPlanetarySystem->WarmZoneOuter = PlanetraySystemModel->WarmZoneOuter;
	NewPlanetarySystem->ColdZoneOuter = PlanetraySystemModel->ColdZoneOuter;
	NewPlanetarySystem->IceZoneOuter = PlanetraySystemModel->IceZoneOuter;
	NewPlanetarySystem->GasGiantsZoneOuter = PlanetraySystemModel->GasGiantsZoneOuter;
	NewPlanetarySystem->KuiperBeltZoneOuter = PlanetraySystemModel->KuiperBeltZoneOuter;
	NewPlanetarySystem->InnerZoneOuter = PlanetraySystemModel->InnerZoneOuter;
	NewPlanetarySystem->OuterZoneOuter = PlanetraySystemModel->OuterZoneOuter;
	NewPlanetarySystem->HabitableZoneOuter = PlanetraySystemModel->HabitableZoneOuter;
	NewPlanetarySystem->StarDeadZoneOuter = PlanetraySystemModel->StarDeadZoneOuter;
}


void UPlanetarySystemGenerator::GeneratePlanetMoonsList(
	UPlanetGenerator* PlanetGenerator,
	UMoonGenerator* MoonGenerator,
	TSharedPtr<FPlanetModel> PlanetModel,
	const double PlanetRadius, const int AmountOfMoons, const int32 StablePlanetIndex)
{
	if (!PlanetModel.IsValid())
	{
		return;
	}
	const int32 FinalMoonCount = FMath::Max(AmountOfMoons, 0);
	if (StablePlanetIndex != INDEX_NONE) ResetBodyRandom(StablePlanetIndex, 0x4d4f4f4e);
	PlanetModel->AmountOfMoons = FinalMoonCount;
	TArray<TSharedPtr<FMoonData>> MoonsList{};
	TArray<double> MoonOrbits;
	MoonOrbits.Reserve(FinalMoonCount);

	if (PlanetModel->PlanetType == EPlanetType::GasGiant
		|| PlanetModel->PlanetType == EPlanetType::IceGiant
		|| PlanetModel->PlanetType == EPlanetType::HotGiant)
	{
		// ������������� ����� �� 1 �� 10 �������� �������
		for (int i = 0; i < FinalMoonCount; i++)
		{
			double orbitRadius = GenerationRandRange(PlanetRadius * 1.0, PlanetRadius * 10.0);
			orbitRadius /= 40;
			MoonOrbits.Add(orbitRadius);
		}
	}
	else
	{
		double a = 1.5;
		double d = 1.4;
		// ������������ ������ �������-���� ��� ��������� ������
		for (int i = 0; i < FinalMoonCount; i++)
		{
			double MoonOrbitRadius = a + d * pow(2, i);
			MoonOrbitRadius = GenerationRandRange(MoonOrbitRadius * 0.9, MoonOrbitRadius * 1.3);
			MoonOrbits.Add(MoonOrbitRadius);
		}
	}
	MoonOrbits.Sort();

	for (double MoonOrbit : MoonOrbits)
	{
		//FMoonGenerationModel MoonModel;
		TSharedPtr<FMoonModel> MoonModel = MakeShared<FMoonModel>();

		EMoonType MoonType = MoonGenerator->GenerateMoonType(PlanetModel, GetGenerationRandom());

		// ��������� ���������� ��������� ����
		double MoonMass = MoonGenerator->CalculateRandomMoonMass(GetGenerationRandom());
		double MoonDensity = MoonGenerator->CalculateRandomMoonDensity(MoonType, GetGenerationRandom());
		double MoonRadius = MoonGenerator->CalculateMoonRadius(MoonDensity, MoonMass);
		double MoonGravity = MoonMass / FMath::Pow(MoonRadius, 2);
		/// TODO: to MoonGenerator->CalculateGravitationalForce(PlanetModel.Mass, MoonMass, MoonOrbit);

		// ������� ������ ����
		MoonModel->Type = MoonType;
		MoonModel->Mass = MoonMass;
		MoonModel->Radius = MoonRadius;
		MoonModel->RadiusKM = MoonModel->Radius * 6371.0;
		MoonModel->MoonDensity = MoonDensity; // TODO: To Parent 
		MoonModel->MoonGravity = MoonGravity;
		MoonModel->OrbitDistance = MoonOrbit;
		MoonModel->MoonAtmosphereHeight = MoonModel->RadiusKM / 30;
		MoonModel->PlanetType = UMoonGenerator::ResolveSurfaceType(*MoonModel);
		MoonModel->PlanetHabitability =
			UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
				MoonModel->PlanetType, PlanetModel->PlanetZone,
				MoonModel->MoonAtmosphereHeight);

		// ������� ������ � ����
		int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
		TSharedPtr<FMoonData> MoonData = MakeShared<FMoonData>(MoonIndex + 1, MoonOrbit, MoonModel);

		// ��������� ������ � ���� � ������
		MoonsList.Add(MoonData);
	}

	PlanetModel->Orbits.Reset();
	for (int32 i = 0; i <= (int32)EOrbitHeight::VeryHighOrbit; ++i)
	{
		FOrbitInfo OrbitInfo{};
		OrbitInfo.OrbitHeightType = (EOrbitHeight)i;
		OrbitInfo.OrbitHeight = PlanetGenerator->CalculateOrbitHeight(OrbitInfo.OrbitHeightType, PlanetRadius);
		PlanetModel->Orbits.Add(OrbitInfo);
	}

	PlanetModel->MoonsList = MoonsList;
	EnforceSafeMoonOrbitSpacing(*PlanetModel);
	PlanetModel->MoonsListData = PlanetModel->GetMoonsData();
	/*TSharedPtr<FPlanetData> PlanetData = MakeShared<FPlanetData>(PlanetIndex, OrbitRadius, PlanetModel);
	PlanetarySystemModel->PlanetsList.Add(PlanetData);*/
}

void UPlanetarySystemGenerator::GenerateCustomPlanetarySystemModel(
	TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel,
	TSharedPtr<FStarModel> StarModel,
	UPlanetGenerator* PlanetGenerator,
	UMoonGenerator* MoonGenerator
)
{
	// OrbitRadii is scratch storage owned by the reusable generator object.  It
	// must describe this model only: retaining a previous star's radii created
	// 15, then 30, then 45 planets in a triple-star preview and drew matching
	// phantom-looking orbit rings.
	OrbitRadii.Reset();
	PlanetarySystemModel->PlanetsList.Reset();
	if (bSeededGeneration) GenerationRandom.Initialize(GenerationSeed);
	int32 FinalPlanetCount = PlanetarySystemModel->AmountOfPlanets; //GenerationRandRange(MinPlanetCount, MaxPlanetCount);

	// ��� ����� ���� ��������� ��� ����������, ��������� �� ����������� ������
	double MinOrbitScalingFactor = 1.0f;
	double MaxOrbitScalingFactor = 10.0f;

	if (StarModel->StellarType == EStellarType::HyperGiant)
	{
		MaxOrbitScalingFactor = 5.0f; // ��������� ������������ ������ ��� �������������
	}
	else if (StarModel->StellarType == EStellarType::SuperGiant)
	{
		MaxOrbitScalingFactor = 6.0f; // ��������� ������������ ������ ��� �������������
	}

	double MinOrbit = StarModel->Mass * MinOrbitScalingFactor;
	double MaxOrbit = StarModel->Mass * MaxOrbitScalingFactor;
	StarModel->MinOrbit = MinOrbit;
	StarModel->MaxOrbit = MaxOrbit;
	UE_LOG(LogTemp, VeryVerbose, TEXT("MAX Orbit: %f"), MaxOrbit);


	// ��������� ��������� ������������� ��� ����� �������
	EOrbitDistributionType OrbitDistributionType = PlanetarySystemModel->OrbitDistributionType;
	FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Orbit Distribution Type: %s"), *OrbitType);

	if (PlanetarySystemModel->PlanetarySystemType == EPlanetarySystemType::NoPlanetSystem)
	{
		FinalPlanetCount = 0;
	}
	if (PlanetarySystemModel->PlanetarySystemType == EPlanetarySystemType::SinglePlanetSystem)
	{
		FinalPlanetCount = 1;
	}

	for (int i = 0; i < FinalPlanetCount; i++)
	{
		double OrbitDistributionValue;
		double OrbitRadius;
		switch (OrbitDistributionType)
		{
		case EOrbitDistributionType::Uniform:
			OrbitDistributionValue = GenerationRandRange(0.1, 1.0);
		// next orbit - PlanetAffection Zone + Star Radius
			break;
		case EOrbitDistributionType::Gaussian:
			OrbitDistributionValue = RandGauss();
		// Overlap Fix
			break;
		case EOrbitDistributionType::Chaotic:
			{
				OrbitDistributionValue = GenerationRandRange(MinOrbit, MaxOrbit);
				OrbitRadius = OrbitDistributionValue;
				break;
			}
		case EOrbitDistributionType::InnerOuter:
			{
				if (i < FinalPlanetCount / 2.0)
				{
					OrbitDistributionValue = GenerationRandRange(0.01, 0.5);
				}
				else
				{
					OrbitDistributionValue = GenerationRandRange(0.5, 1.0);
				}
				break;
			}
		case EOrbitDistributionType::Dense:
			OrbitDistributionValue = GenerationRandRange(0.05, 0.5);
			break;
		}

		if (OrbitDistributionType != EOrbitDistributionType::Chaotic)
		{
			OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
		}

		// ��������� ������� ������������� � ������ ��������� �����
		OrbitRadii.Add(OrbitRadius);
	}
	OrbitRadii.Sort();
	EnforceMinimumPlanetOrbitSpacing(
		OrbitRadii, MinOrbit, MaxOrbit, StarModel->Radius, OrbitDistributionType);
	if (!OrbitRadii.IsEmpty())
	{
		MaxOrbit = FMath::Max(MaxOrbit, OrbitRadii.Last());
		StarModel->MinOrbit = FMath::Min(MinOrbit, OrbitRadii[0]);
		StarModel->MaxOrbit = MaxOrbit;
	}
	UE_LOG(LogTemp, VeryVerbose, TEXT("OrbitRadii Num: %d "), OrbitRadii.Num());
	UE_LOG(LogTemp, VeryVerbose, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

	// ��������� ��������� ����
	// AU equilibrium bounds. The random-system path uses the same equations;
	// the legacy extra x2 here mislabeled temperate worlds as cold/ice planets.
	double HabitableZoneInner = sqrt(StarModel->Luminosity / 1.1);
	double HabitableZoneOuter = sqrt(StarModel->Luminosity / 0.53);

	// Star Dead zone
	double StarDeadZoneInner = 0; // ���������� �� ������
	double StarRadiusInAU = StarModel->Radius * 0.00465047;
	double StarDeadZoneOuter = StarRadiusInAU * 2; // ������������� �� ����������, ������ �������� ������� ������ � AU

	// Zones
	double HotZoneInner = 0;
	double HotZoneOuter = 0;
	double WarmZoneInner = 0;
	double WarmZoneOuter = 0;
	double ColdZoneInner = 0;
	double ColdZoneOuter = 0;
	double IceZoneInner = 0;
	double IceZoneOuter = 0;
	double GasGiantsZoneInner = 0;
	double GasGiantsZoneOuter = 0;
	double KuiperBeltZoneInner = 0;
	double KuiperBeltZoneOuter = 0;
	double InnerZoneInner = 0; // ���������� �� ������
	double InnerZoneOuter = 0; // ������������� �������� ������� ����
	double OuterZoneInner = 0; // ���������� �� ������� ���� ������� ��������
	double OuterZoneOuter = 0; // ��������� �������

	if (HabitableZoneOuter < MaxOrbit)
	{
		// ��������� ���� StarDeadZone
		StarDeadZoneInner = 0; // ���������� �� ������
		StarRadiusInAU = StarModel->Radius * 0.00465047;
		StarDeadZoneOuter = StarRadiusInAU; // ������������� �� ����������, ������ �������� ������� ������ � AU

		// ��������� ������� ����
		HotZoneInner = StarDeadZoneOuter;
		HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

		// ��������� ������ ����
		WarmZoneInner = HotZoneOuter;
		WarmZoneOuter = HabitableZoneInner;

		// ��������� �������� ����
		ColdZoneInner = HabitableZoneOuter; //FMath::Max(HabitableZoneOuter, MinOrbit);
		ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2;

		// ��������� ������� ����
		IceZoneInner = ColdZoneOuter; //FMath::Max(ColdZoneOuter, MinOrbit);
		IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2;

		// ��������� ���� ������� ��������
		GasGiantsZoneInner = FMath::Max(IceZoneOuter, MinOrbit);
		GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2;

		// ��������� ���� ����� �������
		KuiperBeltZoneInner = FMath::Max(GasGiantsZoneOuter, MinOrbit);
		KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2;


		if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
		{
			// ��������� ���������� ����
			InnerZoneInner = StarDeadZoneOuter; //0; // ���������� �� ������
			InnerZoneOuter = HotZoneOuter; // ������������� �������� ������� ����

			// ��������� ������� ����
			OuterZoneInner = GasGiantsZoneOuter; // ���������� �� ������� ���� ������� ��������
			OuterZoneOuter = OuterZoneInner * 2; // ��������� �������

			PlanetarySystemModel->InnerPlanetZoneRadius = FZoneRadius(InnerZoneInner, InnerZoneOuter);
			PlanetarySystemModel->OuterPlanetZoneRadius = FZoneRadius(OuterZoneInner, OuterZoneOuter);
		}
	}
	else
	{
		StarDeadZoneInner = 0;
		StarRadiusInAU = StarModel->Radius * 0.00465047;
		StarDeadZoneOuter = StarRadiusInAU * 2;
		HotZoneInner = StarDeadZoneOuter;
		HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

		WarmZoneInner = HotZoneOuter;
		WarmZoneOuter = HabitableZoneInner;

		ColdZoneInner = WarmZoneOuter;
		ColdZoneOuter = FMath::Min(WarmZoneOuter * 2, MaxOrbit);

		if (ColdZoneOuter < MaxOrbit)
		{
			IceZoneInner = ColdZoneOuter;
			IceZoneOuter = FMath::Min(ColdZoneOuter * 2, MaxOrbit);
		}

		if (IceZoneOuter < MaxOrbit)
		{
			GasGiantsZoneInner = IceZoneOuter;
			GasGiantsZoneOuter = FMath::Min(IceZoneOuter * 2, MaxOrbit);
		}

		if (GasGiantsZoneOuter < MaxOrbit)
		{
			KuiperBeltZoneInner = GasGiantsZoneOuter;
			KuiperBeltZoneOuter = FMath::Min(GasGiantsZoneOuter * 2, MaxOrbit);
		}
	}

	// �������������� ������������ ��� ������ ���
	//double scaleCoeff = 149597870 * 3000;

	/* StarDeadZoneOuter = StarDeadZoneOuter * 149597870 * 3000;
	 HotZoneOuter = (StarDeadZoneOuter + ((HabitableZoneInner * scaleCoeff) - StarDeadZoneOuter) / 2) * 149597870 * 3000;
	 WarmZoneOuter = HabitableZoneInner * scaleCoeff * 149597870 * 3000;
	 ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2 * scaleCoeff * 149597870 * 3000;
	 IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2 * scaleCoeff * 149597870 * 3000;
	 GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2 * scaleCoeff * 149597870 * 3000;
	 KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2 * scaleCoeff * 149597870 * 3000;*/

	//*149597870 * 3000
	//PlanetarySystemModel->MinOrbit
	/*HotZoneInner *= 149597870 * 3000;
	HotZoneOuter *= 149597870 * 3000;
	WarmZoneInner *= 149597870 * 3000;
	WarmZoneOuter *= 149597870 * 3000;
	ColdZoneInner *= 149597870 * 3000;
	ColdZoneOuter *= 149597870 * 3000;
	IceZoneInner *= 149597870 * 3000;
	IceZoneOuter *= 149597870 * 3000;
	GasGiantsZoneInner *= 149597870 * 3000;
	GasGiantsZoneOuter *= 149597870 * 3000;
	KuiperBeltZoneInner *= 149597870 * 3000;
	KuiperBeltZoneOuter *= 149597870 * 3000;
	InnerZoneInner *= 149597870 * 3000;
	InnerZoneOuter *= 149597870 * 3000;
	OuterZoneInner *= 149597870 * 3000;
	OuterZoneOuter *= 149597870 * 3000;*/

	//PlanetarySystemModel->HotZoneOuter = HotZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->WarmZoneOuter = WarmZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->ColdZoneOuter = ColdZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->IceZoneOuter = IceZoneOuter *149597870 * 3000;
	//PlanetarySystemModel->GasGiantsZoneOuter = GasGiantsZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->KuiperBeltZoneOuter = KuiperBeltZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->InnerZoneOuter = InnerZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->OuterZoneOuter = OuterZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->HabitableZoneOuter = HabitableZoneOuter * 149597870 * 3000;
	//PlanetarySystemModel->StarDeadZoneOuter = StarDeadZoneOuter * 149597870 * 3000;

	PlanetarySystemModel->DeadZoneRadius = FZoneRadius(StarDeadZoneInner, StarDeadZoneOuter);
	PlanetarySystemModel->HabitableZoneRadius = FZoneRadius(HabitableZoneInner, HabitableZoneOuter);
	PlanetarySystemModel->ColdZoneRadius = FZoneRadius(ColdZoneInner, ColdZoneOuter);
	PlanetarySystemModel->IceZoneRadius = FZoneRadius(IceZoneInner, IceZoneOuter);
	PlanetarySystemModel->WarmZoneRadius = FZoneRadius(WarmZoneInner, WarmZoneOuter);
	PlanetarySystemModel->HotZoneRadius = FZoneRadius(HotZoneInner, HotZoneOuter);
	PlanetarySystemModel->GasGiantsZoneRadius = FZoneRadius(GasGiantsZoneInner, GasGiantsZoneOuter);
	PlanetarySystemModel->KuiperBeltZoneRadius = FZoneRadius(KuiperBeltZoneInner, KuiperBeltZoneOuter);

	// ������� ��������� ����
	UE_LOG(LogTemp, VeryVerbose, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

	// ������� ����������� ������
	// ������� ��� ������ ������
	for (int i = 0; i < OrbitRadii.Num(); ++i)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Dead Zone         - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->DeadZoneRadius.InnerRadius, PlanetarySystemModel->DeadZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Hot Zone          - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->HotZoneRadius.InnerRadius, PlanetarySystemModel->HotZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Warm Zone         - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->WarmZoneRadius.InnerRadius, PlanetarySystemModel->WarmZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Habitable Zone    - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->HabitableZoneRadius.InnerRadius,
	       PlanetarySystemModel->HabitableZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Cold Zone         - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->ColdZoneRadius.InnerRadius, PlanetarySystemModel->ColdZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Ice Zone          - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->IceZoneRadius.InnerRadius, PlanetarySystemModel->IceZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Gas Giants Zone   - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius,
	       PlanetarySystemModel->GasGiantsZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Kuiper Belt Zone  - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius,
	       PlanetarySystemModel->KuiperBeltZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->InnerPlanetZoneRadius.InnerRadius,
	       PlanetarySystemModel->InnerPlanetZoneRadius.OuterRadius);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"),
	       PlanetarySystemModel->OuterPlanetZoneRadius.InnerRadius,
	       PlanetarySystemModel->OuterPlanetZoneRadius.OuterRadius);


	UE_LOG(LogTemp, VeryVerbose, TEXT("Hot Zone Outer: %f"), PlanetarySystemModel->HotZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Warm Zone Outer: %f"), PlanetarySystemModel->WarmZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Cold Zone Outer: %f"), PlanetarySystemModel->ColdZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Ice Zone Outer: %f"), PlanetarySystemModel->IceZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Gas Giants Zone Outer: %f"), PlanetarySystemModel->GasGiantsZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Kuiper Belt Zone Outer: %f"), PlanetarySystemModel->KuiperBeltZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Inner Zone Outer: %f"), PlanetarySystemModel->InnerZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Outer Zone Outer: %f"), PlanetarySystemModel->OuterZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Habitable Zone Outer: %f"), PlanetarySystemModel->HabitableZoneOuter);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Star Dead Zone Outer: %f"), PlanetarySystemModel->StarDeadZoneOuter);

	/// TODO: To GeneratePlanetOrbits
	{
		for (double OrbitRadius : OrbitRadii)
		{
			int PlanetIndex = OrbitRadii.IndexOfByKey(OrbitRadius);
			ResetBodyRandom(PlanetIndex, 0x504c4e54);

			//FPlanetModel PlanetModel; /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);
			TSharedPtr<FPlanetModel> PlanetModel = MakeShared<FPlanetModel>();
			/// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);


			PlanetModel->OrbitDistance = OrbitRadius;

			// planet temperature
			double PlanetTemperature = StarModel->SurfaceTemperature * sqrt(StarModel->Radius / (2 * OrbitRadius));
			PlanetModel->Temperature = PlanetTemperature; // to celestial body

			// ����������, � ����� ���� ��������� �������.
			EPlanetaryZoneType PlanetZone = DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
			//EPlanetaryZoneType::Unknown;//DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
			PlanetModel->PlanetZone = PlanetZone;

			EPlanetType PlanetType = DeterminePlanetType(PlanetZone);
			PlanetModel->PlanetType = PlanetType;

			FDensityRange PlanetDensityRange = GetPlanetDensityRange(PlanetType);
			double PlanetDensity = GenerationRandRange(PlanetDensityRange.MinDensity, PlanetDensityRange.MaxDensity);
			PlanetModel->PlanetDensity = PlanetDensity;

			// assuming that radius is random in the range 1 - 2 Earth radii (this can be adjusted)
			FRadiusRange PlanetRadiusRange = GetPlanetRadiusRange(PlanetType);
			double PlanetRadius = GenerationRandRange(PlanetRadiusRange.MinRadius, PlanetRadiusRange.MaxRadius);
			PlanetModel->Radius = PlanetRadius;

			// now calculate mass based on density and radius
			PlanetModel->Mass = PlanetDensity * (4.0 / 3.0) * PI * FMath::Pow(PlanetRadius, 3);

			const double EARTH_RADIUS_KM = 6371.0; // ������ ����� � ����������
			double RadiusKM = PlanetRadius * EARTH_RADIUS_KM;
			PlanetModel->RadiusKM = RadiusKM;

			// for Gravity
			PlanetModel->PlanetGravityStrength = PlanetModel->Mass / FMath::Pow(PlanetModel->Radius, 2);
			PlanetModel->AmountOfMoons = CalculateMoons(PlanetModel->Mass, PlanetType);

			// Moons orbits
			double PlanetMass = PlanetModel->Mass;
			double StarMass = StarModel->Mass;
			double PlanetToStarDistance = PlanetModel->Radius;
			double planetRadius = PlanetModel->Radius;

			double PlanetAtmosphereHeight = PlanetModel->RadiusKM / 30; // ������ ��������� �������
			PlanetModel->AtmosphereHeight = PlanetAtmosphereHeight;
			PlanetModel->PlanetHabitability =
				UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
					PlanetModel->PlanetType, PlanetModel->PlanetZone,
					PlanetModel->AtmosphereHeight);

			// ����������� ���������� - ������ ������� ���� ������ ���������
			const double MinOrbitRadius = planetRadius + PlanetAtmosphereHeight;
			double Eccentricity = 0;
			double RadiusHill = PlanetToStarDistance * (1 - Eccentricity) * pow(PlanetMass / (3 * StarMass), 1.0 / 3);
			const double MaxOrbitRadius = RadiusHill;
			TPair<double, double> OrbitRadiusPair;
			if (MinOrbitRadius > MaxOrbitRadius)
			{
				OrbitRadiusPair.Key = MaxOrbitRadius;
				OrbitRadiusPair.Value = MinOrbitRadius;
			}
			else
			{
				OrbitRadiusPair.Key = MinOrbitRadius;
				OrbitRadiusPair.Value = MaxOrbitRadius;
			}

			PlanetModel->MoonOrbitsRange = OrbitRadiusPair;
			UE_LOG(LogTemp, VeryVerbose, TEXT("Planet Moons Orbit Radius    - Min: %f, Max: %f x"), OrbitRadiusPair.Key,
			       OrbitRadiusPair.Value);

			const int AmountOfMoons = PlanetModel->AmountOfMoons;

			GeneratePlanetMoonsList(PlanetGenerator, MoonGenerator, PlanetModel, PlanetRadius, AmountOfMoons, PlanetIndex);
			TSharedPtr<FPlanetData> PlanetData = MakeShared<FPlanetData>(PlanetIndex, OrbitRadius, PlanetModel);
			PlanetarySystemModel->PlanetsList.Add(PlanetData);
		}
	}
	EnforcePlanetSurfaceClearance(*PlanetarySystemModel);
}


void UPlanetarySystemGenerator::GeneratePlanetarySystemModelByStar(
	TSharedPtr<FPlanetarySystemModel> PlanetarySystemModel, TSharedPtr<FStarModel> StarModel,
	UPlanetGenerator* PlanetGenerator, UMoonGenerator* MoonGenerator)
{
	if (bSeededGeneration) GenerationRandom.Initialize(GenerationSeed);
	// The same generator services every star in the hierarchy.  Never leak
	// scratch orbits/planets from the preceding system into this one.
	OrbitRadii.Reset();
	PlanetarySystemModel->PlanetsList.Reset();
	// ��������� ����������� ��� ����� �������
	// ������� ���� � ��� ��� �� ������
	// ���������� ������������ ����� ������  

	//FPlanetarySystemModel PlanetarySystemModel;
	PlanetarySystemModel->FullSpectralName = StarModel->FullSpectralName;

	// ������� ������� ����������� ��� ������� ���� ������
	PlanetProbability BaseProbability = BasePlanetProbabilities[StarModel->StellarType];
	// ������������ ����������� �� ������ ����� ������.
	PlanetProbability MassModifier;
	if (StarModel->StellarType == EStellarType::MainSequence)
	{
		MassModifier = 1 / (1 + FMath::Exp(-StarModel->Mass));
	}
	else
	{
		MassModifier = 1 / (1 + FMath::Exp(-StarModel->Mass / 10));
	}

	PlanetProbability FinalProbability = BaseProbability * MassModifier;
	UE_LOG(LogTemp, VeryVerbose, TEXT("FinalProbability: %f"), FinalProbability);
	bool HasPlanets = true; //GenerationRand() <= FinalProbability;
	//bool HasPlanets = false;//GenerationRand() <= FinalProbability;

	// ������� ���������� � ������
	UE_LOG(LogTemp, VeryVerbose, TEXT("HasPlanets: %s"), HasPlanets ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, VeryVerbose, TEXT("Star Information:"));
	UE_LOG(LogTemp, VeryVerbose, TEXT("Spectral Class: %s"), *UEnum::GetValueAsString(StarModel->SpectralClass));
	UE_LOG(LogTemp, VeryVerbose, TEXT("Stellar Class: %s"), *UEnum::GetValueAsString(StarModel->StellarType));
	UE_LOG(LogTemp, VeryVerbose, TEXT("Mass: %f Solar Masses"), StarModel->Mass);
	UE_LOG(LogTemp, VeryVerbose, TEXT("Radius: %f Solar Radii"), StarModel->Radius);

	if (HasPlanets)
	{
		const int32 MaxPlanetsAllowed = 20;
		int32 MinPlanetCount = 1;
		int32 MaxPlanetCount = FMath::Min(MaxPlanetsAllowed,
		                                  FMath::Max(1, FMath::RoundToInt(
			                                             StarModel->Mass * BasePlanetCount[StarModel->StellarType] *
			                                             MassModifier)));

		if (StarModel->StellarType == EStellarType::MainSequence && StarModel->SpectralClass == ESpectralClass::M)
		{
			MaxPlanetCount = 5;
		}

		int32 FinalPlanetCount = GenerationRandRange(MinPlanetCount, MaxPlanetCount);
		PlanetarySystemModel->AmountOfPlanets = FinalPlanetCount;
		PlanetarySystemModel->PlanetarySystemType = EPlanetarySystemType::Unknown; ///

		UE_LOG(LogTemp, VeryVerbose, TEXT("MinPlanetCount: %d"), MinPlanetCount);
		UE_LOG(LogTemp, VeryVerbose, TEXT("MaxPlanetCount: %d"), MaxPlanetCount);
		UE_LOG(LogTemp, VeryVerbose, TEXT("FinalPlanetCount: %d"), FinalPlanetCount);

		// ��� ����� ���� ��������� ��� ����������, ��������� �� ����������� ������
		double MinOrbitScalingFactor = 1.0f;
		double MaxOrbitScalingFactor = 10.0f;

		if (StarModel->StellarType == EStellarType::HyperGiant)
		{
			MaxOrbitScalingFactor = 5.0f; // ��������� ������������ ������ ��� �������������
		}
		else if (StarModel->StellarType == EStellarType::SuperGiant)
		{
			MaxOrbitScalingFactor = 6.0f; // ��������� ������������ ������ ��� �������������
		}

		double MinOrbit = StarModel->Mass * MinOrbitScalingFactor;
		double MaxOrbit = StarModel->Mass * MaxOrbitScalingFactor;

		// ��������� ��������� ������������� ��� ����� �������
		EOrbitDistributionType OrbitDistributionType = ChooseOrbitDistribution(StarModel->StellarType);
		PlanetarySystemModel->OrbitDistributionType = OrbitDistributionType;

		FString OrbitType = UEnum::GetValueAsString(OrbitDistributionType);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Orbit Distribution Type: %s"), *OrbitType);

		for (int i = 0; i < FinalPlanetCount; i++)
		{
			double OrbitDistributionValue;
			double OrbitRadius;
			switch (OrbitDistributionType)
			{
			case EOrbitDistributionType::Uniform:
				OrbitDistributionValue = GenerationRandRange(0.0, 1.0);
				break;
			case EOrbitDistributionType::Gaussian:
				OrbitDistributionValue = RandGauss();
				break;
			case EOrbitDistributionType::Chaotic:
				{
					OrbitDistributionValue = GenerationRandRange(MinOrbit, MaxOrbit);
					OrbitRadius = OrbitDistributionValue;
					break;
				}
			case EOrbitDistributionType::InnerOuter:
				{
					if (i < FinalPlanetCount / 2.0)
					{
						OrbitDistributionValue = GenerationRandRange(0.01, 0.5);
					}
					else
					{
						OrbitDistributionValue = GenerationRandRange(0.5, 1.0);
					}
					break;
				}
			case EOrbitDistributionType::Dense:
				OrbitDistributionValue = GenerationRandRange(0.01, 0.5);
				break;
			}

			if (OrbitDistributionType != EOrbitDistributionType::Chaotic)
			{
				OrbitRadius = FMath::Lerp(MinOrbit, MaxOrbit, OrbitDistributionValue);
			}

			// ��������� ������� ������������� � ������ ��������� �����
			OrbitRadii.Add(OrbitRadius);
		}

		OrbitRadii.Sort();
		EnforceMinimumPlanetOrbitSpacing(
			OrbitRadii, MinOrbit, MaxOrbit, StarModel->Radius, OrbitDistributionType);
		if (!OrbitRadii.IsEmpty())
		{
			MaxOrbit = FMath::Max(MaxOrbit, OrbitRadii.Last());
			StarModel->MinOrbit = FMath::Min(MinOrbit, OrbitRadii[0]);
			StarModel->MaxOrbit = MaxOrbit;
		}
		UE_LOG(LogTemp, VeryVerbose, TEXT("OrbitRadii Num: %d "), OrbitRadii.Num());

		// ������� ����������� � ������������ ������
		UE_LOG(LogTemp, VeryVerbose, TEXT("MinOrbit: %f, MaxOrbit: %f"), MinOrbit, MaxOrbit);

		// ��������� ��������� ����
		float HabitableZoneInner = sqrt(StarModel->Luminosity / 1.1);
		float HabitableZoneOuter = sqrt(StarModel->Luminosity / 0.53);

		// Star Dead zone
		double StarDeadZoneInner = 0; // ���������� �� ������
		double StarRadiusInAU = StarModel->Radius * 0.00465047;
		double StarDeadZoneOuter = StarRadiusInAU * 2;
		// ������������� �� ����������, ������ �������� ������� ������ � AU

		// Zones
		double HotZoneInner = 0;
		double HotZoneOuter = 0;
		double WarmZoneInner = 0;
		double WarmZoneOuter = 0;
		double ColdZoneInner = 0;
		double ColdZoneOuter = 0;
		double IceZoneInner = 0;
		double IceZoneOuter = 0;
		double GasGiantsZoneInner = 0;
		double GasGiantsZoneOuter = 0;
		double KuiperBeltZoneInner = 0;
		double KuiperBeltZoneOuter = 0;
		double InnerZoneInner = 0; // ���������� �� ������
		double InnerZoneOuter = 0; // ������������� �������� ������� ����
		double OuterZoneInner = 0; // ���������� �� ������� ���� ������� ��������
		double OuterZoneOuter = 0; // ��������� �������

		if (HabitableZoneOuter < MaxOrbit)
		{
			// ��������� ���� StarDeadZone
			StarDeadZoneInner = 0; // ���������� �� ������
			StarRadiusInAU = StarModel->Radius * 0.00465047;
			StarDeadZoneOuter = StarRadiusInAU * 2; // ������������� �� ����������, ������ �������� ������� ������ � AU

			// ��������� ������� ����
			HotZoneInner = StarDeadZoneOuter;
			HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

			// ��������� ������ ����
			WarmZoneInner = HotZoneOuter;
			WarmZoneOuter = HabitableZoneInner;

			// ��������� �������� ����
			ColdZoneInner = HabitableZoneOuter; //FMath::Max(HabitableZoneOuter, MinOrbit);
			ColdZoneOuter = (ColdZoneInner * 2 > MaxOrbit) ? MaxOrbit : ColdZoneInner * 2;

			// ��������� ������� ����
			IceZoneInner = ColdZoneOuter; //FMath::Max(ColdZoneOuter, MinOrbit);
			IceZoneOuter = (IceZoneInner * 2 > MaxOrbit) ? MaxOrbit : IceZoneInner * 2;

			// ��������� ���� ������� ��������
			GasGiantsZoneInner = FMath::Max(IceZoneOuter, MinOrbit);
			GasGiantsZoneOuter = (GasGiantsZoneInner * 2 > MaxOrbit) ? MaxOrbit : GasGiantsZoneInner * 2;

			// ��������� ���� ����� �������
			KuiperBeltZoneInner = FMath::Max(GasGiantsZoneOuter, MinOrbit);
			KuiperBeltZoneOuter = (KuiperBeltZoneInner * 2 > MaxOrbit) ? MaxOrbit : KuiperBeltZoneInner * 2;


			if (OrbitDistributionType == EOrbitDistributionType::InnerOuter)
			{
				// ��������� ���������� ����
				InnerZoneInner = StarDeadZoneOuter; //0; // ���������� �� ������
				InnerZoneOuter = HotZoneOuter; // ������������� �������� ������� ����

				// ��������� ������� ����
				OuterZoneInner = GasGiantsZoneOuter; // ���������� �� ������� ���� ������� ��������
				OuterZoneOuter = OuterZoneInner * 2; // ��������� �������

				PlanetarySystemModel->InnerPlanetZoneRadius = FZoneRadius(InnerZoneInner, InnerZoneOuter);
				PlanetarySystemModel->OuterPlanetZoneRadius = FZoneRadius(OuterZoneInner, OuterZoneOuter);
			}
		}
		else
		{
			StarDeadZoneInner = 0;
			StarRadiusInAU = StarModel->Radius * 0.00465047;
			StarDeadZoneOuter = StarRadiusInAU * 2;
			HotZoneInner = StarDeadZoneOuter;
			HotZoneOuter = StarDeadZoneOuter + (HabitableZoneInner - StarDeadZoneOuter) / 2;

			WarmZoneInner = HotZoneOuter;
			WarmZoneOuter = HabitableZoneInner;

			ColdZoneInner = WarmZoneOuter;
			ColdZoneOuter = FMath::Min(WarmZoneOuter * 2, MaxOrbit);

			if (ColdZoneOuter < MaxOrbit)
			{
				IceZoneInner = ColdZoneOuter;
				IceZoneOuter = FMath::Min(ColdZoneOuter * 2, MaxOrbit);
			}

			if (IceZoneOuter < MaxOrbit)
			{
				GasGiantsZoneInner = IceZoneOuter;
				GasGiantsZoneOuter = FMath::Min(IceZoneOuter * 2, MaxOrbit);
			}

			if (GasGiantsZoneOuter < MaxOrbit)
			{
				KuiperBeltZoneInner = GasGiantsZoneOuter;
				KuiperBeltZoneOuter = FMath::Min(GasGiantsZoneOuter * 2, MaxOrbit);
			}
		}

		PlanetarySystemModel->DeadZoneRadius = FZoneRadius(StarDeadZoneInner, StarDeadZoneOuter);
		PlanetarySystemModel->HabitableZoneRadius = FZoneRadius(HabitableZoneInner, HabitableZoneOuter);
		PlanetarySystemModel->ColdZoneRadius = FZoneRadius(ColdZoneInner, ColdZoneOuter);
		PlanetarySystemModel->IceZoneRadius = FZoneRadius(IceZoneInner, IceZoneOuter);
		PlanetarySystemModel->WarmZoneRadius = FZoneRadius(WarmZoneInner, WarmZoneOuter);
		PlanetarySystemModel->HotZoneRadius = FZoneRadius(HotZoneInner, HotZoneOuter);
		PlanetarySystemModel->GasGiantsZoneRadius = FZoneRadius(GasGiantsZoneInner, GasGiantsZoneOuter);
		PlanetarySystemModel->KuiperBeltZoneRadius = FZoneRadius(KuiperBeltZoneInner, KuiperBeltZoneOuter);

		// ������� ��������� ����
		UE_LOG(LogTemp, VeryVerbose, TEXT("Habitable Zone: %f AU - %f AU"), HabitableZoneInner, HabitableZoneOuter);

		// ������� ����������� ������
		// ������� ��� ������ ������
		for (int i = 0; i < OrbitRadii.Num(); ++i)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("Planet %d Orbit Radius: %f AU"), i + 1, OrbitRadii[i]);
		}

		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Dead Zone         - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->DeadZoneRadius.InnerRadius, PlanetarySystemModel->DeadZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Hot Zone          - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->HotZoneRadius.InnerRadius, PlanetarySystemModel->HotZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Warm Zone         - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->WarmZoneRadius.InnerRadius, PlanetarySystemModel->WarmZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Habitable Zone    - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->HabitableZoneRadius.InnerRadius,
		       PlanetarySystemModel->HabitableZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Cold Zone         - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->ColdZoneRadius.InnerRadius, PlanetarySystemModel->ColdZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Ice Zone          - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->IceZoneRadius.InnerRadius, PlanetarySystemModel->IceZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Gas Giants Zone   - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius,
		       PlanetarySystemModel->GasGiantsZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Kuiper Belt Zone  - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius,
		       PlanetarySystemModel->KuiperBeltZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Inner Planet Zone - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->InnerPlanetZoneRadius.InnerRadius,
		       PlanetarySystemModel->InnerPlanetZoneRadius.OuterRadius);
		UE_LOG(LogTemp, VeryVerbose, TEXT("Radius Outer Planet Zone - Inner: %f, Outer: %f AU"),
		       PlanetarySystemModel->OuterPlanetZoneRadius.InnerRadius,
		       PlanetarySystemModel->OuterPlanetZoneRadius.OuterRadius);

		/// TODO: To GeneratePlanetOrbits
		// populate planets list by new PlanetModel
		for (double OrbitRadius : OrbitRadii)
		{
			int PlanetIndex = OrbitRadii.IndexOfByKey(OrbitRadius);
			ResetBodyRandom(PlanetIndex, 0x504c4e54);

			//FPlanetModel PlanetModel; /// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);
			TSharedPtr<FPlanetModel> PlanetModel = MakeShared<FPlanetModel>();
			/// TODO: PlanetGenerator->GeneratePlanetModel(PlanetarySystemModel, OrbitRadius);


			PlanetModel->OrbitDistance = OrbitRadius;

			// planet temperature
			double PlanetTemperature = StarModel->SurfaceTemperature * sqrt(StarModel->Radius / (2 * OrbitRadius));
			PlanetModel->Temperature = PlanetTemperature; // to celestial body

			// ����������, � ����� ���� ��������� �������.
			EPlanetaryZoneType PlanetZone = DeterminePlanetZone(OrbitRadius, PlanetarySystemModel);
			PlanetModel->PlanetZone = PlanetZone;

			EPlanetType PlanetType = DeterminePlanetType(PlanetZone);
			PlanetModel->PlanetType = PlanetType;

			FDensityRange PlanetDensityRange = GetPlanetDensityRange(PlanetType);
			double PlanetDensity = GenerationRandRange(PlanetDensityRange.MinDensity, PlanetDensityRange.MaxDensity);
			PlanetModel->PlanetDensity = PlanetDensity;

			// assuming that radius is random in the range 1 - 2 Earth radii (this can be adjusted)
			FRadiusRange PlanetRadiusRange = GetPlanetRadiusRange(PlanetType);
			double PlanetRadius = GenerationRandRange(PlanetRadiusRange.MinRadius, PlanetRadiusRange.MaxRadius);
			PlanetModel->Radius = PlanetRadius;

			// now calculate mass based on density and radius
			PlanetModel->Mass = PlanetDensity * (4.0 / 3.0) * PI * FMath::Pow(PlanetRadius, 3);

			const double EARTH_RADIUS_KM = 6371.0; // ������ ����� � ����������
			double RadiusKM = PlanetRadius * EARTH_RADIUS_KM;
			PlanetModel->RadiusKM = RadiusKM;

			// for Gravity
			PlanetModel->PlanetGravityStrength = PlanetModel->Mass / FMath::Pow(PlanetModel->Radius, 2);
			PlanetModel->AmountOfMoons = CalculateMoons(PlanetModel->Mass, PlanetType);

			// Moons orbits
			double PlanetMass = PlanetModel->Mass;
			double StarMass = StarModel->Mass;
			double PlanetToStarDistance = PlanetModel->Radius;
			double planetRadius = PlanetModel->Radius;

			//const double EARTH_RADIUS_KM = 6371.0; // ������ ����� � ����������
			//const double EARTH_MASS = 1.0; // ����� ����� (� ������ ������ ����� ������� ���������� � �������� ����� �����)
			// const double ATMOSPHERE_HEIGHT_COEFFICIENT = 100.0;
			//double massFactor = FMath::Clamp(PlanetModel->Mass / EARTH_MASS, 0.1, 5.0);
			//double PlanetAtmosphereHeight = (PlanetModel->Radius ) * massFactor * ATMOSPHERE_HEIGHT_COEFFICIENT;
			/// TODO: PlanetAtmosphere //double planetAtmosphereHeight = PlanetModel.AtmosphereHeight; // ������ ��������� �������
			double PlanetAtmosphereHeight = PlanetModel->RadiusKM / 30; // ������ ��������� �������
			PlanetModel->AtmosphereHeight = PlanetAtmosphereHeight;
			PlanetModel->PlanetHabitability =
				UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
					PlanetModel->PlanetType, PlanetModel->PlanetZone,
					PlanetModel->AtmosphereHeight);

			// ����������� ���������� - ������ ������� ���� ������ ���������
			const double MinOrbitRadius = planetRadius + PlanetAtmosphereHeight;
			double eccentricity = 0;
			double RadiusHill = PlanetToStarDistance * (1 - eccentricity) * pow(PlanetMass / (3 * StarMass), 1.0 / 3);
			const double MaxOrbitRadius = RadiusHill;
			TPair<double, double> OrbitRadiusPair;
			if (MinOrbitRadius > MaxOrbitRadius)
			{
				OrbitRadiusPair.Key = MaxOrbitRadius;
				OrbitRadiusPair.Value = MinOrbitRadius;
			}
			else
			{
				OrbitRadiusPair.Key = MinOrbitRadius;
				OrbitRadiusPair.Value = MaxOrbitRadius;
			}

			PlanetModel->MoonOrbitsRange = OrbitRadiusPair;
			UE_LOG(LogTemp, VeryVerbose, TEXT("Planet Moons Orbit Radius    - Min: %f, Max: %f x"), OrbitRadiusPair.Key,
			       OrbitRadiusPair.Value);

			const int AmountOfMoons = PlanetModel->AmountOfMoons;
			TArray<TSharedPtr<FMoonData>> MoonsList{};
			TArray<float> MoonOrbits;
			// ������� ������ �� ����� ���������� ��������
			MoonOrbits.Reserve(AmountOfMoons);

			if (PlanetModel->PlanetType == EPlanetType::GasGiant
				|| PlanetModel->PlanetType == EPlanetType::IceGiant
				|| PlanetModel->PlanetType == EPlanetType::HotGiant
				|| PlanetModel->PlanetType == EPlanetType::Ocean
			)
			{
				// ������������� ����� �� 1 �� 10 �������� �������
				for (int i = 0; i < AmountOfMoons; i++)
				{
					double orbitRadius = GenerationRandRange(PlanetRadius * 1.0, PlanetRadius * 10.0);
					orbitRadius /= 40;
					MoonOrbits.Add(orbitRadius);
				}
			}
			else
			{
				double a = 1.5;
				double d = 1.4;
				// ������������ ������ �������-���� ��� ��������� ������
				for (int i = 0; i < AmountOfMoons; i++)
				{
					double orbitRadius = a + d * pow(2, i);
					orbitRadius = GenerationRandRange(orbitRadius * 0.9, orbitRadius * 1.3);
					MoonOrbits.Add(orbitRadius);
				}
			}
			MoonOrbits.Sort();

			for (double MoonOrbit : MoonOrbits)
			{
				//FMoonGenerationModel MoonModel;
				TSharedPtr<FMoonModel> MoonModel = MakeShared<FMoonModel>();


				EMoonType MoonType = MoonGenerator->GenerateMoonType(PlanetModel, GetGenerationRandom());

				// ��������� ���������� ��������� ����
				double MoonMass = MoonGenerator->CalculateRandomMoonMass(GetGenerationRandom());
				double MoonDensity = MoonGenerator->CalculateRandomMoonDensity(MoonType, GetGenerationRandom());
				double MoonRadius = MoonGenerator->CalculateMoonRadius(MoonDensity, MoonMass);
				double MoonGravity = MoonMass / FMath::Pow(MoonRadius, 2);
				/// TODO: to MoonGenerator->CalculateGravitationalForce(PlanetModel.Mass, MoonMass, MoonOrbit);

				// ������� ������ ����
				MoonModel->Type = MoonType;
				MoonModel->Mass = MoonMass;
				MoonModel->Radius = MoonRadius;
				MoonModel->RadiusKM = MoonModel->Radius * 6371.0;
				MoonModel->MoonDensity = MoonDensity; // TODO: To Parent 
				MoonModel->MoonGravity = MoonGravity;
				MoonModel->OrbitDistance = MoonOrbit;
				MoonModel->MoonAtmosphereHeight = MoonModel->RadiusKM / 30;
				MoonModel->PlanetType = UMoonGenerator::ResolveSurfaceType(*MoonModel);
				MoonModel->PlanetHabitability =
					UAPSPlanetHabitabilityLibrary::ResolveDefaultHabitability(
						MoonModel->PlanetType, PlanetModel->PlanetZone,
						MoonModel->MoonAtmosphereHeight);

				// ������� ������ � ����
				int MoonIndex = MoonOrbits.IndexOfByKey(MoonOrbit);
				TSharedPtr<FMoonData> MoonData = MakeShared<FMoonData>(MoonIndex + 1, MoonOrbit, MoonModel);

				// ��������� ������ � ���� � ������
				MoonsList.Add(MoonData);
			}


			//// ���������� ������� ����� �������� (��� ���������� �������, � ���������� ��� �������)
			//FVector L1_Position = FVector(OrbitRadius * (1 - pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
			//FVector L2_Position = FVector(OrbitRadius * (1 + pow(PlanetMass / 3, 1.0 / 3.0)), 0, 0);
			//FVector L3_Position = FVector(-OrbitRadius * (1 + 5 * PlanetMass / 12), 0, 0);
			//FVector L4_Position = FVector(OrbitRadius * cos(PI / 3), OrbitRadius * sin(PI / 3), 0);
			//FVector L5_Position = FVector(OrbitRadius * cos(PI / 3), -OrbitRadius * sin(PI / 3), 0);
			//PlanetModel->LagrangePoints.Add(L1_Position);
			//PlanetModel->LagrangePoints.Add(L2_Position);
			//PlanetModel->LagrangePoints.Add(L3_Position);
			//PlanetModel->LagrangePoints.Add(L4_Position);
			//PlanetModel->LagrangePoints.Add(L5_Position);

			for (int32 i = 0; i <= (int32)EOrbitHeight::VeryHighOrbit; ++i)
			{
				FOrbitInfo OrbitInfo{};
				OrbitInfo.OrbitHeightType = (EOrbitHeight)i;
				OrbitInfo.OrbitHeight = PlanetGenerator->CalculateOrbitHeight(OrbitInfo.OrbitHeightType, PlanetRadius);
				PlanetModel->Orbits.Add(OrbitInfo);
			}

			PlanetModel->MoonsList = MoonsList;
			EnforceSafeMoonOrbitSpacing(*PlanetModel);
			TSharedPtr<FPlanetData> PlanetData = MakeShared<FPlanetData>(PlanetIndex, OrbitRadius, PlanetModel);
			PlanetarySystemModel->PlanetsList.Add(PlanetData);
		}
	}
	else
	{
		PlanetarySystemModel->AmountOfPlanets = 0;
		PlanetarySystemModel->PlanetarySystemType = EPlanetarySystemType::NoPlanetSystem;
	}
	EnforcePlanetSurfaceClearance(*PlanetarySystemModel);

	OrbitRadii.Reset();
	//return PlanetarySystemModel;
}


void UPlanetarySystemGenerator::GeneratePlanetOrbits()
{
}


int UPlanetarySystemGenerator::CalculateMoons(double PlanetMass, EPlanetType PlanetType)
{
	// Calculate the base number of moons based on the planet mass
	int BaseMoonCount = PlanetMass / 10.0;

	// Generate a random number between 0 and 1
	double RandomNumber = GenerationRand();

	if (PlanetType == EPlanetType::Unknown)
	{
		return GenerationRandRange(0, 3);
	}

	if (PlanetType == EPlanetType::Ocean || PlanetType == EPlanetType::SuperEarth)
	{
		return GenerationRandRange(0, 1);
	}

	// Apply the chance to have no moons
	if (RandomNumber < 0.3)
	{
		return 0;
	}
	else if (BaseMoonCount > 10)
	{
		return
			PlanetType == EPlanetType::GasGiant ||
			PlanetType == EPlanetType::IceGiant ||
			PlanetType == EPlanetType::HotGiant
				? GenerationRandRange(0, 10)
				: GenerationRandRange(0, 5);
	}
	{
		// Otherwise, the number of moons is the base moon count plus a random number
		return BaseMoonCount + GenerationRandRange(0, 3);
	}
}

FRadiusRange UPlanetarySystemGenerator::GetPlanetRadiusRange(EPlanetType PlanetType)
{
	if (PlanetRadiusRanges.Contains(PlanetType))
	{
		return PlanetRadiusRanges[PlanetType];
	}

	// ���������� �������� �� ��������� ��� ����������� ������
	return FRadiusRange(0.2, 12.6);
}

FDensityRange UPlanetarySystemGenerator::GetPlanetDensityRange(EPlanetType PlanetType)
{
	if (PlanetDensityRanges.Contains(PlanetType))
	{
		return PlanetDensityRanges[PlanetType];
	}

	// default value if planet type is not in map
	return FDensityRange(5.0, 5.0);
}

EPlanetaryZoneType UPlanetarySystemGenerator::DeterminePlanetZone(double OrbitRadius,
                                                                  TSharedPtr<FPlanetarySystemModel>
                                                                  PlanetarySystemModel)
{
	/// TODO: To Struct
	/*for (FZone Zone : Zones)
	{
	    if (OrbitRadius >= Zone.Radius.InnerRadius && OrbitRadius <= Zone.Radius.OuterRadius)
	    {
	        return Zone.ZoneType;
	    }
	}*/

	if (OrbitRadius >= PlanetarySystemModel->DeadZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		DeadZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::DeadZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->HotZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		HotZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::HotZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->WarmZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		WarmZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::WarmZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->HabitableZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		HabitableZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::HabitableZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->ColdZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		ColdZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::ColdZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->IceZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		IceZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::IceZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->GasGiantsZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		GasGiantsZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::GasGiantsZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->KuiperBeltZoneRadius.InnerRadius && OrbitRadius <= PlanetarySystemModel->
		KuiperBeltZoneRadius.OuterRadius)
	{
		return EPlanetaryZoneType::KuiperBeltZone;
	}

	///OrbitRadius *= 149597870; // * 100000;

	/*if (OrbitRadius <= PlanetarySystemModel->StarDeadZoneOuter)
	{
	    return EPlanetaryZoneType::DeadZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->StarDeadZoneOuter && OrbitRadius <= PlanetarySystemModel->HotZoneOuter)
	{
	    return EPlanetaryZoneType::HotZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->HotZoneOuter && OrbitRadius <= PlanetarySystemModel->WarmZoneOuter)
	{
	    return EPlanetaryZoneType::WarmZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->WarmZoneOuter && OrbitRadius <= PlanetarySystemModel->HabitableZoneOuter)
	{
	    return EPlanetaryZoneType::HabitableZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->HabitableZoneOuter && OrbitRadius <= PlanetarySystemModel->ColdZoneOuter)
	{
	    return EPlanetaryZoneType::ColdZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->ColdZoneOuter && OrbitRadius <= PlanetarySystemModel->IceZoneOuter)
	{
	    return EPlanetaryZoneType::IceZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->IceZoneOuter && OrbitRadius <= PlanetarySystemModel->GasGiantsZoneOuter)
	{
	    return EPlanetaryZoneType::GasGiantsZone;
	}
	if (OrbitRadius >= PlanetarySystemModel->GasGiantsZoneOuter && OrbitRadius <= PlanetarySystemModel->KuiperBeltZoneOuter)
	{
	    return EPlanetaryZoneType::KuiperBeltZone;
	}*/

	// ���� �� �� ����� ���������� ����, ���������� Unknown.
	return EPlanetaryZoneType::Unknown;
}

EPlanetType UPlanetarySystemGenerator::DeterminePlanetType(EPlanetaryZoneType PlanetZone)
{
	// �������� ������ ������������ ��� ����� ������ � ���� ����.
	const TArray<FPlanetTypeProbability>* PlanetTypeProbabilities =
		ZonePlanetProbabilities.Find(PlanetZone);
	if (!PlanetTypeProbabilities || PlanetTypeProbabilities->IsEmpty())
	{
		return EPlanetType::Unknown;
	}

	// The tables contain relative weights; several intentionally do not sum to 1.
	// Sampling them as absolute thresholds produced Unknown planets and biased the
	// first entry, so normalize against the actual positive weight sum.
	double TotalWeight = 0.0;
	for (const FPlanetTypeProbability& Entry : *PlanetTypeProbabilities)
	{
		if (FMath::IsFinite(Entry.Probability) && Entry.Probability > 0.0f)
		{
			TotalWeight += Entry.Probability;
		}
	}
	if (TotalWeight <= UE_DOUBLE_SMALL_NUMBER)
	{
		return EPlanetType::Unknown;
	}

	const double RandomWeight = GenerationRandRange(0.0, TotalWeight);
	double CumulativeWeight = 0.0;
	EPlanetType LastConcreteType = EPlanetType::Unknown;
	for (const FPlanetTypeProbability& Entry : *PlanetTypeProbabilities)
	{
		if (!FMath::IsFinite(Entry.Probability) || Entry.Probability <= 0.0f)
		{
			continue;
		}
		LastConcreteType = Entry.PlanetType;
		CumulativeWeight += Entry.Probability;
		if (RandomWeight <= CumulativeWeight)
		{
			return Entry.PlanetType;
		}
	}
	return LastConcreteType;
}

double UPlanetarySystemGenerator::RandGauss()
{
	double U = GenerationRand();
	double V = GenerationRand();
	double X = sqrt(-2.0 * log(U)) * cos(2.0 * PI * V);
	return X * 0.15 + 0.5;
}

EOrbitDistributionType UPlanetarySystemGenerator::ChooseDistributionType(
	EStellarType StellarClass, float StarMass, float MinOrbit, float MaxOrbit)
{
	EOrbitDistributionType OrbitDistributionType;

	if (StellarClass == EStellarType::MainSequence)
	{
		return ChooseOrbitDistribution(StellarClass);
	}
	else if (StellarClass == EStellarType::WhiteDwarf || StellarClass == EStellarType::SubDwarf)
	{
		// ���� � ��� ��������� ������, �� ���������� Dense
		OrbitDistributionType = EOrbitDistributionType::Dense;
	}
	else if (StellarClass == EStellarType::Giant || StellarClass == EStellarType::SuperGiant || StellarClass ==
		EStellarType::HyperGiant)
	{
		// ���� � ��� ������� ������, �� ���������� InnerOuter
		OrbitDistributionType = EOrbitDistributionType::InnerOuter;
	}
	else
	{
		// ���� � ��� ������ �������� �������, ����� ������������ ������ ���� ������������� � ����������� �� �����
		if (StarMass < 0.5f)
		{
			OrbitDistributionType = EOrbitDistributionType::Uniform;
		}
		else if (StarMass < 1.0f)
		{
			OrbitDistributionType = EOrbitDistributionType::Gaussian;
		}
		else if (MaxOrbit - MinOrbit < 5.0f)
		// ��������, ��������� ������ ���� � ����� ����������� � ������������ ������
		{
			OrbitDistributionType = EOrbitDistributionType::Dense;
			// ���� ������ ������, ����� ������������ ������� �������������
		}
		else
		{
			OrbitDistributionType = EOrbitDistributionType::Chaotic;
		}
	}
	return OrbitDistributionType;
}


EOrbitDistributionType UPlanetarySystemGenerator::ChooseOrbitDistribution(EStellarType StellarClass)
{
	// �������� ���� ������������ ��� ������� ������ ������
	auto probabilities = StellarOrbitDistributions[StellarClass];

	// ���������� ��������� ����� � ��������� �� 0 �� 1
	float randomValue = GenerationRand();

	// ���� �� ���� ����� ����-�������� � ������� ������������
	for (auto& keyValue : probabilities)
	{
		// ��������� ��������� ����� �� ����������� �������� ����
		randomValue -= keyValue.Value;

		// ���� ��������� ����� ����� ������ ����, �������� ������� ���
		if (randomValue < 0)
		{
			return keyValue.Key;
		}
	}

	// ���������� ��������� ���, ���� ���-�� ����� �� ���
	return probabilities.end().Key();
}

void UPlanetarySystemGenerator::SetAstroLocation(int StarNumber, APlanetarySystem* NewPlanetarySystem)
{
	// NewPlanetarySystem->Get
}

void UPlanetarySystemGenerator::GenerateCustomPlanetarySystem()
{
}

int UPlanetarySystemGenerator::DetermineMaxPlanets(EStellarType StellarClass, FStarModel StarModel)
{
	int MaxPlanets;

	// � ��������� ����� ����� ������ ��� ������
	if (StellarClass == EStellarType::WhiteDwarf || StellarClass == EStellarType::Neutron)
	{
		return 0;
	}

	// ������ ������� ���������� ������ � ����������� �� ������ ������
	switch (StellarClass)
	{
	case EStellarType::HyperGiant:
	case EStellarType::SuperGiant:
	case EStellarType::Giant:
		MaxPlanets = 10; // ������� ���������� ��� ���������� �����
		break;
	case EStellarType::MainSequence:
		MaxPlanets = 5; // ������� ���������� ��� ����� ������� ������������������
		break;
	default:
		MaxPlanets = 3; // ������� ���������� ��� ��������� ����� �����
		break;
	}

	// ������������ ���������� ������ � ����������� �� ����� � �������� ������
	MaxPlanets = MaxPlanets + StarModel.Mass * 0.5 + 0.2 * 0.1;

	// ��������� ������������ ���������� ������, ����� ��� ���� ��������
	if (MaxPlanets > 20)
	{
		MaxPlanets = 20;
	}

	return MaxPlanets;
}

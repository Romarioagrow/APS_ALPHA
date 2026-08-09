#include "GalaxyGenerator.h"
#include "Octree.h"
#include <cmath>
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Rendering/APSStarRenderStabilitySubsystem.h"
#include "APS_ALPHA/Core/Structs/GalaxyModel.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "Engine/StaticMesh.h"

void UGalaxyGenerator::GenerateRandomGalaxyModel(TSharedPtr<FGalaxyModel> GalaxyModel)
{
	if (!GalaxyModel.IsValid())
	{
		return;
	}

	static constexpr EGalaxyType GalaxyTypes[] = {
		EGalaxyType::Elliptical,
		EGalaxyType::Lenticular,
		EGalaxyType::Spiral,
		EGalaxyType::BarredSpiral,
		EGalaxyType::Irregular,
		EGalaxyType::Peculiar
	};
	GalaxyModel->GalaxyType = GalaxyTypes[FMath::RandRange(0, UE_ARRAY_COUNT(GalaxyTypes) - 1)];

	switch (GalaxyModel->GalaxyType)
	{
	case EGalaxyType::Elliptical:
		GalaxyModel->GalaxyClass = static_cast<EGalaxyClass>(FMath::RandRange(
			static_cast<int32>(EGalaxyClass::E0), static_cast<int32>(EGalaxyClass::E7)));
		break;
	case EGalaxyType::Lenticular:
		GalaxyModel->GalaxyClass = EGalaxyClass::S0;
		break;
	case EGalaxyType::Spiral:
		GalaxyModel->GalaxyClass = static_cast<EGalaxyClass>(FMath::RandRange(
			static_cast<int32>(EGalaxyClass::Sa), static_cast<int32>(EGalaxyClass::Sd)));
		break;
	case EGalaxyType::BarredSpiral:
		GalaxyModel->GalaxyClass = static_cast<EGalaxyClass>(FMath::RandRange(
			static_cast<int32>(EGalaxyClass::SBa), static_cast<int32>(EGalaxyClass::SBd)));
		break;
	case EGalaxyType::Irregular:
	case EGalaxyType::Peculiar:
	default:
		GalaxyModel->GalaxyClass = EGalaxyClass::Irr;
		break;
	}

	// The count describes the logical catalog. It does not allocate an actor or
	// an array entry per star; rendering remains governed by the separate HISM budget.
	GalaxyModel->StarsCount = FMath::RandRange(75000000, 350000000);
	GalaxyModel->GalaxySize = FMath::RandRange(180, 900);
	GalaxyModel->StarsDensity = FMath::FRandRange(4.0, 45.0);
}

FGalaxyModel UGalaxyGenerator::GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass)
{
	FGalaxyModel Model;
	Model.GalaxyType = GalaxyType;
	Model.GalaxyClass = GalaxyGlass;
	Model.StarsCount = 100000000;
	Model.GalaxySize = 250;
	Model.StarsDensity = 10.0;
	return Model;
}

namespace APSGalaxyVisuals
{
	double RadiusForSpectralClass(const ESpectralClass SpectralClass)
	{
		switch (SpectralClass)
		{
		case ESpectralClass::O: return 9.0;
		case ESpectralClass::B: return 4.8;
		case ESpectralClass::A: return 2.1;
		case ESpectralClass::F: return 1.35;
		case ESpectralClass::G: return 1.0;
		case ESpectralClass::K: return 0.78;
		case ESpectralClass::M: return 0.42;
		case ESpectralClass::L: return 0.18;
		case ESpectralClass::T: return 0.14;
		case ESpectralClass::Y: return 0.11;
		case ESpectralClass::NS: return 0.08;
		case ESpectralClass::PS: return 2.8;
		case ESpectralClass::BH: return 0.12;
		default: return 1.0;
		}
	}

	double LuminosityForSpectralClass(const ESpectralClass SpectralClass)
	{
		switch (SpectralClass)
		{
		case ESpectralClass::O: return 60000.0;
		case ESpectralClass::B: return 1200.0;
		case ESpectralClass::A: return 55.0;
		case ESpectralClass::F: return 6.0;
		case ESpectralClass::G: return 1.0;
		case ESpectralClass::K: return 0.35;
		case ESpectralClass::M: return 0.045;
		case ESpectralClass::L: return 0.008;
		case ESpectralClass::T: return 0.003;
		case ESpectralClass::Y: return 0.001;
		case ESpectralClass::NS: return 35.0;
		case ESpectralClass::PS: return 140.0;
		case ESpectralClass::BH: return 0.0;
		default: return 1.0;
		}
	}
}

void UGalaxyGenerator::GenerateGalaxyOctreeStars(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy,
	TSharedPtr<FGalaxyModel> GalaxyModel, const int32 RenderedStarBudget, const int32 GenerationSeed,
	const bool bUsePreviewPresentation)
{
	if (!IsValid(StarGenerator) || !IsValid(NewGalaxy) || !IsValid(NewGalaxy->StarMeshInstances)
		|| !GalaxyModel.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.WorldGeneration] Cannot generate galaxy: invalid input"));
		return;
	}

	const int64 ModeledStarCount = FMath::Max<int64>(1, GalaxyModel->StarsCount);
	const int32 RenderedStarCount = static_cast<int32>(FMath::Clamp<int64>(
		RenderedStarBudget, 1, ModeledStarCount));
	const double DensityScale = FMath::Sqrt(10.0 / FMath::Clamp(GalaxyModel->StarsDensity, 0.01, 1000.0));
	const double GalaxyRadius = FMath::Max(50000.0,
		static_cast<double>(FMath::Max(GalaxyModel->GalaxySize, 1)) * 50000.0) * DensityScale;

	NewGalaxy->StarCatalog.GenerationSeed = GenerationSeed;
	NewGalaxy->StarCatalog.ModeledStarCount = ModeledStarCount;
	NewGalaxy->StarCatalog.RenderedSampleCount = 0;
	NewGalaxy->StarCatalog.GalaxySize = GalaxyModel->GalaxySize;
	NewGalaxy->StarCatalog.StarDensity = GalaxyModel->StarsDensity;
	NewGalaxy->StarCatalog.GalaxyType = GalaxyModel->GalaxyType;
	NewGalaxy->StarCatalog.GalaxyClass = GalaxyModel->GalaxyClass;
	NewGalaxy->StarCatalog.CatalogHalfExtent = FVector(GalaxyRadius);

	NewGalaxy->StarMeshInstances->ClearInstances();
	NewGalaxy->StarMeshInstances->NumCustomDataFloats = 6;
	NewGalaxy->StarMeshInstances->PreAllocateInstancesMemory(RenderedStarCount);

	const UStaticMesh* ProxyMesh = NewGalaxy->StarMeshInstances->GetStaticMesh();
	const double ProxyMeshRadius = IsValid(ProxyMesh)
		? FMath::Max(static_cast<double>(ProxyMesh->GetBounds().SphereRadius), 1.0) : 50.0;
	const double SparseSampleCompensation = FMath::Clamp(
		FMath::Sqrt(1800.0 / FMath::Max(RenderedStarCount, 1)), 0.85, 1.60);
	// Physical stellar radii are many orders of magnitude below the galaxy frame
	// after full-scale preview normalization. Keep model radii untouched and apply
	// a preview-only screen-stable impostor floor of roughly one rendered pixel.
	const double MinimumPreviewProxyRadius = bUsePreviewPresentation
		? GalaxyRadius * 0.00055 * SparseSampleCompensation : 0.0;
	const double MinimumPreviewProxyScale = MinimumPreviewProxyRadius / ProxyMeshRadius;
	FBox RenderedSampleBounds(EForceInit::ForceInit);

	for (int32 RenderIndex = 0; RenderIndex < RenderedStarCount; ++RenderIndex)
	{
		// One deterministic sample per catalog stratum gives stable coverage of the
		// entire logical galaxy even when only a tiny percentage is rendered.
		const int64 StratumBegin = ModeledStarCount * RenderIndex / RenderedStarCount;
		const int64 StratumEnd = ModeledStarCount * (RenderIndex + 1) / RenderedStarCount;
		const int64 StratumSize = FMath::Max<int64>(1, StratumEnd - StratumBegin);
		const uint32 SampleHash = HashCombine(GetTypeHash(GenerationSeed), GetTypeHash(RenderIndex));
		const int64 CatalogIndex = StratumBegin
			+ static_cast<int64>(static_cast<uint64>(SampleHash) % static_cast<uint64>(StratumSize));

		FGalaxyCatalogStarRecord StarRecord;
		if (!NewGalaxy->StarCatalog.ResolveStar(CatalogIndex, StarRecord))
		{
			continue;
		}

		const double PhysicalRadius = APSGalaxyVisuals::RadiusForSpectralClass(StarRecord.SpectralClass);
		FTransform StarTransform;
		StarTransform.SetLocation(StarRecord.GalaxyLocalLocation);
		const double VisualScale = FMath::Max(
			UStarGenerator::GetFarStarVisualRadius(PhysicalRadius), MinimumPreviewProxyScale);
		StarTransform.SetScale3D(FVector(VisualScale));
		RenderedSampleBounds += StarRecord.GalaxyLocalLocation;
		// Catalog coordinates are galaxy-local.  Passing world-space here applies
		// the parent transform twice once the generated hierarchy is moved.
		const int32 InstanceIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, false);

		const FLinearColor ColorValue = UStarGenerator::GetStarColor(
			StarRecord.SpectralClass, StarRecord.SpectralSubclass);
		const double PhysicalEmission = StarGenerator->CalculateEmission(
			static_cast<float>(APSGalaxyVisuals::LuminosityForSpectralClass(StarRecord.SpectralClass) * 25.0));
		const double VisualEmission = UStarGenerator::GetFarStarVisualEmission(
			PhysicalRadius, PhysicalEmission, VisualScale);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 0, ColorValue.R, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 1, ColorValue.G, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 2, ColorValue.B, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 3, VisualEmission, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 4,
			static_cast<float>((StarRecord.GenerationSeed & 0xffff) / 65535.0), false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(InstanceIndex, 5,
			StarRecord.bPotentialStarSystem ? 1.0f : 0.0f, false);
	}

	NewGalaxy->StarCatalog.RenderedSampleCount = NewGalaxy->StarMeshInstances->GetInstanceCount();
	UAPSStarRenderStabilitySubsystem::StabilizeInstances(NewGalaxy->StarMeshInstances);
	const FVector SampleCenter = RenderedSampleBounds.IsValid
		? RenderedSampleBounds.GetCenter() : FVector::ZeroVector;
	const FVector SampleExtent = RenderedSampleBounds.IsValid
		? RenderedSampleBounds.GetExtent() : FVector::ZeroVector;
	UE_LOG(LogTemp, Log,
		TEXT("[APS.GalaxyPreview] type=%d modeled=%lld rendered=%d seed=%d catalogRadius=%.3e "
			"sampleCenter=%s sampleExtent=%s minProxyRadius=%.3e minProxyScale=%.3e preview=%d"),
		static_cast<int32>(GalaxyModel->GalaxyType), ModeledStarCount,
		NewGalaxy->StarCatalog.RenderedSampleCount, GenerationSeed, GalaxyRadius,
		*SampleCenter.ToCompactString(), *SampleExtent.ToCompactString(),
		MinimumPreviewProxyRadius, MinimumPreviewProxyScale, bUsePreviewPresentation ? 1 : 0);
}

void UGalaxyGenerator::GenerateLegacyGalaxyOctreeStars(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy,
                                                 TSharedPtr<FGalaxyModel> GalaxyModel)
{
	// Создаем октодерево
	double GalaxyOctreeHalfDimension = GalaxyModel->GalaxySize;
	Octree* galaxyOctree = new Octree(FVector(0), FVector(GalaxyOctreeHalfDimension));

	// Определение функции генерации в зависимости от типа галактики
	FVector (UGalaxyGenerator::*generateStar)(EGalaxyClass, double, double);
	switch (GalaxyModel->GalaxyType)
	{
	case EGalaxyType::Elliptical:
		generateStar = &UGalaxyGenerator::GenerateStarInEllipticalGalaxy;
		break;
	case EGalaxyType::Lenticular:
		generateStar = &UGalaxyGenerator::GenerateStarInLenticularGalaxy;
		break;
	case EGalaxyType::Spiral:
		generateStar = &UGalaxyGenerator::GenerateStarInSpiralGalaxy;
		break;
	case EGalaxyType::BarredSpiral:
		generateStar = &UGalaxyGenerator::GenerateStarInBarredSpiralGalaxy;
		break;
	case EGalaxyType::Irregular:
		generateStar = &UGalaxyGenerator::GenerateStarInIrregularGalaxy;
		break;
	case EGalaxyType::Peculiar:
		generateStar = &UGalaxyGenerator::GenerateStarInPeculiarGalaxy;
		break;
	default:
		generateStar = &UGalaxyGenerator::GenerateStarInEllipticalGalaxy; // или любую другую функцию по умолчанию
		break;
	}

	double StarsCount = GalaxyModel->StarsCount;
	int32 SkippedStarCount = 0;
	constexpr int32 MaxPlacementAttempts = 96;
	NewGalaxy->StarMeshInstances->NumCustomDataFloats = 6;
	NewGalaxy->StarMeshInstances->PreAllocateInstancesMemory(FMath::Max(0, FMath::FloorToInt(StarsCount)));
	for (int i = 0; i < StarsCount; i++)
	{
		TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();
		StarGenerator->GenerateRandomStarModel(StarModel);
		bool bPositionAvailable = false;
		FVector Position = FVector::ZeroVector;

		double LightYearInKm = 9.461e12;
		double UnitInKm = 6963.4;
		double LightYearInUnrealUnits = LightYearInKm / UnitInKm;
		double AstroScaleCoeff = GalaxyModel->StarsDensity;
		LightYearInUnrealUnits /= AstroScaleCoeff;

		for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
		{
			Position = (this->*generateStar)(GalaxyModel->GalaxyClass, LightYearInUnrealUnits, StarModel->Radius);
			if (!galaxyOctree->SpaceOccupied(Position, StarModel->Radius))
			{
				bPositionAvailable = true;
				break;
			}
		}
		if (!bPositionAvailable)
		{
			++SkippedStarCount;
			continue;
		}

		// Вставляем звезду в октодерево
		galaxyOctree->InsertStar(Position, StarModel->Radius);

		FTransform StarTransform;
		StarTransform.SetLocation(Position);
		StarTransform.SetScale3D(FVector(UStarGenerator::GetFarStarVisualRadius(StarModel->Radius)));
		int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, false);

		FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel->SpectralClass, StarModel->SpectralSubclass);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B, false);
		double StarEmission = UStarGenerator::GetFarStarVisualEmission(StarModel->Radius,
			StarGenerator->CalculateEmission(StarModel->Luminosity * 25));
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission, false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 4, FMath::FRand(), false);
		NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 5, 0.0f, false);
	}
	if (SkippedStarCount > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.WorldGeneration] Galaxy skipped %d visual stars after bounded placement retries"),
			SkippedStarCount);
	}
	UAPSStarRenderStabilitySubsystem::StabilizeInstances(NewGalaxy->StarMeshInstances);
	delete galaxyOctree;
}

FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, double StarDistance,
                                                         double StarRadius)
{
	int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
	FVector Direction;

	do
	{
		Direction.X = FMath::FRandRange(-1.0, 1.0);
		Direction.Y = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
		Direction.Z = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
	}
	while (Direction.SizeSquared() > 1.0f); // If the direction is outside the unit sphere, try again

	double RandomDistance = FMath::FRand() * StarDistance + StarRadius;

	return Direction.GetSafeNormal() * RandomDistance;
}


FVector UGalaxyGenerator::GenerateStarInLenticularGalaxy(EGalaxyClass GalaxyClass, double StarDistance,
                                                         double StarRadius)
{
	// В линзовидных галактиках звезды распределены в диске с небольшой центральной выпуклостью.
	// Мы можем сгенерировать эту форму, добавив небольшой случайный шум к радиусу звезды в центре диска.

	int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::S0);
	FVector Direction;

	do
	{
		Direction.X = FMath::FRandRange(-1.0, 1.0);
		Direction.Y = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
		Direction.Z = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
	}
	while (Direction.SizeSquared() > 1.0f);

	double RandomDistance = FMath::FRand() * StarDistance + StarRadius + FMath::FRandRange(
		-StarRadius / 2, StarRadius / 2);

	return Direction.GetSafeNormal() * RandomDistance;
}

FVector UGalaxyGenerator::GenerateStarInSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
	// В спиральных галактиках звезды распределены вдоль спиральных рукавов. 
	// Для генерации этой формы мы можем использовать логарифмическую спираль.

	double angle = FMath::FRandRange(0, 2 * PI);
	double RandomDistance = StarDistance * exp(angle / tan(0.1 * PI));
	// Adjust this to change the tightness of the spiral

	FVector Direction(cos(angle), sin(angle), FMath::FRandRange(-1.0, 1.0));

	return Direction.GetSafeNormal() * RandomDistance;
}

FVector UGalaxyGenerator::GenerateStarInBarredSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance,
                                                           double StarRadius)
{
	// In barred spiral galaxies, stars are distributed along a central bar and spiral arms that extend from the ends of the bar.
	// We can generate this shape by adding a random offset to the X position of the star in a spiral galaxy.

	FVector position = GenerateStarInSpiralGalaxy(GalaxyClass, StarDistance, StarRadius);

	position.X += FMath::FRandRange(-StarDistance / 2, StarDistance / 2); // Adjust this to change the length of the bar

	return position;
}

FVector UGalaxyGenerator::GenerateStarInIrregularGalaxy(EGalaxyClass GalaxyClass, double StarDistance,
                                                        double StarRadius)
{
	// In irregular galaxies, stars are distributed randomly.
	// We can generate this shape by generating a random position for the star within the bounding box of the galaxy.

	FVector Direction;

	do
	{
		Direction.X = FMath::FRandRange(-1.0, 1.0);
		Direction.Y = FMath::FRandRange(-1.0, 1.0);
		Direction.Z = FMath::FRandRange(-1.0, 1.0);
	}
	while (Direction.SizeSquared() > 1.0f);

	double RandomDistance = FMath::FRand() * StarDistance + StarRadius;

	return Direction.GetSafeNormal() * RandomDistance;
}

FVector UGalaxyGenerator::GenerateStarInPeculiarGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
	// Peculiar galaxies are galaxies that cannot be easily classified into any of the other categories.
	// For this reason, generating stars in a peculiar galaxy can involve any type of shape and distribution. 
	// For the sake of simplicity, we will generate stars randomly just like in an irregular galaxy.

	return GenerateStarInIrregularGalaxy(GalaxyClass, StarDistance, StarRadius);
}

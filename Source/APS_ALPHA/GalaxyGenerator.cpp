// Fill out your copyright notice in the Description page of Project Settings.


#include "GalaxyGenerator.h"
#include "Galaxy.h"
#include "Octree.h"
#include "StarGenerationModel.h"
#include <cmath>

void UGalaxyGenerator::GenerateRandomGalaxyModel(TSharedPtr<FGalaxyModel> GalaxyModel)
{
	//return FGalaxyModel();
}

FGalaxyModel UGalaxyGenerator::GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass)
{
	return FGalaxyModel();
}

void UGalaxyGenerator::GenerateGalaxyOctreeStars(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy, TSharedPtr<FGalaxyModel> GalaxyModel)
{
    // Создаем октодерево
    double GalaxyOctreeHalfDimension = GalaxyModel->GalaxySize;
    Octree* galaxyOctree = new Octree(FVector(0), FVector(GalaxyOctreeHalfDimension));

    // Определение функции генерации в зависимости от типа галактики
    FVector(UGalaxyGenerator:: * generateStar)(EGalaxyClass, double, double);
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
    for (int i = 0; i < StarsCount; i++)
    {
        TSharedPtr<FStarModel> StarModel = MakeShared<FStarModel>();
        StarGenerator->GenerateRandomStarModel(StarModel);
        bool spaceOccupied = true;
        FVector position;

        double LightYearInKm = 9.461e12;
        double UnitInKm = 6963.4;
        double LightYearInUnrealUnits = LightYearInKm / UnitInKm;
        double AstroScaleCoeff = GalaxyModel->StarsDensity;
        LightYearInUnrealUnits /= AstroScaleCoeff;

        while (spaceOccupied) {
            position = (this->*generateStar)(GalaxyModel->GalaxyClass, LightYearInUnrealUnits, StarModel->Radius);
            spaceOccupied = galaxyOctree->SpaceOccupied(position, StarModel->Radius);
        }

        // Вставляем звезду в октодерево
        galaxyOctree->InsertStar(position, StarModel->Radius);

        FTransform StarTransform;
        StarTransform.SetLocation(position);
        StarTransform.SetScale3D(FVector(StarModel->Radius));
        int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);

        FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel->SpectralClass, StarModel->SpectralSubclass);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
        double StarEmission = StarGenerator->CalculateEmission(StarModel->Luminosity * 25);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);
    }
    delete galaxyOctree;
}

FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
    int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
    FVector Direction;

    do
    {
        Direction.X = FMath::FRandRange(-1.0, 1.0);
        Direction.Y = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
        Direction.Z = FMath::FRandRange(-1.0, 1.0) * (1.0 - 0.1 * ellipticity);
    } while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again

    double RandomDistance = FMath::FRand() * StarDistance + StarRadius;

    return Direction.GetSafeNormal() * RandomDistance ;
}


FVector UGalaxyGenerator::GenerateStarInLenticularGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
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
    } while (Direction.SizeSquared() > 1.0f);

    double RandomDistance = FMath::FRand() * StarDistance + StarRadius + FMath::FRandRange(-StarRadius / 2, StarRadius / 2);

    return Direction.GetSafeNormal() * RandomDistance;
}

FVector UGalaxyGenerator::GenerateStarInSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
    // В спиральных галактиках звезды распределены вдоль спиральных рукавов. 
    // Для генерации этой формы мы можем использовать логарифмическую спираль.

    double angle = FMath::FRandRange(0, 2 * PI);
    double RandomDistance = StarDistance * exp(angle / tan(0.1 * PI));  // Adjust this to change the tightness of the spiral

    FVector Direction(cos(angle), sin(angle), FMath::FRandRange(-1.0, 1.0));

    return Direction.GetSafeNormal() * RandomDistance;
}

FVector UGalaxyGenerator::GenerateStarInBarredSpiralGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
    // In barred spiral galaxies, stars are distributed along a central bar and spiral arms that extend from the ends of the bar.
    // We can generate this shape by adding a random offset to the X position of the star in a spiral galaxy.

    FVector position = GenerateStarInSpiralGalaxy(GalaxyClass, StarDistance, StarRadius);

    position.X += FMath::FRandRange(-StarDistance / 2, StarDistance / 2);  // Adjust this to change the length of the bar

    return position;
}

FVector UGalaxyGenerator::GenerateStarInIrregularGalaxy(EGalaxyClass GalaxyClass, double StarDistance, double StarRadius)
{
    // In irregular galaxies, stars are distributed randomly.
    // We can generate this shape by generating a random position for the star within the bounding box of the galaxy.

    FVector Direction;

    do
    {
        Direction.X = FMath::FRandRange(-1.0, 1.0);
        Direction.Y = FMath::FRandRange(-1.0, 1.0);
        Direction.Z = FMath::FRandRange(-1.0, 1.0);
    } while (Direction.SizeSquared() > 1.0f);

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
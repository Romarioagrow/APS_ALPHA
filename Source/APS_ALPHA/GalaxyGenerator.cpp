// Fill out your copyright notice in the Description page of Project Settings.


#include "GalaxyGenerator.h"
#include "Galaxy.h"
#include "Octree.h"
#include "StarGenerationModel.h"
#include <cmath>

FGalaxyModel UGalaxyGenerator::GenerateRandomGalaxyModel()
{
	return FGalaxyModel();
}

FGalaxyModel UGalaxyGenerator::GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass)
{
	return FGalaxyModel();
}

void UGalaxyGenerator::GenerateGalaxyOctreeStars(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy, FGalaxyModel GalaxyModel)
{
    // —оздаем октодерево
    double GalaxyOctreeHalfDimension = 500;
    Octree* galaxyOctree = new Octree(FVector(0), FVector(GalaxyOctreeHalfDimension));

    double StarsCount = GalaxyModel.StarsCount;
    for (int i = 0; i < StarsCount; i++)
    {
        FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
        bool spaceOccupied = true;
        FVector position;

        double LightYearInKm = 9.461e12;
        double UnitInKm = 6963.4;
        double LightYearInUnrealUnits = LightYearInKm / UnitInKm;
        double AstroScaleCoeff = 1;
        LightYearInUnrealUnits /= AstroScaleCoeff;

        // ѕровер€ем, зан€то ли пространство, прежде чем вставл€ть новую звезду
        while (spaceOccupied) {
            position = GenerateStarInEllipticalGalaxy(GalaxyModel.GalaxyClass, LightYearInUnrealUnits, StarModel.Radius);
            double checkRadius = StarModel.Radius;
            spaceOccupied = galaxyOctree->SpaceOccupied(position, checkRadius + StarModel.Radius);
        }

        // ¬ставл€ем звезду в октодерево
        galaxyOctree->InsertStar(position, StarModel.Radius);

        FTransform StarTransform;
        StarTransform.SetLocation(position);
        StarTransform.SetScale3D(FVector(StarModel.Radius));
        int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);

        FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel.SpectralClass, StarModel.SpectralSubclass);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
        double StarEmission = StarGenerator->CalculateEmission(StarModel.Luminosity * 25);
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
        Direction.X = FMath::FRandRange(-1, 1);
        Direction.Y = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
        Direction.Z = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
    } while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again

    double RandomDistance = FMath::FRand() * StarDistance + StarRadius;

    return Direction.GetSafeNormal() * RandomDistance ;
}

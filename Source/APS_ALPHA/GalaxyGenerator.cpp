// Fill out your copyright notice in the Description page of Project Settings.


#include "GalaxyGenerator.h"

FGalaxyModel UGalaxyGenerator::GenerateRandomGalaxyModel()
{
	return FGalaxyModel();
}

FGalaxyModel UGalaxyGenerator::GenerateGalaxyByParamsModel(EGalaxyType GalaxyType, EGalaxyClass GalaxyGlass)
{
	return FGalaxyModel();
}

//TArray<FVector> UGalaxyGenerator::GenerateStarsInEllipticalGalaxy(EGalaxyClass GalaxyClass, int NumStars, float StarDistance)
//{
//    TArray<FVector> Stars;
//    Stars.Reserve(NumStars);
//
//    // Add the first star at the origin
//    Stars.Add(FVector::ZeroVector);
//
//    // Add the remaining stars
//    for (int i = 1; i < NumStars; ++i)
//    {
//        FVector NewStarPosition;
//        do
//        {
//            // Pick a random star to be the "current" star
//            FVector CurrentStar = Stars[FMath::RandRange(0, Stars.Num() - 1)];
//
//            // Get a new position at a distance StarDistance from the current star
//            NewStarPosition = CurrentStar + GenerateStarInEllipticalGalaxy(GalaxyClass, StarDistance);
//        } while (NewStarPosition.Size() > GalaxyRadius);  // If the new star is outside the galaxy, try again
//
//        Stars.Add(NewStarPosition);
//    }
//
//    return Stars;
//}

FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, float StarDistance)
{
    int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
    FVector Direction;

    do
    {
        Direction.X = FMath::FRandRange(-1, 1);
        Direction.Y = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
        Direction.Z = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
    } while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again

    float RandomDistance = FMath::FRand() * StarDistance;

    return Direction.GetSafeNormal() * RandomDistance;
}


//FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, float StarDistance)
//{
//    int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
//    FVector Direction;
//
//    do
//    {
//        Direction.X = FMath::FRandRange(-1, 1);
//        Direction.Y = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
//        Direction.Z = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
//    } 
//    while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again
//
//    return Direction.GetSafeNormal() * StarDistance;
//}

//FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass)
//{
//    int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
//    float x = FMath::FRandRange(-1, 1);
//    float y = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
//    float z = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
//    return FVector(x, y, z);
//}

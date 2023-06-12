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



void UGalaxyGenerator::GenerateStarsInGalaxy(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy)
{

    // Создаем октодерево
    double GalaxyRadius = 10000;
    double StarsCount = 100000;
    Octree* galaxyOctree = new Octree(FVector(0, 0, 0), FVector(GalaxyRadius, GalaxyRadius, GalaxyRadius));

    for (int i = 0; i < StarsCount; i++)
    {
        FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
        bool spaceOccupied = true;
        FVector position;

        double StarDistance = 10000;

        // Проверяем, занято ли пространство, прежде чем вставлять новую звезду
        while (spaceOccupied) {
            position = GenerateStarInEllipticalGalaxy(EGalaxyClass::E7, StarDistance, StarModel.Radius);
            float radius = StarModel.Radius;

            spaceOccupied = galaxyOctree->SpaceOccupied(position, radius);
        }

        // Вставляем звезду в октодерево
        //galaxyOctree->Insert(new OctreeElement(position, StarModel.Radius)); // предполагая, что у вас есть класс OctreeElement или что-то подобное
        galaxyOctree->InsertStar(position, StarModel.Radius);

        //GalaxyStars.Add(CreateStar(StarModel, position));
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




    //int NumCellsPerAxis = FMath::CeilToInt(FMath::Pow(10000, 1.0f / 3.0f));  // Рассчитываем 
    //float CellSize = 100000 / NumCellsPerAxis;  // Рассчитываем размер ячейки

    //for (int i = 0; i < 10000; ++i) {
    //    // Вычисляем индексы ячейки для этой звезды
    //    int x = i % NumCellsPerAxis;
    //    int y = (i / NumCellsPerAxis) % NumCellsPerAxis;
    //    int z = i / (NumCellsPerAxis * NumCellsPerAxis);

    //    // Генерируем случайное смещение внутри ячейки
    //    float offsetX = FMath::FRandRange(-CellSize / 2, CellSize / 2);
    //    float offsetY = FMath::FRandRange(-CellSize / 2, CellSize / 2);
    //    float offsetZ = FMath::FRandRange(-CellSize / 2, CellSize / 2);

    //    // Вычисляем положение звезды
    //    FVector position = FVector(x * CellSize + offsetX, y * CellSize + offsetY, z * CellSize + offsetZ);

    //    FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
    //    FTransform StarTransform;
    //    StarTransform.SetLocation(position);
    //    StarTransform.SetScale3D(FVector(StarModel.Radius));
    //    int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);
    //    FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel.SpectralClass, StarModel.SpectralSubclass);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
    //    double StarEmission = StarGenerator->CalculateEmission(StarModel.Luminosity * 25);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);
    //}



    //float galaxyRadius = 10000; //NewGalaxy->GalaxyRadius;
    //FVector galaxyCenter = FVector(0, 0, 0); // Находим центр галактики, здесь я предполагаю, что это (0, 0, 0)

    //Octree* galaxyOctree = new Octree(galaxyCenter, FVector(galaxyRadius, galaxyRadius, galaxyRadius));
    //int StarCount;

    //for (int i = 0; i < 10000; ++i) {
    //    FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
    //    bool spaceOccupied = true;
    //    FVector position;

    //    while (spaceOccupied) {
    //        position = GenerateStarInEllipticalGalaxy(EGalaxyClass::E7, 10000, StarModel.Radius);
    //        float radius = StarModel.Radius;

    //        spaceOccupied = galaxyOctree->SpaceOccupied(position, radius);
    //    }

    //    galaxyOctree->InsertStar(position, StarModel.Radius);


    //    FTransform StarTransform;
    //    StarTransform.SetLocation(position);
    //    StarTransform.SetRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, FMath::RandRange(-180.0f, 180.0f))));
    //    StarTransform.SetScale3D(FVector(1.f));

    //    int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);
    //    FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel.SpectralClass, StarModel.SpectralSubclass);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
    //    double StarEmission = StarGenerator->CalculateEmission(StarModel.Luminosity * 25);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);

    //}

    //for (int i = 0; i < 10000; ++i) {
    //    bool spaceOccupied = true;
    //    FStarModel StarModel;
    //    FVector position;

    //    // Проверяем, занято ли пространство, прежде чем вставлять звезду
    //    while (spaceOccupied) {
    //        StarModel = StarGenerator->GenerateRandomStarModel();
    //        position = GenerateStarInEllipticalGalaxy(EGalaxyClass::E7, 10000, StarModel.Radius);
    //        float radius = StarModel.Radius;

    //        spaceOccupied = galaxyOctree->SpaceOccupied(position, radius);
    //    }

    //    // Если пространство свободно, вставляем звезду в октодерево
    //    galaxyOctree->InsertStar(position, StarModel.Radius);

    //    // Создание звезды в галактике после успешной вставки в октодерево
    //    FTransform StarTransform;
    //    StarTransform.SetLocation(position);
    //    StarTransform.SetRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, FMath::RandRange(-180.0f, 180.0f))));
    //    StarTransform.SetScale3D(FVector(1.f));

    //    int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);

    //    FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel.SpectralClass, StarModel.SpectralSubclass);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
    //    double StarEmission = StarGenerator->CalculateEmission(StarModel.Luminosity * 25);
    //    NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);
    //    StarCount = StarInstIndex;
    //}

   // UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), StarCount);

    // Удалить октодерево после использования
   // delete galaxyOctree;
}



//void UGalaxyGenerator::GenerateStarsInGalaxy(UStarGenerator* StarGenerator, AGalaxy* NewGalaxy)
//{
//    float galaxyRadius = 10000;//NewGalaxy->GalaxyRadius;
//    FVector galaxyCenter = FVector(0, 0, 0);  // Находим центр галактики, здесь я предполагаю, что это (0, 0, 0)
//
//    Octree* galaxyOctree = new Octree(galaxyCenter, FVector(galaxyRadius, galaxyRadius, galaxyRadius));
//    int StarCount;
//    for (int i = 0; i < 10000; ++i) {
//        bool starInserted = false;
//
//
//        FStarModel StarModel = StarGenerator->GenerateRandomStarModel();
//        FVector position;
//
//        while (!starInserted) {
//            // Генерация параметров звезды
//            position = GenerateStarInEllipticalGalaxy(EGalaxyClass::E7, 10000, StarModel.Radius);
//            float radius = StarModel.Radius;
//
//            // Попытка вставки звезды в октодерево
//            starInserted = galaxyOctree->InsertStar(position, radius);
//        }
//
//        // Создание звезды в галактике после успешной вставки в октодерево
//        FTransform StarTransform;
//        StarTransform.SetLocation(position);
//        //StarTransform.SetRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, FMath::RandRange(-180.0f, 180.0f))));
//        StarTransform.SetScale3D(FVector(1.f));
//
//        int32 StarInstIndex = NewGalaxy->StarMeshInstances->AddInstance(StarTransform, true);
//
//        FLinearColor ColorValue = StarGenerator->GetStarColor(StarModel.SpectralClass, StarModel.SpectralSubclass);
//        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 0, ColorValue.R);
//        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 1, ColorValue.G);
//        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 2, ColorValue.B);
//        double StarEmission = StarGenerator->CalculateEmission(StarModel.Luminosity * 25);
//        NewGalaxy->StarMeshInstances->SetCustomDataValue(StarInstIndex, 3, StarEmission);
//        StarCount = StarInstIndex;
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), StarCount);
//
//    // Удалить октодерево после использования
//   // delete galaxyOctree;
//}



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

FVector UGalaxyGenerator::GenerateStarInEllipticalGalaxy(EGalaxyClass GalaxyClass, float StarDistance, float StarRadius)
{
    int ellipticity = static_cast<int>(GalaxyClass) - static_cast<int>(EGalaxyClass::E0);
    FVector Direction;

    do
    {
        Direction.X = FMath::FRandRange(-1, 1);
        Direction.Y = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
        Direction.Z = FMath::FRandRange(-1, 1) * (1 - 0.1f * ellipticity);
    } while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again

    float RandomDistance = FMath::FRand() * StarDistance + StarRadius;

    return Direction.GetSafeNormal() * RandomDistance * 100;
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
//    } while (Direction.SizeSquared() > 1.0f);  // If the direction is outside the unit sphere, try again
//
//    float RandomDistance = FMath::FRand() * StarDistance;
//
//    return Direction.GetSafeNormal() * RandomDistance;
//}


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

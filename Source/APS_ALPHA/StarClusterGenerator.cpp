// Fill out your copyright notice in the Description page of Project Settings.


#include "StarClusterGenerator.h"
#include "StarCluster.h"

UStarClusterGenerator::UStarClusterGenerator()
{
    BP_StarClusterClass = AStarCluster::StaticClass();
}

FVector UStarClusterGenerator::CalculateStarPosition(int StarIndex, const AStarCluster* StarCluster, const FStarModel& StarModel)
{
    FVector StarPosition;

    // Размер звезды
    float StarSize = StarModel.Radius;

    switch (StarCluster->ClusterType)
    {
        case EStarClusterType::OpenCluster:
        {
            // Равномерное распределение
            StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2, -StarCluster->ClusterBounds.Y / 2, -StarCluster->ClusterBounds.Z / 2), FVector(StarCluster->ClusterBounds.X / 2, StarCluster->ClusterBounds.Y / 2, StarCluster->ClusterBounds.Z / 2)));
        }
        break;
        case EStarClusterType::GlobularCluster:
        {
            // Больше звезд ближе к центру
            double Radius = FMath::RandRange(static_cast<double>(StarSize), StarCluster->ClusterBounds.X / 2);
            //float Radius = FMath::RandRange(StarSize, StarCluster->ClusterBounds.X / 2);
            float Angle = FMath::RandRange(0.0f, 2 * PI);
            float Z = FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2);
            StarPosition = FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
        }
        break;
        case EStarClusterType::Supercluster:
        {
            // Почти равномерное распределение, но с некоторыми большими звездами в центре
            StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2, -StarCluster->ClusterBounds.Y / 2, -StarCluster->ClusterBounds.Z / 2), FVector(StarCluster->ClusterBounds.X / 2, StarCluster->ClusterBounds.Y / 2, StarCluster->ClusterBounds.Z / 2)));
            if (StarSize > 10.0f)
            {
                StarPosition /= 2;  // Перемещаем большие звезды ближе к центру
            }
        }
        break;
        case EStarClusterType::Nebula:
        {
            // Распределение звезд по спирали
            float SpiralRadius = StarIndex * StarSize;
            float SpiralAngle = 2 * PI * StarIndex / StarCluster->StarCount;
            StarPosition = FVector(SpiralRadius * FMath::Cos(SpiralAngle), SpiralRadius * FMath::Sin(SpiralAngle), FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2));
        }
        break;
    }
    return StarPosition * 100;
}

/// TODO: USAGE?
void UStarClusterGenerator::GenerateRandomStarCluster(UWorld* World)
{
	UE_LOG(LogTemp, Warning, TEXT("GenerateRandomStarCluster"));

    //UWorld* World = GetWorld();
    if (World)
    {
        AStarCluster* NewStarCluster = World->SpawnActor<AStarCluster>(BP_StarClusterClass);
        
        //...
        EStarClusterType ClusterType = static_cast<EStarClusterType>(FMath::RandRange(0, static_cast<int>(EStarClusterType::Nebula)));

        NewStarCluster->ClusterType = ClusterType;
        NewStarCluster->StarCount = FMath::RandRange(100, 1000);
        NewStarCluster->StarDensity = FMath::RandRange(0.1f, 1.0f);
        NewStarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);


        UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarCount);
        UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
        UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
        UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

        for (size_t i = 0; i < NewStarCluster->StarCount; i++)
        {
            //// Создаем модель звезды
            //FStarModel NewStarModel = StarGenerator.GenerateStarModel();

            //// Добавляем модель звезды в список моделей звезд кластера
            //NewStarCluster->StarModels.Add(NewStarModel);

            //// Позиционируем звезду в кластере
            //FVector StarPosition = CalculateStarPosition(i, NewStarCluster);

            //// Создаем инстанс звезды и добавляем его в HISM компонент
            //FTransform StarTransform(StarPosition);
            //NewStarCluster->StarMeshInstances->AddInstance(StarTransform);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null"));
    }


    //NewStarCluster->GenerateCluster();

    // Инициализируем генератор случайных чисел
    //FRandomStream RandStream;

    //// Создаем первую звездную систему
    //FStarModel FirstStar;
    //FirstStar.Position = FVector(0, 0, 0);
    //FirstStar.SpectralClass = static_cast<ESpectralClass>(RandStream.RandRange(0, static_cast<int>(ESpectralClass::Unknown)));
    //FirstStar.Size = RandStream.FRandRange(MinStarSize, MaxStarSize);
    //StarModels.Add(FirstStar);

    // Создаем остальные звездные системы
    //for (int i = 1; i < StarCount; ++i)
    //{
    //    FStarModel NewStar;

    //    // Выбираем случайное направление и расстояние
    //    FVector Direction = RandStream.GetUnitVector();
    //    float Distance = RandStream.FRandRange(MinStarDistance, MaxStarDistance);

    //    // Ищем подходящее место для новой звездной системы
    //    bool bValidPosition = false;
    //    while (!bValidPosition)
    //    {
    //        NewStar.Position = StarModels[i - 1].Position + Direction * Distance;
    //        NewStar.SpectralClass = static_cast<ESpectralClass>(RandStream.RandRange(0, static_cast<int>(ESpectralClass::Unknown)));
    //        NewStar.Size = RandStream.FRandRange(MinStarSize, MaxStarSize);

    //        // Проверяем, не пересекается ли новая звездная система с уже существующими
    //        bValidPosition = true;
    //        for (const FStarModel& OtherStar : StarModels)
    //        {
    //            if ((NewStar.Position - OtherStar.Position).Size() < NewStar.Size + OtherStar.Size)
    //            {
    //                bValidPosition = false;
    //                Direction = RandStream.GetUnitVector();
    //                Distance = RandStream.FRandRange(MinStarDistance, MaxStarDistance);
    //                break;
    //            }
    //        }
    //    }

    //    // Добавляем новую звездную систему
    //    StarModels.Add(NewStar);
    //}

    // TODO: Визуализируем звездные системы
}

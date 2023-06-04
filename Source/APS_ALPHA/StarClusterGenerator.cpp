// Fill out your copyright notice in the Description page of Project Settings.


#include "StarClusterGenerator.h"
#include "StarCluster.h"

UStarClusterGenerator::UStarClusterGenerator()
{
    BP_StarClusterClass = AStarCluster::StaticClass();
}

FVector UStarClusterGenerator::CalculateStarPosition(int StarIndex, AStarCluster* StarCluster, const FStarModel& StarModel)
{
    FVector StarPosition;

    // –азмер звезды
    double StarSize = StarModel.Radius;

    switch (StarCluster->ClusterType)
    {
    case EStarClusterType::OpenCluster:
    {
        StarCluster->ClusterBounds = FVector(100000.0f, 100000.0f, 100000.0f);
        //tarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);

        // –авномерное распределение с учетом размера звезды
        StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
            , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
            , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
            , StarCluster->ClusterBounds.Z / 2 + StarSize)));
    }
    break;
    case EStarClusterType::GlobularCluster:
    {
        // Ѕольше звезд ближе к центру, с учетом размера звезды
        double Radius = FMath::RandRange(static_cast<double>(StarSize), StarCluster->ClusterBounds.X / 2) + StarSize * 100; // умножаем на 100, так как StarSize в UE scale, а не в координатах.
        double Angle = FMath::RandRange(0.0f, 2 * PI);
        double Z = FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2);
        StarPosition = FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
    }
    break;
    case EStarClusterType::Supercluster:
    {
        //     ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
        StarCluster->ClusterBounds = FVector(100000.0f, 100000.0f, 100000.0f);

        // ѕочти равномерное распределение, но с некоторыми большими звездами в центре, с учетом размера звезды
        StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
            , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
            , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
            , StarCluster->ClusterBounds.Z / 2 + StarSize)));
        if (StarSize > 10.0f)
        {
            StarPosition /= 2;  // ѕеремещаем большие звезды ближе к центру
        }
    }
    break;
    case EStarClusterType::Nebula:
    {
        // –аспределение звезд по спирали с учетом размера звезды
        double SpiralRadius = StarIndex * StarSize;
        //double SpiralRadius = StarIndex * (StarSize + StarSize); // ”величиваем радиус спирали на размер звезды
        double SpiralAngle = 2 * PI * StarIndex / StarCluster->StarCount;
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
            //// —оздаем модель звезды
            //FStarModel NewStarModel = StarGenerator.GenerateStarModel();

            //// ƒобавл€ем модель звезды в список моделей звезд кластера
            //NewStarCluster->StarModels.Add(NewStarModel);

            //// ѕозиционируем звезду в кластере
            //FVector StarPosition = CalculateStarPosition(i, NewStarCluster);

            //// —оздаем инстанс звезды и добавл€ем его в HISM компонент
            //FTransform StarTransform(StarPosition);
            //NewStarCluster->StarMeshInstances->AddInstance(StarTransform);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null"));
    }


    //NewStarCluster->GenerateCluster();

    // »нициализируем генератор случайных чисел
    //FRandomStream RandStream;

    //// —оздаем первую звездную систему
    //FStarModel FirstStar;
    //FirstStar.Position = FVector(0, 0, 0);
    //FirstStar.SpectralClass = static_cast<ESpectralClass>(RandStream.RandRange(0, static_cast<int>(ESpectralClass::Unknown)));
    //FirstStar.Size = RandStream.FRandRange(MinStarSize, MaxStarSize);
    //StarModels.Add(FirstStar);

    // —оздаем остальные звездные системы
    //for (int i = 1; i < StarCount; ++i)
    //{
    //    FStarModel NewStar;

    //    // ¬ыбираем случайное направление и рассто€ние
    //    FVector Direction = RandStream.GetUnitVector();
    //    float Distance = RandStream.FRandRange(MinStarDistance, MaxStarDistance);

    //    // »щем подход€щее место дл€ новой звездной системы
    //    bool bValidPosition = false;
    //    while (!bValidPosition)
    //    {
    //        NewStar.Position = StarModels[i - 1].Position + Direction * Distance;
    //        NewStar.SpectralClass = static_cast<ESpectralClass>(RandStream.RandRange(0, static_cast<int>(ESpectralClass::Unknown)));
    //        NewStar.Size = RandStream.FRandRange(MinStarSize, MaxStarSize);

    //        // ѕровер€ем, не пересекаетс€ ли нова€ звездна€ система с уже существующими
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

    //    // ƒобавл€ем новую звездную систему
    //    StarModels.Add(NewStar);
    //}

    // TODO: ¬изуализируем звездные системы
}

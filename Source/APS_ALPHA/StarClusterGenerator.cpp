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

    // Размер звезды
    double StarSize = StarModel.Radius;

    switch (StarCluster->ClusterType)
    {
    case EStarClusterType::OpenCluster: 
    {
        StarCluster->ClusterBounds = FVector(100000.0f, 100000.0f, 100000.0f);
        // Равномерное распределение с учетом размера звезды
        StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
            , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
            , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
            , StarCluster->ClusterBounds.Z / 2 + StarSize)));
    }
    break;
    case EStarClusterType::GlobularCluster: /// TODO: ALL STAR MAIN SEQUENCE !
    {
        // Больше звезд ближе к центру, с учетом размера звезды
        double Radius = FMath::RandRange(static_cast<double>(StarSize), StarCluster->ClusterBounds.X / 2) + StarSize * 100; // умножаем на 100, так как StarSize в UE scale, а не в координатах.
        double Angle = FMath::RandRange(0.0f, 2 * PI);
        double Z = FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2);
        StarPosition = FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
    }
    break;
    case EStarClusterType::Supercluster:
    {
        StarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);

        double SphereRadius = FMath::Max(StarCluster->ClusterBounds.X, FMath::Max(StarCluster->ClusterBounds.Y, StarCluster->ClusterBounds.Z)) / 2;
        FVector RandomPoint = FMath::VRand();
        double Weight = 1 - FMath::Pow(StarSize, 3); // измененная часть
        double RandomScale = SphereRadius - (FMath::FRand() * Weight * (SphereRadius - StarSize));
        RandomPoint *= RandomScale;
        StarPosition = RandomPoint;

        if (StarSize > 10.0f)
        {
            StarPosition /= 2;  // Перемещаем большие звезды ближе к центру
        }

        //// Почти равномерное распределение, но с некоторыми большими звездами в центре, с учетом размера звезды
        //StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
        //    , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
        //    , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
        //    , StarCluster->ClusterBounds.Z / 2 + StarSize)));
        if (StarSize > 10.0f)
        {
            StarPosition /= 2;  // Перемещаем большие звезды ближе к центру
        }
    }
    break;
    case EStarClusterType::Nebula:
    {
        //StarCluster->ClusterBounds = FVector(10, 10, 10);

        // Распределение звезд по спирали с учетом размера звезды
        double SpiralRadius = StarIndex * StarSize;
        //double SpiralRadius = StarIndex * (StarSize + StarSize); // Увеличиваем радиус спирали на размер звезды
        double SpiralAngle = 2 * PI * StarIndex / StarCluster->StarAmount;
        StarPosition = FVector(SpiralRadius * FMath::Cos(SpiralAngle), SpiralRadius * FMath::Sin(SpiralAngle), FMath::RandRange(-StarCluster->ClusterBounds.Z / 2, StarCluster->ClusterBounds.Z / 2));
        //StarPosition /= 5;
    }
    break;
    }
    return StarPosition * 100;
}

TMap<EStarClusterSize, TPair<int, int>> StarClusterSizes =
{
    {EStarClusterSize::Tiny, TPair<int, int>(100, 500)},
    {EStarClusterSize::Small, TPair<int, int>(500, 1500)},
    {EStarClusterSize::Medium, TPair<int, int>(1500, 5000)},
    {EStarClusterSize::Large, TPair<int, int>(5000, 25000)},
    {EStarClusterSize::Giant, TPair<int, int>(25000, 50000)},
    {EStarClusterSize::Unknown, TPair<int, int>(0, 0)},
};

int UStarClusterGenerator::GetStarsAmountByRange(EStarClusterSize StarClusterSize)
{
    const TPair<int, int>* Range = StarClusterSizes.Find(StarClusterSize);
    if (Range == nullptr)
    {
        return 0;
    }
    return FMath::RandRange(Range->Key, Range->Value);
}

double UStarClusterGenerator::GetStarClusterDensityByRange()
{
    return FMath::RandRange(0.1f, 1.0f); /// TODO: RANGE
}

FVector UStarClusterGenerator::GetStarClusterBoundsByRange(EStarClusterType ClusterType)
{
    return FVector(1000.0f, 1000.0f, 1000.0f); /// TODO: RANGE
}

EStarClusterType UStarClusterGenerator::GetRandomClusterType()
{
    return static_cast<EStarClusterType>(FMath::RandRange(0, static_cast<int>(EStarClusterType::Nebula)));
}

FStarClusterModel UStarClusterGenerator::GenerateRandomStarClusterModelByParams(FStarClusterModel& StarClusterModel)
{
    
    
    return StarClusterModel;
}

FStarClusterModel UStarClusterGenerator::GetRandomStarClusterModel()
{
    FStarClusterModel StarClusterModel{};
    
    StarClusterModel.StarClusterSize = static_cast<EStarClusterSize>(FMath::RandRange(0, static_cast<int>(EStarClusterSize::Giant)));
    StarClusterModel.StarClusterType = static_cast<EStarClusterType>(FMath::RandRange(0, static_cast<int>(EStarClusterType::Nebula)));
    StarClusterModel.StarClusterPopulation = static_cast<EStarClusterPopulation>(FMath::RandRange(0, static_cast<int>(EStarClusterPopulation::Protostars)));
    StarClusterModel.StarClusterComposition = static_cast<EStarClusterComposition>(FMath::RandRange(0, static_cast<int>(EStarClusterComposition::MostlyRed)));
    
    return StarClusterModel;
}

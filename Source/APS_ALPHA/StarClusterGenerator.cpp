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
        // –авномерное распределение с учетом размера звезды
        StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
            , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
            , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
            , StarCluster->ClusterBounds.Z / 2 + StarSize)));
    }
    break;
    case EStarClusterType::GlobularCluster: /// TODO: ALL STAR MAIN SEQUENCE !
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
        StarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);

        double SphereRadius = FMath::Max(StarCluster->ClusterBounds.X, FMath::Max(StarCluster->ClusterBounds.Y, StarCluster->ClusterBounds.Z)) / 2;
        FVector RandomPoint = FMath::VRand();
        double Weight = 1 - FMath::Pow(StarSize, 3); // измененна€ часть
        double RandomScale = SphereRadius - (FMath::FRand() * Weight * (SphereRadius - StarSize));
        RandomPoint *= RandomScale;
        StarPosition = RandomPoint;

        if (StarSize > 10.0f)
        {
            StarPosition /= 2;  // ѕеремещаем большие звезды ближе к центру
        }

        //// ѕочти равномерное распределение, но с некоторыми большими звездами в центре, с учетом размера звезды
        //StarPosition = FMath::RandPointInBox(FBox(FVector(-StarCluster->ClusterBounds.X / 2 - StarSize
        //    , -StarCluster->ClusterBounds.Y / 2 - StarSize, -StarCluster->ClusterBounds.Z / 2 - StarSize)
        //    , FVector(StarCluster->ClusterBounds.X / 2 + StarSize, StarCluster->ClusterBounds.Y / 2 + StarSize
        //    , StarCluster->ClusterBounds.Z / 2 + StarSize)));
        if (StarSize > 10.0f)
        {
            StarPosition /= 2;  // ѕеремещаем большие звезды ближе к центру
        }
    }
    break;
    case EStarClusterType::Nebula:
    {
        //StarCluster->ClusterBounds = FVector(10, 10, 10);

        // –аспределение звезд по спирали с учетом размера звезды
        double SpiralRadius = StarIndex * StarSize;
        //double SpiralRadius = StarIndex * (StarSize + StarSize); // ”величиваем радиус спирали на размер звезды
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
    {EStarClusterSize::Giant, TPair<int, int>(250000, 50000)},
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
        NewStarCluster->StarAmount = FMath::RandRange(100, 1000);
        NewStarCluster->StarDensity = FMath::RandRange(0.1f, 1.0f);
        NewStarCluster->ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);


        UE_LOG(LogTemp, Warning, TEXT("StarCount: %d"), NewStarCluster->StarAmount);
        UE_LOG(LogTemp, Warning, TEXT("StarDensity: %f"), NewStarCluster->StarDensity);
        UE_LOG(LogTemp, Warning, TEXT("ClusterBounds: %s"), *NewStarCluster->ClusterBounds.ToString());
        UE_LOG(LogTemp, Warning, TEXT("ClusterType: %d"), static_cast<int>(NewStarCluster->ClusterType));

        for (size_t i = 0; i < NewStarCluster->StarAmount; i++)
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

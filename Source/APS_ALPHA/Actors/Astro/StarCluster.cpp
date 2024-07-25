#include "StarCluster.h"
#include "Star.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"

AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);

    // Инициализация значений по умолчанию
    StarAmount = 100;
    StarDensity = 1.0f;
    ClusterType = EStarClusterType::OpenCluster;
    ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
}

void AStarCluster::AddStarToCluster(AStar* Star)
{
    Stars.Add(Star);
}

void AStarCluster::AddStarToClusterModel(FVector StarLocation, TSharedPtr<FStarModel> StarModel)
{
    StarsModel.Add(StarLocation, StarModel);
}

// Функция генерации скопления
void AStarCluster::GenerateCluster()
{
    // Логика генерации здесь
}

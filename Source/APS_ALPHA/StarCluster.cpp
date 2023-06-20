// Fill out your copyright notice in the Description page of Project Settings.


#include "StarCluster.h"
#include "Star.h"

// Конструктор
AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
    //RootComponent->SetupAttachment(StarMeshInstances);
    StarMeshInstances->SetupAttachment(RootComponent);

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
    // Ваша логика генерации здесь
}

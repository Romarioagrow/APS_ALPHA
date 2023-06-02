// Fill out your copyright notice in the Description page of Project Settings.


#include "StarCluster.h"
#include "Star.h"

// Конструктор
AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    // Инициализация значений по умолчанию
    StarCount = 100;
    StarDensity = 1.0f;
    ClusterType = EStarClusterType::OpenCluster;
    ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
}

// Функция генерации скопления
void AStarCluster::GenerateCluster()
{
    // Ваша логика генерации здесь
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "StarCluster.h"
#include "Star.h"

// �����������
AStarCluster::AStarCluster()
{
    PrimaryActorTick.bCanEverTick = false;

    // ������������� �������� �� ���������
    StarCount = 100;
    StarDensity = 1.0f;
    ClusterType = EStarClusterType::OpenCluster;
    ClusterBounds = FVector(1000.0f, 1000.0f, 1000.0f);
}

// ������� ��������� ���������
void AStarCluster::GenerateCluster()
{
    // ���� ������ ��������� �����
}

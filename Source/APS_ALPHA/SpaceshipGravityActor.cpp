// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipGravityActor.h"

ASpaceshipGravityActor::ASpaceshipGravityActor()
{
    // �������� � ��������� StaticMeshComponent
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    RootComponent = StaticMeshComponent;

    SphereCollisionComponent->SetupAttachment(RootComponent);
}

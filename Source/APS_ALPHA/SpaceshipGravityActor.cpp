// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipGravityActor.h"

ASpaceshipGravityActor::ASpaceshipGravityActor()
{
    
    // Создание и настройка StaticMeshComponent
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    RootComponent = StaticMeshComponent;

    // Создание и настройка SphereCollisionComponent
    //SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
    SphereCollisionComponent->SetupAttachment(RootComponent);
}

//USphereComponent* ASpaceshipGravityActor::GetGravityField()
//{
//    return SphereCollisionComponent;
//}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Galaxy.h"

AGalaxy::AGalaxy()
{
    PrimaryActorTick.bCanEverTick = false;

    StarMeshInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StarMeshInstances"));
    RootComponent = Cast<USceneComponent>(StarMeshInstances);
    RootComponent->SetupAttachment(StarMeshInstances);


}

// Fill out your copyright notice in the Description page of Project Settings.


#include "AstroActor.h"

AAstroActor::AAstroActor()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("AstroActorRootComponent"));
	RootComponent = Root;
}

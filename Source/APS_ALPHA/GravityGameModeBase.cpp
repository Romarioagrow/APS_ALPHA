// Fill out your copyright notice in the Description page of Project Settings.


#include "GravityGameModeBase.h"

#include "SMENU_PlayerController.h"
#include "SMENU_HUD.h"

#include "UObject/ConstructorHelpers.h"

AGravityGameModeBase::AGravityGameModeBase()
{

	//static ConstructorHelpers::FClassFinder<Pawn> PlayerPawnBPClass(TEXT(""))

	PlayerControllerClass = ASMENU_PlayerController::StaticClass();
	HUDClass = ASMENU_HUD::StaticClass();
}
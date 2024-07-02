// Fill out your copyright notice in the Description page of Project Settings.


#include "SMENU_HUD.h"
#include "SMainMenuWidget.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/Engine.h"

void ASMENU_HUD::BeginPlay()
{
	Super::BeginPlay();


	//if (GEngine && GEngine->GameViewport) 
	//{
	//	MenuWidget = SNew(SMainMenuWidget);
	//	MenuWidget->OwningHud = this;

	//	//TSharedPtr<SWidget> GenericMenuWidget = MenuWidget;


	//	//GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef());


	//	


	//}

	//if (GEngine && GEngine->GameViewport)
	//{



	//	//MenuWidget = SNew(SMainMenuWidget).OwningHud(this);
	//	//GEngine->GameViewport->AddViewportWidgetContent(SAssignNew(MenuWidgetContainer, SWeakWidget).PossiblyNullContent(MenuWidget.ToSharedRef()));
	//}
}


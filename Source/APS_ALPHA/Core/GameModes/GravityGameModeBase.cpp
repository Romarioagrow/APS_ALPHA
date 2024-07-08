#include "GravityGameModeBase.h"
#include "APS_ALPHA/UI/SMENU_HUD.h"
#include "APS_ALPHA/UI/SMENU_PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AGravityGameModeBase::AGravityGameModeBase()
{
	PlayerControllerClass = ASMENU_PlayerController::StaticClass();
	HUDClass = ASMENU_HUD::StaticClass();
}
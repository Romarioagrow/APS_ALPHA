#include "W_CivMenuRoot.h"
#include "Components/WidgetSwitcher.h"

void UW_CivMenuRoot::NativeConstruct()
{
	Super::NativeConstruct();

	// MVP: по умолчанию Overview
	GoToScreen(ECivMenuScreen::Overview);
}

void UW_CivMenuRoot::GoToScreen(ECivMenuScreen Screen)
{
	if (!Switcher_Content) return;

	// Индексы должны соответствовать порядку виджетов в WidgetSwitcher в UMG.
	int32 Index = 0;
	switch (Screen)
	{
	case ECivMenuScreen::Overview:        Index = 0; break;
	case ECivMenuScreen::Divisions:       Index = 1; break;
	case ECivMenuScreen::DivisionDetails: Index = 2; break;
	default: Index = 0; break;
	}

	Switcher_Content->SetActiveWidgetIndex(Index);
}

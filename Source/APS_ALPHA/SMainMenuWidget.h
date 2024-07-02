// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "CoreMinimal.h"

/**
 *
 */
class SMainMenuWidget : SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainMenuWidget) {}

	SLATE_ARGUMENT(TWeakObjectPtr<class ASMENU_HUD>, OwningHud)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TWeakObjectPtr<class ASMENU_HUD> OwningHud;

	virtual bool SupportsKeyboardFocus() const override { return true; };
};

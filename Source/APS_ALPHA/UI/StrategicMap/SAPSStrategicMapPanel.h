#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Widgets/SCompoundWidget.h"

class AGravityPlayerController;

class SAPSStrategicMapPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSStrategicMapPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGravityPlayerController>, Controller)
		SLATE_ARGUMENT(TWeakObjectPtr<AAstroGenerator>, Generator)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
	FReply Close();
	FReply Focus(uint8 FocusValue);
	bool FocusUnderCursor();
	FText GetWorldSummary() const;

	TWeakObjectPtr<AGravityPlayerController> Controller;
	TWeakObjectPtr<AAstroGenerator> Generator;
	FSimpleDelegate OnClose;
};

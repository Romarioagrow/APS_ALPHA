// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#include "EditorUtils.h"
#include "EngineUtils.h"

FVector AtmoScapeEditor::GetViewPortCameraPosition() {
	if (IsValid(GEditor)) {
		if (GEditor->GetActiveViewport()) {
			FEditorViewportClient* Client = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
			if (Client)
			{
				return Client->GetViewLocation();
			}
		}
	}
	return FVector(0, 0, 0);
}

bool AtmoScapeEditor::IsInViewPort() {

	if (IsValid(GEditor)) {
		if (GEditor->GetActiveViewport()) {
			if (GEditor->PlayWorld == NULL) {
				return true;
			}
		}
	}
	return false;
}


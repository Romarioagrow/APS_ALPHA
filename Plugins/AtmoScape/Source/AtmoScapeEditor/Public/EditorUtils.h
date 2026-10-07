// Copyright 2021 IOLACORP STUDIO. All Rights Reserved


#pragma once
#include "EditorViewportClient.h"
#include "CoreMinimal.h"
#include "Math/Vector.h"



class ATMOSCAPEEDITOR_API AtmoScapeEditor
{
public:
	static FVector GetViewPortCameraPosition();
	static bool IsInViewPort();
};



// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "Runtime/Core/Public/Modules/ModuleManager.h"


class FAtmoScapeEditorModule : public IModuleInterface
{

  virtual void StartupModule();




private:
	TSharedPtr<FSlateStyleSet> StyleSet;


};

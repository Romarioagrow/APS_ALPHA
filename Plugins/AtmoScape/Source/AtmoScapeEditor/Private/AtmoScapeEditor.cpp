#include "AtmoScapeEditor.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/Paths.h"

void FAtmoScapeEditorModule::StartupModule()
{
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("AtmoScape");
    if (Plugin.IsValid())
    {
        FString ContentDir = Plugin->GetBaseDir() + TEXT("/Resources/");

        StyleSet = MakeShareable(new FSlateStyleSet("AtmoScapeStyle"));
        StyleSet->SetContentRoot(ContentDir);

        StyleSet->Set("ClassThumbnail.AtmoScape", new FSlateImageBrush(ContentDir + TEXT("Icon64.png"), FVector2D(64, 64)));
        StyleSet->Set("ClassIcon.AtmoScape", new FSlateImageBrush(ContentDir + TEXT("Icon16.png"), FVector2D(16, 16)));
        StyleSet->Set("ClassThumbnail.StarfieldScape", new FSlateImageBrush(ContentDir + TEXT("Icon64.png"), FVector2D(64, 64)));
        StyleSet->Set("ClassIcon.StarfieldScape", new FSlateImageBrush(ContentDir + TEXT("Icon16.png"), FVector2D(16, 16)));

        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
    }
}

IMPLEMENT_MODULE(FAtmoScapeEditorModule, AtmoScapeEditor)

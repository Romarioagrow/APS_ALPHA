#include "APS_ALPHA/UI/MainMenu/APSUIThumbnails.h"

#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/Package.h"

const FSlateBrush* APSUIThumbnails::FindBrush(const UClass* ActorClass)
{
	if (!ActorClass)
	{
		return nullptr;
	}
	// A Blueprint class lives in its Blueprint's package; native classes (/Script/...) have no thumbnail.
	const FString PackageName = ActorClass->GetOutermost()->GetName();
	if (!PackageName.StartsWith(TEXT("/Game/")))
	{
		return nullptr;
	}
	static TMap<FString, TSharedPtr<FSlateBrush>> Brushes;
	if (const TSharedPtr<FSlateBrush>* Found = Brushes.Find(PackageName))
	{
		return Found->Get();
	}
	TSharedPtr<FSlateBrush> Brush;
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePathForBlueprintPackage(PackageName), nullptr,
		LOAD_NoWarn | LOAD_Quiet))
	{
		// A handful of small textures, shown through brushes the garbage collector cannot see: kept for the session.
		Texture->AddToRoot();
		Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Texture);
		Brush->ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
		Brush->DrawAs = ESlateBrushDrawType::Image;
	}
	Brushes.Add(PackageName, Brush);
	return Brush.Get();
}

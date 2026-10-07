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
		InitBrush(*Brush, Texture);
	}
	Brushes.Add(PackageName, Brush);
	return Brush.Get();
}

void APSUIThumbnails::InitBrush(FSlateBrush& Brush, UTexture2D* Texture)
{
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	if (!Texture)
	{
		return;
	}
	const FVector2D TextureSize(static_cast<double>(Texture->GetSizeX()), static_cast<double>(Texture->GetSizeY()));
	Brush.ImageSize = TextureSize;
#if WITH_EDITORONLY_DATA
	// The visible part of each texture as a UV box, worked out once (an empty box: nothing to crop).
	static TMap<FString, FBox2f> Crops;
	const FString Key = Texture->GetPathName();
	const FBox2f* Crop = Crops.Find(Key);
	if (!Crop)
	{
		FBox2f Found(ForceInit);
		FTextureSource& Source = Texture->Source;
		TArray64<uint8> Pixels;
		const int32 Width = Source.IsValid() ? Source.GetSizeX() : 0;
		const int32 Height = Source.IsValid() ? Source.GetSizeY() : 0;
		if (Width > 0 && Height > 0 && Source.GetFormat() == TSF_BGRA8 && Source.GetMipData(Pixels, 0)
			&& Pixels.Num() >= static_cast<int64>(Width) * Height * 4)
		{
			int32 MinX = Width;
			int32 MinY = Height;
			int32 MaxX = -1;
			int32 MaxY = -1;
			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					if (Pixels[(static_cast<int64>(Y) * Width + X) * 4 + 3] > 8)
					{
						MinX = FMath::Min(MinX, X);
						MaxX = FMath::Max(MaxX, X);
						MinY = FMath::Min(MinY, Y);
						MaxY = FMath::Max(MaxY, Y);
					}
				}
			}
			if (MaxX >= MinX && MaxY >= MinY)
			{
				// A thin margin, a twentieth of the object's longer side, so no edge touches the frame.
				const float Margin = FMath::Max(MaxX - MinX + 1, MaxY - MinY + 1) * 0.05f;
				const float Left = FMath::Max(0.0f, MinX - Margin);
				const float Top = FMath::Max(0.0f, MinY - Margin);
				const float Right = FMath::Min(static_cast<float>(Width), MaxX + 1 + Margin);
				const float Bottom = FMath::Min(static_cast<float>(Height), MaxY + 1 + Margin);
				// Only worth a crop when it removes something.
				if ((Right - Left) * (Bottom - Top) < Width * Height * 0.95f)
				{
					Found = FBox2f(FVector2f(Left / Width, Top / Height), FVector2f(Right / Width, Bottom / Height));
				}
			}
		}
		Crop = &Crops.Add(Key, Found);
	}
	if (Crop->bIsValid)
	{
		Brush.SetUVRegion(*Crop);
		Brush.ImageSize = FVector2D(TextureSize.X * (Crop->Max.X - Crop->Min.X), TextureSize.Y * (Crop->Max.Y - Crop->Min.Y));
	}
#endif
}

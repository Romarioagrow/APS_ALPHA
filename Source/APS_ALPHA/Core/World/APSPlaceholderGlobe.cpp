#include "APSPlaceholderGlobe.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"

namespace APSPlaceholderGlobePrivate
{
	TAutoConsoleVariable<int32> CVarTint(TEXT("aps.Surface.PlaceholderTint"), 1,
		TEXT("1: a body without its WorldScape surface on screen wears its own palette colour instead of the authored ")
		TEXT("prototype grid (Rio 02.10, grey balls). 0: bodies tinted from now on keep the authored look."));

	const FName ColorParameter(TEXT("Color"));
	const TCHAR* MaterialPrefix = TEXT("APS_PlaceholderGlobe");

	/** Bodies already handled, so the per-update call costs a set lookup. */
	TSet<TWeakObjectPtr<const APlanetaryBody>> GHandled;

	UMaterialInterface* Parent()
	{
		// A plain lit engine material with a Color parameter: always cooked, no shader of its own to compile.
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}

	/** The authored globe: the StarterContent material sphere of the planet and moon Blueprints (not clouds, not gas). */
	bool IsAuthoredGlobe(const UStaticMeshComponent* Mesh)
	{
		const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
		return Asset && !Mesh->ComponentHasTag(TEXT("APS.GasGiantVisual"))
			&& Asset->GetPathName().Contains(TEXT("/StarterContent/Props/MaterialSphere"));
	}
}

FLinearColor APSPlaceholderGlobe::ColorOf(const APlanetaryBody* Body)
{
	const FAPSResolvedPlanetSurfaceProfile Profile = UAPSPlanetSurfaceProfileResolver::ResolveForBody(Body);
	const FAPSPlanetSurfacePalette& Palette = Profile.Palette;
	// The land as seen from orbit: mostly lowlands and highlands, some dry ground and slopes, a touch of the peaks.
	FLinearColor Color = Palette.Lowland * 0.25f + Palette.MidLowland * 0.25f + Palette.Highland * 0.2f
		+ Palette.Dryland * 0.15f + Palette.Slope * 0.1f + Palette.Peak * 0.05f;
	FLinearColor Liquid = FLinearColor::Black;
	switch (Profile.LiquidType)
	{
	case EAPSPlanetLiquidType::Water:
		Liquid = FLinearColor(0.02f, 0.07f, 0.16f);
		break;
	case EAPSPlanetLiquidType::Lava:
		Liquid = FLinearColor(0.42f, 0.08f, 0.02f);
		break;
	case EAPSPlanetLiquidType::Ammonia:
		Liquid = FLinearColor(0.08f, 0.20f, 0.18f);
		break;
	default:
		break;
	}
	if (Profile.LiquidType != EAPSPlanetLiquidType::None)
	{
		Color = FMath::Lerp(Color, Liquid, FMath::Clamp(1.0f - Profile.LandCoverage, 0.15f, 0.85f));
	}
	Color.A = 1.0f;
	return Color;
}

void APSPlaceholderGlobe::Apply(APlanetaryBody* Body)
{
	using namespace APSPlaceholderGlobePrivate;
	if (!IsValid(Body) || GHandled.Contains(Body) || CVarTint.GetValueOnGameThread() == 0
		// Menu previews render a compressed root and their own globe proxies.
		|| !FMath::IsNearlyEqual(Body->WorldScapePresentationScale, 1.0)
		|| !UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType))
	{
		return;
	}
	if (GHandled.Num() > 4096)
	{
		for (auto It = GHandled.CreateIterator(); It; ++It)
		{
			if (!It->IsValid()) It.RemoveCurrent();
		}
	}
	UMaterialInterface* Base = Parent();
	if (!Base)
	{
		return;
	}
	GHandled.Add(Body);
	TInlineComponentArray<UStaticMeshComponent*> Meshes(Body);
	bool bResolved = false;
	FLinearColor Color = FLinearColor::Gray;
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || !IsAuthoredGlobe(Mesh))
		{
			continue;
		}
		if (!bResolved)
		{
			Color = ColorOf(Body);
			bResolved = true;
		}
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Mesh,
			MakeUniqueObjectName(Mesh, UMaterialInstanceDynamic::StaticClass(), FName(MaterialPrefix)));
		Material->SetVectorParameterValue(ColorParameter, Color);
		for (int32 Slot = 0; Slot < FMath::Max(Mesh->GetNumMaterials(), 1); ++Slot)
		{
			Mesh->SetMaterial(Slot, Material);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Placeholder] %s globe %s wears %s"), *Body->GetName(), *Mesh->GetName(),
			*Color.ToString());
	}
}

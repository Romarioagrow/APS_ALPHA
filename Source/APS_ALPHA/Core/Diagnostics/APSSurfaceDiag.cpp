#include "APSSurfaceDiag.h"

#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "WorldScapeCore/Public/WorldScapeMeshComponent.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

namespace APSSurfaceDiagLocal
{
	struct FWatch
	{
		TWeakObjectPtr<const APlanetaryBody> Body;
		double FirstSeconds{0.0};
		bool bSecondDone{false};
	};
	TWeakObjectPtr<UWorld> GDiagWorld;
	TArray<FWatch> GWatched;
	float GClock = 0.0f;

	FString MaterialOf(const UPrimitiveComponent* Component)
	{
		const UMaterialInterface* Material = Component && Component->GetNumMaterials() > 0 ? Component->GetMaterial(0) : nullptr;
		if (!Material) return TEXT("none");
		FString Text = Material->GetName();
		if (const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material); Instance && Instance->Parent)
		{
			Text += TEXT(" <- ") + Instance->Parent->GetName();
		}
		return Text;
	}

	bool ViewPoint(UWorld* World, FVector& OutLocation)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		if (!Controller) return false;
		FRotator Rotation;
		Controller->GetPlayerViewPoint(OutLocation, Rotation);
		return true;
	}

	void DumpBody(UWorld* World, const APlanetaryBody* Body, const FVector& View, const TCHAR* Reason)
	{
		if (!Body) return;
		const double Radius = Body->GetWorldScapeBodyRadiusCm();
		const double Altitude = FVector::Dist(View, Body->GetActorLocation()) - Radius;
		UE_LOG(LogTemp, Warning, TEXT("[APS.SurfaceDiag] %s body=%s name=%s type=%d radius=%.1f km altitude=%.1f km hidden=%d"),
			Reason, *Body->GetName(), *Body->AstroName.ToString(), static_cast<int32>(Body->PlanetType), Radius / 100000.0,
			Altitude / 100000.0, Body->IsHidden() ? 1 : 0);
		TInlineComponentArray<UPrimitiveComponent*> Components;
		Body->GetComponents(Components);
		for (const UPrimitiveComponent* Component : Components)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.SurfaceDiag]   body component %s (%s) visible=%d hiddenInGame=%d boundsRadius=%.1f km material=%s"),
				*Component->GetName(), *Component->GetClass()->GetName(), Component->IsVisible() ? 1 : 0,
				Component->bHiddenInGame ? 1 : 0, Component->Bounds.SphereRadius / 100000.0, *MaterialOf(Component));
		}
		const APlanetarySurfaceGenerator* Generator = Body->PlanetaryEnvironmentGenerator;
		const AWorldScapeRoot* Root = Generator ? Generator->WorldScapeRootInstance : nullptr;
		if (!Root)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.SurfaceDiag]   no WorldScape root (generator=%s)"), *GetNameSafe(Generator));
		}
		else
		{
			int32 VisibleLods = 0;
			for (const UWorldScapeLod* Lod : Root->WorldScapeLod)
			{
				VisibleLods += Lod && Lod->Mesh && Lod->Mesh->IsVisible() ? 1 : 0;
			}
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.SurfaceDiag]   root %s hidden=%d init=%d tick=%d planetScale=%.1f km lods=%d visibleLods=%d inGeneration=%d ocean=%d observerFromView=%.1f km rootFromBody=%.1f km"),
				*Root->GetName(), Root->IsHidden() ? 1 : 0, Root->init ? 1 : 0, Root->IsActorTickEnabled() ? 1 : 0,
				Root->PlanetScale / 100000.0, Root->WorldScapeLod.Num(), VisibleLods, Root->WorldScapeLodInGeneration.Num(),
				Root->bOcean ? 1 : 0, FVector::Dist(Root->OverridedPlayerPosition, View) / 100000.0,
				FVector::Dist(Root->GetActorLocation(), Body->GetActorLocation()) / 100000.0);
			for (int32 Index = 0; Index < Root->WorldScapeLod.Num() && Index < 3; ++Index)
			{
				const UWorldScapeLod* Lod = Root->WorldScapeLod[Index];
				const UWorldScapeMeshComponent* Mesh = Lod ? Lod->Mesh : nullptr;
				if (!Mesh) continue;
				UE_LOG(LogTemp, Warning, TEXT("[APS.SurfaceDiag]     lod %d mesh visible=%d hiddenInGame=%d boundsRadius=%.1f km material=%s"),
					Index, Mesh->IsVisible() ? 1 : 0, Mesh->bHiddenInGame ? 1 : 0, Mesh->Bounds.SphereRadius / 100000.0,
					*MaterialOf(Mesh));
			}
		}
		TArray<AActor*> Attached;
		Body->GetAttachedActors(Attached, true, false);
		if (Generator && Generator->PlanetAtmosphere) Attached.AddUnique(Generator->PlanetAtmosphere);
		for (const AActor* Actor : Attached)
		{
			if (!Actor || Actor->IsA<APlanetaryBody>()) continue;
			TInlineComponentArray<UPrimitiveComponent*> ActorComponents;
			Actor->GetComponents(ActorComponents);
			int32 Visible = 0;
			double Largest = 0.0;
			FString LargestMaterial;
			for (const UPrimitiveComponent* Component : ActorComponents)
			{
				if (!Component->IsVisible() || Component->bHiddenInGame) continue;
				++Visible;
				if (Component->Bounds.SphereRadius > Largest)
				{
					Largest = Component->Bounds.SphereRadius;
					LargestMaterial = MaterialOf(Component);
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("[APS.SurfaceDiag]   attached %s (%s) hidden=%d visibleComponents=%d largest=%.1f km material=%s"),
				*Actor->GetName(), *Actor->GetClass()->GetName(), Actor->IsHidden() ? 1 : 0, Visible, Largest / 100000.0,
				*LargestMaterial);
		}
	}
}

void APSSurfaceDiag::DumpNearest(UWorld* World, const int32 Count, const TCHAR* Reason)
{
	using namespace APSSurfaceDiagLocal;
	FVector View;
	if (!ViewPoint(World, View)) return;
	TArray<TPair<double, const APlanetaryBody*>> Bodies;
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		if (IsValid(*It)) Bodies.Emplace(FVector::Dist(View, It->GetActorLocation()) - It->GetWorldScapeBodyRadiusCm(), *It);
	}
	Bodies.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });
	for (int32 Index = 0; Index < Bodies.Num() && Index < Count; ++Index)
	{
		DumpBody(World, Bodies[Index].Value, View, Reason);
	}
}

void APSSurfaceDiag::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSSurfaceDiagLocal;
	if (!World) return;
	if (GDiagWorld.Get() != World)
	{
		GDiagWorld = World;
		GWatched.Reset();
	}
	GClock += DeltaSeconds;
	if (GClock < 1.0f) return;
	GClock = 0.0f;
	FVector View;
	if (!ViewPoint(World, View)) return;
	const double Now = World->GetTimeSeconds();
	for (TActorIterator<APlanetaryBody> It(World); It; ++It)
	{
		const APlanetaryBody* Body = *It;
		if (!IsValid(Body)) continue;
		const double Radius = Body->GetWorldScapeBodyRadiusCm();
		if (Radius <= 0.0 || FVector::Dist(View, Body->GetActorLocation()) > Radius * 3.0) continue;
		FWatch* Watch = GWatched.FindByPredicate([Body](const FWatch& Each) { return Each.Body.Get() == Body; });
		if (!Watch)
		{
			Watch = &GWatched.AddDefaulted_GetRef();
			Watch->Body = Body;
			Watch->FirstSeconds = Now;
			DumpBody(World, Body, View, TEXT("approach"));
		}
		else if (!Watch->bSecondDone && Now - Watch->FirstSeconds > 15.0)
		{
			Watch->bSecondDone = true;
			DumpBody(World, Body, View, TEXT("approach+15s"));
		}
	}
}

namespace APSSurfaceDiagLocal
{
	FAutoConsoleCommandWithWorld DiagCommand(TEXT("aps.Surface.Diag"),
		TEXT("Logs what stands for the two bodies nearest the view: components, WorldScape root, LOD materials, atmosphere."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { APSSurfaceDiag::DumpNearest(World, 2, TEXT("console")); }));
}

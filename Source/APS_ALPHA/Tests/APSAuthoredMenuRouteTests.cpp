#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "SceneView.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Widgets/SViewport.h"

namespace APSAuthoredMenuRoute
{
TSharedPtr<SWidget> FindFramingPanel(const TSharedRef<SWidget>& Widget)
{
	if (Widget->GetType() == TEXT("SPreviewSystemOverlay")) return Widget;
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
		if (TSharedPtr<SWidget> Found = FindFramingPanel(Children->GetChildAt(Index))) return Found;
	return nullptr;
}

class FRoute final : public IAutomationLatentCommand
{
public:
	explicit FRoute(FAutomationTestBase* InTest) : Test(InTest), Start(FPlatformTime::Seconds()) {}
	bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		AMainMenuController* Controller = World ? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
		UWorldGenerationViewModel* VM = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
		if (Now - Start > 180.0 && Step != 99)
		{ Test->AddError(TEXT("Authored menu route timed out")); Step = 99; StepStart = Now; }
		if (Step == 99)
		{
			if (Generator.IsValid() && !Generator->PreparePreviewForTravel() && Now - StepStart < 30.0) return false;
			if (VM) VM->Shutdown();
			return true;
		}
		if (!VM || !VM->GeneratedWorld) return false;
		if (Step == -1)
		{
			// Do not inject a seed, rewrite the home recipe or enable a test-only path.
			Seed = VM->GeneratedWorld->GenerationSeed;
			Test->AddInfo(FString::Printf(TEXT("Authored menu entry seed=%d level=%d planets=%d moons=%d radiusKm=%.9g; no model or console overrides"),
				Seed, static_cast<int32>(VM->GeneratedWorld->AstroGenerationLevel), VM->GeneratedWorld->PlanetsAmount,
				VM->GeneratedWorld->MoonsAmount, VM->GeneratedWorld->PlanetRadius));
			if (!Controller->OpenAstronomicalGenerationForAutomation(Scopes[0], EAPSGenerationRoute::Civilization)) return false;
			Step = 0; StepStart = Now;
			return false;
		}
		if (!VM->bPreviewReady) return false;
		if (!Generator.IsValid())
		{
			for (TActorIterator<AAstroGenerator> It(World); It; ++It)
				if (It->ActorHasTag(TEXT("WorldGenerationPreview"))) { Generator = *It; break; }
			if (!Generator.IsValid()) return false;
			if (!Test->TestTrue(TEXT("Ordinary menu actually uses the common physical observer"), Generator->UsesContinuousPreviewFrame()))
			{ Step = 99; StepStart = Now; return false; }
			WorldHash = Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash;
			Test->AddInfo(FString::Printf(TEXT("Authored menu ready after %.3fs; canonicalHash=%u"), Now - Start, WorldHash));
		}
		if (Step > 0 && !bMidpoint && Now - StepStart >= 0.15)
		{
			CheckView(World, VM);
			Capture(FString::Printf(TEXT("%02d-motion"), Step));
			bMidpoint = true;
		}
		if (Now - StepStart < 1.8 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
		CheckView(World, VM);
		CheckFraming(Controller);
		Test->TestEqual(TEXT("Authored menu reaches the requested scope"), VM->GetPreviewFocus(), Scopes[Step]);
		if (Scopes[Step] == EAstroPreviewFocus::HomePlanet)
		{
			const FString Key = Generator->GetPreviewBodyStableKey(Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor()));
			Test->TestFalse(TEXT("Planet inspection resolves a concrete physical address"), Key.IsEmpty());
			if (PlanetKey.IsEmpty()) PlanetKey = Key;
			else Test->TestEqual(TEXT("Authored round trip returns to the same planet"), Key, PlanetKey);
		}
		Capture(FString::Printf(TEXT("%02d-%s"), Step, *UEnum::GetValueAsString(Scopes[Step])));
		const FVector Observer = Generator->GetContinuousPreviewOrbit().ObserverCm();
		VM->SetPreviewFocus(Scopes[Step]);
		Test->TestTrue(TEXT("Repeated current-scope click retains the displayed observer"),
			Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(Observer, 0.0));
		if (++Step >= UE_ARRAY_COUNT(Scopes)) { Step = 99; StepStart = Now; return false; }
		Test->TestTrue(TEXT("The next authored-menu scope is available"), VM->IsPreviewFocusAvailable(Scopes[Step]));
		VM->SetPreviewFocus(Scopes[Step]);
		Test->TestTrue(TEXT("The next flight starts at the displayed observer"),
			Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(Observer, 0.0));
		StepStart = Now; bMidpoint = false;
		return false;
	}
private:
	void CheckFraming(AMainMenuController* Controller)
	{
		if (Scopes[Step] != EAstroPreviewFocus::HomeStar && Scopes[Step] != EAstroPreviewFocus::HomePlanet) return;
		const TSharedPtr<SAPSMainMenuRoot> Root = Controller->GetSlateMenuRootForAutomation();
		const TSharedPtr<SWidget> Panel = Root ? FindFramingPanel(Root.ToSharedRef()) : nullptr;
		const ULocalPlayer* Player = Controller->GetLocalPlayer();
		const TSharedPtr<SViewport> Viewport = Player && Player->ViewportClient ? Player->ViewportClient->GetGameViewportWidget() : nullptr;
		FSceneViewProjectionData Projection;
		if (!Test->TestTrue(TEXT("Selected-body framing uses the actual menu panel"), Panel && Viewport && Player
			&& Player->GetProjectionData(Player->ViewportClient->Viewport, Projection))) return;
		APlanetaryBody* Body = Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor());
		UPrimitiveComponent* Mesh = Generator->GetPreviewTerrainProxyForBody(Body);
		if (AStar* Star = Cast<AStar>(Generator->GetSelectedPreviewBodyActor())) Mesh = Star->CoronaMesh;
		if (!Test->TestNotNull(TEXT("Selected physical globe/corona exists for framing"), Mesh)) return;
		const double Radius = Mesh->CalcBounds(FTransform::Identity).BoxExtent.GetMax()
			/ FMath::Max(static_cast<double>(Mesh->BoundsScale), 1.0e-12) * Mesh->GetComponentScale().GetAbsMax();
		const UCameraComponent* Camera = Generator->GetPreviewCameraComponent();
		const FVector Center = Mesh->GetComponentLocation();
		const double Distance = (Center - Camera->GetComponentLocation()).Size();
		if (!Test->TestTrue(TEXT("Camera stays outside the selected globe/corona"), Distance > Radius)) return;
		FVector2D ScreenCenter;
		if (!Test->TestTrue(TEXT("Selected body projects into the menu"), Controller->ProjectWorldLocationToScreen(Center, ScreenCenter))) return;
		const FIntRect ViewRect = Projection.GetConstrainedViewRect();
		const double RadiusPixels = Radius / FMath::Sqrt(Distance * Distance - Radius * Radius)
			* ViewRect.Width() * 0.5 * Projection.ProjectionMatrix.M[0][0];
		int32 Width = 0, Height = 0;
		Controller->GetViewportSize(Width, Height);
		const FGeometry Geometry = Viewport->GetCachedGeometry();
		const FVector2D PixelScale(Width / Geometry.GetLocalSize().X, Height / Geometry.GetLocalSize().Y);
		const FGeometry PanelGeometry = Panel->GetCachedGeometry();
		const FVector2D Min = Geometry.AbsoluteToLocal(PanelGeometry.LocalToAbsolute(FVector2D::ZeroVector)) * PixelScale;
		const FVector2D Max = Geometry.AbsoluteToLocal(PanelGeometry.LocalToAbsolute(PanelGeometry.GetLocalSize())) * PixelScale;
		const double Available = FMath::Min(FMath::Min(ScreenCenter.X - Min.X, Max.X - ScreenCenter.X),
			FMath::Min(ScreenCenter.Y - Min.Y, Max.Y - ScreenCenter.Y));
		Test->TestTrue(TEXT("Selected physical globe/corona fits between the actual status rows and control panels"), RadiusPixels + 2.0 < Available);
		Test->AddInfo(FString::Printf(TEXT("Authored framing step=%d radiusPixels=%.3f availablePixels=%.3f"), Step, RadiusPixels, Available));
	}

	void CheckView(UWorld* World, UWorldGenerationViewModel* VM)
	{
		Test->TestEqual(TEXT("Passive authored navigation retains the world seed"), VM->GeneratedWorld->GenerationSeed, Seed);
		Test->TestEqual(TEXT("Passive authored navigation retains the canonical world"),
			Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash, WorldHash);
		const FAPSContinuousPreviewFrame& Frame = Generator->GetContinuousPreviewFrame();
		Test->TestTrue(TEXT("Authored observer frame remains finite"), Frame.IsValid());
		bool bStarsValid = true;
		for (TActorIterator<AStar> It(World); It; ++It)
		{
			if (!It->IsAttachedTo(Generator.Get()) || !It->StarMesh || !It->StarMesh->GetStaticMesh()) continue;
			FAPSPreviewProjectedSphere Expected;
			if (!Frame.ProjectSphere(Generator->GetContinuousPreviewPhysicalPosition(*It), It->RadiusKM * 1.0e5, Expected))
			{ bStarsValid = false; continue; }
			const double Radius = It->StarMesh->GetStaticMesh()->GetBounds().BoxExtent.GetMax() * It->StarMesh->GetComponentScale().GetAbsMax();
			FVector Label;
			bStarsValid &= It->StarMesh->Bounds.Origin.Equals(Expected.Center, 1.0)
				&& FMath::IsNearlyEqual(Radius / FMath::Max(Expected.Radius, 1.0e-12), 1.0, 1.0e-6)
				&& Generator->GetPreviewPresentationLocation(*It, Label) && Label.Equals(Expected.Center, 1.0);
		}
		Test->TestTrue(TEXT("Authored star surfaces and labels share physical center and radius in motion"), bStarsValid);
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			if (!It->IsAttachedTo(Generator.Get())) continue;
			UProceduralMeshComponent* Terrain = Generator->GetPreviewTerrainProxyForBody(*It);
			if (!Terrain || !Terrain->IsVisible() || Terrain->bHiddenInGame) continue;
			FAPSPreviewProjectedSphere Expected;
			if (!Frame.ProjectSphere(Generator->GetContinuousPreviewPhysicalPosition(*It), It->RadiusKM * 1.0e5, Expected))
			{ Test->AddError(TEXT("Visible authored globe has no physical sphere")); continue; }
			// Inspect actual vertex radii independently of the rotated world AABB.
			// Both implementation and old tests used that inflated AABB, allowing a
			// shrunken globe to pass while its visible limb disagreed with the model.
			double MaxVertexRadius = 0.0;
			for (int32 SectionIndex = 0; SectionIndex < Terrain->GetNumSections(); ++SectionIndex)
				if (const FProcMeshSection* Section = Terrain->GetProcMeshSection(SectionIndex))
					for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
						MaxVertexRadius = FMath::Max(MaxVertexRadius, Vertex.Position.Size());
			MaxVertexRadius *= Terrain->GetComponentScale().GetAbsMax();
			const double LocalRadius = Terrain->CalcBounds(FTransform::Identity).BoxExtent.GetMax()
				/ FMath::Max(static_cast<double>(Terrain->BoundsScale), 1.0e-12) * Terrain->GetComponentScale().GetAbsMax();
			const double VertexRatio = MaxVertexRadius / Expected.Radius;
			Test->TestTrue(TEXT("Actual authored globe vertices retain the physical limb, independent of rotation"), VertexRatio >= 0.99 && VertexRatio <= 1.05);
			Test->TestTrue(TEXT("Unrotated globe reference radius matches the physical body"), FMath::IsNearlyEqual(LocalRadius / Expected.Radius, 1.0, 1.0e-6));
			Test->AddInfo(FString::Printf(TEXT("Authored globe step=%d key=%s vertexRadiusRatio=%.9g localRadiusRatio=%.9g worldBoxRatio=%.9g"),
				Step, *Generator->GetPreviewBodyStableKey(*It), VertexRatio, LocalRadius / Expected.Radius,
				Terrain->Bounds.BoxExtent.GetMax() / Expected.Radius));
		}
		TInlineComponentArray<UInstancedStaticMeshComponent*> Views;
		Generator->GetComponents(Views);
		int32 VisibleCatalogs = 0;
		for (const UInstancedStaticMeshComponent* View : Views)
			if (View->GetName().StartsWith(TEXT("Continuous")) && View->IsVisible() && !View->bHiddenInGame) ++VisibleCatalogs;
		Test->TestEqual(TEXT("Authored menu retains both galaxy and cluster context"), VisibleCatalogs, 2);
	}
	void Capture(const FString& Name)
	{
		UGameViewportClient* Client = AutomationCommon::GetAnyGameViewportClient();
		if (!Client || !Client->GetGameViewportWidget().IsValid()) { Test->AddError(TEXT("No authored menu viewport")); return; }
		TArray<FColor> Pixels; FIntVector Size;
		if (!FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), Pixels, Size)
			|| Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y)
		{ Test->AddError(TEXT("Cannot capture authored menu page")); return; }
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/AuthoredMenuRoute");
		IFileManager::Get().MakeDirectory(*Directory, true);
		TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
		Test->TestTrue(TEXT("Authored menu page saved"), FFileHelper::SaveArrayToFile(Png,
			*(Directory / (Name.Replace(TEXT("::"), TEXT("-")) + TEXT(".png")))));
	}
	FAutomationTestBase* Test;
	TWeakObjectPtr<AAstroGenerator> Generator;
	double Start, StepStart{0.0};
	int32 Step{-1}, Seed{0};
	uint32 WorldHash{0};
	bool bMidpoint{false};
	FString PlanetKey;
	const EAstroPreviewFocus Scopes[11] = {EAstroPreviewFocus::Galaxy, EAstroPreviewFocus::StarCluster,
		EAstroPreviewFocus::HomeSystem, EAstroPreviewFocus::HomeStar, EAstroPreviewFocus::HomePlanet,
		EAstroPreviewFocus::HomeStar, EAstroPreviewFocus::HomeSystem, EAstroPreviewFocus::StarCluster,
		EAstroPreviewFocus::Galaxy, EAstroPreviewFocus::Overview, EAstroPreviewFocus::HomePlanet};
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAuthoredMenuRoute, "APS.Rendered.MainMenu.AuthoredEntryRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSAuthoredMenuRoute::RunTest(const FString& Parameters)
{
	const IConsoleVariable* Switch = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestTrue(TEXT("The shipping menu switch is enabled without a test override"), Switch && Switch->GetInt() == 1)) return false;
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(APSAuthoredMenuRoute::FRoute(this));
	return true;
}

#endif

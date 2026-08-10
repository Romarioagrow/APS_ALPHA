#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/StarGenerator.h"
#include "APS_ALPHA/UI/MainMenu/SWorldGenerationPanel.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "AssetCompilingManager.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"

namespace APSMainMenuPreviewSmokeTests
{
	constexpr double WholeTestTimeoutSeconds = 1800.0;
	constexpr double SurfaceTimeoutSeconds = 60.0;
	constexpr double ScopeSettleSeconds = 0.85;
	constexpr double ScopeSampleSeconds = 1.25;
	constexpr double PlanetSampleSeconds = 3.0;
	constexpr double ScreenshotTimeoutSeconds = 10.0;
	constexpr double CleanupTimeoutSeconds = 20.0;
	constexpr int32 SliderSurfaceSeed = 94094;
	constexpr int32 RequiredPreviewGlobeVertexCount = 6 * 65 * 65;
	constexpr int32 RequiredPreviewGlobeIndexCount = 6 * 64 * 64 * 6;
	// Exactly representable values keep the float-backed Slate slider and the
	// double model comparison deterministic.
	constexpr double SliderFeatureScale = 1.75;
	constexpr double SliderReliefScale = 1.50;
	constexpr double SliderLandCoverageScale = 0.75;
	constexpr double SliderMountainScale = 1.25;
	constexpr double SliderCraterScale = 0.50;
	constexpr double SliderRoughnessScale = 1.50;
	constexpr const TCHAR* PreviewGuideMaterialPath =
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_PreviewGuide.M_APS_PreviewGuide");
	const FLinearColor ExpectedStarGuideColor(1.00f, 0.36f, 0.06f, 1.0f);
	const FLinearColor ExpectedSystemGuideColor(0.92f, 0.055f, 0.02f, 1.0f);
	constexpr float ExpectedStarGuideOpacity = 0.50f;
	constexpr float ExpectedSystemGuideOpacity = 0.46f;
	constexpr int32 ExpectedGuideRingCount = 3;
	constexpr int32 ExpectedGuideMajorSegments = 192;
	constexpr int32 ExpectedGuideMinorSegments = 8;
	// RenderOffscreen commandlets are substantially slower and noisier than the PIE
	// viewport. Keep a hard operational floor that still catches the reported 30-FPS
	// failure, and report the 75-FPS product target independently instead of making a
	// functionally valid closed orbital-proxy smoke hardware-dependent.
	constexpr double OperationalRegressionFloorFps = 40.0;
	constexpr double DesiredPreviewFps = 75.0;

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return *It;
			}
		}
		return nullptr;
	}

	struct FPreviewWorldScapeTopology
	{
		TArray<APlanetarySurfaceGenerator*> Generators;
		TArray<AWorldScapeRoot*> Roots;
		int32 VisibleRootCount{0};
		bool bHasGeneratorWithoutRoot{false};
		bool bHasRootWithoutGenerator{false};
		bool bHasDuplicateRootLink{false};
	};

	FPreviewWorldScapeTopology CapturePreviewWorldScapeTopology(
		UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		FPreviewWorldScapeTopology Result;
		if (!IsValid(World) || !IsValid(PreviewGenerator))
		{
			return Result;
		}

		for (TActorIterator<APlanetarySurfaceGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetOwner() == PreviewGenerator)
			{
				Result.Generators.Add(*It);
			}
		}
		for (TActorIterator<AWorldScapeRoot> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetOwner() == PreviewGenerator)
			{
				Result.Roots.Add(*It);
				if (!It->IsHidden())
				{
					++Result.VisibleRootCount;
				}
			}
		}

		TMap<AWorldScapeRoot*, int32> RootLinkCounts;
		for (APlanetarySurfaceGenerator* Generator : Result.Generators)
		{
			AWorldScapeRoot* Root = IsValid(Generator)
				? Generator->WorldScapeRootInstance : nullptr;
			if (!IsValid(Root) || !Result.Roots.Contains(Root))
			{
				Result.bHasGeneratorWithoutRoot = true;
				continue;
			}
			RootLinkCounts.FindOrAdd(Root) += 1;
		}
		for (AWorldScapeRoot* Root : Result.Roots)
		{
			const int32 LinkCount = RootLinkCounts.FindRef(Root);
			Result.bHasRootWithoutGenerator = Result.bHasRootWithoutGenerator
				|| LinkCount == 0;
			Result.bHasDuplicateRootLink = Result.bHasDuplicateRootLink
				|| LinkCount > 1;
		}

		return Result;
	}

	APlanetarySurfaceGenerator* FindPreviewSurfaceGenerator(
		UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		const FPreviewWorldScapeTopology Topology =
			CapturePreviewWorldScapeTopology(World, PreviewGenerator);
		// Menu preview owns one persistent profile resolver. Its linked WorldScape root
		// is deliberately hidden, so visibility can never be used to select the resolver.
		return Topology.Generators.Num() == 1 ? Topology.Generators[0] : nullptr;
	}

	bool BelongsToPreview(const AActor* Actor, const AAstroGenerator* PreviewGenerator)
	{
		return IsValid(Actor) && IsValid(PreviewGenerator)
			&& (Actor->GetOwner() == PreviewGenerator || Actor->IsAttachedTo(PreviewGenerator));
	}

	AStarSystem* FindGeneratedStarSystem(UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (TActorIterator<AStarSystem> It(World); It; ++It)
		{
			if (BelongsToPreview(*It, PreviewGenerator) && It->GetStars().Num() > 0)
			{
				return *It;
			}
		}
		return nullptr;
	}

	AStarCluster* FindGeneratedCluster(UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (TActorIterator<AStarCluster> It(World); It; ++It)
		{
			if (BelongsToPreview(*It, PreviewGenerator) && IsValid(It->StarMeshInstances)
				&& It->StarMeshInstances->GetInstanceCount() > 0)
			{
				return *It;
			}
		}
		return nullptr;
	}

	AGalaxy* FindGeneratedGalaxy(UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (TActorIterator<AGalaxy> It(World); It; ++It)
		{
			if (BelongsToPreview(*It, PreviewGenerator) && IsValid(It->StarMeshInstances)
				&& It->StarMeshInstances->GetInstanceCount() > 0)
			{
				return *It;
			}
		}
		return nullptr;
	}

	class FRenderedMenuPreviewCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRenderedMenuPreviewCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (TestStartSeconds <= 0.0)
			{
				TestStartSeconds = Now;
				StepStartSeconds = Now;
			}
			if (Step != 9 && Now - TestStartSeconds > WholeTestTimeoutSeconds)
			{
				return Fail(TEXT("Rendered MainMenu preview smoke timed out"));
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;

			switch (Step)
			{
			case 0:
				return UpdateMenuStartup(World, Controller, ViewModel, Now);
			case 1:
				return UpdateInitialPreview(World, ViewModel, Now);
			case 2:
				return UpdateFocusSequence(World, ViewModel, Controller, Now);
			case 3:
				return UpdateInitialSurface(World, ViewModel, Controller, Now);
			case 4:
				return UpdateChangedSurface(World, ViewModel, Controller, Now);
			case 5:
				return UpdateOrbitedSurface(World, ViewModel, Controller, Now);
			case 6:
				return UpdatePlanetPerformance(World, ViewModel, Now);
			case 7:
				return UpdateScreenshot(Now);
			case 8:
				return UpdateUnsupportedSurfaceCleanup(World, ViewModel, Now);
			case 9:
				return UpdateCleanup(Now);
			default:
				return true;
			}
		}

	private:
		static bool IsPresented(const UPrimitiveComponent* Component)
		{
			return IsValid(Component) && Component->IsVisible()
				&& !Component->bHiddenInGame;
		}

		static bool HasPresentedStaticMesh(AActor* Actor)
		{
			if (!IsValid(Actor))
			{
				return false;
			}
			TInlineComponentArray<UStaticMeshComponent*> StaticMeshes;
			Actor->GetComponents(StaticMeshes);
			for (const UStaticMeshComponent* StaticMesh : StaticMeshes)
			{
				if (IsPresented(StaticMesh))
				{
					return true;
				}
			}
			return false;
		}

		bool AssertSelectedBodyOrbitalLayersPresented(
			APlanetaryBody* Body, const FString& Context)
		{
			if (!IsValid(Body) || !PreviewGenerator.IsValid())
			{
				return false;
			}

			UProceduralMeshComponent* TerrainProxy =
				PreviewGenerator->GetPreviewTerrainProxyForBody(Body);
			if (!Test->TestNotNull(FString::Printf(
				TEXT("%s owns a closed orbital terrain proxy"), *Context), TerrainProxy))
			{
				return false;
			}
			Test->TestTrue(FString::Printf(
				TEXT("%s presents the closed orbital terrain proxy"), *Context),
				IsPresented(TerrainProxy));
			Test->TestNotNull(FString::Printf(
				TEXT("%s orbital terrain owns its closed mesh section"), *Context),
				TerrainProxy->GetProcMeshSection(0));
			Test->TestEqual(FString::Printf(
				TEXT("%s orbital terrain remains owned by the preview generator"), *Context),
				TerrainProxy->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));
			Test->TestEqual(FString::Printf(
				TEXT("%s orbital terrain has no gameplay collision"), *Context),
				TerrainProxy->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

			if (UProceduralMeshComponent* OceanProxy =
				PreviewGenerator->GetPreviewOceanProxyForBody(Body))
			{
				Test->TestTrue(FString::Printf(
					TEXT("%s presents its closed orbital ocean with terrain"), *Context),
					IsPresented(OceanProxy));
				Test->TestNotNull(FString::Printf(
					TEXT("%s orbital ocean owns its closed mesh section"), *Context),
					OceanProxy->GetProcMeshSection(0));
				Test->TestEqual(FString::Printf(
					TEXT("%s orbital ocean remains owned by the preview generator"), *Context),
					OceanProxy->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));
				Test->TestEqual(FString::Printf(
					TEXT("%s orbital ocean has no gameplay collision"), *Context),
					OceanProxy->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			}

			bool bBackingVisible = false;
			TInlineComponentArray<UStaticMeshComponent*> BackingMeshes;
			Body->GetComponents(BackingMeshes);
			for (UStaticMeshComponent* BackingMesh : BackingMeshes)
			{
				const bool bMeshVisible = IsPresented(BackingMesh);
				bBackingVisible = bBackingVisible || bMeshVisible;
				Test->TestFalse(FString::Printf(
					TEXT("%s hides authored backing mesh %s"), *Context,
					*GetNameSafe(BackingMesh)), bMeshVisible);
			}
			return IsPresented(TerrainProxy) && !bBackingVisible;
		}

		bool HasReadyPlanetOrbitalSurface(APlanetaryBody* Body,
			APlanetarySurfaceGenerator* Surface, AWorldScapeRoot* Root) const
		{
			UProceduralMeshComponent* TerrainProxy = IsValid(Body)
				&& PreviewGenerator.IsValid()
				? PreviewGenerator->GetPreviewTerrainProxyForBody(Body) : nullptr;
			return IsValid(Body) && IsValid(Surface) && IsValid(Root)
				&& Body->bWorldScapeSurfaceReady
				&& Surface->IsSurfaceProfileCurrent(Body)
				&& IsValid(TerrainProxy) && IsPresented(TerrainProxy)
				&& TerrainProxy->GetProcMeshSection(0) != nullptr
				&& PreviewGenerator->GetPreviewGlobeProfileSignature()
					== Surface->AppliedSurfaceProfileSignature
				&& Root->IsHidden() && !Root->IsActorTickEnabled()
				&& !Root->bGenerateWorldScape
				&& Root->WorldScapeLodInGeneration.Num() == 0;
		}

		bool AssertReadyPlanetOrbitalSurface(APlanetarySurfaceGenerator* Surface,
			AWorldScapeRoot* Root, APlanetaryBody* Body, const FString& Context)
		{
			if (!Test->TestNotNull(FString::Printf(
				TEXT("%s owns the persistent profile resolver"), *Context), Surface)
				|| !Test->TestNotNull(FString::Printf(
					TEXT("%s owns the persistent inert WorldScape root"), *Context), Root)
				|| !Test->TestNotNull(FString::Printf(
					TEXT("%s retains its selected planetary body"), *Context), Body))
			{
				return false;
			}

			Test->TestTrue(FString::Printf(
				TEXT("%s reports ready only for a committed orbital proxy"), *Context),
				HasReadyPlanetOrbitalSurface(Body, Surface, Root));
			Test->TestEqual(FString::Printf(
				TEXT("%s keeps the committed body selected"), *Context),
				PreviewGenerator->GetActivePreviewWorldScapeBody(), Body);
			Test->TestEqual(FString::Printf(
				TEXT("%s resolver targets the committed body"), *Context),
				Surface->PlanetaryBody, Body);
			Test->TestTrue(FString::Printf(
				TEXT("%s resolver profile is current"), *Context),
				Surface->IsSurfaceProfileCurrent(Body));
			Test->TestNotNull(FString::Printf(
				TEXT("%s resolver owns an immutable noise instance"), *Context),
				Surface->ResolvedNoiseInstance);
			Test->TestNotNull(FString::Printf(
				TEXT("%s resolver owns the canonical terrain MID"), *Context),
				Surface->ResolvedTerrainMaterialInstance);

			const FPreviewWorldScapeTopology Topology = CapturePreviewWorldScapeTopology(
				Root->GetWorld(), PreviewGenerator.Get());
			Test->TestEqual(FString::Printf(
				TEXT("%s owns exactly one profile resolver"), *Context),
				Topology.Generators.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s owns exactly one inert resolver root"), *Context),
				Topology.Roots.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s presents no WorldScape root in the menu"), *Context),
				Topology.VisibleRootCount, 0);
			Test->TestTrue(FString::Printf(
				TEXT("%s topology contains the selected resolver/root"), *Context),
				Topology.Generators.Contains(Surface) && Topology.Roots.Contains(Root));
			Test->TestFalse(FString::Printf(
				TEXT("%s leaves no orphan surface resolver"), *Context),
				Topology.bHasGeneratorWithoutRoot);
			Test->TestFalse(FString::Printf(
				TEXT("%s leaves no orphan resolver root"), *Context),
				Topology.bHasRootWithoutGenerator);
			Test->TestFalse(FString::Printf(
				TEXT("%s links the resolver root exactly once"), *Context),
				Topology.bHasDuplicateRootLink);
			Test->TestEqual(FString::Printf(
				TEXT("%s resolver remains linked to its persistent root"), *Context),
				Surface->WorldScapeRootInstance, Root);
			Test->TestEqual(FString::Printf(
				TEXT("%s resolver remains owned by the preview generator"), *Context),
				Surface->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));
			Test->TestEqual(FString::Printf(
				TEXT("%s inert root remains owned by the preview generator"), *Context),
				Root->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));

			Test->TestTrue(FString::Printf(
				TEXT("%s keeps the resolver root hidden"), *Context), Root->IsHidden());
			Test->TestFalse(FString::Printf(
				TEXT("%s inert resolver root never ticks"), *Context),
				Root->IsActorTickEnabled());
			Test->TestFalse(FString::Printf(
				TEXT("%s inert resolver root launches no generation"), *Context),
				Root->bGenerateWorldScape);
			Test->TestTrue(FString::Printf(
				TEXT("%s inert resolver root stays frozen"), *Context),
				Root->bFreezeGeneration);
			Test->TestFalse(FString::Printf(
				TEXT("%s inert resolver root has no collision"), *Context),
				Root->GetActorEnableCollision());
			Test->TestEqual(FString::Printf(
				TEXT("%s menu launches no WorldScape workers"), *Context),
				Root->WorldScapeLodInGeneration.Num(), 0);
			Test->TestEqual(FString::Printf(
				TEXT("%s resolver root retains the exact canonical terrain MID"), *Context),
				Root->TerrainMaterial.DefaultMaterial,
				static_cast<UMaterialInterface*>(Surface->ResolvedTerrainMaterialInstance));

			UProceduralMeshComponent* TerrainProxy =
				PreviewGenerator->GetPreviewTerrainProxyForBody(Body);
			if (!Test->TestNotNull(FString::Printf(
				TEXT("%s owns the committed closed terrain proxy"), *Context), TerrainProxy))
			{
				return false;
			}
			Test->TestEqual(FString::Printf(
				TEXT("%s body proxy is the active orbital terrain"), *Context),
				PreviewGenerator->GetActivePreviewTerrainProxy(), TerrainProxy);
			Test->TestEqual(FString::Printf(
				TEXT("%s orbital signature matches the resolved body profile"), *Context),
				PreviewGenerator->GetPreviewGlobeProfileSignature(),
				Surface->AppliedSurfaceProfileSignature);
			Test->TestTrue(FString::Printf(
				TEXT("%s orbital proxy has full closed-sphere topology"), *Context),
				PreviewGenerator->GetPreviewGlobeVertexCount() >= RequiredPreviewGlobeVertexCount
				&& PreviewGenerator->GetPreviewGlobeIndexCount() >= RequiredPreviewGlobeIndexCount);

			UMaterialInstanceDynamic* OrbitalTerrainMaterial =
				Cast<UMaterialInstanceDynamic>(TerrainProxy->GetMaterial(0));
			if (Test->TestNotNull(FString::Printf(
				TEXT("%s closed terrain owns an orbital MID"), *Context),
				OrbitalTerrainMaterial)
				&& IsValid(Surface->ResolvedTerrainMaterialInstance))
			{
				Test->TestNotEqual(FString::Printf(
					TEXT("%s orbital and physical terrain use distinct presentation masters"), *Context),
					OrbitalTerrainMaterial->GetMaterial(),
					Surface->ResolvedTerrainMaterialInstance->GetMaterial());
				Test->TestEqual(FString::Printf(
					TEXT("%s orbital terrain remains opaque"), *Context),
					OrbitalTerrainMaterial->GetBlendMode(), BLEND_Opaque);
			}

			UProceduralMeshComponent* OceanProxy =
				PreviewGenerator->GetPreviewOceanProxyForBody(Body);
			if (Root->bOcean)
			{
				Test->TestEqual(FString::Printf(
					TEXT("%s resolver root retains the exact canonical ocean MID"), *Context),
					Root->OceanMaterial.DefaultMaterial,
					static_cast<UMaterialInterface*>(Surface->ResolvedOceanMaterialInstance));
				if (Test->TestNotNull(FString::Printf(
					TEXT("%s owns the committed closed ocean proxy"), *Context), OceanProxy))
				{
					UMaterialInstanceDynamic* OrbitalOceanMaterial =
						Cast<UMaterialInstanceDynamic>(OceanProxy->GetMaterial(0));
					Test->TestTrue(FString::Printf(
						TEXT("%s closed ocean is presented with terrain"), *Context),
						IsPresented(OceanProxy) && OceanProxy->GetProcMeshSection(0) != nullptr);
					if (Test->TestNotNull(FString::Printf(
						TEXT("%s closed ocean owns an orbital MID"), *Context),
						OrbitalOceanMaterial)
						&& IsValid(Surface->ResolvedOceanMaterialInstance))
					{
						Test->TestNotEqual(FString::Printf(
							TEXT("%s orbital and physical oceans use distinct presentation masters"), *Context),
							OrbitalOceanMaterial->GetMaterial(),
							Surface->ResolvedOceanMaterialInstance->GetMaterial());
					}
				}
			}

			AssertSelectedBodyOrbitalLayersPresented(Body, Context);
			double PresentedRadius = 0.0;
			if (Test->TestTrue(FString::Printf(
				TEXT("%s publishes the orbital terrain presentation radius"), *Context),
				PreviewGenerator->GetPreviewPresentationRadius(Body, PresentedRadius)))
			{
				TerrainProxy->UpdateBounds();
				Test->TestTrue(FString::Printf(
					TEXT("%s closed terrain matches the published presentation radius"), *Context),
					FMath::IsNearlyEqual(TerrainProxy->Bounds.BoxExtent.GetMax(), PresentedRadius,
						FMath::Max(PresentedRadius * 0.01, 1.0)));
			}
			return true;
		}

		void BeginPlanetOrbitalSwapObservation(UWorld* World,
			APlanetaryBody* Body, const FString& Context)
		{
			const FPreviewWorldScapeTopology Topology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			Test->TestEqual(FString::Printf(
				TEXT("%s begins with one persistent resolver"), *Context),
				Topology.Generators.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s begins with one persistent inert root"), *Context),
				Topology.Roots.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s begins with no visible WorldScape root"), *Context),
				Topology.VisibleRootCount, 0);

			SwapPreviousSurfaceGenerator = Topology.Generators.Num() == 1
				? Topology.Generators[0] : nullptr;
			SwapPreviousWorldScapeRoot = Topology.Roots.Num() == 1
				? Topology.Roots[0] : nullptr;
			SwapPreviousTerrainProxy = PreviewGenerator.IsValid()
				? PreviewGenerator->GetPreviewTerrainProxyForBody(Body) : nullptr;
			SwapPreviousTerrainMaterial = SwapPreviousTerrainProxy.IsValid()
				? SwapPreviousTerrainProxy->GetMaterial(0) : nullptr;
			bOrbitalSwapObservationActive = SwapPreviousWorldScapeRoot.IsValid()
				&& SwapPreviousSurfaceGenerator.IsValid()
				&& SwapPreviousTerrainProxy.IsValid();
			bObservedOrbitalProxyCommit = false;
			bObservedOrbitalProxyContinuityViolation = false;
			bObservedResolverTopologyViolation = false;
			bObservedResolverMutationViolation = false;
			AssertSelectedBodyOrbitalLayersPresented(Body,
				FString::Printf(TEXT("%s pre-refresh"), *Context));
		}

		void ObservePlanetOrbitalSwap(UWorld* World, APlanetaryBody* Body)
		{
			if (!bOrbitalSwapObservationActive || !IsValid(World)
				|| !IsValid(Body) || !PreviewGenerator.IsValid())
			{
				return;
			}

			const FPreviewWorldScapeTopology Topology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			bObservedResolverTopologyViolation = bObservedResolverTopologyViolation
				|| Topology.Generators.Num() != 1 || Topology.Roots.Num() != 1
				|| Topology.VisibleRootCount != 0
				|| Topology.bHasGeneratorWithoutRoot
				|| Topology.bHasRootWithoutGenerator
				|| Topology.bHasDuplicateRootLink;

			APlanetarySurfaceGenerator* Resolver = Topology.Generators.Num() == 1
				? Topology.Generators[0] : nullptr;
			AWorldScapeRoot* ResolverRoot = Topology.Roots.Num() == 1
				? Topology.Roots[0] : nullptr;
			bObservedResolverMutationViolation = bObservedResolverMutationViolation
				|| Resolver != SwapPreviousSurfaceGenerator.Get()
				|| ResolverRoot != SwapPreviousWorldScapeRoot.Get()
				|| !IsValid(Resolver) || !IsValid(ResolverRoot)
				|| (IsValid(Resolver) && Resolver->WorldScapeRootInstance != ResolverRoot)
				|| (IsValid(ResolverRoot) && (!ResolverRoot->IsHidden()
					|| ResolverRoot->IsActorTickEnabled()
					|| ResolverRoot->bGenerateWorldScape
					|| !ResolverRoot->bFreezeGeneration
					|| ResolverRoot->WorldScapeLodInGeneration.Num() != 0));

			UProceduralMeshComponent* CurrentTerrain =
				PreviewGenerator->GetPreviewTerrainProxyForBody(Body);
			const bool bCommittedReplacement = IsValid(CurrentTerrain)
				&& CurrentTerrain != SwapPreviousTerrainProxy.Get();
			bObservedOrbitalProxyCommit = bObservedOrbitalProxyCommit
				|| bCommittedReplacement;
			bObservedOrbitalProxyContinuityViolation =
				bObservedOrbitalProxyContinuityViolation
				|| !IsPresented(CurrentTerrain) || HasPresentedStaticMesh(Body)
				|| (!Body->bWorldScapeSurfaceReady
					&& (CurrentTerrain != SwapPreviousTerrainProxy.Get()
						|| !IsPresented(SwapPreviousTerrainProxy.Get())))
				|| (bCommittedReplacement && IsPresented(SwapPreviousTerrainProxy.Get()));
		}

		void AssertPlanetOrbitalSwapCommitted(UWorld* World,
			APlanetarySurfaceGenerator* Surface, AWorldScapeRoot* Root,
			APlanetaryBody* Body, const FString& Context)
		{
			ObservePlanetOrbitalSwap(World, Body);
			const FPreviewWorldScapeTopology Topology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			UProceduralMeshComponent* NewTerrain =
				PreviewGenerator->GetPreviewTerrainProxyForBody(Body);
			Test->TestTrue(FString::Printf(
				TEXT("%s observes the atomic closed-globe front-buffer commit"), *Context),
				bObservedOrbitalProxyCommit);
			Test->TestFalse(FString::Printf(
				TEXT("%s keeps one complete orbital terrain visible throughout refresh"), *Context),
				bObservedOrbitalProxyContinuityViolation);
			Test->TestFalse(FString::Printf(
				TEXT("%s retains exactly one hidden linked resolver/root"), *Context),
				bObservedResolverTopologyViolation);
			Test->TestFalse(FString::Printf(
				TEXT("%s never replaces or activates the inert resolver/root"), *Context),
				bObservedResolverMutationViolation);
			Test->TestEqual(FString::Printf(
				TEXT("%s reuses the persistent profile resolver"), *Context),
				Surface, SwapPreviousSurfaceGenerator.Get());
			Test->TestEqual(FString::Printf(
				TEXT("%s reuses the persistent inert root"), *Context),
				Root, SwapPreviousWorldScapeRoot.Get());
			Test->TestTrue(FString::Printf(
				TEXT("%s commits into the alternate terrain buffer"), *Context),
				IsValid(NewTerrain) && NewTerrain != SwapPreviousTerrainProxy.Get());
			Test->TestTrue(FString::Printf(
				TEXT("%s retains the preceding terrain buffer for reuse"), *Context),
				SwapPreviousTerrainProxy.IsValid()
				&& SwapPreviousTerrainProxy->GetProcMeshSection(0) != nullptr
				&& SwapPreviousTerrainProxy->GetMaterial(0)
					== SwapPreviousTerrainMaterial.Get());
			Test->TestFalse(FString::Printf(
				TEXT("%s hides the preceding terrain buffer after commit"), *Context),
				IsPresented(SwapPreviousTerrainProxy.Get()));
			Test->TestTrue(FString::Printf(
				TEXT("%s presents the newly committed terrain buffer"), *Context),
				IsPresented(NewTerrain));
			Test->TestEqual(FString::Printf(
				TEXT("%s leaves one profile resolver after commit"), *Context),
				Topology.Generators.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s leaves one inert resolver root after commit"), *Context),
				Topology.Roots.Num(), 1);
			Test->TestEqual(FString::Printf(
				TEXT("%s leaves the resolver root hidden after commit"), *Context),
				Topology.VisibleRootCount, 0);
			CurrentResolverRoot = Root;
			CurrentSurfaceResolver = Surface;
			bOrbitalSwapObservationActive = false;
		}

		bool AssertAtmospherePresentation(APlanetaryBody* Body, AAtmoScape* Atmosphere,
			const FString& Context, UStaticMeshComponent*& OutSpaceShell,
			double& OutPresentedRadius)
		{
			OutSpaceShell = nullptr;
			OutPresentedRadius = 0.0;
			if (!Test->TestNotNull(FString::Printf(
				TEXT("%s has an atmosphere actor"), *Context), Atmosphere)
				|| !IsValid(Body) || !PreviewGenerator.IsValid())
			{
				return false;
			}

			FVector PresentedCenter = FVector::ZeroVector;
			const bool bHasCenter = PreviewGenerator->GetPreviewPresentationLocation(
				Body, PresentedCenter);
			const bool bHasRadius = PreviewGenerator->GetPreviewPresentationRadius(
				Body, OutPresentedRadius);
			Test->TestTrue(FString::Printf(
				TEXT("%s publishes its atmosphere centre"), *Context), bHasCenter);
			Test->TestTrue(FString::Printf(
				TEXT("%s publishes its atmosphere terrain radius"), *Context), bHasRadius);

			TInlineComponentArray<UStaticMeshComponent*> AtmosphereMeshes;
			Atmosphere->GetComponents(AtmosphereMeshes);
			for (UStaticMeshComponent* Mesh : AtmosphereMeshes)
			{
				if (IsValid(Mesh) && Mesh->GetName().Contains(TEXT("SpacePlanetaryAtmoMesh")))
				{
					OutSpaceShell = Mesh;
					break;
				}
			}
			if (!Test->TestNotNull(FString::Printf(
				TEXT("%s exposes its space-view atmosphere shell"), *Context), OutSpaceShell)
				|| !bHasCenter || !bHasRadius || OutPresentedRadius <= UE_SMALL_NUMBER)
			{
				return false;
			}

			OutSpaceShell->UpdateBounds();
			const double ShellRadius = OutSpaceShell->Bounds.BoxExtent.GetMax();
			const double ShellRatio = ShellRadius / OutPresentedRadius;
			const double CenterError = FVector::Distance(
				OutSpaceShell->Bounds.Origin, PresentedCenter);
			UProceduralMeshComponent* TerrainProxy =
				PreviewGenerator->GetActivePreviewTerrainProxy();
			if (Test->TestNotNull(FString::Printf(
				TEXT("%s exposes the selected orbital terrain"), *Context), TerrainProxy))
			{
				TerrainProxy->UpdateBounds();
				const double TerrainRadius = TerrainProxy->Bounds.BoxExtent.GetMax();
				Test->TestTrue(FString::Printf(
					TEXT("%s rendered terrain matches its semantic presentation radius"), *Context),
					FMath::IsNearlyEqual(TerrainRadius, OutPresentedRadius,
						FMath::Max(OutPresentedRadius * 0.01, 1.0)));
			}
			Test->TestTrue(FString::Printf(
				TEXT("%s atmosphere shell is visible"), *Context),
				OutSpaceShell->IsVisible() && !OutSpaceShell->bHiddenInGame);
			Test->TestFalse(FString::Printf(
				TEXT("%s atmosphere keeps physical UI scaling"), *Context),
				Atmosphere->bKeepRelativeScale);
			Test->TestTrue(FString::Printf(
				TEXT("%s atmosphere halo clears the live terrain"), *Context),
				ShellRatio >= 1.001);
			Test->TestTrue(FString::Printf(
				TEXT("%s atmosphere halo cannot become a giant sphere"), *Context),
				ShellRatio <= 1.04);
			const double PhysicalRadiusKm = FMath::Max(
				Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM));
			const double ExpectedHeightRatio = FMath::Clamp(
				static_cast<double>(FMath::Max(Atmosphere->AtmosphereHeight, 0.0f))
					/ FMath::Max(PhysicalRadiusKm, 1.0), 0.001, 0.04);
			Test->TestTrue(FString::Printf(
				TEXT("%s atmosphere uses the authored physical thickness"), *Context),
				FMath::Abs(ShellRatio - (1.0 + ExpectedHeightRatio)) <= 5.0e-4);
			Test->TestTrue(FString::Printf(
				TEXT("%s atmosphere shell is centred on the presented body"), *Context),
				CenterError <= FMath::Max(OutPresentedRadius * 1.0e-5, 1.0));
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] %s atmosphere planetRadius=%.0f shellRadius=%.0f ratio=%.4f centerError=%.3f"),
				*Context, OutPresentedRadius, ShellRadius,
				ShellRatio, CenterError);
			return true;
		}

		void PrepareCleanRenderWarmup()
		{
			if (bRenderWarmupPrepared)
			{
				return;
			}
			bRenderWarmupPrepared = true;

			for (const TCHAR* CVarName : {TEXT("r.VSync"), TEXT("rhi.SyncInterval"),
				TEXT("t.MaxFPS"), TEXT("t.IdleWhenNotForeground"),
				TEXT("r.DontLimitOnBattery")})
			{
				if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(CVarName))
				{
					CVar->Set(FCString::Strcmp(CVarName, TEXT("r.DontLimitOnBattery")) == 0 ? 1 : 0,
						ECVF_SetByCode);
				}
			}

			UAPSPlanetSurfaceCatalog* Catalog = LoadObject<UAPSPlanetSurfaceCatalog>(nullptr,
				TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/DA_PlanetSurfaceCatalog.DA_PlanetSurfaceCatalog"));
			const auto KeepResident = [this](UObject* Asset)
			{
				if (IsValid(Asset) && !Asset->IsRooted())
				{
					Asset->AddToRoot();
					WarmedSurfaceAssets.Add(Asset);
				}
			};
			KeepResident(Catalog);
			if (Catalog)
			{
				for (const TPair<EAPSPlanetSurfaceArchetype,
					FAPSPlanetSurfaceArchetypeDefinition>& Entry : Catalog->Archetypes)
				{
					KeepResident(Entry.Value.TerrainMaterial.LoadSynchronous());
					KeepResident(Entry.Value.OceanMaterial.LoadSynchronous());
				}
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Requested catalog/material warmup assets=%d"),
				WarmedSurfaceAssets.Num());
		}

		bool WaitForCompilationIdle(double Now, const TCHAR* Context)
		{
			if (CompilationWaitContext != Context)
			{
				CompilationWaitContext = Context;
				CompilationIdleSince = 0.0;
				LastCompilationLogSeconds = 0.0;
			}
			const int32 RemainingAssets =
				FAssetCompilingManager::Get().GetNumRemainingAssets();
			if (RemainingAssets > 0)
			{
				CompilationIdleSince = 0.0;
				if (LastCompilationLogSeconds <= 0.0 || Now - LastCompilationLogSeconds >= 5.0)
				{
					LastCompilationLogSeconds = Now;
					UE_LOG(LogTemp, Display,
						TEXT("[APS.Smoke.Warmup] context=%s remaining=%d"),
						Context, RemainingAssets);
				}
				return false;
			}
			if (CompilationIdleSince <= 0.0)
			{
				CompilationIdleSince = Now;
				return false;
			}
			if (Now - CompilationIdleSince < 1.5)
			{
				return false;
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke.Warmup] context=%s idle"), Context);
			return true;
		}

		bool Fail(const FString& Message)
		{
			if (PendingFailure.IsEmpty())
			{
				PendingFailure = Message;
				UE_LOG(LogTemp, Error, TEXT("[APS.Smoke] %s; draining WorldScape before exit"), *Message);
			}
			Step = 8;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool UpdateMenuStartup(UWorld* World, AMainMenuController* Controller,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !Controller || !ViewModel || !ViewModel->GeneratedWorld)
			{
				return false;
			}
			PrepareCleanRenderWarmup();
			if (!WaitForCompilationIdle(Now, TEXT("MENU_ASSETS")))
			{
				return false;
			}

			UGeneratedWorld* Model = ViewModel->GeneratedWorld;
			Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
			Model->bGenerateFullScaledWorld = true;
			Model->bGenerateHomeSystem = true;
			Model->bStartWithHomePlanet = true;
			Model->GalaxyType = EGalaxyType::Spiral;
			Model->GalaxyClass = EGalaxyClass::Sc;
			Model->GalaxySize = 17033;
			Model->GalaxyStarCount = 9382900;
			Model->GalaxyStarDensity = 156.8;
			Model->StarClusterSize = EStarClusterSize::Giant;
			Model->StarClusterType = EStarClusterType::RingArc;
			Model->StarType = EStarType::TripleStar;
			Model->PlanetsAmount = 6;
			Model->MoonsAmount = 2;
			Model->StartPlanetIndex = 1;
			Model->PlanetRadius = 6750.0;
			Model->PlanetType = EPlanetType::Ocean;
			Model->PlanetSurfaceSeed = 42042;
			Model->AtmosphereHeight = 140.0;
			Model->AtmosphereOpacity = 12.0;

			if (!Controller->OpenAstronomicalGenerationForAutomation(
				EAstroPreviewFocus::Galaxy, EAPSGenerationRoute::Space))
			{
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Opened rendered astronomical generator page world=%s"),
				*GetNameSafe(World));
			Step = 1;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateInitialPreview(UWorld* World, UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !ViewModel || !ViewModel->bPreviewReady)
			{
				return false;
			}
			if (!WaitForCompilationIdle(Now, TEXT("GENERATED_HIERARCHY")))
			{
				return false;
			}
			PreviewGenerator = FindPreviewGenerator(World);
			if (!PreviewGenerator.IsValid())
			{
				return Fail(TEXT("Preview reports ready but no tagged AAstroGenerator exists"));
			}

			AAstroGenerator* Generator = PreviewGenerator.Get();
			Test->TestTrue(TEXT("Orbital materials are resident before PLANET focus"),
				Generator->ArePreviewMaterialAssetsWarmed());
			Test->TestTrue(TEXT("Galaxy HISM preview is populated"),
				Generator->GetPreviewGalaxyRenderedStarCount() > 0);
			Test->TestTrue(TEXT("Galaxy preview respects its 1800-instance budget"),
				Generator->GetPreviewGalaxyRenderedStarCount() <= 1800);
			Test->TestTrue(TEXT("Cluster HISM preview is populated"),
				Generator->GetPreviewClusterRenderedStarCount() > 0);
			Test->TestTrue(TEXT("Cluster preview respects its 1600-instance budget"),
				Generator->GetPreviewClusterRenderedStarCount() <= 1600);
			if (AGalaxy* Galaxy = FindGeneratedGalaxy(World, Generator))
			{
				InitialGalaxyInstanceCount = Galaxy->StarMeshInstances->GetInstanceCount();
			}
			if (AStarCluster* Cluster = FindGeneratedCluster(World, Generator))
			{
				InitialClusterInstanceCount = Cluster->StarMeshInstances->GetInstanceCount();
			}

			FVector GalaxyCenter;
			FVector ClusterCenter;
			FVector SystemCenter;
			double GalaxyRadius = 0.0;
			double ClusterRadius = 0.0;
			double SystemRadius = 0.0;
			const bool bGalaxySphere = Generator->GetPreviewFocusSphere(
				EAstroPreviewFocus::Galaxy, GalaxyCenter, GalaxyRadius);
			const bool bClusterSphere = Generator->GetPreviewFocusSphere(
				EAstroPreviewFocus::StarCluster, ClusterCenter, ClusterRadius);
			const bool bSystemSphere = Generator->GetPreviewFocusSphere(
				EAstroPreviewFocus::HomeSystem, SystemCenter, SystemRadius);
			Test->TestTrue(TEXT("Galaxy focus sphere is valid"), bGalaxySphere);
			Test->TestTrue(TEXT("Cluster focus sphere is valid"), bClusterSphere);
			Test->TestTrue(TEXT("System focus sphere is valid"), bSystemSphere);
			constexpr double MaxNormalizedPreviewRadius = 5.05e10;
			if (bGalaxySphere)
			{
				Test->TestTrue(TEXT("Full-scale galaxy is normalized into the safe preview envelope"),
					FMath::IsFinite(GalaxyRadius) && GalaxyRadius <= MaxNormalizedPreviewRadius);
			}
			if (bClusterSphere)
			{
				Test->TestTrue(TEXT("Full-scale cluster is normalized into the safe preview envelope"),
					FMath::IsFinite(ClusterRadius) && ClusterRadius <= MaxNormalizedPreviewRadius);
			}
			if (bSystemSphere)
			{
				Test->TestTrue(TEXT("Full-scale system is normalized into the safe preview envelope"),
					FMath::IsFinite(SystemRadius) && SystemRadius <= MaxNormalizedPreviewRadius);
			}
			if (bGalaxySphere && bClusterSphere)
			{
				Test->TestTrue(TEXT("Home cluster is centered inside its galaxy"),
					FVector::Distance(GalaxyCenter, ClusterCenter) <= GalaxyRadius * 0.05);
				Test->TestTrue(TEXT("Cluster envelope fits inside its parent galaxy"),
					FVector::Distance(GalaxyCenter, ClusterCenter) + ClusterRadius
						<= GalaxyRadius * 1.05);
			}
			if (bClusterSphere && bSystemSphere)
			{
				Test->TestTrue(TEXT("Home system remains at a point inside its cluster"),
					FVector::Distance(ClusterCenter, SystemCenter) <= ClusterRadius * 1.01);
				Test->TestTrue(TEXT("Home-system focus radius remains smaller than its cluster"),
					SystemRadius < ClusterRadius);
			}

			// Pick one non-culled address from each parent point field. These sentinels
			// prove that focus changes preserve catalogue locations while detail scopes
			// are free to reduce the proxy's mesh-only angular LOD. Index zero alone can
			// legitimately fall inside the system safe zone.
			const auto CaptureExteriorSentinel = [&](UHierarchicalInstancedStaticMeshComponent* Hism,
				int32& OutIndex, FVector& OutLocation, FVector& OutScale,
				float& OutEmission, const TCHAR* LayerName)
			{
				OutIndex = INDEX_NONE;
				OutEmission = 0.0f;
				if (!bSystemSphere || !IsValid(Hism)) return;
				const UStaticMesh* ProxyMesh = Hism->GetStaticMesh();
				const double MeshRadius = ProxyMesh
					? FMath::Max(static_cast<double>(ProxyMesh->GetBounds().SphereRadius), 1.0)
					: 1.0;
				for (int32 InstanceIndex = 0; InstanceIndex < Hism->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform WorldTransform;
					if (!Hism->GetInstanceTransform(InstanceIndex, WorldTransform, true)
						|| WorldTransform.GetScale3D().IsNearlyZero())
					{
						continue;
					}
					const double ProxyRadius = MeshRadius * WorldTransform.GetScale3D().GetAbsMax();
					if (FVector::Distance(WorldTransform.GetLocation(), SystemCenter) - ProxyRadius
						<= SystemRadius * 1.20)
					{
						continue;
					}
					OutIndex = InstanceIndex;
					OutLocation = WorldTransform.GetLocation();
					OutScale = WorldTransform.GetScale3D();
					const int32 EmissionOffset = InstanceIndex * Hism->NumCustomDataFloats + 3;
					if (Hism->NumCustomDataFloats > 3
						&& Hism->PerInstanceSMCustomData.IsValidIndex(EmissionOffset))
					{
						OutEmission = Hism->PerInstanceSMCustomData[EmissionOffset];
					}
					break;
				}
				Test->TestTrue(FString::Printf(TEXT("%s owns an exterior invariant sentinel"),
					LayerName), OutIndex != INDEX_NONE);
				const int32 EmissionOffset = OutIndex * Hism->NumCustomDataFloats + 3;
				const bool bHasEmission = OutIndex != INDEX_NONE
					&& Hism->NumCustomDataFloats > 3
					&& Hism->PerInstanceSMCustomData.IsValidIndex(EmissionOffset);
				Test->TestTrue(FString::Printf(TEXT("%s sentinel owns finite emissive custom data"),
					LayerName), bHasEmission && FMath::IsFinite(OutEmission));
			};
			AGalaxy* InitialGalaxy = FindGeneratedGalaxy(World, Generator);
			AStarCluster* InitialCluster = FindGeneratedCluster(World, Generator);
			CaptureExteriorSentinel(InitialGalaxy ? InitialGalaxy->StarMeshInstances : nullptr,
				InitialGalaxySentinelIndex, InitialGalaxySentinelLocation,
				InitialGalaxySentinelScale, InitialGalaxySentinelEmission, TEXT("Galaxy"));
			CaptureExteriorSentinel(InitialCluster ? InitialCluster->StarMeshInstances : nullptr,
				InitialClusterSentinelIndex, InitialClusterSentinelLocation,
				InitialClusterSentinelScale, InitialClusterSentinelEmission, TEXT("Cluster"));

			AStarSystem* StarSystem = FindGeneratedStarSystem(World, Generator);
			if (Test->TestNotNull(TEXT("Triple star system is materialized"), StarSystem))
			{
				InvariantHierarchyActors.Reset();
				InitialInvariantActorLocations.Reset();
				TArray<AActor*> PendingActors{StarSystem};
				TSet<AActor*> VisitedActors;
				while (!PendingActors.IsEmpty())
				{
					AActor* Actor = PendingActors.Pop(EAllowShrinking::No);
					if (!IsValid(Actor) || VisitedActors.Contains(Actor)) continue;
					VisitedActors.Add(Actor);
					if (Actor->IsA<AStarSystem>() || Actor->IsA<AStar>()
						|| Actor->IsA<APlanetaryBody>())
					{
						InvariantHierarchyActors.Add(Actor);
						InitialInvariantActorLocations.Add(Actor->GetActorLocation());
					}
					TArray<AActor*> Children;
					Actor->GetAttachedActors(Children, true, false);
					PendingActors.Append(Children);
				}
				if (bSystemSphere
					&& Test->TestNotNull(TEXT("Materialized system owns its native zone"),
						StarSystem->StarSystemZone))
				{
					const double NativeScaledRadius =
						StarSystem->StarSystemZone->GetScaledSphereRadius();
					Test->TestTrue(
						TEXT("Native system safe zone remains finite data after presentation scaling"),
						FMath::IsFinite(NativeScaledRadius) && NativeScaledRadius > UE_SMALL_NUMBER);
					Test->TestTrue(TEXT("Native system safe zone stays hidden behind the presentation shell"),
						!StarSystem->StarSystemZone->IsVisible()
						&& StarSystem->StarSystemZone->bHiddenInGame);
				}
				Test->TestEqual(TEXT("Triple-star mode creates exactly three stars"),
					StarSystem->GetStars().Num(), 3);
				TSet<const APlanet*> UniquePlanets;
				for (const AStar* Star : StarSystem->GetStars())
				{
					const APlanetarySystem* PlanetarySystem = Star ? Star->PlanetarySystem : nullptr;
					Test->TestNotNull(TEXT("Every star owns a planetary hierarchy"), PlanetarySystem);
					if (!PlanetarySystem)
					{
						continue;
					}
					Test->TestEqual(TEXT("Orbit count equals requested planets per star"),
						PlanetarySystem->PlanetOrbitsList.Num(), 6);
					for (const APlanetOrbit* Orbit : PlanetarySystem->PlanetOrbitsList)
					{
						Test->TestNotNull(TEXT("Every generated orbit owns one planet"),
							Orbit ? Orbit->Planet : nullptr);
						if (Orbit && Orbit->Planet)
						{
							Test->TestFalse(TEXT("No planet is duplicated between orbit records"),
								UniquePlanets.Contains(Orbit->Planet));
							UniquePlanets.Add(Orbit->Planet);
						}
					}
				}
				Test->TestEqual(TEXT("Triple system owns exactly eighteen unique planets"),
					UniquePlanets.Num(), 18);
			}

			if (AStarCluster* Cluster = FindGeneratedCluster(World, Generator); Cluster && StarSystem)
			{
				double NearestInstanceDistance = TNumericLimits<double>::Max();
				for (int32 InstanceIndex = 0;
					InstanceIndex < Cluster->StarMeshInstances->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					if (Cluster->StarMeshInstances->GetInstanceTransform(
						InstanceIndex, InstanceTransform, true))
					{
						const double CandidateDistance = FVector::Distance(
							InstanceTransform.GetLocation(), StarSystem->GetActorLocation());
						if (CandidateDistance < NearestInstanceDistance)
						{
							NearestInstanceDistance = CandidateDistance;
							HomeClusterProxyIndex = InstanceIndex;
							HomeClusterProxyScale = InstanceTransform.GetScale3D();
						}
					}
				}
				Test->TestTrue(TEXT("Materialized home system coincides with a cluster HISM point"),
					NearestInstanceDistance <= FMath::Max(10.0, ClusterRadius * 1.0e-5));
				Test->TestTrue(TEXT("Distant materialized home system retains a visible HISM proxy"),
					HomeClusterProxyIndex != INDEX_NONE && !HomeClusterProxyScale.IsNearlyZero());
				if (HomeClusterProxyIndex != INDEX_NONE && IsValid(StarSystem->MainStar))
				{
					const UHierarchicalInstancedStaticMeshComponent* Hism =
						Cluster->StarMeshInstances;
					if (const FClusterStarSystemRecord* HomeRecord =
						Cluster->FindPotentialSystem(HomeClusterProxyIndex))
					{
						Test->TestEqual(TEXT("Materialized cluster record keeps the actual star count"),
							HomeRecord->SystemModel.AmountOfStars, 3);
						Test->TestEqual(TEXT("Materialized cluster record keeps the actual total planet count"),
							HomeRecord->SystemModel.PotentialPlanetCount, 18);
					}
					else
					{
						Test->AddError(TEXT("Home HISM marker has no matching cluster system record"));
					}
					const int32 ColorOffset = HomeClusterProxyIndex * Hism->NumCustomDataFloats;
					const bool bHasRgb = Hism->NumCustomDataFloats >= 3
						&& Hism->PerInstanceSMCustomData.IsValidIndex(ColorOffset + 2);
					Test->TestTrue(TEXT("Home cluster marker owns RGB custom data"), bHasRgb);
					if (bHasRgb)
					{
						const FLinearColor ExpectedColor = UStarGenerator::GetStarColor(
							StarSystem->MainStar->SpectralClass,
							StarSystem->MainStar->SpectralSubclass);
						Test->TestTrue(TEXT("Home cluster marker keeps the materialized primary-star color"),
							FMath::IsNearlyEqual(Hism->PerInstanceSMCustomData[ColorOffset], ExpectedColor.R, 1.0e-4f)
							&& FMath::IsNearlyEqual(Hism->PerInstanceSMCustomData[ColorOffset + 1], ExpectedColor.G, 1.0e-4f)
							&& FMath::IsNearlyEqual(Hism->PerInstanceSMCustomData[ColorOffset + 2], ExpectedColor.B, 1.0e-4f));
					}
					const int32 OccupancyOffset = ColorOffset + 5;
					Test->TestTrue(TEXT("Home cluster marker occupancy matches the materialized planet count"),
						Hism->NumCustomDataFloats > 5
						&& Hism->PerInstanceSMCustomData.IsValidIndex(OccupancyOffset)
						&& FMath::IsNearlyEqual(
							Hism->PerInstanceSMCustomData[OccupancyOffset], 18.0f / 12.0f, 1.0e-4f));
				}
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Initial hierarchy ready in %.2fs galaxy=%d cluster=%d"),
				Now - StepStartSeconds, Generator->GetPreviewGalaxyRenderedStarCount(),
				Generator->GetPreviewClusterRenderedStarCount());
			FocusIndex = 0;
			BeginFocus(ViewModel, Now);
			Step = 2;
			return false;
		}

		void BeginFocus(UWorldGenerationViewModel* ViewModel, double Now)
		{
			static constexpr EAstroPreviewFocus FocusSequence[] = {
				EAstroPreviewFocus::Galaxy,
				EAstroPreviewFocus::StarCluster,
				EAstroPreviewFocus::HomeSystem,
				EAstroPreviewFocus::HomeStar,
				EAstroPreviewFocus::HomePlanet,
				// Return through both distant scopes to prove that detail-only HISM
				// scale/emission changes are reversible, then finish in PLANET.
				EAstroPreviewFocus::StarCluster,
				EAstroPreviewFocus::Galaxy,
				EAstroPreviewFocus::HomePlanet
			};
			bHasPreviousScopeViewDirection = false;
			FVector PreviousCenter;
			double PreviousRadius = 0.0;
			UWorld* World = ViewModel ? ViewModel->GetWorld() : nullptr;
			APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
			if (ViewModel && ViewModel->GetPreviewFocusSphere(PreviousCenter, PreviousRadius)
				&& IsValid(PlayerController) && IsValid(PlayerController->PlayerCameraManager))
			{
				PreviousScopeViewDirection = (
					PlayerController->PlayerCameraManager->GetCameraLocation() - PreviousCenter).GetSafeNormal();
				bHasPreviousScopeViewDirection = !PreviousScopeViewDirection.IsNearlyZero()
					&& !PreviousScopeViewDirection.ContainsNaN();
			}
			BeginOperationFrameTracking(Now);
			ViewModel->SetPreviewFocus(FocusSequence[FocusIndex]);
			StepStartSeconds = Now;
			bSampling = false;
		}

		void AssertNativeSystemZonesHidden(UWorld* World, EAstroPreviewFocus Focus)
		{
			AStarSystem* System = FindGeneratedStarSystem(World, PreviewGenerator.Get());
			const FString ScopeName = UEnum::GetValueAsString(Focus);
			if (!Test->TestNotNull(
				FString::Printf(TEXT("%s retains its materialized star system"), *ScopeName), System))
			{
				return;
			}

			Test->TestTrue(
				FString::Printf(TEXT("%s keeps the native star-system zone hidden"), *ScopeName),
				IsValid(System->StarSystemZone)
				&& !System->StarSystemZone->IsVisible()
				&& System->StarSystemZone->bHiddenInGame);
			for (int32 StarIndex = 0; StarIndex < System->GetStars().Num(); ++StarIndex)
			{
				const AStar* Star = System->GetStars()[StarIndex];
				Test->TestTrue(
					FString::Printf(TEXT("%s keeps native stellar zone %d hidden"),
						*ScopeName, StarIndex),
					IsValid(Star) && IsValid(Star->PlanetarySystemZone)
					&& !Star->PlanetarySystemZone->IsVisible()
					&& Star->PlanetarySystemZone->bHiddenInGame);
			}
		}

		void AssertBackgroundContextOutsideSystem(UWorld* World, EAstroPreviewFocus Focus)
		{
			FVector SystemCenter;
			double SystemRadius = 0.0;
			const FString ScopeName = UEnum::GetValueAsString(Focus);
			if (!Test->TestTrue(
				FString::Printf(TEXT("%s has a valid home-system safe zone"), *ScopeName),
				PreviewGenerator->GetPreviewFocusSphere(
					EAstroPreviewFocus::HomeSystem, SystemCenter, SystemRadius)
				&& FMath::IsFinite(SystemRadius) && SystemRadius > 0.0))
			{
				return;
			}
			FVector DetailFocusCenter = SystemCenter;
			if ((Focus == EAstroPreviewFocus::HomeStar
				|| Focus == EAstroPreviewFocus::HomePlanet)
				&& PreviewGenerator->GetSelectedPreviewBodyActor())
			{
				PreviewGenerator->GetPreviewPresentationLocation(
					PreviewGenerator->GetSelectedPreviewBodyActor(), DetailFocusCenter);
			}
			double AngularRadiusLimitDegrees = 0.0;
			float EmissionLimit = TNumericLimits<float>::Max();
			switch (Focus)
			{
			case EAstroPreviewFocus::HomeSystem:
				AngularRadiusLimitDegrees = 0.035;
				EmissionLimit = 24.0f;
				break;
			case EAstroPreviewFocus::HomeStar:
				AngularRadiusLimitDegrees = 0.022;
				EmissionLimit = 12.0f;
				break;
			case EAstroPreviewFocus::HomePlanet:
				AngularRadiusLimitDegrees = 0.015;
				EmissionLimit = 6.0f;
				break;
			default: break;
			}

			const auto CheckLayer = [&](UHierarchicalInstancedStaticMeshComponent* Hism,
				const TCHAR* LayerName)
			{
				if (!Test->TestNotNull(
					FString::Printf(TEXT("%s retains the %s context HISM"),
						*ScopeName, LayerName), Hism))
				{
					return;
				}
				Test->TestTrue(
					FString::Printf(TEXT("%s keeps the %s context HISM visible"),
						*ScopeName, LayerName),
					Hism->IsVisible() && !Hism->bHiddenInGame);

				const UStaticMesh* ProxyMesh = Hism->GetStaticMesh();
				const double MeshRadius = ProxyMesh
					? FMath::Max(static_cast<double>(ProxyMesh->GetBounds().SphereRadius), 1.0)
					: 1.0;
				int32 VisibleExteriorProxyCount = 0;
				int32 IntrudingVisibleProxyCount = 0;
				int32 OversizedVisibleProxyCount = 0;
				int32 OverbrightVisibleProxyCount = 0;
				for (int32 InstanceIndex = 0;
					InstanceIndex < Hism->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					if (!Hism->GetInstanceTransform(InstanceIndex, InstanceTransform, true)
						|| InstanceTransform.GetScale3D().IsNearlyZero())
					{
						continue;
					}
					const double ProxyRadius = MeshRadius
						* InstanceTransform.GetScale3D().GetAbsMax();
					const double DetailDistance = FVector::Distance(
						InstanceTransform.GetLocation(), DetailFocusCenter);
					if (AngularRadiusLimitDegrees > 0.0 && DetailDistance > UE_SMALL_NUMBER)
					{
						const double AngularRadiusDegrees = FMath::RadiansToDegrees(
							FMath::Atan2(ProxyRadius, DetailDistance));
						if (AngularRadiusDegrees > AngularRadiusLimitDegrees + 0.001)
						{
							++OversizedVisibleProxyCount;
						}
					}
					const int32 EmissionOffset = InstanceIndex * Hism->NumCustomDataFloats + 3;
					const bool bHasEmission = Hism->NumCustomDataFloats > 3
						&& Hism->PerInstanceSMCustomData.IsValidIndex(EmissionOffset);
					const float Emission = bHasEmission
						? Hism->PerInstanceSMCustomData[EmissionOffset]
						: TNumericLimits<float>::Max();
					if (!bHasEmission || !FMath::IsFinite(Emission)
						|| Emission > EmissionLimit + 0.01f)
					{
						++OverbrightVisibleProxyCount;
					}
					if (FVector::Distance(InstanceTransform.GetLocation(), SystemCenter)
						- ProxyRadius > SystemRadius * 1.099)
					{
						++VisibleExteriorProxyCount;
					}
					else
					{
						++IntrudingVisibleProxyCount;
					}
				}
				Test->TestTrue(
					FString::Printf(TEXT("%s keeps a visible nonzero-scale %s proxy outside the home-system safe zone"),
						*ScopeName, LayerName),
					VisibleExteriorProxyCount > 0);
				Test->TestEqual(
					FString::Printf(TEXT("%s culls every %s proxy intersecting the home-system safe zone"),
						*ScopeName, LayerName),
					IntrudingVisibleProxyCount, 0);
				Test->TestEqual(
					FString::Printf(TEXT("%s caps every visible %s proxy to its detail-scope angular radius"),
						*ScopeName, LayerName),
					OversizedVisibleProxyCount, 0);
				Test->TestEqual(
					FString::Printf(TEXT("%s caps every visible %s proxy emission for its detail scope"),
						*ScopeName, LayerName),
					OverbrightVisibleProxyCount, 0);
			};

			AGalaxy* Galaxy = FindGeneratedGalaxy(World, PreviewGenerator.Get());
			AStarCluster* Cluster = FindGeneratedCluster(World, PreviewGenerator.Get());
			CheckLayer(Galaxy ? Galaxy->StarMeshInstances : nullptr, TEXT("galaxy"));
			CheckLayer(Cluster ? Cluster->StarMeshInstances : nullptr, TEXT("cluster"));
		}

		void AssertHierarchyPresentationInvariants(UWorld* World, EAstroPreviewFocus Focus)
		{
			const FString ScopeName = UEnum::GetValueAsString(Focus);
			Test->TestEqual(FString::Printf(TEXT("%s retains invariant hierarchy actor count"),
				*ScopeName), InvariantHierarchyActors.Num(), InitialInvariantActorLocations.Num());
			for (int32 ActorIndex = 0;
				ActorIndex < FMath::Min(InvariantHierarchyActors.Num(),
					InitialInvariantActorLocations.Num()); ++ActorIndex)
			{
				const AActor* Actor = InvariantHierarchyActors[ActorIndex].Get();
				Test->TestTrue(FString::Printf(TEXT("%s retains hierarchy actor %d"),
					*ScopeName, ActorIndex), IsValid(Actor));
				if (IsValid(Actor))
				{
					Test->TestTrue(FString::Printf(TEXT("%s keeps hierarchy actor %d at one world address"),
						*ScopeName, ActorIndex),
						Actor->GetActorLocation().Equals(InitialInvariantActorLocations[ActorIndex], 1.0));
				}
			}

			const auto CheckExteriorSentinel = [&](UHierarchicalInstancedStaticMeshComponent* Hism,
				int32 SentinelIndex, const FVector& InitialLocation, const FVector& InitialScale,
				float InitialEmission, const TCHAR* LayerName)
			{
				if (!Test->TestNotNull(FString::Printf(TEXT("%s retains %s invariant HISM"),
					*ScopeName, LayerName), Hism) || SentinelIndex == INDEX_NONE)
				{
					return;
				}
				FTransform WorldTransform;
				if (!Test->TestTrue(FString::Printf(TEXT("%s retains %s invariant sentinel"),
					*ScopeName, LayerName),
					Hism->GetInstanceTransform(SentinelIndex, WorldTransform, true)))
				{
					return;
				}
				Test->TestTrue(FString::Printf(TEXT("%s keeps %s sentinel world location"),
					*ScopeName, LayerName), WorldTransform.GetLocation().Equals(InitialLocation, 1.0));
				const double ScaleTolerance = FMath::Max(InitialScale.GetAbsMax() * 1.0e-6, 1.0e-9);
				const bool bDistantProxyScope = Focus == EAstroPreviewFocus::Galaxy
					|| Focus == EAstroPreviewFocus::StarCluster
					|| Focus == EAstroPreviewFocus::Overview;
				const int32 EmissionOffset = SentinelIndex * Hism->NumCustomDataFloats + 3;
				const bool bHasEmission = Hism->NumCustomDataFloats > 3
					&& Hism->PerInstanceSMCustomData.IsValidIndex(EmissionOffset);
				const float CurrentEmission = bHasEmission
					? Hism->PerInstanceSMCustomData[EmissionOffset]
					: TNumericLimits<float>::Max();
				Test->TestTrue(FString::Printf(TEXT("%s retains %s sentinel emissive custom data"),
					*ScopeName, LayerName), bHasEmission && FMath::IsFinite(CurrentEmission));
				if (bDistantProxyScope)
				{
					Test->TestTrue(FString::Printf(TEXT("%s keeps distant %s sentinel apparent scale"),
						*ScopeName, LayerName),
						WorldTransform.GetScale3D().Equals(InitialScale, ScaleTolerance));
					Test->TestTrue(FString::Printf(TEXT("%s restores distant %s sentinel emission"),
						*ScopeName, LayerName), bHasEmission && FMath::IsNearlyEqual(
						CurrentEmission, InitialEmission, 1.0e-4f));
				}
				else
				{
					Test->TestTrue(FString::Printf(TEXT("%s only reduces %s sentinel for detail LOD"),
						*ScopeName, LayerName),
						!WorldTransform.GetScale3D().IsNearlyZero()
						&& WorldTransform.GetScale3D().GetAbsMax()
							<= InitialScale.GetAbsMax() + ScaleTolerance);
				}
			};

			AGalaxy* Galaxy = FindGeneratedGalaxy(World, PreviewGenerator.Get());
			AStarCluster* Cluster = FindGeneratedCluster(World, PreviewGenerator.Get());
			CheckExteriorSentinel(Galaxy ? Galaxy->StarMeshInstances : nullptr,
				InitialGalaxySentinelIndex, InitialGalaxySentinelLocation,
				InitialGalaxySentinelScale, InitialGalaxySentinelEmission, TEXT("galaxy"));
			CheckExteriorSentinel(Cluster ? Cluster->StarMeshInstances : nullptr,
				InitialClusterSentinelIndex, InitialClusterSentinelLocation,
				InitialClusterSentinelScale, InitialClusterSentinelEmission, TEXT("cluster"));

			AStarSystem* System = FindGeneratedStarSystem(World, PreviewGenerator.Get());
			if (!IsValid(System)) return;
			const bool bDistantScope = Focus == EAstroPreviewFocus::Galaxy
				|| Focus == EAstroPreviewFocus::StarCluster;
			if (IsValid(Cluster) && IsValid(Cluster->StarMeshInstances)
				&& HomeClusterProxyIndex != INDEX_NONE)
			{
				FTransform HomeProxyTransform;
				if (Cluster->StarMeshInstances->GetInstanceTransform(
					HomeClusterProxyIndex, HomeProxyTransform, true))
				{
					if (bDistantScope)
					{
						const double ScaleTolerance = FMath::Max(
							HomeClusterProxyScale.GetAbsMax() * 1.0e-6, 1.0e-9);
						Test->TestTrue(FString::Printf(TEXT("%s keeps the barycentric home HISM proxy"),
							*ScopeName), HomeProxyTransform.GetScale3D().Equals(
								HomeClusterProxyScale, ScaleTolerance));
					}
					else
					{
						Test->TestTrue(FString::Printf(TEXT("%s culls the duplicate home HISM proxy"),
							*ScopeName), HomeProxyTransform.GetScale3D().IsNearlyZero());
					}
				}
			}

			for (AStar* Star : System->GetStars())
			{
				if (!IsValid(Star) || !IsValid(Star->StarMesh)) continue;
				if (bDistantScope)
				{
					Test->TestFalse(FString::Printf(TEXT("%s does not repurpose a real star as proxy"),
						*ScopeName), Star->StarMesh->IsVisible());
					continue;
				}
				if (!Star->StarMesh->IsVisible()) continue;
				Star->StarMesh->UpdateBounds();
				Test->TestTrue(FString::Printf(TEXT("%s centres rendered star %s on its semantic actor"),
					*ScopeName, *Star->GetName()),
					Star->StarMesh->Bounds.Origin.Equals(Star->GetActorLocation(), 1.0));
			}
		}

		bool UpdateFocusSequence(UWorld* World, UWorldGenerationViewModel* ViewModel,
			AMainMenuController* Controller, double Now)
		{
			if (!World || !ViewModel || !Controller || !PreviewGenerator.IsValid())
			{
				return Fail(TEXT("Preview hierarchy disappeared while switching scopes"));
			}
			static constexpr EAstroPreviewFocus FocusSequence[] = {
				EAstroPreviewFocus::Galaxy,
				EAstroPreviewFocus::StarCluster,
				EAstroPreviewFocus::HomeSystem,
				EAstroPreviewFocus::HomeStar,
				EAstroPreviewFocus::HomePlanet,
				EAstroPreviewFocus::StarCluster,
				EAstroPreviewFocus::Galaxy,
				EAstroPreviewFocus::HomePlanet
			};
			const EAstroPreviewFocus Focus = FocusSequence[FocusIndex];
			if (!bSampling)
			{
				TrackOperationFrame(Now);
				if (Now - StepStartSeconds < ScopeSettleSeconds)
				{
					return false;
				}
				FVector Center;
				double Radius = 0.0;
				if (!PreviewGenerator->GetPreviewFocusSphere(Focus, Center, Radius)
					|| !FMath::IsFinite(Radius) || Radius <= 0.0)
				{
					return Fail(FString::Printf(TEXT("Invalid focus sphere for scope %s"),
						*UEnum::GetValueAsString(Focus)));
				}
				APlayerCameraManager* Camera = Controller->PlayerCameraManager;
				const double CameraDistance = Camera
					? FVector::Distance(Camera->GetCameraLocation(), Center) : 0.0;
				Test->TestTrue(TEXT("Scope camera remains outside its focused body"),
					CameraDistance > Radius * 1.05);
				Test->TestTrue(TEXT("Scope camera does not fly away from its focused body"),
					CameraDistance < Radius * 12.0);
				if (Camera && bHasPreviousScopeViewDirection)
				{
					const FVector CurrentViewDirection = (
						Camera->GetCameraLocation() - Center).GetSafeNormal();
					Test->TestTrue(TEXT("Hierarchy zoom preserves camera viewing direction"),
						FVector::DotProduct(PreviousScopeViewDirection, CurrentViewDirection) > 0.9999);
				}
				FVector2D ProjectedCenter;
				int32 ViewWidth = 0;
				int32 ViewHeight = 0;
				Controller->GetViewportSize(ViewWidth, ViewHeight);
				if (ViewWidth > 0 && ViewHeight > 0
					&& Controller->ProjectWorldLocationToScreen(Center, ProjectedCenter, true))
				{
					Test->TestTrue(TEXT("Settled hierarchy focus projects to viewport centre"),
						FVector2D::Distance(ProjectedCenter,
							FVector2D(ViewWidth * 0.5, ViewHeight * 0.5)) <= 4.0);
				}

				AssertHierarchyPresentationInvariants(World, Focus);

				if (Focus == EAstroPreviewFocus::HomeSystem
					|| Focus == EAstroPreviewFocus::HomeStar
					|| Focus == EAstroPreviewFocus::HomePlanet)
				{
					AssertNativeSystemZonesHidden(World, Focus);
					AssertBackgroundContextOutsideSystem(World, Focus);
				}

				if (Focus == EAstroPreviewFocus::HomeStar)
				{
					if (AStarSystem* System = FindGeneratedStarSystem(
						World, PreviewGenerator.Get()); System && System->GetStars().Num() > 0)
					{
						AStar* FocusedStar = Cast<AStar>(
							PreviewGenerator->GetSelectedPreviewBodyActor());
						FocusedStar = IsValid(FocusedStar) ? FocusedStar : System->MainStar;
						FocusedStar = IsValid(FocusedStar)
							? FocusedStar : System->GetStars()[0];
						if (Test->TestNotNull(TEXT("STAR focus retains its rendered star mesh"),
							FocusedStar ? FocusedStar->StarMesh : nullptr))
						{
							FocusedStar->StarMesh->UpdateBounds();
							Test->TestTrue(TEXT("STAR focus sphere is centred on the rendered mesh bounds"),
								Center.Equals(FocusedStar->StarMesh->Bounds.Origin, 1.0));
							FVector ShellCenter;
							double ShellRadius = 0.0;
							bool bShellVisible = false;
							Test->TestTrue(TEXT("STAR focus owns a real influence shell"),
								PreviewGenerator->GetPreviewGuideShellState(
									EAstroPreviewFocus::HomeStar, ShellCenter, ShellRadius, bShellVisible));
							Test->TestTrue(TEXT("STAR influence shell is visible"), bShellVisible);
							Test->TestTrue(TEXT("STAR influence shell shares rendered mesh centre"),
								ShellCenter.Equals(FocusedStar->StarMesh->Bounds.Origin, 1.0));
							Test->TestTrue(TEXT("STAR influence shell is exactly 1.36 rendered radii"),
								FMath::IsNearlyEqual(ShellRadius,
									FocusedStar->StarMesh->Bounds.SphereRadius * 1.36,
									FMath::Max(2.0, ShellRadius * 1.0e-5)));
							FVector SystemShellCenter;
							double SystemShellRadius = 0.0;
							bool bSystemShellVisible = true;
							if (PreviewGenerator->GetPreviewGuideShellState(
								EAstroPreviewFocus::HomeSystem, SystemShellCenter,
								SystemShellRadius, bSystemShellVisible))
							{
								Test->TestFalse(TEXT("STAR focus hides the outer system shell"),
									bSystemShellVisible);
							}
						}
					}
				}

				if (Focus == EAstroPreviewFocus::HomeSystem)
				{
					if (AStarSystem* System = FindGeneratedStarSystem(World, PreviewGenerator.Get()))
					{
						TArray<FAPSPreviewBodyEntry> SystemEntries;
						PreviewGenerator->GetPreviewBodyEntries(SystemEntries);
						int32 SystemPlanetRows = 0;
						for (const FAPSPreviewBodyEntry& Entry : SystemEntries)
						{
							SystemPlanetRows += Entry.Depth == 1
								&& Entry.Actor.IsValid() && Entry.Actor->IsA<APlanet>() ? 1 : 0;
						}
						Test->TestEqual(TEXT("SYSTEM exposes exactly one label row per generated planet"),
							SystemPlanetRows, 18);
						Test->TestTrue(TEXT("SYSTEM keeps the native safe-zone sphere hidden"),
							IsValid(System->StarSystemZone)
							&& !System->StarSystemZone->IsVisible()
							&& System->StarSystemZone->bHiddenInGame);
						FVector SystemGuideCenter;
						double SystemGuideRadius = 0.0;
						if (PreviewGenerator->GetPreviewFocusSphere(
							EAstroPreviewFocus::HomeSystem, SystemGuideCenter, SystemGuideRadius))
						{
							Test->TestTrue(TEXT("Multi-star SYSTEM guide is centred on its barycentre"),
								SystemGuideCenter.Equals(System->GetActorLocation(), 1.0));
							FVector BoundaryCenter;
							double BoundaryRadius = 0.0;
							bool bBoundaryVisible = false;
							Test->TestTrue(TEXT("SYSTEM owns one real outer boundary shell"),
								PreviewGenerator->GetPreviewGuideShellState(
									EAstroPreviewFocus::HomeSystem, BoundaryCenter,
									BoundaryRadius, bBoundaryVisible));
							Test->TestTrue(TEXT("SYSTEM outer boundary shell is visible"),
								bBoundaryVisible);
							Test->TestTrue(TEXT("SYSTEM shell shares the exact barycentre"),
								BoundaryCenter.Equals(System->GetActorLocation(), 1.0));
							Test->TestTrue(TEXT("SYSTEM shell radius matches camera framing radius"),
								FMath::IsNearlyEqual(BoundaryRadius, SystemGuideRadius,
									FMath::Max(2.0, SystemGuideRadius * 1.0e-5)));
						}
						double SmallestPhysicalRadius = TNumericLimits<double>::Max();
						double LargestPhysicalRadius = 0.0;
						double SmallestPresentedRadius = TNumericLimits<double>::Max();
						double LargestPresentedRadius = 0.0;
						for (AStar* Star : System->GetStars())
						{
							if (!IsValid(Star) || !IsValid(Star->StarMesh)) continue;
							Test->TestTrue(TEXT("SYSTEM keeps every native stellar safe-zone sphere hidden"),
								IsValid(Star->PlanetarySystemZone)
								&& !Star->PlanetarySystemZone->IsVisible()
								&& Star->PlanetarySystemZone->bHiddenInGame);
							Star->StarMesh->UpdateBounds();
							const double PhysicalRadius = FMath::Max(
								static_cast<double>(Star->StarRadiusKM), 1.0);
							const double PresentedRadius = Star->StarMesh->Bounds.SphereRadius;
							SmallestPhysicalRadius = FMath::Min(SmallestPhysicalRadius, PhysicalRadius);
							LargestPhysicalRadius = FMath::Max(LargestPhysicalRadius, PhysicalRadius);
							SmallestPresentedRadius = FMath::Min(SmallestPresentedRadius, PresentedRadius);
							LargestPresentedRadius = FMath::Max(LargestPresentedRadius, PresentedRadius);
							if (IsValid(Star->PlanetarySystem))
							{
								for (const APlanet* OrbitPlanet : Star->PlanetarySystem->PlanetsActorsList)
								{
									if (!IsValid(OrbitPlanet)) continue;
									const double OrbitRadius = FVector::Distance(
										Star->GetActorLocation(), OrbitPlanet->GetActorLocation());
									Test->TestTrue(TEXT("SYSTEM star mesh stays clear of every planet orbit"),
										PresentedRadius <= OrbitRadius * 0.451);
								}
							}
						}
						AStar* GuideStar = Cast<AStar>(
							PreviewGenerator->GetSelectedPreviewBodyActor());
						GuideStar = IsValid(GuideStar) ? GuideStar : System->MainStar;
						GuideStar = IsValid(GuideStar) ? GuideStar
							: (System->GetStars().Num() > 0 ? System->GetStars()[0] : nullptr);
						if (IsValid(GuideStar) && IsValid(GuideStar->StarMesh))
						{
							GuideStar->StarMesh->UpdateBounds();
							FVector StarGuideCenter;
							double StarGuideRadius = 0.0;
							if (PreviewGenerator->GetPreviewFocusSphere(
								EAstroPreviewFocus::HomeStar, StarGuideCenter, StarGuideRadius))
							{
								Test->TestTrue(TEXT("STAR influence guide uses the rendered mesh centre"),
									StarGuideCenter.Equals(
										GuideStar->StarMesh->Bounds.Origin, 1.0));
								FVector InfluenceCenter;
								double InfluenceRadius = 0.0;
								bool bInfluenceVisible = false;
								Test->TestTrue(TEXT("SYSTEM owns one real stellar influence shell"),
									PreviewGenerator->GetPreviewGuideShellState(
										EAstroPreviewFocus::HomeStar, InfluenceCenter,
										InfluenceRadius, bInfluenceVisible));
								Test->TestTrue(TEXT("SYSTEM stellar influence shell is visible"),
									bInfluenceVisible);
								Test->TestTrue(TEXT("SYSTEM stellar shell uses rendered mesh centre"),
									InfluenceCenter.Equals(
										GuideStar->StarMesh->Bounds.Origin, 1.0));
							}
						}
						if (LargestPhysicalRadius > SmallestPhysicalRadius * 1.05)
						{
							Test->TestTrue(TEXT("SYSTEM presentation retains relative multi-star radii"),
								LargestPresentedRadius > SmallestPresentedRadius * 1.01);
						}
					}
				}

				AssertOperationFrameBudget(
					FString::Printf(TEXT("%s_TRANSITION"), *UEnum::GetValueAsString(Focus)));
				if (Focus == EAstroPreviewFocus::HomePlanet
					&& FocusIndex == UE_ARRAY_COUNT(FocusSequence) - 1)
				{
					InitialPlanetCameraDistance = CameraDistance;
					BeginOperationFrameTracking(Now);
					Step = 3;
					StepStartSeconds = Now;
					return false;
				}
				BeginSample(Now);
				bSampling = true;
				return false;
			}

			if (!UpdateSample(Now, ScopeSampleSeconds,
				UEnum::GetValueAsString(Focus)))
			{
				return false;
			}
			++FocusIndex;
			BeginFocus(ViewModel, Now);
			return false;
		}

		bool UpdateInitialSurface(UWorld* World, UWorldGenerationViewModel* ViewModel,
			AMainMenuController* Controller, double Now)
		{
			if (!World || !ViewModel || !Controller || !PreviewGenerator.IsValid())
			{
				return Fail(TEXT("World or generator disappeared while waiting for the orbital surface"));
			}
			if (Now - StepStartSeconds > SurfaceTimeoutSeconds)
			{
				return Fail(TEXT("Initial PLANET closed orbital surface did not become ready"));
			}
			TrackOperationFrame(Now);

			APlanetaryBody* Body = PreviewGenerator->GetActivePreviewWorldScapeBody();
			APlanet* Planet = Cast<APlanet>(Body);
			APlanetarySurfaceGenerator* Surface = World && PreviewGenerator.IsValid()
				? FindPreviewSurfaceGenerator(World, PreviewGenerator.Get()) : nullptr;
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			if (!Planet || !Surface || !Root
				|| !PreviewGenerator->IsPreviewGlobeFamilyWarmQueueDrained()
				|| !HasReadyPlanetOrbitalSurface(Planet, Surface, Root))
			{
				return false;
			}
			Test->TestNotNull(TEXT("PLANET preview uses one persistent profile resolver"), Surface);
			Test->TestNotNull(TEXT("PLANET preview owns one persistent inert WorldScape root"), Root);
			TArray<APlanetaryBody*> ProceduralFamily;
			if (UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Planet->PlanetType))
			{
				ProceduralFamily.Add(Planet);
			}
			for (AMoon* Moon : Planet->Moons)
			{
				if (IsValid(Moon)
					&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Moon->PlanetType))
				{
					ProceduralFamily.Add(Moon);
				}
			}
			// Every supported member of the selected planet/moon family owns a retained
			// closed orbital LOD. One hidden resolver/root pair is reused to build them.
			Test->TestTrue(TEXT("PLANET family warm queue is idle after all orbital proxies commit"),
				PreviewGenerator->IsPreviewGlobeFamilyWarmQueueDrained());
			Test->TestEqual(TEXT("PLANET retains one closed globe per supported family body"),
				PreviewGenerator->GetRetainedPreviewGlobeCount(), ProceduralFamily.Num());

			TSet<UProceduralMeshComponent*> FamilyTerrainProxies;
			TSet<UProceduralMeshComponent*> FamilyOceanProxies;
			TSet<AAtmoScape*> FamilyAtmospheres;
			TSet<FString> FamilyStableKeys;
			for (APlanetaryBody* FamilyBody : ProceduralFamily)
			{
				const FString StableKey = PreviewGenerator->GetPreviewBodyStableKey(FamilyBody);
				Test->TestFalse(TEXT("Each procedural family body owns a nonempty stable key"),
					StableKey.IsEmpty());
				Test->TestFalse(TEXT("Planet and moon stable keys never collide"),
					FamilyStableKeys.Contains(StableKey));
				FamilyStableKeys.Add(StableKey);

				UProceduralMeshComponent* FamilyTerrain =
					PreviewGenerator->GetPreviewTerrainProxyForBody(FamilyBody);
				if (Test->TestNotNull(TEXT("Each supported family body owns a terrain proxy"),
					FamilyTerrain))
				{
					Test->TestFalse(TEXT("No two bodies share/transfer a terrain proxy"),
						FamilyTerrainProxies.Contains(FamilyTerrain));
					FamilyTerrainProxies.Add(FamilyTerrain);
					Test->TestEqual(TEXT("Retained terrain proxy stays owned by the preview generator"),
						FamilyTerrain->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));
					Test->TestTrue(TEXT("PLANET presents each supported family terrain proxy"),
						IsPresented(FamilyTerrain));
					Test->TestNotNull(TEXT("Each family terrain proxy owns a closed mesh section"),
						FamilyTerrain->GetProcMeshSection(0));
					Test->TestEqual(TEXT("Preview terrain proxy has no gameplay collision"),
						FamilyTerrain->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
					Test->TestFalse(TEXT("Preview terrain proxy skips shadow-map passes"),
						FamilyTerrain->CastShadow);
				}

				if (UProceduralMeshComponent* FamilyOcean =
					PreviewGenerator->GetPreviewOceanProxyForBody(FamilyBody))
				{
					Test->TestFalse(TEXT("No two bodies share/transfer an ocean proxy"),
						FamilyOceanProxies.Contains(FamilyOcean));
					FamilyOceanProxies.Add(FamilyOcean);
					Test->TestTrue(TEXT("Terrain and ocean remain independent components"),
						FamilyOcean != FamilyTerrain);
					Test->TestEqual(TEXT("Retained ocean proxy stays owned by the preview generator"),
						FamilyOcean->GetOwner(), static_cast<AActor*>(PreviewGenerator.Get()));
					Test->TestTrue(TEXT("PLANET presents each retained family ocean proxy"),
						IsPresented(FamilyOcean));
					Test->TestNotNull(TEXT("Each family ocean proxy owns a closed mesh section"),
						FamilyOcean->GetProcMeshSection(0));
					Test->TestEqual(TEXT("Preview ocean proxy has no gameplay collision"),
						FamilyOcean->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
					Test->TestFalse(TEXT("Preview ocean proxy skips shadow-map passes"),
						FamilyOcean->CastShadow);
				}

				if (IsValid(FamilyBody->PlanetaryEnvironmentGenerator)
					&& IsValid(FamilyBody->PlanetaryEnvironmentGenerator->PlanetAtmosphere))
				{
					AAtmoScape* FamilyAtmosphere =
						FamilyBody->PlanetaryEnvironmentGenerator->PlanetAtmosphere;
					Test->TestFalse(TEXT("Every body retains its own atmosphere actor"),
						FamilyAtmospheres.Contains(FamilyAtmosphere));
					FamilyAtmospheres.Add(FamilyAtmosphere);
					if (FamilyBody == Planet)
					{
						Test->TestFalse(TEXT("Only the selected planet atmosphere is presented"),
							FamilyAtmosphere->IsHidden());
					}
					else
					{
						Test->TestTrue(TEXT("Non-selected moon atmospheres stay hidden"),
							FamilyAtmosphere->IsHidden());
					}
				}
			}
			AssertOperationFrameBudget(TEXT("PLANET_INITIAL_SURFACE"));
			Test->TestTrue(TEXT("Resolved profile is current for the selected orbital planet"),
				Surface->IsSurfaceProfileCurrent(Planet));
			Test->TestNotNull(TEXT("Resolver created an individual canonical noise object"),
				Surface->ResolvedNoiseInstance);
			Test->TestNotNull(TEXT("Resolver selected a terrain material"),
				Root->TerrainMaterial.DefaultMaterial);
			Test->TestTrue(TEXT("WorldScape NoiseScale is quantized at the plugin boundary"),
				FMath::IsNearlyEqual(Root->NoiseScale,
					static_cast<float>(FMath::RoundToInt(Root->NoiseScale))));
			Test->TestTrue(TEXT("WorldScape NoiseIntensity is quantized at the plugin boundary"),
				FMath::IsNearlyEqual(Root->NoiseIntensity,
					static_cast<float>(FMath::RoundToInt(Root->NoiseIntensity))));
			AssertReadyPlanetOrbitalSurface(Surface, Root, Planet, TEXT("Initial PLANET"));
			CurrentResolverRoot = Root;
			CurrentSurfaceResolver = Surface;
			Test->TestTrue(TEXT("Refocusing the selected PLANET is accepted"),
				ViewModel->FocusPreviewBody(Planet));
			const FPreviewWorldScapeTopology RefocusedTopology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			Test->TestEqual(TEXT("Same-body refocus keeps the resolver root hidden"),
				RefocusedTopology.VisibleRootCount, 0);
			Test->TestEqual(TEXT("Same-body refocus does not create a second generator"),
				RefocusedTopology.Generators.Num(), 1);
			Test->TestEqual(TEXT("Same-body refocus does not create a second root"),
				RefocusedTopology.Roots.Num(), 1);
			Test->TestTrue(TEXT("Same-body refocus retains the persistent resolver root"),
				RefocusedTopology.Roots.Contains(Root));
			AssertSelectedBodyOrbitalLayersPresented(Planet,
				TEXT("Same-body PLANET refocus"));

			Test->TestEqual(TEXT("WorldScape root retains the resolved terrain MID"),
				Root->TerrainMaterial.DefaultMaterial,
				static_cast<UMaterialInterface*>(Surface->ResolvedTerrainMaterialInstance));
			if (Root->bOcean)
			{
				Test->TestEqual(TEXT("WorldScape root retains the resolved ocean MID"),
					Root->OceanMaterial.DefaultMaterial,
					static_cast<UMaterialInterface*>(Surface->ResolvedOceanMaterialInstance));
			}

			FVector SystemCenter;
			double SystemRadius = 0.0;
			const bool bHasSystemSphere = PreviewGenerator->GetPreviewFocusSphere(
				EAstroPreviewFocus::HomeSystem, SystemCenter, SystemRadius);
			const auto CheckStableContext = [&](UHierarchicalInstancedStaticMeshComponent* Hism,
				int32 InitialCount, int32 SentinelIndex, const FVector& InitialLocation,
				const FVector& InitialScale, const TCHAR* LayerName)
			{
				if (!Test->TestNotNull(FString::Printf(TEXT("PLANET retains %s context"), LayerName), Hism))
				{
					return;
				}
				Test->TestTrue(FString::Printf(TEXT("PLANET keeps %s visible"), LayerName),
					Hism->IsVisible() && !Hism->bHiddenInGame);
				Test->TestEqual(FString::Printf(TEXT("%s instance count is scope invariant"), LayerName),
					Hism->GetInstanceCount(), InitialCount);
				FTransform Sentinel;
				if (Hism->GetInstanceTransform(SentinelIndex, Sentinel, true))
				{
					Test->TestTrue(FString::Printf(TEXT("%s sentinel keeps one world address"), LayerName),
						Sentinel.GetLocation().Equals(InitialLocation, 1.0));
					const double ScaleTolerance = FMath::Max(
						InitialScale.GetAbsMax() * 1.0e-6, 1.0e-9);
					Test->TestTrue(FString::Printf(TEXT("%s sentinel uses only a smaller detail LOD scale"),
						LayerName),
						!Sentinel.GetScale3D().IsNearlyZero()
						&& Sentinel.GetScale3D().GetAbsMax()
							<= InitialScale.GetAbsMax() + ScaleTolerance);
				}
				if (!bHasSystemSphere) return;
				const UStaticMesh* ProxyMesh = Hism->GetStaticMesh();
				const double MeshRadius = ProxyMesh
					? FMath::Max(static_cast<double>(ProxyMesh->GetBounds().SphereRadius), 1.0) : 1.0;
				int32 IntrudingVisibleProxyCount = 0;
				int32 OversizedVisibleProxyCount = 0;
				int32 OverbrightVisibleProxyCount = 0;
				for (int32 Index = 0; Index < Hism->GetInstanceCount(); ++Index)
				{
					FTransform WorldTransform;
					if (!Hism->GetInstanceTransform(Index, WorldTransform, true)
						|| WorldTransform.GetScale3D().IsNearlyZero())
					{
						continue;
					}
					const double ProxyRadius = MeshRadius * WorldTransform.GetScale3D().GetAbsMax();
					const double PlanetDistance = FVector::Distance(
						WorldTransform.GetLocation(), Planet->GetActorLocation());
					if (PlanetDistance > UE_SMALL_NUMBER)
					{
						const double AngularRadiusDegrees = FMath::RadiansToDegrees(
							FMath::Atan2(ProxyRadius, PlanetDistance));
						if (AngularRadiusDegrees > 0.051)
						{
							++OversizedVisibleProxyCount;
						}
					}
					const int32 EmissionOffset = Index * Hism->NumCustomDataFloats + 3;
					const bool bHasEmission = Hism->NumCustomDataFloats > 3
						&& Hism->PerInstanceSMCustomData.IsValidIndex(EmissionOffset);
					const float Emission = bHasEmission
						? Hism->PerInstanceSMCustomData[EmissionOffset]
						: TNumericLimits<float>::Max();
					if (!bHasEmission || !FMath::IsFinite(Emission) || Emission > 6.01f)
					{
						++OverbrightVisibleProxyCount;
					}
					if (FVector::Distance(WorldTransform.GetLocation(), SystemCenter) - ProxyRadius
						<= SystemRadius * 1.099)
					{
						++IntrudingVisibleProxyCount;
					}
				}
				Test->TestEqual(
					FString::Printf(TEXT("%s has no visible proxy inside the home-system safe zone"),
						LayerName), IntrudingVisibleProxyCount, 0);
				Test->TestEqual(
					FString::Printf(TEXT("%s has no oversized bright proxy in PLANET"), LayerName),
					OversizedVisibleProxyCount, 0);
				Test->TestEqual(
					FString::Printf(TEXT("%s has no over-emissive background proxy in PLANET"), LayerName),
					OverbrightVisibleProxyCount, 0);
			};
			AGalaxy* Galaxy = FindGeneratedGalaxy(World, PreviewGenerator.Get());
			AStarCluster* Cluster = FindGeneratedCluster(World, PreviewGenerator.Get());
			CheckStableContext(Galaxy ? Galaxy->StarMeshInstances : nullptr,
				InitialGalaxyInstanceCount, InitialGalaxySentinelIndex,
				InitialGalaxySentinelLocation, InitialGalaxySentinelScale, TEXT("galaxy"));
			CheckStableContext(Cluster ? Cluster->StarMeshInstances : nullptr,
				InitialClusterInstanceCount, InitialClusterSentinelIndex,
				InitialClusterSentinelLocation, InitialClusterSentinelScale, TEXT("cluster"));
			if (AStarSystem* System = FindGeneratedStarSystem(World, PreviewGenerator.Get()))
			{
				bool bFoundRecognizableParentStar = false;
				for (AStar* Star : System->GetStars())
				{
					if (!IsValid(Star) || !IsValid(Star->StarMesh))
					{
						continue;
					}
					Test->TestTrue(TEXT("PLANET keeps every system star visible as stationary context"),
						Star->StarMesh->IsVisible() && !Star->StarMesh->bHiddenInGame);
					Star->StarMesh->UpdateBounds();
					const double StarDistance = FVector::Distance(
						Star->GetActorLocation(), Planet->GetActorLocation());
					if (StarDistance > UE_SMALL_NUMBER)
					{
						const double AngularRadiusDegrees = FMath::RadiansToDegrees(FMath::Atan2(
							Star->StarMesh->Bounds.SphereRadius, StarDistance));
						if (Star == Planet->ParentStar)
						{
							bFoundRecognizableParentStar = true;
							Test->TestTrue(TEXT("PLANET presents its parent star as a distinct but subordinate sun"),
								AngularRadiusDegrees >= 0.45 && AngularRadiusDegrees <= 1.25);
						}
						else
						{
							Test->TestTrue(TEXT("PLANET keeps secondary system stars subordinate to the parent"),
								AngularRadiusDegrees <= 2.10);
						}
					}
				}
				Test->TestTrue(TEXT("PLANET exposes a recognizable generated parent star"),
					bFoundRecognizableParentStar);
			}

			TArray<FAPSPreviewBodyEntry> PlanetHierarchy;
			PreviewGenerator->GetPreviewBodyEntries(PlanetHierarchy);
			bool bFoundParentStarRow = false;
			int32 VisibleMoonRows = 0;
			for (const FAPSPreviewBodyEntry& Entry : PlanetHierarchy)
			{
				bFoundParentStarRow = bFoundParentStarRow
					|| (Entry.Actor.Get() == Planet->ParentStar && Entry.Depth == 0);
				if (Entry.Actor.IsValid() && Entry.Actor->IsA<AMoon>())
				{
					++VisibleMoonRows;
					Test->TestEqual(TEXT("PLANET hierarchy nests moons below their planet"),
						Entry.Depth, 2);
					Test->TestFalse(TEXT("PLANET hierarchy never reports a generated moon as 0 KM"),
						Entry.Details.ToString().Contains(TEXT("/ 0 KM")));
				}
			}
			Test->TestTrue(TEXT("PLANET hierarchy exposes its parent star"), bFoundParentStarRow);
			Test->TestEqual(TEXT("PLANET hierarchy exposes every generated moon"),
				VisibleMoonRows, Planet->Moons.Num());
			double PlanetPresentationRadius = 0.0;
			Test->TestTrue(TEXT("PLANET publishes its nominal presentation radius"),
				PreviewGenerator->GetPreviewPresentationRadius(
					Planet, PlanetPresentationRadius));
			FVector PlanetPresentationCenter = Planet->GetActorLocation();
			Test->TestTrue(TEXT("PLANET publishes its nominal presentation centre"),
				PreviewGenerator->GetPreviewPresentationLocation(
					Planet, PlanetPresentationCenter));
			for (const AMoon* Moon : Planet->Moons)
			{
				if (!IsValid(Moon)) continue;
				Test->TestTrue(TEXT("Generated moon keeps a nonzero legacy hierarchy radius"),
					Moon->PlanetRadiusKM > 0);
				UStaticMeshComponent* MoonMesh = Cast<UStaticMeshComponent>(
					Moon->GetComponentByClass(UStaticMeshComponent::StaticClass()));
				if (!Test->TestNotNull(TEXT("PLANET moon retains its authored backing mesh"), MoonMesh))
				{
					continue;
				}
				const bool bProceduralMoon =
					UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Moon->PlanetType);
				UProceduralMeshComponent* MoonTerrain = bProceduralMoon
					? PreviewGenerator->GetPreviewTerrainProxyForBody(Moon) : nullptr;
				UPrimitiveComponent* PresentedMoonSurface = bProceduralMoon
					? static_cast<UPrimitiveComponent*>(MoonTerrain)
					: static_cast<UPrimitiveComponent*>(MoonMesh);
				if (bProceduralMoon)
				{
					Test->TestNotNull(TEXT("Supported moon owns its body-specific closed terrain proxy"),
						MoonTerrain);
					Test->TestTrue(TEXT("Supported moon presents its closed orbital proxy"),
						IsPresented(MoonTerrain));
					Test->TestFalse(TEXT("Supported moon hides its authored backing mesh"),
						IsPresented(MoonMesh));
				}
				else
				{
					Test->TestTrue(TEXT("Non-procedural moon keeps its authored orbital mesh"),
						IsPresented(MoonMesh));
				}
				if (!IsValid(PresentedMoonSurface)) continue;
				PresentedMoonSurface->UpdateBounds();
				const double PresentedMoonRadius = bProceduralMoon
					? PresentedMoonSurface->Bounds.BoxExtent.GetMax()
					: PresentedMoonSurface->Bounds.SphereRadius;
				Test->TestTrue(TEXT("PLANET presents moons at a readable bounded radius"),
					PresentedMoonRadius >= 3.5e4 && PresentedMoonRadius <= 1.81e5);
				FVector PresentationCenter;
				if (Test->TestTrue(TEXT("PLANET publishes each moon's rendered presentation centre"),
					PreviewGenerator->GetPreviewPresentationLocation(Moon, PresentationCenter)))
				{
					const FVector PresentedMoonCenter = bProceduralMoon
						? PresentedMoonSurface->GetComponentLocation()
						: PresentedMoonSurface->Bounds.Origin;
					Test->TestTrue(TEXT("Moon surface is centred on its published presentation orbit"),
						PresentedMoonCenter.Equals(PresentationCenter, 1.0));
					Test->TestTrue(TEXT("PLANET satellite presentation stays outside the globe"),
						FVector::Distance(PlanetPresentationCenter, PresentationCenter)
							> PlanetPresentationRadius + PresentedMoonRadius);
					FVector2D MoonScreen;
					int32 ViewWidth = 0;
					int32 ViewHeight = 0;
					Controller->GetViewportSize(ViewWidth, ViewHeight);
					Test->TestTrue(TEXT("PLANET frames every satellite inside the central preview lane"),
						ViewWidth > 0 && ViewHeight > 0
						&& Controller->ProjectWorldLocationToScreen(PresentationCenter, MoonScreen, true)
						&& MoonScreen.X >= ViewWidth * 0.24 && MoonScreen.X <= ViewWidth * 0.76
						&& MoonScreen.Y >= 0.0 && MoonScreen.Y <= ViewHeight);
				}
			}

			AAtmoScape* Atmosphere = Planet->PlanetaryEnvironmentGenerator
				? Planet->PlanetaryEnvironmentGenerator->PlanetAtmosphere : nullptr;
			UStaticMeshComponent* VisibleSpaceShell = nullptr;
			double AtmospherePresentationRadius = 0.0;
			if (AssertAtmospherePresentation(Planet, Atmosphere, TEXT("Selected PLANET"),
				VisibleSpaceShell, AtmospherePresentationRadius))
			{
				Test->TestFalse(TEXT("Full-scale atmosphere disables relative actor scaling"),
					Atmosphere->bKeepRelativeScale);
				Test->TestEqual(TEXT("Atmosphere uses the generated parent star as its light source"),
					Atmosphere->LightSource, static_cast<AActor*>(Planet->ParentStar));
				Test->TestFalse(TEXT("Atmosphere remains visible over the loading hand-off"),
					Atmosphere->IsHidden());
				Test->TestEqual(TEXT("Orbital atmosphere uses the bounded camera sample budget"),
					Atmosphere->CameraSamplesCount, 8);
				Test->TestEqual(TEXT("Orbital atmosphere uses the bounded light sample budget"),
					Atmosphere->LightSamplesCount, 4);
				if (VisibleSpaceShell)
				{
					VisibleSpaceShell->UpdateBounds();
					UMaterialInstanceDynamic* SpaceMaterial = Cast<UMaterialInstanceDynamic>(
						VisibleSpaceShell->GetMaterial(0));
					Test->TestNotNull(TEXT("Atmosphere space shell uses a dynamic ray-march material"),
						SpaceMaterial);
					if (SpaceMaterial)
					{
						const float MaterialPlanetRadius = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("EarthRadius"));
						const float MaterialAtmosphereRadius = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("AtmosRadius"));
						const float MaterialActorScale = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("ActorScale"));
						const float MaterialRayleighHeight = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("ScaleHeight_R"));
						const float CustomLightSource = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("CustomLightSource"));
						const float LightIntensity = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("LightIntensity"));
						const float AtmosphereOpacity = SpaceMaterial->K2_GetScalarParameterValue(
							TEXT("AtmosOpacity"));
						const FLinearColor RayleighCoefficient =
							SpaceMaterial->K2_GetVectorParameterValue(TEXT("coef_R"));
						const FLinearColor LightPosition =
							SpaceMaterial->K2_GetVectorParameterValue(TEXT("LightPosition"));
						Test->TestTrue(TEXT("Atmosphere shader uses the presented planet radius"),
							FMath::IsNearlyEqual(MaterialPlanetRadius,
								static_cast<float>(AtmospherePresentationRadius),
								static_cast<float>(FMath::Max(
									AtmospherePresentationRadius * 0.01, 1.0))));
						Test->TestTrue(TEXT("Atmosphere shader radius matches the visible shell"),
							FMath::IsNearlyEqual(MaterialAtmosphereRadius,
								static_cast<float>(VisibleSpaceShell->Bounds.BoxExtent.GetMax()),
								static_cast<float>(FMath::Max(
									AtmospherePresentationRadius * 0.015, 1.0))));
						Test->TestTrue(TEXT("Atmosphere camera remains outside the shader planet"),
							MaterialPlanetRadius < InitialPlanetCameraDistance);
						Test->TestTrue(TEXT("Atmosphere shader length scales stay finite and positive"),
							FMath::IsFinite(MaterialActorScale) && MaterialActorScale > 0.0f
							&& FMath::IsFinite(MaterialRayleighHeight) && MaterialRayleighHeight > 0.0f);
						Test->TestTrue(TEXT("Atmosphere preview uses the LWC-safe directional-light path"),
							CustomLightSource < 0.5f);
						Test->TestTrue(TEXT("Atmosphere preview has explicit nonzero light intensity"),
							LightIntensity >= 0.5f);
						Test->TestTrue(TEXT("Atmosphere opacity reaches the ray-march material"),
							AtmosphereOpacity > 0.5f && AtmosphereOpacity < 3.0f);
						Test->TestTrue(TEXT("Atmosphere Rayleigh coefficient remains nonzero"),
							RayleighCoefficient.R > 0.0f || RayleighCoefficient.G > 0.0f
							|| RayleighCoefficient.B > 0.0f);
						Test->TestTrue(TEXT("Atmosphere receives a finite generated-star position"),
							FMath::IsFinite(LightPosition.R) && FMath::IsFinite(LightPosition.G)
							&& FMath::IsFinite(LightPosition.B));
						UE_LOG(LogTemp, Display,
							TEXT("[APS.Smoke] Atmosphere material earth=%.0f atmosphere=%.0f actorScale=%.2f rayleighHeight=%.2f customLight=%.0f lightIntensity=%.2f opacity=%.2f coefR=(%.6f,%.6f,%.6f)"),
							MaterialPlanetRadius, MaterialAtmosphereRadius,
							MaterialActorScale, MaterialRayleighHeight, CustomLightSource,
							LightIntensity, AtmosphereOpacity, RayleighCoefficient.R,
							RayleighCoefficient.G, RayleighCoefficient.B);
					}
				}
			}

			AMoon* SelectionProbeMoon = nullptr;
			for (AMoon* Moon : Planet->Moons)
			{
				if (IsValid(Moon)
					&& UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Moon->PlanetType))
				{
					SelectionProbeMoon = Moon;
					break;
				}
			}
			if (SelectionProbeMoon)
			{
				const double MoonFeatureBeforeSelection = SelectionProbeMoon->SurfaceFeatureScale;
				const int32 MoonSeedBeforeSelection = SelectionProbeMoon->WorldScapeSeed;
				const double PendingPlanetFeature = FMath::Clamp(
					Planet->SurfaceFeatureScale > 2.0
						? Planet->SurfaceFeatureScale - 0.25
						: Planet->SurfaceFeatureScale + 0.25,
					0.25, 4.0);
				ViewModel->SetSurfaceFeatureScale(PendingPlanetFeature);
				Test->TestTrue(TEXT("Hierarchy selection accepts a generated moon without STAR refresh"),
					ViewModel->FocusPreviewBody(SelectionProbeMoon));
				Test->TestEqual(TEXT("Moon selection changes only the active editor/focus body"),
					PreviewGenerator->GetActivePreviewWorldScapeBody(),
					static_cast<APlanetaryBody*>(SelectionProbeMoon));
				Test->TestEqual(TEXT("Pending planet debounce never mutates the newly selected moon"),
					SelectionProbeMoon->SurfaceFeatureScale, MoonFeatureBeforeSelection);
				Test->TestEqual(TEXT("Moon hydration publishes the moon's own seed"),
					ViewModel->GeneratedWorld->PlanetSurfaceSeed, MoonSeedBeforeSelection);
				Test->TestEqual(TEXT("Moon hydration publishes the moon's own feature scale"),
					ViewModel->GeneratedWorld->SurfaceFeatureScale, MoonFeatureBeforeSelection);
				AssertSelectedBodyOrbitalLayersPresented(
					SelectionProbeMoon, TEXT("Selected moon hand-off"));

				Test->TestTrue(TEXT("Hierarchy can return directly from moon to planet"),
					ViewModel->FocusPreviewBody(Planet));
				AssertSelectedBodyOrbitalLayersPresented(
					Planet, TEXT("Returned planet hand-off"));
				Test->TestEqual(TEXT("Planet hydration restores its pending stable-key edit"),
					ViewModel->GeneratedWorld->SurfaceFeatureScale, PendingPlanetFeature);
				const FAPSPreviewBodyEditOverride* PlanetOverride =
					ViewModel->GeneratedWorld->FindPreviewBodyEditOverride(
						PreviewGenerator->GetPreviewBodyStableKey(Planet));
				const FAPSPreviewBodyEditOverride* MoonOverride =
					ViewModel->GeneratedWorld->FindPreviewBodyEditOverride(
						PreviewGenerator->GetPreviewBodyStableKey(SelectionProbeMoon));
				Test->TestTrue(TEXT("Planet pending edit remains isolated under its stable key"),
					PlanetOverride && FMath::IsNearlyEqual(
						PlanetOverride->SurfaceFeatureScale, PendingPlanetFeature));
				Test->TestTrue(TEXT("Moon editor snapshot remains isolated under its stable key"),
					MoonOverride && MoonOverride->SurfaceSeed == MoonSeedBeforeSelection);
			}

			InitialPlanet = Planet;
			SurfaceRevisionBeforeChange = ViewModel->PreviewRevision;
			SurfaceSubtypeProbeIndex = 0;
			bObservedReadyWithoutCommittedOrbitalProxy = false;
			ViewModel->GeneratedWorld->PlanetSurfaceSeed = 84084;
			const FPreviewWorldScapeTopology PreIceTopology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			CurrentResolverRoot = PreIceTopology.Roots.Num() == 1
				? PreIceTopology.Roots[0] : nullptr;
			CurrentSurfaceResolver = PreIceTopology.Generators.Num() == 1
				? PreIceTopology.Generators[0] : nullptr;
			PreviousSurfaceProfileSignature = CurrentSurfaceResolver.IsValid()
				? CurrentSurfaceResolver->AppliedSurfaceProfileSignature : 0;
			BeginPlanetOrbitalSwapObservation(World, Planet, TEXT("ICE"));
			BeginOperationFrameTracking(Now);
			ViewModel->SetEnumValue(StaticEnum<EPlanetType>(),
				static_cast<int32>(EPlanetType::Ice));
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Initial closed orbital surface ready in %.2fs; probing Ice -> Lava -> High Mountain in place"),
				Now - StepStartSeconds);
			Step = 4;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateChangedSurface(UWorld* World, UWorldGenerationViewModel* ViewModel,
			AMainMenuController* Controller, double Now)
		{
			if (!World || !ViewModel || !Controller || !PreviewGenerator.IsValid())
			{
				return Fail(TEXT("Preview disappeared during the in-place subtype change"));
			}
			if (Now - StepStartSeconds > SurfaceTimeoutSeconds)
			{
				const FPreviewWorldScapeTopology TimedOutTopology =
					CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
				APlanetarySurfaceGenerator* TimedOutSurface = FindPreviewSurfaceGenerator(
					World, PreviewGenerator.Get());
				AWorldScapeRoot* TimedOutRoot = TimedOutSurface
					? TimedOutSurface->WorldScapeRootInstance : nullptr;
				APlanetaryBody* TimedOutBody =
					PreviewGenerator->GetActivePreviewWorldScapeBody();
				UProceduralMeshComponent* TimedOutTerrain = IsValid(TimedOutBody)
					? PreviewGenerator->GetPreviewTerrainProxyForBody(TimedOutBody) : nullptr;
				return Fail(FString::Printf(
					TEXT("Changed PLANET orbital surface did not become ready body=%s terrain=%s visible=%s section=%s ready=%s globeSignature=%u profileSignature=%u profileCurrent=%s root=%s hidden=%s tick=%s generate=%s workers=%d generators=%d roots=%d visibleRoots=%d orphanGenerator=%s orphanRoot=%s duplicateLink=%s"),
					*GetNameSafe(TimedOutBody), *GetNameSafe(TimedOutTerrain),
					IsPresented(TimedOutTerrain) ? TEXT("true") : TEXT("false"),
					TimedOutTerrain && TimedOutTerrain->GetProcMeshSection(0)
						? TEXT("true") : TEXT("false"),
					TimedOutBody && TimedOutBody->bWorldScapeSurfaceReady
						? TEXT("true") : TEXT("false"),
					PreviewGenerator->GetPreviewGlobeProfileSignature(),
					TimedOutSurface ? TimedOutSurface->AppliedSurfaceProfileSignature : 0,
					TimedOutSurface && TimedOutSurface->IsSurfaceProfileCurrent(TimedOutBody)
						? TEXT("true") : TEXT("false"),
					*GetNameSafe(TimedOutRoot),
					TimedOutRoot && TimedOutRoot->IsHidden() ? TEXT("true") : TEXT("false"),
					TimedOutRoot && TimedOutRoot->IsActorTickEnabled() ? TEXT("true") : TEXT("false"),
					TimedOutRoot && TimedOutRoot->bGenerateWorldScape ? TEXT("true") : TEXT("false"),
					TimedOutRoot ? TimedOutRoot->WorldScapeLodInGeneration.Num() : 0,
					TimedOutTopology.Generators.Num(), TimedOutTopology.Roots.Num(),
					TimedOutTopology.VisibleRootCount,
					TimedOutTopology.bHasGeneratorWithoutRoot ? TEXT("true") : TEXT("false"),
					TimedOutTopology.bHasRootWithoutGenerator ? TEXT("true") : TEXT("false"),
					TimedOutTopology.bHasDuplicateRootLink ? TEXT("true") : TEXT("false")));
			}
			TrackOperationFrame(Now);
			APlanet* Planet = Cast<APlanet>(PreviewGenerator->GetActivePreviewWorldScapeBody());
			if (!IsValid(Planet))
			{
				return false;
			}
			if (Planet != InitialPlanet.Get())
			{
				return Fail(TEXT("Planet subtype change rebuilt the hierarchy instead of refreshing the same actor"));
			}
			APlanetarySurfaceGenerator* Surface = World && PreviewGenerator.IsValid()
				? FindPreviewSurfaceGenerator(World, PreviewGenerator.Get()) : nullptr;
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			ObservePlanetOrbitalSwap(World, Planet);
			const bool bCommittedBackingVisible = HasPresentedStaticMesh(Planet);
			if (Planet->bWorldScapeSurfaceReady)
			{
				bObservedReadyWithoutCommittedOrbitalProxy =
					bObservedReadyWithoutCommittedOrbitalProxy
						|| !HasReadyPlanetOrbitalSurface(Planet, Surface, Root);
			}

			if (!bSurfaceSliderBurstStarted && SurfaceSubtypeProbeIndex >= 0
				&& SurfaceSubtypeProbeIndex < 2)
			{
				const EPlanetType ExpectedType = SurfaceSubtypeProbeIndex == 0
					? EPlanetType::Ice : EPlanetType::Lava;
				if (ViewModel->PreviewRevision <= SurfaceRevisionBeforeChange
					|| Planet->PlanetType != ExpectedType
					|| !HasReadyPlanetOrbitalSurface(Planet, Surface, Root))
				{
					return false;
				}
				const FString ProbeName = ExpectedType == EPlanetType::Ice
					? TEXT("ICE") : TEXT("LAVA");
				if (!WaitForCompilationIdle(Now,
					ExpectedType == EPlanetType::Ice ? TEXT("PLANET_ICE_ORBITAL") : TEXT("PLANET_LAVA_ORBITAL")))
				{
					BeginOperationFrameTracking(Now);
					return false;
				}

				Test->TestFalse(FString::Printf(
					TEXT("%s reports ready only after its closed orbital proxy commits"), *ProbeName),
					bObservedReadyWithoutCommittedOrbitalProxy);
				AssertReadyPlanetOrbitalSurface(Surface, Root, Planet, ProbeName);
				AssertPlanetOrbitalSwapCommitted(World, Surface, Root, Planet, ProbeName);
				Test->TestEqual(FString::Printf(
					TEXT("%s WorldScape root retains its exact resolver terrain MID"), *ProbeName),
					Root->TerrainMaterial.DefaultMaterial,
					static_cast<UMaterialInterface*>(Surface->ResolvedTerrainMaterialInstance));
				Test->TestTrue(FString::Printf(
					TEXT("%s changes the previous resolved profile signature"), *ProbeName),
					Surface->AppliedSurfaceProfileSignature
						!= PreviousSurfaceProfileSignature);
				Test->TestFalse(FString::Printf(
					TEXT("%s never exposes the authored backing sphere after commit"), *ProbeName),
					bCommittedBackingVisible);

				if (Root->bOcean)
				{
					Test->TestEqual(FString::Printf(
						TEXT("%s WorldScape root retains its exact resolver liquid MID"), *ProbeName),
						Root->OceanMaterial.DefaultMaterial,
						static_cast<UMaterialInterface*>(Surface->ResolvedOceanMaterialInstance));
				}
				AssertOperationFrameBudget(FString::Printf(
					TEXT("PLANET_%s_ORBITAL_PROXY"), *ProbeName));
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Smoke] %s closed orbital proxy committed signature=%u terrain=%s ocean=%s"),
					*ProbeName, Surface->AppliedSurfaceProfileSignature,
					*GetNameSafe(Root->TerrainMaterial.DefaultMaterial),
					Root->bOcean ? *GetNameSafe(Root->OceanMaterial.DefaultMaterial) : TEXT("none"));

				PreviousSurfaceProfileSignature = Surface->AppliedSurfaceProfileSignature;
				++SurfaceSubtypeProbeIndex;
				SurfaceRevisionBeforeChange = ViewModel->PreviewRevision;
				bObservedReadyWithoutCommittedOrbitalProxy = false;
				const FString NextProbeName = SurfaceSubtypeProbeIndex == 1
					? TEXT("LAVA") : TEXT("High Mountain");
				BeginPlanetOrbitalSwapObservation(World, Planet, NextProbeName);
				BeginOperationFrameTracking(Now);
				ViewModel->SetEnumValue(StaticEnum<EPlanetType>(),
					static_cast<int32>(SurfaceSubtypeProbeIndex == 1
						? EPlanetType::Lava : EPlanetType::HighMountain));
				StepStartSeconds = Now;
				return false;
			}
			if (bSurfaceSliderBurstStarted)
			{
				if (ViewModel->PreviewRevision <= SurfaceRevisionBeforeSliderBurst
					|| !HasReadyPlanetOrbitalSurface(Planet, Surface, Root)
					|| PreviewGenerator->GetPreviewSurfaceProfileApplyCount()
						<= SurfaceProfileApplyCountBeforeSliderBurst)
				{
					return false;
				}
				if (!WaitForCompilationIdle(Now, TEXT("PLANET_SURFACE_SLIDERS")))
				{
					BeginOperationFrameTracking(Now);
					return false;
				}

				Test->TestFalse(TEXT("Surface slider refresh reports ready only after its closed orbital proxy commits"),
					bObservedReadyWithoutCommittedOrbitalProxy);
				AssertReadyPlanetOrbitalSurface(Surface, Root, Planet,
					TEXT("Surface slider refresh"));
				AssertPlanetOrbitalSwapCommitted(World, Surface, Root, Planet,
					TEXT("Surface slider refresh"));
				const int32 SliderApplyCount =
					PreviewGenerator->GetPreviewSurfaceProfileApplyCount();
				Test->TestTrue(TEXT("Rapid surface slider events coalesce to a bounded orbital-proxy update"),
					SliderApplyCount > SurfaceProfileApplyCountBeforeSliderBurst
						&& SliderApplyCount <= SurfaceProfileApplyCountBeforeSliderBurst + 2);
				Test->TestTrue(TEXT("Surface slider burst changes the applied resolver signature"),
					Surface->AppliedSurfaceProfileSignature != SurfaceSignatureBeforeSliderBurst);
				Test->TestTrue(TEXT("Surface slider burst replaces the immutable per-body noise instance"),
					Surface->ResolvedNoiseInstance != NoiseBeforeSliderBurst.Get());
				Test->TestEqual(TEXT("Surface seed reaches the selected actor"),
					Planet->WorldScapeSeed, SliderSurfaceSeed);
				Test->TestTrue(TEXT("Feature scale reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceFeatureScale, SliderFeatureScale));
				Test->TestTrue(TEXT("Relief scale reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceReliefScale, SliderReliefScale));
				Test->TestTrue(TEXT("Land coverage reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceLandCoverageScale, SliderLandCoverageScale));
				Test->TestTrue(TEXT("Mountain scale reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceMountainScale, SliderMountainScale));
				Test->TestTrue(TEXT("Crater scale reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceCraterScale, SliderCraterScale));
				Test->TestTrue(TEXT("Roughness scale reaches the selected actor"),
					FMath::IsNearlyEqual(Planet->SurfaceRoughnessScale, SliderRoughnessScale));
				if (Planet->PlanetData.PlanetModel.IsValid())
				{
					const FPlanetModel& Model = *Planet->PlanetData.PlanetModel;
					Test->TestEqual(TEXT("Surface seed reaches the authoritative planet model"),
						Model.SurfaceSeed, SliderSurfaceSeed);
					Test->TestTrue(TEXT("All six surface multipliers reach the authoritative planet model"),
						FMath::IsNearlyEqual(Model.SurfaceFeatureScale, SliderFeatureScale)
						&& FMath::IsNearlyEqual(Model.SurfaceReliefScale, SliderReliefScale)
						&& FMath::IsNearlyEqual(Model.SurfaceLandCoverageScale, SliderLandCoverageScale)
						&& FMath::IsNearlyEqual(Model.SurfaceMountainScale, SliderMountainScale)
						&& FMath::IsNearlyEqual(Model.SurfaceCraterScale, SliderCraterScale)
						&& FMath::IsNearlyEqual(Model.SurfaceRoughnessScale, SliderRoughnessScale));
				}
				else
				{
					Test->AddError(TEXT("Selected planet lost its authoritative model during slider regeneration"));
				}
				Test->TestTrue(TEXT("Surface slider regeneration keeps the exact selected actor"),
					Planet == InitialPlanet.Get());
				const FVector CameraAfterSliders = Controller->PlayerCameraManager
					? Controller->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
				Test->TestTrue(TEXT("Surface slider regeneration preserves the PLANET camera"),
					CameraAfterSliders.Equals(SliderCameraLocationBefore, 1.0));
				AAtmoScape* SliderAtmosphere = Planet->PlanetaryEnvironmentGenerator
					? Planet->PlanetaryEnvironmentGenerator->PlanetAtmosphere : nullptr;
				UStaticMeshComponent* SliderAtmosphereShell = nullptr;
				double SliderPresentationRadius = 0.0;
				AssertAtmospherePresentation(Planet, SliderAtmosphere,
					TEXT("Surface slider PLANET"), SliderAtmosphereShell,
					SliderPresentationRadius);
				AssertOperationFrameBudget(TEXT("PLANET_SURFACE_SLIDER_BURST"));

				OrbitCameraLocationBefore = CameraAfterSliders;
				FVector PlanetCenter = FVector::ZeroVector;
				double PlanetRadius = 0.0;
				Test->TestTrue(TEXT("PLANET RMB retains a valid focus sphere"),
					PreviewGenerator->GetPreviewFocusSphere(
						EAstroPreviewFocus::HomePlanet, PlanetCenter, PlanetRadius));
				OrbitCameraDistanceBefore = FVector::Distance(CameraAfterSliders, PlanetCenter);
				OrbitResolverRootBefore = Root;
				OrbitResolverRotationBefore = Root->GetActorQuat();
				OrbitResolverLocationBefore = Root->GetActorLocation();
				OrbitOverridePositionBefore = Root->OverridedPlayerPosition;
				OrbitTerrainProxyBefore =
					PreviewGenerator->GetPreviewTerrainProxyForBody(Planet);
				if (Test->TestNotNull(TEXT("PLANET RMB starts from a committed orbital terrain proxy"),
					OrbitTerrainProxyBefore.Get()))
				{
					OrbitTerrainRotationBefore = OrbitTerrainProxyBefore->GetComponentQuat();
					OrbitTerrainLocationBefore = OrbitTerrainProxyBefore->GetComponentLocation();
				}
				OrbitFamilyBody.Reset();
				if (Planet->Moons.Num() > 0 && IsValid(Planet->Moons[0]))
				{
					OrbitFamilyBody = Planet->Moons[0];
					PreviewGenerator->GetPreviewPresentationLocation(
						Planet->Moons[0], OrbitFamilyBodyLocationBefore);
					OrbitFamilyBodyDistanceBefore = FVector::Distance(
						OrbitFamilyBodyLocationBefore, PlanetCenter);
				}
				BeginOperationFrameTracking(Now);
				ViewModel->BeginPreviewOrbit();
				// Exercise the defensive update path as well as the normal sleeping path.
				// An unrelated tick while RMB is held must not wake the inert resolver or
				// rebuild the committed closed globe.
				PreviewGenerator->SetActorTickEnabled(true);
				// Exercise a complete horizontal revolution before ending at a distinct
				// orientation. No intermediate delta may move the fixed camera or rebuild the proxy.
				for (int32 QuarterTurn = 0; QuarterTurn < 4; ++QuarterTurn)
				{
					ViewModel->OrbitPreview(FVector2D(500.0, 0.0));
				}
				ViewModel->OrbitPreview(FVector2D(420.0, 120.0));
				bOrbitReleasePending = true;
				OrbitSurfaceProfileSignatureBefore = Surface->AppliedSurfaceProfileSignature;
				OrbitSurfaceProfileApplyCountBefore =
					PreviewGenerator->GetPreviewSurfaceProfileApplyCount();
				bObservedInteractionRootVisible = !Root->IsHidden();
				bObservedOrbitBackingVisible = HasPresentedStaticMesh(Planet);
				bObservedInteractionSelectedProxyHidden =
					!IsPresented(PreviewGenerator->GetPreviewTerrainProxyForBody(Planet));
				Test->TestTrue(TEXT("PLANET RMB keeps the resolver root hidden"),
					Root->IsHidden());
				Test->TestFalse(TEXT("PLANET RMB does not expose the authored backing sphere"),
					bObservedOrbitBackingVisible);
				Test->TestTrue(TEXT("PLANET RMB keeps the selected orbital proxy visible"),
					!bObservedInteractionSelectedProxyHidden);
				Test->TestFalse(TEXT("PLANET RMB keeps the inert resolver tick disabled"),
					Root->IsActorTickEnabled());
				Test->TestEqual(TEXT("PLANET RMB launches no WorldScape workers"),
					Root->WorldScapeLodInGeneration.Num(), 0);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Smoke] Slider burst committed to closed orbital proxy; holding RMB across a latent frame"));
				Step = 5;
				StepStartSeconds = Now;
				return false;
			}

			if (ViewModel->PreviewRevision <= SurfaceRevisionBeforeChange
				|| Planet->PlanetType != EPlanetType::HighMountain
				|| !HasReadyPlanetOrbitalSurface(Planet, Surface, Root))
			{
				return false;
			}
			if (!WaitForCompilationIdle(Now, TEXT("PLANET_SUBTYPE")))
			{
				// Asset/shader preparation is reported separately from the already-complete
				// interaction; do not misclassify editor compilation as a runtime camera hitch.
				BeginOperationFrameTracking(Now);
				return false;
			}

			Test->TestFalse(TEXT("High Mountain reports ready only after its closed orbital proxy commits"),
				bObservedReadyWithoutCommittedOrbitalProxy);
			AssertReadyPlanetOrbitalSurface(Surface, Root, Planet, TEXT("High Mountain"));
			AssertPlanetOrbitalSwapCommitted(World, Surface, Root, Planet,
				TEXT("High Mountain"));
			AssertOperationFrameBudget(TEXT("PLANET_SUBTYPE_REFRESH"));
			Test->TestEqual(TEXT("Resolved profile follows the changed EPlanetType"),
				Surface->ResolvedSurfaceProfile.PlanetType, EPlanetType::HighMountain);
			const FString ScopeSummary = ViewModel->GetPreviewScopeSummary().ToString();
			Test->TestTrue(TEXT("Planet summary exposes the selected High Mountain subtype"),
				ScopeSummary.Contains(TEXT("HIGH MOUNTAIN")));
			Test->TestFalse(TEXT("Planet summary does not alias High Mountain to Metallic"),
				ScopeSummary.Contains(TEXT("METALLIC")));
			AAtmoScape* RefreshedAtmosphere = Planet->PlanetaryEnvironmentGenerator
				? Planet->PlanetaryEnvironmentGenerator->PlanetAtmosphere : nullptr;
			UStaticMeshComponent* RefreshedAtmosphereShell = nullptr;
			double RefreshedPresentationRadius = 0.0;
			AssertAtmospherePresentation(Planet, RefreshedAtmosphere,
				TEXT("High Mountain PLANET"), RefreshedAtmosphereShell,
				RefreshedPresentationRadius);
			UStaticMeshComponent* BackingSphere = Cast<UStaticMeshComponent>(
				Planet->GetComponentByClass(UStaticMeshComponent::StaticClass()));
			if (BackingSphere)
			{
				BackingSphere->UpdateBounds();
				Test->TestTrue(TEXT("Ready closed orbital proxy hides its authored backing sphere"),
					!BackingSphere->IsVisible() && BackingSphere->bHiddenInGame);
				Test->TestTrue(TEXT("Authored backing sphere retains the normalized preview radius"),
					FMath::IsNearlyEqual(BackingSphere->Bounds.SphereRadius,
						RefreshedPresentationRadius,
						FMath::Max(RefreshedPresentationRadius * 0.01, 1.0)));
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Smoke] Hidden authored backing radius=%.0f material=%s"),
					BackingSphere->Bounds.SphereRadius,
					*GetNameSafe(BackingSphere->GetMaterial(0)));
			}
			FVector PlanetCenter;
			double PlanetRadius = 0.0;
			PreviewGenerator->GetPreviewFocusSphere(
				EAstroPreviewFocus::HomePlanet, PlanetCenter, PlanetRadius);
			const double CameraDistance = Controller->PlayerCameraManager
				? FVector::Distance(Controller->PlayerCameraManager->GetCameraLocation(), PlanetCenter) : 0.0;
			const double RelativeCameraError = FMath::Abs(CameraDistance - InitialPlanetCameraDistance)
				/ FMath::Max(InitialPlanetCameraDistance, 1.0);
			Test->TestTrue(TEXT("Planet subtype refresh preserves proportional camera distance"),
				RelativeCameraError <= 0.03);
			Test->TestFalse(TEXT("Ready preview generator sleeps outside camera transitions"),
				PreviewGenerator->IsActorTickEnabled());

			SliderCameraLocationBefore = Controller->PlayerCameraManager
				? Controller->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
			SurfaceRevisionBeforeSliderBurst = ViewModel->PreviewRevision;
			SurfaceProfileApplyCountBeforeSliderBurst =
				PreviewGenerator->GetPreviewSurfaceProfileApplyCount();
			SurfaceSignatureBeforeSliderBurst = Surface->AppliedSurfaceProfileSignature;
			NoiseBeforeSliderBurst = Surface->ResolvedNoiseInstance;
			bSurfaceSliderBurstStarted = true;
			bObservedReadyWithoutCommittedOrbitalProxy = false;
			BeginPlanetOrbitalSwapObservation(World, Planet,
				TEXT("Surface slider refresh"));
			BeginOperationFrameTracking(Now);
			const auto CommitSlateSurfaceControl = [this, Controller](
				const EAPSGenerationSurfaceControl Control, const double Value,
				const TCHAR* Label)
			{
				Test->TestTrue(FString::Printf(TEXT("%s real Slate delegate commits"), Label),
					Controller->CommitSurfaceControlForAutomation(Control, Value));
				Test->TestTrue(FString::Printf(TEXT("%s bound Slate Value updates"), Label),
					FMath::IsNearlyEqual(
						Controller->GetSurfaceControlValueForAutomation(Control), Value, 1.0e-4));
			};
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::Seed,
				SliderSurfaceSeed, TEXT("Surface seed"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::FeatureScale,
				SliderFeatureScale, TEXT("Feature scale"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::ReliefScale,
				SliderReliefScale, TEXT("Relief"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::LandCoverage,
				SliderLandCoverageScale, TEXT("Land coverage"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::Mountains,
				SliderMountainScale, TEXT("Mountains"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::Craters,
				SliderCraterScale, TEXT("Craters"));
			CommitSlateSurfaceControl(EAPSGenerationSurfaceControl::Roughness,
				SliderRoughnessScale, TEXT("Roughness"));
			Test->TestEqual(TEXT("Slate seed delegate updates the editor buffer immediately"),
				ViewModel->GeneratedWorld->PlanetSurfaceSeed, SliderSurfaceSeed);
			Test->TestTrue(TEXT("All six Slate delegates update the editor buffer immediately"),
				FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceFeatureScale, SliderFeatureScale)
				&& FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceReliefScale, SliderReliefScale)
				&& FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceLandCoverageScale, SliderLandCoverageScale)
				&& FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceMountainScale, SliderMountainScale)
				&& FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceCraterScale, SliderCraterScale)
				&& FMath::IsNearlyEqual(ViewModel->GeneratedWorld->SurfaceRoughnessScale, SliderRoughnessScale));
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Changed orbital proxy ready in %.2fs cameraError=%.4f; coalescing all real Slate surface controls"),
				Now - StepStartSeconds, RelativeCameraError);
			return false;
		}

		bool UpdateOrbitedSurface(UWorld* World, UWorldGenerationViewModel* ViewModel,
			AMainMenuController* Controller, double Now)
		{
			if (!World || !ViewModel || !Controller || !PreviewGenerator.IsValid())
			{
				return Fail(TEXT("Preview disappeared during PLANET RMB/zoom validation"));
			}
			if (Now - StepStartSeconds > SurfaceTimeoutSeconds)
			{
				return Fail(TEXT("PLANET orbital proxy did not settle after RMB/zoom"));
			}
			TrackOperationFrame(Now);

			APlanet* Planet = Cast<APlanet>(PreviewGenerator->GetActivePreviewWorldScapeBody());
			APlanetarySurfaceGenerator* Surface = FindPreviewSurfaceGenerator(
				World, PreviewGenerator.Get());
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			if (!Planet || !Surface || !Root)
			{
				return false;
			}
			UProceduralMeshComponent* TerrainProxy =
				PreviewGenerator->GetPreviewTerrainProxyForBody(Planet);
			const bool bRootVisible = !Root->IsHidden();
			const bool bBackingVisible = HasPresentedStaticMesh(Planet);
			const bool bTerrainVisible = IsPresented(TerrainProxy);
			bObservedInteractionRootVisible =
				bObservedInteractionRootVisible || bRootVisible;
			bObservedOrbitBackingVisible = bObservedOrbitBackingVisible || bBackingVisible;
			bObservedInteractionSelectedProxyHidden =
				bObservedInteractionSelectedProxyHidden || !bTerrainVisible;
			bObservedInteractionSurfaceNotReady = bObservedInteractionSurfaceNotReady
				|| !Planet->bWorldScapeSurfaceReady;
			bObservedInteractionWorkersInFlight = bObservedInteractionWorkersInFlight
				|| Root->WorldScapeLodInGeneration.Num() > 0
				|| Root->bGenerateWorldScape || Root->IsActorTickEnabled();
			if (OrbitResolverRootBefore.IsValid()
				&& Root != OrbitResolverRootBefore.Get())
			{
				return Fail(TEXT("PLANET RMB/zoom replaced the persistent resolver root"));
			}
			if (OrbitTerrainProxyBefore.IsValid()
				&& TerrainProxy != OrbitTerrainProxyBefore.Get())
			{
				return Fail(TEXT("PLANET RMB/zoom rebuilt or swapped the orbital terrain proxy"));
			}
			if (bOrbitReleasePending)
			{
				// The interaction remains active for a complete latent frame. This catches
				// the reported regression at the exact time RMB is physically held, rather
				// than testing only the final camera result.
				Test->TestFalse(TEXT("Held RMB never exposes the inert resolver root"),
					bRootVisible);
				Test->TestFalse(TEXT("Held RMB does not expose the authored backing sphere"),
					bBackingVisible);
				Test->TestTrue(TEXT("Held RMB keeps the selected closed terrain visible"),
					bTerrainVisible);
				Test->TestEqual(TEXT("Held RMB retains the same orbital front buffer"),
					TerrainProxy, OrbitTerrainProxyBefore.Get());
				Test->TestEqual(TEXT("Held RMB does not rebuild the surface profile"),
					Surface->AppliedSurfaceProfileSignature,
					OrbitSurfaceProfileSignatureBefore);
				Test->TestEqual(TEXT("Held RMB does not reapply the surface profile"),
					PreviewGenerator->GetPreviewSurfaceProfileApplyCount(),
					OrbitSurfaceProfileApplyCountBefore);
				Test->TestEqual(TEXT("Held RMB launches no WorldScape worker batch"),
					Root->WorldScapeLodInGeneration.Num(), 0);
				Test->TestFalse(TEXT("Held RMB leaves the resolver producer disabled"),
					Root->bGenerateWorldScape);
				Test->TestFalse(TEXT("Held RMB leaves the resolver tick disabled"),
					Root->IsActorTickEnabled());
				Test->TestEqual(TEXT("Held RMB preserves the committed orbital signature"),
					PreviewGenerator->GetPreviewGlobeProfileSignature(),
					OrbitSurfaceProfileSignatureBefore);

				FVector PlanetCenter = FVector::ZeroVector;
				double PlanetRadius = 0.0;
				Test->TestTrue(TEXT("PLANET wheel zoom retains a valid focus sphere"),
					PreviewGenerator->GetPreviewFocusSphere(
						EAstroPreviewFocus::HomePlanet, PlanetCenter, PlanetRadius));
				const FVector OrbitCameraLocation = Controller->PlayerCameraManager
					? Controller->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
				Test->TestTrue(TEXT("PLANET RMB keeps the preview camera fixed"),
					OrbitCameraLocation.Equals(OrbitCameraLocationBefore, 1.0));
				ZoomCameraDistanceBefore = FVector::Distance(OrbitCameraLocation, PlanetCenter);
				LastOrbitDistanceError = FMath::Abs(
					ZoomCameraDistanceBefore - OrbitCameraDistanceBefore)
					/ FMath::Max(OrbitCameraDistanceBefore, 1.0);
				Test->TestTrue(TEXT("PLANET orbit preserves proportional camera distance"),
					LastOrbitDistanceError <= 0.01);
				Test->TestTrue(TEXT("PLANET RMB keeps the inert resolver rotation fixed"),
					Root->GetActorQuat().Equals(OrbitResolverRotationBefore, 1.0e-4));
				Test->TestTrue(TEXT("PLANET RMB keeps the inert resolver centre fixed"),
					Root->GetActorLocation().Equals(OrbitResolverLocationBefore, 1.0));
				Test->TestTrue(TEXT("PLANET RMB keeps the resolver observer fixed"),
					Root->OverridedPlayerPosition.Equals(OrbitOverridePositionBefore, 1.0));
				if (IsValid(TerrainProxy))
				{
					Test->TestTrue(TEXT("PLANET RMB rotates the selected closed globe"),
						!TerrainProxy->GetComponentQuat().Equals(
							OrbitTerrainRotationBefore, 1.0e-4));
					Test->TestTrue(TEXT("PLANET RMB keeps the selected globe centred"),
						TerrainProxy->GetComponentLocation().Equals(
							OrbitTerrainLocationBefore, 1.0));
				}
				if (APlanetaryBody* FamilyBody = OrbitFamilyBody.Get())
				{
					FVector FamilyBodyLocationAfter = FVector::ZeroVector;
					Test->TestTrue(TEXT("PLANET RMB retains a presentation centre for its moon"),
						PreviewGenerator->GetPreviewPresentationLocation(
							FamilyBody, FamilyBodyLocationAfter));
					Test->TestTrue(TEXT("PLANET RMB rotates the selected planet/moon family"),
						!FamilyBodyLocationAfter.Equals(OrbitFamilyBodyLocationBefore, 1.0));
					const double FamilyDistanceAfter = FVector::Distance(
						FamilyBodyLocationAfter, PlanetCenter);
					Test->TestTrue(TEXT("PLANET RMB preserves the moon presentation orbit radius"),
						FMath::IsNearlyEqual(FamilyDistanceAfter,
							OrbitFamilyBodyDistanceBefore,
							FMath::Max(OrbitFamilyBodyDistanceBefore * 0.01, 1.0)));
				}
				ZoomCameraLocationBefore = OrbitCameraLocation;
				ViewModel->EndPreviewOrbit();
				bOrbitReleasePending = false;
				bZoomValidationPending = true;
				ViewModel->ZoomPreview(1.0f);
				return false;
			}
			if (bZoomValidationPending)
			{
				Test->TestFalse(TEXT("PLANET wheel zoom keeps the resolver root hidden"),
					bRootVisible);
				Test->TestFalse(TEXT("PLANET wheel zoom does not expose backing meshes"),
					bBackingVisible);
				Test->TestTrue(TEXT("PLANET wheel zoom keeps the closed terrain visible"),
					bTerrainVisible);
				Test->TestEqual(TEXT("PLANET wheel zoom retains the same orbital front buffer"),
					TerrainProxy, OrbitTerrainProxyBefore.Get());
				Test->TestEqual(TEXT("PLANET wheel zoom does not rebuild the surface profile"),
					Surface->AppliedSurfaceProfileSignature,
					OrbitSurfaceProfileSignatureBefore);
				Test->TestEqual(TEXT("PLANET wheel zoom does not reapply the surface profile"),
					PreviewGenerator->GetPreviewSurfaceProfileApplyCount(),
					OrbitSurfaceProfileApplyCountBefore);
				Test->TestEqual(TEXT("PLANET wheel zoom launches no WorldScape worker batch"),
					Root->WorldScapeLodInGeneration.Num(), 0);
				Test->TestFalse(TEXT("PLANET wheel zoom leaves the resolver producer disabled"),
					Root->bGenerateWorldScape);
				Test->TestTrue(TEXT("PLANET wheel zoom keeps the resolver root centre fixed"),
					Root->GetActorLocation().Equals(OrbitResolverLocationBefore, 1.0));
				Test->TestTrue(TEXT("PLANET wheel zoom keeps the resolver root rotation fixed"),
					Root->GetActorQuat().Equals(OrbitResolverRotationBefore, 1.0e-4));
				Test->TestTrue(TEXT("PLANET wheel zoom keeps the resolver observer fixed"),
					Root->OverridedPlayerPosition.Equals(OrbitOverridePositionBefore, 1.0));
				if (PreviewGenerator->IsActorTickEnabled()
					|| Root->WorldScapeLodInGeneration.Num() > 0)
				{
					return false;
				}

				const FVector ZoomCameraLocation = Controller->PlayerCameraManager
					? Controller->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
				FVector PlanetCenter;
				double PlanetRadius = 0.0;
				PreviewGenerator->GetPreviewFocusSphere(
					EAstroPreviewFocus::HomePlanet, PlanetCenter, PlanetRadius);
				const double ZoomCameraDistance = FVector::Distance(
					ZoomCameraLocation, PlanetCenter);
				Test->TestTrue(TEXT("PLANET wheel zoom moves the preview camera"),
					!ZoomCameraLocation.Equals(ZoomCameraLocationBefore, 1.0));
				Test->TestTrue(TEXT("Positive PLANET wheel zoom moves toward the body"),
					ZoomCameraDistance < ZoomCameraDistanceBefore);
				bZoomValidationPending = false;
			}
			if (!HasReadyPlanetOrbitalSurface(Planet, Surface, Root)
				|| bRootVisible || TerrainProxy != OrbitTerrainProxyBefore.Get())
			{
				return false;
			}
			if (!WaitForCompilationIdle(Now, TEXT("PLANET_ORBIT")))
			{
				BeginOperationFrameTracking(Now);
				return false;
			}

			Test->TestFalse(TEXT("PLANET RMB/zoom never exposes the inert resolver root"),
				bObservedInteractionRootVisible);
			Test->TestFalse(TEXT("PLANET orbit never exposes an authored backing mesh"),
				bObservedOrbitBackingVisible);
			Test->TestFalse(TEXT("PLANET RMB/zoom never hides the selected orbital proxy"),
				bObservedInteractionSelectedProxyHidden);
			Test->TestFalse(TEXT("PLANET RMB/zoom never clears validated orbital readiness"),
				bObservedInteractionSurfaceNotReady);
			Test->TestFalse(TEXT("PLANET RMB/zoom never wakes the WorldScape resolver"),
				bObservedInteractionWorkersInFlight);
			AssertOperationFrameBudget(TEXT("PLANET_ORBIT_REFRESH"));
			AssertReadyPlanetOrbitalSurface(Surface, Root, Planet, TEXT("PLANET orbit"));
			AAtmoScape* OrbitAtmosphere = Planet->PlanetaryEnvironmentGenerator
				? Planet->PlanetaryEnvironmentGenerator->PlanetAtmosphere : nullptr;
			UStaticMeshComponent* OrbitAtmosphereShell = nullptr;
			double OrbitAtmospherePresentationRadius = 0.0;
			AssertAtmospherePresentation(Planet, OrbitAtmosphere,
				TEXT("PLANET orbit/zoom"), OrbitAtmosphereShell,
				OrbitAtmospherePresentationRadius);
			Test->TestEqual(TEXT("PLANET orbit retains the persistent inert resolver root"),
				Root, CurrentResolverRoot.Get());
			Test->TestEqual(TEXT("PLANET orbit retains the persistent profile resolver"),
				Surface, CurrentSurfaceResolver.Get());
			Test->TestEqual(TEXT("PLANET RMB/zoom retains the same committed terrain proxy"),
				TerrainProxy, OrbitTerrainProxyBefore.Get());
			Test->TestEqual(TEXT("PLANET orbit retains the exact resolved profile signature"),
				Surface->AppliedSurfaceProfileSignature,
				OrbitSurfaceProfileSignatureBefore);
			Test->TestEqual(TEXT("PLANET RMB/zoom retains one surface-profile application"),
				PreviewGenerator->GetPreviewSurfaceProfileApplyCount(),
				OrbitSurfaceProfileApplyCountBefore);
			Test->TestFalse(TEXT("Ready preview generator sleeps after camera-only RMB orbit"),
				PreviewGenerator->IsActorTickEnabled());

			BeginSample(Now);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke] Closed PLANET proxy survived held RMB and wheel zoom in %.2fs cameraError=%.4f; starting warm sample"),
				Now - StepStartSeconds, LastOrbitDistanceError);
			Step = 6;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdatePlanetPerformance(UWorld* World,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !ViewModel || !PreviewGenerator.IsValid())
			{
				return Fail(TEXT("Preview disappeared during the warm PLANET sample"));
			}
			if (!UpdateSample(Now, PlanetSampleSeconds, TEXT("PLANET_ORBITAL_PROXY")))
			{
				return false;
			}

			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				TEXT("Screenshots/Windows/APS_MainMenu_PlanetSmoke.png"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false, false);
			UE_LOG(LogTemp, Display, TEXT("[APS.Smoke] Warm sample complete; screenshot=%s"),
				*ScreenshotPath);
			Step = 7;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateScreenshot(double Now)
		{
			if (!IFileManager::Get().FileExists(*ScreenshotPath))
			{
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("Rendered PLANET screenshot was not written"));
				}
				return false;
			}

			UWorld* World = PreviewGenerator.IsValid() ? PreviewGenerator->GetWorld() : nullptr;
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UGameViewportClient* GameViewportClient = AutomationCommon::GetAnyGameViewportClient();
			FViewport* Viewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
			APlanetaryBody* Body = PreviewGenerator.IsValid()
				? PreviewGenerator->GetActivePreviewWorldScapeBody() : nullptr;
			APlanetarySurfaceGenerator* Surface = World && PreviewGenerator.IsValid()
				? FindPreviewSurfaceGenerator(World, PreviewGenerator.Get()) : nullptr;
			AWorldScapeRoot* Root = Surface ? Surface->WorldScapeRootInstance : nullptr;
			UProceduralMeshComponent* TerrainProxy = IsValid(Body)
				? PreviewGenerator->GetPreviewTerrainProxyForBody(Body) : nullptr;
			if (!World || !Controller || !Viewport || GameViewportClient->GetWorld() != World
				|| !HasReadyPlanetOrbitalSurface(Body, Surface, Root)
				|| !IsPresented(TerrainProxy)
				|| TerrainProxy->GetProcMeshSection(0) == nullptr)
			{
				return Fail(TEXT("Rendered PLANET frame has no readable game viewport/closed orbital proxy"));
			}
			AssertReadyPlanetOrbitalSurface(Surface, Root, Body,
				TEXT("Rendered PLANET frame"));

			const FIntPoint ViewportSize = Viewport->GetSizeXY();
			TArray<FColor> Pixels;
			if (ViewportSize.X <= 0 || ViewportSize.Y <= 0 || !Viewport->ReadPixels(Pixels)
				|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
			{
				if (Now - StepStartSeconds <= ScreenshotTimeoutSeconds) return false;
				return Fail(TEXT("Rendered PLANET viewport pixels could not be read"));
			}

			struct FScreenExclusion
			{
				FVector2D Center{FVector2D::ZeroVector};
				double RadiusSquared{0.0};
			};
			TArray<FScreenExclusion> ScreenExclusions;
			const auto AddProjectedSphereExclusion = [&](const FVector& Center,
				double SphereRadius, double RadiusMultiplier, double PaddingPixels)
			{
				const FVector CameraRight = Controller->PlayerCameraManager
					? Controller->PlayerCameraManager->GetCameraRotation().RotateVector(FVector::RightVector)
					: FVector::RightVector;
				FVector2D ScreenCenter;
				FVector2D ScreenEdge;
				if (!Controller->ProjectWorldLocationToScreen(Center, ScreenCenter, false)
					|| !Controller->ProjectWorldLocationToScreen(
						Center + CameraRight * SphereRadius, ScreenEdge, false))
				{
					return;
				}
				const double Radius = FVector2D::Distance(ScreenCenter, ScreenEdge)
					* RadiusMultiplier + PaddingPixels;
				if (FMath::IsFinite(Radius) && Radius > 0.0)
				{
					ScreenExclusions.Add({ScreenCenter, FMath::Square(Radius)});
				}
			};
			const auto AddProjectedExclusion = [&](UPrimitiveComponent* Primitive,
				double RadiusMultiplier, double PaddingPixels)
			{
				if (!IsValid(Primitive)) return;
				Primitive->UpdateBounds();
				AddProjectedSphereExclusion(Primitive->Bounds.Origin,
					Primitive->Bounds.SphereRadius, RadiusMultiplier, PaddingPixels);
			};
			FVector PlanetCenter = FVector::ZeroVector;
			double PlanetRadius = 0.0;
			if (PreviewGenerator->GetPreviewFocusSphere(
				EAstroPreviewFocus::HomePlanet, PlanetCenter, PlanetRadius))
			{
				AddProjectedSphereExclusion(PlanetCenter, PlanetRadius, 1.22, 10.0);
			}
			if (AStarSystem* System = FindGeneratedStarSystem(World, PreviewGenerator.Get()))
			{
				for (AStar* Star : System->GetStars())
				{
					AddProjectedExclusion(IsValid(Star) ? Star->StarMesh : nullptr, 2.5, 16.0);
				}
			}

			const int32 MinX = FMath::FloorToInt(ViewportSize.X * 0.25);
			const int32 MaxX = FMath::CeilToInt(ViewportSize.X * 0.75);
			const int32 MinY = FMath::FloorToInt(ViewportSize.Y * 0.04);
			const int32 MaxY = FMath::CeilToInt(ViewportSize.Y * 0.82);
			int64 BackgroundPixelCount = 0;
			int64 NearClippedPixelCount = 0;
			int64 HardClippedPixelCount = 0;
			for (int32 Y = MinY; Y < MaxY; ++Y)
			{
				for (int32 X = MinX; X < MaxX; ++X)
				{
					// The central logo is UI, not astronomical background.
					if (Y < ViewportSize.Y * 0.17
						&& X > ViewportSize.X * 0.35 && X < ViewportSize.X * 0.65)
					{
						continue;
					}
					bool bExcluded = false;
					for (const FScreenExclusion& Exclusion : ScreenExclusions)
					{
						const FVector2D Delta = FVector2D(X, Y) - Exclusion.Center;
						if (Delta.SizeSquared() <= Exclusion.RadiusSquared)
						{
							bExcluded = true;
							break;
						}
					}
					if (bExcluded) continue;
					const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
					const uint8 Brightness = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
					NearClippedPixelCount += Brightness >= 225 ? 1 : 0;
					HardClippedPixelCount += Brightness >= 250 ? 1 : 0;
					++BackgroundPixelCount;
				}
			}
			if (BackgroundPixelCount <= 0)
			{
				return Fail(TEXT("Rendered PLANET frame has no unobscured canvas background pixels"));
			}
			const double NearClippedRatio = static_cast<double>(NearClippedPixelCount)
				/ static_cast<double>(BackgroundPixelCount);
			const double HardClippedRatio = static_cast<double>(HardClippedPixelCount)
				/ static_cast<double>(BackgroundPixelCount);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke.Pixel] PLANET backgroundSamples=%lld nearClipped=%lld ratio=%.6f hardClipped=%lld ratio=%.6f exclusions=%d"),
				BackgroundPixelCount, NearClippedPixelCount, NearClippedRatio,
				HardClippedPixelCount, HardClippedRatio, ScreenExclusions.Num());
			Test->TestTrue(TEXT("PLANET canvas has no clustered near-white background bloom"),
				NearClippedRatio <= 0.006);
			Test->TestTrue(TEXT("PLANET canvas has no clipped background wash"),
				HardClippedRatio <= 0.002);
			Test->TestTrue(TEXT("Rendered PLANET smoke screenshot was written"), true);
			UWorldGenerationViewModel* ViewModel = Controller->GetWorldGenerationViewModel();
			if (!ViewModel || !IsValid(Body))
			{
				return Fail(TEXT("Cannot start unsupported-body WorldScape cleanup probe"));
			}
			UnsupportedSurfaceRevisionBeforeChange = ViewModel->PreviewRevision;
			UnsupportedSurfaceBody = Body;
			bObservedUnsupportedWorldScapeVisible = false;
			ViewModel->SetEnumValue(StaticEnum<EPlanetType>(),
				static_cast<int32>(EPlanetType::GasGiant));
			Step = 8;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateUnsupportedSurfaceCleanup(UWorld* World,
			UWorldGenerationViewModel* ViewModel, double Now)
		{
			if (!World || !ViewModel || !PreviewGenerator.IsValid()
				|| !UnsupportedSurfaceBody.IsValid())
			{
				return Fail(TEXT("Preview disappeared during unsupported-body cleanup probe"));
			}
			if (Now - StepStartSeconds > SurfaceTimeoutSeconds)
			{
				const FPreviewWorldScapeTopology TimedOutTopology =
					CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
				return Fail(FString::Printf(
					TEXT("Unsupported body did not settle with one inert resolver pair generators=%d roots=%d visible=%d orphanGenerator=%s orphanRoot=%s duplicateLink=%s"),
					TimedOutTopology.Generators.Num(), TimedOutTopology.Roots.Num(),
					TimedOutTopology.VisibleRootCount,
					TimedOutTopology.bHasGeneratorWithoutRoot ? TEXT("true") : TEXT("false"),
					TimedOutTopology.bHasRootWithoutGenerator ? TEXT("true") : TEXT("false"),
					TimedOutTopology.bHasDuplicateRootLink ? TEXT("true") : TEXT("false")));
			}

			APlanetaryBody* Body = UnsupportedSurfaceBody.Get();
			const FPreviewWorldScapeTopology Topology =
				CapturePreviewWorldScapeTopology(World, PreviewGenerator.Get());
			bObservedUnsupportedWorldScapeVisible =
				bObservedUnsupportedWorldScapeVisible || Topology.VisibleRootCount > 0;
			if (ViewModel->PreviewRevision <= UnsupportedSurfaceRevisionBeforeChange
				|| Body->PlanetType != EPlanetType::GasGiant)
			{
				return false;
			}
			Test->TestFalse(TEXT("Gas giant is outside the WorldScape solid-body pipeline"),
				UAPSPlanetSurfaceProfileResolver::SupportsWorldScape(Body->PlanetType));
			APlanetarySurfaceGenerator* Resolver = Topology.Generators.Num() == 1
				? Topology.Generators[0] : nullptr;
			AWorldScapeRoot* ResolverRoot = Topology.Roots.Num() == 1
				? Topology.Roots[0] : nullptr;
			if (!IsValid(Resolver) || !IsValid(ResolverRoot)
				|| PreviewGenerator->IsActorTickEnabled()
				|| ResolverRoot->WorldScapeLodInGeneration.Num() > 0)
			{
				return false;
			}

			Test->TestFalse(TEXT("Unsupported-body hand-off never leaves a WorldScape root visible"),
				bObservedUnsupportedWorldScapeVisible);
			Test->TestEqual(TEXT("Unsupported body retains the one reusable profile resolver"),
				Topology.Generators.Num(), 1);
			Test->TestEqual(TEXT("Unsupported body retains the one reusable inert root"),
				Topology.Roots.Num(), 1);
			Test->TestEqual(TEXT("Unsupported body presents no WorldScape root"),
				Topology.VisibleRootCount, 0);
			Test->TestEqual(TEXT("Unsupported-body hand-off retains resolver identity"),
				Resolver, CurrentSurfaceResolver.Get());
			Test->TestEqual(TEXT("Unsupported-body hand-off retains inert root identity"),
				ResolverRoot, CurrentResolverRoot.Get());
			Test->TestTrue(TEXT("Unsupported-body resolver root remains hidden"),
				ResolverRoot->IsHidden());
			Test->TestFalse(TEXT("Unsupported-body resolver root remains unticked"),
				ResolverRoot->IsActorTickEnabled());
			Test->TestFalse(TEXT("Unsupported-body resolver root launches no generation"),
				ResolverRoot->bGenerateWorldScape);
			Test->TestFalse(TEXT("Unsupported body leaves no orphan generator"),
				Topology.bHasGeneratorWithoutRoot);
			Test->TestFalse(TEXT("Unsupported body leaves no orphan root"),
				Topology.bHasRootWithoutGenerator);
			Test->TestFalse(TEXT("Unsupported body leaves no duplicate root link"),
				Topology.bHasDuplicateRootLink);
			Test->TestFalse(TEXT("Unsupported body never presents a selected terrain proxy"),
				IsPresented(PreviewGenerator->GetPreviewTerrainProxyForBody(Body)));
			Test->TestFalse(TEXT("Unsupported body never presents a selected ocean proxy"),
				IsPresented(PreviewGenerator->GetPreviewOceanProxyForBody(Body)));
			Test->TestFalse(TEXT("Unsupported body cannot report an orbital proxy ready"),
				Body->bWorldScapeSurfaceReady);
			Test->TestTrue(TEXT("Unsupported body falls back to its authored presentation mesh"),
				HasPresentedStaticMesh(Body));
			Step = 9;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateCleanup(double Now)
		{
			if (PreviewGenerator.IsValid() && !PreviewGenerator->PreparePreviewForTravel())
			{
				if (Now - StepStartSeconds <= CleanupTimeoutSeconds)
				{
					return false;
				}
				if (PendingFailure.IsEmpty())
				{
					PendingFailure = TEXT("WorldScape workers did not drain during rendered smoke cleanup");
				}
			}

			if (!PendingFailure.IsEmpty())
			{
				Test->AddError(PendingFailure);
			}
			else
			{
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Smoke] PASS hierarchy, cameras, resolver, atmosphere, closed orbital proxies and worker drain"));
			}
			for (const TWeakObjectPtr<UObject>& WarmedAsset : WarmedSurfaceAssets)
			{
				if (UObject* Asset = WarmedAsset.Get(); IsValid(Asset) && Asset->IsRooted())
				{
					Asset->RemoveFromRoot();
				}
			}
			WarmedSurfaceAssets.Reset();
			return true;
		}

		void BeginSample(double Now)
		{
			SampleStartSeconds = Now;
			SampleLastFrameSeconds = Now;
			SampleFrameCount = 0;
			SampleMaxFrameSeconds = 0.0;
		}

		void BeginOperationFrameTracking(double Now)
		{
			OperationLastFrameSeconds = Now;
			OperationMaxFrameSeconds = 0.0;
		}

		void TrackOperationFrame(double Now)
		{
			const double FrameSeconds = Now - OperationLastFrameSeconds;
			OperationLastFrameSeconds = Now;
			if (FrameSeconds > 0.0 && FMath::IsFinite(FrameSeconds))
			{
				OperationMaxFrameSeconds = FMath::Max(OperationMaxFrameSeconds, FrameSeconds);
			}
		}

		void AssertOperationFrameBudget(const FString& OperationName)
		{
			const double MaxFrameMilliseconds = OperationMaxFrameSeconds * 1000.0;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke.Perf] operation=%s maxFrameMs=%.2f"),
				*OperationName, MaxFrameMilliseconds);
			Test->TestTrue(
				FString::Printf(TEXT("%s has no quarter-second interaction stall"), *OperationName),
				MaxFrameMilliseconds < 250.0);
		}

		bool UpdateSample(double Now, double RequiredSeconds, const FString& ScopeName)
		{
			const double FrameSeconds = Now - SampleLastFrameSeconds;
			SampleLastFrameSeconds = Now;
			if (FrameSeconds > 0.0 && FMath::IsFinite(FrameSeconds))
			{
				++SampleFrameCount;
				SampleMaxFrameSeconds = FMath::Max(SampleMaxFrameSeconds, FrameSeconds);
			}
			const double Elapsed = Now - SampleStartSeconds;
			if (Elapsed < RequiredSeconds)
			{
				return false;
			}
			const double AverageFps = SampleFrameCount / FMath::Max(Elapsed, UE_DOUBLE_SMALL_NUMBER);
			const double MaxFrameMilliseconds = SampleMaxFrameSeconds * 1000.0;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Smoke.Perf] scope=%s frames=%d seconds=%.2f avgFps=%.1f maxFrameMs=%.2f operationalFloor=%.0f qualityTarget=%.0f"),
				*ScopeName, SampleFrameCount, Elapsed, AverageFps, MaxFrameMilliseconds,
				OperationalRegressionFloorFps, DesiredPreviewFps);
			Test->TestTrue(
				FString::Printf(TEXT("%s avoids the sustained 30-FPS regression"), *ScopeName),
				AverageFps >= OperationalRegressionFloorFps);
			if (AverageFps < DesiredPreviewFps)
			{
				Test->AddWarning(FString::Printf(
					TEXT("%s averaged %.1f FPS below the %.0f FPS quality target in RenderOffscreen"),
					*ScopeName, AverageFps, DesiredPreviewFps));
			}
			Test->TestTrue(
				FString::Printf(TEXT("%s has no quarter-second warm-frame stall"), *ScopeName),
				MaxFrameMilliseconds < 250.0);
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
		TWeakObjectPtr<APlanet> InitialPlanet;
		TWeakObjectPtr<AWorldScapeRoot> CurrentResolverRoot;
		TWeakObjectPtr<APlanetarySurfaceGenerator> CurrentSurfaceResolver;
		TWeakObjectPtr<AWorldScapeRoot> SwapPreviousWorldScapeRoot;
		TWeakObjectPtr<APlanetarySurfaceGenerator> SwapPreviousSurfaceGenerator;
		TWeakObjectPtr<UProceduralMeshComponent> SwapPreviousTerrainProxy;
		TWeakObjectPtr<UMaterialInterface> SwapPreviousTerrainMaterial;
		int32 Step{0};
		int32 FocusIndex{0};
		int32 SurfaceRevisionBeforeChange{0};
		int32 SurfaceSubtypeProbeIndex{INDEX_NONE};
		uint32 PreviousSurfaceProfileSignature{0};
		int32 SurfaceRevisionBeforeSliderBurst{0};
		int32 UnsupportedSurfaceRevisionBeforeChange{0};
		int32 SurfaceProfileApplyCountBeforeSliderBurst{0};
		uint32 SurfaceSignatureBeforeSliderBurst{0};
		TWeakObjectPtr<UAPSWorldScapePlanetNoise> NoiseBeforeSliderBurst;
		TWeakObjectPtr<APlanetaryBody> UnsupportedSurfaceBody;
		int32 InitialGalaxyInstanceCount{0};
		int32 InitialClusterInstanceCount{0};
		int32 InitialGalaxySentinelIndex{INDEX_NONE};
		int32 InitialClusterSentinelIndex{INDEX_NONE};
		FVector InitialGalaxySentinelLocation{FVector::ZeroVector};
		FVector InitialClusterSentinelLocation{FVector::ZeroVector};
		FVector InitialGalaxySentinelScale{FVector::ZeroVector};
		FVector InitialClusterSentinelScale{FVector::ZeroVector};
		float InitialGalaxySentinelEmission{0.0f};
		float InitialClusterSentinelEmission{0.0f};
		int32 HomeClusterProxyIndex{INDEX_NONE};
		FVector HomeClusterProxyScale{FVector::ZeroVector};
		TArray<TWeakObjectPtr<AActor>> InvariantHierarchyActors;
		TArray<FVector> InitialInvariantActorLocations;
		FVector PreviousScopeViewDirection{FVector::ZeroVector};
		bool bHasPreviousScopeViewDirection{false};
		double TestStartSeconds{0.0};
		double StepStartSeconds{0.0};
		double InitialPlanetCameraDistance{0.0};
		FVector OrbitCameraLocationBefore{FVector::ZeroVector};
		double OrbitCameraDistanceBefore{0.0};
		TWeakObjectPtr<AWorldScapeRoot> OrbitResolverRootBefore;
		FQuat OrbitResolverRotationBefore{FQuat::Identity};
		FVector OrbitResolverLocationBefore{FVector::ZeroVector};
		FVector OrbitOverridePositionBefore{FVector::ZeroVector};
		TWeakObjectPtr<UProceduralMeshComponent> OrbitTerrainProxyBefore;
		FQuat OrbitTerrainRotationBefore{FQuat::Identity};
		FVector OrbitTerrainLocationBefore{FVector::ZeroVector};
		TWeakObjectPtr<APlanetaryBody> OrbitFamilyBody;
		FVector OrbitFamilyBodyLocationBefore{FVector::ZeroVector};
		double OrbitFamilyBodyDistanceBefore{0.0};
		uint32 OrbitSurfaceProfileSignatureBefore{0};
		int32 OrbitSurfaceProfileApplyCountBefore{0};
		FVector ZoomCameraLocationBefore{FVector::ZeroVector};
		double ZoomCameraDistanceBefore{0.0};
		double LastOrbitDistanceError{0.0};
		FVector SliderCameraLocationBefore{FVector::ZeroVector};
		double SampleStartSeconds{0.0};
		double SampleLastFrameSeconds{0.0};
		double SampleMaxFrameSeconds{0.0};
		double OperationLastFrameSeconds{0.0};
		double OperationMaxFrameSeconds{0.0};
		int32 SampleFrameCount{0};
		bool bSampling{false};
		bool bObservedReadyWithoutCommittedOrbitalProxy{false};
		bool bOrbitalSwapObservationActive{false};
		bool bObservedOrbitalProxyCommit{false};
		bool bObservedOrbitalProxyContinuityViolation{false};
		bool bObservedResolverTopologyViolation{false};
		bool bObservedResolverMutationViolation{false};
		bool bObservedUnsupportedWorldScapeVisible{false};
		bool bSurfaceSliderBurstStarted{false};
		bool bOrbitReleasePending{false};
		bool bZoomValidationPending{false};
		bool bObservedInteractionRootVisible{false};
		bool bObservedOrbitBackingVisible{false};
		bool bObservedInteractionSelectedProxyHidden{false};
		bool bObservedInteractionSurfaceNotReady{false};
		bool bObservedInteractionWorkersInFlight{false};
		bool bRenderWarmupPrepared{false};
		double CompilationIdleSince{0.0};
		double LastCompilationLogSeconds{0.0};
		FString CompilationWaitContext;
		TArray<TWeakObjectPtr<UObject>> WarmedSurfaceAssets;
		FString ScreenshotPath;
		FString PendingFailure;
	};

	class FRenderedStellarPresentationCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRenderedStellarPresentationCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (StartSeconds <= 0.0)
			{
				StartSeconds = Now;
				StepStartSeconds = Now;
			}
			if (Step != 6 && Now - StartSeconds > 300.0)
			{
				return Fail(TEXT("Stellar presentation type-switch scenario timed out"));
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			if (Step == 6)
			{
				if (Generator.IsValid() && !Generator->PreparePreviewForTravel())
				{
					if (Now - StepStartSeconds < 20.0)
					{
						return false;
					}
					return FailImmediate(TEXT("Stellar presentation cleanup could not drain preview workers"));
				}
				return PendingFailure.IsEmpty() ? true : FailImmediate(PendingFailure);
			}
			if (Step == 7)
			{
				if (Now - StepStartSeconds < 0.25)
				{
					return false;
				}
				SystemScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
					TEXT("Screenshots/Windows/APS_MainMenu_StellarPresentation_SYSTEM.png"));
				FString CaptureFailure;
				if (!CaptureGameViewport(World, SystemScreenshotPath,
					TEXT("SYSTEM"), CaptureFailure))
				{
					if (CaptureFailure.IsEmpty()
						&& Now - StepStartSeconds <= ScreenshotTimeoutSeconds)
					{
						return false;
					}
					return Fail(CaptureFailure.IsEmpty()
						? TEXT("Rendered SYSTEM game viewport pixels could not be read")
						: CaptureFailure);
				}
				Step = 3;
				StepStartSeconds = Now;
				return false;
			}
			if (Step == 8)
			{
				if (Now - StepStartSeconds < 0.25)
				{
					return false;
				}
				StarScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
					TEXT("Screenshots/Windows/APS_MainMenu_StellarPresentation_STAR.png"));
				FString CaptureFailure;
				if (!CaptureGameViewport(World, StarScreenshotPath,
					TEXT("STAR"), CaptureFailure))
				{
					if (CaptureFailure.IsEmpty()
						&& Now - StepStartSeconds <= ScreenshotTimeoutSeconds)
					{
						return false;
					}
					return Fail(CaptureFailure.IsEmpty()
						? TEXT("Rendered STAR game viewport pixels could not be read")
						: CaptureFailure);
				}
				Step = 5;
				StepStartSeconds = Now;
				return false;
			}
			if (Step == 3)
			{
				if (!IFileManager::Get().FileExists(*SystemScreenshotPath))
				{
					if (Now - StepStartSeconds <= ScreenshotTimeoutSeconds)
					{
						return false;
					}
					return Fail(TEXT("Rendered SYSTEM stellar-presentation screenshot was not written"));
				}
				ViewModel->SetPreviewFocus(EAstroPreviewFocus::HomeStar);
				Step = 4;
				StepStartSeconds = Now;
				return false;
			}
			if (Step == 4)
			{
				if (!ViewModel->bPreviewReady
					|| ViewModel->GetPreviewFocus() != EAstroPreviewFocus::HomeStar
					|| Now - StepStartSeconds < 1.5)
				{
					return false;
				}
				AStarSystem* System = Generator.IsValid()
					? FindGeneratedStarSystem(World, Generator.Get()) : nullptr;
				AStar* Star = System && !System->GetStars().IsEmpty()
					? System->GetStars()[0] : nullptr;
				if (!IsValid(Star) || !IsValid(Star->StarMesh))
				{
					return Fail(TEXT("Rendered STAR focus has no readable stellar mesh"));
				}
				Star->StarMesh->UpdateBounds();
				FVector InfluenceCenter;
				double InfluenceRadius = 0.0;
				bool bInfluenceVisible = false;
				if (!Generator->GetPreviewGuideShellState(EAstroPreviewFocus::HomeStar,
					InfluenceCenter, InfluenceRadius, bInfluenceVisible))
				{
					return Fail(TEXT("Rendered STAR focus has no line-only influence guide"));
				}
				Test->TestTrue(TEXT("STAR focus keeps its compact influence guide visible"),
					bInfluenceVisible);
				Test->TestTrue(TEXT("STAR focus guide stays centred on rendered star"),
					InfluenceCenter.Equals(Star->StarMesh->Bounds.Origin, 1.0));
				Test->TestTrue(TEXT("STAR focus guide is exactly 1.36 visible star radii"),
					FMath::IsNearlyEqual(InfluenceRadius,
						Star->StarMesh->Bounds.SphereRadius * 1.36,
						FMath::Max(2.0, InfluenceRadius * 1.0e-5)));
				FVector StarFrameCenter;
				double StarFrameRadius = 0.0;
				if (!Generator->GetPreviewFocusSphere(EAstroPreviewFocus::HomeStar,
					StarFrameCenter, StarFrameRadius))
				{
					return Fail(TEXT("Rendered STAR focus has no camera framing sphere"));
				}
				Test->TestTrue(TEXT("STAR camera frame shares the guide centre"),
					StarFrameCenter.Equals(InfluenceCenter, 1.0));
				Test->TestTrue(TEXT("STAR camera frame contains the complete influence guide"),
					StarFrameRadius >= InfluenceRadius * 1.04);
				if (IsValid(Controller->PlayerCameraManager))
				{
					int32 ViewWidth = 0;
					int32 ViewHeight = 0;
					Controller->GetViewportSize(ViewWidth, ViewHeight);
					const FRotator ViewRotation =
						Controller->PlayerCameraManager->GetCameraRotation();
					const FVector GuideExtrema[] =
					{
						InfluenceCenter + ViewRotation.RotateVector(FVector::RightVector) * InfluenceRadius,
						InfluenceCenter - ViewRotation.RotateVector(FVector::RightVector) * InfluenceRadius,
						InfluenceCenter + ViewRotation.RotateVector(FVector::UpVector) * InfluenceRadius,
						InfluenceCenter - ViewRotation.RotateVector(FVector::UpVector) * InfluenceRadius
					};
					for (int32 ExtremeIndex = 0; ExtremeIndex < UE_ARRAY_COUNT(GuideExtrema); ++ExtremeIndex)
					{
						FVector2D ScreenPosition;
						const bool bProjected = ViewWidth > 0 && ViewHeight > 0
							&& Controller->ProjectWorldLocationToScreen(
								GuideExtrema[ExtremeIndex], ScreenPosition, true);
						Test->TestTrue(*FString::Printf(
							TEXT("STAR guide extremum %d projects into the viewport"), ExtremeIndex),
							bProjected && ScreenPosition.X >= 0.0 && ScreenPosition.X <= ViewWidth
							&& ScreenPosition.Y >= 0.0 && ScreenPosition.Y <= ViewHeight);
					}
				}
				FVector SystemBoundaryCenter;
				double SystemBoundaryRadius = 0.0;
				bool bSystemBoundaryVisible = true;
				if (!Generator->GetPreviewGuideShellState(EAstroPreviewFocus::HomeSystem,
					SystemBoundaryCenter, SystemBoundaryRadius, bSystemBoundaryVisible))
				{
					return Fail(TEXT("Rendered STAR focus cannot inspect system boundary guide"));
				}
				Test->TestFalse(TEXT("STAR focus hides the outer system boundary"),
					bSystemBoundaryVisible);
				FinishScreenshotCompilation(TEXT("STAR"));
				// Let Slate consume the now-idle compiler state before capturing; otherwise
				// the previous frame can still show the shader-preparation overlay/fallback.
				Step = 8;
				StepStartSeconds = Now;
				return false;
			}
			if (Step == 5)
			{
				if (!IFileManager::Get().FileExists(*StarScreenshotPath))
				{
					if (Now - StepStartSeconds <= ScreenshotTimeoutSeconds)
					{
						return false;
					}
					return Fail(TEXT("Rendered STAR stellar-presentation screenshot was not written"));
				}
				UE_LOG(LogTemp, Display,
					TEXT("[APS.StellarPresentation] screenshots system=%s star=%s"),
					*SystemScreenshotPath, *StarScreenshotPath);
				Step = 6;
				StepStartSeconds = Now;
				return false;
			}
			if (!World || !Controller || !ViewModel || !ViewModel->GeneratedWorld)
			{
				return false;
			}

			if (Step == 0)
			{
				UGeneratedWorld* Model = ViewModel->GeneratedWorld;
				Model->AstroGenerationLevel = EAstroGenerationLevel::StarSystem;
				Model->bGenerateFullScaledWorld = true;
				Model->bGenerateHomeSystem = true;
				Model->bStartWithHomePlanet = true;
				Model->bRandomHomeSystem = false;
				Model->bRandomHomeSystemType = false;
				Model->bRandomHomeStar = false;
				Model->bRandomStartPlanetNumber = false;
				Model->GenerationSeed = 271828;
				Model->StarType = EStarType::SingleStar;
				Model->StellarType = EStellarType::MainSequence;
				Model->SpectralClass = ESpectralClass::G;
				Model->PlanetarySystemType = EPlanetarySystemType::MultiPlanetSystem;
				Model->OrbitDistributionType = EOrbitDistributionType::Uniform;
				Model->PlanetsAmount = 5;
				Model->MoonsAmount = 1;
				Model->StartPlanetIndex = 1;
				Model->PlanetRadius = 6750.0;
				if (!Controller->OpenAstronomicalGenerationForAutomation(
					EAstroPreviewFocus::HomeSystem, EAPSGenerationRoute::Space))
				{
					return false;
				}
				Step = 1;
				StepStartSeconds = Now;
				return false;
			}

			if (!ViewModel->bPreviewReady || Now - StepStartSeconds < 1.0)
			{
				return false;
			}
			if (Step == 2 && ViewModel->PreviewRevision < RequestedRevision)
			{
				return false;
			}
			Generator = FindPreviewGenerator(World);
			if (!Generator.IsValid())
			{
				return Fail(TEXT("No tagged preview generator after stellar type switch"));
			}

			static constexpr EStellarType TypeSequence[] = {
				EStellarType::MainSequence,
				EStellarType::Giant,
				EStellarType::HyperGiant,
				EStellarType::SubDwarf,
				EStellarType::MainSequence
			};
			if (!ValidatePresentation(
				World, Controller, TypeSequence[TypeIndex]))
			{
				return Fail(FString::Printf(TEXT("Invalid rendered presentation for %s"),
					*UEnum::GetValueAsString(TypeSequence[TypeIndex])));
			}

			++TypeIndex;
			if (TypeIndex >= UE_ARRAY_COUNT(TypeSequence))
			{
				if (HyperGiantPhysicalRadius > 0.0 && SubDwarfPhysicalRadius > 0.0)
				{
					Test->TestTrue(TEXT("Physical stellar classes remain distinct under preview clamp"),
						HyperGiantPhysicalRadius > SubDwarfPhysicalRadius);
				}
				FinishScreenshotCompilation(TEXT("SYSTEM"));
				Step = 7;
				StepStartSeconds = Now;
				return false;
			}

			ViewModel->SetEnumValue(StaticEnum<EStellarType>(),
				static_cast<int32>(TypeSequence[TypeIndex]));
			RequestedRevision = ViewModel->PreviewRevision;
			Step = 2;
			StepStartSeconds = Now;
			return false;
		}

	private:
		bool CaptureGameViewport(UWorld* World, const FString& ScreenshotPath,
			const TCHAR* Context, FString& OutFailure) const
		{
			OutFailure.Reset();
			UGameViewportClient* GameViewportClient =
				AutomationCommon::GetAnyGameViewportClient();
			FViewport* Viewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
			if (!World || !Viewport || GameViewportClient->GetWorld() != World)
			{
				return false;
			}

			const FIntPoint ViewportSize = Viewport->GetSizeXY();
			TArray<FColor> Pixels;
			if (ViewportSize.X <= 0 || ViewportSize.Y <= 0
				|| !Viewport->ReadPixels(Pixels)
				|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
			{
				return false;
			}

			double BrightnessSum = 0.0;
			int64 NonBlackPixelCount = 0;
			const bool bStarFocus = FCString::Stricmp(Context, TEXT("STAR")) == 0;
			const double MinimumViewportExtent = FMath::Min(
				static_cast<double>(ViewportSize.X), static_cast<double>(ViewportSize.Y));
			const double StellarCoreRadiusSq = FMath::Square(MinimumViewportExtent * 0.22);
			const double StellarCenterRadiusSq = FMath::Square(MinimumViewportExtent * 0.10);
			const double StellarLimbInnerRadiusSq = FMath::Square(MinimumViewportExtent * 0.20);
			const double StellarLimbOuterRadiusSq = FMath::Square(MinimumViewportExtent * 0.24);
			int64 StellarCorePixelCount = 0;
			int64 StellarWhiteClipPixelCount = 0;
			int64 StellarPeakClipPixelCount = 0;
			double StellarRedSum = 0.0;
			double StellarGreenSum = 0.0;
			double StellarBlueSum = 0.0;
			double StellarPeakSum = 0.0;
			double StellarPeakSquaredSum = 0.0;
			int64 StellarCenterPixelCount = 0;
			int64 StellarLimbPixelCount = 0;
			double StellarCenterPeakSum = 0.0;
			double StellarLimbPeakSum = 0.0;
			for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
			{
				const FColor& Pixel = Pixels[PixelIndex];
				const uint8 Brightness = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
				BrightnessSum += Brightness;
				NonBlackPixelCount += Brightness > 12 ? 1 : 0;
				if (!bStarFocus)
				{
					continue;
				}

				const int32 PixelX = PixelIndex % ViewportSize.X;
				const int32 PixelY = PixelIndex / ViewportSize.X;
				const double DeltaX = PixelX - static_cast<double>(ViewportSize.X) * 0.5;
				const double DeltaY = PixelY - static_cast<double>(ViewportSize.Y) * 0.5;
				const double RadiusSq = DeltaX * DeltaX + DeltaY * DeltaY;
				if (RadiusSq <= StellarCenterRadiusSq)
				{
					++StellarCenterPixelCount;
					StellarCenterPeakSum += Brightness;
				}
				else if (RadiusSq >= StellarLimbInnerRadiusSq
					&& RadiusSq <= StellarLimbOuterRadiusSq)
				{
					++StellarLimbPixelCount;
					StellarLimbPeakSum += Brightness;
				}
				if (RadiusSq > StellarCoreRadiusSq || Brightness <= 12)
				{
					continue;
				}

				++StellarCorePixelCount;
				StellarRedSum += Pixel.R;
				StellarGreenSum += Pixel.G;
				StellarBlueSum += Pixel.B;
				StellarPeakSum += Brightness;
				StellarPeakSquaredSum += static_cast<double>(Brightness) * Brightness;
				StellarWhiteClipPixelCount +=
					Pixel.R >= 250 && Pixel.G >= 250 && Pixel.B >= 250 ? 1 : 0;
				StellarPeakClipPixelCount += Brightness >= 250 ? 1 : 0;
			}
			const double PixelCount = static_cast<double>(Pixels.Num());
			const double MeanBrightness = BrightnessSum / FMath::Max(PixelCount, 1.0);
			const double NonBlackRatio = static_cast<double>(NonBlackPixelCount)
				/ FMath::Max(PixelCount, 1.0);

			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			TArray64<uint8> PngData;
			FImageUtils::PNGCompressImageArray(ViewportSize.X, ViewportSize.Y,
				TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
			if (PngData.IsEmpty()
				|| !FFileHelper::SaveArrayToFile(PngData, *ScreenshotPath))
			{
				OutFailure = FString::Printf(
					TEXT("Could not encode/write %s stellar viewport screenshot %s"),
					Context, *ScreenshotPath);
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.StellarPresentation.Pixel] focus=%s viewport=%dx%d mean=%.3f nonBlackRatio=%.5f screenshot=%s"),
				Context, ViewportSize.X, ViewportSize.Y,
				MeanBrightness, NonBlackRatio, *ScreenshotPath);
			if (MeanBrightness < 1.0 || NonBlackRatio < 0.01)
			{
				OutFailure = FString::Printf(
					TEXT("%s stellar viewport is black/empty (mean=%.3f nonBlackRatio=%.5f; minimum 1.0/0.01); diagnostic screenshot=%s"),
					Context, MeanBrightness, NonBlackRatio, *ScreenshotPath);
				return false;
			}
			if (bStarFocus)
			{
				if (StellarCorePixelCount < 1000
					|| StellarCenterPixelCount <= 0 || StellarLimbPixelCount <= 0)
				{
					OutFailure = FString::Printf(
						TEXT("STAR viewport has no measurable centred stellar disc (core=%lld center=%lld limb=%lld); diagnostic screenshot=%s"),
						StellarCorePixelCount, StellarCenterPixelCount,
						StellarLimbPixelCount, *ScreenshotPath);
					return false;
				}

				const double MeanRed = StellarRedSum / StellarCorePixelCount;
				const double MeanGreen = StellarGreenSum / StellarCorePixelCount;
				const double MeanBlue = StellarBlueSum / StellarCorePixelCount;
				const double MeanPeak = StellarPeakSum / StellarCorePixelCount;
				const double PeakVariance = FMath::Max(0.0,
					StellarPeakSquaredSum / StellarCorePixelCount - MeanPeak * MeanPeak);
				const double SurfaceContrast = FMath::Sqrt(PeakVariance);
				const double WhiteClipRatio = static_cast<double>(StellarWhiteClipPixelCount)
					/ StellarCorePixelCount;
				const double PeakClipRatio = static_cast<double>(StellarPeakClipPixelCount)
					/ StellarCorePixelCount;
				const double CenterPeak = StellarCenterPeakSum / StellarCenterPixelCount;
				const double LimbPeak = StellarLimbPeakSum / StellarLimbPixelCount;
				UE_LOG(LogTemp, Display,
					TEXT("[APS.StellarPresentation.Surface] core=%lld rgb=%.2f/%.2f/%.2f peak=%.2f contrast=%.2f whiteClip=%.5f peakClip=%.5f center=%.2f limb=%.2f"),
					StellarCorePixelCount, MeanRed, MeanGreen, MeanBlue, MeanPeak,
					SurfaceContrast, WhiteClipRatio, PeakClipRatio, CenterPeak, LimbPeak);

				if (SurfaceContrast < 4.0)
				{
					OutFailure = FString::Printf(
						TEXT("STAR procedural surface is visually flat (peak sigma %.2f; minimum 4.0); diagnostic screenshot=%s"),
						SurfaceContrast, *ScreenshotPath);
					return false;
				}
				if (!(MeanRed > MeanGreen + 4.0 && MeanGreen > MeanBlue + 4.0
					&& MeanRed > MeanBlue + 24.0))
				{
					OutFailure = FString::Printf(
						TEXT("STAR lost the requested G spectral hue (mean RGB %.2f/%.2f/%.2f); diagnostic screenshot=%s"),
						MeanRed, MeanGreen, MeanBlue, *ScreenshotPath);
					return false;
				}
				if (WhiteClipRatio > 0.005 || PeakClipRatio > 0.02)
				{
					OutFailure = FString::Printf(
						TEXT("STAR is clipping toward a white disc (white %.5f peak %.5f; maxima 0.005/0.02); diagnostic screenshot=%s"),
						WhiteClipRatio, PeakClipRatio, *ScreenshotPath);
					return false;
				}
				if (LimbPeak >= CenterPeak - 2.0 || LimbPeak <= CenterPeak * 0.55)
				{
					OutFailure = FString::Printf(
						TEXT("STAR limb is not soft and dimensional (center %.2f limb %.2f); diagnostic screenshot=%s"),
						CenterPeak, LimbPeak, *ScreenshotPath);
					return false;
				}
			}
			return true;
		}

		static void FinishScreenshotCompilation(const TCHAR* Context)
		{
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
			{
				GShaderCompilingManager->FinishAllCompilation();
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.StellarPresentation] %s screenshot compilation idle"), Context);
		}

		bool ValidatePresentation(UWorld* World, AMainMenuController* Controller,
			const EStellarType ExpectedType)
		{
			AStarSystem* System = FindGeneratedStarSystem(World, Generator.Get());
			if (!System || System->GetStars().Num() != 1 || !IsValid(System->GetStars()[0]))
			{
				return false;
			}
			AStar* Star = System->GetStars()[0];
			if (!IsValid(Star->StarMesh))
			{
				return false;
			}
			UMaterialInstanceDynamic* RuntimeStellarMaterial =
				Cast<UMaterialInstanceDynamic>(Star->StarMesh->GetMaterial(0));
			Test->TestNotNull(TEXT("Generated star owns a runtime stellar MID"),
				RuntimeStellarMaterial);
			if (RuntimeStellarMaterial)
			{
				Test->TestTrue(TEXT("AStar runtime MID handle matches rendered mesh material"),
					Star->StarDynamicMaterial == RuntimeStellarMaterial);
				UMaterial* RuntimeStellarBase = RuntimeStellarMaterial->GetBaseMaterial();
				Test->TestNotNull(TEXT("Generated star runtime MID has a base material"),
					RuntimeStellarBase);
				Test->TestTrue(TEXT("Generated star runtime MID uses exact canonical SUN base"),
					IsValid(RuntimeStellarBase)
					&& RuntimeStellarBase->GetPathName()
						== APSStellarMaterialContract::ActorBaseObjectPath);
				Test->TestFalse(TEXT("Generated star runtime MID never uses WorldGrid"),
					APSStellarMaterialContract::UsesWorldGrid(RuntimeStellarMaterial));

				TArray<FMaterialParameterInfo> ScalarParameters;
				TArray<FGuid> ScalarParameterIds;
				RuntimeStellarMaterial->GetAllScalarParameterInfo(
					ScalarParameters, ScalarParameterIds);
				TArray<FMaterialParameterInfo> VectorParameters;
				TArray<FGuid> VectorParameterIds;
				RuntimeStellarMaterial->GetAllVectorParameterInfo(
					VectorParameters, VectorParameterIds);
				auto HasScalar = [&ScalarParameters](const FName Name)
				{
					return ScalarParameters.ContainsByPredicate(
						[Name](const FMaterialParameterInfo& Info)
						{
							return Info.Name == Name;
						});
				};
				auto HasVector = [&VectorParameters](const FName Name)
				{
					return VectorParameters.ContainsByPredicate(
						[Name](const FMaterialParameterInfo& Info)
						{
							return Info.Name == Name;
						});
				};
				static constexpr const TCHAR* RequiredRuntimeScalars[] = {
					TEXT("Multiplier"), TEXT("SurfaceSeed"), TEXT("SurfaceVariation"),
					TEXT("GranulationStrength"), TEXT("SpotStrength"),
					TEXT("CoronaStrength")
				};
				for (const TCHAR* ParameterName : RequiredRuntimeScalars)
				{
					Test->TestTrue(*FString::Printf(
						TEXT("Runtime stellar MID exposes %s"), ParameterName),
						HasScalar(FName(ParameterName)));
				}
				Test->TestTrue(TEXT("Runtime stellar MID exposes spectral Color"),
					HasVector(FName(TEXT("Color"))));

				const float RuntimeEmission =
					RuntimeStellarMaterial->K2_GetScalarParameterValue(TEXT("Multiplier"));
				const float RuntimeSeed =
					RuntimeStellarMaterial->K2_GetScalarParameterValue(TEXT("SurfaceSeed"));
				const float RuntimeGranulation =
					RuntimeStellarMaterial->K2_GetScalarParameterValue(TEXT("GranulationStrength"));
				const float RuntimeSpots =
					RuntimeStellarMaterial->K2_GetScalarParameterValue(TEXT("SpotStrength"));
				const float RuntimeCorona =
					RuntimeStellarMaterial->K2_GetScalarParameterValue(TEXT("CoronaStrength"));
				const FLinearColor RuntimeColor =
					RuntimeStellarMaterial->K2_GetVectorParameterValue(TEXT("Color"));
				const FLinearColor ExpectedColor = UStarGenerator::GetStarColor(
					Star->SpectralClass, Star->SpectralSubclass);
				Test->TestTrue(TEXT("Runtime stellar emission is finite and positive"),
					FMath::IsFinite(RuntimeEmission) && RuntimeEmission > 0.0f);
				Test->TestTrue(TEXT("Runtime stellar seed remains normalized"),
					FMath::IsFinite(RuntimeSeed) && RuntimeSeed >= 0.0f && RuntimeSeed <= 1.0f);
				Test->TestTrue(TEXT("Runtime stellar surface controls remain active"),
					RuntimeGranulation > 0.0f && RuntimeSpots > 0.0f && RuntimeCorona > 0.0f);
				Test->TestTrue(TEXT("Runtime stellar corona remains inside the compact safe band"),
					RuntimeCorona >= 0.10f && RuntimeCorona <= 0.24f);
				Test->TestTrue(TEXT("Runtime stellar spectral colour reaches the MID"),
					RuntimeColor.Equals(ExpectedColor, 1.0e-3f));
			}
			Test->TestEqual(TEXT("Requested stellar class reaches generated star"),
				static_cast<int32>(Star->StellarClass), static_cast<int32>(ExpectedType));
			if (ExpectedType == EStellarType::HyperGiant)
			{
				HyperGiantPhysicalRadius = Star->StarRadiusKM;
			}
			else if (ExpectedType == EStellarType::SubDwarf)
			{
				SubDwarfPhysicalRadius = Star->StarRadiusKM;
			}

			FVector SystemCenter;
			double SystemRadius = 0.0;
			if (!Generator->GetPreviewFocusSphere(
				EAstroPreviewFocus::HomeSystem, SystemCenter, SystemRadius))
			{
				return false;
			}
			if (BaselineSystemRadius <= UE_SMALL_NUMBER)
			{
				BaselineSystemRadius = SystemRadius;
			}
			UE_LOG(LogTemp, Display,
				TEXT("[APS.StellarPresentation] type=%s systemRadius=%.3e baseline=%.3e ratio=%.4f rootScale=%.3e"),
				*UEnum::GetValueAsString(ExpectedType), SystemRadius,
				BaselineSystemRadius, SystemRadius / BaselineSystemRadius,
				Generator->GetActorTransform().GetScale3D().GetAbsMax());
			Test->TestTrue(TEXT("Stellar type switch keeps SYSTEM framing in bounded preview range"),
				SystemRadius >= BaselineSystemRadius * 0.70
				&& SystemRadius <= BaselineSystemRadius * 1.40);
			Test->TestTrue(TEXT("SYSTEM focus remains on exact generated barycentre"),
				SystemCenter.Equals(System->GetActorLocation(), 1.0));

			FVector BoundaryCenter;
			double BoundaryRadius = 0.0;
			bool bBoundaryVisible = false;
			if (!Generator->GetPreviewGuideShellState(EAstroPreviewFocus::HomeSystem,
				BoundaryCenter, BoundaryRadius, bBoundaryVisible))
			{
				return false;
			}
			Test->TestTrue(TEXT("SYSTEM boundary shell stays visible through type switches"),
				bBoundaryVisible);
			Test->TestTrue(TEXT("SYSTEM boundary shell never drifts from barycentre"),
				BoundaryCenter.Equals(System->GetActorLocation(), 1.0));
			Test->TestTrue(TEXT("SYSTEM boundary shell tracks clamped focus radius"),
				FMath::IsNearlyEqual(BoundaryRadius, SystemRadius,
					FMath::Max(2.0, SystemRadius * 1.0e-5)));

			Star->StarMesh->UpdateBounds();
			FVector PresentedStarCenter = Star->GetActorLocation();
			Generator->GetPreviewPresentationLocation(Star, PresentedStarCenter);
			Test->TestTrue(TEXT("Rendered star mesh follows its presentation centre"),
				Star->StarMesh->Bounds.Origin.Equals(PresentedStarCenter, 1.0));
			const double PresentedStarRadius = Star->StarMesh->Bounds.SphereRadius;
			Test->TestTrue(TEXT("Rendered star remains legible without consuming its system"),
				PresentedStarRadius >= SystemRadius * 0.01
				&& PresentedStarRadius <= SystemRadius * 0.12);

			FVector InfluenceCenter;
			double InfluenceRadius = 0.0;
			bool bInfluenceVisible = false;
			if (!Generator->GetPreviewGuideShellState(EAstroPreviewFocus::HomeStar,
				InfluenceCenter, InfluenceRadius, bInfluenceVisible))
			{
				return false;
			}
			Test->TestTrue(TEXT("STAR influence shell stays visible in SYSTEM"),
				bInfluenceVisible);
			Test->TestTrue(TEXT("STAR influence shell never drifts from rendered star"),
				InfluenceCenter.Equals(Star->StarMesh->Bounds.Origin, 1.0));
			Test->TestTrue(TEXT("STAR influence shell remains 1.5 rendered radii"),
				FMath::IsNearlyEqual(InfluenceRadius, PresentedStarRadius * 1.5,
					FMath::Max(2.0, InfluenceRadius * 1.0e-5)));

			Test->TestTrue(TEXT("Native system safe-zone stays hidden behind preview shell"),
				IsValid(System->StarSystemZone) && !System->StarSystemZone->IsVisible()
				&& System->StarSystemZone->bHiddenInGame);
			Test->TestTrue(TEXT("Native stellar safe-zone stays hidden behind preview shell"),
				IsValid(Star->PlanetarySystemZone) && !Star->PlanetarySystemZone->IsVisible()
				&& Star->PlanetarySystemZone->bHiddenInGame);
			TInlineComponentArray<UPrimitiveComponent*> GeneratorMeshes;
			Generator->GetComponents(GeneratorMeshes);

			// Reproduce the stale map payload that originally rendered filled red spheres,
			// then exercise the pre-BeginPlay migration hook directly. The structural
			// assertions below also prove that construction cleanup leaves both wire guides
			// visible, centred and backed by their line-only procedural sections.
			UStaticMesh* LegacySphere = LoadObject<UStaticMesh>(nullptr,
				TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			UMaterialInterface* LegacyMaterial = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			Test->TestNotNull(TEXT("Legacy guide regression fixture loads Engine sphere"),
				LegacySphere);
			Test->TestNotNull(TEXT("Legacy guide regression fixture loads Engine material"),
				LegacyMaterial);
			int32 PrimedLegacyShellCount = 0;
			for (UPrimitiveComponent* Mesh : GeneratorMeshes)
			{
				if (!IsValid(Mesh))
				{
					continue;
				}
				const bool bLegacyShell = Mesh->GetName().StartsWith(
					TEXT("PreviewStarInfluenceShell"))
					|| Mesh->GetName().StartsWith(TEXT("PreviewSystemBoundaryShell"));
				UStaticMeshComponent* LegacyStaticMesh = bLegacyShell
					? Cast<UStaticMeshComponent>(Mesh) : nullptr;
				if (!LegacyStaticMesh)
				{
					continue;
				}
				LegacyStaticMesh->SetStaticMesh(LegacySphere);
				LegacyStaticMesh->SetMaterial(0, LegacyMaterial);
				LegacyStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				LegacyStaticMesh->SetCollisionResponseToAllChannels(ECR_Block);
				LegacyStaticMesh->SetCastShadow(true);
				LegacyStaticMesh->SetVisibility(true, true);
				LegacyStaticMesh->SetHiddenInGame(false, true);
				++PrimedLegacyShellCount;
			}
			Test->TestTrue(TEXT("Regression fixture primes both serialized legacy guide spheres"),
				PrimedLegacyShellCount >= 2);
			Generator->OnConstruction(Generator->GetActorTransform());

			int32 WireGuideCount = 0;
			int32 VisibleWireGuideCount = 0;
			int32 LegacyShellCount = 0;
			for (UPrimitiveComponent* Mesh : GeneratorMeshes)
			{
				if (!IsValid(Mesh))
				{
					continue;
				}
				const bool bWireGuide = Mesh->GetName().StartsWith(
					TEXT("PreviewStarInfluenceWireGuide"))
					|| Mesh->GetName().StartsWith(TEXT("PreviewSystemBoundaryWireGuide"));
				if (bWireGuide)
				{
					++WireGuideCount;
					VisibleWireGuideCount += Mesh->IsVisible() && !Mesh->bHiddenInGame ? 1 : 0;
					Test->TestTrue(TEXT("Scope boundaries use line-only procedural guides"),
						Mesh->IsA<UProceduralMeshComponent>());
					UProceduralMeshComponent* WireGuide =
						Cast<UProceduralMeshComponent>(Mesh);
					FProcMeshSection* WireSection = WireGuide
						? WireGuide->GetProcMeshSection(0) : nullptr;
					Test->TestNotNull(TEXT("Scope boundary owns one generated wire section"),
						WireSection);
					if (WireSection)
					{
						const int32 ExpectedVertexCount = ExpectedGuideRingCount
							* ExpectedGuideMajorSegments * ExpectedGuideMinorSegments;
						const int32 ExpectedIndexCount = ExpectedVertexCount * 12;
						double MinimumVertexRadius = TNumericLimits<double>::Max();
						double MaximumVertexRadius = 0.0;
						int32 VerticesOutsidePrincipalRingBands = 0;
						for (const FProcMeshVertex& Vertex : WireSection->ProcVertexBuffer)
						{
							const double VertexRadius = Vertex.Position.Size();
							MinimumVertexRadius = FMath::Min(
								MinimumVertexRadius, VertexRadius);
							MaximumVertexRadius = FMath::Max(
								MaximumVertexRadius, VertexRadius);
							const double NearestPrincipalPlane = FMath::Min(
								FMath::Abs(Vertex.Position.X), FMath::Min(
									FMath::Abs(Vertex.Position.Y),
									FMath::Abs(Vertex.Position.Z)));
							VerticesOutsidePrincipalRingBands +=
								NearestPrincipalPlane > 0.20 ? 1 : 0;
						}
						Test->TestTrue(TEXT("Scope boundary wire contains renderable geometry"),
							!WireSection->ProcVertexBuffer.IsEmpty()
							&& !WireSection->ProcIndexBuffer.IsEmpty());
						Test->TestEqual(TEXT("Scope boundary has exactly three thin ring vertex grids"),
							WireSection->ProcVertexBuffer.Num(), ExpectedVertexCount);
						Test->TestEqual(TEXT("Scope boundary has only ring-tube triangles"),
							WireSection->ProcIndexBuffer.Num(), ExpectedIndexCount);
						Test->TestTrue(TEXT("Scope boundary has no filled centre or shell surface"),
							MaximumVertexRadius > UE_SMALL_NUMBER
							&& MinimumVertexRadius > MaximumVertexRadius * 0.995
							&& VerticesOutsidePrincipalRingBands == 0);
						WireGuide->UpdateBounds();
						Test->TestTrue(TEXT("Scope boundary geometry is centred on its world transform"),
							WireGuide->Bounds.Origin.Equals(
								WireGuide->GetComponentLocation(), 1.0));
					}
					UMaterialInterface* GuideMaterial = Mesh->GetMaterial(0);
					Test->TestNotNull(TEXT("Scope boundary owns a guide material"), GuideMaterial);
					UMaterialInstanceDynamic* GuideMid =
						Cast<UMaterialInstanceDynamic>(GuideMaterial);
					Test->TestNotNull(TEXT("Scope boundary owns a runtime guide MID"), GuideMid);
					if (GuideMid)
					{
						UMaterial* GuideBase = GuideMid->GetBaseMaterial();
						Test->TestTrue(TEXT("Scope boundary uses the project preview-guide master"),
							IsValid(GuideBase)
							&& GuideBase->GetPathName() == PreviewGuideMaterialPath);
						if (IsValid(GuideBase))
						{
							Test->TestEqual(TEXT("Scope boundary master remains translucent"),
								GuideBase->GetBlendMode(), BLEND_Translucent);
							Test->TestTrue(TEXT("Scope boundary master remains unlit"),
								GuideBase->GetShadingModels().HasShadingModel(MSM_Unlit));
						}
						const bool bStarGuide = Mesh->GetName().StartsWith(
							TEXT("PreviewStarInfluenceWireGuide"));
						const FLinearColor ExpectedColor = bStarGuide
							? ExpectedStarGuideColor : ExpectedSystemGuideColor;
						const float ExpectedOpacity = bStarGuide
							? ExpectedStarGuideOpacity : ExpectedSystemGuideOpacity;
						Test->TestTrue(TEXT("Scope boundary receives its semantic guide colour"),
							GuideMid->K2_GetVectorParameterValue(TEXT("GuideColor"))
								.Equals(ExpectedColor, 1.0e-4f));
						Test->TestTrue(TEXT("Scope boundary receives its restrained guide opacity"),
							FMath::IsNearlyEqual(
								GuideMid->K2_GetScalarParameterValue(TEXT("GuideOpacity")),
								ExpectedOpacity, 1.0e-4f));
					}
					continue;
				}
				const bool bLegacyShell = Mesh->GetName().StartsWith(
					TEXT("PreviewStarInfluenceShell"))
					|| Mesh->GetName().StartsWith(TEXT("PreviewSystemBoundaryShell"));
				if (bLegacyShell)
				{
					++LegacyShellCount;
					Test->TestTrue(TEXT("Serialized legacy guide sphere stays hidden"),
						!Mesh->IsVisible() && Mesh->bHiddenInGame);
					Test->TestEqual(TEXT("Serialized legacy guide sphere never collides"),
						Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
					UStaticMeshComponent* LegacyStaticMesh =
						Cast<UStaticMeshComponent>(Mesh);
					Test->TestNotNull(TEXT("Serialized legacy guide remains a compatibility component"),
						LegacyStaticMesh);
					if (LegacyStaticMesh)
					{
						Test->TestNull(TEXT("Serialized legacy guide has no renderable sphere mesh"),
							LegacyStaticMesh->GetStaticMesh());
						for (int32 MaterialIndex = 0;
							MaterialIndex < LegacyStaticMesh->GetNumMaterials(); ++MaterialIndex)
						{
							Test->TestNull(TEXT("Serialized legacy guide has no material override"),
								LegacyStaticMesh->GetMaterial(MaterialIndex));
						}
					}
				}
			}
			Test->TestEqual(TEXT("Generator owns exactly two line-only guide components"),
				WireGuideCount, 2);
			Test->TestEqual(TEXT("SYSTEM renders exactly two line-only guides"),
				VisibleWireGuideCount, 2);
			Test->TestTrue(TEXT("Generator retains and neutralizes serialized guide placeholders"),
				LegacyShellCount >= 2);

			if (IsValid(Controller->PlayerCameraManager))
			{
				const double CameraDistance = FVector::Distance(
					Controller->PlayerCameraManager->GetCameraLocation(), SystemCenter);
				Test->TestTrue(TEXT("Type switch camera stays outside and near the system shell"),
					CameraDistance > SystemRadius * 1.05
					&& CameraDistance < SystemRadius * 12.0);
			}
			return true;
		}

		bool Fail(const FString& Message)
		{
			PendingFailure = Message;
			Step = 6;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool FailImmediate(const FString& Message)
		{
			Test->AddError(Message);
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		TWeakObjectPtr<AAstroGenerator> Generator;
		int32 Step{0};
		int32 TypeIndex{0};
		int32 RequestedRevision{0};
		double StartSeconds{0.0};
		double StepStartSeconds{0.0};
		double BaselineSystemRadius{0.0};
		double HyperGiantPhysicalRadius{0.0};
		double SubDwarfPhysicalRadius{0.0};
		FString SystemScreenshotPath;
		FString StarScreenshotPath;
		FString PendingFailure;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSRenderedMainMenuPreviewSmokeTest,
	"APS.Rendered.MainMenu.AstronomicalPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSRenderedMainMenuPreviewSmokeTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for rendered preview smoke"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSMainMenuPreviewSmokeTests::FRenderedMenuPreviewCommand(this));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSRenderedStellarPresentationTest,
	"APS.Rendered.MainMenu.StellarPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSRenderedStellarPresentationTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for stellar presentation scenario"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSMainMenuPreviewSmokeTests::FRenderedStellarPresentationCommand(this));
	return true;
}

#endif

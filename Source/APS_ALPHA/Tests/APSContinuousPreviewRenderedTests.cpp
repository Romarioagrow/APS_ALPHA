#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "APS_ALPHA/UI/MainMenu/SAPSMainMenuRoot.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformStackWalk.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "ProceduralMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineModule.h"
#include "EngineGlobals.h"
#include "RendererInterface.h"
#include "RenderingThread.h"
#include "RenderTimer.h"
#include "Misc/CoreDelegates.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(APSPreview);

namespace APSContinuousPreviewRendered
{
TSharedPtr<SWidget> FindAnnotationOverlay(const TSharedRef<SWidget>& Widget, const FName Match = TEXT("SPreviewSystemOverlay"))
{
	if (Widget->GetType() == Match || Widget->GetTag() == Match) return Widget;
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
		if (TSharedPtr<SWidget> Found = FindAnnotationOverlay(Children->GetChildAt(Index), Match)) return Found;
	return nullptr;
}

FString StellarPhysics(UWorld* World, const AAstroGenerator* Generator)
{
	TArray<FString> Stars;
	for (TActorIterator<AStar> It(World); It; ++It)
	{
		if (!It->IsAttachedTo(Generator)) continue;
		Stars.Add(FString::Printf(TEXT("%s,%d,%d,%d,%.17g,%.17g,%.17g,%d;"),
			*It->AstroName.ToString(), static_cast<int32>(It->StellarClass),
			static_cast<int32>(It->SpectralClass), It->SpectralSubclass,
			It->RadiusKM, It->Mass, It->Luminosity, It->SurfaceTemperature));
	}
	Stars.Sort();
	return FString::Join(Stars, TEXT(""));
}

FString SatellitePhysics(const APlanet* Planet)
{
	FString Result;
	if (!Planet) return Result;
	for (const AMoon* Moon : Planet->Moons)
	{
		if (!IsValid(Moon) || !Moon->GenerationModel) { Result += TEXT("invalid;"); continue; }
		const FMoonModel& Model = *Moon->GenerationModel;
		Result += FString::Printf(TEXT("%d,%.17g,%.17g,%.17g,%.17g,%.17g;"),
			static_cast<int32>(Moon->PlanetType), Moon->RadiusKM, Model.Mass,
			Model.MoonDensity, Model.MoonAtmosphereHeight, Model.OrbitDistance);
	}
	return Result;
}

FString OneStarPhysics(const AStar* Star)
{
	return FString::Printf(TEXT("%s,%d,%d,%d,%.17g,%.17g,%.17g,%d;"),
		*Star->AstroName.ToString(), static_cast<int32>(Star->StellarClass),
		static_cast<int32>(Star->SpectralClass), Star->SpectralSubclass,
		Star->RadiusKM, Star->Mass, Star->Luminosity, Star->SurfaceTemperature);
}

FString SystemStellarPhysics(UWorld* World, const FGuid& SystemId, const int32 ExceptIndex = INDEX_NONE)
{
	for (TActorIterator<AStarSystem> It(World); It; ++It)
	{
		if (It->StableSystemId != SystemId) continue;
		FString Result;
		for (int32 Index = 0; Index < It->GetStars().Num(); ++Index)
		{
			if (Index == ExceptIndex) continue;
			const AStar* Star = It->GetStars()[Index];
			Result += OneStarPhysics(Star);
		}
		return Result;
	}
	return TEXT("missing-system");
}

FString HierarchyPhysics(const AStarSystem* System, const AAstroGenerator* Generator)
{
	FString Result;
	for (const AStar* Star : System->GetStars())
	{
		Result += OneStarPhysics(Star) + Star->GetActorLocation().ToString();
		for (const APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
		{
			Result += FString::Printf(TEXT("%s,%s,%d,%.17g,%.17g,%s,[%s];"),
				*Generator->GetPreviewBodyStableKey(Planet), *Planet->AstroName.ToString(),
				static_cast<int32>(Planet->PlanetType), Planet->RadiusKM, Planet->PlanetData.OrbitRadius,
				*Planet->GetActorLocation().ToString(), *SatellitePhysics(Planet));
			for (const AMoon* Moon : Planet->Moons)
				Result += Generator->GetPreviewBodyStableKey(Moon) + Moon->AstroName.ToString() + Moon->GetActorLocation().ToString();
		}
	}
	return Result;
}

struct FStarFieldSeries
{
	static constexpr int32 SampleCount = 64;
	TArray<double> Energies;
	TArray<uint64> Frames;
	TArray<double> Times;
	TArray<FVector3d> PixelSums;
	FVector3d ColorSum{FVector3d::ZeroVector};
	FIntVector Size{FIntVector::ZeroValue};
};

struct FRouteFixture
{
	int32 Seed{271828};
	EStellarType StellarType{EStellarType::MainSequence};
	ESpectralClass Spectrum{ESpectralClass::G};
	EStarClusterType Formation{EStarClusterType::RingArc};
	FString CaptureFolder{TEXT("ContinuousPreviewFrames")};
	bool bResolvedNeighborProof{false};
	bool bHighCountProof{false};
	bool bHighCountRemote{false};
};

struct FResolvedFrameSample
{
	uint64 Frame{0};
	double Seconds{0.0}, FrameMs{0.0}, GameMs{0.0}, RenderMs{0.0}, RhiMs{0.0}, GpuMs{0.0};
	double Pixels{0.0}, DistanceCm{0.0};
	FIntPoint SceneSize{FIntPoint::ZeroValue};
	int32 ProxyCount{0};
	int32 PoolCount{0}, Allocations{0}, Reuses{0};
	int32 Preparations{0};
	bool bPoolHidden{true};
	bool bCommittedGlobesStable{true};
	bool bSurface{false}, bGeometry{true}, bGlyph{true};
};

// Read the actual opaque-pass rectangle, after the renderer has applied its
// resolution policy. No texture readback or extra render pass is submitted.
struct FAppliedViewportProbe
{
	TAtomic<uint64> PackedSize{0};
	FDelegateHandle Handle;
};

struct FResolvedFramePhase
{
	FString Name;
	double Started{0.0};
	bool bGeometry{false};
	TArray<FResolvedFrameSample> Samples;
	int32 GlobeRenderStateDirties{0};
	int32 GlobeVisibilityChanges{0};
	TMap<TWeakObjectPtr<UProceduralMeshComponent>, uint8> InitialGlobeVisibility;
	TArray<FString> GlobeDirtyStacks;
	int32 InitialPairCapacity{0}, InitialAllocations{0}, InitialReuses{0};
	int32 InitialPreparations{0};
	bool bMeasureFromStart{false};
};

class FRoute final : public IAutomationLatentCommand
{
public:
	explicit FRoute(FAutomationTestBase* InTest, int32 InPreviousCvar, bool bInGiantFixture = false)
		: FRoute(InTest, InPreviousCvar, FRouteFixture{271828,
			bInGiantFixture ? EStellarType::Giant : EStellarType::MainSequence,
			bInGiantFixture ? ESpectralClass::K : ESpectralClass::G, EStarClusterType::RingArc,
			bInGiantFixture ? TEXT("ContinuousGiantFrames") : TEXT("ContinuousPreviewFrames")}) {}
	FRoute(FAutomationTestBase* InTest, int32 InPreviousCvar, FRouteFixture InFixture)
		: Test(InTest), PreviousCvar(InPreviousCvar), Fixture(MoveTemp(InFixture)), Start(FPlatformTime::Seconds()) {}
	~FRoute() override { ClearStarFieldCapture(); ClearResolvedFrameProbe(); }

	bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		AMainMenuController* Controller = World ? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
		UWorldGenerationViewModel* VM = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
		if (Now - Start > 240.0 && Step != 99)
		{
			Test->AddError(TEXT("Continuous rendered route timed out"));
			Step = 99;
			StepStart = Now;
		}
		if (Step == 99)
		{
			ClearStarFieldCapture();
			ClearResolvedFrameProbe();
			if (ResolvedCsvFuture.IsValid() && !ResolvedCsvFuture.IsReady() && Now - StepStart < 30.0) return false;
			if (Generator.IsValid() && !Generator->PreparePreviewForTravel() && Now - StepStart < 30.0) return false;
			if (Generator.IsValid())
			{
				TArray<UProceduralMeshComponent*> Globes;
				Generator->GetComponents(Globes);
				bool bAllGlobesHidden = true;
				for (const UProceduralMeshComponent* Globe : Globes)
					bAllGlobesHidden &= !Globe->IsVisible() && Globe->bHiddenInGame;
				Test->TestTrue(TEXT("Travel preparation explicitly hides all committed globe buffers"), bAllGlobesHidden);
			}
			if (VM) VM->Shutdown();
			IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"))->Set(PreviousCvar, ECVF_SetByCode);
			return true;
		}
		if (!VM || !VM->GeneratedWorld) return false;
		if (Step == 0)
		{
			Test->AddInfo(FString::Printf(TEXT("Full coherence route fixture: seed=%d stellarType=%s spectrum=%s formation=%s captures=%s"),
				Fixture.Seed, *UEnum::GetValueAsString(Fixture.StellarType), *UEnum::GetValueAsString(Fixture.Spectrum),
				*UEnum::GetValueAsString(Fixture.Formation), *Fixture.CaptureFolder));
			UGeneratedWorld* Model = VM->GeneratedWorld;
			Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
			Model->bGenerateFullScaledWorld = true;
			Model->bGenerateHomeSystem = true;
			Model->bStartWithHomePlanet = true;
			Model->GenerationSeed = Fixture.Seed;
			Model->GalaxyType = EGalaxyType::Spiral;
			Model->GalaxyClass = EGalaxyClass::Sc;
			Model->GalaxySize = 17033;
			Model->GalaxyStarCount = 9382900;
			Model->GalaxyStarDensity = 156.8;
			Model->StarClusterSize = EStarClusterSize::Giant;
			Model->StarClusterType = Fixture.Formation;
			Model->StarType = EStarType::TripleStar;
			Model->bRandomHomeStar = false;
			Model->StellarType = Fixture.StellarType;
			Model->SpectralClass = Fixture.Spectrum;
			Model->HomeStarRadiusOverrideSolar = 0.0;
			Model->ClearPreviewStarEditOverrides();
			Model->ClearPreviewSystemEditOverrides();
			Model->PlanetsAmount = 6;
			Model->MoonsAmount = 2;
			Model->StartPlanetIndex = 1;
			Model->PlanetRadius = 6750.0;
			Model->PlanetType = EPlanetType::Ocean;
			Model->PlanetSurfaceSeed = 42042;
			Model->AtmosphereHeight = 140.0;
			Model->AtmosphereOpacity = 12.0;
			if (!Controller->OpenAstronomicalGenerationForAutomation(EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization)) return false;
			Step = 1;
			StepStart = Now;
			return false;
		}
		if (!VM->bPreviewReady) return false;
		if (!Generator.IsValid())
		{
			for (TActorIterator<AAstroGenerator> It(World); It; ++It)
				if (It->ActorHasTag(TEXT("WorldGenerationPreview"))) { Generator = *It; break; }
		}
		if (!Generator.IsValid()) return false;
		if (Step >= 70 && Step <= 79) return UpdateHighCount(World, VM, Now);
		if (Step == 1)
		{
			if (!Generator->IsPreviewGlobeFamilyWarmQueueDrained() || !Generator->GetActivePreviewTerrainProxy()) return false;
			const TSharedPtr<SAPSMainMenuRoot> Root = Controller->GetSlateMenuRootForAutomation();
			const TSharedPtr<SWidget> AnnotationOverlay = Root.IsValid() ? FindAnnotationOverlay(Root.ToSharedRef()) : nullptr;
			if (Test->TestTrue(TEXT("The real menu mounts the annotation overlay"), AnnotationOverlay.IsValid()))
			{
				Test->TestTrue(TEXT("Orbit and label paint is clipped to the central panel"),
					AnnotationOverlay->GetClipping() == EWidgetClipping::ClipToBounds);
				Test->TestTrue(TEXT("Annotation clipping does not intercept camera or body picking"),
					AnnotationOverlay->GetVisibility() == EVisibility::HitTestInvisible);
				for (const FName Tag : {FName(TEXT("PreviewStatusHeading")), FName(TEXT("PreviewStatusFooter"))})
				{
					const TSharedPtr<SWidget> Status = FindAnnotationOverlay(Root.ToSharedRef(), Tag);
					if (!Test->TestTrue(TEXT("Preview status text has its own layout row"), Status.IsValid())) continue;
					const FSlateRect AnnotationBounds = AnnotationOverlay->GetCachedGeometry().GetLayoutBoundingRect();
					const FSlateRect StatusBounds = Status->GetCachedGeometry().GetLayoutBoundingRect();
					Test->TestTrue(TEXT("Annotation clipping area cannot overlap preview status text"),
						!FSlateRect::DoRectanglesIntersect(AnnotationBounds, StatusBounds));
				}
			}
			Test->TestTrue(TEXT("Normal generation page uses the common observer"), Generator->UsesContinuousPreviewFrame());
			Test->TestEqual(TEXT("Initial menu presentation does not prepare speculative stellar components"),
				Generator->GetContinuousPreviewResolvedStarPreparationCount(), 0);
			int32 CatalogStarCount = 0;
			// The canonical population is larger than the existing bounded render
			// sample. Check every star in that sample, not absent unsampled systems.
			for (TActorIterator<AStarCluster> It(World); It; ++It)
				if (It->IsAttachedTo(Generator.Get()))
					for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
						CatalogStarCount += Record.SystemModel.AmountOfStars;
			Test->TestTrue(TEXT("The rendered cluster sample is populated"), CatalogStarCount > 0);
			Test->TestEqual(TEXT("Every primary and companion in the rendered sample exists before any system visit"),
				Generator->GetContinuousPreviewClusterPoints().Num(), CatalogStarCount);
			Test->TestEqual(TEXT("The menu's rendered-star counter includes pre-existing companions"),
				Generator->GetPreviewClusterRenderedStarCount(), CatalogStarCount);
			Test->TestEqual(TEXT("Complete background does not pre-spawn remote actor hierarchies"), Generator->GetContinuousPreviewResidentSystemCount(), 0);
			InitialPlanet = Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor());
			const AStar* FixturePrimary = Generator->GetPreviewHomeSystem()->MainStar;
			Test->TestEqual(TEXT("The route actually materializes its requested stellar type"), FixturePrimary->StellarClass, Fixture.StellarType);
			Test->TestEqual(TEXT("The route actually materializes its requested spectrum"), FixturePrimary->SpectralClass, Fixture.Spectrum);
			Test->TestTrue(TEXT("The fixture has a finite positive physical stellar radius"),
				FMath::IsFinite(FixturePrimary->RadiusKM) && FixturePrimary->RadiusKM > 0.0);
			InitialPlanetKey = Generator->GetPreviewBodyStableKey(InitialPlanet.Get());
			InitialPlanetType = InitialPlanet->PlanetType;
			InitialProfileSignature = Generator->GetPreviewGlobeProfileSignature();
			InitialSatellitePhysics = SatellitePhysics(Cast<APlanet>(InitialPlanet.Get()));
			InitialStellarPhysics = StellarPhysics(World, Generator.Get());
			Test->AddInfo(TEXT("Initial stellar physics: ") + InitialStellarPhysics);
			Test->AddInfo(TEXT("Initial selected satellite physics: ") + InitialSatellitePhysics);
			InitialIdentityHash = Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash;
			for (TActorIterator<APlanetaryBody> It(World); It; ++It)
			{
				if (!It->IsAttachedTo(Generator.Get())) continue;
				const FString Address = Generator->GetPreviewBodyStableKey(*It);
				InitialNames.Add(Address, It->AstroName);
				InitialRotations.Add(Address, It->GetActorQuat());
			}
			FVector PlanetCenter;
			FVector2D PlanetScreen;
			if (Test->TestTrue(TEXT("Selected globe has a screen-space center"),
				Generator->GetPreviewPresentationLocation(InitialPlanet.Get(), PlanetCenter)
				&& Controller->ProjectWorldLocationToScreen(PlanetCenter, PlanetScreen, true)))
			{
				Test->TestTrue(TEXT("The normal scene pick resolves the visible globe"),
					VM->FocusPreviewAtScreenPosition(PlanetScreen));
				Test->TestEqual(TEXT("Visible-globe picking selects its physical body, not an invisible collision zone"),
					Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(InitialPlanet.Get()));
			}
			Capture(TEXT("00-Planet-initial"));
			if (Fixture.bHighCountProof)
			{
				BeginHighCount(World, VM, Now);
				return false;
			}
			if (Fixture.bResolvedNeighborProof)
			{
				// The authored HyperGiant home is far outside the dense cluster. Find
				// a real small host star near a resolved neighbor, then navigate there.
				// No synthetic camera coordinates or catalog size edits are involved.
				double BestScore = TNumericLimits<double>::Max();
				int32 HostInstance = INDEX_NONE;
				const auto& Points = Generator->GetContinuousPreviewClusterPoints();
				for (const FAPSContinuousPreviewPoint& Host : Points)
				{
					if (Host.MaterializedStar.IsValid() || Host.SystemStarIndex != 0) continue;
					for (const FAPSContinuousPreviewPoint& Neighbor : Points)
					{
						if (Neighbor.MaterializedStar.IsValid() || Neighbor.StableId == Host.StableId) continue;
						const double Distance = (Neighbor.CenterCm - Host.CenterCm).Size();
						if (Distance <= 0.0 || Host.RadiusCm / Distance > 0.0001) continue;
						const double Pixels = Neighbor.RadiusCm / (Distance * GetPixelTangent(Controller));
						const double Score = FMath::Abs(Pixels - 3.0);
						if (Pixels >= 1.5 && Pixels <= 6.0 && Score < BestScore)
						{ BestScore = Score; HostInstance = Host.SourceInstanceIndex; }
					}
				}
				if (!Test->TestTrue(TEXT("The unchanged catalog has an observing system near a resolved unvisited neighbor"),
					HostInstance != INDEX_NONE))
				{ Step = 99; StepStart = Now; return false; }
				ResolvedInitialHostInstance = HostInstance;
				EnsureResolvedFrameProbe();
				ResolvedHostPreparedFrame = GFrameCounter;
				Step = 47;
				StepStart = Now;
				return false;
			}
			Step = 2;
			BeginScope(VM, Now);
			return false;
		}
		if (Step == 47)
		{
			// Catalog-pair search, PNG capture and profiler initialization belong to
			// the harness, not to a user's focus action. Leave two completed frames
			// between that setup and dispatch so last-frame counters cannot include it.
			if (GFrameCounter <= ResolvedHostPreparedFrame + 2 || AppliedViewportProbe->PackedSize.Load() == 0) return false;
			BeginResolvedFramePhase(TEXT("00-First-Host-Arrival"), false, true);
			const double FocusStarted = FPlatformTime::Seconds();
			const bool bFocused = VM->FocusPreviewClusterSystem(ResolvedInitialHostInstance);
			FirstHostDispatchMs = (FPlatformTime::Seconds() - FocusStarted) * 1000.0;
			FirstHostDispatchGlobeDirties = ResolvedFramePhases.Last().GlobeRenderStateDirties;
			if (!Test->TestTrue(TEXT("First host arrival uses the normal focus command"), bFocused))
			{ Step = 99; StepStart = Now; return false; }
			Step = 49;
			StepStart = Now;
			return false;
		}
		if (Step == 49)
		{
			if (Now - StepStart < 2.0) return false;
			FinishResolvedFramePhase();
			BeginResolvedFramePhase(TEXT("01-First-Host-Star-Focus"), false, true);
			AStarSystem* Host = Generator->GetContinuousPreviewActiveSystem();
			ResolvedReturnBody = Host ? Host->MainStar : nullptr;
			if (!Test->TestTrue(TEXT("Resolved comparison uses the actual host star as its return address"),
				ResolvedReturnBody.IsValid() && VM->FocusPreviewBody(ResolvedReturnBody.Get())))
			{ Step = 99; StepStart = Now; return false; }
			Step = 48;
			StepStart = Now;
			return false;
		}
		if (Step == 48)
		{
			if (Now - StepStart < 2.0) return false;
			FinishResolvedFramePhase();
			BeginResolvedFramePhase(TEXT("02-First-Neighbor-Orientation"), false, true);
			Generator->ZoomPreviewCamera(-4.0f);
			const FVector Observer = Generator->GetContinuousPreviewFrame().ObserverCm;
			for (const FAPSContinuousPreviewPoint& Point : Generator->GetContinuousPreviewClusterPoints())
			{
				const double PixelRadius = Point.RadiusCm / (FMath::Max((Point.CenterCm - Observer).Size(), 1.0) * GetPixelTangent(Controller));
				if (!Point.MaterializedStar.IsValid() && PixelRadius >= 1.0 && PixelRadius <= 8.0) ResolvedCandidates.Add(Point);
			}
			ResolvedCandidates.Sort([Observer](const auto& A, const auto& B)
				{ return A.RadiusCm / (A.CenterCm - Observer).Size() > B.RadiusCm / (B.CenterCm - Observer).Size(); });
			Test->AddInfo(FString::Printf(TEXT("Resolved first-visit candidates in the unchanged catalog: %d"), ResolvedCandidates.Num()));
			Step = 50;
			StepStart = Now;
			return false;
		}
		if (Step == 50)
		{
			if (Now - StepStart < 0.5) return false;
			if (!Test->TestTrue(TEXT("A resolved unvisited neighbor can be framed without altering the catalog"),
				ResolvedCandidates.IsValidIndex(ResolvedAttempt / 2) && ResolvedAttempt < 64))
			{ Step = 99; StepStart = Now; return false; }
			const FAPSContinuousPreviewPoint& Candidate = ResolvedCandidates[ResolvedAttempt / 2];
			if (!bResolvedViewOriented)
			{
				// Ordinary orbit input keeps the host centered. Put the neighbor
				// beside its limb rather than hiding it behind the selected body.
				const FVector Direction = (Candidate.CenterCm - Generator->GetContinuousPreviewOrbit().CenterCm).GetSafeNormal();
				FRotator Desired = (-Direction).Rotation();
				Desired.Yaw += (ResolvedAttempt % 2 == 0 ? 14.0 : -14.0);
				const FRotator Current = Generator->GetContinuousPreviewOrbit().Outward.Rotation();
				Generator->BeginPreviewCameraOrbit();
				Generator->OrbitPreviewCamera(FVector2D(
					FRotator::NormalizeAxis(Current.Yaw - Desired.Yaw) / 0.18,
					(Desired.Pitch - Current.Pitch) / 0.14));
				Generator->EndPreviewCameraOrbit();
				bResolvedViewOriented = true;
				StepStart = Now;
				return false;
			}
			int32 Width = 0, Height = 0;
			Controller->GetViewportSize(Width, Height);
			FVector RenderCenter;
			FVector2D Screen;
			bool bUsable = Generator->GetContinuousPreviewFrame().ProjectPosition(Candidate.CenterCm, RenderCenter)
				&& Controller->ProjectWorldLocationToScreen(RenderCenter, Screen, true)
				&& Screen.X > Width * 0.26 && Screen.X < Width * 0.74 && Screen.Y > Height * 0.15 && Screen.Y < Height * 0.84;
			const FVector Observer = Generator->GetContinuousPreviewFrame().ObserverCm;
			const FVector ToStar = Candidate.CenterCm - Observer;
			const FVector Ray = ToStar.GetSafeNormal();
			for (TActorIterator<AActor> It(World); bUsable && It; ++It)
			{
				if (!It->IsAttachedTo(Generator.Get())) continue;
				const AStar* Star = Cast<AStar>(*It);
				const APlanetaryBody* Body = Cast<APlanetaryBody>(*It);
				const double RadiusCm = (Star ? Star->RadiusKM : (Body ? Body->RadiusKM : 0.0)) * 1.0e5 * 1.04;
				if (RadiusCm <= 0.0) continue;
				const FVector ToBody = Generator->GetContinuousPreviewPhysicalPosition(*It) - Observer;
				const double Along = FVector::DotProduct(ToBody, Ray);
				if (Along > 0.0 && Along < ToStar.Size() && (ToBody - Ray * Along).SizeSquared() < FMath::Square(RadiusCm)) bUsable = false;
			}
			for (const FAPSContinuousPreviewPoint& Point : Generator->GetContinuousPreviewClusterPoints())
			{
				if (!bUsable || Point.SourceInstanceIndex == Candidate.SourceInstanceIndex) continue;
				FVector OtherCenter;
				FVector2D OtherScreen;
				if (Generator->GetContinuousPreviewFrame().ProjectPosition(Point.CenterCm, OtherCenter)
					&& Controller->ProjectWorldLocationToScreen(OtherCenter, OtherScreen, true)
					&& (OtherScreen - Screen).SizeSquared() < FMath::Square(22.0)) bUsable = false;
			}
			if (!bUsable)
			{
				++ResolvedAttempt;
				bResolvedViewOriented = false;
				return false;
			}
			ResolvedNeighbor = Candidate;
			FirstVisitScreen = Screen;
			FirstVisitOrbit = Generator->GetContinuousPreviewOrbit();
			GetClusterView()->GetInstanceTransform(Candidate.SourceInstanceIndex, ResolvedPointBefore, true);
			UStaticMeshComponent* Surface = Generator->GetContinuousPreviewResolvedStarMesh(Candidate.SourceInstanceIndex);
			if (Test->TestNotNull(TEXT("Unvisited resolved neighbor already has a physical photosphere"), Surface))
			{
				ResolvedSurfaceBefore = Surface->GetComponentTransform();
				ResolvedSurfaceAsset = Surface->GetStaticMesh();
				Test->TestTrue(TEXT("Unvisited photosphere uses physical angular radius, not the point support size"),
					FMath::IsNearlyEqual(Surface->Bounds.BoxExtent.GetMax() / Surface->Bounds.Origin.Size(),
						Candidate.RadiusCm / ToStar.Size(), 1.0e-7));
				if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Surface->GetMaterial(0)))
				{
					for (const auto& Parameter : Material->ScalarParameterValues)
						ResolvedSurfaceScalars.Add(Parameter.ParameterInfo.Name, Parameter.ParameterValue);
					for (const auto& Parameter : Material->VectorParameterValues)
						ResolvedSurfaceColors.Add(Parameter.ParameterInfo.Name, Parameter.ParameterValue);
				}
				Test->TestTrue(TEXT("Resolved proxy has the complete canonical surface parameters"), ResolvedSurfaceScalars.Num() >= 7);
				UStaticMeshComponent* Corona = nullptr;
				for (USceneComponent* Child : Surface->GetAttachChildren())
					if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child)) { Corona = Mesh; break; }
				if (Test->TestNotNull(TEXT("Unvisited resolved surface owns its canonical corona"), Corona))
				{
					ResolvedCoronaTransform = Corona->GetRelativeTransform();
					ResolvedCoronaAsset = Corona->GetStaticMesh();
					bResolvedCoronaVisible = Corona->IsVisible() && !Corona->bHiddenInGame;
					if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Corona->GetMaterial(0)))
					{
						ResolvedCoronaBase = Material->Parent;
						for (const auto& Parameter : Material->ScalarParameterValues)
							ResolvedCoronaScalars.Add(Parameter.ParameterInfo.Name, Parameter.ParameterValue);
						for (const auto& Parameter : Material->VectorParameterValues)
							ResolvedCoronaColors.Add(Parameter.ParameterInfo.Name, Parameter.ParameterValue);
					}
					Test->TestTrue(TEXT("Resolved corona has its complete canonical parameters"), ResolvedCoronaScalars.Num() >= 5);
				}
			}
			for (TActorIterator<AStarCluster> It(World); It; ++It)
				if (It->IsAttachedTo(Generator.Get()))
					for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
						if (Record.StableId == Candidate.StableId) RemoteInstance = Record.InstanceIndex;
			Test->AddInfo(FString::Printf(TEXT("Resolved first-visit target: system=%s star=%d radiusCm=%.17g distanceCm=%.17g radiusPixels=%.9g patch=(%.3f,%.3f) orientationAttempts=%d"),
				*Candidate.StableId.ToString(), Candidate.SystemStarIndex, Candidate.RadiusCm, ToStar.Size(),
				Candidate.RadiusCm / (ToStar.Size() * GetPixelTangent(Controller)), Screen.X, Screen.Y, ResolvedAttempt + 1));
			FinishResolvedFramePhase();
			BeginResolvedFramePhase(TEXT("04-Dense-Unvisited-Warm"), false);
			Step = 56;
			StepStart = Now;
			return false;
		}
		if (Step == 56)
		{
			if (Now - StepStart < 6.0) return false;
			FinishResolvedFramePhase();
			BeginResolvedFramePhase(TEXT("05-Resolved-Departure"), true);
			VM->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
			Step = 57;
			StepStart = Now;
			return false;
		}
		if (Step == 57)
		{
			if (Now - StepStart < 2.0) return false;
			FinishResolvedFramePhase();
			Test->TestNull(TEXT("Unvisited resolved surface is released when it becomes subpixel at Galaxy"),
				Generator->GetContinuousPreviewResolvedStarMesh(ResolvedNeighbor.SourceInstanceIndex));
			BeginResolvedFramePhase(TEXT("06-Resolved-Arrival"), true);
			Test->TestTrue(TEXT("Return from Galaxy uses the same real observing star"), VM->FocusPreviewBody(ResolvedReturnBody.Get()));
			Step = 58;
			StepStart = Now;
			return false;
		}
		if (Step == 58)
		{
			if (Now - StepStart < 2.0) return false;
			FinishResolvedFramePhase();
			Generator->ZoomPreviewCamera(-4.0f);
			Test->TestEqual(TEXT("Camera returns to the original observing position after angular LOD retirement"),
				Generator->GetContinuousPreviewOrbit().ObserverCm(), FirstVisitOrbit.ObserverCm());
			Test->TestNotNull(TEXT("Unvisited star recreates its resolved surface before any visit"),
				Generator->GetContinuousPreviewResolvedStarMesh(ResolvedNeighbor.SourceInstanceIndex));
			Capture(TEXT("06-Resolved-Returned"));
			BeginResolvedFramePhase(TEXT("07-Dense-Recreated-Warm"), false);
			Step = 59;
			StepStart = Now;
			return false;
		}
		if (Step == 59)
		{
			if (Now - StepStart < 6.0) return false;
			FinishResolvedFramePhase();
			ReportResolvedFramePhases();
			Step = 60;
			return false;
		}
		if (Step == 60)
		{
			if (ResolvedCsvFuture.IsValid() && !ResolvedCsvFuture.IsReady()) return false;
			if (ResolvedCsvFuture.IsValid())
			{
				const FString Filename = ResolvedCsvFuture.Get();
				Test->TestTrue(TEXT("Per-stage CPU profile is saved before photometric captures"), IFileManager::Get().FileExists(*Filename));
				Test->AddInfo(TEXT("Resolved CPU profile: ") + Filename);
			}
			Step = 51;
			return false;
		}
		if (Step == 51)
		{
			if (!CaptureStarField(TEXT("01-Resolved-Before"), FirstVisitPixels, FirstVisitSize, FirstVisitSeries)) return false;
			Step = 52;
			return false;
		}
		if (Step == 52)
		{
			TArray<FColor> Pixels;
			FIntVector Size;
			FStarFieldSeries Series;
			if (!CaptureStarField(TEXT("02-Resolved-Unvisited-Control"), Pixels, Size, Series)) return false;
			CompareStarFieldSeries(TEXT("resolved unvisited control"), FirstVisitSeries, Series);
			FirstVisitSeries = MoveTemp(Series);
			FirstVisitPixels = MoveTemp(Pixels);
			FirstVisitSize = Size;
			Test->TestTrue(TEXT("First visit opens the exact previously unvisited neighbor system"),
				RemoteInstance != INDEX_NONE && VM->FocusPreviewClusterSystem(RemoteInstance));
			Step = 53;
			StepStart = Now;
			return false;
		}
		if (Step == 53)
		{
			if (Now - StepStart < 2.0) return false;
			Test->TestTrue(TEXT("Return uses the same originally inspected host"), VM->FocusPreviewBody(ResolvedReturnBody.Get()));
			Step = 54;
			StepStart = Now;
			return false;
		}
		if (Step == 54)
		{
			if (Now - StepStart < 2.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			Generator->ZoomPreviewCamera(-4.0f);
			Test->TestEqual(TEXT("Resolved comparison returns to the exact original observer"),
				Generator->GetContinuousPreviewOrbit().ObserverCm(), FirstVisitOrbit.ObserverCm());
			const FAPSContinuousPreviewPoint* After = Generator->GetContinuousPreviewClusterPoints().FindByPredicate(
				[this](const auto& Point) { return Point.StableId == ResolvedNeighbor.StableId && Point.SystemStarIndex == ResolvedNeighbor.SystemStarIndex; });
			if (!Test->TestNotNull(TEXT("Resolved neighbor keeps the same catalog address"), After)) { Step = 99; StepStart = Now; return false; }
			Test->TestTrue(TEXT("The same resolved neighbor is now physically materialized"), After->MaterializedStar.IsValid());
			if (After->MaterializedStar.IsValid())
			{
				Test->TestEqual(TEXT("Resolved neighbor physical radius is unchanged by visiting"), After->MaterializedStar->RadiusKM * 1.0e5, ResolvedNeighbor.RadiusCm);
				Test->TestTrue(TEXT("Resolved neighbor physical center is unchanged by visiting"),
					Generator->GetContinuousPreviewPhysicalPosition(After->MaterializedStar.Get()).Equals(ResolvedNeighbor.CenterCm, 1.0));
				UStaticMeshComponent* Surface = After->MaterializedStar->StarMesh;
				Test->TestTrue(TEXT("First visit keeps the same photosphere mesh asset"), Surface->GetStaticMesh() == ResolvedSurfaceAsset.Get());
				Test->TestTrue(TEXT("First visit keeps the actual photosphere's presentation transform"),
					Surface->GetComponentTransform().Equals(ResolvedSurfaceBefore, 1.0));
				UMaterialInterface* Material = Surface->GetMaterial(0);
				for (const auto& Parameter : ResolvedSurfaceScalars)
				{
					float Value = 0.0f;
					Test->TestTrue(TEXT("First visit keeps the exact surface scalar ") + Parameter.Key.ToString(),
						Material && Material->GetScalarParameterValue(Parameter.Key, Value) && Value == Parameter.Value);
				}
				for (const auto& Parameter : ResolvedSurfaceColors)
				{
					FLinearColor Value;
					Test->TestTrue(TEXT("First visit keeps the exact surface color ") + Parameter.Key.ToString(),
						Material && Material->GetVectorParameterValue(Parameter.Key, Value) && Value == Parameter.Value);
				}
				UStaticMeshComponent* Corona = After->MaterializedStar->CoronaMesh;
				if (Test->TestNotNull(TEXT("Visited star owns its canonical corona"), Corona))
				{
					Test->TestTrue(TEXT("First visit keeps corona mesh, shell transform and visibility"),
						Corona->GetStaticMesh() == ResolvedCoronaAsset.Get()
						&& Corona->GetRelativeTransform().Equals(ResolvedCoronaTransform, 1.0e-6)
						&& (Corona->IsVisible() && !Corona->bHiddenInGame) == bResolvedCoronaVisible);
					UMaterialInstanceDynamic* CoronaMaterial = Cast<UMaterialInstanceDynamic>(Corona->GetMaterial(0));
					Test->TestTrue(TEXT("First visit keeps the same canonical corona base"),
						CoronaMaterial && CoronaMaterial->Parent == ResolvedCoronaBase.Get());
					for (const auto& Parameter : ResolvedCoronaScalars)
					{
						float Value = 0.0f;
						Test->TestTrue(TEXT("First visit keeps the exact corona scalar ") + Parameter.Key.ToString(),
							CoronaMaterial && CoronaMaterial->GetScalarParameterValue(Parameter.Key, Value) && Value == Parameter.Value);
					}
					for (const auto& Parameter : ResolvedCoronaColors)
					{
						FLinearColor Value;
						Test->TestTrue(TEXT("First visit keeps the exact corona color ") + Parameter.Key.ToString(),
							CoronaMaterial && CoronaMaterial->GetVectorParameterValue(Parameter.Key, Value) && Value == Parameter.Value);
					}
				}
			}
			FTransform AfterTransform;
			GetClusterView()->GetInstanceTransform(After->SourceInstanceIndex, AfterTransform, true);
			Test->TestTrue(TEXT("Resolved point-to-surface handoff cannot depend on whether its system was visited"), AfterTransform.Equals(ResolvedPointBefore, 1.0));
			CheckGeometry(World);
			Step = 55;
			return false;
		}
		if (Step == 55)
		{
			TArray<FColor> Pixels;
			FIntVector Size;
			FStarFieldSeries Series;
			if (!CaptureStarField(TEXT("03-Resolved-After-Visit"), Pixels, Size, Series)) return false;
			CompareStarFieldPatch(TEXT("resolved after first visit"), Pixels, Size);
			CompareStarFieldSeries(TEXT("resolved after first visit"), FirstVisitSeries, Series);
			Step = 99;
			StepStart = Now;
			return false;
		}
		if (Step == 2)
		{
			CheckGeometry(World);
			if (!bCapturedMidpoint && Now - StepStart > 0.35)
			{
				Capture(FString::Printf(TEXT("%02d-transition"), ScopeIndex + 1));
				bCapturedMidpoint = true;
			}
			if (Now - StepStart < 2.0) return false;
			Capture(FString::Printf(TEXT("%02d-%s-settled"), ScopeIndex + 1,
				*UEnum::GetValueAsString(Scopes[ScopeIndex])));
			Test->TestEqual(TEXT("Scope navigation leaves the canonical dataset unchanged"),
				Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash, InitialIdentityHash);
			const FVector ObserverBeforeRepeat = Generator->GetContinuousPreviewOrbit().ObserverCm();
			VM->SetPreviewFocus(Scopes[ScopeIndex]);
			Test->TestTrue(TEXT("Repeated scope click retains the displayed observer"),
				Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(ObserverBeforeRepeat, 0.0));
			if (++ScopeIndex < UE_ARRAY_COUNT(Scopes)) { BeginScope(VM, Now); return false; }
			Test->TestEqual(TEXT("Round trip returns to the same planet actor"),
				Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(InitialPlanet.Get()));
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			TArray<FAPSPreviewBodyEntry> Bodies;
			VM->GetPreviewBodyEntries(Bodies);
			APlanet* OtherStarPlanet = nullptr;
			const APlanet* OriginalPlanet = Cast<APlanet>(InitialPlanet.Get());
			for (const FAPSPreviewBodyEntry& Entry : Bodies)
			{
				APlanet* Planet = Cast<APlanet>(Entry.Actor.Get());
				if (Planet && OriginalPlanet && Planet->ParentStar != OriginalPlanet->ParentStar)
				{ OtherStarPlanet = Planet; break; }
			}
			if (Test->TestNotNull(TEXT("Fixture has a planet belonging to another physical star"), OtherStarPlanet))
			{
				VM->FocusPreviewBody(OtherStarPlanet);
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeStar);
				Test->TestEqual(TEXT("Star button follows the selected planet's actual parent"),
					Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(OtherStarPlanet->ParentStar));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
				Test->TestEqual(TEXT("Planet button returns to the selected companion-star planet"),
					Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(OtherStarPlanet));
			}
			VM->FocusPreviewBody(InitialPlanet.Get());
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			Step = 5;
			StepStart = Now;
			return false;
		}
		if (Step == 5)
		{
			if (Now - StepStart < 0.2) return false;
			const FVector BeforeReverse = Generator->GetContinuousPreviewOrbit().ObserverCm();
			VM->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
			Test->TestTrue(TEXT("Interrupting an active focus move preserves the exact current observer"),
				Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(BeforeReverse, 0.0));
			Step = 6;
			StepStart = Now;
			return false;
		}
		if (Step == 6)
		{
			if (Now - StepStart < 0.2) return false;
			const FVector BeforeGrab = Generator->GetContinuousPreviewOrbit().ObserverCm();
			VM->BeginPreviewOrbit();
			Test->TestTrue(TEXT("Grabbing an active camera move does not jump to its destination"),
				Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(BeforeGrab, 0.0));
			VM->OrbitPreview(FVector2D(15.0, -7.0));
			VM->EndPreviewOrbit();
			const double BeforeZoom = Generator->GetContinuousPreviewOrbit().DistanceCm;
			VM->ZoomPreview(0.25f);
			Test->TestTrue(TEXT("Interrupted zoom changes only by the requested wheel factor"),
				FMath::IsNearlyEqual(Generator->GetContinuousPreviewOrbit().DistanceCm / BeforeZoom,
					FMath::Pow(0.82, 0.25), 1.0e-9));
			Capture(TEXT("09-interrupted-orbit-zoom"));
			AfterManualOrbitPreparations = Generator->GetContinuousPreviewResolvedStarPreparationCount();
			Step = 61;
			StepStart = Now;
			return false;
		}
		if (Step == 61)
		{
			if (Now - StepStart < 0.2) return false;
			Test->TestEqual(TEXT("Cancelled camera flight stops speculative stellar preparation across subsequent frames"),
				Generator->GetContinuousPreviewResolvedStarPreparationCount(), AfterManualOrbitPreparations);
			Test->TestTrue(TEXT("Interrupted flight leaves all unused stellar components hidden"), Generator->IsContinuousResolvedStarPoolHidden());
			VM->FocusPreviewBody(InitialPlanet.Get());
			Step = 7;
			StepStart = Now;
			return false;
		}
		if (Step == 7)
		{
			if (Now - StepStart < 2.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			VM->SetPlanetsAmount(9);
			Step = 3;
			StepStart = Now;
			return false;
		}
		if (Step == 3 || Step == 4)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			CheckGeometry(World);
			Capture(Step == 3 ? TEXT("09-System-nine-planets") : TEXT("10-Planet-three-planets"));
			Test->TestEqual(TEXT("Changing planet count retains every star's physical identity and semantic name"),
				StellarPhysics(World, Generator.Get()), InitialStellarPhysics);
			for (TActorIterator<APlanetaryBody> It(World); It; ++It)
			{
				if (!It->IsAttachedTo(Generator.Get())) continue;
				const FString Address = Generator->GetPreviewBodyStableKey(*It);
				if (const FName* Name = InitialNames.Find(Address))
					Test->TestEqual(TEXT("Surviving body retains its semantic name: ") + Address, It->AstroName, *Name);
				if (const FQuat* Rotation = InitialRotations.Find(Address))
					Test->TestTrue(TEXT("Surviving body retains its orientation: ") + Address,
						It->GetActorQuat().Equals(*Rotation, 1.0e-6));
			}
			if (Step == 3)
			{
				VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
				APlanetaryBody* RetainedBody = Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor());
				Test->TestEqual(TEXT("Structural edit in System retains the selected planet's stable address"),
					Generator->GetPreviewBodyStableKey(RetainedBody), InitialPlanetKey);
				if (RetainedBody) Test->TestEqual(TEXT("Structural edit retains the selected planet's surface type"),
					RetainedBody->PlanetType, InitialPlanetType);
				VM->SetPlanetsAmount(3);
				Step = 4;
				StepStart = Now;
				return false;
			}
			APlanetaryBody* RetainedBody = Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor());
			Test->TestEqual(TEXT("Decreasing planet count retains surviving selected planet address"),
				Generator->GetPreviewBodyStableKey(RetainedBody), InitialPlanetKey);
			if (RetainedBody) Test->TestEqual(TEXT("Decreasing count retains surviving selected planet type"),
				RetainedBody->PlanetType, InitialPlanetType);
			Test->TestEqual(TEXT("Unchanged selected planet rebuild resolves the same complete surface profile"),
				Generator->GetPreviewGlobeProfileSignature(), InitialProfileSignature);
			Test->TestEqual(TEXT("Unchanged selected planet retains its satellites' physical models through count edits"),
				SatellitePhysics(Cast<APlanet>(RetainedBody)), InitialSatellitePhysics);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			TArray<FAPSPreviewBodyEntry> RemainingBodies;
			VM->GetPreviewBodyEntries(RemainingBodies);
			APlanet* RemovalTarget = nullptr;
			for (const FAPSPreviewBodyEntry& Entry : RemainingBodies)
				if (APlanet* Planet = Cast<APlanet>(Entry.Actor.Get());
					Planet && Generator->GetPreviewBodyStableKey(Planet) == TEXT("SYS0/S0/P2")) RemovalTarget = Planet;
			if (Test->TestNotNull(TEXT("Removal fixture has a third planet"), RemovalTarget))
			{
				VM->FocusPreviewBody(RemovalTarget);
				RemovedPlanet = RemovalTarget;
				VM->SetPlanetsAmount(1);
			}
			Step = 8;
			StepStart = Now;
			return false;
		}
		if (Step == 8)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			APlanetaryBody* Fallback = Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor());
			Test->TestFalse(TEXT("Removed selected planet actor no longer exists"), RemovedPlanet.IsValid());
			Test->TestEqual(TEXT("Removing the selected planet resolves the surviving home planet"),
				Generator->GetPreviewBodyStableKey(Fallback), FString(TEXT("SYS0/S0/P0")));
			Test->TestEqual(TEXT("Editor selection and displayed fallback identify the same body"),
				VM->GetSelectedPreviewBody(), static_cast<AActor*>(Fallback));
			CheckGeometry(World);
			Capture(TEXT("11-Removed-selection-fallback"));
			Step = 9;
			StepStart = FPlatformTime::Seconds();
			return false;
		}
		if (Step == 9)
		{
			// Exclude screenshot/readback and rebuild stalls from this explicitly warm
			// sample. Transition and structural-edit latency require separate evidence.
			if (Now - StepStart < 1.0) return false;
			WarmFrameMs.Add(FApp::GetDeltaTime() * 1000.0);
			if (Now - StepStart < 9.0) return false;
			WarmFrameMs.Sort();
			double Sum = 0.0;
			for (const double Milliseconds : WarmFrameMs) Sum += Milliseconds;
			const double Mean = Sum / FMath::Max(WarmFrameMs.Num(), 1);
			Test->AddInfo(FString::Printf(TEXT("Warm Planet sample: frames=%d mean=%.3fms median=%.3fms p95=%.3fms max=%.3fms equivalentFps=%.1f (offscreen, not interactive acceptance)"),
				WarmFrameMs.Num(), Mean, WarmFrameMs[WarmFrameMs.Num() / 2],
				WarmFrameMs[FMath::FloorToInt((WarmFrameMs.Num() - 1) * 0.95)], WarmFrameMs.Last(), 1000.0 / Mean));
			VM->SetPreviewFocus(EAstroPreviewFocus::StarCluster);
			Step = 10;
			StepStart = Now;
		}
		if (Step == 10)
		{
			if (Now - StepStart < 2.0) return false;
			AStarCluster* Cluster = nullptr;
			for (TActorIterator<AStarCluster> It(World); It; ++It)
				if (It->IsAttachedTo(Generator.Get())) { Cluster = *It; break; }
			if (!Test->TestNotNull(TEXT("Remote navigation fixture has a catalog"), Cluster)) { Step = 99; return false; }
			int32 ViewportWidth = 0, ViewportHeight = 0;
			Controller->GetViewportSize(ViewportWidth, ViewportHeight);
			for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
			{
				// Later uneven-count and companion-removal steps require a triple
				// with three initial planets. Make that fixture contract explicit
				// instead of relying on seed 271828's first multistar row by accident.
				if (Record.bMaterialized || Record.SystemModel.AmountOfStars != 3 || Record.SystemModel.PotentialPlanetCount != 3) continue;
				FVector PresentedCenter;
				FVector2D Screen;
				if (!Generator->GetContinuousPreviewClusterLocation(Record.InstanceIndex, PresentedCenter)
					|| !Controller->ProjectWorldLocationToScreen(PresentedCenter, Screen, true)
					|| Screen.X < 16 || Screen.Y < 16 || Screen.X >= ViewportWidth - 16 || Screen.Y >= ViewportHeight - 16) continue;
				RemoteInstance = Record.InstanceIndex;
				RemoteStableId = Record.StableId;
				RemoteExpectedStars = Record.SystemModel.AmountOfStars;
				RemoteExpectedPlanets = Record.SystemModel.PotentialPlanetCount;
				break;
			}
			if (!Test->TestTrue(TEXT("The sampled catalog contains a visible non-home triple with three planets"), RemoteInstance != INDEX_NONE))
			{ Step = 99; return false; }
			FirstVisitOrbit = Generator->GetContinuousPreviewOrbit();
			UInstancedStaticMeshComponent* ClusterView = GetClusterView();
			if (!Test->TestNotNull(TEXT("First-visit audit has the actual rendered star layer"), ClusterView)) { Step = 99; return false; }
			for (const FAPSContinuousPreviewPoint& Point : Generator->GetContinuousPreviewClusterPoints())
			{
				if (Point.StableId != RemoteStableId) continue;
				FirstVisitPoints.Add(Point);
				Test->TestFalse(TEXT("Unvisited star data is actor-free"), Point.MaterializedStar.IsValid());
				FTransform Transform;
				ClusterView->GetInstanceTransform(Point.SourceInstanceIndex, Transform, true);
				FirstVisitPointTransforms.Add(Transform);
				for (int32 Field = 0; Field < ClusterView->NumCustomDataFloats; ++Field)
					FirstVisitCustomData.Add(ClusterView->PerInstanceSMCustomData[Point.SourceInstanceIndex * ClusterView->NumCustomDataFloats + Field]);
			}
			Test->TestEqual(TEXT("Unvisited system already publishes all declared stellar addresses"), FirstVisitPoints.Num(), RemoteExpectedStars);
			FVector FirstVisitCenter;
			Test->TestTrue(TEXT("First-visit photometric patch addresses the selected catalog star"),
				Generator->GetContinuousPreviewClusterLocation(RemoteInstance, FirstVisitCenter)
				&& Controller->ProjectWorldLocationToScreen(FirstVisitCenter, FirstVisitScreen, true));
			Step = 41;
			return false;
		}
		if (Step == 41)
		{
			if (!CaptureStarField(TEXT("12A-Cluster-Before-First-Visit"), FirstVisitPixels, FirstVisitSize, FirstVisitSeries)) return false;
			Step = 43;
			StepStart = Now;
			return false;
		}
		if (Step == 43)
		{
			if (Now - StepStart < 0.2) return false;
			TArray<FColor> RepeatPixels;
			FIntVector RepeatSize{FIntVector::ZeroValue};
			FStarFieldSeries RepeatSeries;
			if (!CaptureStarField(TEXT("12A2-Cluster-Unvisited-Control"), RepeatPixels, RepeatSize, RepeatSeries)) return false;
			CompareStarFieldPatch(TEXT("unvisited temporal control"), RepeatPixels, RepeatSize);
			CompareStarFieldSeries(TEXT("unvisited temporal control"), FirstVisitSeries, RepeatSeries);
			FirstVisitSeries = MoveTemp(RepeatSeries);
			FirstVisitPixels = MoveTemp(RepeatPixels);
			FirstVisitSize = RepeatSize;
			AStarCluster* Cluster = nullptr;
			for (TActorIterator<AStarCluster> It(World); It; ++It)
				if (It->IsAttachedTo(Generator.Get())) { Cluster = *It; break; }
			if (!Test->TestNotNull(TEXT("First-visit capture retains the catalog"), Cluster)) { Step = 99; return false; }
			CatalogTransforms.Reset();
			for (int32 Index = 0; Index < Cluster->StarMeshInstances->GetInstanceCount(); ++Index)
			{
				FTransform Transform;
				Cluster->StarMeshInstances->GetInstanceTransform(Index, Transform, false);
				CatalogTransforms.Add(Transform);
			}
			RemoteCluster = Cluster;
			const FVector ObserverBefore = Generator->GetContinuousPreviewOrbit().ObserverCm();
			Test->TestTrue(TEXT("The hierarchy opens another actual system"), VM->FocusPreviewClusterSystem(RemoteInstance));
			RemoteSystem = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("The remote record has a materialized hierarchy"), RemoteSystem.Get()))
			{ Step = 99; return false; }
			Test->TestEqual(TEXT("Materialization retains the chosen system identity"), RemoteSystem->StableSystemId, RemoteStableId);
			Test->TestEqual(TEXT("Every catalog companion is materialized"), RemoteSystem->GetStars().Num(), RemoteExpectedStars);
			int32 PlanetCount = 0;
			for (AStar* Star : RemoteSystem->GetStars())
				if (Star && Star->PlanetarySystem) PlanetCount += Star->PlanetarySystem->PlanetsActorsList.Num();
			Test->TestEqual(TEXT("Catalog total planets is not incorrectly multiplied by star count"), PlanetCount, RemoteExpectedPlanets);
			Test->TestTrue(TEXT("Opening a remote hierarchy starts at the displayed observer"),
				Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(ObserverBefore, 0.0));
			Step = 11;
			StepStart = Now;
			return false;
		}
		if (Step == 11)
		{
			CheckGeometry(World);
			if (Now - StepStart < 2.0) return false;
			Capture(TEXT("12-Remote-System"));
			VM->SetPreviewFocus(EAstroPreviewFocus::StarCluster);
			Step = 40;
			StepStart = Now;
			return false;
		}
		if (Step == 40)
		{
			if (Now - StepStart < 2.0) return false;
			const FAPSContinuousPreviewOrbit& AfterOrbit = Generator->GetContinuousPreviewOrbit();
			Test->TestEqual(TEXT("Far-view comparison returns to the same physical observer"), AfterOrbit.ObserverCm(), FirstVisitOrbit.ObserverCm());
			UInstancedStaticMeshComponent* ClusterView = GetClusterView();
			if (!Test->TestNotNull(TEXT("First-visit return retains the star layer"), ClusterView)) { Step = 99; return false; }
			for (int32 Index = 0; Index < FirstVisitPoints.Num(); ++Index)
			{
				const FAPSContinuousPreviewPoint& Before = FirstVisitPoints[Index];
				const FAPSContinuousPreviewPoint* After = Generator->GetContinuousPreviewClusterPoints().FindByPredicate(
					[&Before](const FAPSContinuousPreviewPoint& Point) { return Point.StableId == Before.StableId && Point.SystemStarIndex == Before.SystemStarIndex; });
				if (!Test->TestNotNull(TEXT("First visit retains the original stellar address"), After)) continue;
				Test->TestTrue(TEXT("Pre-existing point binds its precise physical star"), After->MaterializedStar.IsValid());
				if (After->MaterializedStar.IsValid())
				{
					Test->TestTrue(TEXT("A physical star is at its already-published position, not a new offset"),
						Before.CenterCm.Equals(Generator->GetContinuousPreviewPhysicalPosition(After->MaterializedStar.Get()), 1.0));
					Test->TestEqual(TEXT("Physical radius matches the unvisited stellar record"), After->MaterializedStar->RadiusKM * 1.0e5, Before.RadiusCm);
				}
				Test->TestEqual(TEXT("First visit cannot allocate a replacement render slot"), After->SourceInstanceIndex, Before.SourceInstanceIndex);
				FTransform Transform;
				ClusterView->GetInstanceTransform(After->SourceInstanceIndex, Transform, true);
				Test->TestTrue(TEXT("At the same far observer the actual rendered PSF transform is unchanged"), Transform.Equals(FirstVisitPointTransforms[Index], 1.0));
				for (int32 Field = 0; Field < ClusterView->NumCustomDataFloats; ++Field)
					Test->TestEqual(TEXT("First visit cannot alter the star's color, emission or material metadata"),
						ClusterView->PerInstanceSMCustomData[After->SourceInstanceIndex * ClusterView->NumCustomDataFloats + Field],
						FirstVisitCustomData[Index * ClusterView->NumCustomDataFloats + Field]);
			}
			Step = 42;
			return false;
		}
		if (Step == 42)
		{
			TArray<FColor> AfterPixels;
			FIntVector AfterSize{FIntVector::ZeroValue};
			FStarFieldSeries AfterSeries;
			if (!CaptureStarField(TEXT("12B-Cluster-After-First-Visit"), AfterPixels, AfterSize, AfterSeries)) return false;
			CompareStarFieldPatch(TEXT("after first visit"), AfterPixels, AfterSize);
			CompareStarFieldSeries(TEXT("after first visit"), FirstVisitSeries, AfterSeries);
			VM->FocusPreviewClusterSystem(RemoteInstance);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeStar);
			Test->TestEqual(TEXT("Root Star belongs to the chosen catalog system, not home"),
				Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(RemoteSystem->MainStar));
			Step = 12;
			StepStart = Now;
			return false;
		}
		if (Step == 12)
		{
			CheckGeometry(World);
			if (Now - StepStart < 2.0) return false;
			Capture(TEXT("13-Remote-Star"));
			VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
			RemotePlanet = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
			if (!Test->TestNotNull(TEXT("Root Planet opens an actual catalog planet"), RemotePlanet.Get()))
			{ Step = 99; return false; }
			Test->TestTrue(TEXT("The chosen planet belongs to the same system"), RemotePlanet->IsAttachedTo(RemoteSystem.Get()));
			RemoteRadiusKm = RemotePlanet->RadiusKM;
			RemotePhysicalTransform = RemotePlanet->GetActorTransform();
			RemoteSatellites = SatellitePhysics(RemotePlanet.Get());
			Test->TestEqual(TEXT("Selecting a remote planet loads its exact physical radius, not a terrestrial clamp"),
				VM->GeneratedWorld->PlanetRadius, RemoteRadiusKm);
			VM->SetPlanetRadius(RemoteRadiusKm);
			Test->TestEqual(TEXT("Recommitting the displayed radius is a no-op"), VM->GeneratedWorld->PlanetRadius, RemoteRadiusKm);
			Test->TestTrue(TEXT("Remote body edit address cannot alias SYS0"),
				Generator->GetPreviewBodyStableKey(RemotePlanet.Get()).StartsWith(TEXT("SYS-") + RemoteStableId.ToString(EGuidFormats::Digits)));
			Step = 13;
			StepStart = Now;
			return false;
		}
		if (Step == 13)
		{
			CheckGeometry(World);
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			Capture(TEXT("14-Remote-Planet"));
			FVector RemoteCenter;
			FVector2D RemoteScreen;
			if (Test->TestTrue(TEXT("The remote globe exposes the same center to screen picking"),
				Generator->GetPreviewPresentationLocation(RemotePlanet.Get(), RemoteCenter)
					&& Controller->ProjectWorldLocationToScreen(RemoteCenter, RemoteScreen, true)))
			{
				Test->TestTrue(TEXT("The normal picking route can select the remote globe through its overlay"),
					VM->FocusPreviewAtScreenPosition(RemoteScreen));
				Test->TestEqual(TEXT("Screen picking selects that remote body, not an overlapping local physics bubble"),
					VM->GetSelectedPreviewBody(), static_cast<AActor*>(RemotePlanet.Get()));
			}
			VM->SetPreviewFocus(EAstroPreviewFocus::StarCluster);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			Test->TestEqual(TEXT("Parent round trip retains the same remote system"), Generator->GetContinuousPreviewActiveSystem(), RemoteSystem.Get());
			VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
			Test->TestEqual(TEXT("Parent round trip retains the same remote planet"),
				Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(RemotePlanet.Get()));
			Test->TestEqual(TEXT("Passive parent navigation cannot clamp the editor radius"),
				VM->GeneratedWorld->PlanetRadius, RemoteRadiusKm);
			Test->TestTrue(TEXT("A selected-body snapshot can be replayed"),
				AAstroGenerator::ApplyPreviewBodyEditOverrideByKey(VM->GeneratedWorld,
					Generator->GetPreviewBodyStableKey(RemotePlanet.Get()), RemotePlanet.Get()));
			Test->TestEqual(TEXT("Snapshot replay retains the exact physical radius"), RemotePlanet->RadiusKM, RemoteRadiusKm);
			Test->TestTrue(TEXT("Snapshot replay cannot rescale or move the physical actor"),
				RemotePlanet->GetActorTransform().Equals(RemotePhysicalTransform, 0.0));
			Test->TestEqual(TEXT("Passive radius snapshot cannot repack moons"), SatellitePhysics(RemotePlanet.Get()), RemoteSatellites);
			VM->FocusPreviewClusterSystem(RemoteInstance);
			Test->TestEqual(TEXT("Reopening a record reuses its hierarchy, not a second world"), Generator->GetContinuousPreviewActiveSystem(), RemoteSystem.Get());
			for (int32 Index = 0; Index < CatalogTransforms.Num(); ++Index)
			{
				FTransform Transform;
				RemoteCluster->StarMeshInstances->GetInstanceTransform(Index, Transform, false);
				Test->TestTrue(TEXT("Catalog source transforms are immutable during lazy navigation"), Transform.Equals(CatalogTransforms[Index], 0.0));
			}
			const FClusterStarSystemRecord* Record = RemoteCluster->FindPotentialSystem(RemoteInstance);
			Test->TestTrue(TEXT("Disposable materialization does not rewrite the sealed catalog flags"), Record && !Record->bMaterialized);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
			RemoteProfileSignature = Generator->GetPreviewGlobeProfileSignature();
			RemotePlanetKey = Generator->GetPreviewBodyStableKey(RemotePlanet.Get());
			BeforeRemoteRebuildStars = StellarPhysics(World, Generator.Get());
			// Earlier route steps intentionally changed home planets 6 -> 9 -> 3 -> 1.
			// That updates its catalog count/input hash. Compare this satellite-only
			// edit to the immediately preceding world, not the initial six-planet one.
			BeforeRemoteRebuildDatasetHash = Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash;
			RemoteDesiredMoons = RemotePlanet->Moons.Num() == 2 ? 3 : 2;
			VM->SetMoonsAmount(RemoteDesiredMoons);
			Step = 14;
			StepStart = Now;
			return false;
		}
		if (Step == 14)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			APlanet* RebuiltPlanet = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
			if (!Test->TestNotNull(TEXT("Remote structural edit restores a selected planet"), RebuiltPlanet))
			{ Step = 99; return false; }
			Test->TestTrue(TEXT("Structural edit replaces the disposable actor"), RebuiltPlanet != RemotePlanet.Get());
			Test->TestEqual(TEXT("Structural edit restores the remote body address"),
				Generator->GetPreviewBodyStableKey(RebuiltPlanet), RemotePlanetKey);
			RemoteSystem = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("The selected catalog system survives structural regeneration"), RemoteSystem.Get()))
			{ Step = 99; return false; }
			Test->TestEqual(TEXT("Structural edit retains selected system identity"), RemoteSystem->StableSystemId, RemoteStableId);
			Test->TestEqual(TEXT("Moon count edits apply to the selected remote planet"), RebuiltPlanet->Moons.Num(), RemoteDesiredMoons);
			Test->TestEqual(TEXT("Remote physical radius survives full reconstruction"), RebuiltPlanet->RadiusKM, RemoteRadiusKm);
			Test->TestEqual(TEXT("Remote editor rehydrates retained physical radius"), VM->GeneratedWorld->PlanetRadius, RemoteRadiusKm);
			Test->TestEqual(TEXT("Changing satellites does not replace the parent surface profile"),
				Generator->GetPreviewGlobeProfileSignature(), RemoteProfileSignature);
			Test->TestEqual(TEXT("Remote structural edits do not reroll any stellar physics"),
				StellarPhysics(World, Generator.Get()), BeforeRemoteRebuildStars);
			Test->TestEqual(TEXT("Remote structural edit retains the sealed world dataset"),
				Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash, BeforeRemoteRebuildDatasetHash);
			CheckGeometry(World);
			Capture(TEXT("15-Remote-Rebuilt"));
			int32 HomeInstance = INDEX_NONE;
			for (TActorIterator<AStarCluster> It(World); It; ++It)
			{
				if (!It->IsAttachedTo(Generator.Get())) continue;
				RemoteCluster = *It;
				for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
					if (Record.bMaterialized) { HomeInstance = Record.InstanceIndex; break; }
			}
			Test->TestTrue(TEXT("The home catalog record remains explicitly addressable"), HomeInstance != INDEX_NONE);
			VM->FocusPreviewClusterSystem(HomeInstance);
			Test->TestTrue(TEXT("An explicit home selection leaves the remote system"),
				Generator->GetContinuousPreviewActiveSystem() != RemoteSystem.Get());
			Step = 15;
			StepStart = Now;
			return false;
		}
		if (Step == 15)
		{
			CheckGeometry(World);
			if (Now - StepStart < 2.0) return false;
			Capture(TEXT("16-Explicit-Home"));
			int32 EmptyInstance = INDEX_NONE;
			for (const FClusterStarSystemRecord& Record : RemoteCluster->PotentialStarSystems)
				if (!Record.bMaterialized && Record.SystemModel.PotentialPlanetCount == 0)
				{ EmptyInstance = Record.InstanceIndex; break; }
			if (Test->TestTrue(TEXT("Catalog includes a system without planets"), EmptyInstance != INDEX_NONE))
			{
				Test->TestTrue(TEXT("A system without planets remains navigable"), VM->FocusPreviewClusterSystem(EmptyInstance));
				Test->TestTrue(TEXT("A planetless system still has an available Star scope"), VM->IsPreviewFocusAvailable(EAstroPreviewFocus::HomeStar));
				Test->TestFalse(TEXT("A planetless system cannot expose the home planet as its own"), VM->IsPreviewFocusAvailable(EAstroPreviewFocus::HomePlanet));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeStar);
			}
			Step = 16;
			StepStart = Now;
			return false;
		}
		if (Step == 16)
		{
			CheckGeometry(World);
			if (Now - StepStart < 2.0) return false;
			Capture(TEXT("17-Planetless-Star"));
			VM->FocusPreviewClusterSystem(RemoteInstance);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeStar);
			RemoteSystem = Generator->GetContinuousPreviewActiveSystem();
			BeforeStarEditDataset = VM->GeneratedWorld->CanonicalStellarDataset;
			UneditedHomePhysics = SystemStellarPhysics(World, BeforeStarEditDataset.HomeStableId);
			UneditedSiblingPhysics = SystemStellarPhysics(World, RemoteStableId, 0);
			FStarModel Current;
			if (!Test->TestTrue(TEXT("Remote primary has an addressed editor model"),
				Generator->GetPreviewStarEditContext(EditedStarAddress, Current))) { Step = 99; return false; }
			Test->TestEqual(TEXT("Remote editor reads selected stellar type"), VM->GetSelectedStellarType(), Current.StellarType);
			Test->TestEqual(TEXT("Remote editor reads selected spectrum"), VM->GetSelectedSpectralClass(), Current.SpectralClass);
			VM->SetSelectedStarRadiusOverrideSolar(2.25);
			Test->TestEqual(TEXT("Pending radius is immediately displayed for this address"), VM->GetSelectedStarRadiusOverrideSolar(), 2.25);
			Step = 17;
			StepStart = Now;
			return false;
		}
		if (Step >= 17 && Step <= 20)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			RemoteSystem = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("Stellar edit retains active system"), RemoteSystem.Get())) { Step = 99; return false; }
			Test->TestEqual(TEXT("Stellar edit cannot select a different system"), RemoteSystem->StableSystemId, RemoteStableId);
			FString Address;
			FStarModel Current;
			if (!Test->TestTrue(TEXT("Rebuilt selected star retains an editor model"),
				Generator->GetPreviewStarEditContext(Address, Current))) { Step = 99; return false; }
			Test->TestEqual(TEXT("Stellar edit retains exact selected address"), Address, EditedStarAddress);
			Test->TestEqual(TEXT("Selected stellar edits never change home-star physics"),
				SystemStellarPhysics(World, BeforeStarEditDataset.HomeStableId), UneditedHomePhysics);
			Test->TestEqual(TEXT("Selected stellar edits never reroll siblings"),
				SystemStellarPhysics(World, RemoteStableId, Step == 20 ? 1 : 0), UneditedSiblingPhysics);
			const FAPSCanonicalStellarDataset& Dataset = VM->GeneratedWorld->CanonicalStellarDataset;
			Test->TestTrue(TEXT("Explicit stellar authoring creates a usable finalized manifest"), Dataset.IsUsable(Dataset.InputHash));
			Test->TestTrue(TEXT("New stellar authoring has a new input hash"), Dataset.InputHash != BeforeStarEditDataset.InputHash);
			Test->TestTrue(TEXT("New stellar authoring advances the dataset version serial"), Dataset.BuildSerial > BeforeStarEditDataset.BuildSerial);
			Test->TestEqual(TEXT("Stellar edits preserve catalog population"), Dataset.ClusterRecords.Num(), BeforeStarEditDataset.ClusterRecords.Num());
			for (int32 Index = 0; Index < FMath::Min(Dataset.ClusterRecords.Num(), BeforeStarEditDataset.ClusterRecords.Num()); ++Index)
			{
				const FAPSCanonicalClusterSystemRecord& A = Dataset.ClusterRecords[Index];
				const FAPSCanonicalClusterSystemRecord& B = BeforeStarEditDataset.ClusterRecords[Index];
				Test->TestEqual(TEXT("Stellar authoring preserves every system address"), A.StableId, B.StableId);
				Test->TestEqual(TEXT("Stellar authoring preserves every canonical position"), A.ClusterLocalLocation, B.ClusterLocalLocation);
				Test->TestTrue(TEXT("Stellar authoring cannot mutate another system recipe"),
					FStarSystemModel::StaticStruct()->CompareScriptStruct(&A.SystemModel, &B.SystemModel, 0));
				if (A.StableId != RemoteStableId || Step == 20)
					Test->TestTrue(TEXT("Unedited catalog primaries remain byte-for-field identical"),
						FStarModel::StaticStruct()->CompareScriptStruct(&A.PrimaryStarModel, &B.PrimaryStarModel, 0));
				else
					Test->TestTrue(TEXT("Selected primary and its distant catalog record use the same model"),
						FStarModel::StaticStruct()->CompareScriptStruct(&A.PrimaryStarModel, &Current, 0));
			}
			Test->TestEqual(TEXT("Stellar controls leave the home recipe radius untouched"), VM->GeneratedWorld->HomeStarRadiusOverrideSolar, 0.0);
			Test->TestEqual(TEXT("Stellar controls leave the home recipe type untouched"),
				VM->GeneratedWorld->StellarType, Fixture.StellarType);
			CheckGeometry(World);
			if (Step == 17)
			{
				Test->TestEqual(TEXT("Remote primary receives its physical radius edit"), Current.Radius, 2.25f);
				Capture(TEXT("18-Remote-Star-Radius"));
				BeforeStarEditDataset = Dataset;
				VM->SetEnumValue(StaticEnum<EStellarType>(), static_cast<int32>(EStellarType::MainSequence));
				VM->SetEnumValue(StaticEnum<ESpectralClass>(), static_cast<int32>(ESpectralClass::B));
			}
			else if (Step == 18)
			{
				Test->TestEqual(TEXT("Remote type edit is applied to the selected star"), Current.StellarType, EStellarType::MainSequence);
				Test->TestEqual(TEXT("Remote spectral edit is applied to the selected star"), Current.SpectralClass, ESpectralClass::B);
				Test->TestEqual(TEXT("Explicit size survives type and spectral changes"), Current.Radius, 2.25f);
				Capture(TEXT("19-Remote-Star-Spectrum"));
				BeforeStarEditDataset = Dataset;
				VM->SetSelectedStarRadiusOverrideSolar(0.0);
			}
			else if (Step == 19)
			{
				const FAPSPreviewStarEditOverride* Edit = VM->GeneratedWorld->FindPreviewStarEditOverride(Address);
				if (Test->TestNotNull(TEXT("AUTO retains the addressed type edit"), Edit))
				{
					Test->TestEqual(TEXT("AUTO restores the retained generated radius"), Current.Radius, Edit->AutomaticModel.Radius);
					Test->TestEqual(TEXT("AUTO clears only the size override"), Edit->RadiusOverrideSolar, 0.0);
				}
				BeforeStarEditDataset = Dataset;
				VM->SetPreviewFocus(EAstroPreviewFocus::StarCluster);
				VM->FocusPreviewClusterSystem(RemoteInstance);
				VM->FocusPreviewBody(RemoteSystem->GetStars()[1]);
				Test->TestTrue(TEXT("Companion editor has a separate stable address"),
					Generator->GetPreviewStarEditContext(EditedStarAddress, Current) && EditedStarAddress.EndsWith(TEXT("/S1")));
				UneditedSiblingPhysics = SystemStellarPhysics(World, RemoteStableId, 1);
				VM->SetSelectedStarRadiusOverrideSolar(0.75);
			}
			else
			{
				Test->TestEqual(TEXT("Companion radius edit does not redirect to the primary"), Current.Radius, 0.75f);
				Test->TestEqual(TEXT("Companion controls display its own authored radius"), VM->GetSelectedStarRadiusOverrideSolar(), 0.75);
				Capture(TEXT("20-Remote-Companion-Edit"));
				BeforeSystemEditDataset = Dataset;
				RemoteUneditedStellarPhysics = SystemStellarPhysics(World, RemoteStableId);
				RemoteUneditedPrimaryPhysics = OneStarPhysics(RemoteSystem->MainStar);
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
				Test->TestEqual(TEXT("System control initially shows actual remote total"), VM->GetSelectedSystemPlanetCount(), 3);
				VM->SetSelectedSystemPlanetCount(8);
				VM->SetEnumValue(StaticEnum<EOrbitDistributionType>(), static_cast<int32>(EOrbitDistributionType::Dense));
				Test->TestEqual(TEXT("Pending system count is immediately visible"), VM->GetSelectedSystemPlanetCount(), 8);
				Step = 21;
				StepStart = Now;
				return false;
			}
			++Step;
			StepStart = Now;
			return false;
		}
		if (Step >= 21 && Step <= 30)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			AStarSystem* System = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("System edit retains an actual selected hierarchy"), System)) { Step = 99; return false; }
			const bool bHomeEdit = Step >= 27;
			const FGuid ExpectedId = bHomeEdit ? BeforeSystemEditDataset.HomeStableId : RemoteStableId;
			Test->TestEqual(TEXT("System controls retain their target system"), System->StableSystemId, ExpectedId);
			constexpr int32 ExpectedTotals[] = {8, 1, 0, 3, 3, 3, 7, 7, 0, 3};
			const int32 Total = ExpectedTotals[Step - 21];
			int32 ActualTotal = 0;
			for (const AStar* Star : System->GetStars()) ActualTotal += Star->PlanetarySystem->PlanetsActorsList.Num();
			Test->TestEqual(TEXT("Materialized system has the exact requested total"), ActualTotal, Total);
			Test->TestEqual(TEXT("System count control agrees with materialized hierarchy"), VM->GetSelectedSystemPlanetCount(), Total);
			if (VM->GetPreviewFocus() == EAstroPreviewFocus::HomeSystem)
			{
				const FString Summary = VM->GetPreviewScopeSummary().ToString();
				Test->TestTrue(TEXT("Live system summary uses the actual total, including home systems"),
					Summary.Contains(FString::Printf(TEXT("TOTAL PLANETS  %d"), Total)));
				Test->TestFalse(TEXT("Live system summary cannot reuse the old per-star recipe"), Summary.Contains(TEXT("PLANETS PER STAR")));
			}
			int32 ActualPlanetActors = 0;
			for (TActorIterator<APlanet> It(World); It; ++It) if (It->IsAttachedTo(System)) ++ActualPlanetActors;
			Test->TestEqual(TEXT("No stale planet actor remains in the edited hierarchy"), ActualPlanetActors, Total);
			const FAPSCanonicalStellarDataset& Dataset = VM->GeneratedWorld->CanonicalStellarDataset;
			Test->TestTrue(TEXT("System authoring leaves a usable finalized dataset"), Dataset.IsUsable(Dataset.InputHash));
			Test->TestEqual(TEXT("System authoring preserves catalog population"), Dataset.ClusterRecords.Num(), BeforeSystemEditDataset.ClusterRecords.Num());
			for (int32 Index = 0; Index < FMath::Min(Dataset.ClusterRecords.Num(), BeforeSystemEditDataset.ClusterRecords.Num()); ++Index)
			{
				const FAPSCanonicalClusterSystemRecord& A = Dataset.ClusterRecords[Index];
				const FAPSCanonicalClusterSystemRecord& B = BeforeSystemEditDataset.ClusterRecords[Index];
				Test->TestEqual(TEXT("System authoring preserves every catalog address"), A.StableId, B.StableId);
				Test->TestEqual(TEXT("System authoring preserves every catalog position"), A.ClusterLocalLocation, B.ClusterLocalLocation);
				Test->TestTrue(TEXT("System authoring cannot reroll a primary star"), FStarModel::StaticStruct()->CompareScriptStruct(&A.PrimaryStarModel, &B.PrimaryStarModel, 0));
				if (A.StableId != ExpectedId)
					Test->TestTrue(TEXT("Neighboring system recipes are unchanged"), FStarSystemModel::StaticStruct()->CompareScriptStruct(&A.SystemModel, &B.SystemModel, 0));
				else
				{
					Test->TestEqual(TEXT("Catalog multiplicity matches actual stars"), A.SystemModel.AmountOfStars, System->GetStars().Num());
					Test->TestEqual(TEXT("Catalog total matches actual planets"), A.SystemModel.PotentialPlanetCount, Total);
					Test->TestEqual(TEXT("Catalog occupancy matches actual planets"), A.SystemModel.bHasPlanetarySystem, Total > 0);
				}
			}
			Test->TestEqual(TEXT("System controls never mutate the legacy home planet recipe"), VM->GeneratedWorld->PlanetsAmount, 1);
			Test->TestEqual(TEXT("System controls never mutate the legacy home star recipe"), VM->GeneratedWorld->StarType, EStarType::TripleStar);
			Test->TestEqual(TEXT("System edits cannot reroll any home star"),
				SystemStellarPhysics(World, Dataset.HomeStableId), UneditedHomePhysics);
			if (!bHomeEdit)
			{
				Test->TestEqual(TEXT("System edits preserve surviving remote stellar physics"), SystemStellarPhysics(World, RemoteStableId),
					Step == 25 ? RemoteUneditedPrimaryPhysics : RemoteUneditedStellarPhysics);
				Test->TestEqual(TEXT("Selected orbit distribution persists through structural edits"), VM->GetSelectedSystemOrbitDistribution(), EOrbitDistributionType::Dense);
			}
			CheckGeometry(World);
			BeforeSystemEditDataset = Dataset;
			if (Step == 21)
			{
				Capture(TEXT("21-Remote-System-Eight"));
				Test->TestEqual(TEXT("Uneven total assigns three planets to primary"), System->MainStar->PlanetarySystem->PlanetsActorsList.Num(), 3);
				Test->TestEqual(TEXT("Uneven total assigns two planets to last companion"), System->GetStars()[2]->PlanetarySystem->PlanetsActorsList.Num(), 2);
				VM->FocusPreviewBody(System->GetStars()[2]->PlanetarySystem->PlanetsActorsList.Last());
				VM->SetSelectedSystemPlanetCount(1);
			}
			else if (Step == 22)
			{
				APlanet* Selected = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
				Test->TestTrue(TEXT("Removed remote planet falls back inside the same system"), Selected && Selected->ParentStar == System->MainStar);
				VM->SetSelectedSystemPlanetCount(0);
			}
			else if (Step == 23)
			{
				Test->TestEqual(TEXT("Removing the last remote planet moves up to Star"), VM->GetPreviewFocus(), EAstroPreviewFocus::HomeStar);
				Test->TestEqual(TEXT("Zero-planet fallback is the same system's star"), Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(System->MainStar));
				Test->TestFalse(TEXT("Zero-planet system cannot expose a home planet"), VM->IsPreviewFocusAvailable(EAstroPreviewFocus::HomePlanet));
				Capture(TEXT("22-Remote-System-Zero"));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
				VM->SetEnumValue(StaticEnum<EPlanetarySystemType>(), static_cast<int32>(EPlanetarySystemType::SinglePlanetSystem));
			}
			else if (Step == 24)
			{
				for (const AStar* Star : System->GetStars()) Test->TestEqual(TEXT("Single family preset allocates one planet per star"), Star->PlanetarySystem->PlanetsActorsList.Num(), 1);
				VM->FocusPreviewBody(System->GetStars()[1]);
				VM->SetEnumValue(StaticEnum<EStarType>(), static_cast<int32>(EStarType::SingleStar));
			}
			else if (Step == 25)
			{
				Test->TestEqual(TEXT("Removing a selected companion retains the primary star"), Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(System->MainStar));
				Test->TestEqual(TEXT("Single-star system preserves total instead of truncating it"), System->GetStars().Num(), 1);
				Capture(TEXT("23-Remote-Single-Star"));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
				VM->SetEnumValue(StaticEnum<EStarType>(), static_cast<int32>(EStarType::TripleStar));
			}
			else if (Step == 26)
			{
				Test->TestEqual(TEXT("Readded companion retains its authored stellar radius"), System->GetStars()[1]->RadiusKM, 521775.0);
				VM->FocusPreviewClusterSystem(Dataset.HomeCanonicalIndex);
				VM->SetSelectedSystemPlanetCount(7);
			}
			else if (Step == 27)
			{
				Capture(TEXT("24-Home-System-Seven"));
				Test->TestEqual(TEXT("Home start index range comes from actual primary family"), VM->GetHomeStartPlanetCount(), 3);
				VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
				StartInspectionKey = Generator->GetPreviewBodyStableKey(Cast<APlanet>(Generator->GetSelectedPreviewBodyActor()));
				VM->SetStartPlanetIndex(3);
			}
			else if (Step == 28)
			{
				Test->TestEqual(TEXT("Home start index is not limited by stale per-star defaults"), VM->GeneratedWorld->StartPlanetIndex, 3);
				const FObjectPropertyBase* HomeProperty = FindFProperty<FObjectPropertyBase>(AAstroGenerator::StaticClass(), TEXT("HomePlanet"));
				const APlanet* StartPlanet = HomeProperty ? Cast<APlanet>(HomeProperty->GetObjectPropertyValue_InContainer(Generator.Get())) : nullptr;
				Test->TestTrue(TEXT("Home start assignment resolves its actual third orbit"), StartPlanet && Generator->GetPreviewBodyStableKey(StartPlanet) == TEXT("SYS0/S0/P2"));
				Test->TestEqual(TEXT("Changing home start assignment cannot replace the inspected planet"),
					Generator->GetPreviewBodyStableKey(Cast<APlanet>(Generator->GetSelectedPreviewBodyActor())), StartInspectionKey);
				VM->SetSelectedSystemPlanetCount(0);
			}
			else if (Step == 29)
			{
				Test->TestEqual(TEXT("Zero home planets clears the usable start range"), VM->GetHomeStartPlanetCount(), 0);
				Test->TestEqual(TEXT("Zero home planets also moves view up to Star"), VM->GetPreviewFocus(), EAstroPreviewFocus::HomeStar);
				Capture(TEXT("25-Home-Zero-Start-Unavailable"));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
				VM->SetSelectedSystemPlanetCount(3);
			}
			else
			{
				VM->FocusPreviewClusterSystem(RemoteInstance);
				Test->TestEqual(TEXT("Revisiting remote system after home edits retains stellar edits"), SystemStellarPhysics(World, RemoteStableId), RemoteUneditedStellarPhysics);
				Test->TestEqual(TEXT("Revisiting remote system retains its own total"), VM->GetSelectedSystemPlanetCount(), 3);
				VM->SetPreviewFocus(EAstroPreviewFocus::Overview);
				Test->TestTrue(TEXT("Overview describes the actual edited home hierarchy, not the inspected remote system or old defaults"),
					VM->GetPreviewScopeSummary().ToString().Contains(TEXT("HOME SYSTEM PLANETS  3")));
				VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
				Step = 31;
				StepStart = Now;
				return false;
			}
			++Step;
			StepStart = Now;
			return false;
		}
		if (Step == 31)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			if (!SampleCacheFrames(Now, TEXT("before visits"))) return false;
			RemoteSystem = Generator->GetContinuousPreviewActiveSystem();
			CacheOriginalSystem = RemoteSystem;
			CacheOriginalTerrain = Generator->GetActivePreviewTerrainProxy();
			Test->TestTrue(TEXT("Cache test begins with a real retained planetary globe"), CacheOriginalTerrain.IsValid());
			CacheOriginalPhysics = HierarchyPhysics(RemoteSystem.Get(), Generator.Get());
			CacheOriginalProfile = Generator->GetPreviewGlobeProfileSignature();
			CacheDatasetHash = Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash;
			CachePointCountBeforeVisits = Generator->GetContinuousPreviewClusterPoints().Num();
			for (TActorIterator<AStarCluster> It(World); It; ++It)
				if (It->IsAttachedTo(Generator.Get()))
				{
					const FAPSCanonicalStellarProjectionFrame& Frame = It->CanonicalProjectionFrame;
					Test->AddInfo(FString::Printf(TEXT("Cache cluster frame: cmPerUnit=%.17g layerToRoot=%.17g halfExtentUnits=%.17g halfExtentKm=%.17g"),
						Frame.CanonicalCmPerUnit, Frame.LayerToRootPositionScale, Frame.CanonicalHalfExtentUnits,
						Frame.CanonicalHalfExtentUnits * Frame.LayerToRootPositionScale * Frame.CanonicalCmPerUnit / 1.0e5));
				}
			CacheOriginalBodySpheres.Reset();
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->IsAttachedTo(RemoteSystem.Get())) continue;
				const AStar* Star = Cast<AStar>(*It);
				const APlanetaryBody* Body = Cast<APlanetaryBody>(*It);
				const double RadiusKm = Star ? Star->RadiusKM : (Body ? Body->RadiusKM : 0.0);
				if (RadiusKm <= 0.0) continue;
				CacheOriginalBodySpheres.Emplace(Generator->GetContinuousPreviewPhysicalPosition(*It), RadiusKm * 1.0e5);
				if (Star) Test->AddInfo(FString::Printf(TEXT("Cache original star: physics=[%s] physicalCenter=%s systemOffsetKm=%.17g"),
					*OneStarPhysics(Star), *CacheOriginalBodySpheres.Last().Key.ToString(),
					(Star->GetActorLocation() - RemoteSystem->GetActorLocation()).Size() / 1.0e5));
			}
			for (const FAPSContinuousPreviewPoint& Point : Generator->GetContinuousPreviewClusterPoints())
			{
				if (Point.StableId != RemoteStableId || !Point.MaterializedStar.IsValid()) continue;
				FAPSContinuousPreviewPoint Snapshot = Point;
				Snapshot.CenterCm = Generator->GetContinuousPreviewPhysicalPosition(Point.MaterializedStar.Get());
				Snapshot.RadiusCm = Point.MaterializedStar->RadiusKM * 1.0e5;
				CacheStarPoints.Add(Snapshot);
			}
			Test->TestEqual(TEXT("Cache snapshot includes the primary and both companions"), CacheStarPoints.Num(), 3);
			for (const FAPSCanonicalClusterSystemRecord& Record : VM->GeneratedWorld->CanonicalStellarDataset.ClusterRecords)
			{
				if (Record.StableId == RemoteStableId || Record.StableId == VM->GeneratedWorld->CanonicalStellarDataset.HomeStableId
					|| Record.SystemModel.PotentialPlanetCount > 20) continue;
				CacheVisitIds.Add(Record.StableId);
				if (CacheVisitIds.Num() == AAstroGenerator::ContinuousPreviewSystemCacheLimit + 2) break;
			}
			if (!Test->TestEqual(TEXT("Cache route has more unique systems than resident slots"), CacheVisitIds.Num(), 6))
			{ Step = 99; return false; }
			BeginCacheVisit(VM, Now);
			Step = 32;
			return false;
		}
		if (Step == 32)
		{
			if (Now - StepStart < 2.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			AStarSystem* Current = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("Cache visit has a live target system"), Current)) { Step = 99; return false; }
			Test->TestEqual(TEXT("Cache visit focuses the exact requested address"), Current->StableSystemId, CacheVisitIds[CacheVisitIndex % CacheVisitIds.Num()]);
			const double OriginalPixelRadius = LogOriginalCacheResidency(World, Controller);
			CheckCache(World);
			CheckGeometry(World);
			++CacheVisitIndex;
			if (CacheVisitIndex == CacheVisitIds.Num()) CachePointCountAfterFirstPass = Generator->GetContinuousPreviewClusterPoints().Num();
			if (CacheVisitIndex < CacheVisitIds.Num() * 2)
			{
				BeginCacheVisit(VM, Now);
				return false;
			}
			if (CacheOriginalSystem.IsValid() && !bCacheFarPass)
			{
				// A dense formation may leave the original star resolved throughout
				// the nearby route. Retirement there would itself change the scene.
				// Prove that justified retention, then extend the sampled route while
				// retaining the actual-retirement/reconstruction assertions below.
				if (!Test->TestTrue(TEXT("A retained original hierarchy has a resolved physical neighbor, not only a stale reference"),
					OriginalPixelRadius >= 0.3)) { Step = 99; StepStart = Now; return false; }
				Test->TestEqual(TEXT("A protected resolved neighbor retains its exact authored hierarchy"),
					HierarchyPhysics(CacheOriginalSystem.Get(), Generator.Get()), CacheOriginalPhysics);
				Capture(TEXT("26A-Resolved-Neighbor-Retained"));
				AStarCluster* Cluster = nullptr;
				for (TActorIterator<AStarCluster> It(World); It; ++It)
					if (It->IsAttachedTo(Generator.Get())) { Cluster = *It; break; }
				if (!Test->TestNotNull(TEXT("Far cache route retains the same sampled catalog"), Cluster))
				{ Step = 99; StepStart = Now; return false; }
				const FVector OriginalCenter = Generator->GetContinuousPreviewPhysicalPosition(CacheOriginalSystem.Get());
				TArray<TPair<double, FGuid>> FarCandidates;
				for (const FClusterStarSystemRecord& Record : Cluster->PotentialStarSystems)
				{
					if (Record.StableId == RemoteStableId || Record.StableId == VM->GeneratedWorld->CanonicalStellarDataset.HomeStableId
						|| CacheVisitIds.Contains(Record.StableId) || Record.SystemModel.PotentialPlanetCount > 20) continue;
					const FAPSCanonicalStellarProjectionFrame& Frame = Cluster->CanonicalProjectionFrame;
					const FVector Center = Frame.GetCanonicalRootPositionCm(Record.ClusterLocalLocation) - Frame.CanonicalAnchorCm;
					FarCandidates.Emplace((Center - OriginalCenter).SizeSquared(), Record.StableId);
				}
				FarCandidates.Sort([](const auto& A, const auto& B) { return A.Key > B.Key; });
				if (!Test->TestTrue(TEXT("The same render sample contains six distinct farther cache targets"), FarCandidates.Num() >= 6))
				{ Step = 99; StepStart = Now; return false; }
				CacheVisitIds.Reset();
				for (int32 Index = 0; Index < 6; ++Index) CacheVisitIds.Add(FarCandidates[Index].Value);
				bCacheFarPass = true;
				CacheVisitIndex = 0;
				Test->AddInfo(TEXT("Resolved-neighbor route extends to six outer-sample system centers, visited twice; actual stellar departure is measured separately."));
				BeginCacheVisit(VM, Now);
				return false;
			}
			if (CacheOriginalSystem.IsValid())
			{
				// Distance from a system center does not bound distance from each
				// companion. Leave the neighborhood through the real Galaxy button,
				// then measure every original physical body before testing retirement.
				VM->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
				Step = 44;
			}
			else Step = 45;
			StepStart = Now;
			return false;
		}
		if (Step == 44)
		{
			if (Now - StepStart < 2.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			double LargestOriginalPixelRadius = 0.0;
			for (const auto& Sphere : CacheOriginalBodySpheres)
				LargestOriginalPixelRadius = FMath::Max(LargestOriginalPixelRadius,
					Sphere.Value / (FMath::Max((Sphere.Key - Generator->GetContinuousPreviewFrame().ObserverCm).Size(), 1.0) * GetPixelTangent(Controller)));
			Test->AddInfo(FString::Printf(TEXT("Measured Galaxy cache departure: maxOriginalRadiusPixels=%.17g residentRemote=%d pressureApplied=%d"),
				LargestOriginalPixelRadius, Generator->GetContinuousPreviewResidentSystemCount(), bCacheDeparturePressure));
			if (!Test->TestTrue(TEXT("Galaxy departure makes every original body physically unresolved"), LargestOriginalPixelRadius < 0.3))
			{ Step = 99; StepStart = Now; return false; }
			CheckGeometry(World);
			if (CacheOriginalSystem.IsValid() && !bCacheDeparturePressure)
			{
				// A four-entry cache need not evict an idle entry without pressure.
				// Open one previously unresident sampled record, immediately retain
				// the far Galaxy observer, and let the normal cache policy do the work.
				TSet<FGuid> ResidentIds;
				for (TActorIterator<AStarSystem> It(World); It; ++It)
					if (It->IsAttachedTo(Generator.Get())) ResidentIds.Add(It->StableSystemId);
				int32 PressureInstance = INDEX_NONE;
				for (TActorIterator<AStarCluster> It(World); It; ++It)
				{
					if (!It->IsAttachedTo(Generator.Get())) continue;
					for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
						if (!ResidentIds.Contains(Record.StableId) && Record.SystemModel.PotentialPlanetCount <= 20)
						{ PressureInstance = Record.InstanceIndex; break; }
				}
				if (!Test->TestTrue(TEXT("Departure pressure uses an unresident record in the existing sample"), PressureInstance != INDEX_NONE))
				{ Step = 99; StepStart = Now; return false; }
				Test->TestTrue(TEXT("An ordinary hierarchy selection supplies cache pressure"), VM->FocusPreviewClusterSystem(PressureInstance));
				VM->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
				bCacheDeparturePressure = true;
				StepStart = Now;
				return false;
			}
			Test->TestTrue(TEXT("After physical departure the resident cache returns to its four-system target"),
				Generator->GetContinuousPreviewResidentSystemCount() <= AAstroGenerator::ContinuousPreviewSystemCacheLimit);
			CheckCache(World);
			Capture(TEXT("26B-Measured-Galaxy-Departure"));
			Step = 45;
			return false;
		}
		if (Step == 45)
		{
			Test->TestFalse(TEXT("Least-recent remote hierarchy is actually retired, not only hidden"), CacheOriginalSystem.IsValid());
			// The first globe can borrow one of the four native reusable proxy
			// components. Retirement must release its buffers/material and hide it,
			// not destroy that fixed empty component along with dynamic globes.
			Test->TestTrue(TEXT("Retiring a system releases its globe geometry and material, including reusable default buffers"),
				!CacheOriginalTerrain.IsValid() || (!CacheOriginalTerrain->IsVisible()
					&& CacheOriginalTerrain->GetNumSections() == 0 && CacheOriginalTerrain->GetMaterial(0) == nullptr));
			Test->TestEqual(TEXT("Repeated revisits cannot accumulate companion glyphs"), Generator->GetContinuousPreviewClusterPoints().Num(), CachePointCountAfterFirstPass);
			for (const FAPSContinuousPreviewPoint& Before : CacheStarPoints)
			{
				const FAPSContinuousPreviewPoint* After = Generator->GetContinuousPreviewClusterPoints().FindByPredicate(
					[&Before](const FAPSContinuousPreviewPoint& Point) { return Point.StableId == Before.StableId && Point.SystemStarIndex == Before.SystemStarIndex; });
				if (!Test->TestNotNull(TEXT("Retired stars retain their background points"), After)) continue;
				Test->TestFalse(TEXT("Retained point no longer holds a live heavyweight star"), After->MaterializedStar.IsValid());
				Test->TestEqual(TEXT("Retired star keeps its exact physical center"), After->CenterCm, Before.CenterCm);
				Test->TestEqual(TEXT("Retired star keeps its physical radius"), After->RadiusCm, Before.RadiusCm);
				Test->TestEqual(TEXT("Retired star reuses its original render slot"), After->SourceInstanceIndex, Before.SourceInstanceIndex);
			}
			Capture(TEXT("26-Bounded-Cache-Visited"));
			VM->FocusPreviewClusterSystem(RemoteInstance);
			VM->SetPreviewFocus(EAstroPreviewFocus::HomePlanet);
			Step = 33;
			StepStart = Now;
			return false;
		}
		if (Step == 33)
		{
			if (Now - StepStart < 3.0 || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
			if (!SampleCacheFrames(Now, TEXT("after visits, same planet"))) return false;
			AStarSystem* Restored = Generator->GetContinuousPreviewActiveSystem();
			if (!Test->TestNotNull(TEXT("Retired system can be materialized again"), Restored)) { Step = 99; return false; }
			Test->TestEqual(TEXT("Cache retirement preserves every addressed body, name, orbit and stellar edit"), HierarchyPhysics(Restored, Generator.Get()), CacheOriginalPhysics);
			Test->TestEqual(TEXT("Retired planet restores the same resolved surface recipe"), Generator->GetPreviewGlobeProfileSignature(), CacheOriginalProfile);
			Test->TestTrue(TEXT("Retired planet gets a fresh ready globe"), IsValid(Generator->GetActivePreviewTerrainProxy()));
			Test->TestEqual(TEXT("Rebinding all remote companions does not grow the point layer"), Generator->GetContinuousPreviewClusterPoints().Num(), CachePointCountAfterFirstPass);
			CheckCache(World);
			CheckGeometry(World);
			Capture(TEXT("27-Bounded-Cache-Restored-Planet"));
			Step = 99;
			StepStart = Now;
			return false;
		}
		return false;
	}

private:
	UInstancedStaticMeshComponent* GetClusterView() const
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Views;
		Generator->GetComponents(Views);
		for (UInstancedStaticMeshComponent* View : Views)
			if (View->GetName().StartsWith(TEXT("ContinuousClusterView"))) return View;
		return nullptr;
	}

	void ClearResolvedFrameProbe()
	{
		UActorComponent::MarkRenderStateDirtyEvent.Remove(ResolvedGlobeDirtyHandle);
		ResolvedGlobeDirtyHandle.Reset();
#if CSV_PROFILER
		if (bResolvedCsvOwned)
		{
			ResolvedCsvFuture = FCsvProfiler::Get()->EndCapture();
			bResolvedCsvOwned = false;
		}
#endif
		if (ResolvedFrameHandle.IsValid())
		{
			FCoreDelegates::OnEndFrame.Remove(ResolvedFrameHandle);
			ResolvedFrameHandle.Reset();
		}
		if (AppliedViewportProbe.IsValid())
		{
			IRendererModule* Renderer = &GetRendererModule();
			const auto Probe = AppliedViewportProbe;
			ENQUEUE_RENDER_COMMAND(APSStopResolvedViewportProbe)([Renderer, Probe](FRHICommandListImmediate&)
				{ Renderer->RemovePostOpaqueRenderDelegate(Probe->Handle); });
			AppliedViewportProbe.Reset();
		}
	}

	void EnsureResolvedFrameProbe()
	{
		if (!AppliedViewportProbe.IsValid())
		{
#if CSV_PROFILER
			if (!FCsvProfiler::Get()->IsCapturing() && !FCsvProfiler::Get()->IsWritingFile())
			{
				FCsvProfiler::Get()->BeginCapture(-1, FPaths::ConvertRelativePathToFull(
					FPaths::ProjectSavedDir() / TEXT("Automation") / Fixture.CaptureFolder), TEXT("resolved-cpu.csv"));
				bResolvedCsvOwned = true;
			}
			else Test->AddWarning(TEXT("An unrelated CSV capture is active; this test will not replace it"));
#endif
			AppliedViewportProbe = MakeShared<FAppliedViewportProbe, ESPMode::ThreadSafe>();
			IRendererModule* Renderer = &GetRendererModule();
			const auto Probe = AppliedViewportProbe;
			ENQUEUE_RENDER_COMMAND(APSStartResolvedViewportProbe)([Renderer, Probe](FRHICommandListImmediate&)
			{
				Probe->Handle = Renderer->RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateLambda(
					[Probe](FPostOpaqueRenderParameters& Parameters)
					{
						Probe->PackedSize.Store((uint64(uint32(Parameters.ViewportRect.Width())) << 32)
							| uint32(Parameters.ViewportRect.Height()));
					}));
			});
			// One initialization fence, before any timed phase starts.
			FlushRenderingCommands();
		}
	}

	void BeginResolvedFramePhase(const TCHAR* Name, const bool bGeometry, const bool bMeasureFromStart = false)
	{
		EnsureResolvedFrameProbe();
		ResolvedFramePhases.Add({Name, FPlatformTime::Seconds(), bGeometry, {}});
		ResolvedFramePhases.Last().bMeasureFromStart = bMeasureFromStart;
		ResolvedFramePhases.Last().InitialPairCapacity = Generator->GetContinuousPreviewResolvedStarCount()
			+ Generator->GetContinuousPreviewResolvedStarPoolCount();
		ResolvedFramePhases.Last().InitialAllocations = Generator->GetContinuousPreviewResolvedStarAllocationCount();
		ResolvedFramePhases.Last().InitialReuses = Generator->GetContinuousPreviewResolvedStarReuseCount();
		ResolvedFramePhases.Last().InitialPreparations = Generator->GetContinuousPreviewResolvedStarPreparationCount();
		if (ResolvedFramePhases.Last().Name == TEXT("00-First-Host-Arrival"))
		{
			TArray<UProceduralMeshComponent*> Globes;
			Generator->GetComponents(Globes);
			for (UProceduralMeshComponent* Globe : Globes)
				ResolvedFramePhases.Last().InitialGlobeVisibility.Add(Globe,
					uint8(Globe->IsVisible()) | (uint8(Globe->bHiddenInGame) << 1));
		}
		const bool bProbeDirtyStacks = FParse::Param(FCommandLine::Get(), TEXT("APSProbeGlobeDirtyStacks"));
		ResolvedGlobeDirtyHandle = UActorComponent::MarkRenderStateDirtyEvent.AddLambda([this, bProbeDirtyStacks](UActorComponent& Component)
		{
			if (IsInGameThread() && Generator.IsValid() && Component.GetOwner() == Generator.Get()
				&& Component.IsA<UProceduralMeshComponent>() && !ResolvedFramePhases.IsEmpty())
			{
				FResolvedFramePhase& Phase = ResolvedFramePhases.Last();
				++Phase.GlobeRenderStateDirties;
				UProceduralMeshComponent* Globe = CastChecked<UProceduralMeshComponent>(&Component);
				if (const uint8* Initial = Phase.InitialGlobeVisibility.Find(Globe))
					Phase.GlobeVisibilityChanges += *Initial != (uint8(Globe->IsVisible()) | (uint8(Globe->bHiddenInGame) << 1));
				// Explicit causal probe only: symbol resolution perturbs frame timings.
				if (bProbeDirtyStacks && Phase.GlobeDirtyStacks.Num() < 8)
				{
					ANSICHAR Stack[8192] = {};
					FPlatformStackWalk::StackWalkAndDump(Stack, UE_ARRAY_COUNT(Stack), 2);
					Phase.GlobeDirtyStacks.Add(FString::Printf(TEXT("Globe dirty stack %s frame=%llu component=%s:\n%s"),
						*Phase.Name, GFrameCounter, *Component.GetName(), ANSI_TO_TCHAR(Stack)));
				}
			}
		});
		ResolvedFrameClusterView = GetClusterView();
		ResolvedFrameHandle = FCoreDelegates::OnEndFrame.AddLambda([this]()
		{
			if (!Generator.IsValid() || ResolvedFramePhases.IsEmpty()) return;
			APlayerController* Controller = Generator->GetWorld()->GetFirstPlayerController();
			FResolvedFramePhase& Phase = ResolvedFramePhases.Last();
			FResolvedFrameSample Sample;
			Sample.Frame = GFrameCounter;
			CSV_CUSTOM_STAT(APSPreview, EngineFrame, int32(GFrameCounter), ECsvCustomStatOp::Set);
			Sample.Seconds = FPlatformTime::Seconds() - Phase.Started;
			Sample.FrameMs = FApp::GetDeltaTime() * 1000.0;
			Sample.GameMs = FPlatformTime::ToMilliseconds(GGameThreadTime);
			Sample.RenderMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);
			Sample.RhiMs = FPlatformTime::ToMilliseconds(GRHIThreadTime);
			Sample.GpuMs = FPlatformTime::ToMilliseconds(GGPUFrameTime);
			const uint64 PackedSize = AppliedViewportProbe->PackedSize.Load();
			Sample.SceneSize = FIntPoint(int32(PackedSize >> 32), int32(PackedSize & 0xffffffff));
			Sample.ProxyCount = Generator->GetContinuousPreviewResolvedStarCount();
			Sample.PoolCount = Generator->GetContinuousPreviewResolvedStarPoolCount();
			Sample.Allocations = Generator->GetContinuousPreviewResolvedStarAllocationCount();
			Sample.Reuses = Generator->GetContinuousPreviewResolvedStarReuseCount();
			Sample.Preparations = Generator->GetContinuousPreviewResolvedStarPreparationCount();
			Sample.bPoolHidden = Generator->IsContinuousResolvedStarPoolHidden();
			for (const auto& Globe : Phase.InitialGlobeVisibility)
				Sample.bCommittedGlobesStable &= Globe.Key.IsValid()
					&& Globe.Value == (uint8(Globe.Key->IsVisible()) | (uint8(Globe.Key->bHiddenInGame) << 1));
			const auto& Frame = Generator->GetContinuousPreviewFrame();
			Sample.DistanceCm = (ResolvedNeighbor.CenterCm - Frame.ObserverCm).Size();
			const double PixelTangent = GetPixelTangent(Controller);
			Sample.Pixels = ResolvedNeighbor.RadiusCm / (FMath::Max(Sample.DistanceCm, 1.0) * PixelTangent);
			UStaticMeshComponent* Surface = Generator->GetContinuousPreviewResolvedStarMesh(ResolvedNeighbor.SourceInstanceIndex);
			Sample.bSurface = IsValid(Surface) && Surface->IsVisible() && !Surface->bHiddenInGame;
			if (Phase.bGeometry)
			{
				FAPSPreviewProjectedSphere Expected;
				Sample.bGeometry = Frame.ProjectSphere(ResolvedNeighbor.CenterCm, ResolvedNeighbor.RadiusCm, Expected);
				// These flights happen BEFORE the target's first visit: no resident
				// actor may mask the angular-LOD creation/retirement behavior.
				if (Surface)
					Sample.bGeometry &= Surface->Bounds.Origin.Equals(Expected.Center, 1.0)
						&& FMath::IsNearlyEqual(Surface->Bounds.BoxExtent.GetMax() / FMath::Max(Surface->Bounds.Origin.Size(), 1.0e-12),
							ResolvedNeighbor.RadiusCm / Sample.DistanceCm, 1.0e-7);
				FVector2D Screen;
				int32 Width = 0, Height = 0;
				Controller->GetViewportSize(Width, Height);
				const bool bCenterOnScreen = Controller->ProjectWorldLocationToScreen(Expected.Center, Screen, true)
					&& Screen.X >= 0.0 && Screen.X < Width && Screen.Y >= 0.0 && Screen.Y < Height;
				if (Sample.Pixels >= 0.3 && bCenterOnScreen) Sample.bGeometry &= Sample.bSurface;
				UInstancedStaticMeshComponent* Points = ResolvedFrameClusterView.Get();
				FTransform Glyph;
				Sample.bGlyph = Points && Points->GetInstanceTransform(ResolvedNeighbor.SourceInstanceIndex, Glyph, true);
				if (Sample.bGlyph)
				{
					const double ExpectedRadius = Sample.bSurface
						? Expected.Center.Size() * PixelTangent * 2.2 * (1.0 - FMath::Clamp((Sample.Pixels - 0.3) / 0.7, 0.0, 1.0))
						: FMath::Max(Expected.Radius, Expected.Center.Size() * PixelTangent * 2.2);
					const double ActualRadius = Glyph.GetScale3D().GetAbsMax() * Points->GetStaticMesh()->GetBounds().BoxExtent.GetMax();
					Sample.bGlyph = Glyph.GetLocation().Equals(Expected.Center, 1.0)
						&& FMath::IsNearlyEqual(ActualRadius / FMath::Max(Expected.Center.Size(), 1.0e-12),
							ExpectedRadius / FMath::Max(Expected.Center.Size(), 1.0e-12), 1.0e-7);
				}
			}
			Phase.Samples.Add(Sample);
		});
	}

	void FinishResolvedFramePhase()
	{
		UActorComponent::MarkRenderStateDirtyEvent.Remove(ResolvedGlobeDirtyHandle);
		ResolvedGlobeDirtyHandle.Reset();
		FCoreDelegates::OnEndFrame.Remove(ResolvedFrameHandle);
		ResolvedFrameHandle.Reset();
	}

	void ReportResolvedFramePhases()
	{
		// Disk I/O, report formatting and assertions occur after all timed flights.
		Test->AddInfo(FString::Printf(TEXT("First host focus synchronous dispatch: %.3fms; catalog search/profiler setup completed before this command"), FirstHostDispatchMs));
		Test->AddInfo(FString::Printf(TEXT("First host focus synchronous committed-globe dirties: %d"), FirstHostDispatchGlobeDirties));
		Test->TestEqual(TEXT("The first focus command itself does not recreate committed globe render state"), FirstHostDispatchGlobeDirties, 0);
		for (const FResolvedFramePhase& Phase : ResolvedFramePhases)
		{
			TArray<double> FrameMs, GameMs, RenderMs, GpuMs;
			TSet<FIntPoint> SceneSizes;
			int32 MaxProxies = 0, InvalidGeometry = 0, InvalidGlyphs = 0, BelowPoint = 0, Resolved = 0;
			int32 MaxPool = 0, InvalidPoolFrames = 0, UnstableGlobeFrames = 0, MaxPreparedPerFrame = 0;
			bool bConsecutive = true;
			FString Csv = TEXT("frame,elapsed_seconds,frame_ms,game_ms,render_ms,rhi_ms,gpu_ms,scene_x,scene_y,resolved_views,target_radius_px,target_distance_cm,target_surface,geometry_ok,glyph_ok,pooled_pairs,allocations,reuses,pool_hidden,committed_globes_stable,prepared_pairs\n");
			for (int32 Index = 0; Index < Phase.Samples.Num(); ++Index)
			{
				const FResolvedFrameSample& Sample = Phase.Samples[Index];
				if (Index > 0 && Sample.Frame != Phase.Samples[Index - 1].Frame + 1) bConsecutive = false;
				if (Phase.bGeometry || Phase.bMeasureFromStart || Sample.Seconds >= 1.0)
				{
					FrameMs.Add(Sample.FrameMs); GameMs.Add(Sample.GameMs);
					RenderMs.Add(Sample.RenderMs); GpuMs.Add(Sample.GpuMs);
					SceneSizes.Add(Sample.SceneSize);
					MaxProxies = FMath::Max(MaxProxies, Sample.ProxyCount);
				}
				InvalidGeometry += !Sample.bGeometry; InvalidGlyphs += !Sample.bGlyph;
				MaxPool = FMath::Max(MaxPool, Sample.PoolCount);
				InvalidPoolFrames += !Sample.bPoolHidden;
				UnstableGlobeFrames += !Sample.bCommittedGlobesStable;
				MaxPreparedPerFrame = FMath::Max(MaxPreparedPerFrame, Sample.Preparations
					- (Index > 0 ? Phase.Samples[Index - 1].Preparations : Phase.InitialPreparations));
				BelowPoint += Sample.Pixels < 0.1; Resolved += Sample.Pixels >= 1.0;
				Csv += FString::Printf(TEXT("%llu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%.12g,%.17g,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"),
					Sample.Frame, Sample.Seconds, Sample.FrameMs, Sample.GameMs, Sample.RenderMs, Sample.RhiMs, Sample.GpuMs,
					Sample.SceneSize.X, Sample.SceneSize.Y, Sample.ProxyCount, Sample.Pixels, Sample.DistanceCm,
					Sample.bSurface, Sample.bGeometry, Sample.bGlyph, Sample.PoolCount, Sample.Allocations, Sample.Reuses, Sample.bPoolHidden, Sample.bCommittedGlobesStable, Sample.Preparations);
			}
			Test->TestTrue(TEXT("Resolved diagnostic records consecutive game frames: ") + Phase.Name, bConsecutive && Phase.Samples.Num() > 1);
			Test->TestEqual(TEXT("Inactive stellar pairs are hidden in every captured frame: ") + Phase.Name, InvalidPoolFrames, 0);
			Test->TestTrue(TEXT("Inactive stellar pair pool remains bounded: ") + Phase.Name,
				MaxPool <= AAstroGenerator::ContinuousResolvedStarPoolLimit);
			Test->TestTrue(TEXT("Speculative stellar work obeys its per-frame pair bound: ") + Phase.Name,
				MaxPreparedPerFrame <= AAstroGenerator::ContinuousResolvedStarPreparePairLimit);
			if (!Phase.Samples.IsEmpty())
			{
				const int32 NewPairs = Phase.Samples.Last().Allocations - Phase.InitialAllocations;
				const int32 ReusedPairs = Phase.Samples.Last().Reuses - Phase.InitialReuses;
				const int32 PreparedPairs = Phase.Samples.Last().Preparations - Phase.InitialPreparations;
				const int32 DemandPairs = NewPairs - PreparedPairs;
				Test->AddInfo(FString::Printf(TEXT("Resolved pool %s: initialCapacity=%d peakActive=%d peakInactive=%d allocated=%d reused=%d prepared=%d demandAllocated=%d maxPreparedPerFrame=%d"),
					*Phase.Name, Phase.InitialPairCapacity, MaxProxies, MaxPool, NewPairs, ReusedPairs, PreparedPairs, DemandPairs, MaxPreparedPerFrame));
				if (Phase.Name == TEXT("00-First-Host-Arrival")
					&& IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ResolvedStarPreparation"))->GetInt() != 0)
				{
					Test->TestTrue(TEXT("Cold first flight exercises advance preparation from an empty pool"), Phase.InitialPairCapacity == 0 && PreparedPairs > 0);
					Test->TestEqual(TEXT("Advance preparation covers first-flight visible demand without burst allocation"), DemandPairs, 0);
				}
				if (Phase.bGeometry && MaxProxies <= Phase.InitialPairCapacity
					&& Phase.InitialPairCapacity <= AAstroGenerator::ContinuousResolvedStarPoolLimit)
					Test->TestEqual(TEXT("Retained capacity covers flight without any demand-time stellar pair allocation: ") + Phase.Name, DemandPairs, 0);
				if (Phase.Name == TEXT("06-Resolved-Arrival"))
					Test->TestTrue(TEXT("Real return flight exercises pooled stellar pair reuse"), ReusedPairs > 0);
			}
			Test->TestTrue(TEXT("Resolved diagnostic observed a positive actual scene rectangle: ") + Phase.Name,
				!SceneSizes.IsEmpty() && !SceneSizes.Contains(FIntPoint::ZeroValue));
			Test->AddInfo(FString::Printf(TEXT("Committed globe render-state dirties %s: %d"),
				*Phase.Name, Phase.GlobeRenderStateDirties));
			for (const FString& Stack : Phase.GlobeDirtyStacks) Test->AddInfo(Stack);
			if (Phase.Name == TEXT("00-First-Host-Arrival"))
			{
				// Cold ocean shader completion also legitimately recreates a scene proxy.
				// Keep its raw count/timing, but test navigation-caused hiding separately,
				// including transient hide/show within a single game frame.
				Test->TestTrue(TEXT("First flight tracks pre-existing committed globe buffers"), !Phase.InitialGlobeVisibility.IsEmpty());
				Test->TestEqual(TEXT("First flight never toggles committed globe visibility, even within a frame"), Phase.GlobeVisibilityChanges, 0);
				Test->TestEqual(TEXT("All committed globe buffers retain their visibility on every first-flight frame"), UnstableGlobeFrames, 0);
			}
			if (Phase.bGeometry)
				Test->TestEqual(TEXT("Camera-only flight does not recreate committed globe render state: ") + Phase.Name,
					Phase.GlobeRenderStateDirties, 0);
			if (Phase.bGeometry)
			{
				Test->TestEqual(TEXT("Every moving resolved surface preserves its center and physical angular radius: ") + Phase.Name, InvalidGeometry, 0);
				Test->TestEqual(TEXT("Every moving point obeys the shared angular handoff: ") + Phase.Name, InvalidGlyphs, 0);
				Test->TestTrue(TEXT("The real flight crosses both unresolved and resolved scales: ") + Phase.Name, BelowPoint > 0 && Resolved > 0);
			}
			const auto Stats = [](TArray<double> Values)
			{
				if (Values.IsEmpty()) return FString(TEXT("unavailable"));
				double Sum = 0.0; for (const double Value : Values) Sum += Value;
				Values.Sort();
				return FString::Printf(TEXT("mean=%.3f median=%.3f p95=%.3f max=%.3f"), Sum / Values.Num(),
					Values[Values.Num() / 2], Values[FMath::FloorToInt((Values.Num() - 1) * 0.95)], Values.Last());
			};
			FString Sizes; for (const FIntPoint& Size : SceneSizes) Sizes += Size.ToString() + TEXT(";");
			Test->AddInfo(FString::Printf(TEXT("Resolved frame phase %s: samples=%d measured=%d maxProxyPairs=%d opaqueRects=%s frameMs{%s} gameMs{%s} renderMs{%s} gpuMs{%s}; no screenshot readback, engine last-frame timing counters, offscreen only"),
				*Phase.Name, Phase.Samples.Num(), FrameMs.Num(), MaxProxies, *Sizes, *Stats(FrameMs), *Stats(GameMs), *Stats(RenderMs), *Stats(GpuMs)));
			Test->TestTrue(TEXT("Resolved per-frame geometry and timing evidence is saved"), FFileHelper::SaveStringToFile(Csv,
				*(FPaths::ProjectSavedDir() / TEXT("Automation") / Fixture.CaptureFolder / (Phase.Name + TEXT("-frames.csv")))));
		}
		ClearResolvedFrameProbe();
	}

	void ClearStarFieldCapture()
	{
		if (StarFieldProcessedHandle.IsValid())
		{
			FScreenshotRequest::OnScreenshotRequestProcessed().Remove(StarFieldProcessedHandle);
			StarFieldProcessedHandle.Reset();
		}
		if (StarFieldCaptureHandle.IsValid())
		{
			UGameViewportClient::OnScreenshotCaptured().Remove(StarFieldCaptureHandle);
			StarFieldCaptureHandle.Reset();
			if (!bStarFieldCaptureReady) FScreenshotRequest::Reset();
			GAreScreenMessagesEnabled = bStarFieldPreviousScreenMessages;
		}
		bStarFieldCaptureReady = false;
		CapturedStarFieldPixels.Reset();
	}

	bool CaptureStarField(const FString& Name, TArray<FColor>& Pixels, FIntVector& Size, FStarFieldSeries& Series)
	{
		if (!StarFieldCaptureHandle.IsValid())
		{
			if (FScreenshotRequest::IsScreenshotRequested())
			{ Test->AddError(TEXT("An unrelated screenshot request is pending")); Step = 99; return false; }
			CapturedStarSeries = FStarFieldSeries{};
			bStarFieldPreviousScreenMessages = GAreScreenMessagesEnabled;
			CapturedStarSeries.PixelSums.Init(FVector3d::ZeroVector, 17 * 17);
			FString Settings;
			for (const TCHAR* Key : { TEXT("r.AntiAliasingMethod"), TEXT("r.ScreenPercentage"),
				TEXT("r.TemporalAASamples"), TEXT("r.TSR.History.SampleCount"), TEXT("r.EyeAdaptationQuality"),
				TEXT("r.DefaultFeature.AutoExposure"), TEXT("sg.AntiAliasingQuality"), TEXT("sg.PostProcessQuality") })
			{
				const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Key);
				Settings += FString::Printf(TEXT(" %s=%s"), Key, Variable ? *Variable->GetString() : TEXT("unavailable"));
			}
			Test->AddInfo(FString::Printf(TEXT("Starfield series %s: %d consecutive scene frames; cvars (not final postprocess overrides):%s"),
				*Name, FStarFieldSeries::SampleCount, *Settings));
			StarFieldCaptureHandle = UGameViewportClient::OnScreenshotCaptured().AddLambda(
				[this](int32 Width, int32 Height, const TArray<FColor>& Bitmap)
				{
					CapturedStarFieldSize = FIntVector(Width, Height, 0);
					CapturedStarSeries.Size = CapturedStarFieldSize;
					FVector3d ColorSum = FVector3d::ZeroVector;
					const FIntPoint Center(FMath::RoundToInt(FirstVisitScreen.X), FMath::RoundToInt(FirstVisitScreen.Y));
					for (int32 DY = -8; DY <= 8; ++DY)
						for (int32 DX = -8; DX <= 8; ++DX)
						{
							const int32 X = Center.X + DX, Y = Center.Y + DY;
							if (X < 0 || X >= Width || Y < 0 || Y >= Height || !Bitmap.IsValidIndex(Y * Width + X)) continue;
							const FColor& Pixel = Bitmap[Y * Width + X];
							const FVector3d Color(Pixel.R, Pixel.G, Pixel.B);
							CapturedStarSeries.PixelSums[(DY + 8) * 17 + DX + 8] += Color;
							ColorSum += Color;
						}
					CapturedStarSeries.ColorSum += ColorSum;
					CapturedStarSeries.Energies.Add(ColorSum.X + ColorSum.Y + ColorSum.Z);
					CapturedStarSeries.Frames.Add(GFrameCounter);
					CapturedStarSeries.Times.Add(FPlatformTime::Seconds());
					bStarFieldCaptureReady = CapturedStarSeries.Energies.Num() >= FStarFieldSeries::SampleCount;
					if (bStarFieldCaptureReady) CapturedStarFieldPixels = Bitmap;
				});
			// The engine resets the completed request before this callback. Requeue
			// here, not in OnScreenshotCaptured, to sample successive rendered frames
			// rather than a latent-command stride that can alias the jitter period.
			StarFieldProcessedHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddLambda(
				[this, Name]()
				{
					if (bStarFieldCaptureReady) return;
					// The processed delegate precedes the engine's message-state
					// restoration; preserve the original value for the next request.
					GAreScreenMessagesEnabled = bStarFieldPreviousScreenMessages;
					FScreenshotRequest::RequestScreenshot(Name, false, false);
				});
			StarFieldCaptureStart = FPlatformTime::Seconds();
			// Capture in the engine's next scene-render screenshot phase, before
			// Slate composition. Hiding a widget during an immediate Slate snapshot
			// can reuse its cached paint and leaves labels in the measured pixels.
			FScreenshotRequest::RequestScreenshot(Name, false, false);
			return false;
		}
		if (!bStarFieldCaptureReady)
		{
			if (FPlatformTime::Seconds() - StarFieldCaptureStart > 20.0)
			{ Test->AddError(TEXT("Scene-only 64-frame series did not complete within twenty seconds")); Step = 99; }
			return false;
		}
		Pixels = MoveTemp(CapturedStarFieldPixels);
		Size = CapturedStarFieldSize;
		Series = MoveTemp(CapturedStarSeries);
		ClearStarFieldCapture();
		if (!Test->TestTrue(TEXT("First-visit comparison captures actual scene pixels"),
			Size.X > 0 && Size.Y > 0 && Pixels.Num() == Size.X * Size.Y)) { Step = 99; return false; }
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation") / Fixture.CaptureFolder;
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
		Test->TestTrue(TEXT("First-visit star field evidence is saved"), FFileHelper::SaveArrayToFile(Png, *(Directory / (Name + TEXT("-starfield.png")))));
		Test->TestEqual(TEXT("Photometric series has the full temporal sample count"), Series.Energies.Num(), FStarFieldSeries::SampleCount);
		bool bConsecutive = true;
		FString Csv = TEXT("frame,elapsed_seconds,patch_rgb_sum\n");
		for (int32 Index = 0; Index < Series.Energies.Num(); ++Index)
		{
			if (Index > 0 && Series.Frames[Index] != Series.Frames[Index - 1] + 1) bConsecutive = false;
			Csv += FString::Printf(TEXT("%llu,%.6f,%.0f\n"), Series.Frames[Index], Series.Times[Index] - Series.Times[0], Series.Energies[Index]);
		}
		Test->TestTrue(TEXT("Photometric samples span consecutive engine frames, not a jitter-aliased stride"), bConsecutive);
		Test->TestTrue(TEXT("Raw consecutive-frame photometry is saved"),
			FFileHelper::SaveStringToFile(Csv, *(Directory / (Name + TEXT("-photometry.csv")))));
		return true;
	}

	void CompareStarFieldSeries(const TCHAR* Label, const FStarFieldSeries& Before, const FStarFieldSeries& After)
	{
		if (!Test->TestEqual(TEXT("Photometric series have matching resolution"), Before.Size, After.Size)
			|| Before.Energies.IsEmpty() || After.Energies.IsEmpty()) return;
		const auto Stats = [](const TArray<double>& Values)
		{
			double Mean = 0.0, Variance = 0.0;
			for (const double Value : Values) Mean += Value / Values.Num();
			for (const double Value : Values) Variance += FMath::Square(Value - Mean) / Values.Num();
			return FVector2D(Mean, FMath::Sqrt(Variance));
		};
		const FVector2D A = Stats(Before.Energies), B = Stats(After.Energies);
		double AverageImageL1 = 0.0;
		for (int32 Index = 0; Index < Before.PixelSums.Num(); ++Index)
		{
			const FVector3d Delta = (Before.PixelSums[Index] / Before.Energies.Num()
				- After.PixelSums[Index] / After.Energies.Num()).GetAbs();
			AverageImageL1 += Delta.X + Delta.Y + Delta.Z;
		}
		const FVector3d ColorBefore = Before.ColorSum / FMath::Max(Before.ColorSum.X + Before.ColorSum.Y + Before.ColorSum.Z, 1.0);
		const FVector3d ColorAfter = After.ColorSum / FMath::Max(After.ColorSum.X + After.ColorSum.Y + After.ColorSum.Z, 1.0);
		Test->AddInfo(FString::Printf(TEXT("Starfield 64-frame comparison (%s): meanRgb=%.3f -> %.3f stddev=%.3f/%.3f meanDelta=%.6f averagedImageL1=%.6f rgbFraction=%s -> %s duration=%.3f/%.3fs (diagnostic, correlated temporal samples; not independent-sample confidence)"),
			Label, A.X, B.X, A.Y, B.Y, (B.X - A.X) / FMath::Max(A.X, 1.0),
			AverageImageL1 / FMath::Max(A.X, 1.0), *ColorBefore.ToString(), *ColorAfter.ToString(),
			Before.Times.Last() - Before.Times[0], After.Times.Last() - After.Times[0]));
	}

	void CompareStarFieldPatch(const TCHAR* Label, const TArray<FColor>& AfterPixels, const FIntVector& AfterSize)
	{
		if (!Test->TestEqual(TEXT("First-visit image pair uses the same viewport resolution"), AfterSize, FirstVisitSize)
			|| AfterPixels.Num() != FirstVisitPixels.Num() || AfterPixels.IsEmpty()) return;
		double BeforeEnergy = 0.0, AfterEnergy = 0.0, Delta = 0.0;
		int32 Samples = 0;
		const FIntPoint Center(FMath::RoundToInt(FirstVisitScreen.X), FMath::RoundToInt(FirstVisitScreen.Y));
		for (int32 Y = FMath::Max(0, Center.Y - 8); Y <= FMath::Min(AfterSize.Y - 1, Center.Y + 8); ++Y)
			for (int32 X = FMath::Max(0, Center.X - 8); X <= FMath::Min(AfterSize.X - 1, Center.X + 8); ++X)
			{
				const FColor& A = FirstVisitPixels[Y * AfterSize.X + X];
				const FColor& B = AfterPixels[Y * AfterSize.X + X];
				BeforeEnergy += A.R + A.G + A.B;
				AfterEnergy += B.R + B.G + B.B;
				Delta += FMath::Abs(static_cast<int32>(A.R) - B.R) + FMath::Abs(static_cast<int32>(A.G) - B.G) + FMath::Abs(static_cast<int32>(A.B) - B.B);
				++Samples;
			}
		Test->TestTrue(TEXT("First-visit photometric patch is inside the rendered image"), Samples > 0);
		Test->TestTrue(TEXT("First-visit photometric patch contains visible stellar light before and after"), BeforeEnergy > 0.0 && AfterEnergy > 0.0);
		Test->AddInfo(FString::Printf(TEXT("First-visit star patch (%s): center=(%d,%d) pixels=%d beforeRgbSum=%.0f afterRgbSum=%.0f normalizedL1=%.6f (scene-only diagnostic; temporal rendering not frozen)"),
			Label, Center.X, Center.Y, Samples, BeforeEnergy, AfterEnergy, Delta / FMath::Max(BeforeEnergy, 1.0)));
	}

	bool SampleCacheFrames(const double Now, const TCHAR* Label)
	{
		if (CacheSampleStart == 0.0) CacheSampleStart = Now;
		if (Now - CacheSampleStart < 8.0)
		{
			CacheFrameMs.Add(FApp::GetDeltaTime() * 1000.0);
			return false;
		}
		CacheFrameMs.Sort();
		double Sum = 0.0;
		for (const double Ms : CacheFrameMs) Sum += Ms;
		const double Mean = Sum / FMath::Max(1, CacheFrameMs.Num());
		const double P95 = CacheFrameMs.IsEmpty() ? 0.0 : CacheFrameMs[FMath::Min(CacheFrameMs.Num() - 1, FMath::FloorToInt(CacheFrameMs.Num() * 0.95))];
		Test->AddInfo(FString::Printf(TEXT("Cache Planet sample (%s): frames=%d mean=%.3fms p95=%.3fms residentRemote=%d starPoints=%d (offscreen, not interactive acceptance)"),
			Label, CacheFrameMs.Num(), Mean, P95, Generator->GetContinuousPreviewResidentSystemCount(), Generator->GetContinuousPreviewClusterPoints().Num()));
		CacheSampleStart = 0.0;
		CacheFrameMs.Reset();
		return true;
	}

	void BeginCacheVisit(UWorldGenerationViewModel* VM, const double Now)
	{
		const FGuid Id = CacheVisitIds[CacheVisitIndex % CacheVisitIds.Num()];
		FAPSCanonicalClusterSystemAddress Address;
		if (!Test->TestTrue(TEXT("Cache visit resolves its immutable catalog address"), Generator->ResolveCanonicalClusterSystemAddress(Id, Address))) return;
		Test->TestTrue(TEXT("Cache visit can focus its system"), VM->FocusPreviewClusterSystem(Address.InstanceIndex));
		if (const AStarSystem* System = Generator->GetContinuousPreviewActiveSystem())
		{
			bool bFocusedPlanet = false;
			for (const AStar* Star : System->GetStars())
			{
				for (APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
				{
					if (Planet->PlanetType == EPlanetType::GasGiant || Planet->PlanetType == EPlanetType::HotGiant
						|| Planet->PlanetType == EPlanetType::IceGiant) continue;
					Test->TestTrue(TEXT("Cache churn also opens detailed planetary globes"), VM->FocusPreviewBody(Planet));
					bFocusedPlanet = true;
					break;
				}
				if (bFocusedPlanet) break;
			}
		}
		StepStart = Now;
	}

	double LogOriginalCacheResidency(UWorld* World, APlayerController* Controller)
	{
		return InspectSystemAngularRadius(World, CacheOriginalSystem.Get(), TEXT("original"), true);
	}

	double GetPixelTangent(APlayerController* Controller) const
	{
		int32 Width = 0, Height = 0;
		Controller->GetViewportSize(Width, Height);
		return 2.0 * FMath::Tan(FMath::DegreesToRadians(
			Generator->GetPreviewCameraComponent()->FieldOfView * 0.5)) / FMath::Max(Width, 320);
	}

	double InspectSystemAngularRadius(UWorld* World, const AStarSystem* System, const TCHAR* Context, bool bLog)
	{
		if (!IsValid(System)) return 0.0;
		const double PixelTangent = GetPixelTangent(World->GetFirstPlayerController());
		double LargestPixelRadius = 0.0;
		double LargestRadiusKm = 0.0, LargestDistanceKm = 0.0, LargestOffsetKm = 0.0;
		FVector LargestCenter = FVector::ZeroVector;
		FString LargestBody;
		FString LargestType;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->IsAttachedTo(System)) continue;
			const AStar* Star = Cast<AStar>(*It);
			const APlanetaryBody* Planet = Cast<APlanetaryBody>(*It);
			const double RadiusKm = Star ? Star->RadiusKM : (Planet ? Planet->RadiusKM : 0.0);
			const FVector Center = Generator->GetContinuousPreviewPhysicalPosition(*It);
			const double DistanceCm = (Center - Generator->GetContinuousPreviewFrame().ObserverCm).Size();
			const double PixelRadius = RadiusKm * 1.0e5 / (FMath::Max(DistanceCm, 1.0) * PixelTangent);
			if (PixelRadius > LargestPixelRadius)
			{
				LargestPixelRadius = PixelRadius;
				LargestBody = It->GetName();
				LargestType = Star ? UEnum::GetValueAsString(Star->StellarClass) : UEnum::GetValueAsString(Planet->PlanetType);
				LargestRadiusKm = RadiusKm;
				LargestDistanceKm = DistanceCm / 1.0e5;
				LargestOffsetKm = (It->GetActorLocation() - System->GetActorLocation()).Size() / 1.0e5;
				LargestCenter = Center;
			}
		}
		if (bLog)
		{
			const APlanetaryBody* SurfaceBody = Generator->GetActivePreviewWorldScapeBody();
			Test->AddInfo(FString::Printf(TEXT("Cache angular audit context=%s visit=%d system=%s largestBody=%s type=%s radiusKm=%.17g distanceKm=%.17g systemOffsetKm=%.17g radiusPixels=%.9g centerCm=%s observerCm=%s active=%d surfaceBelongs=%d queueDrained=%d"),
				Context, CacheVisitIndex, *System->StableSystemId.ToString(), *LargestBody, *LargestType,
				LargestRadiusKm, LargestDistanceKm, LargestOffsetKm, LargestPixelRadius, *LargestCenter.ToString(),
				*Generator->GetContinuousPreviewFrame().ObserverCm.ToString(), System == Generator->GetContinuousPreviewActiveSystem(),
				SurfaceBody && SurfaceBody->IsAttachedTo(System), Generator->IsPreviewGlobeFamilyWarmQueueDrained()));
		}
		return LargestPixelRadius;
	}

	void CheckCache(UWorld* World)
	{
		const bool bAboveTarget = Generator->GetContinuousPreviewResidentSystemCount() > AAstroGenerator::ContinuousPreviewSystemCacheLimit;
		// Four is an eviction target, not permission to remove a still-resolved
		// photosphere. At settled views the build queue is drained: every resident
		// above target must be independently justified by current use or geometry.
		Test->TestTrue(TEXT("Settled cache checks run after surface workers drain"), Generator->IsPreviewGlobeFamilyWarmQueueDrained());
		int32 ResidentActors = 0;
		for (TActorIterator<AStarSystem> It(World); It; ++It)
			if (It->IsAttachedTo(Generator.Get()) && *It != Generator->GetPreviewHomeSystem())
			{
				++ResidentActors;
				if (!bAboveTarget) continue;
				const APlanetaryBody* SurfaceBody = Generator->GetActivePreviewWorldScapeBody();
				const AActor* SelectedBody = Generator->GetSelectedPreviewBodyActor();
				const double PixelRadius = InspectSystemAngularRadius(World, *It, TEXT("above-target"), true);
				const bool bInUse = *It == Generator->GetContinuousPreviewActiveSystem()
					|| (SurfaceBody && SurfaceBody->IsAttachedTo(*It)) || (SelectedBody && SelectedBody->IsAttachedTo(*It));
				Test->TestTrue(TEXT("Every above-target resident is actively used or has a physically resolved body"), bInUse || PixelRadius >= 0.3);
			}
		Test->TestEqual(TEXT("Cache bookkeeping equals actual live remote actor trees"), ResidentActors, Generator->GetContinuousPreviewResidentSystemCount());
		Test->TestEqual(TEXT("Cache churn never changes the canonical dataset"), Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash, CacheDatasetHash);
		Test->TestEqual(TEXT("First visits and revisits never add or remove background stars"), Generator->GetContinuousPreviewClusterPoints().Num(), CachePointCountBeforeVisits);
		Test->TestEqual(TEXT("Cache churn never changes home stellar physics"),
			SystemStellarPhysics(World, Generator->GetPreviewHomeSystem()->StableSystemId), UneditedHomePhysics);
		TSet<FString> StarAddresses;
		for (const FAPSContinuousPreviewPoint& Point : Generator->GetContinuousPreviewClusterPoints())
		{
			const FString Address = Point.StableId.ToString() + FString::Printf(TEXT("/S%d"), Point.SystemStarIndex);
			Test->TestFalse(TEXT("Every retained star glyph has one unique stable address"), StarAddresses.Contains(Address));
			StarAddresses.Add(Address);
		}
	}

	void BeginScope(UWorldGenerationViewModel* VM, double Now)
	{
		const FVector PreviousObserver = Generator->GetContinuousPreviewOrbit().ObserverCm();
		VM->SetPreviewFocus(Scopes[ScopeIndex]);
		if (Scopes[ScopeIndex] == EAstroPreviewFocus::Overview)
			Test->TestTrue(TEXT("Overview totals all three home planetary families"),
				VM->GetPreviewScopeSummary().ToString().Contains(TEXT("HOME SYSTEM PLANETS  18")));
		Test->TestTrue(TEXT("Scope switch begins at the actual displayed observer"),
			Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(PreviousObserver, 0.0));
		StepStart = Now;
		bCapturedMidpoint = false;
	}

	void BeginHighCount(UWorld* World, UWorldGenerationViewModel* VM, const double Now)
	{
		if (Fixture.bHighCountRemote)
		{
			int32 Instance = INDEX_NONE;
			for (TActorIterator<AStarCluster> It(World); It && Instance == INDEX_NONE; ++It)
				if (It->IsAttachedTo(Generator.Get()))
					for (const FClusterStarSystemRecord& Record : It->PotentialStarSystems)
						if (!Record.bMaterialized && Record.SystemModel.PotentialPlanetCount > 0)
						{ Instance = Record.InstanceIndex; break; }
			if (!Test->TestTrue(TEXT("High-count route selects a real non-home catalog system"),
				Instance != INDEX_NONE && VM->FocusPreviewClusterSystem(Instance)))
			{ Step = 99; StepStart = Now; return; }
		}
		VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
		AStarSystem* System = Generator->GetContinuousPreviewActiveSystem();
		if (!Test->TestNotNull(TEXT("High-count route has an actual selected hierarchy"), System))
		{ Step = 99; StepStart = Now; return; }
		HighCountSystemId = System->StableSystemId;
		HighCountCatalogBefore = VM->GeneratedWorld->CanonicalStellarDataset;
		HighCountHomeBefore = SystemStellarPhysics(World, HighCountCatalogBefore.HomeStableId);
		for (const AStar* Star : System->GetStars()) HighCountInitialStars.Add(OneStarPhysics(Star));
		Test->AddInfo(FString::Printf(TEXT("High-count system target=%s home=%d originalStars=%d seed=%d; uses real TOTAL PLANETS and STAR SYSTEM TYPE controls"),
			*HighCountSystemId.ToString(), !Fixture.bHighCountRemote, System->GetStars().Num(), System->GenerationSeed));
		VM->SetEnumValue(StaticEnum<EStarType>(), static_cast<int32>(EStarType::MultipleStar));
		VM->SetSelectedSystemPlanetCount(120);
		Step = 70;
		StepStart = Now;
	}

	void CheckHighCountHierarchy(UWorld* World, UWorldGenerationViewModel* VM, const int32 ExpectedTotal)
	{
		AStarSystem* System = Generator->GetContinuousPreviewActiveSystem();
		if (!Test->TestNotNull(TEXT("High-count edit keeps a selected system"), System)) return;
		Test->TestEqual(TEXT("High-count control never redirects to another system"), System->StableSystemId, HighCountSystemId);
		int32 Total = 0;
		TSet<const APlanet*> Members;
		TSet<FString> Addresses;
		bool bHierarchyValid = true, bPrimaryPhysicsStable = true;
		for (int32 Index = 0; Index < System->GetStars().Num(); ++Index)
		{
			const AStar* Star = System->GetStars()[Index];
			if (!IsValid(Star) || !IsValid(Star->PlanetarySystem)) { bHierarchyValid = false; continue; }
			if (HighCountInitialStars.IsValidIndex(Index)) bPrimaryPhysicsStable &= OneStarPhysics(Star) == HighCountInitialStars[Index];
			for (const APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
			{
				++Total;
				if (!IsValid(Planet)) { bHierarchyValid = false; continue; }
				const FString Address = Generator->GetPreviewBodyStableKey(Planet);
				bHierarchyValid &= Planet->ParentStar == Star && Planet->IsAttachedTo(System)
					&& !Members.Contains(Planet) && !Addresses.Contains(Address) && !Address.IsEmpty();
				Members.Add(Planet); Addresses.Add(Address);
			}
		}
		int32 ScenePlanets = 0;
		for (TActorIterator<APlanet> It(World); It; ++It)
			if (It->IsAttachedTo(System)) { ++ScenePlanets; bHierarchyValid &= Members.Contains(*It); }
		Test->TestEqual(TEXT("High-count generated hierarchy honors the exact requested TOTAL"), Total, ExpectedTotal);
		Test->TestEqual(TEXT("High-count control agrees with generated hierarchy"), VM->GetSelectedSystemPlanetCount(), ExpectedTotal);
		Test->TestEqual(TEXT("High-count scene contains no stale or orphaned planets"), ScenePlanets, ExpectedTotal);
		Test->TestTrue(TEXT("Every high-count planet has a unique address and the correct physical parent"), bHierarchyValid);
		Test->TestTrue(TEXT("Changing high planet counts preserves surviving stellar physics and names"), bPrimaryPhysicsStable);
		const FAPSCanonicalStellarDataset& Dataset = VM->GeneratedWorld->CanonicalStellarDataset;
		Test->TestEqual(TEXT("High-count edits retain catalog record count"), Dataset.ClusterRecords.Num(), HighCountCatalogBefore.ClusterRecords.Num());
		bool bAddressesStable = true, bNeighborsStable = true;
		for (int32 Index = 0; Index < FMath::Min(Dataset.ClusterRecords.Num(), HighCountCatalogBefore.ClusterRecords.Num()); ++Index)
		{
			const auto& Current = Dataset.ClusterRecords[Index];
			const auto& Before = HighCountCatalogBefore.ClusterRecords[Index];
			bAddressesStable &= Current.StableId == Before.StableId && Current.ClusterLocalLocation == Before.ClusterLocalLocation;
			bNeighborsStable &= FStarModel::StaticStruct()->CompareScriptStruct(&Current.PrimaryStarModel, &Before.PrimaryStarModel, 0);
			if (Current.StableId != HighCountSystemId)
				bNeighborsStable &= FStarSystemModel::StaticStruct()->CompareScriptStruct(&Current.SystemModel, &Before.SystemModel, 0);
		}
		Test->TestTrue(TEXT("High-count edits retain every catalog address and position"), bAddressesStable);
		Test->TestTrue(TEXT("High-count edits do not reroll neighboring systems or stellar models"), bNeighborsStable);
		if (Fixture.bHighCountRemote)
			Test->TestEqual(TEXT("Remote high-count edits leave home stellar physics untouched"),
				SystemStellarPhysics(World, Dataset.HomeStableId), HighCountHomeBefore);
		Test->AddInfo(FString::Printf(TEXT("High-count hierarchy step=%d system=%s stars=%d planets=%d displayed=%d"),
			Step, *System->StableSystemId.ToString(), System->GetStars().Num(), Total, VM->GetSelectedSystemPlanetCount()));
		CheckGeometry(World);
		CheckHighCountVisibleBodies(World);
	}

	void CheckHighCountVisibleBodies(UWorld* World)
	{
		bool bBodiesValid = true, bNoOverlappingLayers = true;
		int32 VisibleFallbacks = 0, VisibleGlobes = 0;
		int32 InvalidProjections = 0, InvalidMeshes = 0;
		double MaxCenterError = 0.0, MaxRadiusError = 0.0;
		const FAPSContinuousPreviewFrame& Frame = Generator->GetContinuousPreviewFrame();
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			APlanetaryBody* Body = *It;
			if (!Body->IsAttachedTo(Generator.Get())) continue;
			FAPSPreviewProjectedSphere Expected;
			const bool bProjectionValid = Body->RadiusKM > 0.0 && Frame.ProjectSphere(
				Generator->GetContinuousPreviewPhysicalPosition(Body), Body->RadiusKM * 1.0e5, Expected);
			bBodiesValid &= bProjectionValid;
			if (!bProjectionValid)
			{
				if (++InvalidProjections <= 5) Test->AddInfo(FString::Printf(TEXT("High-count invalid projection body=%s address=%s radiusKm=%.9g alternateRadiusKm=%.9g"),
					*Body->GetName(), *Generator->GetPreviewBodyStableKey(Body), Body->RadiusKM, Body->PlanetRadiusKM));
				continue;
			}
			UProceduralMeshComponent* Terrain = Generator->GetPreviewTerrainProxyForBody(Body);
			const bool bGlobeVisible = Terrain && Terrain->IsVisible() && !Terrain->bHiddenInGame;
			VisibleGlobes += bGlobeVisible;
			TInlineComponentArray<UStaticMeshComponent*> Meshes;
			Body->GetComponents(Meshes);
			for (const UStaticMeshComponent* Mesh : Meshes)
			{
				if (!Mesh->IsVisible() || Mesh->bHiddenInGame || Body->IsHidden() || !Mesh->GetStaticMesh()) continue;
				++VisibleFallbacks;
				bNoOverlappingLayers &= !bGlobeVisible;
				const double Radius = Mesh->GetStaticMesh()->GetBounds().BoxExtent.GetMax() * Mesh->GetComponentScale().GetAbsMax();
				const double CenterError = FVector::Distance(Mesh->Bounds.Origin, Expected.Center);
				const double RadiusRatio = Radius / FMath::Max(Expected.Radius, 1.0e-12);
				MaxCenterError = FMath::Max(MaxCenterError, CenterError);
				MaxRadiusError = FMath::Max(MaxRadiusError, FMath::Abs(RadiusRatio - 1.0));
				const bool bMeshValid = Mesh->Bounds.Origin.Equals(Expected.Center, 1.0)
					&& FMath::IsNearlyEqual(RadiusRatio, 1.0, 1.0e-6);
				bBodiesValid &= bMeshValid;
				if (!bMeshValid && ++InvalidMeshes <= 5) Test->AddInfo(FString::Printf(TEXT("High-count mesh mismatch step=%d body=%s address=%s mesh=%s radiusRatio=%.12g centerError=%.12g expectedRadius=%.12g position=%s expected=%s scale=%s"),
					Step, *Body->GetName(), *Generator->GetPreviewBodyStableKey(Body), *Mesh->GetName(), RadiusRatio, CenterError,
					Expected.Radius, *Mesh->Bounds.Origin.ToString(), *Expected.Center.ToString(), *Mesh->GetComponentScale().ToString()));
			}
		}
		Test->TestTrue(TEXT("Every visible planetary fallback has the same physical center/radius as its body, with no oversized spheres"), bBodiesValid);
		Test->TestTrue(TEXT("A retained high-count globe never overlaps a visible backing sphere"), bNoOverlappingLayers);
		Test->TestTrue(TEXT("High-count unused stellar components remain hidden"), Generator->IsContinuousResolvedStarPoolHidden());
		Test->AddInfo(FString::Printf(TEXT("High-count body-layer audit step=%d visibleFallbacks=%d visibleGlobes=%d"), Step, VisibleFallbacks, VisibleGlobes));
		Test->AddInfo(FString::Printf(TEXT("High-count geometry detail step=%d invalidProjections=%d invalidMeshes=%d maxCenterError=%.12g maxRadiusRelativeError=%.12g"),
			Step, InvalidProjections, InvalidMeshes, MaxCenterError, MaxRadiusError));
	}

	bool UpdateHighCount(UWorld* World, UWorldGenerationViewModel* VM, const double Now)
	{
		AStarSystem* System = Generator->GetContinuousPreviewActiveSystem();
		if (!System) { Test->AddError(TEXT("High-count route lost its system")); Step = 99; StepStart = Now; return false; }
		if ((Step == 71 || Step == 75) && !bHighCountMidpoint && Now - StepStart >= 0.15)
		{
			CheckGeometry(World); CheckHighCountVisibleBodies(World);
			Capture(FString::Printf(TEXT("%02d-HighCount-Planet-InMotion"), Step));
			bHighCountMidpoint = true;
		}
		if (Now - StepStart < (Step >= 77 ? 2.0 : 3.0) || !Generator->IsPreviewGlobeFamilyWarmQueueDrained()) return false;
		const int32 Total = Step == 72 ? 1 : Step == 73 ? 0 : 120;
		CheckHighCountHierarchy(World, VM, Total);
		if (Step == 70)
		{
			HighCountMultipleStars = System->GetStars().Num();
			Test->TestTrue(TEXT("Actual Multiple Star control creates its supported 4..6 stars"), HighCountMultipleStars >= 4 && HighCountMultipleStars <= 6);
			Capture(TEXT("01-HighCount-Multiple-120"));
			AStar* LastStar = System->GetStars().IsEmpty() ? nullptr : System->GetStars().Last();
			if (!LastStar || !LastStar->PlanetarySystem || LastStar->PlanetarySystem->PlanetsActorsList.IsEmpty())
			{ Test->AddError(TEXT("High-count hierarchy has no last-family planet to inspect")); Step = 99; StepStart = Now; return false; }
			HighCountRemovedPlanet = LastStar->PlanetarySystem->PlanetsActorsList.Last();
			HighCountPlanetKey = Generator->GetPreviewBodyStableKey(HighCountRemovedPlanet.Get());
			Test->TestTrue(TEXT("Normal menu focuses the last planet of the high-count companion family"), VM->FocusPreviewBody(HighCountRemovedPlanet.Get()));
			bHighCountMidpoint = false;
		}
		else if (Step == 71)
		{
			Test->TestEqual(TEXT("High-count Planet view inspects the requested last-family address"),
				Generator->GetPreviewBodyStableKey(Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor())), HighCountPlanetKey);
			Capture(TEXT("02-HighCount-Last-Planet"));
			VM->SetSelectedSystemPlanetCount(1);
		}
		else if (Step == 72)
		{
			APlanet* Selected = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
			Test->TestFalse(TEXT("High-count removed selected planet no longer exists"), HighCountRemovedPlanet.IsValid());
			Test->TestTrue(TEXT("High-count removal resolves a surviving planet in the same system"), Selected && Selected->ParentStar == System->MainStar);
			Test->TestEqual(TEXT("High-count removal keeps editor and displayed selection synchronized"), VM->GetSelectedPreviewBody(), Generator->GetSelectedPreviewBodyActor());
			Capture(TEXT("03-HighCount-One-Planet-Fallback"));
			VM->SetSelectedSystemPlanetCount(0);
		}
		else if (Step == 73)
		{
			Test->TestEqual(TEXT("Removing all high-count planets moves to their own primary star"), Generator->GetSelectedPreviewBodyActor(), static_cast<AActor*>(System->MainStar));
			Test->TestEqual(TEXT("High-count zero fallback changes scope to Star"), VM->GetPreviewFocus(), EAstroPreviewFocus::HomeStar);
			Test->TestFalse(TEXT("High-count zero does not offer an unrelated home Planet view"), VM->IsPreviewFocusAvailable(EAstroPreviewFocus::HomePlanet));
			Capture(TEXT("04-HighCount-Zero"));
			VM->SetPreviewFocus(EAstroPreviewFocus::HomeSystem);
			VM->SetEnumValue(StaticEnum<EStarType>(), static_cast<int32>(EStarType::SingleStar));
			VM->SetSelectedSystemPlanetCount(120);
		}
		else if (Step == 74)
		{
			Test->TestEqual(TEXT("A single-star system also honors the 120 total control"), System->GetStars().Num(), 1);
			Capture(TEXT("05-HighCount-Single-120"));
			if (!System->MainStar || !System->MainStar->PlanetarySystem || System->MainStar->PlanetarySystem->PlanetsActorsList.IsEmpty())
			{ Test->AddError(TEXT("Single-star high-count hierarchy has no planet")); Step = 99; StepStart = Now; return false; }
			HighCountRemovedPlanet = System->MainStar->PlanetarySystem->PlanetsActorsList.Last();
			Test->TestTrue(TEXT("Single-star high-count route can inspect its last planet"), VM->FocusPreviewBody(HighCountRemovedPlanet.Get()));
			bHighCountMidpoint = false;
		}
		else if (Step == 75)
		{
			VM->SetEnumValue(StaticEnum<EStarType>(), static_cast<int32>(EStarType::MultipleStar));
		}
		else if (Step == 76)
		{
			Test->TestEqual(TEXT("Restoring Multiple Star uses the same deterministic multiplicity"), System->GetStars().Num(), HighCountMultipleStars);
			Test->TestFalse(TEXT("Redistribution removes the now-out-of-range selected orbit"), HighCountRemovedPlanet.IsValid());
			APlanet* Selected = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
			Test->TestTrue(TEXT("Redistribution retains a valid selected planet in the edited system"), Selected && Selected->IsAttachedTo(System));
			HighCountPlanetKey = Generator->GetPreviewBodyStableKey(Selected);
			HighCountPassivePhysics = HierarchyPhysics(System, Generator.Get());
			HighCountPassiveHash = Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash;
			Capture(TEXT("06-HighCount-Multiple-Restored"));
			HighCountScopeIndex = 0;
			VM->SetPreviewFocus(EAstroPreviewFocus::Galaxy);
		}
		else if (Step == 77)
		{
			Test->TestEqual(TEXT("Passive high-count navigation cannot regenerate physical hierarchy"), HierarchyPhysics(System, Generator.Get()), HighCountPassivePhysics);
			Test->TestEqual(TEXT("Passive high-count navigation cannot change canonical world version"),
				Generator->GetCanonicalStellarProjectionDescriptor().CanonicalDatasetHash, HighCountPassiveHash);
			Capture(FString::Printf(TEXT("07-HighCount-Scope-%d"), HighCountScopeIndex));
			constexpr EAstroPreviewFocus Route[] = {EAstroPreviewFocus::StarCluster, EAstroPreviewFocus::HomeSystem,
				EAstroPreviewFocus::HomeStar, EAstroPreviewFocus::HomePlanet};
			if (HighCountScopeIndex < UE_ARRAY_COUNT(Route))
			{
				VM->SetPreviewFocus(Route[HighCountScopeIndex++]);
				StepStart = Now;
				return false;
			}
			Test->TestEqual(TEXT("High-count scope round trip returns to the same planet address"),
				Generator->GetPreviewBodyStableKey(Cast<APlanetaryBody>(Generator->GetSelectedPreviewBodyActor())), HighCountPlanetKey);
			Step = 99; StepStart = Now; return false;
		}
		++Step;
		StepStart = Now;
		return false;
	}

	void CheckGeometry(UWorld* World)
	{
		const FAPSContinuousPreviewFrame& Frame = Generator->GetContinuousPreviewFrame();
		Test->TestTrue(TEXT("Active observer and numerical scale are finite"), Frame.IsValid());
		AStarSystem* System = nullptr;
		for (TActorIterator<AStarSystem> It(World); It; ++It)
			if (It->IsAttachedTo(Generator.Get())) { System = *It; break; }
		if (!System) { Test->AddError(TEXT("No materialized home system")); return; }
		for (TActorIterator<AStar> StarIt(World); StarIt; ++StarIt)
		{
			AStar* Star = *StarIt;
			if (!Star->IsAttachedTo(Generator.Get())) continue;
			if (!IsValid(Star) || !IsValid(Star->StarMesh) || !Star->StarMesh->GetStaticMesh()) continue;
			const double PhysicalDistance = (Generator->GetContinuousPreviewPhysicalPosition(Star) - Frame.ObserverCm).Size();
			const double ActualRadius = Star->StarMesh->GetStaticMesh()->GetBounds().BoxExtent.GetMax()
				* Star->StarMesh->GetComponentScale().GetAbsMax();
			const double RenderDistance = Star->StarMesh->Bounds.Origin.Size();
			Test->TestTrue(TEXT("Rendered photosphere has physical R/D at intermediate and settled views"),
				FMath::IsNearlyEqual(ActualRadius / FMath::Max(RenderDistance, 1.0e-12),
					Star->RadiusKM * 1.0e5 / FMath::Max(PhysicalDistance, 1.0), 1.0e-7));
			FVector LabelCenter;
			Test->TestTrue(TEXT("Star label follows its rendered photosphere center"),
				Generator->GetPreviewPresentationLocation(Star, LabelCenter)
				&& LabelCenter.Equals(Star->StarMesh->Bounds.Origin, 1.0));
		}
		for (TActorIterator<APlanetaryBody> It(World); It; ++It)
		{
			APlanetaryBody* Body = *It;
			if (!Body->IsAttachedTo(Generator.Get())) continue;
			UProceduralMeshComponent* Terrain = Generator->GetPreviewTerrainProxyForBody(Body);
			if (!Terrain || !Terrain->IsVisible()) continue;
			const double PhysicalDistance = (Generator->GetContinuousPreviewPhysicalPosition(Body) - Frame.ObserverCm).Size();
			const double ActualRadius = Terrain->CalcBounds(FTransform::Identity).BoxExtent.GetMax()
				/ FMath::Max(static_cast<double>(Terrain->BoundsScale), 1.0e-12)
				* Terrain->GetComponentScale().GetAbsMax();
			const double RenderDistance = Terrain->GetComponentLocation().Size();
			Test->TestTrue(TEXT("Retained terrain globe preserves the physical body's angular radius"),
				FMath::IsNearlyEqual(ActualRadius / FMath::Max(RenderDistance, 1.0e-12),
					Body->RadiusKM * 1.0e5 / FMath::Max(PhysicalDistance, 1.0), 1.0e-6));
		}
		int32 VisibleCatalogs = 0;
		TInlineComponentArray<UInstancedStaticMeshComponent*> Views;
		Generator->GetComponents(Views);
		for (UInstancedStaticMeshComponent* View : Views)
			if (View->GetName().StartsWith(TEXT("Continuous")) && View->IsVisible() && !View->bHiddenInGame
				&& View->GetInstanceCount() > 0)
			{
				++VisibleCatalogs;
				Test->TestTrue(TEXT("Additive catalog views retain the source's non-Nanite rendering contract"),
					View->bDisallowNanite && View->bForceDisableNanite);
			}
		Test->TestEqual(TEXT("Galaxy and cluster view layers persist in every scope"), VisibleCatalogs, 2);
	}

	void Capture(const FString& Name)
	{
		UGameViewportClient* Client = AutomationCommon::GetAnyGameViewportClient();
		if (!Client || !Client->GetGameViewportWidget().IsValid())
		{ Test->AddError(TEXT("No rendered Slate viewport for coherence capture")); return; }
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation") / Fixture.CaptureFolder;
		IFileManager::Get().MakeDirectory(*Directory, true);
		FString SafeName = Name.Replace(TEXT("::"), TEXT("-"));
		// In standalone RenderOffscreen the scene viewport has no readable backbuffer.
		// Slate owns the composited texture and captures both the real scene and controls.
		TArray<FColor> UiPixels;
		FIntVector UiSize;
		if (!Test->TestTrue(TEXT("The composited generation page can be captured"),
			FSlateApplication::Get().TakeScreenshot(Client->GetGameViewportWidget().ToSharedRef(), UiPixels, UiSize)
			&& UiSize.X > 0 && UiSize.Y > 0 && UiPixels.Num() == UiSize.X * UiSize.Y)) return;
		Test->TestTrue(TEXT("Captured page is not an empty backbuffer"),
			UiPixels.ContainsByPredicate([](const FColor& Pixel) { return Pixel.R > 32 && Pixel.G > 32 && Pixel.B > 32; }));
		TArray64<uint8> UiPng;
		FImageUtils::PNGCompressImageArray(UiSize.X, UiSize.Y, UiPixels, UiPng);
		Test->TestTrue(TEXT("Rendered coherence evidence is saved"),
			FFileHelper::SaveArrayToFile(UiPng, *(Directory / (SafeName + TEXT("-ui.png")))));
	}

	FAutomationTestBase* Test;
	int32 PreviousCvar;
	FRouteFixture Fixture;
	TSharedPtr<FAppliedViewportProbe, ESPMode::ThreadSafe> AppliedViewportProbe;
	FDelegateHandle ResolvedFrameHandle;
	TWeakObjectPtr<UInstancedStaticMeshComponent> ResolvedFrameClusterView;
	TArray<FResolvedFramePhase> ResolvedFramePhases;
	FDelegateHandle ResolvedGlobeDirtyHandle;
	TSharedFuture<FString> ResolvedCsvFuture;
	bool bResolvedCsvOwned{false};
	TArray<FAPSContinuousPreviewPoint> ResolvedCandidates;
	int32 ResolvedInitialHostInstance{INDEX_NONE};
	uint64 ResolvedHostPreparedFrame{0};
	double FirstHostDispatchMs{0.0};
	int32 FirstHostDispatchGlobeDirties{0};
	int32 AfterManualOrbitPreparations{0};
	FGuid HighCountSystemId;
	FAPSCanonicalStellarDataset HighCountCatalogBefore;
	TArray<FString> HighCountInitialStars;
	FString HighCountHomeBefore, HighCountPlanetKey, HighCountPassivePhysics;
	TWeakObjectPtr<APlanet> HighCountRemovedPlanet;
	int32 HighCountMultipleStars{0}, HighCountScopeIndex{0};
	uint32 HighCountPassiveHash{0};
	bool bHighCountMidpoint{false};
	TWeakObjectPtr<AActor> ResolvedReturnBody;
	FAPSContinuousPreviewPoint ResolvedNeighbor;
	FTransform ResolvedPointBefore;
	FTransform ResolvedSurfaceBefore;
	TWeakObjectPtr<UStaticMesh> ResolvedSurfaceAsset;
	TMap<FName, float> ResolvedSurfaceScalars;
	TMap<FName, FLinearColor> ResolvedSurfaceColors;
	FTransform ResolvedCoronaTransform;
	TWeakObjectPtr<UStaticMesh> ResolvedCoronaAsset;
	TWeakObjectPtr<UMaterialInterface> ResolvedCoronaBase;
	TMap<FName, float> ResolvedCoronaScalars;
	TMap<FName, FLinearColor> ResolvedCoronaColors;
	bool bResolvedCoronaVisible{false};
	int32 ResolvedAttempt{0};
	bool bResolvedViewOriented{false};
	double Start;
	double StepStart{0.0};
	int32 Step{0};
	int32 ScopeIndex{0};
	bool bCapturedMidpoint{false};
	uint32 InitialIdentityHash{0};
	uint32 InitialProfileSignature{0};
	TWeakObjectPtr<AAstroGenerator> Generator;
	TWeakObjectPtr<APlanetaryBody> InitialPlanet;
	TWeakObjectPtr<APlanet> RemovedPlanet;
	TWeakObjectPtr<AStarSystem> RemoteSystem;
	TWeakObjectPtr<APlanet> RemotePlanet;
	TWeakObjectPtr<AStarCluster> RemoteCluster;
	double RemoteRadiusKm{0.0};
	FTransform RemotePhysicalTransform;
	FString RemoteSatellites;
	uint32 RemoteProfileSignature{0};
	FString RemotePlanetKey;
	FString BeforeRemoteRebuildStars;
	uint32 BeforeRemoteRebuildDatasetHash{0};
	int32 RemoteDesiredMoons{0};
	int32 RemoteInstance{INDEX_NONE};
	int32 RemoteExpectedStars{0};
	int32 RemoteExpectedPlanets{0};
	FGuid RemoteStableId;
	FAPSCanonicalStellarDataset BeforeStarEditDataset;
	FString EditedStarAddress;
	FString UneditedHomePhysics;
	FString UneditedSiblingPhysics;
	FAPSCanonicalStellarDataset BeforeSystemEditDataset;
	FString RemoteUneditedStellarPhysics;
	FString RemoteUneditedPrimaryPhysics;
	FString StartInspectionKey;
	TWeakObjectPtr<AStarSystem> CacheOriginalSystem;
	TWeakObjectPtr<UProceduralMeshComponent> CacheOriginalTerrain;
	FString CacheOriginalPhysics;
	uint32 CacheOriginalProfile{0};
	uint32 CacheDatasetHash{0};
	TArray<FAPSContinuousPreviewPoint> CacheStarPoints;
	TArray<FGuid> CacheVisitIds;
	int32 CacheVisitIndex{0};
	bool bCacheFarPass{false};
	bool bCacheDeparturePressure{false};
	TArray<TPair<FVector, double>> CacheOriginalBodySpheres;
	int32 CachePointCountAfterFirstPass{0};
	int32 CachePointCountBeforeVisits{0};
	double CacheSampleStart{0.0};
	TArray<double> CacheFrameMs;
	FAPSContinuousPreviewOrbit FirstVisitOrbit;
	FDelegateHandle StarFieldCaptureHandle;
	FDelegateHandle StarFieldProcessedHandle;
	FStarFieldSeries CapturedStarSeries;
	FStarFieldSeries FirstVisitSeries;
	bool bStarFieldCaptureReady{false};
	bool bStarFieldPreviousScreenMessages{true};
	double StarFieldCaptureStart{0.0};
	TArray<FColor> CapturedStarFieldPixels;
	FIntVector CapturedStarFieldSize{FIntVector::ZeroValue};
	TArray<FAPSContinuousPreviewPoint> FirstVisitPoints;
	TArray<FTransform> FirstVisitPointTransforms;
	TArray<float> FirstVisitCustomData;
	TArray<FColor> FirstVisitPixels;
	FIntVector FirstVisitSize{FIntVector::ZeroValue};
	FVector2D FirstVisitScreen{FVector2D::ZeroVector};
	TArray<FTransform> CatalogTransforms;
	TArray<double> WarmFrameMs;
	TMap<FString, FName> InitialNames;
	TMap<FString, FQuat> InitialRotations;
	FString InitialPlanetKey;
	FString InitialSatellitePhysics;
	FString InitialStellarPhysics;
	EPlanetType InitialPlanetType{EPlanetType::Ocean};
	EAstroPreviewFocus Scopes[9] = {EAstroPreviewFocus::HomeStar, EAstroPreviewFocus::HomeSystem,
		EAstroPreviewFocus::StarCluster, EAstroPreviewFocus::Galaxy, EAstroPreviewFocus::Overview, EAstroPreviewFocus::StarCluster,
		EAstroPreviewFocus::HomeSystem, EAstroPreviewFocus::HomeStar, EAstroPreviewFocus::HomePlanet};
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSContinuousPreviewRenderedRoute,
	"APS.Rendered.MainMenu.ContinuousPhysicalRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSContinuousPreviewRenderedRoute::RunTest(const FString& Parameters)
{
	IConsoleVariable* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestNotNull(TEXT("Continuous frame switch exists"), Cvar)) return false;
	const int32 Previous = Cvar->GetInt();
	Cvar->Set(1, ECVF_SetByCode);
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		Cvar->Set(Previous, ECVF_SetByCode);
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(APSContinuousPreviewRendered::FRoute(this, Previous));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSContinuousPreviewRenderedGiantRoute,
	"APS.Rendered.MainMenu.ContinuousGiantRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSContinuousPreviewRenderedGiantRoute::RunTest(const FString& Parameters)
{
	IConsoleVariable* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestNotNull(TEXT("Continuous frame switch exists"), Cvar)) return false;
	const int32 Previous = Cvar->GetInt();
	Cvar->Set(1, ECVF_SetByCode);
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		Cvar->Set(Previous, ECVF_SetByCode);
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(APSContinuousPreviewRendered::FRoute(this, Previous, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSContinuousPreviewResolvedFirstVisit,
	"APS.Rendered.MainMenu.ContinuousResolvedFirstVisit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSContinuousPreviewResolvedFirstVisit::RunTest(const FString& Parameters)
{
	IConsoleVariable* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestNotNull(TEXT("Continuous frame switch exists"), Cvar)) return false;
	const int32 Previous = Cvar->GetInt();
	Cvar->Set(1, ECVF_SetByCode);
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		Cvar->Set(Previous, ECVF_SetByCode);
		return false;
	}
	APSContinuousPreviewRendered::FRouteFixture Fixture{161803, EStellarType::HyperGiant,
		ESpectralClass::M, EStarClusterType::ElongatedStream, TEXT("ContinuousResolvedFirstVisitFrames"), true};
	ADD_LATENT_AUTOMATION_COMMAND(APSContinuousPreviewRendered::FRoute(this, Previous, MoveTemp(Fixture)));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAPSContinuousPreviewHighCount,
	"APS.Rendered.MainMenu.ContinuousHighCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

void FAPSContinuousPreviewHighCount::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("Home"), TEXT("Remote") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FAPSContinuousPreviewHighCount::RunTest(const FString& Parameters)
{
	if (Parameters != TEXT("Home") && Parameters != TEXT("Remote"))
	{ AddError(TEXT("Unknown high-count fixture")); return false; }
	IConsoleVariable* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestNotNull(TEXT("Continuous frame switch exists"), Cvar)) return false;
	const int32 Previous = Cvar->GetInt();
	Cvar->Set(1, ECVF_SetByCode);
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		Cvar->Set(Previous, ECVF_SetByCode);
		return false;
	}
	APSContinuousPreviewRendered::FRouteFixture Fixture;
	Fixture.CaptureFolder = TEXT("ContinuousHighCount-") + Parameters;
	Fixture.bHighCountProof = true;
	Fixture.bHighCountRemote = Parameters == TEXT("Remote");
	ADD_LATENT_AUTOMATION_COMMAND(APSContinuousPreviewRendered::FRoute(this, Previous, MoveTemp(Fixture)));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAPSContinuousPreviewSeedMatrix,
	"APS.Rendered.MainMenu.ContinuousSeedMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

void FAPSContinuousPreviewSeedMatrix::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("NeutronGlobular"), TEXT("HyperGiantStream"), TEXT("WhiteDwarfCompact") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FAPSContinuousPreviewSeedMatrix::RunTest(const FString& Parameters)
{
	APSContinuousPreviewRendered::FRouteFixture Fixture;
	if (Parameters == TEXT("NeutronGlobular"))
		Fixture = {314159, EStellarType::Neutron, ESpectralClass::NS, EStarClusterType::GlobularCluster,
			TEXT("ContinuousSeed-314159-Neutron-Globular")};
	else if (Parameters == TEXT("HyperGiantStream"))
		Fixture = {161803, EStellarType::HyperGiant, ESpectralClass::M, EStarClusterType::ElongatedStream,
			TEXT("ContinuousSeed-161803-HyperGiant-Stream")};
	else if (Parameters == TEXT("WhiteDwarfCompact"))
		Fixture = {8675309, EStellarType::WhiteDwarf, ESpectralClass::A, EStarClusterType::OpenCluster,
			TEXT("ContinuousSeed-8675309-WhiteDwarf-Compact")};
	else { AddError(TEXT("Unknown full-route seed fixture")); return false; }
	IConsoleVariable* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestNotNull(TEXT("Continuous frame switch exists"), Cvar)) return false;
	const int32 Previous = Cvar->GetInt();
	Cvar->Set(1, ECVF_SetByCode);
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		Cvar->Set(Previous, ECVF_SetByCode);
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(APSContinuousPreviewRendered::FRoute(this, Previous, MoveTemp(Fixture)));
	return true;
}

#endif

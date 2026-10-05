#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceRadius.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"

namespace APSPlanetCryogenicPreviewTests
{
	constexpr double OverallTimeoutSeconds = 240.0;
	constexpr double StageTimeoutSeconds = 90.0;
	constexpr double DrainTimeoutSeconds = 30.0;
	constexpr double StableSeconds = 0.5;
	constexpr double ReturnToleranceCm = 1.0;

	struct FPublishedGeometry
	{
		TArray<double> HeightCm;
		TArray<FVector> Directions;
		TArray<int32> Indices;
	};

	class FRoundTrip final : public IAutomationLatentCommand
	{
	public:
		explicit FRoundTrip(FAutomationTestBase* InTest)
			: Test(InTest), Started(FPlatformTime::Seconds()), StageStarted(Started) {}

		bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (Stage == EStage::Cleanup) return Cleanup(Now);
			if (Now - Started > OverallTimeoutSeconds || Now - StageStarted > StageTimeoutSeconds)
			{
				Fail(FString::Printf(TEXT("Cryogenic preview timeout stage=%d elapsed=%.1fs applyCount=%d bodyType=%d"),
					int32(Stage), Now - Started, Generator.IsValid() ? Generator->GetPreviewSurfaceProfileApplyCount() : -1,
					Body.IsValid() ? int32(Body->PlanetType) : -1));
				return false;
			}
			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World ? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* VM = Controller ? Controller->GetWorldGenerationViewModel() : nullptr;
			if (!VM || !VM->GeneratedWorld) return false;
			ViewModel = VM;
			if (Stage == EStage::Menu)
			{
				if (!Controller->OpenAstronomicalGenerationForAutomation(EAstroPreviewFocus::HomePlanet,
					EAPSGenerationRoute::Civilization)) return false;
				Stage = EStage::InitialPreview;
				StageStarted = Now;
				return false;
			}
			if (!VM->bPreviewReady || !IsValid(VM->GetPreviewGenerator())) return false;
			if (!Generator.IsValid()) Generator = VM->GetPreviewGenerator();
			if (Generator.Get() != VM->GetPreviewGenerator())
			{
				Fail(TEXT("A subtype edit replaced the preview generator instead of updating the selected body"));
				return false;
			}
			if (Stage == EStage::InitialPreview)
			{
				if (!Generator->IsPreviewGlobeFamilyWarmQueueDrained() || Generator->IsPreviewCameraTransitionActive()) return false;
				APlanet* Selected = Cast<APlanet>(Generator->GetSelectedPreviewBodyActor());
				if (!Selected) return false;
				if (!Test->TestTrue(TEXT("The fixture is a generated planet in the continuous PLANET view"),
					!Selected->IsManual && Generator->UsesContinuousPreviewFrame()
					&& VM->GetPreviewFocus() == EAstroPreviewFocus::HomePlanet))
				{
					BeginCleanup();
					return false;
				}
				Body = Selected;
				StableKey = Generator->GetPreviewBodyStableKey(Selected);
				Seed = Selected->WorldScapeSeed;
				RadiusKm = APSPlanetSurfaceRadius::Kilometres(Selected->RadiusKM, Selected->PlanetRadiusKM);
				if (StableKey.IsEmpty() || Seed <= 0 || !FMath::IsFinite(RadiusKm) || RadiusKm <= 0.0)
				{
					Fail(TEXT("The selected preview body has no stable key, canonical seed or physical radius"));
					return false;
				}
				// Preserve the authored recipe and observer; only the actual subtype UI control changes.
				Test->AddInfo(FString::Printf(TEXT("Cryogenic preview fixture key=%s seed=%d radius=%.9g km; no seed, radius, camera or console overrides"),
					*StableKey, Seed, RadiusKm));
				// An authored/saved recipe may already be Ice. Do not require a
				// redundant selection to rebuild: first publish a genuinely different
				// subtype, then start the measured Ice -> Frozen -> Ice sequence.
				if (Selected->PlanetType == EPlanetType::Ice)
					RequestType(VM, EPlanetType::Frozen, EStage::PrimingFrozen);
				else
					RequestType(VM, EPlanetType::Ice, EStage::FirstIce);
				return false;
			}
			if (!Body.IsValid() || Generator->GetSelectedPreviewBodyActor() != Body.Get()
				|| VM->GetSelectedPreviewBody() != Body.Get()
				|| Generator->GetPreviewBodyStableKey(Body.Get()) != StableKey)
			{
				Fail(TEXT("Subtype switch lost the original selected body or its stable address"));
				return false;
			}
			const EPlanetType ExpectedType = Stage == EStage::PrimingFrozen || Stage == EStage::Frozen
				? EPlanetType::Frozen : EPlanetType::Ice;
			UProceduralMeshComponent* Proxy = Generator->GetPreviewTerrainProxyForBody(Body.Get());
			const int32 ApplyCount = Generator->GetPreviewSurfaceProfileApplyCount();
			const bool bPublished = Body->PlanetType == ExpectedType && VM->GeneratedWorld->PlanetType == ExpectedType
				&& Body->bWorldScapeSurfaceReady && ApplyCount > ApplyCountBeforeEdit
				&& Generator->GetActivePreviewWorldScapeBody() == Body.Get()
				&& Generator->IsPreviewGlobeFamilyWarmQueueDrained() && !Generator->IsPreviewCameraTransitionActive()
				&& IsValid(Proxy) && Proxy->IsVisible() && !Proxy->bHiddenInGame && Proxy->GetProcMeshSection(0);
			if (!bPublished)
			{
				StableSince = 0.0;
				return false;
			}
			// A counter is only a wait condition. It never substitutes for inspecting vertices.
			if (StableSince == 0.0 || StableProxy.Get() != Proxy || StableApplyCount != ApplyCount)
			{
				StableSince = Now;
				StableProxy = Proxy;
				StableApplyCount = ApplyCount;
				return false;
			}
			if (Now - StableSince < StableSeconds) return false;
			bool bIdentity = Test->TestEqual(TEXT("Subtype switch retains seed"), Body->WorldScapeSeed, Seed);
			bIdentity &= Test->TestEqual(TEXT("Subtype switch retains physical radius"),
				APSPlanetSurfaceRadius::Kilometres(Body->RadiusKM, Body->PlanetRadiusKM), RadiusKm);
			if (Stage == EStage::Frozen || Stage == EStage::ReturnedIce)
				bIdentity &= Test->TestTrue(TEXT("Ice/Frozen comparison retains the physical observer"),
					Generator->GetContinuousPreviewOrbit().ObserverCm().Equals(FirstIceObserver, 1.0));
			FPublishedGeometry Geometry;
			if (!bIdentity || !ReadPublishedGeometry(Proxy, Geometry))
			{
				BeginCleanup();
				return false;
			}
			if (Stage == EStage::PrimingFrozen)
			{
				RequestType(VM, EPlanetType::Ice, EStage::FirstIce);
			}
			else if (Stage == EStage::FirstIce)
			{
				FirstIce = MoveTemp(Geometry);
				FirstIceObserver = Generator->GetContinuousPreviewOrbit().ObserverCm();
				RequestType(VM, EPlanetType::Frozen, EStage::Frozen);
			}
			else if (Stage == EStage::Frozen)
			{
				Compare(FirstIce, Geometry, false);
				RequestType(VM, EPlanetType::Ice, EStage::ReturnedIce);
			}
			else
			{
				Compare(FirstIce, Geometry, true);
				BeginCleanup();
			}
			return false;
		}

	private:
		enum class EStage : uint8 { Menu, InitialPreview, PrimingFrozen, FirstIce, Frozen, ReturnedIce, Cleanup };

		void RequestType(UWorldGenerationViewModel* VM, EPlanetType Type, EStage Next)
		{
			ApplyCountBeforeEdit = Generator->GetPreviewSurfaceProfileApplyCount();
			StableSince = 0.0;
			Stage = Next;
			StageStarted = FPlatformTime::Seconds();
			VM->SetEnumValue(StaticEnum<EPlanetType>(), int32(Type));
		}

		bool ReadPublishedGeometry(UProceduralMeshComponent* Proxy, FPublishedGeometry& Out)
		{
			const FProcMeshSection* Section = Proxy->GetProcMeshSection(0);
			const int32 Resolution = Generator->GetPreviewGlobeFaceResolutionForBody(Body.Get());
			if (!Test->TestTrue(TEXT("Published selected globe has its complete closed-cube topology"),
				Section && Proxy->GetNumSections() == 1 && Resolution >= 16
				&& Section->ProcVertexBuffer.Num() == 6 * FMath::Square(Resolution + 1)
				&& Section->ProcIndexBuffer.Num() == 36 * FMath::Square(Resolution))) return false;
			const double PresentationScale = Body->WorldScapePresentationScale;
			if (!Test->TestTrue(TEXT("Published geometry has a finite positive presentation scale"),
				FMath::IsFinite(PresentationScale) && PresentationScale >= 1.e-9 && PresentationScale <= 1.0)) return false;
			const double LocalRadius = FMath::Max(RadiusKm * 100000.0 * PresentationScale, 100000.0);
			Out.HeightCm.Reserve(Section->ProcVertexBuffer.Num());
			Out.Directions.Reserve(Section->ProcVertexBuffer.Num());
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
			{
				const double VertexRadius = Vertex.Position.Size();
				if (!FMath::IsFinite(VertexRadius) || VertexRadius <= 0.0)
				{
					Test->AddError(TEXT("Published vertex has an invalid physical radius"));
					return false;
				}
				// Convert the real compressed displacement to comparable physical-equivalent cm.
				// This is the preview's low-bandwidth field, not full-scale gameplay terrain.
				Out.HeightCm.Add((VertexRadius - LocalRadius) / PresentationScale);
				Out.Directions.Add(Vertex.Position / VertexRadius);
			}
			for (uint32 Index : Section->ProcIndexBuffer) Out.Indices.Add(int32(Index));
			Test->AddInfo(FString::Printf(TEXT("Published %s: proxy=%s vertices=%d scale=%.12g applyCount=%d"),
				*UEnum::GetValueAsString(Body->PlanetType), *Proxy->GetName(), Out.HeightCm.Num(),
				PresentationScale, Generator->GetPreviewSurfaceProfileApplyCount()));
			return true;
		}

		void Compare(const FPublishedGeometry& A, const FPublishedGeometry& B, bool bReturning)
		{
			const FString Label = bReturning ? TEXT("Published Ice return") : TEXT("Published Ice/Frozen");
			if (!Test->TestTrue(Label + TEXT(" retains vertex correspondence and topology"),
				A.HeightCm.Num() == B.HeightCm.Num() && A.Indices == B.Indices && !A.HeightCm.IsEmpty())) return;
			double SumA = 0.0, SumB = 0.0, SquaredDifference = 0.0, MaximumDifference = 0.0;
			int32 Changed = 0;
			bool bDirectionsMatch = true;
			for (int32 I = 0; I < A.HeightCm.Num(); ++I)
			{
				bDirectionsMatch &= A.Directions[I].Equals(B.Directions[I], 1.e-10);
				SumA += A.HeightCm[I]; SumB += B.HeightCm[I];
				const double Difference = FMath::Abs(A.HeightCm[I] - B.HeightCm[I]);
				SquaredDifference += Difference * Difference;
				MaximumDifference = FMath::Max(MaximumDifference, Difference);
				Changed += Difference > ReturnToleranceCm ? 1 : 0;
			}
			if (!Test->TestTrue(Label + TEXT(" compares the same local directions"), bDirectionsMatch)) return;
			if (bReturning)
			{
				Test->TestTrue(FString::Printf(TEXT("Published Ice -> Frozen -> Ice restores every radial height within 1 cm (max %.9g cm)"),
					MaximumDifference), MaximumDifference <= ReturnToleranceCm);
				return;
			}
			const double MeanA = SumA / A.HeightCm.Num(), MeanB = SumB / B.HeightCm.Num();
			double VarianceA = 0.0, VarianceB = 0.0;
			for (int32 I = 0; I < A.HeightCm.Num(); ++I)
			{
				VarianceA += FMath::Square(A.HeightCm[I] - MeanA);
				VarianceB += FMath::Square(B.HeightCm[I] - MeanB);
			}
			const double DeviationA = FMath::Sqrt(VarianceA / A.HeightCm.Num());
			const double DeviationB = FMath::Sqrt(VarianceB / B.HeightCm.Num());
			if (!Test->TestTrue(TEXT("Both published cryogenic meshes contain physical relief"),
				DeviationA > 1.0 && DeviationB > 1.0)) return;
			double ShapeSquaredDifference = 0.0;
			for (int32 I = 0; I < A.HeightCm.Num(); ++I)
				ShapeSquaredDifference += FMath::Square((A.HeightCm[I] - MeanA) / DeviationA
					- (B.HeightCm[I] - MeanB) / DeviationB);
			const double Rms = FMath::Sqrt(SquaredDifference / A.HeightCm.Num());
			const double Fraction = double(Changed) / A.HeightCm.Num();
			const double ShapeRms = FMath::Sqrt(ShapeSquaredDifference / A.HeightCm.Num());
			Test->AddInfo(FString::Printf(TEXT("Published Ice/Frozen: RMS=%.9g cm changedFraction=%.6f unitShapeRMS=%.9g"), Rms, Fraction, ShapeRms));
			// Small identity guards, not a perceptual/visual acceptance threshold.
			Test->TestTrue(TEXT("Published subtype displacement RMS is at least 1 m"), Rms >= 100.0);
			Test->TestTrue(TEXT("At least 25 percent of published vertices move by more than 1 cm"), Fraction >= .25);
			Test->TestTrue(TEXT("Published shape differs beyond a uniform offset/amplitude"), ShapeRms >= .001);
		}

		void Fail(const FString& Message) { Test->AddError(Message); BeginCleanup(); }
		void BeginCleanup() { Stage = EStage::Cleanup; StageStarted = FPlatformTime::Seconds(); }
		bool Cleanup(double Now)
		{
			if (!Generator.IsValid() && ViewModel.IsValid()) Generator = ViewModel->GetPreviewGenerator();
			if (Generator.IsValid() && !Generator->PreparePreviewForTravel())
			{
				if (Now - StageStarted < DrainTimeoutSeconds) return false;
				Test->AddError(TEXT("Preview workers did not drain in 30 seconds; keeping their owner alive. End this isolated test process; do not continue a shared test batch."));
				return true;
			}
			if (ViewModel.IsValid()) ViewModel->Shutdown();
			return true;
		}

		FAutomationTestBase* Test;
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
		TWeakObjectPtr<AAstroGenerator> Generator;
		TWeakObjectPtr<APlanet> Body;
		TWeakObjectPtr<UProceduralMeshComponent> StableProxy;
		EStage Stage = EStage::Menu;
		double Started, StageStarted, StableSince = 0.0, RadiusKm = 0.0;
		int32 Seed = 0, ApplyCountBeforeEdit = 0, StableApplyCount = 0;
		FString StableKey;
		FVector FirstIceObserver = FVector::ZeroVector;
		FPublishedGeometry FirstIce;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPlanetCryogenicPublishedPreview,
	"APS.Rendered.PlanetSurface.CryogenicPreview.PublishedGeometryRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FAPSPlanetCryogenicPublishedPreview::RunTest(const FString& Parameters)
{
	// Opens an actual menu map and edits its transient generated world. Never run
	// automatically in an existing work session or as part of flight performance.
	if (!TestTrue(TEXT("Requires explicit -APSProbeCryogenicPreview in an isolated, last-test menu process"),
		FParse::Param(FCommandLine::Get(), TEXT("APSProbeCryogenicPreview")))) return false;
	const IConsoleVariable* Continuous = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Preview.ContinuousFrame"));
	if (!TestTrue(TEXT("Continuous preview must already be enabled; this test does not override it"),
		Continuous && Continuous->GetInt() == 1)) return false;
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(APSPlanetCryogenicPreviewTests::FRoundTrip(this));
	AddInfo(TEXT("Actual published CPU vertex buffers only; no screenshots, GPU visual acceptance, saved-world or gameplay-height claim."));
	return true;
}

#endif

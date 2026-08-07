#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/HomeSystemPosition.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StarType.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/GameModes/GravityGameModeBase.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Generation/WorldScapePayloadValidation.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "PlanetaryAtmosphere.h"
#include "UnrealClient.h"

namespace APSGeneratedGameplayHandoffSmokeTests
{
	constexpr double WholeTestTimeoutSeconds = 120.0;
	constexpr double ScreenshotTimeoutSeconds = 10.0;
	constexpr double CleanupTimeoutSeconds = 20.0;
	constexpr double SpawnPointToleranceCm = 500.0;
	constexpr double MinimumNonBlackPixelRatio = 0.005;
	constexpr double MinimumBrightnessVariance = 4.0;
	constexpr double MinimumSubjectMeanBrightness = 5.0;
	constexpr double MinimumSubjectNonBlackPixelRatio = 0.08;
	constexpr double MinimumSubjectBrightnessVariance = 24.0;

	template <typename TActorType>
	TArray<TActorType*> FindActors(UWorld* World)
	{
		TArray<TActorType*> Result;
		if (!World)
		{
			return Result;
		}
		for (TActorIterator<TActorType> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Result.Add(*It);
			}
		}
		return Result;
	}

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (AAstroGenerator* Generator : FindActors<AAstroGenerator>(World))
		{
			if (Generator->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return Generator;
			}
		}
		return nullptr;
	}

	APlanetarySurfaceGenerator* FindPreviewSurfaceGenerator(
		UWorld* World, const AAstroGenerator* PreviewGenerator)
	{
		for (APlanetarySurfaceGenerator* Generator
			: FindActors<APlanetarySurfaceGenerator>(World))
		{
			if (Generator->GetOwner() == PreviewGenerator)
			{
				return Generator;
			}
		}
		return nullptr;
	}

	bool IsFiniteTransform(const FTransform& Transform)
	{
		const FVector Location = Transform.GetLocation();
		const FVector Scale = Transform.GetScale3D();
		const FQuat Rotation = Transform.GetRotation();
		return !Transform.ContainsNaN()
			&& FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y)
			&& FMath::IsFinite(Location.Z) && FMath::IsFinite(Scale.X)
			&& FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
			&& FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y)
			&& FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W);
	}

	int32 CountVisibleRegisteredRenderComponents(const AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsHidden())
		{
			return 0;
		}

		int32 Result = 0;
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents(PrimitiveComponents);
		for (const UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!IsValid(Component) || !Component->IsRegistered()
				|| !Component->IsVisible() || Component->bHiddenInGame)
			{
				continue;
			}
			if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component))
			{
				Result += StaticMesh->GetStaticMesh() ? 1 : 0;
			}
			else if (const USkeletalMeshComponent* SkeletalMesh =
				Cast<USkeletalMeshComponent>(Component))
			{
				Result += SkeletalMesh->GetSkeletalMeshAsset() ? 1 : 0;
			}
		}
		return Result;
	}

	class FGeneratedCivilizationHandoffCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FGeneratedCivilizationHandoffCommand(FAutomationTestBase* InTest)
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
			if (Step != EStep::Cleanup && Now - TestStartSeconds > WholeTestTimeoutSeconds)
			{
				return Fail(FString::Printf(
					TEXT("120-second timeout at step %d"), static_cast<int32>(Step)));
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			switch (Step)
			{
			case EStep::OpenGenerator:
				return UpdateOpenGenerator(World, Now);
			case EStep::WaitForPreview:
				return UpdateWaitForPreview(World, Now);
			case EStep::WaitForGameplayTravel:
				return UpdateWaitForGameplayTravel(World, Now);
			case EStep::ValidateGameplayHierarchy:
				return UpdateValidateGameplayHierarchy(World, Now);
			case EStep::WaitForGameplaySurface:
				return UpdateWaitForGameplaySurface(World, Now);
			case EStep::WaitForScreenshot:
				return UpdateWaitForScreenshot(World, Now);
			case EStep::Cleanup:
				return UpdateCleanup(World, Now);
			default:
				return true;
			}
		}

	private:
		enum class EStep : uint8
		{
			OpenGenerator,
			WaitForPreview,
			WaitForGameplayTravel,
			ValidateGameplayHierarchy,
			WaitForGameplaySurface,
			WaitForScreenshot,
			Cleanup
		};

		bool Fail(const FString& Message)
		{
			if (PendingFailure.IsEmpty())
			{
				PendingFailure = Message;
				UE_LOG(LogTemp, Error,
					TEXT("[APS.Handoff.Smoke] FAIL %s; beginning safe WorldScape drain"),
					*Message);
			}
			Step = EStep::Cleanup;
			StepStartSeconds = FPlatformTime::Seconds();
			return false;
		}

		bool UpdateOpenGenerator(UWorld* World, double Now)
		{
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			UGeneratedWorld* Model = ViewModel ? ViewModel->GetGeneratedWorld() : nullptr;
			if (!World || !Controller || !ViewModel || !Model)
			{
				return false;
			}

			MenuWorld = World;
			EditableGeneratedWorldAddress = Model;
			Model->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
			Model->bGenerateFullScaledWorld = true;
			Model->bGenerateHomeSystem = true;
			Model->bStartWithHomePlanet = true;
			Model->bRandomHomeSystem = false;
			Model->bRandomHomeSystemType = false;
			Model->bRandomHomeStar = false;
			Model->bRandomStartPlanetNumber = false;
			Model->GalaxyType = EGalaxyType::Spiral;
			Model->GalaxyClass = EGalaxyClass::Sc;
			Model->GalaxySize = 64;
			Model->GalaxyStarCount = 2048;
			Model->GalaxyStarDensity = 12.0;
			Model->StarClusterSize = EStarClusterSize::Tiny;
			Model->StarClusterType = EStarClusterType::OpenCluster;
			Model->StarClusterPopulation = EStarClusterPopulation::Dwarfs;
			Model->StarClusterComposition = EStarClusterComposition::AllSpectral;
			Model->StarType = EStarType::SingleStar;
			Model->StellarType = EStellarType::MainSequence;
			Model->SpectralClass = ESpectralClass::G;
			Model->HomeSystemPosition = EHomeSystemPosition::DirectPosition;
			Model->PlanetarySystemType = EPlanetarySystemType::SinglePlanetSystem;
			Model->OrbitDistributionType = EOrbitDistributionType::Uniform;
			Model->PlanetsAmount = 1;
			Model->MoonsAmount = 0;
			Model->StartPlanetIndex = 1;
			Model->PlanetType = EPlanetType::HighMountain;
			Model->PlanetRadius = 6371.0;
			Model->PlanetSurfaceSeed = 424242;
			Model->SurfaceFeatureScale = 1.15;
			Model->SurfaceReliefScale = 1.10;
			Model->SurfaceLandCoverageScale = 0.90;
			Model->SurfaceMountainScale = 1.25;
			Model->SurfaceCraterScale = 0.75;
			Model->SurfaceRoughnessScale = 1.05;
			Model->AtmosphereHeight = 140.0;
			Model->AtmosphereOpacity = 12.0;
			Model->AtmosphereMultiScattering = 1.0;
			Model->AtmosphereRayleighScattering = 8.0;

			if (!Controller->OpenAstronomicalGenerationForAutomation(
				EAstroPreviewFocus::HomePlanet, EAPSGenerationRoute::Civilization))
			{
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Civilization generator opened map=%s seed=%d level=StarCluster fullScale=true"),
				*World->GetMapName(), Model->PlanetSurfaceSeed);
			Step = EStep::WaitForPreview;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForPreview(UWorld* World, double Now)
		{
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			if (!World || !Controller || !ViewModel || !ViewModel->bPreviewReady)
			{
				return false;
			}

			PreviewGenerator = FindPreviewGenerator(World);
			APlanet* PreviewPlanet = PreviewGenerator.IsValid()
				? Cast<APlanet>(PreviewGenerator->GetActivePreviewWorldScapeBody()) : nullptr;
			// The menu owns one persistent streaming generator outside the generated
			// planet actor so subtype changes can swap profiles without rebuilding the
			// hierarchy. The planet's gameplay generator is intentionally not the
			// authoritative preview root.
			APlanetarySurfaceGenerator* PreviewSurface = FindPreviewSurfaceGenerator(
				World, PreviewGenerator.Get());
			if (!PreviewGenerator.IsValid() || !IsValid(PreviewPlanet)
				|| !PreviewPlanet->bWorldScapeSurfaceReady || !IsValid(PreviewSurface)
				|| !PreviewSurface->IsSurfaceProfileCurrent(PreviewPlanet))
			{
				return false;
			}
			if (!Cast<UAPSWorldScapePlanetNoise>(PreviewSurface->ResolvedNoiseInstance))
			{
				return Fail(TEXT("menu preview became ready through a legacy/non-APS noise generator"));
			}

			USpawnParameters* Spawn = ViewModel->SpawnParameters;
			if (!Spawn || !Spawn->BP_CharacterClass || !Spawn->BP_HomeSpaceship
				|| !Spawn->BP_HomeSpaceStation || !Spawn->BP_HomeSpaceHeadquarters
				|| !Spawn->BP_HomeSpaceShipyard)
			{
				return Fail(TEXT("Civilization route did not initialize all five selected spawn classes"));
			}
			SelectedPawnClass = Spawn->BP_CharacterClass.Get();
			SelectedPawnClassPath = SelectedPawnClass.IsValid()
				? SelectedPawnClass->GetPathName() : FString();
			if (!SelectedPawnClass.IsValid() || SelectedPawnClassPath.IsEmpty())
			{
				return Fail(TEXT("selected pawn class is invalid before commit"));
			}
			ViewModel->SetSpawnClass(EAPSStartAssetSlot::Character, SelectedPawnClass.Get());
			Spawn->CivilizationName = TEXT("APS HANDOFF SMOKE CIVILIZATION");
			Spawn->CharacterSpawnPlace = ECharSpawnPlace::PlanetOrbit;
			Spawn->HomeStationOrbitHeight = EOrbitHeight::LowOrbit;
			EditableSpawnParametersAddress = Spawn;
			PreviewProfileSignature = PreviewSurface->AppliedSurfaceProfileSignature;

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Preview ready in %.2fs pawn=%s profile=%u; committing L_WorldGeneration"),
				Now - StepStartSeconds, *SelectedPawnClassPath, PreviewProfileSignature);
			ViewModel->CommitAndOpenLevel(TEXT("L_WorldGeneration"));
			Step = EStep::WaitForGameplayTravel;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForGameplayTravel(UWorld* World, double Now)
		{
			if (!World || World == MenuWorld.Get()
				|| !World->GetMapName().Contains(TEXT("L_WorldGeneration")))
			{
				return false;
			}
			if (!Cast<AGravityGameModeBase>(World->GetAuthGameMode()))
			{
				return false;
			}

			GameplayWorld = World;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Travel complete in %.2fs map=%s gameMode=%s"),
				Now - StepStartSeconds, *World->GetMapName(),
				*GetNameSafe(World->GetAuthGameMode()));
			Step = EStep::ValidateGameplayHierarchy;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateValidateGameplayHierarchy(UWorld* World, double Now)
		{
			if (!World || World != GameplayWorld.Get())
			{
				return false;
			}

			const TArray<AAstroGenerator*> Generators = FindActors<AAstroGenerator>(World);
			const TArray<AGalaxy*> Galaxies = FindActors<AGalaxy>(World);
			const TArray<AStarCluster*> Clusters = FindActors<AStarCluster>(World);
			const TArray<AStarSystem*> StarSystems = FindActors<AStarSystem>(World);
			const TArray<AStar*> Stars = FindActors<AStar>(World);
			const TArray<APlanetarySystem*> PlanetarySystems = FindActors<APlanetarySystem>(World);
			const TArray<APlanetOrbit*> PlanetOrbits = FindActors<APlanetOrbit>(World);
			const TArray<APlanet*> Planets = FindActors<APlanet>(World);
			const TArray<ASpaceHeadquarters*> Headquarters = FindActors<ASpaceHeadquarters>(World);
			const TArray<ASpaceShipyard*> Shipyards = FindActors<ASpaceShipyard>(World);
			const TArray<ASpaceship*> Spaceships = FindActors<ASpaceship>(World);
			TArray<ASpaceStation*> PlainStations;
			for (ASpaceStation* Station : FindActors<ASpaceStation>(World))
			{
				if (!Cast<ASpaceHeadquarters>(Station) && !Cast<ASpaceShipyard>(Station))
				{
					PlainStations.Add(Station);
				}
			}
			UMainGameplayInstance* GameplayState = World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr;
			APlayerController* PlayerController = World->GetFirstPlayerController();
			APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
			if (!GameplayState || !GameplayState->NewGeneratedWorld || !GameplayState->SpawnParameters
				|| !GameplayState->CurrentCivilization || !PlayerController || !PlayerPawn
				|| Generators.Num() == 0 || Galaxies.Num() == 0 || Clusters.Num() == 0
				|| StarSystems.Num() == 0 || Stars.Num() == 0 || PlanetarySystems.Num() == 0
				|| PlanetOrbits.Num() == 0 || Planets.Num() == 0 || Headquarters.Num() == 0
				|| PlainStations.Num() == 0 || Shipyards.Num() == 0 || Spaceships.Num() == 0)
			{
				return false;
			}

			if (Generators.Num() != 1 || Galaxies.Num() != 1 || Clusters.Num() != 1
				|| StarSystems.Num() != 1 || Stars.Num() != 1 || PlanetarySystems.Num() != 1
				|| PlanetOrbits.Num() != 1 || Planets.Num() != 1 || Headquarters.Num() != 1
				|| PlainStations.Num() != 1 || Shipyards.Num() != 1)
			{
				return Fail(FString::Printf(
					TEXT("duplicate/incomplete materialization generators=%d galaxy=%d cluster=%d systems=%d stars=%d planetarySystems=%d orbits=%d planets=%d HQ=%d stations=%d yards=%d ships=%d"),
					Generators.Num(), Galaxies.Num(), Clusters.Num(), StarSystems.Num(), Stars.Num(),
					PlanetarySystems.Num(), PlanetOrbits.Num(), Planets.Num(), Headquarters.Num(),
					PlainStations.Num(), Shipyards.Num(), Spaceships.Num()));
			}

			AGravityGameModeBase* GravityMode = Cast<AGravityGameModeBase>(World->GetAuthGameMode());
			if (!GravityMode)
			{
				return Fail(TEXT("L_WorldGeneration is not using GravityGameMode"));
			}
			if (GameplayState->bIsLoadingMode || GameplayState->bUseAuthoredSinglePlayWorld
				|| !GameplayState->bSpawnGeneratedCivilization || !GameplayState->SaveSlotName.IsEmpty())
			{
				return Fail(TEXT("GameInstance route flags do not describe a fresh generated civilization"));
			}
			if (GameplayState->NewGeneratedWorld == EditableGeneratedWorldAddress
				|| GameplayState->SpawnParameters == EditableSpawnParametersAddress)
			{
				return Fail(TEXT("menu-owned models were not frozen into independent GameInstance snapshots"));
			}
			const UGeneratedWorld* Snapshot = GameplayState->NewGeneratedWorld;
			if (Snapshot->AstroGenerationLevel != EAstroGenerationLevel::StarCluster
				|| !Snapshot->bGenerateFullScaledWorld || !Snapshot->bGenerateHomeSystem
				|| !Snapshot->bStartWithHomePlanet || Snapshot->StarType != EStarType::SingleStar
				|| Snapshot->PlanetsAmount != 1 || Snapshot->MoonsAmount != 0
				|| Snapshot->StartPlanetIndex != 1 || Snapshot->PlanetType != EPlanetType::HighMountain
				|| Snapshot->PlanetSurfaceSeed != 424242)
			{
				return Fail(TEXT("committed generated-world snapshot differs from deterministic menu model"));
			}
			UClass* GameModePawnClass = GravityMode->GetDefaultPawnClassForController(PlayerController);
			if (!GameplayState->SpawnParameters->BP_CharacterClass || !GameModePawnClass
				|| GameplayState->SpawnParameters->BP_CharacterClass.Get()->GetPathName()
					!= SelectedPawnClassPath
				|| PlayerPawn->GetClass()->GetPathName() != SelectedPawnClassPath
				|| GameModePawnClass->GetPathName() != SelectedPawnClassPath)
			{
				return Fail(FString::Printf(
					TEXT("selected pawn handoff mismatch selected=%s snapshot=%s possessed=%s"),
					*SelectedPawnClassPath,
					*GetNameSafe(GameplayState->SpawnParameters->BP_CharacterClass.Get()),
					*GetNameSafe(PlayerPawn->GetClass())));
			}
			AGravityPlayerController* GravityController =
				Cast<AGravityPlayerController>(PlayerController);
			AGravityCharacterPawn* GravityPawn = Cast<AGravityCharacterPawn>(PlayerPawn);
			if (!GravityController || !GravityPawn)
			{
				return Fail(FString::Printf(
					TEXT("generated handoff controller/pawn types mismatch controller=%s pawn=%s"),
					*GetNameSafe(PlayerController ? PlayerController->GetClass() : nullptr),
					*GetNameSafe(PlayerPawn ? PlayerPawn->GetClass() : nullptr)));
			}
			if (GravityController->GetViewTarget() != PlayerPawn
				|| !IsValid(GravityController->PlayerCameraManager)
				|| !IsValid(GravityPawn->PlayerCamera)
				|| !GravityPawn->PlayerCamera->IsRegistered()
				|| !GravityPawn->PlayerCamera->IsActive()
				|| !IsFiniteTransform(GravityPawn->PlayerCamera->GetComponentTransform()))
			{
				return Fail(FString::Printf(
					TEXT("possessed pawn camera contract mismatch viewTarget=%s pawn=%s manager=%s camera=%s registered=%d active=%d"),
					*GetNameSafe(GravityController->GetViewTarget()), *GetNameSafe(PlayerPawn),
					*GetNameSafe(GravityController->PlayerCameraManager),
					*GetNameSafe(GravityPawn->PlayerCamera),
					IsValid(GravityPawn->PlayerCamera) && GravityPawn->PlayerCamera->IsRegistered() ? 1 : 0,
					IsValid(GravityPawn->PlayerCamera) && GravityPawn->PlayerCamera->IsActive() ? 1 : 0));
			}

			AAstroGenerator* Generator = Generators[0];
			AGalaxy* Galaxy = Galaxies[0];
			AStarCluster* Cluster = Clusters[0];
			AStarSystem* StarSystem = StarSystems[0];
			AStar* Star = Stars[0];
			APlanetarySystem* PlanetarySystem = PlanetarySystems[0];
			APlanetOrbit* Orbit = PlanetOrbits[0];
			APlanet* Planet = Planets[0];
			ASpaceHeadquarters* HeadquartersActor = Headquarters[0];
			ASpaceStation* StationActor = PlainStations[0];
			ASpaceShipyard* ShipyardActor = Shipyards[0];
			ASpaceship* SpaceshipActor = nullptr;
			int32 MatchingHomeShipCount = 0;
			for (ASpaceship* Candidate : Spaceships)
			{
				if (Candidate != PlayerPawn
					&& Candidate->GetAttachParentActor() == ShipyardActor
					&& Candidate->IsA(GameplayState->SpawnParameters->BP_HomeSpaceship))
				{
					SpaceshipActor = Candidate;
					++MatchingHomeShipCount;
				}
			}
			if (!IsValid(SpaceshipActor) || MatchingHomeShipCount != 1)
			{
				return Fail(FString::Printf(
					TEXT("expected exactly one generated home spaceship attached to shipyard, found %d (all ASpaceship actors=%d)"),
					MatchingHomeShipCount, Spaceships.Num()));
			}
			if (Galaxy->GetAttachParentActor() != Generator
				|| Cluster->GetAttachParentActor() != Galaxy
				|| StarSystem->GetAttachParentActor() != Cluster
				|| Star->GetAttachParentActor() != StarSystem
				|| PlanetarySystem->GetAttachParentActor() != Star
				|| Orbit->GetAttachParentActor() != PlanetarySystem
				|| Planet->GetAttachParentActor() != Orbit)
			{
				return Fail(TEXT("Galaxy/cluster/system/star/planet attachment chain is not continuous"));
			}
			if (!StarSystem->bMaterializedFromCluster || !StarSystem->StableSystemId.IsValid()
				|| StarSystem->GetStars().Num() != 1 || StarSystem->GetStars()[0] != Star
				|| StarSystem->MainStar != Star || Star->PlanetarySystem != PlanetarySystem
				|| PlanetarySystem->PlanetOrbitsList.Num() != 1
				|| PlanetarySystem->PlanetsActorsList.Num() != 1
				|| PlanetarySystem->PlanetOrbitsList[0] != Orbit
				|| PlanetarySystem->PlanetsActorsList[0] != Planet || Orbit->Planet != Planet
				|| Generator->GeneratedStarSystems.Num() != 1
				|| Generator->GeneratedStarSystems[0] != StarSystem
				|| Generator->HomeStar != Star || Generator->HomePlanetarySystem != PlanetarySystem
				|| Generator->HomePlanet != Planet)
			{
				return Fail(FString::Printf(
					TEXT("single-star/single-planet materialized records are inconsistent: materialized=%d stable=%d stars=%d main=%d starPS=%d orbits=%d planets=%d orbit0=%d planet0=%d orbitPlanet=%d registry=%d registry0=%d homeStar=%d homePS=%d homePlanet=%d"),
					StarSystem->bMaterializedFromCluster ? 1 : 0,
					StarSystem->StableSystemId.IsValid() ? 1 : 0,
					StarSystem->GetStars().Num(), StarSystem->MainStar == Star ? 1 : 0,
					Star->PlanetarySystem == PlanetarySystem ? 1 : 0,
					PlanetarySystem->PlanetOrbitsList.Num(), PlanetarySystem->PlanetsActorsList.Num(),
					PlanetarySystem->PlanetOrbitsList.Num() == 1 && PlanetarySystem->PlanetOrbitsList[0] == Orbit ? 1 : 0,
					PlanetarySystem->PlanetsActorsList.Num() == 1 && PlanetarySystem->PlanetsActorsList[0] == Planet ? 1 : 0,
					Orbit->Planet == Planet ? 1 : 0,
					Generator->GeneratedStarSystems.Num(),
					Generator->GeneratedStarSystems.Num() == 1 && Generator->GeneratedStarSystems[0] == StarSystem ? 1 : 0,
					Generator->HomeStar == Star ? 1 : 0,
					Generator->HomePlanetarySystem == PlanetarySystem ? 1 : 0,
					Generator->HomePlanet == Planet ? 1 : 0));
			}
			if (HeadquartersActor->GetAttachParentActor() != Planet
				|| StationActor->GetAttachParentActor() != HeadquartersActor
				|| ShipyardActor->GetAttachParentActor() != HeadquartersActor
				|| SpaceshipActor->GetAttachParentActor() != ShipyardActor
				|| HeadquartersActor->Civilization != GameplayState->CurrentCivilization
				|| !HeadquartersActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceHeadquarters)
				|| !StationActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceStation)
				|| !ShipyardActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceShipyard)
				|| !SpaceshipActor->IsA(GameplayState->SpawnParameters->BP_HomeSpaceship))
			{
				return Fail(TEXT("starter HQ/station/shipyard/spaceship classes or attachments mismatch"));
			}
			const bool bFinitePawnTransform = IsFiniteTransform(PlayerPawn->GetActorTransform());
			const bool bFiniteStationTransform = IsFiniteTransform(StationActor->GetActorTransform());
			const bool bValidSpawnPoint = IsValid(StationActor->SpawnPoint)
				&& StationActor->SpawnPoint->IsRegistered()
				&& IsFiniteTransform(StationActor->SpawnPoint->GetComponentTransform());
			const FVector SpawnPointLocation = bValidSpawnPoint
				? StationActor->SpawnPoint->GetComponentLocation() : FVector::ZeroVector;
			const double PawnToSpawnPointCm = bValidSpawnPoint && bFinitePawnTransform
				? FVector::Distance(PlayerPawn->GetActorLocation(), SpawnPointLocation)
				: TNumericLimits<double>::Max();
			const int32 CharacterRenderComponentCount =
				CountVisibleRegisteredRenderComponents(GravityPawn);
			const int32 StationRenderComponentCount =
				CountVisibleRegisteredRenderComponents(StationActor);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] controller=%s pawn=%s viewTarget=%s pawnLocation=%s stationLocation=%s spawnPoint=%s pawnToSpawn=%.2fcm cameraLocation=%s cameraRotation=%s characterRenderers=%d stationRenderers=%d"),
				*GetNameSafe(GravityController->GetClass()), *GetNameSafe(PlayerPawn),
				*GetNameSafe(GravityController->GetViewTarget()),
				*PlayerPawn->GetActorLocation().ToCompactString(),
				*StationActor->GetActorLocation().ToCompactString(),
				*SpawnPointLocation.ToCompactString(), PawnToSpawnPointCm,
				*GravityController->PlayerCameraManager->GetCameraLocation().ToCompactString(),
				*GravityController->PlayerCameraManager->GetCameraRotation().ToCompactString(),
				CharacterRenderComponentCount, StationRenderComponentCount);
			if (!bFinitePawnTransform || !bFiniteStationTransform || !bValidSpawnPoint
				|| PawnToSpawnPointCm > SpawnPointToleranceCm)
			{
				return Fail(FString::Printf(
					TEXT("starter spawn transform mismatch finitePawn=%d finiteStation=%d validSpawnPoint=%d pawnToSpawn=%.2fcm tolerance=%.2fcm"),
					bFinitePawnTransform ? 1 : 0, bFiniteStationTransform ? 1 : 0,
					bValidSpawnPoint ? 1 : 0, PawnToSpawnPointCm, SpawnPointToleranceCm));
			}
			if (CharacterRenderComponentCount == 0 || StationRenderComponentCount == 0)
			{
				return Fail(FString::Printf(
					TEXT("starter character/station has no visible registered render asset character=%d station=%d"),
					CharacterRenderComponentCount, StationRenderComponentCount));
			}
			if (!IsValid(StationActor->GravityCollisionZone)
				|| StationActor->GravityCollisionZone->IsVisible()
				|| !StationActor->GravityCollisionZone->bHiddenInGame
				|| !IsValid(ShipyardActor->GravityCollisionZone)
				|| ShipyardActor->GravityCollisionZone->IsVisible()
				|| !ShipyardActor->GravityCollisionZone->bHiddenInGame)
			{
				return Fail(TEXT("station gravity collision volume is rendering in gameplay"));
			}

			RuntimeHomePlanet = Planet;
			RuntimeGravityPawn = GravityPawn;
			RuntimeStation = StationActor;
			GeneratedSaveSlotName = GravityController->CurrentSaveSlotName;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Gameplay hierarchy ready in %.2fs system=%s planet=%s pawn=%s saveSlot=%s starterAttachments=OK"),
				Now - StepStartSeconds, *StarSystem->StableSystemId.ToString(),
				*GetNameSafe(Planet), *SelectedPawnClassPath, *GeneratedSaveSlotName);
			Step = EStep::WaitForGameplaySurface;
			StepStartSeconds = Now;
			return false;
		}

		bool UpdateWaitForGameplaySurface(UWorld* World, double Now)
		{
			APlanet* Planet = RuntimeHomePlanet.Get();
			APlanetarySurfaceGenerator* Surface = IsValid(Planet)
				? Planet->PlanetaryEnvironmentGenerator : nullptr;
			AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
			AAtmoScape* Atmosphere = IsValid(Surface) ? Surface->PlanetAtmosphere : nullptr;
			if (!World || World != GameplayWorld.Get() || !IsValid(Planet) || !IsValid(Surface)
				|| !IsValid(Root) || !IsValid(Atmosphere) || !Planet->bWorldScapeSurfaceReady)
			{
				return false;
			}

			const FAPSResolvedPlanetSurfaceProfile ExpectedProfile =
				UAPSPlanetSurfaceProfileResolver::ResolveForBody(Planet, Surface->SurfaceProfileCatalog);
			const uint32 ExpectedSignature =
				UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(ExpectedProfile);
			const uint32 AppliedSignature =
				UAPSPlanetSurfaceProfileResolver::BuildProfileSignature(Surface->ResolvedSurfaceProfile);
			const bool bCompleteTerrainPayload = Root->WorldScapeLod.Num() >= Root->MaxLod
				&& !Root->WorldScapeLod.ContainsByPredicate([](const UWorldScapeLod* Lod)
				{
					return !APSWorldScapePayloadValidation::HasCompletePayload(Lod, true);
				});
			const bool bCompleteOceanPayload = !Root->bOcean
				|| (Root->WorldScapeLodOcean.Num() > 0
					&& !Root->WorldScapeLodOcean.ContainsByPredicate([](const UWorldScapeLod* Lod)
					{
						return !APSWorldScapePayloadValidation::HasCompletePayload(Lod, false);
					}));
			if (!Surface->IsSurfaceProfileCurrent(Planet)
				|| Surface->ResolvedSurfaceProfile.PlanetType != EPlanetType::HighMountain
				|| ExpectedSignature != AppliedSignature
				|| !Cast<UAPSWorldScapePlanetNoise>(Surface->ResolvedNoiseInstance)
				|| Root->WorldScapeNoise != Surface->ResolvedNoiseInstance
				|| !Surface->ResolvedTerrainMaterialInstance
				|| Root->TerrainMaterial.DefaultMaterial != Surface->ResolvedTerrainMaterialInstance
				|| Atmosphere->bKeepRelativeScale || Atmosphere->LightSource != Planet->ParentStar
				|| !FMath::IsNearlyEqual(Planet->WorldScapePresentationScale, 1.0)
				|| Planet->GetWorldScapeStreamingState() != EWorldScapeSurfaceState::Active
				|| Root->WorldScapeLodInGeneration.Num() != 0
				|| !bCompleteTerrainPayload || !bCompleteOceanPayload || Root->IsHidden())
			{
				return Fail(FString::Printf(
					TEXT("gameplay surface bypassed resolver or is not stably rendered type=%d expectedSig=%u appliedSig=%u lods=%d workers=%d scale=%.9f"),
					static_cast<int32>(Surface->ResolvedSurfaceProfile.PlanetType), ExpectedSignature,
					AppliedSignature, Root->WorldScapeLod.Num(),
					Root->WorldScapeLodInGeneration.Num(), Planet->WorldScapePresentationScale));
			}

			ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(),
				TEXT("Screenshots/Windows/APS_GeneratedCivilization_Handoff.png"));
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
			IFileManager::Get().Delete(*ScreenshotPath, false, true);
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Gameplay WorldScape ready in %.2fs resolverSig=%u lods=%d; settling game viewport for %s"),
				Now - StepStartSeconds, AppliedSignature, Root->WorldScapeLod.Num(),
				*ScreenshotPath);
			ScreenshotSettleFramesRemaining = 2;
			Step = EStep::WaitForScreenshot;
			StepStartSeconds = Now;
			return false;
		}

		bool CaptureGameplayViewport(UWorld* World, FString& OutFailure)
		{
			OutFailure.Reset();
			UGameViewportClient* GameViewportClient = AutomationCommon::GetAnyGameViewportClient();
			FViewport* GameViewport = GameViewportClient ? GameViewportClient->Viewport : nullptr;
			if (!World || !GameViewportClient || GameViewportClient->GetWorld() != World || !GameViewport)
			{
				return false;
			}

			const FIntPoint ViewportSize = GameViewport->GetSizeXY();
			if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
			{
				return false;
			}
			TArray<FColor> Pixels;
			if (!GameViewport->ReadPixels(Pixels)
				|| Pixels.Num() != static_cast<int64>(ViewportSize.X) * ViewportSize.Y)
			{
				return false;
			}

			TArray64<uint8> PngData;
			FImageUtils::PNGCompressImageArray(ViewportSize.X, ViewportSize.Y,
				TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
			if (PngData.IsEmpty() || !FFileHelper::SaveArrayToFile(PngData, *ScreenshotPath))
			{
				OutFailure = FString::Printf(
					TEXT("could not encode/write gameplay viewport screenshot %s"), *ScreenshotPath);
				return false;
			}

			double BrightnessSum = 0.0;
			double BrightnessSquaredSum = 0.0;
			int64 NonBlackPixelCount = 0;
			for (const FColor& Pixel : Pixels)
			{
				const double Brightness = static_cast<double>(
					FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
				BrightnessSum += Brightness;
				BrightnessSquaredSum += Brightness * Brightness;
				NonBlackPixelCount += Brightness > 8.0 ? 1 : 0;
			}
			const double PixelCount = static_cast<double>(Pixels.Num());
			const double MeanBrightness = BrightnessSum / PixelCount;
			const double BrightnessVariance = FMath::Max(
				0.0, BrightnessSquaredSum / PixelCount - MeanBrightness * MeanBrightness);
			const double NonBlackPixelRatio = static_cast<double>(NonBlackPixelCount) / PixelCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] captured GAME viewport=%dx%d meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f screenshot=%s"),
				ViewportSize.X, ViewportSize.Y, MeanBrightness, BrightnessVariance,
				NonBlackPixelRatio, *ScreenshotPath);
			if (NonBlackPixelRatio < MinimumNonBlackPixelRatio
				|| BrightnessVariance < MinimumBrightnessVariance)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay camera frame is blank/flat meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f (minimum ratio=%.5f variance=%.3f)"),
					MeanBrightness, BrightnessVariance, NonBlackPixelRatio,
					MinimumNonBlackPixelRatio, MinimumBrightnessVariance);
				return false;
			}

			AGravityCharacterPawn* GravityPawn = RuntimeGravityPawn.Get();
			ASpaceStation* Station = RuntimeStation.Get();
			APlayerController* PlayerController = World->GetFirstPlayerController();
			if (!IsValid(GravityPawn) || !IsValid(Station) || !IsValid(Station->SpawnPoint)
				|| !IsValid(PlayerController))
			{
				OutFailure = FString::Printf(
					TEXT("gameplay subject ROI has invalid contract pawn=%s station=%s spawnPoint=%s controller=%s"),
					*GetNameSafe(GravityPawn), *GetNameSafe(Station),
					*GetNameSafe(Station ? Station->SpawnPoint : nullptr),
					*GetNameSafe(PlayerController));
				return false;
			}

			FVector2D PawnScreenPosition = FVector2D::ZeroVector;
			FVector2D StationScreenPosition = FVector2D::ZeroVector;
			const bool bPawnProjected = PlayerController->ProjectWorldLocationToScreen(
				GravityPawn->GetActorLocation(), PawnScreenPosition, false);
			const bool bStationProjected = PlayerController->ProjectWorldLocationToScreen(
				Station->SpawnPoint->GetComponentLocation(), StationScreenPosition, false);
			if (!bPawnProjected || !bStationProjected
				|| !FMath::IsFinite(PawnScreenPosition.X) || !FMath::IsFinite(PawnScreenPosition.Y)
				|| !FMath::IsFinite(StationScreenPosition.X) || !FMath::IsFinite(StationScreenPosition.Y))
			{
				OutFailure = FString::Printf(
					TEXT("could not project gameplay subject pawnProjected=%d stationProjected=%d pawnScreen=%s stationScreen=%s"),
					bPawnProjected ? 1 : 0, bStationProjected ? 1 : 0,
					*PawnScreenPosition.ToString(), *StationScreenPosition.ToString());
				return false;
			}

			const FVector2D SubjectScreenPosition =
				(PawnScreenPosition + StationScreenPosition) * 0.5;
			if (SubjectScreenPosition.X < 0.0 || SubjectScreenPosition.Y < 0.0
				|| SubjectScreenPosition.X >= ViewportSize.X
				|| SubjectScreenPosition.Y >= ViewportSize.Y)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay pawn/station subject is outside viewport anchor=%s viewport=%dx%d"),
					*SubjectScreenPosition.ToString(), ViewportSize.X, ViewportSize.Y);
				return false;
			}
			const int32 SubjectHalfWidth = FMath::Clamp(
				FMath::RoundToInt(static_cast<double>(ViewportSize.X) * 0.10), 96, 240);
			const int32 SubjectHalfHeight = FMath::Clamp(
				FMath::RoundToInt(static_cast<double>(ViewportSize.Y) * 0.18), 96, 180);
			const int32 SubjectMinX = FMath::Clamp(
				FMath::FloorToInt(SubjectScreenPosition.X) - SubjectHalfWidth,
				0, ViewportSize.X - 1);
			const int32 SubjectMinY = FMath::Clamp(
				FMath::FloorToInt(SubjectScreenPosition.Y) - SubjectHalfHeight,
				0, ViewportSize.Y - 1);
			const int32 SubjectMaxX = FMath::Clamp(
				FMath::CeilToInt(SubjectScreenPosition.X) + SubjectHalfWidth,
				SubjectMinX + 1, ViewportSize.X);
			const int32 SubjectMaxY = FMath::Clamp(
				FMath::CeilToInt(SubjectScreenPosition.Y) + SubjectHalfHeight,
				SubjectMinY + 1, ViewportSize.Y);

			double SubjectBrightnessSum = 0.0;
			double SubjectBrightnessSquaredSum = 0.0;
			int64 SubjectNonBlackPixelCount = 0;
			int64 SubjectPixelCount = 0;
			for (int32 Y = SubjectMinY; Y < SubjectMaxY; ++Y)
			{
				for (int32 X = SubjectMinX; X < SubjectMaxX; ++X)
				{
					const FColor& Pixel = Pixels[Y * ViewportSize.X + X];
					const double Brightness = static_cast<double>(
						FMath::Max3(Pixel.R, Pixel.G, Pixel.B));
					SubjectBrightnessSum += Brightness;
					SubjectBrightnessSquaredSum += Brightness * Brightness;
					SubjectNonBlackPixelCount += Brightness > 8.0 ? 1 : 0;
					++SubjectPixelCount;
				}
			}
			const double SubjectMeanBrightness = SubjectBrightnessSum / SubjectPixelCount;
			const double SubjectBrightnessVariance = FMath::Max(
				0.0, SubjectBrightnessSquaredSum / SubjectPixelCount
					- SubjectMeanBrightness * SubjectMeanBrightness);
			const double SubjectNonBlackPixelRatio =
				static_cast<double>(SubjectNonBlackPixelCount) / SubjectPixelCount;
			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Camera] subjectROI=(%d,%d)-(%d,%d) pawnScreen=%s stationSpawnScreen=%s meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f"),
				SubjectMinX, SubjectMinY, SubjectMaxX, SubjectMaxY,
				*PawnScreenPosition.ToString(), *StationScreenPosition.ToString(),
				SubjectMeanBrightness, SubjectBrightnessVariance,
				SubjectNonBlackPixelRatio);
			if (SubjectMeanBrightness < MinimumSubjectMeanBrightness
				|| SubjectNonBlackPixelRatio < MinimumSubjectNonBlackPixelRatio
				|| SubjectBrightnessVariance < MinimumSubjectBrightnessVariance)
			{
				OutFailure = FString::Printf(
					TEXT("gameplay pawn/station ROI is unreadable meanBrightness=%.3f variance=%.3f nonBlackRatio=%.5f (minimum mean=%.3f ratio=%.3f variance=%.3f)"),
					SubjectMeanBrightness, SubjectBrightnessVariance,
					SubjectNonBlackPixelRatio, MinimumSubjectMeanBrightness,
					MinimumSubjectNonBlackPixelRatio,
					MinimumSubjectBrightnessVariance);
				return false;
			}
			return true;
		}

		bool UpdateWaitForScreenshot(UWorld* World, double Now)
		{
			if (!World || World != GameplayWorld.Get())
			{
				return false;
			}
			if (ScreenshotSettleFramesRemaining > 0)
			{
				--ScreenshotSettleFramesRemaining;
				return false;
			}

			FString CaptureFailure;
			if (!CaptureGameplayViewport(World, CaptureFailure))
			{
				if (!CaptureFailure.IsEmpty())
				{
					return Fail(CaptureFailure);
				}
				if (Now - StepStartSeconds > ScreenshotTimeoutSeconds)
				{
					return Fail(TEXT("game viewport pixels were unavailable for ten seconds"));
				}
				return false;
			}

			UE_LOG(LogTemp, Display,
				TEXT("[APS.Handoff.Smoke] Non-blank game viewport screenshot written; freezing every WorldScape producer"));
			Step = EStep::Cleanup;
			StepStartSeconds = Now;
			return false;
		}

		void BeginGameplayWorldScapeCleanup(UWorld* World)
		{
			if (GeneratedSaveSlotName.IsEmpty())
			{
				if (const AGravityPlayerController* GravityController =
					Cast<AGravityPlayerController>(World ? World->GetFirstPlayerController() : nullptr))
				{
					GeneratedSaveSlotName = GravityController->CurrentSaveSlotName;
				}
			}
			if (!GeneratedSaveSlotName.IsEmpty())
			{
				const bool bDeletedSave = UGameplayStatics::DeleteGameInSlot(GeneratedSaveSlotName, 0);
				const FString SidecarPath = FPaths::Combine(FPaths::ProjectSavedDir(),
					TEXT("SaveGames"), GeneratedSaveSlotName + TEXT(".apsmeta"));
				const bool bDeletedSidecar = !IFileManager::Get().FileExists(*SidecarPath)
					|| IFileManager::Get().Delete(*SidecarPath, false, true);
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.Smoke] Removed generated smoke save slot=%s save=%s sidecar=%s"),
					*GeneratedSaveSlotName, bDeletedSave ? TEXT("deleted") : TEXT("missing"),
					bDeletedSidecar ? TEXT("deleted/missing") : TEXT("FAILED"));
			}
			for (APlanetaryBody* Body : FindActors<APlanetaryBody>(World))
			{
				Body->bStreamWorldScapeSurface = false;
				Body->bGenerateByDefault = false;
			}
			for (APlanetarySurfaceGenerator* Surface :
				FindActors<APlanetarySurfaceGenerator>(World))
			{
				Surface->UnloadWorldScapeRoot();
			}
			for (AWorldScapeRoot* Root : FindActors<AWorldScapeRoot>(World))
			{
				Root->bFreezeGeneration = true;
				Root->SetActorTickEnabled(false);
				Root->SetActorHiddenInGame(true);
				Root->SetActorEnableCollision(false);
			}
		}

		bool AreGameplayWorldScapeWorkersDrained(UWorld* World)
		{
			bool bDrained = true;
			for (AWorldScapeRoot* Root : FindActors<AWorldScapeRoot>(World))
			{
				Root->bFreezeGeneration = true;
				Root->SetActorTickEnabled(false);
				if (Root->WorldScapeLodInGeneration.Num() > 0)
				{
					Root->CheckForLodGeneration();
				}
				if (Root->WorldScapeLodInGeneration.Num() > 0)
				{
					bDrained = false;
				}
				else
				{
					Root->bGenerateWorldScape = false;
				}
			}
			for (APlanetarySurfaceGenerator* Surface :
				FindActors<APlanetarySurfaceGenerator>(World))
			{
				if (Surface->bPendingWorldScapeUnload)
				{
					bDrained = false;
				}
			}
			return bDrained;
		}

		bool UpdateCleanup(UWorld* World, double Now)
		{
			if (!bCleanupStarted)
			{
				bCleanupStarted = true;
				if (World && World == GameplayWorld.Get())
				{
					BeginGameplayWorldScapeCleanup(World);
				}
			}

			bool bPreviewDrained = true;
			if (PreviewGenerator.IsValid())
			{
				bPreviewDrained = PreviewGenerator->PreparePreviewForTravel();
			}
			bool bGameplayDrained = true;
			if (World && World == GameplayWorld.Get())
			{
				bGameplayDrained = AreGameplayWorldScapeWorkersDrained(World);
			}
			if ((!bPreviewDrained || !bGameplayDrained)
				&& Now - StepStartSeconds <= CleanupTimeoutSeconds)
			{
				return false;
			}
			if (!bPreviewDrained || !bGameplayDrained)
			{
				if (PendingFailure.IsEmpty())
				{
					PendingFailure = TEXT("WorldScape workers did not drain within 20 seconds");
				}
			}

			if (!PendingFailure.IsEmpty())
			{
				Test->AddError(PendingFailure);
			}
			else
			{
				UE_LOG(LogTemp, Display,
					TEXT("[APS.Handoff.Smoke] PASS menu preview -> immutable handoff -> GravityGameMode -> exact hierarchy -> selected pawn -> starter attachments -> resolver WorldScape -> screenshot -> safe worker drain"));
			}
			return true;
		}

		FAutomationTestBase* Test{nullptr};
		EStep Step{EStep::OpenGenerator};
		double TestStartSeconds{0.0};
		double StepStartSeconds{0.0};
		TWeakObjectPtr<UWorld> MenuWorld;
		TWeakObjectPtr<UWorld> GameplayWorld;
		TWeakObjectPtr<AAstroGenerator> PreviewGenerator;
		TWeakObjectPtr<APlanet> RuntimeHomePlanet;
		TWeakObjectPtr<AGravityCharacterPawn> RuntimeGravityPawn;
		TWeakObjectPtr<ASpaceStation> RuntimeStation;
		TWeakObjectPtr<UClass> SelectedPawnClass;
		const UGeneratedWorld* EditableGeneratedWorldAddress{nullptr};
		const USpawnParameters* EditableSpawnParametersAddress{nullptr};
		FString SelectedPawnClassPath;
		FString GeneratedSaveSlotName;
		FString ScreenshotPath;
		FString PendingFailure;
		uint32 PreviewProfileSignature{0};
		int32 ScreenshotSettleFramesRemaining{0};
		bool bCleanupStarted{false};
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGeneratedCivilizationHandoffSmokeTest,
	"APS.Rendered.Gameplay.GeneratedCivilizationHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGeneratedCivilizationHandoffSmokeTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("[APS.Handoff.Smoke] Could not open the current MainMenu map"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSGeneratedGameplayHandoffSmokeTests::FGeneratedCivilizationHandoffCommand(this));
	return true;
}

#endif

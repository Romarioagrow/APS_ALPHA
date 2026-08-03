#include "WorldGenerationViewModel.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Enums/OrbitHeight.h"
#include "APS_ALPHA/Pawns/Base/ControlledPawn.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Astro/Galaxy.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "WorldGenerationViewModel"

void UWorldGenerationViewModel::Initialize(UObject* InWorldContext, UGeneratedWorld* InGeneratedWorld)
{
	WorldContext = InWorldContext;
	if (InGeneratedWorld)
	{
		InGeneratedWorld->PlanetsAmount = FMath::Clamp(InGeneratedWorld->PlanetsAmount, 1, 20);
		InGeneratedWorld->MoonsAmount = FMath::Clamp(InGeneratedWorld->MoonsAmount, 0, 10);
		InGeneratedWorld->PlanetRadius = FMath::Clamp(InGeneratedWorld->PlanetRadius, 100.0, 20000.0);
		InGeneratedWorld->StartPlanetIndex = FMath::Clamp(
			InGeneratedWorld->StartPlanetIndex, 1, InGeneratedWorld->PlanetsAmount);
	}
	UE_MVVM_SET_PROPERTY_VALUE(GeneratedWorld, InGeneratedWorld);
	if (UWorld* World = InWorldContext ? InWorldContext->GetWorld() : nullptr)
	{
		if (UMainGameplayInstance* GameplayInstance = World->GetGameInstance()
			? World->GetGameInstance()->GetSubsystem<UMainGameplayInstance>() : nullptr)
		{
			if (!GameplayInstance->SpawnParameters)
			{
				GameplayInstance->SpawnParameters = NewObject<USpawnParameters>(GameplayInstance);
			}
			UE_MVVM_SET_PROPERTY_VALUE(SpawnParameters, GameplayInstance->SpawnParameters);
		}
	}
	SetPreviewStatus(LOCTEXT("PreviewPending", "PREVIEW PENDING"), false);
}

void UWorldGenerationViewModel::Shutdown()
{
	if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
	}
	WorldContext.Reset();
	PreviewGenerator.Reset();
}

void UWorldGenerationViewModel::SetEnumValue(const UEnum* EnumClass, int32 SelectedValue)
{
	if (!GeneratedWorld || !EnumClass)
	{
		return;
	}

	FString PropertyName = EnumClass->GetName();
	if (PropertyName.StartsWith(TEXT("E")))
	{
		PropertyName.RightChopInline(1);
	}

	FProperty* Property = FindFProperty<FProperty>(UGeneratedWorld::StaticClass(), *PropertyName);
	if (!Property)
	{
		UE_LOG(LogTemp, Warning, TEXT("No UGeneratedWorld property matches enum %s"), *EnumClass->GetName());
		return;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(GeneratedWorld);
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(SelectedValue));
	}
	else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, static_cast<int64>(SelectedValue));
	}
	else
	{
		return;
	}

	RequestPreview();
}

void UWorldGenerationViewModel::SetGalaxySize(double Value)
{
	if (GeneratedWorld && GeneratedWorld->GalaxySize != FMath::RoundToInt(Value))
	{
		GeneratedWorld->GalaxySize = FMath::Max(1, FMath::RoundToInt(Value));
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetGalaxyStarCount(double Value)
{
	if (GeneratedWorld && GeneratedWorld->GalaxyStarCount != FMath::RoundToInt(Value))
	{
		GeneratedWorld->GalaxyStarCount = FMath::Max(1, FMath::RoundToInt(Value));
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetGalaxyStarDensity(double Value)
{
	if (GeneratedWorld && !FMath::IsNearlyEqual(GeneratedWorld->GalaxyStarDensity, Value))
	{
		GeneratedWorld->GalaxyStarDensity = FMath::Max(0.01, Value);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetPlanetRadius(double Value)
{
	if (GeneratedWorld && !FMath::IsNearlyEqual(GeneratedWorld->PlanetRadius, Value))
	{
		GeneratedWorld->PlanetRadius = FMath::Clamp(Value, 100.0, 20000.0);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetMoonsAmount(double Value)
{
	if (GeneratedWorld && GeneratedWorld->MoonsAmount != FMath::RoundToInt(Value))
	{
		GeneratedWorld->MoonsAmount = FMath::Clamp(FMath::RoundToInt(Value), 0, 10);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetPlanetsAmount(double Value)
{
	if (GeneratedWorld && GeneratedWorld->PlanetsAmount != FMath::RoundToInt(Value))
	{
		GeneratedWorld->PlanetsAmount = FMath::Clamp(FMath::RoundToInt(Value), 1, 20);
		GeneratedWorld->StartPlanetIndex = FMath::Clamp(
			GeneratedWorld->StartPlanetIndex, 1, GeneratedWorld->PlanetsAmount);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetStartPlanetIndex(double Value)
{
	if (!GeneratedWorld) return;

	const int32 MaxPlanetIndex = FMath::Max(1, GeneratedWorld->PlanetsAmount);
	const int32 NewIndex = FMath::Clamp(FMath::RoundToInt(Value), 1, MaxPlanetIndex);
	if (GeneratedWorld->StartPlanetIndex != NewIndex)
	{
		GeneratedWorld->StartPlanetIndex = NewIndex;
		RequestPreview();
	}
}

void UWorldGenerationViewModel::RequestPreview()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || !GeneratedWorld)
	{
		return;
	}

	// A parameter drag can enqueue many debounced rebuilds. Remember that a
	// useful live camera already existed before PreviewStatus temporarily marks
	// the scene as updating, so a local edit does not recenter orbit/zoom.
	if (!bForceRefocusOnNextPreview)
	{
		bPreserveCameraOnNextPreview = bPreserveCameraOnNextPreview || bPreviewReady;
	}
	else
	{
		bPreserveCameraOnNextPreview = false;
	}
	UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	SetPreviewStatus(LOCTEXT("PreviewUpdating", "UPDATING LIVE SCENE"), false);
	World->GetTimerManager().SetTimer(
		PreviewTimerHandle, this, &UWorldGenerationViewModel::ExecutePreview, 0.2f, false);
}

void UWorldGenerationViewModel::CancelPendingPreview()
{
	if (UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(PreviewTimerHandle);
	}
}

void UWorldGenerationViewModel::RegeneratePreviewVariant()
{
	if (AAstroGenerator* Generator = FindOrCreatePreviewGenerator())
	{
		Generator->AdvancePreviewGenerationSeed();
	}
	bForceRefocusOnNextPreview = true;
	RequestPreview();
}

void UWorldGenerationViewModel::SetPreviewFocus(EAstroPreviewFocus NewFocus)
{
	PreviewFocus = NewFocus;
	SelectedPreviewBody.Reset();
	// Before the first live model is ready an existing level generator may still
	// contain its old background world. Remember the desired focus, but do not
	// steer the camera toward an actor that RequestPreview is about to destroy.
	if (!bPreviewReady)
	{
		return;
	}
	if (AAstroGenerator* Generator = FindOrCreatePreviewGenerator())
	{
		APlayerController* PC = WorldContext.IsValid() && WorldContext->GetWorld()
			? WorldContext->GetWorld()->GetFirstPlayerController() : nullptr;
		Generator->FocusPreviewTarget(NewFocus, PC);
		UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Focus=%d generator=%s"),
			static_cast<int32>(NewFocus), *GetNameSafe(Generator));
	}
}

void UWorldGenerationViewModel::SetGenerationRoute(EAPSGenerationRoute NewRoute)
{
	if (!GeneratedWorld)
	{
		GenerationRoute = NewRoute;
		return;
	}

	const EAPSGenerationRoute PreviousRoute = GenerationRoute;
	GenerationRoute = NewRoute;
	bForceRefocusOnNextPreview = true;
	if (NewRoute == EAPSGenerationRoute::Planet)
	{
		GeneratedWorld->AstroGenerationLevel = EAstroGenerationLevel::SinglePlanet;
		GeneratedWorld->bGenerateHomeSystem = false;
		GeneratedWorld->bStartWithHomePlanet = true;
		GeneratedWorld->PlanetsAmount = 1;
		GeneratedWorld->StartPlanetIndex = 1;
	}
	else
	{
		// Leaving the dedicated planet route restores a complete astronomical
		// hierarchy. The user may still choose another generation level explicitly.
		if (PreviousRoute == EAPSGenerationRoute::Planet
			|| GeneratedWorld->AstroGenerationLevel == EAstroGenerationLevel::SinglePlanet)
		{
			GeneratedWorld->AstroGenerationLevel = EAstroGenerationLevel::StarCluster;
		}
		GeneratedWorld->bGenerateHomeSystem = true;
		GeneratedWorld->bStartWithHomePlanet = true;
		GeneratedWorld->PlanetsAmount = FMath::Max(1, GeneratedWorld->PlanetsAmount);
		GeneratedWorld->StartPlanetIndex = FMath::Clamp(
			GeneratedWorld->StartPlanetIndex, 1, GeneratedWorld->PlanetsAmount);
	}
	RequestPreview();
}

void UWorldGenerationViewModel::OrbitPreview(FVector2D ScreenDelta)
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->OrbitPreviewCamera(ScreenDelta);
	}
}

void UWorldGenerationViewModel::ZoomPreview(float WheelDelta)
{
	if (AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->ZoomPreviewCamera(WheelDelta);
	}
}

bool UWorldGenerationViewModel::FocusPreviewUnderCursor()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!World || !PlayerController)
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->GetMousePosition(MouseX, MouseY)
		|| !PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, RayOrigin, RayDirection))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstroPreviewPick), true);
	QueryParams.bTraceComplex = false;
	const bool bBlockingHit = World->LineTraceSingleByChannel(
		Hit, RayOrigin, RayOrigin + RayDirection * 1.0e15, ECC_Visibility, QueryParams);
	if (bBlockingHit)
	{
		for (AActor* Candidate = Hit.GetActor(); IsValid(Candidate); Candidate = Candidate->GetAttachParentActor())
		{
			if (Candidate->IsA<APlanet>() || Candidate->IsA<AMoon>() || Candidate->IsA<AStar>())
			{
				return FocusPreviewBody(Candidate);
			}
			if (Candidate->IsA<AStarSystem>()) { SetPreviewFocus(EAstroPreviewFocus::HomeSystem); return true; }
			if (Candidate->IsA<AGalaxy>()) { SetPreviewFocus(EAstroPreviewFocus::Galaxy); return true; }
			if (Candidate->IsA<AStarCluster>()) { SetPreviewFocus(EAstroPreviewFocus::StarCluster); return true; }
		}
		return false;
	}

	if (AAstroGenerator* Generator = PreviewGenerator.Get();
		Generator && Generator->FocusPreviewClusterSystemAtScreenPosition(
			PlayerController, FVector2D(MouseX, MouseY)))
	{
		PreviewFocus = EAstroPreviewFocus::HomeSystem;
		SelectedPreviewBody.Reset();
		UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
		return true;
	}
	return false;
}

void UWorldGenerationViewModel::GetPreviewBodyEntries(TArray<FAPSPreviewBodyEntry>& OutEntries) const
{
	OutEntries.Reset();
	if (const AAstroGenerator* Generator = PreviewGenerator.Get())
	{
		Generator->GetPreviewBodyEntries(OutEntries);
	}
}

bool UWorldGenerationViewModel::FocusPreviewBody(const TWeakObjectPtr<AActor>& BodyActor)
{
	AAstroGenerator* Generator = PreviewGenerator.Get();
	AActor* Actor = BodyActor.Get();
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!Generator || !Actor || !Generator->FocusPreviewBodyActor(Actor, PlayerController))
	{
		return false;
	}

	SelectedPreviewBody = Actor;
	PreviewFocus = Actor->IsA<AStar>() ? EAstroPreviewFocus::HomeStar : EAstroPreviewFocus::HomePlanet;
	return true;
}

void UWorldGenerationViewModel::SetSpawnClass(EAPSStartAssetSlot Slot, UClass* NewClass)
{
	if (!SpawnParameters || !NewClass)
	{
		return;
	}

	switch (Slot)
	{
	case EAPSStartAssetSlot::Character:
		if (NewClass->IsChildOf(AControlledPawn::StaticClass())) SpawnParameters->BP_CharacterClass = NewClass;
		break;
	case EAPSStartAssetSlot::Spaceship:
		if (NewClass->IsChildOf(ASpaceship::StaticClass())) SpawnParameters->BP_HomeSpaceship = NewClass;
		break;
	case EAPSStartAssetSlot::SpaceStation:
		if (NewClass->IsChildOf(ASpaceStation::StaticClass())) SpawnParameters->BP_HomeSpaceStation = NewClass;
		break;
	case EAPSStartAssetSlot::Headquarters:
		if (NewClass->IsChildOf(ASpaceHeadquarters::StaticClass())) SpawnParameters->BP_HomeSpaceHeadquarters = NewClass;
		break;
	case EAPSStartAssetSlot::Shipyard:
		if (NewClass->IsChildOf(ASpaceShipyard::StaticClass())) SpawnParameters->BP_HomeSpaceShipyard = NewClass;
		break;
	}
}

void UWorldGenerationViewModel::SetCharacterSpawnPlace(int32 Value)
{
	if (SpawnParameters)
	{
		const int32 MaxValue = StaticEnum<ECharSpawnPlace>()->NumEnums() - 2;
		SpawnParameters->CharacterSpawnPlace = static_cast<ECharSpawnPlace>(FMath::Clamp(Value, 0, MaxValue));
		if (GeneratedWorld
			&& (SpawnParameters->CharacterSpawnPlace == ECharSpawnPlace::MoonOrbit
				|| SpawnParameters->CharacterSpawnPlace == ECharSpawnPlace::MoonSurface))
		{
			// A lunar start must commit a hierarchy that actually contains a moon.
			// Keep the user's larger value, only repairing the impossible zero case.
			GeneratedWorld->MoonsAmount = FMath::Max(1, GeneratedWorld->MoonsAmount);
		}
	}
}

void UWorldGenerationViewModel::SetStationOrbitHeight(int32 Value)
{
	if (SpawnParameters)
	{
		const int32 MaxValue = StaticEnum<EOrbitHeight>()->NumEnums() - 2;
		SpawnParameters->HomeStationOrbitHeight = static_cast<EOrbitHeight>(FMath::Clamp(Value, 0, MaxValue));
	}
}

void UWorldGenerationViewModel::ExecutePreview()
{
	AAstroGenerator* Generator = FindOrCreatePreviewGenerator();
	if (!Generator)
	{
		SetPreviewStatus(LOCTEXT("PreviewGeneratorMissing", "PREVIEW GENERATOR UNAVAILABLE"), false);
		return;
	}

	const bool bGenerated = Generator->RegeneratePreview(GeneratedWorld);
	if (bGenerated && !bPreserveCameraOnNextPreview)
	{
		Generator->FocusPreviewTarget(PreviewFocus);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Preview generated=%s revision=%d focus=%d level=%d planets=%d moons=%d radius=%.0f"),
		bGenerated ? TEXT("true") : TEXT("false"), PreviewRevision, static_cast<int32>(PreviewFocus),
		GeneratedWorld ? static_cast<int32>(GeneratedWorld->AstroGenerationLevel) : -1,
		GeneratedWorld ? GeneratedWorld->PlanetsAmount : 0, GeneratedWorld ? GeneratedWorld->MoonsAmount : 0,
		GeneratedWorld ? GeneratedWorld->PlanetRadius : 0.0);
	SetPreviewStatus(
		bGenerated ? LOCTEXT("PreviewReady", "LIVE FULL-SCALE PREVIEW") : LOCTEXT("PreviewFailed", "PREVIEW NEEDS GENERATOR ASSETS"),
		bGenerated);
	bPreserveCameraOnNextPreview = false;
	bForceRefocusOnNextPreview = false;
}

AAstroGenerator* UWorldGenerationViewModel::FindOrCreatePreviewGenerator()
{
	if (PreviewGenerator.IsValid())
	{
		return PreviewGenerator.Get();
	}

	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	// A menu preview must never take ownership of a generator authored into a
	// gameplay map. Reusing the first AAstroGenerator used to let preview state
	// overwrite the legacy SinglePlay references. Only reuse an actor that this
	// view model explicitly created for preview work.
	for (TActorIterator<AAstroGenerator> It(World); It; ++It)
	{
		AAstroGenerator* ExistingGenerator = *It;
		if (IsValid(ExistingGenerator) && ExistingGenerator->ActorHasTag(TEXT("WorldGenerationPreview")))
		{
			InitializeSpawnDefaultsFromGenerator(ExistingGenerator);
			PreviewGenerator = ExistingGenerator;
			return ExistingGenerator;
		}
	}

	UClass* GeneratorClass = LoadClass<AAstroGenerator>(
		nullptr, TEXT("/Game/APS/APS_ALPHA/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
	if (!GeneratorClass)
	{
		GeneratorClass = LoadClass<AAstroGenerator>(
			nullptr, TEXT("/Game/APS/Core/BP_AstroGenerator.BP_AstroGenerator_C"));
	}
	if (!GeneratorClass)
	{
		return nullptr;
	}

	const FTransform PreviewTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	AAstroGenerator* NewGenerator = World->SpawnActorDeferred<AAstroGenerator>(
		GeneratorClass, PreviewTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (NewGenerator)
	{
		// Prevent Blueprint defaults from running the normal gameplay BeginPlay
		// generation before the preview model has been supplied.
		NewGenerator->bAutoGeneration = false;
		NewGenerator->Tags.AddUnique(TEXT("WorldGenerationPreview"));
		UGameplayStatics::FinishSpawningActor(NewGenerator, PreviewTransform);
		InitializeSpawnDefaultsFromGenerator(NewGenerator);
		PreviewGenerator = NewGenerator;
	}
	return NewGenerator;
}

void UWorldGenerationViewModel::InitializeSpawnDefaultsFromGenerator(AAstroGenerator* Generator)
{
	if (!Generator || !SpawnParameters)
	{
		return;
	}

	if (!SpawnParameters->BP_CharacterClass) SpawnParameters->BP_CharacterClass = Generator->BP_CharacterClass;
	if (!SpawnParameters->BP_HomeSpaceship) SpawnParameters->BP_HomeSpaceship = Generator->BP_HomeSpaceship;
	if (!SpawnParameters->BP_HomeSpaceStation) SpawnParameters->BP_HomeSpaceStation = Generator->BP_HomeSpaceStation;
	if (!SpawnParameters->BP_HomeSpaceHeadquarters) SpawnParameters->BP_HomeSpaceHeadquarters = Generator->BP_HomeSpaceHeadquarters;
	if (!SpawnParameters->BP_HomeSpaceShipyard) SpawnParameters->BP_HomeSpaceShipyard = Generator->BP_HomeSpaceShipyard;
}

void UWorldGenerationViewModel::CommitAndOpenLevel(FName LevelName)
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World || !GeneratedWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("[APS.WorldGeneration] Commit rejected: world context or generated model is missing"));
		return;
	}
	if (GenerationRoute == EAPSGenerationRoute::Civilization)
	{
		const auto IsSpawnableClass = [](const UClass* Class)
		{
			return Class && !Class->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_NotPlaceable);
		};
		const bool bClassesReady = SpawnParameters
			&& IsSpawnableClass(SpawnParameters->BP_CharacterClass)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceship)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceStation)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceHeadquarters)
			&& IsSpawnableClass(SpawnParameters->BP_HomeSpaceShipyard);
		const bool bHierarchyReady = GeneratedWorld->bGenerateHomeSystem
			&& GeneratedWorld->bStartWithHomePlanet
			&& GeneratedWorld->PlanetsAmount > 0
			&& GeneratedWorld->StartPlanetIndex > 0
			&& GeneratedWorld->StartPlanetIndex <= GeneratedWorld->PlanetsAmount;
		if (!bClassesReady || !bHierarchyReady)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[APS.Civilization] Commit rejected: classes=%s hierarchy=%s planets=%d start=%d"),
				bClassesReady ? TEXT("OK") : TEXT("INCOMPLETE"),
				bHierarchyReady ? TEXT("OK") : TEXT("INCOMPLETE"),
				GeneratedWorld->PlanetsAmount, GeneratedWorld->StartPlanetIndex);
			return;
		}
	}

	bool bHandoffCommitted = false;
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMainGameplayInstance* GameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			// This is a newly generated world, not a request to restore the last
			// selected save slot. Explicitly reset both modes before travel.
			GameplayInstance->bIsLoadingMode = false;
			GameplayInstance->bUseAuthoredSinglePlayWorld = false;
			GameplayInstance->bSpawnGeneratedCivilization =
				GenerationRoute == EAPSGenerationRoute::Civilization;
			GameplayInstance->SaveSlotName.Reset();
			// The editable model is owned by the menu controller. Duplicate it into
			// the GameInstance subsystem before travel so it survives destruction of
			// the menu world and remains the exact model consumed by gameplay.
			GameplayInstance->NewGeneratedWorld = DuplicateObject<UGeneratedWorld>(GeneratedWorld, GameplayInstance);
			if (!GameplayInstance->NewGeneratedWorld)
			{
				UE_LOG(LogTemp, Error, TEXT("[APS.WorldGeneration] Commit rejected: failed to persist generated model"));
				return;
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& !GameplayInstance->CurrentCivilization)
			{
				GameplayInstance->CurrentCivilization = NewObject<UCivilization>(GameplayInstance);
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& !GameplayInstance->CurrentCivilization)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[APS.Civilization] Commit rejected: failed to create runtime civilization"));
				GameplayInstance->NewGeneratedWorld = nullptr;
				GameplayInstance->bSpawnGeneratedCivilization = false;
				return;
			}
			if (GenerationRoute == EAPSGenerationRoute::Civilization
				&& GameplayInstance->CurrentCivilization && SpawnParameters)
			{
				// Freeze an independent GameInstance-owned snapshot across OpenLevel,
				// just like the generated astronomical model. The editable menu model
				// must not keep mutating the committed gameplay session during travel.
				GameplayInstance->SpawnParameters = DuplicateObject<USpawnParameters>(
					SpawnParameters, GameplayInstance);
				if (!GameplayInstance->SpawnParameters)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[APS.Civilization] Commit rejected: failed to persist spawn parameters"));
					GameplayInstance->CurrentCivilization = nullptr;
					GameplayInstance->bSpawnGeneratedCivilization = false;
					return;
				}
				GameplayInstance->CurrentCivilization->InitializeFromSpawnParameters(
					GameplayInstance->SpawnParameters);
				UE_LOG(LogTemp, Log, TEXT("[APS.Civilization] Committed '%s': population=%d tech=%d fleet=%d"),
					*GameplayInstance->CurrentCivilization->Name,
					GameplayInstance->CurrentCivilization->Population,
					GameplayInstance->CurrentCivilization->TechnologyLevel,
					GameplayInstance->CurrentCivilization->FleetSize);
			}
			else
			{
				// Space/planet creation commits the astronomical model only. Do not
				// leak an earlier civilization or its infrastructure into this route.
				GameplayInstance->CurrentCivilization = nullptr;
				GameplayInstance->SpawnParameters = nullptr;
				UE_LOG(LogTemp, Log, TEXT("[APS.WorldGeneration] Committed route=%d without civilization spawn data"),
					static_cast<int32>(GenerationRoute));
			}
			bHandoffCommitted = true;
		}
	}
	if (!bHandoffCommitted)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.WorldGeneration] Commit rejected: MainGameplayInstance subsystem is unavailable"));
		return;
	}

	UGameplayStatics::OpenLevel(World, LevelName);
}

void UWorldGenerationViewModel::SetPreviewStatus(const FText& Status, bool bReady)
{
	UE_MVVM_SET_PROPERTY_VALUE(PreviewStatus, Status);
	UE_MVVM_SET_PROPERTY_VALUE(bPreviewReady, bReady);
}

#undef LOCTEXT_NAMESPACE

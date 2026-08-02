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
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Gameplay/Civilizations/Civilization.h"
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
		InGeneratedWorld->PlanetsAmount = FMath::Max(1, InGeneratedWorld->PlanetsAmount);
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
		GeneratedWorld->PlanetRadius = FMath::Max(1.0, Value);
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetMoonsAmount(double Value)
{
	if (GeneratedWorld && GeneratedWorld->MoonsAmount != FMath::RoundToInt(Value))
	{
		GeneratedWorld->MoonsAmount = FMath::Max(0, FMath::RoundToInt(Value));
		RequestPreview();
	}
}

void UWorldGenerationViewModel::SetPlanetsAmount(double Value)
{
	if (GeneratedWorld && GeneratedWorld->PlanetsAmount != FMath::RoundToInt(Value))
	{
		GeneratedWorld->PlanetsAmount = FMath::Max(1, FMath::RoundToInt(Value));
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

	UE_MVVM_SET_PROPERTY_VALUE(PreviewRevision, PreviewRevision + 1);
	SetPreviewStatus(LOCTEXT("PreviewUpdating", "UPDATING LIVE SCENE"), false);
	World->GetTimerManager().SetTimer(
		PreviewTimerHandle, this, &UWorldGenerationViewModel::ExecutePreview, 0.2f, false);
}

void UWorldGenerationViewModel::RegeneratePreviewVariant()
{
	if (AAstroGenerator* Generator = FindOrCreatePreviewGenerator())
	{
		Generator->AdvancePreviewGenerationSeed();
	}
	RequestPreview();
}

void UWorldGenerationViewModel::SetPreviewFocus(EAstroPreviewFocus NewFocus)
{
	PreviewFocus = NewFocus;
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
	if (!World->LineTraceSingleByChannel(Hit, RayOrigin, RayOrigin + RayDirection * 1.0e15, ECC_Visibility, QueryParams))
	{
		return false;
	}

	for (AActor* Candidate = Hit.GetActor(); IsValid(Candidate); Candidate = Candidate->GetAttachParentActor())
	{
		if (Candidate->IsA<APlanet>()) { SetPreviewFocus(EAstroPreviewFocus::HomePlanet); return true; }
		if (Candidate->IsA<AStar>()) { SetPreviewFocus(EAstroPreviewFocus::HomeStar); return true; }
		if (Candidate->IsA<AStarSystem>()) { SetPreviewFocus(EAstroPreviewFocus::HomeSystem); return true; }
		if (Candidate->IsA<AGalaxy>()) { SetPreviewFocus(EAstroPreviewFocus::Galaxy); return true; }
		if (Candidate->IsA<AStarCluster>()) { SetPreviewFocus(EAstroPreviewFocus::StarCluster); return true; }
	}
	return false;
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
	if (bGenerated)
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

	if (AAstroGenerator* ExistingGenerator = Cast<AAstroGenerator>(
		UGameplayStatics::GetActorOfClass(World, AAstroGenerator::StaticClass())))
	{
		InitializeSpawnDefaultsFromGenerator(ExistingGenerator);
		PreviewGenerator = ExistingGenerator;
		return ExistingGenerator;
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

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAstroGenerator* NewGenerator = World->SpawnActor<AAstroGenerator>(
		GeneratorClass, FVector::ZeroVector, FRotator::ZeroRotator, ActorSpawnParameters);
	if (NewGenerator)
	{
		NewGenerator->Tags.AddUnique(TEXT("WorldGenerationPreview"));
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
		return;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UMainGameplayInstance* GameplayInstance = GameInstance->GetSubsystem<UMainGameplayInstance>())
		{
			// This is a newly generated world, not a request to restore the last
			// selected save slot. Explicitly reset both modes before travel.
			GameplayInstance->bIsLoadingMode = false;
			GameplayInstance->SaveSlotName.Reset();
			// The editable model is owned by the menu controller. Duplicate it into
			// the GameInstance subsystem before travel so it survives destruction of
			// the menu world and remains the exact model consumed by gameplay.
			GameplayInstance->NewGeneratedWorld = DuplicateObject<UGeneratedWorld>(GeneratedWorld, GameplayInstance);
			if (!GameplayInstance->CurrentCivilization)
			{
				GameplayInstance->CurrentCivilization = NewObject<UCivilization>(GameplayInstance);
			}
			GameplayInstance->CurrentCivilization->InitializeFromSpawnParameters(SpawnParameters);
			UE_LOG(LogTemp, Log, TEXT("[APS.Civilization] Committed '%s': population=%d tech=%d fleet=%d"),
				*GameplayInstance->CurrentCivilization->Name,
				GameplayInstance->CurrentCivilization->Population,
				GameplayInstance->CurrentCivilization->TechnologyLevel,
				GameplayInstance->CurrentCivilization->FleetSize);
		}
	}

	UGameplayStatics::OpenLevel(World, LevelName);
}

void UWorldGenerationViewModel::SetPreviewStatus(const FText& Status, bool bReady)
{
	UE_MVVM_SET_PROPERTY_VALUE(PreviewStatus, Status);
	UE_MVVM_SET_PROPERTY_VALUE(bPreviewReady, bReady);
}

#undef LOCTEXT_NAMESPACE

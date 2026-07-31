#include "WorldGenerationViewModel.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/World.h"
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

void UWorldGenerationViewModel::ExecutePreview()
{
	AAstroGenerator* Generator = FindOrCreatePreviewGenerator();
	if (!Generator)
	{
		SetPreviewStatus(LOCTEXT("PreviewGeneratorMissing", "PREVIEW GENERATOR UNAVAILABLE"), false);
		return;
	}

	const bool bGenerated = Generator->RegeneratePreview(GeneratedWorld);
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

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAstroGenerator* NewGenerator = World->SpawnActor<AAstroGenerator>(
		GeneratorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	if (NewGenerator)
	{
		NewGenerator->Tags.AddUnique(TEXT("WorldGenerationPreview"));
		PreviewGenerator = NewGenerator;
	}
	return NewGenerator;
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
			GameplayInstance->NewGeneratedWorld = GeneratedWorld;
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

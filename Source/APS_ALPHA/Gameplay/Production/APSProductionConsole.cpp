#include "APSProductionConsole.h"

#include "APSProductionSubsystem.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationIdentityComponent.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogAPSProductionConsole, Log, All);

namespace APSProductionConsoleRuntime
{
	const FName OpenAction(TEXT("APS.Production.Open"));
	const FName PanelRequestedCode(TEXT("APS.Production.PanelRequested"));
	const FName InvalidAction(TEXT("APS.Production.InvalidConsoleAction"));
	const FName PanelRejected(TEXT("APS.Production.PanelRequestRejected"));
}

AAPSProductionConsole::AAPSProductionConsole()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionRoot"));
	RootComponent = InteractionRoot;
	DisplayName.Namespace = TEXT("APSProduction");
	DisplayName.Key = TEXT("ProductionConsole");
	DisplayName.DefaultText = TEXT("Production Console");
}

void AAPSProductionConsole::BeginPlay()
{
	Super::BeginPlay();
	PromptId = FGuid::NewGuid();
	SubscribeToCivilizationLifecycle();
	FString Failure;
	InitializeProductionContext(Failure);
}

void AAPSProductionConsole::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeFromCivilizationLifecycle();
	if (bContextRegistered)
	{
		if (UWorld* World = GetWorld())
		{
			if (UAPSProductionSubsystem* Production = World->GetSubsystem<UAPSProductionSubsystem>())
			{
				FString Failure;
				if (!Production->UnregisterContext(RegisteredContextStableId, this,
					EAPSProductionAccessMode::ActorGated, Failure))
				{
					UE_LOG(LogAPSProductionConsole, Verbose,
						TEXT("[APS.Production.Console] context detach deferred actor=%s reason=%s"),
						*GetNameSafe(this), *Failure);
				}
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool AAPSProductionConsole::ConfigureCanonicalIdentity(const FGuid ContextStableId,
	const FGuid OwnerStableId, const FGuid SpawnPadStableId, FString& OutFailure)
{
	OutFailure.Reset();
	if (bContextRegistered)
	{
		OutFailure = TEXT("APS.Production.ConsoleAlreadyRegistered");
		return false;
	}
	if (!ContextStableId.IsValid() || !OwnerStableId.IsValid())
	{
		OutFailure = TEXT("APS.Production.ConsoleMissingCanonicalIdentity");
		return false;
	}
	if (Domain == EAPSProductionDomain::Shipyard && !SpawnPadStableId.IsValid())
	{
		OutFailure = TEXT("APS.Production.ConsoleMissingSpawnPadIdentity");
		return false;
	}
	ConfiguredContextStableId = ContextStableId;
	ConfiguredOwnerStableId = OwnerStableId;
	ConfiguredSpawnPadStableId = SpawnPadStableId;
	return true;
}

const UAPSCivilizationIdentityComponent* AAPSProductionConsole::FindIdentity(
	const AActor* Actor) const
{
	return IsValid(Actor)
		? Actor->FindComponentByClass<UAPSCivilizationIdentityComponent>() : nullptr;
}

bool AAPSProductionConsole::ResolveCanonicalIdentity(FGuid& OutContextStableId,
	FGuid& OutOwnerStableId, FGuid& OutSpawnPadStableId, FString& OutFailure) const
{
	OutContextStableId = ConfiguredContextStableId;
	OutOwnerStableId = ConfiguredOwnerStableId;
	OutSpawnPadStableId = ConfiguredSpawnPadStableId;
	OutFailure.Reset();

	if (!OutContextStableId.IsValid() || !OutOwnerStableId.IsValid())
	{
		TArray<const AActor*> Candidates;
		Candidates.Add(ContextIdentityActor.Get());
		Candidates.Add(this);
		Candidates.Add(GetOwner());
		for (const AActor* Parent = GetAttachParentActor(); IsValid(Parent);
			Parent = Parent->GetAttachParentActor())
		{
			Candidates.Add(Parent);
		}
		for (const AActor* Candidate : Candidates)
		{
			const UAPSCivilizationIdentityComponent* Identity = FindIdentity(Candidate);
			if (Identity && Identity->StableEntityId.IsValid()
				&& Identity->OwnerCivilizationId.IsValid())
			{
				OutContextStableId = Identity->StableEntityId;
				OutOwnerStableId = Identity->OwnerCivilizationId;
				break;
			}
		}
	}
	if (!OutSpawnPadStableId.IsValid() && IsValid(SpawnPadIdentityActor))
	{
		if (const UAPSCivilizationIdentityComponent* PadIdentity =
			FindIdentity(SpawnPadIdentityActor))
		{
			OutSpawnPadStableId = PadIdentity->StableEntityId;
		}
	}
	if (!OutContextStableId.IsValid() || !OutOwnerStableId.IsValid())
	{
		OutFailure = TEXT("APS.Production.ConsoleMissingCanonicalIdentity");
		return false;
	}
	if (Domain == EAPSProductionDomain::Shipyard && !OutSpawnPadStableId.IsValid())
	{
		OutFailure = TEXT("APS.Production.ConsoleMissingSpawnPadIdentity");
		return false;
	}
	return true;
}

bool AAPSProductionConsole::InitializeProductionContext(FString& OutFailure)
{
	OutFailure.Reset();
	if (bContextRegistered)
	{
		return true;
	}
	UWorld* World = GetWorld();
	UAPSProductionSubsystem* Production = World
		? World->GetSubsystem<UAPSProductionSubsystem>() : nullptr;
	if (!Production)
	{
		OutFailure = TEXT("APS.Production.ConsoleSubsystemUnavailable");
		return false;
	}
	FGuid ContextStableId;
	FGuid OwnerStableId;
	FGuid SpawnPadStableId;
	if (!ResolveCanonicalIdentity(ContextStableId, OwnerStableId,
		SpawnPadStableId, OutFailure))
	{
		return false;
	}
	FAPSProductionContextRegistration Registration;
	Registration.ContextStableId = ContextStableId;
	Registration.OwnerStableId = OwnerStableId;
	Registration.Domain = Domain;
	Registration.AccessMode = EAPSProductionAccessMode::ActorGated;
	Registration.ContextActor = this;
	Registration.QueueCapacity = QueueCapacity;
	Registration.MaximumConcurrentJobs = MaximumConcurrentJobs;
	Registration.SpawnPadStableId = SpawnPadStableId;
	if (!Production->RegisterContext(Registration, OutFailure))
	{
		return false;
	}
	RegisteredContextStableId = ContextStableId;
	bContextRegistered = true;
	UE_LOG(LogAPSProductionConsole, Display,
		TEXT("[APS.Production.Console] registered actor=%s context=%s owner=%s domain=%d"),
		*GetNameSafe(this), *ContextStableId.ToString(), *OwnerStableId.ToString(),
		static_cast<int32>(Domain));
	return true;
}

void AAPSProductionConsole::SubscribeToCivilizationLifecycle()
{
	UWorld* World = GetWorld();
	UAPSCivilizationMaterializationSubsystem* Subsystem = World
		? World->GetSubsystem<UAPSCivilizationMaterializationSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}
	CivilizationSubsystem = Subsystem;
	MaterializationStateChangedHandle = Subsystem->OnMaterializationStateChanged().AddUObject(
		this, &AAPSProductionConsole::HandleCivilizationMaterializationStateChanged);
	const FAPSCivilizationRuntimeManifest& Manifest = Subsystem->GetRuntimeManifest();
	const bool bActorReadyState = Manifest.MaterializationState
		== EAPSCivilizationMaterializationState::Materialized
		|| Manifest.MaterializationState == EAPSCivilizationMaterializationState::LoadedFromSave;
	if (bActorReadyState && Subsystem->IsMaterializationComplete())
	{
		FString Failure;
		InitializeProductionContext(Failure);
	}
}

void AAPSProductionConsole::UnsubscribeFromCivilizationLifecycle()
{
	if (UAPSCivilizationMaterializationSubsystem* Subsystem = CivilizationSubsystem.Get())
	{
		if (MaterializationStateChangedHandle.IsValid())
		{
			Subsystem->OnMaterializationStateChanged().Remove(MaterializationStateChangedHandle);
		}
	}
	MaterializationStateChangedHandle.Reset();
	CivilizationSubsystem.Reset();
}

void AAPSProductionConsole::HandleCivilizationMaterializationStateChanged(
	const FAPSCivilizationRuntimeManifest& Manifest,
	const EAPSCivilizationMaterializationState Previous,
	const EAPSCivilizationMaterializationState Current)
{
	const bool bActorReadyState = Current == EAPSCivilizationMaterializationState::Materialized
		|| Current == EAPSCivilizationMaterializationState::LoadedFromSave;
	UAPSCivilizationMaterializationSubsystem* Subsystem = CivilizationSubsystem.Get();
	if (bActorReadyState && Subsystem && Subsystem->IsMaterializationComplete())
	{
		FString Failure;
		if (!InitializeProductionContext(Failure))
		{
			UE_LOG(LogAPSProductionConsole, Warning,
				TEXT("[APS.Production.Console] actor-ready registration rejected actor=%s reason=%s"),
				*GetNameSafe(this), *Failure);
		}
	}
}

FGuid AAPSProductionConsole::GetInteractionTargetStableId_Implementation() const
{
	FGuid ContextStableId;
	FGuid OwnerStableId;
	FGuid SpawnPadStableId;
	FString Failure;
	return ResolveCanonicalIdentity(ContextStableId, OwnerStableId,
		SpawnPadStableId, Failure) ? ContextStableId : FGuid{};
}

EAPSInteractionActionKind AAPSProductionConsole::ResolveActionKind() const
{
	switch (Domain)
	{
	case EAPSProductionDomain::Crafting: return EAPSInteractionActionKind::Craft;
	case EAPSProductionDomain::Building: return EAPSInteractionActionKind::Build;
	case EAPSProductionDomain::Shipyard: return EAPSInteractionActionKind::Shipyard;
	default: return EAPSInteractionActionKind::Open;
	}
}

bool AAPSProductionConsole::QueryInteraction_Implementation(
	const FAPSInteractionContext& Context,
	FAPSInteractionPromptDescriptor& OutPrompt) const
{
	OutPrompt = FAPSInteractionPromptDescriptor{};
	if (!bContextRegistered || !PromptId.IsValid())
	{
		return false;
	}
	UWorld* World = GetWorld();
	const UAPSProductionSubsystem* Production = World
		? World->GetSubsystem<UAPSProductionSubsystem>() : nullptr;
	FAPSProductionSnapshot Snapshot;
	FString Failure;
	if (!Production || !Production->QuerySnapshot(RegisteredContextStableId,
		const_cast<AAPSProductionConsole*>(this), EAPSProductionAccessMode::ActorGated,
		Snapshot, Failure))
	{
		return false;
	}
	OutPrompt.PromptId = PromptId;
	OutPrompt.Revision = Snapshot.Revision;
	OutPrompt.PromptKind = EAPSInteractionPromptKind::OpenPanel;
	OutPrompt.ContextKind = PromptContextKind;
	OutPrompt.TargetStableId = RegisteredContextStableId;
	OutPrompt.ContextStableId = RegisteredContextStableId;
	OutPrompt.DisplayName = DisplayName;
	OutPrompt.RangeCm = InteractionRangeCm;
	OutPrompt.Availability = EAPSInteractionAvailability::Available;
	OutPrompt.Priority = 50;
	OutPrompt.bBlocking = false;
	OutPrompt.bDismissible = true;
	FAPSInteractionActionDescriptor& Action = OutPrompt.Actions.AddDefaulted_GetRef();
	Action.ActionId = APSProductionConsoleRuntime::OpenAction;
	Action.ActionKind = ResolveActionKind();
	Action.InputActionName = TEXT("Interact");
	Action.InputMode = EAPSInteractionInputMode::Press;
	return true;
}

FAPSInteractionExecutionResult AAPSProductionConsole::ExecuteInteraction_Implementation(
	const FAPSInteractionExecutionRequest& Request)
{
	FAPSInteractionExecutionResult Result;
	Result.CorrelationId = Request.CorrelationId;
	Result.Quantity = Request.Quantity;
	if (Request.ActionId != APSProductionConsoleRuntime::OpenAction)
	{
		Result.FailureCode = APSProductionConsoleRuntime::InvalidAction;
		return Result;
	}
	UWorld* World = GetWorld();
	UAPSProductionSubsystem* Production = World
		? World->GetSubsystem<UAPSProductionSubsystem>() : nullptr;
	FString Failure;
	if (!Production || !Production->RequestPanel(RegisteredContextStableId, this,
		EAPSProductionAccessMode::ActorGated, Failure))
	{
		Result.FailureCode = APSProductionConsoleRuntime::PanelRejected;
		return Result;
	}
	Result.Status = EAPSInteractionExecutionStatus::Succeeded;
	Result.ResultCode = APSProductionConsoleRuntime::PanelRequestedCode;
	return Result;
}

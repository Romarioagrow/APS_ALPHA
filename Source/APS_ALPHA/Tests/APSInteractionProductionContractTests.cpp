#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Gameplay/Interaction/APSInteractionSubsystem.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractionTypes.h"
#include "APS_ALPHA/Gameplay/Interaction/APSInteractable.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionConsole.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionEventSubsystem.h"
#include "APSInteractionContractTestActor.h"
#include "Engine/World.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace APSInteractionReflectionDiagnostic
{
	struct FQueryProcessEventOutcome
	{
		bool bInvoked{false};
		bool bReturnValue{false};
		FAPSInteractionPromptDescriptor Prompt;
		FString Failure;

		FString Describe() const
		{
			return FString::Printf(
				TEXT("invoked=%d return=%d promptId=%d revision=%lld target=%d actions=%d failure=%s"),
				bInvoked, bReturnValue, Prompt.PromptId.IsValid(),
				static_cast<long long>(Prompt.Revision), Prompt.TargetStableId.IsValid(),
				Prompt.Actions.Num(), *Failure);
		}
	};

	struct FExecuteProcessEventOutcome
	{
		bool bInvoked{false};
		FAPSInteractionExecutionResult Result;
		FString Failure;

		FString Describe() const
		{
			return FString::Printf(
				TEXT("invoked=%d status=%d correlation=%d verb=%s quantity=%d result=%s failureCode=%s probeFailure=%s"),
				bInvoked, static_cast<int32>(Result.Status), Result.CorrelationId.IsValid(),
				*Result.Verb.ToString(), Result.Quantity, *Result.ResultCode.ToString(),
				*Result.FailureCode.ToString(), *Failure);
		}
	};

	static FString DescribeFunction(
		const UFunction* Function,
		const UFunction* InterfaceFunction,
		const UFunction* ClassLocalFunction)
	{
		const UClass* OwnerClass = Function ? Cast<UClass>(Function->GetOuter()) : nullptr;
		const auto NativeFunction = Function ? Function->GetNativeFunc() : nullptr;
		const bool bNativeMatchesInterface = Function && InterfaceFunction
			&& NativeFunction && NativeFunction == InterfaceFunction->GetNativeFunc();
		const bool bNativeMatchesClassLocal = Function && ClassLocalFunction
			&& NativeFunction && NativeFunction == ClassLocalFunction->GetNativeFunc();
		return FString::Printf(
			TEXT("present=%d owner=%s flags=0x%08x native=%d sameInterface=%d sameLocal=%d nativeEqInterface=%d nativeEqLocal=%d"),
			Function != nullptr, OwnerClass ? *OwnerClass->GetName() : TEXT("None"),
			Function ? static_cast<uint32>(Function->FunctionFlags) : 0,
			NativeFunction != nullptr, Function && Function == InterfaceFunction,
			Function && Function == ClassLocalFunction, bNativeMatchesInterface,
			bNativeMatchesClassLocal);
	}

	static FQueryProcessEventOutcome ProbeQueryProcessEvent(
		UObject* Candidate,
		UFunction* Function,
		const FAPSInteractionContext& Context)
	{
		FQueryProcessEventOutcome Outcome;
		if (!Candidate || !Function)
		{
			Outcome.Failure = TEXT("MissingCandidateOrFunction");
			return Outcome;
		}

		FStructOnScope Parameters(Function);
		uint8* ParameterMemory = Parameters.GetStructMemory();
		FStructProperty* ContextProperty = FindFProperty<FStructProperty>(
			Function, TEXT("Context"));
		FStructProperty* PromptProperty = FindFProperty<FStructProperty>(
			Function, TEXT("OutPrompt"));
		FBoolProperty* ReturnProperty = CastField<FBoolProperty>(
			Function->GetReturnProperty());
		if (!ParameterMemory || !ContextProperty || !PromptProperty || !ReturnProperty)
		{
			Outcome.Failure = TEXT("IncompleteQueryParameterLayout");
			return Outcome;
		}

		ContextProperty->CopyCompleteValue(
			ContextProperty->ContainerPtrToValuePtr<void>(ParameterMemory), &Context);
		Candidate->ProcessEvent(Function, ParameterMemory);
		Outcome.bInvoked = true;
		Outcome.bReturnValue = ReturnProperty->GetPropertyValue_InContainer(ParameterMemory);
		PromptProperty->CopyCompleteValue(
			&Outcome.Prompt,
			PromptProperty->ContainerPtrToValuePtr<void>(ParameterMemory));
		return Outcome;
	}

	static FExecuteProcessEventOutcome ProbeExecuteProcessEvent(
		UObject* Candidate,
		UFunction* Function,
		const FAPSInteractionExecutionRequest& Request)
	{
		FExecuteProcessEventOutcome Outcome;
		if (!Candidate || !Function)
		{
			Outcome.Failure = TEXT("MissingCandidateOrFunction");
			return Outcome;
		}

		FStructOnScope Parameters(Function);
		uint8* ParameterMemory = Parameters.GetStructMemory();
		FStructProperty* RequestProperty = FindFProperty<FStructProperty>(
			Function, TEXT("Request"));
		FStructProperty* ReturnProperty = CastField<FStructProperty>(
			Function->GetReturnProperty());
		if (!ParameterMemory || !RequestProperty || !ReturnProperty)
		{
			Outcome.Failure = TEXT("IncompleteExecuteParameterLayout");
			return Outcome;
		}

		RequestProperty->CopyCompleteValue(
			RequestProperty->ContainerPtrToValuePtr<void>(ParameterMemory), &Request);
		Candidate->ProcessEvent(Function, ParameterMemory);
		Outcome.bInvoked = true;
		ReturnProperty->CopyCompleteValue(
			&Outcome.Result,
			ReturnProperty->ContainerPtrToValuePtr<void>(ParameterMemory));
		return Outcome;
	}

	static FString ProbeCandidate(
		UObject* Candidate,
		const FAPSInteractionContext& Context,
		const FAPSInteractionExecutionRequest& RequestedExecution,
		int64& OutObservedRevision)
	{
		OutObservedRevision = 0;
		const FName QueryName(TEXT("QueryInteraction"));
		const FName ExecuteName(TEXT("ExecuteInteraction"));
		UClass* InterfaceClass = UAPSInteractable::StaticClass();
		UClass* CandidateClass = Candidate ? Candidate->GetClass() : nullptr;
		UFunction* InterfaceQuery = InterfaceClass
			? InterfaceClass->FindFunctionByName(QueryName) : nullptr;
		UFunction* ResolvedQuery = Candidate ? Candidate->FindFunction(QueryName) : nullptr;
		UFunction* LocalQuery = CandidateClass
			? CandidateClass->FindFunctionByName(QueryName, EIncludeSuperFlag::ExcludeSuper)
			: nullptr;
		UFunction* InterfaceExecute = InterfaceClass
			? InterfaceClass->FindFunctionByName(ExecuteName) : nullptr;
		UFunction* ResolvedExecute = Candidate ? Candidate->FindFunction(ExecuteName) : nullptr;
		UFunction* LocalExecute = CandidateClass
			? CandidateClass->FindFunctionByName(ExecuteName, EIncludeSuperFlag::ExcludeSuper)
			: nullptr;

		const FQueryProcessEventOutcome ResolvedQueryOutcome =
			ProbeQueryProcessEvent(Candidate, ResolvedQuery, Context);
		const bool bQueryFunctionsMatch = ResolvedQuery && ResolvedQuery == LocalQuery;
		const FQueryProcessEventOutcome LocalQueryOutcome = bQueryFunctionsMatch
			? ResolvedQueryOutcome
			: ProbeQueryProcessEvent(Candidate, LocalQuery, Context);
		if (LocalQueryOutcome.bReturnValue)
		{
			OutObservedRevision = LocalQueryOutcome.Prompt.Revision;
		}
		else if (ResolvedQueryOutcome.bReturnValue)
		{
			OutObservedRevision = ResolvedQueryOutcome.Prompt.Revision;
		}

		FAPSInteractionExecutionRequest ProbeRequest = RequestedExecution;
		if (OutObservedRevision > 0)
		{
			ProbeRequest.ExpectedRevision = OutObservedRevision;
		}
		const FExecuteProcessEventOutcome ResolvedExecuteOutcome =
			ProbeExecuteProcessEvent(Candidate, ResolvedExecute, ProbeRequest);
		const bool bExecuteFunctionsMatch = ResolvedExecute && ResolvedExecute == LocalExecute;
		const FExecuteProcessEventOutcome LocalExecuteOutcome = bExecuteFunctionsMatch
			? ResolvedExecuteOutcome
			: ProbeExecuteProcessEvent(Candidate, LocalExecute, ProbeRequest);

		return FString::Printf(
			TEXT("class=%s implements=%d query{interface=[%s] resolved=[%s] local=[%s] resolvedPE=[%s] localPE=[%s] same=%d} execute{interface=[%s] resolved=[%s] local=[%s] resolvedPE=[%s] localPE=[%s] same=%d}"),
			CandidateClass ? *CandidateClass->GetName() : TEXT("None"),
			CandidateClass && CandidateClass->ImplementsInterface(InterfaceClass),
			*DescribeFunction(InterfaceQuery, InterfaceQuery, LocalQuery),
			*DescribeFunction(ResolvedQuery, InterfaceQuery, LocalQuery),
			*DescribeFunction(LocalQuery, InterfaceQuery, LocalQuery),
			*ResolvedQueryOutcome.Describe(), *LocalQueryOutcome.Describe(),
			bQueryFunctionsMatch,
			*DescribeFunction(InterfaceExecute, InterfaceExecute, LocalExecute),
			*DescribeFunction(ResolvedExecute, InterfaceExecute, LocalExecute),
			*DescribeFunction(LocalExecute, InterfaceExecute, LocalExecute),
			*ResolvedExecuteOutcome.Describe(), *LocalExecuteOutcome.Describe(),
			bExecuteFunctionsMatch);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSInteractionDescriptorContractTest,
	"APS.Gameplay.Interaction.DescriptorContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSInteractionDescriptorContractTest::RunTest(const FString& Parameters)
{
	FAPSInteractionActionDescriptor Action;
	Action.ActionId = TEXT("OpenConsole");
	Action.InputActionName = TEXT("Interact");
	Action.InputMode = EAPSInteractionInputMode::Press;

	FString Reason;
	TestTrue(TEXT("Enabled press action is structurally valid"),
		Action.IsStructurallyValid(&Reason));

	Action.InputMode = EAPSInteractionInputMode::Hold;
	Action.HoldDurationSeconds = 0.0;
	TestFalse(TEXT("Hold action requires positive duration"),
		Action.IsStructurallyValid(&Reason));
	Action.HoldDurationSeconds = 0.75;
	TestTrue(TEXT("Configured hold action is structurally valid"),
		Action.IsStructurallyValid(&Reason));

	FAPSInteractionPromptDescriptor Prompt;
	Prompt.PromptId = FGuid::NewGuid();
	Prompt.Revision = 3;
	Prompt.TargetStableId = FGuid::NewGuid();
	Prompt.TargetIdentityDomain = EAPSTargetIdentityDomain::CivilizationEntity;
	Prompt.ContextStableId = Prompt.TargetStableId;
	Prompt.DisplayName.Namespace = TEXT("APSInteraction");
	Prompt.DisplayName.Key = TEXT("HeadquartersConsole");
	Prompt.DistanceCm = 120.0;
	Prompt.RangeCm = 600.0;
	Prompt.Availability = EAPSInteractionAvailability::Available;
	Prompt.Actions.Add(Action);
	TestTrue(TEXT("Production prompt with canonical ID and action is valid"),
		Prompt.IsStructurallyValid(true, &Reason));

	Prompt.TargetIdentityDomain = EAPSTargetIdentityDomain::None;
	TestFalse(TEXT("Production prompt rejects missing target identity domain"),
		Prompt.IsStructurallyValid(true, &Reason));
	Prompt.TargetIdentityDomain = EAPSTargetIdentityDomain::CivilizationEntity;

	Prompt.TargetStableId.Invalidate();
	TestFalse(TEXT("Production prompt rejects missing canonical TargetStableId"),
		Prompt.IsStructurallyValid(true, &Reason));
	TestTrue(TEXT("Explicit debug prompt may omit canonical TargetStableId"),
		Prompt.IsStructurallyValid(false, &Reason));

	FAPSInteractionExecutionRequest Request;
	Request.CorrelationId = FGuid::NewGuid();
	Request.ActionId = Action.ActionId;
	Request.ExpectedRevision = Prompt.Revision;
	Request.SubjectStableId = FGuid::NewGuid();
	Request.TargetStableId = FGuid::NewGuid();
	Request.Quantity = 1;
	TestFalse(TEXT("Production request rejects a GUID without identity domain"),
		Request.IsStructurallyValid(true, &Reason));
	Request.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	TestFalse(TEXT("Production request also requires target identity domain"),
		Request.IsStructurallyValid(true, &Reason));
	Request.TargetIdentityDomain = EAPSTargetIdentityDomain::CivilizationEntity;
	TestTrue(TEXT("Production request with canonical subject/target IDs is valid"),
		Request.IsStructurallyValid(true, &Reason));
	Request.Quantity = 0;
	TestFalse(TEXT("Interaction request rejects zero quantity"),
		Request.IsStructurallyValid(true, &Reason));

	FAPSInteractionFocusScore Baseline;
	Baseline.Priority = 10;
	Baseline.FocusAlignment = 0.90;
	Baseline.DistanceCm = 200.0;
	Baseline.TargetStableId = FGuid(2, 0, 0, 0);
	FAPSInteractionFocusScore Candidate = Baseline;
	Candidate.Priority = 11;
	TestTrue(TEXT("Focus ordering prefers semantic priority first"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.FocusAlignment = 0.95;
	TestTrue(TEXT("Focus ordering prefers alignment second"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.DistanceCm = 150.0;
	TestTrue(TEXT("Focus ordering prefers distance third"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.TargetStableId = FGuid(1, 0, 0, 0);
	TestTrue(TEXT("Focus ordering uses canonical binary StableId last"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	Candidate = Baseline;
	Candidate.TargetStableId.Invalidate();
	TestFalse(TEXT("Focus ordering rejects unresolved canonical StableId"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Candidate, Baseline));
	FAPSInteractionFocusScore EmptyScore;
	TestTrue(TEXT("First valid focus candidate replaces empty score"),
		UAPSInteractionSubsystem::IsFocusScorePreferred(Baseline, EmptyScore));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSInteractionExecutionEventContractTest,
	"APS.Gameplay.Interaction.ExecutionEventContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSInteractionExecutionEventContractTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(false)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
		nullptr, false, ERHIFeatureLevel::Num, &InitializationValues);
	if (!TestNotNull(TEXT("Isolated Interaction test world exists"), World))
	{
		return false;
	}
	AAPSInteractionContractTestActor* Actor =
		World->SpawnActor<AAPSInteractionContractTestActor>(
			FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
	AAPSProductionConsole* Console = World->SpawnActor<AAPSProductionConsole>(
		FVector(200.0, 0.0, 0.0), FRotator::ZeroRotator);
	UAPSInteractionSubsystem* Interaction =
		NewObject<UAPSInteractionSubsystem>(World);
	if (!TestNotNull(TEXT("Contract interactable actor exists"), Actor)
		|| !TestNotNull(TEXT("Production console exists"), Console)
		|| !TestNotNull(TEXT("Interaction subsystem exists"), Interaction))
	{
		World->DestroyWorld(false);
		return false;
	}

	Actor->TargetStableId = FGuid::NewGuid();
	TestEqual(TEXT("Scene-only interactable falls back to finite actor location"),
		UAPSInteractionSubsystem::DistanceToActorBoundsCm(Actor, FVector::ZeroVector),
		100.0);
	Actor->TargetIdentityDomain = EAPSTargetIdentityDomain::CivilizationEntity;
	const FGuid ConsoleContextId = FGuid::NewGuid();
	const FGuid ConsoleOwnerId = FGuid::NewGuid();
	FString ConsoleSetupFailure;
	const bool bConsoleConfigured = Console->ConfigureCanonicalIdentity(
		ConsoleContextId, EAPSTargetIdentityDomain::CivilizationEntity,
		ConsoleOwnerId, FGuid{}, ConsoleSetupFailure);
	if (bConsoleConfigured)
	{
		Console->DispatchBeginPlay();
	}
	const bool bConsoleInitialized = bConsoleConfigured
		&& Console->InitializeProductionContext(ConsoleSetupFailure);
	FAPSInteractionContext Context;
	Context.InstigatorActor = Actor;
	Context.SubjectStableId = FGuid::NewGuid();
	Context.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	Context.ViewOrigin = FVector::ZeroVector;
	Context.ViewDirection = FVector::ForwardVector;
	Context.MaximumRangeCm = 600.0;
	Context.MinimumFocusDot = 0.5;

	FAPSInteractionExecutionRequest Request;
	Request.CorrelationId = FGuid::NewGuid();
	Request.ActionId = Actor->ActionId;
	Request.ExpectedRevision = Actor->Revision;
	Request.SubjectStableId = Context.SubjectStableId;
	Request.SubjectIdentityDomain = Context.SubjectIdentityDomain;
	Request.TargetStableId = Actor->TargetStableId;
	Request.TargetIdentityDomain = Actor->TargetIdentityDomain;
	Request.Quantity = 1;
	Request.InstigatorActor = Context.InstigatorActor;

	FAPSInteractionExecutionRequest ConsoleRequest = Request;
	ConsoleRequest.CorrelationId = FGuid::NewGuid();
	ConsoleRequest.ActionId = TEXT("APS.Production.Open");
	ConsoleRequest.ExpectedRevision = 0;
	ConsoleRequest.TargetStableId = ConsoleContextId;
	ConsoleRequest.TargetIdentityDomain = EAPSTargetIdentityDomain::CivilizationEntity;

	// Evidence-only reflection probe. It compares interface, runtime-resolved and
	// class-local UFunctions and observes ProcessEvent parameter/result mutation.
	// It never calls _Implementation directly and is not a gameplay fallback.
	int64 FixtureObservedRevision = 0;
	const FString FixtureReflectionProbe =
		APSInteractionReflectionDiagnostic::ProbeCandidate(
			Actor, Context, Request, FixtureObservedRevision);
	Actor->ExecutionCount = 0;
	int64 ConsoleObservedRevision = 0;
	const FString ConsoleReflectionProbe = bConsoleInitialized
		? APSInteractionReflectionDiagnostic::ProbeCandidate(
			Console, Context, ConsoleRequest, ConsoleObservedRevision)
		: FString::Printf(TEXT("consoleNotInitialized setupFailure=%s"),
			*ConsoleSetupFailure);
	if (ConsoleObservedRevision > 0)
	{
		ConsoleRequest.ExpectedRevision = ConsoleObservedRevision;
	}
	Actor->ExecutionCount = 0;

	// Contract both native implementers through canonical reflected Execute_ dispatch.
	// Direct _Implementation calls remain forbidden.
	FAPSInteractionPromptDescriptor FixtureInterfacePrompt;
	const bool bFixtureInterfaceQuery = IAPSInteractable::Execute_QueryInteraction(
		Actor, Context, FixtureInterfacePrompt);
	const FAPSInteractionExecutionResult FixtureInterfaceResult =
		IAPSInteractable::Execute_ExecuteInteraction(Actor, Request);
	Actor->ExecutionCount = 0;

	FAPSInteractionPromptDescriptor ConsoleInterfacePrompt;
	const bool bConsoleInterfaceQuery = bConsoleInitialized
		&& IAPSInteractable::Execute_QueryInteraction(
			Console, Context, ConsoleInterfacePrompt);
	if (bConsoleInterfaceQuery)
	{
		ConsoleRequest.ExpectedRevision = ConsoleInterfacePrompt.Revision;
	}
	const FAPSInteractionExecutionResult ConsoleInterfaceResult = bConsoleInitialized
		? IAPSInteractable::Execute_ExecuteInteraction(Console, ConsoleRequest)
		: FAPSInteractionExecutionResult{};

	const bool bFixtureInterfaceExecute = FixtureInterfaceResult.Status
		== EAPSInteractionExecutionStatus::Succeeded;
	const bool bConsoleInterfaceExecute = ConsoleInterfaceResult.Status
		== EAPSInteractionExecutionStatus::Succeeded;
	if (!bFixtureInterfaceQuery || !bFixtureInterfaceExecute
		|| !bConsoleInitialized || !bConsoleInterfaceQuery || !bConsoleInterfaceExecute)
	{
		AddError(FString::Printf(
			TEXT("Native Execute_ dispatch contract failed: fixtureQuery=%d fixtureExecute=%d fixtureFailure=%s consoleSetup=%d consoleQuery=%d consoleExecute=%d consoleFailure=%s setupFailure=%s fixtureReflection={%s} consoleReflection={%s}"),
			bFixtureInterfaceQuery, bFixtureInterfaceExecute,
			*FixtureInterfaceResult.FailureCode.ToString(), bConsoleInitialized,
			bConsoleInterfaceQuery, bConsoleInterfaceExecute,
			*ConsoleInterfaceResult.FailureCode.ToString(), *ConsoleSetupFailure,
			*FixtureReflectionProbe, *ConsoleReflectionProbe));
		World->DestroyWorld(false);
		return false;
	}

	FAPSInteractionPromptDescriptor ProbePrompt;
	FString ProbeFailure;
	if (!Interaction->QueryActor(Actor, Context, ProbePrompt, ProbeFailure))
	{
		AddError(FString::Printf(
			TEXT("Pre-execution QueryActor rejected the fixture: reason=%s"),
			*ProbeFailure));
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Probe target identity matches request"),
		ProbePrompt.TargetStableId, Request.TargetStableId);
	TestEqual(TEXT("Probe target domain matches request"),
		ProbePrompt.TargetIdentityDomain, Request.TargetIdentityDomain);
	TestEqual(TEXT("Probe revision matches request"),
		ProbePrompt.Revision, Request.ExpectedRevision);

	int32 BroadcastCount = 0;
	FAPSInteractionExecutionEvent LastEvent;
	Interaction->OnExecutionPublished().AddLambda(
		[&BroadcastCount, &LastEvent](const FAPSInteractionExecutionEvent& Event)
		{
			++BroadcastCount;
			LastEvent = Event;
		});
	FAPSInteractionExecutionResult Result =
		Interaction->ExecuteActor(Actor, Context, Request);
	if (Result.Status != EAPSInteractionExecutionStatus::Succeeded)
	{
		AddError(FString::Printf(
			TEXT("Pre-publication ExecuteActor rejected the fixture: status=%d failure=%s verb=%s quantity=%d"),
			static_cast<int32>(Result.Status), *Result.FailureCode.ToString(),
			*Result.Verb.ToString(), Result.Quantity));
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Exactly one execution event broadcasts"), BroadcastCount, 1);
	TestEqual(TEXT("Actor executes exactly once"), Actor->ExecutionCount, 1);
	TestTrue(TEXT("Interaction stream identity is owner-generated"),
		LastEvent.StreamId.IsValid());
	TestEqual(TEXT("Published stream matches subsystem"), LastEvent.StreamId,
		Interaction->GetExecutionStreamId());
	TestTrue(TEXT("Event identity is owner-generated"), LastEvent.EventId.IsValid());
	TestEqual(TEXT("First execution sequence is one"), LastEvent.Sequence, int64{1});
	TestEqual(TEXT("Explicit verb is preserved"), LastEvent.Verb, Actor->Verb);
	TestEqual(TEXT("Validated action identity is preserved"), LastEvent.ActionId,
		Request.ActionId);
	TestEqual(TEXT("Correlation identity is preserved"), LastEvent.CorrelationId,
		Request.CorrelationId);
	TestEqual(TEXT("Subject identity is preserved"), LastEvent.SubjectStableId,
		Context.SubjectStableId);
	TestEqual(TEXT("Subject domain is GameplayEntity"),
		LastEvent.SubjectIdentityDomain, EAPSSubjectIdentityDomain::GameplayEntity);
	TestEqual(TEXT("Base target identity is preserved"), LastEvent.TargetStableId,
		Actor->TargetStableId);
	TestEqual(TEXT("Base target domain is CivilizationEntity"),
		LastEvent.TargetIdentityDomain, EAPSTargetIdentityDomain::CivilizationEntity);
	TestEqual(TEXT("Executed quantity is preserved"), LastEvent.Quantity, 1);
	TestEqual(TEXT("Execution result code is preserved"), LastEvent.ResultCode,
		Actor->ResultCode);
	TestFalse(TEXT("Production event is never debug-classified"), LastEvent.bDebugOnly);

	FAPSInteractionExecutionRequest MismatchedDomain = Request;
	MismatchedDomain.CorrelationId = FGuid::NewGuid();
	MismatchedDomain.TargetIdentityDomain = EAPSTargetIdentityDomain::GameplayEntity;
	Result = Interaction->ExecuteActor(Actor, Context, MismatchedDomain);
	TestEqual(TEXT("Target-domain alias is rejected before execution"),
		Result.Status, EAPSInteractionExecutionStatus::Failed);
	TestEqual(TEXT("Target-domain alias does not execute actor"),
		Actor->ExecutionCount, 1);
	TestEqual(TEXT("Target-domain alias publishes no event"), BroadcastCount, 1);

	Actor->ExecutionStatus = EAPSInteractionExecutionStatus::Deferred;
	Actor->ResultCode = TEXT("APS.Interaction.Deferred");
	Request.CorrelationId = FGuid::NewGuid();
	Result = Interaction->ExecuteActor(Actor, Context, Request);
	TestEqual(TEXT("Deferred result remains deferred"), Result.Status,
		EAPSInteractionExecutionStatus::Deferred);
	TestEqual(TEXT("Deferred event broadcasts once"), BroadcastCount, 2);
	TestEqual(TEXT("Deferred is not reinterpreted as success"), LastEvent.Status,
		EAPSInteractionExecutionStatus::Deferred);

	Actor->ExecutionStatus = EAPSInteractionExecutionStatus::Failed;
	Actor->ResultCode = NAME_None;
	Actor->FailureCode = TEXT("APS.Interaction.ActorRejected");
	Request.CorrelationId = FGuid::NewGuid();
	Result = Interaction->ExecuteActor(Actor, Context, Request);
	TestEqual(TEXT("Executed failure remains failed"), Result.Status,
		EAPSInteractionExecutionStatus::Failed);
	TestEqual(TEXT("Executed failure publishes once"), BroadcastCount, 3);
	TestEqual(TEXT("Failure code is preserved"), LastEvent.FailureCode,
		Actor->FailureCode);

	Actor->ExecutionStatus = EAPSInteractionExecutionStatus::Succeeded;
	Actor->ResultCode = TEXT("APS.Production.PanelRequested");
	Actor->FailureCode = NAME_None;
	Actor->Verb = NAME_None;
	Request.CorrelationId = FGuid::NewGuid();
	Result = Interaction->ExecuteActor(Actor, Context, Request);
	TestEqual(TEXT("Actor result without explicit verb is rejected"), Result.FailureCode,
		FName(TEXT("APS.Interaction.InvalidExecutionResult")));
	TestEqual(TEXT("Invalid execution result publishes no event"), BroadcastCount, 3);

	int32 LateSubscriberCount = 0;
	Interaction->OnExecutionPublished().AddLambda(
		[&LateSubscriberCount](const FAPSInteractionExecutionEvent&)
		{
			++LateSubscriberCount;
		});
	TestEqual(TEXT("Late subscriber receives no historical replay"), LateSubscriberCount, 0);

	Actor->Verb = TEXT("APS.Interaction.Open");
	Actor->TargetStableId.Invalidate();
	Actor->TargetIdentityDomain = EAPSTargetIdentityDomain::None;
	Context.SubjectStableId.Invalidate();
	Context.SubjectIdentityDomain = EAPSSubjectIdentityDomain::None;
	Context.bDebugIdentityOverride = true;
	Request.CorrelationId = FGuid::NewGuid();
	Request.SubjectStableId.Invalidate();
	Request.SubjectIdentityDomain = EAPSSubjectIdentityDomain::None;
	Request.TargetStableId.Invalidate();
	Request.TargetIdentityDomain = EAPSTargetIdentityDomain::None;
	Result = Interaction->ExecuteActor(Actor, Context, Request);
	TestEqual(TEXT("Explicit debug execution succeeds"), Result.Status,
		EAPSInteractionExecutionStatus::Succeeded);
	TestEqual(TEXT("Debug result publishes exactly once"), BroadcastCount, 4);
	TestTrue(TEXT("Debug publication remains explicitly classified"),
		LastEvent.bDebugOnly);
	TestEqual(TEXT("Sequence advances only for published events"),
		Interaction->GetLastExecutionSequence(), int64{4});
	TestEqual(TEXT("Late subscriber receives only the new event"), LateSubscriberCount, 1);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionEventLifecycleContractTest,
	"APS.Gameplay.Production.EventLifecycleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionEventLifecycleContractTest::RunTest(const FString& Parameters)
{
	UAPSProductionEventSubsystem* Events = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Production event subsystem can be created for contract test"), Events);
	if (!Events)
	{
		return false;
	}

	int32 BroadcastCount = 0;
	Events->OnEventPublished().AddLambda(
		[&BroadcastCount](const FAPSProductionEvent&)
		{
			++BroadcastCount;
		});

	FAPSProductionEventPublishPolicy Policy;
	FAPSProductionEvent Requested;
	Requested.Verb = TEXT("APS.Shipyard.Enqueue");
	Requested.SubjectStableId = FGuid::NewGuid();
	Requested.TargetStableId = FGuid::NewGuid();
	Requested.DefinitionId = FPrimaryAssetId(TEXT("Ship"), TEXT("APS.TestShip"));
	Requested.DefinitionSchemaVersion = 1;
	Requested.Quantity = 2;
	Requested.Result = EAPSProductionEventResult::Requested;

	FString Failure;
	TestFalse(TEXT("Actor-gated publication rejects a GUID without identity domain"),
		Events->PublishEvent(Requested, Policy, Failure));
	Requested.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	TestTrue(TEXT("Requested event publishes"),
		Events->PublishEvent(Requested, Policy, Failure));
	TestTrue(TEXT("Publisher assigns EventId"), Requested.EventId.IsValid());
	TestTrue(TEXT("Publisher assigns CorrelationId"), Requested.CorrelationId.IsValid());
	TestTrue(TEXT("Publisher assigns Production-owned StreamId"), Requested.StreamId.IsValid());
	TestEqual(TEXT("Subsystem exposes the same StreamId"), Events->GetStreamId(), Requested.StreamId);
	TestEqual(TEXT("First event sequence is one"), Requested.Sequence, int64{1});

	FAPSProductionEvent Started = Requested;
	Started.EventId.Invalidate();
	Started.Result = EAPSProductionEventResult::Started;
	Started.Sequence = 0;
	TestTrue(TEXT("Requested may transition to Started"),
		Events->PublishEvent(Started, Policy, Failure));

	FAPSProductionEvent Drifted = Started;
	Drifted.EventId.Invalidate();
	Drifted.TargetStableId = FGuid::NewGuid();
	Drifted.Result = EAPSProductionEventResult::Succeeded;
	Drifted.Sequence = 0;
	TestFalse(TEXT("Correlation rejects target payload drift"),
		Events->PublishEvent(Drifted, Policy, Failure));

	FAPSProductionEvent Succeeded = Started;
	Succeeded.EventId.Invalidate();
	Succeeded.Result = EAPSProductionEventResult::Succeeded;
	Succeeded.ResultCode = TEXT("APS.Shipyard.JobCompleted");
	Succeeded.Sequence = 0;
	TestTrue(TEXT("Started may transition to terminal success"),
		Events->PublishEvent(Succeeded, Policy, Failure));

	FAPSProductionEvent DuplicateTerminal = Succeeded;
	DuplicateTerminal.EventId.Invalidate();
	DuplicateTerminal.Sequence = 0;
	TestFalse(TEXT("Correlation accepts only one terminal result"),
		Events->PublishEvent(DuplicateTerminal, Policy, Failure));
	TestEqual(TEXT("Only accepted lifecycle events broadcast"), BroadcastCount, 3);
	TestEqual(TEXT("Rejected events do not consume sequence"),
		Events->GetLastSequence(), int64{3});

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSProductionEventPersistenceContractTest,
	"APS.Gameplay.Production.EventPersistenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSProductionEventPersistenceContractTest::RunTest(const FString& Parameters)
{
	UAPSProductionEventSubsystem* Source = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Source event subsystem exists"), Source);
	if (!Source)
	{
		return false;
	}

	FAPSProductionEventPublishPolicy ProductionPolicy;
	FAPSProductionEvent Requested;
	Requested.Verb = TEXT("APS.Building.Build");
	Requested.SubjectStableId = FGuid::NewGuid();
	Requested.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	Requested.TargetStableId = FGuid::NewGuid();
	Requested.DefinitionId = FPrimaryAssetId(TEXT("Buildable"), TEXT("APS.TestHQ"));
	Requested.DefinitionSchemaVersion = 2;
	Requested.Quantity = 1;
	Requested.Result = EAPSProductionEventResult::Requested;
	FString Failure;
	TestTrue(TEXT("Production request publishes before save"),
		Source->PublishEvent(Requested, ProductionPolicy, Failure));

	FAPSProductionEvent Started = Requested;
	Started.EventId.Invalidate();
	Started.Sequence = 0;
	Started.Result = EAPSProductionEventResult::Started;
	TestTrue(TEXT("Production lifecycle reaches Started before save"),
		Source->PublishEvent(Started, ProductionPolicy, Failure));

	FAPSProductionEventPublishPolicy DebugPolicy;
	DebugPolicy.bDebugOnly = true;
	FAPSProductionEvent DebugRequested;
	DebugRequested.Verb = TEXT("APS.Crafting.Craft");
	DebugRequested.Result = EAPSProductionEventResult::Requested;
	TestTrue(TEXT("Explicit debug event publishes without canonical actor IDs"),
		Source->PublishEvent(DebugRequested, DebugPolicy, Failure));

	FAPSProductionEventStreamState Persisted;
	Source->ExportStreamState(Persisted);
	TestEqual(TEXT("Global sequence includes accepted debug events"),
		Persisted.LastSequence, int64{3});
	TestTrue(TEXT("Persisted stream has Production-owned identity"), Persisted.StreamId.IsValid());
	TestEqual(TEXT("Persisted stream identity matches source"), Persisted.StreamId, Requested.StreamId);
	TestEqual(TEXT("Debug-only correlation is excluded from persistence"),
		Persisted.Correlations.Num(), 1);
	TestEqual(TEXT("Persisted correlation is the production lifecycle"),
		Persisted.Correlations[0].CorrelationId, Requested.CorrelationId);

	UAPSProductionEventSubsystem* Restored = NewObject<UAPSProductionEventSubsystem>();
	TestNotNull(TEXT("Restored event subsystem exists"), Restored);
	if (!Restored)
	{
		return false;
	}
	int32 RestoreBroadcastCount = 0;
	Restored->OnEventPublished().AddLambda(
		[&RestoreBroadcastCount](const FAPSProductionEvent&)
		{
			++RestoreBroadcastCount;
		});
	TestTrue(TEXT("Persisted event stream restores"),
		Restored->RestoreStreamState(Persisted, Failure));
	TestEqual(TEXT("Restore never rebroadcasts historical events"),
		RestoreBroadcastCount, 0);
	TestEqual(TEXT("Restore keeps the same Production stream identity"),
		Restored->GetStreamId(), Persisted.StreamId);

	FAPSProductionEvent Succeeded = Started;
	Succeeded.EventId.Invalidate();
	Succeeded.Sequence = 0;
	Succeeded.Result = EAPSProductionEventResult::Succeeded;
	Succeeded.ResultCode = TEXT("APS.Building.JobCompleted");
	TestTrue(TEXT("Restored Started correlation may complete"),
		Restored->PublishEvent(Succeeded, ProductionPolicy, Failure));
	TestEqual(TEXT("Sequence continues after restored stream"),
		Succeeded.Sequence, int64{4});

	FAPSProductionEvent DuplicateTerminal = Succeeded;
	DuplicateTerminal.EventId.Invalidate();
	DuplicateTerminal.Sequence = 0;
	TestFalse(TEXT("Restored stream still rejects duplicate terminal result"),
		Restored->PublishEvent(DuplicateTerminal, ProductionPolicy, Failure));
	return true;
}

#endif

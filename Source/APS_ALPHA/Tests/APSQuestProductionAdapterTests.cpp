#include "APS_ALPHA/Gameplay/Quests/Adapters/APSProductionQuestAdapter.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSQuestProductionAdapterTest,
	"APS.Quest.Adapters.ProductionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSQuestProductionAdapterTest::RunTest(const FString& Parameters)
{
	FAPSProductionEvent Source;
	Source.EventId = FGuid(1, 2, 3, 4);
	Source.StreamId = FGuid(17, 18, 19, 20);
	Source.CorrelationId = FGuid(5, 6, 7, 8);
	Source.Verb = TEXT("APS.Build.Place");
	Source.SubjectStableId = FGuid(9, 10, 11, 12);
	Source.SubjectIdentityDomain = EAPSSubjectIdentityDomain::GameplayEntity;
	Source.TargetStableId = FGuid(13, 14, 15, 16);
	Source.DefinitionId = FPrimaryAssetId(
		FPrimaryAssetType(TEXT("APSBuildable")), FName(TEXT("Foundation")));
	Source.DefinitionSchemaVersion = 3;
	Source.Quantity = 2;
	Source.Result = EAPSProductionEventResult::Succeeded;
	Source.ResultCode = TEXT("APS.Build.Foundation");
	Source.Sequence = 7;

	FAPSQuestEvent Normalized;
	FString Reason;
	TestTrue(TEXT("Committed production event normalizes"),
		FAPSProductionQuestAdapter::Normalize(Source, Normalized, Reason));
	TestEqual(TEXT("Production-owned StreamId is preserved"), Normalized.StreamId,
		Source.StreamId);
	TestEqual(TEXT("Event identity is preserved"), Normalized.EventId, Source.EventId);
	TestEqual(TEXT("Correlation is preserved"), Normalized.CorrelationId, Source.CorrelationId);
	TestEqual(TEXT("Sequence is authoritative"), Normalized.Sequence, Source.Sequence);
	TestEqual(TEXT("Verb is preserved"), Normalized.Verb, Source.Verb);
	TestEqual(TEXT("Authoritative definition identity is preserved"),
		Normalized.DefinitionId, Source.DefinitionId);
	TestEqual(TEXT("Definition schema version is preserved"),
		Normalized.DefinitionSchemaVersion, Source.DefinitionSchemaVersion);
	TestEqual(TEXT("Result is mapped"), Normalized.Result, EAPSQuestEventResult::Succeeded);
	TestEqual(TEXT("Subject uses generic gameplay identity"), Normalized.Subject.Kind,
		EAPSQuestEntityKind::GameplayEntity);
	TestEqual(TEXT("Target stable ID is preserved"), Normalized.Target.Guid,
		Source.TargetStableId);
	TestTrue(TEXT("Stable result classification is retained"),
		Normalized.ContextLabels.Contains(Source.ResultCode));

	FAPSProductionEvent PlayerSource = Source;
	PlayerSource.SubjectIdentityDomain = EAPSSubjectIdentityDomain::Player;
	TestTrue(TEXT("Explicit account/controller Player domain normalizes"),
		FAPSProductionQuestAdapter::Normalize(PlayerSource, Normalized, Reason));
	TestEqual(TEXT("Player domain remains distinct"), Normalized.Subject.Kind,
		EAPSQuestEntityKind::Player);

	FAPSProductionEvent Invalid = Source;
	Invalid.SubjectIdentityDomain = EAPSSubjectIdentityDomain::None;
	TestFalse(TEXT("Missing subject domain is rejected"),
		FAPSProductionQuestAdapter::Normalize(Invalid, Normalized, Reason));
	Invalid = Source;
	Invalid.SubjectStableId.Invalidate();
	TestFalse(TEXT("Missing canonical subject ID is rejected"),
		FAPSProductionQuestAdapter::Normalize(Invalid, Normalized, Reason));
	Invalid = Source;
	Invalid.StreamId.Invalidate();
	TestFalse(TEXT("Missing Production-owned stream is rejected"),
		FAPSProductionQuestAdapter::Normalize(Invalid, Normalized, Reason));
	Invalid = Source;
	Invalid.ContextTags.Add(FName(TEXT("APS.DebugOnly")));
	TestFalse(TEXT("Debug-only event cannot advance Quest"),
		FAPSProductionQuestAdapter::Normalize(Invalid, Normalized, Reason));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Loading/APSAuthoredLevelLaunchGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAuthoredLevelLaunchGateTest,
	"APS.Menu.AuthoredLaunch.NonBlockingGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAuthoredLevelLaunchGateTest::RunTest(const FString& Parameters)
{
	FAPSAuthoredLevelLaunchGate Gate;
	using EPhase = FAPSAuthoredLevelLaunchGate::EPhase;
	TestTrue(TEXT("Start accepted"), Gate.Begin(10.0));
	TestFalse(TEXT("Double click does not restart preparation"), Gate.Begin(11.0));
	TestEqual(TEXT("Original request deadline preserved"), Gate.StartedAt, 10.0);
	TestFalse(TEXT("Unloaded map cannot travel"), Gate.Advance(12.0, false, false, 0, true));
	TestFalse(TEXT("Loaded map must wait for cold assets"), Gate.Advance(13.0, true, false, 60, true));
	TestTrue(TEXT("Compilation phase visible"), Gate.Phase == EPhase::Compiling);
	TestFalse(TEXT("Preview workers must finish"), Gate.Advance(14.0, true, false, 0, false));
	TestFalse(TEXT("One quiet poll is insufficient"), Gate.Advance(15.0, true, false, 0, true));
	TestFalse(TEXT("New asset work resets readiness"), Gate.Advance(16.0, true, false, 1, true));
	TestFalse(TEXT("First quiet poll after new work"), Gate.Advance(17.0, true, false, 0, true));
	TestTrue(TEXT("Second quiet poll opens once"), Gate.Advance(18.0, true, false, 0, true));
	TestFalse(TEXT("Double click during opening rejected"), Gate.Begin(18.1));
	TestFalse(TEXT("Travel cannot be queued twice"), Gate.Advance(19.0, true, false, 0, true));
	TestTrue(TEXT("Destination completes opening"), Gate.CompleteOpening());
	TestFalse(TEXT("Duplicate completion ignored"), Gate.CompleteOpening());
	TestTrue(TEXT("Completion removes dialog"), Gate.Phase == EPhase::Idle);
	TestTrue(TEXT("Next preparation accepted"), Gate.Begin(21.0));
	Gate.Cancel();
	TestFalse(TEXT("Late completion after cancel cannot open"), Gate.Advance(22.0, true, false, 0, true));
	TestFalse(TEXT("Late travel callback after cancel ignored"), Gate.CompleteOpening());
	TestTrue(TEXT("Retry after cancellation"), Gate.Begin(30.0));
	TestFalse(TEXT("Missing package fails without travel"), Gate.Advance(31.0, true, true, 0, true));
	TestTrue(TEXT("Failed phase remains for user"), Gate.Phase == EPhase::Failed);
	TestTrue(TEXT("Retry after failure"), Gate.Begin(40.0));
	TestFalse(TEXT("Timed out compilation cannot travel"), Gate.Advance(640.0, true, false, 60, true));
	TestTrue(TEXT("Preparation timeout is finite"), Gate.Phase == EPhase::Failed);
	TestTrue(TEXT("Retry after timeout"), Gate.Begin(700.0));
	TestFalse(TEXT("First ready poll"), Gate.Advance(701.0, true, false, 0, true));
	TestTrue(TEXT("Second ready poll queues retry"), Gate.Advance(702.0, true, false, 0, true));
	TestFalse(TEXT("Opening waits inside its own deadline"), Gate.Advance(761.0, true, false, 0, true));
	TestTrue(TEXT("Still opening before deadline"), Gate.Phase == EPhase::Opening);
	TestFalse(TEXT("Missing travel completion expires"), Gate.Advance(762.0, true, false, 0, true));
	TestTrue(TEXT("Opening timeout does not lock menu indefinitely"), Gate.Phase == EPhase::Failed);
	TestFalse(TEXT("Late success cannot consume failed transition"), Gate.CompleteOpening());
	Gate.Cancel();
	TestTrue(TEXT("Failed dialog can be dismissed"), Gate.Phase == EPhase::Idle);
	return true;
}
#endif

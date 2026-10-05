#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Audio/APSPoseFootContact.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAudioPoseContactTest, "APS.Audio.PoseContacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAudioPoseContactTest::RunTest(const FString& Parameters)
{
	using APSAudioPlayback::FPoseFootContact;
	FPoseFootContact Contact;
	TestEqual(TEXT("A planted startup foot is silent"), Contact.Advance(true, 170, 0, 0, .02f), -1);
	TestEqual(TEXT("Lifting a foot is silent"), Contact.Advance(true, 170, 9, 0, .02f), -1);
	TestEqual(TEXT("Descending left foot emits its contact"), Contact.Advance(true, 170, 1, 0, .02f), 0);
	for (int32 I = 0; I < 60; ++I)
		TestEqual(TEXT("A planted foot cannot repeat"), Contact.Advance(true, 170, 1, 0, .02f), -1);
	Contact.Advance(true, 170, 0, 9, .02f);
	TestEqual(TEXT("Right foot is independent"), Contact.Advance(true, 170, 0, 1, .02f), 1);
	Contact.Reset(.16f);
	Contact.Advance(true, 170, 9, 0, .02f);
	TestEqual(TEXT("Landing owns the contact"), Contact.Advance(true, 170, 1, 0, .02f), -1);
	Contact.Reset();
	Contact.Advance(true, 170, 9, 0, .02f);
	Contact.Advance(false, 170, 9, 0, .02f);
	TestEqual(TEXT("Airborne swings cannot leak into touchdown"), Contact.Advance(true, 170, 1, 0, .02f), -1);
	Contact.Advance(true, 170, 9, 0, .02f);
	TestEqual(TEXT("No catchup contacts after a hitch"), Contact.Advance(true, 170, 1, 0, .3f), -1);
	return true;
}
#endif

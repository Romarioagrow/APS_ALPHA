#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Audio/APSAudioPlaybackPolicy.h"
#include "APS_ALPHA/Core/Audio/APSAudioPreferences.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAudioCadenceTest, "APS.Audio.FootstepCadence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAudioCadenceTest::RunTest(const FString& Parameters)
{
	using namespace APSAudioPlayback;
	FFootstepCadence Cadence;
	for (int32 I = 0; I < 60; ++I)
	{
		TestFalse(TEXT("Standing remains silent"), Cadence.Advance(true, 0.f, 1.f / 60.f));
		TestFalse(TEXT("Airborne motion remains silent"), Cadence.Advance(false, 600.f, 1.f / 60.f));
	}
	for (const int32 FPS : {30, 60, 120})
	{
		FFootstepCadence Start;
		const float Dt = 1.f / FPS;
		float Speed = 0.f, FirstStepTime = 0.f;
		for (int32 I = 1; I <= FPS; ++I)
		{
			// Actual slow-walk mode: 170 cm/s, acceleration 1200 cm/s^2.
			Speed = FMath::Min(170.f, Speed + 1200.f * Dt);
			if (Start.Advance(true, Speed, Dt)) { FirstStepTime = I * Dt; break; }
		}
		TestTrue(TEXT("Slow walk schedules its first step within 150 ms"), FirstStepTime > 0.f && FirstStepTime <= 0.15f);
		Start.Advance(true, 0.f, 0.3f);
		TestTrue(TEXT("Restart does not wait a full stride"), Start.Advance(true, 170.f, 0.1f));
		Start.Advance(true, 0.f, 0.001f);
		TestFalse(TEXT("Quick stop/start cannot double-trigger"), Start.Advance(true, 600.f, 0.001f));
	}
	FFootstepCadence Landing;
	Landing.OnLanding();
	TestFalse(TEXT("Landing does not also emit a startup footstep"), Landing.Advance(true, 600.f, 0.1f));
	TestFalse(TEXT("Tiny grounded movement is silent"), FFootstepCadence{}.Advance(true, 25.f, 0.01f));

	auto CountSteps = [](int32 FPS)
	{
		FFootstepCadence Local;
		int32 Count = 0;
		for (int32 I = 0; I < FPS * 10; ++I) Count += Local.Advance(true, 350.f, 1.f / FPS) ? 1 : 0;
		return Count;
	};
	const int32 SlowFPS = CountSteps(30), FastFPS = CountSteps(120);
	TestTrue(TEXT("Walking produces a plausible cadence"), SlowFPS >= 25 && SlowFPS <= 36);
	TestTrue(TEXT("Cadence stays stable across frame rates"), FMath::Abs(SlowFPS - FastFPS) <= 2);
	Cadence.Advance(true, 600.f, 1.f); // A hitch cannot queue a burst of sounds.
	TestFalse(TEXT("A hitch has no immediate catch-up step"), Cadence.Advance(true, 600.f, 0.001f));
	TestFalse(TEXT("Invalid speed is silent"), Cadence.Advance(true, std::numeric_limits<float>::quiet_NaN(), 0.016f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAudioEnvelopeTest, "APS.Audio.EngineEnvelopeAndVolumes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAudioEnvelopeTest::RunTest(const FString& Parameters)
{
	using namespace APSAudioPlayback;
	TestEqual(TEXT("Stationary thrust layer is quiet"), EngineLoad(0., 0., 60., 0.016f, false, false), 0.f);
	TestTrue(TEXT("Boost is audible at rest"), EngineLoad(0., 0., 60., 0.016f, true, false) >= 0.8f);
	TestTrue(TEXT("Acceleration is audible"), EngineLoad(6., 0., 60., 0.016f, false, false) > 0.8f);
	TestTrue(TEXT("Extreme stellar speeds stay bounded"),
		EngineLoad(1.e17, 0., 1.e18, 0.016f, false, false) <= 1.f);
	TestEqual(TEXT("Non-finite input cannot reach audio mixer"),
		EngineLoad(std::numeric_limits<double>::infinity(), 0., 60., 0.016f, false, false), 0.f);
	UAPSAudioPreferences* Settings = NewObject<UAPSAudioPreferences>();
	Settings->Set(EAPSAudioChannel::Music, -10.f);
	TestEqual(TEXT("Negative level clamps to mute"), Settings->Get(EAPSAudioChannel::Music), 0.f);
	Settings->Set(EAPSAudioChannel::Music, 10.f);
	TestEqual(TEXT("Level clamps to unity"), Settings->Get(EAPSAudioChannel::Music), 1.f);
	Settings->Set(EAPSAudioChannel::Music, std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("NaN level mutes"), Settings->Get(EAPSAudioChannel::Music), 0.f);
	return true;
}
#endif

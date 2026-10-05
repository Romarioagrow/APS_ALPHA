#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Audio/APSVehicleAudioPolicy.h"
#include "Sound/SoundWave.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSVehicleAudioTest, "APS.Audio.GroundVehicleProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSVehicleAudioTest::RunTest(const FString& Parameters)
{
	using namespace APSVehicleAudio;
	TSet<FString> Paths;
	for (const FProfile& Profile : Profiles)
	{
		for (const TCHAR* Name : {Profile.Idle, Profile.Drive, Profile.Start})
		{
			if (!Name) continue;
			const FString Path = AssetPath(Name);
			TestFalse(TEXT("Each vehicle has separate recordings"), Paths.Contains(Path));
			Paths.Add(Path);
			const USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Path);
			if (TestNotNull(*Path, Wave))
			{
				TestEqual(TEXT("Loop metadata matches the profile"), Wave->IsLooping(), Name != Profile.Start);
				TestTrue(TEXT("Vehicle recording contains audio"), Wave->Duration > .1f);
			}
		}
		FInput Input;
		Input.Kind = Profile.Kind;
		Input.Translation = FVector::OneVector;
		Input.bBoost = true;
		Input.SpeedMetersPerSecond = 1000.;
		const FMix Off = Resolve(Input);
		TestEqual(TEXT("Engine-off motion cannot produce engine sound"), Off.IdleGain + Off.DriveGain, 0.f);
		Input.bPowered = true;
		for (const double Speed : {0., 27., 80., 1.e17, std::numeric_limits<double>::infinity()})
		{
			Input.SpeedMetersPerSecond = Speed;
			const FMix Out = Resolve(Input);
			TestTrue(TEXT("Simultaneous layers stay within their gain budget"),
				FMath::IsFinite(Out.IdleGain + Out.DriveGain) && Out.IdleGain + Out.DriveGain <= .90001f);
			TestTrue(TEXT("Pitch stays within the playback range"),
				Out.IdlePitch >= .65f && Out.IdlePitch <= 1.5f && Out.DrivePitch >= .65f && Out.DrivePitch <= 1.5f);
		}
	}
	TestEqual(TEXT("Three distinct pairs plus the rover starter"), Paths.Num(), 7);
	FInput Rover;
	Rover.Kind = EAPSGroundVehicleKind::Rover;
	Rover.bPowered = true;
	const FMix Idle = Resolve(Rover);
	Rover.Translation.X = 1.;
	const FMix Gas = Resolve(Rover);
	TestTrue(TEXT("Gas revs the motor even before the vehicle moves"), Gas.DriveGain > Idle.DriveGain && Gas.DrivePitch > Idle.DrivePitch);
	Rover.Translation.X = -1.;
	TestTrue(TEXT("Reverse also loads the motor"), Resolve(Rover).DriveGain > Idle.DriveGain);
	Rover.bBrake = true;
	TestTrue(TEXT("The handbrake does not trigger full acceleration audio"), Resolve(Rover).DriveGain < Gas.DriveGain);
	Rover.bBrake = false;
	Rover.Translation = FVector(0.,1.,0.);
	TestEqual(TEXT("Steering a stopped car does not rev its engine"), Resolve(Rover).DriveGain, Idle.DriveGain);
	FInput Drone;
	Drone.Kind = EAPSGroundVehicleKind::Drone;
	Drone.bPowered = true;
	const FMix Hovering = Resolve(Drone);
	Drone.Translation.Z = 1.;
	const FMix Climbing = Resolve(Drone);
	TestTrue(TEXT("Vertical takeoff raises rotor load without forward speed"), Climbing.DriveGain > Hovering.DriveGain);
	Drone.Translation.Z = 0.;
	Drone.bBrake = true;
	const FMix Descending = Resolve(Drone);
	TestTrue(TEXT("Ctrl descent keeps rotors working, below climb load"),
		Descending.DriveGain > Hovering.DriveGain && Descending.DriveGain < Climbing.DriveGain);
	Drone.bBrake = false;
	Drone.Translation.Y = 1.;
	TestTrue(TEXT("Drone strafing is audible"), Resolve(Drone).DriveGain > Hovering.DriveGain);
	Drone.Translation = FVector(std::numeric_limits<double>::quiet_NaN());
	Drone.SpeedScale = std::numeric_limits<float>::quiet_NaN();
	TestTrue(TEXT("Invalid controls cannot create invalid audio"), FMath::IsFinite(Resolve(Drone).DrivePitch));
	TestNull(TEXT("A spacecraft has no ground vehicle profile"), FindProfile(EAPSGroundVehicleKind::None));
	return true;
}
#endif

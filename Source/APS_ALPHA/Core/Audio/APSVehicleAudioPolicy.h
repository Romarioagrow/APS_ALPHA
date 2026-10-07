#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Vehicles/APSGroundVehicleTypes.h"

namespace APSVehicleAudio
{
	struct FProfile
	{
		EAPSGroundVehicleKind Kind;
		const TCHAR* Idle;
		const TCHAR* Drive;
		const TCHAR* Start;
	};

	inline constexpr FProfile Profiles[] = {
		{EAPSGroundVehicleKind::Rover, TEXT("SW_Rover_Idle"), TEXT("SW_Rover_Drive"), TEXT("SW_Rover_Start")},
		{EAPSGroundVehicleKind::Hover, TEXT("SW_Hover_Idle"), TEXT("SW_Hover_Drive"), nullptr},
		{EAPSGroundVehicleKind::Drone, TEXT("SW_Drone_Idle"), TEXT("SW_Drone_Drive"), nullptr}};

	inline const FProfile* FindProfile(EAPSGroundVehicleKind Kind)
	{
		for (const FProfile& Profile : Profiles) if (Profile.Kind == Kind) return &Profile;
		return nullptr;
	}

	inline FString AssetPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/APS/APS_ALPHA/Audio/Vehicles/%s.%s"), Name, Name);
	}

	struct FInput
	{
		EAPSGroundVehicleKind Kind = EAPSGroundVehicleKind::None;
		bool bPowered = false;
		FVector Translation = FVector::ZeroVector; // forward, strafe, lift; observes the pawn, never polls keys
		bool bBoost = false;
		bool bBrake = false; // rover/hover brake; drone's additional descent control
		double SpeedMetersPerSecond = 0.0;
		float SpeedScale = 1.f;
	};

	struct FMix
	{
		float IdleGain = 0.f;
		float DriveGain = 0.f;
		float IdlePitch = 1.f;
		float DrivePitch = 1.f;
	};

	inline FMix Resolve(const FInput& In)
	{
		FMix Out;
		if (!In.bPowered || !FindProfile(In.Kind)) return Out;
		const auto Axis = [](double Value)
		{
			return FMath::IsFinite(Value) ? static_cast<float>(FMath::Clamp(Value, -1.0, 1.0)) : 0.f;
		};
		const float Forward = FMath::Abs(Axis(In.Translation.X));
		const float Strafe = FMath::Abs(Axis(In.Translation.Y));
		const float Lift = FMath::Clamp(Axis(In.Translation.Z) - (In.bBrake ? 1.f : 0.f), -1.f, 1.f);
		const double Speed = FMath::IsFinite(In.SpeedMetersPerSecond) ? FMath::Max(0.0, In.SpeedMetersPerSecond) : 0.0;
		const float Scale = FMath::IsFinite(In.SpeedScale) ? FMath::Clamp(In.SpeedScale, .05f, 100.f) : 1.f;
		const double ReferenceSpeed = In.Kind == EAPSGroundVehicleKind::Rover ? 27.0
			: (In.Kind == EAPSGroundVehicleKind::Hover ? 55.0 : 80.0);
		const float SpeedRatio = static_cast<float>(FMath::Clamp(Speed / (ReferenceSpeed * Scale), 0.0, 1.0));
		float Demand = Forward;
		if (In.Kind == EAPSGroundVehicleKind::Drone)
		{
			Demand = FMath::Max3(Forward, .8f * Strafe, FMath::Abs(Lift) * (Lift >= 0.f ? 1.f : .6f));
		}
		else if (In.bBrake)
		{
			Demand *= .25f;
		}
		const float Boost = In.bBoost ? Demand : 0.f;
		switch (In.Kind)
		{
		case EAPSGroundVehicleKind::Rover:
		{
			// Gas raises the revs even against an obstacle; coasting retains lower drivetrain revs.
			const float Revs = FMath::Clamp(.6f * SpeedRatio + .35f * Demand + .1f * Boost, 0.f, 1.f);
			Out.IdleGain = .46f - .24f * Revs;
			Out.DriveGain = .02f + .60f * Demand + .12f * SpeedRatio;
			Out.IdlePitch = .85f + .25f * Revs;
			Out.DrivePitch = .78f + .65f * Revs;
			break;
		}
		case EAPSGroundVehicleKind::Hover:
			Out.IdleGain = .44f;
			Out.DriveGain = .03f + .52f * Demand + .08f * Boost;
			Out.IdlePitch = .9f + .1f * Demand;
			Out.DrivePitch = .82f + .27f * Demand + .10f * SpeedRatio;
			break;
		case EAPSGroundVehicleKind::Drone:
			Out.IdleGain = .46f - .12f * Demand;
			Out.DriveGain = .04f + .5f * Demand + .08f * Boost;
			Out.IdlePitch = .87f + .12f * Demand;
			Out.DrivePitch = .9f + .24f * Demand + .08f * Boost;
			break;
		default: break;
		}
		// Keep the two layers within a fixed gain budget before the user's Effects mix.
		const float Total = Out.IdleGain + Out.DriveGain;
		if (Total > .9f)
		{
			Out.IdleGain *= .9f / Total;
			Out.DriveGain *= .9f / Total;
		}
		return Out;
	}
}

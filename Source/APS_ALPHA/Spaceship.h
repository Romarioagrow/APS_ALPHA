// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/SphereComponent.h>
#include "GravitySource.h"
#include "GravityCharacterPawn.h"

#include "CoreMinimal.h"
#include "Spacecraft.h"
#include "Spaceship.generated.h"

///class AGravityCharacterPawn;





//
//// ����� ���������� � ��������� ������
//class AutopilotSystem {
//public:
//    void enableAutopilot();
//    void disableAutopilot();
//    void enableFlightAssistant();
//    void disableFlightAssistant();
//};
//
//// ����� ������� ������
//class HotButtonSystem {
//public:
//    void exitAtmosphere();
//    void stopRotation();
//};
//
//// ����� ��������������� �����
//class CommunicationSystem {
//public:
//    void sendMessage(Message* message);
//    void receiveMessage(Message* message);
//    void emergencyBeacon();
//    void enableSecureCommunication();
//    void disableSecureCommunication();
//};
//
//// ����� ������� �������� � ��������������
//class DockingAndCargoSystem {
//public:
//    void dock(Station* station);
//    void undock();
//    void loadCargo(Cargo* cargo);
//    void unloadCargo(Cargo* cargo);
//};
//
//// ����� ������� ������������
//class ExplorationSystem {
//public:
//    void launchProbe(Planet* planet);
//    void receiveProbeData(ProbeData* data);
//    void deployScientificEquipment();
//    void collectScientificData();
//};
//
//// ����� ������� ����������������
//class LifeSupportSystem {
//public:
//    void enableLifeSupport();
//    void disableLifeSupport();
//    void checkLifeSupport();
//    void initiateEmergencyLifeSupport();
//};
//
//// ����� ������� ������������� ����������
//class ArtificialGravitySystem {
//public:
//    void enableArtificialGravity();
//    void disableArtificialGravity();
//    void adjustArtificialGravity(float level);
//};
//
//// ����� ������� ����������� ����� � �������
//class ThreatDetectionAndDefenceSystem {
//public:
//    void initiateThreatScan();
//    void initiateCountermeasures();
//    void initiateEvasiveManeuvers();
//    void initiateCombatMode();
//    void initiateStealthMode();
//};
//
//// ����� ������� �������������� � ������������
//class ScanningSystem {
//public:
//    void initiateShortRangeScan();
//    void initiateLongRangeScan();
//    void initiateFullSpectrumScan();
//};



/**
 * 
 */
UCLASS()
class APS_ALPHA_API ASpaceship : public ASpacecraft, public IGravitySource
{
	GENERATED_BODY()
	
public:
	ASpaceship();

public:
	void SwitchCamera();

	double ThrustForce { 100 };

	bool bEngineRunning {false };

	void SwitchEngines();

	void ThrustForward(float Value);

	void ThrustSide(float Value);

	void ThrustVertical(float Value);

	void ThrustYaw(float Value);

	void ThrustPitch(float Value);

	void ThrustRoll(float Value);

	void SetPilot(AGravityCharacterPawn* NewPilot);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* SphereCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* SpaceshipHull;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* PilotChair;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;


	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};



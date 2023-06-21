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
//// Класс автопилота и помощника полета
//class AutopilotSystem {
//public:
//    void enableAutopilot();
//    void disableAutopilot();
//    void enableFlightAssistant();
//    void disableFlightAssistant();
//};
//
//// Класс горячих кнопок
//class HotButtonSystem {
//public:
//    void exitAtmosphere();
//    void stopRotation();
//};
//
//// Класс интерстелларной связи
//class CommunicationSystem {
//public:
//    void sendMessage(Message* message);
//    void receiveMessage(Message* message);
//    void emergencyBeacon();
//    void enableSecureCommunication();
//    void disableSecureCommunication();
//};
//
//// Класс системы стыковки и грузоперевозок
//class DockingAndCargoSystem {
//public:
//    void dock(Station* station);
//    void undock();
//    void loadCargo(Cargo* cargo);
//    void unloadCargo(Cargo* cargo);
//};
//
//// Класс системы исследования
//class ExplorationSystem {
//public:
//    void launchProbe(Planet* planet);
//    void receiveProbeData(ProbeData* data);
//    void deployScientificEquipment();
//    void collectScientificData();
//};
//
//// Класс системы жизнеобеспечения
//class LifeSupportSystem {
//public:
//    void enableLifeSupport();
//    void disableLifeSupport();
//    void checkLifeSupport();
//    void initiateEmergencyLifeSupport();
//};
//
//// Класс системы искусственной гравитации
//class ArtificialGravitySystem {
//public:
//    void enableArtificialGravity();
//    void disableArtificialGravity();
//    void adjustArtificialGravity(float level);
//};
//
//// Класс системы обнаружения угроз и обороны
//class ThreatDetectionAndDefenceSystem {
//public:
//    void initiateThreatScan();
//    void initiateCountermeasures();
//    void initiateEvasiveManeuvers();
//    void initiateCombatMode();
//    void initiateStealthMode();
//};
//
//// Класс системы детектирования и сканирования
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



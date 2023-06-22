#pragma once
#include "AstroActor.h"
#include "SpaceStation.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpaceshipOnboardComputer.generated.h"

//#define GETENUMSTRING(etype, evalue) ( (FindObject(ANY_PACKAGE, TEXT(etype), true) != nullptr) ? FindObject(ANY_PACKAGE, TEXT(etype), true)->GetEnumName((int32)evalue) : FString("Invalid - are you sure enum uses UENUM() macro?") )

UENUM(BlueprintType)
enum class EFlightType : uint8
{
    Station UMETA(DisplayName = "Elliptical Galaxy"),
    Surface UMETA(DisplayName = "Elliptical Galaxy"),
    Atmospheric UMETA(DisplayName = "Elliptical Galaxy"),
    Orbital UMETA(DisplayName = "Elliptical Galaxy"),
    Planetary UMETA(DisplayName = "Elliptical Galaxy"),
    Interplanetray UMETA(DisplayName = "Elliptical Galaxy"),
    Interstellar UMETA(DisplayName = "Elliptical Galaxy"),
    Intergalaxy UMETA(DisplayName = "Elliptical Galaxy")
};

UENUM(BlueprintType)
enum class EFlightMode : uint8
{
    ZeroG,
    Atmospheric,
    SubOrbital,
    Orbital,
    SubLightSpeed,
    LightSpeed,
    FTL,
    Hyperspace,
    Tunneling
};

UENUM(BlueprintType)
enum class EEngineType : uint8
{
    Impulse,
    Offset,
    SpaceWrap,
};

UENUM(BlueprintType)
enum class EEngineState : uint8
{
    Idle,
    Accelerating,
    Decelerating,
    Cruise,
    Drift,
};

UENUM(BlueprintType)
enum class EFlightStatus : uint8
{
    OK,
    Attention,
    Warning,
    Danger,
    Critical,
    Emergency,
    Combat,
};

UENUM(BlueprintType)
enum class EFlightRangeType : uint8
{
    Short,
    Medium,
    Long,
    Extreme
};

UENUM(BlueprintType)
enum class EFlightSafeMode : uint8
{
    Safe,
    Normal,
    Unsafe,
    Danger,
    Combat
};

struct Target
{
    FName TargetName;
    FName TargetType;
    FVector TargetLocation;
};

struct AstroTarget : public Target
{

};

struct TechTarget : public Target
{

};

struct FlightRange
{
    double MinRange;
    double MaxRange;
};

struct ThrustMode
{
    double MinThrust;
    double MaxThrust;
    double CurrentTrust;
};

struct ProbeData
{
    FName Data;
};

struct Message
{
    FName Message;
};

struct Cargo
{
    FName CargoName;
    double CargoVolume;
    double CargoWeight;
};

class TargetSystem
{
public:
    Target* CurrentTarget;
    TArray<Target*> TargetList;

    void LockTarget(Target* Target);
    void FlyTo(Target* Target);
    void JumpTo(Target* Target);
    void ScanTargets();
    void provideScanInfo();
};

class AstroNavigationSystem
{
public:
    bool IsAstroNavigationEnabled;
    AstroTarget* CurrentAstroTarget;
    TArray<AstroTarget*> AstroTargetList;

    void CalculateRoute(Target* Target);
    void ChangeMapMode(); // Navigation/Astronomical/Projection Map Modes
    void MultiTargetTracking();
};

//USTRUCT(BlueprintType)


class EngineSystem {
public:
    EEngineState CurrentEngineState{ EEngineState::Idle };
    EEngineType CurrentEngineType{ EEngineType::Impulse };
    ThrustMode CurrentThrustMode;

    void MarshallerEngineMode();
    void TraverserEngineMode();
    void SetCruiserFlightMode();
    void InitiateThrusterBurst();
    void InitiateLowThrustMode();
    void InitiateHighThrustMode();
    void InitiateFTL();
    void InitiateTunnelJump();
    void InitiateTraverserFlight();
};

class AstroExplorationSystem {
public:
    bool IsExplorationUnderway;
    ProbeData* CurrentProbeData;
    TArray<ProbeData*> DataCollection;

    void ExploreAstroObject();
    void CollectSamples();
    void LaunchProbe(AAstroActor* AstroObject);
    void ReceiveProbeData(ProbeData* data);
    void DeployScientificEquipment();
    void CollectScientificData();

    void initiateShortRangeScan();
    void initiateLongRangeScan();
    void initiateFullSpectrumScan();
};

class BoardSystems {
public:
    class ShieldSubSystem
    {
    public:
        bool IsActivated;
        double PowerLevel;

        void Activate();
        void Deactivate();
        void AdjustPower();
    };

    class WeaponSubSystem
    {
    public:
        bool IsWeaponsLoaded;

        void LoadWeapons();
        void FireWeapons();
    };

    class DamageControlSubSystem
    {
    public:
        bool IsDamagePresent;

        void RunDiagnostics();
        void RepairDamage();

    };

    class DefenceSubSystem
    {
    public:
        void InitiateThreatScan();
        void InitiateCountermeasures();
        void InitiateEvasiveManeuvers();
        void InitiateCombatMode();
        void InitiateStealthMode();
    };

    class VitalSubSystem
    {
    public:
        double CurrentFuelLevel;
        double CurrentEnergyLevel;
        double HullHealth;
        double ShieldsHealth;
        double EngineHealth;

        void Refuel();
        void Repair();
        void Recharge();
        void Regenerate();
        void Reboot();
        void Shutdown();
    };
};

class CommunicationSystem {
public:
    Message* CurrentMessage;
    TArray<Message*> MessageQueue;
    bool SecureCommunicationEnabled;

    void SendMessage(Message* message);
    void EmergencyBeacon();
    void SwitchSecureCommunication();

    class InterstellarCommunication {
    public:
        bool IsSignalReceived;

        void SendSignal();
        void ReceiveSignal();
    };

    class ShipToShipCommunication {
    public:
        bool IsSignalReceived;

        void SendSignal();
        void ReceiveSignal();
    };

    class EmergencyBeacon {
    public:
        bool IsActivated;

        void Activate();
        void Deactivate();
    };
};

class DockingCargoSystem {
public:
    bool IsDockingInitiated;
    double CargoWeight;
    ASpaceStation* DockedStation;
    TArray<Cargo*> CargoList;

    void InitiateDocking();
    void UnloadCargo();
    void Dock(ASpaceStation* station);
    void Undock();
    void LoadCargo(Cargo* cargo);
    void UnloadCargo(Cargo* cargo);
};

class LifeSupportSystems {
public:
    bool LIsLifeSupportEnabled;
    float CurrentLifeSupportLevel;
    float EmergencyLifeSupportLevel;

    class LifeSupport {
    public:
        double OxygenLevel;
        double Temperature;
        double Humidity;

        void GenerateOxygen();
        void RegulateTemperature();
        void ManageHumidity();
    };

    class FoodWaterSubSystem {
    public:
        double FoodStock;
        double WaterStock;

        void GenerateFood();
        void PurifyWater();
    };

    class MedicalSubSystem {
    public:
        bool IsInjuryPresent;
        bool IsIllnessPresent;

        void TreatInjuries();
        void TreatIllnesses();
    };

    class ArtificialGravitySubSystem {
    public:
        bool IsActivated;
        bool ArtificialGravityEnabled;
        float currentGravityLevel;

        void Activate();
        void Deactivate();
    };
};



USTRUCT(BlueprintType)
struct FFlightSystem
{
    //GENERATED_BODY()
    GENERATED_USTRUCT_BODY()

public:
    //FlightSystem();

    EFlightType CurrentFlightType{ EFlightType::Station };
    EFlightMode CurrentFlightMode{ EFlightMode::ZeroG };
    EFlightStatus CurrentFlightStatus{ EFlightStatus::OK };

    // Метод для переключения типа полёта
    void SwitchEngineType(EEngineType EngineType);
    void SwitchFlightMode(EFlightMode FlightMode);

    void CheckFlightStatus();
    void SetFlightRange(FlightRange Range);
    void SetFlightMode(EFlightMode FlightMode);
    void SwitchFlightMode(); ///  Atmospheric, Orbital, ZeroG, Interplanetary, Interstellar 

    void SwitchAccelerationLimit();
    void SetAccelerationLimit();
    void EmergencySpeedDrop();
    void AlignToHorizon();
    void StabilizeFlight();

    void ZeroGFlight();
    void AtmosphericFlight();
    void FasterThanLightFlight();
    void TunnelingJumps();
    void InitiateDriftMode();
    void InitiateAccelerationMode();
    void InitiateDecelerationMode();
    void InitiateCruiseMode(/*Type, Limit*/);

    /*class FlightAssistant
    {
    public:
        void SwitchAutopilot();
        void SwitchFlightAssistant();
        void ExitAtmosphere();
        void ExitOrbit();
    };*/

    /*FlightSystem()
        : CurrentFlightType(EFlightType::Station)
        , CurrentFlightMode(EFlightMode::ZeroG)
        , CurrentFlightStatus(EFlightStatus::OK)
    { }*/
};


/**
 * 
 */
UCLASS()
class APS_ALPHA_API USpaceshipOnboardComputer : public UObject
{
	GENERATED_BODY()

public:
    USpaceshipOnboardComputer();

    UPROPERTY()
        FFlightSystem FlightSystem;

   /* TargetSystem TargetSystemInstance{};
    AstroNavigationSystem AstroNavigationSystemInstance{};
    EngineSystem EngineSystemInstance{};
    AstroExplorationSystem AstroExplorationSystemInstance{};
    BoardSystems BoardSystemsInstance{};
    CommunicationSystem CommunicationSystemInstance{};
    DockingCargoSystem DockingCargoSystemInstance{};
    LifeSupportSystems LifeSupportSystemsInstance{};*/

    FString GetCurrentFlightType();

};

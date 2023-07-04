#pragma once
#include "AstroActor.h"
#include "SpaceStation.h"
#include "IMovementStrategy.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpaceshipOnboardComputer.generated.h"

USTRUCT(BlueprintType)
struct FFlightParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        double ThrustForce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        double LinearResistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        double AngularResistance;

    // Конструктор по умолчанию
    FFlightParams() : ThrustForce(100), LinearResistance(0.1), AngularResistance(0.1) {}

    // Конструктор с параметрами
    FFlightParams(double InThrustForce, double InLinearResistance, double InAngularResistance)
        : ThrustForce(InThrustForce), LinearResistance(InLinearResistance), AngularResistance(InAngularResistance) {}
};



/**
 * @brief Diapasones, limits, ranges, etc.
*/
UENUM(BlueprintType)
enum class EFlightMode : uint8
{
    Station         UMETA(DisplayName = "Station"),
    Surface         UMETA(DisplayName = "Surface"),
    Atmospheric     UMETA(DisplayName = "Atmospheric"),
    Orbital         UMETA(DisplayName = "Orbital"),
    Planetary       UMETA(DisplayName = "Planetary"),
    Interplanetary  UMETA(DisplayName = "Interplanetary"),
    Stellar         UMETA(DisplayName = "Stellar "),
    Interstellar    UMETA(DisplayName = "Interstellar"),
    Intergalaxy     UMETA(DisplayName = "Intergalaxy"),
    Basic           UMETA(DisplayName = "Basic")
};

/**
 * @brief Physic forces, effects, resistance etc
*/
UENUM(BlueprintType)
enum class EFlightType : uint8
{
    ZeroG,
    LowG,
    Atmospheric,
    SubOrbital,
    Orbital,
    SubLightSpeed,
    LightSpeed,
    FTL,
    Hyperspace,
    Tunneling,
    Landing,
    Docking,
    Attracting,
    ArtificialGravity
};

/**
 * @brief Fundamental Principle of moving ship
*/
UENUM(BlueprintType)
enum class EEngineMode : uint8
{
    Impulse,
    Offset,
    SpaceWrap,
};

/**
 * @brief Power and usage of engine
*/
UENUM(BlueprintType)
enum class EEngineState : uint8
{
    Idle,
    Accelerating,
    Decelerating,
    Cruise,
    Drift,
};

/**
 * @brief Global ship status
*/
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

/**
 * @brief Presets for Fuel, power, supply etc ranges
*/
UENUM(BlueprintType)
enum class EFlightRange : uint8
{
    Standart,
    Nearby,
    Short,
    Close,
    Medium,
    Long,
    Far,
    Distant,
    Extreme
};

/**
 * @brief Acceleration extremums, flight behavior, safety regulations etc
*/
UENUM(BlueprintType)
enum class EFlightSafeMode : uint8
{
    Safe,
    Normal,
    Unsafe,
    Danger,
    Combat
};

USTRUCT(BlueprintType)
struct FTarget
{
    GENERATED_USTRUCT_BODY()

    FName TargetName;
    FName TargetType;
    FVector TargetLocation;
};

USTRUCT(BlueprintType)
struct FAstroTarget : public FTarget
{
    GENERATED_USTRUCT_BODY()

};

USTRUCT(BlueprintType)
struct FTechTarget : public FTarget
{
    GENERATED_USTRUCT_BODY()

};

USTRUCT(BlueprintType)
struct FFlightRange
{
    GENERATED_USTRUCT_BODY()

    double MinRange;
    double MaxRange;
};

USTRUCT(BlueprintType)
struct FThrustMode
{
    GENERATED_USTRUCT_BODY()

    double MinThrust;
    double MaxThrust;
    double CurrentTrust;
};

USTRUCT(BlueprintType)
struct FProbeData
{
    GENERATED_USTRUCT_BODY()

    FName Data;
};

USTRUCT(BlueprintType)
struct FMessage
{
    GENERATED_USTRUCT_BODY()

    FName Message;
};

USTRUCT(BlueprintType)
struct FCargo
{
    GENERATED_USTRUCT_BODY()

    FName CargoName;
    double CargoVolume;
    double CargoWeight;
};



USTRUCT(BlueprintType)
struct FTargetSystem
{
    GENERATED_USTRUCT_BODY()

public:
    FTarget* CurrentTarget;
    TArray<FTarget*> TargetList;

    void LockTarget(FTarget* Target);
    void FlyTo(FTarget* Target);
    void JumpTo(FTarget* Target);
    void ScanTargets();
    void provideScanInfo();
};

USTRUCT(BlueprintType)
struct FAstroNavigationSystem
{
    GENERATED_USTRUCT_BODY()

public:
    bool IsAstroNavigationEnabled;
    FAstroTarget* CurrentAstroTarget;
    TArray<FAstroTarget*> AstroTargetList;

    void CalculateRoute(FTarget* Target);
    void ChangeMapMode(); // Navigation/Astronomical/Projection Map Modes
    void MultiTargetTracking();
};

//USTRUCT(BlueprintType)

USTRUCT(BlueprintType)
struct FEngineSystem 
{
    GENERATED_USTRUCT_BODY()

public:
    double ThrustForce { 100 };

    EEngineState CurrentEngineState{ EEngineState::Idle };
    EEngineMode CurrentEngineMode{ EEngineMode::Impulse };
    FThrustMode CurrentThrustMode;

    void InitiateOffsetMode();
    void InitiateImpulseMode();
    //void SwitchEngineMode(EEngineMode EngineMode);

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

USTRUCT(BlueprintType)
struct FAstroExplorationSystem 
{
    GENERATED_USTRUCT_BODY()

public:
    bool IsExplorationUnderway;
    FProbeData* CurrentProbeData;
    TArray<FProbeData*> DataCollection;

    void ExploreAstroObject();
    void CollectSamples();
    void LaunchProbe(AAstroActor* AstroObject);
    void ReceiveProbeData(FProbeData* data);
    void DeployScientificEquipment();
    void CollectScientificData();

    void initiateShortRangeScan();
    void initiateLongRangeScan();
    void initiateFullSpectrumScan();
};

USTRUCT(BlueprintType)
struct FBoardSystems 
{
    GENERATED_USTRUCT_BODY()

public:
    struct FShieldSubSystem
    {
    public:
        bool IsActivated;
        double PowerLevel;

        void Activate();
        void Deactivate();
        void AdjustPower();
    };

    struct FWeaponSubSystem
    {
    public:
        bool IsWeaponsLoaded;

        void LoadWeapons();
        void FireWeapons();
    };

    struct FDamageControlSubSystem
    {
    public:
        bool IsDamagePresent;

        void RunDiagnostics();
        void RepairDamage();

    };

    struct FDefenceSubSystem
    {
    public:
        void InitiateThreatScan();
        void InitiateCountermeasures();
        void InitiateEvasiveManeuvers();
        void InitiateCombatMode();
        void InitiateStealthMode();
    };

    struct FVitalSubSystem
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

USTRUCT(BlueprintType)
struct FCommunicationSystem 
{
    GENERATED_USTRUCT_BODY()

public:
    FMessage* CurrentMessage;
    TArray<FMessage*> MessageQueue;
    bool SecureCommunicationEnabled;

    void SendMessage(FMessage* message);
    void EmergencyBeacon();
    void SwitchSecureCommunication();

    struct FInterstellarCommunication {
    public:
        bool IsSignalReceived;

        void SendSignal();
        void ReceiveSignal();
    };

    struct FShipToShipCommunication {
    public:
        bool IsSignalReceived;

        void SendSignal();
        void ReceiveSignal();
    };

    struct FEmergencyBeacon {
    public:
        bool IsActivated;

        void Activate();
        void Deactivate();
    };
};

USTRUCT(BlueprintType)
struct FDockingCargoSystem 
{
    GENERATED_USTRUCT_BODY()

public:
    bool IsDockingInitiated;
    double CargoWeight;
    ASpaceStation* DockedStation;
    TArray<FCargo*> CargoList;

    void InitiateDocking();
    void UnloadCargo();
    void Dock(ASpaceStation* station);
    void Undock();
    void LoadCargo(FCargo* cargo);
    void UnloadCargo(FCargo* cargo);
};

USTRUCT(BlueprintType)
struct FLifeSupportSystems 
{
    GENERATED_USTRUCT_BODY()

public:
    bool LIsLifeSupportEnabled;
    float CurrentLifeSupportLevel;
    float EmergencyLifeSupportLevel;

    struct FLifeSupport {
    public:
        double OxygenLevel;
        double Temperature;
        double Humidity;

        void GenerateOxygen();
        void RegulateTemperature();
        void ManageHumidity();
    };

    struct FFoodWaterSubSystem {
    public:
        double FoodStock;
        double WaterStock;

        void GenerateFood();
        void PurifyWater();
    };

    struct FMedicalSubSystem {
    public:
        bool IsInjuryPresent;
        bool IsIllnessPresent;

        void TreatInjuries();
        void TreatIllnesses();
    };

    struct FArtificialGravitySubSystem {
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
    GENERATED_USTRUCT_BODY()

public:
    //FlightSystem();
    FFlightParams FlightParams {};

    EFlightMode CurrentFlightMode{ EFlightMode::Station };
    EFlightType CurrentFlightType{ EFlightType::ZeroG };
    EFlightStatus CurrentFlightStatus{ EFlightStatus::OK };
    EFlightRange CurrentFlightRange{ EFlightRange::Standart };
    EFlightSafeMode CurrentFlightSafeMode{ EFlightSafeMode::Normal };

    // Метод для переключения типа полёта
    void SwitchEngineType(EEngineMode EngineType);
    void SwitchFlightMode(EFlightMode FlightMode);

    void CheckFlightStatus();
    void SetFlightRange(FFlightRange Range);
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

    struct FlightAssistant
    {
    public:
        void SwitchAutopilot();
        void SwitchFlightAssistant();
        void ExitAtmosphere();
        void ExitOrbit();
    };
};


/**
 * 
 */
UCLASS()
class APS_ALPHA_API USpaceshipOnboardComputer : public UActorComponent
{
	GENERATED_BODY()

public:
    USpaceshipOnboardComputer(AActor* InOwner);

    USpaceshipOnboardComputer();

public:
    AActor* Owner; //

    double GetEngineThrustForce();

    void IncreaseFlightMode();

    void DecreaseFlightMode();

    void AccelerateBoost(float DeltaTime);

    void DecelerateBoost(float DeltaTime);

    void SwitchEngineMode(EEngineMode EngineMode);

    void ComputeFlightStatus(AWorldActor* AffectedActor);

    void ComputeFlightParams();

    FString GetEnumValueAsString(const TCHAR* EnumName, int32 EnumValue);

    FString GetFlightTypeAsString();

    FString GetFlightModeAsString();

    FString GetFlightStatusAsString();

    FString GetEngineStateAsString();

    FString GetEngineTypeAsString();

    UPROPERTY()
        FFlightSystem FlightSystem {};

    UPROPERTY()
        FEngineSystem EngineSystem {};

    UPROPERTY()
        FTargetSystem TargetSystem {};

    UPROPERTY()
        FAstroNavigationSystem AstroNavigationSystem {};

    UPROPERTY()
        FAstroExplorationSystem AstroExplorationSystem {};

    UPROPERTY()
        FBoardSystems BoardSystems {};

    UPROPERTY()
        FCommunicationSystem CommunicationSystem {};

    UPROPERTY()
        FDockingCargoSystem DockingCargoSystem {};

    UPROPERTY()
        FLifeSupportSystems LifeSupportSystems {};

    FString GetCurrentFlightType();

    UPROPERTY()
        TMap<EFlightMode, FFlightParams> FlightModeParams =
        {
            {EFlightMode::Station, FFlightParams(55, 0.01, 0.05)},
            {EFlightMode::Surface, FFlightParams(100, 0.03, 0.08)},
            {EFlightMode::Atmospheric, FFlightParams(500, 0.04, 0.12)},
            {EFlightMode::Orbital, FFlightParams(0.0002, 0.05, 0.16)},
            {EFlightMode::Planetary, FFlightParams(0.0005, 0.06, 0.2)},
            {EFlightMode::Interplanetary, FFlightParams(0.666, 0.07, 0.24)},
            {EFlightMode::Stellar, FFlightParams(1, 0.06, 0.2)},
            {EFlightMode::Interstellar, FFlightParams(15000000000, 0.08, 0.28)},
            {EFlightMode::Intergalaxy, FFlightParams(100000000000, 0.09, 0.32)}
        };

    bool IsBoosting{ false };

    bool CalculateProximity{ true };



    UPROPERTY()
        UStaticMeshComponent* SpaceshipHull;

    TUniquePtr<IMovementStrategy> CurrentMovementStrategy;

    AStarSystem* OffsetSystem;

    AAstroActor* OffsetGalaxy;

    bool bIsRescaling{ false };

    void ComputeInterstellarFlight();


};

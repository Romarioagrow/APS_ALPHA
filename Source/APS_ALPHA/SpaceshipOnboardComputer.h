#pragma once
#include "AstroActor.h"
#include "SpaceStation.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpaceshipOnboardComputer.generated.h"

/**
 * 
 */
UCLASS()
class APS_ALPHA_API USpaceshipOnboardComputer : public UObject
{
	GENERATED_BODY()

protected:
    enum class FlightType 
    {
        Station,
        Surface,
        Atmospheric,
        Orbital,
        Planetary,
        Interplanetray,
        Interstellar,
        Intergalaxy
    };

    enum class FlightMode 
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

    enum class EngineType 
    {
        Impulse,
        Offset,
        SpaceWrap,
    };

    enum class EngineState
    {
        Idle,
        Accelerating,
        Decelerating,
        Cruise,
        Drift,
    };

    enum class FlightStatus
    {
        OK,
        Attention,
        Warning,
        Danger,
        Critical,
        Emergency,
        Combat,
    };

    class Target 
    {
    public: 
        FName TargetName;
        FName TargetType;
        FVector TargetLocation;
    };

    class AstroTarget : public Target 
    {
    public: 

    };

    class TechTarget : public Target 
    {
    public: 

    };

    class FlightRange
    {
    public:
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

    class FlightSystem 
    {
    public:
        FlightType FlightType;
        //FlightMode FlightMode;
        FlightStatus FlightStatus;

        // Метод для переключения типа полёта
        void SwitchEngineType(EngineType EngineType);
        void SwitchFlightMode(FlightMode FlightMode);

        void CheckFlightStatus();
        void SetFlightRange(FlightRange Range);
        void SetFlightMode(FlightMode FlightMode);
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

        class FlightAssistant 
        {
        public:
            void SwitchAutopilot();
            void SwitchFlightAssistant();
            void ExitAtmosphere();
            void ExitOrbit();
        };
    };

    class EngineSystem {
    public:
        EngineState CurrentEngineState;
        EngineType CurrentEngineType;
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
    };

    class InteractiveSystems{
    public:
        class ShieldSubSystem {
        public:
            bool IsActivated;
            double PowerLevel;

            void Activate();
            void Deactivate();
            void AdjustPower();
        };

        class WeaponSubSystem {
        public:
            bool IsWeaponsLoaded;

            void LoadWeapons();
            void FireWeapons();
        };

        class DamageControlSubSystem {
        public:
            bool IsDamagePresent;

            void RunDiagnostics();
            void RepairDamage();
        };

        class VitalSystem
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
};

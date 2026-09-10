#pragma once

#include "Planet.h"
#include "CoreMinimal.h"
#include "CelestialBody.h"
#include "Star.generated.h"

enum class ESpectralType : uint8;
enum class ESpectralClass : uint8;
enum class EStellarType : uint8;
class APlanetarySystem;
class UPointLightComponent;

UCLASS()
class APS_ALPHA_API AStar : public ACelestialBody, public INavigatableBody
{
	GENERATED_BODY()

public:
	AStar();

protected:
	virtual void BeginPlay() override;

public:
	void SetStarProperties(FLinearColor Color, float Multiplier);
	UMaterialInstanceDynamic* EnsureCanonicalStellarMaterial();
	/** Synchronizes the additive corona shell and the local spectral emitter. */
	void ConfigureStellarPresentation(
		const FLinearColor& Color, float Emission, float SurfaceSeed,
		EStellarType StellarType);
	/** Shared unchanged corona recipe; preview-only meshes need no physical actor or light. */
	static void ConfigureStellarPresentationComponents(
		UStaticMeshComponent* Photosphere, UStaticMeshComponent* Corona,
		UMaterialInstanceDynamic* CoronaMaterial, UPointLightComponent* Light,
		const FLinearColor& Color, float Emission, float SurfaceSeed, EStellarType StellarType);
	/** Rebinds attenuation to the final preview/gameplay mesh bound after render-only scaling. */
	void SyncStellarLightToPresentedBounds();
	static void SyncStellarLightToBounds(UStaticMeshComponent* Photosphere, UPointLightComponent* Light);

	UPROPERTY(VisibleAnywhere, Category = "Star")
	UStaticMeshComponent* StarMesh;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	UMaterialInstanceDynamic* StarDynamicMaterial;

	/** Actor-only additive shell. HISM stars keep the separate point-star path. */
	UPROPERTY(VisibleAnywhere, Category = "Star|Visuals")
	UStaticMeshComponent* CoronaMesh;

	UPROPERTY(VisibleAnywhere, Category = "Star|Visuals")
	UMaterialInstanceDynamic* CoronaDynamicMaterial;

	/** Cheap local contribution; the world subsystem remains the astronomical key. */
	UPROPERTY(VisibleAnywhere, Category = "Star|Lighting")
	UPointLightComponent* StellarLight;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	APlanetarySystem* PlanetarySystem;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	USphereComponent* PlanetarySystemZone;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	int StarRadiusKM;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	double StarAffectionZoneRadius{0.0};

	double MinOrbit;

	double MaxOrbit;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	float Luminosity;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	int SurfaceTemperature;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	EStellarType StellarClass;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	ESpectralClass SpectralClass;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	ESpectralType SpectralType;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	FName FullSpectralClass;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	FName FullSpectralName;

	UPROPERTY(VisibleAnywhere, Category = "Star")
	int SpectralSubclass;

	UPROPERTY()
	TArray<APlanet*> Planets;

	void AddPlanet(APlanet* Planet);

	void SetPlanetarySystem(APlanetarySystem* PlanetarySystem);

	void SetLuminosity(float Luminosity);

	void SetSurfaceTemperature(int SurfaceTemperature);

	void SetStarType(EStellarType StarType);

	void SetStarSpectralClass(ESpectralClass StarSpectralClass);

	void SetStarSpectralType(ESpectralType StarSpectralType);

	void SetFullSpectralClass(FName FullSpectralClass);

	void SetFullSpectralName(FName FullSpectralName);

	void SetSpectralSubclass(int SpectralSubclass);
	
	FName GenerateFullSpectralName();
};

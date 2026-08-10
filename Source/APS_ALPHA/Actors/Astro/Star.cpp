#include "Star.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStar::AStar()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetarySystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetarySystemZoneComponent"));
	PlanetarySystemZone->SetupAttachment(RootComponent);
	PlanetarySystemZone->SetVisibility(false);
	PlanetarySystemZone->SetHiddenInGame(true);

	StarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
	StarMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> CanonicalActorMaterial(
		APSStellarMaterialContract::ActorBaseObjectPath);
	if (CanonicalActorMaterial.Succeeded())
	{
		StarMesh->SetMaterial(0, CanonicalActorMaterial.Object);
	}
}

void AStar::BeginPlay()
{
	Super::BeginPlay();
	PlanetarySystemZone->SetVisibility(false, true);
	PlanetarySystemZone->SetHiddenInGame(true, true);

	EnsureCanonicalStellarMaterial();
}

UMaterialInstanceDynamic* AStar::EnsureCanonicalStellarMaterial()
{
	if (!IsValid(StarMesh))
	{
		StarDynamicMaterial = nullptr;
		return nullptr;
	}

	UMaterial* CanonicalBase = APSStellarMaterialContract::LoadCanonicalBase(
		APSStellarMaterialContract::ActorBaseObjectPath);
	if (!IsValid(CanonicalBase))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.StellarMaterial] Canonical actor master is unavailable: %s"),
			APSStellarMaterialContract::ActorBaseObjectPath);
		StarDynamicMaterial = nullptr;
		return nullptr;
	}

	UMaterialInterface* CurrentMaterial = StarMesh->GetMaterial(0);
	if (UMaterialInstanceDynamic* CurrentDynamic =
		Cast<UMaterialInstanceDynamic>(CurrentMaterial);
		IsValid(CurrentDynamic)
		&& APSStellarMaterialContract::HasExactBase(
			CurrentDynamic, APSStellarMaterialContract::ActorBaseObjectPath))
	{
		StarDynamicMaterial = CurrentDynamic;
		return CurrentDynamic;
	}

	const bool bCurrentUsesCanonicalBase = APSStellarMaterialContract::HasExactBase(
		CurrentMaterial, APSStellarMaterialContract::ActorBaseObjectPath);
	if (!bCurrentUsesCanonicalBase)
	{
		const UMaterial* CurrentBase = APSStellarMaterialContract::GetBaseMaterial(CurrentMaterial);
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.StellarMaterial] Replacing %s actor material base %s with canonical %s%s"),
			*GetNameSafe(this),
			IsValid(CurrentBase) ? *CurrentBase->GetPathName() : TEXT("<null>"),
			APSStellarMaterialContract::ActorBaseObjectPath,
			APSStellarMaterialContract::UsesWorldGrid(CurrentMaterial)
				? TEXT(" (WorldGrid fallback)") : TEXT(""));
	}

	UMaterialInterface* DynamicParent = bCurrentUsesCanonicalBase
		? CurrentMaterial : CanonicalBase;
	UMaterialInstanceDynamic* CanonicalDynamic =
		UMaterialInstanceDynamic::Create(DynamicParent, this);
	if (!IsValid(CanonicalDynamic)
		|| !APSStellarMaterialContract::HasExactBase(
			CanonicalDynamic, APSStellarMaterialContract::ActorBaseObjectPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[APS.StellarMaterial] Failed to create canonical actor MID for %s"),
			*GetNameSafe(this));
		StarDynamicMaterial = nullptr;
		return nullptr;
	}

	StarDynamicMaterial = CanonicalDynamic;
	StarMesh->SetMaterial(0, CanonicalDynamic);
	return CanonicalDynamic;
}

void AStar::SetStarProperties(FLinearColor Color, float Multiplier)
{
	if (StarDynamicMaterial != nullptr)
	{
		StarDynamicMaterial->SetVectorParameterValue(FName("Color"), Color);
		StarDynamicMaterial->SetScalarParameterValue(FName("Multiplier"), Multiplier);
	}
}

void AStar::AddPlanet(APlanet* Planet)
{
	this->Planets.Add(Planet);
}

void AStar::SetPlanetarySystem(APlanetarySystem* NewPlanetarySystem)
{
	this->PlanetarySystem = NewPlanetarySystem;
}

void AStar::SetLuminosity(float StarLuminosity)
{
	this->Luminosity = StarLuminosity;
}

void AStar::SetSurfaceTemperature(int NewSurfaceTemperature)
{
	this->SurfaceTemperature = NewSurfaceTemperature;
}

void AStar::SetStarType(EStellarType NewStarType)
{
	this->StellarClass = NewStarType;
}

void AStar::SetStarSpectralClass(ESpectralClass NewStarSpectralClass)
{
	this->SpectralClass = NewStarSpectralClass;
}

void AStar::SetStarSpectralType(ESpectralType NewSpectralType)
{
	this->SpectralType = NewSpectralType;
}

void AStar::SetFullSpectralClass(FName NewFullSpectralClass)
{
	this->FullSpectralClass = NewFullSpectralClass;
}

void AStar::SetFullSpectralName(FName NewFullSpectralName)
{
	this->FullSpectralName = NewFullSpectralName;
}

void AStar::SetSpectralSubclass(int NewSpectralSubclass)
{
	this->SpectralSubclass = NewSpectralSubclass;
}

FName AStar::GenerateFullSpectralName()
{
	FString SpectralClassName = UEnum::GetValueAsString(SpectralClass);
	FString SpectralTypeName = UEnum::GetValueAsString(SpectralType);
	SpectralClassName = SpectralClassName.RightChop(SpectralClassName.Find(TEXT("::")) + 2);
	SpectralTypeName = SpectralTypeName.RightChop(SpectralTypeName.Find(TEXT("::")) + 2);
	return FName(*FString::Printf(TEXT("%s%d%s"), *SpectralClassName, SpectralSubclass, *SpectralTypeName));
}

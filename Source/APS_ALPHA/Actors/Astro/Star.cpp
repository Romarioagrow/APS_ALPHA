#include "Star.h"

AStar::AStar()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetarySystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetarySystemZoneComponent"));
	PlanetarySystemZone->SetupAttachment(RootComponent);

	StarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarMesh"));
	StarMesh->SetupAttachment(RootComponent);
}

void AStar::BeginPlay()
{
	Super::BeginPlay();

	StarDynamicMaterial = UMaterialInstanceDynamic::Create(StarMesh->GetMaterial(0), this);
	if (StarDynamicMaterial)
	{
		StarMesh->SetMaterial(0, StarDynamicMaterial);
	}
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

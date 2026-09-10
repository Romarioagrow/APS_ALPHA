#include "Star.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStar::AStar()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = Root;

	PlanetarySystemZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlanetarySystemZoneComponent"));
	PlanetarySystemZone->SetupAttachment(RootComponent);
	PlanetarySystemZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanetarySystemZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlanetarySystemZone->SetGenerateOverlapEvents(false);
	PlanetarySystemZone->SetCanEverAffectNavigation(false);
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

	// The photosphere stays opaque and detailed. This second shell is the geometry
	// that can actually extend beyond that silhouette; post-process bloom alone
	// cannot manufacture a broad physical corona from an opaque sphere.
	CoronaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoronaMesh"));
	// UE 5.4 rejects the additive corona MID on a Nanite copy of the photosphere
	// mesh. Force only this shell through the regular fallback proxy before a mesh
	// or material can create its render state; the opaque photosphere keeps Nanite.
	CoronaMesh->bDisallowNanite = true;
	CoronaMesh->SetForceDisableNanite(true);
	CoronaMesh->SetupAttachment(StarMesh);
	CoronaMesh->SetRelativeScale3D(FVector(1.12));
	CoronaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoronaMesh->SetGenerateOverlapEvents(false);
	CoronaMesh->SetCanEverAffectNavigation(false);
	CoronaMesh->SetCastShadow(false);
	CoronaMesh->bAffectDynamicIndirectLighting = false;
	CoronaMesh->bAffectDistanceFieldLighting = false;
	CoronaMesh->SetReceivesDecals(false);
	CoronaMesh->SetTranslucentSortPriority(2);

	// This is deliberately a small, unshadowed local emitter. Long-range physical
	// daylight remains owned by UAPSStellarVisualSubsystem's directional key, while
	// the component proves that a materialized star contributes real scene light.
	StellarLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StellarLight"));
	StellarLight->SetupAttachment(StarMesh);
	StellarLight->SetAbsolute(false, false, true);
	StellarLight->SetMobility(EComponentMobility::Movable);
	StellarLight->SetUseInverseSquaredFalloff(false);
	StellarLight->SetLightFalloffExponent(2.0f);
	StellarLight->SetIntensity(8.0f);
	StellarLight->SetAttenuationRadius(500000.0f);
	StellarLight->SetInverseExposureBlend(1.0f);
	StellarLight->SetCastShadows(false);
	StellarLight->SetAffectTranslucentLighting(false);
	StellarLight->SetVolumetricScatteringIntensity(0.05f);
}

void AStar::BeginPlay()
{
	Super::BeginPlay();
	PlanetarySystemZone->SetVisibility(false, true);
	PlanetarySystemZone->SetHiddenInGame(true, true);
	// Blueprint component templates can retain an old blocking collision profile.
	// This sphere is radius metadata for navigation/HUD only; it must never become
	// an invisible wall around a generated system.
	PlanetarySystemZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanetarySystemZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlanetarySystemZone->SetGenerateOverlapEvents(false);

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

	if (IsValid(CoronaMesh))
	{
		// Reassert the native default for pre-existing Blueprint component templates
		// before assigning the Nanite photosphere mesh and additive corona material.
		CoronaMesh->bDisallowNanite = true;
		CoronaMesh->SetForceDisableNanite(true);
		CoronaMesh->SetStaticMesh(StarMesh->GetStaticMesh());
		UMaterial* CoronaBase = APSStellarMaterialContract::LoadCanonicalBase(
			APSStellarMaterialContract::CoronaBaseObjectPath);
		if (IsValid(CoronaBase))
		{
			CoronaDynamicMaterial = UMaterialInstanceDynamic::Create(CoronaBase, this);
			CoronaMesh->SetMaterial(0, CoronaDynamicMaterial);
		}
		else
		{
			CoronaDynamicMaterial = nullptr;
			CoronaMesh->SetVisibility(false, true);
			UE_LOG(LogTemp, Error,
				TEXT("[APS.StellarMaterial] Canonical corona master is unavailable: %s"),
				APSStellarMaterialContract::CoronaBaseObjectPath);
		}
	}
	return CanonicalDynamic;
}

void AStar::ConfigureStellarPresentation(
	const FLinearColor& Color, const float Emission, const float SurfaceSeed,
	const EStellarType StellarType)
{
	ConfigureStellarPresentationComponents(StarMesh, CoronaMesh, CoronaDynamicMaterial,
		StellarLight, Color, Emission, SurfaceSeed, StellarType);
}

void AStar::ConfigureStellarPresentationComponents(
	UStaticMeshComponent* StarMesh, UStaticMeshComponent* CoronaMesh,
	UMaterialInstanceDynamic* CoronaDynamicMaterial, UPointLightComponent* StellarLight,
	const FLinearColor& Color, const float Emission, const float SurfaceSeed,
	const EStellarType StellarType)
{
	if (!IsValid(StarMesh))
	{
		return;
	}

	StarMesh->UpdateBounds();
	const bool bBlackHole = StellarType == EStellarType::BlackHole;
	// The fallback proxy stays close enough to the photosphere that it cannot read
	// as a second globe. Its material receives the exact reciprocal below and emits
	// a monotonic limb-to-space falloff, concentrating HDR at the photosphere edge
	// instead of drawing a detached geometric crown.
	float CoronaScale = 1.12f;
	float TypeGain = 1.0f;
	switch (StellarType)
	{
	case EStellarType::Protostar: CoronaScale = 1.14f; TypeGain = 1.40f; break;
	case EStellarType::HyperGiant: CoronaScale = 1.14f; TypeGain = 1.35f; break;
	case EStellarType::SuperGiant: CoronaScale = 1.13f; TypeGain = 1.28f; break;
	case EStellarType::BrightGiant: CoronaScale = 1.13f; TypeGain = 1.20f; break;
	case EStellarType::Giant: CoronaScale = 1.12f; TypeGain = 1.14f; break;
	case EStellarType::SubGiant: CoronaScale = 1.11f; TypeGain = 1.08f; break;
	case EStellarType::Neutron: CoronaScale = 1.08f; TypeGain = 1.18f; break;
	case EStellarType::Pulsar: CoronaScale = 1.10f; TypeGain = 1.34f; break;
	case EStellarType::WhiteDwarf: CoronaScale = 1.08f; TypeGain = 0.92f; break;
	case EStellarType::SubDwarf: CoronaScale = 1.09f; TypeGain = 0.82f; break;
	case EStellarType::BrownDwarf: CoronaScale = 1.08f; TypeGain = 0.42f; break;
	default: break;
	}

	const float EmissionActivity = FMath::Clamp(
		FMath::Log2(1.0f + FMath::Max(Emission, 0.0f)) / 8.97f, 0.0f, 1.0f);
	if (IsValid(CoronaMesh))
	{
		CoronaMesh->SetStaticMesh(StarMesh->GetStaticMesh());
		CoronaMesh->SetRelativeScale3D(FVector(CoronaScale));
		CoronaMesh->SetVisibility(!bBlackHole && IsValid(CoronaDynamicMaterial), true);
	}
	if (IsValid(CoronaDynamicMaterial) && !bBlackHole)
	{
		CoronaDynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		CoronaDynamicMaterial->SetScalarParameterValue(TEXT("CoronaShellMode"), 1.0f);
		CoronaDynamicMaterial->SetScalarParameterValue(TEXT("CoronaInnerRadius"),
			1.0f / CoronaScale);
		CoronaDynamicMaterial->SetScalarParameterValue(TEXT("CoronaSeed"), SurfaceSeed);
		CoronaDynamicMaterial->SetScalarParameterValue(TEXT("CoronaIntensity"),
			FMath::Min(FMath::Lerp(128.0f, 160.0f, EmissionActivity) * TypeGain, 192.0f));
		CoronaDynamicMaterial->SetScalarParameterValue(TEXT("CoronaOpacity"),
			FMath::Lerp(0.68f, 0.76f, EmissionActivity));
	}

	if (IsValid(StellarLight))
	{
		StellarLight->SetVisibility(!bBlackHole);
		StellarLight->SetLightColor(Color.GetClamped());
		StellarLight->SetIntensity(bBlackHole ? 0.0f
			: FMath::Lerp(6.0f, 24.0f, EmissionActivity) * TypeGain);
		SyncStellarLightToBounds(StarMesh, StellarLight);
		if (const UStaticMesh* Mesh = StarMesh->GetStaticMesh())
		{
			StellarLight->SetRelativeLocation(Mesh->GetBounds().Origin);
		}
	}
}

void AStar::SyncStellarLightToPresentedBounds()
{
	SyncStellarLightToBounds(StarMesh, StellarLight);
}

void AStar::SyncStellarLightToBounds(UStaticMeshComponent* StarMesh, UPointLightComponent* StellarLight)
{
	if (!IsValid(StarMesh) || !IsValid(StellarLight))
	{
		return;
	}
	StarMesh->UpdateBounds();
	const double VisualRadius = FMath::Max(
		static_cast<double>(StarMesh->Bounds.SphereRadius), 100.0);
	StellarLight->SetAttenuationRadius(static_cast<float>(FMath::Clamp(
		VisualRadius * 96.0, 500000.0, 2.0e13)));
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

#include "APSColonyModule.h"

#include "APSColonyModuleCatalogue.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace APSColonyModuleBuild
{
	/** The plinth's top above the support plane; every part stands on it. */
	constexpr double FoundationTopCm = 30.0;
	constexpr double MinimumFoundationDepthCm = 20.0;
	constexpr double BoomThicknessCm = 70.0;
	const FLinearColor Concrete(0.30f, 0.29f, 0.27f);
	const FLinearColor BoomColor(0.22f, 0.23f, 0.25f);

	const TCHAR* ShapePath(const EAPSColonyPartShape Shape)
	{
		switch (Shape)
		{
		case EAPSColonyPartShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case EAPSColonyPartShape::Sphere: return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case EAPSColonyPartShape::Cone: return TEXT("/Engine/BasicShapes/Cone.Cone");
		default: return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return Path ? LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	}
}

AAPSColonyModule::AAPSColonyModule()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.AddUnique(TEXT("APS.Colony.Module"));
	ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
	ModuleRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(ModuleRoot);
}

void AAPSColonyModule::Configure(const FName InModuleId, const FGuid& InStableId, const FGuid& InOwnerCivilizationId,
	const double InFoundationDepthCm, const double InBoomLengthCm)
{
	ModuleId = InModuleId;
	StableId = InStableId;
	OwnerCivilizationId = InOwnerCivilizationId;
	FoundationDepthCm = FMath::Max(0.0, InFoundationDepthCm);
	BoomLengthCm = FMath::Max(0.0, InBoomLengthCm);
	// Navigation lists and the terminal show the module by its catalogue name.
	if (const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId))
	{
		InGameName = Spec->Name;
	}
}

void AAPSColonyModule::BeginPlay()
{
	Super::BeginPlay();
	BuildParts();
}

void AAPSColonyModule::BuildParts()
{
	using namespace APSColonyModuleBuild;
	const FAPSColonyModuleSpec* Spec = FAPSColonyModuleCatalogue::Find(ModuleId);
	if (bPartsBuilt || !Spec)
	{
		return;
	}
	bPartsBuilt = true;
	Tags.AddUnique(FName(*FString::Printf(TEXT("APS.Colony.Module.%s"), *ModuleId.ToString())));

	FVector Lift = FVector::ZeroVector;
	if (Spec->bFoundation)
	{
		// The plinth: its top above the support plane, its bottom down to the lowest ground under the footprint.
		const double Depth = FMath::Max(FoundationDepthCm, MinimumFoundationDepthCm);
		FAPSColonyModulePart Plinth;
		Plinth.Size = FVector(Spec->SizeCm.X, Spec->SizeCm.Y, Depth + FoundationTopCm);
		Plinth.Center = FVector(0.0, 0.0, (FoundationTopCm - Depth) * 0.5);
		Plinth.Color = Concrete;
		AddPart(Plinth, FVector::ZeroVector);
		Lift.Z = FoundationTopCm;
	}
	for (const FAPSColonyModulePart& Part : Spec->Parts)
	{
		AddPart(Part, Lift);
	}
	if (BoomLengthCm > 0.0)
	{
		// Orbit: a boom from the front face into the station the module is bolted to.
		FAPSColonyModulePart Boom;
		Boom.Size = FVector(BoomLengthCm, BoomThicknessCm, BoomThicknessCm);
		Boom.Center = FVector(Spec->SizeCm.X * 0.5 + BoomLengthCm * 0.5, 0.0, Spec->SizeCm.Z * 0.5);
		Boom.Color = BoomColor;
		AddPart(Boom, FVector::ZeroVector);
	}
	for (const FAPSColonyModuleLight& LightSpec : Spec->Lights)
	{
		ULocalLightComponent* Light = nullptr;
		if (LightSpec.ConeDegrees > 0.0f)
		{
			USpotLightComponent* Spot = NewObject<USpotLightComponent>(this);
			Spot->SetOuterConeAngle(LightSpec.ConeDegrees * 0.5f);
			Spot->SetInnerConeAngle(LightSpec.ConeDegrees * 0.3f);
			Light = Spot;
		}
		else
		{
			Light = NewObject<UPointLightComponent>(this);
		}
		Light->SetupAttachment(ModuleRoot);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetRelativeLocationAndRotation(LightSpec.Location + Lift, LightSpec.Rotation);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(LightSpec.IntensityCandelas);
		Light->SetLightColor(LightSpec.Color);
		Light->SetAttenuationRadius(LightSpec.RadiusCm);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
	}
}

void AAPSColonyModule::AddPart(const FAPSColonyModulePart& Part, const FVector& Lift)
{
	using namespace APSColonyModuleBuild;
	UStaticMesh* Preferred = LoadMesh(Part.PreferredMesh);
	UMaterialInterface* PackMaterial = Part.PreferredMaterial
		? LoadObject<UMaterialInterface>(nullptr, Part.PreferredMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	if ((Part.PreferredMesh && !Preferred) || (Part.PreferredMaterial && !PackMaterial))
	{
		++FallbackPartCount;
		if (Part.bSkipWithoutPack)
		{
			return;
		}
	}
	const FVector Center = Part.Center + Lift;
	if (!Preferred)
	{
		// Engine shapes are 100 cm and centred: they fill the slot exactly.
		if (UStaticMeshComponent* Component = AddMeshComponent(LoadMesh(ShapePath(Part.Shape)),
			FTransform(Part.Rotation, Center, Part.Size / 100.0), Part.bCollides))
		{
			Component->SetMaterial(0, PackMaterial ? PackMaterial : ShapeMaterial(Part.Color, Part.Glow));
		}
		return;
	}

	// A pack mesh keeps its proportions: long side along the slot's, uniform fit, on the slot's floor, tiled if asked.
	const FBox MeshBox = Preferred->GetBoundingBox();
	const FVector Extent = MeshBox.GetExtent();
	const bool bSlotLongX = Part.Size.X >= Part.Size.Y;
	const FRotator AutoYaw(0.0, (Extent.X >= Extent.Y) == bSlotLongX ? 0.0 : 90.0, 0.0);
	const FVector TurnedExtent = AutoYaw.RotateVector(Extent).GetAbs();
	const FVector TurnedCenter = AutoYaw.RotateVector(MeshBox.GetCenter());
	const int32 LongAxis = bSlotLongX ? 0 : 1;
	const int32 ShortAxis = 1 - LongAxis;
	const double SlotLong = Part.Size[LongAxis];
	double Fit = FMath::Min(Part.Size.Z / (2.0 * FMath::Max(TurnedExtent.Z, 1.0)),
		Part.Size[ShortAxis] / (2.0 * FMath::Max(TurnedExtent[ShortAxis], 1.0)));
	int32 Tiles = 1;
	if (Part.bTile)
	{
		const double TileLength = 2.0 * TurnedExtent[LongAxis] * Fit;
		Tiles = FMath::Clamp(FMath::RoundToInt(SlotLong / FMath::Max(TileLength, 1.0)), 1, 24);
	}
	Fit = FMath::Min(Fit, (SlotLong / Tiles) / (2.0 * FMath::Max(TurnedExtent[LongAxis], 1.0)));
	const FQuat Rotation = Part.Rotation.Quaternion() * AutoYaw.Quaternion();
	for (int32 Tile = 0; Tile < Tiles; ++Tile)
	{
		FVector Local = FVector::ZeroVector;
		Local[LongAxis] = -SlotLong * 0.5 + (Tile + 0.5) * SlotLong / Tiles;
		Local -= TurnedCenter * Fit;
		Local.Z += -Part.Size.Z * 0.5 + TurnedExtent.Z * Fit;
		AddMeshComponent(Preferred, FTransform(Rotation, Center + Part.Rotation.RotateVector(Local), FVector(Fit)),
			Part.bCollides);
	}
}

UStaticMeshComponent* AAPSColonyModule::AddMeshComponent(UStaticMesh* Mesh, const FTransform& RelativeTransform,
	const bool bCollides)
{
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetupAttachment(ModuleRoot);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(Mesh);
	Component->SetRelativeTransform(RelativeTransform);
	Component->SetCollisionProfileName(bCollides
		? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->RegisterComponent();
	AddInstanceComponent(Component);
	++PartCount;
	return Component;
}

UMaterialInterface* AAPSColonyModule::ShapeMaterial(const FLinearColor& Color, const float Glow)
{
	const FString Key = FString::Printf(TEXT("%s|%.2f"), *Color.ToString(), Glow);
	if (const TObjectPtr<UMaterialInterface>* Cached = MaterialCache.Find(Key))
	{
		return *Cached;
	}
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, Glow > 0.0f
		? TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")
		: TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Parent)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, this);
	Material->SetVectorParameterValue(TEXT("Color"), Glow > 0.0f ? Color * Glow : Color);
	MaterialCache.Add(Key, Material);
	return Material;
}

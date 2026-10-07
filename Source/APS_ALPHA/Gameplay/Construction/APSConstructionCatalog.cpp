#include "APSConstructionCatalog.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "APSConstruction"

namespace APSConstructionCatalogLocal
{
	using namespace APSConstruction;

	const FName PlacedTagName(TEXT("APS.Construction.Placed"));
	const FString PropTagPrefix(TEXT("APS.Construction.Prop."));

	const FLinearColor Concrete(0.30f, 0.29f, 0.27f);
	const FLinearColor PadConcrete(0.16f, 0.16f, 0.17f);
	const FLinearColor Metal(0.22f, 0.23f, 0.25f);
	const FLinearColor Hull(0.72f, 0.74f, 0.76f);
	const FLinearColor Steel(0.25f, 0.30f, 0.36f);
	const FLinearColor CrateBrown(0.46f, 0.38f, 0.24f);
	const FLinearColor Solar(0.015f, 0.03f, 0.10f);
	const FLinearColor Amber(0.90f, 0.45f, 0.05f);
	const FLinearColor Paint(0.75f, 0.75f, 0.72f);
	const FLinearColor Warm(1.0f, 0.85f, 0.65f);
	const FLinearColor Signal(1.0f, 0.06f, 0.03f);

	const TCHAR* ShapePath(const EShape Shape)
	{
		switch (Shape)
		{
		case EShape::Cylinder: return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		case EShape::Sphere: return TEXT("/Engine/BasicShapes/Sphere.Sphere");
		case EShape::Cone: return TEXT("/Engine/BasicShapes/Cone.Cone");
		default: return TEXT("/Engine/BasicShapes/Cube.Cube");
		}
	}

	/** Pack paths found missing once are not looked for again (the packs do not appear while the game runs). */
	UStaticMesh* LoadMesh(const FString& Path)
	{
		static TSet<FString> Missing;
		if (Path.IsEmpty() || Missing.Contains(Path))
		{
			return nullptr;
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh)
		{
			Missing.Add(Path);
		}
		return Mesh;
	}

	UMaterialInterface* ShapeMaterial(UObject* Outer, const FLinearColor& Color, const float Glow)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, Glow > 0.0f
			? TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")
			: TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInstanceDynamic* Material = Parent && Outer ? UMaterialInstanceDynamic::Create(Parent, Outer) : nullptr;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Glow > 0.0f ? Color * Glow : Color);
		}
		return Material;
	}

	/** A pack mesh in its slot: uniform fit, centred across, standing on the slot's floor, turned with the slot. */
	FTransform FitPack(const UStaticMesh& Mesh, const FPart& Part)
	{
		const FBox Box = Mesh.GetBoundingBox();
		const FVector Extent = Box.GetExtent();
		const double Fit = FMath::Min3(Part.Size.X / (2.0 * FMath::Max(Extent.X, 1.0)),
			Part.Size.Y / (2.0 * FMath::Max(Extent.Y, 1.0)), Part.Size.Z / (2.0 * FMath::Max(Extent.Z, 1.0)));
		FVector Offset = -Box.GetCenter() * Fit;
		Offset.Z = -Part.Size.Z * 0.5 - Box.Min.Z * Fit;
		const FQuat Rotation = Part.Rotation.Quaternion();
		return FTransform(Rotation, Part.Center + Rotation.RotateVector(Offset), FVector(Fit));
	}

	FPart Shape(const EShape InShape, const FVector& Center, const FVector& Size, const FLinearColor& Color,
		const bool bCollides = true, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FPart Part;
		Part.Shape = InShape;
		Part.Center = Center;
		Part.Size = Size;
		Part.Color = Color;
		Part.bCollides = bCollides;
		Part.Rotation = Rotation;
		return Part;
	}

	FPart Pack(const TCHAR* Path, const EShape Fallback, const FVector& Center, const FVector& Size,
		const FLinearColor& FallbackColor)
	{
		FPart Part = Shape(Fallback, Center, Size, FallbackColor);
		Part.PackMesh = Path;
		return Part;
	}

	FPart Glow(const EShape InShape, const FVector& Center, const FVector& Size, const FLinearColor& Color,
		const float Strength)
	{
		FPart Part = Shape(InShape, Center, Size, Color, false);
		Part.Glow = Strength;
		return Part;
	}

	FLight Light(const FVector& Location, const FLinearColor& Color, const float Candelas, const float RadiusCm)
	{
		FLight Result;
		Result.Location = Location;
		Result.Color = Color;
		Result.Candelas = Candelas;
		Result.RadiusCm = RadiusCm;
		return Result;
	}

	TArray<FPropType> Build()
	{
		TArray<FPropType> Catalogue;
		const auto Add = [&Catalogue](const TCHAR* Id, const FText& Name, const FText& Role, const float MaxSlope,
			const bool bAlignToGround) -> FPropType&
		{
			FPropType& Type = Catalogue.AddDefaulted_GetRef();
			Type.Id = FName(Id);
			Type.Name = Name;
			Type.Role = Role;
			Type.MaxSlopeDegrees = MaxSlope;
			Type.bAlignToGround = bAlignToGround;
			return Type;
		};

		// Sizes are the packs' own (bounds read from the assets): the fallback shape fills the same slot.
		{
			FPropType& Type = Add(TEXT("CargoCrate"), LOCTEXT("CargoCrate", "CARGO CRATE"),
				LOCTEXT("CargoCrateRole", "A one-metre crate of colony supplies."), 30.0f, true);
			Type.Parts.Add(Pack(TEXT("/Game/SpaceColonies/Meshes/SM_ColonyBox01.SM_ColonyBox01"), EShape::Cube,
				FVector(0.0, 0.0, 50.0), FVector(100.0, 100.0, 100.0), CrateBrown));
		}
		{
			FPropType& Type = Add(TEXT("CargoContainer"), LOCTEXT("CargoContainer", "CARGO CONTAINER"),
				LOCTEXT("CargoContainerRole", "A cargo ship's pod set down as storage."), 20.0f, true);
			Type.Parts.Add(Pack(TEXT("/Game/missiontominerva/Geometries/KB3D_MTM_VehicleCargoShip_A_CargoB.KB3D_MTM_VehicleCargoShip_A_CargoB"),
				EShape::Cube, FVector(0.0, 0.0, 163.0), FVector(313.0, 548.0, 326.0), Steel));
		}
		{
			FPropType& Type = Add(TEXT("WaterTank"), LOCTEXT("WaterTank", "WATER TANK"),
				LOCTEXT("WaterTankRole", "Water and volatiles for the colony."), 20.0f, true);
			Type.Parts.Add(Pack(TEXT("/Game/BigCompanyArchViz/StaticMesh/Probs/SM_WaterTank.SM_WaterTank"), EShape::Cylinder,
				FVector(0.0, 0.0, 100.0), FVector(182.0, 353.0, 199.0), Hull));
		}
		{
			FPropType& Type = Add(TEXT("LandingPad"), LOCTEXT("LandingPad", "LANDING PAD"),
				LOCTEXT("LandingPadRole", "An eight-metre pad for shuttles and rovers."), 12.0f, true);
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 15.0), FVector(800.0, 800.0, 30.0), PadConcrete));
			// The amber ring: a ring-coloured disc under a slightly smaller pad-coloured one.
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 31.0), FVector(640.0, 640.0, 2.0), Amber, false));
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 32.0), FVector(590.0, 590.0, 2.0), PadConcrete, false));
			// The H.
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, -80.0, 33.5), FVector(240.0, 36.0, 1.0), Paint, false));
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 80.0, 33.5), FVector(240.0, 36.0, 1.0), Paint, false));
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 33.5), FVector(36.0, 160.0, 1.0), Paint, false));
			for (const FVector& Stud : {FVector(340.0, 0.0, 34.0), FVector(-340.0, 0.0, 34.0), FVector(0.0, 340.0, 34.0),
				FVector(0.0, -340.0, 34.0)})
			{
				Type.Parts.Add(Glow(EShape::Cylinder, Stud, FVector(18.0, 18.0, 8.0), Amber, 6.0f));
			}
		}
		{
			FPropType& Type = Add(TEXT("SolarPanel"), LOCTEXT("SolarPanel", "SOLAR PANEL"),
				LOCTEXT("SolarPanelRole", "Three metres of photovoltaic film on a post."), 25.0f, true);
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 60.0), FVector(16.0, 16.0, 120.0), Metal));
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 120.0), FVector(24.0, 300.0, 12.0), Metal));
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 136.0), FVector(170.0, 320.0, 6.0), Solar, true,
				FRotator(25.0, 0.0, 0.0)));
		}
		{
			FPropType& Type = Add(TEXT("LightMast"), LOCTEXT("LightMast", "LIGHT MAST"),
				LOCTEXT("LightMastRole", "A five-metre floodlight for work after dark."), 30.0f, false);
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 10.0), FVector(50.0, 50.0, 20.0), Concrete));
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 260.0), FVector(16.0, 16.0, 480.0), Metal));
			Type.Parts.Add(Shape(EShape::Cube, FVector(20.0, 0.0, 506.0), FVector(70.0, 40.0, 18.0), Metal));
			Type.Parts.Add(Glow(EShape::Cube, FVector(20.0, 0.0, 495.0), FVector(60.0, 32.0, 4.0), Warm, 10.0f));
			Type.Lights.Add(Light(FVector(20.0, 0.0, 470.0), Warm, 2500.0f, 3500.0f));
		}
		{
			FPropType& Type = Add(TEXT("NavBeacon"), LOCTEXT("NavBeacon", "NAV BEACON"),
				LOCTEXT("NavBeaconRole", "An amber marker light for paths and pads."), 35.0f, false);
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 10.0), FVector(60.0, 60.0, 20.0), Metal));
			Type.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 120.0), FVector(12.0, 12.0, 200.0), Hull));
			Type.Parts.Add(Glow(EShape::Sphere, FVector(0.0, 0.0, 232.0), FVector(34.0, 34.0, 34.0), Amber, 12.0f));
			Type.Lights.Add(Light(FVector(0.0, 0.0, 232.0), Amber, 800.0f, 1800.0f));
		}
		{
			FPropType& Type = Add(TEXT("Barrier"), LOCTEXT("Barrier", "BARRIER"),
				LOCTEXT("BarrierRole", "A three-metre concrete barrier."), 25.0f, true);
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 45.0), FVector(60.0, 300.0, 90.0), Concrete));
			Type.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 70.0), FVector(62.0, 302.0, 12.0), Amber, false));
		}
		{
			FPropType& Type = Add(TEXT("Terminal"), LOCTEXT("Terminal", "TERMINAL"),
				LOCTEXT("TerminalRole", "A field terminal on the colony network."), 20.0f, true);
			Type.Parts.Add(Pack(TEXT("/Game/ModularSciFiOffice/Meshes/Props/SM_Client_Terminal_01.SM_Client_Terminal_01"),
				EShape::Cube, FVector(0.0, 0.0, 68.5), FVector(50.0, 50.0, 137.0), Metal));
		}
		{
			// The tower is in the repository (Content/APS), so it is never a fallback.
			FPropType& Type = Add(TEXT("SignalTower"), LOCTEXT("SignalTower", "SIGNAL TOWER"),
				LOCTEXT("SignalTowerRole", "A fourteen-metre relay mast with an aviation light."), 15.0f, false);
			Type.Parts.Add(Pack(TEXT("/Game/APS/APS_ALPHA/Assets/Meshes/KB3D_MTM_BldgLgTerraformer_A_ElevatorTowerB.KB3D_MTM_BldgLgTerraformer_A_ElevatorTowerB"),
				EShape::Cylinder, FVector(0.0, 0.0, 680.0), FVector(380.0, 270.0, 1360.0), Metal));
			Type.Parts.Add(Glow(EShape::Sphere, FVector(0.0, 0.0, 1378.0), FVector(30.0, 30.0, 30.0), Signal, 14.0f));
			Type.Lights.Add(Light(FVector(0.0, 0.0, 1378.0), Signal, 400.0f, 2500.0f));
		}
		return Catalogue;
	}
}

const TArray<APSConstruction::FPropType>& APSConstruction::Props()
{
	static const TArray<FPropType> Catalogue = APSConstructionCatalogLocal::Build();
	return Catalogue;
}

const APSConstruction::FPropType* APSConstruction::FindProp(const FName Id)
{
	return Id.IsNone() ? nullptr : Props().FindByPredicate([Id](const FPropType& Type) { return Type.Id == Id; });
}

void APSConstruction::ResolveProp(const FPropType& Type, UObject* MaterialOuter, TArray<FMeshPiece>& OutPieces,
	int32* OutFallbacks)
{
	using namespace APSConstructionCatalogLocal;
	OutPieces.Reset();
	int32 Fallbacks = 0;
	for (const FPart& Part : Type.Parts)
	{
		UStaticMesh* PackMesh = Part.PackMesh ? LoadMesh(Part.PackMesh) : nullptr;
		if (Part.PackMesh && !PackMesh)
		{
			++Fallbacks;
		}
		FMeshPiece Piece;
		Piece.bCollides = Part.bCollides;
		if (PackMesh)
		{
			Piece.Mesh = PackMesh;
			Piece.Transform = FitPack(*PackMesh, Part);
		}
		else
		{
			// Engine shapes are 100 cm and centred: they fill the slot exactly.
			Piece.Mesh = LoadMesh(ShapePath(Part.Shape));
			Piece.Transform = FTransform(Part.Rotation, Part.Center, Part.Size / 100.0);
			Piece.Material = MaterialOuter ? ShapeMaterial(MaterialOuter, Part.Color, Part.Glow) : nullptr;
		}
		if (Piece.Mesh)
		{
			OutPieces.Add(Piece);
		}
	}
	if (OutFallbacks)
	{
		*OutFallbacks = Fallbacks;
	}
}

FString APSConstruction::PrimaryMeshPath(const FPropType& Type)
{
	using namespace APSConstructionCatalogLocal;
	if (Type.Parts.IsEmpty())
	{
		return FString();
	}
	const FPart& First = Type.Parts[0];
	return First.PackMesh && LoadMesh(First.PackMesh) ? FString(First.PackMesh) : FString(ShapePath(First.Shape));
}

void APSConstruction::GatherClassPieces(UClass* Class, TArray<FMeshPiece>& OutPieces, const int32 MaxPieces)
{
	OutPieces.Reset();
	const AActor* Default = Class ? Class->GetDefaultObject<AActor>() : nullptr;
	if (!Default)
	{
		return;
	}
	// Native components: their relative transforms up to the root (whose own is the actor's).
	const USceneComponent* DefaultRoot = Default->GetRootComponent();
	TInlineComponentArray<UStaticMeshComponent*> NativeMeshes;
	Default->GetComponents(NativeMeshes);
	for (const UStaticMeshComponent* Component : NativeMeshes)
	{
		if (OutPieces.Num() >= MaxPieces)
		{
			return;
		}
		if (!Component || !Component->GetStaticMesh() || !Component->IsVisible())
		{
			continue;
		}
		FTransform ToRoot = Component == DefaultRoot ? FTransform::Identity : Component->GetRelativeTransform();
		for (const USceneComponent* Parent = Component->GetAttachParent(); Parent && Parent != DefaultRoot;
			Parent = Parent->GetAttachParent())
		{
			ToRoot = ToRoot * Parent->GetRelativeTransform();
		}
		FMeshPiece& Piece = OutPieces.AddDefaulted_GetRef();
		Piece.Mesh = Component->GetStaticMesh();
		Piece.Transform = ToRoot;
	}
	// Blueprint components: the construction scripts' templates, through their parents in the same script. A root node
	// of a class without a native root becomes the actor's root, so its own transform is the actor's.
	for (UClass* Current = Class; Current; Current = Current->GetSuperClass())
	{
		const UBlueprintGeneratedClass* Generated = Cast<UBlueprintGeneratedClass>(Current);
		const USimpleConstructionScript* Script = Generated ? Generated->SimpleConstructionScript.Get() : nullptr;
		if (!Script)
		{
			continue;
		}
		const TArray<USCS_Node*>& Roots = Script->GetRootNodes();
		for (USCS_Node* Node : Script->GetAllNodes())
		{
			if (OutPieces.Num() >= MaxPieces)
			{
				return;
			}
			const UStaticMeshComponent* Template = Node ? Cast<UStaticMeshComponent>(Node->ComponentTemplate) : nullptr;
			if (!Template || !Template->GetStaticMesh() || !Template->IsVisible())
			{
				continue;
			}
			const bool bBecomesRoot = !DefaultRoot && Roots.Contains(Node);
			FTransform ToRoot = bBecomesRoot ? FTransform::Identity : Template->GetRelativeTransform();
			for (USCS_Node* Parent = Script->FindParentNode(Node); Parent; Parent = Script->FindParentNode(Parent))
			{
				const USceneComponent* ParentTemplate = Cast<USceneComponent>(Parent->ComponentTemplate);
				if (!ParentTemplate || (!DefaultRoot && Roots.Contains(Parent)))
				{
					break;
				}
				ToRoot = ToRoot * ParentTemplate->GetRelativeTransform();
			}
			FMeshPiece& Piece = OutPieces.AddDefaulted_GetRef();
			Piece.Mesh = Template->GetStaticMesh();
			Piece.Transform = ToRoot;
		}
	}
}

FBox APSConstruction::PiecesBounds(const TArray<FMeshPiece>& Pieces)
{
	FBox Bounds(ForceInit);
	for (const FMeshPiece& Piece : Pieces)
	{
		if (Piece.Mesh)
		{
			Bounds += Piece.Mesh->GetBoundingBox().TransformBy(Piece.Transform);
		}
	}
	return Bounds;
}

FName APSConstruction::PlacedTag()
{
	return APSConstructionCatalogLocal::PlacedTagName;
}

FName APSConstruction::PropIdOf(const AActor* Actor)
{
	if (Actor)
	{
		for (const FName& Tag : Actor->Tags)
		{
			const FString Text = Tag.ToString();
			if (Text.StartsWith(APSConstructionCatalogLocal::PropTagPrefix))
			{
				return FName(*Text.RightChop(APSConstructionCatalogLocal::PropTagPrefix.Len()));
			}
		}
	}
	return NAME_None;
}

AStaticMeshActor* APSConstruction::SpawnProp(UWorld* World, const FName PropId, const FString& MeshPath,
	const FTransform& WorldTransform, AActor* Site)
{
	using namespace APSConstructionCatalogLocal;
	if (!World)
	{
		return nullptr;
	}
	const FPropType* Type = FindProp(PropId);
	UStaticMesh* LoneMesh = Type ? nullptr : LoadMesh(MeshPath);
	if (!Type && !LoneMesh)
	{
		return nullptr;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Kept by the infrastructure's save, never by the level or the actor archive.
	Parameters.ObjectFlags |= RF_Transient;
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), WorldTransform, Parameters);
	if (!Actor)
	{
		return nullptr;
	}
	Actor->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* Root = Actor->GetStaticMeshComponent();
	Root->SetCanEverAffectNavigation(false);
	if (LoneMesh)
	{
		Root->SetStaticMesh(LoneMesh);
		Root->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		TArray<FMeshPiece> Pieces;
		ResolveProp(*Type, Actor, Pieces);
		for (const FMeshPiece& Piece : Pieces)
		{
			UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Actor);
			Component->SetupAttachment(Root);
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetStaticMesh(Piece.Mesh);
			Component->SetRelativeTransform(Piece.Transform);
			Component->SetCollisionProfileName(Piece.bCollides
				? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
			Component->SetGenerateOverlapEvents(false);
			Component->SetCanEverAffectNavigation(false);
			if (Piece.Material)
			{
				Component->SetMaterial(0, Piece.Material);
			}
			Component->RegisterComponent();
			Actor->AddInstanceComponent(Component);
		}
		for (const FLight& LightSpec : Type->Lights)
		{
			UPointLightComponent* PointLight = NewObject<UPointLightComponent>(Actor);
			PointLight->SetupAttachment(Root);
			PointLight->SetMobility(EComponentMobility::Movable);
			PointLight->SetRelativeLocation(LightSpec.Location);
			PointLight->SetIntensityUnits(ELightUnits::Candelas);
			PointLight->SetIntensity(LightSpec.Candelas);
			PointLight->SetLightColor(LightSpec.Color);
			PointLight->SetAttenuationRadius(LightSpec.RadiusCm);
			PointLight->SetCastShadows(false);
			PointLight->RegisterComponent();
			Actor->AddInstanceComponent(PointLight);
		}
	}
	if (Site)
	{
		Actor->AttachToActor(Site, FAttachmentTransformRules::KeepWorldTransform);
	}
	Actor->Tags.AddUnique(PlacedTagName);
	if (!PropId.IsNone())
	{
		Actor->Tags.AddUnique(FName(*(PropTagPrefix + PropId.ToString())));
	}
	return Actor;
}

#undef LOCTEXT_NAMESPACE

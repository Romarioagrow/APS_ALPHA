#include "APSStoryFinds.h"

#include "APSOriginAdrift.h"
#include "APSOriginArk.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Tech/TechInfrastructure.h"
#include "APS_ALPHA/Core/Audio/APSAudioSubsystem.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/APSWorldRules.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Core/Planetary/APSPlanetSurfaceProfile.h"
#include "APS_ALPHA/Gameplay/Ancients/APSAncientsGeometry.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationJournalSubsystem.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Gameplay/Expansion/APSMissions.h"
#include "APS_ALPHA/Gameplay/Origins/APSOriginQuests.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/CustomGravityCharacter.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/UI/Colony/APSMissionTracker.h"
#include "APS_ALPHA/UI/Hud/APSHudKit.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "WorldScapeCore/Public/WorldScapeRoot.h"

#define LOCTEXT_NAMESPACE "APSStoryFinds"

namespace APSStoryFindsLocal
{
	TAutoConsoleVariable<int32> CVarEnable(TEXT("aps.Finds.Enable"), 1,
		TEXT("Rio 19.10 (\"fill the planets with objects along the story\"): 1 places the finds round an ARK or ADRIFT colony (a lander, ")
		TEXT("a supply pod, crystals, vents; the Builders' waymarks in ARK). 0: none."));
	TAutoConsoleVariable<int32> CVarMarkers(TEXT("aps.Finds.Markers"), 1,
		TEXT("1: the walker's view marks the finds the radar has found (name and distance). 0: no markers."));
	TAutoConsoleVariable<float> CVarReachM(TEXT("aps.Finds.ReachM"), 35.0f,
		TEXT("How near the middle of a find the walker's F takes it, metres."));

	enum class EKind : uint8 { Lander, Pod, Crystals, Vents };
	constexpr int32 FindCount = 4;
	constexpr int32 WaymarkCount = 3;
	constexpr double FootprintCm = 2500.0;
	constexpr double MaxSlope = 0.18;
	constexpr double DrawCm = 900000.0;
	constexpr double SmallDrawCm = 250000.0;
	constexpr double SeenCm = 150000.0;

	struct FDef
	{
		EKind Kind;
		const TCHAR* Id;
		double MinCm;
		double MaxCm;
		float Energy;
		float Metals;
		FLinearColor Lamp;
	};

	const FDef Defs[FindCount] = {
		{EKind::Lander, TEXT("LANDER"), 150000.0, 250000.0, 40.0f, 60.0f, FLinearColor(1.0f, 0.18f, 0.06f)},
		{EKind::Pod, TEXT("POD"), 200000.0, 350000.0, 30.0f, 80.0f, FLinearColor(0.35f, 0.6f, 1.0f)},
		{EKind::Crystals, TEXT("CRYSTALS"), 300000.0, 450000.0, 25.0f, 0.0f, FLinearColor(0.35f, 0.85f, 1.0f)},
		{EKind::Vents, TEXT("VENTS"), 350000.0, 550000.0, 50.0f, 0.0f, FLinearColor(1.0f, 0.45f, 0.08f)}};

	FText NameOf(const EKind Kind, const bool bAdrift)
	{
		switch (Kind)
		{
		case EKind::Lander: return bAdrift ? LOCTEXT("NameOldLander", "THE OLD LANDER") : LOCTEXT("NameLander", "THE LANDER");
		case EKind::Pod: return bAdrift ? LOCTEXT("NameCache", "THE CACHE") : LOCTEXT("NamePod", "THE SUPPLY POD");
		case EKind::Crystals: return LOCTEXT("NameCrystals", "THE CRYSTALS");
		default: return LOCTEXT("NameVents", "THE VENTS");
		}
	}

	FText ActionOf(const EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Lander: return LOCTEXT("ActLander", "SALVAGE THE LANDER");
		case EKind::Pod: return LOCTEXT("ActPod", "OPEN THE CRATES");
		case EKind::Crystals: return LOCTEXT("ActCrystals", "TAKE A SAMPLE");
		default: return LOCTEXT("ActVents", "TAP THE VENT");
		}
	}

	FText LoreOf(const EKind Kind, const bool bAdrift)
	{
		switch (Kind)
		{
		case EKind::Lander: return bAdrift
			? LOCTEXT("LoreOldLander", "A lander of the expedition before ours, its paint faded by years of sun. Its log ends: \"Supplies cached by the vents. Going up for the station.\" The cells still hold a charge.")
			: LOCTEXT("LoreLander", "The ark's lander, torn away at forty kilometres on the way down. Nobody was aboard; its autopilot fought to the last metre. Its cells still hold a charge, its hold is full of plate.");
		case EKind::Pod: return bAdrift
			? LOCTEXT("LoreCache", "Their cache, stacked and sealed as if they meant to come back for it. They did not. Tools, plate, sealant: ours now.")
			: LOCTEXT("LorePod", "One of the ark's cargo pods, thrown clear in the fall. The chute burned away; the crates held. Tools, plate, sealant: the colony's first riches.");
		case EKind::Crystals: return LOCTEXT("LoreCrystals", "Six-sided columns that hum when touched, at the very pitch of the glyphs on the shape on the horizon. The sample warms in the hand: it gives back the light it drank all day.");
		default: return LOCTEXT("LoreVents", "The ground breathes here: hot gas from far below, steady as a heart. A tap and a coil, and the vent shares its heat with the colony.");
		}
	}

	FText WaymarkLore(const int32 Index)
	{
		switch (Index)
		{
		case 0: return LOCTEXT("Waymark0", "A stone with the glyphs of the shape on the horizon, leaning toward it like a signpost.");
		case 1: return LOCTEXT("Waymark1", "Another stone, its glyphs brighter. Someone marked the way, and meant it to be walked.");
		default: return LOCTEXT("Waymark2", "The last stone. From here the shape fills half the sky, and the glyphs pulse in time with it.");
		}
	}

	FName FindToken(const int32 Index, const bool bAdrift)
	{
		return FName(*FString::Printf(TEXT("ORIGIN_FIND_%s_%s"), bAdrift ? TEXT("ADRIFT") : TEXT("ARK"), Defs[Index].Id));
	}

	FName WaymarkToken(const int32 Index)
	{
		return FName(*FString::Printf(TEXT("ORIGIN_WAYMARK_%d"), Index));
	}

	struct FPlaced
	{
		bool bResolved{false};
		bool bBuilt{false};
		bool bSkipped{false};
		bool bSeen{false};
		FVector LocalUp{FVector::ZeroVector};
		double Yaw{0.0};
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UPointLightComponent> Lamp;
		TWeakObjectPtr<UMaterialInstanceDynamic> LampGlow;
	};

	class SFindMarkers;

	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<APlanetaryBody> Body;
		FVector CentreLocal{FVector::ZeroVector};
		bool bCentre{false};
		bool bAdrift{false};
		FPlaced Finds[FindCount];
		FPlaced Waymarks[WaymarkCount];
		bool bWaymarksResolved{false};
		double NextCentreTry{0.0};
		double NextWaymarkTry{0.0};
		TSharedPtr<SWidget> Markers;
	};

	TArray<FState>& States()
	{
		static TArray<FState> Entries;
		return Entries;
	}

	FState* Find(const UWorld* World)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	void ForgetWorld(UWorld* World, bool, bool)
	{
		for (FState& Entry : States())
		{
			if (Entry.World.Get() == World && Entry.Markers.IsValid() && GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(Entry.Markers.ToSharedRef());
			}
		}
		States().RemoveAll([World](const FState& Entry) { return Entry.World.Get() == World || !Entry.World.IsValid(); });
	}

	FState& StateFor(UWorld* World)
	{
		if (FState* Existing = Find(World))
		{
			return *Existing;
		}
		static bool bCleanupBound = false;
		if (!bCleanupBound)
		{
			bCleanupBound = true;
			FWorldDelegates::OnWorldCleanup.AddStatic(&ForgetWorld);
		}
		States().RemoveAll([](const FState& Entry) { return !Entry.World.IsValid(); });
		FState& Added = States().AddDefaulted_GetRef();
		Added.World = World;
		return Added;
	}

	/** The body's WorldScape root once its surface is built from the current seed and radius (as the Ancients wait for it). */
	AWorldScapeRoot* BuiltRoot(const APlanetaryBody* Body)
	{
		APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0f || !Surface->IsSurfaceProfileCurrent(Body))
		{
			return nullptr;
		}
		return Body->bWorldScapeSurfaceReady && Root->init && Root->PlanetScaleCode == static_cast<double>(Root->PlanetScale) ? Root : nullptr;
	}

	double SeaRadiusOf(const APlanetaryBody& Body, const AWorldScapeRoot& Root)
	{
		const APlanetarySurfaceGenerator* Surface = Body.PlanetaryEnvironmentGenerator;
		return IsValid(Surface) && Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None
			? static_cast<double>(Root.PlanetScale) + static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root.NoiseIntensity
			: -1.0;
	}

	double RadiusAt(AWorldScapeRoot& Root, const FVector& Direction)
	{
		const FVector Centre = Root.GetActorLocation();
		return Root.PlanetScale + Root.GetGroundHeight(Centre + Direction * Root.PlanetScale, false);
	}

	/** The ground's spread under a footprint and whether all of it stands above the sea (the Ancients' own test). */
	bool Fits(AWorldScapeRoot& Root, const double SeaRadius, const FVector& Up, const double Footprint, double& OutScore)
	{
		FVector Forward;
		FVector Right;
		Up.FindBestAxisVectors(Forward, Right);
		const FVector Centre = Root.GetActorLocation();
		const double CentreRadius = RadiusAt(Root, Up);
		double Low = CentreRadius;
		double High = CentreRadius;
		const FVector Origin = Centre + Up * CentreRadius;
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / 8.0;
			const double Radius = RadiusAt(Root, (Origin + (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)) * Footprint - Centre).GetSafeNormal());
			Low = FMath::Min(Low, Radius);
			High = FMath::Max(High, Radius);
		}
		const double Slope = (High - Low) / FMath::Max(2.0 * Footprint, 1.0);
		const bool bDry = SeaRadius <= 0.0 || Low > SeaRadius + 300.0;
		OutScore = Slope + (bDry ? 0.0 : 10.0);
		return bDry && Slope <= MaxSlope;
	}

	/** A body-local direction Distance along the surface from Centre at Azimuth (the frame is the body's own: stable). */
	FVector Offset(const FVector& CentreLocal, const double BodyRadius, const double Distance, const double Azimuth)
	{
		FVector TangentA;
		FVector TangentB;
		CentreLocal.FindBestAxisVectors(TangentA, TangentB);
		const double Angle = Distance / FMath::Max(BodyRadius, 1.0);
		return (CentreLocal * FMath::Cos(Angle) + (TangentA * FMath::Cos(Azimuth) + TangentB * FMath::Sin(Azimuth)) * FMath::Sin(Angle))
			.GetSafeNormal();
	}

	uint32 SeedOf(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		return Gameplay && Gameplay->NewGeneratedWorld ? static_cast<uint32>(Gameplay->NewGeneratedWorld->GenerationSeed) : 1u;
	}

	void SetInGameName(AActor* Actor, const FText& Name)
	{
		if (FTextProperty* NameProperty = Actor ? FindFProperty<FTextProperty>(Actor->GetClass(), TEXT("InGameName")) : nullptr)
		{
			NameProperty->SetPropertyValue_InContainer(Actor, Name);
		}
	}

	UMaterialInstanceDynamic* Material(AActor& Owner, const FLinearColor& Colour, const float Glow)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, Glow > 0.0f
			? TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")
			: TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Parent)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Parent, &Owner);
		Instance->SetVectorParameterValue(TEXT("Color"), Glow > 0.0f ? Colour * Glow : Colour);
		return Instance;
	}

	/** The ground of a find in its own frame (X forward, Y right, Z up from the ground at its middle), from WorldScape itself. */
	struct FGround
	{
		AWorldScapeRoot* Root{nullptr};
		FVector Origin{FVector::ZeroVector};
		FVector Forward{FVector::ForwardVector};
		FVector Right{FVector::RightVector};
		FVector Up{FVector::UpVector};

		double Z(const double X, const double Y) const
		{
			const FVector BodyCentre = Root->GetActorLocation();
			const FVector Direction = (Origin + Forward * X + Right * Y - BodyCentre).GetSafeNormal();
			return FVector::DotProduct(BodyCentre + Direction * RadiusAt(*Root, Direction) - Origin, Up);
		}
	};

	UProceduralMeshComponent* AddMesh(AActor& Owner, USceneComponent& Root, const TCHAR* Name, const FAPSAncientMesh& Data,
		UMaterialInterface* Mat, const bool bCollision, const bool bShadow, const double CullCm)
	{
		if (Data.IsEmpty())
		{
			return nullptr;
		}
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(&Owner, Name);
		Mesh->SetupAttachment(&Root);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bUseAsyncCooking = true;
		Mesh->bUseComplexAsSimpleCollision = true;
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetGenerateOverlapEvents(false);
		if (bCollision)
		{
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Mesh->SetCastShadow(bShadow);
		Mesh->bCastFarShadow = false;
		Mesh->bAffectDistanceFieldLighting = false;
		Mesh->SetCullDistance(static_cast<float>(CullCm));
		Mesh->RegisterComponent();
		Owner.AddInstanceComponent(Mesh);
		Mesh->CreateMeshSection(0, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, TArray<FColor>(), Data.Tangents, bCollision);
		if (Mat)
		{
			Mesh->SetMaterial(0, Mat);
		}
		return Mesh;
	}

	/**
	 * A cooked mesh of the project laid on the find's ground: LengthCm its longest side, Bury the part of its height under
	 * the ground, Rotation its own turn and tilt in the find's frame.
	 */
	void AddStatic(AActor& Owner, USceneComponent& Root, const FGround& Ground, const TCHAR* Path, const double X, const double Y,
		const double LengthCm, const FRotator& Rotation, const double Bury, const bool bShadow, const double CullCm)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Finds] mesh %s does not load; that part of the find stays out"), Path);
			return;
		}
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const double Longest = FMath::Max(2.0 * Bounds.BoxExtent.GetMax(), 1.0);
		const double Scale = LengthCm / Longest;
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(&Owner);
		Component->SetupAttachment(&Root);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Mesh);
		Component->SetCanEverAffectNavigation(false);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCastShadow(bShadow);
		Component->bCastFarShadow = false;
		Component->bAffectDistanceFieldLighting = false;
		Component->SetCullDistance(static_cast<float>(CullCm));
		const FQuat Turn = Rotation.Quaternion();
		// The bounds' lowest point in the find's frame after the turn, so the part sits on (and Bury into) its ground.
		FVector Corners[8];
		FBox(Bounds.Origin - Bounds.BoxExtent, Bounds.Origin + Bounds.BoxExtent).GetVertices(Corners);
		double Lowest = TNumericLimits<double>::Max();
		FVector Middle = FVector::ZeroVector;
		for (const FVector& Corner : Corners)
		{
			const FVector Turned = Turn.RotateVector(Corner * Scale);
			Lowest = FMath::Min(Lowest, Turned.Z);
			Middle += Turned / 8.0;
		}
		const double Height = 2.0 * Bounds.BoxExtent.Z * Scale;
		const double GroundZ = Ground.Z(X, Y);
		Component->SetRelativeLocationAndRotation(FVector(X - Middle.X, Y - Middle.Y, GroundZ - Lowest - Bury * Height), Turn);
		Component->SetRelativeScale3D(FVector(Scale));
		Component->RegisterComponent();
		Owner.AddInstanceComponent(Component);
	}

	/** A lamp on a mast: the mast in the stone, the lamp's ball in the glow (it stays visible at night from kilometres). */
	void AddMast(FAPSAncientMesh& Stone, FAPSAncientMesh& Glow, const FGround& Ground, const double X, const double Y, const double Height)
	{
		const double Base = Ground.Z(X, Y);
		Stone.Box(FVector(X, Y, Base + Height * 0.5), FVector(9.0, 9.0, Height * 0.5));
		Glow.Frustum(FVector(X, Y, Base + Height + 30.0), FQuat::Identity, 32.0, 32.0, 30.0, 10, true);
	}

	UPointLightComponent* AddLamp(AActor& Owner, USceneComponent& Root, const FVector& Where, const FLinearColor& Colour, const float Candelas,
		const float RadiusCm)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(&Owner);
		Light->SetupAttachment(&Root);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetRelativeLocation(Where);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Candelas);
		Light->SetLightColor(Colour);
		Light->SetAttenuationRadius(RadiusCm);
		// The rule for every spawned lamp: no shadows, a draw distance with a fade (no blink at its edge).
		Light->SetCastShadows(false);
		Light->MaxDrawDistance = 200000.0f;
		Light->MaxDistanceFadeRange = 40000.0f;
		Light->RegisterComponent();
		Owner.AddInstanceComponent(Light);
		return Light;
	}

	/** A rock half sunk in the ground: a faceted frustum on a tilted axis (thrown-up ground, a crater's rim, a furrow's ridge). */
	void AddRubble(FAPSAncientMesh& Stone, const FGround& Ground, const FRandomStream& Stream, const double X, const double Y, const double Size)
	{
		const FVector Axis = FVector(Stream.FRandRange(-0.5, 0.5), Stream.FRandRange(-0.5, 0.5), 1.0).GetSafeNormal();
		Stone.Frustum(FVector(X, Y, Ground.Z(X, Y) + Size * 0.12), FQuat::FindBetweenNormals(FVector::UpVector, Axis)
			* FQuat(FVector::UpVector, Stream.FRandRange(0.0, UE_TWO_PI)), Size, Size * Stream.FRandRange(0.35, 0.6),
			Size * Stream.FRandRange(0.3, 0.5), Stream.RandRange(5, 7));
	}

	/** A plate torn off a hull, stuck in the ground at an angle. */
	void AddPlate(FAPSAncientMesh& Metal, const FGround& Ground, const FRandomStream& Stream, const double X, const double Y)
	{
		const FQuat Turn = FQuat(FVector::UpVector, Stream.FRandRange(0.0, UE_TWO_PI))
			* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Stream.FRandRange(20.0, 65.0)));
		Metal.Box(FVector(X, Y, Ground.Z(X, Y) + 12.0), FVector(Stream.FRandRange(35.0, 80.0), Stream.FRandRange(25.0, 50.0), 2.5), Turn);
	}

	/**
	 * A find's scene in its own frame. Lamps are a few tens of candelas: the project's sun is a few lux, and the first run
	 * (19.10, 1,500 cd) lit the basalt tan and the rocks pink in full daylight; at night the exposure opens and they pool.
	 * Glow (the lamp's ball, crystal tips, vent throats and seams) is crisp emissive geometry, never a blurred sprite.
	 */
	void BuildScene(AActor& Actor, USceneComponent& Root, const FGround& Ground, const EKind Kind, const bool bAdrift, const uint32 Seed,
		FPlaced& Placed)
	{
		const FRandomStream Stream(static_cast<int32>(Seed));
		FAPSAncientMesh Stone;
		FAPSAncientMesh Metal;
		FAPSAncientMesh Crystal;
		FAPSAncientMesh Glow;
		FAPSAncientMesh Seam;
		const FLinearColor Lamp = Defs[static_cast<int32>(Kind)].Lamp;
		FVector LampAt = FVector(0.0, 0.0, Ground.Z(0.0, 0.0) + 300.0);
		float LampCandelas = 60.0f;
		float LampRadius = 1800.0f;
		float GlowStrength = 12.0f;
		switch (Kind)
		{
		case EKind::Lander:
		{
			// The furrow it ploughed: two ridges of thrown-up ground and the plates it shed, rock round the hull.
			for (int32 Index = 0; Index < 11; ++Index)
			{
				const double X = -650.0 - Index * 300.0;
				for (const double Side : {-1.0, 1.0})
				{
					AddRubble(Stone, Ground, Stream, X + Stream.FRandRange(-90.0, 90.0), Side * Stream.FRandRange(170.0, 260.0) * (1.0 - Index * 0.04),
						Stream.FRandRange(25.0, 70.0) * (1.0 - Index * 0.05));
				}
			}
			for (int32 Index = 0; Index < 6; ++Index)
			{
				AddPlate(Metal, Ground, Stream, -900.0 - Index * 520.0 + Stream.FRandRange(-150.0, 150.0), Stream.FRandRange(-150.0, 150.0));
			}
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const double Angle = Stream.FRandRange(0.0, UE_TWO_PI);
				const double Distance = Stream.FRandRange(650.0, 1150.0);
				AddRubble(Stone, Ground, Stream, FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Stream.FRandRange(50.0, 120.0));
			}
			AddStatic(Actor, Root, Ground, bAdrift ? TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Body.SM_Offroad_Body")
				: TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_XXS_P1_09/SM_Spaceship_XXS_P1_09.SM_Spaceship_XXS_P1_09"),
				0.0, 0.0, bAdrift ? 520.0 : 1400.0, FRotator(-12.0, 20.0, 16.0), 0.22, true, DrawCm);
			if (bAdrift)
			{
				// The expedition before ours drove here: their rover, a wheel thrown off, and their lander beside it.
				AddStatic(Actor, Root, Ground, TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_XXS_P1_09/SM_Spaceship_XXS_P1_09.SM_Spaceship_XXS_P1_09"),
					1700.0, -600.0, 1400.0, FRotator(-6.0, 140.0, -9.0), 0.15, true, DrawCm);
				AddStatic(Actor, Root, Ground, TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Tire.SM_Offroad_Tire"), 600.0, 450.0, 110.0,
					FRotator(0.0, 30.0, 80.0), 0.1, false, SmallDrawCm);
			}
			AddStatic(Actor, Root, Ground, TEXT("/Game/APS_PREA/APS_RELEASE/V_EA/MESH_PARTS/SM_Gunship_v3_EngineLeft.SM_Gunship_v3_EngineLeft"),
				900.0, 700.0, 350.0, FRotator(25.0, 70.0, 80.0), 0.15, true, SmallDrawCm * 2.0);
			AddStatic(Actor, Root, Ground, TEXT("/Game/APS_PREA/APS_RELEASE/V_EA/MESH_PARTS/SM_Gunship_v3_EngineRight.SM_Gunship_v3_EngineRight"),
				-1200.0, -900.0, 350.0, FRotator(-30.0, 200.0, 10.0), 0.2, true, SmallDrawCm * 2.0);
			AddStatic(Actor, Root, Ground, TEXT("/Game/APS_PREA/APS_APOSFERA_SPACETRIPS/Meshes/SM_MERGED_SHIP_LANDGEARS_LEFT.SM_MERGED_SHIP_LANDGEARS_LEFT"),
				500.0, -900.0, 300.0, FRotator(60.0, 35.0, 0.0), 0.1, false, SmallDrawCm);
			const TCHAR* Crates[] = {TEXT("SM_CargoCrate_M_Orange"), TEXT("SM_CargoCrate_M_Grey"), TEXT("SM_CargoCrate_L_Grey"), TEXT("SM_CargoCrate_H_Blue")};
			const FVector2D CratePlaces[] = {{1500.0, -300.0}, {1800.0, 400.0}, {-600.0, 1300.0}, {2300.0, 1200.0}};
			for (int32 Index = 0; Index < 4; ++Index)
			{
				AddStatic(Actor, Root, Ground, *FString::Printf(TEXT("/Game/APS/APS_ALPHA/Assets/Props/CargoCrates/Meshes/%s.%s"), Crates[Index], Crates[Index]),
					CratePlaces[Index].X, CratePlaces[Index].Y, Index == 2 ? 240.0 : 120.0,
					FRotator(Stream.FRandRange(-20.0, 20.0), Stream.FRandRange(0.0, 360.0), Stream.FRandRange(-25.0, 25.0)), 0.1, Index == 2, SmallDrawCm);
			}
			AddMast(Metal, Glow, Ground, 300.0, -1500.0, 600.0);
			LampAt = FVector(300.0, -1500.0, Ground.Z(300.0, -1500.0) + 640.0);
			LampCandelas = 80.0f;
			break;
		}
		case EKind::Pod:
		{
			// The pod itself on its side, its heat shield black, the hatch thrown open; a rim of thrown ground; the crates round it.
			const double Yaw = Stream.FRandRange(0.0, UE_TWO_PI);
			const FQuat Lying = FQuat(FVector::UpVector, Yaw) * FQuat(FVector::RightVector, FMath::DegreesToRadians(78.0));
			const FVector Axis = Lying.RotateVector(FVector::UpVector);
			const FVector Middle(0.0, 0.0, Ground.Z(0.0, 0.0) + 105.0);
			Metal.Frustum(Middle, Lying, 170.0, 95.0, 210.0, 16, true);
			Stone.Frustum(Middle - Axis * 222.0, Lying, 182.0, 182.0, 12.0, 16, true);
			const FVector Side = FQuat(FVector::UpVector, Yaw).RotateVector(FVector::RightVector);
			Metal.Box(Middle + Axis * 90.0 + Side * 190.0 + FVector(0.0, 0.0, 40.0), FVector(80.0, 4.0, 60.0),
				FQuat(FVector::UpVector, Yaw) * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-35.0)));
			Glow.Box(Middle + Axis * 150.0 + FVector(0.0, 0.0, 128.0), FVector(30.0, 3.0, 3.0), FQuat(FVector::UpVector, Yaw));
			for (int32 Index = 0; Index < 18; ++Index)
			{
				const double Angle = UE_TWO_PI * Index / 18.0 + Stream.FRandRange(-0.12, 0.12);
				const double Distance = Stream.FRandRange(650.0, 900.0);
				AddRubble(Stone, Ground, Stream, FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Stream.FRandRange(30.0, 75.0));
			}
			struct FCrate { const TCHAR* Name; double X; double Y; double Length; FRotator Turn; double Bury; };
			const FCrate Crates[] = {
				{TEXT("SM_CargoCrate_L_Grey"), 450.0, -380.0, 240.0, FRotator(18.0, 15.0, 22.0), 0.3},
				{TEXT("SM_CargoCrate_L_Red"), 520.0, 420.0, 240.0, FRotator(0.0, 40.0, 8.0), 0.05},
				{TEXT("SM_CargoCrate_M_Orange"), -480.0, 300.0, 120.0, FRotator(0.0, 70.0, 0.0), 0.0},
				{TEXT("SM_CargoCrate_M_Grey"), -300.0, -600.0, 120.0, FRotator(40.0, 10.0, 0.0), 0.1},
				{TEXT("SM_CargoCrate_H_Blue"), 950.0, -200.0, 240.0, FRotator(0.0, 110.0, 0.0), 0.0},
				{TEXT("SM_CargoCrate_M_Orange"), 1050.0, 520.0, 120.0, FRotator(0.0, 25.0, 88.0), 0.0}};
			for (const FCrate& Crate : Crates)
			{
				AddStatic(Actor, Root, Ground, *FString::Printf(TEXT("/Game/APS/APS_ALPHA/Assets/Props/CargoCrates/Meshes/%s.%s"), Crate.Name, Crate.Name),
					Crate.X, Crate.Y, Crate.Length, Crate.Turn, Crate.Bury, Crate.Length > 200.0, Crate.Length > 200.0 ? DrawCm : SmallDrawCm);
			}
			AddMast(Metal, Glow, Ground, -750.0, -750.0, 500.0);
			LampAt = FVector(-750.0, -750.0, Ground.Z(-750.0, -750.0) + 540.0);
			LampCandelas = 60.0f;
			break;
		}
		case EKind::Crystals:
		{
			// Dark rock, and the crystal growing out of it in three clusters, tilted out from each heart: lit faceted
			// columns (they read as crystal by day) whose tips glow (they read from afar at night).
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const double X = Stream.FRandRange(-700.0, 700.0);
				const double Y = Stream.FRandRange(-700.0, 700.0);
				Stone.Frustum(FVector(X, Y, Ground.Z(X, Y) + 40.0), FQuat(FVector::UpVector, Stream.FRandRange(0.0, UE_TWO_PI)),
					Stream.FRandRange(150.0, 350.0), Stream.FRandRange(60.0, 140.0), Stream.FRandRange(30.0, 80.0), 7);
			}
			for (int32 Cluster = 0; Cluster < 3; ++Cluster)
			{
				const double CX = Stream.FRandRange(-500.0, 500.0);
				const double CY = Stream.FRandRange(-500.0, 500.0);
				const int32 Count = Stream.RandRange(6, 10);
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const double Angle = Stream.FRandRange(0.0, UE_TWO_PI);
					const double Spread = Stream.FRandRange(0.0, 220.0);
					const double X = CX + FMath::Cos(Angle) * Spread;
					const double Y = CY + FMath::Sin(Angle) * Spread;
					const double Length = Stream.FRandRange(80.0, 460.0) * (Spread < 80.0 ? 1.4 : 1.0);
					const double Radius = Length * Stream.FRandRange(0.06, 0.1);
					const FVector Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
					const FVector Axis = (FVector::UpVector + Outward * FMath::Tan(FMath::DegreesToRadians(Stream.FRandRange(8.0, 35.0)))).GetSafeNormal();
					const FQuat Tilt = FQuat::FindBetweenNormals(FVector::UpVector, Axis) * FQuat(FVector::UpVector, Stream.FRandRange(0.0, UE_TWO_PI));
					const FVector Base = FVector(X, Y, Ground.Z(X, Y) - 30.0);
					Crystal.Frustum(Base + Axis * (Length * 0.375), Tilt, Radius, Radius * 0.75, Length * 0.375, 6);
					Glow.Frustum(Base + Axis * (Length * 0.875), Tilt, Radius * 0.75, Radius * 0.06, Length * 0.125, 6);
				}
			}
			LampAt = FVector(0.0, 0.0, Ground.Z(0.0, 0.0) + 250.0);
			LampCandelas = 40.0f;
			LampRadius = 1400.0f;
			GlowStrength = 1.2f;
			break;
		}
		case EKind::Vents:
		default:
		{
			// Low basalt cones with glowing throats, and thin glowing seams running out from them through the ground.
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const double Angle = UE_TWO_PI * Index / 4.0 + Stream.FRandRange(-0.4, 0.4);
				const double Distance = Index == 0 ? 0.0 : Stream.FRandRange(650.0, 1100.0);
				const double X = FMath::Cos(Angle) * Distance;
				const double Y = FMath::Sin(Angle) * Distance;
				const double Bottom = Stream.FRandRange(300.0, 520.0) * (Index == 0 ? 1.3 : 1.0);
				const double Top = Bottom * Stream.FRandRange(0.18, 0.25);
				const double HalfHeight = Bottom * Stream.FRandRange(0.18, 0.26);
				const double BaseZ = Ground.Z(X, Y) - 30.0;
				Stone.Frustum(FVector(X, Y, BaseZ + HalfHeight), FQuat::Identity, Bottom, Top, HalfHeight, 16, true);
				Glow.Frustum(FVector(X, Y, BaseZ + 2.0 * HalfHeight + 3.0), FQuat::Identity, Top * 0.85, Top * 0.85, 4.0, 16, true);
				for (int32 Rock = 0; Rock < 3; ++Rock)
				{
					const double RockAngle = Stream.FRandRange(0.0, UE_TWO_PI);
					const double RockDistance = Bottom * Stream.FRandRange(1.05, 1.5);
					AddRubble(Stone, Ground, Stream, X + FMath::Cos(RockAngle) * RockDistance, Y + FMath::Sin(RockAngle) * RockDistance,
						Stream.FRandRange(30.0, 80.0));
				}
				// Two or three seams from the cone's foot, each a chain of short segments laid on the ground.
				const int32 Seams = Stream.RandRange(2, 3);
				for (int32 SeamIndex = 0; SeamIndex < Seams; ++SeamIndex)
				{
					double Heading = Stream.FRandRange(0.0, UE_TWO_PI);
					FVector2D At(X + FMath::Cos(Heading) * Bottom * 0.95, Y + FMath::Sin(Heading) * Bottom * 0.95);
					const int32 Segments = Stream.RandRange(4, 8);
					for (int32 Segment = 0; Segment < Segments; ++Segment)
					{
						const double Length = Stream.FRandRange(60.0, 120.0);
						const FVector2D Step(FMath::Cos(Heading) * Length, FMath::Sin(Heading) * Length);
						const FVector2D Mid = At + Step * 0.5;
						Seam.Box(FVector(Mid.X, Mid.Y, Ground.Z(Mid.X, Mid.Y) + 1.0), FVector(Length * 0.5 + 2.0, 2.5, 1.5),
							FQuat(FVector::UpVector, Heading));
						At += Step;
						Heading += Stream.FRandRange(-0.5, 0.5);
					}
				}
			}
			LampAt = FVector(0.0, 0.0, Ground.Z(0.0, 0.0) + 300.0);
			LampCandelas = 100.0f;
			LampRadius = 2200.0f;
			GlowStrength = 4.0f;
			break;
		}
		}
		// Run 2 (19.10): 0.035 read light grey in this sun, lighter than the ground; the rock and basalt go near black.
		const FLinearColor StoneColour = Kind == EKind::Vents ? FLinearColor(0.007f, 0.0065f, 0.006f) : FLinearColor(0.014f, 0.013f, 0.012f);
		AddMesh(Actor, Root, TEXT("FindStone"), Stone, Material(Actor, StoneColour, 0.0f), true, true, DrawCm);
		AddMesh(Actor, Root, TEXT("FindMetal"), Metal, Metal.IsEmpty() ? nullptr : Material(Actor, FLinearColor(0.07f, 0.07f, 0.075f), 0.0f),
			true, true, DrawCm);
		AddMesh(Actor, Root, TEXT("FindCrystal"), Crystal, Crystal.IsEmpty() ? nullptr : Material(Actor, FLinearColor(0.03f, 0.25f, 0.36f), 0.0f),
			true, true, DrawCm);
		AddMesh(Actor, Root, TEXT("FindSeam"), Seam, Seam.IsEmpty() ? nullptr : Material(Actor, Lamp, 1.5f), false, false, SmallDrawCm);
		UMaterialInstanceDynamic* GlowMaterial = Material(Actor, Lamp, GlowStrength);
		AddMesh(Actor, Root, TEXT("FindGlow"), Glow, GlowMaterial, false, false, DrawCm);
		Placed.LampGlow = GlowMaterial;
		Placed.Lamp = AddLamp(Actor, Root, LampAt, Lamp, LampCandelas, LampRadius);
	}

	void BuildWaymark(AActor& Actor, USceneComponent& Root, const FGround& Ground, const FVector& TowardLocal, const uint32 Seed)
	{
		const FRandomStream Stream(static_cast<int32>(Seed));
		FAPSAncientMesh Stone;
		FAPSAncientMesh Glow;
		// A slab leaning toward the monument, glyph strips on its face, a cairn of small stones at its foot.
		const FVector Toward = FVector(TowardLocal.X, TowardLocal.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FQuat Face(FVector::UpVector, FMath::Atan2(Toward.Y, Toward.X));
		const FQuat Lean = FQuat(Face.RotateVector(FVector::RightVector), FMath::DegreesToRadians(9.0)) * Face;
		const double Base = Ground.Z(0.0, 0.0) - 60.0;
		Stone.Box(FVector(0.0, 0.0, Base + 270.0), FVector(70.0, 25.0, 300.0), Lean);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector Strip = Lean.RotateVector(FVector(Stream.FRandRange(-30.0, 30.0), -26.0, -150.0 + Index * 70.0));
			Glow.Box(FVector(0.0, 0.0, Base + 270.0) + Strip, FVector(Stream.FRandRange(15.0, 35.0), 1.5, 3.0), Lean);
		}
		Glow.Box(FVector(0.0, 0.0, Base + 270.0) + Lean.RotateVector(FVector(0.0, 0.0, 285.0)), FVector(72.0, 27.0, 6.0), Lean);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const double Angle = Stream.FRandRange(0.0, UE_TWO_PI);
			const double X = FMath::Cos(Angle) * Stream.FRandRange(120.0, 220.0);
			const double Y = FMath::Sin(Angle) * Stream.FRandRange(120.0, 220.0);
			Stone.Box(FVector(X, Y, Ground.Z(X, Y) + 15.0), FVector(Stream.FRandRange(20.0, 45.0), Stream.FRandRange(20.0, 40.0), Stream.FRandRange(15.0, 35.0)),
				FQuat(FVector::UpVector, Stream.FRandRange(0.0, UE_TWO_PI)));
		}
		AddMesh(Actor, Root, TEXT("WaymarkStone"), Stone, Material(Actor, FLinearColor(0.03f, 0.03f, 0.035f), 0.0f), true, true, DrawCm);
		AddMesh(Actor, Root, TEXT("WaymarkGlow"), Glow, Material(Actor, FLinearColor(0.25f, 0.9f, 1.0f), 8.0f), false, false, DrawCm);
	}

	/** The find's actor: its root on the ground at the place, its frame (forward by Yaw), attached to the body, charted by the rover. */
	AActor* SpawnSite(UWorld& World, APlanetaryBody& Body, AWorldScapeRoot& Root, const FVector& LocalUp, const double Yaw,
		const FName Tag, FGround& OutGround, USceneComponent*& OutRoot)
	{
		const FVector Up = Body.GetActorTransform().TransformVectorNoScale(LocalUp).GetSafeNormal();
		FVector North = FVector::VectorPlaneProject(Body.GetActorUpVector(), Up).GetSafeNormal();
		if (North.IsNearlyZero())
		{
			North = FVector::VectorPlaneProject(Body.GetActorForwardVector(), Up).GetSafeNormal();
		}
		const FVector Forward = FQuat(Up, Yaw).RotateVector(North).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
		const FVector Origin = Root.GetActorLocation() + Up * RadiusAt(Root, Up);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATechInfrastructure* Actor = World.SpawnActor<ATechInfrastructure>(ATechInfrastructure::StaticClass(), Origin,
			FRotationMatrix::MakeFromXZ(Forward, Up).Rotator(), Params);
		if (!IsValid(Actor))
		{
			return nullptr;
		}
		USceneComponent* SceneRoot = NewObject<USceneComponent>(Actor, TEXT("FindRoot"));
		SceneRoot->SetMobility(EComponentMobility::Movable);
		Actor->SetRootComponent(SceneRoot);
		SceneRoot->RegisterComponent();
		Actor->SetActorLocationAndRotation(Origin, FRotationMatrix::MakeFromXZ(Forward, Up).Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		Actor->SetActorTickEnabled(false);
		Actor->Tags.AddUnique(TEXT("APS.Story.Site"));
		Actor->Tags.AddUnique(TEXT("APS.Story.Find"));
		Actor->Tags.AddUnique(Tag);
		OutGround.Root = &Root;
		OutGround.Origin = Origin;
		OutGround.Forward = Forward;
		OutGround.Right = Right;
		OutGround.Up = Up;
		OutRoot = SceneRoot;
		return Actor;
	}

	bool IsKnown(const UWorld* World, const bool bAdrift)
	{
		const FAPSMissionBoard* Board = APSMissionsFind(World);
		if (!Board)
		{
			return false;
		}
		// ARK: the deep radar on the comms mast; ADRIFT: the ship down (the colony's site at the landing).
		return Board->IsUnlocked(bAdrift ? FName(TEXT("ORIGIN_ADRIFT_HOMED")) : FName(TEXT("ORIGIN_Ark_Ears")));
	}

	/** The walker's view: the found finds marked with their name and distance (only on foot; the rover's navigation charts them). */
	class SFindMarkers final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFindMarkers) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&, UWorld* InWorld)
		{
			World = InWorld;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Elements,
			int32 Layer, const FWidgetStyle&, bool) const override
		{
			UWorld* Live = World.Get();
			const FState* State = Live ? Find(Live) : nullptr;
			APlayerController* Controller = Live ? Live->GetFirstPlayerController() : nullptr;
			const ACustomGravityCharacter* Walker = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
			if (!State || !Walker || CVarMarkers.GetValueOnGameThread() == 0)
			{
				return Layer;
			}
			if (const AGravityPlayerController* Gravity = Cast<AGravityPlayerController>(Controller); Gravity && Gravity->IsStrategicMapOpen())
			{
				return Layer;
			}
			if (const UAPSColonyTerminalSubsystem* Terminal = Live->GetSubsystem<UAPSColonyTerminalSubsystem>(); Terminal && Terminal->IsTerminalOpen())
			{
				return Layer;
			}
			const bool bKnown = IsKnown(Live, State->bAdrift);
			const FAPSMissionBoard* Board = APSMissionsFind(Live);
			const FSlateFontInfo Font = APSHud::ValueFont(12);
			const FSlateBrush* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FAPSUIThemePalette& Palette = APSUITheme::Palette();
			const float Scale = FMath::Max(Geometry.Scale, 0.01f);
			for (int32 Index = 0; Index < FindCount; ++Index)
			{
				const FPlaced& Placed = State->Finds[Index];
				const AActor* Actor = Placed.Actor.Get();
				if (!Actor || !(bKnown || Placed.bSeen) || (Board && Board->IsUnlocked(FindToken(Index, State->bAdrift))))
				{
					continue;
				}
				// Within reach the F prompt names it; a marker over the walker's head would only hide the find.
				if (FVector::Dist(Walker->GetActorLocation(), Actor->GetActorLocation()) < 6000.0)
				{
					continue;
				}
				const FVector Where = Actor->GetActorLocation() + Actor->GetActorUpVector() * 900.0;
				FVector2D Screen;
				if (!Controller->ProjectWorldLocationToScreen(Where, Screen, false))
				{
					continue;
				}
				const FVector2D Local = Screen / Scale;
				if (Local.X < 0.0 || Local.Y < 0.0 || Local.X > Geometry.GetLocalSize().X || Local.Y > Geometry.GetLocalSize().Y)
				{
					continue;
				}
				const double Km = FVector::Dist(Walker->GetActorLocation(), Actor->GetActorLocation()) / 100000.0;
				const FString Label = FString::Printf(TEXT("%s  %s"), *NameOf(Defs[Index].Kind, State->bAdrift).ToString(),
					Km >= 1.0 ? *FString::Printf(TEXT("%.1f km"), Km) : *FString::Printf(TEXT("%.0f m"), Km * 1000.0));
				const FLinearColor Colour = Defs[Index].Lamp * 0.35f + Palette.Text * 0.65f;
				FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(10.0f, 10.0f),
					FSlateLayoutTransform(FVector2f(static_cast<float>(Local.X) - 5.0f, static_cast<float>(Local.Y) - 5.0f))), Brush,
					ESlateDrawEffect::None, Colour);
				FSlateDrawElement::MakeText(Elements, Layer + 1, Geometry.ToPaintGeometry(FVector2f(400.0f, 20.0f),
					FSlateLayoutTransform(FVector2f(static_cast<float>(Local.X) + 10.0f, static_cast<float>(Local.Y) - 9.0f))), Label, Font,
					ESlateDrawEffect::None, Colour);
			}
			return Layer + 1;
		}

	private:
		TWeakObjectPtr<UWorld> World;
	};

	void EnsureMarkers(FState& State)
	{
		if (State.Markers.IsValid() || !GEngine || !GEngine->GameViewport || !State.World.IsValid())
		{
			return;
		}
		State.Markers = SNew(SFindMarkers, State.World.Get());
		GEngine->GameViewport->AddViewportWidgetContent(State.Markers.ToSharedRef(), 30);
	}

	/** Where the finds gather: the colony's site anchor (ARK beside the ark; ADRIFT where the pilot first stood). */
	bool ResolveCentre(UWorld* World, FState& State, APlanetaryBody& Home)
	{
		if (State.bAdrift)
		{
			const FAPSMissionBoard* Board = APSMissionsFind(World);
			if (!Board || !Board->IsUnlocked(FName(TEXT("ORIGIN_ADRIFT_HOMED"))))
			{
				return false;
			}
		}
		else if (!APSOriginArk::FindArk(World))
		{
			return false;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			bool bOnHome = false;
			for (const AActor* Parent = IsValid(*It) ? It->GetAttachParentActor() : nullptr; Parent && !bOnHome; Parent = Parent->GetAttachParentActor())
			{
				bOnHome = Parent == &Home;
			}
			if (bOnHome && It->ActorHasTag(TEXT("APS.Placeholder.LegacyColony")))
			{
				State.CentreLocal = Home.GetActorTransform().InverseTransformVectorNoScale(
					(It->GetActorLocation() - Home.GetActorLocation()).GetSafeNormal());
				State.Body = &Home;
				State.bCentre = !State.CentreLocal.IsNearlyZero();
				return State.bCentre;
			}
		}
		return false;
	}

	void ResolveFinds(UWorld* World, FState& State, AWorldScapeRoot& Root)
	{
		APlanetaryBody* Body = State.Body.Get();
		const double Radius = Root.PlanetScale;
		const double Sea = SeaRadiusOf(*Body, Root);
		const uint32 Seed = SeedOf(World) * 2246822519u + 101u;
		for (int32 Index = 0; Index < FindCount; ++Index)
		{
			FPlaced& Placed = State.Finds[Index];
			if (Placed.bResolved)
			{
				continue;
			}
			const FRandomStream Stream(static_cast<int32>(Seed + static_cast<uint32>(Index) * 7919u));
			const double Sector = UE_TWO_PI * Index / FindCount;
			double BestScore = TNumericLimits<double>::Max();
			FVector Best = FVector::ZeroVector;
			for (int32 Try = 0; Try < 20; ++Try)
			{
				const double Azimuth = Sector + Stream.FRandRange(-0.6, 0.6);
				const double Distance = FMath::Lerp(Defs[Index].MinCm, Defs[Index].MaxCm, static_cast<double>(Stream.FRand()));
				const FVector Local = Offset(State.CentreLocal, Radius, Distance, Azimuth);
				const FVector Up = Body->GetActorTransform().TransformVectorNoScale(Local);
				double Score = 0.0;
				if (Fits(Root, Sea, Up, FootprintCm, Score))
				{
					Best = Local;
					BestScore = -1.0;
					break;
				}
				if (Score < BestScore)
				{
					BestScore = Score;
					Best = Local;
				}
			}
			Placed.bResolved = true;
			// A find under the sea would only confuse: a world without dry gentle ground there keeps three.
			Placed.bSkipped = BestScore >= 10.0;
			Placed.LocalUp = Best;
			Placed.Yaw = Stream.FRandRange(0.0, UE_TWO_PI);
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] %s %s (score %.3f)"), Defs[Index].Id, Placed.bSkipped ? TEXT("has no dry place: left out") : TEXT("placed"),
				BestScore);
		}
	}

	void ResolveWaymarks(UWorld* World, FState& State, AWorldScapeRoot& Root)
	{
		const AActor* Monument = APSOriginArk::FindMonument(World);
		APlanetaryBody* Body = State.Body.Get();
		if (!Monument || !Body)
		{
			return;
		}
		const FVector To = Body->GetActorTransform().InverseTransformVectorNoScale((Monument->GetActorLocation() - Body->GetActorLocation()).GetSafeNormal());
		const double Sea = SeaRadiusOf(*Body, Root);
		for (int32 Index = 0; Index < WaymarkCount; ++Index)
		{
			FPlaced& Placed = State.Waymarks[Index];
			// A quarter, a half and three quarters of the way, on the great circle from the colony's site to the monument.
			const double Fraction = (Index + 1) / 4.0;
			const FVector Local = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(State.CentreLocal, To), Fraction)
				.RotateVector(State.CentreLocal).GetSafeNormal();
			double Score = 0.0;
			Fits(Root, Sea, Body->GetActorTransform().TransformVectorNoScale(Local), 400.0, Score);
			Placed.bResolved = true;
			Placed.bSkipped = Score >= 10.0;
			Placed.LocalUp = Local;
			Placed.Yaw = 0.0;
		}
		State.bWaymarksResolved = true;
	}

	void BuildOne(UWorld* World, FState& State, AWorldScapeRoot& Root)
	{
		APlanetaryBody* Body = State.Body.Get();
		const FAPSMissionBoard* Board = APSMissionsFind(World);
		for (int32 Index = 0; Index < FindCount; ++Index)
		{
			FPlaced& Placed = State.Finds[Index];
			if (!Placed.bResolved || Placed.bSkipped || Placed.Actor.IsValid())
			{
				continue;
			}
			FGround Ground;
			USceneComponent* SceneRoot = nullptr;
			AActor* Actor = SpawnSite(*World, *Body, Root, Placed.LocalUp, Placed.Yaw, FName(*FString::Printf(TEXT("APS.Story.Find.%s"), Defs[Index].Id)),
				Ground, SceneRoot);
			if (!Actor)
			{
				Placed.bSkipped = true;
				return;
			}
			BuildScene(*Actor, *SceneRoot, Ground, Defs[Index].Kind, State.bAdrift, SeedOf(World) + 31u * static_cast<uint32>(Index), Placed);
			Actor->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
			const bool bTaken = Board && Board->IsUnlocked(FindToken(Index, State.bAdrift));
			SetInGameName(Actor, bTaken ? FText::Format(LOCTEXT("TakenName", "{0} (TAKEN)"), NameOf(Defs[Index].Kind, State.bAdrift))
				: NameOf(Defs[Index].Kind, State.bAdrift));
			if (bTaken && Placed.Lamp.IsValid())
			{
				Placed.Lamp->SetVisibility(false);
			}
			Placed.Actor = Actor;
			Placed.bBuilt = true;
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] %s stands %.1f km from the colony"), Defs[Index].Id,
				FMath::Acos(FMath::Clamp(FVector::DotProduct(Placed.LocalUp, State.CentreLocal), -1.0, 1.0)) * Root.PlanetScale / 100000.0);
			return;
		}
		for (int32 Index = 0; Index < WaymarkCount; ++Index)
		{
			FPlaced& Placed = State.Waymarks[Index];
			if (!Placed.bResolved || Placed.bSkipped || Placed.Actor.IsValid())
			{
				continue;
			}
			FGround Ground;
			USceneComponent* SceneRoot = nullptr;
			const AActor* Monument = APSOriginArk::FindMonument(World);
			AActor* Actor = Monument ? SpawnSite(*World, *Body, Root, Placed.LocalUp, 0.0, FName(*FString::Printf(TEXT("APS.Story.Waymark.%d"), Index)),
				Ground, SceneRoot) : nullptr;
			if (!Actor)
			{
				Placed.bSkipped = true;
				return;
			}
			Actor->Tags.Remove(TEXT("APS.Story.Find"));
			Actor->Tags.AddUnique(TEXT("APS.Nav.Hidden"));
			const FVector TowardWorld = (Monument->GetActorLocation() - Actor->GetActorLocation());
			BuildWaymark(*Actor, *SceneRoot, Ground, FVector(FVector::DotProduct(TowardWorld, Ground.Forward), FVector::DotProduct(TowardWorld, Ground.Right), 0.0),
				SeedOf(World) + 977u * static_cast<uint32>(Index + 1));
			Actor->AttachToActor(Body, FAttachmentTransformRules::KeepWorldTransform);
			Placed.Actor = Actor;
			Placed.bBuilt = true;
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] waymark %d stands"), Index);
			return;
		}
	}

	FPlaced* PlacedOf(FState& State, const AActor* Actor, int32& OutIndex)
	{
		for (int32 Index = 0; Index < FindCount; ++Index)
		{
			if (Actor && State.Finds[Index].Actor.Get() == Actor)
			{
				OutIndex = Index;
				return &State.Finds[Index];
			}
		}
		OutIndex = INDEX_NONE;
		return nullptr;
	}
}

bool APSStoryFinds::IsActive(const UWorld* World)
{
	return World && APSStoryFindsLocal::CVarEnable.GetValueOnGameThread() != 0
		&& (APSOriginArk::IsArkWorld(World) || APSOriginAdrift::IsAdriftWorld(World));
}

void APSStoryFinds::Tick(UWorld* World, const float DeltaSeconds)
{
	using namespace APSStoryFindsLocal;
	if (!World || !World->IsGameWorld() || !IsActive(World))
	{
		return;
	}
	const UGameInstance* GameInstance = World->GetGameInstance();
	const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
	if (Gameplay && Gameplay->bPendingSavedWorldReplay)
	{
		return;
	}
	AAstroGenerator* Gen = nullptr;
	for (TActorIterator<AAstroGenerator> It(World); It; ++It)
	{
		if (IsValid(*It) && IsValid(It->HomePlanet))
		{
			Gen = *It;
			break;
		}
	}
	FAPSMissionBoard* Board = APSMissionsFind(World);
	if (!Gen || !Board)
	{
		return;
	}
	FState& State = StateFor(World);
	State.bAdrift = APSOriginAdrift::IsAdriftWorld(World);
	AWorldScapeRoot* Root = BuiltRoot(Gen->HomePlanet);
	if (!Root)
	{
		return;
	}
	if (!State.bCentre)
	{
		// Once a second until the colony's site stands (the ark adopted, ADRIFT's ship down): an actor walk, not each frame.
		if (World->GetTimeSeconds() < State.NextCentreTry)
		{
			return;
		}
		State.NextCentreTry = World->GetTimeSeconds() + 1.0;
		if (!ResolveCentre(World, State, *Gen->HomePlanet))
		{
			return;
		}
	}
	ResolveFinds(World, State, *Root);
	if (!State.bAdrift && !State.bWaymarksResolved && World->GetTimeSeconds() >= State.NextWaymarkTry)
	{
		State.NextWaymarkTry = World->GetTimeSeconds() + 1.0;
		ResolveWaymarks(World, State, *Root);
	}
	BuildOne(World, State, *Root);
	EnsureMarkers(State);
	// What the walker comes near: a find seen marks itself; a waymark tells its line once.
	const APlayerController* Controller = World->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	for (FPlaced& Placed : State.Finds)
	{
		if (const AActor* Actor = Placed.Actor.Get(); Actor && !Placed.bSeen && FVector::Dist(Actor->GetActorLocation(), Pawn->GetActorLocation()) < SeenCm)
		{
			Placed.bSeen = true;
		}
	}
	for (int32 Index = 0; Index < WaymarkCount; ++Index)
	{
		const AActor* Actor = State.Waymarks[Index].Actor.Get();
		if (Actor && Pawn->IsA<ACustomGravityCharacter>() && !Board->IsUnlocked(WaymarkToken(Index))
			&& FVector::Dist(Actor->GetActorLocation(), Pawn->GetActorLocation()) < 4000.0)
		{
			Board->Unlock(WaymarkToken(Index));
			UAPSCivilizationJournalSubsystem::Post(World, TEXT("Origin"), WaymarkLore(Index));
			APSMissionTracker::PushNotice(World, LOCTEXT("WaymarkKicker", "THE BUILDERS' WAY"), FText::Format(LOCTEXT("WaymarkTitle", "WAYMARK {0} OF 3"),
				FText::AsNumber(Index + 1)), WaymarkLore(Index), FText::GetEmpty(), FLinearColor(0.25f, 0.9f, 1.0f));
		}
	}
}

AActor* APSStoryFinds::Candidate(const APawn* Pawn)
{
	using namespace APSStoryFindsLocal;
	const UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	FState* State = World && IsActive(World) ? Find(World) : nullptr;
	const FAPSMissionBoard* Board = World ? APSMissionsFind(World) : nullptr;
	if (!State || !Board || !Pawn->IsA<ACustomGravityCharacter>())
	{
		return nullptr;
	}
	const double Reach = FMath::Max(CVarReachM.GetValueOnGameThread(), 1.0f) * 100.0;
	for (int32 Index = 0; Index < FindCount; ++Index)
	{
		AActor* Actor = State->Finds[Index].Actor.Get();
		if (Actor && !Board->IsUnlocked(FindToken(Index, State->bAdrift)) && FVector::Dist(Actor->GetActorLocation(), Pawn->GetActorLocation()) < Reach)
		{
			return Actor;
		}
	}
	return nullptr;
}

FText APSStoryFinds::ActionFor(const APawn* Pawn, const AActor* Candidate)
{
	using namespace APSStoryFindsLocal;
	const UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	FState* State = World ? Find(World) : nullptr;
	int32 Index = INDEX_NONE;
	if (!State || !PlacedOf(*State, Candidate, Index) || Candidate != APSStoryFinds::Candidate(Pawn))
	{
		return FText::GetEmpty();
	}
	return ActionOf(Defs[Index].Kind);
}

FText APSStoryFinds::SubjectFor(const AActor* Candidate)
{
	using namespace APSStoryFindsLocal;
	const UWorld* World = Candidate ? Candidate->GetWorld() : nullptr;
	FState* State = World ? Find(World) : nullptr;
	int32 Index = INDEX_NONE;
	return State && PlacedOf(*State, Candidate, Index) ? NameOf(Defs[Index].Kind, State->bAdrift) : FText::GetEmpty();
}

bool APSStoryFinds::TryInteract(APawn* Pawn, AActor* Candidate)
{
	using namespace APSStoryFindsLocal;
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	FState* State = World ? Find(World) : nullptr;
	FAPSMissionBoard* Board = World ? APSMissionsFind(World) : nullptr;
	int32 Index = INDEX_NONE;
	FPlaced* Placed = State ? PlacedOf(*State, Candidate, Index) : nullptr;
	if (!Placed || !Board || ActionFor(Pawn, Candidate).IsEmpty())
	{
		return false;
	}
	Board->Unlock(FindToken(Index, State->bAdrift));
	const FDef& Def = Defs[Index];
	FAPSInfrastructure* Infrastructure = APSInfrastructureFind(World);
	float Energy = Def.Energy;
	float Metals = 0.0f;
	if (Infrastructure && Def.Metals > 0.0f)
	{
		// Plate counts as metal once the metals are open; before that the cells are what the colony can use.
		if (Infrastructure->IsResourceOpen(APSInfrastructure::EResource::Metals))
		{
			Metals = Def.Metals;
		}
		else
		{
			Energy += Def.Metals * 0.25f;
		}
	}
	if (Infrastructure)
	{
		Infrastructure->AddStock(APSInfrastructure::EResource::Energy, Energy);
		Infrastructure->AddStock(APSInfrastructure::EResource::Metals, Metals);
	}
	const FText Gain = Metals > 0.0f
		? FText::Format(LOCTEXT("GainBoth", "+{0} energy, +{1} metals"), FText::AsNumber(FMath::RoundToInt(Energy)), FText::AsNumber(FMath::RoundToInt(Metals)))
		: FText::Format(LOCTEXT("GainEnergy", "+{0} energy"), FText::AsNumber(FMath::RoundToInt(Energy)));
	const FText Name = NameOf(Def.Kind, State->bAdrift);
	UAPSCivilizationJournalSubsystem::Post(World, TEXT("Origin"), FText::Format(LOCTEXT("FindJournal", "{0}: {1} ({2})"), Name,
		LoreOf(Def.Kind, State->bAdrift), Gain));
	APSMissionTracker::PushNotice(World, LOCTEXT("FindKicker", "A FIND"), Name, LoreOf(Def.Kind, State->bAdrift), Gain, Def.Lamp);
	if (Placed->Lamp.IsValid())
	{
		Placed->Lamp->SetVisibility(false);
	}
	if (Placed->LampGlow.IsValid() && Def.Kind != EKind::Crystals && Def.Kind != EKind::Vents)
	{
		Placed->LampGlow->SetVectorParameterValue(TEXT("Color"), Def.Lamp * 0.4f);
	}
	SetInGameName(Candidate, FText::Format(LOCTEXT("TakenName", "{0} (TAKEN)"), Name));
	if (UAPSAudioSubsystem* Audio = World->GetSubsystem<UAPSAudioSubsystem>())
	{
		Audio->PlayConfirm();
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Finds] %s taken: %s"), Def.Id, *Gain.ToString());
	return true;
}

FText APSStoryFinds::FindsLine(const UWorld* World)
{
	using namespace APSStoryFindsLocal;
	const FState* State = World && IsActive(World) ? Find(World) : nullptr;
	const FAPSMissionBoard* Board = World ? APSMissionsFind(World) : nullptr;
	if (!State || !Board || !IsKnown(World, State->bAdrift))
	{
		return FText::GetEmpty();
	}
	int32 Total = 0;
	int32 Taken = 0;
	for (int32 Index = 0; Index < FindCount; ++Index)
	{
		if (State->Finds[Index].bBuilt)
		{
			++Total;
			Taken += Board->IsUnlocked(FindToken(Index, State->bAdrift)) ? 1 : 0;
		}
	}
	return Total == 0 ? FText::GetEmpty() : FText::Format(LOCTEXT("FindsLine", "FINDS {0} OF {1}: markers on the horizon"),
		FText::AsNumber(Taken), FText::AsNumber(Total));
}

namespace APSStoryFindsConsole
{
	// Offscreen runs: a walk to a find (40 m short of it, facing it) and a report.
	FAutoConsoleCommandWithWorldAndArgs GoCommand(TEXT("aps.Finds.Go"),
		TEXT("Test: the walker stands 25 m from a find, facing it: aps.Finds.Go 0..3 (lander, pod, crystals, vents) | w0..w2 (waymarks)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			using namespace APSStoryFindsLocal;
			FState* State = World ? Find(World) : nullptr;
			APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			ACustomGravityCharacter* Walker = Controller ? Cast<ACustomGravityCharacter>(Controller->GetPawn()) : nullptr;
			if (!State || !Walker || Args.IsEmpty())
			{
				return;
			}
			const bool bWaymark = Args[0].StartsWith(TEXT("w"));
			const int32 Index = FCString::Atoi(*Args[0].RightChop(bWaymark ? 1 : 0));
			const AActor* Actor = bWaymark ? (State->Waymarks[FMath::Clamp(Index, 0, WaymarkCount - 1)].Actor.Get())
				: State->Finds[FMath::Clamp(Index, 0, FindCount - 1)].Actor.Get();
			AWorldScapeRoot* Root = BuiltRoot(State->Body.Get());
			if (!Actor || !Root)
			{
				UE_LOG(LogTemp, Warning, TEXT("[APS.Finds] aps.Finds.Go %s: not standing"), *Args[0]);
				return;
			}
			const FVector Up = Actor->GetActorUpVector();
			const FVector Back = -Actor->GetActorForwardVector();
			const FVector Centre = Root->GetActorLocation();
			const FVector Direction = (Actor->GetActorLocation() + Back * 2500.0 - Centre).GetSafeNormal();
			const FVector Stand = Centre + Direction * (RadiusAt(*Root, Direction) + 150.0);
			Walker->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
			// The find 18 degrees aside, beside the walker rather than behind her (as the ark's camera moments do).
			const FVector Toward = (Actor->GetActorLocation() - Stand).GetSafeNormal();
			Walker->SetViewDirection(FQuat(Up, FMath::DegreesToRadians(18.0)).RotateVector(Toward), 8.0f);
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] the walker stands 25 m from %s"), *GetNameSafe(Actor));
		}));

	// The A/B of what the finds cost: the same pose with them shown and hidden.
	FAutoConsoleCommandWithWorldAndArgs ShowCommand(TEXT("aps.Finds.Show"),
		TEXT("Test: aps.Finds.Show 0 hides every find and waymark (and their lamps), 1 shows them again."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			using namespace APSStoryFindsLocal;
			FState* State = World ? Find(World) : nullptr;
			const bool bShow = Args.IsEmpty() || FCString::Atoi(*Args[0]) != 0;
			int32 Count = 0;
			for (int32 Index = 0; State && Index < FindCount + WaymarkCount; ++Index)
			{
				const FPlaced& Placed = Index < FindCount ? State->Finds[Index] : State->Waymarks[Index - FindCount];
				if (AActor* Actor = Placed.Actor.Get())
				{
					Actor->SetActorHiddenInGame(!bShow);
					Actor->SetActorEnableCollision(bShow);
					++Count;
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] %s %d sites"), bShow ? TEXT("shown") : TEXT("hidden"), Count);
		}));

	FAutoConsoleCommandWithWorld UseCommand(TEXT("aps.Finds.Use"),
		TEXT("Test: the walker's F at the find within reach."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			AActor* Candidate = APSStoryFinds::Candidate(Pawn);
			UE_LOG(LogTemp, Log, TEXT("[APS.Finds] aps.Finds.Use: %s"), Candidate && APSStoryFinds::TryInteract(Pawn, Candidate)
				? TEXT("taken") : TEXT("nothing within reach"));
		}));
}

#undef LOCTEXT_NAMESPACE

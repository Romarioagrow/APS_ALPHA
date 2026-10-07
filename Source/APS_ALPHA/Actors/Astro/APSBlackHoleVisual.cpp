#include "APSBlackHoleVisual.h"

#include "Star.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Rendering/APSStellarMaterialContract.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"
#include "Math/RotationMatrix.h"
#include "ProceduralMeshComponent.h"

namespace APSBlackHoleVisualDetail
{
TAutoConsoleVariable<int32> CVarBlackHoleV2(
	TEXT("aps.Stars.BlackHoleV2"), 1,
	TEXT("1: a black hole is a black shadow with a photon ring, a tilted accretion disc and lensed arcs (Rio 03.10).\n")
	TEXT("0: the earlier sphere with a painted stripe and rim; built black holes switch back at once."));

constexpr const TCHAR* GuideMaterialPath =
	TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_PreviewGuide.M_APS_PreviewGuide");
const FName VisualTag(TEXT("APS.BlackHoleV2"));

// Layout in photosphere radii. That radius is the envelope every preview scope frames, scales and keeps clear of
// planets, so the whole black hole stays inside the bounds the camera and guides already use.
constexpr double BHDiskOuter = 1.0;
constexpr double BHShadow = BHDiskOuter / 3.2;   // a 2.6 r_s shadow inside a disc of ~8 r_s
constexpr double BHRing = BHShadow * 1.045;      // thin photon ring just outside the shadow
constexpr double BHDiskInner = BHShadow * 1.2;   // ISCO (3 r_s) against the 2.6 r_s shadow
constexpr double BHHaloInner = BHShadow * 1.03;  // the lensed far side of the disc hugs the ring
constexpr double BHOuterWave = 0.025;            // ragged outer rim of the disc
constexpr double BHBandWave = 0.012;             // wavering boundaries between the temperature bands
constexpr int32 BHDiskBands = 12;
constexpr int32 BHDiskSegments = 192;
constexpr int32 BHVeilLayers = 6;
constexpr int32 BHHaloArcs = 5;
// Translucent order after the stellar corona (2): lensed arcs, disc, then the disc's beaming veils.
constexpr int32 BHSortHalo = 3;
constexpr int32 BHSortDisk = 4;
constexpr int32 BHSortBeaming = 5;

struct FVisual
{
	TWeakObjectPtr<AStar> Star;
	TWeakObjectPtr<UStaticMeshComponent> Photosphere;
	TWeakObjectPtr<UProceduralMeshComponent> Shadow;
	TWeakObjectPtr<UProceduralMeshComponent> Ring;
	TWeakObjectPtr<UProceduralMeshComponent> Disk;
	TWeakObjectPtr<UProceduralMeshComponent> Beaming;
	TWeakObjectPtr<UProceduralMeshComponent> Halo;
	/** World-space disc normal; the photosphere master derives the ring's lensed edges from the same axis. */
	FVector Axis{FVector::UpVector};
	FVector LastView{FVector::ZeroVector};
	float SurfaceSeed{-1.0f};
	bool bShown{false};
	bool bRingHidden{false};

	bool IsComplete() const
	{
		return Star.IsValid() && Photosphere.IsValid() && Shadow.IsValid() && Ring.IsValid()
			&& Disk.IsValid() && Beaming.IsValid() && Halo.IsValid();
	}

	TArray<UProceduralMeshComponent*, TInlineAllocator<5>> Parts() const
	{
		TArray<UProceduralMeshComponent*, TInlineAllocator<5>> Result;
		for (const TWeakObjectPtr<UProceduralMeshComponent>& WeakPart : {Shadow, Ring, Disk, Beaming, Halo})
		{
			if (UProceduralMeshComponent* Part = WeakPart.Get()) Result.Add(Part);
		}
		return Result;
	}
};

TArray<FVisual> Visuals;
FTSTicker::FDelegateHandle TickHandle;

/** The black-hole axis of M_SpectralStarMat_SUN (APSFixStarHISMMaterialCommandlet), in float like the shader. */
FVector ShaderAxis(const float Seed)
{
	const FVector3f Raw(Seed * 2.0f - 0.83f,
		FMath::Frac(Seed * 7.13f + 0.31f) * 2.0f - 1.0f,
		FMath::Frac(Seed * 13.71f + 0.73f) * 2.0f - 1.0f);
	return FVector(Raw.GetSafeNormal());
}

/** Shader seeds whose axis tilts the disc 8..26 degrees off the system plane: the classic, readable view. */
const TArray<float>& DiskSeeds()
{
	static const TArray<float> Seeds = []
	{
		TArray<float> Result;
		constexpr int32 Count = 8192;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Seed = (static_cast<float>(Index) + 0.5f) / Count;
			const float WrapY = FMath::Frac(Seed * 7.13f + 0.31f);
			const float WrapZ = FMath::Frac(Seed * 13.71f + 0.73f);
			// Away from frac() wrap points the GPU and the CPU round to the same axis.
			if (WrapY < 0.01f || WrapY > 0.99f || WrapZ < 0.01f || WrapZ > 0.99f) continue;
			const double Tilt = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(ShaderAxis(Seed).Z)));
			if (Tilt >= 8.0 && Tilt <= 26.0) Result.Add(Seed);
		}
		return Result;
	}();
	return Seeds;
}

struct FMeshBuffers
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	int32 Vertex(const FVector& Position, const FVector& Normal)
	{
		Normals.Add(Normal);
		UV0.Add(FVector2D::ZeroVector);
		Colors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		return Vertices.Add(Position);
	}

	/** UE draws a triangle front-facing when Cross(B - A, C - A) points away from the viewer (cf. the engine box mesh). */
	void Triangle(const int32 A, int32 B, int32 C, const FVector& TowardViewer)
	{
		const FVector Cross = FVector::CrossProduct(Vertices[B] - Vertices[A], Vertices[C] - Vertices[A]);
		if (FVector::DotProduct(Cross, TowardViewer) > 0.0) Swap(B, C);
		Triangles.Add(A);
		Triangles.Add(B);
		Triangles.Add(C);
	}

	/** Two triangles per cell of a (Rows + 1) x (Columns + 1) vertex grid that starts at First. */
	void Grid(const int32 First, const int32 Rows, const int32 Columns, const FVector& TowardViewer)
	{
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			for (int32 Column = 0; Column < Columns; ++Column)
			{
				const int32 A = First + Row * (Columns + 1) + Column;
				const int32 C = A + Columns + 1;
				Triangle(A, C, A + 1, TowardViewer);
				Triangle(A + 1, C, C + 1, TowardViewer);
			}
		}
	}

	void Commit(UProceduralMeshComponent* Part, const int32 Section)
	{
		Part->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, UV0, Colors, Tangents, false);
		Vertices.Reset();
		Triangles.Reset();
		Normals.Reset();
		UV0.Reset();
		Colors.Reset();
		Tangents.Reset();
	}
};

/** The shadow is seen from outside; the inside-out ring shell only by the inner side of its far half. */
void AddSphere(FMeshBuffers& Mesh, const double Radius, const bool bInsideOut)
{
	constexpr int32 Segments = 96;
	constexpr int32 Latitudes = 48;
	for (int32 Latitude = 0; Latitude <= Latitudes; ++Latitude)
	{
		const double Theta = UE_DOUBLE_PI * Latitude / Latitudes;
		for (int32 Segment = 0; Segment <= Segments; ++Segment)
		{
			const double Phi = UE_DOUBLE_TWO_PI * Segment / Segments;
			const FVector Direction(FMath::Sin(Theta) * FMath::Cos(Phi),
				FMath::Sin(Theta) * FMath::Sin(Phi), FMath::Cos(Theta));
			Mesh.Vertex(Direction * Radius, bInsideOut ? -Direction : Direction);
		}
	}
	for (int32 Latitude = 0; Latitude < Latitudes; ++Latitude)
	{
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const int32 A = Latitude * (Segments + 1) + Segment;
			const int32 C = A + Segments + 1;
			const FVector Outward = (Mesh.Vertices[A] + Mesh.Vertices[C + 1]).GetSafeNormal();
			const FVector TowardViewer = bInsideOut ? -Outward : Outward;
			Mesh.Triangle(A, C, A + 1, TowardViewer);
			Mesh.Triangle(A + 1, C, C + 1, TowardViewer);
		}
	}
}

/** A wavering circle: integer harmonics close seamlessly, so neighbouring bands share it exactly. */
struct FEdgeWave
{
	double Amplitude{0.0};
	double Phase[3]{0.0, 0.0, 0.0};

	double At(const double Base, const double Angle) const
	{
		return Base * (1.0 + Amplitude * (0.55 * FMath::Sin(5.0 * Angle + Phase[0])
			+ 0.30 * FMath::Sin(9.0 * Angle + Phase[1]) + 0.15 * FMath::Sin(14.0 * Angle + Phase[2])));
	}
};

/** Band edges crowd towards the hot inner edge, where the colour changes fastest. */
double BandEdge(const int32 Edge)
{
	return BHDiskInner + (BHDiskOuter - BHDiskInner)
		* FMath::Pow(static_cast<double>(Edge) / BHDiskBands, 1.35);
}

void AddBand(FMeshBuffers& Mesh, const double Inner, const FEdgeWave& InnerEdge,
	const double Outer, const FEdgeWave& OuterEdge)
{
	for (int32 Segment = 0; Segment <= BHDiskSegments; ++Segment)
	{
		const double Angle = UE_DOUBLE_TWO_PI * Segment / BHDiskSegments;
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
		Mesh.Vertex(Direction * InnerEdge.At(Inner, Angle), FVector::UpVector);
		Mesh.Vertex(Direction * OuterEdge.At(Outer, Angle), FVector::UpVector);
	}
	Mesh.Grid(0, BHDiskSegments, 1, FVector::UpVector);
}

/** A dimming veil over the receding half, centred on local -X; its edges waver so the steps read as gas. */
void AddRecedingVeil(FMeshBuffers& Mesh, const double HalfWidthDegrees, const int32 Layer)
{
	constexpr int32 Rows = 12;
	constexpr int32 Columns = 72;
	const double OuterLimit = BHDiskOuter * (1.0 + BHOuterWave);
	for (int32 Row = 0; Row <= Rows; ++Row)
	{
		const double Along = static_cast<double>(Row) / Rows;
		const double Radius = FMath::Lerp(BHDiskInner, OuterLimit, Along);
		const double HalfWidth = FMath::DegreesToRadians(HalfWidthDegrees
			+ 5.0 * FMath::Sin(Along * 7.0 + Layer * 1.3) + 2.5 * FMath::Sin(Along * 17.0 + Layer * 0.7));
		for (int32 Column = 0; Column <= Columns; ++Column)
		{
			const double Angle = UE_DOUBLE_PI + HalfWidth * (2.0 * Column / Columns - 1.0);
			Mesh.Vertex(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius, FVector::UpVector);
		}
	}
	Mesh.Grid(0, Rows, Columns, FVector::UpVector);
}

/** A crescent hugging the ring on local +X (Side 1) or -X (Side -1), thinning to nothing at the disc's edges. */
void AddCrescent(FMeshBuffers& Mesh, const double Thickness, const double Side)
{
	constexpr int32 Columns = 96;
	for (int32 Column = 0; Column <= Columns; ++Column)
	{
		const double Around = UE_DOUBLE_HALF_PI * (2.0 * Column / Columns - 1.0);
		const FVector Direction(Side * FMath::Cos(Around), FMath::Sin(Around), 0.0);
		const double Reach = Thickness * FMath::Pow(FMath::Max(FMath::Cos(Around), 0.0), 0.6);
		Mesh.Vertex(Direction * BHHaloInner, FVector::UpVector);
		Mesh.Vertex(Direction * (BHHaloInner + Reach), FVector::UpVector);
	}
	Mesh.Grid(0, Columns, 1, FVector::UpVector);
}

/** Hot white-blue inner edge, white, yellow, orange to orange-red at the rim (linear colour, 0..1 across). */
FLinearColor DiskTint(const double Across)
{
	static const double Stops[] = {0.0, 0.10, 0.24, 0.40, 0.58, 0.78, 1.0};
	static const FLinearColor Tints[] = {
		FLinearColor(0.80f, 0.88f, 1.00f), FLinearColor(0.96f, 0.97f, 1.00f), FLinearColor(1.00f, 0.90f, 0.74f),
		FLinearColor(1.00f, 0.74f, 0.44f), FLinearColor(1.00f, 0.56f, 0.24f), FLinearColor(0.96f, 0.40f, 0.13f),
		FLinearColor(0.82f, 0.26f, 0.07f)};
	for (int32 Stop = 1; Stop < 7; ++Stop)
	{
		if (Across <= Stops[Stop])
		{
			const float Blend = static_cast<float>((Across - Stops[Stop - 1]) / (Stops[Stop] - Stops[Stop - 1]));
			return FMath::Lerp(Tints[Stop - 1], Tints[Stop], FMath::Clamp(Blend, 0.0f, 1.0f));
		}
	}
	return Tints[6];
}

/** Emitted HDR light of a band: about the photon ring's 6.8 at the inner edge, a dim red glow at the rim. */
FLinearColor DiskEmission(const double Across)
{
	return DiskTint(Across) * static_cast<float>(0.25 + 6.8 * FMath::Pow(1.0 - Across, 1.8));
}

/** Opacity hides the stars behind the dense inner disc; the outer bands fade so the rim stays soft. */
float DiskOpacity(const double Across, const int32 Band)
{
	float Opacity = static_cast<float>(0.92 - 0.5 * Across);
	if (Band == BHDiskBands - 1) Opacity *= 0.55f;
	else if (Band == BHDiskBands - 2) Opacity *= 0.8f;
	return Opacity;
}

UMaterialInstanceDynamic* StellarPart(UMaterialInterface* Base, AStar* Star, const float Archetype, const float Seed)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Star);
	if (Material)
	{
		// A black spectrum: the ordinary photosphere then emits nothing; archetype 6 has its own fixed colours.
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
		Material->SetScalarParameterValue(TEXT("Multiplier"), 0.0f);
		Material->SetScalarParameterValue(TEXT("StellarArchetype"), Archetype);
		Material->SetScalarParameterValue(TEXT("SurfaceSeed"), Seed);
	}
	return Material;
}

/** Translucent unlit guide master: the eye receives GuideColor x GuideOpacity over the scene. */
UMaterialInstanceDynamic* GuidePart(UMaterialInterface* Guide, AStar* Star, const FLinearColor& Colour, const float Opacity)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Guide, Star);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("GuideColor"), Colour);
		Material->SetScalarParameterValue(TEXT("GuideOpacity"), Opacity);
	}
	return Material;
}

UProceduralMeshComponent* NewPart(AStar* Star, UStaticMeshComponent* Photosphere, const int32 SortPriority)
{
	UProceduralMeshComponent* Part = NewObject<UProceduralMeshComponent>(Star, NAME_None, RF_Transient);
	Part->ComponentTags.Add(VisualTag);
	Part->SetupAttachment(Photosphere);
	Part->SetMobility(EComponentMobility::Movable);
	// Position and scale follow every presentation of the photosphere; orientation stays in world space,
	// where the photosphere master evaluates the black-hole axis.
	Part->SetAbsolute(false, true, false);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCollisionResponseToAllChannels(ECR_Ignore);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->SetCastShadow(false);
	Part->bAffectDynamicIndirectLighting = false;
	Part->bAffectDistanceFieldLighting = false;
	Part->bVisibleInReflectionCaptures = false;
	Part->bVisibleInRealTimeSkyCaptures = false;
	Part->bVisibleInRayTracing = false;
	Part->SetReceivesDecals(false);
	Part->SetTranslucentSortPriority(SortPriority);
	return Part;
}

double EnvelopeAssetRadius(const UStaticMeshComponent* Photosphere)
{
	const UStaticMesh* Mesh = IsValid(Photosphere) ? Photosphere->GetStaticMesh() : nullptr;
	return IsValid(Mesh) ? FMath::Max(Mesh->GetBounds().BoxExtent.GetMax(), UE_DOUBLE_SMALL_NUMBER) : 1.0;
}

/** Centre on the photosphere's rendered bounds (Blueprint pivots may be off-centre) at its radius. */
void Place(const FVisual& Visual)
{
	const UStaticMeshComponent* Photosphere = Visual.Photosphere.Get();
	const UStaticMesh* Mesh = IsValid(Photosphere) ? Photosphere->GetStaticMesh() : nullptr;
	if (!IsValid(Mesh)) return;
	const FVector Origin = Mesh->GetBounds().Origin;
	const double AssetRadius = EnvelopeAssetRadius(Photosphere);
	for (UProceduralMeshComponent* Part : Visual.Parts())
	{
		Part->SetRelativeLocation(Origin);
		Part->SetRelativeScale3D(FVector(AssetRadius));
	}
}

void Show(FVisual& Visual, const bool bShow)
{
	Visual.bShown = bShow;
	// V2 keeps the V1 sphere for bounds, framing, selection and visibility state; no view draws it.
	if (UStaticMeshComponent* Photosphere = Visual.Photosphere.Get())
	{
		Photosphere->SetVisibleInSceneCaptureOnly(bShow);
		Photosphere->SetHiddenInSceneCapture(bShow);
	}
	const UProceduralMeshComponent* RingPart = Visual.Ring.Get();
	for (UProceduralMeshComponent* Part : Visual.Parts())
	{
		Part->SetVisibleInSceneCaptureOnly(!bShow || (Part == RingPart && Visual.bRingHidden));
	}
}

void Release(const FVisual& Visual)
{
	if (UStaticMeshComponent* Photosphere = Visual.Photosphere.Get())
	{
		Photosphere->SetVisibleInSceneCaptureOnly(false);
		Photosphere->SetHiddenInSceneCapture(false);
	}
	for (UProceduralMeshComponent* Part : Visual.Parts())
	{
		Part->DestroyComponent();
	}
}

void Remove(AStar* Star)
{
	bool bReleased = false;
	for (int32 Index = Visuals.Num() - 1; Index >= 0; --Index)
	{
		if (Visuals[Index].Star.Get() != Star) continue;
		Release(Visuals[Index]);
		Visuals.RemoveAtSwap(Index, 1, EAllowShrinking::No);
		bReleased = true;
	}
	// Parts that outlived their record (a live-coding patch restarts this registry).
	TInlineComponentArray<UProceduralMeshComponent*> Tagged(Star);
	for (UProceduralMeshComponent* Part : Tagged)
	{
		if (IsValid(Part) && Part->ComponentHasTag(VisualTag))
		{
			Part->DestroyComponent();
			bReleased = true;
		}
	}
	if (bReleased && IsValid(Star->StarMesh))
	{
		Star->StarMesh->SetVisibleInSceneCaptureOnly(false);
		Star->StarMesh->SetHiddenInSceneCapture(false);
	}
}

/** The view that rendered this world last frame (closest to the hole), else the first player's camera. */
bool ResolveView(const UWorld* World, const FVector& Center, FVector& OutCamera)
{
	if (!World) return false;
	double Closest = TNumericLimits<double>::Max();
	for (const FVector& Location : World->ViewLocationsRenderedLastFrame)
	{
		const double DistanceSquared = FVector::DistSquared(Location, Center);
		if (DistanceSquared < Closest)
		{
			Closest = DistanceSquared;
			OutCamera = Location;
		}
	}
	if (Closest < TNumericLimits<double>::Max()) return true;
	const APlayerController* Controller = World->GetFirstPlayerController();
	if (Controller && Controller->PlayerCameraManager)
	{
		OutCamera = Controller->PlayerCameraManager->GetCameraLocation();
		return true;
	}
	return false;
}

/** Turns the view-dependent parts: beaming towards the approaching side, lensed arcs facing the camera. */
void UpdateView(FVisual& Visual, const bool bForce)
{
	UProceduralMeshComponent* ShadowPart = Visual.Shadow.Get();
	UProceduralMeshComponent* RingPart = Visual.Ring.Get();
	UProceduralMeshComponent* BeamingPart = Visual.Beaming.Get();
	UProceduralMeshComponent* HaloPart = Visual.Halo.Get();
	if (!ShadowPart || !RingPart || !BeamingPart || !HaloPart) return;
	const FVector Center = ShadowPart->GetComponentLocation();
	FVector Camera = FVector::ZeroVector;
	if (!ResolveView(ShadowPart->GetWorld(), Center, Camera)) return;
	const FVector Offset = Camera - Center;
	const double Distance = Offset.Size();
	if (!FMath::IsFinite(Distance) || Distance <= UE_DOUBLE_SMALL_NUMBER) return;
	const FVector View = Offset / Distance;

	// Inside the ring shell its inner faces would surround the camera (parts are built in envelope radii).
	const double RingWorldRadius = BHRing * RingPart->GetComponentScale().GetAbsMax();
	const bool bRingHidden = Distance < RingWorldRadius * 1.1;
	if (bRingHidden != Visual.bRingHidden)
	{
		Visual.bRingHidden = bRingHidden;
		RingPart->SetVisibleInSceneCaptureOnly(bRingHidden || !Visual.bShown);
	}
	if (!bForce && FVector::DotProduct(View, Visual.LastView) > 0.99999999) return;
	Visual.LastView = View;

	// The photosphere master brightens the lensed disc edge at +(axis x view); the beaming veils darken the
	// opposite, receding half (their local -X), so the ring and the disc agree on the rotation.
	const FVector Approaching = FVector::CrossProduct(Visual.Axis, View);
	if (Approaching.SizeSquared() > 1.0e-10)
	{
		BeamingPart->SetWorldRotation(FRotationMatrix::MakeFromZX(Visual.Axis, Approaching).ToQuat());
	}
	// The thick arc shows the far side's lit face: above the shadow when the camera is above the disc.
	FVector Top = Visual.Axis - View * FVector::DotProduct(Visual.Axis, View);
	if (Top.SizeSquared() > 1.0e-10)
	{
		Top.Normalize();
		if (FVector::DotProduct(View, Visual.Axis) < 0.0) Top = -Top;
		HaloPart->SetWorldRotation(FRotationMatrix::MakeFromZX(View, Top).ToQuat());
	}
	else
	{
		HaloPart->SetWorldRotation(FRotationMatrix::MakeFromZ(View).ToQuat());
	}
}

bool TickVisuals(float)
{
	const bool bEnabled = CVarBlackHoleV2.GetValueOnGameThread() != 0;
	for (int32 Index = Visuals.Num() - 1; Index >= 0; --Index)
	{
		FVisual& Visual = Visuals[Index];
		if (!Visual.IsComplete())
		{
			Release(Visual);
			Visuals.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Visual.bShown != bEnabled) Show(Visual, bEnabled);
		if (bEnabled) UpdateView(Visual, false);
	}
	return !Visuals.IsEmpty();
}

void EnsureTicker()
{
	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickVisuals), 0.0f);
	}
}
}

bool APSBlackHoleVisual::IsEnabled()
{
	return APSBlackHoleVisualDetail::CVarBlackHoleV2.GetValueOnGameThread() != 0;
}

void APSBlackHoleVisual::GetDiskKeyLight(float& OutIntensity, float& OutTemperatureKelvin)
{
	// About a third of the dimmest stellar key (9) and the colour of the disc's orange body.
	OutIntensity = 3.0f;
	OutTemperatureKelvin = 4300.0f;
}

FLinearColor APSBlackHoleVisual::GetCatalogueTint()
{
	// The point material keeps the hue and scales energy by the peak channel: 0.12 gives about a fifth of a star.
	return FLinearColor(0.12f, 0.066f, 0.03f);
}

void APSBlackHoleVisual::Configure(AStar* Star, const EStellarType StellarType, const float SurfaceSeed)
{
	using namespace APSBlackHoleVisualDetail;
	if (!IsValid(Star)) return;
	UStaticMeshComponent* Photosphere = Star->StarMesh;
	if (StellarType != EStellarType::BlackHole || CVarBlackHoleV2.GetValueOnGameThread() == 0
		|| !IsValid(Photosphere) || !IsValid(Photosphere->GetStaticMesh()) || !Star->GetWorld())
	{
		Remove(Star);
		return;
	}
	for (FVisual& Existing : Visuals)
	{
		if (Existing.Star.Get() == Star && Existing.IsComplete() && Existing.Photosphere.Get() == Photosphere
			&& Existing.SurfaceSeed == SurfaceSeed)
		{
			// A re-applied spectrum (UI edit, replay) keeps the same disc; follow the mesh only.
			Place(Existing);
			Show(Existing, true);
			UpdateView(Existing, true);
			return;
		}
	}
	Remove(Star);

	UMaterialInterface* Stellar = APSStellarMaterialContract::LoadCanonicalBase(
		APSStellarMaterialContract::ActorBaseObjectPath);
	UMaterialInterface* Guide = LoadObject<UMaterialInterface>(nullptr, GuideMaterialPath);
	if (!IsValid(Stellar) || !IsValid(Guide))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Stars] black hole visual v2 unavailable for %s (stellar master=%d guide master=%d); the earlier look stays"),
			*GetNameSafe(Star), IsValid(Stellar) ? 1 : 0, IsValid(Guide) ? 1 : 0);
		return;
	}

	const TArray<float>& Seeds = DiskSeeds();
	const float DiskSeed = Seeds.IsEmpty() ? 0.4474f : Seeds[FMath::Clamp(
		FMath::FloorToInt(FMath::Frac(SurfaceSeed * 97.31f + 0.137f) * Seeds.Num()), 0, Seeds.Num() - 1)];
	FRandomStream Random(static_cast<int32>(GetTypeHash(SurfaceSeed) & 0x7fffffff) ^ 0x5B1A7E3D);
	FVisual Visual;
	Visual.Star = Star;
	Visual.Photosphere = Photosphere;
	Visual.SurfaceSeed = SurfaceSeed;
	Visual.Axis = ShaderAxis(DiskSeed);

	UProceduralMeshComponent* ShadowPart = NewPart(Star, Photosphere, 0);
	UProceduralMeshComponent* RingPart = NewPart(Star, Photosphere, 0);
	UProceduralMeshComponent* DiskPart = NewPart(Star, Photosphere, BHSortDisk);
	UProceduralMeshComponent* BeamingPart = NewPart(Star, Photosphere, BHSortBeaming);
	UProceduralMeshComponent* HaloPart = NewPart(Star, Photosphere, BHSortHalo);
	FMeshBuffers Mesh;

	// The event horizon's shadow: an opaque photosphere with a black spectrum emits exactly nothing.
	AddSphere(Mesh, BHShadow, false);
	Mesh.Commit(ShadowPart, 0);
	ShadowPart->SetMaterial(0, StellarPart(Stellar, Star, 0.0f, DiskSeed));
	// Photon ring: the archetype-6 limb glow fills the far half of a slightly larger inside-out shell, which
	// shows only as a thin camera-facing ring around the shadow, brightened where the disc's edges are lensed.
	AddSphere(Mesh, BHRing, true);
	Mesh.Commit(RingPart, 0);
	RingPart->SetMaterial(0, StellarPart(Stellar, Star, 6.0f, DiskSeed));

	FEdgeWave Edges[BHDiskBands + 1];
	for (int32 Edge = 0; Edge <= BHDiskBands; ++Edge)
	{
		// The inner (ISCO) edge stays sharp; the rim and the band boundaries waver.
		Edges[Edge].Amplitude = Edge == 0 ? 0.0 : Edge == BHDiskBands ? BHOuterWave : BHBandWave;
		for (double& Phase : Edges[Edge].Phase) Phase = Random.FRandRange(0.0f, UE_TWO_PI);
	}
	for (int32 Band = 0; Band < BHDiskBands; ++Band)
	{
		AddBand(Mesh, BandEdge(Band), Edges[Band], BandEdge(Band + 1), Edges[Band + 1]);
		Mesh.Commit(DiskPart, Band);
		const double Across = FMath::Pow((Band + 0.5) / BHDiskBands, 1.35);
		const float Opacity = DiskOpacity(Across, Band);
		DiskPart->SetMaterial(Band, GuidePart(Guide, Star, DiskEmission(Across) / Opacity, Opacity));
	}
	// Relativistic beaming: nested veils of one dark red over the receding half; the same colour on every
	// layer keeps their blend order-independent. About 2.5x dimmer at the receding centre.
	const double VeilHalfWidths[BHVeilLayers] = {30.0, 52.0, 74.0, 96.0, 118.0, 140.0};
	for (int32 Layer = 0; Layer < BHVeilLayers; ++Layer)
	{
		AddRecedingVeil(Mesh, VeilHalfWidths[Layer], Layer);
		Mesh.Commit(BeamingPart, Layer);
		BeamingPart->SetMaterial(Layer, GuidePart(Guide, Star, FLinearColor(0.22f, 0.045f, 0.012f), 0.14f));
	}
	// Lensed far side of the disc: a thick arc on the lit side, a thin one opposite, one warm colour.
	const double ArcThickness[BHHaloArcs] = {0.16, 0.30, 0.50, 0.08, 0.17};
	const double ArcSide[BHHaloArcs] = {1.0, 1.0, 1.0, -1.0, -1.0};
	const float ArcOpacity[BHHaloArcs] = {0.30f, 0.18f, 0.10f, 0.24f, 0.12f};
	for (int32 Arc = 0; Arc < BHHaloArcs; ++Arc)
	{
		AddCrescent(Mesh, ArcThickness[Arc] * BHShadow, ArcSide[Arc]);
		Mesh.Commit(HaloPart, Arc);
		HaloPart->SetMaterial(Arc, GuidePart(Guide, Star, FLinearColor(1.0f, 0.86f, 0.66f) * 6.5f, ArcOpacity[Arc]));
	}

	for (UProceduralMeshComponent* Part : {ShadowPart, RingPart, DiskPart, BeamingPart, HaloPart})
	{
		Star->AddInstanceComponent(Part);
		Part->RegisterComponent();
	}
	DiskPart->SetWorldRotation(FRotationMatrix::MakeFromZ(Visual.Axis).ToQuat());
	Visual.Shadow = ShadowPart;
	Visual.Ring = RingPart;
	Visual.Disk = DiskPart;
	Visual.Beaming = BeamingPart;
	Visual.Halo = HaloPart;
	Place(Visual);
	FVisual& Added = Visuals.Add_GetRef(Visual);
	Show(Added, true);
	UpdateView(Added, true);
	EnsureTicker();

	const double EnvelopeRadius = EnvelopeAssetRadius(Photosphere) * Photosphere->GetComponentScale().GetAbsMax();
	UE_LOG(LogTemp, Log,
		TEXT("[APS.Stars] black hole visual v2 star=%s envelopeRadius=%.4g shadowRadius=%.4g diskTiltDeg=%.1f diskSeed=%.5f parts=5 sections=%d (aps.Stars.BlackHoleV2 0 = earlier look)"),
		*GetNameSafe(Star), EnvelopeRadius, EnvelopeRadius * BHShadow,
		FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(Visual.Axis.Z))), DiskSeed,
		2 + BHDiskBands + BHVeilLayers + BHHaloArcs);
}

#include "SAPSStrategicMapView.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"

#include "APSStrategicMapCamera.h"
#include "APSStrategicMapScene.h"
#include "APS_ALPHA/Core/Rendering/APSGalaxyGpuStars.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewVisibility.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/Pawns/Spaceships/APSShipFlightModel.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "APS_ALPHA/UI/MainMenu/APSPreviewAnnotationLayout.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "SceneView.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "APSStrategicMapView"

namespace APSStrategicMapViewLocal
{
	using APSStrategicMap::EKind;
	using APSStrategicMap::ELayer;
	using APSStrategicMap::FObject;
	using APSStrategicMap::FPlate;

	/** A click picks the nearest marker this close (px). */
	constexpr double PickReach = 12.0;
	/** Markers closer than this merge into the first one drawn there, with a "+N" (px). */
	constexpr double MergeDistance = 6.0;
	/** A body larger than this share of the view is seen itself: no ring round it. */
	constexpr double ResolvedBodyShare = 0.16;
	/** The home system counts as resolved, its own bodies labelled instead of its catalogue mark, from this size (px). */
	constexpr double ResolvedSystemPixels = 36.0;

	/** One camera snapshot for the whole paint: the projection the frame was rendered with (SPreviewSystemOverlay's). */
	struct FProjector
	{
		FVector Origin{FVector::ZeroVector};
		FVector Forward{FVector::ForwardVector};
		FVector Right{FVector::RightVector};
		FVector Up{FVector::UpVector};
		/** View rotation times projection, applied to positions relative to Origin (full double precision at AU range). */
		FMatrix ViewProjection{FMatrix::Identity};
		FIntRect Rect;
		/** Viewport pixels to panel space. */
		FVector2D PixelScale{1.0, 1.0};
		FVector2D PixelOffset{0.0, 0.0};
		FVector2D ViewportPixels{1.0, 1.0};
		/** Panel units per unit of radius/depth. */
		double ScaleX{1.0};
		double Near{10.0};

		bool Build(APlayerController* Controller, const FGeometry& Geometry)
		{
			ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
			if (!Player || !Player->ViewportClient || !Player->ViewportClient->Viewport)
			{
				return false;
			}
			FSceneViewProjectionData Projection;
			if (!Player->GetProjectionData(Player->ViewportClient->Viewport, Projection))
			{
				return false;
			}
			const TSharedPtr<SViewport> ViewportWidget = Player->ViewportClient->GetGameViewportWidget();
			int32 Width = 0;
			int32 Height = 0;
			Controller->GetViewportSize(Width, Height);
			Rect = Projection.GetConstrainedViewRect();
			if (!ViewportWidget.IsValid() || Width <= 0 || Height <= 0 || Rect.Width() <= 0 || Rect.Height() <= 0)
			{
				return false;
			}
			// Paint space on both sides: the tick-space geometry is desktop space and shifted every marker in a window.
			const FGeometry ViewportGeometry = ViewportWidget->GetPaintSpaceGeometry();
			const FVector2D Min = Geometry.AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D Max = Geometry.AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(ViewportGeometry.GetLocalSize()));
			ViewportPixels = FVector2D(Width, Height);
			PixelOffset = Min;
			PixelScale = (Max - Min) / ViewportPixels;
			if (PixelScale.X <= 0.0 || PixelScale.Y <= 0.0 || PixelScale.ContainsNaN())
			{
				return false;
			}
			Origin = Projection.ViewOrigin;
			ViewProjection = Projection.ViewRotationMatrix * Projection.ProjectionMatrix;
			ScaleX = Projection.ProjectionMatrix.M[0][0] * Rect.Width() * 0.5 * PixelScale.X;
			Near = FMath::Max(static_cast<double>(FSceneViewProjectionData::GetNearPlaneFromProjectionMatrix(
				Projection.ProjectionMatrix)), 1.0);
			FVector Location;
			FRotator Rotation;
			Controller->GetPlayerViewPoint(Location, Rotation);
			const FQuat Quat = Rotation.Quaternion();
			Forward = Quat.GetForwardVector();
			Right = Quat.GetRightVector();
			Up = Quat.GetUpVector();
			return true;
		}

		bool ProjectRelative(const FVector& Relative, FVector2D& OutLocal) const
		{
			const FVector4 Clip = ViewProjection.TransformFVector4(FVector4(Relative, 1.0));
			if (Clip.W <= Near * 0.5)
			{
				return false;
			}
			const double X = Clip.X / Clip.W;
			const double Y = Clip.Y / Clip.W;
			const FVector2D Pixel(Rect.Min.X + (X * 0.5 + 0.5) * Rect.Width(), Rect.Min.Y + (0.5 - Y * 0.5) * Rect.Height());
			OutLocal = PixelOffset + Pixel * PixelScale;
			return FMath::IsFinite(OutLocal.X) && FMath::IsFinite(OutLocal.Y);
		}

		bool Project(const FVector& World, FVector2D& OutLocal, double& OutDepth) const
		{
			const FVector Relative = World - Origin;
			OutDepth = FVector::DotProduct(Relative, Forward);
			return OutDepth > Near && ProjectRelative(Relative, OutLocal);
		}

		double PixelRadius(const double Radius, const double Depth) const
		{
			return Depth > 0.0 ? Radius / Depth * ScaleX : 0.0;
		}

		/** Cuts a camera-relative segment at the near plane; false when it lies wholly behind. */
		bool ClipToNear(FVector& A, FVector& B) const
		{
			const double Limit = Near * 2.0;
			const double DepthA = FVector::DotProduct(A, Forward);
			const double DepthB = FVector::DotProduct(B, Forward);
			if (DepthA <= Limit && DepthB <= Limit)
			{
				return false;
			}
			if (DepthA < Limit)
			{
				A = FMath::Lerp(A, B, (Limit - DepthA) / (DepthB - DepthA));
			}
			else if (DepthB < Limit)
			{
				B = FMath::Lerp(A, B, (Limit - DepthA) / (DepthB - DepthA));
			}
			return true;
		}

		/** A world segment on the panel: near-clipped, projected and cut to the panel. */
		bool ProjectSegment(const FVector& WorldA, const FVector& WorldB, const FVector2D& Size, FVector2D& OutA,
			FVector2D& OutB) const
		{
			FVector A = WorldA - Origin;
			FVector B = WorldB - Origin;
			return ClipToNear(A, B) && ProjectRelative(A, OutA) && ProjectRelative(B, OutB)
				&& APSPreviewVisibility::ClipToPanel(OutA, OutB, Size);
		}

		/** The screen direction to a point, also behind the camera (the edge arrow). */
		FVector2D Direction(const FVector& World) const
		{
			const FVector Relative = World - Origin;
			return FVector2D(FVector::DotProduct(Relative, Right), -FVector::DotProduct(Relative, Up)).GetSafeNormal();
		}
	};

	FLinearColor WithAlpha(const FLinearColor& Colour, const float Alpha)
	{
		return FLinearColor(Colour.R, Colour.G, Colour.B, Alpha);
	}

	const FSlateBrush* DiscBrush()
	{
		// The brush's default half-height rounding makes a circle of any square box.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	void Lines(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, TArray<FVector2f>&& Points,
		const FLinearColor& Colour, const float Thickness)
	{
		if (Points.Num() > 1)
		{
			FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None,
				Colour, true, Thickness);
		}
	}

	void Segment(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& A,
		const FVector2D& B, const FLinearColor& Colour, const float Thickness)
	{
		Lines(Out, Layer, Geometry, TArray<FVector2f>{FVector2f(A), FVector2f(B)}, Colour, Thickness);
	}

	void Circle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 0.75)
		{
			return;
		}
		const int32 Segments = FMath::Clamp(FMath::CeilToInt(Radius * 0.75), 16, 160);
		TArray<FVector2f> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const double Angle = UE_TWO_PI * Index / Segments;
			Points.Add(FVector2f(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius));
		}
		Lines(Out, Layer, Geometry, MoveTemp(Points), Colour, Thickness);
	}

	/** A dashed circle: a boundary (a system's sphere, a relay's reach), not an orbit. */
	void DashedCircle(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour, const float Thickness)
	{
		if (Radius < 2.0)
		{
			return;
		}
		const int32 Dashes = FMath::Clamp(FMath::RoundToInt(Radius * 0.22), 18, 120);
		for (int32 Index = 0; Index < Dashes; ++Index)
		{
			const double From = UE_TWO_PI * Index / Dashes;
			const double To = From + UE_TWO_PI * 0.55 / Dashes;
			TArray<FVector2f> Points;
			for (int32 Step = 0; Step <= 3; ++Step)
			{
				const double Angle = FMath::Lerp(From, To, Step / 3.0);
				Points.Add(FVector2f(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius));
			}
			Lines(Out, Layer, Geometry, MoveTemp(Points), Colour, Thickness);
		}
	}

	void Dot(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const FLinearColor& Colour)
	{
		const float Size = static_cast<float>(Radius * 2.0);
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2f(Size, Size),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Centre.X - Radius), static_cast<float>(Centre.Y - Radius)))),
			DiscBrush(), ESlateDrawEffect::None, Colour);
	}

	void Polygon(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const TArray<FVector2D>& Corners, const FLinearColor& Colour, const float Thickness)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Corners.Num() + 1);
		for (const FVector2D& Corner : Corners)
		{
			Points.Add(FVector2f(Centre + Corner));
		}
		if (!Corners.IsEmpty())
		{
			Points.Add(FVector2f(Centre + Corners[0]));
		}
		Lines(Out, Layer, Geometry, MoveTemp(Points), Colour, Thickness);
	}

	/** A dashed line, the look of an order under way. */
	void Dashes(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& From,
		const FVector2D& To, const FLinearColor& Colour, const double Dash, const double Gap, const float Thickness)
	{
		const double Length = FVector2D::Distance(From, To);
		if (Length < 1.0)
		{
			return;
		}
		const FVector2D Along = (To - From) / Length;
		// Capped: a line across the whole screen never costs more than a few hundred dashes.
		const double Step = FMath::Max(Dash + Gap, Length / 400.0);
		for (double Start = 0.0; Start < Length; Start += Step)
		{
			Segment(Out, Layer, Geometry, From + Along * Start, From + Along * FMath::Min(Start + Dash, Length), Colour,
				Thickness);
		}
	}

	/** An arc from the top, clockwise, for a share of a full turn (work progress). */
	void Arc(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const double Radius, const double Share, const FLinearColor& Colour)
	{
		const double Clamped = FMath::Clamp(Share, 0.0, 1.0);
		const int32 Segments = FMath::Max(2, FMath::RoundToInt(48.0 * Clamped));
		TArray<FVector2f> Points;
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const double Angle = UE_TWO_PI * Clamped * Index / Segments - UE_HALF_PI;
			Points.Add(FVector2f(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius));
		}
		Lines(Out, Layer, Geometry, MoveTemp(Points), Colour, 2.0f);
	}

	void Text(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Position,
		const FString& String, const FSlateFontInfo& Font, const FLinearColor& Colour)
	{
		FSlateDrawElement::MakeText(Out, Layer, Geometry.ToPaintGeometry(FVector2f(600.0f, 20.0f),
			FSlateLayoutTransform(FVector2f(FMath::RoundToFloat(Position.X), FMath::RoundToFloat(Position.Y)))),
			String, Font, ESlateDrawEffect::None, Colour);
	}

	/**
	 * Rio 04.10 ("the course line and its label are white on the bright star field, hard to see"): a line that reads on
	 * any background: a dark halo under it, then the line in its colour.
	 */
	void HaloDashes(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& From,
		const FVector2D& To, const FLinearColor& Colour, const double Dash, const double Gap, const float Thickness)
	{
		Dashes(Out, Layer, Geometry, From, To, APSUITheme::Retint(FLinearColor(0.0f, 0.006f, 0.012f, 0.72f * Colour.A)), Dash, Gap, Thickness + 3.0f);
		Dashes(Out, Layer, Geometry, From, To, Colour, Dash, Gap, Thickness);
	}

	/** Text on a dark plate with a bar in its colour, like the map's object plates. */
	void PlatedText(FSlateWindowElementList& Out, const int32 Layer, const FGeometry& Geometry, const FVector2D& Position,
		const FString& String, const FSlateFontInfo& Font, const FLinearColor& Colour)
	{
		const FVector2D Measured = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(String, Font);
		const FVector2D PlateAt(FMath::RoundToDouble(Position.X) - 8.0, FMath::RoundToDouble(Position.Y) - 3.0);
		const FVector2D PlateSize(Measured.X + 14.0, Measured.Y + 6.0);
		const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(PlateSize, FSlateLayoutTransform(PlateAt)), White,
			ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.002f, 0.014f, 0.026f, 0.9f)));
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(FVector2D(3.0, PlateSize.Y), FSlateLayoutTransform(PlateAt)),
			White, ESlateDrawEffect::None, Colour);
		Text(Out, Layer + 1, Geometry, Position, String, Font, Colour);
	}

	/** "1.24 AU", "38,400 KM", "820 M". */
	FString DistanceString(const double Cm)
	{
		FNumberFormattingOptions Options;
		if (Cm >= 0.01 * APSStars::AstronomicalUnitCm)
		{
			const double Au = Cm / APSStars::AstronomicalUnitCm;
			Options.SetMaximumFractionalDigits(Au >= 100.0 ? 0 : Au >= 10.0 ? 1 : 2);
			return APSUINumber::Number(Au, &Options).ToString() + TEXT(" AU");
		}
		if (Cm >= 100000.0)
		{
			Options.SetMaximumFractionalDigits(Cm >= 1000000.0 ? 0 : 1);
			return APSUINumber::Number(Cm / 100000.0, &Options).ToString() + TEXT(" KM");
		}
		Options.SetMaximumFractionalDigits(0);
		return APSUINumber::Number(Cm / 100.0, &Options).ToString() + TEXT(" M");
	}

	bool LayerShows(const FAPSStrategicMapScene& Scene, const FObject& Object)
	{
		if (Object.bPlayer || Object.Kind == EKind::Pilot)
		{
			return true;
		}
		switch (Object.Kind)
		{
		case EKind::Ship:
			return Scene.IsLayerOn(ELayer::Ships);
		case EKind::Station:
		case EKind::Outpost:
		case EKind::Colony:
		case EKind::Anomaly:
			return Scene.IsLayerOn(ELayer::Stations);
		default:
			return true;
		}
	}

	/** A label waiting for a place (FAPSPreviewAnnotationCandidate carries only its index into these). */
	struct FLabel
	{
		const FPlate* Plate{nullptr};
		const FText* Type{nullptr};
		const FText* Name{nullptr};
		const FText* Designation{nullptr};
		FLinearColor Colour{FLinearColor::White};
		FVector2D Anchor{FVector2D::ZeroVector};
		float RingRadius{7.0f};
		int32 Rank{0};
		bool bSelected{false};
		bool bHovered{false};
		/** A body that fills the view: its plate docks in the corner, without a leader. */
		bool bDocked{false};
	};
}

void SAPSStrategicMapView::Construct(const FArguments& InArgs)
{
	Controller = InArgs._Controller;
	Scene = InArgs._Scene;
	Camera = InArgs._Camera;
	OnSelect = InArgs._OnSelect;
	OnFocus = InArgs._OnFocus;
	SetClipping(EWidgetClipping::ClipToBounds);
}

int32 SAPSStrategicMapView::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using APSStrategicMap::EKind;
	using APSStrategicMap::ELayer;
	using APSStrategicMap::FObject;
	using APSStrategicMap::FPlate;
	using APSStrategicMap::FSelection;
	using APSStrategicMap::FSystemMark;
	using APSStrategicMap::PlateBar;
	using APSStrategicMap::PlateDesignationFont;
	using APSStrategicMap::PlateDesignationGap;
	using APSStrategicMap::PlateNameFont;
	using APSStrategicMap::PlatePadX;
	using APSStrategicMap::PlateTypeFont;
	using APSStrategicMapViewLocal::Arc;
	using APSStrategicMapViewLocal::Circle;
	using APSStrategicMapViewLocal::Dashes;
	using APSStrategicMapViewLocal::DashedCircle;
	using APSStrategicMapViewLocal::DistanceString;
	using APSStrategicMapViewLocal::Dot;
	using APSStrategicMapViewLocal::FLabel;
	using APSStrategicMapViewLocal::FProjector;
	using APSStrategicMapViewLocal::HaloDashes;
	using APSStrategicMapViewLocal::LayerShows;
	using APSStrategicMapViewLocal::PlatedText;
	using APSStrategicMapViewLocal::Lines;
	using APSStrategicMapViewLocal::MergeDistance;
	using APSStrategicMapViewLocal::Polygon;
	using APSStrategicMapViewLocal::ResolvedBodyShare;
	using APSStrategicMapViewLocal::ResolvedSystemPixels;
	using APSStrategicMapViewLocal::Segment;
	using APSStrategicMapViewLocal::Text;
	using APSStrategicMapViewLocal::WithAlpha;
	Painted.Reset();
	LabelledCount = 0;
	const FAPSStrategicMapScene* Map = Scene.Get();
	FProjector View;
	if (!Map || Map->IsCleanView() || !View.Build(Controller.Get(), AllottedGeometry))
	{
		return LayerId;
	}
	const FGeometry& Geometry = AllottedGeometry;
	const FVector2D Size = Geometry.GetLocalSize();
	if (Size.X < 8.0 || Size.Y < 8.0)
	{
		return LayerId;
	}
	ViewportWidthPixels = View.ViewportPixels.X;
	// The camera keeps its focus in the middle of this region and frames objects in it, not in the whole viewport.
	if (Camera.IsValid())
	{
		const FVector2D Half = View.ViewportPixels * 0.5;
		const FVector2D RegionCentre = (Size * 0.5 - View.PixelOffset) / View.PixelScale;
		Camera->SetRegion(FVector2D((RegionCentre.X - Half.X) / Half.X, (RegionCentre.Y - Half.Y) / Half.X),
			FMath::Min(Size.X / View.PixelScale.X, Size.Y / View.PixelScale.Y) * 0.5 / Half.X);
	}

	const int32 LayerGuides = LayerId;
	const int32 LayerLines = LayerId + 1;
	const int32 LayerMarkers = LayerId + 2;
	const int32 LayerLeaders = LayerId + 3;
	const int32 LayerPlates = LayerId + 4;
	const int32 LayerPlateText = LayerId + 5;
	const int32 LayerTop = LayerId + 6;
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo SmallFont = APSUITheme::BodyFont("Bold", 9);
	const FSlateFontInfo NoteFont = APSUITheme::BodyFont("Regular", 10);
	const FLinearColor Amber = APSChrome::Amber();
	const FLinearColor Cyan = APSChrome::Cyan();
	const FLinearColor Muted = APSChrome::Muted();
	const FSelection& Selected = Map->GetSelection();
	const FSelection& Hovered = Map->Hover;
	UWorld* World = Map->GetWorld();
	const TArray<FObject>& Objects = Map->GetObjects();
	const TArray<FSystemMark>& Systems = Map->GetSystems();
	const double MinSide = FMath::Min(Size.X, Size.Y);
	const auto Inside = [&Size](const FVector2D& Point, const double Margin)
	{
		return Point.X >= -Margin && Point.Y >= -Margin && Point.X <= Size.X + Margin && Point.Y <= Size.Y + Margin;
	};

	// Opaque bodies that cover a few pixels hide what is behind them: markers, orbit arcs.
	TArray<FAPSPreviewOccluder> Occluders;
	TArray<const AActor*> OccluderActors;
	for (const FObject& Object : Objects)
	{
		const AActor* Actor = Object.Actor.Get();
		if (!Actor || Object.RadiusCm <= 0.0
			|| (Object.Kind != EKind::Star && Object.Kind != EKind::Planet && Object.Kind != EKind::Moon))
		{
			continue;
		}
		const FVector Relative = Actor->GetActorLocation() - View.Origin;
		const double Depth = FVector::DotProduct(Relative, View.Forward);
		if (Depth + Object.RadiusCm <= 0.0 || (Depth > 0.0 && View.PixelRadius(Object.RadiusCm, Depth) < 2.0))
		{
			continue;
		}
		// A little under sea level: a colony or a beacon on the ground stays in view on the near side.
		Occluders.Add({Relative, Object.RadiusCm * 0.995});
		OccluderActors.Add(Actor);
	}
	const auto Occluded = [&View, &Occluders, &OccluderActors](const FVector& World, const AActor* Self)
	{
		const FVector Relative = World - View.Origin;
		for (int32 Index = 0; Index < Occluders.Num(); ++Index)
		{
			if (OccluderActors[Index] != Self && Occluders[Index].Occludes(Relative))
			{
				return true;
			}
		}
		return false;
	};

	// Orbits: circles through each planet round its star and each moon round its planet, sampled and projected every
	// paint, cut at the camera plane, behind bodies and at the panel's edge.
	if (Map->IsLayerOn(ELayer::Orbits))
	{
		TArray<FVector2D> Intervals;
		for (const FObject& Object : Objects)
		{
			const AActor* Body = Object.Actor.Get();
			const AActor* Centre = Object.OrbitCentre.Get();
			if (!Body || !Centre || Object.OrbitRadiusCm <= 0.0)
			{
				continue;
			}
			const FVector CentreLocation = Centre->GetActorLocation();
			const double CentreDepth = FVector::DotProduct(CentreLocation - View.Origin, View.Forward);
			const double Extent = CentreDepth > Object.OrbitRadiusCm * 1.01
				? View.PixelRadius(Object.OrbitRadiusCm, CentreDepth) : 1.0e6;
			// A moon's orbit round a distant planet is a dot: its marker is enough.
			if (Extent < 6.0)
			{
				continue;
			}
			FVector2D CentreScreen;
			double Depth = 0.0;
			if (CentreDepth > Object.OrbitRadiusCm * 1.01 && View.Project(CentreLocation, CentreScreen, Depth)
				&& (CentreScreen.X + Extent < 0.0 || CentreScreen.Y + Extent < 0.0
					|| CentreScreen.X - Extent > Size.X || CentreScreen.Y - Extent > Size.Y))
			{
				continue;
			}
			const bool bSelected = Selected.Actor.Get() == Body;
			const FLinearColor Colour = WithAlpha(Object.Colour, bSelected ? 0.75f : Object.Kind == EKind::Moon ? 0.22f : 0.32f);
			const float Thickness = bSelected ? 1.6f : Object.Kind == EKind::Moon ? 0.8f : 1.0f;
			const int32 Samples = FMath::Clamp(FMath::CeilToInt(Extent / 5.0), 48, 192);
			TArray<FVector2f> Polyline;
			const auto Flush = [&]()
			{
				Lines(OutDrawElements, LayerGuides, Geometry, MoveTemp(Polyline), Colour, Thickness);
				Polyline.Reset();
			};
			FVector Previous = FVector::ZeroVector;
			for (int32 Sample = 0; Sample <= Samples; ++Sample)
			{
				const double Angle = UE_TWO_PI * Sample / Samples;
				const FVector Current = CentreLocation + (Object.OrbitAxisX * FMath::Cos(Angle) + Object.OrbitAxisY * FMath::Sin(Angle))
					* Object.OrbitRadiusCm - View.Origin;
				if (Sample > 0)
				{
					FVector A = Previous;
					FVector B = Current;
					FVector2D PanelA;
					FVector2D PanelB;
					if (!View.ClipToNear(A, B))
					{
						Flush();
					}
					else if (!View.ProjectRelative(A, PanelA) || !View.ProjectRelative(B, PanelB)
						|| !APSPreviewVisibility::ClipToPanel(PanelA, PanelB, Size))
					{
						// Rio 03.10 (FPS on the map): a segment off the panel needs no occlusion test. Close to a planet most of
						// the orbits round the camera are off the screen, and each test walks every occluder.
						Flush();
					}
					else
					{
						if (Occluders.IsEmpty())
						{
							Intervals.Reset();
							Intervals.Emplace(0.0, 1.0);
						}
						else
						{
							APSPreviewVisibility::VisibleIntervals(A, B, Occluders, Intervals);
						}
						if (Intervals.IsEmpty())
						{
							Flush();
						}
						for (const FVector2D& Interval : Intervals)
						{
							FVector2D Start;
							FVector2D End;
							if (!View.ProjectRelative(FMath::Lerp(A, B, Interval.X), Start)
								|| !View.ProjectRelative(FMath::Lerp(A, B, Interval.Y), End)
								|| !APSPreviewVisibility::ClipToPanel(Start, End, Size))
							{
								Flush();
								continue;
							}
							if (!Polyline.IsEmpty() && !Polyline.Last().Equals(FVector2f(Start), 0.05f))
							{
								Flush();
							}
							if (Polyline.IsEmpty())
							{
								Polyline.Add(FVector2f(Start));
							}
							Polyline.Add(FVector2f(End));
							if (Interval.Y < 1.0)
							{
								Flush();
							}
						}
					}
				}
				Previous = Current;
			}
			Flush();
		}
	}

	// The relay network: links between claimed systems and each relay's reach.
	if (Map->IsLayerOn(ELayer::Network))
	{
		for (const TPair<int32, int32>& Link : Map->GetLinks())
		{
			FVector A;
			FVector B;
			FVector2D From;
			FVector2D To;
			if (Map->LocateSystem(Link.Key, A) && Map->LocateSystem(Link.Value, B)
				&& View.ProjectSegment(A, B, Size, From, To))
			{
				Segment(OutDrawElements, LayerLines, Geometry, From, To, WithAlpha(APSChrome::Success(), 0.62f), 1.6f);
			}
		}
		for (const FSystemMark& Mark : Systems)
		{
			FVector Location;
			FVector2D Screen;
			double Depth = 0.0;
			if (Mark.ReachCm <= 0.0 || !Map->LocateSystem(Mark.Index, Location) || !View.Project(Location, Screen, Depth))
			{
				continue;
			}
			const double Reach = View.PixelRadius(Mark.ReachCm, Depth);
			if (Reach >= 8.0 && Reach < 6000.0 && Inside(Screen, Reach))
			{
				DashedCircle(OutDrawElements, LayerGuides, Geometry, Screen, Reach, WithAlpha(APSChrome::Success(), 0.18f), 1.0f);
			}
		}
	}

	// Labels collected from systems and objects, then placed together.
	TArray<FLabel> Labels;
	// Merged markers: each drawn marker and how many others landed on it.
	struct FDrawn
	{
		FVector2D Position;
		int32 Merged{0};
		bool bSystem{false};
	};
	TArray<FDrawn> Drawn;
	// Inside the home system a catalogue star never swallows a planet's marker (nor the reverse); out in the cluster the
	// home system's bodies and a system's structures gather into its mark.
	bool bMergeAcross = false;
	const auto MergeInto = [&Drawn, &bMergeAcross](const FVector2D& Position, const bool bSystem) -> FDrawn*
	{
		for (FDrawn& Entry : Drawn)
		{
			if ((bMergeAcross || Entry.bSystem == bSystem)
				&& FVector2D::DistSquared(Entry.Position, Position) < MergeDistance * MergeDistance)
			{
				return &Entry;
			}
		}
		return nullptr;
	};
	FVector OffscreenLocation = FVector::ZeroVector;
	bool bSelectedOffscreen = false;
	const auto MarkOffscreen = [&](const FVector& Location, const bool bIsSelected)
	{
		if (bIsSelected)
		{
			OffscreenLocation = Location;
			bSelectedOffscreen = true;
		}
	};

	// Is the home system resolved on screen? Then its star, planets and moons speak for it; else its mark does.
	bool bHomeResolved = true;
	for (const FSystemMark& Mark : Systems)
	{
		FVector Location;
		if (Mark.bHome && Map->LocateSystem(Mark.Index, Location))
		{
			const double Distance = FVector::Dist(Location, View.Origin);
			bHomeResolved = Distance <= Mark.RoomCm || View.PixelRadius(Mark.RoomCm, Distance) >= ResolvedSystemPixels;
		}
	}

	const auto PaintSystems = [&]()
	{
		const bool bLayer = Map->IsLayerOn(ELayer::Systems);
		for (const FSystemMark& Mark : Systems)
		{
			const bool bSelected = Selected.SystemIndex == Mark.Index;
			const bool bHovered = Hovered.SystemIndex == Mark.Index;
			if (!bLayer && !bSelected)
			{
				continue;
			}
			FVector Location;
			if (!Map->LocateSystem(Mark.Index, Location))
			{
				continue;
			}
			FVector2D Screen;
			double Depth = 0.0;
			if (!View.Project(Location, Screen, Depth) || !Inside(Screen, 4.0))
			{
				MarkOffscreen(Location, bSelected);
				continue;
			}
			const double RoomPixels = View.PixelRadius(Mark.RoomCm, Depth);
			if (Mark.bHome && bHomeResolved && !bSelected)
			{
				continue;
			}
			if (bSelected && RoomPixels >= 6.0 && RoomPixels < 8000.0)
			{
				// The selected system's sphere: its room among its neighbours.
				DashedCircle(OutDrawElements, LayerGuides, Geometry, Screen, RoomPixels, WithAlpha(Mark.Colour, 0.5f), 1.2f);
			}
			if (FDrawn* Occupant = !bSelected && !bHovered ? MergeInto(Screen, true) : nullptr)
			{
				++Occupant->Merged;
				continue;
			}
			Drawn.Add({Screen, 0, true});
			const double Half = Mark.StarCount > 2 ? 5.5 : Mark.StarCount > 1 ? 4.8 : 4.0;
			const FLinearColor Colour = WithAlpha(Mark.Colour, Mark.Knowledge == APSStars::EKnowledge::Catalogued && !Mark.bHome
				&& !Mark.bClaimed ? 0.72f : 1.0f);
			Polygon(OutDrawElements, LayerMarkers, Geometry, Screen, {FVector2D(0.0, -Half), FVector2D(Half, 0.0),
				FVector2D(0.0, Half), FVector2D(-Half, 0.0)}, Colour, 1.5f);
			Dot(OutDrawElements, LayerMarkers, Geometry, Screen, 1.6, Colour);
			if (Mark.bClaimed || Mark.bHome)
			{
				// A lit beacon: the civilization holds the system.
				Circle(OutDrawElements, LayerMarkers, Geometry, Screen, Half + 4.5,
					WithAlpha(Mark.bHome ? Mark.Colour : APSChrome::Success(), 0.9f), 1.4f);
			}
			if (Mark.Anomaly > 0)
			{
				// Its deep-space anomaly: bright while it waits (detected, located), dim once investigated.
				const FVector2D Badge = Screen + FVector2D(Half + 5.0, -Half - 4.0);
				const FLinearColor AnomalyColour(1.0f, 0.62f, 0.2f, Mark.Anomaly >= 3 ? 0.4f : 1.0f);
				Polygon(OutDrawElements, LayerMarkers, Geometry, Badge, {FVector2D(0.0, -4.0), FVector2D(4.0, 3.0),
					FVector2D(-4.0, 3.0)}, AnomalyColour, 1.4f);
			}
			if (bSelected)
			{
				Circle(OutDrawElements, LayerTop, Geometry, Screen, Half + 8.0, Amber, 2.0f);
			}
			else if (bHovered)
			{
				Circle(OutDrawElements, LayerTop, Geometry, Screen, Half + 7.0, Cyan, 1.3f);
			}
			Painted.Add({FSelection::OfSystem(Mark.Index), Screen, static_cast<float>(Half + 4.0)});
			if (Map->IsLayerOn(ELayer::Labels) || bSelected || bHovered)
			{
				FLabel& Label = Labels.AddDefaulted_GetRef();
				Label.Plate = &Mark.Plate;
				Label.Type = &Mark.Type;
				Label.Name = &Mark.Name;
				Label.Colour = Mark.Colour;
				Label.Anchor = Screen;
				Label.RingRadius = static_cast<float>(Half + 3.0);
				Label.bSelected = bSelected;
				Label.bHovered = bHovered;
				// Out among the stars the systems come first; inside the home system its bodies do.
				Label.Rank = bHomeResolved ? (Mark.bClaimed ? 6 : Mark.Knowledge != APSStars::EKnowledge::Catalogued ? 7 : 9)
					: (Mark.bHome ? 0 : Mark.bClaimed ? 1 : Mark.Knowledge != APSStars::EKnowledge::Catalogued ? 3 : 6);
			}
		}
	};

	// Objects: the docked plate (a body filling the view) and every marker.
	int32 DockedLabel = INDEX_NONE;
	double DockedPixels = 0.0;
	const auto PaintObjects = [&]()
	{
		for (const FObject& Object : Objects)
		{
			AActor* Actor = Object.Actor.Get();
			if (!Actor)
			{
				continue;
			}
			const bool bSelected = Selected.Actor.Get() == Actor;
			const bool bHovered = Hovered.Actor.Get() == Actor;
			if (!bSelected && !LayerShows(*Map, Object))
			{
				continue;
			}
			const FVector Location = Actor->GetActorLocation();
			FVector2D Screen;
			double Depth = 0.0;
			if (!View.Project(Location, Screen, Depth))
			{
				MarkOffscreen(Location, bSelected);
				continue;
			}
			const double BodyPixels = View.PixelRadius(Object.RadiusCm, Depth);
			if (!Inside(Screen, BodyPixels + 4.0))
			{
				MarkOffscreen(Location, bSelected);
				continue;
			}
			if (!bSelected && Occluded(Location, Actor))
			{
				continue;
			}
			const bool bBody = Object.Kind == EKind::Star || Object.Kind == EKind::Planet || Object.Kind == EKind::Moon;
			if (!bSelected && !bHovered && !Object.bPlayer && BodyPixels < 4.0)
			{
				if (FDrawn* Occupant = MergeInto(Screen, false))
				{
					++Occupant->Merged;
					continue;
				}
			}
			Drawn.Add({Screen, 0, false});
			const FLinearColor Colour = Object.Colour;
			// The ring outlines the limb with room (the menu's rule) and gives way once the body is seen itself.
			float RingRadius = static_cast<float>(FMath::Max(7.0, BodyPixels * 1.15 + 6.0));
			const bool bResolved = BodyPixels > MinSide * ResolvedBodyShare;
			switch (Object.Kind)
			{
			case EKind::Star:
			case EKind::Planet:
			case EKind::Moon:
				if (!bResolved)
				{
					Circle(OutDrawElements, LayerMarkers, Geometry, Screen, RingRadius, WithAlpha(Colour, 0.85f),
						Object.Kind == EKind::Moon ? 1.0f : 1.3f);
					if (BodyPixels < 1.5)
					{
						Dot(OutDrawElements, LayerMarkers, Geometry, Screen, Object.Kind == EKind::Star ? 2.6 : 1.8, Colour);
					}
				}
				break;
			case EKind::Station:
			{
				const double Half = FMath::Max(4.5, BodyPixels * 1.1);
				Polygon(OutDrawElements, LayerMarkers, Geometry, Screen, {FVector2D(-Half, -Half), FVector2D(Half, -Half),
					FVector2D(Half, Half), FVector2D(-Half, Half)}, Colour, 1.5f);
				Dot(OutDrawElements, LayerMarkers, Geometry, Screen, 1.5, Colour);
				RingRadius = static_cast<float>(Half + 3.0);
				break;
			}
			case EKind::Outpost:
			{
				const double Half = FMath::Max(5.0, BodyPixels * 1.2);
				Polygon(OutDrawElements, LayerMarkers, Geometry, Screen, {FVector2D(0.0, -Half), FVector2D(Half, 0.0),
					FVector2D(0.0, Half), FVector2D(-Half, 0.0)}, Colour, 1.5f);
				RingRadius = static_cast<float>(Half + 2.0);
				break;
			}
			case EKind::Colony:
				Dot(OutDrawElements, LayerMarkers, Geometry, Screen, 3.0, Colour);
				Circle(OutDrawElements, LayerMarkers, Geometry, Screen, FMath::Max(7.0, BodyPixels * 1.1), Colour, 1.3f);
				RingRadius = static_cast<float>(FMath::Max(8.0, BodyPixels * 1.1 + 1.0));
				break;
			case EKind::Anomaly:
			{
				const double Half = 5.5;
				Polygon(OutDrawElements, LayerMarkers, Geometry, Screen, {FVector2D(0.0, -Half), FVector2D(Half, Half * 0.8),
					FVector2D(-Half, Half * 0.8)}, Colour, 1.5f);
				Dot(OutDrawElements, LayerMarkers, Geometry, Screen, 1.3, Colour);
				RingRadius = 8.0f;
				break;
			}
			case EKind::Ship:
			{
				// A chevron along the hull's heading on the screen.
				FVector2D Heading(0.0, -1.0);
				FVector2D Ahead;
				double AheadDepth = 0.0;
				if (View.Project(Location + Actor->GetActorForwardVector() * FMath::Max(Depth * 0.02, 1000.0), Ahead, AheadDepth)
					&& FVector2D::DistSquared(Ahead, Screen) > 0.25)
				{
					Heading = (Ahead - Screen).GetSafeNormal();
				}
				const FVector2D Side(-Heading.Y, Heading.X);
				const double Tip = FMath::Max(7.0, BodyPixels * 1.1);
				const FVector2D Nose = Heading * Tip;
				Polygon(OutDrawElements, LayerMarkers, Geometry, Screen, {Nose, -Heading * Tip * 0.55 + Side * Tip * 0.7,
					-Heading * Tip * 0.2, -Heading * Tip * 0.55 - Side * Tip * 0.7}, Colour, 1.6f);
				RingRadius = static_cast<float>(Tip + 3.0);
				if (Object.bPlayer)
				{
					Circle(OutDrawElements, LayerMarkers, Geometry, Screen, Tip + 5.0, WithAlpha(Colour, 0.8f), 1.2f);
					RingRadius += 2.0f;
				}
				break;
			}
			case EKind::Pilot:
				Circle(OutDrawElements, LayerMarkers, Geometry, Screen, 8.0, Colour, 1.5f);
				Dot(OutDrawElements, LayerMarkers, Geometry, Screen, 2.5, Colour);
				RingRadius = 10.0f;
				break;
			}
			if (!bBody && BodyPixels > RingRadius)
			{
				// Close enough to see the structure itself: outline its size too.
				Circle(OutDrawElements, LayerMarkers, Geometry, Screen, BodyPixels * 1.1, WithAlpha(Colour, 0.6f), 1.0f);
				RingRadius = static_cast<float>(BodyPixels * 1.1);
			}
			const float HighlightRadius = bResolved ? static_cast<float>(FMath::Min(BodyPixels * 1.04 + 4.0, MinSide)) : RingRadius + 4.0f;
			if (bSelected)
			{
				Circle(OutDrawElements, LayerTop, Geometry, Screen, HighlightRadius, Amber, 2.0f);
			}
			else if (bHovered)
			{
				Circle(OutDrawElements, LayerTop, Geometry, Screen, HighlightRadius, Cyan, 1.3f);
			}
			Painted.Add({FSelection::OfActor(Actor), Screen, static_cast<float>(FMath::Max<double>(RingRadius, BodyPixels))});
			const bool bWanted = Map->IsLayerOn(ELayer::Labels) || bSelected || bHovered || Object.bPlayer;
			// Out in the cluster the home system's bodies sit in its mark; only the chosen ones keep a plate.
			if (!bWanted || (!bHomeResolved && !bSelected && !bHovered && !Object.bPlayer))
			{
				continue;
			}
			FLabel& Label = Labels.AddDefaulted_GetRef();
			Label.Plate = &Object.Plate;
			Label.Type = &Object.Type;
			Label.Name = &Object.Name;
			Label.Designation = &Object.Designation;
			Label.Colour = Colour;
			Label.RingRadius = RingRadius;
			Label.bSelected = bSelected;
			Label.bHovered = bHovered;
			Label.Rank = Object.Priority;
			Label.Anchor = Screen;
			if (bResolved)
			{
				// A large body: the plate stands over its limb; one that fills the view docks its plate in the corner.
				Label.Anchor = FVector2D(FMath::Clamp(Screen.X, 0.0, Size.X), FMath::Clamp(Screen.Y - BodyPixels, 0.0, Size.Y));
				Label.RingRadius = 0.0f;
				if (BodyPixels > MinSide * 0.42 && (DockedLabel == INDEX_NONE || bSelected || BodyPixels > DockedPixels))
				{
					DockedLabel = Labels.Num() - 1;
					DockedPixels = BodyPixels;
				}
			}
		}
	};
	bMergeAcross = !bHomeResolved;
	if (bHomeResolved)
	{
		PaintObjects();
		PaintSystems();
	}
	else
	{
		PaintSystems();
		PaintObjects();
	}

	// Merged markers say how many more stand there.
	for (const FDrawn& Entry : Drawn)
	{
		if (Entry.Merged > 0)
		{
			Text(OutDrawElements, LayerMarkers, Geometry, Entry.Position + FVector2D(7.0, -17.0),
				TEXT("+") + APSUINumber::Number(Entry.Merged).ToString(), SmallFont, WithAlpha(Muted, 0.9f));
		}
	}

	// Fleet routes: a dashed line from each ship under way to its target, a progress arc round a ship at work.
	if (Map->IsLayerOn(ELayer::Routes) && World)
	{
		if (const FAPSFleetCommand* Fleet = APSFleetFind(World))
		{
			for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
			{
				const ASpaceship* Ship = Unit.Ship.Get();
				if (!Ship)
				{
					continue;
				}
				const FLinearColor Colour = APSFleet::DivisionColour(Unit.Division);
				if (Unit.Phase == APSFleet::EPhase::Departing || Unit.Phase == APSFleet::EPhase::Transit)
				{
					const AActor* Goal = Unit.Order == APSFleet::EOrder::Return ? Unit.Berth.Get() : Unit.Target.Get();
					FVector2D From;
					FVector2D To;
					if (!Goal || !View.ProjectSegment(Ship->GetActorLocation(), Goal->GetActorLocation(), Size, From, To))
					{
						continue;
					}
					Dashes(OutDrawElements, LayerLines, Geometry, From, To, WithAlpha(Colour, 0.85f), 7.0, 5.0, 1.4f);
					FVector2D GoalScreen;
					double GoalDepth = 0.0;
					if (View.Project(Goal->GetActorLocation(), GoalScreen, GoalDepth) && Inside(GoalScreen, 0.0))
					{
						Circle(OutDrawElements, LayerLines, Geometry, GoalScreen, 10.0, WithAlpha(Colour, 0.8f), 1.2f);
					}
					if (FVector2D::Distance(From, To) > 140.0)
					{
						// Who goes and how far is left, along the line.
						Text(OutDrawElements, LayerLines, Geometry, FMath::Lerp(From, To, 0.5) + FVector2D(6.0, -16.0),
							FString::Printf(TEXT("%s  /  %s"), *Unit.CallSign, *DistanceString(Unit.RemainingCm)), SmallFont,
							WithAlpha(Colour, 0.95f));
					}
				}
				else if (Unit.Phase == APSFleet::EPhase::Working)
				{
					FVector2D Screen;
					double Depth = 0.0;
					if (View.Project(Ship->GetActorLocation(), Screen, Depth) && Inside(Screen, 0.0))
					{
						Arc(OutDrawElements, LayerLines, Geometry, Screen, 13.0, Unit.Progress, Colour);
					}
				}
			}
		}
		// The pilot's own course: the autopilot's target, else the navigation course.
		if (const ASpaceship* Piloted = Cast<ASpaceship>(Map->GetPilot()))
		{
			FVector Goal = FVector::ZeroVector;
			bool bGoal = false;
			bool bAutopilot = false;
			if (const AActor* Target = Piloted->FlightModel ? Piloted->FlightModel->GetAutopilotTarget() : nullptr)
			{
				Goal = Target->GetActorLocation();
				bGoal = bAutopilot = true;
			}
			else if (const FShipNavigationContact* Contact = Piloted->ShipNavigation
				? Piloted->ShipNavigation->GetSelectedContact() : nullptr)
			{
				Goal = Contact->GetWorldLocation();
				bGoal = true;
			}
			FVector2D From;
			FVector2D To;
			if (bGoal && View.ProjectSegment(Piloted->GetActorLocation(), Goal, Size, From, To))
			{
				// The course in cyan (amber under the autopilot), on a dark halo and its distance on a plate.
				const FLinearColor Colour = bAutopilot ? Amber : Cyan;
				HaloDashes(OutDrawElements, LayerLines, Geometry, From, To, WithAlpha(Colour, 0.95f), 10.0, 6.0, 2.0f);
				if (FVector2D::Distance(From, To) > 120.0)
				{
					PlatedText(OutDrawElements, LayerLines, Geometry, FMath::Lerp(From, To, 0.35) + FVector2D(10.0, 6.0),
						FString::Printf(TEXT("%s  /  %s"), bAutopilot ? TEXT("AUTOPILOT") : TEXT("COURSE"),
							*DistanceString(FVector::Dist(Piloted->GetActorLocation(), Goal))), SmallFont, Colour);
				}
			}
		}
	}

	// The way from the pilot to the selection, and how long it is.
	if (const APawn* Pilot = Map->GetPilot(); Pilot && Selected.IsSet() && Selected.Actor.Get() != Pilot)
	{
		FVector Target;
		FVector2D From;
		FVector2D To;
		if (Map->Locate(Selected, Target) && View.ProjectSegment(Pilot->GetActorLocation(), Target, Size, From, To)
			&& FVector2D::Distance(From, To) > 24.0)
		{
			// Rio 04.10 ("the line from the start planet to me is hard to see on a light background"): brighter, wider dashes
			// on their dark halo.
			HaloDashes(OutDrawElements, LayerGuides, Geometry, From, To, WithAlpha(APSChrome::White(), 0.92f), 4.0, 4.0, 1.6f);
			PlatedText(OutDrawElements, LayerGuides, Geometry, FMath::Lerp(From, To, 0.55) + FVector2D(12.0, 4.0),
				DistanceString(FVector::Dist(Pilot->GetActorLocation(), Target)), SmallFont, WithAlpha(APSChrome::White(), 0.9f));
		}
	}

	// Plates: the selection first, then the hovered object, then by rank; the menu's non-overlapping placement.
	if (Labels.IsValidIndex(DockedLabel))
	{
		Labels[DockedLabel].bDocked = true;
	}
	// Rio 04.10 ("the labels jump back and forth as the mouse passes over the stars"): the hovered object no longer goes
	// first, which re-laid every plate several times a second; it keeps its own place, or floats over the rest when it
	// has none (below).
	Labels.StableSort([](const FLabel& A, const FLabel& B)
	{
		const int32 RankA = A.bSelected ? -2 : A.Rank;
		const int32 RankB = B.bSelected ? -2 : B.Rank;
		return RankA < RankB;
	});
	const auto DrawPlate = [&](const FLabel& Label, const FVector2D& LocalPosition, const bool bLeader)
	{
		if (!Label.Plate || !Label.Name)
		{
			return;
		}
		++LabelledCount;
		const FPlate& Plate = *Label.Plate;
		const FVector2D PlateSize = Plate.Size;
		// The plate and its texts share one origin on the pixel grid, so they never drift apart while the camera turns.
		const FVector2D Absolute = Geometry.LocalToAbsolute(LocalPosition);
		const FVector2D Position = Geometry.AbsoluteToLocal(FVector2D(FMath::RoundToDouble(Absolute.X),
			FMath::RoundToDouble(Absolute.Y)));
		const FVector2D Anchor = Label.Anchor;
		const FLinearColor Marker = Label.bSelected ? Amber : Label.Colour;
		if (bLeader)
		{
			// The leader runs from the ring to the nearest point of the plate.
			const FVector2D Nearest(FMath::Clamp(Anchor.X, Position.X, Position.X + PlateSize.X),
				FMath::Clamp(Anchor.Y, Position.Y, Position.Y + PlateSize.Y));
			if (FVector2D::Distance(Nearest, Anchor) > Label.RingRadius + 2.0)
			{
				const FVector2D Start = Anchor + (Nearest - Anchor).GetSafeNormal() * Label.RingRadius;
				Segment(OutDrawElements, LayerLeaders, Geometry, Start, Nearest, WithAlpha(Marker, 0.72f), 1.0f);
			}
		}
		FSlateDrawElement::MakeBox(OutDrawElements, LayerPlates, Geometry.ToPaintGeometry(PlateSize,
			FSlateLayoutTransform(Position)), WhiteBrush, ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.002f, 0.014f, 0.026f, 0.94f)));
		FSlateDrawElement::MakeBox(OutDrawElements, LayerPlateText, Geometry.ToPaintGeometry(FVector2D(PlateBar, PlateSize.Y),
			FSlateLayoutTransform(Position)), WhiteBrush, ESlateDrawEffect::None, Marker);
		if (Label.bSelected)
		{
			Lines(OutDrawElements, LayerPlateText, Geometry, TArray<FVector2f>{FVector2f(Position),
				FVector2f(Position + FVector2D(PlateSize.X, 0.0)), FVector2f(Position + PlateSize),
				FVector2f(Position + FVector2D(0.0, PlateSize.Y)), FVector2f(Position)}, Amber, 2.0f);
		}
		const float TextLeft = PlateBar + PlatePadX;
		if (Label.Type && !Label.Type->IsEmpty())
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerPlateText, Geometry.ToPaintGeometry(
				FVector2D(PlateSize.X - TextLeft, 18.0), FSlateLayoutTransform(Position + FVector2D(TextLeft, Plate.TypeTop))),
				*Label.Type, PlateTypeFont(), ESlateDrawEffect::None, Muted);
		}
		FSlateDrawElement::MakeText(OutDrawElements, LayerPlateText, Geometry.ToPaintGeometry(
			FVector2D(Plate.NameWidth + 2.0, 20.0), FSlateLayoutTransform(Position + FVector2D(TextLeft, Plate.NameTop))),
			*Label.Name, PlateNameFont(), ESlateDrawEffect::None, Label.bSelected ? Amber : APSChrome::White());
		if (Label.Designation && !Label.Designation->IsEmpty())
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerPlateText, Geometry.ToPaintGeometry(FVector2D(PlateSize.X, 20.0),
				FSlateLayoutTransform(Position + FVector2D(TextLeft + Plate.NameWidth + PlateDesignationGap, Plate.DesignationTop))),
				*Label.Designation, PlateDesignationFont(), ESlateDrawEffect::None, Marker);
		}
	};
	TArray<FAPSPreviewAnnotationCandidate> Candidates;
	Candidates.Reserve(Labels.Num());
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		const FLabel& Label = Labels[Index];
		if (Label.bDocked)
		{
			// Under the region's top edge, on its left: no ring and no leader (the menu's docked plate).
			DrawPlate(Label, FVector2D(10.0, 8.0), false);
			continue;
		}
		Candidates.Add({Index, Label.Anchor, Label.bSelected, false,
			Label.Plate ? Label.Plate->Size : APSPreviewAnnotationLayout::LabelSize()});
	}
	bool bHoveredPlaced = false;
	for (const FAPSPreviewAnnotationPlacement& Placement : APSPreviewAnnotationLayout::Arrange(MoveTemp(Candidates), Size))
	{
		if (Placement.bHasLabel && Labels.IsValidIndex(Placement.Candidate.EntryIndex))
		{
			DrawPlate(Labels[Placement.Candidate.EntryIndex], Placement.LabelPosition, true);
			bHoveredPlaced |= Labels[Placement.Candidate.EntryIndex].bHovered;
		}
	}
	if (!bHoveredPlaced)
	{
		// The hovered object without room of its own: its plate beside its mark, over the others.
		for (const FLabel& Label : Labels)
		{
			if (Label.bHovered && !Label.bDocked && Label.Plate)
			{
				const FVector2D PlateSize = Label.Plate->Size;
				const FVector2D At(FMath::Clamp(Label.Anchor.X + 14.0, 4.0, FMath::Max(4.0, Size.X - PlateSize.X - 4.0)),
					FMath::Clamp(Label.Anchor.Y - PlateSize.Y * 0.5, 4.0, FMath::Max(4.0, Size.Y - PlateSize.Y - 26.0)));
				DrawPlate(Label, At, true);
				break;
			}
		}
	}

	// The selection off the view: an arrow at the region's edge points to it, with its name.
	if (bSelectedOffscreen)
	{
		const FVector2D Direction = View.Direction(OffscreenLocation);
		if (!Direction.IsNearlyZero())
		{
			const FVector2D Centre = Size * 0.5;
			const FVector2D Inset = Centre - FVector2D(30.0, 30.0);
			const double Scale = FMath::Min(Direction.X != 0.0 ? Inset.X / FMath::Abs(Direction.X) : TNumericLimits<double>::Max(),
				Direction.Y != 0.0 ? Inset.Y / FMath::Abs(Direction.Y) : TNumericLimits<double>::Max());
			const FVector2D Tip = Centre + Direction * Scale;
			const FVector2D Side(-Direction.Y, Direction.X);
			Polygon(OutDrawElements, LayerTop, Geometry, Tip, {Direction * 9.0, -Direction * 7.0 + Side * 8.0,
				-Direction * 7.0 - Side * 8.0}, Amber, 2.0f);
			const FText Name = Map->NameOf(Selected);
			const FVector2D TextAt = Tip - Direction * 22.0 + FVector2D(Direction.X > 0.3 ? -150.0 : 6.0, -8.0);
			Text(OutDrawElements, LayerTop, Geometry, TextAt, Name.ToString(), SmallFont, Amber);
		}
	}

	// The pivot: where the camera circles.
	if (Camera.IsValid())
	{
		FVector2D Pivot;
		double PivotDepth = 0.0;
		if (View.Project(Camera->GetLookLocation(), Pivot, PivotDepth) && Inside(Pivot, -2.0))
		{
			const FLinearColor PivotColour = WithAlpha(Cyan, 0.45f);
			Segment(OutDrawElements, LayerGuides, Geometry, Pivot - FVector2D(6.0, 0.0), Pivot - FVector2D(2.0, 0.0), PivotColour, 1.0f);
			Segment(OutDrawElements, LayerGuides, Geometry, Pivot + FVector2D(2.0, 0.0), Pivot + FVector2D(6.0, 0.0), PivotColour, 1.0f);
			Segment(OutDrawElements, LayerGuides, Geometry, Pivot - FVector2D(0.0, 6.0), Pivot - FVector2D(0.0, 2.0), PivotColour, 1.0f);
			Segment(OutDrawElements, LayerGuides, Geometry, Pivot + FVector2D(0.0, 2.0), Pivot + FVector2D(0.0, 6.0), PivotColour, 1.0f);
		}

		// Scale bar at the focus' depth: a round length near 140 px.
		const double CmPerUnit = Camera->GetDistance() / FMath::Max(View.ScaleX, 1.0e-6);
		const double Wanted = CmPerUnit * 140.0;
		if (FMath::IsFinite(Wanted) && Wanted > 0.0)
		{
			const bool bAu = Wanted >= 0.05 * APSStars::AstronomicalUnitCm;
			const bool bKm = !bAu && Wanted >= 100000.0;
			const double UnitCm = bAu ? APSStars::AstronomicalUnitCm : bKm ? 100000.0 : 100.0;
			const double InUnits = Wanted / UnitCm;
			const double Power = FMath::Pow(10.0, FMath::FloorToDouble(FMath::LogX(10.0, InUnits)));
			const double Leading = InUnits / Power;
			const double Nice = (Leading >= 5.0 ? 5.0 : Leading >= 2.0 ? 2.0 : 1.0) * Power;
			const double Length = Nice * UnitCm / CmPerUnit;
			const FVector2D Start(18.0, Size.Y - 18.0);
			const FVector2D End = Start + FVector2D(Length, 0.0);
			const FLinearColor ScaleColour = WithAlpha(Muted, 0.85f);
			Segment(OutDrawElements, LayerTop, Geometry, Start, End, ScaleColour, 1.4f);
			Segment(OutDrawElements, LayerTop, Geometry, Start - FVector2D(0.0, 5.0), Start + FVector2D(0.0, 1.0), ScaleColour, 1.4f);
			Segment(OutDrawElements, LayerTop, Geometry, End - FVector2D(0.0, 5.0), End + FVector2D(0.0, 1.0), ScaleColour, 1.4f);
			FNumberFormattingOptions Options;
			Options.SetMaximumFractionalDigits(Nice < 1.0 ? 2 : 0);
			Text(OutDrawElements, LayerTop, Geometry, Start + FVector2D(0.0, -21.0),
				APSUINumber::Number(Nice, &Options).ToString() + (bAu ? TEXT(" AU") : bKm ? TEXT(" KM") : TEXT(" M")), SmallFont,
				ScaleColour);
		}
	}
	// How much is drawn and labelled, when some markers go without a plate.
	if (LabelledCount < Labels.Num())
	{
		Text(OutDrawElements, LayerTop, Geometry, FVector2D(FMath::Max(Size.X - 330.0, 170.0), Size.Y - 24.0),
			FText::Format(LOCTEXT("Density", "{0} MARKERS  /  {1} LABELS  /  ZOOM IN FOR MORE"),
				APSUINumber::Number(Painted.Num()), APSUINumber::Number(LabelledCount)).ToString(), NoteFont, WithAlpha(Muted, 0.8f));
	}
	return LayerTop;
}

bool SAPSStrategicMapView::Pick(const FVector2D& LocalPosition, APSStrategicMap::FSelection& OutTarget) const
{
	double Best = TNumericLimits<double>::Max();
	for (const FPainted& Entry : Painted)
	{
		const double Distance = FVector2D::Distance(Entry.Position, LocalPosition);
		double Score = TNumericLimits<double>::Max();
		if (Distance <= APSStrategicMapViewLocal::PickReach)
		{
			Score = Distance;
		}
		else if (Distance <= Entry.Radius)
		{
			// Anywhere on a large body picks it, unless a small marker is within reach.
			Score = APSStrategicMapViewLocal::PickReach + Distance / FMath::Max(static_cast<double>(Entry.Radius), 1.0);
		}
		if (Score < Best)
		{
			Best = Score;
			OutTarget = Entry.Target;
		}
	}
	return Best < TNumericLimits<double>::Max();
}

bool SAPSStrategicMapView::PickGalaxyStar(const FGeometry& Geometry, const FVector2D& LocalPosition,
	APSStrategicMap::FSelection& OutTarget) const
{
	// Rio 04.10 ("how do I set a course to a star at the far end of the galaxy?"): a click on the star field, away from
	// every mark, takes the drawn catalogue star it points at and makes it a star system there and then, selected: the
	// course, the autopilot and the drive work on it as on any other.
	APlayerController* PlayerController = Controller.Get();
	UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
	FAPSStarSystems* Stars = APSStarSystemsFind(World);
	APSStrategicMapViewLocal::FProjector View;
	if (!World || !Stars || !Stars->IsReady() || !View.Build(PlayerController, Geometry))
	{
		return false;
	}
	const FVector2D Pixel = (LocalPosition - View.PixelOffset) / View.PixelScale;
	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(Pixel.X, Pixel.Y, RayOrigin, RayDirection))
	{
		return false;
	}
	APSGalaxyGpuStars::FNearStar Star;
	const double MaxAngle = APSStrategicMapViewLocal::PickReach / FMath::Max(View.ScaleX, 1.0e-6);
	if (!APSGalaxyGpuStars::PickAlongRay(World, RayOrigin, RayDirection, MaxAngle, Star))
	{
		return false;
	}
	int32 Index = Stars->RegisterGalaxyStar(Star.CatalogIndex);
	if (Index == INDEX_NONE)
	{
		// A star in a cluster system's room (or the home's sphere) is that system's.
		Index = Stars->FindContaining(Star.WorldLocation);
	}
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutTarget = APSStrategicMap::FSelection::OfSystem(Index);
	return true;
}

FReply SAPSStrategicMapView::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FKey Button = MouseEvent.GetEffectingButton();
	if (Button != EKeys::LeftMouseButton && Button != EKeys::RightMouseButton && Button != EKeys::MiddleMouseButton)
	{
		return FReply::Unhandled();
	}
	if (HasMouseCapture())
	{
		return FReply::Handled();
	}
	PressPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	DragButton = Button;
	FReply Reply = FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse)
		.PreventThrottling();
	if (Button == EKeys::LeftMouseButton)
	{
		// A click selects; a drag past a few pixels orbits like the right button.
		Drag = EDrag::Pending;
		return Reply;
	}
	Drag = Button == EKeys::MiddleMouseButton || MouseEvent.IsShiftDown() ? EDrag::Pan : EDrag::Orbit;
	return Reply.UseHighPrecisionMouseMovement(SharedThis(this));
}

FReply SAPSStrategicMapView::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (HasMouseCapture() && Drag != EDrag::None)
	{
		const FVector2D Delta = MouseEvent.GetCursorDelta();
		if (Drag == EDrag::Pending && FVector2D::Distance(Local, PressPosition) > 4.0)
		{
			Drag = EDrag::Orbit;
		}
		if (Camera.IsValid())
		{
			if (Drag == EDrag::Orbit)
			{
				Camera->Orbit(Delta);
			}
			else if (Drag == EDrag::Pan)
			{
				Camera->Pan(Delta, ViewportWidthPixels);
			}
		}
		return FReply::Handled();
	}
	if (Scene.IsValid())
	{
		// Rio 04.10 ("the icons twitch when I hover over the stars"): a hovered mark is drawn on its own (no longer merged
		// into its neighbour) with a wider ring, which could move the pick to the neighbour and back every frame. The
		// hover stays while the cursor is still within reach of the hovered mark.
		const APSStrategicMap::FSelection Current = Scene->Hover;
		const bool bStillOver = Current.IsSet() && Painted.ContainsByPredicate([&Current, &Local](const FPainted& Entry)
		{
			return Entry.Target == Current && FVector2D::Distance(Entry.Position, Local)
				<= FMath::Max(APSStrategicMapViewLocal::PickReach * 1.5, static_cast<double>(Entry.Radius));
		});
		APSStrategicMap::FSelection Under;
		if (!bStillOver)
		{
			Scene->Hover = Pick(Local, Under) ? Under : APSStrategicMap::FSelection();
		}
	}
	return FReply::Unhandled();
}

FReply SAPSStrategicMapView::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}
	if (MouseEvent.GetEffectingButton() != DragButton)
	{
		return FReply::Handled();
	}
	const bool bClick = Drag == EDrag::Pending;
	Drag = EDrag::None;
	if (bClick)
	{
		APSStrategicMap::FSelection Under;
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (Pick(Local, Under) || PickGalaxyStar(MyGeometry, Local, Under))
		{
			OnSelect.ExecuteIfBound(Under);
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAPSStrategicMapView::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	APSStrategicMap::FSelection Under;
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (Pick(Local, Under) || PickGalaxyStar(MyGeometry, Local, Under))
	{
		OnSelect.ExecuteIfBound(Under);
		OnFocus.ExecuteIfBound(Under);
	}
	return FReply::Handled();
}

FReply SAPSStrategicMapView::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (Camera.IsValid())
	{
		Camera->Zoom(MouseEvent.GetWheelDelta(), MouseEvent.IsShiftDown());
	}
	return FReply::Handled();
}

void SAPSStrategicMapView::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Drag = EDrag::None;
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
}

void SAPSStrategicMapView::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseLeave(MouseEvent);
	if (Scene.IsValid() && !HasMouseCapture())
	{
		Scene->Hover = APSStrategicMap::FSelection();
	}
}

FCursorReply SAPSStrategicMapView::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	if (Drag == EDrag::Orbit || Drag == EDrag::Pan)
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	}
	return Scene.IsValid() && Scene->Hover.IsSet() ? FCursorReply::Cursor(EMouseCursor::Hand) : FCursorReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE

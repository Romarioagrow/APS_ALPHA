#pragma once

#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

class AActor;
class APlanetaryBody;
class UTexture2D;
class UWorld;

DECLARE_DELEGATE_OneParam(FOnSurfaceObjectOpen, AActor*);

/**
 * Surface map of the civilization menu (C6, Rio 02.10: "a rotating globe and the unwrapped map with markers"). The
 * planet's own WorldScape profile is sampled off the game thread on a latitude/longitude grid (the sampler the
 * terrain workers and the orbital globe use), coloured with the planet's palette and shaded by its relief. The globe
 * turns on its own (drag to turn it, the real star lights its day side); beside it the whole surface unwrapped. Both
 * carry the markers: the colony, outposts, stations in orbit, ships near the world and the pilot.
 *
 * Only what the civilization knows is shown (Rio 02.10: "only the scanned ones"): an unsurveyed world is a dark disc
 * and map under a scanner's noise with what orbits it, a surveyed one coarse and pale, a studied one in full. The
 * sampled heights, liquid and climate are kept per texel, so a new survey level or look only colours them again.
 */
class APS_ALPHA_API SAPSSurfaceMap final : public SLeafWidget
{
public:
	/** How the surface is coloured. */
	enum class EMode : uint8
	{
		Terrain,
		Realistic,
		Geology,
		Scan
	};

	SLATE_BEGIN_ARGS(SAPSSurfaceMap) : _GlobeOnly(false) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		/** Only the turning globe on a transparent background: no flat map, hints or labels (the ORDERS panel). */
		SLATE_ARGUMENT(bool, GlobeOnly)
		/** A double click on a marker: open its object's page. */
		SLATE_EVENT(FOnSurfaceObjectOpen, OnOpenObject)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAPSSurfaceMap() override;

	/** Shows a world; a new one is sampled in the background (a fraction of a second) once it is surveyed. */
	void SetBody(APlanetaryBody* Body);
	APlanetaryBody* GetBody() const { return Body.Get(); }
	/** The world the pilot is on or nearest to. */
	static APlanetaryBody* FindDefaultBody(UWorld* World);
	/** Re-reads the markers and the survey level; the terminal calls it a few times a second while the map is shown. */
	void RefreshMarkers();

	/** For the side panel: what was sampled and where the pilot stands. */
	FText GetStatusText() const;
	FText GetPilotText() const;
	FText GetLegendText() const;
	bool IsReady() const { return MapTexture.IsValid(); }
	/** A world with a solid surface to show (gas worlds have none). */
	bool HasSurface() const { return Body.IsValid() && !bNoSurface; }

	/** The look of the surface (Rio 02.10: as it is, realistic, geology, anomalies); the sampled fields are coloured again. */
	void SetMode(EMode NewMode);
	EMode GetMode() const { return Mode; }
	static FText ModeName(EMode InMode);
	/** The picked marker for the side panel: what and where it is, and how to open it; a hint without a pick. */
	FText GetSelectionText() const;

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return bGlobeOnly ? FVector2D(220.0, 220.0) : FVector2D(900.0, 420.0);
	}
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const override;

	struct FBake;
	struct FJob;
	struct FSampler;

private:
	enum class EMarker : uint8
	{
		Colony,
		Outpost,
		Station,
		Ship,
		Pilot,
		Anomaly
	};
	struct FMarker
	{
		EMarker Kind{EMarker::Ship};
		/** What a double click opens (the world itself for an anomaly site). */
		TWeakObjectPtr<AActor> Actor;
		/** Unit direction from the centre in the world's own frame (Z north). */
		FVector Direction{FVector::UpVector};
		FString Label;
		FLinearColor Color{FLinearColor::White};
		/** Above the surface, km (orbit markers and flying ships). */
		double AltitudeKm{0.0};
	};
	/** What the civilization knows of the world: fleet command's survey level (everything without fleet command). */
	enum class EKnown : uint8
	{
		Unknown,
		Surveyed,
		Studied
	};

	struct FLayout
	{
		FVector2D GlobeCentre{FVector2D::ZeroVector};
		/** The globe's radius at x1. */
		float GlobeRadius{1.0f};
		/** The globe's square: a zoomed globe is clipped to it. */
		FBox2D GlobeBox{FVector2D::ZeroVector, FVector2D::ZeroVector};
		FBox2D MapBox{FVector2D::ZeroVector, FVector2D::ZeroVector};
	};

	/** Globe on the left, unwrapped map on the right, for the widget size. */
	FLayout LayoutViews(const FVector2D& Size) const;
	/** View basis of the globe: right, up, towards the viewer (world frame of the planet). */
	void GlobeBasis(FVector& OutRight, FVector& OutUp, FVector& OutForward) const;
	static FVector2D MapUV(const FVector& Direction);
	/** The part of the unwrapped map shown: left and top edge and size in map UV (the whole map at x1). */
	void MapWindow(double& OutU0, double& OutV0, double& OutSize) const;
	/** Where a map UV lands in the map box; false when outside the shown window. */
	bool MapPoint(const FVector2D& UV, const FBox2D& MapBox, FVector2D& OutPoint) const;
	/** Where a direction lands on the globe; false on the far side or outside the globe's square. */
	bool GlobePoint(const FVector& Direction, const FLayout& View, const FVector& Right, const FVector& Up, const FVector& Forward,
		FVector2D& OutPoint) const;
	EKnown ReadKnown() const;
	/** Follows a new survey level: the first survey samples the world, a study colours it in full. */
	void UpdateKnown();
	/** Samples the whole surface (when not sampled yet) and colours it for the current look. */
	void StartBaseJob();
	/** Colours the held fields again, or samples new ones when they are not valid yet; the slot keeps the job. */
	void LaunchJob(TSharedPtr<FJob, ESPMode::ThreadSafe>& Slot, const TSharedPtr<FBake, ESPMode::ThreadSafe>& Fields, bool bDetail);
	/** A settled close view is sampled again finer, in the background. */
	void UpdateDetail();
	void DropDetail();
	/** Takes finished jobs' colours into the textures. */
	void UploadJobs();
	/** Paints the scanner's noise over an unsurveyed world: a dark disc or rectangle with drifting lines. */
	void PaintNoSurvey(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId, const FVector2D& Centre,
		float Radius, const FBox2D* Box) const;
	/** The marker drawn nearest a screen position on either view (within a few pixels), or INDEX_NONE. */
	int32 PickMarker(const FGeometry& Geometry, const FVector2D& ScreenPosition) const;
	/** The picked marker in Markers (it is found again after every refresh by its actor and label), or INDEX_NONE. */
	int32 SelectedIndex() const;

	FOnSurfaceObjectOpen OnOpenObject;
	TWeakObjectPtr<AActor> SelectedActor;
	FString SelectedLabel;
	/** A detected anomaly whose site is not located yet. */
	FText AnomalyNote;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APlanetaryBody> Body;
	bool bGlobeOnly{false};
	EMode Mode{EMode::Terrain};
	EKnown Known{EKnown::Studied};
	/** The world's profile and seeded noise, shared with the workers. */
	TSharedPtr<const FSampler, ESPMode::ThreadSafe> Sampler;
	/** The whole surface once sampled: heights, liquid and climate per texel. */
	TSharedPtr<FBake, ESPMode::ThreadSafe> BaseFields;
	TSharedPtr<FJob, ESPMode::ThreadSafe> BaseJob;
	/** A UTexture2D; held as UObject so includers need no texture header. */
	TStrongObjectPtr<UObject> MapTexture;
	FSlateBrush MapBrush;
	/** What the shown colours were made for. */
	EMode PaintedMode{EMode::Terrain};
	bool bPaintedCoarse{false};
	/** The share of the texture width that holds one full turn of longitude. */
	float MapTurnU{1.0f};
	/** The zoomed view sampled finer (Rio 02.10: "the camera does not come closer"), drawn over the base texture. */
	TSharedPtr<FBake, ESPMode::ThreadSafe> DetailFields;
	TSharedPtr<FJob, ESPMode::ThreadSafe> DetailJob;
	TStrongObjectPtr<UObject> DetailTexture;
	FSlateBrush DetailBrush;
	EMode DetailPaintedMode{EMode::Terrain};
	/** The zoom the shown and the pending detail were made for. */
	float DetailZoom{1.0f};
	float DetailJobZoom{1.0f};
	FText BodyTitle;
	FText BakeSummary;
	bool bNoSurface{false};
	TArray<FMarker> Markers;
	/**
	 * The view both halves share (Rio 02.10: "both maps linked, the globe and the plane zoomed"): the longitude and
	 * latitude at the centre, radians, and the zoom, 1..12. The globe looks at the centre; the map shows a window
	 * around it once closer than x1.
	 */
	double CentreLongitude{0.0};
	double CentreLatitude{0.35};
	float Zoom{1.0f};
	double LastInputSeconds{-100.0};
	/** Markers are read again once a second while the map is shown (Slate time). */
	double NextMarkerRefreshSeconds{0.0};
	double LastViewChangeSeconds{-100.0};
	bool bDragging{false};
	/** A press that has not moved yet may still be a click. */
	bool bPressMoved{false};
	/** Where the drag started: 0 the globe, 1 the map. */
	int32 DragView{0};
	FVector2D DragLast{FVector2D::ZeroVector};
	FVector2D PressPosition{FVector2D::ZeroVector};
	/** The star's direction in the planet's frame, for the day side. */
	FVector SunDirection{FVector(1.0, 0.0, 0.25).GetSafeNormal()};
	mutable TArray<FSlateVertex> GlobeVertices;
	mutable TArray<SlateIndex> GlobeIndices;
	mutable TArray<FSlateVertex> DetailVertices;
	mutable TArray<SlateIndex> DetailIndices;
};

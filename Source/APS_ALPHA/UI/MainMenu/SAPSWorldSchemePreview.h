#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

/** One planet the scheme draws, in orbit order around its star. */
struct FAPSWorldSchemePlanet
{
	/** EPlanetType display name ("Forest Planet") or bare enum name ("Forest"). */
	FString Type;
	/** Orbit radius in any unit (AU from sidecars); zero spaces the orbits evenly. */
	double Orbit{0.0};
	/** Zero sizes the disc by its type. */
	int32 RadiusKm{0};
	int32 Moons{0};
	/** 0 = the central star; 1 and 2 = drawn companions. */
	int32 Star{0};
	bool bInhabited{false};
};

/** What one saved world's scheme is drawn from: only what the browser scan already holds (the .apsmeta sidecar). */
struct FAPSWorldSchemeInput
{
	/** Seeds the cosmetic parts (orbit phases, tilt): the same slot always paints the same picture. */
	FString Key;
	/** Spectral class: "G", "K"... or "NS", "PS", "BH"; empty when unknown. */
	FString StarClass;
	/** EStellarType display name ("Super Giant") when recorded; empty otherwise. */
	FString StellarType;
	int32 StarCount{1};
	/** The recorded home system (sidecar version 2). Without it the scheme is illustrative, derived from Key. */
	bool bRecorded{false};
	TArray<FAPSWorldSchemePlanet> Planets;
	/** Index into Planets of the home world; INDEX_NONE when unknown. */
	int32 HomeIndex{INDEX_NONE};
};

namespace APSWorldScheme
{
	/** "G - Yellow" or "G" -> "G"; "Neutron Star"/"NS" -> "NS"; "Proto Star"/"PS" -> "PS"; "Black Hole"/"BH" -> "BH". */
	FString StarClassFromLabel(const FString& SpectralLabel);
	/** "G YELLOW STAR", "M RED DWARF" (main sequence recorded), "K ORANGE GIANT", "BLACK HOLE"; "UNKNOWN STAR". */
	FString DescribeStar(const FString& StarClass, const FString& StellarType);
	/** The colour the scheme paints a star of this class with (linear). */
	FLinearColor StarColor(const FString& StarClass);
}

/**
 * Rio 03.10 ("instead of the picture, a real snapshot: a simplified scheme of the system, orbits, like on the map, just
 * so that it is pretty"): a static, deterministic Slate painting of one saved world. Its star in its spectral colour with
 * a soft glow, thin tilted orbits, planets by type and size, moons, the inhabited worlds ringed in amber; companions of a
 * multiple star to the sides. The model is built once in Construct; pixel geometry is cached per size, so a paint only
 * submits draw elements. Rio 03.10: the picture carries no text or badges; captions live beside it.
 */
class SAPSWorldSchemePreview final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSWorldSchemePreview)
		: _Centred(false)
	{}
		SLATE_ARGUMENT(FAPSWorldSchemeInput, Input)
		/** The system in the middle of the view (the details header); cards sit it a little low, under their overlays. */
		SLATE_ARGUMENT(bool, Centred)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(96.0, 54.0); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	enum class EStarLook : uint8
	{
		Normal,
		BlackHole,
		Neutron,
		Proto
	};

	struct FBody
	{
		FLinearColor Color{FLinearColor::White};
		/** Disc radius in planet units. */
		float Scale{0.8f};
		/** 0 = innermost orbit .. 1 = outermost, per star. */
		float Orbit{0.5f};
		float Phase{0.0f};
		int32 Moons{0};
		int32 Star{0};
		bool bInhabited{false};
		bool bRing{false};
		bool bBands{false};
		bool bAtmosphere{false};
	};

	struct FSpeck
	{
		float X{0.0f};
		float Y{0.0f};
		float Size{1.0f};
		float Alpha{0.2f};
		bool bWarm{false};
	};

	struct FBeltDot
	{
		float Phase{0.0f};
		float Offset{0.0f};
		float Size{1.0f};
		float Alpha{0.3f};
	};

	/** Pixel geometry for one widget size; rebuilt only when the size changes. */
	struct FPixels
	{
		FVector2D Size{FVector2D::ZeroVector};
		FVector2D Centre{FVector2D::ZeroVector};
		float StarRadius{0.0f};
		int32 SpeckCount{0};
		TArray<FVector2D> SpeckPositions;
		TArray<FVector2D> Ecliptic;
		TArray<FVector2D> BodyCentres;
		TArray<float> BodyRadii;
		TArray<bool> BodyFront;
		/** 0 = far side of the orbit .. 1 = near side. */
		TArray<float> BodyDepth;
		/** Unit direction from each body to its star. */
		TArray<FVector2D> BodyLight;
		TArray<TArray<FVector2D>> OrbitBack;
		TArray<TArray<FLinearColor>> OrbitBackColors;
		TArray<TArray<FVector2D>> OrbitFront;
		TArray<TArray<FLinearColor>> OrbitFrontColors;
		TArray<TArray<FVector2D>> RingBack;
		TArray<TArray<FVector2D>> RingFront;
		TArray<TArray<FVector2D>> InhabitedRings;
		TArray<TArray<FVector2D>> BandLines;
		TArray<int32> BandOwners;
		TArray<FVector2D> MoonPositions;
		TArray<int32> MoonOwners;
		TArray<FVector2D> BeltPositions;
		TArray<bool> BeltFront;
		TArray<FVector2D> CompanionCentres;
		TArray<float> CompanionRadii;
		TArray<FVector2D> DiscBack;
		TArray<FVector2D> DiscFront;
		TArray<FVector2D> PhotonRing;
		TArray<FVector2D> Beams;
	};

	void UpdatePixels(const FVector2D& LocalSize) const;

	TArray<FBody> Bodies;
	TArray<FSpeck> Specks;
	TArray<FBeltDot> BeltDots;
	TArray<FLinearColor> CompanionColors;
	FLinearColor StarTint{FLinearColor::White};
	EStarLook StarLook{EStarLook::Normal};
	float StarScale{1.0f};
	float PlaneRoll{0.0f};
	float BeltOrbit{-1.0f};
	float BeltWidth{0.0f};
	int32 HomeBody{INDEX_NONE};
	bool bRecordedSystem{false};
	bool bCentred{false};
	mutable FPixels Pixels;
};

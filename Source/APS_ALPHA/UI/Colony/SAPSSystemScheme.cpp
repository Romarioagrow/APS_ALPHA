#include "SAPSSystemScheme.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"

#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetarySystem.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "APS_ALPHA/UI/Style/APSSlateLineGuard.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "APSSystemScheme"

namespace APSSystemSchemePrivate
{
	// The largest planet's disc radius at zoom 1 (the default view); every planet and moon uses the same kilometre scale.
	// Rio 09.10 (item 33): smaller only where the lanes would not fit the view with it, and never under the second one.
	constexpr double LargestPlanetPixels = 34.0;
	constexpr double LargestPlanetLeastPixels = 10.0;
	// Bodies that would vanish at this scale keep a visible dot (Rio 09.10: "all readable"; to scale above it).
	constexpr float MinimumPlanetPixels = 2.5f;
	constexpr float MinimumMoonPixels = 2.0f;
	// A star never drawn smaller than this.
	constexpr double MinimumStarPixels = 6.0;
	// Between neighbouring slots (each slot fits its disc, its labels and its moon column).
	constexpr double SlotGapPixels = 34.0;
	// Above a lane's axis: the star's labels.
	constexpr double LaneHeaderPixels = 70.0;
	// From the largest disc of a lane to its label row; to the first moon. The row's own height follows its fonts
	// (LabelRowHeight: the name with its designation after it, then the type).
	constexpr double LabelGapPixels = 14.0;
	constexpr double NameDesignationGap = 7.0;
	constexpr double LabelLineGap = 2.0;
	constexpr double MoonGapPixels = 12.0;
	constexpr double MoonRowPixels = 20.0;
	constexpr double MoonTextGapPixels = 7.0;

	FSlateFontInfo DesignationFont() { return APSChrome::Font(TEXT("Bold"), 10); }
	FSlateFontInfo NameFont() { return APSUITheme::BodyFont(TEXT("Bold"), 12); }
	FSlateFontInfo DetailFont() { return APSUITheme::BodyFont(TEXT("Regular"), 10); }
	/** Secondary text, brighter than the chrome's muted grey: readable on the dark scheme. */
	FLinearColor Soft() { return APSUITheme::Retint(FLinearColor(0.64f, 0.75f, 0.80f, 1.0f)); }

	FVector2D Measure(const FText& Text, const FSlateFontInfo& Font)
	{
		return Text.IsEmpty() ? FVector2D::ZeroVector
			: FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
	}

	/** A line's height in this font, text or not. */
	double LineHeight(const FSlateFontInfo& Font)
	{
		return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(Font);
	}

	/** Rio 06.10: "KYPHOTHEA  A1", the designation after the name, then the type under it. */
	double LabelRowHeight()
	{
		return LineHeight(NameFont()) + LabelLineGap + LineHeight(DetailFont());
	}

	double NameLineWidth(const FText& Name, const FText& Designation)
	{
		const double Designated = Designation.IsEmpty() ? 0.0 : NameDesignationGap + Measure(Designation, DesignationFont()).X;
		return Measure(Name, NameFont()).X + Designated;
	}
	// Rio 04.10 ("what are these stars? it was fine"): a star is always a filled round disc inside its lane. Rio 09.10: in
	// the default view every star is whole in its lane's room (the stars' own scale); zoomed in, the view's edges cut it.
	constexpr double StarMarginPixels = 12.0;
	constexpr double LaneMarginPixels = 12.0;

	/** Test runs (aps.Test.Scheme): the scheme on screen, the newest one (the terminal builds its own on every open). */
	TWeakPtr<SAPSSystemScheme> LiveScheme;

	FLinearColor TypeColor(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Melted: case EPlanetType::Volcanic: case EPlanetType::Lava: case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.32f, 0.12f);
		case EPlanetType::GasGiant: return FLinearColor(0.98f, 0.70f, 0.30f);
		case EPlanetType::IceGiant: case EPlanetType::Ice: case EPlanetType::Frozen: return FLinearColor(0.55f, 0.85f, 1.0f);
		case EPlanetType::Ocean: case EPlanetType::Water: case EPlanetType::Archipelago: return FLinearColor(0.18f, 0.55f, 1.0f);
		case EPlanetType::Terrestrial: case EPlanetType::Forest: case EPlanetType::Oasis: return FLinearColor(0.25f, 0.88f, 0.55f);
		case EPlanetType::Desert: case EPlanetType::Sand: return FLinearColor(0.95f, 0.74f, 0.36f);
		case EPlanetType::Metal: case EPlanetType::Metallic: case EPlanetType::Carbon: return FLinearColor(0.72f, 0.66f, 0.90f);
		default: return FLinearColor(0.70f, 0.72f, 0.76f);
		}
	}

	FText Upper(const FName Name, const FText& Fallback)
	{
		return Name.IsNone() ? Fallback : FText::FromString(Name.ToString().ToUpper());
	}

	/** "ROCKY", "FROZEN", "GAS GIANT" (no " PLANET": moons are not planets). */
	FText EnumText(const EPlanetType Type)
	{
		FString Name = UEnum::GetDisplayValueAsText(Type).ToString().ToUpper();
		Name.RemoveFromEnd(TEXT(" PLANET"));
		return FText::FromString(Name);
	}

	const FSlateBrush* Disc()
	{
		// No corner radius: half the height, so a square box paints a circle.
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	void Arc(FSlateWindowElementList& Elements, const int32 Layer, const FGeometry& Geometry, const FVector2D& Centre,
		const float Radius, const float From, const float To, const FLinearColor& Color, const float Width)
	{
		TArray<FVector2D> Points;
		const int32 Segments = FMath::Clamp(FMath::CeilToInt((To - From) * Radius / 6.0f), 12, 96);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = FMath::Lerp(From, To, static_cast<float>(Index) / Segments);
			Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		if (APSSlateLineGuard::IsDrawable(Points))
		{
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color,
				true, Width);
		}
	}
}

void SAPSSystemScheme::Construct(const FArguments& InArgs)
{
	World = InArgs._World;
	OnPicked = InArgs._OnPicked;
	// Rio 09.10: zoomed in, the scheme runs past its edges; it is cut there instead of drawing over the terminal around it.
	SetClipping(EWidgetClipping::ClipToBounds);
	APSSystemSchemePrivate::LiveScheme = SharedThis(this);
	Refresh();
}

void SAPSSystemScheme::ShowSystem(AActor* Star)
{
	// Rio 05.10 (star map): another system opens whole, at the default zoom.
	if (PinnedStar.Get() != Star)
	{
		PinnedStar = Star;
		Picked = nullptr;
		Zoom = 1.0;
		Pan = FVector2D::ZeroVector;
	}
	Refresh();
}

FText SAPSSystemScheme::StarClassText(const AStar& Star)
{
	// Rio 09.10 (item 33, "BHOUNKNOWN" on a star): FullSpectralName is AStar::GenerateFullSpectralName, the class, the
	// subclass and the luminosity class glued from their enum names ("%s%d%s"). A black hole has subclass 0 and no
	// luminosity class, so it read "BH0Unknown" (the zero looks like an O in the UI font). Compact objects say what they
	// are (the stellar type owns them, as in StarGenerator; the spectral class as a fallback), and a part the generator
	// could not name is left out. The name itself stays as generated: star names are derived from it.
	const EStellarType Type = Star.StellarClass;
	if (Type == EStellarType::BlackHole || Star.SpectralClass == ESpectralClass::BH)
	{
		return LOCTEXT("ClassBlackHole", "BLACK HOLE");
	}
	if (Type == EStellarType::Pulsar)
	{
		return LOCTEXT("ClassPulsar", "PULSAR");
	}
	if (Type == EStellarType::Neutron || Star.SpectralClass == ESpectralClass::NS)
	{
		return LOCTEXT("ClassNeutron", "NEUTRON STAR");
	}
	if (Type == EStellarType::Protostar || Star.SpectralClass == ESpectralClass::PS)
	{
		return LOCTEXT("ClassProtostar", "PROTOSTAR");
	}
	FString Name = Star.FullSpectralName.IsNone() ? FString() : Star.FullSpectralName.ToString();
	if (Name.StartsWith(TEXT("Unknown")))
	{
		Name.Reset();
	}
	Name.ReplaceInline(TEXT("Unknown"), TEXT(""));
	if (Name.IsEmpty() && !Star.FullSpectralClass.IsNone())
	{
		Name = Star.FullSpectralClass.ToString();
	}
	return Name.IsEmpty() ? FText::GetEmpty() : FText::FromString(Name.ToUpper());
}

void SAPSSystemScheme::Refresh()
{
	using namespace APSSystemSchemePrivate;
	Bodies.Reset();
	Lanes = 0;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	const APlayerController* Controller = LiveWorld->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	// The current system: the star nearest the player, with every star of its system (Rio 05.10: or the system the star
	// map's drill-down pinned).
	const AStar* Nearest = Cast<AStar>(PinnedStar.Get());
	const bool bPinned = Nearest != nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AStar> It(LiveWorld); It && !bPinned; ++It)
	{
		if (!IsValid(*It) || !IsValid(It->PlanetarySystem)) continue;
		const double Distance = FVector::DistSquared(It->GetActorLocation(), Origin);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = *It;
		}
	}
	if (!Nearest)
	{
		return;
	}
	TArray<AStar*> Stars;
	if (const AStarSystem* System = Cast<AStarSystem>(Nearest->GetAttachParentActor()))
	{
		Stars = System->GetStars();
	}
	if (Stars.IsEmpty())
	{
		Stars.Add(const_cast<AStar*>(Nearest));
	}
	for (AStar* Star : Stars)
	{
		if (!IsValid(Star)) continue;
		FBody& StarBody = Bodies.AddDefaulted_GetRef();
		StarBody.Actor = Star;
		StarBody.bStar = true;
		StarBody.Lane = Lanes;
		StarBody.RadiusKm = Star->RadiusKM > 0.0 ? Star->RadiusKM : static_cast<double>(Star->StarRadiusKM);
		StarBody.Designation = FText::FromString(APSBodyDesignation::Of(Star));
		StarBody.Name = Upper(Star->AstroName, LOCTEXT("Star", "STAR"));
		StarBody.Detail = StarClassText(*Star);
		StarBody.Color = FLinearColor(1.0f, 0.82f, 0.46f);
		if (IsValid(Star->PlanetarySystem))
		{
			for (APlanet* Planet : Star->PlanetarySystem->PlanetsActorsList)
			{
				if (!IsValid(Planet)) continue;
				const int32 PlanetIndex = Bodies.Num();
				FBody& PlanetBody = Bodies.AddDefaulted_GetRef();
				PlanetBody.Actor = Planet;
				PlanetBody.Lane = Lanes;
				PlanetBody.RadiusKm = Planet->PlanetRadiusKM;
				PlanetBody.Designation = FText::FromString(APSBodyDesignation::Of(Planet));
				PlanetBody.Name = Upper(Planet->AstroName, LOCTEXT("Planet", "PLANET"));
				PlanetBody.Detail = FText::Format(LOCTEXT("PlanetDetail", "{0}  /  {1} KM"), EnumText(Planet->PlanetType),
					APSUINumber::Number(Planet->PlanetRadiusKM));
				PlanetBody.Color = TypeColor(Planet->PlanetType);
				for (AMoon* Moon : Planet->Moons)
				{
					if (!IsValid(Moon)) continue;
					FBody& MoonBody = Bodies.AddDefaulted_GetRef();
					MoonBody.Actor = Moon;
					MoonBody.bMoon = true;
					MoonBody.Lane = Lanes;
					MoonBody.Parent = PlanetIndex;
					MoonBody.RadiusKm = Moon->PlanetRadiusKM;
					MoonBody.Designation = FText::FromString(APSBodyDesignation::Of(Moon));
					MoonBody.Name = Upper(Moon->AstroName, LOCTEXT("Moon", "MOON"));
					MoonBody.Detail = FText::Format(LOCTEXT("MoonDetail", "{0}  /  {1} KM"), EnumText(Moon->PlanetType),
						APSUINumber::Number(Moon->PlanetRadiusKM));
					// The moon column reads "A1.02  SIAX".
					MoonBody.Label = FText::Format(LOCTEXT("MoonLabel", "{0}  {1}"), MoonBody.Designation, MoonBody.Name);
					MoonBody.Color = TypeColor(Moon->PlanetType);
				}
			}
		}
		++Lanes;
	}
}

void SAPSSystemScheme::Layout(const FVector2D& Size) const
{
	using namespace APSSystemSchemePrivate;
	Centres.SetNumZeroed(Bodies.Num());
	Radii.SetNumZeroed(Bodies.Num());
	LabelRows.SetNumZeroed(FMath::Max(Lanes, 1));
	LaneTops.SetNumZeroed(FMath::Max(Lanes, 1));
	LaneHeights.SetNumZeroed(FMath::Max(Lanes, 1));
	LaidOutSize = Size;
	if (Bodies.IsEmpty() || Lanes == 0)
	{
		ContentSize = FVector2D::ZeroVector;
		LaidOutPan = FVector2D::ZeroVector;
		return;
	}
	const double BaseLaneHeight = Size.Y / Lanes;
	// Rio 09.10 (item 33, "the planets are all the same dots, although their sizes differ a lot"): the stars no longer set
	// the planets' scale (fitted to the stars, every planet was under a pixel and got the same minimum dot). Planets and
	// moons share one kilometre scale of their own: the system's largest planet LargestPlanetPixels, or less where a lane
	// with its labels and moon column would not fit its share of the view, never under LargestPlanetLeastPixels.
	double LargestPlanetKm = 0.0;
	TArray<double> LaneLargestKm;
	LaneLargestKm.Init(0.0, Lanes);
	TArray<int32> MoonCounts;
	MoonCounts.Init(0, Bodies.Num());
	for (const FBody& Body : Bodies)
	{
		if (Body.bStar) continue;
		LargestPlanetKm = FMath::Max(LargestPlanetKm, Body.RadiusKm);
		if (Body.bMoon && Bodies.IsValidIndex(Body.Parent)) ++MoonCounts[Body.Parent];
		else if (!Body.bMoon && Body.Lane < Lanes) LaneLargestKm[Body.Lane] = FMath::Max(LaneLargestKm[Body.Lane], Body.RadiusKm);
	}
	double PlanetScale = 0.0;
	if (LargestPlanetKm > 0.0)
	{
		PlanetScale = LargestPlanetPixels / LargestPlanetKm;
		TArray<int32> LaneMoons;
		LaneMoons.Init(0, Lanes);
		for (int32 Index = 0; Index < Bodies.Num(); ++Index)
		{
			const FBody& Body = Bodies[Index];
			if (!Body.bStar && !Body.bMoon && Body.Lane < Lanes) LaneMoons[Body.Lane] = FMath::Max(LaneMoons[Body.Lane], MoonCounts[Index]);
		}
		for (int32 Lane = 0; Lane < Lanes; ++Lane)
		{
			if (LaneLargestKm[Lane] <= 0.0) continue;
			// The lane's height as LaneShape builds it below: its header, the largest disc twice, the label row and the moon
			// column (rows of their minimum height).
			const double MoonsBelow = LaneMoons[Lane] > 0 ? MoonGapPixels + LaneMoons[Lane] * (MoonRowPixels + 4.0) : 0.0;
			const double Fits = 0.5 * (BaseLaneHeight - LaneHeaderPixels - LabelGapPixels - LabelRowHeight() - MoonsBelow
				- LaneMarginPixels);
			PlanetScale = FMath::Min(PlanetScale, Fits / LaneLargestKm[Lane]);
		}
		PlanetScale = FMath::Max(PlanetScale, LargestPlanetLeastPixels / LargestPlanetKm);
	}
	// First pass: each lane's largest planet disc (the axis and the label row follow it) and each planet's moon column.
	TArray<double> LaneLargest;
	LaneLargest.Init(MinimumPlanetPixels, Lanes);
	TArray<double> MoonColumns;
	MoonColumns.Init(0.0, Bodies.Num());
	TArray<double> MoonHeights;
	MoonHeights.Init(0.0, Bodies.Num());
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const double Radius = Body.RadiusKm * PlanetScale;
		if (Body.bMoon && Bodies.IsValidIndex(Body.Parent))
		{
			const double Dot = 2.0 * FMath::Max(Radius, static_cast<double>(MinimumMoonPixels));
			MoonColumns[Body.Parent] = FMath::Max(MoonColumns[Body.Parent],
				Dot + MoonTextGapPixels + Measure(Body.Label, DetailFont()).X);
			MoonHeights[Body.Parent] += FMath::Max(Dot, MoonRowPixels) + 4.0;
		}
		else if (!Body.bStar && Body.Lane < Lanes)
		{
			LaneLargest[Body.Lane] = FMath::Max(LaneLargest[Body.Lane], FMath::Max(Radius, static_cast<double>(MinimumPlanetPixels)));
		}
	}
	// Under each lane's axis: the label row and the tallest moon column.
	TArray<double> LaneBelow;
	LaneBelow.Init(0.0, Lanes);
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		if (!Body.bStar && !Body.bMoon && Body.Lane < Lanes && MoonHeights[Index] > 0.0)
		{
			LaneBelow[Body.Lane] = FMath::Max(LaneBelow[Body.Lane], MoonGapPixels + MoonHeights[Index]);
		}
	}
	// Rio 04.10 evening ("zoomed in, the bodies lie on each other"): a lane is its share of the widget, or taller when its
	// star labels, largest disc, label row and moons need more, so lanes stack instead of overlapping (and the scheme
	// pans vertically). The axis sits mid-lane when the labels and moons fit under it, so the star is as tall as its lane
	// allows.
	const auto LaneShape = [BaseLaneHeight](const double Largest, const double MoonsBelow, double& OutHeight, double& OutOffset)
	{
		const double Below = Largest + LabelGapPixels + LabelRowHeight() + MoonsBelow + LaneMarginPixels;
		OutHeight = FMath::Max(BaseLaneHeight, LaneHeaderPixels + Largest + Below);
		OutOffset = FMath::Max(LaneHeaderPixels + Largest, FMath::Min(OutHeight * 0.5, OutHeight - Below));
	};
	const auto RoomOf = [](const double Height, const double Offset, const double Largest)
	{
		// Never smaller than the lane's largest planet.
		return FMath::Max3(FMath::Min(Offset, Height - Offset) - StarMarginPixels, 1.3 * Largest, 8.0);
	};
	TArray<double> Axes;
	Axes.SetNumZeroed(Lanes);
	TArray<double> StarRoom;
	StarRoom.SetNumZeroed(Lanes);
	double Top = 0.0;
	for (int32 Lane = 0; Lane < Lanes; ++Lane)
	{
		double Height = 0.0;
		double Offset = 0.0;
		LaneShape(LaneLargest[Lane], LaneBelow[Lane], Height, Offset);
		LaneTops[Lane] = Top;
		LaneHeights[Lane] = Height;
		Axes[Lane] = Top + Offset;
		StarRoom[Lane] = RoomOf(Height, Offset, LaneLargest[Lane]);
		Top += Height;
	}
	// Rio 06.10 ("by default it should look like this", the star whole beside its planets): every star of the system on
	// one scale, the planets' own or smaller, so that each star is whole in its lane's room. Rio 09.10: a star drawn
	// smaller than the planets' scale says by how much on its plate (OnPaint), the same factor for every star of the system.
	double StarScale = PlanetScale;
	for (const FBody& Body : Bodies)
	{
		if (Body.bStar && Body.RadiusKm > 0.0 && Body.Lane < Lanes)
		{
			const double Fit = 0.97 * StarRoom[Body.Lane] / Body.RadiusKm;
			StarScale = StarScale > 0.0 ? FMath::Min(StarScale, Fit) : Fit;
		}
	}
	double Along = 0.0;
	double MoonY = 0.0;
	double MoonLeft = 0.0;
	double Right = 0.0;
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const int32 Lane = FMath::Clamp(Body.Lane, 0, Lanes - 1);
		const double AxisY = Axes[Lane];
		if (Body.bStar)
		{
			const double DrawnRadius = FMath::Max(Body.RadiusKm * StarScale, MinimumStarPixels);
			const double RightEdge = 24.0 + 2.0 * DrawnRadius;
			Radii[Index] = static_cast<float>(DrawnRadius);
			Centres[Index] = FVector2D(RightEdge - DrawnRadius, AxisY);
			Along = RightEdge + 48.0;
			Right = FMath::Max(Right, RightEdge);
			continue;
		}
		const double Radius = Body.RadiusKm * PlanetScale;
		if (!Body.bMoon)
		{
			Radii[Index] = FMath::Max(static_cast<float>(Radius), MinimumPlanetPixels);
			const double Labels = FMath::Max(NameLineWidth(Body.Name, Body.Designation), Measure(Body.Detail, DetailFont()).X);
			const double Slot = FMath::Max3(2.0 * Radii[Index], Labels, MoonColumns[Index]) + SlotGapPixels;
			Centres[Index] = FVector2D(Along + Slot * 0.5, AxisY);
			Along += Slot;
			Right = FMath::Max(Right, Along);
			// The moon column sits centred under the planet's labels.
			MoonY = AxisY + LaneLargest[Lane] + LabelGapPixels + LabelRowHeight() + MoonGapPixels;
			MoonLeft = Centres[Index].X - MoonColumns[Index] * 0.5;
			continue;
		}
		Radii[Index] = FMath::Max(static_cast<float>(Radius), MinimumMoonPixels);
		const double Row = FMath::Max(2.0 * Radii[Index], MoonRowPixels);
		Centres[Index] = FVector2D(MoonLeft + Radii[Index], MoonY + Row * 0.5);
		MoonY += Row + 4.0;
	}
	// Rio 09.10 ("when I turn the wheel they all drift apart differently"): the zoom used to resize the discs inside a layout
	// that did not scale with them (slots as wide as their labels, a star sliding and flattening on its own curve, each
	// lane differently), while the wheel moved the pan as if all of it had scaled around the cursor. Now everything above
	// is the default view, and the zoom and the pan are one transform of it for every disc, lane and label row; labels
	// keep their size at their bodies, and zoomed in the gaps between them only grow. The wheel stops where the largest
	// planet's disc is about 40% of the view's height.
	ContentSize = FVector2D(Right, Top);
	double LargestDisc = 8.0;
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		if (!Bodies[Index].bStar) LargestDisc = FMath::Max(LargestDisc, static_cast<double>(Radii[Index]));
	}
	MaxZoom = FMath::Max(1.0, 0.4 * Size.Y / LargestDisc);
	Zoom = FMath::Clamp(Zoom, 1.0, MaxZoom);
	PixelsPerKm = (PlanetScale > 0.0 ? PlanetScale : StarScale) * Zoom;
	LaidOutPan = ClampPan(Pan, Size);
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		Centres[Index] = Centres[Index] * Zoom + LaidOutPan;
		Radii[Index] = static_cast<float>(Radii[Index] * Zoom);
	}
	for (int32 Lane = 0; Lane < Lanes; ++Lane)
	{
		LabelRows[Lane] = (Axes[Lane] + LaneLargest[Lane]) * Zoom + LaidOutPan.Y + LabelGapPixels;
		LaneTops[Lane] = LaneTops[Lane] * Zoom + LaidOutPan.Y;
		LaneHeights[Lane] *= Zoom;
	}
}

FVector2D SAPSSystemScheme::ClampPan(const FVector2D& Wanted, const FVector2D& Size) const
{
	// Never right of the scheme's left edge; to the left only until half the view still shows the scheme; vertically
	// within the lanes (no pan while they fit). The scheme on screen is its laid-out size at the zoom.
	const FVector2D Content = ContentSize * Zoom;
	const double MinX = FMath::Min(0.0, Size.X * 0.5 - Content.X);
	const double MinY = FMath::Min(0.0, Size.Y - Content.Y);
	return FVector2D(FMath::Clamp(Wanted.X, MinX, 0.0), FMath::Clamp(Wanted.Y, MinY, 0.0));
}

int32 SAPSSystemScheme::HitTest(const FVector2D& Local) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Bodies.Num() && Index < Centres.Num(); ++Index)
	{
		const double Distance = FVector2D::Distance(Local, Centres[Index]);
		// A star's limb is picked anywhere on its disc; small bodies get a few pixels of slack.
		if (Distance <= Radii[Index] + 6.0 && Distance - Radii[Index] < BestDistance)
		{
			BestDistance = Distance - Radii[Index];
			Best = Index;
		}
	}
	return Best;
}

int32 SAPSSystemScheme::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle&, bool) const
{
	using namespace APSSystemSchemePrivate;
	const FVector2D Size = Geometry.GetLocalSize();
	Layout(Size);
	const FSlateFontInfo DesignationFont = APSSystemSchemePrivate::DesignationFont();
	const FSlateFontInfo NameFont = APSSystemSchemePrivate::NameFont();
	const FSlateFontInfo DetailFont = APSSystemSchemePrivate::DetailFont();
	// Positions are line tops; bCentred centres the line on X, bMiddle centres its capitals on Y (moon labels beside their
	// dots; Rio 03.10: by the letters, not by Slate's line box).
	const auto Text = [&](const FText& Value, const FVector2D& Position, const FSlateFontInfo& Font, const FLinearColor& Color,
		const bool bCentred, const bool bMiddle = false)
	{
		if (Value.IsEmpty()) return;
		const FVector2D Measured = APSSystemSchemePrivate::Measure(Value, Font);
		const FVector2D At = Position - FVector2D(bCentred ? Measured.X * 0.5 : 0.0,
			bMiddle ? Measured.Y * 0.5 - APSChrome::CapsCenterOffset(Font) : 0.0);
		FSlateDrawElement::MakeText(Elements, LayerId + 4, Geometry.ToPaintGeometry(Measured, FSlateLayoutTransform(At)),
			Value, Font, ESlateDrawEffect::None, Color);
	};
	FSlateDrawElement::MakeBox(Elements, LayerId, Geometry.ToPaintGeometry(), FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None, APSUITheme::Retint(FLinearColor(0.002f, 0.010f, 0.018f, 0.92f)));
	// Rio 06.10: the name, then its designation in the body's colour, both centred on the name line's capitals.
	const auto NameLineAt = [&](const FBody& Body, const FVector2D& LineTop, const bool bCentred, const bool bIsPicked)
	{
		const double Width = NameLineWidth(Body.Name, Body.Designation);
		const double Left = bCentred ? LineTop.X - Width * 0.5 : LineTop.X;
		const double Middle = LineTop.Y + LineHeight(NameFont) * 0.5;
		Text(Body.Name, FVector2D(Left, Middle), NameFont, bIsPicked ? APSChrome::Amber() : APSChrome::White(), false, true);
		Text(Body.Designation, FVector2D(Left + APSSystemSchemePrivate::Measure(Body.Name, NameFont).X + NameDesignationGap, Middle),
			DesignationFont, bIsPicked ? APSChrome::Amber() : Body.Color, false, true);
	};
	if (Bodies.IsEmpty())
	{
		Text(LOCTEXT("NoSystem", "NO STAR SYSTEM NEARBY"), Size * 0.5, NameFont, APSChrome::Muted(), true);
		return LayerId + 4;
	}
	for (int32 Lane = 1; Lane < Lanes && Lane < LaneTops.Num(); ++Lane)
	{
		const double Y = LaneTops[Lane];
		if (Y <= 0.0 || Y >= Size.Y) continue;
		const TArray<FVector2D> LanePoints{FVector2D(0.0, Y), FVector2D(Size.X, Y)};
		if (APSSlateLineGuard::IsDrawable(LanePoints))
		{
			FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
				LanePoints, ESlateDrawEffect::None, APSChrome::CyanDim(), true, 1.0f);
		}
	}
	const AActor* PickedActor = Picked.Get();
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		const FBody& Body = Bodies[Index];
		const FVector2D Centre = Centres[Index];
		const float Radius = Radii[Index];
		const bool bPicked = PickedActor && Body.Actor.Get() == PickedActor;
		if (Body.bStar)
		{
			// The orbital order axis from the star through its planets.
			const TArray<FVector2D> AxisPoints{FVector2D(FMath::Max(0.0, Centre.X + Radius), Centre.Y), FVector2D(Size.X, Centre.Y)};
			if (APSSlateLineGuard::IsDrawable(AxisPoints))
			{
				FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
					AxisPoints,
					ESlateDrawEffect::None, FLinearColor(APSChrome::Cyan().R, APSChrome::Cyan().G, APSChrome::Cyan().B, 0.18f),
					true, 1.0f);
			}
			if (Centre.X + Radius < -2.0) continue;
			// Rio 04.10 ("what are these stars? it was fine"): the filled disc with a soft glow and a bright limb, always;
			// Layout keeps it whole in its lane, and zoomed in the widget's edges cut the part out of view (the limb is
			// drawn only where it is in view on the left).
			const float Span = FMath::Acos(static_cast<float>(FMath::Clamp(-Centre.X / Radius, -1.0, 1.0)));
			const FVector2D ClipMin = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
			const FVector2D ClipMax = Geometry.LocalToAbsolute(Size);
			Elements.PushClip(FSlateClippingZone(FSlateRect(ClipMin.X, ClipMin.Y, ClipMax.X, ClipMax.Y)));
			FSlateDrawElement::MakeBox(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2D(2.0f * Radius),
				FSlateLayoutTransform(Centre - FVector2D(Radius))), Disc(), ESlateDrawEffect::None,
				FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, 0.85f));
			for (const TPair<float, float>& Stroke : {TPair<float, float>(14.0f, 0.10f), TPair<float, float>(6.0f, 0.30f),
				TPair<float, float>(1.6f, 1.0f)})
			{
				Arc(Elements, LayerId + 2, Geometry, Centre, Radius, -Span, Span,
					FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, Stroke.Value), Stroke.Key);
			}
			Elements.PopClip();
			// The star's labels on a dark plate (Rio 02.10: "the star's label drifts off somewhere"): a small star has them
			// under its disc like a planet; a large one centred on the part of the disc in view, on the axis, so the
			// label moves with the star under zoom and pan instead of sticking near the left edge.
			// A star drawn smaller than the planets' scale says by how much. Rio 09.10 (item 33, "SHOWN 1x SMALLER"): the
			// factor was rounded to whole numbers from 1.05 up; now under 10 with one decimal ("1.4x"), and said only
			// when it rounds above 1. The zoom scales the star and the planets alike, so the factor holds at any zoom.
			const double Factor = Radius > 0.0f ? Body.RadiusKm * PixelsPerKm / Radius : 1.0;
			const double Shown = Factor < 10.0 ? FMath::RoundToDouble(Factor * 10.0) / 10.0 : FMath::RoundToDouble(Factor);
			FNumberFormattingOptions FactorFormat;
			FactorFormat.SetMaximumFractionalDigits(1);
			const FText ScaleNote = Shown > 1.0
				? FText::Format(LOCTEXT("StarScaleNote", "SHOWN {0}x SMALLER"), APSUINumber::Number(Shown, &FactorFormat))
				: FText::GetEmpty();
			const double NameLine = LineHeight(NameFont);
			const double DetailLine = LineHeight(DetailFont);
			const double PlateWidth = FMath::Max3(NameLineWidth(Body.Name, Body.Designation), Measure(Body.Detail, DetailFont).X,
				Measure(ScaleNote, DetailFont).X) + 20.0;
			const double PlateHeight = 12.0 + NameLine + (Body.Detail.IsEmpty() ? 0.0 : LabelLineGap + DetailLine)
				+ (ScaleNote.IsEmpty() ? 0.0 : LabelLineGap + DetailLine);
			FVector2D LabelAt;
			const double LaneHeight = LaneHeights.IsValidIndex(Body.Lane) ? LaneHeights[Body.Lane] : Size.Y;
			if (2.0 * Radius < LaneHeight * 0.5)
			{
				LabelAt = FVector2D(Centre.X - PlateWidth * 0.5 + 10.0, Centre.Y + Radius + LabelGapPixels);
			}
			else
			{
				const double VisibleLeft = FMath::Max(0.0, Centre.X - Radius);
				const double VisibleRight = FMath::Min(Size.X, Centre.X + Radius);
				const double Left = (VisibleLeft + VisibleRight - PlateWidth) * 0.5;
				LabelAt = FVector2D(FMath::Max(VisibleLeft + 4.0, Left) + 10.0, Centre.Y - PlateHeight * 0.5 + 6.0);
			}
			{
				FSlateDrawElement::MakeBox(Elements, LayerId + 3, Geometry.ToPaintGeometry(FVector2D(PlateWidth, PlateHeight),
					FSlateLayoutTransform(LabelAt - FVector2D(10.0, 6.0))), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None,
					APSUITheme::Retint(FLinearColor(0.004f, 0.016f, 0.026f, 0.82f)));
			}
			NameLineAt(Body, LabelAt, false, bPicked);
			Text(Body.Detail, LabelAt + FVector2D(0.0, NameLine + LabelLineGap), DetailFont, Soft(), false);
			Text(ScaleNote, LabelAt + FVector2D(0.0, NameLine + 2.0 * LabelLineGap + DetailLine), DetailFont, APSChrome::Muted(), false);
			continue;
		}
		FSlateDrawElement::MakeBox(Elements, LayerId + 2, Geometry.ToPaintGeometry(FVector2D(2.0f * Radius),
			FSlateLayoutTransform(Centre - FVector2D(Radius))), Disc(), ESlateDrawEffect::None, Body.Color);
		if (bPicked)
		{
			Arc(Elements, LayerId + 3, Geometry, Centre, Radius + 4.0f, 0.0f, UE_TWO_PI, APSChrome::Amber(), 1.6f);
		}
		if (!Body.bMoon)
		{
			// Every label of a lane on one row under its largest disc; a faint stem joins a small disc to its labels.
			const double Row = LabelRows.IsValidIndex(Body.Lane) ? LabelRows[Body.Lane] : Centre.Y + Radius + 14.0;
			if (Row - (Centre.Y + Radius) > 10.0)
			{
				FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{FVector2D(Centre.X, Centre.Y + Radius + 4.0), FVector2D(Centre.X, Row - 4.0)},
					ESlateDrawEffect::None, FLinearColor(Body.Color.R, Body.Color.G, Body.Color.B, 0.28f), true, 1.0f);
			}
			NameLineAt(Body, FVector2D(Centre.X, Row), true, bPicked);
			Text(Body.Detail, FVector2D(Centre.X, Row + LineHeight(NameFont) + LabelLineGap), DetailFont, Soft(), true);
		}
		else
		{
			Text(Body.Label, Centre + FVector2D(Radius + MoonTextGapPixels, 0.0), DetailFont,
				bPicked ? APSChrome::Amber() : Soft(), false, true);
		}
	}
	// Scale bar: 100 pixels in kilometres, and the honest caption.
	const double BarKm = PixelsPerKm > 0.0 ? 100.0 / PixelsPerKm : 0.0;
	const FVector2D BarEnd(Size.X - 16.0, Size.Y - 20.0);
	FSlateDrawElement::MakeLines(Elements, LayerId + 3, Geometry.ToPaintGeometry(),
		TArray<FVector2D>{BarEnd - FVector2D(100.0, 0.0), BarEnd}, ESlateDrawEffect::None, APSChrome::Cyan(), true, 1.5f);
	Text(FText::Format(LOCTEXT("ScaleBar", "{0} KM"), APSUINumber::Number(FMath::RoundToInt(BarKm))),
		BarEnd - FVector2D(50.0, 20.0), DetailFont, APSChrome::Cyan(), true);
	Text(LOCTEXT("SchemeHint", "SIZES TO SCALE, DISTANCES NOT  /  WHEEL ZOOM  /  DRAG TO PAN  /  CLICK A BODY"),
		FVector2D(14.0, Size.Y - 24.0), DetailFont, Soft(), false);
	return LayerId + 4;
}

FReply SAPSSystemScheme::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	ZoomAt(Event.GetWheelDelta(), Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
	return FReply::Handled();
}

void SAPSSystemScheme::ZoomAt(const double WheelSteps, const FVector2D& Local)
{
	// Rio 06.10: zooming scales the whole system. Rio 09.10 ("they all drift apart differently"): the layout is one
	// transform of the default view (Layout), so keeping the point under the cursor in place is exact: the pan follows
	// the zoom's ratio around it. Out no further than the default view (every star whole, the labels laid out to fit),
	// in no further than MaxZoom; the pan stays within the scheme (ClampPan), which moves everything alike.
	const double Previous = Zoom;
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.2, WheelSteps), 1.0, FMath::Max(MaxZoom, 1.0));
	Pan = ClampPan(Local - (Local - LaidOutPan) * (Zoom / Previous), LaidOutSize);
}

void SAPSSystemScheme::RunTestCommand(const TArray<FString>& Args, UWorld* World)
{
	// Rio 09.10 (item 33 A/B captures, test runs only): "open" (the default) opens the colony terminal on SYSTEM SCHEME;
	// "zoom <wheel steps> [x y]" turns the wheel there over a point given as fractions of the scheme (its centre by
	// default); "log" writes the view and every body's disc as the last frame drew them.
	const FString Verb = Args.IsEmpty() ? FString(TEXT("open")) : Args[0];
	if (Verb.Equals(TEXT("open"), ESearchCase::IgnoreCase))
	{
		constexpr int32 SchemeTab = 7;
		UAPSColonyTerminalSubsystem* Terminal = World ? World->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
		if (Terminal)
		{
			Terminal->OpenTerminalTab(SchemeTab);
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Scheme] open: %s"), Terminal && Terminal->IsTerminalOpen()
			? TEXT("SYSTEM SCHEME") : TEXT("no colony terminal (a generated game binds it)"));
		return;
	}
	const TSharedPtr<SAPSSystemScheme> Scheme = APSSystemSchemePrivate::LiveScheme.Pin();
	if (!Scheme.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[APS.Colony.Scheme] %s: no scheme on screen (aps.Test.Scheme open first)"), *Verb);
		return;
	}
	if (Verb.Equals(TEXT("zoom"), ESearchCase::IgnoreCase) && Args.Num() >= 2)
	{
		const FVector2D At = Args.Num() >= 4 ? FVector2D(FCString::Atod(*Args[2]), FCString::Atod(*Args[3])) : FVector2D(0.5, 0.5);
		Scheme->ZoomAt(FCString::Atod(*Args[1]), Scheme->LaidOutSize * At);
		UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Scheme] zoom %s at (%.2f, %.2f): zoom=%.3f (max %.3f) pan=(%.1f, %.1f)"), *Args[1],
			At.X, At.Y, Scheme->Zoom, Scheme->MaxZoom, Scheme->Pan.X, Scheme->Pan.Y);
		return;
	}
	if (Verb.Equals(TEXT("log"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Scheme] view size=(%.0f x %.0f) zoom=%.3f (max %.3f) pan=(%.1f, %.1f) km/px=%.1f bodies=%d"),
			Scheme->LaidOutSize.X, Scheme->LaidOutSize.Y, Scheme->Zoom, Scheme->MaxZoom, Scheme->LaidOutPan.X, Scheme->LaidOutPan.Y,
			Scheme->PixelsPerKm > 0.0 ? 1.0 / Scheme->PixelsPerKm : 0.0, Scheme->Bodies.Num());
		for (int32 Index = 0; Index < Scheme->Bodies.Num() && Index < Scheme->Centres.Num(); ++Index)
		{
			const FBody& Body = Scheme->Bodies[Index];
			UE_LOG(LogTemp, Log, TEXT("[APS.Colony.Scheme] %s %s %s km=%.0f centre=(%.1f, %.1f) r=%.2f %s"),
				Body.bStar ? TEXT("star") : Body.bMoon ? TEXT("moon") : TEXT("planet"), *Body.Designation.ToString(),
				*Body.Name.ToString(), Body.RadiusKm, Scheme->Centres[Index].X, Scheme->Centres[Index].Y, Scheme->Radii[Index],
				*Body.Detail.ToString());
		}
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[APS.Colony.Scheme] aps.Test.Scheme [open | zoom <wheel steps> [x y] | log]"));
}

namespace APSSystemSchemePrivate
{
	FAutoConsoleCommandWithWorldAndArgs TestSchemeCommand(TEXT("aps.Test.Scheme"),
		TEXT("Test runs: aps.Test.Scheme [open | zoom <wheel steps> [x y] | log]: opens the colony terminal on SYSTEM SCHEME, ")
		TEXT("turns the wheel over a point of the scheme (fractions of its size, the centre by default), logs every body's disc."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SAPSSystemScheme::RunTestCommand));
}

FReply SAPSSystemScheme::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = true;
	bDragged = false;
	DragStart = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	PanStart = LaidOutPan;
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SAPSSystemScheme::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bDragging)
	{
		return FReply::Unhandled();
	}
	const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
	bDragged |= FVector2D::Distance(Local, DragStart) > 4.0;
	if (bDragged)
	{
		Pan = ClampPan(PanStart + (Local - DragStart), Geometry.GetLocalSize());
	}
	return FReply::Handled();
}

FReply SAPSSystemScheme::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bDragging || Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = false;
	if (!bDragged)
	{
		const int32 Hit = HitTest(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
		if (Bodies.IsValidIndex(Hit))
		{
			Picked = Bodies[Hit].Actor;
			OnPicked.ExecuteIfBound(Picked.Get());
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SAPSSystemScheme::OnCursorQuery(const FGeometry&, const FPointerEvent&) const
{
	return FCursorReply::Cursor(bDragged ? EMouseCursor::GrabHandClosed : EMouseCursor::Default);
}

void SAPSSystemScheme::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	// Rio 06.10 (audit: the terminal closing or a window switch mid-drag left bDragging set, so the next hover panned):
	// only the drag stops; bDragged stays as a normal release leaves it, so the cursor is the same.
	bDragging = false;
	SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
}

#undef LOCTEXT_NAMESPACE

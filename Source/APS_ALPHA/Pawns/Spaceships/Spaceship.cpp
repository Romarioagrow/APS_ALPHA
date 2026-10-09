#include "Spaceship.h"
#include "APS_ALPHA/UI/Hud/SAPSShipHud.h"
#include "APS_ALPHA/UI/Style/APSUITheme.h"
#include "APSM5HullSweepComponent.h"
#include "APSShipHullComponent.h"
#include "APS_ALPHA/Core/Controllers/GravityPlayerController.h"
#include "APS_ALPHA/Core/Rendering/APSPreviewVisibility.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "APS_ALPHA/Actors/Astro/APSBodyDesignation.h"
#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/PlanetaryBody.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/Actors/Astro/StarCluster.h"
#include "APS_ALPHA/Actors/Astro/StarSystem.h"
#include "APS_ALPHA/Actors/Astro/WorldActor.h"
#include "APS_ALPHA/Actors/Tech/TechActor.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Interfaces/NavigatableBody.h"
#include "APS_ALPHA/Core/Structs/StarGenerationModel.h"
#include "APS_ALPHA/Core/World/APSPlanetEnvironmentStreamingSubsystem.h"
#include "APS_ALPHA/Core/World/APSWorldOriginSubsystem.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/Generation/PlanetarySurfaceGenerator.h"
#include "APS_ALPHA/Pawns/Characters/GravityCharacterPawn.h"
#include "APS_ALPHA/Pawns/Characters/GravityDetectorComponent.h"
#include "APS_ALPHA/Pawns/Spaceships/ShipNavigationComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Algo/BinarySearch.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "APSShipFlightBenchmark.h"
#include "APSShipFlightModel.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "SceneView.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "APS_ALPHA/Gameplay/Fleet/APSFleetCommand.h"
#include "APS_ALPHA/UI/Colony/APSColonyTerminalSubsystem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/MeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "APS_ALPHA/Gameplay/Vehicles/APSGroundVehicleTypes.h"
#include "APS_ALPHA/Gameplay/Construction/APSShipBuildComponent.h"
#include "Engine/CollisionProfile.h"
#include "Containers/Ticker.h"
#include "Misc/DelayedAutoRegister.h"
#include "TimerManager.h"

class SAPSShipNavigationOverlay final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSShipNavigationOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ASpaceship>, Ship)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Ship = InArgs._Ship;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(1.0f, 1.0f);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		return Ship.IsValid()
			? Ship->PaintNavigationOverlay(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId)
			: LayerId;
	}

private:
	TWeakObjectPtr<ASpaceship> Ship;
};

namespace APSNavigationHud
{
	// Rio 02.10 ("labels run past their frame, hard to see"): bold 9 pt and a denser background over star fields. Each card
	// is as wide as its own text: one width for every card of the frame crowded the sky (Rio 02.10: "labels pile up").
	constexpr float MarkerMinimumWidth = 120.0f;
	constexpr float MarkerHeight = 42.0f;
	constexpr float MarkerMaximumWidth = 320.0f;
	inline FSlateFontInfo MarkerFont() { return APSUITheme::BodyFont("Bold", 9); }
	inline FSlateFontInfo UnitTitleFont() { return APSUITheme::BodyFont("Bold", 8); }
	inline FSlateFontInfo UnitLineFont() { return APSUITheme::BodyFont("Regular", 8); }
	/** The line of a card that lists the other objects it stands for. */
	inline FSlateFontInfo MergedFont() { return APSUITheme::BodyFont("Bold", 8); }
	constexpr float MarkerGap = 5.0f;
	constexpr float FlagHorizontalShift = 0.62f;
	/** Pole from the edge of an object's mark to its flag. */
	constexpr float FlagPoleLength = 14.0f;
	constexpr float UnitCardHeight = 30.0f;
	constexpr float MergedLineHeight = 13.0f;
	/** Objects closer on screen than this share one card; a shared card holds together up to the second distance. */
	constexpr double MergeRadius = 6.0;
	constexpr double MergeHoldRadius = 10.0;
	/** A card without room of its own is listed on a placed card at most this far away, or left out. */
	constexpr double FallbackMergeRadius = 18.0;
	constexpr float ScreenMargin = 10.0f;
	/** Clear space around every card, and the room a card's first choice needs before the card returns to it. */
	constexpr float CardClearance = 3.0f;
	constexpr float ReturnClearance = 12.0f;
	/** Panels and instruments keep this much more space around them. */
	constexpr float PanelClearance = 8.0f;
	/** Middle dot between the names on a shared card. */
	const TCHAR* const ListSeparator = TEXT(" · ");

	/**
	 * The overlay projects hundreds of points per frame (orbit rings, markers). The engine's widget projection
	 * rebuilds the view from the camera on every call; this keeps one view per frame and derives the
	 * screen-to-viewport mapping (DPI scale and offset) from the same engine call, so results are unchanged.
	 */
	struct FProjectionFrame
	{
		const ASpaceship* Ship{nullptr};
		uint64 Frame{MAX_uint64};
		bool bValid{false};
		FMatrix ViewProjection{FMatrix::Identity};
		FIntRect ViewRect;
		FVector2D Offset{FVector2D::ZeroVector};
		FVector2D Scale{FVector2D::UnitVector};
	};
	FProjectionFrame GProjectionFrame;

	/** Card placement of the last painted frame, for GetNavigationMarkerLayout: contact index -> (anchor, card corner). */
	struct FLayoutFrame
	{
		const ASpaceship* Ship{nullptr};
		uint64 Frame{MAX_uint64};
		TMap<int32, TPair<FVector2D, FVector2D>> Layouts;
	};
	FLayoutFrame GLayoutFrame;
	/** Rio 02.10 ("labels jump over each other, now above, now below"): the slot each object's card took last, so a card
	 * keeps its place while that place stays free and only returns to its first choice when there is clear room. */
	TMap<TWeakObjectPtr<const AActor>, int32> GPreviousSlots;
	/** The object whose card an object shared last frame: a shared card holds together a little longer. */
	TMap<TWeakObjectPtr<const AActor>, TWeakObjectPtr<const AActor>> GPreviousLeaders;
	/** Size of the HUD overlay as last painted, the space the markers are projected into and drawn in. The viewport
	 * widget itself is larger than that by the UI scale whenever the scale is not 1. */
	FVector2D GHudSize{FVector2D::ZeroVector};
	/** The navigation (top right) and flight status (bottom left) panels of the HUD: no card covers them. */
	TWeakPtr<SWidget> GNavigationPanel;
	TWeakPtr<SWidget> GStatusPanel;

	enum class ECardKind : uint8
	{
		/** A navigation contact: star, planet, moon, station, colony, star-system beacon. */
		Contact,
		/** A ship of the civilization's fleet. */
		Unit
	};

	/** One object's card on the HUD, and where the layout put it. */
	struct FCard
	{
		ECardKind Kind{ECardKind::Contact};
		int32 ContactIndex{INDEX_NONE};
		TWeakObjectPtr<const AActor> Key;
		FVector2D Anchor{FVector2D::ZeroVector};
		/** Radius of the object's own mark (limb ring, diamond, course brackets): cards and leaders start beyond it. */
		float KeepOut{6.0f};
		/** Limb ring of a body; 0 when the body is too large on screen for one. */
		float RingRadius{0.0f};
		/** Rio 06.10 (aps.Ship.HudSubPixelRings): the ring's centre, unrounded; Anchor itself stays on whole pixels. */
		FVector2D RingCenter{FVector2D::ZeroVector};
		double Distance{0.0};
		bool bSelected{false};
		/** Which card leads a shared one: stars and planets, then moons, other contacts, ships. */
		int32 Rank{0};
		FLinearColor Color{FLinearColor::White};
		FString Title;
		/** Catalogue designation after the name (A, A7, A5.01), drawn in the marker colour. */
		FString Designation;
		FString Detail;
		/** The object's name where a shared card lists it. */
		FString ShortName;
		/** The objects this card stands for besides its own: name, and whether it is a ship. */
		TArray<TPair<FString, bool>> MergedNames;
		/** The texts as drawn, fitted to the card. */
		FString DrawTitle;
		FString DrawDetail;
		FString MergedLine;
		FVector2D Size{FVector2D::ZeroVector};
		/** The card this object is listed on instead of a card of its own. */
		int32 MergedInto{INDEX_NONE};
		bool bPlaced{false};
		int32 Slot{INDEX_NONE};
		FVector2D Position{FVector2D::ZeroVector};
	};

	/** Where a card sits around its mark: flags hang beside a vertical pole, side cards beside a short leader. */
	enum ESlotSide : int32
	{
		SideUpLeft,
		SideUpRight,
		SideRight,
		SideLeft,
		SideDownLeft,
		SideDownRight,
		SideRightUp,
		SideRightDown,
		SideLeftUp,
		SideLeftDown,
		SideCount
	};
	/** Level 0 is next to the mark; each further level stacks the card one card further out on a longer leader. */
	constexpr int32 SlotLevels = 7;

	/**
	 * Slots in order of preference. Flags first, as the HUD always drew them: a body's card above its mark with the pole
	 * on the card's right edge, a ship's card up and to the right. Then the mirrored and the side slots, those below, and
	 * the same again further out.
	 */
	void BuildSlotOrder(const ECardKind Kind, TArray<int32, TInlineAllocator<64>>& OutSlots)
	{
		constexpr int32 NearCount = 6;
		constexpr int32 FarCount = 8;
		static const int32 ContactNear[NearCount] = {SideUpLeft, SideUpRight, SideRight, SideLeft, SideDownLeft, SideDownRight};
		static const int32 UnitNear[NearCount] = {SideUpRight, SideDownRight, SideUpLeft, SideDownLeft, SideRight, SideLeft};
		static const int32 ContactFar[FarCount] = {SideUpLeft, SideUpRight, SideRightUp, SideLeftUp, SideDownLeft,
			SideDownRight, SideRightDown, SideLeftDown};
		static const int32 UnitFar[FarCount] = {SideUpRight, SideUpLeft, SideDownRight, SideDownLeft, SideRightUp,
			SideRightDown, SideLeftUp, SideLeftDown};
		const bool bUnit = Kind == ECardKind::Unit;
		const int32* Near = bUnit ? UnitNear : ContactNear;
		const int32* Far = bUnit ? UnitFar : ContactFar;
		OutSlots.Reset();
		for (int32 Index = 0; Index < NearCount; ++Index)
		{
			OutSlots.Add(Near[Index]);
		}
		for (int32 Level = 1; Level < SlotLevels; ++Level)
		{
			for (int32 Index = 0; Index < FarCount; ++Index)
			{
				OutSlots.Add(Level * SideCount + Far[Index]);
			}
		}
	}

	/** Top-left corner of a card of this size in a slot around its mark. */
	FVector2D SlotPosition(const FCard& Card, const int32 Slot, const FVector2D& Size)
	{
		const int32 Side = Slot % SideCount;
		const double Level = static_cast<double>(Slot / SideCount);
		const double Step = Size.Y + MarkerGap + 3.0;
		const bool bUnit = Card.Kind == ECardKind::Unit;
		// A flag's pole runs inside the accent bar on the card's near edge; a ship's card sits off a short diagonal leader.
		const double Inner = bUnit ? Card.KeepOut + 9.0 : -1.5;
		const double Rise = Card.KeepOut + (bUnit ? 11.0 : FlagPoleLength);
		constexpr double SideGap = 10.0;
		const FVector2D& Mark = Card.Anchor;
		const double UpY = Mark.Y - Rise - Size.Y - Level * Step;
		const double DownY = Mark.Y + Rise + Level * Step;
		const double MiddleY = Mark.Y - Size.Y * 0.5;
		const double RightX = Mark.X + Card.KeepOut + SideGap;
		const double LeftX = Mark.X - Card.KeepOut - SideGap - Size.X;
		switch (Side)
		{
		case SideUpLeft: return FVector2D(Mark.X - Inner - Size.X, UpY);
		case SideUpRight: return FVector2D(Mark.X + Inner, UpY);
		case SideDownLeft: return FVector2D(Mark.X - Inner - Size.X, DownY);
		case SideDownRight: return FVector2D(Mark.X + Inner, DownY);
		case SideRight: return FVector2D(RightX, MiddleY);
		case SideLeft: return FVector2D(LeftX, MiddleY);
		case SideRightUp: return FVector2D(RightX, MiddleY - Level * Step);
		case SideRightDown: return FVector2D(RightX, MiddleY + Level * Step);
		case SideLeftUp: return FVector2D(LeftX, MiddleY - Level * Step);
		default: return FVector2D(LeftX, MiddleY + Level * Step);
		}
	}

	FSlateRect CardRect(const FVector2D& Position, const FVector2D& Size)
	{
		return FSlateRect(static_cast<float>(Position.X), static_cast<float>(Position.Y),
			static_cast<float>(Position.X + Size.X), static_cast<float>(Position.Y + Size.Y));
	}

	FSlateRect Inflate(const FSlateRect& Rect, const float Amount)
	{
		return FSlateRect(Rect.Left - Amount, Rect.Top - Amount, Rect.Right + Amount, Rect.Bottom + Amount);
	}

	/** The leader from the edge of a card's mark to the nearest point of the card: a flag's pole, a side card's tick. */
	void LeaderSegment(const FCard& Card, const FSlateRect& Rect, FVector2D& OutStart, FVector2D& OutEnd)
	{
		OutEnd = FVector2D(FMath::Clamp(Card.Anchor.X, static_cast<double>(Rect.Left), static_cast<double>(Rect.Right)),
			FMath::Clamp(Card.Anchor.Y, static_cast<double>(Rect.Top), static_cast<double>(Rect.Bottom)));
		const FVector2D Delta = OutEnd - Card.Anchor;
		const double Length = Delta.Size();
		OutStart = Length > Card.KeepOut ? Card.Anchor + Delta * (Card.KeepOut / Length) : OutEnd;
	}

	/** True when the segment touches the rectangle (Liang-Barsky clipping). */
	bool SegmentHitsRect(const FVector2D& Start, const FVector2D& End, const FSlateRect& Rect)
	{
		const double DeltaX = End.X - Start.X;
		const double DeltaY = End.Y - Start.Y;
		const double Directions[4] = {-DeltaX, DeltaX, -DeltaY, DeltaY};
		const double Distances[4] = {Start.X - Rect.Left, Rect.Right - Start.X, Start.Y - Rect.Top, Rect.Bottom - Start.Y};
		double Enter = 0.0;
		double Leave = 1.0;
		for (int32 Edge = 0; Edge < 4; ++Edge)
		{
			if (FMath::IsNearlyZero(Directions[Edge]))
			{
				if (Distances[Edge] < 0.0)
				{
					return false;
				}
				continue;
			}
			const double Crossing = Distances[Edge] / Directions[Edge];
			if (Directions[Edge] < 0.0)
			{
				Enter = FMath::Max(Enter, Crossing);
			}
			else
			{
				Leave = FMath::Min(Leave, Crossing);
			}
			if (Enter > Leave)
			{
				return false;
			}
		}
		return true;
	}

	/** The text cut to MaxWidth, with an ellipsis where it had to be cut. */
	FString FitText(const FString& Text, const FSlateFontInfo& Font, const float MaxWidth, FSlateFontMeasure& FontMeasure)
	{
		if (FontMeasure.Measure(Text, Font).X <= MaxWidth)
		{
			return Text;
		}
		FString Fitted = Text;
		while (Fitted.Len() > 1 && FontMeasure.Measure(Fitted + TEXT("..."), Font).X > MaxWidth)
		{
			Fitted.LeftChopInline(1);
		}
		Fitted.TrimEndInline();
		return Fitted + TEXT("...");
	}

	/** Names joined by a middle dot, as many as fit MaxWidth, the rest counted: "S-04 · S-05 · M-01  +2". */
	FString JoinFitted(const TArray<FString>& Names, const FString& Prefix, const FSlateFontInfo& Font, const float MaxWidth,
		FSlateFontMeasure& FontMeasure)
	{
		FString Line = Prefix;
		for (int32 Index = 0; Index < Names.Num(); ++Index)
		{
			const FString Next = Index == 0 ? Line + Names[Index] : Line + ListSeparator + Names[Index];
			const int32 Rest = Names.Num() - Index - 1;
			if (Index > 0 && FontMeasure.Measure(Rest > 0 ? Next + FString::Printf(TEXT("  +%d"), Rest) : Next, Font).X > MaxWidth)
			{
				return FitText(Line + FString::Printf(TEXT("  +%d"), Names.Num() - Index), Font, MaxWidth, FontMeasure);
			}
			Line = Next;
		}
		return FitText(Line, Font, MaxWidth, FontMeasure);
	}

	/** Fits a card's texts and sizes the card: ships sharing a ship's card join its call sign ("S-04 · S-05"), anything
	 * else a card stands for goes on a line of its own ("+ S-04 · KRA A5.01"). */
	void SizeCard(FCard& Card, FSlateFontMeasure& FontMeasure)
	{
		const bool bUnit = Card.Kind == ECardKind::Unit;
		const float Padding = bUnit ? 20.0f : 26.0f;
		const float MaxText = MarkerMaximumWidth - Padding;
		TArray<FString> Joined;
		TArray<FString> Listed;
		for (const TPair<FString, bool>& Merged : Card.MergedNames)
		{
			(bUnit && Merged.Value ? Joined : Listed).Add(Merged.Key);
		}
		const FSlateFontInfo TitleFont = bUnit ? UnitTitleFont() : MarkerFont();
		const FSlateFontInfo DetailFont = bUnit ? UnitLineFont() : MarkerFont();
		const FString Suffix = Card.Designation.IsEmpty() || !Joined.IsEmpty() ? FString() : TEXT("  ") + Card.Designation;
		if (Joined.IsEmpty())
		{
			const float SuffixWidth = Suffix.IsEmpty() ? 0.0f : static_cast<float>(FontMeasure.Measure(Suffix, TitleFont).X);
			Card.DrawTitle = FitText(Card.Title, TitleFont, MaxText - SuffixWidth, FontMeasure);
		}
		else
		{
			Joined.Insert(Card.Title, 0);
			Card.DrawTitle = JoinFitted(Joined, FString(), TitleFont, MaxText, FontMeasure);
		}
		Card.DrawDetail = FitText(Card.Detail, DetailFont, MaxText, FontMeasure);
		Card.MergedLine = Listed.IsEmpty() ? FString() : JoinFitted(Listed, TEXT("+ "), MergedFont(), MaxText, FontMeasure);
		float TextWidth = static_cast<float>(FontMeasure.Measure(Card.DrawTitle + Suffix, TitleFont).X);
		TextWidth = FMath::Max(TextWidth, static_cast<float>(FontMeasure.Measure(Card.DrawDetail, DetailFont).X));
		if (!Card.MergedLine.IsEmpty())
		{
			TextWidth = FMath::Max(TextWidth, static_cast<float>(FontMeasure.Measure(Card.MergedLine, MergedFont()).X));
		}
		Card.Size.X = FMath::Clamp(TextWidth + Padding, bUnit ? 64.0f : MarkerMinimumWidth, MarkerMaximumWidth);
		Card.Size.Y = (bUnit ? UnitCardHeight : MarkerHeight) + (Card.MergedLine.IsEmpty() ? 0.0f : MergedLineHeight);
	}

	/**
	 * Rio 02.10 ("labels pile onto each other"): every card of the frame (planets, moons, stations, beacons, ships) is laid
	 * out in one pass, so no two overlap whatever their kind. Objects within a few pixels share one card. Then, greedily
	 * by priority (the course target first, then the nearest), each card takes the first free slot around its mark:
	 * beside it, above or below it, then stacked further out on a longer leader. A card keeps last frame's slot while it
	 * stays free, so cards do not jump. A card without room is listed on a placed card a few pixels away, or left out
	 * (its mark stays). No card leaves the screen, covers a panel, another card or another card's leader.
	 */
	void LayOutCards(TArray<FCard>& Cards, const TArray<FSlateRect>& Obstacles, const FVector2D& Screen,
		FSlateFontMeasure& FontMeasure)
	{
		const int32 CardCount = Cards.Num();
		// Who leads a shared card: the course target, then stars and planets, moons, other contacts, ships; the nearest.
		TArray<int32, TInlineAllocator<64>> Leadership;
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			Leadership.Add(Index);
		}
		Leadership.Sort([&Cards](const int32 Left, const int32 Right)
		{
			const FCard& A = Cards[Left];
			const FCard& B = Cards[Right];
			if (A.bSelected != B.bSelected) return A.bSelected;
			if (A.Rank != B.Rank) return A.Rank < B.Rank;
			return A.Distance < B.Distance;
		});
		for (int32 LeaderOrder = 0; LeaderOrder < Leadership.Num(); ++LeaderOrder)
		{
			const int32 LeaderIndex = Leadership[LeaderOrder];
			if (Cards[LeaderIndex].MergedInto != INDEX_NONE)
			{
				continue;
			}
			for (int32 MemberOrder = LeaderOrder + 1; MemberOrder < Leadership.Num(); ++MemberOrder)
			{
				FCard& Member = Cards[Leadership[MemberOrder]];
				const FCard& Leader = Cards[LeaderIndex];
				if (Member.MergedInto != INDEX_NONE || Member.bSelected)
				{
					continue;
				}
				const double Gap = FVector2D::Distance(Leader.Anchor, Member.Anchor);
				const TWeakObjectPtr<const AActor>* Previous = Member.Key.IsValid() ? GPreviousLeaders.Find(Member.Key) : nullptr;
				const bool bHeld = Previous && Leader.Key.IsValid() && *Previous == Leader.Key;
				if (Gap <= MergeRadius || (bHeld && Gap <= MergeHoldRadius))
				{
					Member.MergedInto = LeaderIndex;
				}
			}
		}
		for (const int32 Index : Leadership)
		{
			if (const int32 Host = Cards[Index].MergedInto; Host != INDEX_NONE)
			{
				Cards[Host].MergedNames.Emplace(Cards[Index].ShortName, Cards[Index].Kind == ECardKind::Unit);
			}
		}

		// Greedy by priority: the course target first, then the nearest.
		TArray<int32, TInlineAllocator<64>> Placement;
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			if (Cards[Index].MergedInto == INDEX_NONE)
			{
				SizeCard(Cards[Index], FontMeasure);
				Placement.Add(Index);
			}
		}
		Placement.Sort([&Cards](const int32 Left, const int32 Right)
		{
			const FCard& A = Cards[Left];
			const FCard& B = Cards[Right];
			if (A.bSelected != B.bSelected) return A.bSelected;
			return A.Distance < B.Distance;
		});

		struct FPlaced
		{
			int32 Card{INDEX_NONE};
			FSlateRect Rect;
			FVector2D LeaderStart{FVector2D::ZeroVector};
			FVector2D LeaderEnd{FVector2D::ZeroVector};
		};
		TArray<FPlaced, TInlineAllocator<64>> Placed;
		// A slot is free when the card stays on screen with its margin, keeps clear of the panels and the placed cards,
		// its leader crosses neither, it covers no placed leader and, while bAvoidMarks, no other object's mark.
		const auto Fits = [&](const FCard& Card, const int32 CardIndex, const int32 Slot, const float Clearance,
			const bool bAvoidMarks)
		{
			const FSlateRect Rect = CardRect(SlotPosition(Card, Slot, Card.Size), Card.Size);
			if (Rect.Left < ScreenMargin || Rect.Top < ScreenMargin
				|| Rect.Right > Screen.X - ScreenMargin || Rect.Bottom > Screen.Y - ScreenMargin)
			{
				return false;
			}
			// Rio 03.10 ("with the whole body in view its label goes above it"): a card never covers its own body inside
			// the limb ring; the slots above the ring come first.
			if (Card.RingRadius > 0.0f)
			{
				const FVector2D Nearest(
					FMath::Clamp(Card.Anchor.X, static_cast<double>(Rect.Left), static_cast<double>(Rect.Right)),
					FMath::Clamp(Card.Anchor.Y, static_cast<double>(Rect.Top), static_cast<double>(Rect.Bottom)));
				if (FVector2D::DistSquared(Nearest, Card.Anchor) < FMath::Square(static_cast<double>(Card.RingRadius) + 2.0))
				{
					return false;
				}
			}
			const FSlateRect Padded = Inflate(Rect, Clearance);
			FVector2D LeaderStart;
			FVector2D LeaderEnd;
			LeaderSegment(Card, Rect, LeaderStart, LeaderEnd);
			for (const FSlateRect& Obstacle : Obstacles)
			{
				// A mark inside an obstacle (a moon within the course brackets of its planet) still gets a card outside it.
				const bool bMarkInside = Card.Anchor.X >= Obstacle.Left && Card.Anchor.X <= Obstacle.Right
					&& Card.Anchor.Y >= Obstacle.Top && Card.Anchor.Y <= Obstacle.Bottom;
				if (FSlateRect::DoRectanglesIntersect(Padded, Obstacle)
					|| (!bMarkInside && SegmentHitsRect(LeaderStart, LeaderEnd, Obstacle)))
				{
					return false;
				}
			}
			const FSlateRect LeaderGuard = Inflate(Rect, 2.0f);
			for (const FPlaced& Other : Placed)
			{
				if (Other.Card == CardIndex)
				{
					continue;
				}
				// A leader may start under a card placed earlier (its mark covered there); it never runs into one.
				const bool bMarkUnder = Card.Anchor.X >= Other.Rect.Left && Card.Anchor.X <= Other.Rect.Right
					&& Card.Anchor.Y >= Other.Rect.Top && Card.Anchor.Y <= Other.Rect.Bottom;
				if (FSlateRect::DoRectanglesIntersect(Padded, Inflate(Other.Rect, CardClearance))
					|| (!bMarkUnder && SegmentHitsRect(LeaderStart, LeaderEnd, Other.Rect))
					|| SegmentHitsRect(Other.LeaderStart, Other.LeaderEnd, LeaderGuard))
				{
					return false;
				}
			}
			if (bAvoidMarks)
			{
				for (int32 OtherIndex = 0; OtherIndex < CardCount; ++OtherIndex)
				{
					const FCard& Other = Cards[OtherIndex];
					if (OtherIndex == CardIndex || Other.MergedInto == CardIndex)
					{
						continue;
					}
					const double Mark = FMath::Min(static_cast<double>(Other.KeepOut), 24.0);
					if (FSlateRect::DoRectanglesIntersect(Padded, FSlateRect(static_cast<float>(Other.Anchor.X - Mark),
						static_cast<float>(Other.Anchor.Y - Mark), static_cast<float>(Other.Anchor.X + Mark),
						static_cast<float>(Other.Anchor.Y + Mark))))
					{
						return false;
					}
				}
			}
			return true;
		};
		const auto Settle = [&Cards](const int32 CardIndex, FPlaced& Entry)
		{
			FCard& Card = Cards[CardIndex];
			Card.Position = SlotPosition(Card, Card.Slot, Card.Size);
			Entry.Card = CardIndex;
			Entry.Rect = CardRect(Card.Position, Card.Size);
			LeaderSegment(Card, Entry.Rect, Entry.LeaderStart, Entry.LeaderEnd);
		};

		TArray<int32, TInlineAllocator<64>> ContactSlots;
		TArray<int32, TInlineAllocator<64>> UnitSlots;
		BuildSlotOrder(ECardKind::Contact, ContactSlots);
		BuildSlotOrder(ECardKind::Unit, UnitSlots);
		for (const int32 CardIndex : Placement)
		{
			FCard& Card = Cards[CardIndex];
			const TArray<int32, TInlineAllocator<64>>& Slots = Card.Kind == ECardKind::Unit ? UnitSlots : ContactSlots;
			const int32* Previous = Card.Key.IsValid() ? GPreviousSlots.Find(Card.Key) : nullptr;
			const bool bKnownPrevious = Previous && Slots.Contains(*Previous);
			int32 Chosen = INDEX_NONE;
			// Its first choice when that is clearly free; else last frame's slot while it stays free; else the first
			// free slot, clear of other objects' marks where possible.
			if (bKnownPrevious && *Previous != Slots[0] && Fits(Card, CardIndex, Slots[0], ReturnClearance, true))
			{
				Chosen = Slots[0];
			}
			else if (bKnownPrevious && Fits(Card, CardIndex, *Previous, CardClearance, false))
			{
				Chosen = *Previous;
			}
			for (int32 Pass = 0; Pass < 2 && Chosen == INDEX_NONE; ++Pass)
			{
				for (const int32 Slot : Slots)
				{
					// The second pass, which may cover other objects' marks, keeps to the nearer slots (bounded cost).
					if (Pass == 1 && Slot >= 3 * SideCount)
					{
						break;
					}
					if (Fits(Card, CardIndex, Slot, CardClearance, Pass == 0))
					{
						Chosen = Slot;
						break;
					}
				}
			}
			if (Chosen != INDEX_NONE)
			{
				Card.bPlaced = true;
				Card.Slot = Chosen;
				Settle(CardIndex, Placed.AddDefaulted_GetRef());
				continue;
			}

			// No room: listed on the nearest placed card a few pixels away (a ship's card lists only ships), or left out.
			int32 HostEntry = INDEX_NONE;
			double HostGap = FallbackMergeRadius;
			for (int32 EntryIndex = 0; EntryIndex < Placed.Num(); ++EntryIndex)
			{
				const FCard& Host = Cards[Placed[EntryIndex].Card];
				const double Gap = FVector2D::Distance(Host.Anchor, Card.Anchor);
				if ((Host.Kind != ECardKind::Unit || Card.Kind == ECardKind::Unit) && Gap <= HostGap)
				{
					HostGap = Gap;
					HostEntry = EntryIndex;
				}
			}
			if (HostEntry == INDEX_NONE)
			{
				continue;
			}
			const int32 HostIndex = Placed[HostEntry].Card;
			const FCard Saved = Cards[HostIndex];
			{
				FCard& Host = Cards[HostIndex];
				Host.MergedNames.Emplace(Card.ShortName, Card.Kind == ECardKind::Unit);
				Host.MergedNames.Append(Card.MergedNames);
				SizeCard(Host, FontMeasure);
			}
			if (Fits(Cards[HostIndex], HostIndex, Cards[HostIndex].Slot, CardClearance, false))
			{
				Settle(HostIndex, Placed[HostEntry]);
				for (FCard& Other : Cards)
				{
					if (Other.MergedInto == CardIndex)
					{
						Other.MergedInto = HostIndex;
					}
				}
				Card.MergedInto = HostIndex;
			}
			else
			{
				Cards[HostIndex] = Saved;
			}
		}

		GPreviousSlots.Reset();
		GPreviousLeaders.Reset();
		for (const FCard& Card : Cards)
		{
			if (!Card.Key.IsValid())
			{
				continue;
			}
			if (Card.bPlaced)
			{
				GPreviousSlots.Add(Card.Key, Card.Slot);
			}
			else if (Card.MergedInto != INDEX_NONE && Cards[Card.MergedInto].Key.IsValid())
			{
				GPreviousLeaders.Add(Card.Key, Cards[Card.MergedInto].Key);
			}
		}
	}

	FLinearColor PlanetMarkerColor(const EPlanetType Type)
	{
		switch (Type)
		{
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Nordic:
		case EPlanetType::Tundra:
		case EPlanetType::IceGiant:
			return FLinearColor(0.72f, 0.9f, 1.0f, 0.96f);
		case EPlanetType::Ocean:
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			return FLinearColor(0.16f, 0.62f, 1.0f, 0.96f);
		case EPlanetType::Terrestrial:
		case EPlanetType::Forest:
		case EPlanetType::Oasis:
		case EPlanetType::Pangea:
		case EPlanetType::SuperEarth:
			return FLinearColor(0.2f, 0.92f, 0.58f, 0.96f);
		case EPlanetType::Desert:
		case EPlanetType::Sand:
			return FLinearColor(1.0f, 0.68f, 0.24f, 0.96f);
		case EPlanetType::Volcanic:
		case EPlanetType::Melted:
		case EPlanetType::Lava:
		case EPlanetType::HotGiant:
			return FLinearColor(1.0f, 0.25f, 0.1f, 0.96f);
		case EPlanetType::GasGiant:
		case EPlanetType::Greenhouse:
		case EPlanetType::Ammonia:
			return FLinearColor(0.92f, 0.72f, 0.3f, 0.96f);
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
		case EPlanetType::Carbon:
			return FLinearColor(0.74f, 0.72f, 0.88f, 0.96f);
		default:
			return FLinearColor(0.28f, 0.84f, 0.75f, 0.95f);
		}
	}

	FLinearColor MoonMarkerColor(const EMoonType Type)
	{
		switch (Type)
		{
		case EMoonType::Icy:
			return FLinearColor(0.82f, 0.93f, 1.0f, 0.96f);
		case EMoonType::Ocean:
			return FLinearColor(0.22f, 0.64f, 1.0f, 0.96f);
		case EMoonType::Continental:
			return FLinearColor(0.38f, 0.84f, 0.65f, 0.96f);
		case EMoonType::Desert:
			return FLinearColor(0.96f, 0.67f, 0.34f, 0.96f);
		case EMoonType::Volcanic:
			return FLinearColor(1.0f, 0.31f, 0.12f, 0.96f);
		case EMoonType::Iron:
			return FLinearColor(0.68f, 0.74f, 0.82f, 0.96f);
		case EMoonType::Gas:
		case EMoonType::Peculiar:
			return FLinearColor(0.72f, 0.52f, 1.0f, 0.96f);
		default:
			return FLinearColor(0.66f, 0.76f, 0.9f, 0.95f);
		}
	}

	/** Marker colour of a planet or a moon by its type; false for any other actor. */
	bool BodyMarkerColor(const AActor* Actor, FLinearColor& OutColor)
	{
		if (const APlanet* Planet = Cast<APlanet>(Actor))
		{
			OutColor = PlanetMarkerColor(Planet->PlanetType);
			return true;
		}
		if (const AMoon* Moon = Cast<AMoon>(Actor))
		{
			OutColor = MoonMarkerColor(Moon->MoonType);
			return true;
		}
		return false;
	}

	/** Altimeter of the piloted ship (Rio 02.10): the planet or moon it reads and what it showed last. */
	struct FAltimeter
	{
		const ASpaceship* Ship{nullptr};
		TWeakObjectPtr<const APlanetaryBody> Body;
		double LastSeconds{0.0};
		double PickSeconds{-1.0e9};
		double SampleSeconds{-1.0e9};
		/** Ground clearance at the last terrain sample and the base-sphere altitude then: between the samples the
		 * clearance follows the change of the base altitude, so a fast descent never reads a stale sample. */
		double SampleGroundCm{-1.0};
		double SampleBaseCm{0.0};
		double AltitudeCm{0.0};
		double VerticalCmPerSecond{0.0};
		/** Top of the tape and the edge of the atmosphere (negative without one), cm above the surface. */
		double TopCm{1.0};
		double AtmosphereCm{-1.0};
		/** Fades in near a body and out after leaving it, showing the last reading meanwhile. */
		float Alpha{0.0f};
		/** 0 calm, 1 amber, 2 red: closing on the surface fast. */
		float Severity{0.0f};
		bool bReading{false};
		FString BodyName;
		FString Designation;
		FLinearColor BodyColor{FLinearColor::White};
	};
	FAltimeter GAltimeter;
	/** The altimeter reads a planet or moon within this many of its radii above the surface. */
	constexpr double AltimeterReachRadii = 2.0;
	/** Above this the base sphere stands in for the terrain (the flight model's limit for its terrain queries). */
	constexpr double AltimeterTerrainAltitudeCm = 5.0e7;

	/**
	 * Height of Location above the WorldScape terrain of Body, or above the sea of a liquid world: the canonical surface
	 * the flight model and the placement resolver sample too. -1 while the body has no current surface.
	 */
	double TerrainClearanceCm(const APlanetaryBody* Body, const FVector& Location)
	{
		APlanetarySurfaceGenerator* Surface = IsValid(Body) ? Body->PlanetaryEnvironmentGenerator : nullptr;
		AWorldScapeRoot* Root = IsValid(Surface) ? Surface->WorldScapeRootInstance : nullptr;
		if (!IsValid(Root) || !IsValid(Root->WorldScapeNoise) || Root->PlanetScale <= 0.0
			|| !Surface->IsSurfaceProfileCurrent(Body))
		{
			return -1.0;
		}
		const FVector Center = Root->GetActorLocation();
		const FVector Offset = Location - Center;
		const double CenterDistance = Offset.Size();
		if (CenterDistance <= UE_DOUBLE_SMALL_NUMBER)
		{
			return -1.0;
		}
		const FVector Direction = Offset / CenterDistance;
		double SurfaceRadius = Root->PlanetScale + Root->GetGroundHeight(Center + Direction * Root->PlanetScale, false);
		if (Surface->ResolvedSurfaceProfile.LiquidType != EAPSPlanetLiquidType::None)
		{
			SurfaceRadius = FMath::Max(SurfaceRadius, Root->PlanetScale
				+ static_cast<double>(Surface->ResolvedSurfaceProfile.OceanLevel) * Root->NoiseIntensity);
		}
		return FMath::Max(CenterDistance - SurfaceRadius, 0.0);
	}

	FString FormatAltitude(const double Centimetres)
	{
		const double Metres = FMath::Max(Centimetres, 0.0) / 100.0;
		if (Metres < 1000.0)
		{
			return APSUINumber::Number(FMath::RoundToInt(Metres)).ToString() + TEXT(" m");
		}
		if (Metres < 100000.0)
		{
			return FString::Printf(TEXT("%.1f km"), Metres / 1000.0);
		}
		return APSUINumber::Number(FMath::RoundToInt64(Metres / 1000.0)).ToString() + TEXT(" km");
	}

	/** A round altitude of the tape scale: 100 m, 1 km ... 10,000 km. */
	FString FormatTick(const double Centimetres)
	{
		const double Metres = Centimetres / 100.0;
		return Metres < 1000.0
			? APSUINumber::Number(FMath::RoundToInt(Metres)).ToString() + TEXT(" m")
			: APSUINumber::Number(FMath::RoundToInt64(Metres / 1000.0)).ToString() + TEXT(" km");
	}

	FString FormatVerticalSpeed(const double CentimetresPerSecond)
	{
		const double Metres = FMath::Abs(CentimetresPerSecond) / 100.0;
		if (Metres < 10.0)
		{
			return FString::Printf(TEXT("%.1f m/s"), Metres);
		}
		if (Metres < 1000.0)
		{
			return FString::Printf(TEXT("%.0f m/s"), Metres);
		}
		if (Metres < 1.0e6)
		{
			return FString::Printf(TEXT("%.1f km/s"), Metres / 1000.0);
		}
		return APSUINumber::Number(FMath::RoundToInt64(Metres / 1000.0)).ToString() + TEXT(" km/s");
	}

	FString FormatSurfaceTime(const double Seconds)
	{
		if (Seconds < 10.0)
		{
			return FString::Printf(TEXT("%.1f s"), Seconds);
		}
		if (Seconds < 60.0)
		{
			return FString::Printf(TEXT("%.0f s"), Seconds);
		}
		if (Seconds < 3600.0)
		{
			const int32 Whole = FMath::FloorToInt(Seconds);
			return FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60);
		}
		if (Seconds < 36000.0)
		{
			const int32 Minutes = FMath::FloorToInt(Seconds / 60.0);
			return FString::Printf(TEXT("%d h %02d min"), Minutes / 60, Minutes % 60);
		}
		return TEXT("over 10 h");
	}
}

namespace APSAutomaticShipInteraction
{
	const FVector NativeExitLocation(0.0, -200.0, 100.0);

	bool IsNativeSeatTransform(const USceneComponent* Component)
	{
		return Component
			&& Component->GetRelativeLocation().IsNearlyZero(0.1)
			&& Component->GetRelativeRotation().IsNearlyZero(0.1);
	}

	bool IsNativeExitTransform(const USceneComponent* Component)
	{
		return Component
			&& Component->GetRelativeLocation().Equals(NativeExitLocation, 0.1)
			&& Component->GetRelativeRotation().IsNearlyZero(0.1);
	}

	// Rio 09.10 (playtest 08.10 items 1-3, Single: stuck in the hull, a corridor instead of the bridge, no gravity): the authored
	// L_APS_SinglePlay_StartLocation (29.09) saved for every placed ship the bounds-fallback seat, exit point and sphere of the
	// hulls of that time; on today's refit hulls those seats sit 2-30 cm from the hull's pivot (the keel on M_P2_01/02/03, S and L
	// ships), and the pilot got up there. See RefreshInteractionGeometry.
	TAutoConsoleVariable<int32> CVarCabinSeatFromSocket(
		TEXT("aps.Ship.CabinSeatFromSocket"), 1,
		TEXT("Rio 09.10 (playtest 08.10 items 1-3): 1: a hull with an authored cabin seat socket (PilotSeat/PilotChair/CockpitSeat) ")
		TEXT("always takes its seat, its exit point and its interaction/gravity sphere from the hull, replacing what a level ")
		TEXT("instance saved for those native components (L_APS_SinglePlay_StartLocation keeps 29.09 fallback seats 2-30 cm from ")
		TEXT("today's hull pivots). 0: a saved instance transform is kept, as before."));

	UStaticMeshComponent* FindLargestMesh(const ASpaceship* Ship)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		Ship->GetComponents(MeshComponents);

		UStaticMeshComponent* BestMesh = nullptr;
		double BestBoundsSizeSquared = 0.0;
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			// A collision shell (an invisible complex-collision copy of the hull, tagged) is not the hull.
			if (!IsValid(MeshComponent) || !MeshComponent->GetStaticMesh() || MeshComponent == Ship->ForwardVector
				|| MeshComponent->ComponentHasTag(TEXT("APS.Ship.CollisionShell")))
			{
				continue;
			}

			MeshComponent->UpdateBounds();
			const double BoundsSizeSquared = MeshComponent->Bounds.BoxExtent.SizeSquared();
			if (BoundsSizeSquared > BestBoundsSizeSquared)
			{
				BestMesh = MeshComponent;
				BestBoundsSizeSquared = BoundsSizeSquared;
			}
		}

		return BestMesh;
	}

	bool FindSocketTransform(const USceneComponent* MeshComponent, const TArray<FName>& SocketNames,
		FTransform& OutTransform)
	{
		for (const FName SocketName : SocketNames)
		{
			if (MeshComponent->DoesSocketExist(SocketName))
			{
				OutTransform = MeshComponent->GetSocketTransform(SocketName, RTS_World);
				return true;
			}
		}
		return false;
	}
}

/*
press shift + hold Lshift = busting
double press Lshift = increase engine mode
*/

void ASpaceship::UpdateNavigatableActorsForInterstellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterstellar"));

	WorldNavigatableActors.Empty();
	if (LastFlightMode == EFlightMode::Stellar)
	{
		ToggleScale();
		bIsScaled = true;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Downscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForStellar()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForStellar"));

	TArray<AActor*> WorldActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWorldActor::StaticClass(), WorldActors);
	for (AActor* Actor : WorldActors)
	{
		if (Actor->GetClass()->ImplementsInterface(UNavigatableBody::StaticClass()))
		{
			AWorldActor* WorldNavigatableActor = Cast<AWorldActor>(Actor);
			WorldNavigatableActors.Add(WorldNavigatableActor);
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow,
			                                 FString::Printf(
				                                 TEXT("WorldActor: %s"), *WorldNavigatableActor->GetName()));
		}
	}

	if (LastFlightMode == EFlightMode::Interstellar)
	{
		// reorigin star system and ship to 0 0 0
		FVector PlayerLocation = this->GetActorLocation();
		FVector NewSystemLocation = OffsetSystem->GetActorLocation() - PlayerLocation;
		OffsetSystem->SetActorLocation(NewSystemLocation, false);
		this->SetActorLocation(FVector(0, 0, 0), false);

		ToggleScale();
		bIsScaled = false;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Galaxy is Upscaled")));
	}
}

void ASpaceship::UpdateNavigatableActorsForInterplanetary()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("UpdateNavigatableActorsForInterplanetary"));
}

namespace APSShipPerf
{
	TAutoConsoleVariable<int32> CVarShipPerfLog(
		TEXT("aps.Ship.PerfLog"), 0,
		TEXT("1 logs the piloted ship's tick cost per section (environment, navigation, move/sweep, rotation, camera, HUD) every 2 s."));
	TAutoConsoleVariable<int32> CVarSweepPrecheck(
		TEXT("aps.Ship.SweepPrecheck"), 1,
		TEXT("1 tests a kinematic ship move with the hull's bounding sphere first and sweeps the hull body only when "
			"that sphere may hit. 0 always sweeps the hull body (every convex hull separately)."));
	TAutoConsoleVariable<int32> CVarHudProjectionCheck(
		TEXT("aps.Ship.HudProjectionCheck"), 0,
		TEXT("1 compares the navigation HUD's per-frame projection with the engine's per-call projection and logs ")
		TEXT("the largest difference every second (verification only)."));
	TAutoConsoleVariable<int32> CVarResetHullVelocity(
		TEXT("aps.Ship.ResetHullVelocity"), 1,
		TEXT("1 removes the piloted kinematic ship's primitives from the renderer's velocity data every frame (the behaviour ")
		TEXT("before 29.09): TSR and Lumen then reproject the hull as static world geometry while the camera flies with it. ")
		TEXT("0 leaves the engine's own motion vectors (A/B for the hull shimmer at speed)."));
	TAutoConsoleVariable<int32> CVarTsrHistoryInFlight(
		TEXT("aps.Ship.TsrHistoryInFlight"), 1,
		TEXT("Rio 05.10 (\"the edges still ripple at speed, walking aboard too\"): above ~6 km/s the camera moves over 100 m ")
		TEXT("a frame and the renderer dropped TSR's history every frame (r.CameraCutTranslationThreshold; r.TSR.Visualize 0 ")
		TEXT("showed nothing accumulated), so edges and noise flickered anew each frame. 1 (default since the evening's A/B) ")
		TEXT("keeps the history while the view rides a ship (flown or walked in; the strategic map and taking or leaving the ")
		TEXT("seat still cut): that threshold goes up, and the piloted hull keeps its own motion vectors (aps.Ship.")
		TEXT("ResetHullVelocity is skipped). 0: as before."));
	TAutoConsoleVariable<float> CVarTsrHistoryMaxStepKm(
		TEXT("aps.Ship.TsrHistoryMaxStepKm"), 500.0f,
		TEXT("Rio 05.10 night: with aps.Ship.TsrHistoryInFlight, TSR keeps its history up to this camera step a frame, km ")
		TEXT("(r.CameraCutTranslationThreshold); beyond it the reprojection is too coarse and history would smear."));
	/**
	 * Raises r.CameraCutTranslationThreshold while aps.Ship.TsrHistoryInFlight is on and the player's view rides a ship
	 * (flown, or walked in), and gives the saved value back otherwise. Rio 05.10 evening: a camera moving more than the
	 * threshold in a frame (UE 5.4 SceneVisibility.cpp: in a REAL SCALE flow the renderer even moves the previous view
	 * by the frame's whole world shift) gets a fresh previous view, so TSR loses its history every frame and the edges
	 * alias anew with each jitter. A ship's view moves with the ship and its motion vectors are right. On a core ticker,
	 * so the value comes back even when no ship ticks (the player on foot, a level change).
	 */
	bool TickTsrHistory(float)
	{
		static IConsoleVariable* const Threshold =
			IConsoleManager::Get().FindConsoleVariable(TEXT("r.CameraCutTranslationThreshold"));
		static float Saved = -1.0f;
		if (!Threshold || !GEngine)
		{
			return true;
		}
		bool bRidesShip = false;
		if (CVarTsrHistoryInFlight.GetValueOnGameThread() != 0)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				const UWorld* World = Context.World();
				const APlayerController* Controller = World
					&& (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
					? World->GetFirstPlayerController() : nullptr;
				// The view itself must ride the ship: the strategic map's camera (and its blend back) jumps hundreds of AU
				// and still needs the renderer's own cut.
				const AActor* ViewTarget = Controller && Controller->PlayerCameraManager
					? Controller->PlayerCameraManager->GetViewTarget() : nullptr;
				for (const AActor* Link = ViewTarget; Link && !bRidesShip; Link = Link->GetAttachParentActor())
				{
					bRidesShip = Link->IsA<ASpaceship>();
				}
			}
		}
		// Rio 05.10 night (the hull's texture smeared at legacy-scale cruise speeds): past ~500 km a frame the float maths
		// of TSR's reprojection is off by more than a tenth of a pixel at the hull's distance, and kept history smears; above
		// that the renderer's own cut (and its edge ripple) is the lesser evil. A REAL SCALE flow keeps the view still, so
		// there the cut never comes.
		const float KeptStepCm = FMath::Max(CVarTsrHistoryMaxStepKm.GetValueOnGameThread(), 0.0f) * 1.0e5f;
		if (bRidesShip && (Saved < 0.0f || Threshold->GetFloat() != KeptStepCm))
		{
			if (Saved < 0.0f)
			{
				Saved = Threshold->GetFloat();
			}
			Threshold->Set(KeptStepCm, ECVF_SetByCode);
		}
		else if (!bRidesShip && Saved >= 0.0f)
		{
			Threshold->Set(Saved, ECVF_SetByCode);
			Saved = -1.0f;
		}
		return true;
	}
	FDelayedAutoRegisterHelper GTsrHistoryRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickTsrHistory), 0.0f);
	});
	TAutoConsoleVariable<float> CVarIdleCameraTickSeconds(
		TEXT("aps.Ship.IdleCameraTickSeconds"), 0.5f,
		TEXT("Rio 05.10 (night optimisation; a trace at drive speed: the camera arms of six pawns cost 1.5 ms a frame, the ")
		TEXT("ground vehicles' with a collision sweep each): the camera arm of a ship the player does not fly ticks this often, ")
		TEXT("s (its camera shows nothing); the piloted one every frame. 0: every frame, as before."));
	TAutoConsoleVariable<int32> CVarProxySweep(
		TEXT("aps.Ship.ProxySweep"), 1,
		TEXT("1 sweeps the flight proxy boxes of detailed hulls (M3 and similar) so they collide with stations, ships and ")
		TEXT("terrain; 0 restores the collision-free root move of those ships (before 29.09)."));
	// Rio 06.10 ("20 fps in flight, unplayable"): on by default. Same flight, same build (p1-far-b/-h): near the planet
	// 68 -> 112 fps (game thread 17.3 -> 5.3 ms, hitches 23 -> 1), CRUISE 68 -> 96, STELLAR 90 -> 109; nothing drawn changes.
	TAutoConsoleVariable<int32> CVarDetailedHullFlightProxy(
		TEXT("aps.Ship.DetailedHullFlightProxy"), 1,
		TEXT("Rio 05.10 evening (flight FPS; on by default since 06.10): 1 flies a piloted ship whose hull has more collision shapes ")
		TEXT("than aps.Ship.DetailedHullShapes on the flight proxy boxes, as the M3 does, and takes the hull's own body out ")
		TEXT("of the physics scene until the pilot gets up (the M5's 13.5k convex shapes cost ~6 ms a frame in any flight: ")
		TEXT("every move of the hull and every query near it walks them all). 0: as the ship is authored."));
	TAutoConsoleVariable<int32> CVarDetailedHullShapes(
		TEXT("aps.Ship.DetailedHullShapes"), 512,
		TEXT("Collision shapes from which aps.Ship.DetailedHullFlightProxy treats a hull as detailed."));
	// Rio 06.10 ("20 fps in flight", and he hates freezes): with the flight proxy on by default, every time the pilot gets up
	// the M5's 13.5k-shape body is built again in one frame (~300 ms in the restore log line). It must be built exactly once,
	// already carrying its final profile and responses, and not at all for a world or a ship that is going away.
	// Rio 06.10 afternoon (no regressions from the accepted base): never seen in a run; 0 = the base build's restore path.
	TAutoConsoleVariable<int32> CVarHullRestoreOnce(
		TEXT("aps.Ship.HullRestoreOnce"), 0,
		TEXT("Rio 06.10 (freeze when the pilot gets up): 1 gives a hull whose body left the scene in flight its profile, ")
		TEXT("responses and kinematic state while it is still out, then builds the body once with them; a world being torn ")
		TEXT("down or a ship being destroyed gets no body back at all. 0 restores profile, type and responses one call ")
		TEXT("after another (each may build or re-filter the body), as before 06.10."));
	// Rio 06.10 (review of the night's ship group): the proxy build switches the hull's overlap events off and the 61532ed6
	// build never gave them back, so after the first flight every hull moved without them. With them on again, a move of the
	// hull's own body updates its overlaps and the M5 hull sweep stands aside for the engine's body sweep (TryMoveHull
	// bails on overlap events): a separate switch, so an A/B control can reproduce that build exactly.
	// Rio 06.10 afternoon: default 0 = exactly the 61532ed6 build (overlaps stay off after the first flight; with them on,
	// every move of the 13.5k-shape body walked its shapes for overlaps and the M5 sweep stood aside).
	TAutoConsoleVariable<int32> CVarHullRestoreOverlaps(
		TEXT("aps.Ship.HullRestoreOverlaps"), 0,
		TEXT("Rio 06.10 (aps.Ship.HullRestoreOnce 1): 1 gives a restored hull its authored overlap events back (the flight ")
		TEXT("proxy switches them off); 0 leaves them off after the first flight, as the 61532ed6 build did (set it together ")
		TEXT("with aps.Ship.KeepHullOutWhileMoving 0 to reproduce that build's walk aboard a moving ship)."));
	TAutoConsoleVariable<int32> CVarHullSceneLightingInFlight(
		TEXT("aps.Ship.HullSceneLightingInFlight"), 2,
		TEXT("2 (default since Rio's check on 29.09) takes a piloted ship out of the global distance field: that copy ")
		TEXT("updates a frame behind a hull moving hundreds of metres per frame and lit it on alternate frames (shimmer ")
		TEXT("27 -> 4; the hull loses some self-bounce light). 1 keeps the ship in the distance field and the Lumen scene ")
		TEXT("(before 29.09), 3 takes it out of the Lumen scene only, 0 out of both. Changes apply in flight; the flags ")
		TEXT("come back when the pilot leaves."));
	TAutoConsoleVariable<int32> CVarHudOrbitPlanets(
		TEXT("aps.Hud.OrbitPlanets"), 0,
		TEXT("Rio 04.10 (\"orbits for every planet\"): 0 draws the orbit of every planet listed (a system seen from outside is ")
		TEXT("one card, so these are the system's own); N draws only the N nearest (4 before: in a system of nine the others ")
		TEXT("sat between rings that were not theirs)."));
	TAutoConsoleVariable<int32> CVarSpeedFov(
		TEXT("aps.Ship.SpeedFov"), 0,
		TEXT("1 widens the flight camera's field of view with speed (up to +12 deg, the camera before 29.09). Every ")
		TEXT("frame of that widening re-sizes and re-uploads the full-scale star catalogue (APS_GameplayStellarView), ")
		TEXT("which cost 6-8 ms per frame and 80-100 ms hitches while accelerating at power 3. 0 keeps the base FOV."));
	// Rio 06.10 (collisions: "Codex's detailed collisions are good for walking, keep them while the ship stands; whenever it
	// moves, fly on the primitive ones so ships do not pass through each other; the motion decides, not who sits in the seat").
	// Walking aboard at speed cost 120 -> 40-60 fps: every character query and every move walked the M5's 13.5k shapes, and
	// getting up froze 300-470 ms while that body was built again in flight.
	// Rio 06.10 (audit: default 0): with aps.Ship.WalkOnShellAtSpeed 0 this is exactly the 61532ed6 collision path; the
	// unpiloted take-out and the proximity rebuild (an unmeasured 330-440 ms freeze on fleet recall) stay off until tested.
	// Rio 07.10 ("turn the fleet proxies on"; fleet take-off 49 -> 113 fps, flight beside it 103 -> 133 in fl-fly-fix-x2): on
	// by default. With aps.Ship.HullHold 1 a fleet ship's body is held in the scene, inert, instead of destroyed, so its
	// return is a teleport and a filter pass, not a build: no freeze when the fleet comes back, and a rested ship takes it
	// back at once (no invisible proxy boxes left on parked ships). 0 is exactly the 61532ed6 collision path.
	TAutoConsoleVariable<int32> CVarKeepHullOutWhileMoving(
		TEXT("aps.Ship.KeepHullOutWhileMoving"), 1,
		TEXT("Rio 07.10: default 1. A detailed hull (aps.Ship.DetailedHullShapes) moving without a pilot in the seat (a fleet ")
		TEXT("unit under orders, the autopilot, the world flow or owed travel) flies on its proxy boxes; its own body is held ")
		TEXT("in the scene, inert (aps.Ship.HullHold 1), or taken out and built again (HullHold 0). The body comes back once ")
		TEXT("the ship has rested (aps.Ship.HullRestoreRestSeconds), at once for a walker aboard or the player on foot within ")
		TEXT("its radius (aps.Ship.HullRestoreOnFootInside). 0: fleet units keep their body in flight and only the seated ")
		TEXT("pilot's flight uses the boxes, as in the 61532ed6 build."));
	// Rio 06.10 afternoon (regression: "got up at speed on M_P2_03: no gravity, I fall through the floor"): the walk shell
	// (M5HullShellCollision, ShellCol) is the OUTER hull; the decks inside are the root hull's own walk UCX. On M_P2_02 the
	// shell happens to carry the deck, on M_P2_03 it does not, so a walker left on the shell alone fell. Until the walk body
	// is split from the 13.5k-shape hull (interior UCX as its own query body), a walker gets the hull's own body back.
	TAutoConsoleVariable<int32> CVarWalkOnShellAtSpeed(
		TEXT("aps.Ship.WalkOnShellAtSpeed"), 0,
		TEXT("1 (TEST ONLY, broken): a walker aboard a moving ship is left on the walk shell while the hull's own body stays out; ")
		TEXT("the gravity probe (a Visibility sweep) finds no floor on the shell on any ship tested, so the walker turns ")
		TEXT("weightless and falls through the decks (Rio 06.10, M_P2_03; q02 M_P2_02 too). 0: a walker gets the hull's own body ")
		TEXT("back, as before 06.10; ")
		TEXT("unpiloted moving ships without walkers still fly on their proxy boxes (aps.Ship.KeepHullOutWhileMoving)."));
	TAutoConsoleVariable<float> CVarHullRestoreMaxSpeedCm(
		TEXT("aps.Ship.HullRestoreMaxSpeedCm"), 100.0f,
		TEXT("Rio 06.10 (aps.Ship.KeepHullOutWhileMoving): a ship without a pilot faster than this (cm/s) moves."));
	TAutoConsoleVariable<float> CVarHullRestoreRestSeconds(
		TEXT("aps.Ship.HullRestoreRestSeconds"), 1.0f,
		TEXT("Rio 06.10 (aps.Ship.KeepHullOutWhileMoving): seconds at rest, without a world flow or owed travel, before a ship ")
		TEXT("kept on its proxy boxes may get its hull's own body back."));
	TAutoConsoleVariable<int32> CVarHullRestoreWithWalker(
		TEXT("aps.Ship.HullRestoreWithWalker"), 0,
		TEXT("Rio 06.10 (aps.Ship.KeepHullOutWhileMoving): 0 builds a rested ship's body when the walker steps off it (the freeze ")
		TEXT("of the build moves from getting up to leaving the ship); 1 builds it after the rest with the walker still aboard."));
	TAutoConsoleVariable<float> CVarHullRestoreNearM(
		TEXT("aps.Ship.HullRestoreNearM"), 50.0f,
		TEXT("Rio 06.10 (aps.Ship.KeepHullOutWhileMoving): a rested ship with nobody aboard (a fleet unit back from an order, a ")
		TEXT("ship just left) gets its body back once the player's character on foot is within its radius + this many metres; far ")
		TEXT("away it stays on its proxy boxes (no build nobody walks on). 0: right after the rest. Rio 07.10: only for a body ")
		TEXT("that is built again (aps.Ship.HullHold 0); a held body comes back right after the rest."));
	// Rio 07.10 ("turn the fleet proxies on", without the freeze): UE 5.4 builds a body in one synchronous InitBody over all
	// its shapes (~22 us a shape: 297-675 ms per return of the M02's 13.5k in the 06.10 runs, still ~20-60 ms after Codex's
	// phase 1), it cannot be sliced or made async. A filter change that keeps the physics bit and one teleport only walk the
	// shapes (FBodyInstance::SetCollisionEnabled rebuilds only when that bit flips), so the body stays in the scene.
	TAutoConsoleVariable<int32> CVarHullHold(
		TEXT("aps.Ship.HullHold"), 1,
		TEXT("Rio 07.10 (fleet on proxies without the restore freeze): 1: a detailed hull flying on its proxy boxes with nobody ")
		TEXT("in the seat keeps its body in the physics scene, held where the ship began to move and inert (PhysicsOnly, every ")
		TEXT("channel ignored; its moves and world shifts not sent to physics, UAPSShipHullComponent); its return is one ")
		TEXT("teleport and its authored filters, no build. 2: the seated pilot's flight holds it too (no build when the pilot ")
		TEXT("gets up). 0: the body leaves the scene and is built again, as before 07.10."));
	TAutoConsoleVariable<int32> CVarHullRestoresPerFrame(
		TEXT("aps.Ship.HullRestoresPerFrame"), 1,
		TEXT("Rio 07.10 (fleet on proxies): at most this many rested ships get their hull's body back in one frame; the others ")
		TEXT("retry at their next 0.1 s check (a fleet back from one order rests together). A walker aboard never waits. 0: no limit."));
	TAutoConsoleVariable<float> CVarHullRestoreSpacingSeconds(
		TEXT("aps.Ship.HullRestoreSpacingSeconds"), 0.75f,
		TEXT("Rio 07.10 (fleet on proxies, audit): a body that has to be BUILT again (aps.Ship.HullHold 0, or a hull that cannot ")
		TEXT("be held) waits until this many real seconds have passed since the last build ended (a pilot getting up included), ")
		TEXT("so two builds never land in one hitch. A walker aboard never waits. 0: no spacing."));
	TAutoConsoleVariable<int32> CVarHullRestoreOnFootInside(
		TEXT("aps.Ship.HullRestoreOnFootInside"), 1,
		TEXT("Rio 07.10 (fleet on proxies, audit): 1 gives a ship flying on its boxes its body back whenever the player's ")
		TEXT("character on foot is within its radius, moving or not (a fleet unit arriving on top of him, a ship drifting past): ")
		TEXT("boxes wider than the hull would push him away. 0: only after the rest, as on 06.10."));
	TAutoConsoleVariable<float> CVarHullTakeOutClearM(
		TEXT("aps.Ship.HullTakeOutClearM"), 30.0f,
		TEXT("Rio 07.10 (fleet on proxies, audit; with aps.Ship.HullRestoreOnFootInside): a moving ship without a pilot leaves ")
		TEXT("its body to the boxes only once the player on foot is farther than its radius + this many metres, so walking ")
		TEXT("along the radius does not swap them back and forth."));
	// Rio 07.10 ("fix the ship stuck on the pad too"): a proxy box that STARTS inside something is only let go when the move
	// leaves along the overlap's own push-out direction (the engine's rule). For a box sunk through a thin plate (the colony
	// pad's 0.5 m deck, after the nose was pitched up on the pad) that direction points down through the plate, so every move
	// up counted as "into" it and the ship stayed pinned for good; nothing ever pushed it out.
	TAutoConsoleVariable<int32> CVarProxyUnstick(
		TEXT("aps.Ship.ProxyUnstick"), 1,
		TEXT("Rio 07.10 (ship pinned on the colony pad): 1: a ship flying on its proxy boxes that starts a move already inside ")
		TEXT("something under it (the ground, a pad; never a ceiling, a ship or a pawn) climbs out of it when the move goes 30 deg ")
		TEXT("or more above the horizon of a planet's or a moon's gravity and the overlap is deeper than ")
		TEXT("aps.Ship.ProxyUnstickDepthCm; what is ahead of that box still blocks. Any other move held by such an overlap slides ")
		TEXT("along it (never deeper). Hits ahead of the ship (not overlaps) block as before. 0: the overlap holds the ship, as before."));
	TAutoConsoleVariable<float> CVarProxyUnstickDepthCm(
		TEXT("aps.Ship.ProxyUnstickDepthCm"), 50.0f,
		TEXT("Rio 07.10 (aps.Ship.ProxyUnstick): how deep (cm, at least 1) a proxy box must already be inside something for a ")
		TEXT("move up to climb out of it; resting and grazing contacts are shallower and stay as they were. Depth and push-out ")
		TEXT("direction of a triangle-mesh body come from its first overlapped triangle (engine)."));
	TAutoConsoleVariable<int32> CVarInstancedResend(
		TEXT("aps.Ship.InstancedResend"), 1,
		TEXT("Rio 07.10 (GPUScene.cpp:367 ensure, a 0.8-1.2 s freeze in the editor, ceiling fixtures left behind the hull): UE 5.4 ")
		TEXT("re-sends an instanced mesh's instances a new primitive transform only when it changed by more than 1e-4 since ")
		TEXT("the last frame while its scene proxy takes every change. 1: a ship's own plain instanced meshes re-send all ")
		TEXT("their instances whenever the hull's render matrix changed at all, in the same end-of-frame update. 0: as before."));
	// Rio 07.10: one budget for the restores of all ships in a frame (aps.Ship.HullRestoresPerFrame).
	bool TakeHullRestoreBudget()
	{
		static uint64 Frame = 0;
		static int32 Count = 0;
		if (Frame != GFrameCounter)
		{
			Frame = GFrameCounter;
			Count = 0;
		}
		const int32 Budget = CVarHullRestoresPerFrame.GetValueOnGameThread();
		if (Budget > 0 && Count >= Budget)
		{
			return false;
		}
		++Count;
		return true;
	}
	// Rio 07.10 (aps.Ship.HullRestoreSpacingSeconds): when the last body build of this world ended, in real time (a hitch
	// lets world time jump by up to 0.4 s and looping timers catch up several calls in one frame).
	TWeakObjectPtr<const UWorld> GLastHullBuildWorld;
	double GLastHullBuildEndSeconds = -1.0e9;
	TAutoConsoleVariable<int32> CVarSweepPrecheckFirst(
		TEXT("aps.Ship.SweepPrecheckFirst"), 1,
		TEXT("Rio 06.10 (perf R2): 1 tests a swept move with the hull's bounding sphere (aps.Ship.SweepPrecheck) before the M5 ")
		TEXT("hull sweep, so a clear path moves without walking the hull's shapes; 0 runs the M5 sweep first, as before."));
	TAutoConsoleVariable<int32> CVarHudSubPixelRings(
		TEXT("aps.Ship.HudSubPixelRings"), 1,
		TEXT("Rio 06.10 (\"the planet rings jitter while the nose turns\"): 1 sizes the limb rings and the course brackets from ")
		TEXT("unrounded projections and draws the rings around the unrounded centre without pixel snapping (cards and text stay ")
		TEXT("on whole pixels); 0: whole-pixel rings as before (their radius flipped by ~1.15 px a frame during a turn)."));
	/**
	 * Rio 06.10 (offscreen q10 crash, "Assertion failed: RenderBatch.NumIndices > 0"): Slate's antialiased line builds no
	 * geometry for a zero-length or non-finite segment, and a line element alone in its render batch then reaches the
	 * renderer with no indices and asserts. A course target projected to NaN, or to coordinates a float cannot resolve,
	 * made its brackets exactly that while the autopilot set the course. Such a polyline is invisible: it is not drawn.
	 */
	bool IsDrawableHudLine(const TArray<FVector2D>& Points)
	{
		bool bHasLength = false;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			if (!FMath::IsFinite(Points[Index].X) || !FMath::IsFinite(Points[Index].Y))
			{
				return false;
			}
			bHasLength = bHasLength || (Index > 0
				&& (FVector2f(Points[Index]) - FVector2f(Points[Index - 1])).SizeSquared() > 1.0e-4f);
		}
		return bHasLength;
	}
	TAutoConsoleVariable<int32> CVarFlightPathMarker(
		TEXT("aps.Ship.FlightPathMarker"), 1,
		TEXT("Rio 06.10 (\"show where the ship really flies, whatever the camera does\"): 1 draws a small ring with a centre dot ")
		TEXT("where the ship's velocity points (the nose at a standstill) and a faint tick at the nose when they part by more ")
		TEXT("than 2 degrees; 0: no marker, as before."));
	TAutoConsoleVariable<int32> CVarCameraAlignNose(
		TEXT("aps.Ship.CameraAlignNose"), 0,
		TEXT("Rio 06.10 (\"aiming by the screen centre points 12 degrees below the nose\"): 1 keeps the chase camera where it is, ")
		TEXT("above and behind, but looks parallel to the ship's nose (the ship sits lower in the frame); 0: the accepted ")
		TEXT("framing, looking 12 degrees down at the ship."));

	enum ESection : int32 { Environment, Navigation, Move, Rotation, Stabilize, Camera, Hud, Count };
	const TCHAR* const SectionNames[Count] = {TEXT("env"), TEXT("nav"), TEXT("move"), TEXT("rot"),
		TEXT("stab"), TEXT("cam"), TEXT("hud")};

	struct FWindow
	{
		double Sum[Count] = {};
		double Max[Count] = {};
		double FrameSum = 0.0;
		double FrameMax = 0.0;
		int32 Frames = 0;
		int32 Hitches = 0;
		double Elapsed = 0.0;
		double MaxMove = 0.0;
	};
	FWindow GPiloted;

	bool Enabled() { return CVarShipPerfLog.GetValueOnGameThread() != 0; }

	struct FScope
	{
		FScope(bool bInActive, ESection InSection) : bActive(bInActive), Section(InSection),
			Start(bInActive ? FPlatformTime::Seconds() : 0.0) {}
		~FScope()
		{
			if (bActive)
			{
				const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
				GPiloted.Sum[Section] += Ms;
				GPiloted.Max[Section] = FMath::Max(GPiloted.Max[Section], Ms);
			}
		}
		bool bActive;
		ESection Section;
		double Start;
	};
}

ASpaceship::ASpaceship()
{
	// Rio 07.10 (fleet on proxies without the restore freeze): the root hull can hold its physics body (aps.Ship.HullHold).
	if constexpr (APSShipHull::bSubclass)
	{
		SpaceshipHull = CreateDefaultSubobject<UAPSShipHullComponent>(TEXT("SpaceshipHull"));
	}
	else
	{
		SpaceshipHull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipHull"));
	}
	RootComponent = SpaceshipHull;
	SpaceshipHull->SetMobility(EComponentMobility::Movable);
	SpaceshipHull->SetEnableGravity(false);
	SpaceshipHull->SetSimulatePhysics(false);

	SkeletalSpaceshipHull = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalSpaceshipHull"));
	SkeletalSpaceshipHull->SetupAttachment(SpaceshipHull);
	SkeletalSpaceshipHull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalSpaceshipHull->SetGenerateOverlapEvents(false);

	OnboardComputer = CreateDefaultSubobject<USpaceshipOnboardComputer>(TEXT("OnboardComputer"));
	
	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	SphereCollisionComponent->SetupAttachment(SpaceshipHull);
	SphereCollisionComponent->InitSphereRadius(1000.0f);
	SphereCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollisionComponent->SetGenerateOverlapEvents(true);
	SphereCollisionComponent->SetAbsolute(false, false, true);

	FlightGravityDetector = CreateDefaultSubobject<UGravityDetectorComponent>(TEXT("FlightGravityDetector"));
	FlightGravityDetector->bAutomaticDetection = false;
	FlightGravityDetector->PrimaryComponentTick.bStartWithTickEnabled = false;

	ShipNavigation = CreateDefaultSubobject<UShipNavigationComponent>(TEXT("ShipNavigation"));
	FlightModel = CreateDefaultSubobject<UAPSShipFlightModel>(TEXT("FlightModel"));

	InteractionBoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBoundsComponent->SetupAttachment(SpaceshipHull);
	InteractionBoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionBoundsComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Overlap, not block (Rio 02.10: aboard, the walking animation's foot IK traces Visibility from inside this box,
	// took its nearest face for the floor and sank the pilot's mesh through the deck). The interaction trace takes
	// overlaps (ACustomGravityCharacter::FindInteractionCandidate).
	InteractionBoundsComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	InteractionBoundsComponent->SetGenerateOverlapEvents(false);
	InteractionBoundsComponent->SetCanEverAffectNavigation(false);

	PilotChair = CreateDefaultSubobject<USceneComponent>(TEXT("PilotChair"));
	PilotChair->SetupAttachment(SpaceshipHull);
	PilotChair->SetAbsolute(false, false, true);

	PilotExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PilotExitPoint"));
	PilotExitPoint->SetupAttachment(SpaceshipHull);
	PilotExitPoint->SetRelativeLocation(APSAutomaticShipInteraction::NativeExitLocation);
	PilotExitPoint->SetAbsolute(false, false, true);

	ForwardVector = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardVector"));
	ForwardVector->SetupAttachment(SpaceshipHull);
	ForwardVector->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForwardVector->SetGenerateOverlapEvents(false);
	ForwardVector->SetVisibility(false, true);
	ForwardVector->SetHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	NoseArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("NoseArrow"));
	if (NoseArrow)
	{
		NoseArrow->SetupAttachment(ForwardVector);
		NoseArrow->ArrowColor = FColor(242, 181, 29);
		NoseArrow->ArrowSize = 2.0f;
		NoseArrow->bIsScreenSizeScaled = true;
		// Arrow bounds scale with the hull and would inflate the ship's actor bounds.
		NoseArrow->bUseAttachParentBound = true;
		NoseArrow->SetHiddenInGame(true);
	}
#endif

	OnInterstellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterstellar);
	OnStellarMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForStellar);
	OnInterplanetaryMode.AddDynamic(this, &ASpaceship::UpdateNavigatableActorsForInterplanetary);
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->SetAbsolute(false, false, true);
	SpringArmComponent->TargetArmLength = 1000.0;
	SpringArmComponent->SetRelativeLocation(FVector::ZeroVector);
	SpringArmComponent->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
	SpringArmComponent->bDoCollisionTest = false;
	// Translation lag becomes numerically unstable at full-scale travel speeds:
	// the arm falls kilometres behind the root and periodically catches up. Arm
	// length and FOV still communicate speed without allowing camera/ship separation.
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->CameraLagSpeed = 12.0f;
	SpringArmComponent->bEnableCameraRotationLag = true;
	SpringArmComponent->CameraRotationLagSpeed = 9.0f;
	SpringArmComponent->bUseCameraLagSubstepping = false;
	SpringArmComponent->CameraLagMaxTimeStep = 1.0f / 120.0f;
	SpringArmComponent->bClampToMaxPhysicsDeltaTime = false;
	SpringArmComponent->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->SetAbsolute(false, false, true);
	CameraComponent->bUsePawnControlRotation = false;
#if WITH_EDITORONLY_DATA
	CameraComponent->bDrawFrustumAllowed = false;
	CameraComponent->bCameraMeshHiddenInGame = true;
#endif
	#if WITH_EDITOR
	CameraComponent->SetCameraMesh(nullptr);
	#endif

	PilotFillLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("PilotFillLight"));
	PilotFillLight->SetupAttachment(CameraComponent);
	PilotFillLight->SetRelativeLocation(FVector::ZeroVector);
	PilotFillLight->SetRelativeRotation(FRotator::ZeroRotator);
	PilotFillLight->SetMobility(EComponentMobility::Movable);
	PilotFillLight->SetIntensity(3200.0f);
	PilotFillLight->SetLightColor(FLinearColor(0.72f, 0.82f, 1.0f));
	PilotFillLight->SetInnerConeAngle(38.0f);
	PilotFillLight->SetOuterConeAngle(72.0f);
	PilotFillLight->SetCastShadows(false);
	PilotFillLight->SetAffectTranslucentLighting(false);
	PilotFillLight->SetVisibility(false, true);

	PilotFillPointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PilotFillPointLight"));
	PilotFillPointLight->SetupAttachment(CameraComponent);
	PilotFillPointLight->SetRelativeLocation(FVector::ZeroVector);
	PilotFillPointLight->SetMobility(EComponentMobility::Movable);
	PilotFillPointLight->SetUseInverseSquaredFalloff(false);
	PilotFillPointLight->SetLightFalloffExponent(2.0f);
	PilotFillPointLight->SetIntensity(18.0f);
	PilotFillPointLight->SetLightColor(FLinearColor(0.72f, 0.82f, 1.0f));
	PilotFillPointLight->SetInverseExposureBlend(1.0f);
	PilotFillPointLight->SetCastShadows(false);
	PilotFillPointLight->SetAffectTranslucentLighting(false);
	// Channel 2 is reserved for the controlled ship's readability fill. Keeping
	// channel 0 disabled prevents the camera light from bleaching stations,
	// planets and characters near the ship.
	PilotFillPointLight->SetLightingChannels(false, false, true);
	PilotFillPointLight->SetVisibility(false, true);
}

void ASpaceship::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshInteractionGeometry();
}

void ASpaceship::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFromHull();
	InitializeFlightPostProcess();
	RefreshInteractionGeometry();
	if (IsGroundVehicle())
	{
		ConfigureGroundVehicleExit();
	}
	UpdateFlightEnvironment(0.0f, true);

	GeneratedWorld = Cast<AAstroGenerator>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AAstroGenerator::StaticClass()));
	GeneratedStarCluster = Cast<AStarCluster>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AStarCluster::StaticClass()));

	if (!OffsetSystem)
	{
		OffsetSystem = Cast<AStarSystem>(UGameplayStatics::GetActorOfClass(GetWorld(), AStarSystem::StaticClass()));
		if (OffsetSystem)
		{
			UE_LOG(LogTemp, Log, TEXT("OffsetSystem successfully obtained."));
			//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("OffsetSystem successfully obtained!"));
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("No OffsetSystem is present in this level."));
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to obtain OffsetSystem."));
		}
	}

	if (OnboardComputer)
	{
		OnboardComputer->SpaceshipHull = SpaceshipHull;
		OnboardComputer->OffsetSystem = OffsetSystem;
		SelectedEngineMode = EEngineMode::Impulse;
		OnboardComputer->FlightSystem.CurrentFlightMode = ResolveLegacyFlightModeForDriveMode(SelectedDriveMode);
		OnboardComputer->ComputeFlightParams();
		RequestEngineModeForFlightMode(true);
	}
	ApplyEngineState();

	// Parked fleet actors do not scan the universe. The possessed ship's navigation component owns that work.
	SetActorTickEnabled(IsValid(Pilot) || bEngineRunning);
	UpdateCameraArmTicking();
	// Rio 06.10 (collision by motion): a ship with a detailed hull watches its own motion, also parked and without a tick
	// (a fleet unit is moved by the fleet's orders, not by its own tick).
	bHullGoingAway = false;
	StartHullMotionWatch();
	// Rio 07.10 (aps.Ship.InstancedResend): the ship's own plain instanced meshes (the ceiling fixtures of M_P2_02/03/04)
	// get their instances re-sent whenever the hull moved at all (see ResendInstancedRiders).
	TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(this);
	for (UInstancedStaticMeshComponent* Mesh : Instanced)
	{
		if (Mesh && !Mesh->IsA<UHierarchicalInstancedStaticMeshComponent>())
		{
			InstancedRiders.Add({Mesh, FMatrix::Identity, false});
		}
	}
	if (!InstancedRiders.IsEmpty() && !InstancedResendHandle.IsValid())
	{
		InstancedResendHandle = FWorldDelegates::OnWorldPreSendAllEndOfFrameUpdates.AddUObject(this, &ASpaceship::ResendInstancedRiders);
	}

	//ComputeProximity();
}

void ASpaceship::ResendInstancedRiders(UWorld* World)
{
	// Rio 07.10 ("ships freeze": GPUScene.cpp:367 ensure, 0.8-1.2 s the first time in the editor; fixtures left behind the
	// hull): UE 5.4 sends an ISM's instances a new primitive transform only when it changed by more than 1e-4 since the
	// last frame (ISMInstanceDataManager.cpp:651) while the scene proxy takes every change, so a hull that turns or creeps
	// by tiny steps leaves its instances behind until the gap trips the ensure. Each change of the render matrix now marks
	// all instances changed, so the end-of-frame flush sends them against this frame's transform (no render-state rebuild).
	if (World != GetWorld() || APSShipPerf::CVarInstancedResend.GetValueOnGameThread() == 0)
	{
		return;
	}
	for (FAPSInstancedRider& Rider : InstancedRiders)
	{
		UInstancedStaticMeshComponent* Mesh = Rider.Mesh.Get();
		if (!IsValid(Mesh) || !Mesh->SceneProxy || Mesh->PerInstanceSMData.IsEmpty() || Mesh->IsCollisionEnabled()
			|| Mesh->IsNavigationRelevant())
		{
			Rider.bHasLast = false;
			continue;
		}
		const FMatrix Now = Mesh->GetRenderMatrix();
		if (Rider.bHasLast && !Now.Equals(Rider.Last, 0.0))
		{
			TArray<FMatrix, TInlineAllocator<64>> Exact;
			TArray<FTransform, TInlineAllocator<64>> Local;
			for (const FInstancedStaticMeshInstanceData& Instance : Mesh->PerInstanceSMData)
			{
				Exact.Add(Instance.Transform);
				Local.Add(FTransform(Instance.Transform));
			}
			Mesh->BatchUpdateInstancesTransforms(0, TArrayView<const FTransform>(Local), false, false, false);
			// The matrices stay bit-exact (the transform round trip could move the last bits); the flush reads them.
			for (int32 Index = 0; Index < Exact.Num() && Index < Mesh->PerInstanceSMData.Num(); ++Index)
			{
				Mesh->PerInstanceSMData[Index].Transform = Exact[Index];
			}
		}
		Rider.Last = Now;
		Rider.bHasLast = true;
	}
}

UPrimitiveComponent* ASpaceship::GetPrimaryHullComponent() const
{
	if (SpaceshipHull && SpaceshipHull->GetStaticMesh())
	{
		return SpaceshipHull;
	}
	if (SkeletalSpaceshipHull && SkeletalSpaceshipHull->GetSkeletalMeshAsset())
	{
		return SkeletalSpaceshipHull;
	}
	return nullptr;
}

bool ASpaceship::GetPrimaryHullLocalBounds(UPrimitiveComponent* Hull, FVector& OutMin, FVector& OutMax) const
{
	if (!Hull)
	{
		return false;
	}
	const FBoxSphereBounds LocalBounds = Hull->CalcBounds(FTransform::Identity);
	if (LocalBounds.BoxExtent.IsNearlyZero())
	{
		return false;
	}
	OutMin = LocalBounds.Origin - LocalBounds.BoxExtent;
	OutMax = LocalBounds.Origin + LocalBounds.BoxExtent;
	return true;
}

void ASpaceship::RefreshInteractionGeometry()
{
	if (!bAutoConfigureInteractionGeometry || !PilotChair || !PilotExitPoint || !SphereCollisionComponent
		|| !InteractionBoundsComponent)
	{
		return;
	}
	SphereCollisionComponent->SetCollisionEnabled(
		ProvidesShipGravity() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	UPrimitiveComponent* InteractionMesh = GetPrimaryHullComponent();
	if (!InteractionMesh)
	{
		return;
	}

	FVector LocalBoundsMin;
	FVector LocalBoundsMax;
	if (!GetPrimaryHullLocalBounds(InteractionMesh, LocalBoundsMin, LocalBoundsMax))
	{
		return;
	}
	const FVector LocalCenter = (LocalBoundsMin + LocalBoundsMax) * 0.5;
	const FVector LocalExtent = (LocalBoundsMax - LocalBoundsMin) * 0.5;
	const bool bForceGeneratedConfiguration = bGenerateSimpleHullCollision;

	static const TArray<FName> SeatSocketNames{
		TEXT("PilotSeat"), TEXT("PilotChair"), TEXT("CockpitSeat"), TEXT("DriverSeat"), TEXT("Seat")};
	static const TArray<FName> ExitSocketNames{
		TEXT("PilotExit"), TEXT("ShipExit"), TEXT("RampExit"), TEXT("Entry"), TEXT("Door"), TEXT("Exit")};
	// Rio 09.10 (aps.Ship.CabinSeatFromSocket): a modelled cabin (the sockets HasWalkableInterior counts) owns its seat, exit
	// point and sphere: what a level instance saved for those native components is replaced.
	static const TArray<FName> CabinSeatSocketNames{TEXT("PilotSeat"), TEXT("PilotChair"), TEXT("CockpitSeat")};
	FTransform CabinSeatTransform;
	const bool bCabinOwnsGeometry = !IsGroundVehicle()
		&& APSAutomaticShipInteraction::CVarCabinSeatFromSocket.GetValueOnGameThread() != 0
		&& APSAutomaticShipInteraction::FindSocketTransform(InteractionMesh, CabinSeatSocketNames, CabinSeatTransform);

	const bool bSeatStillUsesLastAutoTransform = bSeatWasAutoConfigured
		&& PilotChair->GetRelativeTransform().Equals(LastAutoSeatRelativeTransform, 0.1);
	const bool bCanConfigureSeat = bForceGeneratedConfiguration || bSeatStillUsesLastAutoTransform
		|| APSAutomaticShipInteraction::IsNativeSeatTransform(PilotChair) || bCabinOwnsGeometry;
	if (bCanConfigureSeat)
	{
		FTransform SeatTransform;
		if (!APSAutomaticShipInteraction::FindSocketTransform(InteractionMesh, SeatSocketNames, SeatTransform))
		{
			const FVector FallbackSeatLocation = LocalCenter
				+ FVector(LocalExtent.X * 0.2, 0.0, LocalExtent.Z * 0.15);
			SeatTransform = FTransform(
				InteractionMesh->GetComponentQuat(),
				InteractionMesh->GetComponentTransform().TransformPosition(FallbackSeatLocation),
				FVector::OneVector);
		}
		if (bCabinOwnsGeometry && !bSeatWasAutoConfigured && !APSAutomaticShipInteraction::IsNativeSeatTransform(PilotChair)
			&& !PilotChair->GetComponentLocation().Equals(SeatTransform.GetLocation(), 1.0))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[APS.Seat] %s: seat taken from the hull's cabin socket; the instance's saved seat (relative %s) is ignored"),
				*GetName(), *PilotChair->GetRelativeLocation().ToCompactString());
		}
		PilotChair->SetWorldTransform(SeatTransform);
		bSeatWasAutoConfigured = true;
		LastAutoSeatRelativeTransform = PilotChair->GetRelativeTransform();
	}

	const bool bExitStillUsesLastAutoTransform = bExitWasAutoConfigured
		&& PilotExitPoint->GetRelativeTransform().Equals(LastAutoExitRelativeTransform, 0.1);
	const bool bCanConfigureExit = bForceGeneratedConfiguration || bExitStillUsesLastAutoTransform
		|| APSAutomaticShipInteraction::IsNativeExitTransform(PilotExitPoint) || bCabinOwnsGeometry;
	if (bCanConfigureExit)
	{
		FTransform ExitTransform;
		if (!APSAutomaticShipInteraction::FindSocketTransform(InteractionMesh, ExitSocketNames, ExitTransform))
		{
			const double MeshYScale = FMath::Max(FMath::Abs(InteractionMesh->GetComponentScale().Y), 0.01);
			const FVector FallbackExitLocation = LocalCenter
				+ FVector(0.0, -(LocalExtent.Y + AutoExitClearance / MeshYScale), 0.0);
			ExitTransform = FTransform(
				InteractionMesh->GetComponentQuat(),
				InteractionMesh->GetComponentTransform().TransformPosition(FallbackExitLocation),
				FVector::OneVector);
		}
		PilotExitPoint->SetWorldTransform(ExitTransform);
		bExitWasAutoConfigured = true;
		LastAutoExitRelativeTransform = PilotExitPoint->GetRelativeTransform();
	}

	const bool bZoneUsesNativeDefaults = SphereCollisionComponent->GetRelativeLocation().IsNearlyZero(0.1)
		&& FMath::IsNearlyEqual(SphereCollisionComponent->GetUnscaledSphereRadius(), 1000.0f, 0.1f);
	const bool bZoneStillUsesLastAutoValues = bInteractionZoneWasAutoConfigured
		&& SphereCollisionComponent->GetRelativeLocation().Equals(LastAutoInteractionZoneRelativeLocation, 0.1)
		&& FMath::IsNearlyEqual(
			SphereCollisionComponent->GetUnscaledSphereRadius(), LastAutoInteractionRadius, 0.1f);
	if (bForceGeneratedConfiguration || bZoneStillUsesLastAutoValues || bZoneUsesNativeDefaults || bCabinOwnsGeometry)
	{
		// The root is the visual hull and imported ships are commonly actor-scaled.
		// Keep the interaction sphere in world units instead of inheriting that scale.
		SphereCollisionComponent->SetAbsolute(false, false, true);
		// Component bounds can still describe the previous mesh for one frame when a
		// runtime fleet ship has just received its static mesh.  Local mesh bounds are
		// available immediately, so derive the world-space interaction sphere from them.
		SphereCollisionComponent->SetWorldLocation(
			InteractionMesh->GetComponentTransform().TransformPosition(LocalCenter));
		const FVector ScaledMeshExtent = LocalExtent * InteractionMesh->GetComponentScale().GetAbs();
		const double MeshSphereRadius = ScaledMeshExtent.Size();
		const double SphereScale = FMath::Max(SphereCollisionComponent->GetComponentScale().GetAbsMax(), 0.01);
		const double InteractionRadius = (MeshSphereRadius + AutoInteractionPadding) / SphereScale;
		SphereCollisionComponent->SetSphereRadius(FMath::Max(1000.0, InteractionRadius), true);
		bInteractionZoneWasAutoConfigured = true;
		LastAutoInteractionZoneRelativeLocation = SphereCollisionComponent->GetRelativeLocation();
		LastAutoInteractionRadius = SphereCollisionComponent->GetUnscaledSphereRadius();
	}

	InteractionBoundsComponent->AttachToComponent(
		InteractionMesh, FAttachmentTransformRules::KeepRelativeTransform);
	InteractionBoundsComponent->SetRelativeLocation(LocalCenter);
	InteractionBoundsComponent->SetRelativeRotation(FRotator::ZeroRotator);
	const FVector MeshScale = InteractionMesh->GetComponentScale().GetAbs().ComponentMax(FVector(0.01));
	const FVector LocalPadding(
		AutoInteractionPadding / MeshScale.X,
		AutoInteractionPadding / MeshScale.Y,
		AutoInteractionPadding / MeshScale.Z);
	InteractionBoundsComponent->SetBoxExtent(LocalExtent + LocalPadding, true);
	InteractionBoundsComponent->SetCollisionEnabled(
		bAllowExteriorInteraction ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	InteractionBoundsComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBoundsComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
}

void GetAttachedActorsRecursively(AActor* ParentActor, TArray<AActor*>& OutActors)
{
	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);
	for (AActor* ChildActor : AttachedActors)
	{
		OutActors.Add(ChildActor);
		GetAttachedActorsRecursively(ChildActor, OutActors);
	}
}

void ASpaceship::CalculateDistanceAndAddToZones(AWorldActor* WorldActor)
{
	if (WorldActor)
	{
		double DistanceSquared = 0;
		FVector ShipLocation = GetActorLocation();

		double InfluenceRadius = WorldActor->AffectionRadiusKM * 100000;
		// �������������� ��������� � ����� Unreal (1 unit = 1 cm)
		DistanceSquared = FVector::DistSquared(WorldActor->GetActorLocation(), ShipLocation) - FMath::Square(
			InfluenceRadius);

		// �������� ������������� ������ � ������� ��� �� �����
		AActor* ParentActor = WorldActor->GetRootComponent()->GetAttachParent()->GetOwner();
		FString ParentName = ParentActor ? *ParentActor->GetName() : FString("No Parent");

		// ������� ��������� �� �����
		double DistanceInKm = FMath::Sqrt(DistanceSquared) / 100000;
		FString Message = FString::Printf(
			TEXT("Distance to %s (orbiting %s): %f kilometers."), *WorldActor->GetName(), *ParentName, DistanceInKm);

		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, Message);
	}
}


TSharedPtr<FStarModel> FindNearestStar(TMap<FVector, TSharedPtr<FStarModel>>& Stars, const FVector& ReferencePoint)
{
	TSharedPtr<FStarModel> NearestStar = nullptr;
	float NearestDistanceSquared = FLT_MAX;

	for (const auto& KeyValuePair : Stars)
	{
		float TotalDistSquared = FVector::DistSquared(ReferencePoint, KeyValuePair.Key);
		float RadiusSquared = FMath::Square(KeyValuePair.Value->Radius);
		float DistanceSquared = TotalDistSquared > RadiusSquared ? TotalDistSquared - RadiusSquared : 0.0f;

		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestStar = KeyValuePair.Value;
		}
	}

	return NearestStar;
}

void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const bool bMeasure = IsValid(Pilot) && APSShipPerf::Enabled();
	const FVector LocationBeforeMove = GetActorLocation();
	if (IsValid(Pilot) && APSShipPerf::CVarHullSceneLightingInFlight.GetValueOnGameThread() != AppliedHullSceneLightingMode)
	{
		// A console A/B of the hull lighting takes effect in flight, without leaving the seat.
		SetHullSceneLightingExcluded(false);
		SetHullSceneLightingExcluded(true);
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Environment);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Environment);
		UpdateFlightEnvironment(DeltaTime);
	}
	if (ShipNavigation && IsValid(Pilot) && (bNavigationMarkersVisible || bNavigationPanelVisible))
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Navigation);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Navigation);
		ShipNavigation->RefreshContacts(GetActorLocation());
	}

	const double TargetBoost = bEngineRunning && bIsAccelerating ? ActiveClassPreset.MaximumBoost : 1.0;
	const double BoostResponse = bIsAccelerating
		? ActiveClassPreset.BoostGrowthPerSecond
		: BoostRecoverySpeed;
	CurrentBoostMultiplier = FMath::FInterpConstantTo(
		CurrentBoostMultiplier, TargetBoost, DeltaTime, BoostResponse);

	AdvanceEngineModeTransition(DeltaTime);
	// Rio 09.10 (aps.Ship.HullBodySyncOnce, default 0): this tick's step and turn reach the hull's own body once.
	APSShipHull::FScopedBodySync HullBodySync(*this);
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Move);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Move);
		if (!FAPSShipFlightBenchmark::TickShip(*this, DeltaTime) && !ApplyCustomFlightTranslation(DeltaTime))
		{
			ApplyFlightInput(DeltaTime);
		}
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Rotation);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Rotation);
		ApplyRotationInput(DeltaTime);
		HullBodySync.End();
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Stabilize);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Stabilize);
		StabilizeFullScaleVisualVelocity();
	}
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_Camera);
		APSShipPerf::FScope Scope(bMeasure, APSShipPerf::Camera);
		UpdateAdaptiveFlightCamera(DeltaTime);
	}
	if (bMeasure)
	{
		APSShipPerf::FWindow& Window = APSShipPerf::GPiloted;
		const double FrameMs = DeltaTime * 1000.0;
		Window.FrameSum += FrameMs;
		Window.FrameMax = FMath::Max(Window.FrameMax, FrameMs);
		Window.Hitches += FrameMs > 33.4 ? 1 : 0;
		Window.MaxMove = FMath::Max(Window.MaxMove, FVector::Distance(LocationBeforeMove, GetActorLocation()));
		++Window.Frames;
		Window.Elapsed += DeltaTime;
		if (Window.Elapsed >= 2.0 && Window.Frames > 0)
		{
			FString Sections;
			for (int32 Index = 0; Index < APSShipPerf::Count; ++Index)
			{
				Sections += FString::Printf(TEXT(" %s %.2f/%.2f"), APSShipPerf::SectionNames[Index],
					Window.Sum[Index] / Window.Frames, Window.Max[Index]);
			}
			UE_LOG(LogTemp, Log,
				TEXT("[APS.ShipPerf] ship=%s speed=%.1f m/s env=%s engine=%s drive=%s frame %.2f/%.2f ms hitches=%d maxMovePerFrame=%.0f m | avg/max ms:%s"),
				*GetName(), GetShipSpeedMetersPerSecond(), *GetFlightEnvironmentName(), *GetEngineModeName(),
				*GetDriveModeName(), Window.FrameSum / Window.Frames, Window.FrameMax, Window.Hitches,
				Window.MaxMove / 100.0, *Sections);
			Window = APSShipPerf::FWindow();
		}
	}
	/*uint64 StartCycles = FPlatformTime::Cycles();

	if (!bEngineRunning)
	{
		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Station || OnboardComputer->FlightSystem.
			CurrentFlightMode == EFlightMode::Planetary)
		{
			const float GravityImpulseStrength = -100.0f;
			FVector DownwardImpulse = GetActorUpVector() * GravityImpulseStrength;
			SpaceshipHull->AddImpulse(DownwardImpulse, NAME_None, true);
		}
	}
	else
	{
		if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
		{
			double EngineThrustForce = OnboardComputer->GetEngineThrustForce();
			const FVector Direction = ForwardVector->GetForwardVector();
			OffsetSystem->AddActorLocalOffset(-Direction * EngineThrustForce); /// CRASHED PIE!
		}

		PrintOnboardComputerBasicIformation();

		if (SpaceshipHull->IsSimulatingPhysics())
		{
			FVector OppositeTorque = -SpaceshipHull->GetPhysicsAngularVelocityInRadians() * 0.5;
			SpaceshipHull->AddTorqueInRadians(OppositeTorque, NAME_None, true);
		}
		if (bIsAccelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Accelerating")));
			OnboardComputer->AccelerateBoost(DeltaTime);
		}
		if (bIsDecelerating)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Decelerating")));
			OnboardComputer->DecelerateBoost(DeltaTime);
		}
		if (ClosestActor != nullptr)
		{
			FString ClosestActorMessage = FString::Printf(TEXT("Closest actor is %s"), *ClosestActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestActorMessage);
		}
		if (AffectedActor != nullptr)
		{
			FString ClosestAffectedActorMessage = FString::Printf(
				TEXT("Affection zone is %s"), *AffectedActor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, ClosestAffectedActorMessage);
		}

		ComputeProximity();

		// ���� �� � ������ Interstellar, ��������� ��������� ������
		if (OnboardComputer->FlightSystem.CurrentFlightMode == EFlightMode::Interstellar)
		{
			//ComputeClosestStar(); // �������� OffsetSystem

			if (!OffsetSystem) return;

			double Distance = FVector::Dist(this->GetActorLocation(), OffsetSystem->GetActorLocation());
			double AffectionRadiusUnits = OffsetSystem->AffectionRadiusKM * 100000 / 1000000000.0; /// CRASH PIE!!!
			// ��������� �� � ����� � ��������� ���������������
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange,
			                                 FString::Printf(
				                                 TEXT("Distance: %f, Affection Radius: %f"), Distance,
				                                 AffectionRadiusUnits));

			// ���� �� � �������� ���� �������, ������������� �� Stellar
			if (Distance < AffectionRadiusUnits && LastFlightMode != EFlightMode::Stellar)
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Stellar;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);

				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, TEXT("AffectionRadiusUnits!!!"));
			}
		}
		else
		{
			if (!AffectedActor) //return;
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interstellar;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::FTL;
				OnboardComputer->SwitchEngineMode(EEngineMode::Offset);
			}
			else if (AffectedActor->IsA(APlanet::StaticClass()))
			{
				APlanet* Planet = Cast<APlanet>(AffectedActor);



				// �������� ������� ������� � �������.
				FVector ShipPosition = GetActorLocation();
				FVector PlanetPosition = Planet->GetActorLocation();
				// ��������� ���������� ����� �������� � ��������.
				double DistanceToPlanet = FVector::Dist(ShipPosition, PlanetPosition) - Planet->GetRadius();
				DistanceToPlanet /= 100000.0;
				// ����� ������ ������ �� ������ ���������� �� �������.
				if (DistanceToPlanet <= Planet->RadiusKM + Planet->AtmosphereHeight)
				{
					// ���� ������� ������ ��������� �������.
					if (DistanceToPlanet - Planet->RadiusKM <= 10.0)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Surface Flight!"));

						// ���� ������� ����� 10 �� � ����������� �������.
						OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Surface;
						//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
						OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
					}
					else
					{
						// ���� ������� ������ ���������, �� ������ 10 �� �� �����������.
						GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Atmospheric Flight!"));

						OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Atmospheric;
						//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
						//OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
					}
					GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
					                                 FString::Printf(
						                                 TEXT("Distance to Planet Surface: %f"),
						                                 DistanceToPlanet - Planet->RadiusKM));
				}
				else if (DistanceToPlanet <= Planet->RadiusKM + Planet->OrbitHeight)
				{
					// ���� ������� ������ ������ �������, �� �� ��������� ���������.
					OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Orbital;
					//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
					//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
					//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, FString::Printf(TEXT("Orbit Height: %f"), DistanceToPlanet - Planet->RadiusKM));
				}
				else
				{
					OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Planetary;
					//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
					//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
				}
			}
			else if (AffectedActor->IsA(AMoon::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Orbital;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);//InitiateOffsetMode();
			}
			else if (AffectedActor->IsA(AStar::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interplanetary;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);//InitiateOffsetMode();
			}
			// Techical objects: space stations, satellites, etc.
			else if (AffectedActor->IsA(ATechActor::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Station;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ArtificialGravity;
				//OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
			}
			// Star clusters: galaxies, nebulae, etc.
			else if (AffectedActor->IsA(AStarCluster::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Interstellar;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::FTL;
				//OnboardComputer->SwitchEngineMode(EEngineMode::Offset);

				OffsetGalaxy = Cast<AAstroActor>(AffectedActor);
			}
			// Between star system and star cluster
			else if (AffectedActor->IsA(AStarSystem::StaticClass()))
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Stellar;
				//OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::LightSpeed;
				//OnboardComputer->SwitchEngineMode(EEngineMode::SpaceWrap);
				//CalculateProximity = false;
				OffsetGalaxy = Cast<AAstroActor>(AffectedActor);
			}
			// All others
			else
			{
				OnboardComputer->FlightSystem.CurrentFlightMode = EFlightMode::Basic;
				OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ZeroG;
				OnboardComputer->SwitchEngineMode(EEngineMode::Impulse);
			}
		}

		if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
		{
			if (LastFlightMode == EFlightMode::Interplanetary && OnboardComputer->FlightSystem.CurrentFlightMode ==
				EFlightMode::Planetary)
			{
				// WorldScape lifetime is owned by APSPlanetEnvironmentStreamingSubsystem.
				// The legacy InitWSC path spawned another root for the planet and every
				// moon whenever flight mode changed, leaving duplicate runtime actors.
				AffectedPlanet = Cast<APlanet>(AffectedActor);
			}
			else if (LastFlightMode == EFlightMode::Planetary && OnboardComputer->FlightSystem.CurrentFlightMode ==
				EFlightMode::Interplanetary)
			{
				AffectedPlanet = nullptr;
			}


			switch (OnboardComputer->FlightSystem.CurrentFlightMode)
			{
			case EFlightMode::Interstellar:
				OnInterstellarMode.Broadcast();
				break;

			case EFlightMode::Stellar:
				OnStellarMode.Broadcast();
				break;

			case EFlightMode::Interplanetary:
				OnInterplanetaryMode.Broadcast();
				break;
			default:
				break;
			}

			UpdateNavigatableActors();
			LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
		}

		//OnboardComputer->ComputeFlightParams();
		// Switch Engine mode
	}
	if (Pilot)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
		                                 FString::Printf(TEXT("Pilot Actor Name: %s"), *Pilot->GetName()));
	}
	uint64 EndCycles = FPlatformTime::Cycles();
	double ElapsedTime = FPlatformTime::ToSeconds(EndCycles - StartCycles);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
	                                 FString::Printf(TEXT("Time elapsed: %f seconds"), ElapsedTime));*/
}

FSpaceshipClassPreset ASpaceship::GetPresetForSizeClass(ESpaceshipSizeClass InSizeClass)
{
	FSpaceshipClassPreset Preset;
	switch (InSizeClass)
	{
	case ESpaceshipSizeClass::XXS:
		Preset.ImpulseAcceleration = 1800.0;
		Preset.MaxImpulseSpeed = 60000.0;
		Preset.RotationSpeed = 105.0;
		Preset.AngularAcceleration = 240.0;
		Preset.LinearDamping = 0.08;
		Preset.AngularDamping = 3.5;
		Preset.MaximumBoost = 5.0;
		Preset.BoostGrowthPerSecond = 1.35;
		Preset.bSupportsSpaceWrap = false;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = false;
		Preset.MaximumFlightMode = EFlightMode::Planetary;
		break;
	case ESpaceshipSizeClass::XS:
		Preset.ImpulseAcceleration = 1500.0;
		Preset.MaxImpulseSpeed = 85000.0;
		Preset.RotationSpeed = 88.0;
		Preset.AngularAcceleration = 180.0;
		Preset.LinearDamping = 0.1;
		Preset.AngularDamping = 3.2;
		Preset.MaximumBoost = 5.0;
		Preset.BoostGrowthPerSecond = 1.2;
		Preset.bSupportsSpaceWrap = false;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = false;
		Preset.MaximumFlightMode = EFlightMode::Planetary;
		break;
	case ESpaceshipSizeClass::S:
		Preset.ImpulseAcceleration = 1200.0;
		Preset.MaxImpulseSpeed = 120000.0;
		Preset.RotationSpeed = 70.0;
		Preset.AngularAcceleration = 125.0;
		Preset.LinearDamping = 0.13;
		Preset.AngularDamping = 2.9;
		Preset.MaximumBoost = 4.5;
		Preset.BoostGrowthPerSecond = 1.05;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = false;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Stellar;
		break;
	case ESpaceshipSizeClass::M:
		Preset.ImpulseAcceleration = 900.0;
		Preset.MaxImpulseSpeed = 160000.0;
		Preset.RotationSpeed = 55.0;
		Preset.AngularAcceleration = 75.0;
		Preset.LinearDamping = 0.18;
		Preset.AngularDamping = 2.5;
		Preset.MaximumBoost = 4.0;
		Preset.BoostGrowthPerSecond = 0.9;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = true;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Interstellar;
		break;
	case ESpaceshipSizeClass::L:
		Preset.ImpulseAcceleration = 650.0;
		Preset.MaxImpulseSpeed = 220000.0;
		Preset.RotationSpeed = 36.0;
		Preset.AngularAcceleration = 42.0;
		Preset.LinearDamping = 0.22;
		Preset.AngularDamping = 2.2;
		Preset.MaximumBoost = 3.5;
		Preset.BoostGrowthPerSecond = 0.72;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::XL:
		Preset.ImpulseAcceleration = 450.0;
		Preset.MaxImpulseSpeed = 300000.0;
		Preset.RotationSpeed = 25.0;
		Preset.AngularAcceleration = 25.0;
		Preset.LinearDamping = 0.27;
		Preset.AngularDamping = 2.0;
		Preset.MaximumBoost = 3.2;
		Preset.BoostGrowthPerSecond = 0.58;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::XXL:
		Preset.ImpulseAcceleration = 300.0;
		Preset.MaxImpulseSpeed = 450000.0;
		Preset.RotationSpeed = 16.0;
		Preset.AngularAcceleration = 12.0;
		Preset.LinearDamping = 0.32;
		Preset.AngularDamping = 1.8;
		Preset.MaximumBoost = 2.8;
		Preset.BoostGrowthPerSecond = 0.45;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	case ESpaceshipSizeClass::Titan:
		Preset.ImpulseAcceleration = 180.0;
		Preset.MaxImpulseSpeed = 600000.0;
		Preset.RotationSpeed = 9.0;
		Preset.AngularAcceleration = 6.0;
		Preset.LinearDamping = 0.38;
		Preset.AngularDamping = 1.6;
		Preset.MaximumBoost = 2.4;
		Preset.BoostGrowthPerSecond = 0.32;
		Preset.bSupportsSpaceWrap = true;
		Preset.bSupportsOffset = true;
		Preset.bUsesPhysicalImpulse = false;
		Preset.bHasInteriorByDefault = true;
		Preset.MaximumFlightMode = EFlightMode::Intergalaxy;
		break;
	}
	return Preset;
}

ESpaceshipSizeClass ASpaceship::InferSizeClassFromLength(double LengthCentimeters)
{
	if (LengthCentimeters <= 2000.0) return ESpaceshipSizeClass::XXS;
	if (LengthCentimeters <= 5000.0) return ESpaceshipSizeClass::XS;
	if (LengthCentimeters <= 15000.0) return ESpaceshipSizeClass::S;
	if (LengthCentimeters <= 50000.0) return ESpaceshipSizeClass::M;
	if (LengthCentimeters <= 200000.0) return ESpaceshipSizeClass::L;
	if (LengthCentimeters <= 1000000.0) return ESpaceshipSizeClass::XL;
	if (LengthCentimeters <= 10000000.0) return ESpaceshipSizeClass::XXL;
	return ESpaceshipSizeClass::Titan;
}

void ASpaceship::ConfigureFlightReferenceFromHull(UPrimitiveComponent* Hull, const FVector& LocalExtent)
{
	// Legacy interior ships already author their actual nose direction with this hidden arrow.
	// Prefer it when authored; generated hulls keep the bounds-axis fallback below.
	const bool bHasAuthoredForward = ForwardVector
		&& (bUseAuthoredNoseDirection
			|| ForwardVector->GetStaticMesh() != nullptr
			|| !ForwardVector->GetRelativeRotation().IsNearlyZero(0.1));
	if (bHasAuthoredForward && SpaceshipHull)
	{
		const FTransform HullTransform = SpaceshipHull->GetComponentTransform();
		FlightForwardLocalAxis = HullTransform.InverseTransformVectorNoScale(
			ForwardVector->GetForwardVector()).GetSafeNormal();
		FlightUpLocalAxis = HullTransform.InverseTransformVectorNoScale(
			ForwardVector->GetUpVector()).GetSafeNormal();
		if (!FlightForwardLocalAxis.IsNearlyZero() && !FlightUpLocalAxis.IsNearlyZero())
		{
			return;
		}
	}

	int32 ForwardAxisIndex = 0;
	if (LocalExtent.Y > LocalExtent.X && LocalExtent.Y >= LocalExtent.Z)
	{
		ForwardAxisIndex = 1;
	}
	else if (LocalExtent.Z > LocalExtent.X && LocalExtent.Z > LocalExtent.Y)
	{
		ForwardAxisIndex = 2;
	}

	FlightForwardLocalAxis = FVector::ZeroVector;
	FlightForwardLocalAxis[ForwardAxisIndex] = 1.0;
	FlightUpLocalAxis = ForwardAxisIndex == 2 ? FVector::RightVector : FVector::UpVector;
	if (Hull && Hull != SpaceshipHull)
	{
		const FQuat RelativeRotation = Hull->GetRelativeRotation().Quaternion();
		FlightForwardLocalAxis = RelativeRotation.RotateVector(FlightForwardLocalAxis).GetSafeNormal();
		FlightUpLocalAxis = RelativeRotation.RotateVector(FlightUpLocalAxis).GetSafeNormal();
	}
}

void ASpaceship::RefreshFlightReferenceFromHull()
{
	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}
	MainMesh->UpdateBounds();
	FVector LocalMin;
	FVector LocalMax;
	if (GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		ConfigureFlightReferenceFromHull(MainMesh, (LocalMax - LocalMin) * 0.5);
	}
}

void ASpaceship::ConfigureFromHull()
{
	RefreshFlightReferenceFromHull();
	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (MainMesh && bInferSizeClassFromHull)
	{
		const FVector Size = MainMesh->Bounds.BoxExtent * 2.0;
		SizeClass = InferSizeClassFromLength(Size.GetMax());
	}

	ActiveClassPreset = GetPresetForSizeClass(SizeClass);
	if (bGenerateSimpleHullCollision)
	{
		// Generated AI hulls can have complex-as-simple or otherwise unusable bodies.
		// Their lightweight proxy collision is moved kinematically with the actor.
		ActiveClassPreset.bUsesPhysicalImpulse = false;
		RebuildSimpleHullCollision();
	}
	if (IsGroundVehicle())
	{
		// Rio 02.10: a ground vehicle has no space engines and never simulates physics; the camera reads its top speed.
		ActiveClassPreset.bSupportsSpaceWrap = false;
		ActiveClassPreset.bSupportsOffset = false;
		ActiveClassPreset.bUsesPhysicalImpulse = false;
		ActiveClassPreset.bHasInteriorByDefault = false;
		ActiveClassPreset.MaximumFlightMode = EFlightMode::Surface;
		ActiveClassPreset.MaxImpulseSpeed = GroundVehicleKind == EAPSGroundVehicleKind::Drone ? 8000.0
			: GroundVehicleKind == EAPSGroundVehicleKind::Hover ? 5500.0 : 2700.0;
	}
	RotationSpeedDegreesPerSecond = ActiveClassPreset.RotationSpeed;
	ImpulseRotationAcceleration = FMath::DegreesToRadians(ActiveClassPreset.AngularAcceleration);
	if (static_cast<uint8>(SelectedDriveMode) > static_cast<uint8>(GetMaximumDriveModeForClass()))
	{
		SelectedDriveMode = GetMaximumDriveModeForClass();
	}

	if (SpaceshipHull)
	{
		SpaceshipHull->SetMobility(EComponentMobility::Movable);
		SpaceshipHull->SetEnableGravity(false);
		SpaceshipHull->SetLinearDamping(ActiveClassPreset.LinearDamping);
		SpaceshipHull->SetAngularDamping(ActiveClassPreset.AngularDamping);
	}
	ConfigureCameraFromHull();
	ConfigurePilotFillLight();
}

bool ASpaceship::HullHasFittedConvexCollision() const
{
	const UStaticMesh* HullMesh = SpaceshipHull ? SpaceshipHull->GetStaticMesh() : nullptr;
	const UBodySetup* BodySetup = HullMesh ? HullMesh->GetBodySetup() : nullptr;
	return BodySetup && BodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple
		&& BodySetup->AggGeom.ConvexElems.Num() > 0;
}

void ASpaceship::RebuildSimpleHullCollision()
{
	for (UBoxComponent* Box : GeneratedCollisionBoxes)
	{
		if (IsValid(Box))
		{
			Box->DestroyComponent();
		}
	}
	GeneratedCollisionBoxes.Reset();

	if (!bGenerateSimpleHullCollision)
	{
		return;
	}

	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}
	// Hull meshes with fitted convex collision already hug the hull; the tapered boxes below span the
	// whole bounding box and block the pilot metres away from wings and fins.
	if (!bBuildingFlightCollisionProxy && HullHasFittedConvexCollision())
	{
		MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		return;
	}

	FVector LocalMin;
	FVector LocalMax;
	if (!GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		return;
	}
	const FVector Center = (LocalMin + LocalMax) * 0.5;
	const FVector Extent = (LocalMax - LocalMin) * 0.5;
	if (Extent.IsNearlyZero())
	{
		return;
	}

	int32 MajorAxis = 0;
	if (Extent.Y > Extent.X && Extent.Y >= Extent.Z) MajorAxis = 1;
	else if (Extent.Z > Extent.X && Extent.Z > Extent.Y) MajorAxis = 2;

	const int32 SliceCount = FMath::Clamp(SimpleCollisionSliceCount, 3, 9);
	const double HalfSliceLength = Extent[MajorAxis] / SliceCount;
	for (int32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
	{
		const double Along01 = (static_cast<double>(SliceIndex) + 0.5) / SliceCount;
		const double DistanceFromCenter = FMath::Abs(Along01 * 2.0 - 1.0);
		const double Taper = FMath::Lerp(1.0, 0.58, FMath::Pow(DistanceFromCenter, 1.55));

		FVector BoxExtent = Extent * Taper;
		BoxExtent[MajorAxis] = HalfSliceLength * 1.03;
		BoxExtent.X = FMath::Max(BoxExtent.X, 25.0);
		BoxExtent.Y = FMath::Max(BoxExtent.Y, 25.0);
		BoxExtent.Z = FMath::Max(BoxExtent.Z, 25.0);

		FVector BoxCenter = Center;
		BoxCenter[MajorAxis] = LocalMin[MajorAxis] + (SliceIndex * 2.0 + 1.0) * HalfSliceLength;
		UBoxComponent* Box = NewObject<UBoxComponent>(this,
			*FString::Printf(TEXT("SimpleHullCollision_%02d"), SliceIndex), RF_Transient);
		Box->SetupAttachment(MainMesh);
		Box->SetMobility(EComponentMobility::Movable);
		Box->SetBoxExtent(BoxExtent, false);
		Box->SetRelativeLocation(BoxCenter);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCanEverAffectNavigation(false);
		// Rio 07.10 (aps.Ship.HullHold, review): a box never welds into the hull's body (a shape component auto-welds when its
		// parent starts simulating): it would sit in the held body at the take-off place instead of flying with the ship.
		Box->BodyInstance.bAutoWeld = false;
		AddInstanceComponent(Box);
		Box->RegisterComponent();
		GeneratedCollisionBoxes.Add(Box);
	}

	// The imported body is never queried after the proxy hull exists.
	// Rio 07.10 (aps.Ship.HullHold): a held body stays in the scene (SetHullBodyHeld makes it inert); NoCollision would
	// destroy it (the physics bit flips) and its return would build it again.
	if (!(bBuildingFlightCollisionProxy && bHullBodyHeld))
	{
		MainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	MainMesh->SetGenerateOverlapEvents(false);
	UE_LOG(LogTemp, Verbose, TEXT("[APS.Ships] %s generated %d low-cost collision slices for %s"),
		*GetName(), GeneratedCollisionBoxes.Num(), *GetNameSafe(MainMesh));
}

void ASpaceship::ConfigureCameraFromHull()
{
	if (!SpringArmComponent)
	{
		return;
	}

	UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	if (!MainMesh)
	{
		return;
	}

	FVector LocalMin;
	FVector LocalMax;
	if (!GetPrimaryHullLocalBounds(MainMesh, LocalMin, LocalMax))
	{
		return;
	}
	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5;
	const FVector LocalExtent = (LocalMax - LocalMin) * 0.5;
	const FVector ScaledExtent = LocalExtent * MainMesh->GetComponentScale().GetAbs();
	// Rio 02.10: a ground vehicle gets a close chase camera, 6-9 m behind it at rest (the adaptive camera adds 30%).
	const double WorldRadius = IsGroundVehicle() ? FMath::Max(ScaledExtent.Size(), 150.0)
		: FMath::Max(ScaledExtent.Size(), 400.0);
	BaseCameraArmLength = IsGroundVehicle() ? FMath::Clamp(WorldRadius * 2.0, 480.0, 760.0)
		: FMath::Max(820.0, WorldRadius * 1.82);
	if (CameraComponent && !bCameraFieldOfViewInitialized)
	{
		BaseCameraFieldOfView = CameraComponent->FieldOfView;
		bCameraFieldOfViewInitialized = true;
	}
	SpringArmComponent->TargetArmLength = BaseCameraArmLength;
	// Never hard-clamp the lagged camera origin. The spring-arm clamp produces a
	// discontinuity whenever the moving ship crosses the limit, which looks like
	// a periodic stop/go pulse even though the pawn trajectory itself is smooth.
	SpringArmComponent->CameraLagMaxDistance = 0.0f;
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->bUseCameraLagSubstepping = false;
	SpringArmComponent->CameraLagMaxTimeStep = 1.0f / 120.0f;
	SpringArmComponent->bClampToMaxPhysicsDeltaTime = false;
	SpringArmComponent->SetWorldLocation(
		MainMesh->GetComponentTransform().TransformPosition(LocalCenter) + GetShipUpVector() * WorldRadius * 0.2);
	const FRotator FlightViewRotation = FRotationMatrix::MakeFromXZ(
		FlightForwardLocalAxis, FlightUpLocalAxis).Rotator();
	SpringArmComponent->SetRelativeRotation(FlightViewRotation + FRotator(-12.0, 0.0, 0.0));
	// A ground vehicle's camera pulls in rather than sink into the hill behind it (the arm ignores its own vehicle).
	SpringArmComponent->bDoCollisionTest = IsGroundVehicle();
}

void ASpaceship::ConfigurePilotFillLight()
{
	const UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	const float HullRadius = MainMesh ? FMath::Max(MainMesh->Bounds.SphereRadius, 400.0f) : 400.0f;
	const float CameraDistance = FMath::Max(BaseCameraArmLength, 820.0f);
	if (PilotFillLight)
	{
		PilotFillLight->SetAttenuationRadius(CameraDistance + HullRadius * 2.5f);
		PilotFillLight->SetIntensity(FMath::Clamp(2200.0f + HullRadius * 0.35f, 2600.0f, 8500.0f));
	}
	if (PilotFillPointLight)
	{
		PilotFillPointLight->SetAttenuationRadius(CameraDistance + HullRadius * 3.0f);
		PilotFillPointLight->SetIntensity(FMath::Clamp(
			12.0f + HullRadius / 900.0f, 14.0f, 32.0f));
	}

	TArray<UMeshComponent*> ShipMeshComponents;
	GetComponents<UMeshComponent>(ShipMeshComponents);
	for (UMeshComponent* ShipMesh : ShipMeshComponents)
	{
		if (IsValid(ShipMesh))
		{
			// Preserve normal world lighting and opt only this ship into the
			// private camera-fill channel. Rio 04.10 ("ships are black from afar"): channel 1 is the object fill every
			// ship shares (UAPSObjectLightingSubsystem); clearing it here left boarded and new ships unlit on their
			// shadow side, so it is kept.
			ShipMesh->SetLightingChannels(true, ShipMesh->LightingChannels.bChannel1, true);
		}
	}
}

void ASpaceship::UpdatePilotFillLightVisibility()
{
	if (PilotFillLight)
	{
		// Retained for serialized BP compatibility; the point fill is independent
		// of legacy mesh-forward conventions and cannot accidentally light empty space.
		PilotFillLight->SetVisibility(false, true);
	}

	if (PilotFillPointLight)
	{
		// The private fill is a readability fallback for interplanetary darkness,
		// not a replacement for physically meaningful local star/planet lighting.
		// Rio 2026-09-28: this fill lit only the piloted ship, so it read white next to black
		// neighbours. The object fill (aps.Lighting.ObjectFill) now lights every ship alike.
		static const IConsoleVariable* PilotFill = IConsoleManager::Get().FindConsoleVariable(TEXT("aps.Lighting.PilotFill"));
		const bool bShouldUseFill = IsValid(Pilot)
			&& CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace
			&& PilotFill && PilotFill->GetInt() != 0;
		PilotFillPointLight->SetVisibility(bShouldUseFill, true);
	}
}

FVector ASpaceship::GetShipForwardVector() const
{
	return SpaceshipHull
		? SpaceshipHull->GetComponentTransform().TransformVectorNoScale(FlightForwardLocalAxis).GetSafeNormal()
		: GetActorForwardVector();
}

FVector ASpaceship::GetShipUpVector() const
{
	return SpaceshipHull
		? SpaceshipHull->GetComponentTransform().TransformVectorNoScale(FlightUpLocalAxis).GetSafeNormal()
		: GetActorUpVector();
}

FVector ASpaceship::GetShipRightVector() const
{
	return FVector::CrossProduct(GetShipUpVector(), GetShipForwardVector()).GetSafeNormal();
}

EShipDriveMode ASpaceship::GetMaximumDriveModeForClass() const
{
	// All spacecraft keep the complete six-step power range. Engine availability is a separate axis.
	return EShipDriveMode::Interstellar;
}

EShipDriveMode ASpaceship::GetMaximumDriveModeForEnvironment() const
{
	// Environment affects drag and telemetry, never removes player authority.
	return EShipDriveMode::Interstellar;
}

bool ASpaceship::CanUseDriveMode(EShipDriveMode DriveMode) const
{
	return static_cast<uint8>(DriveMode) <= static_cast<uint8>(GetMaximumDriveModeForClass())
		&& static_cast<uint8>(DriveMode) <= static_cast<uint8>(GetMaximumDriveModeForEnvironment());
}

double ASpaceship::GetDriveSpeedScale(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return 1.0;
	case EShipDriveMode::Orbital: return 32.0;
	case EShipDriveMode::Interplanetary: return 256.0;
	case EShipDriveMode::Stellar: return 2048.0;
	case EShipDriveMode::Interstellar: return 8192.0;
	case EShipDriveMode::Landing:
	default: return 0.05;
	}
}

double ASpaceship::GetDriveAccelerationScale(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return 4.0;
	case EShipDriveMode::Orbital: return 192.0;
	case EShipDriveMode::Interplanetary: return 1024.0;
	case EShipDriveMode::Stellar: return 4096.0;
	case EShipDriveMode::Interstellar: return 16384.0;
	case EShipDriveMode::Landing:
	default: return 0.75;
	}
}

double ASpaceship::GetMinimumDriveAcceleration(EShipDriveMode DriveMode)
{
	switch (DriveMode)
	{
	case EShipDriveMode::Orbital: return 500000.0;
	case EShipDriveMode::Interplanetary: return 1500000.0;
	case EShipDriveMode::Stellar: return 6000000.0;
	case EShipDriveMode::Interstellar: return 24000000.0;
	case EShipDriveMode::Landing:
	case EShipDriveMode::Local:
	default: return 0.0;
	}
}

bool ASpaceship::CanUseEngineMode(EEngineMode EngineMode) const
{
	switch (EngineMode)
	{
	case EEngineMode::Impulse: return true;
	case EEngineMode::SpaceWrap: return ActiveClassPreset.bSupportsSpaceWrap;
	case EEngineMode::Offset: return ActiveClassPreset.bSupportsOffset;
	default: return false;
	}
}

double ASpaceship::GetEngineSpeedMultiplier(EEngineMode EngineMode)
{
	switch (EngineMode)
	{
	case EEngineMode::SpaceWrap: return 64.0;
	case EEngineMode::Offset: return 4096.0;
	case EEngineMode::Impulse:
	default: return 1.0;
	}
}

double ASpaceship::GetEngineAccelerationMultiplier(EEngineMode EngineMode)
{
	switch (EngineMode)
	{
	case EEngineMode::SpaceWrap: return 24.0;
	case EEngineMode::Offset: return 256.0;
	case EEngineMode::Impulse:
	default: return 1.0;
	}
}

EEngineMode ASpaceship::ResolveEngineModeForDriveMode(EShipDriveMode DriveMode) const
{
	return SelectedEngineMode;
}

EFlightMode ASpaceship::ResolveLegacyFlightModeForDriveMode(EShipDriveMode DriveMode) const
{
	switch (DriveMode)
	{
	case EShipDriveMode::Local: return EFlightMode::Basic;
	case EShipDriveMode::Orbital: return EFlightMode::Orbital;
	case EShipDriveMode::Interplanetary: return EFlightMode::Interplanetary;
	case EShipDriveMode::Stellar: return EFlightMode::Stellar;
	case EShipDriveMode::Interstellar: return EFlightMode::Interstellar;
	case EShipDriveMode::Landing:
	default: return EFlightMode::Surface;
	}
}

void ASpaceship::SetDriveMode(EShipDriveMode NewDriveMode, bool bImmediate)
{
	if (!OnboardComputer || !CanUseDriveMode(NewDriveMode))
	{
		return;
	}

	SelectedDriveMode = NewDriveMode;
	OnboardComputer->FlightSystem.CurrentFlightMode = ResolveLegacyFlightModeForDriveMode(NewDriveMode);
	OnboardComputer->ComputeFlightParams();
	// Power changes stay within the selected engine and therefore never masquerade as an engine transition.
	RequestEngineModeForFlightMode(true);
	CheckFlightModeChange();
}

void ASpaceship::SetEngineMode(EEngineMode NewEngineMode, bool bImmediate)
{
	if (!OnboardComputer || !CanUseEngineMode(NewEngineMode) || SelectedEngineMode == NewEngineMode)
	{
		return;
	}
	SelectedEngineMode = NewEngineMode;
	RequestEngineModeForFlightMode(bImmediate);
}

void ASpaceship::EnforceDriveModeForEnvironment()
{
	const uint8 Maximum = FMath::Min(
		static_cast<uint8>(GetMaximumDriveModeForClass()),
		static_cast<uint8>(GetMaximumDriveModeForEnvironment()));
	if (static_cast<uint8>(SelectedDriveMode) > Maximum)
	{
		SetDriveMode(static_cast<EShipDriveMode>(Maximum), false);
	}
}

bool ASpaceship::IsNearGravitySurface(const FVector& GravityDirection) const
{
	if (!GetWorld() || GravityDirection.IsNearlyZero())
	{
		return false;
	}

	const UPrimitiveComponent* MainMesh = GetPrimaryHullComponent();
	const double ProbeDistance = MainMesh
		? FMath::Max(static_cast<double>(MainMesh->Bounds.SphereRadius) * 1.25, 2000.0)
		: 2000.0;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(APSShipSurfaceProbe), false, this);
	if (IsValid(Pilot))
	{
		QueryParams.AddIgnoredActor(Pilot);
	}
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(
		Hit,
		GetActorLocation(),
		GetActorLocation() + GravityDirection.GetSafeNormal() * ProbeDistance,
		ECC_Visibility,
		QueryParams);
}

void ASpaceship::UpdateFlightEnvironment(float DeltaTime, bool bForce)
{
	EnvironmentDetectionElapsed += FMath::Max(DeltaTime, 0.0f);
	if (!bForce && EnvironmentDetectionElapsed < EnvironmentDetectionInterval)
	{
		return;
	}
	// A single long frame (for example while WorldScape allocates its first chunks)
	// must not satisfy the whole transition window by itself. Count at most one
	// detector interval so a new state still needs a repeated stable observation.
	const float DetectionSampleTime = FMath::Min(
		EnvironmentDetectionElapsed, FMath::Max(EnvironmentDetectionInterval, 0.01f));
	EnvironmentDetectionElapsed = 0.0f;

	AActor* DetectedGravitySource = nullptr;
	FVector DetectedGravityDirection = FVector::ZeroVector;
	if (FlightGravityDetector)
	{
		FlightGravityDetector->RunGravityCheckForActor(this);
		DetectedGravitySource = FlightGravityDetector->GravityTargetActor;
		DetectedGravityDirection = FlightGravityDetector->GetGravityDirectionAtLocation(GetActorLocation());
	}

	double DetectedGravityAcceleration = 0.0;
	EShipFlightEnvironment DetectedEnvironment = EShipFlightEnvironment::DeepSpace;
	if (DetectedGravitySource && !DetectedGravityDirection.IsNearlyZero())
	{
		DetectedGravityAcceleration = 980.0;
		if (const APlanetaryBody* Planet = Cast<APlanetaryBody>(DetectedGravitySource))
		{
			const double RadiusKm = Planet->RadiusKM > UE_SMALL_NUMBER
				? Planet->RadiusKM : FMath::Max(static_cast<double>(Planet->PlanetRadiusKM), 0.0);
			const double CenterDistanceKm = FVector::Distance(GetActorLocation(), Planet->GetActorLocation()) / 100000.0;
			const double AltitudeKm = FMath::Max(0.0, CenterDistanceKm - RadiusKm);
			const double AtmosphereHeightKm = Planet->AtmosphereHeight > UE_SMALL_NUMBER
				? Planet->AtmosphereHeight : FMath::Max(RadiusKm / 30.0, 1.0);
			const double SurfaceAcceleration = Planet->PlanetGravityStrength > UE_SMALL_NUMBER
				? Planet->PlanetGravityStrength * 980.0 : 980.0;
			const double DistanceFalloff = RadiusKm > UE_SMALL_NUMBER && CenterDistanceKm > RadiusKm
				? FMath::Square(RadiusKm / CenterDistanceKm) : 1.0;
			DetectedGravityAcceleration = FMath::Clamp(SurfaceAcceleration * DistanceFalloff, 5.0, 3000.0);

			if (IsNearGravitySurface(DetectedGravityDirection))
			{
				DetectedEnvironment = EShipFlightEnvironment::Surface;
			}
			else if (AltitudeKm <= AtmosphereHeightKm)
			{
				DetectedEnvironment = EShipFlightEnvironment::Atmosphere;
			}
			else
			{
				DetectedEnvironment = EShipFlightEnvironment::GravityWell;
			}
		}
		else
		{
			DetectedEnvironment = IsNearGravitySurface(DetectedGravityDirection)
				? EShipFlightEnvironment::Surface : EShipFlightEnvironment::GravityWell;
		}
	}

	const bool bMatchesActiveState = bFlightEnvironmentInitialized
		&& DetectedEnvironment == CurrentFlightEnvironment
		&& DetectedGravitySource == ActiveGravitySource.Get();
	bool bCommitDetectedState = bForce || !bFlightEnvironmentInitialized;
	if (!bCommitDetectedState && bMatchesActiveState)
	{
		// Direction and acceleration continuously change in a planetary field, but
		// the source and environment remain stable and do not need debouncing.
		ActiveGravityDirection = DetectedGravityDirection;
		ActiveGravityAcceleration = DetectedGravityAcceleration;
		PendingGravitySource.Reset();
		PendingEnvironmentTransitionElapsed = 0.0f;
	}
	else if (!bCommitDetectedState)
	{
		const bool bMatchesPendingState = DetectedEnvironment == PendingFlightEnvironment
			&& DetectedGravitySource == PendingGravitySource.Get();
		if (!bMatchesPendingState)
		{
			PendingFlightEnvironment = DetectedEnvironment;
			PendingGravitySource = DetectedGravitySource;
			PendingEnvironmentTransitionElapsed = 0.0f;
		}

		PendingEnvironmentTransitionElapsed += DetectionSampleTime;
		bCommitDetectedState = PendingEnvironmentTransitionElapsed
			>= EnvironmentTransitionConfirmationTime;
	}

	if (bCommitDetectedState)
	{
		const EShipFlightEnvironment PreviousEnvironment = CurrentFlightEnvironment;
		AActor* PreviousSource = ActiveGravitySource.Get();
		CurrentFlightEnvironment = DetectedEnvironment;
		ActiveGravitySource = DetectedGravitySource;
		ActiveGravityDirection = DetectedGravityDirection;
		ActiveGravityAcceleration = DetectedGravityAcceleration;
		bFlightEnvironmentInitialized = true;
		PendingGravitySource.Reset();
		PendingEnvironmentTransitionElapsed = 0.0f;

		if (PreviousEnvironment != CurrentFlightEnvironment || PreviousSource != ActiveGravitySource.Get())
		{
			EnforceDriveModeForEnvironment();
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] Environment ship=%s environment=%s gravity=%s acceleration=%.2f m/s2"),
				*GetName(), *GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0);
		}
	}
	if (OnboardComputer)
	{
		switch (CurrentFlightEnvironment)
		{
		case EShipFlightEnvironment::Surface:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Landing;
			break;
		case EShipFlightEnvironment::Atmosphere:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Atmospheric;
			break;
		case EShipFlightEnvironment::GravityWell:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::Orbital;
			break;
		case EShipFlightEnvironment::DeepSpace:
		default:
			OnboardComputer->FlightSystem.CurrentFlightType = EFlightType::ZeroG;
			break;
		}
	}

	if (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->SetLinearDamping(GetEnvironmentDrag());
	}
	UpdatePilotFillLightVisibility();
}

double ASpaceship::GetEnvironmentDrag() const
{
	switch (CurrentFlightEnvironment)
	{
	case EShipFlightEnvironment::Surface: return SurfaceDrag;
	case EShipFlightEnvironment::Atmosphere: return AtmosphericDrag;
	case EShipFlightEnvironment::GravityWell: return GravityWellDrag;
	case EShipFlightEnvironment::DeepSpace:
	default: return 0.0;
	}
}

void ASpaceship::ApplyEnvironmentForces(float DeltaTime)
{
	if (!bApplyExternalGravity || !ActiveGravitySource || ActiveGravityDirection.IsNearlyZero()
		|| ActiveGravityAcceleration <= UE_SMALL_NUMBER)
	{
		return;
	}
	if (bEngineRunning && bFlightAssistCompensatesGravity)
	{
		return;
	}

	const FVector GravityAcceleration = ActiveGravityDirection.GetSafeNormal() * ActiveGravityAcceleration;
	if (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->AddForce(GravityAcceleration, NAME_None, true);
	}
	else
	{
		KinematicVelocity += GravityAcceleration * DeltaTime;
	}
}

void ASpaceship::UpdateCameraNoseAlignment()
{
	if (!CameraComponent)
	{
		return;
	}
	// Rio 06.10 ("aiming by the screen centre points 12 degrees below the nose"): the arm stands 12 degrees above the flight
	// axis (ConfigureCameraFromHull) and the camera looks down along it. Turned back up by the same 12 degrees on the arm's
	// end, the camera keeps its place above and behind and looks parallel to the nose; the ship sits lower in the frame.
	// A ground vehicle keeps its own camera.
	const bool bAlign = APSShipPerf::CVarCameraAlignNose.GetValueOnGameThread() != 0 && !IsGroundVehicle();
	if (bAlign == bCameraAlignNoseApplied)
	{
		return;
	}
	if (bAlign)
	{
		CameraAlignBaseRotation = CameraComponent->GetRelativeRotation();
		CameraComponent->SetRelativeRotation(FRotator(12.0, 0.0, 0.0).Quaternion() * CameraAlignBaseRotation.Quaternion());
	}
	else
	{
		CameraComponent->SetRelativeRotation(CameraAlignBaseRotation);
	}
	bCameraAlignNoseApplied = bAlign;
}

void ASpaceship::UpdateAdaptiveFlightCamera(float DeltaTime)
{
	if (IsValid(Pilot))
	{
		UpdateCameraNoseAlignment();
	}
	if (!bUseAdaptiveFlightCamera || !SpringArmComponent || !IsValid(Pilot))
	{
		return;
	}
	const double Speed = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity().Size() : KinematicVelocity.Size();
	// Camera response is driven only by continuous physical speed. Using the selected
	// drive/engine limit here made the same velocity produce a different camera pose
	// immediately after every mode change. Speeds span metres to light years per second, so the
	// pose reads the logarithm of the speed (29.09: the old log2/8 mapping saturated at ~250 km/s
	// and left the camera still for every faster flight).
	const double ClassReferenceSpeed = FMath::Max(ActiveClassPreset.MaxImpulseSpeed, 1.0);
	const float TargetCameraAlpha = FMath::Clamp(static_cast<float>(
		FMath::LogX(10.0, 1.0 + Speed / ClassReferenceSpeed) / 7.0), 0.0f, 1.0f);
	SmoothedCameraSpeedAlpha = FMath::FInterpTo(
		SmoothedCameraSpeedAlpha, TargetCameraAlpha, DeltaTime, 1.2f);
	const float CameraAlpha = SmoothedCameraSpeedAlpha;

	// Acceleration feel at every scale: the rate of ln(speed), smoothed over about a second and only outward. Speeding
	// up lets the ship ease away from the camera; slowing down never pulls the camera in (Rio, 30.09: jerky).
	const double LogSpeed = FMath::Loge(FMath::Max(Speed, 100.0));
	const float LogSpeedRate = PreviousCameraLogSpeed < 0.0 || DeltaTime <= 0.0f ? 0.0f
		: FMath::Clamp(static_cast<float>((LogSpeed - PreviousCameraLogSpeed) / DeltaTime), -4.0f, 4.0f);
	PreviousCameraLogSpeed = LogSpeed;
	SmoothedLogSpeedRate = FMath::FInterpTo(SmoothedLogSpeedRate, LogSpeedRate, DeltaTime, 1.0f);
	const float Thrust = FMath::Clamp(SmoothedLogSpeedRate / 2.0f, 0.0f, 1.0f);

	// Hull bounds define the baseline, a third farther at rest than the hull alone asks for (Rio, 30.09: too close at
	// low speed). Speed and thrust ease the camera back on a critically damped follow (Game Programming Gems 4, 1.10)
	// that settles in about a second without overshoot: no kick on band shifts, no shake.
	// A ground vehicle's camera eases back with its own top speed (Rio 02.10), a quarter farther at full speed.
	const float SpeedPull = IsGroundVehicle()
		? 0.25f * static_cast<float>(FMath::Min(Speed / ClassReferenceSpeed, 1.5)) : 0.30f * CameraAlpha;
	const float TargetArmLength = BaseCameraArmLength * (1.3f + SpeedPull + 0.12f * Thrust);
	if (DeltaTime > 0.0f)
	{
		constexpr float ArmSmoothSeconds = 0.9f;
		const float Omega = 2.0f / ArmSmoothSeconds;
		const float X = Omega * DeltaTime;
		const float Decay = 1.0f / (1.0f + X + 0.48f * X * X + 0.235f * X * X * X);
		const float Change = SpringArmComponent->TargetArmLength - TargetArmLength;
		const float Carried = (CameraArmLengthRate + Omega * Change) * DeltaTime;
		CameraArmLengthRate = (CameraArmLengthRate - Omega * Carried) * Decay;
		SpringArmComponent->TargetArmLength = TargetArmLength + (Change + Carried) * Decay;
	}
	// Positional lag is intentionally disabled. At astronomical velocities a
	// spring-arm positional integrator alternates between a huge error and a huge
	// correction, while smoothly interpolated arm length/FOV retain the chase feel.
	SpringArmComponent->bEnableCameraLag = false;
	SpringArmComponent->CameraRotationLagSpeed = FMath::Lerp(7.0f, 12.0f, CameraAlpha);
	SpringArmComponent->CameraLagMaxDistance = 0.0f;
	UpdateMouseLook(DeltaTime);
	if (IsGroundVehicle())
	{
		UpdateGroundVehicleCamera();
	}
	else if (bMouseLookApplied || !FMath::IsNearlyZero(MouseLookYaw, 0.01) || !FMath::IsNearlyZero(MouseLookPitch, 0.01))
	{
		// Rio 02.10: mouse look orbits the chase view about the ship's own up and the view's right; back at zero the arm
		// stands exactly as the camera setup left it.
		const FQuat Chase = (FRotationMatrix::MakeFromXZ(FlightForwardLocalAxis, FlightUpLocalAxis).Rotator()
			+ FRotator(-12.0, 0.0, 0.0)).Quaternion();
		SpringArmComponent->SetRelativeRotation((FQuat(FlightUpLocalAxis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector),
			FMath::DegreesToRadians(MouseLookYaw)) * Chase
			* FQuat(FVector::RightVector, FMath::DegreesToRadians(-MouseLookPitch))).GetNormalized());
		bMouseLookApplied = !FMath::IsNearlyZero(MouseLookYaw, 0.01) || !FMath::IsNearlyZero(MouseLookPitch, 0.01);
	}
	if (CameraComponent)
	{
		// Speed reads through arm length, vignette and bloom. A changing FOV invalidates the star optics every
		// frame (see aps.Ship.SpeedFov), so the widening is opt-in.
		const float SpeedFieldOfView = APSShipPerf::CVarSpeedFov.GetValueOnGameThread() != 0 ? CameraAlpha * 12.0f : 0.0f;
		const float TargetFieldOfView = BaseCameraFieldOfView + SpeedFieldOfView;
		CameraComponent->SetFieldOfView(FMath::FInterpTo(
			CameraComponent->FieldOfView, TargetFieldOfView, DeltaTime, 2.6f));

		FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
		PostProcess.bOverride_SceneFringeIntensity = true;
		PostProcess.bOverride_ChromaticAberrationStartOffset = true;
		PostProcess.bOverride_VignetteIntensity = true;
		PostProcess.bOverride_BloomIntensity = true;
		PostProcess.bOverride_AutoExposureBias = true;
		const float CinematicAlpha = FMath::Square(CameraAlpha);
		// Chromatic aberration splits every high-contrast HDR point into red/green/
		// blue copies. At interplanetary boost this turned the galaxy into a fringe
		// test chart and destroyed the physically authored spectral colour. Keep the
		// speed sensation in FOV, arm length, vignette and bloom; stellar light must
		// remain spectrally coherent at every velocity.
		PostProcess.SceneFringeIntensity = 0.0f;
		PostProcess.ChromaticAberrationStartOffset = 0.0f;
		// Sustained thrust deepens the vignette and bloom a little on top of the speed level.
		const float Pulse = 0.08f * Thrust;
		PostProcess.VignetteIntensity = FMath::FInterpTo(PostProcess.VignetteIntensity,
			FMath::Max(BaseVignetteIntensity, 0.16f + CinematicAlpha * 0.12f + Pulse), DeltaTime, 1.5f);
		PostProcess.BloomIntensity = FMath::FInterpTo(PostProcess.BloomIntensity,
			FMath::Max(BaseBloomIntensity, 0.45f + CinematicAlpha * 0.22f + Pulse), DeltaTime, 1.5f);
		// Exposure is intentionally speed-invariant; only the lens response changes with velocity.
		PostProcess.AutoExposureBias = BaseAutoExposureBias;
		CameraComponent->PostProcessBlendWeight = FMath::Max(BaseCameraPostProcessBlendWeight, 1.0f);
	}
}

void ASpaceship::InitializeFlightPostProcess()
{
	if (!CameraComponent || bCameraPostProcessInitialized)
	{
		return;
	}
	const FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
	BaseCameraPostProcessBlendWeight = CameraComponent->PostProcessBlendWeight;
	BaseSceneFringeIntensity = PostProcess.SceneFringeIntensity;
	BaseChromaticAberrationStartOffset = PostProcess.ChromaticAberrationStartOffset;
	BaseVignetteIntensity = PostProcess.VignetteIntensity;
	BaseBloomIntensity = PostProcess.BloomIntensity;
	BaseAutoExposureBias = PostProcess.AutoExposureBias;
	bBaseOverrideSceneFringe = PostProcess.bOverride_SceneFringeIntensity;
	bBaseOverrideChromaticStart = PostProcess.bOverride_ChromaticAberrationStartOffset;
	bBaseOverrideVignette = PostProcess.bOverride_VignetteIntensity;
	bBaseOverrideBloom = PostProcess.bOverride_BloomIntensity;
	bBaseOverrideExposureBias = PostProcess.bOverride_AutoExposureBias;
	bCameraPostProcessInitialized = true;
}

void ASpaceship::RestoreFlightPostProcess()
{
	if (!CameraComponent || !bCameraPostProcessInitialized)
	{
		return;
	}
	FPostProcessSettings& PostProcess = CameraComponent->PostProcessSettings;
	PostProcess.SceneFringeIntensity = BaseSceneFringeIntensity;
	PostProcess.ChromaticAberrationStartOffset = BaseChromaticAberrationStartOffset;
	PostProcess.VignetteIntensity = BaseVignetteIntensity;
	PostProcess.BloomIntensity = BaseBloomIntensity;
	PostProcess.AutoExposureBias = BaseAutoExposureBias;
	PostProcess.bOverride_SceneFringeIntensity = bBaseOverrideSceneFringe;
	PostProcess.bOverride_ChromaticAberrationStartOffset = bBaseOverrideChromaticStart;
	PostProcess.bOverride_VignetteIntensity = bBaseOverrideVignette;
	PostProcess.bOverride_BloomIntensity = bBaseOverrideBloom;
	PostProcess.bOverride_AutoExposureBias = bBaseOverrideExposureBias;
	CameraComponent->PostProcessBlendWeight = BaseCameraPostProcessBlendWeight;
}

void ASpaceship::SetHullSceneLightingExcluded(bool bExcluded)
{
	if (!bExcluded)
	{
		for (const FHullSceneLightingFlags& Flags : ExcludedHullSceneLighting)
		{
			if (UPrimitiveComponent* Component = Flags.Component.Get())
			{
				Component->SetAffectDistanceFieldLighting(Flags.bAffectDistanceField);
				Component->SetAffectDynamicIndirectLighting(Flags.bAffectIndirect);
			}
		}
		ExcludedHullSceneLighting.Reset();
		AppliedHullSceneLightingMode = 1;
		return;
	}
	const int32 Mode = APSShipPerf::CVarHullSceneLightingInFlight.GetValueOnGameThread();
	if (!ExcludedHullSceneLighting.IsEmpty() || Mode == 1)
	{
		return;
	}
	AppliedHullSceneLightingMode = Mode;
	const bool bLeaveDistanceField = Mode == 0 || Mode == 2;
	const bool bLeaveLumenScene = Mode == 0 || Mode == 3;
	TArray<AActor*> Actors{this};
	GetAttachedActors(Actors, false, true);
	for (const AActor* Actor : Actors)
	{
		// The pilot keeps its own flags; it is hidden while seated.
		if (!IsValid(Actor) || (Actor != this && Actor->IsA<APawn>()))
		{
			continue;
		}
		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents(Components);
		for (UPrimitiveComponent* Component : Components)
		{
			if (!IsValid(Component) || (!Component->bAffectDistanceFieldLighting && !Component->bAffectDynamicIndirectLighting))
			{
				continue;
			}
			ExcludedHullSceneLighting.Add({Component, Component->bAffectDistanceFieldLighting != 0,
				Component->bAffectDynamicIndirectLighting != 0});
			if (bLeaveDistanceField)
			{
				Component->SetAffectDistanceFieldLighting(false);
			}
			if (bLeaveLumenScene)
			{
				Component->SetAffectDynamicIndirectLighting(false);
			}
		}
	}
}

void ASpaceship::StabilizeFullScaleVisualVelocity()
{
	if (!IsValid(Pilot) || (SpaceshipHull && SpaceshipHull->IsSimulatingPhysics())
		|| APSShipPerf::CVarResetHullVelocity.GetValueOnGameThread() == 0
		|| APSShipPerf::CVarTsrHistoryInFlight.GetValueOnGameThread() != 0)
	{
		return;
	}
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (IsValid(Component) && Component->IsVisible())
		{
			// The camera shares the kinematic full-scale translation. Treating that offset as
			// local object motion produces enormous invalid temporal velocities on the hull.
			Component->ResetSceneVelocity();
		}
	}
}

void ASpaceship::SetFlightCollisionOptimization(bool bEnabled)
{
	UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
	if (bGenerateSimpleHullCollision || !PrimaryHull)
	{
		return;
	}
	// Rio 05.10 evening (flight FPS, A/B): a detailed hull flies on the proxy even when authored without it, and its own
	// body leaves the physics scene meanwhile (aps.Ship.DetailedHullFlightProxy).
	const UBodySetup* HullBodySetup = PrimaryHull->GetBodySetup();
	const bool bDetailedHullProxy = !bOptimizeCollisionWhilePiloted
		&& APSShipPerf::CVarDetailedHullFlightProxy.GetValueOnGameThread() != 0 && HullBodySetup
		&& HullBodySetup->AggGeom.GetElementCount() > APSShipPerf::CVarDetailedHullShapes.GetValueOnGameThread();
	if (bEnabled && !bFlightCollisionOptimizationActive && (bOptimizeCollisionWhilePiloted || bDetailedHullProxy))
	{
		OriginalHullCollisionProfile = PrimaryHull->GetCollisionProfileName();
		OriginalHullCollisionEnabled = PrimaryHull->GetCollisionEnabled();
		OriginalHullCollisionResponses = PrimaryHull->GetCollisionResponseToChannels();
		bOriginalHullSimulatesPhysics = PrimaryHull->IsSimulatingPhysics();
		bOriginalHullGenerateOverlapEvents = PrimaryHull->GetGenerateOverlapEvents();
		// Rio 07.10 (aps.Ship.HullHold): decided once per take-out; the return undoes exactly what was done.
		bHullBodyHeld = bDetailedHullProxy && ShouldHoldHullBody(*PrimaryHull);
		bGenerateSimpleHullCollision = true;
		bBuildingFlightCollisionProxy = true;
		RebuildSimpleHullCollision();
		bBuildingFlightCollisionProxy = false;
		bGenerateSimpleHullCollision = false;
		ActiveClassPreset.bUsesPhysicalImpulse = false;
		bFlightCollisionOptimizationActive = true;
		ApplyEngineState();
		if (bHullBodyHeld)
		{
			const double HoldStart = FPlatformTime::Seconds();
			SetHullBodyHeld(true);
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s flies on %d proxy boxes; its hull's %d collision shapes stay in the scene, ")
				TEXT("held and inert (%.2f ms)"), *GetName(), GeneratedCollisionBoxes.Num(), HullBodySetup->AggGeom.GetElementCount(),
				(FPlatformTime::Seconds() - HoldStart) * 1000.0);
		}
		else if (bDetailedHullProxy && PrimaryHull->IsPhysicsStateCreated())
		{
			// Without collision the body stays in the scene, and so does the walk over its shapes on every move.
			PrimaryHull->DestroyPhysicsState();
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s flies on %d proxy boxes; its hull's %d collision shapes leave the scene"),
				*GetName(), GeneratedCollisionBoxes.Num(), HullBodySetup->AggGeom.GetElementCount());
		}
	}
	else if (!bEnabled && bFlightCollisionOptimizationActive)
	{
		const double RestoreStart = FPlatformTime::Seconds();
		const bool bWasHeld = bHullBodyHeld;
		const bool bHullBodyWasOut = !bWasHeld && !PrimaryHull->IsPhysicsStateCreated();
		const bool bRestoreOnce = APSShipPerf::CVarHullRestoreOnce.GetValueOnGameThread() != 0;
		bHullRestorePending = false;
		bProxyBoxesPassWalkers = false;
		HullRestSeconds = 0.0f;
		if (bRestoreOnce && IsHullGoingAway())
		{
			// Rio 06.10 (aps.Ship.HullRestoreOnce): a ship being destroyed or a world being torn down gets no body back
			// (q01-far-b: 344 ms spent rebuilding the M5's body at the very end of the run).
			bFlightCollisionOptimizationActive = false;
			bHullBodyHeld = false;
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s is going away: its hull's body is not built again"), *GetName());
			return;
		}
		for (UBoxComponent* Box : GeneratedCollisionBoxes)
		{
			if (IsValid(Box))
			{
				Box->DestroyComponent();
			}
		}
		GeneratedCollisionBoxes.Reset();
		UStaticMeshComponent* StaticHull = Cast<UStaticMeshComponent>(PrimaryHull);
		if (bWasHeld)
		{
			// Rio 07.10 (aps.Ship.HullHold): the body never left the scene: one teleport and its authored filters, no build.
			SetHullBodyHeld(false);
		}
		else if (bRestoreOnce && bHullBodyWasOut && StaticHull)
		{
			// Rio 06.10 (aps.Ship.HullRestoreOnce, the freeze when the pilot gets up): the component setters below each check
			// the physics state, and the first one that turns collision on builds the 13.5k-shape body before the rest of its
			// settings are in. While the body is out, the body instance takes the profile, responses and kinematic state as
			// plain data (no shapes to filter), and the body is built once, already final. The overlap events the proxy build
			// switched off come back as authored (aps.Ship.HullRestoreOverlaps).
			FBodyInstance& HullBody = StaticHull->BodyInstance;
			HullBody.SetInstanceSimulatePhysics(false);
			if (APSShipPerf::CVarHullRestoreOverlaps.GetValueOnGameThread() != 0)
			{
				StaticHull->SetGenerateOverlapEvents(bOriginalHullGenerateOverlapEvents);
			}
			HullBody.SetCollisionProfileName(OriginalHullCollisionProfile);
			HullBody.SetResponseToChannels(OriginalHullCollisionResponses);
			if (HullBody.GetCollisionEnabled(false) != OriginalHullCollisionEnabled)
			{
				// Turning physics on here builds the body (FBodyInstance::SetCollisionEnabled), with the settings above.
				HullBody.SetCollisionEnabled(OriginalHullCollisionEnabled, false);
			}
			if (!StaticHull->IsPhysicsStateCreated())
			{
				StaticHull->RecreatePhysicsState();
			}
			StaticHull->ClearSkipUpdateOverlaps();
		}
		else
		{
			PrimaryHull->SetCollisionProfileName(OriginalHullCollisionProfile);
			PrimaryHull->SetCollisionEnabled(OriginalHullCollisionEnabled);
			PrimaryHull->SetCollisionResponseToChannels(OriginalHullCollisionResponses);
			PrimaryHull->SetSimulatePhysics(false);
			if (!PrimaryHull->IsPhysicsStateCreated())
			{
				PrimaryHull->RecreatePhysicsState();
			}
		}
		bFlightCollisionOptimizationActive = false;
		ConfigureFromHull();
		if (!IsValid(Pilot) && !IsPlayerControlled())
		{
			// Rio 07.10 (fleet audit): a restore with nobody at the helm (the motion timer) keeps the hull kinematic, as the band
			// model's EnsureKinematicHull does every tick: SetSimulatePhysics(true) would detach the ship from its berth or
			// target (UE 5.4 FBodyInstance::SetInstanceSimulatePhysics). The pilot getting up still has both here (UnPossessed).
			ActiveClassPreset.bUsesPhysicalImpulse = false;
		}
		ApplyEngineState();
		if (bHullBodyWasOut)
		{
			// Rio 07.10 (aps.Ship.HullRestoreSpacingSeconds): a build ended now, the pilot's own included.
			APSShipPerf::GLastHullBuildWorld = GetWorld();
			APSShipPerf::GLastHullBuildEndSeconds = FPlatformTime::Seconds();
		}
		if (bHullBodyWasOut || bWasHeld)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[APS.Ships] %s: the hull's own collision is back (%.1f ms, body %s, %s; profile %s (authored %s), enabled %d (authored %d), responses %s, overlaps %d)"),
				*GetName(), (FPlatformTime::Seconds() - RestoreStart) * 1000.0,
				PrimaryHull->IsPhysicsStateCreated() ? TEXT("in") : TEXT("OUT"),
				bWasHeld ? TEXT("held: teleport + filters") : bRestoreOnce && StaticHull ? TEXT("one build") : TEXT("setter chain"),
				*PrimaryHull->GetCollisionProfileName().ToString(), *OriginalHullCollisionProfile.ToString(),
				static_cast<int32>(PrimaryHull->GetCollisionEnabled()), static_cast<int32>(OriginalHullCollisionEnabled),
				PrimaryHull->GetCollisionResponseToChannels() == OriginalHullCollisionResponses ? TEXT("as authored") : TEXT("CHANGED"),
				PrimaryHull->GetGenerateOverlapEvents() ? 1 : 0);
		}
	}
}

bool ASpaceship::IsHullGoingAway() const
{
	const UWorld* World = GetWorld();
	return bHullGoingAway || IsActorBeingDestroyed() || !World || World->bIsTearingDown || IsEngineExitRequested();
}

bool ASpaceship::ShouldHoldHullBody(const UPrimitiveComponent& PrimaryHull) const
{
	// Rio 07.10 (aps.Ship.HullHold): only the root hull mesh can stop following its moves, and only a body whose physics bit
	// stays on (QueryAndPhysics -> PhysicsOnly) changes without a build (UE 5.4 FBodyInstance::SetCollisionEnabled); a
	// skeletal or query-only hull, or one frozen far away (APSRealScale: actor collision off), takes the old path.
	const int32 Mode = APSShipPerf::CVarHullHold.GetValueOnGameThread();
	const bool bSeated = IsValid(Pilot) || IsPlayerControlled();
	if (!(Mode > 0 && (Mode >= 2 || !bSeated) && PrimaryHull.IsA<UAPSShipHullComponent>() && GetActorEnableCollision()
		&& PrimaryHull.IsRegistered() && PrimaryHull.IsPhysicsStateCreated()
		&& CollisionEnabledHasPhysics(PrimaryHull.BodyInstance.GetCollisionEnabled(false)) && !IsHullGoingAway()))
	{
		return false;
	}
	// Review: a child welded into the hull's body keeps its own blocking filters there and would stay behind with the held
	// body; such a ship takes the old path.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive != &PrimaryHull && Primitive->BodyInstance.WeldParent == &PrimaryHull.BodyInstance)
		{
			return false;
		}
	}
	return true;
}

void ASpaceship::SetHullBodyHeld(const bool bHold)
{
	UAPSShipHullComponent* Hull = Cast<UAPSShipHullComponent>(GetPrimaryHullComponent());
	if (!Hull)
	{
		bHullBodyHeld = false;
		return;
	}
	if (bHold)
	{
		// Stops following first, then inert in one filter pass: PhysicsOnly takes every shape out of queries, ignoring every
		// channel takes it out of the simulation's pairs; the physics bit stays, so nothing is destroyed or built.
		Hull->SetBodyHeld(true);
		FBodyInstance& Body = Hull->BodyInstance;
		Body.SetCollisionEnabled(ECollisionEnabled::PhysicsOnly, false);
		if (!Body.SetResponseToAllChannels(ECR_Ignore))
		{
			Body.UpdatePhysicsFilterData();
		}
		bHullBodyHeld = true;
		return;
	}
	// Teleported while still inert, then the same setter chain as a built body gets (profile, enabled, responses): with the
	// physics bit unchanged each one only re-filters the shapes.
	Hull->SetBodyHeld(false);
	Hull->SetCollisionProfileName(OriginalHullCollisionProfile);
	Hull->SetCollisionEnabled(OriginalHullCollisionEnabled);
	Hull->SetCollisionResponseToChannels(OriginalHullCollisionResponses);
	Hull->SetSimulatePhysics(false);
	bHullBodyHeld = false;
}

bool ASpaceship::MayRestoreHullBody(const bool bPacedBuild) const
{
	// Rio 07.10 (fleet audit): a ship frozen far away (APSRealScale) has its actor collision off: a build there creates no
	// body (ShouldCreatePhysicsState), the boxes would go and the ship would be left with no collision at all after the
	// thaw. It waits; the thaw gives the actor its collision back and the next check restores.
	if (!GetActorEnableCollision())
	{
		return false;
	}
	if (bPacedBuild && !bHullBodyHeld)
	{
		const float Spacing = APSShipPerf::CVarHullRestoreSpacingSeconds.GetValueOnGameThread();
		if (Spacing > 0.0f && APSShipPerf::GLastHullBuildWorld.Get() == GetWorld()
			&& FPlatformTime::Seconds() - APSShipPerf::GLastHullBuildEndSeconds < Spacing)
		{
			return false;
		}
	}
	return true;
}

bool ASpaceship::HasDetailedHullProxy() const
{
	// The same test SetFlightCollisionOptimization makes for its detailed-hull proxy (aps.Ship.DetailedHullFlightProxy).
	UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
	const UBodySetup* HullBodySetup = PrimaryHull ? PrimaryHull->GetBodySetup() : nullptr;
	return !bGenerateSimpleHullCollision && !bOptimizeCollisionWhilePiloted && !IsGroundVehicle() && HullBodySetup
		&& APSShipPerf::CVarDetailedHullFlightProxy.GetValueOnGameThread() != 0
		&& HullBodySetup->AggGeom.GetElementCount() > APSShipPerf::CVarDetailedHullShapes.GetValueOnGameThread();
}

bool ASpaceship::HasWalkShell() const
{
	// Rio 06.10 afternoon (aps.Ship.WalkOnShellAtSpeed): the outer shell is not the decks on every ship; a walker is only
	// left on it when asked to.
	if (APSShipPerf::CVarWalkOnShellAtSpeed.GetValueOnGameThread() == 0)
	{
		return false;
	}
	// Codex's walk shells (M5HullShellCollision, the cargo ship's ShellCol): a query-only trimesh that blocks pawns. Read
	// without the actor's collision switch, so a ship frozen far away (APSRealScale) still knows it has one.
	const UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		if (IsValid(Primitive) && Primitive != PrimaryHull
			&& (Primitive->ComponentHasTag(TEXT("APS.Ship.CollisionShell")) || Primitive->GetName().Contains(TEXT("ShellCol")))
			&& CollisionEnabledHasQuery(Primitive->BodyInstance.GetCollisionEnabled(false))
			&& Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
		{
			return true;
		}
	}
	return false;
}

bool ASpaceship::IsWalkerAboard() const
{
	TArray<AActor*> Riders;
	GetAttachedActors(Riders, true, true);
	for (const AActor* Rider : Riders)
	{
		// A vehicle carried in a bay is a pawn too, but nobody walks in it.
		if (IsValid(Rider) && Rider != Pilot.Get() && Rider->IsA<APawn>() && !Rider->IsA<ASpaceship>())
		{
			return true;
		}
	}
	return false;
}

const TCHAR* ASpaceship::GetHullMotionReason() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	// The flight model moves a ship only while it ticks; a parked one keeps whatever velocity it was left with.
	const double MaxSpeed = FMath::Max(APSShipPerf::CVarHullRestoreMaxSpeedCm.GetValueOnGameThread(), 0.0f);
	if (IsActorTickEnabled() && KinematicVelocity.SizeSquared() > FMath::Square(MaxSpeed))
	{
		return TEXT("speed");
	}
	// Rio 06.10 (audit: an engaged autopilot on a parked ship): the autopilot moves the ship only through
	// UAPSShipFlightModel::ApplyTranslation, which runs from Tick and returns before UpdateAutopilot unless the band model is
	// on and the engine runs; a ship parked with its engine off and the autopilot still set is not moving.
	if (FlightModel && FlightModel->IsAutopilotEngaged() && IsActorTickEnabled() && GetEngineRunning()
		&& FlightModel->IsBandFlightActive())
	{
		return TEXT("autopilot");
	}
	if (World->GetTimeSeconds() - HullLastFlowSeconds < 0.5)
	{
		return TEXT("world flow");
	}
	if (UAPSWorldOriginSubsystem::IsStillObserver(this))
	{
		return TEXT("owed travel");
	}
	// A fleet unit is teleported along its order by the fleet (APSFleetCommand Fly), not by its own tick.
	if (const FAPSFleetCommand* Fleet = APSFleetFind(World))
	{
		if (const FAPSFleetUnit* Unit = Fleet->FindUnit(this);
			Unit && (Unit->Phase == APSFleet::EPhase::Departing || Unit->Phase == APSFleet::EPhase::Transit))
		{
			return TEXT("fleet order");
		}
	}
	return nullptr;
}

bool ASpaceship::IsPlayerOnFootNear(const double ExtraCm) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = Player ? Player->GetPawn() : nullptr;
	const UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
	if (!PlayerPawn || PlayerPawn->IsA<ASpaceship>() || !PrimaryHull)
	{
		return false;
	}
	const double Reach = PrimaryHull->Bounds.SphereRadius + FMath::Max(ExtraCm, 0.0);
	return FVector::DistSquared(PlayerPawn->GetActorLocation(), PrimaryHull->Bounds.Origin) <= FMath::Square(Reach);
}

void ASpaceship::SetProxyBoxesPassWalkers(const bool bPass)
{
	if (bProxyBoxesPassWalkers == bPass)
	{
		return;
	}
	bProxyBoxesPassWalkers = bPass;
	for (UBoxComponent* Box : GeneratedCollisionBoxes)
	{
		if (!IsValid(Box))
		{
			continue;
		}
		if (bPass)
		{
			// The walker stands on the walk shell; its capsule, its camera arm and its foot traces pass the boxes. Ships,
			// stations and ground still meet them (the proxy sweep reads these responses).
			Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		}
		else
		{
			// As RebuildSimpleHullCollision builds them.
			Box->SetCollisionResponseToAllChannels(ECR_Block);
		}
	}
}

bool ASpaceship::KeepHullOutForWalker()
{
	// Rio 06.10 (collision by motion): the body stays out for the walker; the timer builds it once the ship has rested and
	// the walker has stepped off. Without a walk shell to stand on the walker needs the hull's own body (as before).
	UWorld* World = GetWorld();
	if (!World || APSShipPerf::CVarKeepHullOutWhileMoving.GetValueOnGameThread() == 0 || !bFlightCollisionOptimizationActive
		|| IsHullGoingAway() || !HasDetailedHullProxy() || !HasWalkShell())
	{
		return false;
	}
	bHullRestorePending = true;
	HullRestSeconds = 0.0f;
	HullPendingSinceSeconds = World->GetTimeSeconds();
	SetProxyBoxesPassWalkers(true);
	StartHullMotionWatch();
	const TCHAR* Motion = GetHullMotionReason();
	UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s: the pilot got up (%.1f m/s, %s): the hull stays out; the walker stands on the walk ")
		TEXT("shell, %d proxy boxes still meet ships and ground"), *GetName(), KinematicVelocity.Size() / 100.0,
		Motion ? Motion : TEXT("at rest"), GeneratedCollisionBoxes.Num());
	return true;
}

void ASpaceship::StartHullMotionWatch()
{
	UWorld* World = GetWorld();
	if (!World || bHullGoingAway || !HasDetailedHullProxy())
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (!Timers.TimerExists(HullMotionTimer))
	{
		HullMotionLastCheckSeconds = World->GetTimeSeconds();
		// Rio 07.10 (fleet audit): ships spawned or ordered together no longer check, take out and restore in one frame.
		Timers.SetTimer(HullMotionTimer, this, &ASpaceship::UpdateHullCollisionByMotion, 0.1f, true, FMath::FRandRange(0.02f, 0.1f));
	}
}

void ASpaceship::UpdateHullCollisionByMotion()
{
	UWorld* World = GetWorld();
	if (!World || bHullGoingAway)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const float Elapsed = FMath::Clamp(static_cast<float>(Now - HullMotionLastCheckSeconds), 0.0f, 1.0f);
	HullMotionLastCheckSeconds = Now;
	// In the seat the proxy follows the pilot (PossessedBy and UnPossessed); so it does for a ship the player flies directly
	// (possessed without a seated pilot): the timer neither builds nor takes out anything under a player's hands.
	if (IsValid(Pilot) || IsPlayerControlled())
	{
		return;
	}
	if (APSShipPerf::CVarKeepHullOutWhileMoving.GetValueOnGameThread() == 0)
	{
		if (bHullRestorePending && bFlightCollisionOptimizationActive && MayRestoreHullBody(true))
		{
			// Switched off live: the body comes back now, as it did when the pilot got up.
			SetFlightCollisionOptimization(false);
		}
		return;
	}
	if (!HasDetailedHullProxy())
	{
		if (bHullRestorePending && bFlightCollisionOptimizationActive && MayRestoreHullBody(true))
		{
			// Rio 06.10 (review): the detailed-hull proxy was switched off live (aps.Ship.DetailedHullFlightProxy 0 or a
			// higher aps.Ship.DetailedHullShapes) while the body waited out here: it comes back now, as it did when the
			// pilot got up before 06.10, instead of staying out for good.
			SetFlightCollisionOptimization(false);
		}
		return;
	}
	const TCHAR* Motion = GetHullMotionReason();
	const float NearM = APSShipPerf::CVarHullRestoreNearM.GetValueOnGameThread();
	if (!bFlightCollisionOptimizationActive)
	{
		UPrimitiveComponent* InSceneHull = GetPrimaryHullComponent();
		if (InSceneHull && InSceneHull->IsRegistered() && !InSceneHull->IsPhysicsStateCreated() && GetActorEnableCollision()
			&& InSceneHull->BodyInstance.GetCollisionEnabled(false) != ECollisionEnabled::NoCollision
			&& IsPlayerOnFootNear(FMath::Max(NearM, 0.0f) * 100.0) && MayRestoreHullBody(true))
		{
			// Rio 07.10 (fleet audit, safety net): neither boxes nor its own body (a path that built nothing, as a restore
			// during the REAL SCALE freeze did): the body is built again before the player walks onto a ship without collision.
			const double BuildStart = FPlatformTime::Seconds();
			InSceneHull->RecreatePhysicsState();
			APSShipPerf::GLastHullBuildWorld = World;
			APSShipPerf::GLastHullBuildEndSeconds = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Warning, TEXT("[APS.Ships] %s had no collision at all: its hull's body is built again (%.1f ms, body %s)"),
				*GetName(), (FPlatformTime::Seconds() - BuildStart) * 1000.0,
				InSceneHull->IsPhysicsStateCreated() ? TEXT("in") : TEXT("OUT"));
			return;
		}
		if (!Motion)
		{
			return;
		}
		if (!InSceneHull || !InSceneHull->IsRegistered() || !InSceneHull->IsPhysicsStateCreated()
			|| !GetActorEnableCollision())
		{
			// Nothing of it in the physics scene to take out (a far ship the REAL SCALE freeze unregistered).
			return;
		}
		const bool bWalkShell = HasWalkShell();
		const bool bWalker = IsWalkerAboard();
		if (!bWalkShell && (bWalker || IsPlayerOnFootNear(0.0)))
		{
			// Without a walk shell a walker aboard or beside it needs the hull's own body (as before).
			return;
		}
		if (!bWalkShell && APSShipPerf::CVarHullRestoreOnFootInside.GetValueOnGameThread() != 0
			&& IsPlayerOnFootNear(FMath::Max(APSShipPerf::CVarHullTakeOutClearM.GetValueOnGameThread(), 0.0f) * 100.0))
		{
			// Rio 07.10 (aps.Ship.HullTakeOutClearM): the body came back because the player walked within its radius; it
			// leaves again only once he is well clear, so walking along the radius does not swap it back and forth.
			return;
		}
		// Moving with nobody in the seat (a fleet unit under orders): the ship flies on its proxy boxes, its hull's
		// shapes leave the scene (each move and each query near it walked them all).
		const double TakeOutStart = FPlatformTime::Seconds();
		SetFlightCollisionOptimization(true);
		if (!bFlightCollisionOptimizationActive)
		{
			return;
		}
		bHullRestorePending = true;
		HullRestSeconds = 0.0f;
		HullPendingSinceSeconds = -1.0e9;
		SetProxyBoxesPassWalkers(bWalkShell && (bWalker || IsPlayerOnFootNear(0.0)));
		UPrimitiveComponent* PrimaryHull = GetPrimaryHullComponent();
		const UBodySetup* HullBodySetup = PrimaryHull ? PrimaryHull->GetBodySetup() : nullptr;
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s moves without a pilot (%s): it flies on %d proxy boxes, its hull's %d shapes ")
			TEXT("%s (%.1f ms, walker %d)"), *GetName(), Motion, GeneratedCollisionBoxes.Num(),
			HullBodySetup ? HullBodySetup->AggGeom.GetElementCount() : 0,
			bHullBodyHeld ? TEXT("are held in the scene, inert") : TEXT("left the scene"),
			(FPlatformTime::Seconds() - TakeOutStart) * 1000.0, bWalker ? 1 : 0);
		return;
	}

	// The body is out (or held) and nobody flies the ship.
	bHullRestorePending = true;
	const bool bWalkShell = HasWalkShell();
	const bool bWalker = IsWalkerAboard();
	if (bWalker && !bWalkShell)
	{
		if (!MayRestoreHullBody(false))
		{
			return;
		}
		// Nothing for the walker to stand on but the hull itself.
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s: a walker aboard and no walk shell: the hull's body comes back"), *GetName());
		SetFlightCollisionOptimization(false);
		return;
	}
	if (!bWalkShell && APSShipPerf::CVarHullRestoreOnFootInside.GetValueOnGameThread() != 0 && IsPlayerOnFootNear(0.0))
	{
		// Review: a fleet coming back around the player rests together; held bodies keep to one a frame here too (the boxes
		// carry the ship until the next 0.1 s check), builds are already spaced.
		if (!MayRestoreHullBody(true) || (bHullBodyHeld && !APSShipPerf::TakeHullRestoreBudget()))
		{
			return;
		}
		// Rio 07.10 (aps.Ship.HullRestoreOnFootInside): the player on foot is within its radius (a fleet unit arrived on top
		// of him, a ship drifts past): boxes wider than the hull would push him away; moving or not, the body comes back.
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s: the player on foot is within its radius (%s): the hull's body comes back"),
			*GetName(), Motion ? Motion : TEXT("at rest"));
		SetFlightCollisionOptimization(false);
		return;
	}
	// The boxes let the player's character through while it is aboard, just got up (APilotingVehicle places and attaches
	// it in that frame) or stands anywhere within the hull's sphere: boxes that block a character standing inside them
	// would push it out of the ship. The walk shell carries it.
	SetProxyBoxesPassWalkers(bWalkShell && (bWalker || Now - HullPendingSinceSeconds < 2.0 || IsPlayerOnFootNear(0.0)));
	if (Motion)
	{
		HullRestSeconds = 0.0f;
		return;
	}
	HullRestSeconds += Elapsed;
	if (HullRestSeconds < FMath::Max(APSShipPerf::CVarHullRestoreRestSeconds.GetValueOnGameThread(), 0.0f))
	{
		return;
	}
	if (bWalker && APSShipPerf::CVarHullRestoreWithWalker.GetValueOnGameThread() == 0)
	{
		// Rio's choice pending (the plan's decision 2): the build waits until the walker steps off.
		return;
	}
	// Rio 07.10 (aps.Ship.HullHold): a held body comes back with one teleport, so a rested ship takes it at once (no proxy
	// boxes stay on a parked fleet ship for the player's ship to bump into); the wait for an on-foot player is for a build.
	if (!bHullBodyHeld && !bWalker && NearM > 0.0f && !IsPlayerOnFootNear(NearM * 100.0))
	{
		// Nobody walks near it: it stays on its boxes (ships and ground still meet them), no build for nothing.
		return;
	}
	if (!MayRestoreHullBody(!bWalker))
	{
		return;
	}
	// Rio 07.10 (aps.Ship.HullRestoresPerFrame): a fleet back from one order rests together; one hull a frame.
	if (!bWalker && !APSShipPerf::TakeHullRestoreBudget())
	{
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s rested %.1f s (%s): the hull's body comes back"), *GetName(), HullRestSeconds,
		bWalker ? TEXT("walker aboard") : bHullBodyHeld ? TEXT("held body") : NearM > 0.0f ? TEXT("walker near") : TEXT("nobody aboard"));
	SetFlightCollisionOptimization(false);
}

FVector ASpaceship::GetControlledFlightAcceleration(const FVector& WorldInput,
	const FVector& CurrentVelocity, double RequestedAcceleration, double MaximumSpeed) const
{
	if (WorldInput.IsNearlyZero() || RequestedAcceleration <= UE_DOUBLE_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const FVector RawAcceleration = WorldInput.GetClampedToMaxSize(1.0) * RequestedAcceleration;
	const double CurrentSpeed = CurrentVelocity.Size();
	const double DriveReferenceScale = FMath::Clamp(
		GetDriveSpeedScale(SelectedDriveMode), 1.0, 64.0);
	const double ReferenceSpeed = FMath::Max(
		CurrentSpeed, ActiveClassPreset.MaxImpulseSpeed * DriveReferenceScale);
	const double MaximumParallelAcceleration = ReferenceSpeed
		/ FMath::Max(static_cast<double>(HighSpeedVelocityResponseTime), 0.1);

	if (CurrentSpeed <= 1.0)
	{
		return RawAcceleration.GetClampedToMaxSize(MaximumParallelAcceleration);
	}

	const FVector VelocityDirection = CurrentVelocity / CurrentSpeed;
	double ParallelAcceleration = FVector::DotProduct(RawAcceleration, VelocityDirection);
	FVector LateralAcceleration = RawAcceleration - VelocityDirection * ParallelAcceleration;
	ParallelAcceleration = FMath::Clamp(
		ParallelAcceleration, -MaximumParallelAcceleration, MaximumParallelAcceleration);
	if (CurrentSpeed >= MaximumSpeed && ParallelAcceleration > 0.0)
	{
		ParallelAcceleration = 0.0;
	}

	const double MaximumHeadingRateRadians = FMath::DegreesToRadians(FMath::Max(
		ActiveClassPreset.RotationSpeed * SteeringRateScale * HighSpeedHeadingResponseScale, 1.0));
	const double MaximumLateralAcceleration = ReferenceSpeed * MaximumHeadingRateRadians;
	LateralAcceleration = LateralAcceleration.GetClampedToMaxSize(MaximumLateralAcceleration);
	return VelocityDirection * ParallelAcceleration + LateralAcceleration;
}

bool ASpaceship::MoveShipKinematic(const FVector& Delta, bool bSweep, FHitResult& OutHit)
{
	OutHit = FHitResult();
	if (Delta.IsNearlyZero())
	{
		return false;
	}
	// Rio 05.10 (REAL SCALE: at billions of c the hull, the camera and the pilot walking aboard jumped by metres every
	// frame, "the Deep Space Kraken"): an unswept step of the player's fast ship goes into the world shift by whole
	// grains (KSP's Krakensbane); the ship and its riders move by the small rest only. Same speed, same flight.
	// Rio 05.10 night: every unswept step goes this way (the plain offset below, without a sweep, is the same move), so a
	// long step the view rides between flows gets its riders' previous transforms too (FlowPastShip, "flow ride").
	if (!bSweep)
	{
		UWorld* World = GetWorld();
		if (UAPSWorldOriginSubsystem* Origin = World ? World->GetSubsystem<UAPSWorldOriginSubsystem>() : nullptr)
		{
			const FVector Rest = Origin->FlowPastShip(*this, Delta, KinematicVelocity.Size());
			if (Rest != Delta)
			{
				// Rio 06.10 (collision by motion): this ship carries the world's flow or owes its step; it moves.
				HullLastFlowSeconds = World->GetTimeSeconds();
			}
			AddActorWorldOffset(Rest, false, nullptr, ETeleportType::None);
			Origin->FinishFlowMove(*this);
			return false;
		}
	}
	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
	if (bSweep && RootPrimitive && !RootPrimitive->IsQueryCollisionEnabled() && !GeneratedCollisionBoxes.IsEmpty()
		&& APSShipPerf::CVarProxySweep.GetValueOnGameThread() != 0)
	{
		// Detailed hulls fly on child proxy boxes with a collision-free root (SetFlightCollisionOptimization);
		// the root sweep below would never hit anything, so a piloted M3 passed through stations and ground.
		return MoveShipWithProxySweep(Delta, OutHit);
	}
	if (bSweep && (!RootPrimitive || !RootPrimitive->IsQueryCollisionEnabled()))
	{
		// MoveComponent sweeps only the root body; without query collision the move is a plain offset anyway.
		bSweep = false;
	}
	// The body sweep queries every convex hull of the root separately (12-32 on generated hulls) and its
	// cost grows with the swept length. A sphere around the body's bounding box encloses all of them, so a
	// clear sphere path with the same channel, responses and ignore lists proves the body sweep clear too.
	const auto SphereMayHit = [this, RootPrimitive, &Delta]()
	{
		// Rio 09.10 (aps.Ship.HullBodySyncOnce): this tick's turn may not be in the body yet; the bounds it will have.
		const UAPSShipHullComponent* SyncedHull = Cast<UAPSShipHullComponent>(RootPrimitive);
		const FBox BodyBox = SyncedHull && SyncedHull->HasPendingBodySync() ? SyncedHull->GetPendingBodyBounds()
			: RootPrimitive->BodyInstance.GetBodyBounds();
		const FVector SphereCenter = BodyBox.IsValid ? BodyBox.GetCenter() : RootPrimitive->Bounds.Origin;
		const double SphereRadius = BodyBox.IsValid ? BodyBox.GetExtent().Size() : RootPrimitive->Bounds.BoxExtent.Size();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipSweepPrecheck), RootPrimitive->bTraceComplexOnMove, this);
		Params.AddIgnoredActors(RootPrimitive->GetMoveIgnoreActors());
		Params.AddIgnoredComponents(RootPrimitive->GetMoveIgnoreComponents());
		if (IsValid(Pilot))
		{
			Params.AddIgnoredActor(Pilot);
		}
		return GetWorld()->SweepTestByChannel(SphereCenter, SphereCenter + Delta, FQuat::Identity,
			RootPrimitive->GetCollisionObjectType(), FCollisionShape::MakeSphere(SphereRadius), Params,
			FCollisionResponseParams(RootPrimitive->GetCollisionResponseToChannels()));
	};
	const bool bPrecheck = bSweep && RootPrimitive && APSShipPerf::CVarSweepPrecheck.GetValueOnGameThread() != 0 && GetWorld();
	// Rio 06.10 (perf R2, walking aboard near the ground: 112-139 ms frames): the M5 hull sweep walked the hull's 13.5k
	// shapes on every swept move, even with nothing near. The sphere goes first now; a clear path is a plain offset, the
	// M5 sweep runs only where the sphere may hit (aps.Ship.SweepPrecheckFirst).
	const bool bPrecheckFirst = bPrecheck && APSShipPerf::CVarSweepPrecheckFirst.GetValueOnGameThread() != 0;
	if (bPrecheckFirst)
	{
		bSweep = SphereMayHit();
	}
	if (bSweep && RootPrimitive)
	{
		if (UAPSM5HullSweepComponent* FittedSweep = FindComponentByClass<UAPSM5HullSweepComponent>())
		{
			if (FittedSweep->TryMoveHull(Cast<UStaticMeshComponent>(RootPrimitive), Delta, OutHit)) return OutHit.bBlockingHit;
		}
	}
	if (bSweep && bPrecheck && !bPrecheckFirst)
	{
		bSweep = SphereMayHit();
	}
	AddActorWorldOffset(Delta, bSweep, &OutHit, ETeleportType::None);
	return OutHit.bBlockingHit;
}

bool ASpaceship::MoveShipWithProxySweep(const FVector& Delta, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	TArray<UBoxComponent*, TInlineAllocator<9>> Boxes;
	FBox ProxyBounds(ForceInit);
	for (UBoxComponent* Box : GeneratedCollisionBoxes)
	{
		if (IsValid(Box) && Box->IsQueryCollisionEnabled())
		{
			Boxes.Add(Box);
			ProxyBounds += Box->Bounds.GetBox();
		}
	}
	if (!World || Boxes.IsEmpty() || !ProxyBounds.IsValid)
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::None);
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipProxySweep), false, this);
	TArray<AActor*> Attached;
	GetAttachedActors(Attached, false, true);
	Params.AddIgnoredActors(Attached);
	if (IsValid(Pilot))
	{
		Params.AddIgnoredActor(Pilot);
	}
	const ECollisionChannel Channel = Boxes[0]->GetCollisionObjectType();
	const FCollisionResponseParams Responses(Boxes[0]->GetCollisionResponseToChannels());

	// Nearly every frame of flight is clear: one sphere around all boxes proves that cheaply.
	const FVector Center = ProxyBounds.GetCenter();
	if (!World->SweepTestByChannel(Center, Center + Delta, FQuat::Identity, Channel,
		FCollisionShape::MakeSphere(ProxyBounds.GetExtent().Size()), Params, Responses))
	{
		AddActorWorldOffset(Delta, false, nullptr, ETeleportType::None);
		return false;
	}
	// Rio 07.10 (aps.Ship.ProxyUnstick): which way is up here, and whether this move climbs.
	const bool bUnstick = APSShipPerf::CVarProxyUnstick.GetValueOnGameThread() != 0;
	const FVector Up = -ActiveGravityDirection.GetSafeNormal();
	const double UnstickDepthCm = FMath::Max(APSShipPerf::CVarProxyUnstickDepthCm.GetValueOnGameThread(), 1.0f);
	// Only a planet's or a moon's pull says where "up" is for climbing out (a station's or a ship's gravity volume may
	// point anywhere, and it holds the previous direction for a moment after a change of source).
	const bool bPlanetUp = bUnstick && !Up.IsNearlyZero() && ActiveGravitySource && ActiveGravitySource->IsA<APlanetaryBody>();
	bool bClimbedOut = false;
	FHitResult ClimbedOutHit;
	TArray<FHitResult> Hits;
	// bSlidePass: the move was built tangent to an overlap (Slide . N == 0 up to rounding), which must not read as "into".
	const auto SweepBoxes = [&](const FVector& Move, FHitResult& OutEarliest, const bool bSlidePass) -> bool
	{
		const bool bClimbing = bPlanetUp && FVector::DotProduct(Move.GetSafeNormal(), Up) >= 0.5;
		const double LeaveTolerance = bSlidePass ? -1.0e-3 * Move.Size() : 0.0;
		bool bAnyBlock = false;
		const auto Consider = [&](const FHitResult& Hit)
		{
			if (!bAnyBlock || Hit.Time < OutEarliest.Time)
			{
				OutEarliest = Hit;
				bAnyBlock = true;
			}
		};
		for (const UBoxComponent* Box : Boxes)
		{
			const FVector Start = Box->GetComponentLocation();
			const FCollisionShape Shape = FCollisionShape::MakeBox(Box->GetScaledBoxExtent());
			FCollisionQueryParams BoxParams = Params;
			for (int32 Pass = 0; Pass < 5; ++Pass)
			{
				Hits.Reset();
				World->SweepMultiByChannel(Hits, Start, Start + Move, Box->GetComponentQuat(), Channel, Shape, BoxParams, Responses);
				const FHitResult* ClimbOut = nullptr;
				const FHitResult* BoxBlock = nullptr;
				for (const FHitResult& Hit : Hits)
				{
					// A contact the ship is already leaving (parked on a deck, grazing a wall) must not hold it.
					if (!Hit.bBlockingHit || (Hit.bStartPenetrating && FVector::DotProduct(Move, Hit.ImpactNormal) >= LeaveTolerance))
					{
						continue;
					}
					// Rio 07.10 (aps.Ship.ProxyUnstick): sunk deep into the ground or a pad UNDER the ship (never a ceiling over
					// it, never another ship or a pawn) and lifting off at 30 deg or more above the horizon: it leaves what it
					// is inside. A box inside something sees nothing beyond it (the engine cuts that sweep to zero length and
					// keeps one blocking hit), so the box is swept again without it: whatever is ahead (a ship, a structure
					// over the pad) still holds it, and the ground under the pad is the next layer to leave.
					const AActor* HitActor = Hit.GetActor();
					if (bClimbing && Hit.bStartPenetrating && Hit.PenetrationDepth >= UnstickDepthCm && Hit.GetComponent()
						&& !(HitActor && HitActor->IsA<APawn>()) && !Hit.ImpactPoint.Equals(Start, 1.0)
						&& FVector::DotProduct(Hit.ImpactPoint - ProxyBounds.GetCenter(), Up) < 0.0)
					{
						ClimbOut = &Hit;
						break;
					}
					if (!BoxBlock || Hit.Time < BoxBlock->Time)
					{
						BoxBlock = &Hit;
					}
				}
				if (ClimbOut && Pass < 4)
				{
					BoxParams.AddIgnoredComponent(ClimbOut->GetComponent());
					ClimbedOutHit = *ClimbOut;
					bClimbedOut = true;
					continue;
				}
				if (BoxBlock)
				{
					Consider(*BoxBlock);
				}
				else if (ClimbOut)
				{
					// More layers than that: held, as before.
					Consider(*ClimbOut);
				}
				break;
			}
		}
		return bAnyBlock;
	};
	const auto LogClimbOut = [&](const double MovedCm)
	{
		const double Now = FPlatformTime::Seconds();
		if (bClimbedOut && MovedCm > 0.0 && Now - ProxyUnstickLogSeconds >= 1.0)
		{
			ProxyUnstickLogSeconds = Now;
			UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s climbs out of %s.%s (%.0f cm inside it, moved %.0f cm; aps.Ship.ProxyUnstick)"),
				*GetName(), *GetNameSafe(ClimbedOutHit.GetActor()), *GetNameSafe(ClimbedOutHit.GetComponent()),
				ClimbedOutHit.PenetrationDepth, MovedCm);
		}
	};
	FHitResult Earliest;
	bool bBlocked = SweepBoxes(Delta, Earliest, false);
	FVector Move = Delta;
	const AActor* EarliestActor = bBlocked ? Earliest.GetActor() : nullptr;
	if (bBlocked && bUnstick && Earliest.bStartPenetrating && !(EarliestActor && EarliestActor->IsA<APawn>()))
	{
		// Rio 07.10 (aps.Ship.ProxyUnstick): held by something it is already inside (not a ship or a pawn): slide along it (the
		// part of the move into it removed, never deeper), once. A clear slide moves; a slide that meets something ahead stops
		// there; a slide held by an overlap again stays put, as before.
		const FVector Normal = Earliest.ImpactNormal.GetSafeNormal();
		const FVector Slide = Delta - Normal * FMath::Min(FVector::DotProduct(Delta, Normal), 0.0);
		FHitResult SlideHit;
		if (!Normal.IsNearlyZero() && !Slide.IsNearlyZero(0.01))
		{
			const bool bSlideBlocked = SweepBoxes(Slide, SlideHit, true);
			if (!bSlideBlocked || !SlideHit.bStartPenetrating)
			{
				// The velocity into what it is inside goes as for any contact (the caller only sees the hit it gets back).
				KinematicVelocity -= Normal * FMath::Min(FVector::DotProduct(KinematicVelocity, Normal), 0.0);
				Move = Slide;
				if (bSlideBlocked)
				{
					Earliest = SlideHit;
				}
				else
				{
					AddActorWorldOffset(Move, false, nullptr, ETeleportType::None);
					LogClimbOut(Move.Size());
					OutHit = Earliest;
					return true;
				}
			}
		}
	}
	// Rio 08.10 (0.6.0-alpha): an L flagship spawned inside the HQ could not leave in orbit (the climb-out above needs a
	// planet's "up", and the slide meets the HQ again). Still held by a structure it is already inside (an HQ, a station, a
	// shipyard, a colony; never a ship or a pawn): this move ignores those parts, as ground vehicles do when stuck.
	if (bBlocked && bUnstick && Earliest.bStartPenetrating)
	{
		for (int32 Pass = 0; Pass < 4 && bBlocked && Earliest.bStartPenetrating; ++Pass)
		{
			// Truly inside (a resting or grazing contact still blocks, so a pilot pressing onto a deck never sinks through
			// it); a structure or a part of one (the HQ's parked M3 is its child actor), never a ship or a pawn of its own.
			const AActor* Held = Earliest.GetActor();
			const bool bStructure = Held && (Held->IsA<ATechActor>()
				|| (Held->GetParentActor() && Held->GetParentActor()->IsA<ATechActor>()));
			if (!bStructure || Held->IsA<APawn>() || !Earliest.GetComponent() || Earliest.PenetrationDepth < UnstickDepthCm)
			{
				break;
			}
			Params.AddIgnoredComponent(Earliest.GetComponent());
			ClimbedOutHit = Earliest;
			bClimbedOut = true;
			Move = Delta;
			bBlocked = SweepBoxes(Move, Earliest, false);
		}
	}
	if (!bBlocked)
	{
		AddActorWorldOffset(Move, false, nullptr, ETeleportType::None);
		LogClimbOut(Move.Size());
		return false;
	}
	// Up to the contact with a centimetre to spare; the caller removes the velocity into the surface.
	const double Travel = FMath::Max(Earliest.Time * Move.Size() - 1.0, 0.0);
	if (Travel > 0.0)
	{
		AddActorWorldOffset(Move.GetSafeNormal() * Travel, false, nullptr, ETeleportType::None);
	}
	LogClimbOut(Travel);
	OutHit = Earliest;
	return true;
}

void ASpaceship::ApplyFlightInput(float DeltaTime)
{
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const FVector LocalInput(ForwardInput, SideInput, VerticalInput);
	const FVector ClampedInput = LocalInput.GetClampedToMaxSize(1.0);
	const FVector WorldInput = GetShipForwardVector() * ClampedInput.X
		+ GetShipRightVector() * ClampedInput.Y
		+ GetShipUpVector() * ClampedInput.Z;
	const EEngineMode EngineMode = OnboardComputer->EngineSystem.CurrentEngineMode;
	const double SpeedScale = GetDriveSpeedScale(SelectedDriveMode) * GetEngineSpeedMultiplier(EngineMode);
	const double AccelerationScale = GetDriveAccelerationScale(SelectedDriveMode)
		* GetEngineAccelerationMultiplier(EngineMode);
	const double DriveAcceleration = FMath::Max(
		ActiveClassPreset.ImpulseAcceleration * AccelerationScale,
		GetMinimumDriveAcceleration(SelectedDriveMode) * GetEngineAccelerationMultiplier(EngineMode));
	const double TransitionAlpha = GetEngineTransitionAuthority();
	const double RequestedAcceleration = DriveAcceleration * CurrentBoostMultiplier * TransitionAlpha;
	const double MaxSpeed = ActiveClassPreset.MaxImpulseSpeed * SpeedScale * CurrentBoostMultiplier;

	if (EngineMode == EEngineMode::Impulse && ActiveClassPreset.bUsesPhysicalImpulse
		&& SpaceshipHull->IsSimulatingPhysics())
	{
		const FVector VelocityBeforeForces = SpaceshipHull->GetPhysicsLinearVelocity();
		const double SpeedBeforeForces = VelocityBeforeForces.Size();
		const FVector ControlledAcceleration = GetControlledFlightAcceleration(
			WorldInput, VelocityBeforeForces, RequestedAcceleration, MaxSpeed);
		if (!ControlledAcceleration.IsNearlyZero())
		{
			SpaceshipHull->AddForce(ControlledAcceleration, NAME_None, true);
		}
		ApplyEnvironmentForces(DeltaTime);

		FVector Velocity = SpaceshipHull->GetPhysicsLinearVelocity();
		const double SpeedAfterForces = Velocity.Size();
		if (SpeedAfterForces > MaxSpeed && SpeedAfterForces > SpeedBeforeForces)
		{
			// Boost and mode changes alter available thrust, not existing momentum.
			// Clamp only newly added overspeed so releasing Shift cannot delete most
			// of the ship velocity in a single frame.
			Velocity = Velocity.GetClampedToMaxSize(FMath::Max(MaxSpeed, SpeedBeforeForces));
			SpaceshipHull->SetPhysicsLinearVelocity(Velocity);
		}
		if (bIsDecelerating)
		{
			SpaceshipHull->SetPhysicsLinearVelocity(FMath::VInterpTo(
				Velocity, FVector::ZeroVector, DeltaTime, BrakingResponseSpeed));
		}
		return;
	}

	const double SpeedBeforeAcceleration = KinematicVelocity.Size();
	if (!WorldInput.IsNearlyZero())
	{
		KinematicVelocity += GetControlledFlightAcceleration(
			WorldInput, KinematicVelocity, RequestedAcceleration, MaxSpeed) * DeltaTime;
	}
	else
	{
		const double EnvironmentalDrag = GetEnvironmentDrag();
		if (EnvironmentalDrag > UE_SMALL_NUMBER)
		{
			KinematicVelocity = FMath::VInterpTo(
				KinematicVelocity, FVector::ZeroVector, DeltaTime, EnvironmentalDrag);
		}
	}
	ApplyEnvironmentForces(DeltaTime);
	const double SpeedAfterAcceleration = KinematicVelocity.Size();
	if (SpeedAfterAcceleration > MaxSpeed && SpeedAfterAcceleration > SpeedBeforeAcceleration)
	{
		KinematicVelocity = KinematicVelocity.GetClampedToMaxSize(
			FMath::Max(MaxSpeed, SpeedBeforeAcceleration));
	}
	if (bIsDecelerating)
	{
		KinematicVelocity = FMath::VInterpTo(
			KinematicVelocity, FVector::ZeroVector, DeltaTime, BrakingResponseSpeed);
	}

	FHitResult Hit;
	const bool bSweep = EngineMode == EEngineMode::Impulse;
	MoveShipKinematic(KinematicVelocity * DeltaTime, bSweep, Hit);
	if (Hit.bBlockingHit)
	{
		// A grazing contact must not consume the ship's whole impulse. Multiplying the
		// projected velocity by 0.25 made a held thrust repeatedly accelerate the ship
		// and then discard 75% of its speed on the next sweep, which was perceived as
		// a regular stop/go pulse even while flying straight. Remove only the velocity
		// directed into the obstacle and preserve both tangential and separating motion.
		const FVector SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
		const double VelocityIntoSurface = FVector::DotProduct(KinematicVelocity, SurfaceNormal);
		if (!SurfaceNormal.IsNearlyZero() && VelocityIntoSurface < 0.0)
		{
			KinematicVelocity -= SurfaceNormal * VelocityIntoSurface;
		}
	}
}

void ASpaceship::ApplyRotationInput(float DeltaTime)
{
	if (!bEngineRunning || !SpaceshipHull || IsGroundVehicle())
	{
		// Rio 02.10: a ground vehicle turns in its flight model step (steering on the ground, the drone's yaw and pitch).
		CurrentAngularVelocityDegrees = FVector::ZeroVector;
		return;
	}

	const float TransitionAlpha = GetEngineTransitionAuthority();
	// Rio 02.10: the flight model sets the steering feel. The star drive steers like a yoke with a lag, and the hull is
	// calmer in the air. It also smooths the uneven mouse deltas there. Elsewhere both are neutral.
	double RateScale = 1.0;
	double ResponseScale = 1.0;
	double DampingScale = 1.0;
	FVector Steering(PitchInput, YawInput, RollInput);
	// Rio 03.10 ("why do A and D not turn the ship left and right, as the mouse does?"): with the mouse on the camera (C)
	// the keys turn the hull at the full rate instead of strafing (the flight model drops the strafe then).
	if (IsMouseLookActive())
	{
		Steering.Y = FMath::Clamp(static_cast<double>(SideInput), -1.0, 1.0)
			* FMath::Max(static_cast<double>(SteeringInputLimit), 0.05);
	}
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->GetSteeringFeel(RateScale, ResponseScale, DampingScale);
		Steering = FlightModel->SmoothSteeringInput(Steering, DeltaTime);
	}
	const double RotationSpeed = ActiveClassPreset.RotationSpeed * SteeringRateScale * TransitionAlpha * RateScale;
	const double SafeInputLimit = FMath::Max(static_cast<double>(SteeringInputLimit), 0.05);
	const FVector DesiredAngularVelocityDegrees(
		FMath::Clamp(Steering.X / SafeInputLimit, -1.0, 1.0) * RotationSpeed,
		FMath::Clamp(Steering.Y / SafeInputLimit, -1.0, 1.0) * RotationSpeed,
		FMath::Clamp(Steering.Z / SafeInputLimit, -1.0, 1.0) * RotationSpeed);
	const bool bHasRotationInput = !DesiredAngularVelocityDegrees.IsNearlyZero(0.001);

	const bool bUsePhysicalRotation = OnboardComputer
		&& OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse
		&& ActiveClassPreset.bUsesPhysicalImpulse
		&& SpaceshipHull->IsSimulatingPhysics();
	if (bUsePhysicalRotation)
	{
		const FVector ShipRight = GetShipRightVector();
		const FVector ShipUp = GetShipUpVector();
		const FVector ShipForward = GetShipForwardVector();
		const FVector WorldAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
		const FVector MeasuredAngularVelocityDegrees(
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipRight)),
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipUp)),
			FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, ShipForward)));
		if (!bHasRotationInput)
		{
			CurrentAngularVelocityDegrees = MeasuredAngularVelocityDegrees;
			return;
		}
		const FVector CommandedAngularVelocityDegrees = FMath::VInterpConstantTo(
			MeasuredAngularVelocityDegrees,
			DesiredAngularVelocityDegrees,
			DeltaTime,
			ActiveClassPreset.AngularAcceleration * TransitionAlpha);

		if (DeltaTime > UE_SMALL_NUMBER)
		{
			const FVector LocalAngularAccelerationDegrees =
				(CommandedAngularVelocityDegrees - MeasuredAngularVelocityDegrees) / DeltaTime;
			const FVector WorldAngularAccelerationRadians =
				ShipRight * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.X)
				+ ShipUp * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.Y)
				+ ShipForward * FMath::DegreesToRadians(LocalAngularAccelerationDegrees.Z);
			SpaceshipHull->AddTorqueInRadians(WorldAngularAccelerationRadians, NAME_None, true);
		}
		CurrentAngularVelocityDegrees = CommandedAngularVelocityDegrees;
		return;
	}

	CurrentAngularVelocityDegrees = bHasRotationInput
		? FMath::VInterpConstantTo(
			CurrentAngularVelocityDegrees,
			DesiredAngularVelocityDegrees,
			DeltaTime,
			ActiveClassPreset.AngularAcceleration * TransitionAlpha * ResponseScale)
		: FMath::VInterpTo(
			CurrentAngularVelocityDegrees, FVector::ZeroVector, DeltaTime, PassiveAngularDamping * DampingScale);

	const double PitchRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X * DeltaTime);
	const double YawRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y * DeltaTime);
	const double RollRadians = FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z * DeltaTime);
	if (FMath::IsNearlyZero(YawRadians) && FMath::IsNearlyZero(PitchRadians) && FMath::IsNearlyZero(RollRadians))
	{
		return;
	}

	const FQuat Current = GetActorQuat();
	const FQuat DeltaYaw(GetShipUpVector(), YawRadians);
	const FQuat DeltaPitch(GetShipRightVector(), PitchRadians);
	const FQuat DeltaRoll(GetShipForwardVector(), RollRadians);
	const FQuat Target = (DeltaRoll * DeltaPitch * DeltaYaw * Current).GetNormalized();
	SetActorRotation(Target, ETeleportType::None);
}

float ASpaceship::GetEngineTransitionAuthority() const
{
	if (!bEngineModeTransitionActive || EngineModeTransitionDuration <= UE_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const float LinearAlpha = FMath::Clamp(
		EngineModeTransitionElapsed / EngineModeTransitionDuration, 0.0f, 1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
	return 1.0f - FMath::Sin(PI * SmoothAlpha) * 0.18f;
}

EEngineMode ASpaceship::ResolveEngineModeForFlightMode(EFlightMode FlightMode) const
{
	if (static_cast<uint8>(FlightMode) >= static_cast<uint8>(EFlightMode::Interstellar)
		&& ActiveClassPreset.bSupportsOffset)
	{
		return EEngineMode::Offset;
	}
	if (static_cast<uint8>(FlightMode) >= static_cast<uint8>(EFlightMode::Interplanetary)
		&& ActiveClassPreset.bSupportsSpaceWrap)
	{
		return EEngineMode::SpaceWrap;
	}
	return EEngineMode::Impulse;
}

double ASpaceship::GetFlightModeSpeedScale(EFlightMode FlightMode) const
{
	switch (FlightMode)
	{
	case EFlightMode::Surface: return 1.25;
	case EFlightMode::Atmospheric: return 2.0;
	case EFlightMode::Orbital: return 4.0;
	case EFlightMode::Planetary: return 8.0;
	case EFlightMode::Interplanetary: return 200.0;
	case EFlightMode::Stellar: return 1000.0;
	case EFlightMode::Interstellar: return 10000.0;
	case EFlightMode::Intergalaxy: return 100000.0;
	default: return 1.0;
	}
}

void ASpaceship::RequestEngineModeForFlightMode(bool bImmediate)
{
	if (!OnboardComputer)
	{
		return;
	}
	PendingEngineMode = ResolveEngineModeForDriveMode(SelectedDriveMode);
	if (OnboardComputer->EngineSystem.CurrentEngineMode == PendingEngineMode)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		ApplyEngineState();
		return;
	}
	if (bImmediate || !bEngineRunning)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		CommitEngineModeSwitch(PendingEngineMode);
		return;
	}

	EngineModeTransitionElapsed = 0.0f;
	bEngineModeTransitionActive = true;
	bEngineModeSwitchedAtMidpoint = false;
}

void ASpaceship::CommitEngineModeSwitch(EEngineMode NewEngineMode)
{
	if (!OnboardComputer || !SpaceshipHull)
	{
		return;
	}

	const EEngineMode PreviousMode = OnboardComputer->EngineSystem.CurrentEngineMode;
	const FVector PreservedLinearVelocity = SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity() : KinematicVelocity;
	FVector PreservedAngularVelocityRadians = FVector::ZeroVector;
	if (SpaceshipHull->IsSimulatingPhysics())
	{
		PreservedAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
	}
	else
	{
		PreservedAngularVelocityRadians =
			GetShipRightVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X)
			+ GetShipUpVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y)
			+ GetShipForwardVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z);
	}

	// The onboard computer changes the simulation backend. Capture momentum before
	// that happens, then explicitly seed whichever backend owns the next frame.
	KinematicVelocity = PreservedLinearVelocity;
	OnboardComputer->SwitchEngineMode(NewEngineMode);
	ApplyEngineState();

	if (SpaceshipHull->IsSimulatingPhysics())
	{
		SpaceshipHull->SetPhysicsLinearVelocity(PreservedLinearVelocity);
		SpaceshipHull->SetPhysicsAngularVelocityInRadians(PreservedAngularVelocityRadians);
	}
	else
	{
		KinematicVelocity = PreservedLinearVelocity;
		CurrentAngularVelocityDegrees = FVector(
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipRightVector())),
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipUpVector())),
			FMath::RadiansToDegrees(FVector::DotProduct(PreservedAngularVelocityRadians, GetShipForwardVector())));
	}

	UE_LOG(LogTemp, Log,
		TEXT("[APS.Ships] Engine handoff ship=%s from=%s to=%s speedBefore=%.2f m/s speedAfter=%.2f m/s physics=%s"),
		*GetName(), *UEnum::GetValueAsString(PreviousMode), *UEnum::GetValueAsString(NewEngineMode),
		PreservedLinearVelocity.Size() / 100.0, GetShipSpeedMetersPerSecond(),
		SpaceshipHull->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));
}

void ASpaceship::AdvanceEngineModeTransition(float DeltaTime)
{
	if (!bEngineModeTransitionActive || !OnboardComputer)
	{
		return;
	}
	EngineModeTransitionElapsed += DeltaTime;
	if (!bEngineModeSwitchedAtMidpoint && EngineModeTransitionElapsed >= EngineModeTransitionDuration * 0.5f)
	{
		CommitEngineModeSwitch(PendingEngineMode);
		bEngineModeSwitchedAtMidpoint = true;
	}
	if (EngineModeTransitionElapsed >= EngineModeTransitionDuration)
	{
		bEngineModeTransitionActive = false;
		bEngineModeSwitchedAtMidpoint = false;
		EngineModeTransitionElapsed = 0.0f;
	}
}

void ASpaceship::ApplyEngineState()
{
	if (!SpaceshipHull || !OnboardComputer)
	{
		return;
	}

	SpaceshipHull->SetMobility(EComponentMobility::Movable);
	SpaceshipHull->SetEnableGravity(false);
	if (!bEngineRunning)
	{
		if (SpaceshipHull->IsSimulatingPhysics())
		{
			KinematicVelocity = SpaceshipHull->GetPhysicsLinearVelocity();
			SpaceshipHull->SetPhysicsLinearVelocity(FVector::ZeroVector);
			SpaceshipHull->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		}
		SpaceshipHull->SetSimulatePhysics(false);
		if (!IsGroundVehicle())
		{
			// A ground vehicle switched off (G) keeps rolling, settling or sinking to a stop in its flight model.
			KinematicVelocity = FVector::ZeroVector;
		}
		CurrentAngularVelocityDegrees = FVector::ZeroVector;
		return;
	}

	const bool bUsePhysics = OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse
		&& ActiveClassPreset.bUsesPhysicalImpulse;
	if (bUsePhysics)
	{
		SpaceshipHull->SetSimulatePhysics(true);
		SpaceshipHull->SetLinearDamping(GetEnvironmentDrag());
		SpaceshipHull->SetAngularDamping(PassiveAngularDamping);
		if (!KinematicVelocity.IsNearlyZero())
		{
			SpaceshipHull->SetPhysicsLinearVelocity(KinematicVelocity);
		}
		if (!CurrentAngularVelocityDegrees.IsNearlyZero())
		{
			const FVector WorldAngularVelocityRadians =
				GetShipRightVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.X)
				+ GetShipUpVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Y)
				+ GetShipForwardVector() * FMath::DegreesToRadians(CurrentAngularVelocityDegrees.Z);
			SpaceshipHull->SetPhysicsAngularVelocityInRadians(WorldAngularVelocityRadians);
		}
	}
	else
	{
		if (SpaceshipHull->IsSimulatingPhysics())
		{
			KinematicVelocity = SpaceshipHull->GetPhysicsLinearVelocity();
			const FVector WorldAngularVelocityRadians = SpaceshipHull->GetPhysicsAngularVelocityInRadians();
			CurrentAngularVelocityDegrees = FVector(
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipRightVector())),
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipUpVector())),
				FMath::RadiansToDegrees(FVector::DotProduct(WorldAngularVelocityRadians, GetShipForwardVector())));
		}
		SpaceshipHull->SetSimulatePhysics(false);
	}
}

FString ASpaceship::GetSizeClassName() const
{
	if (const UEnum* Enum = StaticEnum<ESpaceshipSizeClass>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(SizeClass)).ToString();
	}
	return TEXT("M");
}

FString ASpaceship::GetFlightModeName() const
{
	return GetDriveModeName();
}

FString ASpaceship::GetDriveModeName() const
{
	if (SelectedDriveMode == EShipDriveMode::Orbital
		&& CurrentFlightEnvironment == EShipFlightEnvironment::Atmosphere)
	{
		return TEXT("EXIT ATMOSPHERE");
	}

	const int32 PowerIndex = FMath::Clamp(static_cast<int32>(SelectedDriveMode), 0, 5);
	static const TCHAR* ImpulseLabels[] = {
		TEXT("LANDING"), TEXT("LOCAL"), TEXT("ORBITAL"), TEXT("PLANETARY TRANSFER"),
		TEXT("INTERPLANETARY"), TEXT("RAPID INTERPLANETARY")
	};
	static const TCHAR* SpaceWrapLabels[] = {
		TEXT("PLANETARY ENTRY"), TEXT("LOCAL WARP"), TEXT("SYSTEM TRANSFER"), TEXT("SYSTEM CRUISE"),
		TEXT("STELLAR APPROACH"), TEXT("STELLAR TRANSFER")
	};
	static const TCHAR* OffsetLabels[] = {
		TEXT("LOCAL OFFSET"), TEXT("PLANETARY OFFSET"), TEXT("SYSTEM OFFSET"), TEXT("STELLAR OFFSET"),
		TEXT("INTERSTELLAR OFFSET"), TEXT("DEEP OFFSET")
	};
	if (SelectedEngineMode == EEngineMode::SpaceWrap)
	{
		return SpaceWrapLabels[PowerIndex];
	}
	if (SelectedEngineMode == EEngineMode::Offset)
	{
		return OffsetLabels[PowerIndex];
	}
	return ImpulseLabels[PowerIndex];
}

void ASpaceship::ToggleAutopilot()
{
	if (!FlightModel)
	{
		return;
	}
	if (FlightModel->IsAutopilotEngaged())
	{
		FlightModel->DisengageAutopilot(TEXT("Z"));
		return;
	}
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetSelectedContact() : nullptr;
	if (Contact && Contact->Actor.IsValid())
	{
		FlightModel->EngageAutopilot(Contact->Actor.Get());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Autopilot] no target: pick one with T or on the map"));
	}
}

FString ASpaceship::GetFlightEnvironmentName() const
{
	if (const UEnum* Enum = StaticEnum<EShipFlightEnvironment>())
	{
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(CurrentFlightEnvironment)).ToString().ToUpper();
	}
	return TEXT("DEEP SPACE");
}

FString ASpaceship::GetGravitySourceName() const
{
	if (!ActiveGravitySource)
	{
		return TEXT("NONE");
	}
	FString SourceName = ActiveGravitySource->GetName();
	SourceName.RemoveFromStart(TEXT("BP_"));
	const int32 ClassMarker = SourceName.Find(TEXT("_C_"));
	if (ClassMarker != INDEX_NONE)
	{
		SourceName.LeftInline(ClassMarker);
	}
	SourceName.ReplaceInline(TEXT("_"), TEXT(" "));
	return SourceName.ToUpper();
}

FString ASpaceship::GetEngineModeName() const
{
	if (!bEngineRunning) return TEXT("OFF / PARKED");
	if (bEngineModeTransitionActive)
	{
		const UEnum* EngineEnum = StaticEnum<EEngineMode>();
		const FString PendingName = EngineEnum
			? EngineEnum->GetDisplayNameTextByValue(static_cast<int64>(PendingEngineMode)).ToString().ToUpper()
			: TEXT("ENGINE");
		return FString::Printf(TEXT("TRANSITION -> %s"), *PendingName);
	}
	if (!OnboardComputer) return TEXT("UNKNOWN");
	switch (OnboardComputer->EngineSystem.CurrentEngineMode)
	{
	case EEngineMode::Impulse: return TEXT("IMPULSE");
	case EEngineMode::SpaceWrap: return TEXT("SPACE WRAP");
	case EEngineMode::Offset: return TEXT("OFFSET");
	default: return TEXT("UNKNOWN");
	}
}

double ASpaceship::GetShipSpeedMetersPerSecond() const
{
	const FVector Velocity = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
		? SpaceshipHull->GetPhysicsLinearVelocity()
		: KinematicVelocity;
	return Velocity.Size() / 100.0;
}

FText ASpaceship::GetShipStatusText() const
{
	const int32 ModeNumber = static_cast<int32>(SelectedDriveMode) + 1;
	const int32 MaximumModeNumber = static_cast<int32>(GetMaximumDriveModeForClass()) + 1;
	const FString GravityAssist = !ActiveGravitySource
		? TEXT("NONE")
		: (bEngineRunning && bFlightAssistCompensatesGravity ? TEXT("COMPENSATED") : TEXT("ACTIVE"));
	const double SpeedMetersPerSecond = GetShipSpeedMetersPerSecond();
	FString SpeedText;
	if (SpeedMetersPerSecond < 1000.0) SpeedText = FString::Printf(TEXT("%.1f m/s"), SpeedMetersPerSecond);
	else if (SpeedMetersPerSecond < 1000000.0) SpeedText = FString::Printf(TEXT("%.2f km/s"), SpeedMetersPerSecond / 1000.0);
	else if (SpeedMetersPerSecond < 299792458.0) SpeedText = FString::Printf(TEXT("%.2f Mm/s"), SpeedMetersPerSecond / 1000000.0);
	else SpeedText = FString::Printf(TEXT("%.3f c"), SpeedMetersPerSecond / 299792458.0);
	const FString CustomStatus = GetCustomFlightStatus();
	if (!CustomStatus.IsEmpty())
	{
		// The band model's own two lines carry speed, mode and surroundings.
		return FText::FromString(CustomStatus);
	}
	return FText::FromString(FString::Printf(
		TEXT("SHIP %s   |   ENGINE %s\nPOWER %d/%d: %s   |   ENVIRONMENT: %s\nGRAVITY: %s (%.2f m/s2, %s)   |   %s   |   BOOST x%.2f"),
		*GetSizeClassName(), *GetEngineModeName(), ModeNumber, MaximumModeNumber, *GetDriveModeName(),
		*GetFlightEnvironmentName(), *GetGravitySourceName(), ActiveGravityAcceleration / 100.0, *GravityAssist,
		*SpeedText, CurrentBoostMultiplier));
}

FText ASpaceship::GetShipHintText() const
{
	const FString CustomHint = GetCustomFlightHint();
	if (!CustomHint.IsEmpty())
	{
		return FText::FromString(CustomHint);
	}
	const TCHAR* SpaceWrapHint = CanUseEngineMode(EEngineMode::SpaceWrap) ? TEXT("2 SPACE WRAP") : TEXT("2 SPACE WRAP [N/A]");
	const TCHAR* OffsetHint = CanUseEngineMode(EEngineMode::Offset) ? TEXT("3 OFFSET") : TEXT("3 OFFSET [N/A]");
	return FText::FromString(FString::Printf(
		TEXT("1 IMPULSE   %s   %s   |   G POWER   F EXIT   |   WASD / SPACE / ALT THRUST\nRIGHT SHIFT/CTRL POWER STEP   LEFT SHIFT BOOST   LEFT CTRL BRAKE   |   N MARKERS   M NAV LIST   T TARGET   V ORBITS"),
		SpaceWrapHint, OffsetHint));
}

FText ASpaceship::GetNavigationPanelText() const
{
	if (!ShipNavigation)
	{
		return FText::FromString(TEXT("NAVIGATION OFFLINE"));
	}

	FString Text = FString::Printf(TEXT("NAVIGATION // LOCAL SYSTEM\nENVIRONMENT  %s   |   CELESTIAL CONTACTS  %d"),
		*GetFlightEnvironmentName(), ShipNavigation->GetDiscoveredContactCount());
	if (const FShipNavigationContact* Selected = ShipNavigation->GetSelectedContact())
	{
		const double SpeedCmPerSecond = GetShipSpeedMetersPerSecond() * 100.0;
		FString Eta = TEXT("--");
		if (SpeedCmPerSecond > 1.0)
		{
			const double Seconds = Selected->DistanceCentimeters / SpeedCmPerSecond;
			if (Seconds < 120.0) Eta = FString::Printf(TEXT("%.0f s"), Seconds);
			else if (Seconds < 7200.0) Eta = FString::Printf(TEXT("%.1f min"), Seconds / 60.0);
			else if (Seconds < 172800.0) Eta = FString::Printf(TEXT("%.1f h"), Seconds / 3600.0);
			else Eta = FString::Printf(TEXT("%.1f d"), Seconds / 86400.0);
		}
		Text += FString::Printf(TEXT("\n\nACTIVE TARGET\n%s\n%s  //  %s\nRANGE  %s   |   ETA  %s"),
			*Selected->DisplayName, *Selected->TypeLabel, *Selected->Detail,
			*UShipNavigationComponent::FormatDistance(Selected->DistanceCentimeters), *Eta);
		if (!Selected->HierarchyLabel.IsEmpty())
		{
			Text += TEXT("\nSYSTEM  ") + Selected->HierarchyLabel;
		}
	}

	Text += TEXT("\n\nNEAREST CELESTIAL BODIES");
	const TArray<FShipNavigationContact>& Contacts = ShipNavigation->GetContacts();
	const int32 ListCount = FMath::Min(Contacts.Num(), 5);
	for (int32 Index = 0; Index < ListCount; ++Index)
	{
		const TCHAR* Prefix = Index == ShipNavigation->GetSelectedContactIndex() ? TEXT(">") : TEXT(" ");
		Text += FString::Printf(TEXT("\n%s %-12s  %-24s  %s"), Prefix, *Contacts[Index].TypeLabel,
			*Contacts[Index].DisplayName, *UShipNavigationComponent::FormatDistance(Contacts[Index].DistanceCentimeters));
	}
	return FText::FromString(Text);
}

FText ASpaceship::GetNavigationMarkerText(int32 ContactIndex) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact)
	{
		return FText::GetEmpty();
	}
	FString Detail = Contact->Detail;
	FString FamilySummary;
	if (const APlanet* Planet = Cast<APlanet>(Contact->Actor.Get());
		Planet && !IsInsideNavigationFocusGravity(Planet) && ShipNavigation)
	{
		int32 MoonCount = 0;
		for (const FShipNavigationContact& FamilyContact : ShipNavigation->GetContacts())
		{
			const AMoon* Moon = Cast<AMoon>(FamilyContact.Actor.Get());
			MoonCount += Moon && Moon->ParentPlanet == Planet ? 1 : 0;
		}
		if (MoonCount > 0)
		{
			Detail.RemoveFromEnd(TEXT(" PLANET"));
			FamilySummary = FString::Printf(TEXT("   |   %d %s"), MoonCount, MoonCount == 1 ? TEXT("MOON") : TEXT("MOONS"));
		}
	}

	return FText::FromString(FString::Printf(TEXT("%s  //  %s\n%s   |   %s%s"),
		*Contact->TypeLabel, *Contact->DisplayName, *Detail,
		*UShipNavigationComponent::FormatDistance(Contact->DistanceCentimeters), *FamilySummary));
}

FVector ASpaceship::GetNavigationContactWorldAnchor(int32 ContactIndex) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact)
	{
		return FVector::ZeroVector;
	}

	AActor* Actor = Contact->Actor.Get();
	if (!Actor)
	{
		return Contact->GetWorldLocation();
	}
	// Rio 06.10 (still ship): a ship owing its travel sees the world's actors at their sky place (a system riding with the
	// sky where it is, any other at its place + the sky offset).
	const FVector Sky = UAPSWorldOriginSubsystem::SkyPlace(*Actor) - Actor->GetActorLocation();
	// Rio 04.10 ("the planet's mark trembles while the ship moves"): a world's centre is its actor; its largest visible
	// mesh changes as its surface streams (globe, terrain, clouds), and the mark jumped between their bounds.
	if (Actor->IsA<APlanetaryBody>())
	{
		return Actor->GetActorLocation() + Sky;
	}

	FVector VisualCenter = Actor->GetActorLocation();
	double LargestVisualRadius = 0.0;
	TInlineComponentArray<UMeshComponent*> MeshComponents(Actor);
	for (const UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || !MeshComponent->IsRegistered()
			|| !MeshComponent->IsVisible() || MeshComponent->bHiddenInGame)
		{
			continue;
		}
		if (MeshComponent->Bounds.SphereRadius > LargestVisualRadius)
		{
			LargestVisualRadius = MeshComponent->Bounds.SphereRadius;
			VisualCenter = MeshComponent->Bounds.Origin;
		}
	}
	return VisualCenter + Sky;
}

const APlanet* ASpaceship::GetNavigationFocusPlanet() const
{
	auto ResolvePlanetFamily = [](const AActor* Actor) -> const APlanet*
	{
		if (const APlanet* Planet = Cast<APlanet>(Actor))
		{
			return Planet;
		}
		if (const AMoon* Moon = Cast<AMoon>(Actor))
		{
			return Moon->ParentPlanet;
		}
		return nullptr;
	};

	if (const APlanet* ActivePlanet = ResolvePlanetFamily(ActiveGravitySource.Get()))
	{
		return ActivePlanet;
	}
	if (ShipNavigation)
	{
		if (const FShipNavigationContact* SelectedContact = ShipNavigation->GetSelectedContact())
		{
			if (const APlanet* SelectedPlanet = ResolvePlanetFamily(SelectedContact->Actor.Get()))
			{
				return SelectedPlanet;
			}
		}
		for (const FShipNavigationContact& Contact : ShipNavigation->GetContacts())
		{
			if (const APlanet* Planet = Cast<APlanet>(Contact.Actor.Get()))
			{
				return Planet;
			}
		}
	}
	return nullptr;
}

bool ASpaceship::IsInsideNavigationFocusGravity(const APlanet* FocusPlanet) const
{
	if (!FocusPlanet)
	{
		return false;
	}
	// Marker expansion follows the complete resident WorldScape family rather
	// than the momentary gravity source. Gravity can briefly drop while crossing
	// empty space between a planet and its moons; that must not collapse every
	// moon flag while the family is still loaded and retained by the streamer.
	if (const UWorld* World = GetWorld())
	{
		if (const UAPSPlanetEnvironmentStreamingSubsystem* Streaming =
			World->GetSubsystem<UAPSPlanetEnvironmentStreamingSubsystem>())
		{
			if (Streaming->GetResidentFamily() == FocusPlanet)
			{
				return true;
			}
		}
	}
	if (CurrentFlightEnvironment == EShipFlightEnvironment::DeepSpace)
	{
		return false;
	}
	if (ActiveGravitySource.Get() == FocusPlanet)
	{
		return true;
	}
	const AMoon* ActiveMoon = Cast<AMoon>(ActiveGravitySource.Get());
	return ActiveMoon && ActiveMoon->ParentPlanet == FocusPlanet;
}

bool ASpaceship::ShouldShowNavigationMarker(int32 ContactIndex) const
{
	if (!ShipNavigation || ContactIndex < 0 || ContactIndex >= MaximumNavigationMarkers
		|| !ShipNavigation->GetContact(ContactIndex))
	{
		return false;
	}
	const int32 SelectedIndex = ShipNavigation->GetSelectedContactIndex();
	if (ContactIndex == SelectedIndex)
	{
		return true;
	}

	const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
	// Rio 04.10: a star system seen from outside is one card, and the star-label mode labels the nearest stars.
	if (Contact->Type == EShipNavigationContactType::Planet || Contact->bOwnColony || Contact->bSystemSummary
		|| Contact->bStarLabel)
	{
		return true;
	}
	if (const AMoon* Moon = Cast<AMoon>(Contact->Actor.Get()))
	{
		// At system scale a planet and all of its moons are one compact flag.
		// Individual moon flags unfold only after gravity capture; an explicitly
		// selected moon remains visible so target cycling never loses feedback.
		return IsInsideNavigationFocusGravity(Moon->ParentPlanet);
	}
	return false;
}

bool ASpaceship::ProjectWorldLocationToNavigationScreen(const FVector& WorldLocation,
	FVector2D& OutScreenPosition, bool bRequireInsideViewport, bool bSnapToPixel) const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !GEngine || !GEngine->GameViewport)
	{
		return false;
	}
	// Same result as UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(..., true), one view per frame.
	APSNavigationHud::FProjectionFrame& Projection = APSNavigationHud::GProjectionFrame;
	if (Projection.Ship != this || Projection.Frame != GFrameCounter)
	{
		Projection = APSNavigationHud::FProjectionFrame();
		Projection.Ship = this;
		Projection.Frame = GFrameCounter;
		const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		FSceneViewProjectionData ProjectionData;
		if (LocalPlayer && LocalPlayer->ViewportClient
			&& LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
		{
			Projection.ViewProjection = ProjectionData.ComputeViewProjectionMatrix();
			Projection.ViewRect = ProjectionData.GetConstrainedViewRect();
			FVector2D Origin;
			FVector2D Reference;
			USlateBlueprintLibrary::ScreenToViewport(PlayerController, FVector2D::ZeroVector, Origin);
			USlateBlueprintLibrary::ScreenToViewport(PlayerController, FVector2D(1000.0, 1000.0), Reference);
			Projection.Offset = Origin;
			Projection.Scale = (Reference - Origin) / 1000.0;
			Projection.bValid = true;
		}
	}
	FVector2D PixelPosition;
	if (!Projection.bValid || !FSceneView::ProjectWorldToScreen(
			WorldLocation, Projection.ViewRect, Projection.ViewProjection, PixelPosition))
	{
		return false;
	}
	PixelPosition -= FVector2D(Projection.ViewRect.Min);
	// Rio 06.10 (ring jitter): sizes measured between two whole-pixel points flip by a pixel while the view turns; a caller
	// that measures or draws sub-pixel (aps.Ship.HudSubPixelRings) asks for the unrounded point.
	if (bSnapToPixel)
	{
		PixelPosition = FVector2D(FMath::RoundToInt(PixelPosition.X), FMath::RoundToInt(PixelPosition.Y));
	}
	OutScreenPosition = Projection.Offset + PixelPosition * Projection.Scale;
	// The engine's widget projection rounds too, so the check compares snapped points only.
	if (bSnapToPixel && APSShipPerf::CVarHudProjectionCheck.GetValueOnGameThread() != 0)
	{
		static double LargestDifference = 0.0;
		static double LastReportSeconds = 0.0;
		FVector2D EnginePosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, EnginePosition, true))
		{
			LargestDifference = FMath::Max(LargestDifference, FVector2D::Distance(EnginePosition, OutScreenPosition));
		}
		const double Now = FPlatformTime::Seconds();
		if (Now - LastReportSeconds >= 1.0)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.ShipPerf] HUD projection check: largest difference to the engine %.4f slate units"),
				LargestDifference);
			LargestDifference = 0.0;
			LastReportSeconds = Now;
		}
	}

	const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
	// The HUD overlay's own size once it has painted: positions are in its DPI-scaled units, which the viewport widget's
	// size is not whenever the UI scale differs from 1.
	const FVector2D SlateViewportSize = APSNavigationHud::GHudSize.X > 0.0 && APSNavigationHud::GHudSize.Y > 0.0
		? APSNavigationHud::GHudSize
		: ViewportWidget.IsValid() ? ViewportWidget->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (!bRequireInsideViewport)
	{
		return SlateViewportSize.X > 0.0f && SlateViewportSize.Y > 0.0f;
	}
	return OutScreenPosition.X >= 8.0 && OutScreenPosition.Y >= 8.0
		&& OutScreenPosition.X <= SlateViewportSize.X - 8.0
		&& OutScreenPosition.Y <= SlateViewportSize.Y - 8.0;
}

bool ASpaceship::ProjectNavigationContactToScreen(int32 ContactIndex, FVector2D& OutScreenPosition) const
{
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	return Contact && ProjectWorldLocationToNavigationScreen(
		GetNavigationContactWorldAnchor(ContactIndex), OutScreenPosition);
}

bool ASpaceship::GetNavigationMarkerLayout(int32 ContactIndex, FVector2D& OutAnchorPosition,
	FVector2D& OutLabelPosition, const TSet<int32>* OccludedContacts) const
{
	// Rio 02.10: the cards of a frame (bodies, stations, beacons and the fleet's ships) are laid out together in
	// PaintNavigationOverlay so that none overlap; this reads one contact's placement from the last painted frame.
	const APSNavigationHud::FLayoutFrame& LayoutFrame = APSNavigationHud::GLayoutFrame;
	if (LayoutFrame.Ship != this || (OccludedContacts && OccludedContacts->Contains(ContactIndex)))
	{
		return false;
	}
	if (const TPair<FVector2D, FVector2D>* Layout = LayoutFrame.Layouts.Find(ContactIndex))
	{
		OutAnchorPosition = Layout->Key;
		OutLabelPosition = Layout->Value;
		return true;
	}
	return false;
}

int32 ASpaceship::PaintNavigationOverlay(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!ShipNavigation)
	{
		return LayerId;
	}
	TRACE_CPUPROFILER_EVENT_SCOPE(APS_Ship_NavigationHud);
	APSShipPerf::FScope PerfScope(IsValid(Pilot) && APSShipPerf::Enabled(), APSShipPerf::Hud);

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const FVector2f HudLocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D HudSize(HudLocalSize.X, HudLocalSize.Y);
	APSNavigationHud::GHudSize = HudSize;
	struct FNavigationOccluder
	{
		const AActor* Actor{nullptr};
		FVector Center{FVector::ZeroVector};
		double Radius{0.0};
	};
	TArray<FNavigationOccluder, TInlineAllocator<32>> Occluders;
	const int32 NavigationContactCount = FMath::Min(
		ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_Occluders);
		for (int32 ContactIndex = 0; ContactIndex < NavigationContactCount; ++ContactIndex)
		{
			const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
			const APlanetaryBody* Body = Contact ? Cast<APlanetaryBody>(Contact->Actor.Get()) : nullptr;
			if (!Body)
			{
				continue;
			}
			const double OcclusionRadius = Body->GetWorldScapeBodyRadiusCm();
			if (OcclusionRadius > UE_DOUBLE_SMALL_NUMBER)
			{
				Occluders.Add({Body, GetNavigationContactWorldAnchor(ContactIndex), OcclusionRadius});
			}
		}
	}

	const APlayerController* NavigationPlayerController = Cast<APlayerController>(GetController());
	const APlayerCameraManager* NavigationCameraManager = NavigationPlayerController
		? NavigationPlayerController->PlayerCameraManager : nullptr;
	const FVector NavigationCameraLocation = NavigationCameraManager
		? NavigationCameraManager->GetCameraLocation() : FVector::ZeroVector;
	// Markers test against 98% of a body (a flag on its limb stays visible); orbit rings against the whole body, so a
	// ring never shows through the ground (Rio, 30.09).
	auto IsWorldPointOccluded = [&](const FVector& WorldPoint, const AActor* IgnoredActor = nullptr,
		const double RadiusScale = 0.98)
	{
		if (!NavigationCameraManager)
		{
			return false;
		}
		const FVector CameraToPoint = WorldPoint - NavigationCameraLocation;
		const double SegmentLengthSquared = CameraToPoint.SizeSquared();
		if (SegmentLengthSquared <= UE_DOUBLE_SMALL_NUMBER)
		{
			return false;
		}
		for (const FNavigationOccluder& Occluder : Occluders)
		{
			if (Occluder.Actor == IgnoredActor)
			{
				continue;
			}
			const FVector CameraToCenter = Occluder.Center - NavigationCameraLocation;
			// In a valley the camera can sit below the body's base radius: the ground still hides what lies beneath the
			// horizon, so the occluder shrinks to just below the camera instead of being skipped.
			const double Radius = FMath::Min(Occluder.Radius * RadiusScale, CameraToCenter.Size() - 100.0);
			if (Radius <= 0.0)
			{
				continue;
			}
			const double RadiusSquared = FMath::Square(Radius);
			const double SegmentFraction = FVector::DotProduct(CameraToCenter, CameraToPoint)
				/ SegmentLengthSquared;
			if (SegmentFraction <= 0.0 || SegmentFraction >= 0.9995)
			{
				continue;
			}
			const FVector ClosestPoint = NavigationCameraLocation + CameraToPoint * SegmentFraction;
			if (FVector::DistSquared(ClosestPoint, Occluder.Center) < RadiusSquared)
			{
				return true;
			}
		}
		return false;
	};

	auto DrawScreenLine = [&](const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness,
		int32 DrawLayer)
	{
		if (Points.Num() >= 2 && APSShipPerf::IsDrawableHudLine(Points))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, Points,
				ESlateDrawEffect::None, Color, true, Thickness);
		}
	};

	auto DrawProjectedRing = [&](const FVector& Center, const FVector& AxisX, const FVector& AxisY,
		double Radius, const FLinearColor& Color, float Thickness, bool bDashed, int32 DrawLayer)
	{
		if (Radius <= UE_DOUBLE_SMALL_NUMBER) return;
		constexpr int32 SegmentCount = 128;
		TArray<FVector2D> ContinuousSegment;
		FVector2D PreviousPoint;
		bool bPreviousValid = false;
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const double Angle = UE_TWO_PI * static_cast<double>(SegmentIndex) / SegmentCount;
			const FVector WorldPoint = Center + AxisX * (FMath::Cos(Angle) * Radius)
				+ AxisY * (FMath::Sin(Angle) * Radius);
			FVector2D ScreenPoint;
			const bool bValid = !IsWorldPointOccluded(WorldPoint, nullptr, 1.0)
				&& ProjectWorldLocationToNavigationScreen(WorldPoint, ScreenPoint, false);
			if (bDashed)
			{
				if (bValid && bPreviousValid && SegmentIndex % 3 != 0)
				{
					DrawScreenLine({PreviousPoint, ScreenPoint}, Color, Thickness, DrawLayer);
				}
			}
			else if (bValid)
			{
				ContinuousSegment.Add(ScreenPoint);
			}
			else
			{
				DrawScreenLine(ContinuousSegment, Color, Thickness, DrawLayer);
				ContinuousSegment.Reset();
			}
			PreviousPoint = ScreenPoint;
			bPreviousValid = bValid;
		}
		DrawScreenLine(ContinuousSegment, Color, Thickness, DrawLayer);
	};

	// Rio 03.10 ("orbits vanish and show through planets"): a planet's orbit is AU-sized while the ship flies a few
	// thousand kilometres from it, and 192 even samples drew million-kilometre chords straight across nearby worlds (only
	// the samples were tested against bodies) and dropped whole pieces once a sample went behind the camera. Now the ring
	// is sampled densely near the camera and coarsely far away, each piece is clipped at the camera plane and keeps only
	// the parts no body hides (the generation menu's exact sphere test).
	TArray<FAPSPreviewOccluder> OrbitOccluders;
	// Rio 03.10 (perf: every ring segment tests every occluder, ~3 ms a frame at the edge of a standing system): a body
	// under half a pixel on screen hides no part of a line, so only the bodies the camera resolves are tested.
	const double OrbitPixelAngle = NavigationCameraManager && HudSize.X > 1.0
		? 2.0 * FMath::Tan(FMath::DegreesToRadians(0.5 * FMath::Clamp(static_cast<double>(NavigationCameraManager->GetFOVAngle()),
			1.0, 170.0))) / HudSize.X
		: 0.0;
	for (const FNavigationOccluder& Occluder : Occluders)
	{
		const FVector Relative = Occluder.Center - NavigationCameraLocation;
		// Below a body's base radius (a valley) its ground still hides what lies under the horizon.
		const double OccluderRadius = FMath::Min(Occluder.Radius, Relative.Size() - 100.0);
		if (OccluderRadius > 0.0 && OccluderRadius >= 0.5 * OrbitPixelAngle * Relative.Size())
		{
			OrbitOccluders.Add({Relative, OccluderRadius});
		}
	}
	const FVector NavigationCameraForward = NavigationCameraManager
		? NavigationCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector;

	// Rio 02.10: orbits must read at a glance in flight. A soft glow under a firm line, brightest at the body and fading
	// along the orbit away from it, so the ring also shows where its world is.
	auto DrawOrbit = [&](const FVector& Center, const FVector& TowardBody, const FVector& Tangent, double Radius,
		const FLinearColor& Color, bool bSelected, int32 DrawLayer)
	{
		if (Radius <= UE_DOUBLE_SMALL_NUMBER || !NavigationCameraManager) return;
		constexpr int32 ChunkSegments = 7;
		const float CoreWidth = bSelected ? 2.2f : 1.5f;
		const float GlowWidth = bSelected ? 7.0f : 5.0f;

		// Rio 04.10 ("the orbits tremble while the ship moves"): the ring is sampled on angles fixed to the ring itself
		// (from a world axis in its plane), not counted from the body or from the camera's nearest point: those moved every
		// vertex of the polyline along the ring each frame, so its chords swam. Near the camera the steps are finer,
		// each a power-of-two division of 1/192 of a turn, so a vertex stays where it is and only detail comes and goes.
		const FVector RingNormal = FVector::CrossProduct(TowardBody, Tangent).GetSafeNormal();
		FVector AxisX = FVector::VectorPlaneProject(FVector::ForwardVector, RingNormal).GetSafeNormal();
		if (AxisX.IsNearlyZero()) AxisX = FVector::VectorPlaneProject(FVector::RightVector, RingNormal).GetSafeNormal();
		const FVector AxisY = FVector::CrossProduct(RingNormal, AxisX).GetSafeNormal();
		// The body's angle on that ring: the brightness falls off from it.
		const double BodyAngle = FMath::Atan2(FVector::DotProduct(TowardBody, AxisY), FVector::DotProduct(TowardBody, AxisX));
		const FVector CameraRelative = NavigationCameraLocation - Center;
		const double InPlaneX = FVector::DotProduct(CameraRelative, AxisX);
		const double InPlaneY = FVector::DotProduct(CameraRelative, AxisY);
		const double OffPlane = FVector::DotProduct(CameraRelative, RingNormal);
		const double NearestAngle = FMath::Atan2(InPlaneY, InPlaneX);
		const double RingDistance = FMath::Sqrt(
			FMath::Square(FMath::Sqrt(InPlaneX * InPlaneX + InPlaneY * InPlaneY) - Radius) + OffPlane * OffPlane);
		const double MaxStep = UE_TWO_PI / 192.0;
		const double FineStep = FMath::Clamp(0.3 * RingDistance / Radius, MaxStep / 1048576.0, MaxStep);
		// The lattice step wanted at an angular distance from the camera's nearest point (it grows away from it).
		const auto LatticeStep = [MaxStep, FineStep](const double Away)
		{
			const double Wanted = FMath::Clamp(FineStep + 0.18 * Away, FineStep, MaxStep);
			const int32 Level = FMath::Clamp(FMath::CeilToInt(FMath::Log2(MaxStep / Wanted)), 0, 20);
			return MaxStep / static_cast<double>(1 << Level);
		};
		// From the nearest point both ways to the far side, each next angle the next lattice point of its level (always
		// moving on: a point the rounding lands on again is skipped).
		TArray<double, TInlineAllocator<256>> Forward;
		TArray<double, TInlineAllocator<256>> Backward;
		for (double Angle = NearestAngle; Forward.Num() < 600;)
		{
			const double Lattice = LatticeStep(Angle - NearestAngle);
			double Next = (FMath::FloorToDouble(Angle / Lattice) + 1.0) * Lattice;
			if (Next <= Angle + Lattice * 1.0e-3) Next += Lattice;
			Angle = Next;
			if (Angle >= NearestAngle + UE_DOUBLE_PI) break;
			Forward.Add(Angle);
		}
		for (double Angle = NearestAngle; Backward.Num() < 600;)
		{
			const double Lattice = LatticeStep(NearestAngle - Angle);
			double Next = (FMath::CeilToDouble(Angle / Lattice) - 1.0) * Lattice;
			if (Next >= Angle - Lattice * 1.0e-3) Next -= Lattice;
			Angle = Next;
			if (Angle <= NearestAngle - UE_DOUBLE_PI) break;
			Backward.Add(Angle);
		}
		TArray<double, TInlineAllocator<512>> Angles;
		for (int32 Index = Backward.Num() - 1; Index >= 0; --Index)
		{
			Angles.Add(Backward[Index]);
		}
		for (const double Each : Forward)
		{
			Angles.Add(Each);
		}
		// The body itself is a vertex too (it stands still on its orbit), so the line passes through it and not along a
		// chord beside it (thousands of km off at 1 AU between lattice points).
		if (!Angles.IsEmpty())
		{
			double BodyAt = BodyAngle;
			while (BodyAt < Angles[0]) BodyAt += UE_DOUBLE_TWO_PI;
			while (BodyAt >= Angles[0] + UE_DOUBLE_TWO_PI) BodyAt -= UE_DOUBLE_TWO_PI;
			const int32 At = Algo::LowerBound(Angles, BodyAt);
			if (!Angles.IsValidIndex(At) || !FMath::IsNearlyEqual(Angles[At], BodyAt, 1.0e-9))
			{
				Angles.Insert(BodyAt, At);
			}
		}
		// The far side closes on the farthest lattice point of the other side (a turn on), so no vertex follows the camera.
		if (Angles.Num() >= 2)
		{
			Angles.Add(Angles[0] + UE_DOUBLE_TWO_PI);
		}

		TArray<FVector2D> Chunk;
		double ChunkAngleSum = 0.0;
		int32 ChunkCount = 0;
		const auto Flush = [&]()
		{
			if (Chunk.Num() >= 2 && ChunkCount > 0)
			{
				const double Middle = ChunkAngleSum / ChunkCount - BodyAngle;
				const float Near = FMath::Pow(0.5f + 0.5f * static_cast<float>(FMath::Cos(Middle)), 1.6f);
				const float Alpha = Color.A * (0.38f + 0.62f * Near);
				DrawScreenLine(Chunk, FLinearColor(Color.R, Color.G, Color.B, Alpha * 0.22f), GlowWidth, DrawLayer);
				DrawScreenLine(Chunk, FLinearColor(Color.R, Color.G, Color.B, Alpha), CoreWidth, DrawLayer + 1);
			}
			Chunk.Reset();
			ChunkAngleSum = 0.0;
			ChunkCount = 0;
		};
		const auto RingPoint = [&](const double Angle)
		{
			return Center + AxisX * (FMath::Cos(Angle) * Radius) + AxisY * (FMath::Sin(Angle) * Radius)
				- NavigationCameraLocation;
		};
		TArray<FVector2D> Intervals;
		FVector Previous = RingPoint(Angles[0]);
		for (int32 Index = 1; Index < Angles.Num(); ++Index)
		{
			const FVector Current = RingPoint(Angles[Index]);
			const double SegmentAngle = 0.5 * (Angles[Index - 1] + Angles[Index]);
			FVector From = Previous;
			FVector To = Current;
			Previous = Current;
			constexpr double NearPlaneCm = 10.0;
			const double DepthFrom = FVector::DotProduct(From, NavigationCameraForward);
			const double DepthTo = FVector::DotProduct(To, NavigationCameraForward);
			if (DepthFrom <= NearPlaneCm && DepthTo <= NearPlaneCm)
			{
				Flush();
				continue;
			}
			if (DepthFrom <= NearPlaneCm)
			{
				From = FMath::Lerp(From, To, (NearPlaneCm - DepthFrom) / (DepthTo - DepthFrom));
			}
			else if (DepthTo <= NearPlaneCm)
			{
				To = FMath::Lerp(From, To, (NearPlaneCm - DepthFrom) / (DepthTo - DepthFrom));
			}
			APSPreviewVisibility::VisibleIntervals(From, To, OrbitOccluders, Intervals);
			if (Intervals.IsEmpty())
			{
				Flush();
				continue;
			}
			for (const FVector2D& Interval : Intervals)
			{
				FVector2D Start;
				FVector2D End;
				if (!ProjectWorldLocationToNavigationScreen(NavigationCameraLocation + FMath::Lerp(From, To, Interval.X), Start, false)
					|| !ProjectWorldLocationToNavigationScreen(NavigationCameraLocation + FMath::Lerp(From, To, Interval.Y), End, false))
				{
					Flush();
					continue;
				}
				if (!Chunk.IsEmpty() && !Chunk.Last().Equals(Start, 0.5))
				{
					Flush();
				}
				if (Chunk.IsEmpty())
				{
					Chunk.Add(Start);
				}
				Chunk.Add(End);
				ChunkAngleSum += SegmentAngle;
				++ChunkCount;
				if (ChunkCount >= ChunkSegments)
				{
					// Each chunk carries its own brightness; the next one starts where this one ends.
					const FVector2D Joint = Chunk.Last();
					Flush();
					Chunk.Add(Joint);
				}
			}
		}
		Flush();
	};

	if (bNavigationGuidesVisible)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_Orbits);
		TSet<const APlanetOrbit*> PaintedOrbits;
		const int32 SelectedIndex = ShipNavigation->GetSelectedContactIndex();
		const FShipNavigationContact* SelectedContact = ShipNavigation->GetSelectedContact();
		const APlanetaryBody* SelectedBody = SelectedContact
			? Cast<APlanetaryBody>(SelectedContact->Actor.Get()) : nullptr;
		const APlanet* SelectedPlanetFamily = Cast<APlanet>(SelectedBody);
		if (const AMoon* SelectedMoon = Cast<AMoon>(SelectedBody))
		{
			SelectedPlanetFamily = SelectedMoon->ParentPlanet;
		}
		int32 PaintedPlanetCount = 0;
		int32 PaintedMoonCount = 0;
		const int32 ContactCount = FMath::Min(ShipNavigation->GetContacts().Num(), MaximumNavigationMarkers);
		for (int32 ContactIndex = 0; ContactIndex < ContactCount; ++ContactIndex)
		{
			const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
			const APlanetaryBody* Body = Contact ? Cast<APlanetaryBody>(Contact->Actor.Get()) : nullptr;
			const APlanetOrbit* Orbit = Body ? Cast<APlanetOrbit>(Body->GetAttachParentActor()) : nullptr;
			if (!Orbit || PaintedOrbits.Contains(Orbit)) continue;

			const bool bSelectedOrbit = ContactIndex == SelectedIndex;
			bool bShouldPaintOrbit = bSelectedOrbit;
			if (const APlanet* Planet = Cast<APlanet>(Body))
			{
				const int32 OrbitPlanets = APSShipPerf::CVarHudOrbitPlanets.GetValueOnGameThread();
				bShouldPaintOrbit |= Planet == SelectedPlanetFamily || OrbitPlanets <= 0 || PaintedPlanetCount < OrbitPlanets;
				if (bShouldPaintOrbit) ++PaintedPlanetCount;
			}
			else if (const AMoon* Moon = Cast<AMoon>(Body))
			{
				bShouldPaintOrbit |= Moon->ParentPlanet == SelectedPlanetFamily && PaintedMoonCount < 2;
				if (bShouldPaintOrbit) ++PaintedMoonCount;
			}
			if (!bShouldPaintOrbit) continue;
			PaintedOrbits.Add(Orbit);

			// Rio 06.10 (still ship): the orbit's centre where the ship sees it, like the body's (the review: a ring of light
			// years while owing).
			const FVector OrbitCenter = UAPSWorldOriginSubsystem::SkyPlace(*Orbit);
			const FVector BodyCenter = GetNavigationContactWorldAnchor(ContactIndex);
			const FVector RadialVector = BodyCenter - OrbitCenter;
			const double OrbitRadius = RadialVector.Size();
			const FVector OrbitRadial = RadialVector.GetSafeNormal();
			FVector OrbitTangent = FVector::CrossProduct(Orbit->GetActorUpVector(), OrbitRadial).GetSafeNormal();
			if (OrbitTangent.IsNearlyZero()) OrbitTangent = Orbit->GetActorRightVector();
			const FLinearColor MarkerColor = GetNavigationMarkerColor(ContactIndex);
			const FLinearColor OrbitColor(MarkerColor.R, MarkerColor.G, MarkerColor.B,
				bSelectedOrbit ? 0.95f : (Contact->Type == EShipNavigationContactType::Moon ? 0.42f : 0.5f));
			DrawOrbit(OrbitCenter, OrbitRadial, OrbitTangent, OrbitRadius, OrbitColor, bSelectedOrbit, LayerId);
		}

		if (SelectedContact)
		{
			const APlayerController* PC = Cast<APlayerController>(GetController());
			const APlayerCameraManager* CameraManager = PC ? PC->PlayerCameraManager : nullptr;
			const FVector CameraRight = CameraManager ? CameraManager->GetActorRightVector() : FVector::RightVector;
			const FVector CameraUp = CameraManager ? CameraManager->GetActorUpVector() : FVector::UpVector;

			const AStar* ParentStar = nullptr;
			if (const APlanet* Planet = Cast<APlanet>(SelectedBody)) ParentStar = Planet->ParentStar;
			else if (const AMoon* Moon = Cast<AMoon>(SelectedBody))
			{
				ParentStar = Moon->ParentPlanet ? Moon->ParentPlanet->ParentStar : nullptr;
			}

			if (const APlanet* Planet = Cast<APlanet>(SelectedBody);
				Planet && Planet->GravityCollisionZone && Planet->GravityCollisionZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(Planet->GravityCollisionZone->GetComponentLocation()
					+ (UAPSWorldOriginSubsystem::SkyPlace(*Planet) - Planet->GetActorLocation()), CameraRight, CameraUp,
					Planet->GravityCollisionZone->GetScaledSphereRadius(),
					FLinearColor(0.28f, 1.0f, 0.58f, 0.34f), 0.9f, true, LayerId + 1);
			}
			if (ParentStar && ParentStar->PlanetarySystemZone
				&& ParentStar->PlanetarySystemZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(ParentStar->PlanetarySystemZone->GetComponentLocation()
					+ (UAPSWorldOriginSubsystem::SkyPlace(*ParentStar) - ParentStar->GetActorLocation()), CameraRight, CameraUp,
					ParentStar->PlanetarySystemZone->GetScaledSphereRadius(),
					FLinearColor(0.22f, 0.65f, 1.0f, 0.16f), 0.7f, true, LayerId);
			}

			const AActor* Ancestor = ParentStar;
			while (Ancestor && !Ancestor->IsA<AStarSystem>()) Ancestor = Ancestor->GetAttachParentActor();
			if (const AStarSystem* System = Cast<AStarSystem>(Ancestor);
				System && System->StarSystemZone && System->StarSystemZone->GetScaledSphereRadius() > 0.0f)
			{
				DrawProjectedRing(System->StarSystemZone->GetComponentLocation()
					+ (UAPSWorldOriginSubsystem::SkyPlace(*System) - System->GetActorLocation()), CameraRight, CameraUp,
					System->StarSystemZone->GetScaledSphereRadius(),
					FLinearColor(0.65f, 0.42f, 1.0f, 0.13f), 0.65f, true, LayerId);
			}
		}
	}

	// Altimeter (Rio 02.10: "on the approach to a planet, once orbital flight begins, show how far the surface still is,
	// nicely integrated"): the nearest planet or moon within two of its radii above the surface, the terrain or sea
	// under the ship, the vertical speed and the time to the surface at that rate.
	APSNavigationHud::FAltimeter& Altimeter = APSNavigationHud::GAltimeter;
	const double NowSeconds = FPlatformTime::Seconds();
	if (Altimeter.Ship != this)
	{
		Altimeter = APSNavigationHud::FAltimeter();
		Altimeter.Ship = this;
		Altimeter.LastSeconds = NowSeconds;
	}
	const float HudDeltaSeconds = FMath::Clamp(static_cast<float>(NowSeconds - Altimeter.LastSeconds), 0.0f, 0.1f);
	Altimeter.LastSeconds = NowSeconds;
	const FVector ShipLocation = GetActorLocation();
	if (const UWorld* HudWorld = GetWorld(); HudWorld && NowSeconds - Altimeter.PickSeconds >= 0.25)
	{
		Altimeter.PickSeconds = NowSeconds;
		const APlanetaryBody* Shown = Altimeter.Body.Get();
		const APlanetaryBody* Nearest = nullptr;
		double NearestAltitude = 0.0;
		for (TActorIterator<APlanetaryBody> It(HudWorld); It; ++It)
		{
			const APlanetaryBody* Candidate = *It;
			if (!IsValid(Candidate))
			{
				continue;
			}
			const double CandidateRadius = Candidate->GetWorldScapeBodyRadiusCm();
			const double CandidateAltitude = FVector::Dist(ShipLocation, Candidate->GetActorLocation()) - CandidateRadius;
			// The body on the instrument keeps it a little further out, so the instrument does not blink at the edge.
			const double Reach = CandidateRadius * APSNavigationHud::AltimeterReachRadii * (Candidate == Shown ? 1.15 : 1.0);
			if (CandidateAltitude <= Reach && (!Nearest || CandidateAltitude < NearestAltitude))
			{
				Nearest = Candidate;
				NearestAltitude = CandidateAltitude;
			}
		}
		// A rover or a hover lives on the ground (02.10 test shots: "0 m, DESCENT 5.5 m/s" over every bump); the
		// drone flies up to the atmosphere's ceiling and keeps the instrument.
		if (IsGroundVehicle() && GroundVehicleKind != EAPSGroundVehicleKind::Drone)
		{
			Nearest = nullptr;
		}
		if (Nearest != Shown)
		{
			Altimeter.SampleSeconds = -1.0e9;
			Altimeter.SampleGroundCm = -1.0;
			Altimeter.Severity = 0.0f;
		}
		Altimeter.Body = Nearest;
	}
	if (const APlanetaryBody* AltimeterBody = Altimeter.Body.Get())
	{
		const FVector FromCenter = ShipLocation - AltimeterBody->GetActorLocation();
		const double CenterDistance = FromCenter.Size();
		const FVector Up = CenterDistance > 1.0 ? FromCenter / CenterDistance : FVector::UpVector;
		const double BodyRadius = AltimeterBody->GetWorldScapeBodyRadiusCm();
		const double BaseAltitude = CenterDistance - BodyRadius;
		// The flight model's ground clearance measures from half the hull's bounds too: a landed ship reads about 0.
		const double HullReach = SpaceshipHull ? SpaceshipHull->Bounds.SphereRadius * 0.5 : 0.0;
		if (NowSeconds - Altimeter.SampleSeconds >= 0.1)
		{
			Altimeter.SampleSeconds = NowSeconds;
			Altimeter.SampleBaseCm = BaseAltitude;
			double Ground = BaseAltitude < APSNavigationHud::AltimeterTerrainAltitudeCm
				? APSNavigationHud::TerrainClearanceCm(AltimeterBody, ShipLocation) : -1.0;
			// Close to the ground a short trace sees the collision, the colony's pads and whatever stands there.
			const double ProbeLength = FMath::Max(Ground >= 0.0 ? Ground : BaseAltitude, 0.0) + HullReach + 50000.0;
			if (ProbeLength <= 2000000.0 && GetWorld())
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(APSShipAltimeter), false, this);
				if (IsValid(Pilot))
				{
					Params.AddIgnoredActor(Pilot);
				}
				TArray<AActor*> Carried;
				GetAttachedActors(Carried, true, true);
				Params.AddIgnoredActors(Carried);
				FHitResult Hit;
				if (GetWorld()->LineTraceSingleByChannel(Hit, ShipLocation, ShipLocation - Up * ProbeLength,
					ECC_Visibility, Params))
				{
					Ground = Ground >= 0.0 ? FMath::Min(Ground, static_cast<double>(Hit.Distance)) : static_cast<double>(Hit.Distance);
				}
			}
			Altimeter.SampleGroundCm = Ground >= 0.0 ? FMath::Max(Ground - HullReach, 0.0) : -1.0;
		}
		Altimeter.AltitudeCm = Altimeter.SampleGroundCm >= 0.0
			? FMath::Max(Altimeter.SampleGroundCm + BaseAltitude - Altimeter.SampleBaseCm, 0.0)
			: FMath::Max(BaseAltitude - HullReach, 0.0);
		const FVector Velocity = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
			? SpaceshipHull->GetPhysicsLinearVelocity() : KinematicVelocity;
		Altimeter.VerticalCmPerSecond = FMath::Lerp(Altimeter.VerticalCmPerSecond, FVector::DotProduct(Velocity, Up),
			1.0 - FMath::Exp(-static_cast<double>(HudDeltaSeconds) / 0.12));
		Altimeter.TopCm = FMath::Max(BodyRadius * APSNavigationHud::AltimeterReachRadii, 1.0e6);
		Altimeter.AtmosphereCm = AltimeterBody->AtmosphereHeight > UE_SMALL_NUMBER
			? AltimeterBody->AtmosphereHeight * 100000.0 : -1.0;
		// Amber, then red, while closing on the surface fast down low: at this rate the ground is seconds away. A slow
		// landing stays calm.
		const double CloseIn = FMath::Clamp(Altimeter.AtmosphereCm > 0.0 ? Altimeter.AtmosphereCm : BodyRadius / 30.0,
			2.0e6, 3.0e7);
		const double Descent = -Altimeter.VerticalCmPerSecond;
		float Severity = 0.0f;
		if (Descent > 0.0 && Altimeter.AltitudeCm < CloseIn)
		{
			const double SecondsLeft = Altimeter.AltitudeCm / Descent;
			Severity = SecondsLeft < 5.0 && Descent > 2500.0 ? 2.0f : SecondsLeft < 15.0 && Descent > 1000.0 ? 1.0f : 0.0f;
		}
		Altimeter.Severity = FMath::FInterpTo(Altimeter.Severity, Severity, HudDeltaSeconds, 6.0f);
		Altimeter.BodyName = AltimeterBody->AstroName.IsNone()
			? AltimeterBody->GetName().ToUpper() : AltimeterBody->AstroName.ToString().ToUpper();
		Altimeter.Designation = APSBodyDesignation::Of(AltimeterBody);
		if (!APSNavigationHud::BodyMarkerColor(AltimeterBody, Altimeter.BodyColor))
		{
			Altimeter.BodyColor = FLinearColor(0.72f, 0.82f, 0.9f, 1.0f);
		}
		Altimeter.bReading = true;
	}
	Altimeter.Alpha = FMath::FInterpConstantTo(Altimeter.Alpha, Altimeter.Body.IsValid() ? 1.0f : 0.0f, HudDeltaSeconds,
		Altimeter.Body.IsValid() ? 3.5f : 2.0f);

	// The HUD's panels in the overlay's space: no card covers them.
	const auto PanelRect = [&AllottedGeometry](const TWeakPtr<SWidget>& WeakPanel, FSlateRect& OutRect)
	{
		const TSharedPtr<SWidget> Panel = WeakPanel.Pin();
		if (!Panel.IsValid() || !Panel->GetVisibility().IsVisible())
		{
			return false;
		}
		// Rio 06.10 (audit): paint space, like AllottedGeometry: GetCachedGeometry() is the desktop-space tick geometry
		// and shifted every panel rect by the window's position (as SWorldGenerationPanel's guides once did).
		const FGeometry& PanelGeometry = Panel->GetPaintSpaceGeometry();
		const FVector2f PanelPosition = PanelGeometry.GetAbsolutePosition();
		const FVector2f PanelSize = PanelGeometry.GetAbsoluteSize();
		const FVector2f TopLeft = AllottedGeometry.AbsoluteToLocal(PanelPosition);
		const FVector2f BottomRight = AllottedGeometry.AbsoluteToLocal(PanelPosition + PanelSize);
		if (BottomRight.X - TopLeft.X < 2.0f || BottomRight.Y - TopLeft.Y < 2.0f)
		{
			return false;
		}
		OutRect = FSlateRect(TopLeft.X, TopLeft.Y, BottomRight.X, BottomRight.Y);
		return true;
	};
	FSlateRect NavigationPanelRect;
	// Rio 06.10 (audit): M collapses the NAVIGATION card's parent, not the card, whose last geometry then stays frozen;
	// a hidden card is neither an obstacle for the cards nor pushes the altimeter down.
	const bool bNavigationPanelShown = bNavigationPanelVisible
		&& PanelRect(APSNavigationHud::GNavigationPanel, NavigationPanelRect);
	FSlateRect StatusPanelRect;
	const bool bStatusPanelShown = PanelRect(APSNavigationHud::GStatusPanel, StatusPanelRect);
	// The altimeter stands right of centre, below the navigation panel, out of the cards' way (they keep clear of it).
	FSlateRect AltimeterRect;
	const bool bAltimeterShown = Altimeter.bReading && Altimeter.Alpha > 0.01f && HudSize.X > 480.0 && HudSize.Y > 320.0;
	if (bAltimeterShown)
	{
		constexpr float AltimeterWidth = 184.0f;
		const float AltimeterHeight = FMath::Clamp(static_cast<float>(HudSize.Y) * 0.4f, 250.0f, 330.0f);
		const float AltimeterLeft = static_cast<float>(HudSize.X) - 36.0f - AltimeterWidth;
		float AltimeterTop = static_cast<float>(HudSize.Y) * 0.5f - AltimeterHeight * 0.5f;
		if (bNavigationPanelShown && NavigationPanelRect.Left < AltimeterLeft + AltimeterWidth)
		{
			AltimeterTop = FMath::Max(AltimeterTop, NavigationPanelRect.Bottom + 14.0f);
		}
		AltimeterTop = FMath::Max(FMath::Min(AltimeterTop, static_cast<float>(HudSize.Y) - 24.0f - AltimeterHeight), 12.0f);
		AltimeterRect = FSlateRect(AltimeterLeft, AltimeterTop, AltimeterLeft + AltimeterWidth, AltimeterTop + AltimeterHeight);
	}

	if (bNavigationMarkersVisible)
	{
		// Rio 06.10 ("the planet rings jitter while the nose turns"): a ring's size was the distance between two whole-pixel
		// projections, floor or ceil of the true limb by the anchor's sub-pixel phase, so during a turn it flipped by ~1.15 px
		// and the cards (KeepOut) hopped with it. Measured unrounded it changes only with distance; the ring is drawn around
		// its unrounded centre without pixel snapping, the cards and texts stay on whole pixels.
		const bool bSubPixelRings = APSShipPerf::CVarHudSubPixelRings.GetValueOnGameThread() != 0;
		TSet<int32> OccludedContacts;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_Occluders);
			for (int32 ContactIndex = 0; ContactIndex < NavigationContactCount; ++ContactIndex)
			{
				const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
				if (Contact && IsWorldPointOccluded(
					GetNavigationContactWorldAnchor(ContactIndex), Contact->Actor.Get()))
				{
					OccludedContacts.Add(ContactIndex);
				}
			}
		}
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		TArray<FSlateRect> Obstacles;
		if (bNavigationPanelShown)
		{
			Obstacles.Add(APSNavigationHud::Inflate(NavigationPanelRect, APSNavigationHud::PanelClearance));
		}
		if (bStatusPanelShown)
		{
			Obstacles.Add(APSNavigationHud::Inflate(StatusPanelRect, APSNavigationHud::PanelClearance));
		}
		if (bAltimeterShown)
		{
			Obstacles.Add(APSNavigationHud::Inflate(AltimeterRect, APSNavigationHud::PanelClearance));
		}

		// Course target: corner brackets that stand apart from every flag and orbit, pulsing gently, and while it is out
		// of view an arrow at the screen edge toward it (Rio, 30.09: the target has to be clearly marked). Laid out before
		// the cards, which keep clear of the brackets, the arrow and the caption.
		const int32 TargetIndex = ShipNavigation->GetSelectedContactIndex();
		const FShipNavigationContact* Target = ShipNavigation->GetSelectedContact();
		const FSlateFontInfo CaptionFont = APSUITheme::BodyFont("Bold", 8);
		const FLinearColor TargetColor = (APSUITheme::Current() == EAPSUITheme::Classic ? FLinearColor(1.0f, 0.74f, 0.18f, 1.0f)
			: APSUITheme::Palette().ActionPeak).CopyWithNewOpacity(0.78f + 0.22f * static_cast<float>(FMath::Sin(NowSeconds * 3.0)));
		FString Caption;
		FVector2D TargetScreen = FVector2D::ZeroVector;
		FVector2D ArrowTip = FVector2D::ZeroVector;
		FVector2D ArrowDirection = FVector2D::ZeroVector;
		FVector2D CaptionAt = FVector2D::ZeroVector;
		double TargetHalf = 0.0;
		bool bTargetInView = false;
		bool bTargetArrow = false;
		if (Target && NavigationCameraManager)
		{
			const FVector TargetWorld = GetNavigationContactWorldAnchor(TargetIndex);
			Caption = FString::Printf(TEXT("COURSE  %s"),
				*UShipNavigationComponent::FormatDistance(FVector::Distance(GetActorLocation(), TargetWorld)));
			const FVector2D CaptionSize = FontMeasure->Measure(Caption, CaptionFont);
			if (ProjectWorldLocationToNavigationScreen(TargetWorld, TargetScreen, true))
			{
				// Around a resolved disc (a planet or moon) the brackets hug its rim, otherwise they keep a fixed box.
				double DiscPixels = 0.0;
				FVector2D Rim;
				// The disc's size from unrounded points (aps.Ship.HudSubPixelRings); the brackets stay on the snapped centre.
				FVector2D DiscCentre = TargetScreen;
				if (const APlanetaryBody* TargetBody = Cast<APlanetaryBody>(Target->Actor.Get()); TargetBody
					&& (!bSubPixelRings || ProjectWorldLocationToNavigationScreen(TargetWorld, DiscCentre, false, false))
					&& ProjectWorldLocationToNavigationScreen(TargetWorld + NavigationCameraManager->GetActorRightVector()
						* TargetBody->GetWorldScapeBodyRadiusCm(), Rim, false, !bSubPixelRings))
				{
					DiscPixels = FVector2D::Distance(Rim, DiscCentre);
				}
				bTargetInView = true;
				TargetHalf = FMath::Clamp(DiscPixels + 10.0, 16.0, 160.0);
				CaptionAt = FVector2D(TargetScreen.X - TargetHalf, TargetScreen.Y + TargetHalf + 4.0);
				Obstacles.Add(FSlateRect(TargetScreen.X - TargetHalf - 2.0, TargetScreen.Y - TargetHalf - 2.0,
					TargetScreen.X + TargetHalf + 2.0, TargetScreen.Y + TargetHalf + 2.0));
				Obstacles.Add(FSlateRect(CaptionAt.X, CaptionAt.Y, CaptionAt.X + CaptionSize.X, CaptionAt.Y + CaptionSize.Y));
			}
			else if (HudSize.X > 160.0 && HudSize.Y > 160.0)
			{
				// Off screen or behind the camera: the target's direction from the view axis, on an inset ellipse.
				const FVector Local = NavigationCameraManager->GetCameraRotation().UnrotateVector(
					TargetWorld - NavigationCameraLocation);
				ArrowDirection = FVector2D(Local.Y, -Local.Z);
				if (!ArrowDirection.Normalize())
				{
					ArrowDirection = FVector2D(0.0, 1.0);
				}
				const FVector2D Centre = HudSize * 0.5;
				ArrowTip = Centre + FVector2D(ArrowDirection.X * (Centre.X - 56.0), ArrowDirection.Y * (Centre.Y - 56.0));
				// An arrow that would fall on the altimeter stands just left of it.
				if (bAltimeterShown && ArrowTip.X > AltimeterRect.Left - 24.0
					&& ArrowTip.Y > AltimeterRect.Top - 24.0 && ArrowTip.Y < AltimeterRect.Bottom + 24.0)
				{
					ArrowTip.X = AltimeterRect.Left - 24.0;
				}
				CaptionAt = ArrowTip - ArrowDirection * 40.0 - FVector2D(40.0, 7.0);
				bTargetArrow = true;
				Obstacles.Add(FSlateRect(ArrowTip.X - 20.0, ArrowTip.Y - 20.0, ArrowTip.X + 20.0, ArrowTip.Y + 20.0));
				Obstacles.Add(FSlateRect(CaptionAt.X, CaptionAt.Y, CaptionAt.X + CaptionSize.X, CaptionAt.Y + CaptionSize.Y));
			}
		}

		// Every object's card: the navigation contacts here, the fleet's ships below; laid out together.
		TArray<APSNavigationHud::FCard> Cards;
		Cards.Reserve(NavigationContactCount + 12);
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_CardBuild);
			for (int32 ContactIndex = 0; ContactIndex < NavigationContactCount; ++ContactIndex)
			{
				const FShipNavigationContact* Contact = ShipNavigation->GetContact(ContactIndex);
				FVector2D Anchor;
				if (!Contact || OccludedContacts.Contains(ContactIndex) || !ShouldShowNavigationMarker(ContactIndex)
					|| !ProjectNavigationContactToScreen(ContactIndex, Anchor))
				{
					continue;
				}
				const AActor* ContactActor = Contact->Actor.Get();
				APSNavigationHud::FCard& Card = Cards.AddDefaulted_GetRef();
				Card.Kind = APSNavigationHud::ECardKind::Contact;
				Card.ContactIndex = ContactIndex;
				Card.Key = ContactActor;
				Card.Anchor = Anchor;
				Card.Distance = Contact->DistanceCentimeters;
				Card.bSelected = ContactIndex == TargetIndex;
				Card.Rank = ContactActor && (ContactActor->IsA<AStar>() || ContactActor->IsA<APlanet>()) ? 1
					: ContactActor && ContactActor->IsA<AMoon>() ? 2 : 3;
				Card.Color = GetNavigationMarkerColor(ContactIndex);
				const FString MarkerText = GetNavigationMarkerText(ContactIndex).ToString();
				if (!MarkerText.Split(TEXT("\n"), &Card.Title, &Card.Detail))
				{
					Card.Title = MarkerText;
				}
				// Rio 02.10 ("where did the indices of the planets and moons go"): the catalogue designation after the name,
				// as in the generation menu.
				Card.Designation = APSBodyDesignation::Of(ContactActor);
				Card.ShortName = Card.Designation.IsEmpty() ? Contact->DisplayName : Card.Designation + TEXT(" ") + Contact->DisplayName;
				// Rio 02.10: a ring on the limb of the body instead of a cross. It hugs a resolved disc and gives way once the
				// body fills a large part of the view.
				double LimbPixels = 0.0;
				double RingBodyRadius = 0.0;
				if (const APlanetaryBody* RingBody = Cast<APlanetaryBody>(ContactActor))
				{
					RingBodyRadius = RingBody->GetWorldScapeBodyRadiusCm();
				}
				else if (const AStar* RingStar = Cast<AStar>(ContactActor))
				{
					RingBodyRadius = RingStar->RadiusKM * 100000.0;
				}
				const FVector AnchorWorld = GetNavigationContactWorldAnchor(ContactIndex);
				Card.RingCenter = Anchor;
				FVector2D ExactCenter;
				if (bSubPixelRings && ProjectWorldLocationToNavigationScreen(AnchorWorld, ExactCenter, false, false))
				{
					Card.RingCenter = ExactCenter;
				}
				FVector2D Limb;
				if (RingBodyRadius > 0.0 && NavigationCameraManager
					&& ProjectWorldLocationToNavigationScreen(AnchorWorld
						+ NavigationCameraManager->GetActorRightVector() * RingBodyRadius, Limb, false, !bSubPixelRings))
				{
					LimbPixels = FVector2D::Distance(Limb, Card.RingCenter);
				}
				// Room around the limb (Rio 02.10: "borders with a margin, not tight").
				Card.RingRadius = LimbPixels < 160.0
					? FMath::Max(Card.bSelected ? 7.5f : 6.5f, static_cast<float>(LimbPixels) * 1.15f + 7.0f) : 0.0f;
				Card.KeepOut = Card.RingRadius > 0.0f ? Card.RingRadius : 6.0f;
				if (Card.bSelected && bTargetInView)
				{
					Card.KeepOut = FMath::Max(Card.KeepOut, static_cast<float>(TargetHalf) + 3.0f);
				}
			}

			// Rio 02.10: the ships of the civilization are tracked too, besides navigation targets: a diamond in the
			// colour of the division and a flag in the style of the navigation flags with call sign, speed and distance.
			if (const FAPSFleetCommand* Fleet = APSFleetFind(GetWorld()))
			{
				struct FUnitMarker
				{
					const FAPSFleetUnit* Unit;
					double Distance;
				};
				TArray<FUnitMarker, TInlineAllocator<32>> UnitMarkers;
				for (const FAPSFleetUnit& Unit : Fleet->GetUnits())
				{
					const ASpaceship* UnitShip = Unit.Ship.Get();
					if (UnitShip && UnitShip != this)
					{
						UnitMarkers.Add({&Unit, FVector::Distance(UAPSWorldOriginSubsystem::SkyPlace(*UnitShip), GetActorLocation())});
					}
				}
				UnitMarkers.Sort([](const FUnitMarker& A, const FUnitMarker& B) { return A.Distance < B.Distance; });
				const auto FormatSpeed = [](const double CentimetersPerSecond)
				{
					const double KmPerSecond = CentimetersPerSecond / 100000.0;
					return KmPerSecond >= 1000.0 ? APSUINumber::Number(FMath::RoundToInt(KmPerSecond)).ToString() + TEXT(" km/s")
						: KmPerSecond >= 1.0 ? FString::Printf(TEXT("%.1f km/s"), KmPerSecond)
						: FString::Printf(TEXT("%.0f m/s"), CentimetersPerSecond / 100.0);
				};
				int32 ShownUnits = 0;
				for (const FUnitMarker& Marker : UnitMarkers)
				{
					if (ShownUnits >= 12) break;
					const ASpaceship* UnitShip = Marker.Unit->Ship.Get();
					// Rio 06.10 (still ship): a fleet unit where the ship sees it (its world place + the sky offset while owing).
					const FVector UnitWorld = UAPSWorldOriginSubsystem::SkyPlace(*UnitShip);
					// Rio 04.10: ships in a star system seen from outside fold into its card with its worlds.
					if (ShipNavigation && ShipNavigation->IsInFoldedSystem(UnitWorld)) continue;
					FVector2D UnitScreen;
					// On screen only: a card is never drawn for a ship beyond the edge.
					if (IsWorldPointOccluded(UnitWorld, UnitShip)
						|| !ProjectWorldLocationToNavigationScreen(UnitWorld, UnitScreen, true)) continue;
					++ShownUnits;
					APSNavigationHud::FCard& Card = Cards.AddDefaulted_GetRef();
					Card.Kind = APSNavigationHud::ECardKind::Unit;
					Card.Key = UnitShip;
					Card.Anchor = UnitScreen;
					Card.KeepOut = 5.0f;
					Card.Distance = Marker.Distance;
					Card.Rank = 4;
					Card.Color = APSFleet::DivisionColour(Marker.Unit->Division);
					Card.Title = Marker.Unit->CallSign.IsEmpty() ? UnitShip->GetName() : Marker.Unit->CallSign;
					const double SpeedCm = Marker.Unit->Speed > 0.0 ? Marker.Unit->Speed : UnitShip->GetVelocity().Size();
					Card.Detail = FString::Printf(TEXT("%s  /  %s"), *FormatSpeed(SpeedCm),
						*UShipNavigationComponent::FormatDistance(Marker.Distance));
					Card.ShortName = Card.Title;
				}
			}
		}

		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_Layout);
			APSNavigationHud::LayOutCards(Cards, Obstacles, HudSize, *FontMeasure);
		}
		APSNavigationHud::FLayoutFrame& LayoutFrame = APSNavigationHud::GLayoutFrame;
		LayoutFrame.Ship = this;
		LayoutFrame.Frame = GFrameCounter;
		LayoutFrame.Layouts.Reset();
		const FSlateBrush* CardBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const float ContactLine = static_cast<float>(FontMeasure->GetMaxCharacterHeight(APSNavigationHud::MarkerFont()));
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_Rings);
			// Every object keeps its mark (limb ring or diamond), also when its card is shared or there was no room for one.
			for (const APSNavigationHud::FCard& Card : Cards)
			{
				if (Card.Kind == APSNavigationHud::ECardKind::Unit)
				{
					constexpr float Diamond = 5.0f;
					const FVector2D& Mark = Card.Anchor;
					DrawScreenLine({Mark + FVector2D(0.0f, -Diamond), Mark + FVector2D(Diamond, 0.0f),
						Mark + FVector2D(0.0f, Diamond), Mark + FVector2D(-Diamond, 0.0f),
						Mark + FVector2D(0.0f, -Diamond)}, Card.Color, 1.3f, LayerId + 6);
				}
				else if (Card.RingRadius > 0.0f)
				{
					TArray<FVector2D> Ring;
					const int32 Segments = FMath::Clamp(FMath::CeilToInt(Card.RingRadius * 0.9f), 18, 72);
					Ring.Reserve(Segments + 1);
					for (int32 Point = 0; Point <= Segments; ++Point)
					{
						const float Angle = UE_TWO_PI * static_cast<float>(Point) / Segments;
						Ring.Add(Card.RingCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Card.RingRadius);
					}
					if (bSubPixelRings && APSShipPerf::IsDrawableHudLine(Ring))
					{
						// Unsnapped: Slate would round every vertex of a sub-pixel ring on its own and bend it frame to frame.
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 6, PaintGeometry, Ring,
							ESlateDrawEffect::NoPixelSnapping, Card.Color, true, Card.bSelected ? 1.6f : 1.1f);
					}
					else
					{
						DrawScreenLine(Ring, Card.Color, Card.bSelected ? 1.6f : 1.1f, LayerId + 6);
					}
				}
				if (Card.bPlaced && Card.Kind == APSNavigationHud::ECardKind::Contact)
				{
					LayoutFrame.Layouts.Add(Card.ContactIndex, TPair<FVector2D, FVector2D>(Card.Anchor, Card.Position));
				}
			}
		}
		TRACE_CPUPROFILER_EVENT_SCOPE(APS_NavHud_CardDraw);
		for (const APSNavigationHud::FCard& Card : Cards)
		{
			if (!Card.bPlaced)
			{
				continue;
			}
			const bool bUnit = Card.Kind == APSNavigationHud::ECardKind::Unit;
			const FLinearColor& Color = Card.Color;
			const FSlateRect CardBounds = APSNavigationHud::CardRect(Card.Position, Card.Size);
			// The accent bar faces the mark, so the pole or the leader meets it.
			const bool bAccentOnRight = Card.Anchor.X > (CardBounds.Left + CardBounds.Right) * 0.5f;
			FVector2D LeaderStart;
			FVector2D LeaderEnd;
			APSNavigationHud::LeaderSegment(Card, CardBounds, LeaderStart, LeaderEnd);
			if (FVector2D::DistSquared(LeaderStart, LeaderEnd) > 1.0)
			{
				DrawScreenLine({LeaderStart, LeaderEnd}, FLinearColor(Color.R, Color.G, Color.B,
					bUnit ? 0.55f : (Card.bSelected ? 0.82f : 0.42f)), bUnit ? 0.8f : (Card.bSelected ? 1.15f : 0.65f), LayerId + 2);
			}

			// Draw the flag and its pole in the same OnPaint pass. A separate ConstraintCanvas was laid out before the
			// post-physics camera update, so at high speed the card used an older projection than its pole.
			const FVector2f CardPosition(static_cast<float>(Card.Position.X), static_cast<float>(Card.Position.Y));
			const FVector2f CardSize(static_cast<float>(Card.Size.X), static_cast<float>(Card.Size.Y));
			const float Shade = bUnit ? 0.055f : 0.05f;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(
				CardSize, FSlateLayoutTransform(CardPosition)), CardBrush, ESlateDrawEffect::None,
				FLinearColor(Color.R * Shade, Color.G * Shade, Color.B * Shade, bUnit ? 0.74f : (Card.bSelected ? 0.95f : 0.88f)));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(
				FVector2f(3.0f, CardSize.Y), FSlateLayoutTransform(FVector2f(
					bAccentOnRight ? CardPosition.X + CardSize.X - 3.0f : CardPosition.X, CardPosition.Y))),
				CardBrush, ESlateDrawEffect::None, Color);
			const float TextLeft = CardPosition.X + (bAccentOnRight ? (bUnit ? 7.0f : 8.0f) : (bUnit ? 10.0f : 11.0f));
			const auto DrawCardText = [&](const FString& String, const float OffsetX, const float OffsetY,
				const FSlateFontInfo& Font, const FLinearColor& Tone)
			{
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5, AllottedGeometry.ToPaintGeometry(
					FVector2f(FMath::Max(CardSize.X - 12.0f, 1.0f), 16.0f),
					FSlateLayoutTransform(FVector2f(TextLeft + OffsetX, CardPosition.Y + OffsetY))),
					String, Font, ESlateDrawEffect::None, Tone);
			};
			if (bUnit)
			{
				const FLinearColor UnitText(FMath::Lerp(Color.R, 0.9f, 0.38f), FMath::Lerp(Color.G, 0.94f, 0.38f),
					FMath::Lerp(Color.B, 0.98f, 0.38f), 0.96f);
				const FLinearColor UnitLine(0.70f, 0.78f, 0.82f, 0.92f);
				DrawCardText(Card.DrawTitle, 0.0f, 2.0f, APSNavigationHud::UnitTitleFont(), UnitText);
				DrawCardText(Card.DrawDetail, 0.0f, 15.0f, APSNavigationHud::UnitLineFont(), UnitLine);
				if (!Card.MergedLine.IsEmpty())
				{
					DrawCardText(Card.MergedLine, 0.0f, 28.0f, APSNavigationHud::MergedFont(), UnitLine);
				}
				continue;
			}
			const FLinearColor TextColor(
				FMath::Lerp(Color.R, 0.93f, 0.55f),
				FMath::Lerp(Color.G, 0.96f, 0.55f),
				FMath::Lerp(Color.B, 0.99f, 0.55f), 1.0f);
			// Rio 02.10 ("the index stands too far from the name: first the index, one space, then the name"): the
			// designation goes in front of the name, in the marker colour ("PLANET  //  A5 FRAULHOLM").
			int32 NameAt = Card.Designation.IsEmpty() ? INDEX_NONE : Card.DrawTitle.Find(TEXT("//"));
			if (NameAt != INDEX_NONE)
			{
				NameAt += 2;
				while (NameAt < Card.DrawTitle.Len() && Card.DrawTitle[NameAt] == TEXT(' '))
				{
					++NameAt;
				}
				const FString Prefix = Card.DrawTitle.Left(NameAt);
				const float PrefixWidth = static_cast<float>(FontMeasure->Measure(Prefix, APSNavigationHud::MarkerFont()).X);
				const float DesignationWidth = static_cast<float>(
					FontMeasure->Measure(Card.Designation + TEXT(" "), APSNavigationHud::MarkerFont()).X);
				DrawCardText(Prefix, 0.0f, 5.0f, APSNavigationHud::MarkerFont(), TextColor);
				DrawCardText(Card.Designation, PrefixWidth, 5.0f, APSNavigationHud::MarkerFont(),
					FLinearColor(Color.R, Color.G, Color.B, 1.0f));
				DrawCardText(Card.DrawTitle.Mid(NameAt), PrefixWidth + DesignationWidth, 5.0f, APSNavigationHud::MarkerFont(),
					TextColor);
			}
			else
			{
				DrawCardText(Card.DrawTitle, 0.0f, 5.0f, APSNavigationHud::MarkerFont(), TextColor);
				if (!Card.Designation.IsEmpty())
				{
					const float TitleWidth = static_cast<float>(
						FontMeasure->Measure(Card.DrawTitle + TEXT(" "), APSNavigationHud::MarkerFont()).X);
					DrawCardText(Card.Designation, TitleWidth, 5.0f, APSNavigationHud::MarkerFont(),
						FLinearColor(Color.R, Color.G, Color.B, 1.0f));
				}
			}
			DrawCardText(Card.DrawDetail, 0.0f, 5.0f + ContactLine, APSNavigationHud::MarkerFont(), TextColor);
			if (!Card.MergedLine.IsEmpty())
			{
				DrawCardText(Card.MergedLine, 0.0f, 5.0f + 2.0f * ContactLine, APSNavigationHud::MergedFont(),
					FLinearColor(TextColor.R, TextColor.G, TextColor.B, 0.82f));
			}
		}

		// The course target's brackets or edge arrow, with its caption where the layout kept room for it.
		if (bTargetInView)
		{
			const double Arm = FMath::Clamp(TargetHalf * 0.42, 6.0, 18.0);
			for (const FVector2D& Corner : {FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0), FVector2D(-1.0, 1.0)})
			{
				const FVector2D Tip = TargetScreen + Corner * TargetHalf;
				DrawScreenLine({Tip - FVector2D(Corner.X * Arm, 0.0), Tip, Tip - FVector2D(0.0, Corner.Y * Arm)},
					TargetColor, 1.6f, LayerId + 7);
			}
		}
		else if (bTargetArrow)
		{
			const FVector2D Side(-ArrowDirection.Y, ArrowDirection.X);
			DrawScreenLine({ArrowTip - ArrowDirection * 16.0 + Side * 10.0, ArrowTip, ArrowTip - ArrowDirection * 16.0 - Side * 10.0},
				TargetColor, 2.2f, LayerId + 7);
		}
		if (bTargetInView || bTargetArrow)
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 7, AllottedGeometry.ToPaintGeometry(
				FVector2f(220.0f, 14.0f), FSlateLayoutTransform(FVector2f(static_cast<float>(CaptionAt.X),
					static_cast<float>(CaptionAt.Y)))),
				Caption, CaptionFont, ESlateDrawEffect::None, TargetColor);
		}
	}

	if (bAltimeterShown)
	{
		// A plate in the HUD's language: a dark panel with an accent bar that turns amber, then red, when the surface
		// comes up fast. The header names the body, the large figure is the height above its ground (or sea).
		const float Fade = Altimeter.Alpha;
		const float PanelX = AltimeterRect.Left;
		const float PanelY = AltimeterRect.Top;
		const float PanelWidth = AltimeterRect.Right - AltimeterRect.Left;
		const float PanelHeight = AltimeterRect.Bottom - AltimeterRect.Top;
		// Rio 06.10: the chrome tones follow the interface theme; caution and danger keep their meaning.
		const bool bClassicTheme = APSUITheme::Current() == EAPSUITheme::Classic;
		const FLinearColor Calm = bClassicTheme ? FLinearColor(0.18f, 0.84f, 1.0f, 1.0f) : APSUITheme::Palette().Highlight;
		const FLinearColor Caution(1.0f, 0.68f, 0.16f, 1.0f);
		const FLinearColor Danger(1.0f, 0.24f, 0.12f, 1.0f);
		const FLinearColor Bright = bClassicTheme ? FLinearColor(0.93f, 0.96f, 0.99f, 1.0f) : APSUITheme::Palette().Text;
		const FLinearColor Muted = bClassicTheme ? FLinearColor(0.62f, 0.76f, 0.86f, 1.0f) : APSUITheme::Palette().TextSoft;
		const float Alarm = FMath::Clamp(Altimeter.Severity, 0.0f, 1.0f);
		const FLinearColor State = FMath::Lerp(FMath::Lerp(Calm, Caution, Alarm), Danger,
			FMath::Clamp(Altimeter.Severity - 1.0f, 0.0f, 1.0f));
		const float Pulse = Altimeter.Severity > 1.2f ? 0.7f + 0.3f * static_cast<float>(FMath::Sin(NowSeconds * 9.0)) : 1.0f;
		const auto Tint = [Fade](const FLinearColor& Base, const float Opacity)
		{
			return FLinearColor(Base.R, Base.G, Base.B, Opacity * Fade);
		};
		const FSlateBrush* PlateBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const auto Fill = [&](const float FillX, const float FillY, const float FillWidth, const float FillHeight,
			const FLinearColor& Tone, const int32 Layer)
		{
			if (FillWidth > 0.0f && FillHeight > 0.0f)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(
					FVector2f(FillWidth, FillHeight), FSlateLayoutTransform(FVector2f(FillX, FillY))),
					PlateBrush, ESlateDrawEffect::None, Tone);
			}
		};
		const auto Write = [&](const FString& String, const float WriteX, const float WriteY, const FSlateFontInfo& Font,
			const FLinearColor& Tone)
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 6, AllottedGeometry.ToPaintGeometry(
				FVector2f(PanelWidth, 32.0f), FSlateLayoutTransform(FVector2f(WriteX, WriteY))),
				String, Font, ESlateDrawEffect::None, Tone);
		};
		const TSharedRef<FSlateFontMeasure> PlateMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FSlateFontInfo LabelFont = APSUITheme::BodyFont("Bold", 8);
		const FSlateFontInfo LineFont = APSUITheme::BodyFont("Bold", 9);
		const FSlateFontInfo ReadoutFont = APSUITheme::BodyFont("Bold", 20);
		const FSlateFontInfo ScaleFont = APSUITheme::BodyFont("Regular", 8);
		const FSlateFontInfo SmallFont = APSUITheme::BodyFont("Bold", 7);

		Fill(PanelX, PanelY, PanelWidth, PanelHeight, Tint(APSUITheme::Retint(FLinearColor(0.005f, 0.018f, 0.035f, 1.0f)), 0.84f), LayerId + 3);
		Fill(PanelX, PanelY, 3.0f, PanelHeight, Tint(State, 0.95f * Pulse), LayerId + 4);
		Write(TEXT("ALTITUDE"), PanelX + 13.0f, PanelY + 8.0f, LabelFont, Tint(Muted, 0.9f));
		const FString DesignationSuffix = Altimeter.Designation.IsEmpty() ? FString() : TEXT("  ") + Altimeter.Designation;
		const float SuffixWidth = DesignationSuffix.IsEmpty()
			? 0.0f : static_cast<float>(PlateMeasure->Measure(DesignationSuffix, LabelFont).X);
		const FString PlateBodyName = APSNavigationHud::FitText(Altimeter.BodyName, LabelFont,
			FMath::Max(PanelWidth - 90.0f - SuffixWidth, 24.0f), *PlateMeasure);
		const float NameWidth = static_cast<float>(PlateMeasure->Measure(PlateBodyName, LabelFont).X);
		const float NameX = PanelX + PanelWidth - 12.0f - NameWidth - SuffixWidth;
		Write(PlateBodyName, NameX, PanelY + 8.0f, LabelFont, Tint(Bright, 0.92f));
		if (!DesignationSuffix.IsEmpty())
		{
			Write(DesignationSuffix, NameX + NameWidth, PanelY + 8.0f, LabelFont, Tint(Altimeter.BodyColor, 1.0f));
		}
		const float Urgency = Altimeter.Severity > 1.2f ? Pulse : 1.0f;
		Write(APSNavigationHud::FormatAltitude(Altimeter.AltitudeCm), PanelX + 12.0f, PanelY + 21.0f, ReadoutFont,
			Tint(FMath::Lerp(Bright, State, Alarm), Urgency));

		// Vertical speed with its arrow, and the time to the surface at that rate.
		const double Vertical = Altimeter.VerticalCmPerSecond;
		const bool bDescending = Vertical < -50.0;
		const bool bClimbing = Vertical > 50.0;
		const float SpeedY = PanelY + 59.0f;
		const FLinearColor SpeedTone = bDescending ? Tint(FMath::Lerp(Bright, State, 0.65f), 1.0f) : Tint(Bright, 0.9f);
		const float ArrowLift = bDescending ? 4.0f : (bClimbing ? -4.0f : 0.0f);
		DrawScreenLine({FVector2D(PanelX + 14.0f, SpeedY + 7.0f - ArrowLift), FVector2D(PanelX + 19.0f, SpeedY + 7.0f + ArrowLift),
			FVector2D(PanelX + 24.0f, SpeedY + 7.0f - ArrowLift)}, SpeedTone, 1.8f, LayerId + 6);
		Write(bDescending ? TEXT("DESCENT  ") + APSNavigationHud::FormatVerticalSpeed(Vertical)
			: bClimbing ? TEXT("CLIMB  ") + APSNavigationHud::FormatVerticalSpeed(Vertical) : FString(TEXT("LEVEL")),
			PanelX + 30.0f, SpeedY, LineFont, SpeedTone);
		if (bDescending)
		{
			Write(TEXT("SURFACE IN  ") + APSNavigationHud::FormatSurfaceTime(Altimeter.AltitudeCm / -Vertical),
				PanelX + 13.0f, PanelY + 76.0f, LineFont, Tint(FMath::Lerp(Muted, State, Alarm), Urgency * 0.95f));
		}
		else
		{
			Write(TEXT("SURFACE IN  --"), PanelX + 13.0f, PanelY + 76.0f, LineFont, Tint(Muted, 0.6f));
		}

		// The tape: the surface at the bottom, the instrument's reach at the top, on a log scale so the last kilometres
		// open up. It fills from the top as the ship comes down; the dashes below the pointer are what is left.
		const float TapeTop = PanelY + 100.0f;
		const float TapeBottom = PanelY + PanelHeight - 24.0f;
		const float TrackX = PanelX + 30.0f;
		constexpr double ScaleFloorCm = 1000.0;
		const double ScaleTopCm = FMath::Max(Altimeter.TopCm, 1.0e6);
		const auto TapeY = [&](const double Centimetres)
		{
			const double Fraction = FMath::Clamp(FMath::Loge(1.0 + FMath::Max(Centimetres, 0.0) / ScaleFloorCm)
				/ FMath::Loge(1.0 + ScaleTopCm / ScaleFloorCm), 0.0, 1.0);
			return TapeBottom - static_cast<float>(Fraction) * (TapeBottom - TapeTop);
		};
		const float PointerY = TapeY(Altimeter.AltitudeCm);
		Fill(TrackX - 3.0f, TapeTop, 6.0f, TapeBottom - TapeTop, Tint(State, 0.12f), LayerId + 4);
		Fill(TrackX - 6.0f, TapeTop, 12.0f, PointerY - TapeTop, Tint(State, 0.12f), LayerId + 4);
		Fill(TrackX - 3.0f, TapeTop, 6.0f, PointerY - TapeTop, Tint(State, 0.8f), LayerId + 5);
		const float RemainingOpacity = (0.4f + 0.45f * Alarm) * Urgency;
		for (float DashY = PointerY + 5.0f; DashY < TapeBottom - 2.0f; DashY += 7.0f)
		{
			Fill(TrackX - 1.5f, DashY, 3.0f, FMath::Min(4.0f, TapeBottom - 2.0f - DashY), Tint(State, RemainingOpacity), LayerId + 5);
		}
		// Round altitudes, 100 m, 1 km ... up to the top of the tape.
		float LastLabelY = TNumericLimits<float>::Max();
		for (double RoundAltitude = 1.0e4; RoundAltitude < ScaleTopCm * 0.95; RoundAltitude *= 10.0)
		{
			const float TickY = TapeY(RoundAltitude);
			DrawScreenLine({FVector2D(TrackX + 5.0f, TickY), FVector2D(TrackX + 11.0f, TickY)}, Tint(Muted, 0.55f), 1.0f,
				LayerId + 5);
			if (FMath::Abs(LastLabelY - TickY) >= 12.0f && TickY > TapeTop + 5.0f && TickY < TapeBottom - 9.0f)
			{
				Write(APSNavigationHud::FormatTick(RoundAltitude), TrackX + 15.0f, TickY - 7.0f, ScaleFont, Tint(Muted, 0.8f));
				LastLabelY = TickY;
			}
		}
		if (Altimeter.AtmosphereCm > 0.0 && Altimeter.AtmosphereCm < ScaleTopCm)
		{
			// The edge of the atmosphere, dashed in sky blue across the track and beside the scale's labels.
			const FLinearColor Sky(0.45f, 0.78f, 1.0f, 1.0f);
			const float AtmosphereY = TapeY(Altimeter.AtmosphereCm);
			const float DashEnd = PanelX + PanelWidth - 14.0f;
			for (float DashX = TrackX - 8.0f; DashX < DashEnd; DashX += 6.0f)
			{
				if (DashX > TrackX + 12.0f && DashX < PanelX + 104.0f)
				{
					continue;
				}
				DrawScreenLine({FVector2D(DashX, AtmosphereY), FVector2D(FMath::Min(DashX + 3.0f, DashEnd), AtmosphereY)},
					Tint(Sky, 0.5f), 1.0f, LayerId + 5);
			}
			const FString AtmosphereLabel(TEXT("ATMOSPHERE"));
			const float AtmosphereLabelWidth = static_cast<float>(PlateMeasure->Measure(AtmosphereLabel, SmallFont).X);
			Write(AtmosphereLabel, PanelX + PanelWidth - 12.0f - AtmosphereLabelWidth, AtmosphereY - 12.0f, SmallFont,
				Tint(Sky, 0.85f));
		}
		// The ground: a firm line with a short hatch under the track.
		Fill(PanelX + 12.0f, TapeBottom, PanelWidth - 24.0f, 2.0f, Tint(State, 0.9f), LayerId + 5);
		for (float HatchX = PanelX + 16.0f; HatchX < TrackX + 12.0f; HatchX += 5.0f)
		{
			DrawScreenLine({FVector2D(HatchX, TapeBottom + 2.0f), FVector2D(HatchX - 4.0f, TapeBottom + 7.0f)},
				Tint(State, 0.45f), 1.0f, LayerId + 5);
		}
		Write(TEXT("SURFACE"), TrackX + 15.0f, TapeBottom + 4.0f, SmallFont, Tint(Muted, 0.8f));
		// The ship on the tape.
		DrawScreenLine({FVector2D(TrackX - 17.0f, PointerY - 6.0f), FVector2D(TrackX - 8.0f, PointerY),
			FVector2D(TrackX - 17.0f, PointerY + 6.0f), FVector2D(TrackX - 17.0f, PointerY - 6.0f)},
			Tint(FMath::Lerp(Bright, State, 0.55f), Urgency), 1.6f, LayerId + 6);
		DrawScreenLine({FVector2D(TrackX - 6.0f, PointerY), FVector2D(TrackX + 6.0f, PointerY)}, Tint(Bright, 1.0f), 2.0f,
			LayerId + 6);
	}

	// Rio 06.10 (flight path marker, aps.Ship.FlightPathMarker: "show where the ship really flies, whatever the camera
	// does"): a small ring with a centre dot where the velocity points, the nose at a standstill. It is a direction, so it is
	// projected from the camera a long way out: no parallax, the same at a walking pace and at light speed. When the nose
	// and the velocity part by more than 2 degrees, a faint tick marks the nose. Off screen (the mouse-look camera turned
	// away, flying backwards) it is simply not drawn; nothing else of the HUD changes.
	if (APSShipPerf::CVarFlightPathMarker.GetValueOnGameThread() != 0 && NavigationCameraManager
		&& (bNavigationMarkersVisible || bNavigationGuidesVisible))
	{
		const FVector PathVelocity = SpaceshipHull && SpaceshipHull->IsSimulatingPhysics()
			? SpaceshipHull->GetPhysicsLinearVelocity() : KinematicVelocity;
		const FVector Nose = GetShipForwardVector();
		const bool bUnderWay = PathVelocity.SizeSquared() > FMath::Square(100.0);
		const FVector PathDirection = bUnderWay ? PathVelocity.GetSafeNormal() : Nose;
		constexpr double MarkerReachCm = 1.0e7;
		const FLinearColor Quiet = APSUITheme::Palette().TextSoft;
		// Sub-pixel and unsnapped like the limb rings: the marker glides with the view instead of stepping by pixels.
		const auto DrawMarkerLines = [&](const TArray<FVector2D>& Points, const float Opacity, const float Thickness)
		{
			if (!APSShipPerf::IsDrawableHudLine(Points))
			{
				return;
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 6, PaintGeometry, Points, ESlateDrawEffect::NoPixelSnapping,
				FLinearColor(Quiet.R, Quiet.G, Quiet.B, Quiet.A * Opacity), true, Thickness);
		};
		FVector2D PathScreen;
		if (FVector::DotProduct(PathDirection, NavigationCameraForward) > 0.0
			&& ProjectWorldLocationToNavigationScreen(NavigationCameraLocation + PathDirection * MarkerReachCm, PathScreen,
				true, false))
		{
			constexpr int32 MarkerSegments = 24;
			constexpr double MarkerRadius = 7.0;
			TArray<FVector2D> Circle;
			Circle.Reserve(MarkerSegments + 1);
			for (int32 Point = 0; Point <= MarkerSegments; ++Point)
			{
				const double Angle = UE_DOUBLE_TWO_PI * static_cast<double>(Point) / MarkerSegments;
				Circle.Add(PathScreen + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * MarkerRadius);
			}
			DrawMarkerLines(Circle, 0.8f, 1.2f);
			// The centre dot: a short stroke as thick as it is long.
			DrawMarkerLines({PathScreen - FVector2D(0.9, 0.0), PathScreen + FVector2D(0.9, 0.0)}, 0.9f, 1.8f);
		}
		const double NosePartsDegrees = bUnderWay
			? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(PathDirection, Nose), -1.0, 1.0))) : 0.0;
		FVector2D NoseScreen;
		if (NosePartsDegrees > 2.0 && FVector::DotProduct(Nose, NavigationCameraForward) > 0.0
			&& ProjectWorldLocationToNavigationScreen(NavigationCameraLocation + Nose * MarkerReachCm, NoseScreen, true, false))
		{
			// Two short level dashes either side of the nose point.
			DrawMarkerLines({NoseScreen - FVector2D(9.0, 0.0), NoseScreen - FVector2D(4.0, 0.0)}, 0.45f, 1.0f);
			DrawMarkerLines({NoseScreen + FVector2D(4.0, 0.0), NoseScreen + FVector2D(9.0, 0.0)}, 0.45f, 1.0f);
		}
	}
	return LayerId + 7;
}

FLinearColor ASpaceship::GetNavigationMarkerColor(int32 ContactIndex) const
{
	if (ShipNavigation && ContactIndex == ShipNavigation->GetSelectedContactIndex())
	{
		return APSUITheme::Current() == EAPSUITheme::Classic ? FLinearColor(1.0f, 0.68f, 0.16f, 1.0f) : APSUITheme::Palette().ActionPeak;
	}
	const FShipNavigationContact* Contact = ShipNavigation ? ShipNavigation->GetContact(ContactIndex) : nullptr;
	if (!Contact) return FLinearColor::Transparent;
	if (Contact->bOwnColony)
	{
		return FLinearColor(0.36f, 1.0f, 0.58f, 0.98f);
	}
	if (Contact->MarkerColour.A > 0.0f)
	{
		return Contact->MarkerColour;
	}
	FLinearColor BodyColor;
	if (APSNavigationHud::BodyMarkerColor(Contact->Actor.Get(), BodyColor))
	{
		return BodyColor;
	}
	switch (Contact->Type)
	{
	case EShipNavigationContactType::Star: return FLinearColor(0.45f, 0.78f, 1.0f, 0.95f);
	case EShipNavigationContactType::Planet: return FLinearColor(0.18f, 0.92f, 0.74f, 0.94f);
	case EShipNavigationContactType::Moon: return FLinearColor(0.48f, 0.67f, 1.0f, 0.94f);
	case EShipNavigationContactType::Station:
	case EShipNavigationContactType::Settlement:
	case EShipNavigationContactType::Infrastructure: return FLinearColor(1.0f, 0.82f, 0.3f, 0.95f);
	default: return FLinearColor(0.72f, 0.82f, 0.9f, 0.9f);
	}
}

// Rio 06.10 (audit: the ship HUD drew over the colony terminal opened from the seat).
namespace APSShipHudLocal
{
	TAutoConsoleVariable<int32> CVarShipHudHideUnderTerminal(
		TEXT("aps.UI.ShipHudHideUnderTerminal"), 1,
		TEXT("1: the ship HUD collapses while the colony terminal (Tab/K) is open, as the walker HUD and the TASKS card do. ")
		TEXT("0: previous behaviour (only the F10 map hides it)."));
}

void ASpaceship::CreateShipHud()
{
	if (ShipHudWidget.IsValid() || !IsValid(Pilot) || !IsPlayerControlled()
		|| !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	if (ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}

	const TWeakObjectPtr<ASpaceship> WeakThis(this);
	TSharedRef<SOverlay> RootOverlay = SNew(SOverlay);
	RootOverlay->AddSlot()
	[
		SNew(SAPSShipNavigationOverlay)
		.Ship(WeakThis)
	];

	// Rio 06.10 (Claude UI, "Flight HUD v4"): the navigation card and the flight bar in the interface theme
	// (UI/Hud/SAPSShipHud) instead of the two text panels. The navigation cards keep clear of both (their real size).
	const TSharedRef<SAPSShipHud> Instruments = SNew(SAPSShipHud)
		.Ship(WeakThis)
		.ShowNavigation_Lambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->bNavigationPanelVisible; })
		.StatusText_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetShipStatusText() : FText::GetEmpty(); })
		.HintText_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetShipHintText() : FText::GetEmpty(); });
	RootOverlay->AddSlot()
	[
		Instruments
	];
	APSNavigationHud::GNavigationPanel = Instruments->GetNavigationCard();
	APSNavigationHud::GStatusPanel = Instruments->GetFlightBar();
	// The F10 map holds the view with its own camera: the ship's cards and panels would show through it (02.10).
	RootOverlay->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakThis]()
	{
		const AGravityPlayerController* Controller = WeakThis.IsValid()
			? Cast<AGravityPlayerController>(WeakThis->GetController()) : nullptr;
		if (Controller && Controller->IsStrategicMapOpen())
		{
			return EVisibility::Collapsed;
		}
		// Rio 06.10 (audit, aps.UI.ShipHudHideUnderTerminal): the colony terminal covers the view as the map does.
		if (APSShipHudLocal::CVarShipHudHideUnderTerminal.GetValueOnGameThread() != 0 && WeakThis.IsValid())
		{
			const UWorld* HudWorld = WeakThis->GetWorld();
			const UAPSColonyTerminalSubsystem* Terminal = HudWorld
				? HudWorld->GetSubsystem<UAPSColonyTerminalSubsystem>() : nullptr;
			if (Terminal && Terminal->IsTerminalOpen())
			{
				return EVisibility::Collapsed;
			}
		}
		return EVisibility::SelfHitTestInvisible;
	}));
	ShipHudWidget = RootOverlay;
	GEngine->GameViewport->AddViewportWidgetContent(ShipHudWidget.ToSharedRef(), 60);
}

void ASpaceship::RemoveShipHud()
{
	const bool bHadHud = ShipHudWidget.IsValid();
	if (bHadHud && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShipHudWidget.ToSharedRef());
	}
	ShipHudWidget.Reset();
	if (bHadHud)
	{
		// The next HUD starts afresh: no stale altimeter reading, no overlay size of a closed HUD.
		APSNavigationHud::GAltimeter = APSNavigationHud::FAltimeter();
		APSNavigationHud::GHudSize = FVector2D::ZeroVector;
	}
}

bool ASpaceship::ProvidesShipGravity() const
{
	return bProvidesArtificialGravity || (bHasInterior && !IsGroundVehicle() && !Cast<APlanetaryBody>(GetAttachParentActor()));
}

void ASpaceship::RefreshShipGravityZone()
{
	if (SphereCollisionComponent)
	{
		SphereCollisionComponent->SetCollisionEnabled(
			ProvidesShipGravity() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

namespace
{
	/** Rio 02.10: the mouse mode last chosen with C, for the session: a rover or a hover starts on the camera; the drone
	 * and the ships start on steering. */
	bool GMouseLookOnGround = true;
	bool GMouseLookInAir = false;
	/** Degrees of camera orbit per unit of the mouse axes, as the drone's mouse turns it. */
	constexpr double MouseLookDegreesPerUnit = 1.6;

	bool IsGroundMouseLookCraft(const ASpaceship& Ship)
	{
		return Ship.IsGroundVehicle() && Ship.GetGroundVehicleKind() != EAPSGroundVehicleKind::Drone;
	}
}

void ASpaceship::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// Rio 02.10: the mouse mode last chosen for this kind of craft, the camera behind it.
	bMouseLook = IsGroundMouseLookCraft(*this) ? GMouseLookOnGround : GMouseLookInAir;
	MouseLookYaw = 0.0;
	MouseLookPitch = 0.0;
	// Boarding unparked the ship (APilotingVehicle detaches it from its world): its gravity holds aboard from now on.
	RefreshShipGravityZone();
	SetHullSceneLightingExcluded(true);
	if (IsGroundVehicle() && !bEngineRunning)
	{
		// Rio 02.10: a vehicle starts when someone gets in; after they leave it parks and switches off by itself.
		bEngineRunning = true;
		ApplyEngineState();
	}
	if (FlightModel)
	{
		FlightModel->OnPossessed();
	}
	ConfigurePilotFillLight();
	UpdateFlightEnvironment(0.0f, true);
	UpdatePilotFillLightVisibility();
	// Rio 06.10 (collision by motion): a hull kept out for a walker is already on its proxy, which the call below leaves as
	// it is; its boxes block everything again for the flight and nothing waits for a rest any more.
	if (bHullRestorePending || bProxyBoxesPassWalkers)
	{
		SetProxyBoxesPassWalkers(false);
		bHullRestorePending = false;
		HullRestSeconds = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("[APS.Ships] %s is piloted again: its hull stayed out, the proxy boxes block all again"),
			*GetName());
	}
	SetFlightCollisionOptimization(true);
	StartHullMotionWatch();
	SetActorTickEnabled(true);
	CreateShipHud();
	UpdateCameraArmTicking();
}

void ASpaceship::UpdateCameraArmTicking()
{
	if (!SpringArmComponent)
	{
		return;
	}
	SpringArmComponent->SetComponentTickInterval(IsPlayerControlled() ? 0.0f
		: FMath::Max(APSShipPerf::CVarIdleCameraTickSeconds.GetValueOnGameThread(), 0.0f));
}

void ASpaceship::UnPossessed()
{
	RemoveShipHud();
	if (PilotFillLight)
	{
		PilotFillLight->SetVisibility(false, true);
	}
	if (PilotFillPointLight)
	{
		PilotFillPointLight->SetVisibility(false, true);
	}
	RestoreFlightPostProcess();
	SetHullSceneLightingExcluded(false);
	if (CameraComponent && bCameraFieldOfViewInitialized)
	{
		CameraComponent->SetFieldOfView(BaseCameraFieldOfView);
	}
	CameraArmLengthRate = SmoothedLogSpeedRate = 0.0f;
	PreviousCameraLogSpeed = -1.0;
	RefreshShipGravityZone();
	// The flight proxy boxes fill the hull; a pilot leaving a running ship would stand among them (and walk aboard
	// against them), so the hull's own collision comes back whenever the pilot leaves. Rio 06.10 (collision by motion,
	// aps.Ship.KeepHullOutWhileMoving): a detailed hull with a walk shell keeps its body out instead and the boxes let the
	// walker through; the body comes back once the ship has rested and the walker has stepped off (no 300-470 ms build
	// while getting up at speed).
	if (!KeepHullOutForWalker())
	{
		SetFlightCollisionOptimization(false);
	}
	bIsAccelerating = false;
	bIsDecelerating = false;
	ForwardInput = SideInput = VerticalInput = 0.0f;
	YawInput = PitchInput = RollInput = 0.0f;
	CurrentAngularVelocityDegrees = FVector::ZeroVector;
	Super::UnPossessed();
	// A ground vehicle keeps ticking until it has come to rest and parked itself (UAPSShipFlightModel::ParkVehicle).
	SetActorTickEnabled(bEngineRunning || IsGroundVehicle());
	UpdateCameraArmTicking();
}

void ASpaceship::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveShipHud();
	// Rio 06.10 (aps.Ship.HullRestoreOnce): the ship is going away; an UnPossessed from here on builds no body.
	bHullGoingAway = true;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HullMotionTimer);
	}
	if (InstancedResendHandle.IsValid())
	{
		FWorldDelegates::OnWorldPreSendAllEndOfFrameUpdates.Remove(InstancedResendHandle);
		InstancedResendHandle.Reset();
	}
	InstancedRiders.Reset();
	Super::EndPlay(EndPlayReason);
}

void ASpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ReleaseControl", IE_Pressed, this, &ASpaceship::ReleaseControl);
	PlayerInputComponent->BindAction("SwitchEngines", IE_Pressed, this, &ASpaceship::SwitchEngines);
	PlayerInputComponent->BindAxis("ThrustForward", this, &ASpaceship::ThrustForward);
	PlayerInputComponent->BindAxis("ThrustSide", this, &ASpaceship::ThrustSide);
	PlayerInputComponent->BindAxis("ThrustVertical", this, &ASpaceship::ThrustVertical);
	PlayerInputComponent->BindAxis("ThrustYaw", this, &ASpaceship::ThrustYaw);
	PlayerInputComponent->BindAxis("ThrustPitch", this, &ASpaceship::ThrustPitch);
	PlayerInputComponent->BindAxis("ThrustRoll", this, &ASpaceship::ThrustRoll);

	PlayerInputComponent->BindAction("IncreaseFlightMode", IE_Pressed, this, &ASpaceship::IncreaseFlightMode);
	PlayerInputComponent->BindAction("DecreaseFlightMode", IE_Pressed, this, &ASpaceship::DecreaseFlightMode);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Pressed, this, &ASpaceship::StartAccelerationBoost);
	PlayerInputComponent->BindAction("AccelerationBoost", IE_Released, this, &ASpaceship::StopAccelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Pressed, this, &ASpaceship::StartDecelerationBoost);
	PlayerInputComponent->BindAction("DecelerationBoost", IE_Released, this, &ASpaceship::StopDecelerationBoost);

	// Native bindings keep the sprint entirely C++-driven; no InputSettings or Blueprint edits are required.
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASpaceship::SelectImpulseEngine);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASpaceship::SelectSpaceWrapEngine);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASpaceship::SelectOffsetEngine);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ASpaceship::SelectCruiseBand);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ASpaceship::SelectStellarBand);
	PlayerInputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ASpaceship::SelectAutoBands);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &ASpaceship::ToggleNavigationMarkers);
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &ASpaceship::ToggleNavigationPanel);
	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ASpaceship::SelectNextNavigationTarget);
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ASpaceship::ToggleNavigationGuides);
	// Rio 04.10: Y labels the nearest stars (name, class, distance, what is known).
	PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ASpaceship::ToggleNearStarLabels);
	PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ASpaceship::ToggleAutopilot);
	// Rio 02.10: C switches the mouse between steering and the camera.
	PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ASpaceship::ToggleMouseLook);
	// Rio 02.10: J toggles the star drive (spool, cruise, W/S faster/slower).
	if (FlightModel)
	{
		PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, FlightModel, &UAPSShipFlightModel::ToggleStarDrive);
	}
	// Rio 02.10 (C14): B is the orbital build mode, hosted by its own component (not for the ground vehicles).
	if (!IsGroundVehicle())
	{
		UAPSShipBuildComponent* Build = FindComponentByClass<UAPSShipBuildComponent>();
		if (!Build)
		{
			Build = NewObject<UAPSShipBuildComponent>(this, TEXT("OrbitalBuild"));
			Build->RegisterComponent();
			AddInstanceComponent(Build);
		}
		PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, Build, &UAPSShipBuildComponent::Toggle);
	}
}

void ASpaceship::StartAccelerationBoost()
{
	bIsAccelerating = true;
}

void ASpaceship::StopAccelerationBoost()
{
	bIsAccelerating = false;
}

void ASpaceship::StartDecelerationBoost()
{
	bIsDecelerating = true;
}

void ASpaceship::StopDecelerationBoost()
{
	bIsDecelerating = false;
}

void ASpaceship::HandleAccelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->AccelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::HandleDecelerationBoost(float Value)
{
	if (Value > 0)
	{
		OnboardComputer->DecelerateBoost(GetWorld()->GetDeltaSeconds());
	}
}

void ASpaceship::IncreaseFlightMode()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->StepFlightBand(1);
		return;
	}
	if (!OnboardComputer) return;
	const uint8 Current = static_cast<uint8>(SelectedDriveMode);
	const uint8 Maximum = FMath::Min(
		static_cast<uint8>(GetMaximumDriveModeForClass()),
		static_cast<uint8>(GetMaximumDriveModeForEnvironment()));
	if (Current >= Maximum) return;
	SetDriveMode(static_cast<EShipDriveMode>(Current + 1), false);
}

void ASpaceship::DecreaseFlightMode()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->StepFlightBand(-1);
		return;
	}
	if (!OnboardComputer) return;
	const uint8 Current = static_cast<uint8>(SelectedDriveMode);
	if (Current <= static_cast<uint8>(EShipDriveMode::Landing)) return;
	SetDriveMode(static_cast<EShipDriveMode>(Current - 1), false);
}

void ASpaceship::SelectImpulseEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Maneuver);
		return;
	}
	SetEngineMode(EEngineMode::Impulse, false);
}

void ASpaceship::SelectSpaceWrapEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Flight);
		return;
	}
	SetEngineMode(EEngineMode::SpaceWrap, false);
}

void ASpaceship::SelectOffsetEngine()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Orbital);
		return;
	}
	SetEngineMode(EEngineMode::Offset, false);
}

void ASpaceship::SelectCruiseBand()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Cruise);
	}
}

void ASpaceship::SelectStellarBand()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetFlightBand(EAPSFlightBand::Stellar);
	}
}

void ASpaceship::SelectAutoBands()
{
	if (FlightModel && FlightModel->IsBandFlightActive())
	{
		FlightModel->SetAutoBands();
	}
}

bool ASpaceship::ApplyCustomFlightTranslation(float DeltaTime)
{
	return FlightModel && FlightModel->ApplyTranslation(DeltaTime);
}

FString ASpaceship::GetCustomFlightStatus() const
{
	return FlightModel ? FlightModel->GetStatusText() : FString();
}

FString ASpaceship::GetCustomFlightHint() const
{
	return FlightModel ? FlightModel->GetHintText() : FString();
}

void ASpaceship::ToggleNavigationMarkers()
{
	bNavigationMarkersVisible = !bNavigationMarkersVisible;
	if (bNavigationMarkersVisible && ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}
}

void ASpaceship::ToggleNavigationPanel()
{
	bNavigationPanelVisible = !bNavigationPanelVisible;
	if (bNavigationPanelVisible && ShipNavigation)
	{
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
	}
}

void ASpaceship::ToggleNavigationGuides()
{
	bNavigationGuidesVisible = !bNavigationGuidesVisible;
}

void ASpaceship::ToggleNearStarLabels()
{
	if (ShipNavigation)
	{
		ShipNavigation->bShowNearStarLabels = !ShipNavigation->bShowNearStarLabels;
		ShipNavigation->RefreshContacts(GetActorLocation(), true);
		int32 Labels = 0;
		for (const FShipNavigationContact& Contact : ShipNavigation->GetContacts())
		{
			Labels += Contact.bStarLabel ? 1 : 0;
		}
		UE_LOG(LogTemp, Log, TEXT("[APS.Nav] star labels %s: %d"), ShipNavigation->bShowNearStarLabels ? TEXT("on") : TEXT("off"),
			Labels);
	}
}

namespace APSShipNavigationCommands
{
	// The Y key from the console too (offscreen test runs hold no viewport focus for a key).
	FAutoConsoleCommandWithWorld StarLabelsCommand(TEXT("aps.Nav.StarLabels"),
		TEXT("Rio 04.10: the piloted ship's labels on the nearest stars on and off (the Y key)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			if (ASpaceship* Ship = Controller ? Cast<ASpaceship>(Controller->GetPawn()) : nullptr)
			{
				Ship->ToggleNearStarLabels();
			}
		}));
}

void ASpaceship::SelectNextNavigationTarget()
{
	if (ShipNavigation)
	{
		ShipNavigation->CycleTarget(1);
	}
}

void ASpaceship::SelectPreviousNavigationTarget()
{
	if (ShipNavigation)
	{
		ShipNavigation->CycleTarget(-1);
	}
}


void ASpaceship::ToggleScale()
{
	if (GeneratedWorld)
	{
		if (bIsScaledUp)
		{
			GeneratedWorld->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() / 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() / 1000000000.0);
			this->SetActorScale3D(FVector(0.01, 0.01, 0.01));
			/// this->SetSmallHullMesh
			//this->SpaceshipHull = SmallScaleHullMesh;
			//this->SpaceshipHull->SetStaticMesh(SmallScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 35;
			//this->CameraComponent->FieldOfView = 45;
			//this->CameraComponent->FOV

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1")));
		}
		else
		{
			GeneratedWorld->SetActorScale3D(FVector(1000000000.0, 1000000000.0, 1000000000.0));
			GeneratedWorld->SetActorLocation(GeneratedWorld->GetActorLocation() * 1000000000.0);
			this->SetActorLocation(this->GetActorLocation() * 1000000000.0);
			this->SetActorScale3D(FVector(1, 1, 1));
			/// this->SetBigHullMesh
			//this->SpaceshipHull->SetStaticMesh(LargeScaleHullMesh);
			this->SpringArmComponent->TargetArmLength = 1000;
			//this->CameraComponent->FieldOfView = 90;

			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Scaling to 1000000000")));
		}

		// ����������� ��������� ���������������
		bIsScaledUp = !bIsScaledUp;
	}
}


void ASpaceship::PrintOnboardComputerBasicIformation()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Safe Mode: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightSafeMode"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightSafeMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Range: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightRange"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightRange)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Status: %s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightStatus"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightStatus)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine State:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EEngineState"),
		                                                                       (int32)OnboardComputer->EngineSystem.
		                                                                       CurrentEngineState)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Engine Type:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EEngineMode"),
		                                                                       (int32)OnboardComputer->EngineSystem.
		                                                                       CurrentEngineMode)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Type:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightType"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightType)));
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Current Flight Mode:	%s"),
	                                                                       *OnboardComputer->GetEnumValueAsString(
		                                                                       TEXT("EFlightMode"),
		                                                                       (int32)OnboardComputer->FlightSystem.
		                                                                       CurrentFlightMode)));

	// Telemetry
	FVector ActorLocation = GetActorLocation();
	FVector ActorVelocity = GetVelocity();
	double ActorSpeed = ActorVelocity.Size();

	// Displaying them on screen
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(TEXT("Actor Location: %s"), *ActorLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Actor Velocity: %f"), ActorSpeed));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Thrust Force: %f"), OnboardComputer->GetEngineThrustForce()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Hull Angular Damping: %f"), SpaceshipHull->GetAngularDamping()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Hull Linear Damping: %f"), SpaceshipHull->GetLinearDamping()));
}

void ASpaceship::SwitchCamera()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("Camera")));
}

void ASpaceship::SwitchEngines()
{
	bEngineRunning = !bEngineRunning;
	if (bEngineRunning)
	{
		UpdateFlightEnvironment(0.0f, true);
		EnforceDriveModeForEnvironment();
		RequestEngineModeForFlightMode(true);
	}
	ApplyEngineState();
	SetActorTickEnabled(IsValid(Pilot) || bEngineRunning);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
	                                 FString::Printf(
		                                 TEXT("Engine running: %s"), bEngineRunning ? TEXT("true") : TEXT("false")));
}

void ASpaceship::ThrustForward(float Value)
{
	ForwardInput = Value;
}

void ASpaceship::ThrustSide(float Value)
{
	SideInput = Value;
	#if 0 // Legacy per-axis movement is intentionally replaced by ApplyFlightInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ForwardVector->GetRightVector();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// �������� StarSystem
		OffsetSystem->AddActorWorldOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse && SpaceshipHull->IsSimulatingPhysics())
	{
		// �������� ������ ������ �������.
		SpaceshipHull->AddForce(Direction * Value * OnboardComputer->GetEngineThrustForce(), NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime;
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
	#endif
}

void ASpaceship::ThrustVertical(float Value)
{
	VerticalInput = Value;
	#if 0 // Legacy per-axis movement is intentionally replaced by ApplyFlightInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const FVector Direction = ForwardVector->GetUpVector();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	if (OffsetSystem && OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap)
	{
		// �������� StarSystem
		OffsetSystem->AddActorWorldOffset(-Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse && SpaceshipHull->IsSimulatingPhysics())
	{
		// �������� ������ ������ �������.
		SpaceshipHull->AddForce(Direction * Value * OnboardComputer->GetEngineThrustForce(), NAME_None, true);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Offset)
	{
		const FVector Offset = Direction * Value * OnboardComputer->GetEngineThrustForce() * DeltaTime;
		SpaceshipHull->AddWorldOffset(Offset, true);
	}
	#endif
}

void ASpaceship::ThrustYaw(float Value)
{
	// Rio 02.10: in mouse look the mouse orbits the camera and steers nothing.
	if (IsMouseLookActive())
	{
		YawInput = 0.0f;
		if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
		{
			MouseLookYaw = FRotator::NormalizeAxis(MouseLookYaw + Value * MouseLookDegreesPerUnit);
			MouseLookIdleSeconds = 0.0;
		}
		return;
	}
	YawInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();
	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, RotationAmount, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetUpVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::ThrustPitch(float Value)
{
	if (IsMouseLookActive())
	{
		PitchInput = 0.0f;
		if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
		{
			// ThrustPitch is the mouse's Y turned over (DefaultInput scale -1): the mouse up looks up.
			MouseLookPitch = FMath::Clamp(MouseLookPitch - Value * MouseLookDegreesPerUnit, -40.0, 55.0);
			MouseLookIdleSeconds = 0.0;
		}
		return;
	}
	PitchInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();
	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(RotationAmount, 0, 0);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetRightVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::ThrustRoll(float Value)
{
	RollInput = Value;
	#if 0 // Legacy per-axis rotation is intentionally replaced by ApplyRotationInput.
	if (!bEngineRunning || !OnboardComputer || !SpaceshipHull || FMath::Abs(Value) < KINDA_SMALL_NUMBER) return;

	const float RotationAmount = Value * RotationSpeedDegreesPerSecond * GetWorld()->GetDeltaSeconds();

	if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::SpaceWrap || OnboardComputer->EngineSystem.
		CurrentEngineMode == EEngineMode::Offset)
	{
		FRotator NewRotation = FRotator(0, 0, RotationAmount);
		AddActorLocalRotation(NewRotation);
	}
	else if (OnboardComputer->EngineSystem.CurrentEngineMode == EEngineMode::Impulse)
	{
		FVector TorqueVector = ForwardVector->GetForwardVector() * Value * ImpulseRotationAcceleration;
		SpaceshipHull->AddTorqueInRadians(TorqueVector, NAME_None, true);
	}
	#endif
}

void ASpaceship::SetPilot(AGravityCharacterPawn* NewPilot)
{
}

USceneComponent* ASpaceship::GetPilotSeatComponent() const
{
	return PilotChair ? PilotChair : Super::GetPilotSeatComponent();
}

FTransform ASpaceship::GetPilotExitTransform() const
{
	// A ship with an interior gets its pilot up behind the seat, aboard: in flight, docked or landed (Rio 02.10: "F and he
	// is out at once, beside the ship: one cannot walk about the ship"); they leave it on foot through its ramp or door.
	// A ship without one (and a ground vehicle) sets them down through its authored exit outside: only a modelled cabin
	// counts (Rio 03.10, "if a ship has no interior yet, do not touch it"), not the class or its default flags.
	const FTransform AuthoredExit = PilotExitPoint ? PilotExitPoint->GetComponentTransform() : Super::GetPilotExitTransform();
	if (!PilotChair || !HasWalkableInterior())
	{
		return AuthoredExit;
	}
	const FVector Up = GetActorUpVector();
	const FTransform Seat = PilotChair->GetComponentTransform();
	const FVector Back = -FVector::VectorPlaneProject(Seat.GetUnitAxis(EAxis::X), Up).GetSafeNormal();
	return FTransform(Seat.GetRotation(), Seat.GetLocation() + Back * 120.0 + Up * 60.0);
}

namespace APSShipStandUp
{
	// Rio 09.10 (playtest 08.10 items 1-3: stuck in the hull, a corridor instead of the bridge, no gravity): the place behind
	// the seat was never checked (the engine's FindTeleportSpot in APilotingVehicle tests nothing while the pilot's collision is
	// still off), and two cabins have their seat socket facing aft (L_P1_15: the console is on its -X; S_P3_01: yaw 180 under a
	// nose at +X), so "behind the seat" was inside the console or the dash.
	TAutoConsoleVariable<int32> CVarSafeStandUp(
		TEXT("aps.Ship.SafeStandUp"), 1,
		TEXT("Rio 09.10 (playtest 08.10 items 1-3): 1: a pilot getting up in a ship with a cabin stands only on a deck of this ship ")
		TEXT("30-140 cm under the seat socket, in a free capsule, in sight of the seat, checked once the hull's own collision is ")
		TEXT("back: 120 cm aft of the seat (aft = away from the nearest Widget_* console socket, else toward the PilotExit socket, ")
		TEXT("else against the nose), then the PilotExit socket, then aft-left/right, then 200 cm aft; facing the seat. [APS.Seat] ")
		TEXT("logs the choice. 0: 120 cm behind the seat socket's X axis and 60 cm up, unchecked, as before."));

	struct FSpot
	{
		const TCHAR* Name;
		FVector Probe;
		// 09.10 (M_P2_02 on 0.6.4: the 1.2 m spot behind the seat failed the sight line, the pilot stood 2.5 m away at the
		// cabin exit): the place a pilot got up at before (the legacy transform) needs no sight line, only this deck and room.
		bool bNeedsSight{true};
	};
}

FTransform ASpaceship::ResolvePilotExitTransform(APawn& LeavingPilot, const FTransform& Proposed) const
{
	UWorld* World = GetWorld();
	const ACharacter* Walker = Cast<ACharacter>(&LeavingPilot);
	const UCapsuleComponent* Capsule = Walker ? Walker->GetCapsuleComponent() : nullptr;
	UPrimitiveComponent* Hull = GetPrimaryHullComponent();
	if (APSShipStandUp::CVarSafeStandUp.GetValueOnGameThread() == 0 || !World || !Capsule || !Hull || !PilotChair
		|| !HasWalkableInterior())
	{
		return Proposed;
	}
	const double Radius = Capsule->GetScaledCapsuleRadius();
	const double HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Up = GetActorUpVector();
	const FVector Seat = PilotChair->GetComponentLocation();

	// Aft of the seat: away from the nearest console socket the cabin tools put in front of the pilot, else toward the authored
	// cabin exit, else against the nose. The seat socket's own X is not trusted (L_P1_15 and S_P3_01 face aft).
	FVector Aft = FVector::ZeroVector;
	double ConsoleDistance = 400.0;
	for (const FName Socket : Hull->GetAllSocketNames())
	{
		if (Socket.ToString().StartsWith(TEXT("Widget_")))
		{
			const FVector ToConsole = FVector::VectorPlaneProject(Hull->GetSocketLocation(Socket) - Seat, Up);
			const double Distance = ToConsole.Size();
			if (Distance > 30.0 && Distance < ConsoleDistance)
			{
				ConsoleDistance = Distance;
				Aft = -ToConsole / Distance;
			}
		}
	}
	// The cabin tools author PilotExit as the standing capsule's centre just behind the seat; S_P3_01's is the ramp outside.
	static const FName CabinExitSocket(TEXT("PilotExit"));
	const bool bHasExitSocket = Hull->DoesSocketExist(CabinExitSocket);
	const FVector CabinExit = bHasExitSocket ? Hull->GetSocketLocation(CabinExitSocket) : Seat;
	const FVector ExitPlanar = FVector::VectorPlaneProject(CabinExit - Seat, Up);
	const bool bCabinExit = bHasExitSocket && ExitPlanar.Size() <= 400.0
		&& FMath::Abs(FVector::DotProduct(CabinExit - Seat, Up)) <= 150.0;
	if (Aft.IsNearlyZero() && bCabinExit && ExitPlanar.Size() > 30.0)
	{
		Aft = ExitPlanar.GetSafeNormal();
	}
	if (Aft.IsNearlyZero())
	{
		Aft = -FVector::VectorPlaneProject(GetShipForwardVector(), Up).GetSafeNormal();
	}
	if (Aft.IsNearlyZero())
	{
		return Proposed;
	}
	const FVector Side = FVector::CrossProduct(Up, Aft).GetSafeNormal();

	TArray<APSShipStandUp::FSpot, TInlineAllocator<6>> Spots;
	// Do no harm: where today's place stands on the seat's own deck in a free capsule, the pilot keeps getting up there.
	Spots.Add({TEXT("legacy"), Proposed.GetLocation(), false});
	Spots.Add({TEXT("aft120"), Seat + Aft * 120.0});
	if (bCabinExit)
	{
		Spots.Add({TEXT("cabinExit"), CabinExit});
	}
	Spots.Add({TEXT("aft120L"), Seat + Aft * 120.0 - Side * 90.0});
	Spots.Add({TEXT("aft120R"), Seat + Aft * 120.0 + Side * 90.0});
	Spots.Add({TEXT("aft200"), Seat + Aft * 200.0});

	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSStandUpSpot), false, &LeavingPilot);
	const FQuat CapsuleRotation = FQuat::FindBetweenNormals(FVector::UpVector, Up);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		static_cast<float>(FMath::Max(Radius - 2.0, 1.0)), static_cast<float>(FMath::Max(HalfHeight - 2.0, Radius)));
	// Over the backrest and under a 2.2 m cabin ceiling (the seat socket is 0.6-1.0 m over its deck).
	const FVector SeatSight = Seat + Up * 140.0;
	const FQuat Facing = FRotationMatrix::MakeFromXZ(-Aft, Up).ToQuat();
	const FTransform& ShipFrame = GetActorTransform();
	FString Tried;
	const TCHAR* FreeName = nullptr;
	FVector FreeCentre = FVector::ZeroVector;
	for (const APSShipStandUp::FSpot& Spot : Spots)
	{
		FHitResult Deck;
		const bool bDeckHit = World->LineTraceSingleByChannel(
			Deck, Spot.Probe + Up * 80.0, Spot.Probe - Up * 250.0, ECC_Pawn, Params);
		const double DeckBelowSeat = bDeckHit ? FVector::DotProduct(Seat - Deck.ImpactPoint, Up) : 0.0;
		// A walkable deck of this ship at the seat's own level.
		const bool bDeck = bDeckHit && !Deck.bStartPenetrating && Deck.GetActor() == this
			&& FVector::DotProduct(Deck.ImpactNormal, Up) >= 0.7 && DeckBelowSeat >= 30.0 && DeckBelowSeat <= 140.0;
		const FVector Centre = Deck.ImpactPoint + Up * (HalfHeight + 3.0);
		const bool bFree = bDeck && !World->OverlapBlockingTestByChannel(Centre, CapsuleRotation, ECC_Pawn, Shape, Params);
		FHitResult Wall;
		const bool bInSight = bFree && (!Spot.bNeedsSight || !World->LineTraceSingleByChannel(
			Wall, SeatSight, Centre + Up * (HalfHeight - Radius), ECC_Pawn, Params));
		Tried += FString::Printf(TEXT(" %s(deck=%s %.0fcm free=%d sight=%d%s%s)"), Spot.Name,
			bDeckHit ? *GetNameSafe(Deck.GetComponent()) : TEXT("none"), DeckBelowSeat, bFree ? 1 : 0, bInSight ? 1 : 0,
			Wall.bBlockingHit ? TEXT(" wall=") : TEXT(""), Wall.bBlockingHit ? *GetNameSafe(Wall.GetComponent()) : TEXT(""));
		if (bInSight)
		{
			UE_LOG(LogTemp, Log, TEXT("[APS.Seat] %s stand-up: spot=%s local=%s cm seatLocal=%s cm legacyLocal=%s cm;%s"),
				*GetName(), Spot.Name, *ShipFrame.InverseTransformPosition(Centre).ToCompactString(),
				*ShipFrame.InverseTransformPosition(Seat).ToCompactString(),
				*ShipFrame.InverseTransformPosition(Proposed.GetLocation()).ToCompactString(), *Tried);
			return FTransform(Spot.bNeedsSight ? Facing : Proposed.GetRotation(), Centre);
		}
		if (bFree && !FreeName)
		{
			FreeName = Spot.Name;
			FreeCentre = Centre;
		}
	}
	if (FreeName)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[APS.Seat] %s stand-up: no spot in sight of the seat; spot=%s local=%s cm (free, on the seat's deck);%s"),
			*GetName(), FreeName, *ShipFrame.InverseTransformPosition(FreeCentre).ToCompactString(), *Tried);
		return FTransform(Facing, FreeCentre);
	}
	UE_LOG(LogTemp, Warning, TEXT("[APS.Seat] %s stand-up: no free spot on the seat's deck; the old place legacyLocal=%s cm;%s"),
		*GetName(), *ShipFrame.InverseTransformPosition(Proposed.GetLocation()).ToCompactString(), *Tried);
	return Proposed;
}

void ASpaceship::ComputeProximity()
{
	FVector ShipLocation = this->GetActorLocation();
	double ClosestDistance = DBL_MAX;
	double ClosestAffectionDistance = DBL_MAX;
	CurrentZonesInfluence.Empty();

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
	                                 FString::Printf(TEXT("Navigatable proximity: %d"), WorldNavigatableActors.Num()));

	for (AWorldActor* Actor : WorldNavigatableActors)
	{
		FVector ActorLocation = Actor->GetActorLocation();
		double AffectionRadius = Actor->AffectionRadiusKM * 100000;
		double SurfaceRadius = Actor->RadiusKM * 100000;
		double DistanceToActor = (ActorLocation - ShipLocation).Size() - SurfaceRadius;
		double DistanceToAffectionZone = (ActorLocation - ShipLocation).Size();

		// ������� ���������� � ����������
		//FString DistanceMessage = FString::Printf(TEXT("Distance to actor %s is %f km"), *Actor->GetName(), DistanceToActor / 100000);
		//GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, DistanceMessage);

		// ���� ������� ��������� � ���� �������� ����� (�.�. ���������� ������ ��� ����� ����)
		if (DistanceToAffectionZone <= AffectionRadius)
		{
			CurrentZonesInfluence.Add(Actor);

			if (DistanceToAffectionZone < ClosestAffectionDistance)
			{
				ClosestAffectionDistance = DistanceToAffectionZone;
				AffectedActor = Actor;
			}
		}

		if (DistanceToActor < ClosestDistance)
		{
			ClosestDistance = DistanceToActor;
			ClosestActor = Actor;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
	                                 FString::Printf(TEXT("CurrentZonesInfluence: %d"), CurrentZonesInfluence.Num()));
	if (CurrentZonesInfluence.Num() > 0)
	{
		for (AWorldActor* Actor : CurrentZonesInfluence)
		{
			FString RadiusMessage = FString::Printf(TEXT("Ship is affected by %s"), *Actor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, RadiusMessage);
		}
		//OnboardComputer->ComputeFlightStatus(AffectedActor);
	}
	else
	{
		AffectedActor = nullptr;
	}
}

void ASpaceship::UpdateNavigatableActors()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("UpdateNavigatableActors!!!")));
}

void ASpaceship::CheckFlightModeChange()
{
	// ���� ����� ������ ���������
	if (OnboardComputer->FlightSystem.CurrentFlightMode != LastFlightMode)
	{
		// ��������� ������ �������
		UpdateNavigatableActors();

		// ��������� LastFlightMode
		LastFlightMode = OnboardComputer->FlightSystem.CurrentFlightMode;
	}
}

UStaticMeshComponent* ASpaceship::GetSpaceshipHull()
{
	return SpaceshipHull;
}

// ---------------------------------------------------------------------------------------------------------------------
// Ground vehicles (Rio 02.10: "a system of ground transport: a wheeled one, a hover one, and one that flies like a drone
// or a small helicopter"). The pawn stays an ASpaceship, so boarding, exit, camera and HUD are the ship's own.

namespace APSSpaceshipGroundVehicle
{
	/** One look of a vehicle: a static mesh, whether its nose points along the mesh's Y axis, and the vehicle's length. */
	struct FVehicleLook
	{
		const TCHAR* MeshPath;
		bool bNoseAlongY;
		double LengthCm;
	};

	// The rover is the Vehicle Template's offroad buggy, body and tyres as static meshes (no Chaos vehicle: it assumes
	// world -Z gravity). SciFiFlying and SpaceColonies hold no vehicle hulls (cockpit props, colony parts), so the hover
	// and the drone use small hulls of the generated ship pack, nose along +Y as in their ship Blueprints: the speeder
	// P1_06 hovers, the VTOL dropship P1_05 is the drone. A missing mesh falls back to the next one, then to a box.
	const FVehicleLook RoverLooks[] = {
		{TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Body.SM_Offroad_Body"), false, 420.0}};
	const FVehicleLook HoverLooks[] = {
		{TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_P1_06/SM_Spaceship_P1_06.SM_Spaceship_P1_06"), true, 560.0},
		{TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_XXS_P1_09/SM_Spaceship_XXS_P1_09.SM_Spaceship_XXS_P1_09"), true, 560.0}};
	const FVehicleLook DroneLooks[] = {
		{TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_P1_05/SM_Spaceship_P1_05.SM_Spaceship_P1_05"), true, 520.0},
		{TEXT("/Game/APS/APS_ALPHA/Assets/AI_Shpis/Pack_1/Spaceship_XXS_P1_22/SM_Spaceship_XXS_P1_22.SM_Spaceship_XXS_P1_22"), true, 520.0}};
	const TCHAR* const FallbackMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const OffroadTirePath = TEXT("/Game/Vehicles/OffroadCar/SM_Offroad_Tire.SM_Offroad_Tire");

	/** The buggy's tyres: the VisWheel bones of SKM_Offroad in the body mesh's space (its origin is on the ground). */
	struct FTireSlot
	{
		const TCHAR* Name;
		FVector Center;
		bool bLeft;
		bool bFront;
	};
	const FTireSlot OffroadTires[] = {
		{TEXT("VehicleTireFL"), FVector(168.3, -124.1, 51.1), true, true},
		{TEXT("VehicleTireFR"), FVector(168.3, 124.1, 51.1), false, true},
		{TEXT("VehicleTireBL"), FVector(-135.2, -139.8, 50.8), true, false},
		{TEXT("VehicleTireBR"), FVector(-135.2, 139.8, 50.8), false, false}};
	/** SM_Offroad_Tire: 51.2 cm radius around its centre, axle along Y, modelled as a right-side tyre. */
	constexpr double OffroadTireRadius = 51.2;
	/** The template's skeletal chassis: control arms, dampers, hubs and engine, skinned to its suspension bones. */
	const TCHAR* const OffroadSuspensionPath = TEXT("/Game/Vehicles/OffroadCar/SKM_Offroad.SKM_Offroad");

	/** Each tyre's suspension bones (FL, FR, BL, BR, the order of OffroadTires) at rest, parents first. */
	void CollectSuspensionBones(UPoseableMeshComponent& Mesh, TArray<ASpaceship::FGroundVehicleSuspensionBone>& OutBones)
	{
		using FBone = ASpaceship::FGroundVehicleSuspensionBone;
		OutBones.Reset();
		const auto Find = [&Mesh](const FString& Name, FVector& OutLocation)
		{
			if (Mesh.GetBoneIndex(FName(*Name)) == INDEX_NONE)
			{
				return false;
			}
			OutLocation = Mesh.GetBoneLocationByName(FName(*Name), EBoneSpaces::ComponentSpace);
			return true;
		};
		const TCHAR* const Suffixes[] = {TEXT("FL"), TEXT("FR"), TEXT("BL"), TEXT("BR")};
		for (int32 Wheel = 0; Wheel < 4; ++Wheel)
		{
			const FString Side = Suffixes[Wheel];
			FVector LowerRoot;
			FVector LowerEnd;
			const bool bLower = Find(TEXT("LowerControlArm_") + Side, LowerRoot) && Find(TEXT("LowerControlArm_End_") + Side, LowerEnd);
			const auto Add = [&](const FString& Name, const FBone::ERole Role, const FVector& Target, const FVector& Pivot)
			{
				const FName BoneName(*Name);
				const int32 Index = Mesh.GetBoneIndex(BoneName);
				if (Index == INDEX_NONE)
				{
					return;
				}
				FBone& Bone = OutBones.AddDefaulted_GetRef();
				Bone.Name = BoneName;
				Bone.Index = Index;
				Bone.Wheel = Wheel;
				Bone.Role = Role;
				Bone.bSteers = Wheel < 2 && Role == FBone::ERole::Hub;
				Bone.Rest = Mesh.GetBoneTransformByName(BoneName, EBoneSpaces::ComponentSpace);
				Bone.Target = Target;
				Bone.Pivot = Pivot;
				Bone.ArmRoot = LowerRoot;
				Bone.ArmEnd = LowerEnd;
			};
			for (const TCHAR* Hub : {TEXT("HUB_"), TEXT("HUB_Upper_"), TEXT("HUB_Upper_Mnt_"), TEXT("LowerControlArm_End_"),
				TEXT("UpperControlArm_End_"), TEXT("VisWheel_")})
			{
				Add(Hub + Side, FBone::ERole::Hub, FVector::ZeroVector, FVector::ZeroVector);
			}
			FVector UpperEnd;
			if (bLower)
			{
				Add(TEXT("LowerControlArm_") + Side, FBone::ERole::Arm, LowerEnd, FVector::ZeroVector);
			}
			if (Find(TEXT("UpperControlArm_End_") + Side, UpperEnd))
			{
				Add(TEXT("UpperControlArm_") + Side, FBone::ERole::Arm, UpperEnd, FVector::ZeroVector);
			}
			FVector Top;
			FVector Mount;
			if (bLower && Find(TEXT("SpringDamper_") + Side, Top) && Find(TEXT("SpringDamper_End_") + Side, Mount))
			{
				Add(TEXT("SpringDamper_") + Side, FBone::ERole::Damper, Mount, Top);
				Add(TEXT("SpringDamper_End_") + Side, FBone::ERole::DamperEnd, Mount, Top);
			}
		}
		OutBones.Sort([](const FBone& A, const FBone& B) { return A.Index < B.Index; });
	}
}

void ASpaceship::ConfigureAsGroundVehicle(const EAPSGroundVehicleKind Kind)
{
	using namespace APSSpaceshipGroundVehicle;
	GroundVehicleKind = Kind;
	GroundVehicleWheels.Reset();
	GroundVehicleSuspensionBones.Reset();
	if (UPoseableMeshComponent* OldSuspension = GroundVehicleSuspension.Get())
	{
		OldSuspension->DestroyComponent();
	}
	GroundVehicleSuspension.Reset();
	if (!IsGroundVehicle() || !SpaceshipHull)
	{
		return;
	}

	const TArrayView<const FVehicleLook> Looks = Kind == EAPSGroundVehicleKind::Rover ? MakeArrayView(RoverLooks)
		: Kind == EAPSGroundVehicleKind::Hover ? MakeArrayView(HoverLooks) : MakeArrayView(DroneLooks);
	UStaticMesh* BodyMesh = nullptr;
	bool bNoseAlongY = false;
	double LengthCm = 450.0;
	bool bOffroadBuggy = false;
	for (const FVehicleLook& Look : Looks)
	{
		BodyMesh = LoadObject<UStaticMesh>(nullptr, Look.MeshPath);
		if (BodyMesh)
		{
			bNoseAlongY = Look.bNoseAlongY;
			LengthCm = Look.LengthCm;
			bOffroadBuggy = Kind == EAPSGroundVehicleKind::Rover;
			break;
		}
		UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicles] %s: mesh %s is missing; trying the next look"),
			*GetGroundVehicleName(), Look.MeshPath);
	}
	if (!BodyMesh)
	{
		BodyMesh = LoadObject<UStaticMesh>(nullptr, FallbackMeshPath);
	}
	if (BodyMesh)
	{
		SpaceshipHull->SetStaticMesh(BodyMesh);
		const FBoxSphereBounds MeshBounds = BodyMesh->GetBounds();
		const double MeshLength = 2.0 * (bNoseAlongY ? MeshBounds.BoxExtent.Y : MeshBounds.BoxExtent.X);
		// The hull is the root: its scale is the vehicle's. Rover about 4.5 m with its tyres, hover 5.6 m, drone 5.2 m.
		SpaceshipHull->SetRelativeScale3D(FVector(MeshLength > 1.0 ? LengthCm / MeshLength : 1.0));
	}
	// Simple collision only: the meshes' own convex hulls on the root, which the kinematic moves sweep.
	SpaceshipHull->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	SpaceshipHull->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SpaceshipHull->SetSimulatePhysics(false);
	SpaceshipHull->SetEnableGravity(false);
	SpaceshipHull->SetCanEverAffectNavigation(false);
	if (ForwardVector)
	{
		ForwardVector->SetRelativeRotation(bNoseAlongY ? FRotator(0.0, 90.0, 0.0) : FRotator::ZeroRotator);
	}
	bUseAuthoredNoseDirection = true;
	FlightForwardLocalAxis = bNoseAlongY ? FVector::RightVector : FVector::ForwardVector;
	FlightUpLocalAxis = FVector::UpVector;
	SizeClass = ESpaceshipSizeClass::XXS;
	bInferSizeClassFromHull = false;
	bHasInterior = false;
	bProvidesArtificialGravity = false;
	bGenerateSimpleHullCollision = false;
	bOptimizeCollisionWhilePiloted = false;
	bAllowExteriorInteraction = true;
	AutoInteractionPadding = 120.0f;
	AutoExitClearance = 150.0f;
	// On the ground the star list is clutter: the markers (the colony among them) stay, M brings the list back.
	bNavigationPanelVisible = false;
	// The fleet, the maps and the journal name actors by their in-game name (FAPSFleetCommand::DisplayName).
	InGameName = FText::FromString(GetGroundVehicleName());
	Tags.AddUnique(TEXT("APS.Vehicle"));
	Tags.AddUnique(FName(*FString::Printf(TEXT("APS.Vehicle.%s"), *GetGroundVehicleName())));

	if (bOffroadBuggy)
	{
		if (UStaticMesh* TireMesh = LoadObject<UStaticMesh>(nullptr, OffroadTirePath))
		{
			for (const FTireSlot& Slot : OffroadTires)
			{
				UStaticMeshComponent* Tire = NewObject<UStaticMeshComponent>(this, FName(Slot.Name), RF_Transient);
				Tire->SetupAttachment(SpaceshipHull);
				Tire->SetMobility(EComponentMobility::Movable);
				Tire->SetStaticMesh(TireMesh);
				Tire->SetRelativeLocation(Slot.Center);
				// The template turns its left tyres around (their sockets); the mesh itself is a right-side tyre.
				Tire->SetRelativeRotation(Slot.bLeft ? FRotator(0.0, 180.0, 0.0) : FRotator::ZeroRotator);
				Tire->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Tire->SetCollisionResponseToAllChannels(ECR_Ignore);
				Tire->SetGenerateOverlapEvents(false);
				Tire->SetCanEverAffectNavigation(false);
				AddInstanceComponent(Tire);
				Tire->RegisterComponent();
				FGroundVehicleWheel& Wheel = GroundVehicleWheels.AddDefaulted_GetRef();
				Wheel.Tire = Tire;
				Wheel.LocalCenter = Slot.Center;
				Wheel.LocalRadius = OffroadTireRadius;
				Wheel.bLeft = Slot.bLeft;
				Wheel.bFront = Slot.bFront;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicles] ROVER: tyre mesh %s is missing; the body drives without tyres"),
				OffroadTirePath);
		}
		// Rio 03.10 ("the rover's wheels still hang in the air"): the template buggy keeps its control arms, dampers and
		// hubs in the skeletal chassis, not in SM_Offroad_Body, so the tyres stood 30-55 cm off the body with nothing
		// holding them. The chassis is posed after the tyres' travel and steering (UAPSShipFlightModel::PoseVehicle).
		if (USkeletalMesh* Chassis = LoadObject<USkeletalMesh>(nullptr, OffroadSuspensionPath))
		{
			UPoseableMeshComponent* Suspension = NewObject<UPoseableMeshComponent>(this, TEXT("VehicleSuspension"), RF_Transient);
			Suspension->SetupAttachment(SpaceshipHull);
			Suspension->SetMobility(EComponentMobility::Movable);
			Suspension->SetSkinnedAssetAndUpdate(Chassis);
			Suspension->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Suspension->SetCollisionResponseToAllChannels(ECR_Ignore);
			Suspension->SetGenerateOverlapEvents(false);
			Suspension->SetCanEverAffectNavigation(false);
			AddInstanceComponent(Suspension);
			Suspension->RegisterComponent();
			GroundVehicleSuspension = Suspension;
			CollectSuspensionBones(*Suspension, GroundVehicleSuspensionBones);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.Vehicles] ROVER: chassis %s is missing; the tyres stand without arms"),
				OffroadSuspensionPath);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Vehicles] %s configured: %s at scale %.2f, %d tyres, %d suspension bones"),
		*GetGroundVehicleName(), *GetNameSafe(SpaceshipHull->GetStaticMesh()), SpaceshipHull->GetRelativeScale3D().X,
		GroundVehicleWheels.Num(), GroundVehicleSuspensionBones.Num());
}

UPoseableMeshComponent* ASpaceship::GetGroundVehicleSuspension() const
{
	return GroundVehicleSuspension.Get();
}

FString ASpaceship::GetGroundVehicleName() const
{
	return APSGroundVehicle::KindName(GroundVehicleKind);
}

FQuat ASpaceship::GetActorRotationForFlightAxes(const FVector& Forward, const FVector& Up) const
{
	const FQuat LocalFrame = FRotationMatrix::MakeFromXZ(FlightForwardLocalAxis, FlightUpLocalAxis).ToQuat();
	const FQuat WorldFrame = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
	return (WorldFrame * LocalFrame.Inverse()).GetNormalized();
}

void ASpaceship::ConfigureGroundVehicleExit()
{
	UPrimitiveComponent* Hull = GetPrimaryHullComponent();
	FVector LocalMin;
	FVector LocalMax;
	if (!PilotExitPoint || !Hull || !GetPrimaryHullLocalBounds(Hull, LocalMin, LocalMax))
	{
		return;
	}
	const FTransform HullTransform = Hull->GetComponentTransform();
	const FVector Scale = Hull->GetComponentScale().GetAbs();
	const FVector Forward = GetShipForwardVector();
	const FVector Up = GetShipUpVector();
	const FVector Right = GetShipRightVector();
	// Half the width across the nose (a buggy's tyres stand out past its body), then the clearance beside it.
	const FVector LocalRight = HullTransform.InverseTransformVectorNoScale(Right).GetAbs();
	double HalfWidth = FVector::DotProduct((LocalMax - LocalMin) * 0.5 * Scale, LocalRight);
	for (const FGroundVehicleWheel& Wheel : GroundVehicleWheels)
	{
		HalfWidth = FMath::Max(HalfWidth, (FMath::Abs(Wheel.LocalCenter.Y) + 25.0) * Scale.Y);
	}
	const FVector Center = HullTransform.TransformPosition((LocalMin + LocalMax) * 0.5);
	// The vehicle's origin is on the ground (its wheels or skids): the driver stands up a metre above it.
	const FVector Location = GetActorLocation() + FVector::VectorPlaneProject(Center - GetActorLocation(), Up)
		- Right * (HalfWidth + AutoExitClearance) + Up * 100.0;
	PilotExitPoint->SetWorldTransform(FTransform(FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat(), Location));
}

void ASpaceship::UpdateGroundVehicleCamera()
{
	FVector Forward;
	FVector Up;
	double PitchDegrees = 0.0;
	if (!SpringArmComponent || !FlightModel || !FlightModel->GetVehicleCameraFrame(Forward, Up, PitchDegrees))
	{
		return;
	}
	// Behind the heading and level with the gravity, looking down 13 degrees (the drone's look pitch tilts it); the
	// arm's rotation lag smooths the bumps of the ground.
	// Rio 02.10: mouse look turns the view about the gravity up and tilts it (looking up lifts it).
	const FVector LookForward = FQuat(Up, FMath::DegreesToRadians(MouseLookYaw)).RotateVector(Forward);
	const FQuat Frame = FRotationMatrix::MakeFromXZ(LookForward, Up).ToQuat();
	const FQuat Tilt(FVector::RightVector, FMath::DegreesToRadians(13.0 - PitchDegrees - MouseLookPitch));
	SpringArmComponent->SetWorldRotation((Frame * Tilt).GetNormalized());
	SpringArmComponent->CameraRotationLagSpeed = 8.0f;
}

bool ASpaceship::HasWalkableInterior() const
{
	if (WalkableInteriorState < 0)
	{
		// An authored seat socket on a hull mesh marks a modelled cabin (the Blender interiors carry PilotSeat); the
		// generic "Seat"/"DriverSeat" names are left out, a vehicle's or a prop's socket is no cabin.
		static const FName SeatSockets[] = {TEXT("PilotSeat"), TEXT("PilotChair"), TEXT("CockpitSeat")};
		bool bCabin = false;
		if (!IsGroundVehicle())
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(this);
			for (const UStaticMeshComponent* Mesh : Meshes)
			{
				for (const FName Socket : SeatSockets)
				{
					bCabin |= IsValid(Mesh) && Mesh->GetStaticMesh() && Mesh->DoesSocketExist(Socket);
				}
			}
		}
		WalkableInteriorState = bCabin ? 1 : 0;
	}
	return WalkableInteriorState > 0;
}

bool ASpaceship::IsMouseLookActive() const
{
	return bMouseLook || (FlightModel && FlightModel->IsAutopilotEngaged());
}

void ASpaceship::ToggleMouseLook()
{
	bMouseLook = !bMouseLook;
	(IsGroundMouseLookCraft(*this) ? GMouseLookOnGround : GMouseLookInAir) = bMouseLook;
	YawInput = 0.0f;
	PitchInput = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[APS.Ship] %s: the mouse %s"), *GetNameSafe(this),
		bMouseLook ? TEXT("turns the camera") : TEXT("steers"));
}

void ASpaceship::UpdateMouseLook(const float DeltaTime)
{
	MouseLookIdleSeconds += DeltaTime;
	// Off, the camera eases back behind; on a rover or a hover driving on, it also comes back after the mouse rests.
	const bool bRecentre = !IsMouseLookActive() || (IsGroundMouseLookCraft(*this) && MouseLookIdleSeconds > 2.5
		&& KinematicVelocity.SizeSquared() > FMath::Square(500.0));
	if (bRecentre)
	{
		MouseLookYaw = FMath::FInterpTo(MouseLookYaw, 0.0, static_cast<double>(DeltaTime), 2.5);
		MouseLookPitch = FMath::FInterpTo(MouseLookPitch, 0.0, static_cast<double>(DeltaTime), 2.5);
	}
}

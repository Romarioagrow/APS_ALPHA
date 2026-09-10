#pragma once

#include "CoreMinimal.h"
#include "Layout/SlateRect.h"

/** Screen-space annotation only. Every input retains its physical marker even
 * when there is no room for a text box; the hierarchy remains the full browser. */
struct FAPSPreviewAnnotationCandidate
{
	int32 EntryIndex{INDEX_NONE};
	FVector2D Anchor{FVector2D::ZeroVector};
	bool bSelected{false};
	bool bPrimary{false};
};

struct FAPSPreviewAnnotationPlacement
{
	FAPSPreviewAnnotationCandidate Candidate;
	FVector2D LabelPosition{FVector2D::ZeroVector};
	bool bHasLabel{false};
};

namespace APSPreviewAnnotationLayout
{
inline FVector2D LabelSize() { return FVector2D(160.0, 36.0); }
constexpr double MaximumCoverage = 0.16;
constexpr double MaximumLeaderLength = 140.0;

/** Fit actual glyph widths, not a character count: wide generated names must not
 * spill into neighboring labels. The full name remains in the hierarchy. */
inline FString ElideText(const FString& Text, const double Width,
	TFunctionRef<double(const FString&)> Measure)
{
	if (Measure(Text) <= Width) return Text;
	const FString Suffix = TEXT("...");
	if (Measure(Suffix) > Width) return FString();
	int32 Low = 0, High = Text.Len();
	while (Low < High)
	{
		const int32 Middle = (Low + High + 1) / 2;
		if (Measure(Text.Left(Middle) + Suffix) <= Width) Low = Middle;
		else High = Middle - 1;
	}
	return Text.Left(Low).TrimEnd() + Suffix;
}

inline int32 LabelBudget(const FVector2D& PanelSize)
{
	return FMath::Clamp(FMath::FloorToInt(FMath::Max(0.0, PanelSize.X * PanelSize.Y)
		* MaximumCoverage / (LabelSize().X * LabelSize().Y)), 1, 24);
}

inline TArray<FAPSPreviewAnnotationPlacement> Arrange(
	TArray<FAPSPreviewAnnotationCandidate> Candidates, const FVector2D& PanelSize)
{
	Candidates.StableSort([](const FAPSPreviewAnnotationCandidate& A, const FAPSPreviewAnnotationCandidate& B)
	{
		const int32 APriority = A.bSelected ? 0 : A.bPrimary ? 1 : 2;
		const int32 BPriority = B.bSelected ? 0 : B.bPrimary ? 1 : 2;
		return APriority != BPriority ? APriority < BPriority : A.EntryIndex < B.EntryIndex;
	});
	const int32 Budget = LabelBudget(PanelSize);
	const FVector2D Size = LabelSize();
	// Reserve the explanation row even when a locally crowded sparse scene cannot
	// place all its labels. Otherwise that explanation could overlap a bottom label.
	const double Bottom = PanelSize.Y - 22.0;
	TArray<FSlateRect> Occupied;
	TArray<FAPSPreviewAnnotationPlacement> Placements;
	Placements.Reserve(Candidates.Num());
	const FVector2D Offsets[] = {
		FVector2D(-Size.X * 0.5, -Size.Y - 34.0),
		FVector2D(14.0, -Size.Y * 0.5),
		FVector2D(-Size.X - 14.0, -Size.Y * 0.5),
		FVector2D(-Size.X * 0.5, 18.0),
		FVector2D(-Size.X * 0.5, -Size.Y - 78.0),
		FVector2D(-Size.X * 0.5, 62.0)
	};
	for (const FAPSPreviewAnnotationCandidate& Candidate : Candidates)
	{
		FAPSPreviewAnnotationPlacement& Placement = Placements.AddDefaulted_GetRef();
		Placement.Candidate = Candidate;
		if (Occupied.Num() >= Budget || PanelSize.X < Size.X + 8.0 || Bottom < Size.Y + 4.0) continue;
		for (const FVector2D& Offset : Offsets)
		{
			const FVector2D Position(
				FMath::Clamp(Candidate.Anchor.X + Offset.X, 4.0, PanelSize.X - Size.X - 4.0),
				FMath::Clamp(Candidate.Anchor.Y + Offset.Y, 4.0, Bottom - Size.Y));
			const FVector2D PoleEnd = Position + FVector2D(Size.X * 0.5, Size.Y);
			if (!Candidate.bSelected && FVector2D::Distance(Candidate.Anchor, PoleEnd) > MaximumLeaderLength) continue;
			const FSlateRect Rect(Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y);
			const FSlateRect Padded(Rect.Left - 3.0f, Rect.Top - 3.0f, Rect.Right + 3.0f, Rect.Bottom + 3.0f);
			bool bOverlap = false;
			for (const FSlateRect& Existing : Occupied)
				if (FSlateRect::DoRectanglesIntersect(Padded, Existing)) { bOverlap = true; break; }
			if (bOverlap) continue;
			Placement.LabelPosition = Position;
			Placement.bHasLabel = true;
			Occupied.Add(Rect);
			break;
		}
	}
	return Placements;
}
}

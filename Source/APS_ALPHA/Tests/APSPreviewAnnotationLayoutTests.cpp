#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/UI/MainMenu/APSPreviewAnnotationLayout.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSPreviewAnnotationDensity,
	"APS.Preview.Coherence.AnnotationDensity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FAPSPreviewAnnotationDensity::RunTest(const FString& Parameters)
{
	for (const FVector2D Panel : {FVector2D(900, 740), FVector2D(1200, 1000), FVector2D(500, 300)})
	{
		TArray<FAPSPreviewAnnotationCandidate> Candidates;
		for (int32 Index = 0; Index < 125; ++Index)
			Candidates.Add({Index, Panel * 0.5 + FVector2D((Index % 11 - 5) * 9.0, (Index % 7 - 3) * 9.0), Index == 124, Index < 5});
		const auto Layout = APSPreviewAnnotationLayout::Arrange(Candidates, Panel);
		TestEqual(TEXT("Every body retains one marker in a dense system"), Layout.Num(), Candidates.Num());
		TestTrue(TEXT("The selected last body is placed before every other label"), Layout[0].Candidate.EntryIndex == 124 && Layout[0].bHasLabel);
		TSet<int32> Identities;
		TArray<FSlateRect> Rects;
		for (const FAPSPreviewAnnotationPlacement& Placement : Layout)
		{
			Identities.Add(Placement.Candidate.EntryIndex);
			TestTrue(TEXT("Decluttering never displaces a physical marker"), Placement.Candidate.Anchor.Equals(Candidates[Placement.Candidate.EntryIndex].Anchor, 0.0));
			if (!Placement.bHasLabel) continue;
			const FVector2D Position = Placement.LabelPosition, Size = APSPreviewAnnotationLayout::LabelSize();
			const FSlateRect Rect(Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y);
			TestTrue(TEXT("Text boxes remain inside the real panel, above the density explanation"),
				Rect.Left >= 4 && Rect.Top >= 4 && Rect.Right <= Panel.X - 4 && Rect.Bottom <= Panel.Y - 22);
			for (const FSlateRect& Existing : Rects)
				TestFalse(TEXT("Dense text boxes never overlap"), FSlateRect::DoRectanglesIntersect(Rect, Existing));
			Rects.Add(Rect);
			if (!Placement.Candidate.bSelected)
				TestTrue(TEXT("Ordinary leaders remain local instead of crossing the entire scene"),
					FVector2D::Distance(Placement.Candidate.Anchor, Position + FVector2D(Size.X * 0.5, Size.Y)) <= APSPreviewAnnotationLayout::MaximumLeaderLength);
		}
		TestEqual(TEXT("Marker identities are neither duplicated nor dropped"), Identities.Num(), Candidates.Num());
		TestTrue(TEXT("Text uses at most sixteen percent of the panel"), Rects.Num() * 160.0 * 36.0 <= Panel.X * Panel.Y * 0.16);
		const auto Repeated = APSPreviewAnnotationLayout::Arrange(Candidates, Panel);
		for (int32 Index = 0; Index < Layout.Num(); ++Index)
			TestTrue(TEXT("Repeated paints retain deterministic label placement"), Layout[Index].bHasLabel == Repeated[Index].bHasLabel
				&& Layout[Index].LabelPosition.Equals(Repeated[Index].LabelPosition, 0.0));
	}
	TArray<FAPSPreviewAnnotationCandidate> Sparse;
	for (int32 Index = 0; Index < 8; ++Index)
		Sparse.Add({Index, FVector2D(130 + (Index % 4) * 215, 200 + (Index / 4) * 250), false, Index == 0});
	const auto SparseLayout = APSPreviewAnnotationLayout::Arrange(Sparse, FVector2D(950, 740));
	for (const FAPSPreviewAnnotationPlacement& Placement : SparseLayout)
		TestTrue(TEXT("A sparse system retains every full label"), Placement.bHasLabel);
	const auto Tiny = APSPreviewAnnotationLayout::Arrange(Sparse, FVector2D(100, 30));
	TestEqual(TEXT("Even an undersized panel retains every marker identity"), Tiny.Num(), Sparse.Num());
	for (const FAPSPreviewAnnotationPlacement& Placement : Tiny)
		TestFalse(TEXT("An undersized panel does not emit overflowing text boxes"), Placement.bHasLabel);
	if (!TestTrue(TEXT("Real Slate font measurement is available"), FSlateApplication::IsInitialized())) return false;
	const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	for (const FSlateFontInfo Font : {FCoreStyle::GetDefaultFontStyle("Bold", 10), FCoreStyle::GetDefaultFontStyle("Regular", 8)})
	{
		const auto Measure = [&](const FString& Text) { return static_cast<double>(FontMeasure->Measure(Text, Font).X); };
		for (const FString Text : {FString(TEXT("P57 / DAFULEPO PLANET")), FString(TEXT("P02 / LONESOBO PLANET")),
			FString(TEXT("P120 / WWWWWWWWWWWWWWWWWWWW PLANET")), FString(TEXT("ICE GIANT / 33825 KM"))})
		{
			const FString Fitted = APSPreviewAnnotationLayout::ElideText(Text, 148.0, Measure);
			TestTrue(TEXT("Actual rendered glyphs fit the text-box interior"), Measure(Fitted) <= 148.0);
			if (Measure(Text) <= 148.0) TestEqual(TEXT("Already-fitting names remain unchanged"), Fitted, Text);
			else TestTrue(TEXT("Truncated names explicitly indicate elision"), Fitted.EndsWith(TEXT("...")));
			AddInfo(FString::Printf(TEXT("Annotation font fit size=%d sourceWidth=%.3f resultWidth=%.3f text=%s"),
				Font.Size, Measure(Text), Measure(Fitted), *Text));
		}
	}
	return true;
}

#endif

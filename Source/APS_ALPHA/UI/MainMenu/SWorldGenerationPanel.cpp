#include "SWorldGenerationPanel.h"

#include "APS_ALPHA/Core/Enums/AstroGenerationLevel.h"
#include "APS_ALPHA/Core/Enums/GalaxyClass.h"
#include "APS_ALPHA/Core/Enums/GalaxyType.h"
#include "APS_ALPHA/Core/Enums/OrbitDistributionType.h"
#include "APS_ALPHA/Core/Enums/PlanetarySystemType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "APS_ALPHA/Core/Enums/StarClusterSize.h"
#include "APS_ALPHA/Core/Enums/StarClusterType.h"
#include "APS_ALPHA/Core/Enums/StarSpectralClass.h"
#include "APS_ALPHA/Core/Enums/StellarType.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Actors/Astro/PlanetOrbit.h"
#include "APS_ALPHA/Actors/Astro/Star.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WorldGenerationPanel"

namespace APSGenerationUI
{
	class SGenerationChamferedFrame final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGenerationChamferedFrame) {}
			SLATE_ARGUMENT(FLinearColor, Color)
			SLATE_ARGUMENT(float, Thickness)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Color = InArgs._Color;
			Thickness = InArgs._Thickness;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
			const FSlateRect&, FSlateWindowElementList& OutDrawElements, int32 LayerId,
			const FWidgetStyle&, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X <= 4.0f || Size.Y <= 4.0f)
			{
				return LayerId;
			}
			const float Cut = FMath::Clamp(FMath::Min(Size.X, Size.Y) * 0.025f, 9.0f, 18.0f);
			const TArray<FVector2D> Points = {
				FVector2D(Cut, 0.5f), FVector2D(Size.X - Cut, 0.5f),
				FVector2D(Size.X - 0.5f, Cut), FVector2D(Size.X - 0.5f, Size.Y - Cut),
				FVector2D(Size.X - Cut, Size.Y - 0.5f), FVector2D(Cut, Size.Y - 0.5f),
				FVector2D(0.5f, Size.Y - Cut), FVector2D(0.5f, Cut), FVector2D(Cut, 0.5f)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
				Points, ESlateDrawEffect::None, Color, true, Thickness);
			return LayerId;
		}

	private:
		FLinearColor Color{FLinearColor::White};
		float Thickness{1.0f};
	};

	// The astronomical preview is the real level viewport. Keep the root chrome
	// fully transparent so no Slate wash/vignette changes the scene exposure or
	// hides small galaxy/cluster instances in the middle of the screen.
	const FLinearColor Background(0.0f, 0.0f, 0.0f, 0.0f);
	const FLinearColor Panel(0.008f, 0.030f, 0.047f, 0.91f);
	const FLinearColor Cyan(0.12f, 0.82f, 1.0f, 1.0f);
	const FLinearColor CyanDim(0.05f, 0.28f, 0.39f, 1.0f);
	const FLinearColor Amber(1.0f, 0.56f, 0.04f, 1.0f);
	const FLinearColor White(0.92f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.46f, 0.61f, 0.69f, 1.0f);
	TWeakObjectPtr<UFont> DisplayFont;
	TWeakObjectPtr<UFont> BodyFont;
	const FSlateRoundedBoxBrush PanelBrush(Panel, 10.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush ControlBrush(FLinearColor(0.003f, 0.016f, 0.028f, 0.94f), 6.0f, CyanDim, 1.0f);
	const FSlateRoundedBoxBrush BadgeBrush(FLinearColor(0.005f, 0.045f, 0.070f, 0.98f), 16.0f, Cyan, 1.0f);

	FSlateFontInfo Font(const FName Typeface, int32 Size)
	{
		if (!DisplayFont.IsValid())
		{
			DisplayFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Bold_Font.Orbitron_Bold_Font"));
			BodyFont = LoadObject<UFont>(nullptr, TEXT("/Game/APS/APS_ALPHA/UI/Fonts/Orbitron_Medium_Font.Orbitron_Medium_Font"));
		}
		if (UFont* FontObject = (Typeface == TEXT("Bold") ? DisplayFont.Get() : BodyFont.Get()))
		{
			return FSlateFontInfo(FontObject, Size, Typeface);
		}
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	FButtonStyle MakeButtonStyle(const FLinearColor& Outline, const FLinearColor& Fill)
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Fill, 6.0f, Outline, 1.0f))
			.SetHovered(FSlateRoundedBoxBrush(FLinearColor(Fill.R + 0.025f, Fill.G + 0.05f, Fill.B + 0.07f, 0.98f), 6.0f, Cyan, 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.14f, 0.20f, 1.0f), 6.0f, Cyan, 1.5f));
	}

	const FButtonStyle SecondaryButton = MakeButtonStyle(CyanDim, FLinearColor(0.003f, 0.022f, 0.038f, 0.94f));
	const FButtonStyle PrimaryButton = MakeButtonStyle(Amber, FLinearColor(0.30f, 0.12f, 0.004f, 0.96f));

	TSharedRef<SWidget> ChamferPanel(TSharedRef<SWidget> Content)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(&PanelBrush).Padding(16.0f)[Content]
			]
			+ SOverlay::Slot()
			[
				SNew(SGenerationChamferedFrame).Color(Cyan).Thickness(1.15f)
			];
	}

	TArray<int64> GetSelectableEnumValues(const UEnum* Enum)
	{
		TArray<int64> Values;
		if (!Enum)
		{
			return Values;
		}
		for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
		{
			const FString Name = Enum->GetNameStringByIndex(Index);
			if (Name.Equals(TEXT("Unknown"), ESearchCase::IgnoreCase)
				|| Name.EndsWith(TEXT("_Unknown"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			Values.Add(Enum->GetValueByIndex(Index));
		}
		return Values;
	}

	TSharedRef<SWidget> SectionTitle(const FText& Text)
	{
		const FString Upper = Text.ToString().ToUpper();
		const FText Glyph = FText::FromString(
			Upper.Contains(TEXT("ATMOSPHERE")) ? TEXT("ATM") :
			Upper.Contains(TEXT("SYSTEM")) ? TEXT("SYS") :
			Upper.Contains(TEXT("CLUSTER")) ? TEXT("CL") :
			Upper.Contains(TEXT("GALAXY")) ? TEXT("GX") : TEXT("AST"));
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(30.0f).HeightOverride(30.0f)
				[SNew(SBorder).BorderImage(&BadgeBrush).Padding(0.0f)[SNew(STextBlock).Text(Glyph).Justification(ETextJustify::Center).Font(Font("Bold", 8)).ColorAndOpacity(Cyan)]]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(10.0f, 0.0f)
			[SNew(STextBlock).Text(Text).Font(Font("Bold", 16)).ColorAndOpacity(Cyan)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(CyanDim)]];
	}

	template <typename TEnum, typename TGetter>
	TSharedRef<SWidget> EnumRow(const FText& Label, TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font("Regular", 11)).ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("<"))).ContentPadding(FMargin(9.0f, 4.0f))
					.ButtonStyle(&SecondaryButton)
					.OnClicked_Lambda([ViewModel, Getter, Enum]()
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							const TArray<int64> Values = GetSelectableEnumValues(Enum);
							if (!Values.IsEmpty())
							{
								const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
								const int32 CurrentIndex = Values.IndexOfByKey(Current);
								const int32 NewIndex = CurrentIndex == INDEX_NONE
									? Values.Num() - 1 : (CurrentIndex - 1 + Values.Num()) % Values.Num();
								VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
							}
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([ViewModel, Getter, Enum]()
					{
						if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							return Enum->GetDisplayNameTextByValue(static_cast<int64>(Getter(VM->GeneratedWorld)));
						}
						return FText::FromString(TEXT("--"));
					})
					.Font(Font("Bold", 11)).ColorAndOpacity(White)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT(">"))).ContentPadding(FMargin(9.0f, 4.0f))
					.ButtonStyle(&SecondaryButton)
					.OnClicked_Lambda([ViewModel, Getter, Enum]()
					{
						if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld && Enum)
						{
							const TArray<int64> Values = GetSelectableEnumValues(Enum);
							if (!Values.IsEmpty())
							{
								const int64 Current = static_cast<int64>(Getter(VM->GeneratedWorld));
								const int32 CurrentIndex = Values.IndexOfByKey(Current);
								const int32 NewIndex = CurrentIndex == INDEX_NONE
									? 0 : (CurrentIndex + 1) % Values.Num();
								VM->SetEnumValue(Enum, static_cast<int32>(Values[NewIndex]));
							}
						}
						return FReply::Handled();
					})
				]
				]
			];
	}

	template <typename TValue, typename TGetter, typename TSetter>
	TSharedRef<SWidget> NumberRow(const FText& Label, TValue Min, TValue Max, TValue Delta,
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel, TGetter Getter, TSetter Setter)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(Font("Regular", 11)).ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 9.0f)
			[
				SNew(SBorder).BorderImage(&ControlBrush).Padding(2.0f)
				[
				SNew(SSpinBox<TValue>)
				.MinValue(Min).MaxValue(Max).Delta(Delta)
				.Value_Lambda([ViewModel, Getter]()
				{
					if (const UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->GeneratedWorld)
					{
						return static_cast<TValue>(Getter(VM->GeneratedWorld));
					}
					return TValue{};
				})
				.OnValueChanged_Lambda([ViewModel, Setter](TValue Value)
				{
					if (UWorldGenerationViewModel* VM = ViewModel.Get()) Setter(VM, Value);
				})
				]
			];
	}

	class SPreviewInteractionSurface final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreviewInteractionSurface) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ViewModel = InArgs._ViewModel;
			ChildSlot
			[
				// Intentionally empty: this hit-test surface sits over the real scene.
				// No translucent card, labels or fake render target may cover the world.
				SNew(SBox)
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() == EKeys::RightMouseButton || Event.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
		{
			if (HasMouseCapture()) return FReply::Handled().ReleaseMouseCapture();
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent& Event) override
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get(); VM && VM->FocusPreviewUnderCursor())
				{
					return FReply::Handled();
				}
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override
		{
			if (HasMouseCapture())
			{
				if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->OrbitPreview(Event.GetCursorDelta());
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override
		{
			if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->ZoomPreview(Event.GetWheelDelta());
			return FReply::Handled();
		}

	private:
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	};

	class SPreviewSystemOverlay final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPreviewSystemOverlay) {}
			SLATE_ARGUMENT(TWeakObjectPtr<UWorldGenerationViewModel>, ViewModel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ViewModel = InArgs._ViewModel;
			SetVisibility(EVisibility::HitTestInvisible);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(100.0f, 100.0f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const UWorldGenerationViewModel* VM = ViewModel.Get();
			if (!VM || !VM->bPreviewReady)
			{
				return LayerId;
			}
			const EAstroPreviewFocus Focus = VM->GetPreviewFocus();
			if (Focus != EAstroPreviewFocus::HomeSystem && Focus != EAstroPreviewFocus::HomeStar
				&& Focus != EAstroPreviewFocus::HomePlanet)
			{
				return LayerId;
			}

			UWorld* World = VM->GetWorld();
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			if (!PC || !GEngine || !GEngine->GameViewport)
			{
				return LayerId;
			}

			int32 ViewWidth = 0;
			int32 ViewHeight = 0;
			PC->GetViewportSize(ViewWidth, ViewHeight);
			const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
			if (!ViewportWidget.IsValid() || ViewWidth <= 0 || ViewHeight <= 0)
			{
				return LayerId;
			}
			const FGeometry ViewportGeometry = ViewportWidget->GetCachedGeometry();
			const FVector2D ViewportLocalSize = ViewportGeometry.GetLocalSize();
			const FVector2D PanelSize = AllottedGeometry.GetLocalSize();

			const auto ProjectToPanel = [&](const FVector& WorldPosition, FVector2D& OutPanelPosition)
			{
				FVector2D ScreenPosition;
				if (!PC->ProjectWorldLocationToScreen(WorldPosition, ScreenPosition, true))
				{
					return false;
				}
				const FVector2D ViewportLocal(
					ScreenPosition.X * ViewportLocalSize.X / static_cast<float>(ViewWidth),
					ScreenPosition.Y * ViewportLocalSize.Y / static_cast<float>(ViewHeight));
				OutPanelPosition = AllottedGeometry.AbsoluteToLocal(ViewportGeometry.LocalToAbsolute(ViewportLocal));
				return FMath::IsFinite(OutPanelPosition.X) && FMath::IsFinite(OutPanelPosition.Y);
			};

			TArray<FAPSPreviewBodyEntry> Entries;
			VM->GetPreviewBodyEntries(Entries);
			if (Entries.IsEmpty())
			{
				return LayerId;
			}

			// World-space orbit planes are sampled and projected every paint. They remain
			// locked to the generated actors while the preview camera orbits and zooms.
			for (const FAPSPreviewBodyEntry& Entry : Entries)
			{
				const AActor* Body = Entry.Actor.Get();
				const APlanetOrbit* Orbit = Body ? Cast<APlanetOrbit>(Body->GetAttachParentActor()) : nullptr;
				if (!Body || !Orbit)
				{
					continue;
				}
				const FVector Center = Orbit->GetActorLocation();
				const double Radius = FVector::Distance(Center, Body->GetActorLocation());
				if (!FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER)
				{
					continue;
				}
				const FVector AxisX = Orbit->GetActorQuat().GetAxisX();
				const FVector AxisY = Orbit->GetActorQuat().GetAxisY();
				TArray<FVector2D> Segment;
				Segment.Reserve(65);
				for (int32 Sample = 0; Sample <= 64; ++Sample)
				{
					const double Angle = UE_TWO_PI * static_cast<double>(Sample) / 64.0;
					FVector2D Point;
					if (ProjectToPanel(Center + (AxisX * FMath::Cos(Angle) + AxisY * FMath::Sin(Angle)) * Radius, Point)
						&& Point.X > -PanelSize.X && Point.X < PanelSize.X * 2.0f
						&& Point.Y > -PanelSize.Y && Point.Y < PanelSize.Y * 2.0f)
					{
						Segment.Add(Point);
					}
					else if (Segment.Num() > 1)
					{
						FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
							AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
							Entry.Depth > 1 ? FLinearColor(0.36f, 0.65f, 1.0f, 0.22f)
								: FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.30f), true, Entry.Depth > 1 ? 0.65f : 1.0f);
						Segment.Reset();
					}
				}
				if (Segment.Num() > 1)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
						AllottedGeometry.ToPaintGeometry(), Segment, ESlateDrawEffect::None,
						Entry.Depth > 1 ? FLinearColor(0.36f, 0.65f, 1.0f, 0.22f)
							: FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.30f), true, Entry.Depth > 1 ? 0.65f : 1.0f);
				}
			}

			TArray<FSlateRect> OccupiedLabels;
			const FVector2D LabelSize(154.0f, 31.0f);
			FVector ViewLocation = FVector::ZeroVector;
			FRotator ViewRotation = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
			const FVector CameraRight = ViewRotation.Quaternion().GetRightVector();
			for (const FAPSPreviewBodyEntry& Entry : Entries)
			{
				const AActor* Body = Entry.Actor.Get();
				FVector2D Anchor;
				if (!Body || !ProjectToPanel(Body->GetActorLocation(), Anchor)
					|| Anchor.X < 0.0f || Anchor.X > PanelSize.X || Anchor.Y < 0.0f || Anchor.Y > PanelSize.Y)
				{
					continue;
				}

				// Screen-space body discs provide a stable and cheap occlusion test.
				// This avoids visibility traces for every label on every Slate paint,
				// while still hiding a moon/planet marker behind a nearer celestial body.
				const double TargetDistance = FVector::Distance(ViewLocation, Body->GetActorLocation());
				bool bOccluded = false;
				for (const FAPSPreviewBodyEntry& OccluderEntry : Entries)
				{
					const AActor* Occluder = OccluderEntry.Actor.Get();
					if (!Occluder || Occluder == Body
						|| FVector::Distance(ViewLocation, Occluder->GetActorLocation()) >= TargetDistance)
					{
						continue;
					}
					FVector OccluderCenter;
					FVector OccluderExtent;
					Occluder->GetActorBounds(false, OccluderCenter, OccluderExtent, true);
					FVector2D OccluderScreen;
					FVector2D OccluderEdgeScreen;
					const double OccluderRadius = OccluderExtent.GetMax();
					if (OccluderRadius > UE_SMALL_NUMBER
						&& ProjectToPanel(OccluderCenter, OccluderScreen)
						&& ProjectToPanel(OccluderCenter + CameraRight * OccluderRadius, OccluderEdgeScreen))
					{
						const double RadiusPixels = FVector2D::Distance(OccluderScreen, OccluderEdgeScreen);
						if (RadiusPixels > 2.0 && FVector2D::Distance(Anchor, OccluderScreen) < RadiusPixels * 0.86)
						{
							bOccluded = true;
							break;
						}
					}
				}
				if (bOccluded)
				{
					continue;
				}

				FVector2D LabelPosition(
					FMath::Clamp(Anchor.X - LabelSize.X * 0.5f, 4.0f, FMath::Max(4.0f, PanelSize.X - LabelSize.X - 4.0f)),
					FMath::Clamp(Anchor.Y - LabelSize.Y - 34.0f, 4.0f, FMath::Max(4.0f, PanelSize.Y - LabelSize.Y - 4.0f)));
				for (int32 Attempt = 0; Attempt < 10; ++Attempt)
				{
					const FSlateRect Candidate(LabelPosition.X, LabelPosition.Y,
						LabelPosition.X + LabelSize.X, LabelPosition.Y + LabelSize.Y);
					bool bOverlaps = false;
					for (const FSlateRect& Existing : OccupiedLabels)
					{
						if (FSlateRect::DoRectanglesIntersect(Candidate, Existing))
						{
							bOverlaps = true;
							break;
						}
					}
					if (!bOverlaps)
					{
						OccupiedLabels.Add(Candidate);
						break;
					}
					LabelPosition.Y = FMath::Clamp(LabelPosition.Y + LabelSize.Y + 5.0f,
						4.0f, FMath::Max(4.0f, PanelSize.Y - LabelSize.Y - 4.0f));
				}

				const FLinearColor MarkerColor = Entry.Depth == 0 ? Amber
					: Entry.Depth == 1 ? Cyan : FLinearColor(0.44f, 0.72f, 1.0f, 1.0f);
				const FVector2D PoleEnd(LabelPosition.X + LabelSize.X * 0.5f, LabelPosition.Y + LabelSize.Y);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor, PoleEnd}, ESlateDrawEffect::None,
					FLinearColor(MarkerColor.R, MarkerColor.G, MarkerColor.B, 0.72f), true, 1.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor - FVector2D(4.0f, 0.0f), Anchor + FVector2D(4.0f, 0.0f)},
					ESlateDrawEffect::None, MarkerColor, true, 1.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{Anchor - FVector2D(0.0f, 4.0f), Anchor + FVector2D(0.0f, 4.0f)},
					ESlateDrawEffect::None, MarkerColor, true, 1.0f);
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(LabelPosition)),
					&ControlBrush, ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.94f));
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
					AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X - 12.0f, 14.0f),
						FSlateLayoutTransform(LabelPosition + FVector2D(7.0f, 4.0f))),
					Entry.Label, Font("Bold", 8), ESlateDrawEffect::None, MarkerColor);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
					AllottedGeometry.ToPaintGeometry(FVector2D(LabelSize.X - 12.0f, 11.0f),
						FSlateLayoutTransform(LabelPosition + FVector2D(7.0f, 17.0f))),
					Entry.Details, Font("Regular", 6), ESlateDrawEffect::None, Muted);
			}

			return LayerId + 4;
		}

	private:
		TWeakObjectPtr<UWorldGenerationViewModel> ViewModel;
	};
}

void SWorldGenerationPanel::Construct(const FArguments& InArgs)
{
	ViewModel = InArgs._ViewModel;
	OnBack = InArgs._OnBack;
	OnContinue = InArgs._OnContinue;

	using namespace APSGenerationUI;
	const TWeakObjectPtr<UWorldGenerationViewModel> VM = ViewModel;
	SAssignNew(BodyHierarchyBox, SVerticalBox);

	const auto BoolRow = [VM](const FText& Label, TFunction<bool(const UGeneratedWorld*)> Getter,
		TFunction<void(UGeneratedWorld*, bool)> Setter)
	{
		return SNew(SBorder).BorderImage(&ControlBrush).Padding(FMargin(10.0f, 4.0f))
		[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(Label).Font(Font("Regular", 10)).ColorAndOpacity(Muted)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(14.0f, 4.0f))
				.ButtonColorAndOpacity_Lambda([VM, Getter]()
				{
					return VM.IsValid() && VM->GeneratedWorld && Getter(VM->GeneratedWorld)
						? FLinearColor(0.0f, 0.52f, 0.68f, 1.0f) : FLinearColor(0.18f, 0.23f, 0.27f, 1.0f);
				})
				.OnClicked_Lambda([VM, Getter, Setter]()
				{
					if (UWorldGenerationViewModel* MutableVM = VM.Get(); MutableVM && MutableVM->GeneratedWorld)
					{
						Setter(MutableVM->GeneratedWorld, !Getter(MutableVM->GeneratedWorld));
						MutableVM->RequestPreview();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text_Lambda([VM, Getter](){ return FText::FromString(VM.IsValid() && VM->GeneratedWorld && Getter(VM->GeneratedWorld) ? TEXT("ON") : TEXT("OFF")); })
					.Font(Font("Bold", 10)).ColorAndOpacity(Cyan)
				]
			]
		];
	};

	const TSharedRef<SWidget> OverviewControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Astro", "ASTRO GENERATION"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EAstroGenerationLevel>(LOCTEXT("Level", "GENERATION LEVEL"), VM, [](const UGeneratedWorld* W){ return W->AstroGenerationLevel; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("FullScale", "FULL-SCALE WORLD"), [](const UGeneratedWorld* W){return W->bGenerateFullScaledWorld;}, [](UGeneratedWorld* W, bool V){W->bGenerateFullScaledWorld=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("HomeSystemEnabled", "GENERATE HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bGenerateHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bGenerateHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("StartWithHomePlanet", "START WITH HOME PLANET"), [](const UGeneratedWorld* W){return W->bStartWithHomePlanet;}, [](UGeneratedWorld* W, bool V){W->bStartWithHomePlanet=V;})]
	];

	const TSharedRef<SWidget> GalaxyControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Galaxy", "GALAXY"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyType>(LOCTEXT("GalaxyType", "TYPE / PRESET"), VM, [](const UGeneratedWorld* W){ return W->GalaxyType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EGalaxyClass>(LOCTEXT("GalaxyClass", "CLASS"), VM, [](const UGeneratedWorld* W){ return W->GalaxyClass; })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("GalaxySize", "SIZE"), 1, 100000, 1, VM, [](const UGeneratedWorld* W){ return W->GalaxySize; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxySize(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StarCount", "MODELED STAR COUNT"), 1, 100000000, 1000, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarCount; }, [](UWorldGenerationViewModel* V, int32 X){ V->SetGalaxyStarCount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Density", "STAR DENSITY"), 0.01, 1000.0, 0.1, VM, [](const UGeneratedWorld* W){ return W->GalaxyStarDensity; }, [](UWorldGenerationViewModel* V, double X){ V->SetGalaxyStarDensity(X); })]
	];

	const TSharedRef<SWidget> ClusterControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Cluster", "STAR CLUSTER"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterSize>(LOCTEXT("ClusterSize", "SIZE"), VM, [](const UGeneratedWorld* W){ return W->StarClusterSize; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterType>(LOCTEXT("ClusterType", "FORMATION / PRESET"), VM, [](const UGeneratedWorld* W){ return W->StarClusterType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterPopulation>(LOCTEXT("Population", "POPULATION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterPopulation; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarClusterComposition>(LOCTEXT("Composition", "COMPOSITION"), VM, [](const UGeneratedWorld* W){ return W->StarClusterComposition; })]
	];

	const TSharedRef<SWidget> SystemControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("HomeStarSystem", "STAR SYSTEM"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStarType>(LOCTEXT("StarType", "STAR SYSTEM TYPE"), VM, [](const UGeneratedWorld* W){ return W->StarType; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("SystemLayout", "SYSTEM LAYOUT"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeSystem", "RANDOM HOME SYSTEM"), [](const UGeneratedWorld* W){return W->bRandomHomeSystem;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystem=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomSystemType", "RANDOM SYSTEM TYPE"), [](const UGeneratedWorld* W){return W->bRandomHomeSystemType;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeSystemType=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomHomeStar", "RANDOM HOME STAR"), [](const UGeneratedWorld* W){return W->bRandomHomeStar;}, [](UGeneratedWorld* W, bool V){W->bRandomHomeStar=V;})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)[BoolRow(LOCTEXT("RandomStartPlanet", "RANDOM START PLANET"), [](const UGeneratedWorld* W){return W->bRandomStartPlanetNumber;}, [](UGeneratedWorld* W, bool V){W->bRandomStartPlanetNumber=V;})]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetarySystemType>(LOCTEXT("SystemType", "PLANETARY TYPE"), VM, [](const UGeneratedWorld* W){ return W->PlanetarySystemType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EOrbitDistributionType>(LOCTEXT("Distribution", "ORBIT DISTRIBUTION"), VM, [](const UGeneratedWorld* W){ return W->OrbitDistributionType; })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Planets", "PLANETS AMOUNT"), 1, 20, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetsAmount, 1, 20); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetPlanetsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("Moons", "HOME PLANET MOONS"), 0, 10, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->MoonsAmount, 0, 10); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("StartIndex", "START PLANET INDEX"), 1, 20, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->StartPlanetIndex, 1, FMath::Max(1, W->PlanetsAmount)); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetStartPlanetIndex(X); })]
	];

	const TSharedRef<SWidget> StarControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("SelectedStar", "SELECTED STAR"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EStellarType>(LOCTEXT("StellarType", "STELLAR TYPE"), VM, [](const UGeneratedWorld* W){ return W->StellarType; })]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<ESpectralClass>(LOCTEXT("Spectral", "SPECTRAL CLASS"), VM, [](const UGeneratedWorld* W){ return W->SpectralClass; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StarScopeHint", "STELLAR CHANGES UPDATE THIS STAR AND ITS SAFE ORBIT CLEARANCE WITHOUT REGENERATING THE PARENT SYSTEM."))
			.AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(Muted)
		]
	];

	const TSharedRef<SWidget> PlanetControls = SNew(SScrollBox) + SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)[SectionTitle(LOCTEXT("Planet", "SELECTED PLANET"))]
		+ SVerticalBox::Slot().AutoHeight()[EnumRow<EPlanetType>(LOCTEXT("PlanetType", "PLANET TYPE"), VM, [](const UGeneratedWorld* W){ return W->PlanetType; })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("Radius", "PLANET RADIUS / KM"), 100.0, 20000.0, 100.0, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->PlanetRadius, 100.0, 20000.0); }, [](UWorldGenerationViewModel* V, double X){ V->SetPlanetRadius(X); })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<int32>(LOCTEXT("PlanetMoons", "MOONS AMOUNT"), 0, 10, 1, VM, [](const UGeneratedWorld* W){ return FMath::Clamp(W->MoonsAmount, 0, 10); }, [](UWorldGenerationViewModel* V, int32 X){ V->SetMoonsAmount(X); })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 10.0f)[SectionTitle(LOCTEXT("Atmosphere", "PLANET ATMOSPHERE"))]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereHeight", "HEIGHT / KM"), 0.0, 2000.0, 5.0, VM, [](const UGeneratedWorld* W){ return W->AtmosphereHeight; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereHeight=X; V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereOpacity", "OPACITY"), 0.0, 10.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->AtmosphereOpacity; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereOpacity=X; V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereMulti", "MULTI SCATTERING"), 0.0, 10.0, 0.05, VM, [](const UGeneratedWorld* W){ return W->AtmosphereMultiScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereMultiScattering=X; V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereRayleigh", "RAYLEIGH SCATTERING"), 0.0, 64.0, 0.25, VM, [](const UGeneratedWorld* W){ return W->AtmosphereRayleighScattering; }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereRayleighScattering=X; V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorR", "COLOR / RED"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.R); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.R=static_cast<float>(X); V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorG", "COLOR / GREEN"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.G); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.G=static_cast<float>(X); V->RequestPreview();} })]
		+ SVerticalBox::Slot().AutoHeight()[NumberRow<double>(LOCTEXT("AtmosphereColorB", "COLOR / BLUE"), 0.0, 64.0, 0.1, VM, [](const UGeneratedWorld* W){ return static_cast<double>(W->AtmosphereColor.B); }, [](UWorldGenerationViewModel* V, double X){ if(V->GeneratedWorld){V->GeneratedWorld->AtmosphereColor.B=static_cast<float>(X); V->RequestPreview();} })]
	];

	const TSharedRef<SWidget> ControlsSwitcher = SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([VM]()
		{
			if (!VM.IsValid()) return 0;
			switch (VM->GetPreviewFocus())
			{
			case EAstroPreviewFocus::Galaxy: return 1;
			case EAstroPreviewFocus::StarCluster: return 2;
			case EAstroPreviewFocus::HomeSystem: return 3;
			case EAstroPreviewFocus::HomeStar: return 4;
			case EAstroPreviewFocus::HomePlanet: return 5;
			default: return 0;
			}
		})
		+ SWidgetSwitcher::Slot()[OverviewControls]
		+ SWidgetSwitcher::Slot()[GalaxyControls]
		+ SWidgetSwitcher::Slot()[ClusterControls]
		+ SWidgetSwitcher::Slot()[SystemControls]
		+ SWidgetSwitcher::Slot()[StarControls]
		+ SWidgetSwitcher::Slot()[PlanetControls];

	const TSharedRef<SWidget> ContextPanel = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(Font("Bold", 9)).ColorAndOpacity(Cyan)
				.Text_Lambda([VM]()
				{
					if (!VM.IsValid()) return FText::GetEmpty();
					switch (VM->GetPreviewFocus())
					{
					case EAstroPreviewFocus::Galaxy: return LOCTEXT("CrumbGalaxy", "GALAXY");
					case EAstroPreviewFocus::StarCluster: return LOCTEXT("CrumbCluster", "GALAXY  /  CLUSTER");
					case EAstroPreviewFocus::HomeSystem: return LOCTEXT("CrumbSystem", "GALAXY  /  CLUSTER  /  SYSTEM");
					case EAstroPreviewFocus::HomeStar: return LOCTEXT("CrumbStar", "GALAXY  /  CLUSTER  /  SYSTEM  /  STAR");
					case EAstroPreviewFocus::HomePlanet: return LOCTEXT("CrumbPlanet", "GALAXY  /  CLUSTER  /  SYSTEM  /  BODY");
					default: return LOCTEXT("CrumbOverview", "FULL-SCALE OVERVIEW");
					}
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).ButtonStyle(&SecondaryButton).ContentPadding(FMargin(9.0f, 4.0f))
				.OnClicked(this, &SWorldGenerationPanel::FocusPreviewUp)
				.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->GetPreviewFocus() != EAstroPreviewFocus::Overview; })
				[SNew(STextBlock).Text(LOCTEXT("Up", "^  UP")).Font(Font("Bold", 8)).ColorAndOpacity(White)]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)[SectionTitle(LOCTEXT("CurrentScope", "CURRENT SCOPE"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 10.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 11)).ColorAndOpacity(Muted)
			.Text_Lambda([VM]()
			{
				if (!VM.IsValid()) return FText::GetEmpty();
				switch (VM->GetPreviewFocus())
				{
				case EAstroPreviewFocus::Galaxy: return LOCTEXT("GalaxyHelp", "Shape the parent galaxy. Modeled star count remains full-scale while the live preview uses a bounded sample.");
				case EAstroPreviewFocus::StarCluster: return LOCTEXT("ClusterHelp", "Configure the selected cluster population and formation inside its parent galaxy.");
				case EAstroPreviewFocus::HomeSystem: return LOCTEXT("SystemHelp", "Edit the complete star system. Orbits, planets and moons remain children of its selected star hierarchy.");
				case EAstroPreviewFocus::HomeStar: return LOCTEXT("StarHelp", "Edit the selected star while retaining its parent system, planets and camera context.");
				case EAstroPreviewFocus::HomePlanet: return LOCTEXT("PlanetHelp", "Edit the selected planet and atmosphere without randomizing its parent system.");
				default: return LOCTEXT("OverviewHelp", "Choose a hierarchy level below, then drag the live scene to orbit or scroll to zoom.");
				}
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)[SectionTitle(LOCTEXT("LiveModel", "LIVE MODEL"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f)
		[
			SNew(STextBlock).AutoWrapText(true).Font(Font("Regular", 10)).ColorAndOpacity(White)
			.Text_Lambda([VM]()
			{
				const UGeneratedWorld* W = VM.IsValid() ? VM->GeneratedWorld.Get() : nullptr;
				if (!W) return FText::FromString(TEXT("NO MODEL"));
				return FText::FromString(FString::Printf(TEXT("MODELED STARS  %d\nPLANETS  %d\nMOONS  %d\nPLANET RADIUS  %.0f KM\nFULL SCALE  %s"),
					W->GalaxyStarCount, W->PlanetsAmount, W->MoonsAmount, W->PlanetRadius,
					W->bGenerateFullScaledWorld ? TEXT("ON") : TEXT("OFF")));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 7.0f)
		[SectionTitle(LOCTEXT("SystemBodies", "SYSTEM BODIES"))]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[BodyHierarchyBox.ToSharedRef()]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(LOCTEXT("PickHint", "DOUBLE CLICK A BODY TO FOCUS\nUSE HIERARCHY BUTTONS TO MOVE UP")).AutoWrapText(true).Font(Font("Bold", 9)).ColorAndOpacity(Cyan)];

	const auto FocusButton = [this](const FText& Glyph, const FText& Label, EAstroPreviewFocus Focus)
	{
		return SNew(SButton)
			.ButtonStyle(&SecondaryButton)
			.ButtonColorAndOpacity_Lambda([this, Focus]()
			{
				const UWorldGenerationViewModel* VMValue = ViewModel.Get();
				const EAstroPreviewFocus CurrentFocus = VMValue ? VMValue->GetPreviewFocus() : EAstroPreviewFocus::Overview;
				const bool bSelected = CurrentFocus == Focus;
				return bSelected
					? FLinearColor(0.32f, 0.13f, 0.005f, 1.0f) : FLinearColor::White;
			})
			.ContentPadding(FMargin(12.0f, 8.0f))
			.OnClicked(this, &SWorldGenerationPanel::FocusPreview, static_cast<uint8>(Focus))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[SNew(STextBlock).Text(Glyph).Font(APSGenerationUI::Font("Bold", 9)).ColorAndOpacity(APSGenerationUI::Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(Label).Font(APSGenerationUI::Font("Bold", 10)).ColorAndOpacity(APSGenerationUI::White)]
			];
	};

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Background).Padding(FMargin(22.0f, 16.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&SecondaryButton).OnClicked(this, &SWorldGenerationPanel::GoBack).ContentPadding(FMargin(15.0f, 8.0f))
					[SNew(STextBlock).Text(LOCTEXT("Back", "<  BACK")).Font(Font("Bold", 12)).ColorAndOpacity(White)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(LOCTEXT("Title", "A P O S F E R A")).Font(Font("Bold", 42)).ColorAndOpacity(White)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(78.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Cyan)]]
						+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("Subtitle", "A S T R O N O M I C A L   G E N E R A T I O N")).Font(Font("Bold", 13)).ColorAndOpacity(Cyan)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(78.0f).HeightOverride(1.0f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(Cyan)]]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&SecondaryButton).OnClicked(this, &SWorldGenerationPanel::RefreshPreview).ContentPadding(FMargin(14.0f, 8.0f))
					[SNew(STextBlock).Text(LOCTEXT("Refresh", "REGENERATE")).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					ChamferPanel(ControlsSwitcher)
				]
				+ SHorizontalBox::Slot().FillWidth(0.50f).Padding(6.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
					SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SPreviewInteractionSurface).ViewModel(VM)]
						+ SOverlay::Slot()[SNew(SPreviewSystemOverlay).ViewModel(VM)]
						+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f)
						[SNew(STextBlock).Text(LOCTEXT("PreviewCornerTL", "+  LIVE FULL-SCALE PREVIEW")).Font(Font("Bold", 9)).ColorAndOpacity(FLinearColor(0.20f, 0.90f, 0.55f, 0.82f))]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(10.0f)
						[SNew(STextBlock).Text(LOCTEXT("PreviewCornerBR", "FULL SCALE  +")).Font(Font("Bold", 8)).ColorAndOpacity(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.55f))]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 10.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("OverviewGlyph", "O"), LOCTEXT("FocusOverview", "OVERVIEW"), EAstroPreviewFocus::Overview)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("GalaxyGlyph", "GX"), LOCTEXT("FocusGalaxy", "GALAXY"), EAstroPreviewFocus::Galaxy)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("ClusterGlyph", "CL"), LOCTEXT("FocusCluster", "CLUSTER"), EAstroPreviewFocus::StarCluster)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("SystemGlyph", "SYS"), LOCTEXT("FocusSystem", "SYSTEM"), EAstroPreviewFocus::HomeSystem)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("StarGlyph", "S"), LOCTEXT("FocusStar", "STAR"), EAstroPreviewFocus::HomeStar)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)[FocusButton(LOCTEXT("PlanetGlyph", "P"), LOCTEXT("FocusPlanet", "PLANET"), EAstroPreviewFocus::HomePlanet)]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[SNew(STextBlock).Text(LOCTEXT("PreviewHint", "RMB DRAG TO ROTATE   /   MOUSE WHEEL TO ZOOM   /   DOUBLE CLICK TO FOCUS")).Font(Font("Regular", 8)).ColorAndOpacity(Muted)]
				]
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					ChamferPanel(ContextPanel)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetPreviewStatus).Font(Font("Bold", 11)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).ButtonStyle(&PrimaryButton).OnClicked(this, &SWorldGenerationPanel::CommitWorld)
					.IsEnabled_Lambda([VM]() { return VM.IsValid() && VM->bPreviewReady; })
					.ContentPadding(FMargin(52.0f, 13.0f))
					[SNew(STextBlock).Text(this, &SWorldGenerationPanel::GetContinueLabel).Font(Font("Bold", 14)).ColorAndOpacity(White)]
				]
			]
		]
	];

	if (UWorldGenerationViewModel* MutableVM = ViewModel.Get())
	{
		MutableVM->RequestPreview();
	}
	RegisterActiveTimer(0.35f, FWidgetActiveTimerDelegate::CreateSP(this, &SWorldGenerationPanel::RefreshBodyHierarchy));
}

FText SWorldGenerationPanel::GetPreviewStatus() const
{
	if (const UWorldGenerationViewModel* VM = ViewModel.Get()) return VM->PreviewStatus;
	return LOCTEXT("NoViewModel", "PREVIEW OFFLINE");
}

FText SWorldGenerationPanel::GetContinueLabel() const
{
	const UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM) return LOCTEXT("ContinueUnavailable", "CONTINUE   >");
	switch (VM->GetGenerationRoute())
	{
	case EAPSGenerationRoute::Space: return LOCTEXT("GenerateSpace", "GENERATE SPACE WORLD   >");
	case EAPSGenerationRoute::Planet: return LOCTEXT("GeneratePlanet", "GENERATE PLANET   >");
	default: return LOCTEXT("ContinueCivilization", "CONTINUE TO CIVILIZATION   >");
	}
}

FReply SWorldGenerationPanel::CommitWorld()
{
	if (OnContinue.IsBound()) OnContinue.Execute();
	else if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->CommitAndOpenLevel();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::RefreshPreview()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->RegeneratePreviewVariant();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::GoBack()
{
	OnBack.ExecuteIfBound();
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreview(uint8 FocusValue)
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get()) VM->SetPreviewFocus(static_cast<EAstroPreviewFocus>(FocusValue));
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewUp()
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		EAstroPreviewFocus ParentFocus = EAstroPreviewFocus::Overview;
		switch (VM->GetPreviewFocus())
		{
		case EAstroPreviewFocus::HomePlanet:
		case EAstroPreviewFocus::HomeStar:
			ParentFocus = EAstroPreviewFocus::HomeSystem;
			break;
		case EAstroPreviewFocus::HomeSystem:
			ParentFocus = EAstroPreviewFocus::StarCluster;
			break;
		case EAstroPreviewFocus::StarCluster:
			ParentFocus = EAstroPreviewFocus::Galaxy;
			break;
		case EAstroPreviewFocus::Galaxy:
		default:
			ParentFocus = EAstroPreviewFocus::Overview;
			break;
		}
		VM->SetPreviewFocus(ParentFocus);
	}
	return FReply::Handled();
}

FReply SWorldGenerationPanel::FocusPreviewBody(TWeakObjectPtr<AActor> BodyActor)
{
	if (UWorldGenerationViewModel* VM = ViewModel.Get())
	{
		VM->FocusPreviewBody(BodyActor);
	}
	return FReply::Handled();
}

EActiveTimerReturnType SWorldGenerationPanel::RefreshBodyHierarchy(double CurrentTime, float DeltaTime)
{
	UWorldGenerationViewModel* VM = ViewModel.Get();
	if (!VM || !BodyHierarchyBox.IsValid())
	{
		return EActiveTimerReturnType::Continue;
	}

	TArray<FAPSPreviewBodyEntry> Entries;
	VM->GetPreviewBodyEntries(Entries);
	uint32 Signature = HashCombineFast(GetTypeHash(VM->PreviewRevision), GetTypeHash(Entries.Num()));
	for (const FAPSPreviewBodyEntry& Entry : Entries)
	{
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Actor.Get()));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Depth));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Label.ToString()));
		Signature = HashCombineFast(Signature, GetTypeHash(Entry.Details.ToString()));
	}
	if (Signature != BodyHierarchySignature)
	{
		RebuildBodyHierarchy(Entries, Signature);
	}
	return EActiveTimerReturnType::Continue;
}

void SWorldGenerationPanel::RebuildBodyHierarchy(const TArray<FAPSPreviewBodyEntry>& Entries, uint32 Signature)
{
	using namespace APSGenerationUI;
	if (!BodyHierarchyBox.IsValid())
	{
		return;
	}

	BodyHierarchyBox->ClearChildren();
	BodyHierarchySignature = Signature;
	if (Entries.IsEmpty())
	{
		BodyHierarchyBox->AddSlot().AutoHeight().Padding(0.0f, 6.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("BodiesPending", "MATERIALIZING SYSTEM HIERARCHY..."))
			.Font(Font("Regular", 9)).ColorAndOpacity(Muted).AutoWrapText(true)
		];
		return;
	}

	for (const FAPSPreviewBodyEntry& Entry : Entries)
	{
		const TWeakObjectPtr<AActor> BodyActor = Entry.Actor;
		const float LeftIndent = static_cast<float>(FMath::Clamp(Entry.Depth, 0, 4)) * 14.0f;
		const FText Prefix = Entry.Depth < 0 ? LOCTEXT("SystemPrefix", "SYSTEM")
			: Entry.Depth == 0 ? LOCTEXT("StarPrefix", "STAR")
			: Entry.Depth == 1 ? LOCTEXT("PlanetPrefix", "PLANET") : LOCTEXT("MoonPrefix", "MOON");
		BodyHierarchyBox->AddSlot().AutoHeight().Padding(LeftIndent, 2.0f, 0.0f, 2.0f)
		[
			SNew(SButton)
			.ButtonStyle(&SecondaryButton)
			.ButtonColorAndOpacity_Lambda([this, BodyActor]()
			{
				const UWorldGenerationViewModel* VM = ViewModel.Get();
				return VM && VM->GetSelectedPreviewBody() == BodyActor.Get()
					? FLinearColor(0.32f, 0.13f, 0.005f, 1.0f) : FLinearColor::White;
			})
			.ContentPadding(FMargin(8.0f, 5.0f))
			.IsEnabled(BodyActor.IsValid())
			.OnClicked(this, &SWorldGenerationPanel::FocusPreviewBody, BodyActor)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 7.0f, 0.0f)
					[SNew(STextBlock).Text(Prefix).Font(Font("Bold", 8)).ColorAndOpacity(Cyan)]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[SNew(STextBlock).Text(Entry.Label).Font(Font("Bold", 9)).ColorAndOpacity(White)]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(STextBlock).Text(BodyActor.IsValid() ? FText::FromString(TEXT(">")) : FText::GetEmpty()).Font(Font("Bold", 9)).ColorAndOpacity(Cyan)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[SNew(STextBlock).Text(Entry.Details).Font(Font("Regular", 7)).ColorAndOpacity(Muted)]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE

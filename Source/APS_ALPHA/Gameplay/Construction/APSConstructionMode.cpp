#include "APSConstructionMode.h"

#include "APSConstructionCatalog.h"
#include "APS_ALPHA/Actors/Astro/CelestialBody.h"
#include "APS_ALPHA/Actors/Tech/AutonomousOutpost.h"
#include "APS_ALPHA/Actors/Tech/SpaceHeadquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceShipyard.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructure.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/Construction/SAPSBuildPalette.h"
#include "APS_ALPHA/UI/Style/APSUINumber.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "APSConstruction"

namespace APSConstructionModeLocal
{
	using namespace APSConstruction;

	/** Unattended test runs have no mouse: the cursor's ray goes through the viewport's centre instead. */
	TAutoConsoleVariable<int32> CVarCursorAtCenter(TEXT("aps.Construction.CursorAtCenter"), 0,
		TEXT("1: build mode aims through the viewport's centre instead of the mouse cursor (test runs)."));

	bool DeprojectCursor(APlayerController& Controller, FVector& OutOrigin, FVector& OutDirection)
	{
		if (CVarCursorAtCenter.GetValueOnGameThread() != 0)
		{
			int32 Width = 0;
			int32 Height = 0;
			Controller.GetViewportSize(Width, Height);
			return Width > 0 && Height > 0
				&& Controller.DeprojectScreenPositionToWorld(Width * 0.5f, Height * 0.5f, OutOrigin, OutDirection);
		}
		return Controller.DeprojectMousePositionToWorld(OutOrigin, OutDirection);
	}

	/** The cursor's ray reaches this far; the ghost may rise or sink this much against the builder. */
	constexpr double TraceLengthCm = 40000.0;
	constexpr double MaxRiseCm = 3000.0;
	/** A prop tilts with the ground at most this much; a structure stands on ground at most this steep. */
	constexpr float MaxTiltDegrees = 15.0f;
	constexpr float StructureMaxSlopeDegrees = 20.0f;
	/** Into the ground a little, so an uneven ground shows no gap under a flat underside. */
	constexpr double SinkCm = 2.0;
	constexpr int32 RingSegments = 48;
	constexpr double RingWidthCm = 60.0;
	constexpr double RingLiftCm = 15.0;
	/** Above the walking HUD (40), the mission tracker (45) and the prompts (50); under the ship HUD (60). */
	constexpr int32 PaletteZOrder = 55;
	constexpr float GhostOpacity = 0.38f;
	constexpr float RingOpacity = 0.6f;
	constexpr int32 TintValid = 0;
	constexpr int32 TintInvalid = 1;
	constexpr int32 TintRemove = 2;

	/** The project's translucent unlit preview material (the generator's guides use it), else the engine's simplest. */
	const TCHAR* const GhostMaterialPath =
		TEXT("/Game/APS/APS_ALPHA/WSC/PlanetSurface/Preview/M_APS_PreviewGuide.M_APS_PreviewGuide");
	const TCHAR* const GhostMaterialFallback =
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent");

	FLinearColor TintColour(const int32 Tint)
	{
		switch (Tint)
		{
		case TintValid: return FLinearColor(0.15f, 1.0f, 0.45f, 1.0f);
		case TintRemove: return FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);
		default: return FLinearColor(1.0f, 0.16f, 0.10f, 1.0f);
		}
	}

	FLinearColor RingColour()
	{
		return FLinearColor(0.26f, 0.84f, 0.93f, 1.0f);
	}

	void SetTint(UMaterialInstanceDynamic* Material, const FLinearColor& Colour, const float Opacity)
	{
		// The preview-guide material keeps colour and opacity apart; the engine fallback reads both from Color.
		Material->SetVectorParameterValue(TEXT("GuideColor"), Colour);
		Material->SetScalarParameterValue(TEXT("GuideOpacity"), Opacity);
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(Colour.R, Colour.G, Colour.B, Opacity));
		Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}

	UMaterialInstanceDynamic* MakeTint(UObject* Outer, const FLinearColor& Colour, const float Opacity)
	{
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, GhostMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Parent)
		{
			Parent = LoadObject<UMaterialInterface>(nullptr, GhostMaterialFallback, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		UMaterialInstanceDynamic* Material = Parent ? UMaterialInstanceDynamic::Create(Parent, Outer) : nullptr;
		if (Material)
		{
			SetTint(Material, Colour, Opacity);
		}
		return Material;
	}

	/** The class and size a structure type is shown with (mirrors FAPSInfrastructure::SpawnVisual). */
	UClass* StructureClass(UWorld* World, const APSInfrastructure::FType& Type)
	{
		using APSInfrastructure::EVisual;
		if (World && (Type.Visual == EVisual::Station || Type.Visual == EVisual::Shipyard || Type.Visual == EVisual::Headquarters))
		{
			for (TActorIterator<AAstroGenerator> It(World); It; ++It)
			{
				UClass* Found = Type.Visual == EVisual::Shipyard ? It->BP_HomeSpaceShipyard.Get()
					: Type.Visual == EVisual::Headquarters ? It->BP_HomeSpaceHeadquarters.Get() : It->BP_HomeSpaceStation.Get();
				if (Found)
				{
					return Found;
				}
			}
		}
		return AAutonomousOutpost::StaticClass();
	}

	float StructureScale(const APSInfrastructure::FType& Type)
	{
		return Type.Visual == APSInfrastructure::EVisual::Beacon ? 0.6f : Type.VisualScale;
	}

	FText Metres(const double Centimetres)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 1;
		Options.MaximumFractionalDigits = 1;
		return APSUINumber::Number(Centimetres / 100.0, &Options);
	}

	FText WholeNumber(const double Value)
	{
		return APSUINumber::Number(FMath::RoundToInt(Value));
	}

	/** The slots of a prop's parts, in its frame: its size without loading any mesh (the packs' sizes are the slots'). */
	FBox SlotBounds(const FPropType& Type)
	{
		FBox Bounds(ForceInit);
		for (const FPart& Part : Type.Parts)
		{
			Bounds += FBox(-Part.Size * 0.5, Part.Size * 0.5).TransformBy(FTransform(Part.Rotation, Part.Center));
		}
		return Bounds;
	}

	bool IsGameWindowActive()
	{
		if (!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized())
		{
			return false;
		}
		const TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
		return Window.IsValid() && FSlateApplication::Get().GetActiveTopLevelWindow() == Window;
	}

	/**
	 * Escape reaches the build mode before anything else: in PIE the editor would stop the session with it before the
	 * game's own bindings, and outside PIE the pause menu would open. Only while the build mode is on.
	 */
	class FEscapeProcessor final : public IInputProcessor
	{
	public:
		explicit FEscapeProcessor(TWeakPtr<FAPSConstructionMode> InMode)
			: Mode(MoveTemp(InMode))
		{
		}

		virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& InKeyEvent) override
		{
			// Shift+Esc stays the editor's (it also stops a PIE session).
			if (InKeyEvent.GetKey() != EKeys::Escape || InKeyEvent.IsRepeat() || InKeyEvent.IsShiftDown())
			{
				return false;
			}
			const TSharedPtr<FAPSConstructionMode> Pinned = Mode.Pin();
			return Pinned.IsValid() && Pinned->HandleEscape();
		}

		virtual const TCHAR* GetDebugName() const override
		{
			return TEXT("APSBuildModeEscape");
		}

	private:
		TWeakPtr<FAPSConstructionMode> Mode;
	};
}

FAPSConstructionMode::FAPSConstructionMode(APawn* InBuilder)
	: Builder(InBuilder)
	, World(InBuilder ? InBuilder->GetWorld() : nullptr)
{
}

FAPSConstructionMode::~FAPSConstructionMode()
{
	End();
}

void FAPSConstructionMode::Begin(const APSConstruction::FFrame& InFrame)
{
	using namespace APSConstructionModeLocal;
	if (bActive)
	{
		return;
	}
	bActive = true;
	bExitRequested = false;
	Frame = InFrame;
	BuildPropEntries();
	RefreshEntries(true);
	if (GEngine && GEngine->GameViewport)
	{
		Palette = SNew(SAPSBuildPalette).Mode(TWeakPtr<FAPSConstructionMode>(AsShared()));
		PaletteContainer = SNew(SWeakWidget).PossiblyNullContent(Palette.ToSharedRef());
		GEngine->GameViewport->AddViewportWidgetContent(PaletteContainer.ToSharedRef(), PaletteZOrder);
	}
	if (FSlateApplication::IsInitialized())
	{
		EscapeProcessor = MakeShared<FEscapeProcessor>(TWeakPtr<FAPSConstructionMode>(AsShared()));
		FSlateApplication::Get().RegisterInputPreProcessor(EscapeProcessor, 0);
	}
	UpdateRing(true);
	Flash(Frame.Placement == EPlacement::Orbit
		? LOCTEXT("WelcomeOrbit", "ORBITAL BUILD: pick an object below; it floats on the cursor, the wheel sets how far.")
		: LOCTEXT("Welcome", "BUILD MODE: pick an object below, then click the ground inside the ring."), false, 4.0f);
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] build mode on at %s (zone %.0f m)"), *GetSiteName().ToString(),
		Frame.RadiusCm / 100.0);
}

void FAPSConstructionMode::End()
{
	const bool bWasActive = bActive;
	bActive = false;
	if (EscapeProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(EscapeProcessor);
	}
	EscapeProcessor.Reset();
	if (PaletteContainer.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PaletteContainer.ToSharedRef());
	}
	PaletteContainer.Reset();
	Palette.Reset();
	DestroyGhost();
	if (AActor* Ring = RingActor.Get())
	{
		Ring->Destroy();
	}
	RingActor.Reset();
	RingMesh.Reset();
	bRingSectionCreated = false;
	HoveredProp.Reset();
	if (bWasActive)
	{
		UE_LOG(LogTemp, Log, TEXT("[APS.Construction] build mode off"));
	}
}

void FAPSConstructionMode::Tick(const float DeltaSeconds, const APSConstruction::FFrame& InFrame)
{
	if (!bActive)
	{
		return;
	}
	Frame = InFrame;
	Clock += FMath::Max(DeltaSeconds, 0.0f);
	RefreshEntries(false);
	UpdateRing(false);
	UpdateGhost();
}

void FAPSConstructionMode::SelectSlot(const int32 Slot)
{
	const TArray<APSConstruction::FEntry>& Entries = GetEntries(Section);
	if (Entries.IsValidIndex(Slot))
	{
		Select(Section, Entries[Slot].Id);
	}
}

void FAPSConstructionMode::Select(const APSConstruction::ESection InSection, const FName Id)
{
	if (!bActive || Id.IsNone())
	{
		return;
	}
	if (SelectedId == Id && SelectedSection == InSection)
	{
		CancelSelection();
		return;
	}
	SelectedSection = InSection;
	SelectedId = Id;
	Section = InSection;
	bYawInitialized = false;
	PitchDegrees = 0.0f;
	OrbitDistanceCm = 0.0;
	StructureRefusal = FText::GetEmpty();
	NextRefusalCheck = 0.0;
	HoveredProp.Reset();
	// Something not buildable here is still shown (to look at it), with the reason.
	for (const APSConstruction::FEntry& Entry : GetEntries(InSection))
	{
		if (Entry.Id == Id && !Entry.Refusal.IsEmpty())
		{
			Flash(Entry.Refusal, true, 3.0f);
		}
	}
}

void FAPSConstructionMode::SetSection(const APSConstruction::ESection InSection)
{
	Section = InSection;
}

void FAPSConstructionMode::ToggleSection()
{
	using APSConstruction::ESection;
	SetSection(Section == ESection::Props ? ESection::Infrastructure : ESection::Props);
}

void FAPSConstructionMode::Rotate(const float Degrees)
{
	if (!HasSelection())
	{
		return;
	}
	YawDegrees = FRotator::NormalizeAxis(YawDegrees + Degrees);
	bYawInitialized = true;
}

void FAPSConstructionMode::RotatePitch(const float Degrees)
{
	if (HasSelection())
	{
		PitchDegrees = FRotator::NormalizeAxis(PitchDegrees + Degrees);
	}
}

void FAPSConstructionMode::AdjustDistance(const float Steps)
{
	if (HasSelection() && OrbitDistanceCm > 0.0)
	{
		// A tenth further or nearer a step; never inside the camera, never far beyond the zone.
		OrbitDistanceCm = FMath::Clamp(OrbitDistanceCm * FMath::Pow(1.1, static_cast<double>(Steps)), 1500.0,
			FMath::Max(Frame.RadiusCm * 1.8, 1500.0));
	}
}

bool FAPSConstructionMode::Place()
{
	if (!bActive || !HasSelection())
	{
		return false;
	}
	if (!bPlacementValid)
	{
		if (!PlacementStatus.IsEmpty())
		{
			Flash(PlacementStatus, true, 1.5f);
		}
		return false;
	}
	return SelectedSection == APSConstruction::ESection::Props ? PlaceProp() : PlaceStructure();
}

bool FAPSConstructionMode::CancelSelection()
{
	if (!HasSelection())
	{
		return false;
	}
	SelectedId = NAME_None;
	bPlacementValid = false;
	HideGhost();
	return true;
}

bool FAPSConstructionMode::RemoveHovered()
{
	AActor* Actor = HoveredProp.Get();
	if (!bActive || HasSelection() || !Actor)
	{
		return false;
	}
	const APSConstruction::FPropType* Type = APSConstruction::FindProp(APSConstruction::PropIdOf(Actor));
	if (FAPSInfrastructure* Infra = APSInfrastructureFind(World.Get()))
	{
		Infra->RemovePlacedProp(Actor);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] removed %s"), *Actor->GetName());
	Actor->Destroy();
	HoveredProp.Reset();
	DestroyGhost();
	Flash(FText::Format(LOCTEXT("Removed", "{0} REMOVED"), Type ? Type->Name : LOCTEXT("PlacedObject", "OBJECT")), false, 1.5f);
	return true;
}

bool FAPSConstructionMode::HandleEscape()
{
	if (!bActive || !APSConstructionModeLocal::IsGameWindowActive())
	{
		return false;
	}
	if (!CancelSelection())
	{
		bExitRequested = true;
	}
	return true;
}

const TArray<APSConstruction::FEntry>& FAPSConstructionMode::GetEntries(const APSConstruction::ESection InSection) const
{
	return InSection == APSConstruction::ESection::Props ? PropEntries : InfrastructureEntries;
}

FText FAPSConstructionMode::GetSiteName() const
{
	const AActor* Site = Frame.Site.Get();
	if (const ACelestialBody* Body = Cast<ACelestialBody>(Site); Body && !Body->AstroName.IsNone())
	{
		return FText::FromString(Body->AstroName.ToString().ToUpper());
	}
	return Site ? FText::FromString(Site->GetName()) : FText::GetEmpty();
}

FText FAPSConstructionMode::GetStatus() const
{
	return Clock < MessageUntil ? Message : PlacementStatus;
}

bool FAPSConstructionMode::IsStatusError() const
{
	return Clock < MessageUntil ? bMessageIsError : bPlacementStatusError;
}

bool FAPSConstructionMode::IsPointerOverPalette() const
{
	return Palette.IsValid() && Palette->IsPointerOverBar();
}

APlayerController* FAPSConstructionMode::GetController() const
{
	const APawn* Pawn = Builder.Get();
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

void FAPSConstructionMode::BuildPropEntries()
{
	using namespace APSConstructionModeLocal;
	PropEntries.Reset();
	for (const FPropType& Type : Props())
	{
		const FVector Size = SlotBounds(Type).GetSize();
		FEntry& Entry = PropEntries.AddDefaulted_GetRef();
		Entry.Section = ESection::Props;
		Entry.Id = Type.Id;
		Entry.Name = Type.Name;
		Entry.Detail = FText::Format(LOCTEXT("PropSize", "{0} x {1} x {2} M"), Metres(Size.X), Metres(Size.Y), Metres(Size.Z));
	}
	++EntriesRevision;
}

void FAPSConstructionMode::RefreshEntries(const bool bForce)
{
	using namespace APSConstructionModeLocal;
	if (!bForce && Clock < NextEntriesRefresh)
	{
		return;
	}
	NextEntriesRefresh = Clock + 0.5;
	TArray<FEntry> Fresh;
	FString Stocks;
	const FAPSInfrastructure* Infra = APSInfrastructureFind(World.Get());
	const AActor* Site = Frame.Site.Get();
	if (Infra && Site)
	{
		// The runtime's options for this body, its own refusals included; only those raised where the builder is.
		const APSInfrastructure::EPlacement Wanted = Frame.Placement == EPlacement::Orbit
			? APSInfrastructure::EPlacement::Orbit : APSInfrastructure::EPlacement::Surface;
		TArray<TPair<FName, FText>> Options;
		Infra->GetOptions(Site, Options);
		for (const TPair<FName, FText>& Option : Options)
		{
			const APSInfrastructure::FType* Type = APSInfrastructure::Find(Option.Key);
			if (!Type || Type->Placement != Wanted)
			{
				continue;
			}
			FEntry& Entry = Fresh.AddDefaulted_GetRef();
			Entry.Section = ESection::Infrastructure;
			Entry.Id = Type->Id;
			Entry.Name = Type->Name;
			Entry.Detail = APSInfrastructure::DescribeAmounts(Type->Cost);
			Entry.Refusal = Option.Value;
		}
		for (int32 Index = 0; Index < static_cast<int32>(APSInfrastructure::EResource::Count); ++Index)
		{
			const APSInfrastructure::EResource Resource = static_cast<APSInfrastructure::EResource>(Index);
			if (!Stocks.IsEmpty())
			{
				Stocks += TEXT("   ");
			}
			Stocks += APSInfrastructure::ResourceName(Resource).ToString() + TEXT(" ")
				+ WholeNumber(FMath::FloorToDouble(Infra->GetStock(Resource))).ToString();
		}
	}
	StocksText = FText::FromString(Stocks);
	bool bChanged = Fresh.Num() != InfrastructureEntries.Num();
	for (int32 Index = 0; !bChanged && Index < Fresh.Num(); ++Index)
	{
		const FEntry& Old = InfrastructureEntries[Index];
		bChanged = Fresh[Index].Id != Old.Id || !Fresh[Index].Refusal.EqualTo(Old.Refusal) || !Fresh[Index].Detail.EqualTo(Old.Detail);
	}
	if (bChanged)
	{
		InfrastructureEntries = MoveTemp(Fresh);
		++EntriesRevision;
	}
}

FText FAPSConstructionMode::SelectedName() const
{
	if (SelectedSection == APSConstruction::ESection::Props)
	{
		const APSConstruction::FPropType* Type = APSConstruction::FindProp(SelectedId);
		return Type ? Type->Name : FText::FromName(SelectedId);
	}
	const APSInfrastructure::FType* Type = APSInfrastructure::Find(SelectedId);
	return Type ? Type->Name : FText::FromName(SelectedId);
}

void FAPSConstructionMode::UpdateGhost()
{
	using namespace APSConstructionModeLocal;
	bPlacementValid = false;
	UWorld* LiveWorld = World.Get();
	APawn* Pawn = Builder.Get();
	APlayerController* Controller = GetController();
	AActor* Site = Frame.Site.Get();
	if (!LiveWorld || !Pawn || !Controller || !Site)
	{
		HideGhost();
		PlacementStatus = FText::GetEmpty();
		return;
	}
	if (Frame.Placement == EPlacement::Orbit)
	{
		UpdateOrbitGhost(*Controller, *Site);
		return;
	}

	const bool bOverPalette = IsPointerOverPalette();
	FHitResult Hit;
	bool bHit = false;
	FVector RayOrigin;
	FVector RayDirection;
	if (!bOverPalette && DeprojectCursor(*Controller, RayOrigin, RayDirection))
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(APSBuildCursor), false, Pawn);
		if (const AActor* Ghost = GhostActor.Get())
		{
			Params.AddIgnoredActor(Ghost);
		}
		if (const AActor* Ring = RingActor.Get())
		{
			Params.AddIgnoredActor(Ring);
		}
		bHit = LiveWorld->LineTraceSingleByChannel(Hit, RayOrigin, RayOrigin + RayDirection * TraceLengthCm, ECC_Visibility,
			Params);
	}

	if (!HasSelection())
	{
		// Nothing picked: a placed prop under the cursor, inside the zone, can be removed with X.
		AActor* Candidate = bHit ? Hit.GetActor() : nullptr;
		if (Candidate && FVector::VectorPlaneProject(Candidate->GetActorLocation() - Frame.BuilderLocation,
			Frame.BuilderUp).Size() > Frame.RadiusCm)
		{
			Candidate = nullptr;
		}
		ShowHoveredRemoval(Candidate);
		return;
	}
	HoveredProp.Reset();

	const FText Name = SelectedName();
	if (!EnsureSelectionGhost())
	{
		HideGhost();
		PlacementStatus = FText::Format(LOCTEXT("NoGhost", "{0}: no model to show"), Name);
		bPlacementStatusError = true;
		return;
	}
	if (bOverPalette || !bHit)
	{
		HideGhost();
		PlacementStatus = FText::Format(bOverPalette ? LOCTEXT("AimFromPalette", "{0}: move the cursor onto the ground")
			: LOCTEXT("AimAtGround", "{0}: aim at the ground"), Name);
		bPlacementStatusError = false;
		return;
	}

	const bool bStructure = SelectedSection == ESection::Infrastructure;
	const FPropType* PropType = bStructure ? nullptr : FindProp(SelectedId);
	const FVector Centre = Site->GetActorLocation();
	const FVector Up = (Hit.ImpactPoint - Centre).GetSafeNormal();
	if (Up.IsNearlyZero())
	{
		HideGhost();
		return;
	}
	const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
	const float Slope = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		static_cast<float>(FVector::DotProduct(Normal, Up)), -1.0f, 1.0f)));

	// Standing: along gravity (structures, masts), or tilted with the ground by at most 15 degrees (most props).
	FVector StandUp = Up;
	if (PropType && PropType->bAlignToGround && Slope > 0.5f && !Normal.IsNearlyZero())
	{
		const float Tilt = FMath::Min(Slope, MaxTiltDegrees);
		StandUp = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Up, Normal), Tilt / Slope).RotateVector(Up).GetSafeNormal();
	}
	// Heading: a yaw about gravity from the body's north, so a row of barriers lines up; the first ghost faces the builder.
	FVector North = FVector::VectorPlaneProject(Site->GetActorUpVector(), Up).GetSafeNormal();
	if (North.IsNearlyZero())
	{
		North = FVector::VectorPlaneProject(Site->GetActorForwardVector(), Up).GetSafeNormal();
	}
	if (!bYawInitialized)
	{
		const FVector Toward = FVector::VectorPlaneProject(Frame.BuilderLocation - Hit.ImpactPoint, Up).GetSafeNormal();
		if (!Toward.IsNearlyZero())
		{
			const FVector East = FVector::CrossProduct(Up, North);
			YawDegrees = FMath::GridSnap(FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(
				FVector::DotProduct(Toward, East), FVector::DotProduct(Toward, North)))), 15.0f);
		}
		bYawInitialized = true;
	}
	const FQuat Heading = FQuat(Up, FMath::DegreesToRadians(YawDegrees)) * FRotationMatrix::MakeFromZX(Up, North).ToQuat();
	const FQuat Rotation = FQuat::FindBetweenNormals(Up, StandUp) * Heading;
	// A prop's underside centre on the hit point; a structure stands on its origin, as the runtime's settled ones do.
	FVector Location = Hit.ImpactPoint;
	if (!bStructure && GhostBounds.IsValid)
	{
		const FVector Underside(GhostBounds.GetCenter().X, GhostBounds.GetCenter().Y, GhostBounds.Min.Z);
		Location = Hit.ImpactPoint - Rotation.RotateVector(Underside * GhostScale) - StandUp * SinkCm;
	}
	PlacementTransform = FTransform(Rotation, Location, GhostScale);
	const AActor* Ground = Hit.GetActor();
	bPlacementOnGround = !(Ground && Ground->ActorHasTag(PlacedTag()));

	FText Problem;
	const FVector Offset = Hit.ImpactPoint - Frame.BuilderLocation;
	const double Across = FVector::VectorPlaneProject(Offset, Frame.BuilderUp).Size();
	const double Rise = FMath::Abs(FVector::DotProduct(Offset, Frame.BuilderUp));
	const float MaxSlope = PropType ? PropType->MaxSlopeDegrees : StructureMaxSlopeDegrees;
	if (Across > Frame.RadiusCm || Rise > MaxRiseCm)
	{
		Problem = FText::Format(LOCTEXT("OutOfRange", "out of the zone ({0} m, the ring is {1} m)"), WholeNumber(Across / 100.0),
			WholeNumber(Frame.RadiusCm / 100.0));
	}
	else if (Slope > MaxSlope)
	{
		Problem = FText::Format(LOCTEXT("TooSteep", "too steep ({0} deg, at most {1})"), WholeNumber(Slope), WholeNumber(MaxSlope));
	}
	else
	{
		Problem = FindBlocker(PlacementTransform, Ground, bStructure);
	}
	if (Problem.IsEmpty() && bStructure)
	{
		// The runtime's own rules (knowledge, unlocks, levels, the limit here, the stocks), a few times a second.
		if (Clock >= NextRefusalCheck)
		{
			NextRefusalCheck = Clock + 0.25;
			const FAPSInfrastructure* Infra = APSInfrastructureFind(LiveWorld);
			StructureRefusal = Infra ? Infra->CheckBuild(SelectedId, Site)
				: LOCTEXT("NoInfrastructure", "The infrastructure does not run in this world.");
		}
		Problem = StructureRefusal;
	}
	bPlacementValid = Problem.IsEmpty();
	ShowGhost(PlacementTransform, bPlacementValid ? TintValid : TintInvalid);
	PlacementStatus = bPlacementValid ? FText::Format(LOCTEXT("PlaceHint", "{0}: LMB PLACE"), Name)
		: FText::Format(LOCTEXT("CannotPlace", "{0}: {1}"), Name, Problem);
	bPlacementStatusError = !bPlacementValid;
}

bool FAPSConstructionMode::ShowHoveredRemoval(AActor* Candidate)
{
	using namespace APSConstructionModeLocal;
	if (Candidate && !Candidate->ActorHasTag(PlacedTag()))
	{
		Candidate = nullptr;
	}
	HoveredProp = Candidate;
	bPlacementStatusError = false;
	const FPropType* Type = Candidate ? FindProp(PropIdOf(Candidate)) : nullptr;
	if (!Type)
	{
		HideGhost();
		PlacementStatus = LOCTEXT("PickHint", "Pick an object: click a card or press its number.");
		return false;
	}
	const FString Key = TEXT("remove:") + Type->Id.ToString();
	if (GhostKey != Key || !GhostActor.IsValid())
	{
		TArray<FMeshPiece> Pieces;
		ResolveProp(*Type, nullptr, Pieces);
		EnsureGhost(Key, Pieces);
		GhostScale = FVector::OneVector;
	}
	// A shell a little larger than the prop, so it does not flicker in its surface.
	FTransform Shell = Candidate->GetActorTransform();
	Shell.SetScale3D(Shell.GetScale3D() * 1.04);
	ShowGhost(Shell, TintRemove);
	PlacementStatus = FText::Format(LOCTEXT("RemoveHint", "X  REMOVE {0}"), Type->Name);
	return true;
}

void FAPSConstructionMode::UpdateOrbitGhost(APlayerController& Controller, AActor& Site)
{
	using namespace APSConstructionModeLocal;
	UWorld* LiveWorld = World.Get();
	APawn* Pawn = Builder.Get();
	const bool bOverPalette = IsPointerOverPalette();
	FVector RayOrigin;
	FVector RayDirection;
	const bool bRay = !bOverPalette && DeprojectCursor(Controller, RayOrigin, RayDirection);
	if (!HasSelection())
	{
		// Nothing picked: a placed object under the cursor, within the zone, can be removed with X, as on the ground.
		AActor* Candidate = nullptr;
		if (bRay && LiveWorld)
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(APSBuildCursorOrbit), false, Pawn);
			if (const AActor* Ghost = GhostActor.Get())
			{
				Params.AddIgnoredActor(Ghost);
			}
			if (const AActor* Ring = RingActor.Get())
			{
				Params.AddIgnoredActor(Ring);
			}
			if (LiveWorld->LineTraceSingleByChannel(Hit, RayOrigin, RayOrigin + RayDirection * Frame.RadiusCm * 2.0,
				ECC_Visibility, Params))
			{
				Candidate = Hit.GetActor();
			}
		}
		if (Candidate && FVector::Dist(Candidate->GetActorLocation(), Frame.BuilderLocation) > Frame.RadiusCm)
		{
			Candidate = nullptr;
		}
		ShowHoveredRemoval(Candidate);
		return;
	}
	HoveredProp.Reset();
	const FText Name = SelectedName();
	if (!EnsureSelectionGhost())
	{
		HideGhost();
		PlacementStatus = FText::Format(LOCTEXT("NoGhost", "{0}: no model to show"), Name);
		bPlacementStatusError = true;
		return;
	}
	if (!bRay)
	{
		HideGhost();
		PlacementStatus = FText::Format(LOCTEXT("AimIntoSpace", "{0}: move the cursor into space"), Name);
		bPlacementStatusError = false;
		return;
	}
	const bool bStructure = SelectedSection == ESection::Infrastructure;
	const double GhostRadius = GhostBounds.IsValid ? (GhostBounds.GetExtent() * GhostScale).Size() : 500.0;
	if (OrbitDistanceCm <= 0.0)
	{
		// A new selection floats beyond the ship's hull (02.10 run: a pad 30 m from the camera sat in the M3's nose): past
		// the ship from the camera, its own size and ten metres more, and at least three of its sizes out.
		double BuilderRadius = 0.0;
		if (Pawn)
		{
			FVector BuilderOrigin;
			FVector BuilderExtent;
			Pawn->GetActorBounds(true, BuilderOrigin, BuilderExtent);
			BuilderRadius = BuilderExtent.Size();
		}
		const double PastBuilder = FVector::Dist(RayOrigin, Frame.BuilderLocation) + BuilderRadius + GhostRadius + 1000.0;
		OrbitDistanceCm = FMath::Clamp(FMath::Max3(3000.0, GhostRadius * 3.0, PastBuilder), 1500.0,
			FMath::Max(Frame.RadiusCm * 1.8, 1500.0));
	}
	// A free turn in the builder's frame (Rio 02.10: "in orbit it can be rotated freely"): yaw about the frame's up (Q/E),
	// tilt about the turned right (R); the frame's forward is the ship's nose laid into the frame's plane.
	const FVector Up = Frame.BuilderUp.GetSafeNormal();
	FVector Forward = Pawn ? FVector::VectorPlaneProject(Pawn->GetActorForwardVector(), Up).GetSafeNormal() : FVector::ZeroVector;
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::VectorPlaneProject(RayDirection, Up).GetSafeNormal();
	}
	if (Up.IsNearlyZero() || Forward.IsNearlyZero())
	{
		HideGhost();
		return;
	}
	const FQuat Base = FRotationMatrix::MakeFromZX(Up, Forward).ToQuat();
	const FQuat Rotation = Base * FQuat(FVector::UpVector, FMath::DegreesToRadians(YawDegrees))
		* FQuat(FVector::RightVector, FMath::DegreesToRadians(PitchDegrees));
	// The ghost's middle on the cursor's ray; a structure stands on its origin, as the runtime places it.
	FVector Location = RayOrigin + RayDirection * OrbitDistanceCm;
	if (!bStructure && GhostBounds.IsValid)
	{
		Location -= Rotation.RotateVector(GhostBounds.GetCenter() * GhostScale);
	}
	PlacementTransform = FTransform(Rotation, Location, GhostScale);
	bPlacementOnGround = false;

	FText Problem;
	const double Away = FVector::Dist(Location, Frame.BuilderLocation);
	if (Away > Frame.RadiusCm)
	{
		Problem = FText::Format(LOCTEXT("OutOfOrbitZone", "out of the zone ({0} m from the ship, the zone is {1} m)"),
			WholeNumber(Away / 100.0), WholeNumber(Frame.RadiusCm / 100.0));
	}
	else
	{
		Problem = FindBlocker(PlacementTransform, nullptr, bStructure);
	}
	if (Problem.IsEmpty() && bStructure)
	{
		// The runtime's own rules (knowledge, unlocks, levels, the limit here, the stocks), a few times a second.
		if (Clock >= NextRefusalCheck)
		{
			NextRefusalCheck = Clock + 0.25;
			const FAPSInfrastructure* Infra = APSInfrastructureFind(LiveWorld);
			StructureRefusal = Infra ? Infra->CheckBuild(SelectedId, &Site)
				: LOCTEXT("NoInfrastructure", "The infrastructure does not run in this world.");
		}
		Problem = StructureRefusal;
	}
	bPlacementValid = Problem.IsEmpty();
	ShowGhost(PlacementTransform, bPlacementValid ? TintValid : TintInvalid);
	PlacementStatus = bPlacementValid
		? FText::Format(LOCTEXT("PlaceHintOrbit", "{0}: LMB PLACE  ({1} m out)"), Name, WholeNumber(OrbitDistanceCm / 100.0))
		: FText::Format(LOCTEXT("CannotPlace", "{0}: {1}"), Name, Problem);
	bPlacementStatusError = !bPlacementValid;
}

FText FAPSConstructionMode::FindBlocker(const FTransform& Transform, const AActor* Ground, const bool bStructure) const
{
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || !GhostBounds.IsValid)
	{
		return FText::GetEmpty();
	}
	// The object's box a little smaller and its floor lifted: the ground and neighbours that only touch it do not count.
	const FVector Scale = Transform.GetScale3D();
	FVector Extent = GhostBounds.GetExtent() * Scale;
	FVector Middle = GhostBounds.GetCenter() * Scale;
	const double Lift = FMath::Max(2.0, Extent.Z * 0.12);
	Middle.Z += Lift * 0.5;
	Extent = FVector(Extent.X * 0.92, Extent.Y * 0.92, FMath::Max(1.0, Extent.Z - Lift * 0.5));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSBuildOverlap), false);
	if (const AActor* Ghost = GhostActor.Get())
	{
		Params.AddIgnoredActor(Ghost);
	}
	if (const AActor* Ring = RingActor.Get())
	{
		Params.AddIgnoredActor(Ring);
	}
	if (Ground)
	{
		// What it stands on: the terrain (one actor for all of a body's collision), or the prop it is stacked on.
		Params.AddIgnoredActor(Ground);
	}
	TArray<FOverlapResult> Overlaps;
	LiveWorld->OverlapMultiByChannel(Overlaps, Transform.GetLocation() + Transform.GetRotation().RotateVector(Middle),
		Transform.GetRotation(), ECC_Pawn, FCollisionShape::MakeBox(Extent), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (!Overlap.bBlockingHit)
		{
			continue;
		}
		const UPrimitiveComponent* Component = Overlap.GetComponent();
		const AActor* Actor = Overlap.GetActor();
		// WorldScape's vegetation is instanced: it stops a prop, but a structure stands among it (the instances cannot be
		// cleared from here).
		const bool bVegetation = Component && Component->IsA<UInstancedStaticMeshComponent>();
		if (bVegetation && bStructure)
		{
			continue;
		}
		if (Actor && Actor == Builder.Get())
		{
			return IsOrbit()
				? LOCTEXT("BlockedByShip", "the ship is in the way (the wheel floats it further)")
				: LOCTEXT("BlockedByBuilder", "step aside, you are in the way");
		}
		if (bVegetation)
		{
			return LOCTEXT("BlockedByVegetation", "vegetation in the way");
		}
		if (Actor && Actor->ActorHasTag(APSConstruction::PlacedTag()))
		{
			const APSConstruction::FPropType* Type = APSConstruction::FindProp(APSConstruction::PropIdOf(Actor));
			return FText::Format(LOCTEXT("BlockedByProp", "{0} in the way"), Type ? Type->Name : LOCTEXT("APlacedObject", "an object"));
		}
		// Named, so a report says what stood there.
		return FText::Format(LOCTEXT("BlockedByObject", "something is in the way ({0})"),
			FText::FromString(Actor ? Actor->GetName() : GetNameSafe(Component)));
	}
	return FText::GetEmpty();
}

bool FAPSConstructionMode::EnsureSelectionGhost()
{
	using namespace APSConstructionModeLocal;
	const FString Key = FString::Printf(TEXT("%d:%s"), static_cast<int32>(SelectedSection), *SelectedId.ToString());
	if (GhostKey == Key && GhostActor.IsValid())
	{
		return true;
	}
	TArray<FMeshPiece> Pieces;
	FVector Scale = FVector::OneVector;
	if (SelectedSection == ESection::Props)
	{
		const FPropType* Type = FindProp(SelectedId);
		if (!Type)
		{
			return false;
		}
		ResolveProp(*Type, nullptr, Pieces);
	}
	else
	{
		const APSInfrastructure::FType* Type = APSInfrastructure::Find(SelectedId);
		if (!Type)
		{
			return false;
		}
		GatherClassPieces(StructureClass(World.Get(), *Type), Pieces);
		if (Pieces.IsEmpty())
		{
			// A look the ghost cannot read: a plain thirty-metre column where it will stand.
			FMeshPiece& Proxy = Pieces.AddDefaulted_GetRef();
			Proxy.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), nullptr,
				LOAD_NoWarn | LOAD_Quiet);
			Proxy.Transform = FTransform(FQuat::Identity, FVector(0.0, 0.0, 1500.0), FVector(30.0));
		}
		Scale = FVector(StructureScale(*Type));
	}
	if (!EnsureGhost(Key, Pieces))
	{
		return false;
	}
	GhostScale = Scale;
	return true;
}

bool FAPSConstructionMode::EnsureGhost(const FString& Key, const TArray<APSConstruction::FMeshPiece>& Pieces)
{
	using namespace APSConstructionModeLocal;
	if (GhostKey == Key && GhostActor.IsValid())
	{
		return true;
	}
	DestroyGhost();
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld || Pieces.IsEmpty())
	{
		return false;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.ObjectFlags |= RF_Transient;
	AStaticMeshActor* Ghost = LiveWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform::Identity,
		Parameters);
	if (!Ghost)
	{
		return false;
	}
	Ghost->SetMobility(EComponentMobility::Movable);
	Ghost->SetActorEnableCollision(false);
	UStaticMeshComponent* Root = Ghost->GetStaticMeshComponent();
	Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Root->SetCanEverAffectNavigation(false);
	UMaterialInstanceDynamic* Tint = MakeTint(Ghost, TintColour(TintInvalid), GhostOpacity);
	for (const FMeshPiece& Piece : Pieces)
	{
		if (!Piece.Mesh)
		{
			continue;
		}
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Ghost);
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Piece.Mesh);
		Component->SetRelativeTransform(Piece.Transform);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCollisionResponseToAllChannels(ECR_Ignore);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
		if (Tint)
		{
			for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
			{
				Component->SetMaterial(Slot, Tint);
			}
		}
		Component->RegisterComponent();
		Ghost->AddInstanceComponent(Component);
	}
	Ghost->SetActorHiddenInGame(true);
	GhostActor = Ghost;
	GhostMaterial = Tint;
	GhostKey = Key;
	GhostBounds = APSConstruction::PiecesBounds(Pieces);
	GhostTint = TintInvalid;
	bGhostShown = false;
	return true;
}

void FAPSConstructionMode::ShowGhost(const FTransform& Transform, const int32 Tint)
{
	AStaticMeshActor* Ghost = GhostActor.Get();
	if (!Ghost)
	{
		return;
	}
	Ghost->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	if (!bGhostShown)
	{
		Ghost->SetActorHiddenInGame(false);
		bGhostShown = true;
	}
	if (Tint != GhostTint)
	{
		if (UMaterialInstanceDynamic* Material = GhostMaterial.Get())
		{
			APSConstructionModeLocal::SetTint(Material, APSConstructionModeLocal::TintColour(Tint),
				APSConstructionModeLocal::GhostOpacity);
		}
		GhostTint = Tint;
	}
}

void FAPSConstructionMode::HideGhost()
{
	if (AStaticMeshActor* Ghost = GhostActor.Get(); Ghost && bGhostShown)
	{
		Ghost->SetActorHiddenInGame(true);
	}
	bGhostShown = false;
}

void FAPSConstructionMode::DestroyGhost()
{
	if (AStaticMeshActor* Ghost = GhostActor.Get())
	{
		Ghost->Destroy();
	}
	GhostActor.Reset();
	GhostMaterial.Reset();
	GhostKey.Reset();
	GhostBounds = FBox(ForceInit);
	GhostTint = -1;
	bGhostShown = false;
}

void FAPSConstructionMode::UpdateRing(const bool bForce)
{
	using namespace APSConstructionModeLocal;
	UWorld* LiveWorld = World.Get();
	if (!LiveWorld)
	{
		return;
	}
	// Redrawn five times a second while the builder moves, once a second while it stands (49 short traces each).
	const FVector Centre = Frame.BuilderLocation;
	const bool bMoved = FVector::DistSquared(Centre, RingCentre) > FMath::Square(50.0);
	if (!bForce && (Clock < NextRingUpdate || (!bMoved && Clock < NextRingUpdate + 0.8)))
	{
		return;
	}
	NextRingUpdate = Clock + 0.2;
	RingCentre = Centre;
	if (!RingActor.IsValid())
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Parameters.ObjectFlags |= RF_Transient;
		AActor* NewRing = LiveWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Centre), Parameters);
		if (!NewRing)
		{
			return;
		}
		UProceduralMeshComponent* NewMesh = NewObject<UProceduralMeshComponent>(NewRing, TEXT("BuildZoneRing"));
		NewMesh->SetMobility(EComponentMobility::Movable);
		NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewMesh->SetGenerateOverlapEvents(false);
		NewMesh->SetCanEverAffectNavigation(false);
		NewMesh->SetCastShadow(false);
		NewRing->SetRootComponent(NewMesh);
		NewMesh->RegisterComponent();
		NewRing->AddInstanceComponent(NewMesh);
		if (UMaterialInstanceDynamic* Tint = MakeTint(NewRing, RingColour(), RingOpacity))
		{
			NewMesh->SetMaterial(0, Tint);
		}
		RingActor = NewRing;
		RingMesh = NewMesh;
		bRingSectionCreated = false;
	}
	AActor* Ring = RingActor.Get();
	UProceduralMeshComponent* Mesh = RingMesh.Get();
	if (!Ring || !Mesh)
	{
		return;
	}
	// A flat band on the ground at the zone's edge: each point found by a short trace along gravity.
	const FVector Up = Frame.BuilderUp.GetSafeNormal();
	const FQuat Basis = FRotationMatrix::MakeFromZ(Up).ToQuat();
	Ring->SetActorLocationAndRotation(Centre, Basis, false, nullptr, ETeleportType::TeleportPhysics);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(APSBuildZoneRing), false, Builder.Get());
	Params.AddIgnoredActor(Ring);
	if (const AActor* Ghost = GhostActor.Get())
	{
		Params.AddIgnoredActor(Ghost);
	}
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Colours;
	const TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve((RingSegments + 1) * 2);
	Normals.Reserve((RingSegments + 1) * 2);
	UV.Reserve((RingSegments + 1) * 2);
	Colours.Reserve((RingSegments + 1) * 2);
	Triangles.Reserve(RingSegments * 6);
	// A band as wide as the zone asks: 60 cm round a walker, metres round a ship's 2 km zone (60 cm there was invisible).
	const double RingWidth = FMath::Clamp(Frame.RadiusCm * 0.008, RingWidthCm, 5000.0);
	for (int32 Index = 0; Index <= RingSegments; ++Index)
	{
		const double Angle = UE_DOUBLE_TWO_PI * static_cast<double>(Index) / RingSegments;
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
		const FVector Point = Centre + Basis.RotateVector(Direction * Frame.RadiusCm);
		double Height = -Frame.BuilderFeetCm;
		FHitResult Hit;
		if (LiveWorld->LineTraceSingleByChannel(Hit, Point + Up * MaxRiseCm, Point - Up * MaxRiseCm, ECC_Visibility, Params))
		{
			Height = FVector::DotProduct(Hit.ImpactPoint - Centre, Up);
		}
		Height += RingLiftCm;
		Vertices.Add(Direction * (Frame.RadiusCm - RingWidth * 0.5) + FVector(0.0, 0.0, Height));
		Vertices.Add(Direction * (Frame.RadiusCm + RingWidth * 0.5) + FVector(0.0, 0.0, Height));
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		const double Along = static_cast<double>(Index) / RingSegments;
		UV.Add(FVector2D(Along, 0.0));
		UV.Add(FVector2D(Along, 1.0));
		Colours.Add(FLinearColor::White);
		Colours.Add(FLinearColor::White);
		if (Index > 0)
		{
			const int32 Previous = (Index - 1) * 2;
			Triangles.Append({Previous, Previous + 2, Previous + 1, Previous + 1, Previous + 2, Previous + 3});
		}
	}
	if (!bRingSectionCreated)
	{
		Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colours, Tangents, false);
		bRingSectionCreated = true;
	}
	else
	{
		Mesh->UpdateMeshSection_LinearColor(0, Vertices, Normals, UV, Colours, Tangents);
	}
}

bool FAPSConstructionMode::PlaceProp()
{
	UWorld* LiveWorld = World.Get();
	AActor* Site = Frame.Site.Get();
	const APSConstruction::FPropType* Type = APSConstruction::FindProp(SelectedId);
	if (!LiveWorld || !Site || !Type)
	{
		return false;
	}
	const FString MeshPath = APSConstruction::PrimaryMeshPath(*Type);
	AStaticMeshActor* Actor = APSConstruction::SpawnProp(LiveWorld, Type->Id, MeshPath, PlacementTransform, Site);
	if (!Actor)
	{
		Flash(LOCTEXT("PropFailed", "Could not place it here."), true);
		return false;
	}
	if (FAPSInfrastructure* Infra = APSInfrastructureFind(LiveWorld))
	{
		Infra->AddPlacedProp(Actor, Site, Type->Id, MeshPath, bPlacementOnGround);
		Flash(FText::Format(LOCTEXT("PropPlaced", "{0} PLACED"), Type->Name), false, 1.5f);
	}
	else
	{
		Flash(FText::Format(LOCTEXT("PropPlacedUnsaved", "{0} placed; no save keeps it in this world"), Type->Name), true);
	}
	UE_LOG(LogTemp, Log, TEXT("[APS.Construction] placed %s (%s) at %s on %s, %s"), *Type->Id.ToString(), *Actor->GetName(),
		*PlacementTransform.GetLocation().ToCompactString(), *GetSiteName().ToString(),
		IsOrbit() ? TEXT("in orbit") : bPlacementOnGround ? TEXT("on the ground") : TEXT("stacked"));
	return true;
}

bool FAPSConstructionMode::PlaceStructure()
{
	UWorld* LiveWorld = World.Get();
	AActor* Site = Frame.Site.Get();
	FAPSInfrastructure* Infra = APSInfrastructureFind(LiveWorld);
	const APSInfrastructure::FType* Type = APSInfrastructure::Find(SelectedId);
	if (!Infra || !Site || !Type)
	{
		Flash(LOCTEXT("NoInfrastructureHere", "The infrastructure does not run in this world."), true);
		return false;
	}
	// The runtime's own rules once more at the click (the stocks may have changed since the last look).
	const FText Refusal = Infra->CheckBuild(Type->Id, Site);
	if (!Refusal.IsEmpty())
	{
		Flash(Refusal, true);
		return false;
	}
	if (!Infra->Reserve(Type->Id))
	{
		Flash(FText::Format(LOCTEXT("CannotAfford", "Needs {0}."), APSInfrastructure::DescribeAmounts(Type->Cost)), true);
		return false;
	}
	if (!Infra->CompleteAt(Type->Id, Site, PlacementTransform))
	{
		Infra->Refund(Type->Id);
		Flash(LOCTEXT("StructureFailed", "Could not raise it here."), true);
		return false;
	}
	Flash(FText::Format(LOCTEXT("StructureRaised", "{0} RAISED"), Type->Name), false, 2.0f);
	NextRefusalCheck = 0.0;
	RefreshEntries(true);
	return true;
}

void FAPSConstructionMode::Flash(const FText& Text, const bool bError, const float Seconds)
{
	Message = Text;
	bMessageIsError = bError;
	MessageUntil = Clock + Seconds;
}

#undef LOCTEXT_NAMESPACE

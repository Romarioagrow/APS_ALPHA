#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
class APlayerController;
class AStaticMeshActor;
class IInputProcessor;
class SAPSBuildPalette;
class SWeakWidget;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UWorld;

namespace APSConstruction
{
	struct FMeshPiece;

	/** The palette's two sections: build mode's own props, and the infrastructure catalogue's types for this place. */
	enum class ESection : uint8
	{
		Props,
		Infrastructure
	};

	/**
	 * How the ghost finds its place. Surface: on the ground under the cursor, standing along gravity (on foot).
	 * Orbit: free in space on the cursor's ray beside a ship, any turn (the ship hosts it: UAPSShipBuildComponent).
	 */
	enum class EPlacement : uint8
	{
		Surface,
		Orbit
	};

	/** The builder's frame, given by the host pawn every tick. */
	struct FFrame
	{
		/** The body the structures and props belong to (a planet or moon). */
		TWeakObjectPtr<AActor> Site;
		/** The centre of the build zone (the builder) and gravity's up there. */
		FVector BuilderLocation{FVector::ZeroVector};
		FVector BuilderUp{FVector::UpVector};
		/** From the builder's centre down to its feet: where the zone ring is drawn when the ground under it is not found. */
		double BuilderFeetCm{90.0};
		double RadiusCm{7000.0};
		EPlacement Placement{EPlacement::Surface};
	};

	/** One card of the palette. */
	struct FEntry
	{
		ESection Section{ESection::Props};
		FName Id;
		FText Name;
		/** Size for a prop, cost for a structure. */
		FText Detail;
		/** Why it cannot be placed now (the infrastructure runtime's own check); empty when it can. */
		FText Refusal;
	};
}

/**
 * Build mode (Rio 02.10: "a separate build mode ... a menu where we pick what to spawn and drag it with the mouse; the
 * camera pulls back a little; there is a zone where we can build; on a planet it aligns to the ground plane, in orbit it
 * can be rotated freely; it must not replace the strategic one").
 *
 * The core is host-agnostic: it owns the palette (UI/Construction/SAPSBuildPalette), the ghost, the zone ring and the
 * Escape key while active, validates the ghost every tick and places through APSConstruction (props) or the
 * infrastructure runtime (structures, which also keeps the props for the save). The host pawn gives it the frame each
 * tick, forwards its keys and moves its own camera: ACustomGravityCharacter on foot now. In orbit a ship would host it
 * the same way with FFrame::Placement = Orbit (the ghost on the cursor ray at a set distance, free rotation, the
 * infrastructure types of EPlacement::Orbit); that branch is not wired yet and shows a notice.
 */
class APS_ALPHA_API FAPSConstructionMode : public TSharedFromThis<FAPSConstructionMode>
{
public:
	explicit FAPSConstructionMode(APawn* InBuilder);
	~FAPSConstructionMode();

	/** Shows the palette and the zone ring and takes Escape. The mode must already be held by a TSharedPtr. */
	void Begin(const APSConstruction::FFrame& InFrame);
	/** Removes everything the mode spawned or showed. Safe to call twice. */
	void End();
	void Tick(float DeltaSeconds, const APSConstruction::FFrame& InFrame);
	bool IsActive() const { return bActive; }

	// Keys and clicks, forwarded by the host.
	/** 1-9, 0: the card in that place of the open section (0-based). */
	void SelectSlot(int32 Slot);
	/** A card; the selected one again drops the selection. */
	void Select(APSConstruction::ESection InSection, FName Id);
	void SetSection(APSConstruction::ESection InSection);
	void ToggleSection();
	void Rotate(float Degrees);
	/** Orbit: tilts the ghost about its right axis (R, Shift+R). */
	void RotatePitch(float Degrees);
	/** Orbit: the ghost further along the cursor's ray (positive) or nearer (negative), in steps. */
	void AdjustDistance(float Steps);
	bool IsOrbit() const { return Frame.Placement == APSConstruction::EPlacement::Orbit; }
	/** LMB: places the selection where the ghost stands; false (with the reason shown) when it cannot. */
	bool Place();
	/** RMB / Escape: drops the selection; false when nothing was selected. */
	bool CancelSelection();
	/** X: removes the placed prop under the cursor (no selection). */
	bool RemoveHovered();
	/** Escape: drops the selection, else asks to leave. False (not handled) when the game window is not the active one. */
	bool HandleEscape();
	void RequestExit() { bExitRequested = true; }
	bool IsExitRequested() const { return bExitRequested; }
	UWorld* GetWorld() const { return World.Get(); }

	// What the palette shows.
	bool HasSelection() const { return !SelectedId.IsNone(); }
	APSConstruction::ESection GetSection() const { return Section; }
	APSConstruction::ESection GetSelectedSection() const { return SelectedSection; }
	FName GetSelectedId() const { return SelectedId; }
	const TArray<APSConstruction::FEntry>& GetEntries(APSConstruction::ESection InSection) const;
	/** Changes whenever a card's text changes (the palette rebuilds its cards then). */
	uint32 GetEntriesRevision() const { return EntriesRevision; }
	FText GetSiteName() const;
	FText GetStatus() const;
	bool IsStatusError() const;
	/** The civilization's stocks, for the costs of the structures. */
	FText GetStocks() const { return StocksText; }
	bool IsPointerOverPalette() const;

private:
	APlayerController* GetController() const;
	void RefreshEntries(bool bForce);
	void BuildPropEntries();
	void UpdateGhost();
	/** The orbit branch of UpdateGhost: the ghost on the cursor's ray at OrbitDistanceCm, turned freely. */
	void UpdateOrbitGhost(APlayerController& Controller, AActor& Site);
	/** Nothing picked: a placed prop under the cursor shows its removal shell; false when there is none. */
	bool ShowHoveredRemoval(AActor* Candidate);
	void UpdateRing(bool bForce);
	void ShowGhost(const FTransform& Transform, int32 Tint);
	void HideGhost();
	void DestroyGhost();
	/** The ghost for the selection, or for the hovered prop (Key changes rebuild it). */
	bool EnsureGhost(const FString& Key, const TArray<APSConstruction::FMeshPiece>& Pieces);
	bool EnsureSelectionGhost();
	FText SelectedName() const;
	/** What stands in the ghost's box (vegetation does not stop a structure); empty when the place is clear. */
	FText FindBlocker(const FTransform& Transform, const AActor* Ground, bool bStructure) const;
	void Flash(const FText& Text, bool bError, float Seconds = 2.5f);
	bool PlaceProp();
	bool PlaceStructure();

	TWeakObjectPtr<APawn> Builder;
	TWeakObjectPtr<UWorld> World;
	APSConstruction::FFrame Frame;
	bool bActive{false};
	bool bExitRequested{false};
	double Clock{0.0};

	APSConstruction::ESection Section{APSConstruction::ESection::Props};
	APSConstruction::ESection SelectedSection{APSConstruction::ESection::Props};
	FName SelectedId;
	float YawDegrees{0.0f};
	/** The first ghost after a selection faces the builder; the yaw then stays until Q/E. */
	bool bYawInitialized{false};
	/** Orbit: the ghost's tilt, and how far along the cursor's ray it floats (set from its size on a new selection). */
	float PitchDegrees{0.0f};
	double OrbitDistanceCm{0.0};

	TArray<APSConstruction::FEntry> PropEntries;
	TArray<APSConstruction::FEntry> InfrastructureEntries;
	uint32 EntriesRevision{1};
	double NextEntriesRefresh{0.0};
	FText StocksText;

	TWeakObjectPtr<AStaticMeshActor> GhostActor;
	TWeakObjectPtr<UMaterialInstanceDynamic> GhostMaterial;
	FString GhostKey;
	FBox GhostBounds{ForceInit};
	FVector GhostScale{FVector::OneVector};
	int32 GhostTint{-1};
	bool bGhostShown{false};
	FTransform PlacementTransform{FTransform::Identity};
	bool bPlacementValid{false};
	bool bPlacementOnGround{true};
	FText PlacementStatus;
	bool bPlacementStatusError{false};
	/** The infrastructure runtime's refusal for the selected structure here, checked a few times a second. */
	FText StructureRefusal;
	double NextRefusalCheck{0.0};
	TWeakObjectPtr<AActor> HoveredProp;

	TWeakObjectPtr<AActor> RingActor;
	TWeakObjectPtr<UProceduralMeshComponent> RingMesh;
	bool bRingSectionCreated{false};
	FVector RingCentre{FVector::ZeroVector};
	double NextRingUpdate{0.0};

	TSharedPtr<SAPSBuildPalette> Palette;
	TSharedPtr<SWeakWidget> PaletteContainer;
	TSharedPtr<IInputProcessor> EscapeProcessor;

	FText Message;
	bool bMessageIsError{false};
	double MessageUntil{0.0};
};

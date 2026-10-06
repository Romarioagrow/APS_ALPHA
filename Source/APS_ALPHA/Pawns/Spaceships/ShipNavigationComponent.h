#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipNavigationComponent.generated.h"

UENUM(BlueprintType)
enum class EShipNavigationContactType : uint8
{
	Star,
	Planet,
	Moon,
	Station,
	Settlement,
	Infrastructure,
	StarSystem,
	StarCluster,
	Unknown
};

/** A lightweight live view of an astronomical or technological object in the current world. */
USTRUCT(BlueprintType)
struct FShipNavigationContact
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> Actor;
	FVector FixedWorldLocation{FVector::ZeroVector};
	FString StableId;
	FString DisplayName;
	FString TypeLabel;
	FString Detail;
	FString HierarchyLabel;
	EShipNavigationContactType Type{EShipNavigationContactType::Unknown};
	double DistanceCentimeters{0.0};
	bool bVirtualContact{false};
	/** The pilot's own colony: always charted, with a marker of its own. */
	bool bOwnColony{false};
	/** Rio 04.10: a whole star system seen from well outside it, one card at its star; its worlds are folded into it. */
	bool bSystemSummary{false};
	/** Rio 04.10: a nearby star's label (the star-label mode, Y): its name, class, distance and what is known. */
	bool bStarLabel{false};
	/** The card's own colour (a star's spectral colour); transparent: the colour of its kind. */
	FLinearColor MarkerColour{FLinearColor::Transparent};

	/**
	 * Where the marker and the autopilot aim: an actor's place as the still ship sees it (Rio 06.10), or the fixed one.
	 */
	FVector GetWorldLocation() const;
};

/**
 * Reads navigation contacts directly from the persistent actor hierarchy and generated HISM star data.
 * It deliberately creates no world actors and keeps the visualization cost bounded.
 */
UCLASS(ClassGroup=(APS), meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UShipNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipNavigationComponent();

	void RefreshContacts(const FVector& ObserverLocation, bool bForce = false);
	void CycleTarget(int32 Direction);
	/** Selects the listed contact with this stable id (the colony terminal's course); false when it is not listed. */
	bool SelectContact(const FString& StableId);
	/** A course for any charted actor, a station or a settlement too: it stays listed, whatever the marker filters,
	 * until the pilot picks another target. False when no such actor is charted. */
	bool SetCourse(const FString& StableId);

	const TArray<FShipNavigationContact>& GetContacts() const { return Contacts; }
	const FShipNavigationContact* GetContact(int32 Index) const;
	const FShipNavigationContact* GetSelectedContact() const;
	int32 GetSelectedContactIndex() const { return SelectedContactIndex; }
	int32 GetDiscoveredContactCount() const { return DiscoveredContactCount; }

	static FString FormatDistance(double DistanceCentimeters);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation", meta=(ClampMin="0.1"))
	float RefreshInterval{0.75f};

	/** Maximum contacts retained after distance sorting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation", meta=(ClampMin="8", ClampMax="512"))
	int32 MaximumContacts{128};

	/** Generated HISM star markers are disabled until cluster identity/selection UX is finalized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation", meta=(ClampMin="0", ClampMax="512"))
	int32 MaximumVirtualStars{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowPlanetMarkers{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowMoonMarkers{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowStarMarkers{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowStationMarkers{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowInfrastructureMarkers{false};

	/** Rio 04.10 ("labels for the nearest stars, where each one is, its class"): the star-label mode, Y in the cockpit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers")
	bool bShowNearStarLabels{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation|Markers", meta=(ClampMin="1", ClampMax="64"))
	int32 NearStarLabelCount{12};

	/** True when a world location lies in a star system that shows as one card from here (its ships fold into it too). */
	bool IsInFoldedSystem(const FVector& WorldLocation) const;

private:
	void AddActorContact(AActor* Actor, const FVector& ObserverLocation);
	void AddGeneratedStarContacts(const FVector& ObserverLocation);
	/** Star systems seen from well outside become one card at their star (aps.Nav.SystemFold); KeepStableId stays apart. */
	void FoldDistantSystems(const FVector& ObserverLocation, const FString& KeepStableId);
	/** The nearest charted stars' labels (bShowNearStarLabels), but not those whose worlds are shown already. */
	void AddNearStarLabels(const FVector& ObserverLocation);
	void RestoreSelection(const FString& PreviousStableId);
	bool IsContactTypeVisible(EShipNavigationContactType Type) const;

	TArray<FShipNavigationContact> Contacts;
	/** The systems folded at the last refresh: their star (it moves with origin shifts) and the reach of their worlds. */
	struct FFoldedSystem
	{
		TWeakObjectPtr<AActor> Root;
		TWeakObjectPtr<AActor> Star;
		double RadiusCm{0.0};
	};
	TArray<FFoldedSystem> FoldedSystems;
	int32 SelectedContactIndex{INDEX_NONE};
	FString PinnedCourseId;
	/** The selected contact's id while a refresh runs: it stays listed whatever the filters (cycling never loses it). */
	FString KeepStableId;
	int32 DiscoveredContactCount{0};
	float RefreshElapsed{0.0f};
};

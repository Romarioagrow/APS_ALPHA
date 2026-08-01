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
	EShipNavigationContactType Type{EShipNavigationContactType::Unknown};
	double DistanceCentimeters{0.0};
	bool bVirtualContact{false};

	FVector GetWorldLocation() const
	{
		return Actor.IsValid() ? Actor->GetActorLocation() : FixedWorldLocation;
	}
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

	/** Maximum generated HISM stars sampled into the live contact list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Navigation", meta=(ClampMin="0", ClampMax="512"))
	int32 MaximumVirtualStars{96};

private:
	void AddActorContact(AActor* Actor, const FVector& ObserverLocation);
	void AddGeneratedStarContacts(const FVector& ObserverLocation);
	void RestoreSelection(const FString& PreviousStableId);

	TArray<FShipNavigationContact> Contacts;
	int32 SelectedContactIndex{INDEX_NONE};
	int32 DiscoveredContactCount{0};
	float RefreshElapsed{0.0f};
};

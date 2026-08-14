#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "APSCivilizationRuntimeManifest.generated.h"

class USpawnParameters;

UENUM(BlueprintType)
enum class EAPSCivilizationMaterializationState : uint8
{
	PendingPlacement,
	Materialized,
	LoadedFromSave,
	Blocked
};

UENUM(BlueprintType)
enum class EAPSCivilizationEntityRole : uint8
{
	BaseModule,
	LandingPad,
	SelectedShip
};

/** One stable physical entity owned by a generated civilization. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSCivilizationManifestEntity
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid StableId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid ParentStableId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid OwnerCivilizationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid FactionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	EAPSCivilizationEntityRole Role{EAPSCivilizationEntityRole::BaseModule};

	/** Exact authored class selected in the generation menu, or the native placeholder class. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FSoftClassPath ActorClass;

	/** Authoritative transform relative to the home body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FTransform PlanetRelativeTransform{FTransform::Identity};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	bool bHasPersistedTransform{false};

	FName MakeDeterministicActorName() const;
};

/** Versioned, deterministic runtime projection of the one canonical menu civilization. */
USTRUCT(BlueprintType)
struct APS_ALPHA_API FAPSCivilizationRuntimeManifest
{
	GENERATED_BODY()

	static constexpr int32 LatestSchemaVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	int32 SchemaVersion{LatestSchemaVersion};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid ManifestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid CivilizationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid FactionId;

	/** Persisted identity slot; display-name or policy edits never change the civilization ID. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	int32 CivilizationOrdinal{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	int32 WorldGenerationSeed{0};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FGuid HomeSystemId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FString HomeBodyKey;

	/** Canonical normalized menu payload; changing it does not mutate stable identity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FString CivilizationDescriptor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	EAPSCivilizationMaterializationState MaterializationState{
		EAPSCivilizationMaterializationState::PendingPlacement};

	/** True when the manifest explicitly selects documented native modular stand-ins. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	bool bUsesPlaceholderAssets{false};

	/** Requested/actual/reason text for later UI diagnostics; empty when no fallback is used. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FString FallbackReason;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	FSoftClassPath SelectedShipClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	TArray<FAPSCivilizationManifestEntity> Entities;

	bool IsStructurallyValid(FString* OutReason = nullptr) const;
	bool MigrateToLatest(FString* OutReason = nullptr);
	FAPSCivilizationManifestEntity* FindEntity(EAPSCivilizationEntityRole Role);
	const FAPSCivilizationManifestEntity* FindEntity(EAPSCivilizationEntityRole Role) const;
};

/** Pure factory: identical canonical inputs always produce identical IDs and entity ordering. */
struct APS_ALPHA_API FAPSCivilizationRuntimeManifestFactory
{
	static FGuid MakeStableId(const FString& Namespace, const FString& CanonicalValue);
	static FString BuildCivilizationDescriptor(const USpawnParameters* Parameters);
	static FAPSCivilizationRuntimeManifest Build(
		int32 WorldGenerationSeed,
		const FGuid& HomeSystemId,
		const FString& HomeBodyKey,
		const USpawnParameters* Parameters,
		const FSoftClassPath& BaseModuleClass,
		const FSoftClassPath& LandingPadClass,
		int32 CivilizationOrdinal = 0);
};

#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Production/APSProductionTypes.h"
#include "APS_ALPHA/Gameplay/Spawn/APSSpawnPlacementSubsystem.h"

/** The engine shape a part falls back to when its pack mesh is not installed (the pack is not in the repository). */
enum class EAPSColonyPartShape : uint8
{
	Cube,
	Cylinder,
	Sphere,
	Cone
};

/**
 * One visual piece of a module, fitted into a slot of the module frame (X front, toward the anchor; Z up from the
 * support plane). Size is in the part's own axes, before Rotation. A pack mesh keeps its proportions: it turns so its
 * long side follows the slot's, scales uniformly to fit, sits on the slot's floor and, when tiled, repeats along the
 * slot's long side. An engine shape fills the slot exactly.
 */
struct FAPSColonyModulePart
{
	const TCHAR* PreferredMesh{nullptr};
	/** Pack material for the engine shape (window glass on a panel); the colour below otherwise. */
	const TCHAR* PreferredMaterial{nullptr};
	EAPSColonyPartShape Shape{EAPSColonyPartShape::Cube};
	FVector Center{FVector::ZeroVector};
	FVector Size{100.0};
	FRotator Rotation{FRotator::ZeroRotator};
	FLinearColor Color{FLinearColor::White};
	/** Emissive glow for lamps and beacons (engine shape only); 0 keeps the lit surface. */
	float Glow{0.0f};
	bool bCollides{true};
	/** Repeat the fitted pack mesh along the slot's long side (glass walls). */
	bool bTile{false};
	/** Leave the slot empty when the pack's mesh or material is missing (glass that an opaque box would turn into a
	 * wall). */
	bool bSkipWithoutPack{false};
};

/** A light carried by a module: floodlights and beacon lamps. No shadows, to keep the frame cheap. */
struct FAPSColonyModuleLight
{
	FVector Location{FVector::ZeroVector};
	FRotator Rotation{FRotator::ZeroRotator};
	FLinearColor Color{FLinearColor::White};
	float IntensityCandelas{1000.0f};
	float RadiusCm{3000.0f};
	/** A spot light with this cone; 0 is a point light. */
	float ConeDegrees{0.0f};
};

/** One installable module (U1): its site, footprint, build time and look. */
struct FAPSColonyModuleSpec
{
	FName Id;
	EAPSSpawnSite Site{EAPSSpawnSite::Surface};
	FText Name;
	FText Description;
	/** Footprint X (front) and Y, height Z, centimetres. */
	FVector SizeCm{1000.0, 1000.0, 500.0};
	double BuildSeconds{10.0};
	/** Surface modules stand on a concrete plinth that reaches the ground under the whole footprint. */
	bool bFoundation{false};
	/** Turn the module's front toward the star (solar arrays), instead of toward the anchor. */
	bool bFaceSun{false};
	/**
	 * Rio 07-09.10, ORIGIN (T-06): what it feeds the civilization's stocks per minute while it stands, in
	 * APSInfrastructure::EResource order (metals, volatiles, energy, research, influence). FAPSInfrastructure counts the
	 * standing modules in the ladder (and with aps.Colony.ModuleYields elsewhere).
	 */
	float YieldPerMinute[5]{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	/** A token of the ladder it opens when built (APSProgressionTokens); NAME_None: none. */
	FName UnlocksToken;
	TArray<FAPSColonyModulePart> Parts;
	TArray<FAPSColonyModuleLight> Lights;
};

/** The modules a colony can build at its surface base and on its headquarters. */
struct APS_ALPHA_API FAPSColonyModuleCatalogue
{
	static const TArray<FAPSColonyModuleSpec>& Get();
	static const FAPSColonyModuleSpec* Find(FName ModuleId);
	/** Production identity: Buildable:<ModuleId>. */
	static FPrimaryAssetId MakeDefinitionId(FName ModuleId);
	static FName ModuleIdFromDefinition(const FPrimaryAssetId& DefinitionId);
	/** The Building definition registered with the production system; build time scaled for test runs. */
	static FAPSProductionDefinition MakeDefinition(const FAPSColonyModuleSpec& Spec, double TimeScale = 1.0);
	/** Production category per site: APS.Colony.Surface, APS.Colony.Orbit. */
	static FName SiteCategory(EAPSSpawnSite Site);
};

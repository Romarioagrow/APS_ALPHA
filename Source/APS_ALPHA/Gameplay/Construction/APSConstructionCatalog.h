#pragma once

#include "CoreMinimal.h"

class AActor;
class AStaticMeshActor;
class UClass;
class UMaterialInterface;
class UObject;
class UStaticMesh;
class UWorld;

/**
 * Build mode's own catalogue (Rio 02.10: "a separate menu where we pick what to spawn and drag it with the mouse ...
 * here we can place something smaller"): the small objects a player on foot places by hand around the colony. The
 * strategic structures stay in APSInfrastructureCatalog; build mode offers those of a world's surface beside these.
 *
 * An object is a set of parts fitted into slots of its own frame: X front, Y right, Z up from the ground, the origin on
 * the ground at its centre. A part prefers a pack mesh, which keeps its proportions and sits on its slot's floor; the
 * marketplace packs are not in the repository, so a missing one falls back to the engine shape that fills the slot.
 */
namespace APSConstruction
{
	enum class EShape : uint8
	{
		Cube,
		Cylinder,
		Sphere,
		Cone
	};

	struct FPart
	{
		/** Object path of the preferred pack mesh; null: the engine shape only. */
		const TCHAR* PackMesh{nullptr};
		EShape Shape{EShape::Cube};
		/** The slot: centre and size in the object's frame, cm, turned by Rotation about its centre. */
		FVector Center{FVector::ZeroVector};
		FVector Size{100.0};
		FRotator Rotation{FRotator::ZeroRotator};
		/** The engine shape's colour; a pack mesh keeps its own materials. */
		FLinearColor Color{FLinearColor::White};
		/** Emissive strength of an engine shape (lamps); 0 is a lit surface. */
		float Glow{0.0f};
		bool bCollides{true};
	};

	/** A point light the object carries (no shadows, to keep the frame cheap). */
	struct FLight
	{
		FVector Location{FVector::ZeroVector};
		FLinearColor Color{FLinearColor::White};
		float Candelas{500.0f};
		float RadiusCm{1500.0f};
	};

	struct FPropType
	{
		FName Id;
		FText Name;
		/** One line: what it is. */
		FText Role;
		TArray<FPart> Parts;
		TArray<FLight> Lights;
		/** The steepest ground it may stand on. */
		float MaxSlopeDegrees{30.0f};
		/** Tilts with the ground (at most 15 degrees) instead of standing straight along gravity. */
		bool bAlignToGround{true};
	};

	APS_ALPHA_API const TArray<FPropType>& Props();
	APS_ALPHA_API const FPropType* FindProp(FName Id);

	/** One mesh of an object: the mesh, its place in the object's frame, and its material (null: the mesh's own). */
	struct FMeshPiece
	{
		UStaticMesh* Mesh{nullptr};
		FTransform Transform{FTransform::Identity};
		UMaterialInterface* Material{nullptr};
		bool bCollides{true};
	};

	/**
	 * The meshes of a prop: its pack meshes where installed, engine shapes elsewhere. With MaterialOuter the engine shapes
	 * get their coloured materials (owned by it); without, their materials are left to the caller (the ghost tints them).
	 */
	APS_ALPHA_API void ResolveProp(const FPropType& Type, UObject* MaterialOuter, TArray<FMeshPiece>& OutPieces,
		int32* OutFallbacks = nullptr);
	/** The path a save keeps for a prop: its first part's mesh as resolved on this checkout. */
	APS_ALPHA_API FString PrimaryMeshPath(const FPropType& Type);
	/**
	 * The meshes an actor class shows, in its own frame (the native components of its default object, then its Blueprint
	 * components): a ghost for a structure before it is built. At most MaxPieces.
	 */
	APS_ALPHA_API void GatherClassPieces(UClass* Class, TArray<FMeshPiece>& OutPieces, int32 MaxPieces = 96);
	/** The bounds of pieces in the object's frame (unscaled). */
	APS_ALPHA_API FBox PiecesBounds(const TArray<FMeshPiece>& Pieces);

	/** Every prop placed by hand carries this tag (and APS.Construction.Prop.<Id>). */
	APS_ALPHA_API FName PlacedTag();
	APS_ALPHA_API FName PropIdOf(const AActor* Actor);

	/**
	 * A placed prop: a static mesh actor at WorldTransform with the prop's parts and lights, attached to the site (the body
	 * it stands on), tagged as placed. An id this catalogue no longer has spawns MeshPath alone (a save from another
	 * catalogue). Null when neither resolves.
	 */
	APS_ALPHA_API AStaticMeshActor* SpawnProp(UWorld* World, FName PropId, const FString& MeshPath,
		const FTransform& WorldTransform, AActor* Site);
}
